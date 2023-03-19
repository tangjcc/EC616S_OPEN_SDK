/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_tcpip.c
*
*  Description: Process packet switched service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "debug_trace.h"
#include "cmsis_os2.h"
#include "at_util.h"
#include "atec_tcpip.h"
#include "ec_tcpip_api.h"
#include "cmicomm.h"
#include "atec_tcpip_cnf_ind.h"
#include "ps_nm_if.h"
#include "def.h"
#include "inet.h"
#include "sockets.h"
#include "task.h"
#include "netmgr.h"
#include "at_sock_entity.h"
#include "debug_log.h"
#include "mw_config.h"

#define _DEFINE_AT_REQ_FUNCTION_LIST_

static AtSocketSendInfo *pSocketSendInfo = PNULL;



/**
  \fn          CmsRetId nmSNTP(const AtCmdInputContext *pAtCmdReq)
  \brief       AT+ECSNTP CMD codes
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  nmSNTP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 port;
    BOOL autoSync;
    INT32 value;
    UINT16 len = 0;
    UINT8 sntpServer[SNTP_0_STR_MAX_LEN + 1] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+PING=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECSNTP: (\"IP ADDR\\URL\"),(0-65535),(0,1)");
            break;
        }

        case AT_SET_REQ:      /* AT+PING= */
        {
            /* AT+ECSNTP=url/ip [,<port> [,<autosync>]] */
            /*
             * first param is MIX type (could: dec/string)
            */
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, sntpServer, SNTP_0_STR_MAX_LEN+1, &len, SNTP_0_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &value, SNTP_DEC_1_MIN, SNTP_DEC_1_MAX, SNTP_DEC_1_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            port = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, SNTP_DEC_2_MIN, SNTP_DEC_2_MAX, SNTP_DEC_2_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(value != 0)
            {
                autoSync = TRUE;
            }
            else
            {
                autoSync = FALSE;
            }

            if (psSntpReq(reqHandle, (CHAR *)sntpServer, port, autoSync) == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
            }


            break;
        }

        case AT_READ_REQ:           /* AT+ECSNTP?  */
        default:
        {
            OsaDebugBegin(FALSE, operaType, 0, 0);
            OsaDebugEnd();

            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmPING(const AtCmdInputContext *pAtCmdReq)
  \brief       AT+PING CMD codes
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  nmPING(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16   ipAddrLen = 0;
    INT32   count = 0;
    INT32   size = 60;
    INT32   timeout = 20000;
    INT32    raiFlag = FALSE;
    BOOL    validparam = FALSE;
    INT32   dec=0;
    UINT8 pingTarget[PING_0_STR_MAX_LEN + 1] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+PING=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECPING: (\"IP ADDR\\URL\",0),(1-255),(1-1500),(1000-600000),(0,1)");
            break;
        }

        case AT_SET_REQ:      /* AT+PING= */
        {
            /* AT+PING="ip"/0 [,<count> [,<payload_size> [,<time_out>[,<rai_flag>]]]] */
            /*
             * first param is MIX type (could: dec/string)
            */
            if (pAtCmdReq->pParamList[0].type != AT_STR_VAL)
            {
                if (pAtCmdReq->paramRealNum == 1 &&
                    (atGetNumValue(pAtCmdReq->pParamList, 0, &dec,
                                   PING_DEC_0_MIN, PING_DEC_0_MAX, PING_DEC_0_DEF)) == AT_PARA_OK)
                {
                    if (dec == 0)
                    {
                        ret = psStopPing(reqHandle);

                        if (ret == CMS_RET_SUCC)
                        {
                            return atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                        }
                        else
                        {
                            return atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
                        }
                    }
                }
                else
                {
                    return atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                }
            }
            else    //string type
            {
                if ((atGetStrValue(pAtCmdReq->pParamList, 0, pingTarget,
                                   PING_0_STR_MAX_LEN+1, &ipAddrLen, PING_0_STR_DEF)) == AT_PARA_OK)
                {
                    if ((atGetNumValue(pAtCmdReq->pParamList, 1, &count,
                                       PING_1_COUNT_MIN, PING_1_COUNT_MAX, PING_1_COUNT_DEF)) != AT_PARA_ERR)
                    {
                        if ((atGetNumValue(pAtCmdReq->pParamList, 2, &size,
                                           PING_2_PAYLOAD_MIN, PING_2_PAYLOAD_MAX, PING_2_PAYLOAD_DEF)) != AT_PARA_ERR)
                        {
                            if ((atGetNumValue(pAtCmdReq->pParamList, 3, &timeout,
                                               PING_3_TIMEOUT_MIN, PING_3_TIMEOUT_MAX, PING_3_TIMEOUT_DEF)) != AT_PARA_ERR)
                            {
                                if ((atGetNumValue(pAtCmdReq->pParamList, 4, &raiFlag,
                                                    PING_4_RAI_FLAG_MIN, PING_4_RAI_FLAG_MAX, PING_4_RAI_FLAG_DEF)) != AT_PARA_ERR)
                                    {
                                        validparam = true;
                                        if(raiFlag == 0)
                                        {
                                            ret = psStartPing(reqHandle, pingTarget, ipAddrLen, count, size, timeout, FALSE);
                                        }
                                        else
                                        {
                                            ret = psStartPing(reqHandle, pingTarget, ipAddrLen, count, size, timeout, TRUE);
                                        }

                                        if (ret == CMS_RET_SUCC)
                                        {
                                            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                                        }
                                        else
                                        {
                                            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_UNKNOWN, NULL);
                                    }
                                }
                            }
                        }
                    }
                }

                if (!validparam)
                {
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                }
            }
            break;
        }

        case AT_READ_REQ:           /* AT+PING?  */
        default:
        {
            OsaDebugBegin(FALSE, operaType, 0, 0);
            OsaDebugEnd();

            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId nmECDNS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmECDNS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 urlLen = 0;
    UINT8 pUrl[CMDNS_0_STR_MAX_LEN + 1] = {0};

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+ECDNS= */
        {

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, pUrl, CMDNS_0_STR_MAX_LEN+1, &urlLen, CMDNS_0_STR_DEF)) == AT_PARA_OK)
            {
                if (urlLen > 0)
                {
                    ret = psGetDNS(reqHandle, pUrl);

                    if(ret == CMS_FAIL)
                    {
                        atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_UNKNOWN), NULL);
                    }
                }
                else
                {
                    OsaDebugBegin(FALSE, urlLen, 0, 0);
                    OsaDebugEnd();

                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                }

            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
            }

            break;
        }

        case AT_TEST_REQ:
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, PNULL);
            break;
        }

        case AT_READ_REQ:
        default:
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECIPERF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmECIPERF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32   action, port, protocol, duration, payloadSize, pkgNum, tpt, rptInterval;
    UINT8   strAddressTemp[ECIPERF_3_STR_MAX_LEN + 1] = {0};
    UINT16   addressLen = 0;
    NmIperfReq iperfReq;
    BOOL cmdValid = TRUE;
    memset(&iperfReq, 0, sizeof(NmIperfReq));

    /*
     * AT+ECIPERF=<action>,[protcol],[port],["ipaddr"],[tpt],[payload_size],[pkg_num],[duration],[report_interval]
    */

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+ECIPERF= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &action, ECIPERF_0_ACT_MIN, ECIPERF_0_ACT_MAX, ECIPERF_0_ACT_DEF)) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECIPERF_1, P_WARNING, 1,
                            "AT CMD, ECIPERF, invalid ACT value: %d", action);
                cmdValid = FALSE;
            }

            if (cmdValid == TRUE)
            {
                /*protocol*/
                if((atGetNumValue(pAtCmdReq->pParamList, 1, &protocol, ECIPERF_1_PROTO_MIN, ECIPERF_1_PROTO_MAX, ECIPERF_1_PROTO_DEF)) == AT_PARA_ERR)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECIPERF_2, P_WARNING, 0,
                                "AT CMD, ECIPERF, invalid protocol value, should 0 - 1");
                    cmdValid = FALSE;
                }
            }

            if (cmdValid == TRUE)
            {
                /*port*/
                if((atGetNumValue(pAtCmdReq->pParamList, 2, &port, ECIPERF_2_PORT_MIN, ECIPERF_2_PORT_MAX, ECIPERF_2_PORT_DEF)) == AT_PARA_ERR)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECIPERF_3, P_WARNING, 2,
                                "AT CMD, ECIPERF, invalid port value, should: %d - %d", ECIPERF_2_PORT_MIN, ECIPERF_2_PORT_MAX);
                    cmdValid = FALSE;
                }
            }

            if (cmdValid == TRUE)
            {
                iperfReq.reqAct = (UINT8)action;

                if (action == NM_IPERF_START_CLIENT ||
                    action == NM_IPERF_START_SERVER ||
                    action == NM_IPERF_START_UDP_NAT_SERVER)    //need more parameter
                {
                    iperfReq.protocol   = (UINT8)protocol;
                    iperfReq.port       = (UINT16)port;

                    /*["ipaddr"]*/
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, strAddressTemp, ECIPERF_3_STR_MAX_LEN+1, &addressLen, ECIPERF_3_STR_DEF)) == AT_PARA_OK)
                    {
                        if (addressLen > 0) //get IP ADDR
                        {
                            if (ipaddr_aton((CHAR* )strAddressTemp, &(iperfReq.destAddr)))
                            {
                                iperfReq.destAddrPresent = TRUE;
                            }
                            else
                            {
                                ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECIPERF_4, P_WARNING, 0, "AT CMD, ECIPERF, invalid IP ADDR input");
                            }
                        }
                    }

                    if (action == NM_IPERF_START_CLIENT ||
                        action == NM_IPERF_START_UDP_NAT_SERVER)    /*dest ADDR must*/
                    {
                        if (iperfReq.destAddrPresent == FALSE)
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECIPERF_5, P_WARNING, 0,
                                        "AT CMD, ECIPERF client/UDP NAT server, must dest IP ADDR is missing");
                            cmdValid = FALSE;
                        }
                    }
                }
            }

            /*tpt*/
            if (cmdValid == TRUE)
            {
                /*AT+ECIPERF=<action>,[protcol],[port],["ipaddr"],[tpt],[payload_size],[pkg_num],[duration],[report_interval]*/
                if((atGetNumValue(pAtCmdReq->pParamList, 4, &tpt, ECIPERF_4_TPT_MIN, ECIPERF_4_TPT_MAX, ECIPERF_4_TPT_DEF)) != AT_PARA_ERR)
                {
                    if (action == NM_IPERF_START_CLIENT)
                    {
                        iperfReq.tptPresent = TRUE;
                        iperfReq.tpt        = (UINT32)tpt;
                    }
                }
                else
                {
                    cmdValid = FALSE;
                }

            }

            if (cmdValid == TRUE)
            {
                /*AT+ECIPERF=<action>,[protcol],[port],["ipaddr"],[tpt],[payload_size],[pkg_num],[duration],[report_interval]*/

                /*payload_size*/
                if((atGetNumValue(pAtCmdReq->pParamList, 5, &payloadSize, ECIPERF_5_PAYLOAD_MIN, ECIPERF_5_PAYLOAD_MAX, ECIPERF_5_PAYLOAD_DEF)) != AT_PARA_ERR)
                {
                    iperfReq.payloadSizePresent = TRUE;
                    iperfReq.payloadSize        = (UINT16)payloadSize;
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

                /*pkg_num*/
            if (cmdValid == TRUE)
            {
                if((atGetNumValue(pAtCmdReq->pParamList, 6, &pkgNum, ECIPERF_6_PKG_NUM_MIN, ECIPERF_6_PKG_NUM_MAX, ECIPERF_6_PKG_NUM_DEF)) != AT_PARA_ERR)
                {
                    iperfReq.pkgNumPresent  = TRUE;
                    iperfReq.pkgNum         = (UINT16)pkgNum;
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

                /*duration*/
            if (cmdValid == TRUE)
            {
                if((atGetNumValue(pAtCmdReq->pParamList, 7, &duration, ECIPERF_7_DURATION_MIN, ECIPERF_7_DURATION_MAX, ECIPERF_7_DURATION_DEF)) != AT_PARA_ERR)
                {
                    iperfReq.durationPresent  = TRUE;
                    iperfReq.durationS        = (UINT16)duration;
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

                /*report_interval*/
            if (cmdValid == TRUE)
            {
                if((atGetNumValue(pAtCmdReq->pParamList, 8, &rptInterval, ECIPERF_8_RPT_INTERVAL_MIN, ECIPERF_8_RPT_INTERVAL_MAX, ECIPERF_8_RPT_INTERVAL_DEF)) != AT_PARA_ERR)
                {
                    iperfReq.rptIntervalPresent = TRUE;
                    iperfReq.rptIntervalS       = (UINT16)rptInterval;
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

            if (cmdValid == TRUE)
            {
                if (psIperfReq(reqHandle, &iperfReq) == CMS_RET_SUCC)
                {
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
            }

            break;
        }

        case AT_TEST_REQ:   /*AT+ECIPERF=?*/
        {
            /*
             * +ECIPERF: (list of suppprted <action>s),(list of supported <protcol>s),(list of supported <port>s),
             * (list of supported <tpt>s),(list of supported <payload_size>s),(list of supported <pkg_num>s),
             * (list of supported <duration>s),(list of supported <report_interval>s)
            */
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECIPERF: (0-5),(0,1),(1-65535),(1-1200000),(36-1472),(1-65000),(1-65000),(1-65000)");
            break;
        }

        case AT_READ_REQ:     /* AT+ECIPERF? */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId nmSKTCREATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmSKTCREATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 domain = AF_INET;
    UINT8 type = SOCK_STREAM;
    UINT8 protocol = IPPROTO_IP;
    AtecSktCreateReq *sktCreateReq;

    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTCREATE=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTCREATE: (1,2),(1,2),(6,17)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTCREATE= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, SKTCREATE_0_MIN, SKTCREATE_0_MAX, SKTCREATE_0_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            if(value == 1)
            {
                domain = AF_INET;
            }
            else
            {
                domain = AF_INET6;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &value, SKTCREATE_1_MIN, SKTCREATE_1_MAX, SKTCREATE_1_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            if(value == 1)
            {
                type = SOCK_STREAM;
            }
            else if(value == 2)
            {
                type = SOCK_DGRAM;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &value, SKTCREATE_2_MIN, SKTCREATE_2_MAX, SKTCREATE_2_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            protocol = value;

            if(type == SOCK_DGRAM)
            {
                if(protocol != IPPROTO_UDP)
                {
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                    break;
                }
            }
            else if(type == SOCK_STREAM)
            {
                if(protocol != IPPROTO_TCP)
                {
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                    break;
                }
            }

            sktCreateReq = (AtecSktCreateReq *)malloc(sizeof(AtecSktCreateReq));

            if(sktCreateReq)
            {
                sktCreateReq->domain = domain;
                sktCreateReq->type = type;
                sktCreateReq->protocol = protocol;
                sktCreateReq->reqSource = reqHandle;
                sktCreateReq->eventCallback = (void *)atSktEventCallback;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_CREATE, (void *)sktCreateReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktCreateReq);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+SKTCREATE? */         /* AT+SKTCREATE */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmSKTSEND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmSKTSEND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    INT32 dataLen;
    UINT8 * dataTemp = NULL;
    UINT8 dataRai = ATEC_SKT_DATA_RAI_NO_INFO;
    BOOL dataExcept = FALSE;
    UINT16 len = 0;
    INT32 value;

    AtecSktSendReq *sktSendReq;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTSEND=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTSEND: (0-9),(1-1400),\"data\",(0-2),(0,1)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTSEND= */
        {

#if 0
            NmAtiSyncRet netStatus;
            appGetNetInfoSync(0, &netStatus);
            if(netStatus.body.netInfoRet.netifInfo.netStatus != NM_NETIF_ACTIVATED)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTSEND_1, P_INFO, 0, "net status not activated");
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( SOCKET_CONNECT_FAIL), NULL);
                break;
            }
#endif

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, SKTSEND_0_MIN, SKTSEND_0_MAX, SKTSEND_0_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &dataLen, SKTSEND_1_MIN, SKTSEND_1_MAX, SKTSEND_1_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            dataTemp = malloc(dataLen*2+1);

            if(dataTemp == NULL)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( SOCKET_NO_MEMORY), NULL);
                break;
            }

            memset(dataTemp, 0x00, dataLen*2+1);
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, dataTemp, dataLen*2+1, &len, SKTSEND_2_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            if(strlen((CHAR*)dataTemp) != dataLen *2)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, SKTSEND_3_MIN, SKTSEND_3_MAX, SKTSEND_3_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataRai = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 4, &value, SKTSEND_4_MIN, SKTSEND_4_MAX, SKTSEND_4_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataExcept = value;

            sktSendReq = (AtecSktSendReq *)malloc(sizeof(AtecSktSendReq) + dataLen);

            if(sktSendReq)
            {
                if(cmsHexStrToHex(sktSendReq->data, dataLen, (CHAR *)dataTemp, dataLen*2) != dataLen)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTSEND_4_1, P_WARNING, 0, "the hex string data is not valid");
                     free(sktSendReq);
                     ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                     break;
                }
                sktSendReq->sendLen = dataLen;
                sktSendReq->fd = fd;
                sktSendReq->dataRai = dataRai;
                sktSendReq->dataExpect = dataExcept;
                sktSendReq->reqSource = reqHandle;
            }
            else
            {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTSEND_4, P_WARNING, 0, "send fail,malloc buffer fail");
               ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
               break;
            }

            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_SEND, (void *)sktSendReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktSendReq);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+SKTSEND? */
        default:         /* AT+SKTSEND */
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    if(dataTemp)
        free(dataTemp);
    return ret;
}


/**
  \fn          CmsRetId nmSKTSENDT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+SKTSENDT=<socket_id>[,<length>[,<rai>[,<except>]]]
*/
CmsRetId  nmSKTSENDT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    INT32 dataLen = -1;
    UINT8 dataRai = ATEC_SKT_DATA_RAI_NO_INFO;
    BOOL dataExcept = FALSE;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTSENDT=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTSENDT: (0-9),(1-1400),(0-2),(0,1)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTSENDT= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, SKTSENDT_0_MIN, SKTSENDT_0_MAX, SKTSENDT_0_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &dataLen, SKTSENDT_1_MIN, SKTSENDT_1_MAX, SKTSENDT_1_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, SKTSENDT_2_MIN, SKTSENDT_2_MAX, SKTSENDT_2_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataRai = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, SKTSENDT_3_MIN, SKTSENDT_3_MAX, SKTSENDT_3_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataExcept = value;

            if (pSocketSendInfo == PNULL)
            {
                pSocketSendInfo = (AtSocketSendInfo *)OsaAllocZeroMemory(sizeof(AtSocketSendInfo));
            
                OsaDebugBegin(pSocketSendInfo != PNULL, sizeof(AtSocketSendInfo), 0, 0);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
                break;
                OsaDebugEnd();
            }
            else
            {
                memset(pSocketSendInfo, 0, sizeof(AtSocketSendInfo));
            }
            
            pSocketSendInfo->socketId = fd;
            pSocketSendInfo->reqHander = reqHandle;
            pSocketSendInfo->dataLen = dataLen;
            pSocketSendInfo->raiInfo = dataRai;
            pSocketSendInfo->expectData = dataExcept;
            pSocketSendInfo->requestId = ATECSKTREQ_SEND;
            pSocketSendInfo->source = SOCK_SOURCE_ATSKT;
            

            ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SOCKET_SEND_DATA_STATE);

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
            }

            if (ret == CMS_RET_SUCC)
            {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
            }
            else
            {
                OsaDebugBegin(FALSE, ret, 0, 0);
                OsaDebugEnd();

                nmSocketFreeSendInfo();
                
                atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
            }            

            break;
        }
        case AT_READ_REQ:         /* AT+SKTSENDT? */
        default:         /* AT+SKTSENDT */
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmSKTCONNECT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmSKTCONNECT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    INT32 port;
    UINT16 len = 0;
    UINT8 addressTemp[SKTCONNECT_1_ADDR_STR_MAX_LEN+1] = {0};

    AtecSktConnectReq *sktConnectReq;


    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTCONNECT=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTCONNECT: (0-9),\"IP ADDR\",(1-65535)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTCONNECT= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, SKTCONNECT_0_SEQ_MIN, SKTCONNECT_0_SEQ_MAX, SKTCONNECT_0_SEQ_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, addressTemp, SKTCONNECT_1_ADDR_STR_MAX_LEN+1, &len, SKTCONNECT_1_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &port, SKTCONNECT_2_PORT_MIN, SKTCONNECT_2_PORT_MAX, SKTCONNECT_2_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            sktConnectReq = (AtecSktConnectReq *)malloc(sizeof(AtecSktConnectReq));

            if(sktConnectReq)
            {
                if(ipaddr_aton((const char*)addressTemp, &sktConnectReq->remoteAddr) == 0)
                {
                    free(sktConnectReq);
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                    break;
                }
                sktConnectReq->remotePort = port;
                sktConnectReq->fd = fd;
                if (IP_IS_V4(&sktConnectReq->remoteAddr))
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTCONNECT_1, P_INFO, 4,
                        "%d.%d.%d.%d",
                        ip4_addr1_16(&(sktConnectReq->remoteAddr.u_addr.ip4)),
                        ip4_addr2_16(&(sktConnectReq->remoteAddr.u_addr.ip4)),
                        ip4_addr3_16(&(sktConnectReq->remoteAddr.u_addr.ip4)),
                        ip4_addr4_16(&(sktConnectReq->remoteAddr.u_addr.ip4)));

                 }
                 else
                 {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTCONNECT_2, P_INFO, 8,
                            "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                            IP6_ADDR_BLOCK1(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK2(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK3(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK4(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK5(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK6(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK7(ip_2_ip6(&sktConnectReq->remoteAddr)),
                            IP6_ADDR_BLOCK8(ip_2_ip6(&sktConnectReq->remoteAddr)));
                 }
            }
            else
            {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTCONNECT_3, P_WARNING, 0, "send fail,malloc buffer fail");
               ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
               break;
            }

            sktConnectReq->reqSource = reqHandle;
            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_CONNECT, (void *)sktConnectReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktConnectReq);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }
            break;
        }
        case AT_READ_REQ:         /* AT+SKTCONNECT? */
        default:         /* AT+SKTCONNECT */
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId nmSKTBIND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmSKTBIND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 fd;
    INT32 port;
    BOOL noAddress = FALSE;
    UINT16 len = 0;
    UINT8 addressTemp[SKTBIND_1_ADDR_STR_MAX_LEN+1] = {0};

    AtecSktBindReq *sktBindReq;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTBIND=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTBIND: (0-9),\"IP ADDR\",(1-65535)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTBIND= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, SKTBIND_0_SEQ_MIN, SKTBIND_0_SEQ_MAX, SKTBIND_0_SEQ_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(pParamList[1].bDefault){
                noAddress = TRUE;
            }else{
                if((atGetStrValue(pAtCmdReq->pParamList, 1, addressTemp, SKTBIND_1_ADDR_STR_MAX_LEN+1, &len, SKTBIND_1_ADDR_STR_DEF)) == AT_PARA_ERR)
                {
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                    break;
                }
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &port, SKTBIND_2_PORT_MIN, SKTBIND_2_PORT_MAX, SKTBIND_2_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            sktBindReq = (AtecSktBindReq *)malloc(sizeof(AtecSktBindReq));

            if(sktBindReq)
            {
                if(noAddress == FALSE)
                {
                    if(ipaddr_aton((const char*)addressTemp, &sktBindReq->localAddr) == 0)
                    {
                        free(sktBindReq);
                        ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                        break;
                    }
                    if (IP_IS_V4(&sktBindReq->localAddr))
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTBIND_1, P_INFO, 4,
                            "%d.%d.%d.%d",
                            ip4_addr1_16(&(sktBindReq->localAddr.u_addr.ip4)),
                            ip4_addr2_16(&(sktBindReq->localAddr.u_addr.ip4)),
                            ip4_addr3_16(&(sktBindReq->localAddr.u_addr.ip4)),
                            ip4_addr4_16(&(sktBindReq->localAddr.u_addr.ip4)));

                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTBIND_2, P_INFO, 8,
                            "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                            IP6_ADDR_BLOCK1(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK2(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK3(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK4(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK5(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK6(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK7(ip_2_ip6(&sktBindReq->localAddr)),
                            IP6_ADDR_BLOCK8(ip_2_ip6(&sktBindReq->localAddr)));
                    }
                }
                else
                {
                    ip_addr_set_zero(&sktBindReq->localAddr);
                }
                sktBindReq->localPort = port;
                sktBindReq->fd = fd;
                sktBindReq->reqSource = reqHandle;
            }
            else
            {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTBIND_3, P_WARNING, 0, "send fail,malloc buffer fail");
               ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
               break;
            }

            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_BIND, (void *)sktBindReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktBindReq);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }
            break;
        }

        case AT_READ_REQ:         /* AT+SKTBIND? */
        default:         /* AT+SKTBIND */
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId nmSKTSTATUS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmSKTSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    AtecSktStatusReq *sktStatusReq;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTSTATUS=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTSTATUS: (0-9)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTSTATUS= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, SKTSTATUS_0_SEQ_MIN, SKTSTATUS_0_SEQ_MAX, SKTSTATUS_0_SEQ_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            sktStatusReq = (AtecSktStatusReq *)malloc(sizeof(AtecSktStatusReq));

            if(sktStatusReq)
            {
                sktStatusReq->fd = fd;
                sktStatusReq->reqSource = reqHandle;
            }
            else
            {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTSTATUS_1, P_WARNING, 0, "send fail,malloc buffer fail");
               ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
               break;
            }

            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_STATUS, (void *)sktStatusReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktStatusReq);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }
            break;
        }

        case AT_READ_REQ:         /* AT+SKTSTATUS? */
        default:         /* AT+SKTSTATUS */
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId nmSKTDELETE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmSKTDELETE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    AtecSktDeleteReq *sktDeleteReq;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+SKTDELETE=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+SKTDELETE: (0-9)");
            break;
        }

        case AT_SET_REQ:         /* AT+SKTDELETE= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, SKTDELETE_0_SEQ_MIN, SKTDELETE_0_SEQ_MAX, SKTDELETE_0_SEQ_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            sktDeleteReq = (AtecSktDeleteReq *)malloc(sizeof(AtecSktDeleteReq));

            if(sktDeleteReq) {
                sktDeleteReq->fd = fd;
                sktDeleteReq->reqSource = reqHandle;
            }else {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTDELETE_1, P_WARNING, 0, "send fail,malloc buffer fail");
               ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
               break;
            }

            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_DELETE, (void *)sktDeleteReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktDeleteReq);
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }
            break;
        }

        case AT_READ_REQ:         /* AT+SKTDELETE? */
        default:         /* AT+SKTDELETE */
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId nmECDNSCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       Set/Get default DNS config
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  nmECDNSCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32              reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32     operaType = (UINT32)pAtCmdReq->operaType;
    MidWareDefaultDnsCfg    dnsCfg; //40 bytes

    memset(&dnsCfg, 0x00, sizeof(MidWareDefaultDnsCfg));

    /*AT+ECDNSCFG=<dns_1>[,<dns_2>[,<dns_3>[,<dns_4>]]]*/
    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+ECDNSCFG= */
        {
            ip_addr_t   ipAddr; //20 bytes
            UINT8   strDnsTemp[ECDNSCFG_DNS_STR_MAX_LEN + 1] = {0};    //64 bytes
            UINT8   paraIdx = 0, validIpv4DnsNum = 0, validIpv6DnsNum = 0;
            UINT16  ipv6U16 = 0;
            BOOL    validDns = TRUE;
            UINT16 len = 0;

            while (paraIdx < ECDNSCFG_DNS_NUM &&
                   paraIdx < pAtCmdReq->paramRealNum)
            {
                memset(strDnsTemp, 0x00, ECDNSCFG_DNS_STR_MAX_LEN + 1);
                if((ret = atGetStrValue(pAtCmdReq->pParamList, paraIdx, strDnsTemp, ECDNSCFG_DNS_STR_MAX_LEN+1, &len, ECDNSCFG_DNS_STR_DEF)) == AT_PARA_OK)
                {
                    memset(&ipAddr, 0x00, sizeof(ip_addr_t));

                    if (len != 0)
                    {
                        if(ipaddr_aton((CHAR* )strDnsTemp, &ipAddr) == TRUE)
                        {
                            if (ipAddr.type == IPADDR_TYPE_V4)
                            {
                                if (ip4_addr_isany(&(ipAddr.u_addr.ip4)))
                                {
                                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECDNSCFG_ipv4_any, P_WARNING, 1,
                                            "AT CMD, invalid ipv4 addr: 0x%x, in ECDNSCFG", ipAddr.u_addr.ip4.addr);
                                    validDns = FALSE;
                                    break;
                                }

                                if (validIpv4DnsNum >= MID_WARE_DEFAULT_DNS_NUM)
                                {
                                    OsaDebugBegin(FALSE, validIpv4DnsNum, 0, 0);
                                    OsaDebugEnd();
                                    validIpv4DnsNum = MID_WARE_DEFAULT_DNS_NUM - 1;
                                }

                                dnsCfg.ipv4Dns[validIpv4DnsNum][0] = ip4_addr1(&(ipAddr.u_addr.ip4));
                                dnsCfg.ipv4Dns[validIpv4DnsNum][1] = ip4_addr2(&(ipAddr.u_addr.ip4));
                                dnsCfg.ipv4Dns[validIpv4DnsNum][2] = ip4_addr3(&(ipAddr.u_addr.ip4));
                                dnsCfg.ipv4Dns[validIpv4DnsNum][3] = ip4_addr4(&(ipAddr.u_addr.ip4));

                                //whether the same as the first one
                                if (validIpv4DnsNum > 0)
                                {
                                    if (memcmp(dnsCfg.ipv4Dns[0], dnsCfg.ipv4Dns[validIpv4DnsNum], MID_WARE_IPV4_ADDR_LEN) != 0)
                                    {
                                        validIpv4DnsNum++;
                                    }
                                    else
                                    {
                                        //just the duplicate, clear it
                                        memset(dnsCfg.ipv4Dns[validIpv4DnsNum], 0x00, MID_WARE_IPV4_ADDR_LEN);
                                    }
                                }
                                else
                                {
                                    validIpv4DnsNum++;
                                }
                            }
                            else if (ipAddr.type == IPADDR_TYPE_V6)
                            {
                                if (ip6_addr_isany(&(ipAddr.u_addr.ip6)))
                                {
                                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECDNSCFG_ipv6_any, P_WARNING, 4,
                                            "AT CMD, invalid ipv6 addr: 0x%x, 0x%x, 0x%x, 0x%x, in ECDNSCFG",
                                            ipAddr.u_addr.ip6.addr[0], ipAddr.u_addr.ip6.addr[1], ipAddr.u_addr.ip6.addr[2], ipAddr.u_addr.ip6.addr[3]);
                                    validDns = FALSE;
                                    break;
                                }

                                if (validIpv6DnsNum >= MID_WARE_DEFAULT_DNS_NUM)
                                {
                                    OsaDebugBegin(FALSE, validIpv6DnsNum, 0, 0);
                                    OsaDebugEnd();
                                    validIpv6DnsNum = MID_WARE_DEFAULT_DNS_NUM - 1;
                                }

                                ipv6U16 = IP6_ADDR_BLOCK1(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][0] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][1] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK2(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][2] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][3] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK3(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][4] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][5] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK4(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][6] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][7] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK5(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][8] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][9] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK6(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][10] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][11] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK7(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][12] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][13] = (UINT8)(ipv6U16 & 0xFF);
                                ipv6U16 = IP6_ADDR_BLOCK8(&(ipAddr.u_addr.ip6));
                                dnsCfg.ipv6Dns[validIpv6DnsNum][14] = (UINT8)((ipv6U16 >> 8)&0xFF);
                                dnsCfg.ipv6Dns[validIpv6DnsNum][15] = (UINT8)(ipv6U16 & 0xFF);

                                //whether the same as the first one
                                if (validIpv6DnsNum > 0)
                                {
                                    if (memcmp(dnsCfg.ipv6Dns[0], dnsCfg.ipv6Dns[validIpv6DnsNum], MID_WARE_IPV6_ADDR_LEN) != 0)
                                    {
                                        validIpv6DnsNum++;
                                    }
                                    else
                                    {
                                        //duplicated, clear it
                                        memset(dnsCfg.ipv6Dns[validIpv6DnsNum], 0x00, MID_WARE_IPV6_ADDR_LEN);
                                    }
                                }
                                else
                                {
                                    validIpv6DnsNum++;
                                }
                            }
                            else
                            {
                                ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECDNSCFG_invalid_type, P_WARNING, 1,
                                        "AT CMD, invalid addr type: %d, in ECDNSCFG", ipAddr.type);
                                validDns = FALSE;
                                break;
                            }
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmECDNSCFG_invalid_ip, P_WARNING, 0,
                                     "AT CMD, empty DNS, in ECDNSCFG");
                        continue;
                    }
                }
                paraIdx++;
            }

            if (validDns) //save into NVM
            {
                mwSetAndSaveDefaultDnsConfig(&dnsCfg);

                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_INCORRECT_PARAM), NULL);
            }

            break;
        }

        case AT_READ_REQ:     /* AT+ECDNSCFG? */
        {
            CHAR    *pRspStr = PNULL;
            CHAR    tmpStr[ATEC_ECDNSCFG_GET_CNF_TMP_STR_LEN];
            UINT8   tmpIdx = 0;

            pRspStr = (CHAR *)OsaAllocZeroMemory(ATEC_ECDNSCFG_GET_CNF_STR_LEN);

            if (pRspStr == PNULL)
            {
                OsaDebugBegin(FALSE, ATEC_ECDNSCFG_GET_CNF_STR_LEN, 0, 0);
                OsaDebugEnd();

                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_UE_FAIL), NULL);
            }
            else
            {
                mwGetDefaultDnsConfig(&dnsCfg);

                //snprintf(pRspStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN, "+ECDNSCFG: ");

                /*ipv4 DNS*/
                for (tmpIdx = 0; tmpIdx < MID_WARE_DEFAULT_DNS_NUM; tmpIdx++)
                {
                    if (dnsCfg.ipv4Dns[tmpIdx][0] != 0)
                    {
                        if (strlen(pRspStr) == 0)
                        {
                            snprintf(pRspStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN, "+ECDNSCFG: \"%d.%d.%d.%d\"",
                                     dnsCfg.ipv4Dns[tmpIdx][0], dnsCfg.ipv4Dns[tmpIdx][1], dnsCfg.ipv4Dns[tmpIdx][2], dnsCfg.ipv4Dns[tmpIdx][3]);
                        }
                        else
                        {
                            memset(tmpStr, 0x00, ATEC_ECDNSCFG_GET_CNF_TMP_STR_LEN);
                            snprintf(tmpStr, ATEC_ECDNSCFG_GET_CNF_TMP_STR_LEN, ",\"%d.%d.%d.%d\"",
                                     dnsCfg.ipv4Dns[tmpIdx][0], dnsCfg.ipv4Dns[tmpIdx][1], dnsCfg.ipv4Dns[tmpIdx][2], dnsCfg.ipv4Dns[tmpIdx][3]);

                            strncat(pRspStr, tmpStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN - strlen(pRspStr) - 1);
                        }
                    }
                }

                /*ipv6 DNS*/
                for (tmpIdx = 0; tmpIdx < MID_WARE_DEFAULT_DNS_NUM; tmpIdx++)
                {
                    if (dnsCfg.ipv6Dns[tmpIdx][0] != 0 ||
                        dnsCfg.ipv6Dns[tmpIdx][1] != 0)
                    {
                        if (strlen(pRspStr) == 0)
                        {
                            snprintf(pRspStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN, "+ECDNSCFG: \"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\"",
                                     dnsCfg.ipv6Dns[tmpIdx][0], dnsCfg.ipv6Dns[tmpIdx][1], dnsCfg.ipv6Dns[tmpIdx][2], dnsCfg.ipv6Dns[tmpIdx][3],
                                     dnsCfg.ipv6Dns[tmpIdx][4], dnsCfg.ipv6Dns[tmpIdx][5], dnsCfg.ipv6Dns[tmpIdx][6], dnsCfg.ipv6Dns[tmpIdx][7],
                                     dnsCfg.ipv6Dns[tmpIdx][8], dnsCfg.ipv6Dns[tmpIdx][9], dnsCfg.ipv6Dns[tmpIdx][10], dnsCfg.ipv6Dns[tmpIdx][11],
                                     dnsCfg.ipv6Dns[tmpIdx][12], dnsCfg.ipv6Dns[tmpIdx][13], dnsCfg.ipv6Dns[tmpIdx][14], dnsCfg.ipv6Dns[tmpIdx][15]);
                        }
                        else
                        {
                            memset(tmpStr, 0x00, ATEC_ECDNSCFG_GET_CNF_TMP_STR_LEN);
                            snprintf(tmpStr, ATEC_ECDNSCFG_GET_CNF_TMP_STR_LEN, ",\"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\"",
                                     dnsCfg.ipv6Dns[tmpIdx][0], dnsCfg.ipv6Dns[tmpIdx][1], dnsCfg.ipv6Dns[tmpIdx][2], dnsCfg.ipv6Dns[tmpIdx][3],
                                     dnsCfg.ipv6Dns[tmpIdx][4], dnsCfg.ipv6Dns[tmpIdx][5], dnsCfg.ipv6Dns[tmpIdx][6], dnsCfg.ipv6Dns[tmpIdx][7],
                                     dnsCfg.ipv6Dns[tmpIdx][8], dnsCfg.ipv6Dns[tmpIdx][9], dnsCfg.ipv6Dns[tmpIdx][10], dnsCfg.ipv6Dns[tmpIdx][11],
                                     dnsCfg.ipv6Dns[tmpIdx][12], dnsCfg.ipv6Dns[tmpIdx][13], dnsCfg.ipv6Dns[tmpIdx][14], dnsCfg.ipv6Dns[tmpIdx][15]);

                            strncat(pRspStr, tmpStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN - strlen(pRspStr) - 1);
                        }

                    }
                }

                if (strlen(pRspStr) == 0)
                {
                    snprintf(pRspStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN, "+ECDNSCFG: \"\"");
                }

                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, pRspStr);

                OsaFreeMemory(&pRspStr);
            }

            break;
        }

        case AT_TEST_REQ:
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, PNULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSOCR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOCR=<type>,<protocol>,<listen_port>[,<receive_control>[,<af_type>[,<ip_address>]]]
*/
CmsRetId  nmECSOCR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 domain = AF_INET;
    UINT8 type = SOCK_STREAM;
    UINT8 protocol = IPPROTO_IP;
    UINT16 listenPort = 0;
    UINT8 receiveControl = 1;
    EcSocCreateReq *socCreateReq;
    UINT8 tmpValue[ECSOCR_0_TYPE_STR_MAX_LEN+1] = {0};
    UINT8 tmpDomain[ECSOCR_4_AF_TYPE_STR_MAX_LEN +1] = {0};
    UINT8 tmpAddress[ECSOCR_5_IP_ADDR_STR_MAX_LEN+1] = {0};
    UINT16 len = 0;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOCR=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (DGRAM,STREAM),(6,17),(0-65535),(0,1),(AF_INET,AF_INET6),(ip address)", ECSOCR_NAME); 
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOCR= */
        {

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, tmpValue, ECSOCR_0_TYPE_STR_MAX_LEN+1, &len, ECSOCR_0_TYPE_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR *)tmpValue , "DGRAM") == 0)
                {
                    type = SOCK_DGRAM;
                }
                else if(strcmp((CHAR *)tmpValue , "STREAM") == 0)
                {
                    type = SOCK_STREAM;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSOCR_1_PROTOCOL_MIN, ECSOCR_1_PROTOCOL_MAX, ECSOCR_1_PROTOCOL_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            protocol = value;
            if(type == SOCK_DGRAM)
            {
                if(protocol != IPPROTO_UDP)
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
            }
            else if(type == SOCK_STREAM)
            {
                if(protocol != IPPROTO_TCP)
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSOCR_2_LISTEN_PORT_MIN, ECSOCR_2_LISTEN_PORT_MAX, ECSOCR_2_LISTEN_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            listenPort = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECSOCR_3_RECEIVE_CONTROL_MIN, ECSOCR_3_RECEIVE_CONTROL_MAX, ECSOCR_3_RECEIVE_CONTROL_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            receiveControl = value;

            if((atGetStrValue(pAtCmdReq->pParamList, 4, tmpDomain, ECSOCR_4_AF_TYPE_STR_MAX_LEN+1, &len, ECSOCR_4_AF_TYPE_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR *)tmpDomain , "AF_INET") == 0)
                {
                    domain = AF_INET;
                }
                else if(strcmp((CHAR *)tmpDomain , "AF_INET6") == 0)
                {
                    domain = AF_INET6;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
            }

            if((atGetStrValue(pAtCmdReq->pParamList, 5, tmpAddress, ECSOCR_5_IP_ADDR_STR_MAX_LEN+1, &len, ECSOCR_5_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            socCreateReq = (EcSocCreateReq *)malloc(sizeof(EcSocCreateReq));
            if(socCreateReq)
            {
                socCreateReq->domain = domain;
                socCreateReq->type = type;
                socCreateReq->protocol = protocol;
                socCreateReq->listenPort = listenPort;
                socCreateReq->receiveControl = receiveControl;
                socCreateReq->reqSource = reqHandle;
                socCreateReq->eventCallback = (void *)atEcsocEventCallback;
                if(len > 0)
                {
                    if(ipaddr_aton((const char*)tmpAddress, &socCreateReq->localAddr) == 0)
                    {
                        free(socCreateReq);
                        ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                        break;
                    }
                    else
                    {
                        if(domain == AF_INET)
                        {
                            if(IP_IS_V6_VAL(socCreateReq->localAddr) && (!ip_addr_isany_val(socCreateReq->localAddr)))
                            {
                                free(socCreateReq);
                                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                                break;                                
                            }
                        }
                        else if(domain == AF_INET6)
                        {
                            if(IP_IS_V4_VAL(socCreateReq->localAddr) && (!ip_addr_isany_val(socCreateReq->localAddr)))
                            {
                                free(socCreateReq);
                                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                                break;                            
                            }
                        }    
                    }
                }
                else
                {
                    ip_addr_set_zero(&socCreateReq->localAddr);
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_CREATE, (void *)socCreateReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socCreateReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSOCR? */         /* AT+ECSOCR */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSOST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOST=<socket_id>,<remote_addr>,<remote_port>,<length>,<data>[,<sequence>[,<segment_id>,<segment_num>]]
*/
CmsRetId  nmECSOST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 length;
    INT32 socketId = AT_SOC_FD_DEF;
    UINT16 remotePort = 0;
    UINT8 sequence = 0;
    UINT8 segmentId = 0;
    UINT8 segmentNum = 0;
    EcSocUdpSendReq *socUdpSendReq;
    UINT8 * dataTemp = NULL;
    UINT8 tmpValue[ECSOST_1_IP_ADDR_STR_MAX_LEN + 1] = {0};
    INT32 value;
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOCR=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(ip address),(1-65535),(1-1400),(data),(1-255),(1-4),(2-4)", ECSOST_NAME);        
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOCR= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSOST_0_SOCKET_ID_MIN, ECSOST_0_SOCKET_ID_MAX, ECSOST_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, tmpValue, ECSOST_1_IP_ADDR_STR_MAX_LEN+1, &len, ECSOST_1_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSOST_2_REMOTE_PORT_MIN, ECSOST_2_REMOTE_PORT_MAX, ECSOST_2_REMOTE_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            remotePort = value;

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECSOST_3_LENGTH_MIN, ECSOST_3_LENGTH_MAX, ECSOST_3_LENGTH_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            length = value;

            dataTemp = malloc(length*2+1);
            if(dataTemp == NULL)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, SOCKET_NO_MEMORY, NULL);
                break;
            }

            memset(dataTemp, 0x00, length*2+1);
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 4, dataTemp, length*2+1, &len, ECSOST_4_DATA_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if(strlen((CHAR*)dataTemp) != length *2)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 5, &value, ECSOST_5_SEQUENCE_MIN, ECSOST_5_SEQUENCE_MAX, ECSOST_5_SEQUENCE_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            sequence = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 6, &value, ECSOST_6_SEGMENT_ID_MIN, ECSOST_6_SEGMENT_ID_MAX, ECSOST_6_SEGMENT_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            segmentId = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 7, &value, ECSOST_7_SEGMENT_NUM_MIN, ECSOST_7_SEGMENT_NUM_MAX, ECSOST_7_SEGMENT_NUM_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            segmentNum = value;

            if(segmentId > segmentNum)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            socUdpSendReq = (EcSocUdpSendReq *)malloc(sizeof(EcSocUdpSendReq)+length);
            if(socUdpSendReq)
            {
                socUdpSendReq->socketId = socketId;
                socUdpSendReq->remotePort = remotePort;
                socUdpSendReq->length = length;
                socUdpSendReq->raiInfo = 0;
                socUdpSendReq->exceptionFlag = 0;
                socUdpSendReq->sequence = sequence;
                socUdpSendReq->segmentId = segmentId;
                socUdpSendReq->segmentNum = segmentNum;
                socUdpSendReq->reqSource =reqHandle;
                if(ipaddr_aton((const char*)tmpValue, &socUdpSendReq->remoteAddr) == 0)
                {
                    free(socUdpSendReq);
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
                if(cmsHexStrToHex(socUdpSendReq->data, length, (CHAR* )dataTemp, length*2) != length)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmECSOST_1, P_WARNING, 0, "the hex string data is not valid");
                     free(socUdpSendReq);
                     ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                     break;
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_UDPSEND, (void *)socUdpSendReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socUdpSendReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSOST? */         /* AT+ECSOST */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    if(dataTemp)
        free(dataTemp);

    return ret;
}


/**
  \fn          CmsRetId nmECSOSTT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOSTT=<socket_id>,<remote_addr>,<remote_port>[,<length>[,<sequence>]]
*/
CmsRetId  nmECSOSTT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 len;
    INT32 fd;
    INT32 dataLen = -1;
    UINT16 remotePort;
    UINT8 sequence = 0;
    UINT8 tmpValue[ECSOST_1_IP_ADDR_STR_MAX_LEN + 1] = {0};
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOSTT=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(ip address),(1-65535),(1-1400),(1-255)", ECSOSTT_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOSTT= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, ECSOSTT_0_SOCKET_ID_MIN, ECSOSTT_0_SOCKET_ID_MAX, ECSOSTT_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, tmpValue, ECSOSTT_1_IP_ADDR_STR_MAX_LEN+1, &len, ECSOSTT_1_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }            

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSOSTT_2_REMOTE_PORT_MIN, ECSOSTT_2_REMOTE_PORT_MAX, ECSOSTT_2_REMOTE_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            remotePort = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECSOSTT_3_LENGTH_MIN, ECSOSTT_3_LENGTH_MAX, ECSOSTT_3_LENGTH_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataLen = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 4, &value, ECSOSTT_4_SEQUENCE_MIN, ECSOSTT_4_SEQUENCE_MAX, ECSOSTT_4_SEQUENCE_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            sequence = value;

            if (pSocketSendInfo == PNULL)
            {
                pSocketSendInfo = (AtSocketSendInfo *)OsaAllocZeroMemory(sizeof(AtSocketSendInfo));
            
                OsaDebugBegin(pSocketSendInfo != PNULL, sizeof(AtSocketSendInfo), 0, 0);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
                break;
                OsaDebugEnd();
            }
            else
            {
                memset(pSocketSendInfo, 0, sizeof(AtSocketSendInfo));
            }
            
            pSocketSendInfo->socketId = fd;
            pSocketSendInfo->reqHander = reqHandle;
            pSocketSendInfo->dataLen = dataLen;
            pSocketSendInfo->sequence = sequence;
            pSocketSendInfo->remotePort = remotePort;
            pSocketSendInfo->requestId = ECSOCREQ_UDPSEND;
            pSocketSendInfo->source = SOCK_SOURCE_ECSOC;

            if(ipaddr_aton((const char*)tmpValue, &pSocketSendInfo->remoteAddr) == 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }            

            ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SOCKET_SEND_DATA_STATE);

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
            }

            if (ret == CMS_RET_SUCC)
            {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
            }
            else
            {
                OsaDebugBegin(FALSE, ret, 0, 0);
                OsaDebugEnd();

                nmSocketFreeSendInfo();
                
                atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
            }            

            break;
        }
        case AT_READ_REQ:         /* AT+ECSOSTT? */
        default:         /* AT+ECSOSTT */
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSOSTF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOSTF=<socket_id>,<remote_addr>,<remote_port>,<flag>,<length>,<data>[,<sequence>[,<segment_id>,<segment_num>]]
*/
CmsRetId  nmECSOSTF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 length;
    INT32 socketId = AT_SOC_FD_DEF;
    UINT16 remotePort = 0;
    UINT8 exceptionFlag = 0;
    UINT8 raiInfo = 0;
    UINT8 sequence = 0;
    UINT8 segmentId = 1;
    UINT8 segmentNum = 0;
    EcSocUdpSendReq *socUdpSendReq;
    UINT8 * dataTemp = NULL;
    UINT8 tmpValue[ECSOSTF_1_IP_ADDR_STR_MAX_LEN + 1] = {0};
    UINT8 flagValue[ECSOSTF_3_FLAG_STR_MAX_LEN + 1] = {0};
    INT32 value;
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOSTF=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(ip address),(1-65535),(0,0x100,0x200,0x400),(1-1400),(data),(1-255),(1-4),(2-4)", ECSOSTF_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOSTF= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSOSTF_0_SOCKET_ID_MIN, ECSOSTF_0_SOCKET_ID_MAX, ECSOSTF_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, tmpValue, ECSOSTF_1_IP_ADDR_STR_MAX_LEN+1, &len, ECSOSTF_1_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSOSTF_2_REMOTE_PORT_MIN, ECSOSTF_2_REMOTE_PORT_MAX, ECSOSTF_2_REMOTE_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            remotePort = value;

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, flagValue, ECSOSTF_3_FLAG_STR_MAX_LEN+1, &len, ECSOSTF_3_FLAG_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR*)flagValue, "0x100") == 0)
                {
                    exceptionFlag = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x200") == 0)
                {
                    raiInfo = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x400") == 0)
                {
                    raiInfo = 2;
                }
                else if(strcmp((CHAR*)flagValue, "0") == 0)
                {
                    raiInfo = 0;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
           }
           else
           {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }            

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &value, ECSOSTF_4_LENGTH_MIN, ECSOSTF_4_LENGTH_MAX, ECSOSTF_4_LENGTH_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            length = value;

            dataTemp = malloc(length*2+1);
            if(dataTemp == NULL)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, SOCKET_NO_MEMORY, NULL);
                break;
            }

            memset(dataTemp, 0x00, length*2+1);
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 5, dataTemp, length*2+1, &len, ECSOSTF_5_DATA_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if(strlen((CHAR*)dataTemp) != length *2)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
 

            if((atGetNumValue(pAtCmdReq->pParamList, 6, &value, ECSOSTF_6_SEQUENCE_MIN, ECSOSTF_6_SEQUENCE_MAX, ECSOSTF_6_SEQUENCE_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            sequence = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 7, &value, ECSOSTF_7_SEGMENT_ID_MIN, ECSOSTF_7_SEGMENT_ID_MAX, ECSOSTF_7_SEGMENT_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            segmentId = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 8, &value, ECSOSTF_8_SEGMENT_NUM_MIN, ECSOSTF_8_SEGMENT_NUM_MAX, ECSOSTF_8_SEGMENT_NUM_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            segmentNum = value;

            if(segmentId > segmentNum)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            socUdpSendReq = (EcSocUdpSendReq *)malloc(sizeof(EcSocUdpSendReq)+length);
            if(socUdpSendReq)
            {
                socUdpSendReq->socketId = socketId;
                socUdpSendReq->remotePort = remotePort;
                socUdpSendReq->length = length;
                socUdpSendReq->exceptionFlag = exceptionFlag;
                socUdpSendReq->raiInfo = raiInfo;
                socUdpSendReq->sequence = sequence;
                socUdpSendReq->segmentId = segmentId;
                socUdpSendReq->segmentNum = segmentNum;
                socUdpSendReq->reqSource =reqHandle;
                if(ipaddr_aton((const char*)tmpValue, &socUdpSendReq->remoteAddr) == 0)
                {
                    free(socUdpSendReq);
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
                if(cmsHexStrToHex(socUdpSendReq->data, length, (CHAR *)dataTemp, length*2) != length)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmECSOSTF_1, P_WARNING, 0, "the hex string data is not valid");
                     free(socUdpSendReq);
                     ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                     break;
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_UDPSENDF, (void *)socUdpSendReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socUdpSendReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSOST? */         /* AT+ECSOST */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    if(dataTemp)
        free(dataTemp);

    return ret;
}



/**
  \fn          CmsRetId nmECSOSTFT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmECSOSTFT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 len;
    INT32 fd;
    INT32 dataLen = -1;
    UINT8 exceptionFlag = 0;
    UINT8 raiInfo = 0;    
    UINT16 remotePort;
    UINT8 sequence = 0;
    UINT8 tmpValue[ECSOST_1_IP_ADDR_STR_MAX_LEN + 1] = {0};
    UINT8 flagValue[ECSOSTF_3_FLAG_STR_MAX_LEN + 1] = {0};
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOSTFT=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(ip address),(1-65535),(0,0x100,0x200,0x400),(1-1400),(1-255)", ECSOSTFT_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOSTFT= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, ECSOSTFT_0_SOCKET_ID_MIN, ECSOSTFT_0_SOCKET_ID_MAX, ECSOSTFT_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, tmpValue, ECSOSTFT_1_IP_ADDR_STR_MAX_LEN+1, &len, ECSOSTFT_1_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }            

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSOSTFT_2_REMOTE_PORT_MIN, ECSOSTFT_2_REMOTE_PORT_MAX, ECSOSTFT_2_REMOTE_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            remotePort = value;

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, flagValue, ECSOSTFT_3_FLAG_STR_MAX_LEN+1, &len, ECSOSTFT_3_FLAG_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR*)flagValue, "0x100") == 0)
                {
                    exceptionFlag = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x200") == 0)
                {
                    raiInfo = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x400") == 0)
                {
                    raiInfo = 2;
                }
                else if(strcmp((CHAR*)flagValue, "0") == 0)
                {
                    raiInfo = 0;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
           }
           else
           {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
           }


            if((atGetNumValue(pAtCmdReq->pParamList, 4, &value, ECSOSTFT_4_LENGTH_MIN, ECSOSTFT_4_LENGTH_MAX, ECSOSTFT_4_LENGTH_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataLen = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 5, &value, ECSOSTFT_5_SEQUENCE_MIN, ECSOSTFT_5_SEQUENCE_MAX, ECSOSTFT_5_SEQUENCE_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            sequence = value;

            if (pSocketSendInfo == PNULL)
            {
                pSocketSendInfo = (AtSocketSendInfo *)OsaAllocZeroMemory(sizeof(AtSocketSendInfo));
            
                OsaDebugBegin(pSocketSendInfo != PNULL, sizeof(AtSocketSendInfo), 0, 0);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
                break;
                OsaDebugEnd();
            }
            else
            {
                memset(pSocketSendInfo, 0, sizeof(AtSocketSendInfo));
            }
            
            pSocketSendInfo->socketId = fd;
            pSocketSendInfo->reqHander = reqHandle;
            pSocketSendInfo->dataLen = dataLen;
            pSocketSendInfo->sequence = sequence;
            pSocketSendInfo->remotePort = remotePort;
            pSocketSendInfo->expectData = exceptionFlag;
            pSocketSendInfo->raiInfo = raiInfo;
            pSocketSendInfo->requestId = ECSOCREQ_UDPSENDF;
            pSocketSendInfo->source = SOCK_SOURCE_ECSOC;

            if(ipaddr_aton((const char*)tmpValue, &pSocketSendInfo->remoteAddr) == 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }            

            ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SOCKET_SEND_DATA_STATE);

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
            }

            if (ret == CMS_RET_SUCC)
            {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
            }
            else
            {
                OsaDebugBegin(FALSE, ret, 0, 0);
                OsaDebugEnd();

                nmSocketFreeSendInfo();
                
                atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
            }            

            break;
        }
        case AT_READ_REQ:         /* AT+ECSOSTFT? */
        default:         /* AT+ECSOSTFT */
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECQSOS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECQSOS=<socket_id>[,<socket>[,<socket>[,<socket>[...]]]]
*/
CmsRetId  nmECQSOS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSocQueryReq *socQueryReq;
    INT32 value = AT_SOC_FD_DEF;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECQSOS=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(0-9),(0-9),(0-9),(0-9)", ECQSOS_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECQSOS= */
        {

            socQueryReq = (EcSocQueryReq *)malloc(sizeof(EcSocQueryReq));
            if(socQueryReq)
            {
                memset(socQueryReq, 0, sizeof(EcSocQueryReq));
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECQSOS_0_SOCKET1_ID_MIN, ECQSOS_0_SOCKET1_ID_MAX, ECQSOS_0_SOCKET1_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socQueryReq->socketId[0] = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECQSOS_1_SOCKET2_ID_MIN, ECQSOS_1_SOCKET2_ID_MAX, ECQSOS_1_SOCKET2_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socQueryReq->socketId[1] = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECQSOS_2_SOCKET3_ID_MIN, ECQSOS_2_SOCKET3_ID_MAX, ECQSOS_2_SOCKET3_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socQueryReq->socketId[2] = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECQSOS_3_SOCKET4_ID_MIN, ECQSOS_3_SOCKET4_ID_MAX, ECQSOS_3_SOCKET4_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socQueryReq->socketId[3] = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 4, &value, ECQSOS_4_SOCKET5_ID_MIN, ECQSOS_4_SOCKET5_ID_MAX, ECQSOS_4_SOCKET5_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socQueryReq->socketId[4] = value;
            socQueryReq->bQueryAll = FALSE;
            socQueryReq->reqSource = reqHandle;

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_QUERY, (void *)socQueryReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socQueryReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECQSOS? */         /* AT+ECQSOS */
            ret = CMS_RET_SUCC;
            socQueryReq = (EcSocQueryReq *)malloc(sizeof(EcSocQueryReq));
            if(socQueryReq)
            {
                memset(socQueryReq, 0, sizeof(EcSocQueryReq));
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }
            
            socQueryReq->bQueryAll = TRUE;
            socQueryReq->reqSource = reqHandle;

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_QUERY, (void *)socQueryReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socQueryReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;            
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSORF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSORF=<socket_id>,<req_length>
*/
CmsRetId  nmECSORF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSocReadReq *socReadReq;
    INT32 socketId = AT_SOC_FD_DEF;
    UINT16 length = 0;
    INT32 value;


    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSORF=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(1-1358)", ECSORF_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSORF= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSORF_0_SOCKET_ID_MIN, ECSORF_0_SOCKET_ID_MAX, ECSORF_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSORF_1_LENGTH_ID_MIN, ECSORF_1_LENGTH_ID_MAX, ECSORF_1_LENGTH_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            length = value;

            socReadReq = (EcSocReadReq *)malloc(sizeof(EcSocReadReq));
            if(socReadReq)
            {
                socReadReq->socketId = socketId;
                socReadReq->length = length;
                socReadReq->reqSource =reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_READ, (void *)socReadReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socReadReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECQSOS? */         /* AT+ECQSOS */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSOCO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOCO=<socket_id>,<remote_addr>,<remote_port>
*/
CmsRetId  nmECSOCO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    UINT16 remotePort = 0;
    INT32 socketId = AT_SOC_FD_DEF;
    EcSocTcpConnectReq *socTcpConnectReq;
    UINT8 tmpValue[ECSOCO_1_IP_ADDR_STR_MAX_LEN + 1] = {0};
    INT32 value;
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOCO=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(ip address),(1-65535)", ECSOCO_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOCO= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSOCO_0_SOCKET_ID_MIN, ECSOCO_0_SOCKET_ID_MAX, ECSOCO_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, tmpValue, ECSOCO_1_IP_ADDR_STR_MAX_LEN+1, &len, ECSOCO_1_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSOCO_2_REMOTE_PORT_MIN, ECSOCO_2_REMOTE_PORT_MAX, ECSOCO_2_REMOTE_PORT_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            remotePort = value;


            socTcpConnectReq = (EcSocTcpConnectReq *)malloc(sizeof(EcSocTcpConnectReq));
            if(socTcpConnectReq)
            {
                socTcpConnectReq->socketId = socketId;
                socTcpConnectReq->remotePort = remotePort;
                socTcpConnectReq->reqSource = reqHandle;
                if(ipaddr_aton((const char*)tmpValue, &socTcpConnectReq->remoteAddr) == 0)
                {
                    free(socTcpConnectReq);
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }

            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_TCPCONNECT, (void *)socTcpConnectReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socTcpConnectReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSOCO? */         /* AT+ECSOCO */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSOSD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOSTF=<socket_id>,<length>,<data>[,<flag>[,<sequence>]]
*/
CmsRetId  nmECSOSD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    UINT16 length;
    INT32 socketId = AT_SOC_FD_DEF;
    UINT8 expectionFlag = 0;
    UINT8 raiInfo = 0;
    UINT8 sequence = 0;
    EcSocTcpSendReq *socTcpSendReq;
    UINT8 *dataTemp = NULL;
    INT32 value;
    UINT8 flagValue[ECSOSD_3_FLAG_STR_MAX_LEN + 1] = {0};
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOSD=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(1-1400),(data),(0,0x100,0x200,0x400),(1-255)", ECSOSD_NAME);   
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOSD= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSOSD_0_SOCKET_ID_MIN, ECSOSD_0_SOCKET_ID_MAX, ECSOSD_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSOSD_1_LENGTH_MIN, ECSOSD_1_LENGTH_MAX, ECSOSD_1_LENGTH_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            length = value;

            dataTemp = malloc(length*2+1);
            if(dataTemp == NULL)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, SOCKET_NO_MEMORY, NULL);
                break;
            }
            memset(dataTemp, 0x00, length*2+1);

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, dataTemp, length*2+1, &len, ECSOSD_2_DATA_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if(strlen((CHAR*)dataTemp) != length *2)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            if((atGetStrValue(pAtCmdReq->pParamList, 3, flagValue, ECSOSD_3_FLAG_STR_MAX_LEN+1, &len, ECSOSD_3_FLAG_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR*)flagValue, "0x100") == 0)
                {
                    expectionFlag = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x200") == 0)
                {
                    raiInfo = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x400") == 0)
                {
                    raiInfo = 2;
                }
                else if(strcmp((CHAR*)flagValue, "0") == 0)
                {
                    raiInfo = 0;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 4, &value, ECSOSD_4_SEQUENCE_MIN, ECSOSD_4_SEQUENCE_MAX, ECSOSD_4_SEQUENCE_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            sequence = value;

            socTcpSendReq = (EcSocTcpSendReq *)malloc(sizeof(EcSocTcpSendReq)+length);
            if(socTcpSendReq)
            {
                socTcpSendReq->socketId = socketId;
                socTcpSendReq->length = length;
                socTcpSendReq->expectionFlag = expectionFlag;
                socTcpSendReq->raiInfo = raiInfo;
                socTcpSendReq->sequence = sequence;
                socTcpSendReq->reqSource =reqHandle;
                if(cmsHexStrToHex(socTcpSendReq->data, length, (CHAR *)dataTemp, length*2) != length)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmECSOSD_1, P_WARNING, 0, "the hex string data is not valid");
                     free(socTcpSendReq);
                     ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                     break;
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_TCPSEND, (void *)socTcpSendReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socTcpSendReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSOST? */         /* AT+ECSOST */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    if(dataTemp)
        free(dataTemp);

    return ret;
}


/**
  \fn          CmsRetId nmECSOSD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmECSOSDT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 len;
    INT32 fd;
    INT32 dataLen = -1;
    UINT8 exceptionFlag = 0;
    UINT8 raiInfo = 0;
    UINT8 sequence = 0;
    UINT8 flagValue[ECSOSTF_3_FLAG_STR_MAX_LEN + 1] = {0};
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOSDT=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(1-1400),(0,0x100,0x200,0x400),(1-255)", ECSOSDT_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOSDT= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, ECSOSDT_0_SOCKET_ID_MIN, ECSOSDT_0_SOCKET_ID_MAX, ECSOSDT_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSOSDT_1_LENGTH_MIN, ECSOSDT_1_LENGTH_MAX, ECSOSDT_1_LENGTH_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataLen = value;

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, flagValue, ECSOSDT_2_FLAG_STR_MAX_LEN+1, &len, ECSOSDT_2_FLAG_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR*)flagValue, "0x100") == 0)
                {
                    exceptionFlag = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x200") == 0)
                {
                    raiInfo = 1;
                }
                else if(strcmp((CHAR*)flagValue, "0x400") == 0)
                {
                    raiInfo = 2;
                }
                else if(strcmp((CHAR*)flagValue, "0") == 0)
                {
                    raiInfo = 0;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
           }

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECSOSDT_3_SEQUENCE_MIN, ECSOSDT_3_SEQUENCE_MAX, ECSOSDT_3_SEQUENCE_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            sequence = value;

            if (pSocketSendInfo == PNULL)
            {
                pSocketSendInfo = (AtSocketSendInfo *)OsaAllocZeroMemory(sizeof(AtSocketSendInfo));
            
                OsaDebugBegin(pSocketSendInfo != PNULL, sizeof(AtSocketSendInfo), 0, 0);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
                break;
                OsaDebugEnd();
            }
            else
            {
                memset(pSocketSendInfo, 0, sizeof(AtSocketSendInfo));
            }
            
            pSocketSendInfo->socketId = fd;
            pSocketSendInfo->reqHander = reqHandle;
            pSocketSendInfo->dataLen = dataLen;
            pSocketSendInfo->sequence = sequence;
            pSocketSendInfo->expectData = exceptionFlag;
            pSocketSendInfo->raiInfo = raiInfo;
            pSocketSendInfo->requestId = ECSOCREQ_TCPSEND;
            pSocketSendInfo->source = SOCK_SOURCE_ECSOC;           

            ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SOCKET_SEND_DATA_STATE);

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
            }

            if (ret == CMS_RET_SUCC)
            {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
            }
            else
            {
                OsaDebugBegin(FALSE, ret, 0, 0);
                OsaDebugEnd();

                nmSocketFreeSendInfo();
                
                atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
            }            

            break;
        }
        case AT_READ_REQ:         /* AT+ECSOSDT? */
        default:         /* AT+ECSOSDT */
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSOCL(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOCL=<socket_id>
*/
CmsRetId  nmECSOCL(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSocCloseReq *socCloseReq;
    INT32 socketId = AT_SOC_FD_DEF;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOCL=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9)", ECSOCL_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOCL= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSOCL_0_SOCKET_ID_MIN, ECSOCL_0_SOCKET_ID_MAX, ECSOCL_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            socCloseReq = (EcSocCloseReq *)malloc(sizeof(EcSocCloseReq));
            if(socCloseReq)
            {
                socCloseReq->socketId = socketId;
                socCloseReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_CLOSE, (void *)socCloseReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socCloseReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECQSOS? */         /* AT+ECQSOS */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSONMI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSONMI=<mode>[,<max_public_dl_buffer>[,<max_public_dl_pkg_num>]]
*/
CmsRetId  nmECSONMI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSocNMIReq *socNmiReq;
    EcSocNMIGetReq *socNmiGetReq;
    UINT8 mode = 0;
    UINT8 maxPubDlPkgNum = 0;
    UINT16 maxPubDlbuffer = 0;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSONMI=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-3),(1358-3072),(8-16)", ECSONMI_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSONMI= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSONMI_0_MODE_MIN, ECSONMI_0_MODE_MAX, ECSONMI_0_MODE_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            mode = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSONMI_1_DL_BUFFER_MIN, ECSONMI_1_DL_BUFFER_MAX, ECSONMI_1_DL_BUFFER_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            maxPubDlbuffer = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSONMI_2_DL_PKG_NUM_MIN, ECSONMI_2_DL_PKG_NUM_MAX, ECSONMI_2_DL_PKG_NUM_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            maxPubDlPkgNum = value;

            socNmiReq = (EcSocNMIReq *)malloc(sizeof(EcSocNMIReq));
            if(socNmiReq)
            {
                socNmiReq->mode = mode;
                socNmiReq->maxPublicDlBuffer = maxPubDlbuffer;
                socNmiReq->maxPublicDlPkgNum = maxPubDlPkgNum;
                socNmiReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_NMI, (void *)socNmiReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socNmiReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSONMI? */         /* AT+ECSONMI */

            socNmiGetReq = (EcSocNMIGetReq *)malloc(sizeof(EcSocNMIGetReq));
            if(socNmiGetReq)
            {
                socNmiGetReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_GNMI, (void *)socNmiGetReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socNmiGetReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }
            else
            {
                ret = CMS_RET_SUCC;
            }
            break;
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSONMIE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSONMIE=<socket>,<mode>[,<max_public_dl_buffer>[,<max_public_dl_pkg_num>]]
*/
CmsRetId  nmECSONMIE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSocNMIEReq *socNmieReq;
    EcSocNMIEGetReq *socNmieGetReq;
    INT32 socketId = AT_SOC_FD_DEF;
    UINT8 mode = 0;
    UINT8 maxPubDlPkgNum = 0;
    UINT16 maxPubDlbuffer = 0;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSONMIE=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9),(0-3,255),(1358-3072),(1-8)", ECSONMIE_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSONMIE= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSONMIE_0_SOCKET_ID_MIN, ECSONMIE_0_SOCKET_ID_MAX, ECSONMIE_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSONMIE_1_MODE_MIN, ECSONMIE_1_MODE_MAX, ECSONMIE_1_MODE_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            mode = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECSONMIE_2_DL_BUFFER_MIN, ECSONMIE_2_DL_BUFFER_MAX, ECSONMIE_2_DL_BUFFER_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            maxPubDlbuffer = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECSONMIE_3_DL_PKG_NUM_MIN, ECSONMIE_3_DL_PKG_NUM_MAX, ECSONMIE_3_DL_PKG_NUM_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            maxPubDlPkgNum = value;

            socNmieReq = (EcSocNMIEReq *)malloc(sizeof(EcSocNMIEReq));
            if(socNmieReq)
            {
                socNmieReq->socketId = socketId;
                socNmieReq->mode = mode;
                socNmieReq->maxPublicDlBuffer = maxPubDlbuffer;
                socNmieReq->maxPublicDlPkgNum = maxPubDlPkgNum;
                socNmieReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_NMIE, (void *)socNmieReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socNmieReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSONMIE? */         /* AT+ECSONMI */

            socNmieGetReq = (EcSocNMIEGetReq *)malloc(sizeof(EcSocNMIEGetReq));
            if(socNmieGetReq)
            {
                socNmieGetReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_GNMIE, (void *)socNmieGetReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socNmieGetReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }
            else
            {
                ret = CMS_RET_SUCC;
            }

            break;
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSOSTATUS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSOSTATUS=<socket_id>
*/
CmsRetId  nmECSOSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSocStatusReq *socQstatusReq;
    INT32 socketId = AT_SOC_FD_DEF;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSOSTATUS=? */
        {
            CHAR  respBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
            snprintf(respBuf, ATEC_IND_RESP_128_STR_LEN, "%s: (0-9)", ECSOSTATUS_NAME);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, respBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSOSTATUS= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSOSTATUS_0_SOCKET_ID_MIN, ECSOSTATUS_0_SOCKET_ID_MAX, ECSOSTATUS_0_SOCKET_ID_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            socQstatusReq = (EcSocStatusReq *)malloc(sizeof(EcSocStatusReq));
            if(socQstatusReq)
            {
                socQstatusReq->bQuryAll = FALSE;
                socQstatusReq->socketId = socketId;
                socQstatusReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_STATUS, (void *)socQstatusReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socQstatusReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }
        case AT_EXEC_REQ:
        {
            socQstatusReq = (EcSocStatusReq *)malloc(sizeof(EcSocStatusReq));
            if(socQstatusReq)
            {
                socQstatusReq->bQuryAll = TRUE;
                socQstatusReq->socketId = socketId;
                socQstatusReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_STATUS, (void *)socQstatusReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socQstatusReq);
                ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }
            break;
        }
        case AT_READ_REQ:         /* AT+ECSOSTATUS? */         /* AT+ECSOSTATUS */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECCRVSOCR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSRVSOCRTCP=<listen_port>,<af_type>[,<ip_address>]
*/
CmsRetId  nmECSRVSOCRTCP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 domain = AF_INET;
    UINT16 listenPort = 0;
    EcSrvSocCreateTcpReq *srvSocCreateTcpReq;
    UINT8 tmpDomain[ECSRVSOCRTCP_1_AF_TYPE_STR_MAX_LEN +1] = {0};
    UINT8 tmpAddress[ECSRVSOCRTCP_2_IP_ADDR_STR_MAX_LEN+1] = {0};
    UINT16 len = 0;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSRVSOCRTCP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+ECSRVSOCRTCP:(0-65535),(AF_INET,AF_INET6),(ip address)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECSRVSOCRTCP= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSRVSOCRTCP_0_LISTEN_PORT_MIN, ECSRVSOCRTCP_0_LISTEN_PORT_MAX, ECSRVSOCRTCP_0_LISTEN_PORT_DEF)) != AT_PARA_OK)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            listenPort = value;

            if((atGetStrValue(pAtCmdReq->pParamList, 1, tmpDomain, ECSRVSOCRTCP_1_AF_TYPE_STR_MAX_LEN+1, &len, ECSRVSOCRTCP_1_AF_TYPE_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            if(len > 0)
            {
                if(strcmp((CHAR *)tmpDomain , "AF_INET") == 0)
                {
                    domain = AF_INET;
                }
                else if(strcmp((CHAR *)tmpDomain , "AF_INET6") == 0)
                {
                    domain = AF_INET6;
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                    break;
                }
            }

            if((atGetStrValue(pAtCmdReq->pParamList, 2, tmpAddress, ECSRVSOCRTCP_2_IP_ADDR_STR_MAX_LEN+1, &len, ECSRVSOCRTCP_2_IP_ADDR_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }

            srvSocCreateTcpReq = (EcSrvSocCreateTcpReq *)malloc(sizeof(EcSrvSocCreateTcpReq));
            if(srvSocCreateTcpReq)
            {
                srvSocCreateTcpReq->domain = domain;
                srvSocCreateTcpReq->listenPort = listenPort;
                srvSocCreateTcpReq->reqSource = reqHandle;
                srvSocCreateTcpReq->eventCallback = (void *)atEcSrvSocEventCallback;
                if(len > 0)
                {
                    if(ipaddr_aton((const char*)tmpAddress, &srvSocCreateTcpReq->bindAddr) == 0)
                    {
                        free(srvSocCreateTcpReq);
                        ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                        break;
                    }
                    else
                    {
                        if(domain == AF_INET)
                        {
                            if(IP_IS_V6(&srvSocCreateTcpReq->bindAddr))
                            {
                                free(srvSocCreateTcpReq);
                                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                                break;                                
                            }
                        }
                        else if(domain == AF_INET6)
                        {
                            if(IP_IS_V4_VAL(srvSocCreateTcpReq->bindAddr) && (!ip_addr_isany_val(srvSocCreateTcpReq->bindAddr)))
                            {
                                free(srvSocCreateTcpReq);
                                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                                break;                            
                            }
                        }
                    }
                }
                else
                {
                    ip_addr_set_zero(&srvSocCreateTcpReq->bindAddr);
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_CREATETCP, (void *)srvSocCreateTcpReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocCreateTcpReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSRVSOCRTCP? */         /* AT+ECSRVSOCRTCP */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSRVSOCLLISTEN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+nmECSRVSOCLLISTEN=<socket_id>
*/
CmsRetId  nmECSRVSOCLLISTEN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSrvSocCloseTcpServerReq *srvSocCloseTcpServerReq;
    INT32 socketId = AT_SOC_FD_DEF;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSRVSOCLLISTEN=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+ECSRVSOCLTCPLISTEN: (0-9)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECSRVSOCLLISTEN= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSRVSOCLLISTEN_0_SERVER_FD_MIN, ECSRVSOCLLISTEN_0_SERVER_FD_MAX, ECSRVSOCLLISTEN_0_SERVER_FD_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            srvSocCloseTcpServerReq = (EcSrvSocCloseTcpServerReq *)malloc(sizeof(EcSrvSocCloseTcpServerReq));
            if(srvSocCloseTcpServerReq)
            {
                srvSocCloseTcpServerReq->socketId = socketId;
                srvSocCloseTcpServerReq->bCloseClient = TRUE;
                srvSocCloseTcpServerReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_CLOSELISTEN, (void *)srvSocCloseTcpServerReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocCloseTcpServerReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_EXEC_REQ:         /* AT+ECSRVSOCLLISTEN */

            srvSocCloseTcpServerReq = (EcSrvSocCloseTcpServerReq *)malloc(sizeof(EcSrvSocCloseTcpServerReq));
            if(srvSocCloseTcpServerReq)
            {
                srvSocCloseTcpServerReq->socketId = AT_SOC_FD_ALL;
                srvSocCloseTcpServerReq->bCloseClient = TRUE;
                srvSocCloseTcpServerReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_CLOSELISTEN, (void *)srvSocCloseTcpServerReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocCloseTcpServerReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }
            else
            {
                ret = CMS_RET_SUCC;
            }

            break;
        case AT_READ_REQ:
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId nmECSRVSOCLCLIENT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSRVSOCLCLIENT=<server_socket_id>[,<server_socket_id>]
*/
CmsRetId  nmECSRVSOCLCLIENT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSrvSocCloseTcpClientReq *srvSocCloseTcpClientReq;
    INT32 socketServerId = AT_SOC_FD_DEF;
    INT32 socketClientId = AT_SOC_FD_ALL;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+nmECSRVSOCLCLIENT=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+ECSRVSOCLTCPCLIENT:(0-9),(0-9)");
            break;
        }

        case AT_SET_REQ:         /* AT+nmECSRVSOCLCLIENT= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSRVSOCLCLIENT_0_SERVER_FD_MIN, ECSRVSOCLCLIENT_0_SERVER_FD_MAX, ECSRVSOCLCLIENT_0_SERVER_FD_DEF)) != AT_PARA_OK)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketServerId = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &value, ECSRVSOCLCLIENT_1_CLIENT_FD_MIN, ECSRVSOCLCLIENT_1_CLIENT_FD_MAX, ECSRVSOCLCLIENT_1_CLIENT_FD_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketClientId = value;

            srvSocCloseTcpClientReq = (EcSrvSocCloseTcpClientReq *)malloc(sizeof(EcSrvSocCloseTcpClientReq));
            if(srvSocCloseTcpClientReq)
            {
                srvSocCloseTcpClientReq->socketServerId = socketServerId;
                srvSocCloseTcpClientReq->socketClientId = socketClientId;
                srvSocCloseTcpClientReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_CLOSECLIENT, (void *)srvSocCloseTcpClientReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocCloseTcpClientReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+nmECSRVSOCLCLIENT? */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSRVSOTCPSENDCLT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSRVSOTCPSENDCLT=<client_socket_id>,<length>[,<data>[,<rai>[,<except>]]]
*/
CmsRetId  nmECSRVSOTCPSENDCLT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    INT32 dataLen;
    UINT8 * dataTemp = NULL;
    UINT8 dataRai = ATEC_SKT_DATA_RAI_NO_INFO;
    BOOL dataExcept = FALSE;
    UINT16 len = 0;
    INT32 value;

    EcSrvSocSendTcpClientReq *srvSocSendTcpClientReq;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSRVSOTCPSENDCLT=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+ECSRVSOTCPSENDCLT: (0-9),(1-1400),\"data\",(0-2),(0,1)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECSRVSOTCPSENDCLT= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, ECCRVSOTCPSENDCLT_0_MIN, ECCRVSOTCPSENDCLT_0_MAX, ECCRVSOTCPSENDCLT_0_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &dataLen, ECCRVSOTCPSENDCLT_1_MIN, ECCRVSOTCPSENDCLT_1_MAX, ECCRVSOTCPSENDCLT_1_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            dataTemp = malloc(dataLen*2+1);

            if(dataTemp == NULL)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( SOCKET_NO_MEMORY), NULL);
                break;
            }

            memset(dataTemp, 0x00, dataLen*2+1);
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, dataTemp, dataLen*2+1, &len, ECCRVSOTCPSENDCLT_2_STR_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }
            if(strlen((CHAR*)dataTemp) != dataLen *2)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECCRVSOTCPSENDCLT_3_MIN, ECCRVSOTCPSENDCLT_3_MAX, ECCRVSOTCPSENDCLT_3_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataRai = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 4, &value, ECCRVSOTCPSENDCLT_4_MIN, ECCRVSOTCPSENDCLT_4_MAX, ECCRVSOTCPSENDCLT_4_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataExcept = value;

            srvSocSendTcpClientReq = (EcSrvSocSendTcpClientReq *)malloc(sizeof(EcSrvSocSendTcpClientReq) + dataLen);

            if(srvSocSendTcpClientReq)
            {
                if(cmsHexStrToHex(srvSocSendTcpClientReq->data, dataLen, (CHAR *)dataTemp, dataLen*2) != dataLen)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmECSRVSOTCPSENDCLT_1, P_WARNING, 0, "the hex string data is not valid");
                     free(srvSocSendTcpClientReq);
                     ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                     break;
                }
                srvSocSendTcpClientReq->sendLen = dataLen;
                srvSocSendTcpClientReq->socketClientId = fd;
                srvSocSendTcpClientReq->dataRai = dataRai;
                srvSocSendTcpClientReq->dataExpect = dataExcept;
                srvSocSendTcpClientReq->reqSource = reqHandle;
            }
            else
            {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmECSRVSOTCPSENDCLT_2, P_WARNING, 0, "send fail,malloc buffer fail");
               ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
               break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_SENDCLIENT, (void *)srvSocSendTcpClientReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocSendTcpClientReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL), NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSRVSOTCPSENDCLT? */
        default:         /* AT+ECSRVSOTCPSENDCLT */
        {
            ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    if(dataTemp)
        free(dataTemp);
    return ret;
}


/**
  \fn          CmsRetId nmECSRVSOTCPSENDCLTT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSRVSOTCPSENDCLTT=<client_socket_id>[,<length>[,<rai>[,<except>]]]
*/
CmsRetId  nmECSRVSOTCPSENDCLTT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 fd;
    INT32 dataLen = -1;
    UINT8 dataRai = ATEC_SKT_DATA_RAI_NO_INFO;
    BOOL dataExcept = FALSE;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSRVSOTCPSENDCLTT=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+ECSRVSOTCPSENDCLTT: (0-9),(1-1400),(0-2),(0,1)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECSRVSOTCPSENDCLTT= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &fd, ECCRVSOTCPSENDCLTT_0_MIN, ECCRVSOTCPSENDCLTT_0_MAX, ECCRVSOTCPSENDCLTT_0_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if(fd < 0)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 1, &dataLen, ECCRVSOTCPSENDCLTT_1_MIN, ECCRVSOTCPSENDCLTT_1_MAX, ECCRVSOTCPSENDCLTT_1_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                break;
            }

            if((atGetNumValue(pAtCmdReq->pParamList, 2, &value, ECCRVSOTCPSENDCLTT_2_MIN, ECCRVSOTCPSENDCLTT_2_MAX, ECCRVSOTCPSENDCLTT_2_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataRai = value;

            if((atGetNumValue(pAtCmdReq->pParamList, 3, &value, ECCRVSOTCPSENDCLTT_3_MIN, ECCRVSOTCPSENDCLTT_3_MAX, ECCRVSOTCPSENDCLTT_3_DEF)) == AT_PARA_ERR)
            {
                 ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                 break;
            }
            dataExcept = value;

            if (pSocketSendInfo == PNULL)
            {
                pSocketSendInfo = (AtSocketSendInfo *)OsaAllocZeroMemory(sizeof(AtSocketSendInfo));
            
                OsaDebugBegin(pSocketSendInfo != PNULL, sizeof(AtSocketSendInfo), 0, 0);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_NO_MEMORY), NULL);
                break;
                OsaDebugEnd();
            }
            else
            {
                memset(pSocketSendInfo, 0, sizeof(AtSocketSendInfo));
            }
            
            pSocketSendInfo->socketId = fd;
            pSocketSendInfo->reqHander = reqHandle;
            pSocketSendInfo->dataLen = dataLen;
            pSocketSendInfo->raiInfo = dataRai;
            pSocketSendInfo->expectData = dataExcept;
            pSocketSendInfo->source = SOCK_SOURCE_ECSRVSOC;
            pSocketSendInfo->requestId = ECSRVSOCREQ_SENDCLIENT;

            ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SOCKET_SEND_DATA_STATE);

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
            }

            if (ret == CMS_RET_SUCC)
            {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
            }
            else
            {
                OsaDebugBegin(FALSE, ret, 0, 0);
                OsaDebugEnd();

                nmSocketFreeSendInfo();
                
                atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
            }            

            break;
        }
        case AT_READ_REQ:         /* AT+ECSRVSOTCPSENDCLTT? */
        default:         /* AT+ECSRVSOTCPSENDCLTT */
        {
            ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, ( CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId nmECSRVSOTCPLISTENSTATUS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
/* format
* AT+ECSRVSOTCPLISTENSTATUS=<socket_id>
*/
CmsRetId  nmECSRVSOTCPLISTENSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    EcSrvSocStatusTcpServerReq *srvSocStatusTcpServerReq;
    INT32 socketId = AT_SOC_FD_DEF;
    INT32 value;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSRVSOTCPLISTENSTATUS=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+ECSRVSOTCPLISTENSTATUS: (0-9)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECSRVSOTCPLISTENSTATUS= */
        {

            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &value, ECSRVSOTCPLISTENSTATUS_0_SERVER_FD_MIN, ECSRVSOTCPLISTENSTATUS_0_SERVER_FD_MAX, ECSRVSOTCPLISTENSTATUS_0_SERVER_FD_DEF)) == AT_PARA_ERR)
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
                break;
            }
            socketId = value;

            srvSocStatusTcpServerReq = (EcSrvSocStatusTcpServerReq *)malloc(sizeof(EcSrvSocStatusTcpServerReq));
            if(srvSocStatusTcpServerReq)
            {
                srvSocStatusTcpServerReq->socketId = socketId;
                srvSocStatusTcpServerReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_STATUSLISTEN, (void *)srvSocStatusTcpServerReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocStatusTcpServerReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }

            break;
        }

        case AT_READ_REQ:         /* AT+ECSRVSOTCPLISTENSTATUS? */

            srvSocStatusTcpServerReq = (EcSrvSocStatusTcpServerReq *)malloc(sizeof(EcSrvSocStatusTcpServerReq));
            if(srvSocStatusTcpServerReq)
            {
                srvSocStatusTcpServerReq->socketId = AT_SOC_FD_ALL;
                srvSocStatusTcpServerReq->reqSource = reqHandle;
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                break;
            }

            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_STATUSLISTEN, (void *)srvSocStatusTcpServerReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocStatusTcpServerReq);
                ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL, NULL);
                break;
            }
            else
            {
                ret = CMS_RET_SUCC;
            }

            break;
        case AT_EXEC_REQ:
        default:
        {
            ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

void nmSocketFreeSendInfo(void)
{
    if(pSocketSendInfo != PNULL)
    {
        OsaFreeMemory(&pSocketSendInfo);
        
        pSocketSendInfo = PNULL;
    }
}


CmsRetId nmAtSocketSend(AtSocketSendInfo *pSendInfo)
{
    CmsRetId ret = CMS_RET_SUCC;

    
    OsaDebugBegin(pSendInfo != PNULL, pSendInfo, 0, 0);
    return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    OsaDebugEnd();

    switch(pSendInfo->requestId)
    {
        case ATECSKTREQ_SEND:
        {
            AtecSktSendReq *sktSendReq;
            sktSendReq = (AtecSktSendReq *)malloc(sizeof(AtecSktSendReq) + pSendInfo->dataLen);

            if(sktSendReq)
            {
                if(cmsHexStrToHex(sktSendReq->data, pSendInfo->dataLen, (CHAR *)pSendInfo->data, pSendInfo->dataLen * 2) != pSendInfo->dataLen)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_1, P_WARNING, 0, "the hex string data is not valid");
                     free(sktSendReq);
                     return CME_SOCK_MGR_ERROR_PARAM_ERROR;
                }
                sktSendReq->sendLen = pSendInfo->dataLen;
                sktSendReq->fd = pSendInfo->socketId;
                sktSendReq->dataRai = pSendInfo->raiInfo;
                sktSendReq->dataExpect = pSendInfo->expectData;
                sktSendReq->reqSource = pSendInfo->reqHander;
            }
            else
            {
               ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_2, P_WARNING, 0, "send fail,malloc buffer fail");
               return CME_SOCK_MGR_ERROR_NO_MEMORY;
            }

            if(cmsSockMgrSendAsyncRequest(ATECSKTREQ_SEND, (void *)sktSendReq, SOCK_SOURCE_ATSKT) != SOCK_MGR_ACTION_OK)
            {
                free(sktSendReq);
                ret = CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL;
            }

            break;            
        }
        case ECSOCREQ_UDPSEND:
        case ECSOCREQ_UDPSENDF:
        {
            EcSocUdpSendReq *socUdpSendReq;

            socUdpSendReq = (EcSocUdpSendReq *)malloc(sizeof(EcSocUdpSendReq)+pSendInfo->dataLen);
            if(socUdpSendReq)
            {
                socUdpSendReq->socketId = pSendInfo->socketId;
                socUdpSendReq->remotePort = pSendInfo->remotePort;
                socUdpSendReq->length = pSendInfo->dataLen;
                socUdpSendReq->exceptionFlag = pSendInfo->expectData;
                socUdpSendReq->raiInfo = pSendInfo->raiInfo;
                socUdpSendReq->sequence = pSendInfo->sequence;
                socUdpSendReq->reqSource = pSendInfo->reqHander;
                socUdpSendReq->segmentId = 0;
                socUdpSendReq->segmentNum = 0;
                ip_addr_copy(socUdpSendReq->remoteAddr, pSendInfo->remoteAddr);
                
                if(cmsHexStrToHex(socUdpSendReq->data, pSendInfo->dataLen, (CHAR *)pSendInfo->data, pSendInfo->dataLen * 2) != pSendInfo->dataLen)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_3, P_WARNING, 0, "the hex string data is not valid");
                    free(socUdpSendReq);
                    return CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL;
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_4, P_WARNING, 0, "send fail,malloc buffer fail");
                return CME_SOCK_MGR_ERROR_NO_MEMORY;
            }
        
            if(cmsSockMgrSendAsyncRequest(pSendInfo->requestId, (void *)socUdpSendReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socUdpSendReq);
                ret = CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL;
            }
        
            break;
        }
        case ECSOCREQ_TCPSEND:
        {
            EcSocTcpSendReq *socTcpSendReq;

            socTcpSendReq = (EcSocTcpSendReq *)malloc(sizeof(EcSocTcpSendReq)+pSendInfo->dataLen);
            if(socTcpSendReq)
            {
                socTcpSendReq->socketId = pSendInfo->socketId;
                socTcpSendReq->length = pSendInfo->dataLen;
                socTcpSendReq->expectionFlag = pSendInfo->expectData;
                socTcpSendReq->raiInfo = pSendInfo->raiInfo;
                socTcpSendReq->sequence = pSendInfo->sequence;
                socTcpSendReq->reqSource = pSendInfo->reqHander;
                if(cmsHexStrToHex(socTcpSendReq->data, pSendInfo->dataLen, (CHAR *)pSendInfo->data, pSendInfo->dataLen * 2) != pSendInfo->dataLen)
                {
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_5, P_WARNING, 0, "the hex string data is not valid");
                     free(socTcpSendReq);
                     return CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL;
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_6, P_WARNING, 0, "send fail,malloc buffer fail");
                return CME_SOCK_MGR_ERROR_NO_MEMORY;
            }

            if(cmsSockMgrSendAsyncRequest(ECSOCREQ_TCPSEND, (void *)socTcpSendReq, SOCK_SOURCE_ECSOC) != SOCK_MGR_ACTION_OK)
            {
                free(socTcpSendReq);
                ret = CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL;
            }

            break;            
        }
        case ECSRVSOCREQ_SENDCLIENT:
        {

            EcSrvSocSendTcpClientReq *srvSocSendTcpClientReq;
            srvSocSendTcpClientReq = (EcSrvSocSendTcpClientReq *)malloc(sizeof(EcSrvSocSendTcpClientReq) + pSendInfo->dataLen);
    
            if(srvSocSendTcpClientReq)
            {
                if(cmsHexStrToHex(srvSocSendTcpClientReq->data, pSendInfo->dataLen, (CHAR *)pSendInfo->data, pSendInfo->dataLen * 2) != pSendInfo->dataLen)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_7, P_WARNING, 0, "the hex string data is not valid");
                    free(srvSocSendTcpClientReq);
                    return CME_SOCK_MGR_ERROR_PARAM_ERROR;
                }

                srvSocSendTcpClientReq->sendLen = pSendInfo->dataLen;
                srvSocSendTcpClientReq->socketClientId = pSendInfo->socketId;
                srvSocSendTcpClientReq->dataRai = pSendInfo->raiInfo;
                srvSocSendTcpClientReq->dataExpect = pSendInfo->expectData;
                srvSocSendTcpClientReq->sequence = pSendInfo->sequence;
                srvSocSendTcpClientReq->reqSource = pSendInfo->reqHander;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_8, P_WARNING, 0, "send fail,malloc buffer fail");
                return CME_SOCK_MGR_ERROR_NO_MEMORY;
            }
    
            if(cmsSockMgrSendAsyncRequest(ECSRVSOCREQ_SENDCLIENT, (void *)srvSocSendTcpClientReq, SOCK_SOURCE_ECSRVSOC) != SOCK_MGR_ACTION_OK)
            {
                free(srvSocSendTcpClientReq);
                ret = CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL;
            }

            break;
        }
        default:
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmAtSocketSend_9, P_WARNING, 1, "nmAtSocketSend unknow req id %u", pSendInfo->requestId);
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    return ret;

}




