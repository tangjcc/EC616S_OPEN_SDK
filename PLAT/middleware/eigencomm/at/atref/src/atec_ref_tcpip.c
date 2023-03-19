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
#include "atec_ref_tcpip.h"
#include "ec_tcpip_api.h"
#include "cmicomm.h"
#include "atec_ref_tcpip_cnf_ind.h"
#include "ps_nm_if.h"
#include "def.h"
#include "inet.h"
#include "sockets.h"
#include "task.h"
#include "netmgr.h"
#include "at_sock_entity.h"
#include "debug_log.h"
#include "mw_config.h"
#include "mw_aon_info.h"

#define _DEFINE_AT_REQ_FUNCTION_LIST_


/**
  \fn          CmsRetId nmNPING(const AtCmdInputContext *pAtCmdReq)
  \brief       AT+NPING CMD codes
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  nmNPING(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16   ipAddrLen = 0;
    INT32   size = NPING_1_PAYLOAD_DEF;
    INT32   timeout = NPING_2_TIMEOUT_DEF;
    BOOL    validparam = FALSE;
    UINT8 pingTarget[NPING_0_STR_MAX_LEN + 1] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+NPING=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+NPING: (\"IP ADDR\\URL\"),(1-1500),(1000-600000)");
            break;
        }

        case AT_SET_REQ:      /* AT+PING= */
        {
            /* AT+PING="ip/url"[,<payload_size> [,<time_out>]] */
            /*
            *
            */
            if ((ret = atGetStrValue(pAtCmdReq->pParamList, 0, pingTarget,
                               NPING_0_STR_MAX_LEN+1, &ipAddrLen, NPING_0_STR_DEF)) == AT_PARA_OK)
            {
                if ((atGetNumValue(pAtCmdReq->pParamList, 1, &size,
                                   NPING_1_PAYLOAD_MIN, NPING_1_PAYLOAD_MAX, NPING_1_PAYLOAD_DEF)) != AT_PARA_ERR)
                {
                    if ((atGetNumValue(pAtCmdReq->pParamList, 2, &timeout,
                                       NPING_2_TIMEOUT_MIN, NPING_2_TIMEOUT_MAX, NPING_2_TIMEOUT_DEF)) != AT_PARA_ERR)
                    {
                        validparam = true;
                        ret = psStartPing(reqHandle, pingTarget, ipAddrLen, 1, size, timeout, FALSE);

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

            if (!validparam)
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
            }
            break;
        }

        case AT_READ_REQ:           /* AT+NPING?  */
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
  \fn          CmsRetId nmQDNS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  nmQDNS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 mode = 0;
    UINT16 urlLen = 0;
    UINT8 pUrl[QDNS_1_STR_MAX_LEN + 1] = {0};

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QDNS= */
        {

         if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &mode,
                                       QDNS_0_MODE_MIN, QDNS_0_MODE_MAX, QDNS_0_MODE_DEF)) != AT_PARA_ERR)
         {
            if(mode == 0)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, pUrl, QDNS_1_STR_MAX_LEN+1, &urlLen, QDNS_1_STR_DEF)) == AT_PARA_OK)
                {
                    if (urlLen > 0)
                    {
                        ret = psGetDNS(reqHandle, pUrl);

                        if(ret == CMS_FAIL)
                        {
                            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_UNKNOWN), NULL);
                        }
                        else
                        {
                            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            }
            else if(mode == 1)
            {
                if((atGetStrValue(pAtCmdReq->pParamList, 1, pUrl, QDNS_1_STR_MAX_LEN+1, &urlLen, QDNS_1_STR_DEF)) != AT_PARA_ERR)
                {
                    if (urlLen > 0)
                    {
                        ret = psClearDnsCache(FALSE, pUrl);

                        if (ret == CMS_RET_SUCC)
                        {
                            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                        }
                        else
                        {
                            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_UNKNOWN, NULL);
                        }

                    }
                    else
                    {
                        ret = psClearDnsCache(TRUE, PNULL);

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
                else
                {
                    ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_PARAM_ERROR), NULL);
                }
            }
            else if(mode == 2)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, pUrl, QDNS_1_STR_MAX_LEN+1, &urlLen, QDNS_1_STR_DEF)) == AT_PARA_OK)
                {
                    if (urlLen > 0)
                    {
                        ret = psGetDNSWithoutCache(reqHandle, pUrl);

                        if(ret == CMS_FAIL)
                        {
                            ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_SOCK_MGR_ERROR_UNKNOWN), NULL);
                        }
                        else
                        {
                            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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

        case AT_TEST_REQ:
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+QDNS: (0-2),(\"URL\")");
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
  \fn          CmsRetId nmQIDNSCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       Set/Get default DNS config
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  nmQIDNSCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32              reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32     operaType = (UINT32)pAtCmdReq->operaType;
    MidWareDefaultAonDnsCfg    dnsCfg; //40 bytes
    NmNetIfDnsCfg dnsServer;

    memset(&dnsCfg, 0x00, sizeof(MidWareDefaultDnsCfg));
    memset(&dnsServer, 0x00, sizeof(NmNetIfDnsCfg));

    /*AT+QIDNSCFG=<dns_1>[,<dns_2>[,<dns_3>[,<dns_4>]]]*/
    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QIDNSCFG= */
        {
            ip_addr_t   ipAddr; //20 bytes
            UINT8   strDnsTemp[QIDNSCFG_DNS_STR_MAX_LEN + 1] = {0};    //64 bytes
            UINT8   paraIdx = 0, validIpv4DnsNum = 0, validIpv6DnsNum = 0;
            UINT16  ipv6U16 = 0;
            BOOL    validDns = TRUE;
            UINT16 len = 0;

            NmNetIfDnsCfg dnsServer;

            memset(&dnsServer, 0x00, sizeof(NmNetIfDnsCfg));
                
            while (paraIdx < QIDNSCFG_DNS_NUM &&
                   paraIdx < pAtCmdReq->paramRealNum)
            {
                memset(strDnsTemp, 0x00, QIDNSCFG_DNS_STR_MAX_LEN + 1);
                if((ret = atGetStrValue(pAtCmdReq->pParamList, paraIdx, strDnsTemp, QIDNSCFG_DNS_STR_MAX_LEN+1, &len, QIDNSCFG_DNS_STR_DEF)) == AT_PARA_OK)
                {
                    memset(&ipAddr, 0x00, sizeof(ip_addr_t));

                    if (strDnsTemp != PNULL)
                    {
                        if(ipaddr_aton((CHAR* )strDnsTemp, &ipAddr) == TRUE)
                        {
                            if (ipAddr.type == IPADDR_TYPE_V4)
                            {
                                if (ip4_addr_isany(&(ipAddr.u_addr.ip4)))
                                {
                                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_ipv4_any, P_WARNING, 1,
                                            "AT CMD, invalid ipv4 addr: 0x%x, in QIDNSCFG", ipAddr.u_addr.ip4.addr);
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
                                        ip_addr_copy(dnsServer.dns[dnsServer.dnsNum], ipAddr);
                                        dnsServer.dnsNum ++;
                                        
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
                                    ip_addr_copy(dnsServer.dns[dnsServer.dnsNum], ipAddr);
                                    dnsServer.dnsNum ++;
                                    
                                    validIpv4DnsNum++;
                                }
                            }
                            else if (ipAddr.type == IPADDR_TYPE_V6)
                            {
                                if (ip6_addr_isany(&(ipAddr.u_addr.ip6)))
                                {
                                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_ipv6_any, P_WARNING, 4,
                                            "AT CMD, invalid ipv6 addr: 0x%x, 0x%x, 0x%x, 0x%x, in QIDNSCFG",
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
                                        ip_addr_copy(dnsServer.dns[dnsServer.dnsNum], ipAddr);
                                        dnsServer.dnsNum ++;
                                        
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
                                    ip_addr_copy(dnsServer.dns[dnsServer.dnsNum], ipAddr);
                                    dnsServer.dnsNum ++;
                                        
                                    validIpv6DnsNum++;
                                }
                            }
                            else
                            {
                                ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_invalid_type, P_WARNING, 1,
                                        "AT CMD, invalid addr type: %d, in QIDNSCFG", ipAddr.type);
                                validDns = FALSE;
                                break;
                            }
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_invalid_ip, P_WARNING, 0,
                                     "AT CMD, empty DNS, in QIDNSCFG");
                        continue;
                    }
                }
                paraIdx++;
            }

            if (validDns) //save into NVM
            {

                if(psSetDnsServer(NM_PS_INVALID_CID, &dnsServer) == CMS_RET_SUCC)
                {
                    mwAonSetDefaultAonDnsCfgAndSave(&dnsCfg);
                    
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_set_dns_server_fail, P_WARNING, 0,
                                     "AT CMD, SET DNS SERVER FAIL, in QIDNSCFG");
                    
                    ret = atcReply(reqHandle, AT_RC_OK, CME_SOCK_MGR_ERROR_SET_DNS_SERVER_FAIL, NULL);
                }
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
            CHAR    tmpStr[ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN];
            UINT8   tmpIdx = 0;
            char *ip6Str = PNULL;
            BOOL btypeDns = FALSE;

            pRspStr = (CHAR *)OsaAllocZeroMemory(ATEC_QIDNSCFG_GET_CNF_STR_LEN);

            if (pRspStr == PNULL)
            {
                OsaDebugBegin(FALSE, ATEC_QIDNSCFG_GET_CNF_STR_LEN, 0, 0);
                OsaDebugEnd();

                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( CME_UE_FAIL), NULL);
            }
            else
            {
                //first get the current used dns server

                if(psGetDnsServer(0, &dnsServer) == CMS_RET_SUCC)
                {
                   ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_getdnsserver_success, P_SIG, 0,
                                     "AT CMD, get current dns server success");

                    //first check ipv4 dns server
                    for (tmpIdx = 0; tmpIdx < dnsServer.dnsNum && tmpIdx < NM_MAX_DNS_NUM; tmpIdx++)
                    {
                        if(IP_IS_V4(&(dnsServer.dns[tmpIdx])))
                        {
                            if (strlen(pRspStr) == 0)
                            {
                                snprintf(pRspStr,
                                    ATEC_QIDNSCFG_GET_CNF_STR_LEN,
                                    "PrimaryIpv4Dns: %d.%d.%d.%d\r\n",
                                    ip4_addr1_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)),
                                    ip4_addr2_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)),
                                    ip4_addr3_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)),
                                    ip4_addr4_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)));
                            }
                            else
                            {
                                memset(tmpStr, 0x00, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN);
                                snprintf(tmpStr,
                                    ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN,
                                    "SecondaryIpv4Dns: %d.%d.%d.%d\r\n",
                                    ip4_addr1_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)),
                                    ip4_addr2_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)),
                                    ip4_addr3_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)),
                                    ip4_addr4_16(&((dnsServer.dns[tmpIdx]).u_addr.ip4)));

                                    strncat(pRspStr, tmpStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN - strlen(pRspStr) - 1);
                                }
                        }
                    }

                    //second check ipv6 dns server
                    for (tmpIdx = 0; tmpIdx < dnsServer.dnsNum && tmpIdx < NM_MAX_DNS_NUM; tmpIdx++)
                    {
                        if(IP_IS_V6(&(dnsServer.dns[tmpIdx])))
                        {
                            if (strlen(pRspStr) == 0)
                            {
                                ip6Str = ip6addr_ntoa(ip_2_ip6(&(dnsServer.dns[tmpIdx])));

                                if(ip6Str != PNULL)
                                {
                                    if(btypeDns == FALSE)
                                    {
                                        snprintf(pRspStr,
                                            ATEC_QIDNSCFG_GET_CNF_STR_LEN,
                                            "PrimaryIpv6Dns: %s\r\n", ip6Str);
                                        btypeDns = TRUE;
                                    }
                                    else
                                    {
                                        snprintf(pRspStr,
                                            ATEC_QIDNSCFG_GET_CNF_STR_LEN,
                                            "SecondaryIpv6Dns: %s\r\n", ip6Str);
                                    }
                                }
                            }
                            else
                            {
                                memset(tmpStr, 0x00, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN);
                                ip6Str = ip6addr_ntoa(ip_2_ip6(&(dnsServer.dns[tmpIdx])));
                                if(ip6Str != PNULL)
                                {
                                    if(btypeDns == FALSE)
                                    {
                                        snprintf(tmpStr,
                                            ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN,
                                                "PrimaryIpv6Dns: %s\r\n", ip6Str);
                                        btypeDns = TRUE;
                                    }
                                    else
                                    {
                                        snprintf(tmpStr,
                                            ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN,
                                                "SecondaryIpv6Dns: %s\r\n", ip6Str);
                                    }

                                    strncat(pRspStr, tmpStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN - strlen(pRspStr) - 1);
                                }
                            }

                        }
                    }
                }
                else
                {
                   ECOMM_TRACE(UNILOG_ATCMD_PARSER, nmQIDNSCFG_getdnsserver_fail, P_SIG, 0,
                                     "AT CMD, get current dns server fail,load default aon dns config");
                    mwAonGetDefaultAonDnsCfg(&dnsCfg);

                    //snprintf(pRspStr, ATEC_ECDNSCFG_GET_CNF_STR_LEN, "+ECDNSCFG: ");

                    /*ipv4 DNS*/
                    for (tmpIdx = 0; tmpIdx < MID_WARE_DEFAULT_DNS_NUM; tmpIdx++)
                    {
                        if (dnsCfg.ipv4Dns[tmpIdx][0] != 0)
                        {
                            if (strlen(pRspStr) == 0)
                            {
                                snprintf(pRspStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN, "PrimaryIpv4Dns: %d.%d.%d.%d\r\n",
                                     dnsCfg.ipv4Dns[tmpIdx][0], dnsCfg.ipv4Dns[tmpIdx][1], dnsCfg.ipv4Dns[tmpIdx][2], dnsCfg.ipv4Dns[tmpIdx][3]);
                            }
                            else
                            {
                                memset(tmpStr, 0x00, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN);
                                snprintf(tmpStr, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN, "SecondaryIpv4Dns: %d.%d.%d.%d\r\n",
                                     dnsCfg.ipv4Dns[tmpIdx][0], dnsCfg.ipv4Dns[tmpIdx][1], dnsCfg.ipv4Dns[tmpIdx][2], dnsCfg.ipv4Dns[tmpIdx][3]);

                                strncat(pRspStr, tmpStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN - strlen(pRspStr) - 1);
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
                                if(btypeDns == FALSE)
                                {
                                    snprintf(pRspStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN, "PrimaryIpv6Dns: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
                                        dnsCfg.ipv6Dns[tmpIdx][0], dnsCfg.ipv6Dns[tmpIdx][1], dnsCfg.ipv6Dns[tmpIdx][2], dnsCfg.ipv6Dns[tmpIdx][3],
                                        dnsCfg.ipv6Dns[tmpIdx][4], dnsCfg.ipv6Dns[tmpIdx][5], dnsCfg.ipv6Dns[tmpIdx][6], dnsCfg.ipv6Dns[tmpIdx][7],
                                        dnsCfg.ipv6Dns[tmpIdx][8], dnsCfg.ipv6Dns[tmpIdx][9], dnsCfg.ipv6Dns[tmpIdx][10], dnsCfg.ipv6Dns[tmpIdx][11],
                                        dnsCfg.ipv6Dns[tmpIdx][12], dnsCfg.ipv6Dns[tmpIdx][13], dnsCfg.ipv6Dns[tmpIdx][14], dnsCfg.ipv6Dns[tmpIdx][15]);
                                    btypeDns = TRUE;
                                }
                                else
                                {
                                    snprintf(pRspStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN, "SecondaryIpv6Dns: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
                                        dnsCfg.ipv6Dns[tmpIdx][0], dnsCfg.ipv6Dns[tmpIdx][1], dnsCfg.ipv6Dns[tmpIdx][2], dnsCfg.ipv6Dns[tmpIdx][3],
                                        dnsCfg.ipv6Dns[tmpIdx][4], dnsCfg.ipv6Dns[tmpIdx][5], dnsCfg.ipv6Dns[tmpIdx][6], dnsCfg.ipv6Dns[tmpIdx][7],
                                        dnsCfg.ipv6Dns[tmpIdx][8], dnsCfg.ipv6Dns[tmpIdx][9], dnsCfg.ipv6Dns[tmpIdx][10], dnsCfg.ipv6Dns[tmpIdx][11],
                                        dnsCfg.ipv6Dns[tmpIdx][12], dnsCfg.ipv6Dns[tmpIdx][13], dnsCfg.ipv6Dns[tmpIdx][14], dnsCfg.ipv6Dns[tmpIdx][15]);
                                }
                            }
                            else
                            {
                                memset(tmpStr, 0x00, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN);
                                if(btypeDns == FALSE)
                                {
                                    snprintf(tmpStr, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN, "PrimaryIpv6Dns: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
                                        dnsCfg.ipv6Dns[tmpIdx][0], dnsCfg.ipv6Dns[tmpIdx][1], dnsCfg.ipv6Dns[tmpIdx][2], dnsCfg.ipv6Dns[tmpIdx][3],
                                        dnsCfg.ipv6Dns[tmpIdx][4], dnsCfg.ipv6Dns[tmpIdx][5], dnsCfg.ipv6Dns[tmpIdx][6], dnsCfg.ipv6Dns[tmpIdx][7],
                                        dnsCfg.ipv6Dns[tmpIdx][8], dnsCfg.ipv6Dns[tmpIdx][9], dnsCfg.ipv6Dns[tmpIdx][10], dnsCfg.ipv6Dns[tmpIdx][11],
                                        dnsCfg.ipv6Dns[tmpIdx][12], dnsCfg.ipv6Dns[tmpIdx][13], dnsCfg.ipv6Dns[tmpIdx][14], dnsCfg.ipv6Dns[tmpIdx][15]);
                                    btypeDns = TRUE;
                                }
                                else
                                {
                                    snprintf(tmpStr, ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN, "SecondaryIpv6Dns: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
                                        dnsCfg.ipv6Dns[tmpIdx][0], dnsCfg.ipv6Dns[tmpIdx][1], dnsCfg.ipv6Dns[tmpIdx][2], dnsCfg.ipv6Dns[tmpIdx][3],
                                        dnsCfg.ipv6Dns[tmpIdx][4], dnsCfg.ipv6Dns[tmpIdx][5], dnsCfg.ipv6Dns[tmpIdx][6], dnsCfg.ipv6Dns[tmpIdx][7],
                                        dnsCfg.ipv6Dns[tmpIdx][8], dnsCfg.ipv6Dns[tmpIdx][9], dnsCfg.ipv6Dns[tmpIdx][10], dnsCfg.ipv6Dns[tmpIdx][11],
                                        dnsCfg.ipv6Dns[tmpIdx][12], dnsCfg.ipv6Dns[tmpIdx][13], dnsCfg.ipv6Dns[tmpIdx][14], dnsCfg.ipv6Dns[tmpIdx][15]);
                                }

                                strncat(pRspStr, tmpStr, ATEC_QIDNSCFG_GET_CNF_STR_LEN - strlen(pRspStr) - 1);
                            }

                        }
                    }

                }

                if (strlen(pRspStr) != 0)
                {
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, pRspStr);
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, PNULL);
                }

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
  \fn          CmsRetId nmNIPINFO(const AtCmdInputContext *pAtCmdReq)
  \brief       AT+NIPINFO CMD codes
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  nmNIPINFO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32   mode = NIPINFO_0_MODE_DEF;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+NIPINFO=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+NIPINFO: (0,1)");
            break;
        }

        case AT_SET_REQ:      /* AT+NIPINFO= */
        {
            /* AT+NIPINFO=<n> */
            /*
            *
            */
            if ((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &mode,
                                   NIPINFO_0_MODE_MIN, NIPINFO_0_MODE_MAX, NIPINFO_0_MODE_DEF)) == AT_PARA_OK)
            {
                mwAonSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_IPINFO_RPT_MODE_CFG, mode);
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, PNULL);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
            }
            break;
        }

        case AT_READ_REQ:           /* AT+NPING?  */
        {
            mode = mwAonGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_IPINFO_RPT_MODE_CFG);
            if(mode == NIPINFO_0_MODE_MAX)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+NIPINFO: 1");
            }
            else if(mode == NIPINFO_0_MODE_MIN)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+NIPINFO: 0");
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, CME_SOCK_MGR_ERROR_PARAM_ERROR, NULL);
            }
            break;
        }
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


