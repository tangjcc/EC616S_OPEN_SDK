/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: sntp.c
 *
 *  Description: sntp
 *
 *  History:creat by xwang
 *
 *  Notes:
 *
******************************************************************************/
#include "lwip/opt.h"


#include "lwip/def.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "posix/netdb.h"
#include "lwip/api.h"
#include "lwip/priv/api_msg.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include "netmgr.h"
#include "debug_log.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "sntp.h"



#define SNTP_THREAD_STACKSIZE 2048
#define SNTP_THREAD_PRIO 16

#define SNTP_ERR_CODE_KOD  1

/* SNTP defines */
#define SNTP_MSG_LENGTH                    48

#define SNTP_LI_NO_DEFINE                  0x00

#define SNTP_VERSION_DEFINE                (4<<3)


#define SNTP_MODE_MASK_DEFINE              0x07
#define SNTP_CLIENT_MODE                   0x03
#define SNTP_SERVER_MODE                   0x04
#define SNTP_BROADCAST_MODE                0x05

#define SNTP_STRATUM_KOD_DEFINE            0x00

#define SNTP_1900_1970_DIFF_SEC         (2208988800UL)
#define SNTP_1970_2036_DIFF_SEC         (2085978496UL)

#define SNTP_GET_ORIGINAL_SYSTEM_TIME(sec, us)     do { (sec) = 0; (us) = 0; } while(0)


#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct SntpMsg {
  PACK_STRUCT_FLD_8(UINT8      liVnMode);
  PACK_STRUCT_FLD_8(UINT8 stratum);
  PACK_STRUCT_FLD_8(UINT8      poll);
  PACK_STRUCT_FLD_8(UINT8      precision);
  PACK_STRUCT_FIELD(UINT32 rootDelay);
  PACK_STRUCT_FIELD(UINT32 rootDispersion);
  PACK_STRUCT_FIELD(UINT32 referenceIdentifier);
  PACK_STRUCT_FIELD(UINT32 referenceTimeStamp[2]);
  PACK_STRUCT_FIELD(UINT32 originateTimeStamp[2]);
  PACK_STRUCT_FIELD(UINT32 receiveTimeStamp[2]);
  PACK_STRUCT_FIELD(UINT32 transmitTimeStamp[2]);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/epstruct.h"
#endif



typedef struct SntpConfig_Tag{
    UINT16  serverPort;
    UINT16  reqHandler;
    UINT16  autoSync;
    UINT16  rsvd;
    CHAR serverAddr[DNS_MAX_NAME_LENGTH];
}SntpConfig;


static BOOL   sntpIsOngoing = FALSE;
static sys_thread_t sntpThread = NULL;
static BOOL  sntpTerminalFlag = FALSE;
static SntpConfig gSntpConfig;

static UINT32 sntpLastTimeStampSent[2];


void SntpCallResultCallback(UINT32 result, UINT32 seconds, UINT32 us)
{
    NmAtiSntpResultInt resultInd;
    
    memset(&resultInd, 0, sizeof(NmAtiSntpResultInt));


    resultInd.result = result;
    resultInd.autoSync = gSntpConfig.autoSync;
    resultInd.time = seconds;
    resultInd.us = us;

    NetMgrSendSntpResultInd(&resultInd, gSntpConfig.reqHandler);

    ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpCallResultCallback_1, P_INFO, 1, "SntpCallResultCallback result %u", result);

}


BOOL SntpRecvResponse(struct SntpMsg *response, UINT32 *seconds, UINT32 *useconds)
{
    UINT8 mode;
    UINT8 stratum;
    UINT32 rxSecs;
    int is1900Based;

    if(response == NULL || seconds == NULL || useconds == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpRecvResponse_1, P_WARNING, 0, "SntpRecvResponse: invalid parameter");
        return FALSE;
    }

    //init seconds and useconds
    *seconds = 0;
    *useconds = 0;
    
    //parse the sntp response message
    mode = response->liVnMode;
    mode &= SNTP_MODE_MASK_DEFINE;
    
    /* if this is a SNTP response... */
    if (mode == SNTP_SERVER_MODE || mode == SNTP_BROADCAST_MODE)
    {

        stratum = response->stratum;
        
        if (stratum == SNTP_STRATUM_KOD_DEFINE)
        {
            ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpRecvResponse_2, P_WARNING, 0, "SntpRecvResponse: Received Kiss-of-Death");        
        }
        else
        {
          /* check originateTimeStamp*/
            if ((response->originateTimeStamp[0] != sntpLastTimeStampSent[0]) ||
                (response->originateTimeStamp[1] != sntpLastTimeStampSent[1]))
            {
                ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpRecvResponse_3, P_WARNING, 0,
                        "SntpRecvResponse: Invalid originate timestamp in response"); 

            }
            else
            {
               /* correct response */
                rxSecs = ntohl(response->receiveTimeStamp[0]);
                is1900Based = ((rxSecs & 0x80000000) != 0);
                *seconds = is1900Based ? (rxSecs - SNTP_1900_1970_DIFF_SEC) : (rxSecs + SNTP_1970_2036_DIFF_SEC);
                *useconds = ntohl(response->receiveTimeStamp[1]) / 4295;
                ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpRecvResponse_4, P_INFO, 2,
                        "SntpRecvResponse: correct response seconds %u,useconds %u", *seconds, *useconds);
                return TRUE;
            }
       }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, sntp_recv_3, P_WARNING, 1,
            "sntp_recv: Invalid mode in response: %u", mode); 

    }

    return FALSE;
}