/**
  \fn           CmsRetId nmSocketSendCancel(void)
  \brief        cancel socket send
  \param[in]    void
  \returns      void
*/
void nmSocketSendCancel(INT32 cause, UINT8 source)
{
    UINT8   chanId = 0;

    OsaDebugBegin(pSocketSendInfo != PNULL, pSocketSendInfo, 0, 0);
    return;
    OsaDebugEnd();

    /* AT channel state should change back to COMMAND state */
    chanId = AT_GET_HANDLER_CHAN_ID(pSocketSendInfo->reqHander);

    atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);

    /*
     * need send CMS ERROR
    */
    switch(source)
    {
        case SOCK_SOURCE_ATSKT:
        {
            atcReply(pSocketSendInfo->reqHander,
             AT_RC_SOCKET_ERROR,
             cause,
             PNULL);

             break;
        }
        case SOCK_SOURCE_ECSOC:
        {
            atcReply(pSocketSendInfo->reqHander,
             AT_RC_ECSOC_ERROR,
             cause,
             PNULL);

             break;
        }        
        case SOCK_SOURCE_ECSRVSOC:
        {
            atcReply(pSocketSendInfo->reqHander,
             AT_RC_ECSRVSOC_ERROR,
             cause,
             PNULL);

             break;
        }
        default:
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSocketSendCancel_1, P_WARNING, 1, "nmSocketSendCancel, unknown source %u", source);
            break;
    }

    // free the pSmsSendInfo
    nmSocketFreeSendInfo();

    return;
}


