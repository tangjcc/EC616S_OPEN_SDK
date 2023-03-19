#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include <stdlib.h>
#include "sys/socket.h"
#include "posix/errno.h"
#include "lwip/inet.h"
#include "iperf_api.h"
#include "iperf_config.h"
#include "debug_trace.h"
#include "debug_log.h"

#include "psdial.h"
#include "netmgr.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif




/***************************************************************************************
 *                      Macros
 ****************************************************************************************/




static BOOL bIperfCltIsGoingOn = 0;
static BOOL bIperfSrvIsGoingOn = 0;
static osThreadId_t iperfCLtTask = NULL;
static osThreadId_t iperfSrvTask = NULL;
static BOOL bIperfCltTerminRunning = 0;
static BOOL bIperfSrvTerminRunning = 0;
static NmIperfReq pIperfClientReq;
static NmIperfReq pIperfServerReq;

/***************************************************************************************
 * structure
****************************************************************************************/


typedef struct IperfCount_Tag
{
    UINT32 Bytes;
    UINT32 KBytes;
    UINT32 MBytes;
    UINT32 GBytes;
    UINT32 times;
} IperfCount;


typedef struct IperfUdpDatagram_Tag
{
    INT32 pkgId;
    UINT32 sec;
    UINT32 uSec;
} IperfUdpDatagram;


typedef struct IperfClientHeader_Tag
{

    INT32 flags;
    INT32 numChannels;
    INT32 portClient;
    INT32 buffLen;
    INT32 band;
    INT32 amounts;
} IperfClientHeader;


/****************************************************************************************
 *               Function Declarations
 ****************************************************************************************/

IperfCount IperfCalculateResult( INT32 pktSize, IperfCount pktCount, INT32 needToConvert );
IperfCount IperfCalculateAddHeader( INT32 domain, INT32 protocol, IperfCount pktCount );
IperfCount IperfResetCount( IperfCount pktCount );
IperfCount IperfCopyCount( IperfCount pktCount, IperfCount tmpCount );
IperfCount IperfDiffCount( IperfCount pktCount, IperfCount tmpCount );
/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
*             utility API
******************************************************/

void IperfGetCurrentTime(UINT32 *s, UINT32 *ms) {
    UINT32 currentMs = 0;

    if(!s && !ms) {
        return;
    }

    currentMs = osKernelGetTickCount()/portTICK_PERIOD_MS;

    if(s) {
        *s = currentMs/1000;
    }

    if(ms) {
        *ms = currentMs;
    }
}

void IperfmSleep( UINT32 ms ) {
    osDelay(ms/portTICK_PERIOD_MS);
}

INT32 IperfSetupUdpServerSocket(INT32 *fd, INT32 serverPort, UINT32 rcvTimeout, ip_addr_t *serverAddr, BOOL bNat)
{
    struct sockaddr_in servaddr;
    struct sockaddr_in6 servaddr6;

    if(!fd)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_1, P_WARNING, 0, "IperfSetupClientSocket input fd point is invalid");
        return NM_IPERF_PARAM_ERROR;
    }

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    UINT32 timeout = rcvTimeout;
#else
    struct timeval timeout;
    timeout.tv_sec = rcvTimeout;
    timeout.tv_usec = 0;