BOOL SntpSendRequest(INT32 sockFd, ip_addr_t *serverAddr, UINT16 ServerPort)
{
    struct sockaddr_storage to;
    INT32 ret;
    struct SntpMsg sntpReq;
    UINT32 sntpTimeSec;
    UINT32 sntpTimeUs;    

    //check paramter
    if(sockFd < 0 || serverAddr == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpSendRequest_1, P_ERROR, 0, "SntpSendRequest parameter invalid");
        return FALSE;
    }

    if (IP_IS_V4(serverAddr))
    {
        struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
        to4->sin_len    = sizeof(struct sockaddr_in);
        to4->sin_family = AF_INET;
        to4->sin_port = htons(ServerPort);
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(serverAddr));
    }
    else if(IP_IS_V6(serverAddr))
    {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6*)&to;
        to6->sin6_len    = sizeof(struct sockaddr_in6);
        to6->sin6_family = AF_INET6;
        to6->sin6_port = htons(ServerPort);
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(serverAddr));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpSendRequest_2, P_ERROR, 0, "SntpSendRequest parameter invalid");
        return FALSE;
    }

    //prepare sntp request message

    memset(&sntpReq, 0, SNTP_MSG_LENGTH);
    sntpReq.liVnMode = SNTP_LI_NO_DEFINE | SNTP_VERSION_DEFINE | SNTP_CLIENT_MODE;
  
    SNTP_GET_ORIGINAL_SYSTEM_TIME(sntpTimeSec, sntpTimeUs);
    sntpLastTimeStampSent[0] = htonl(sntpTimeSec + SNTP_1900_1970_DIFF_SEC);
    sntpReq.transmitTimeStamp[0] = sntpLastTimeStampSent[0];
    sntpLastTimeStampSent[1] = htonl(sntpTimeUs);
    sntpReq.transmitTimeStamp[1] = sntpLastTimeStampSent[1];

    //send sntp client reqeust
    ret = sendto(sockFd, &sntpReq, SNTP_MSG_LENGTH, 0, (struct sockaddr*)&to, sizeof(to));
    if(ret == SNTP_MSG_LENGTH)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpSendRequest_4, P_INFO, 0, "SntpSendRequest send sntp request success");
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpSendRequest_5, P_ERROR, 0, "SntpSendRequest send sntp request fail");       
    }

    return FALSE;    
    
}