/**
  \fn           CmsRetId nmSocketSendAtReply(void)
  \brief        send At Reply
  \param[in]    void
  \returns      void
*/
CmsRetId nmSocketSendAtReply(UINT16 reqHander, INT32 cause, UINT8 source)
{
    CmsRetId ret = CMS_RET_SUCC;

    /*
     * need send CMS ERROR
    */
    switch(source)
    {
        case SOCK_SOURCE_ATSKT:
        {
            ret = atcReply(reqHander,
             AT_RC_SOCKET_ERROR,
             cause,
             PNULL);

             break;
        }
        case SOCK_SOURCE_ECSOC:
        {
            ret = atcReply(reqHander,
             AT_RC_ECSOC_ERROR,
             cause,
             PNULL);

             break;
        }        
        case SOCK_SOURCE_ECSRVSOC:
        {
            ret = atcReply(reqHander,
             AT_RC_ECSRVSOC_ERROR,
             cause,
             PNULL);

             break;
        }
        default:
            ret = CMS_INVALID_PARAM;
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSocketSendAtReply_1, P_WARNING, 1, "nmSocketSendAtReply, unknown source %u", source);
            break;
    }

    return ret;
}


/**
  \fn          CmsRetId nmSoscketInputData(UINT8 *pInputData, UINT16 length)
  \brief       when at channel change to: ATC_SMS_CMGS_DATA_STATE, all data sent via this
  \            AT channel should pass to soakcet send data by calling this API
  \param[in]   pInput       input data
  \param[in]   length       input data length
  \returns     CmsRetId
  \            if not return: "CMS_RET_SUCC", AT channel should return to command state
*/
CmsRetId nmSocketInputData(UINT8 chanId, UINT8 *pInput, UINT16 length)
{
    CmsRetId cmsRet = CMS_RET_SUCC;

    /* check input */
    OsaCheck(pInput != PNULL && length > 0, pInput, length, 0);

    if (pSocketSendInfo == PNULL)
    {
        OsaDebugBegin(FALSE, pSocketSendInfo, pInput, length);
        OsaDebugEnd();

        return CMS_FAIL;
    }

    // check the channId
    OsaCheck(AT_GET_HANDLER_CHAN_ID(pSocketSendInfo->reqHander) == chanId,
             pSocketSendInfo->reqHander, chanId, 0);

    /*
     * check the input length
    */
    if (pSocketSendInfo->dataOffset + length > SUPPORT_MAX_SOCKET_HEX_STRING_DATA_LENGTH)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_1, P_WARNING, 3,
                    "AT CMD, all input length: %d+%d, extended MAX value: %d",
                    pSocketSendInfo->dataOffset, length, SUPPORT_MAX_SOCKET_HEX_STRING_DATA_LENGTH);

        
        nmSocketSendCancel(CME_SOCK_MGR_ERROR_PARAM_ERROR, pSocketSendInfo->source);

        return CMS_FAIL;
    }

    /*
     * copy the data into buffer
    */
    memcpy(pSocketSendInfo->data+pSocketSendInfo->dataOffset,
           pInput,
           length);
    pSocketSendInfo->dataOffset += length;

    //check CTRL-Z or ESC
    if(pSocketSendInfo->dataLen > 0)
    {
        if((pSocketSendInfo->dataLen * 2) > pSocketSendInfo->dataOffset)
        {
            //need continue
        }
        else if((pSocketSendInfo->dataLen * 2) < pSocketSendInfo->dataOffset)
        {
        
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_2, P_WARNING, 2,
                    "AT CMD, all input length: %d, datalen : %d, noe eq",
                    pSocketSendInfo->dataOffset, pSocketSendInfo->dataLen * 2);
            
            nmSocketSendCancel(CME_SOCK_MGR_ERROR_PARAM_ERROR, pSocketSendInfo->source);
            
            cmsRet = CMS_FAIL;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_3, P_INFO, 2,
                    "AT CMD, all input length: %d",
                    pSocketSendInfo->dataOffset);
            
            cmsRet = nmAtSocketSend(pSocketSendInfo);

            /*done, need change the AT channel state to command state*/
            atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);

            /* if failed, need to send the CMS ERROR */
            if (cmsRet != CMS_RET_SUCC)
            {
                cmsRet = nmSocketSendAtReply(pSocketSendInfo->reqHander, cmsRet, pSocketSendInfo->source);
            }
            
            // free the pSmsSendInfo
            nmSocketFreeSendInfo();
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_4, P_INFO, 2,
             "AT CMD, input end with ctrl + z",);
                
        if (pSocketSendInfo->data[pSocketSendInfo->dataOffset-1] == AT_ASCI_CTRL_Z)   //CTRL-Z
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_5, P_INFO, 2,
                "AT CMD, input one ctrl + z",);        

            if(pSocketSendInfo->dataOffset > 1)
            {
            
                //remove CTRL-Z 
                pSocketSendInfo->dataOffset -= 1;

                if((pSocketSendInfo->dataOffset % 2) != 0)
                {
            
                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_6, P_WARNING, 2,
                        "AT CMD, all input length: %d is invalid",
                        pSocketSendInfo->dataOffset);
                
                    nmSocketSendCancel(CME_SOCK_MGR_ERROR_PARAM_ERROR, pSocketSendInfo->source);
                
                    cmsRet = CMS_FAIL;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_7, P_INFO, 2,
                        "AT CMD, all input length: %d",
                        pSocketSendInfo->dataOffset);

                        pSocketSendInfo->dataLen = pSocketSendInfo->dataOffset / 2;
        
                    cmsRet = nmAtSocketSend(pSocketSendInfo);

                    /*done, need change the AT channel state to command state*/
                    atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);

                    /* if failed, need to send the CMS ERROR */
                    if (cmsRet != CMS_RET_SUCC)
                    {
                        cmsRet = nmSocketSendAtReply(pSocketSendInfo->reqHander, cmsRet, pSocketSendInfo->source);

                    }
                    // free the pSmsSendInfo
                    nmSocketFreeSendInfo();
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmSocketInputData_8, P_WARNING, 2,
                        "AT CMD, rcv ctrl+z，but all input length just: %d with one ctrl+z",
                        pSocketSendInfo->dataOffset);
            
                nmSocketSendCancel(CME_SOCK_MGR_ERROR_PARAM_ERROR, pSocketSendInfo->source);
            
                cmsRet = CMS_FAIL;                
            }
        }
    }

    return cmsRet;
}