#endif

    if(serverAddr && (IP_IS_V4(serverAddr) && !ip_addr_isany(serverAddr)))
    {

        // Create a new UDP connection handle
        if( (*fd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 )
        {
        
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_2, P_WARNING, 0, "create udp server socket fail");
            return NM_IPERF_SOCKET_ERROR;
        }

        // Bind to port and any IP address
        memset( &servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;

        if(bNat == FALSE)
        {
            inet_addr_from_ip4addr(&servaddr.sin_addr, ip_2_ip4(serverAddr));
        }
        servaddr.sin_port = htons( serverPort );

        if( (bind( *fd, (struct sockaddr *) &servaddr, sizeof(servaddr) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_3, P_WARNING, 0, "udp server socket bind fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }
        
    }
    else if(serverAddr && (IP_IS_V6(serverAddr) && !ip_addr_isany(serverAddr)))
    {

        // Create a new UDP connection handle
        if( (*fd = socket( AF_INET6, SOCK_DGRAM, 0 )) < 0 )
        {
        
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_4, P_WARNING, 0, "create udp server socket fail");
            return NM_IPERF_SOCKET_ERROR;
        }

          // Bind to port and IP
        memset( &servaddr6, 0, sizeof(servaddr6) );
        servaddr6.sin6_family = AF_INET6;

        if(bNat == FALSE)
        {        
            inet6_addr_from_ip6addr(&servaddr6.sin6_addr, ip_2_ip6(serverAddr));
        }
        servaddr6.sin6_port = htons( serverPort );

        if( (bind( *fd, (struct sockaddr *) &servaddr6, sizeof(servaddr6) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_5, P_WARNING, 0, "udp server socket bind fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }        
    }
    else
    {

        // Create a new UDP connection handle
        if( (*fd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 )
        {
        
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_6, P_WARNING, 0, "create udp server socket fail");
            return NM_IPERF_SOCKET_ERROR;
        }

        // Bind to port and any IP address
        memset( &servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons( serverPort );

        if( (bind( *fd, (struct sockaddr *) &servaddr, sizeof(servaddr) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_7, P_WARNING, 0, "udp server socket bind fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }
        
    }

    socklen_t len = sizeof(timeout);
    if( setsockopt( *fd, SOL_SOCKET, SO_RCVTIMEO, (CHAR *) &timeout, len ) < 0 )
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_8, P_WARNING, 0, "Setsockopt failed - cancel receive timeout");
        close(*fd);
        return NM_IPERF_SOCKET_ERROR;
    }

    return 0;

}


INT32 IperfSetupTcpServerSocket(INT32 *fd, INT32 serverPort, UINT32 rcvTimeout, ip_addr_t *serverAddr)
{
    struct sockaddr_in servaddr;
    struct sockaddr_in6 servaddr6;    

    if(!fd)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_1, P_WARNING, 0, "IperfSetupTcpServerSocket input fd point is invalid");
        return NM_IPERF_PARAM_ERROR;
    }

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    UINT32 timeout = rcvTimeout;
#else
    struct timeval timeout;
    timeout.tv_sec = rcvTimeout;
    timeout.tv_usec = 0;
#endif

    if(serverAddr && (IP_IS_V4(serverAddr) && !ip_addr_isany(serverAddr)))
    {

        // Create a new TCP connection handle
        if( (*fd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
        {
        
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_3, P_WARNING, 0, "create tcp server socket fail");
            return NM_IPERF_SOCKET_ERROR;
        }

        // Bind to port and any IP address
        memset( &servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;
        inet_addr_from_ip4addr(&servaddr.sin_addr, ip_2_ip4(serverAddr));
        servaddr.sin_port = htons( serverPort );

        if( (bind( *fd, (struct sockaddr *) &servaddr, sizeof(servaddr) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_4, P_WARNING, 0, "tcp server socket bind fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }
        
    }
    else if(serverAddr && (IP_IS_V6(serverAddr) && !ip_addr_isany(serverAddr)))
    {

        // Create a new TCP connection handle
        if( (*fd = socket( AF_INET6, SOCK_STREAM, 0 )) < 0 )
        {
        
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_5, P_WARNING, 0, "create tcp server socket fail");
            return NM_IPERF_SOCKET_ERROR;
        }

          // Bind to port and IP
        memset( &servaddr6, 0, sizeof(servaddr6) );
        servaddr6.sin6_family = AF_INET6;
        inet6_addr_from_ip6addr(&servaddr6.sin6_addr, ip_2_ip6(serverAddr));
        servaddr6.sin6_port = htons( serverPort );

        if( (bind( *fd, (struct sockaddr *) &servaddr6, sizeof(servaddr6) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_6, P_WARNING, 0, "tcp server socket bind fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }        
    }
    else
    {

        // Create a new TCP connection handle
        if( (*fd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
        {
        
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_7, P_WARNING, 0, "create tcp server socket fail");
            return NM_IPERF_SOCKET_ERROR;
        }

        // Bind to port and any IP address
        memset( &servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons( serverPort );

        if( (bind( *fd, (struct sockaddr *) &servaddr, sizeof(servaddr) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupUdpServerSocket_7, P_WARNING, 0, "udp server socket bind fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }
        
    }   

    socklen_t len = sizeof(timeout);
    if( setsockopt( *fd, SOL_SOCKET, SO_RCVTIMEO, (CHAR *) &timeout, len ) < 0 )
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_8, P_WARNING, 0, "Setsockopt failed - cancel receive timeout");
        close(*fd);
        return NM_IPERF_SOCKET_ERROR;
    }

    // Put the connection into LISTEN state
    if( (listen( *fd, IPERF_SERVER_MAX_LISTEN_NUM )) < 0 )
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupTcpServerSocket_9, P_WARNING, 0, "iperf tcp server socket listen fail");
        close(*fd);
        return NM_IPERF_SOCKET_ERROR;
    }

    return 0;


}

INT32 IperfSetupClientSocket(INT32 *fd, INT32 protocol, INT32 destPort, ip_addr_t *destAddr)
{

    if(!fd)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_1, P_WARNING, 0, "IperfSetupClientSocket input fd point is invalid");
        return NM_IPERF_PARAM_ERROR;
    }

    if(destAddr && (IP_IS_V4(destAddr) && !ip_addr_isany(destAddr)))
    {
        struct sockaddr_in servaddr;

        // Create a new socket
        if(protocol == IPERF_STREAM_PROTOCOL_TCP)
        {
            if( (*fd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_2, P_WARNING, 0, "iperf tcp client create ipv4 tcp socket fail");
                return NM_IPERF_SOCKET_ERROR;
            }
        }
        else if(protocol == IPERF_STREAM_PROTOCOL_UDP)
        {
            if( (*fd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 )
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_3, P_WARNING, 0, "iperf tcp client create ipv4 udp socket fail");
                return NM_IPERF_SOCKET_ERROR;
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_4, P_WARNING, 0, "IperfSetupClientSocket paramter invalid");
            return NM_IPERF_PARAM_ERROR;
        }

          // Bind to port and IP
        memset( &servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;
        inet_addr_from_ip4addr(&servaddr.sin_addr, ip_2_ip4(destAddr));
        servaddr.sin_port = htons( destPort );

        if( (connect( *fd, (struct sockaddr *) &servaddr, sizeof(servaddr) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_5, P_WARNING, 0, "iperf tcp client socket connect fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;
        }

    }//ipv6 addr case
    else if(destAddr && (IP_IS_V6(destAddr) && !ip_addr_isany(destAddr)))
    {
        struct sockaddr_in6 servaddr6;

        // Create a new socket
        if(protocol == IPERF_STREAM_PROTOCOL_TCP)
        {
            if( (*fd = socket( AF_INET6, SOCK_STREAM, 0 )) < 0 )
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_6, P_WARNING, 0, "iperf tcp client create ipv6 tcp socket fail");
                return NM_IPERF_SOCKET_ERROR;

            }
        }
        else if(protocol == IPERF_STREAM_PROTOCOL_UDP)
        {
            if( (*fd = socket( AF_INET6, SOCK_DGRAM, 0 )) < 0 )
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_7, P_WARNING, 0, "iperf tcp client create ipv6 udp socket fail");
                return NM_IPERF_SOCKET_ERROR;

            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_8, P_WARNING, 0, "IperfSetupClientSocket paramter invalid");
            return NM_IPERF_PARAM_ERROR;
        }



          // Bind to port and IP
        memset( &servaddr6, 0, sizeof(servaddr6) );
        servaddr6.sin6_family = AF_INET6;
        inet6_addr_from_ip6addr(&servaddr6.sin6_addr, ip_2_ip6(destAddr));
        servaddr6.sin6_port = htons( destPort );

        if( (connect( *fd, (struct sockaddr *) &servaddr6, sizeof(servaddr6) )) < 0 )
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_9, P_WARNING, 0, "iperf tcp client socket connect fail");
            close(*fd);
            return NM_IPERF_SOCKET_ERROR;

        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfSetupClientSocket_10, P_WARNING, 0, "IperfSetupClientSocket paramter invalid");
        return NM_IPERF_PARAM_ERROR;
    }

    return 0;


}

INT32 IperfNatServerSendUdpPkg(INT32 sockfd, INT32 destPort, ip_addr_t *destAddr)
{
    struct sockaddr_in cliaddr;     //16 bytes
    INT32 cliLen;
    IperfUdpDatagram    *udpHdr;
    IperfClientHeader      *clientHdr;
    CHAR            pBuf[IPERF_UDP_NAT_SERVER_SEND_PKT_LEN];
    INT32             ret = -1;

    OsaCheck(destAddr != NULL && sockfd >= 0, sockfd, destPort, destAddr);

    if(!IP_IS_V4(destAddr) || ip_addr_isany(destAddr))
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfNatServerSendUdpPkg_1, P_WARNING, 0,
                "IperfNatServerSendUdpPkg dst addr invalid");
        return ret;
    }


    memset(pBuf, 0x00, IPERF_UDP_NAT_SERVER_SEND_PKT_LEN);

    cliLen = sizeof(cliaddr);

    memset(&cliaddr, 0, sizeof(cliaddr) );
    cliaddr.sin_family = AF_INET;

    if (destPort == 0 )
    {
        destPort = IPERF_DEFAULT_PORT;
    }

    cliaddr.sin_port = htons(destPort);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfNatServerSendUdpPkg_2, P_INFO, 1,
                "Iperf, send one UDP pkg to dest port: %d", destPort);

    inet_addr_from_ip4addr(&cliaddr.sin_addr, ip_2_ip4(destAddr));

    // Init UDP data header
    udpHdr = (IperfUdpDatagram *)&pBuf[0];       //12 bytes
    udpHdr->pkgId = 0;    //htonl(udpHdrId);
    udpHdr->sec = htonl(200);
    udpHdr->uSec = htonl(0);
    clientHdr = (IperfClientHeader *) &pBuf[12];    //24bytes
    clientHdr->flags = 0;
    clientHdr->numChannels = htonl(1);
    clientHdr->portClient = htonl(destPort);
    clientHdr->buffLen = 0;
    clientHdr->band = htonl(22000);
    clientHdr->amounts = htonl( ~(long )(1000 * 100));

    ret = sendto(sockfd, pBuf, IPERF_UDP_NAT_SERVER_SEND_PKT_LEN, 0, (struct sockaddr *)&cliaddr, cliLen);

    return ret;
}



/******************************************************
 *               Function Definitions
 ******************************************************/
/* members are in network byte order */
void IperfUdpRunNatServer( void* arg )
{

    NmIperfReq * serverParam = (NmIperfReq *)arg;
    INT32 sockfd;
    struct sockaddr_in cliaddr;
    INT32 cliLen;
    IperfCount pktCount = {0, 0, 0, 0, 0};
    IperfCount tmpCount = {0, 0, 0, 0, 0};
    INT32 nBytes = 0;
    UINT32 intervalTag;
    INT32 serverPort = 0;
    UINT16 rcvTime;
    UINT32 totalRptNum = 1;
    UINT32 tmpRptNum = 0;    
    UINT32 rcvTimeout;
    INT32 ret;

    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 iperfPmuHdr = 0;

    UINT32 t1, t2, currTime, offsetTime1;
    IperfUdpDatagram *udpHdr;
    IperfClientHeader *clientHdr;
    IperfClientHeader clientHdrTrans;

    INT32 isTestStarted = 0;
    INT32 udpHdrId = 0;

    struct NmAtiIperfResultIndTag nmIperfResult;
    memset(&nmIperfResult, 0, sizeof(struct NmAtiIperfResultIndTag));

    //init setting
    if(serverParam->rptIntervalPresent && serverParam->rptIntervalS > 0)
    {
        intervalTag = serverParam->rptIntervalS;
    }
    else
    {
        intervalTag = IPERF_DEFAULT_REPORT_INTERVAL;
    }

    if(serverParam->durationPresent && serverParam->durationS > 0)
    {
        rcvTime = serverParam->durationS;
    }
    else
    {
        rcvTime = IPERF_MAX_SEND_RCV_TIME;
    }

    if(serverParam->port > 0)
    {
        serverPort = serverParam->port;
    }
    else
    {
        serverPort = IPERF_DEFAULT_PORT;
    }

    if (intervalTag < IPERF_SERVER_RECEIVE_TIMEOUT)
    {
        rcvTimeout = intervalTag;
    }
    else
    {
        rcvTimeout = IPERF_SERVER_RECEIVE_TIMEOUT;
    }

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_1, P_SIG, 0, "iperf udp nat server task start runing");

    //alloc rcv buffer
    CHAR *buffer = (CHAR*) malloc( IPERF_TEST_BUFFER_SIZE );
    if(!buffer)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_2, P_WARNING, 0, "malloc buffer fail");
        nmIperfResult.mode = NM_IPERF_MODE_SERVER;
        nmIperfResult.result = NM_IPERF_MALLOC_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

        iperfSrvTask = NULL;
        bIperfSrvIsGoingOn = 0;
        osThreadExit();
    }
    memset(buffer, 0 ,IPERF_TEST_BUFFER_SIZE);

    //setup udp nat server socket
    ret = IperfSetupUdpServerSocket(&sockfd, serverPort, rcvTimeout, &serverParam->destAddr, TRUE);
    if (ret != 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_3, P_WARNING, 0, "setup udp nat server socket fail");
        nmIperfResult.mode = NM_IPERF_MODE_SERVER;
        nmIperfResult.result = ret;
        NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

        iperfSrvTask = NULL;
        bIperfSrvIsGoingOn = 0;
        free(buffer);
        osThreadExit();
    }

    cliLen = sizeof(cliaddr);

    /*
    * start iperf, iperf task don't allow to enter HIB/Sleep2 state before iperf terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("iperfserver", &iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    //fisrt send one UL pkg for NAT server
    if(IperfNatServerSendUdpPkg(sockfd, serverPort, &serverParam->destAddr) < 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_11, P_WARNING, 0, "send one packet to client for NAT case fail");
    }

    IperfGetCurrentTime( &offsetTime1, 0 );

    // Wait and check the request
    do {
        // Handles request
        do {

            //check cfun status first
            if(!psDailBeCfun1State())
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_4, P_WARNING, 0, "Not CFUN1, iperf will terminal");
                break;
            }

            nBytes = recvfrom(sockfd, buffer, IPERF_TEST_BUFFER_SIZE, 0, (struct sockaddr *) &cliaddr, (socklen_t *) &cliLen );


            udpHdr = (IperfUdpDatagram *) buffer;
            udpHdrId = (INT32) ntohl( udpHdr->pkgId );

            pktCount = IperfCalculateResult( nBytes, pktCount, 0 );

            if(nBytes > 0)
            {
               if(IP_IS_V4(&serverParam->destAddr))
                {
                    pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP4, IPERF_STREAM_PROTOCOL_UDP, pktCount);
                }
                else
                {
                    pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP6, IPERF_STREAM_PROTOCOL_UDP, pktCount);
                }
            }

            //record the time when  rcv the first packet
            if( pktCount.times == 1 )
            {
                IperfGetCurrentTime( &t1, 0 );
            }

            IperfGetCurrentTime( &currTime, 0 );

            // Report every interval time
            if( intervalTag > 0 )
            {
                if( currTime - offsetTime1 > 0 )
                {
                    tmpRptNum = (((int)(currTime - offsetTime1)) / intervalTag);
                    if(tmpRptNum > 0)
                    {
                        if( tmpRptNum == totalRptNum)
                        {
            
                            nmIperfResult.mode = NM_IPERF_MODE_SERVER;
                            nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                            nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                            nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/intervalTag);
            
                            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_report_1, P_SIG, 2, "iperf UDP nat server report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);
            
                            NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);
            
                            tmpCount = IperfCopyCount( pktCount, tmpCount );
            
                            totalRptNum = tmpRptNum + 1;
                        }
                        else if(tmpRptNum > totalRptNum)
                        {
                            nmIperfResult.mode = NM_IPERF_MODE_SERVER;
                            nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                            nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                            nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/(intervalTag *(tmpRptNum - totalRptNum + 1)));
            
                            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_report_2, P_SIG, 2, "iperf UDP nat server report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);
            
                            NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);
            
                            tmpCount = IperfCopyCount( pktCount, tmpCount );
            
                            totalRptNum = tmpRptNum + 1;
            
                        }
                    }
                } 

            }

            //rcv the fisrt valid udp packet
            if( (isTestStarted == 0) && (udpHdrId > 0) && (nBytes > 0) )
            {
                isTestStarted = 1;
            }//rcv the end udp packet
            else if ( (udpHdrId < 0)  && (isTestStarted == 1) )
            { // the last package
                INT32 oldFlag = 0;

                // print out result
                //TODO: need to send the correct report to client-side, flag = 0 means the report is ignored.
                //if( udpHdrId < 0 )
                //{
                oldFlag = clientHdrTrans.flags;
                clientHdrTrans.flags = (INT32) 0;

                // send the server report to client-side
                sendto( sockfd, buffer, nBytes, 0, (struct sockaddr *) &cliaddr, cliLen );
                clientHdrTrans.flags = oldFlag;
                //}

                clientHdr = (IperfClientHeader *) &buffer[12];
                clientHdrTrans.flags = (INT32) (ntohl( clientHdr->flags ));

                // Tradeoff mode
                if( IPERF_HEADER_VERSION1 & clientHdrTrans.flags )
                {
                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_6, P_WARNING, 0, "Tradeoff mode, client-side not support");
                }

                break;
            }

            //send one packet for NAT case,if rcv packet timeout
            if(nBytes <= 0 && sock_get_errno(sockfd) == ETIME)
            {
                if(IperfNatServerSendUdpPkg(sockfd, serverPort, &serverParam->destAddr) < 0)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_7, P_WARNING, 0, "send one packet to client for NAT case fail");
                }
                else
                {//if send fail,check whether need stop iperf server
                    if(bIperfSrvTerminRunning || currTime - offsetTime1 > rcvTime)
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }

            //check whether rcv stop command
            if(bIperfSrvTerminRunning)
            {
                break;
            }

            //check whether the total rcv timer timeout
            if((currTime - offsetTime1 > rcvTime) && (currTime > offsetTime1))
            {
                break;
            }

            //check whether socket error and is not timeout
            if( nBytes <= 0 && sock_get_errno(sockfd) != ETIME)
            {
                if(sock_get_errno(sockfd) != EHOSTUNREACH)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_8, P_WARNING, 0, "iperf UDP nat server socket rcv error");
                    break;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_12, P_INFO, 0, "iperf UDP nat server ,maybe detach, try again");
                }
            }
        }while(TRUE);

    }while(FALSE);

    // the end report
    IperfGetCurrentTime( &t2, 0 );

    nmIperfResult.mode = NM_IPERF_MODE_SERVER;
    nmIperfResult.result = NM_IPERF_END_REPORT_SUCCESS;
    nmIperfResult.dataNum = pktCount.Bytes;
    if((pktCount.times > 0) && (t2 - t1 > 0))
    {
        nmIperfResult.bandwidth = ((pktCount.Bytes * 8)/(t2 - t1));
    }
    else
    {
        nmIperfResult.bandwidth = 0;
    }
    NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_9, P_SIG, 2, "iperf UDP nat server report,TOTAL receive %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

    close( sockfd );
    free( buffer );
    bIperfSrvTerminRunning = 0;
    iperfSrvTask = NULL;
    bIperfSrvIsGoingOn = 0;

    /* before iperf task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunNatServer_10, P_SIG, 0, "iperf UDP nat server terminal");

    osThreadExit();
}

void IperfUdpRunServer( void* arg )
{

    NmIperfReq * serverParam = (NmIperfReq *)arg;
    INT32 sockfd;
    struct sockaddr_in cliaddr;
    INT32 cliLen;
    IperfCount pktCount = {0, 0, 0, 0, 0};
    IperfCount tmpCount = {0, 0, 0, 0, 0};
    INT32 nBytes = 0;
    INT32 serverPort = 0;
    UINT16 rcvTime;
    UINT32 totalRptNum = 1;
    UINT32 tmpRptNum = 0;    
    UINT32 intervalTag;
    UINT32 rcvTimeout;
    INT32 ret;

    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 iperfPmuHdr = 0;

    UINT32 t1, t2, currTime, offsetTime1;
    IperfUdpDatagram *udpHdr;
    IperfClientHeader *clientHdr;
    IperfClientHeader clientHdrTrans;

    int isTestStarted = 0;
    INT32 udpHdrId = 0;

    struct NmAtiIperfResultIndTag nmIperfResult;
    memset(&nmIperfResult, 0, sizeof(struct NmAtiIperfResultIndTag));

    //init setting
    if(serverParam->rptIntervalPresent && serverParam->rptIntervalS > 0)
    {
        intervalTag = serverParam->rptIntervalS;
    }
    else
    {
        intervalTag = IPERF_DEFAULT_REPORT_INTERVAL;
    }

    if(serverParam->durationPresent && serverParam->durationS > 0)
    {
        rcvTime = serverParam->durationS;
    }
    else
    {
        rcvTime = IPERF_MAX_SEND_RCV_TIME;
    }

    if(serverParam->port > 0)
    {
        serverPort = serverParam->port;
    }
    else
    {
        serverPort = IPERF_DEFAULT_PORT;
    }

    if(intervalTag < IPERF_SERVER_RECEIVE_TIMEOUT)
    {
        rcvTimeout = intervalTag;
    }
    else
    {
        rcvTimeout = IPERF_SERVER_RECEIVE_TIMEOUT;
    }

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_1, P_SIG, 0, "iperf udp server task start runing");

    //init rcv buffer
    CHAR *buffer = (CHAR*) malloc( IPERF_TEST_BUFFER_SIZE );
    if(!buffer)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_2, P_WARNING, 0, "malloc buffer fail");
        nmIperfResult.mode = NM_IPERF_MODE_SERVER;
        nmIperfResult.result = NM_IPERF_MALLOC_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

        iperfSrvTask = NULL;
        bIperfSrvIsGoingOn = 0;
        osThreadExit();
    }
    memset(buffer, 0 ,IPERF_TEST_BUFFER_SIZE);

    //setup udp server socket
    ret = IperfSetupUdpServerSocket(&sockfd, serverPort, rcvTimeout, &serverParam->destAddr, FALSE);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_3, P_WARNING, 0, "setup udp server socket fail");
        nmIperfResult.mode = NM_IPERF_MODE_SERVER;
        nmIperfResult.result = ret;
        NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

        iperfSrvTask = NULL;
        bIperfSrvIsGoingOn = 0;
        free(buffer);
        osThreadExit();
    }

    cliLen = sizeof(cliaddr);

    /*
    * start iperf, iperf task don't allow to enter HIB/Sleep2 state before iperf terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("iperfserver", &iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    IperfGetCurrentTime( &offsetTime1, 0 );

    // Wait and check the request
    do {
        // Handles request
        do {

            //check cfun status first
            if(!psDailBeCfun1State())
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_4, P_WARNING, 0, "Not CFUN1, iperf will terminal");
                break;
            }

            nBytes = recvfrom( sockfd, buffer, IPERF_TEST_BUFFER_SIZE, 0, (struct sockaddr *) &cliaddr,
                               (socklen_t *) &cliLen );

            udpHdr = (IperfUdpDatagram *) buffer;
            udpHdrId = (INT32) ntohl( udpHdr->pkgId );

            pktCount = IperfCalculateResult( nBytes, pktCount, 0 );

            if(nBytes > 0)
            {
                if(IP_IS_V4(&serverParam->destAddr))
                {
                    pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP4, IPERF_STREAM_PROTOCOL_UDP, pktCount);
                }
                else
                {
                    pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP6, IPERF_STREAM_PROTOCOL_UDP, pktCount);
                }
            }

            //record the time when  rcv the first packet
            if( pktCount.times == 1 )
            {
                IperfGetCurrentTime( &t1, 0 );
            }

            IperfGetCurrentTime( &currTime, 0 );

            // Report by second
            if( intervalTag > 0 )
            {
                if( currTime - offsetTime1 > 0 )
                {
                    tmpRptNum = (((int)(currTime - offsetTime1)) / intervalTag);
                    if(tmpRptNum > 0)
                    {
                        if( tmpRptNum == totalRptNum)
                        {

                            nmIperfResult.mode = NM_IPERF_MODE_SERVER;
                            nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                            nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                            nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/intervalTag);

                            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_report_1, P_SIG, 2, "iperf udp server report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                            NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

                            tmpCount = IperfCopyCount( pktCount, tmpCount );

                            totalRptNum = tmpRptNum + 1;
                        }
                        else if(tmpRptNum > totalRptNum)
                        {
                            nmIperfResult.mode = NM_IPERF_MODE_SERVER;
                            nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                            nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                            nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/(intervalTag *(tmpRptNum - totalRptNum + 1)));

                            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_report_2, P_SIG, 2, "iperf udp server report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                            NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

                            tmpCount = IperfCopyCount( pktCount, tmpCount );

                            totalRptNum = tmpRptNum + 1;

                        }
                    } 
                }
            }

            //rcv the fisrt valid udp packet
            if( (isTestStarted == 0) && (udpHdrId > 0) && (nBytes > 0) )
            {
                isTestStarted = 1;
            }//rcf the last packet
            else if ( (udpHdrId < 0) && (isTestStarted == 1) )
            { // the last package
                INT32 oldFlag = 0;

                // print out result
                //TODO: need to send the correct report to client-side, flag = 0 means the report is ignored.
                //if( udpHdrId < 0 )
                //{
                oldFlag = clientHdrTrans.flags;
                clientHdrTrans.flags = (INT32) 0;

                // send the server report to client-side
                sendto( sockfd, buffer, nBytes, 0, (struct sockaddr *) &cliaddr, cliLen );
                clientHdrTrans.flags = oldFlag;
                //}

                clientHdr = (IperfClientHeader *) &buffer[12];
                clientHdrTrans.flags = (INT32) (ntohl( clientHdr->flags ));

                // Tradeoff mode
                if( IPERF_HEADER_VERSION1 & clientHdrTrans.flags )
                {
                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_6, P_WARNING, 0, "Tradeoff mode, client-side not support");
                }

                break;
            }

            //check whether rcv stop command
            if(bIperfSrvTerminRunning)
            {
                break;
            }

            //check whether the total rcv timer timeout
            if((currTime - offsetTime1 > rcvTime) && (currTime > offsetTime1))
            {
                break;
            }

            //check socket error and is not timeout
            if( nBytes <= 0 && sock_get_errno(sockfd) != ETIME)
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_7, P_WARNING, 0, "iperf UDP server socket rcv error");
                break;
            }

        }while(TRUE);

    } while (FALSE);

    //the end report
    IperfGetCurrentTime( &t2, 0 );
    nmIperfResult.mode = NM_IPERF_MODE_SERVER;
    nmIperfResult.result = NM_IPERF_END_REPORT_SUCCESS;
    nmIperfResult.dataNum = pktCount.Bytes;
    if((pktCount.times > 0) && (t2 - t1 > 0))
    {
        nmIperfResult.bandwidth = ((pktCount.Bytes * 8)/(t2 - t1));
    }
    else
    {
        nmIperfResult.bandwidth = 0;
    }
    NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_8, P_SIG, 2, "iperf UDP server report,TOTAL receive %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

    close( sockfd );
    free( buffer );
    bIperfSrvTerminRunning = 0;
    iperfSrvTask = NULL;
    bIperfSrvIsGoingOn = 0;

    /* before iperf task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunServer_9, P_SIG, 0, "iperf UDP server terminal");

    osThreadExit();
}

void IperfTcpRunServer( void *arg)
{
    NmIperfReq * serverParam = (NmIperfReq *)arg;
    INT32 listenfd, connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    INT32 serverPort;
    IperfCount pktCount = {0, 0, 0, 0, 0};
    IperfCount tmpCount = {0, 0, 0, 0, 0};
    INT32 nBytes = 0;
    UINT16 rcvTime;
    UINT32 totalRptNum = 1;
    UINT32 tmpRptNum = 0;    
    UINT32 intervalTag;
    UINT32 rcvTimeout;
    INT32 ret;

    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 iperfPmuHdr = 0;

    UINT32 t1, t2, currTime;

    struct NmAtiIperfResultIndTag nmIperfResult;
    memset(&nmIperfResult, 0, sizeof(struct NmAtiIperfResultIndTag));

    //init setting
    if(serverParam->rptIntervalPresent && serverParam->rptIntervalS > 0)
    {
        intervalTag = serverParam->rptIntervalS;
    }
    else
    {
        intervalTag = IPERF_DEFAULT_REPORT_INTERVAL;
    }

    if(serverParam->durationPresent && serverParam->durationS > 0)
    {
        rcvTime = serverParam->durationS;
    }
    else
    {
        rcvTime = IPERF_MAX_SEND_RCV_TIME;
    }

    if(serverParam->port > 0)
    {
        serverPort = serverParam->port;
    }
    else
    {
        serverPort = IPERF_DEFAULT_PORT;
    }

    if(intervalTag < IPERF_SERVER_RECEIVE_TIMEOUT)
    {
        rcvTimeout = intervalTag;
    }
    else
    {
        rcvTimeout = IPERF_SERVER_RECEIVE_TIMEOUT;
    }


    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_1, P_SIG, 0, "iperf tcp server task start runing");

    //init rcv buffer
    CHAR *buffer = (CHAR*) malloc( IPERF_TEST_BUFFER_SIZE );
    if(!buffer)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_2, P_WARNING, 0, "IPERF tcp server malloc buffer fail!");
        nmIperfResult.mode = NM_IPERF_MODE_SERVER;
        nmIperfResult.result = NM_IPERF_MALLOC_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

        iperfSrvTask = NULL;
        bIperfSrvIsGoingOn = 0;
        osThreadExit();
    }


    // Create a new TCP connection handle
    ret = IperfSetupTcpServerSocket(&listenfd, serverPort, rcvTimeout, &serverParam->destAddr);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_3, P_WARNING, 0, "IPERF tcp server setup socket fail!");
        nmIperfResult.mode = NM_IPERF_MODE_SERVER;
        nmIperfResult.result = ret;
        NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

        iperfSrvTask = NULL;
        bIperfSrvIsGoingOn = 0;
        free(buffer);
        osThreadExit();
    }

    /*
    * start iperf, iperf task don't allow to enter HIB/Sleep2 state before iperf terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("iperfserver", &iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    do {

        do {

            // ait for an incoming connection
            if ( (connfd = accept( listenfd, (struct sockaddr *) &cliaddr, &clilen )) != -1 )
            {

                 IperfGetCurrentTime( &t1, 0 );

                //Connection
                do {

                    //check pdn status first
                    if(!psDailBeCfun1State())
                    {
                        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_4, P_WARNING, 0, "Not Cfun1, iperf will terminal");
                        break;
                    }

                    nBytes = recv( connfd, buffer, IPERF_TEST_BUFFER_SIZE, 0 );

                    pktCount = IperfCalculateResult( nBytes, pktCount, 0 );

                    if(nBytes > 0)
                    {
                        if(IP_IS_V4(&serverParam->destAddr))
                        {
                            pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP4, IPERF_STREAM_PROTOCOL_TCP, pktCount);
                        }
                        else
                        {
                            pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP6, IPERF_STREAM_PROTOCOL_TCP, pktCount);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_5, P_WARNING, 0, "iperf tcp rcv fail");
                    }

                    IperfGetCurrentTime( &currTime, 0 );
                    if ( intervalTag > 0 )
                    {                        
                        if( currTime - t1 > 0 )
                        {
                            tmpRptNum = (((int)(currTime - t1)) / intervalTag);
                            if(tmpRptNum > 0)
                            {
                                if( tmpRptNum == totalRptNum)
                                {

                                    nmIperfResult.mode = NM_IPERF_MODE_SERVER;
                                    nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                                    nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                                    nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/intervalTag);

                                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_report_1, P_SIG, 2, "iperf tcp server report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                                    NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

                                    tmpCount = IperfCopyCount( pktCount, tmpCount );

                                    totalRptNum = tmpRptNum + 1;
                                }
                                else if(tmpRptNum > totalRptNum)
                                {
                                    nmIperfResult.mode = NM_IPERF_MODE_SERVER;
                                    nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                                    nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                                    nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/(intervalTag *(tmpRptNum - totalRptNum + 1)));
    
                                    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_report_2, P_SIG, 2, "iperf tcp server report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                                    NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

                                    tmpCount = IperfCopyCount( pktCount, tmpCount );

                                    totalRptNum = tmpRptNum + 1;
                               }
                            }
                       }                          
                    }

                    // check whether rcv stop command
                    if(bIperfSrvTerminRunning)
                    {
                        break;
                    }

                    //check whether the total rcv timer timeout
                    if((currTime - t1 > rcvTime) && (currTime > t1))
                    {
                        break;
                    }

                    //check socket error and is not timeout
                    if( nBytes <= 0 && sock_get_errno(connfd) != ERR_TIMEOUT)
                    {
                        break;
                    }
               }while(TRUE);
                //if the connfd socket error, try accept again
                close( connfd );
            }

            // check whether rcv stop command
            if(bIperfSrvTerminRunning)
            {
                break;
            }

            //check whether the total rcv timer timeout
            if((currTime - t1 > rcvTime)  && (currTime > t1))
            {
                break;
            }
        }while(TRUE);
    } while (FALSE); //Loop just once

    //the end report
    IperfGetCurrentTime( &t2, 0 );
    nmIperfResult.mode = NM_IPERF_MODE_SERVER;
    nmIperfResult.result = NM_IPERF_END_REPORT_SUCCESS;
    nmIperfResult.dataNum = pktCount.Bytes;
    if((pktCount.times > 0) && (t2 - t1 > 0))
    {
        nmIperfResult.bandwidth = ((pktCount.Bytes * 8)/(t2 - t1));
    }
    else
    {
        nmIperfResult.bandwidth = 0;
    }

    NetMgrSendIperfResultInd(&nmIperfResult, serverParam->reqHandle);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_7, P_SIG, 2, "iperf tcp server report,TOTAL receive %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

    close( listenfd );
    free( buffer );
    bIperfSrvTerminRunning = 0;
	iperfSrvTask = NULL;
    bIperfSrvIsGoingOn = 0;

    /* before iperf task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunServer_8, P_SIG, 0, "iperf tcp server terminal");

    osThreadExit();

}

void IperfTcpRunClient( void* arg )
{

    NmIperfReq *clientParam = (NmIperfReq *)arg;
    INT32 sockfd;
    IperfCount pktCount = {0, 0, 0, 0, 0};
    IperfCount tmpCount = {0, 0, 0, 0, 0};
    INT32 nBytes = 0;
    INT32 winSize, sendTime, serverPort;
    UINT32 t1, t2, currTime, bw, pktDelay;
    INT32 totalSend;
    UINT32 totalRptNum = 1;
    UINT32 tmpRptNum = 0;
    INT32 ret;
    INT32 sockErr;

    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 iperfPmuHdr = 0;
    UINT32 intervalTag;

    //Statistics init
    struct NmAtiIperfResultIndTag nmIperfResult;
    memset(&nmIperfResult, 0, sizeof(struct NmAtiIperfResultIndTag));

    //init setting
    if(clientParam->rptIntervalPresent && clientParam->rptIntervalS > 0)
    {
        intervalTag = clientParam->rptIntervalS; /* the tag of parameter "-i"  */
    }
    else
    {
        intervalTag = IPERF_DEFAULT_REPORT_INTERVAL;
    }

    if(clientParam->payloadSizePresent && clientParam->payloadSize >= 36)
    {
        winSize = ((clientParam->payloadSize > IPERF_DEFAULT_PAYLOAD_SIZE) ? IPERF_DEFAULT_PAYLOAD_SIZE : clientParam->payloadSize);
    }
    else
    {
        winSize = IPERF_DEFAULT_PAYLOAD_SIZE;
    }

    if(clientParam->pkgNumPresent && clientParam->pkgNum > 0)
    {
        totalSend = clientParam->pkgNum * winSize; /* the total number of transmit  */
    }
    else
    {
        totalSend = IPERF_TOTAL_SEND_MAX;
    }

    if(clientParam->durationPresent && clientParam->durationS > 0)
    {
        sendTime = clientParam->durationS;
    }
    else
    {
        sendTime = IPERF_MAX_SEND_RCV_TIME;
    }

    if(clientParam->port > 0)
    {
        serverPort = clientParam->port;
    }
    else
    {
        serverPort = IPERF_DEFAULT_PORT;
    }

    if (clientParam->tptPresent && clientParam->tpt > 0)
    {
        /* the input is bps, but bw is Bps (bytes per second)*/
        if ((clientParam->tpt) & 0x07)
        {
            bw = ((clientParam->tpt) >> 3) + 1;
        }
        else
        {
            bw = (clientParam->tpt) >> 3;
        }
    }
    else
    {
        bw = IPERF_DEFAULT_TPT; //8bps
    }

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_1, P_SIG, 0, "iperf tcp client task start runing");

    //init send buffer
    CHAR *buffer = (CHAR*) malloc( winSize );
    if(!buffer)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_2, P_WARNING, 0, "iperf tcp client malloc buffer fail");

        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
        nmIperfResult.result = NM_IPERF_MALLOC_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

        iperfCLtTask = NULL;
        bIperfCltIsGoingOn = 0;
        osThreadExit();
    }

    //calc the send packet delay depend on bw setting
    if( bw > 0 )
    {
        pktDelay = (1000 * winSize) / bw;
    }


    //setup socket connection
    if(clientParam->destAddrPresent)
    {
        ret = IperfSetupClientSocket(&sockfd, IPERF_STREAM_PROTOCOL_TCP, serverPort, &clientParam->destAddr);
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_3, P_WARNING, 0, "iperf tcp client socket setup fail");

            nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
            nmIperfResult.result = ret;
            NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

            iperfCLtTask = NULL;
            bIperfCltIsGoingOn = 0;
            free(buffer);
            osThreadExit();
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_4, P_WARNING, 0, "iperf tcp client dest addr is not present");

        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
        nmIperfResult.result = NM_IPERF_PARAM_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

        iperfCLtTask = NULL;
        bIperfCltIsGoingOn = 0;
        free(buffer);
        osThreadExit();
    }

    /*
    * start iperf, iperf task don't allow to enter HIB/Sleep2 state before iperf terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("iperfclient", &iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    IperfGetCurrentTime( &t1, 0 );

    do {
        //check pdn status first
        if(!psDailBeCfun1State())
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_5, P_WARNING, 0, "Not CFUN1 state, iperf will terminal");
            break;
        }

        nBytes = send( sockfd, buffer, winSize, 0 );

        pktCount = IperfCalculateResult( nBytes, pktCount, 0 );

        if(nBytes > 0)
        {
            if(IP_IS_V4(&clientParam->destAddr))
            {
                pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP4, IPERF_STREAM_PROTOCOL_TCP, pktCount);
            }
            else
            {
                pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP6, IPERF_STREAM_PROTOCOL_TCP, pktCount);
            }
        }
        else
        {
            sockErr = sock_get_errno(sockfd);
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_6, P_ERROR, 1, "iperf tcp client send packet fail,error code %d", sockErr);

            if(socket_error_is_fatal(sockErr))
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_7, P_ERROR, 1, "iperf tcp client send packet fail,the error %d is fata error ,exist", sockErr);
                break;
            }

        }

        IperfmSleep( pktDelay );

        IperfGetCurrentTime( &currTime, 0 );

        if ( intervalTag > 0 )
        {
            if( currTime - t1 > 0 )
            {
                tmpRptNum = (((int)(currTime - t1)) / intervalTag);
                if(tmpRptNum > 0)
                {
                    if( tmpRptNum == totalRptNum)
                    {

                        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
                        nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                        nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                        nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/intervalTag);

                        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_report_1, P_SIG, 2, "iperf tcp client report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

                        tmpCount = IperfCopyCount( pktCount, tmpCount );

                        totalRptNum = tmpRptNum + 1;
                    }
                    else if(tmpRptNum > totalRptNum)
                    {
                        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
                        nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                        nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                        nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/(intervalTag *(tmpRptNum - totalRptNum + 1)));

                        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_report_2, P_SIG, 2, "iperf tcp client report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

                        tmpCount = IperfCopyCount( pktCount, tmpCount );

                        totalRptNum = tmpRptNum + 1;

                    }
                }
            }
        }

        //check whether packet has send complete
        if(nBytes > 0)
        {
            totalSend -= nBytes;
        }
        if(totalSend <= 0)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_9, P_SIG, 0, "iperf tcp client send total packet finish");
            break;
        }

        //check whether rcv stop command
        if(bIperfCltTerminRunning)
        {
            break;
        }

        //check whether the total send timer timeout
        if((currTime - t1 > sendTime) && (currTime > t1))
        {
            break;
        }


    } while(TRUE);

    //the end report
    IperfGetCurrentTime( &t2, 0 );
    nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
    nmIperfResult.result = NM_IPERF_END_REPORT_SUCCESS;
    nmIperfResult.dataNum = pktCount.Bytes;
    if((t2 - t1 > 0) && (pktCount.times > 0)) {
        nmIperfResult.bandwidth = ((pktCount.Bytes * 8)/(t2-t1));
    }
    else
    {
        nmIperfResult.bandwidth = 0;
    }
    NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_10, P_SIG, 2, "iperf tcp client report,TOTAL send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

    close( sockfd );
	free(buffer);
    bIperfCltTerminRunning = 0;
    iperfCLtTask = NULL;
    bIperfCltIsGoingOn = 0;

    /* before iperf task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfTcpRunClient_11, P_SIG, 0, "iperf tcp client terminal");

    osThreadExit();

}

void IperfUdpRunClient( void* arg )
{

    NmIperfReq *clientParam = (NmIperfReq *)arg;

    INT32 sockfd;
    INT32 ret;
    IperfCount pktCount = {0, 0, 0, 0, 0};
    IperfCount tmpCount = {0, 0, 0, 0, 0};
    INT32 nBytes = 0;
    INT32 tradeoffTag = FALSE;
    UINT32 dataSize, sendTime, serverPort;
    UINT32 pktDelay = 0;
    UINT32 pktDelayOffset = 0;
    UINT32 totalRptNum = 1;
    UINT32 tmpRptNum = 0;

    UINT32  bw;    //bandwidth, in Bps (Bytes per second)
    UINT32  t1, t2, currTime, t1_ms, lastTick, currentTick, lastSleep, currentSleep;
    INT32   totalSend;
    INT32   sockErr = 0, lastErr = 0;

    IperfUdpDatagram *udpHdr;
    IperfClientHeader *clientHdr;
    INT32 udpHdrId = 0;

    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 iperfPmuHdr = 0;
    UINT32 intervalTag;

    struct NmAtiIperfResultIndTag nmIperfResult;
    memset(&nmIperfResult, 0, sizeof(struct NmAtiIperfResultIndTag));

    //init setting
    if (clientParam->rptIntervalPresent && clientParam->rptIntervalS > 0)
    {
        intervalTag = clientParam->rptIntervalS;
    }
    else
    {
        intervalTag = IPERF_DEFAULT_REPORT_INTERVAL;
    }

    if (clientParam->payloadSizePresent && clientParam->payloadSize >= 36)
    {
        dataSize = ((clientParam->payloadSize > IPERF_DEFAULT_PAYLOAD_SIZE) ? IPERF_DEFAULT_PAYLOAD_SIZE : clientParam->payloadSize);
    }
    else
    {
        dataSize = IPERF_DEFAULT_PAYLOAD_SIZE;
    }

    if (clientParam->pkgNumPresent && clientParam->pkgNum > 0)
    {
        totalSend = clientParam->pkgNum * dataSize;
    }
    else
    {
        totalSend = IPERF_TOTAL_SEND_MAX;
    }

    if (clientParam->durationPresent && clientParam->durationS > 0)
    {
        sendTime = clientParam->durationS;
    }
    else
    {
        sendTime = IPERF_MAX_SEND_RCV_TIME;
    }

    if (clientParam->port > 0)
    {
        serverPort = clientParam->port;
    }
    else
    {
        serverPort = IPERF_DEFAULT_PORT;
    }

    if (clientParam->tptPresent && clientParam->tpt > 0)
    {
        /* the input is bps, but bw is Bps (bytes per second)*/
        if ((clientParam->tpt) & 0x07)
        {
            bw = ((clientParam->tpt) >> 3) + 1;
        }
        else
        {
            bw = (clientParam->tpt) >> 3;
        }
    }
    else
    {
        bw = IPERF_DEFAULT_TPT; //1KBps
    }

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_1, P_SIG, 0, "iperf udp client task start runing");

    //init send buffer
    CHAR *buffer = (CHAR*) malloc(dataSize);
    if(!buffer)
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_2, P_WARNING, 0, "iperf udp client malloc buffer fail");

        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
        nmIperfResult.result = NM_IPERF_MALLOC_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

        iperfCLtTask = NULL;
        bIperfCltIsGoingOn = 0;
        osThreadExit();
    }
    memset(buffer, 0, dataSize);

    //calc the send packet delay depend on the bw setting
    if (bw > 0)
    {
        pktDelay = (1000 * dataSize)/bw;

        // pkt_dalay add 1ms regularly to reduce the offset
        pktDelayOffset = (((1000 * dataSize) % bw) * 10 / bw);
        if( pktDelayOffset )
        {
            pktDelayOffset = 10 / pktDelayOffset;
        }
    }

    //setup socket connection
    if (clientParam->destAddrPresent)
    {
        ret = IperfSetupClientSocket(&sockfd, IPERF_STREAM_PROTOCOL_UDP, serverPort, &clientParam->destAddr);
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_3, P_WARNING, 0, "iperf udp client socket setup fail");

            nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
            nmIperfResult.result = ret;
            NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

            iperfCLtTask = NULL;
            bIperfCltIsGoingOn = 0;
            free(buffer);
            osThreadExit();
        }

        //enable udp socket high water without delay retry for iperf udp client service
        UINT32 hwwdr = 1;
        ioctl(sockfd, FIOHWODR, &hwwdr);
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_4, P_WARNING, 0, "iperf udp client dest addr is not present");

        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
        nmIperfResult.result = NM_IPERF_PARAM_ERROR;
        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

        iperfCLtTask = NULL;
        bIperfCltIsGoingOn = 0;
        free(buffer);
        osThreadExit();
    }


    // Init UDP data header
    udpHdr = (IperfUdpDatagram *) &buffer[0];
    clientHdr = (IperfClientHeader *) &buffer[12];
    if( tradeoffTag == TRUE )
    {
        clientHdr->flags = htonl( IPERF_HEADER_VERSION1 );
    }
    else
    {
        clientHdr->flags = 0;
    }
    clientHdr->numChannels = htonl( 1 );
    clientHdr->portClient = htonl( serverPort );
    clientHdr->buffLen = 0;
    clientHdr->band = htonl(bw);
    clientHdr->amounts = htonl((long )(sendTime * 100));
    clientHdr->amounts &= htonl(0x7FFFFFFF);

    /*
    * start iperf, iperf task don't allow to enter HIB/Sleep2 state before iperf terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("iperfclient", &iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    IperfGetCurrentTime( &t1, &t1_ms );
    lastTick = t1_ms;
    lastSleep = 0;

    do {

        //check pdn status first
        if(!psDailBeCfun1State())
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_5, P_WARNING, 0, "Not cfun1 state, iperf will terminal");
            break;
        }

        udpHdr->pkgId = htonl( udpHdrId );
        udpHdr->sec = htonl( (lastTick + lastSleep) / 1000 );
        udpHdr->uSec = htonl( lastTick + lastSleep );

        udpHdrId++;

        nBytes = send(sockfd, buffer, dataSize, 0);

        pktCount = IperfCalculateResult( nBytes, pktCount, 0 );

        if (nBytes > 0)
        {
            lastErr = 0;
            if(IP_IS_V4(&clientParam->destAddr))
            {
                pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP4, IPERF_STREAM_PROTOCOL_UDP, pktCount);
            }
            else
            {
                pktCount = IperfCalculateAddHeader(IPERF_STREAM_DOMAIN_IP6, IPERF_STREAM_PROTOCOL_UDP, pktCount);
            }
        }
        else
        {
            sockErr = sock_get_errno(sockfd);

            /* reduce some warning print */
            if (lastErr != sockErr)
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_6, P_ERROR, 1,
                            "iperf udp client send packet fail, error code: %d", sockErr);

                lastErr = sockErr;
            }

            if (socket_error_is_fatal(sockErr))
            {
                ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_7, P_ERROR, 1, "iperf udp client send packet fail,the error %d is fata error ,exist", sockErr);
                break;
            }

        }

        IperfGetCurrentTime(&currTime, &currentTick);

        if (pktDelayOffset > 0)
        {
            if ((udpHdrId % pktDelayOffset) == 0)
            {
                currentSleep = pktDelay - (currentTick - lastTick - lastSleep) + 1;
            }
        }
        else
        {
            currentSleep = pktDelay - (currentTick - lastTick - lastSleep);
        }

        if ((INT32)currentSleep > 0)
        {
            IperfmSleep( currentSleep );
        }
        else
        {
            currentSleep = 0;
        }

        lastTick = currentTick;
        lastSleep = currentSleep;

        if (intervalTag > 0)
        {
            if( currTime - t1 > 0 )
            {
                tmpRptNum = (((int)(currTime - t1)) / intervalTag);
                if(tmpRptNum > 0)
                {
                    if( tmpRptNum == totalRptNum)
                    {

                        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
                        nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                        nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                        nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/intervalTag);

                        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_report_1, P_SIG, 2, "iperf udp client report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

                        tmpCount = IperfCopyCount( pktCount, tmpCount );

                        totalRptNum = tmpRptNum + 1;
                    }
                    else if(tmpRptNum > totalRptNum)
                    {
                        nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
                        nmIperfResult.result = NM_IPERF_ONE_REPORT_SUCCESS;
                        nmIperfResult.dataNum = pktCount.Bytes - tmpCount.Bytes;
                        nmIperfResult.bandwidth = (((pktCount.Bytes - tmpCount.Bytes) * 8)/(intervalTag *(tmpRptNum - totalRptNum + 1)));

                        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_report_2, P_SIG, 2, "iperf udp client report,send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

                        NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

                        tmpCount = IperfCopyCount( pktCount, tmpCount );

                        totalRptNum = tmpRptNum + 1;

                    }
                }
            }            
        }

        //check whether packet has sent complete
        if(nBytes > 0)
        {
            totalSend -= nBytes;
        }
        if(totalSend <= 0)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_9, P_SIG, 0, "iperf udp client send total packet finish");
            break;
        }

        //check whether rcv stop command
        if(bIperfCltTerminRunning)
        {
            break;
        }

        //check whether the total send timer timeout
        if((currTime - t1 > sendTime) && (currTime > t1))
        {
            break;
        }


    } while(TRUE);

    //the total end report
    IperfGetCurrentTime( &t2, 0 );
    nmIperfResult.mode = NM_IPERF_MODE_CLIENT;
    nmIperfResult.result = NM_IPERF_END_REPORT_SUCCESS;
    nmIperfResult.dataNum = pktCount.Bytes;
    if((t2 - t1 > 0) && (pktCount.times > 0))
    {
        nmIperfResult.bandwidth = ((pktCount.Bytes * 8)/(t2-t1));
    }
    else
    {
        nmIperfResult.bandwidth = 0;
    }

    NetMgrSendIperfResultInd(&nmIperfResult, clientParam->reqHandle);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_10, P_SIG, 2, "iperf udp client report,TOTAL send %u bytes payload packet, bandwidth %u bps", nmIperfResult.dataNum, nmIperfResult.bandwidth);

    // send the last datagram
    udpHdrId = (-udpHdrId);
    udpHdr->pkgId = htonl( udpHdrId );
    IperfGetCurrentTime( &currTime, 0 );
    udpHdr->sec = htonl( currTime );
    udpHdr->uSec = htonl( currTime * 1000 );

    nBytes = send( sockfd, buffer, dataSize, 0 );

    //TODO: receive the report from server side and print out

    close( sockfd );
    free( buffer );
    bIperfCltTerminRunning = 0;
    iperfCLtTask = NULL;
    bIperfCltIsGoingOn = 0;

    /* before iperf task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(iperfPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(iperfPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfUdpRunClient_11, P_SIG, 0, "iperf udp client terminal");

    osThreadExit();
}

IperfCount IperfCalculateResult( INT32 pktSize, IperfCount pktCount, INT32 needToConvert )
{
    if ( pktSize > 0 ) {
        pktCount.Bytes += pktSize;
        pktCount.times++;
    }

    if ( needToConvert == 1 ) {
        if ( pktCount.Bytes >= 1024 ) {
            pktCount.KBytes += (pktCount.Bytes / 1024);
            pktCount.Bytes = pktCount.Bytes % 1024;
        }

        if ( pktCount.KBytes >= 1024 ) {
            pktCount.MBytes += (pktCount.KBytes / 1024);
            pktCount.KBytes = pktCount.KBytes % 1024;
        }

        if ( pktCount.MBytes >= 1024 ) {
            pktCount.GBytes += (pktCount.MBytes / 1024);
            pktCount.MBytes = pktCount.MBytes % 1024;
        }
    }

    return pktCount;
}

IperfCount IperfCalculateAddHeader( INT32 domain, INT32 protocol, IperfCount pktCount )
{
    if(domain == IPERF_STREAM_DOMAIN_IP4)
    {
        pktCount.Bytes += 20;
    }else if(domain == IPERF_STREAM_DOMAIN_IP6) {
        pktCount.Bytes += 40;
    }

    if(protocol == IPERF_STREAM_PROTOCOL_UDP)
    {
        pktCount.Bytes += 8;
    }else if(protocol == IPERF_STREAM_PROTOCOL_TCP) {
        pktCount.Bytes += 20;
    }

    return pktCount;
}

IperfCount IperfResetCount( IperfCount pktCount )
{
    pktCount.Bytes = 0;
    pktCount.KBytes = 0;
    pktCount.MBytes = 0;
    pktCount.GBytes = 0;
    pktCount.times = 0;

    return pktCount;
}

IperfCount IperfCopyCount( IperfCount pktCount, IperfCount tmpCount )
{

    tmpCount.Bytes = pktCount.Bytes;
    tmpCount.KBytes = pktCount.KBytes;
    tmpCount.MBytes = pktCount.MBytes;
    tmpCount.GBytes = pktCount.GBytes;
    tmpCount.times = pktCount.times;

    return tmpCount;
}

IperfCount IperfDiffCount( IperfCount pktCount, IperfCount tmpCount )
{
    /* pktCount > tmpCount */
    tmpCount.times = pktCount.times - tmpCount.times;

    if ( pktCount.Bytes >= tmpCount.Bytes ) {
        tmpCount.Bytes = pktCount.Bytes - tmpCount.Bytes;
    } else {
        tmpCount.Bytes = pktCount.Bytes + 1024 - tmpCount.Bytes;
        if ( pktCount.KBytes > 0 ) {
            pktCount.KBytes--;
        } else if ( pktCount.MBytes > 0 ) {
            pktCount.MBytes--;
            pktCount.KBytes = 1023;
        } else if ( pktCount.GBytes > 0 ) {
            pktCount.GBytes--;
            pktCount.MBytes = 1023;
            pktCount.KBytes = 1023;
        }
    }

    if ( pktCount.KBytes >= tmpCount.KBytes ) {
        tmpCount.KBytes = pktCount.KBytes - tmpCount.KBytes;
    } else {
        tmpCount.KBytes = pktCount.KBytes + 1024 - tmpCount.KBytes;
        if ( pktCount.MBytes > 0 ) {
            pktCount.MBytes--;
        } else if ( pktCount.GBytes > 0 ) {
            pktCount.GBytes--;
            pktCount.MBytes = 1023;
        }
    }

    if ( pktCount.MBytes >= tmpCount.MBytes ) {
        tmpCount.MBytes = pktCount.MBytes - tmpCount.MBytes;
    } else {
        tmpCount.MBytes = pktCount.MBytes + 1024 - tmpCount.MBytes;
        if ( pktCount.GBytes > 0 ) {
            pktCount.GBytes--;
        }
    }

    return tmpCount;
}