static void SntpTask(void *arg)
{
    INT32 ret;
    UINT32 result = SNTP_RESULT_FAIL;
    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 sntpPmuHdr = 0;
    ip_addr_t sntpServerAddr;
    INT32 sockFd = -1;
    UINT8 retryTimes = 0;
    UINT32 secondsResponse;
    UINT32 usecondsResponse;
    struct SntpMsg sntpResponse;

    struct addrinfo hints, *target_address = PNULL;

    SntpConfig *sntpParam = PNULL;

    sntpParam = (SntpConfig*)arg;


    //check sntp parameter
    if (!sntpParam) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_1, P_ERROR, 0, "SntpTask start error, as sntp parameter invalid");
        sntpIsOngoing = FALSE;
        sntpThread = NULL;
        sntpTerminalFlag = FALSE;
        SntpCallResultCallback(SNTP_RESULT_PARAMTER_INVALID, 0, 0);
        osThreadExit();    
    }


    /*
     * start sntp, sntp task don't allow to enter HIB/Sleep2 state before ping terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("sntp", &sntpPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(sntpPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    //get ping target address if target is url type
    if(ipaddr_aton((const char*)sntpParam->serverAddr, &sntpServerAddr) == 0)
    {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        //    hints.ai_socktype = SOCK_DGRAM;
        //    hints.ai_protocol = IPPROTO_UDP;
        if (getaddrinfo(sntpParam->serverAddr, NULL, &hints, &target_address) != 0) 
        {
            //      OsaPrintf(P_ERROR, "Ping thread start error, can not get ip from url");
            ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_2, P_ERROR, 0, "SntpTask start error, can not get ip from url");

            result = SNTP_RESULT_URL_RESOLVE_FAIL;

            goto failAction;             
        }
        else 
        {
            if (target_address->ai_family == AF_INET) 
            {
                ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_3, P_SIG, 0, "SntpTaskresolve url success, it is ipv4 type");
                struct sockaddr_in *to = (struct sockaddr_in *)target_address->ai_addr;
                IP_SET_TYPE(&sntpServerAddr, IPADDR_TYPE_V4);
                inet_addr_to_ip4addr(ip_2_ip4(&sntpServerAddr), &to->sin_addr);
            }
            else if (target_address->ai_family == AF_INET6) 
            {
                ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_4, P_SIG, 0, "SntpTask resolve url success, it is ipv6 type");
                struct sockaddr_in6 *to = (struct sockaddr_in6 *)target_address->ai_addr;
                IP_SET_TYPE(&sntpServerAddr, IPADDR_TYPE_V6);
                inet6_addr_to_ip6addr(ip_2_ip6(&sntpServerAddr),&to->sin6_addr);
            } 
            else 
            {
                ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_5, P_ERROR, 0, "SntpTask start error, get ip from url is invalid");

                freeaddrinfo(target_address);

                result = SNTP_RESULT_URL_RESOLVE_FAIL;

                goto failAction;      

            }
            freeaddrinfo(target_address);
        }
    }

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int timeout = SNTP_RCV_TIMEOUT;
#else
    struct timeval timeout;
    timeout.tv_sec = SNTP_RCV_TIMEOUT;
    timeout.tv_usec = 0;
#endif

    if (IP_IS_V4(&sntpServerAddr)) 
    {
        sockFd = lwip_socket(AF_INET, SOCK_DGRAM, IP_PROTO_UDP);
    } 
    else 
    {
        sockFd = lwip_socket(AF_INET6, SOCK_DGRAM, IP_PROTO_UDP);
    }

    if (sockFd < 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_6, P_ERROR, 0, "SntpTask socket create error");

        result = SNTP_RESULT_CREATE_CLIENT_SOCK_FAIL;

        goto failAction;        
    }

    ret = lwip_setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (ret != 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_7, P_ERROR, 0, "SntpTask setting receive timeout failed");

        result = SNTP_RESULT_CREATE_CLIENT_SOCK_FAIL;

        goto failAction;
    }

    LWIP_UNUSED_ARG(ret);

    while ((retryTimes < SNTP_RETRY_TIMES) && (sntpTerminalFlag == FALSE)) 
    {

        if (SntpSendRequest(sockFd, &sntpServerAddr, sntpParam->serverPort) == TRUE) 
        {
            
            ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_8, P_SIG, 0, "SntpTask: send request success");

            memset(&sntpResponse, 0, sizeof(struct SntpMsg));
            ret = recv(sockFd, &sntpResponse, SNTP_MSG_LENGTH, 0);
            if(ret == SNTP_MSG_LENGTH)
            {
              
                if(SntpRecvResponse(&sntpResponse, &secondsResponse, &usecondsResponse) == TRUE)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_9, P_INFO, 0, "SntpTask: rcv response success");
                    result = SNTP_RESULT_OK;
                    break;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_10, P_WARNING, 0, "SntpTask: rcv response, but process fail");                    
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_11, P_WARNING, 0, "SntpTask: rcv response fail");
            }

         }
         else // send request fail, try again
         {
            ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_12, P_WARNING, 0, "SntpTask: send fail, need rty again");
            sys_msleep(SNTP_RESEND_REQUEST_DELAY);

        } 

        retryTimes ++;
        
    }

    if(retryTimes >= SNTP_RETRY_TIMES)
    {
        result = SNTP_RESULT_RETRY_REACH_MAX_TIMES;
    }

    ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpTask_13, P_SIG, 0, "sntp task terminal");


failAction:

    sntpIsOngoing = FALSE;
    sntpThread = NULL;
    sntpTerminalFlag = FALSE;
    if(sockFd >= 0)
    {
        lwip_close(sockFd);
    }

    if(result != SNTP_RESULT_OK)
    {
        SntpCallResultCallback(result, 0, 0);
    }
    else
    {
        SntpCallResultCallback(SNTP_RESULT_OK, secondsResponse, usecondsResponse);
    }

    /* before ping task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(sntpPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(sntpPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    osThreadExit();  
}

void SntpTerminat(void)
{
    if(sntpThread)
    {
        sntpTerminalFlag = TRUE;
    }
    return;
}


BOOL SntpInit(CHAR* serverAddr, UINT16 serverPort, UINT16 reqHandler, BOOL autoSync)
{
    /* check the input */
    if(!serverAddr || strlen(serverAddr) > DNS_MAX_NAME_LENGTH || strlen(serverAddr) == 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpInit_1, P_ERROR, 0, 
                   "ERROR, SntpInit PARAMETER invalid");
        return FALSE;
    }

    if (sntpIsOngoing) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_SNTP, SntpInit_2, P_ERROR, 0, 
                   "ERROR, SntpInit has run");

        return FALSE;
    }

    /* if set it in "sntp" task, maybe modify gSntpConfig for mutiple sntp request */
    sntpIsOngoing = TRUE;

    gSntpConfig.serverPort = serverPort?serverPort:SNTP_DEFAULT_PORT;
    gSntpConfig.reqHandler = reqHandler;
    gSntpConfig.autoSync = (UINT16)autoSync;

    //ping target setting
    strcpy((char *)gSntpConfig.serverAddr, serverAddr);

    sntpThread = sys_thread_new("sntp_task", SntpTask, (void*)&gSntpConfig, SNTP_THREAD_STACKSIZE, SNTP_THREAD_PRIO);

    if (sntpThread == NULL)
    {
        sntpIsOngoing = FALSE;
        return FALSE;
    }

    return TRUE;
}