BOOL IperfInit(NmIperfReq *pIperfReq)
{
    osThreadId_t xReturn = NULL;
    osThreadAttr_t attr;
    CHAR pcNameClient[] = "iperf_client_thread";
    CHAR pcNameServer[] = "iperf_server_thread";

    OsaDebugBegin(pIperfReq != PNULL, pIperfReq, 0, 0);
    return FALSE;
    OsaDebugEnd();

    //check iperf task running status
    if (pIperfReq->reqAct == NM_IPERF_START_CLIENT)
    {
        if (bIperfCltIsGoingOn)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_1, P_WARNING, 0, "iperf client is going on,please run it wait a moment");
            return FALSE;
        }
        else
        {
            bIperfCltIsGoingOn = TRUE;
        }
    }
    else if(pIperfReq->reqAct == NM_IPERF_START_SERVER || pIperfReq->reqAct == NM_IPERF_START_UDP_NAT_SERVER)
    {
        if (bIperfSrvIsGoingOn)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_2, P_WARNING, 0, "iperf server is going on,please run it wait a moment");
            return FALSE;
        }
        else
        {
            bIperfSrvIsGoingOn = TRUE;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_3, P_WARNING, 0, "iperf mode is invalid");
        return FALSE;
    }

    //set task attr
    memset(&attr, 0, sizeof(osThreadAttr_t));
    attr.name = ((pIperfReq->reqAct == NM_IPERF_START_CLIENT) ? pcNameClient : pcNameServer);
    attr.stack_size = IPERF_THREAD_STACKSIZE;
    attr.priority = (osPriority_t)IPERF_THREAD_PRIO;

    //setup iperf task
    if(pIperfReq->reqAct == NM_IPERF_START_CLIENT)
    {
        memcpy(&pIperfClientReq, pIperfReq, sizeof(NmIperfReq));

        if (pIperfReq->protocol == NM_IPERF_PROTOCOL_UDP)
        { //udp
            xReturn = osThreadNew(IperfUdpRunClient, (void*)&pIperfClientReq, &attr);
        }
        else if (pIperfReq->protocol == NM_IPERF_PROTOCOL_TCP)
        { //tcp
            xReturn = osThreadNew(IperfTcpRunClient, (void*)&pIperfClientReq, &attr);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_4, P_WARNING, 0, "iperf protocol is invalid");
            bIperfCltIsGoingOn = FALSE;
            return FALSE;
        }

        if (xReturn == NULL)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_5, P_WARNING, 0, "can not create iperf task");
            bIperfCltIsGoingOn = FALSE;
            return FALSE;
        }
        else
        {
            iperfCLtTask = xReturn;
        }
    }
    else if(pIperfReq->reqAct == NM_IPERF_START_SERVER)
    {
        memcpy(&pIperfServerReq, pIperfReq, sizeof(NmIperfReq));

        if (pIperfReq->protocol == NM_IPERF_PROTOCOL_UDP)
        { //udp
            xReturn = osThreadNew(IperfUdpRunServer, (void*)&pIperfServerReq, &attr);
        }
        else if (pIperfReq->protocol == NM_IPERF_PROTOCOL_TCP)
        { //tcp
            xReturn = osThreadNew(IperfTcpRunServer, (void*)&pIperfServerReq, &attr);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_6, P_WARNING, 0, "iperf protocol is invalid");
            bIperfSrvIsGoingOn = FALSE;
            return FALSE;
        }

        if (xReturn == NULL)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_7, P_WARNING, 0, "can not create iperf task");
            bIperfSrvIsGoingOn = FALSE;
            return FALSE;
        }
        else
        {
            iperfSrvTask = xReturn;
        }
    }
    else if(pIperfReq->reqAct == NM_IPERF_START_UDP_NAT_SERVER)
    {
        memcpy(&pIperfServerReq, pIperfReq, sizeof(NmIperfReq));

        if (pIperfReq->protocol == IPERF_STREAM_PROTOCOL_UDP)
        { //udp
            xReturn = osThreadNew(IperfUdpRunNatServer, (void*)&pIperfServerReq, &attr);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_6, P_WARNING, 0, "iperf protocol is invalid");
            bIperfSrvIsGoingOn = FALSE;
            return FALSE;
        }

        if (xReturn == NULL)
        {
            ECOMM_TRACE(UNILOG_TCPIP_IPERF, IperfInit_7, P_WARNING, 0, "can not create iperf task");
            bIperfSrvIsGoingOn = FALSE;
            return FALSE;
        }
        else
        {
            iperfSrvTask = xReturn;
        }
    }

    return TRUE;
}


void IperfTerminate(NmIperfReq *pIperfReq)
{
    OsaDebugBegin(pIperfReq != PNULL, pIperfReq, 0, 0);
    return;
    OsaDebugEnd();

    switch(pIperfReq->reqAct)
    {
        case NM_IPERF_STOP_ALL:
        {
            if(iperfCLtTask)
            {
                bIperfCltTerminRunning = 1;
            }
            if(iperfSrvTask)
            {
                bIperfSrvTerminRunning = 1;
            }
            break;
        }
        case NM_IPERF_STOP_CLIENT:
        {
            if(iperfCLtTask)
            {
                bIperfCltTerminRunning = 1;
            }
            break;
        }
        case NM_IPERF_STOP_SERVER:
        {
            if(iperfSrvTask)
            {
                bIperfSrvTerminRunning = 1;
            }
            break;
        }
        default:
            break;
    }

    return;
}


