/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_tcpip_cnf_ind.c
*
*  Description: Process TCP/IP service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "atec_tcpip.h"
#include "ec_tcpip_api.h"
#include "ps_nm_if.h"
#include "atec_tcpip_cnf_ind.h"
#include "at_sock_entity.h"
#include <time.h>
#include "cmsis_os2.h"
#include "osasys.h"



/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/

/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS


#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS




/******************************************************************************
 * atNmDNSGetCnf
 * Description: AT+CMDNS=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId nmDNSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    NmAtiGetDnsCnf *pNmCnf = (NmAtiGetDnsCnf *)paras;

    if (rc == NM_SUCCESS)
    {
        snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+ECDNS: \"%s\"", pNmCnf->response);
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspStr);
    }
    else    //NM_FAIL
    {
        ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( SOCKET_PARAM_ERROR), NULL);
    }

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of AT+ECPING
*
****************************************************************************************/
static CmsRetId nmPingInd(UINT16 reqHandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    indBuf[ATEC_IND_RESP_256_STR_LEN] = {0};
    NmAtiPingResultInd *pPingInd = (NmAtiPingResultInd*)paras;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    memset(indBuf, 0, sizeof(indBuf));

    if (pPingInd->result == NM_PING_RET_DONE) //success
    {
        sprintf(indBuf, "+ECPING: DONE");
        atcURC(channelId, (CHAR *)indBuf);

        memset(indBuf, 0, sizeof(indBuf));
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+ECPING: dest: %s, %d packets transmittted, %d received, %d %% packet loss\r rtt min/avg/max = %d / %d / %d ms",
                 pPingInd->pingDst, pPingInd->requestNum, pPingInd->responseNum, pPingInd->packetLossRate,
                 pPingInd->minRtt, pPingInd->avgRtt, pPingInd->maxRtt);
    }
    else if (pPingInd->result == NM_PING_RET_ONE_SUCC)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+ECPING: SUCC, dest: %s, RTT: %d ms",
                 pPingInd->pingDst, pPingInd->minRtt);
    }
    else if (pPingInd->result == NM_PING_RET_ONE_FAIL)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+ECPING: FAIL, dest: %s, time out: %d ms",
                 pPingInd->pingDst, pPingInd->minRtt);
    }
    else if (pPingInd->result == NM_PING_RET_ONE_SEND_FAIL)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+ECPING: SEND FAIL: %d, dest: %s",
                 pPingInd->result, pPingInd->pingDst);
    }
    else
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+ECPING: ERROR, cause: %d", pPingInd->result);
    }

    ret = atcURC(channelId, (CHAR *)indBuf);

    return ret;
}


/****************************************************************************************
*  Process CI ind msg of AT+ECIPERF
*
****************************************************************************************/
static CmsRetId nmIperfInd(UINT16 reqHandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    NmAtiIperfResultInd *pNmInd = (NmAtiIperfResultInd*)paras;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if (pNmInd->result == NM_IPERF_END_REPORT_SUCCESS) //ECIPERF END SUCC
    {
        if (pNmInd->mode == NM_IPERF_MODE_CLIENT) //clent
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Client END, pkg sent total bytes: %d, average UL through put: %d bps",
                     pNmInd->dataNum, pNmInd->bandwidth);
        }
        else
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Server END, pkg recv total bytes: %d, average DL through put: %d bps",
                     pNmInd->dataNum, pNmInd->bandwidth);
        }
    }
    else if (pNmInd->result == NM_IPERF_ONE_REPORT_SUCCESS)
    {
        if (pNmInd->mode == NM_IPERF_MODE_CLIENT) //clent
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Client SUCC, pkg sent bytes: %d, UL through put: %d bps",
                     pNmInd->dataNum, pNmInd->bandwidth);
        }
        else
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Server SUCC, pkg recv bytes: %d, DL through put: %d bps",
                     pNmInd->dataNum, pNmInd->bandwidth);
        }
    }
    else if (pNmInd->result == NM_IPERF_PARAM_ERROR)
    {
        if (pNmInd->mode == NM_IPERF_MODE_CLIENT) //clent
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Client FAIL, invalid/missing input param");
        }
        else
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Server FAIL, invalid/missing input param");
        }
    }
    else if (pNmInd->result == NM_IPERF_SOCKET_ERROR)
    {
        if (pNmInd->mode == NM_IPERF_MODE_CLIENT) //clent
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Client FAIL, socket error");
        }
        else
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Server FAIL, socket error");
        }
    }
    else if (pNmInd->result == NM_IPERF_MALLOC_ERROR)
    {
        if (pNmInd->mode == NM_IPERF_MODE_CLIENT) //clent
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Client FAIL, memory error");
        }
        else
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Server FAIL, memory error");
        }
    }
    else //fail
    {
        if (pNmInd->mode == NM_IPERF_MODE_CLIENT)
        {
            snprintf((char *)indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Client ERROR: %d", pNmInd->result);
        }
        else
        {
            snprintf((char *)indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPERF: Server ERROR: %d", pNmInd->result);
        }
    }

    ret = atcURC(channelId, (CHAR *)indBuf);

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of AT+ECSNTP
*
****************************************************************************************/
static CmsRetId nmSntpInd(UINT16 reqHandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    struct tm gtCurr;
    NmAtiSntpResultInt *pNmInd = (NmAtiSntpResultInt*)paras;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if (pNmInd->result == SNTP_RESULT_OK) //ECIPERF END SUCC
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSntpInd_1, P_INFO, 3, "nmSntpInd success,aotu sync %u, time %u, us %u", pNmInd->autoSync, pNmInd->time, pNmInd->us);
        if(pNmInd->autoSync)
        {
            if(OsaTimerSync(SNTP_TIME_SRC, SET_LOCAL_TIME, pNmInd->time, pNmInd->us, 0) == CMS_RET_SUCC)
            {
                //OsaTimerSync has set the flag
                //if(mwGetTimeSyncFlag()==FALSE)
                //{
                //    mwSetTimeSyncFlag(TRUE);
                //}
            }
        }

        gmtime_ec((time_t *)&(pNmInd->time), &gtCurr);
        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECSNTP: %02d/%02d/%02d,%02d:%02d:%02d",gtCurr.tm_year+1900,gtCurr.tm_mon,gtCurr.tm_mday,
        gtCurr.tm_hour,gtCurr.tm_min,gtCurr.tm_sec);
    }
    else //fail
    {
        snprintf((char *)indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECSNTP: ERROR: %d", pNmInd->result);
    }

    ret = atcURC(channelId, (CHAR *)indBuf);

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of +ECPADDR
*
****************************************************************************************/
static CmsRetId nmECPADDRInd(UINT16 reqHandle, void *paras)
{
    CHAR    indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    NmAtiNetInfoInd *pNetInfoInd = (NmAtiNetInfoInd*)paras;
    NmAtiNetifInfo  *pNetInfo = &(pNetInfoInd->netifInfo);
    UINT8   chanId = CMS_CHAN_1;

    OsaDebugBegin(reqHandle == BROADCAST_IND_HANDLER, reqHandle, BROADCAST_IND_HANDLER, 0);
    OsaDebugEnd();

    if (pNetInfo->netStatus == NM_NETIF_ACTIVATED ||
        pNetInfo->netStatus == NM_NETIF_ACTIVATED_INFO_CHNAGED) //success
    {
        if (pNetInfo->ipType == NM_NET_TYPE_IPV4 || pNetInfo->ipType == NM_NET_TYPE_IPV4_IPV6preparing)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+ECPADDR: %d, \"%d.%d.%d.%d\"",
                     pNetInfo->ipv4Cid,
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[0],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[1],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[2],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[3]);
        }
        else if (pNetInfo->ipType == NM_NET_TYPE_IPV6)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+ECPADDR: %d, \"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\"",
                     pNetInfo->ipv6Cid,
                     IP6_ADDR_BLOCK1(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK2(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK3(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK4(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK5(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK6(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK7(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK8(&(pNetInfo->ipv6Info.ipv6Addr)));

        }
        else if (pNetInfo->ipType == NM_NET_TYPE_IPV4V6)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+ECPADDR: %d, \"%d.%d.%d.%d\", \"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\"",
                     pNetInfo->ipv4Cid,
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[0],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[1],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[2],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[3],
                     IP6_ADDR_BLOCK1(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK2(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK3(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK4(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK5(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK6(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK7(&(pNetInfo->ipv6Info.ipv6Addr)),
                     IP6_ADDR_BLOCK8(&(pNetInfo->ipv6Info.ipv6Addr)));
        }
        else
        {
            OsaDebugBegin(FALSE, pNetInfo->ipType, pNetInfo->netStatus, 0);
            OsaDebugEnd();
        }

        if (indBuf[0] != 0)
        {
            for (chanId = CMS_CHAN_1; chanId < CMS_CHAN_NUM; chanId++)
            {
                if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECPADDR_RPT_CFG) == 1 &&
                    atcBeURCConfiged(chanId))
                {
                    atcURC(chanId, indBuf);
                }
            }
        }
    }

    return CMS_RET_SUCC;
}

static CmsRetId nmSKTGetInd(UINT16 reqHandle, UINT16 primId, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetInd_1, P_ERROR, 0, "nmSKTGetInd invalid paras");
       return ret;
    }

    switch(primId)
    {
        case APPL_SOCKET_ERROR_IND:
            ret = atcURC(channelId, (CHAR*)paras);
            break;
        case APPL_SOCKET_RCV_IND:
        {
            CHAR *buf;
            CHAR *strBuf;
            AtecSktDlInd *dlInd;
            dlInd = (AtecSktDlInd *)paras;
            buf = (CHAR *)malloc(dlInd->len*2+28);
            if(buf)
            {
                strBuf = (CHAR *)malloc(dlInd->len*2+1);
                if(strBuf)
                {
                    memset(buf, 0, dlInd->len*2+28);
                    memset(strBuf, 0, dlInd->len*2+1);

                    if (cmsHexToHexStr((CHAR *)strBuf, dlInd->len*2+1, dlInd->data, dlInd->len) > 0)
                    {
                        snprintf(buf, dlInd->len*2+28, "+SKTRECV: %d,%d,\"%s\"", dlInd->fd, dlInd->len, strBuf);
                        ret = atcURC(channelId, buf);
                    }

                    free(buf);
                    free(strBuf);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetInd_2, P_ERROR, 0, "nmSKTGetInd malloc fail");
                    free(buf);
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetInd_3, P_ERROR, 0, "nmSKTGetInd malloc fail");
            }
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetInd_4, P_ERROR, 1, "nmSKTGetInd invalid primId %d", primId);
            break;
        }
    }

    return ret;
}


/******************************************************************************
 * atNmSKTGetCnf
 * Description:
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static CmsRetId nmSKTGetCnf(UINT16 reqHandle, UINT16 primId, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    AtecSktCnf *Conf = (AtecSktCnf *)paras;
    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == APPL_RET_SUCC)
    {
        switch(primId)
        {
            case APPL_SOCKET_BIND_CNF:
            case APPL_SOCKET_CONNECT_CNF:
            case APPL_SOCKET_SEND_CNF:
            case APPL_SOCKET_DELETE_CNF:
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                break;
            }
            case APPL_SOCKET_CREATE_CNF:
            {
               sprintf(RspBuf, "+SKTCREATE: %d", Conf->cnfBody.fd);
               ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
               break;
            }
            case APPL_SOCKET_STATUS_CNF:
            {
                sprintf(RspBuf, "+SKTSTATUS: %d", Conf->cnfBody.status);
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetCnf_1, P_ERROR, 1, "nmSKTGetCnf invalid primId %d", primId);
                break;;
            }

        }
    }
    else if(rc == APPL_RET_FAIL)
    {
        switch(primId)
        {
            case APPL_SOCKET_BIND_CNF:
            case APPL_SOCKET_CONNECT_CNF:
            case APPL_SOCKET_SEND_CNF:
            case APPL_SOCKET_DELETE_CNF:
            case APPL_SOCKET_CREATE_CNF:
            case APPL_SOCKET_STATUS_CNF:
            {
                ret = atcReply(reqHandle, AT_RC_SOCKET_ERROR, ( Conf->cnfBody.errCode), NULL);
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetCnf_2, P_ERROR, 1, "nmSKTGetCnf invalid primId %d", primId);
                break;
            }

        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetCnf_3, P_ERROR, 1, "nmSKTGetCnf invalid result %d", rc);
    }

    return ret;
}


static CmsRetId nmEcSocInd(UINT16 reqHandle, UINT16 primId, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocInd_1, P_ERROR, 0, "nmEcSocInd invalid paras");
       return ret;
    }

    switch(primId)
    {
        case APPL_ECSOC_CLOSE_IND:
        {
            CHAR indBuf[32];
            EcSocCloseInd *socCloseInd = (EcSocCloseInd *)paras;
            snprintf(indBuf, sizeof(indBuf), "%s: %d,%d", ECSOCLI_NAME, socCloseInd->socketId, socCloseInd->errCode);
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        case APPL_ECSOC_QUERY_RESULT_IND:
        {
            CHAR indBuf[32];
            EcSocQueryInd *socQueryInd = (EcSocQueryInd *)paras;
            snprintf(indBuf, sizeof(indBuf), "%s: %d,%d", ECQSOS_NAME, socQueryInd->socketId, socQueryInd->sequence);
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        case APPL_ECSOC_GNMIE_IND:
        {
            CHAR indBuf[32];
            EcSocGNMIEInd *socGNMIEInd = (EcSocGNMIEInd *)paras;
            snprintf(indBuf, sizeof(indBuf), "%s: %d,%d,%d,%d", ECSONMIE_NAME, socGNMIEInd->socketId, socGNMIEInd->mode, socGNMIEInd->maxDlBufferSize, socGNMIEInd->maxDlPkgNum);
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        case APPL_ECSOC_ULSTATUS_IND:
        {
            CHAR indBuf[32];
            EcSocUlStatusInd *socUlStatusInd = (EcSocUlStatusInd *)paras;
            snprintf(indBuf, sizeof(indBuf), "%s: %d,%d,%d", ECSOSTR_NAME, socUlStatusInd->socketId, socUlStatusInd->sequence, socUlStatusInd->status);
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        case APPL_ECSOC_NMI_IND:
        {
            CHAR *buf;
            CHAR *strBuf;
            EcSocNMInd *socNMIInd =(EcSocNMInd *)paras;
            switch(socNMIInd->modeNMI)
            {
                case NMI_MODE_0:
                {
                    break;
                }
                case NMI_MODE_1:
                {
                    CHAR indBuf[32];
                    snprintf(indBuf, sizeof(indBuf), "%s: %d,%d", ECSONMI_NAME, socNMIInd->socketId, socNMIInd->length);
                    ret = atcURC(channelId, (CHAR*)indBuf);
                    break;
                }
                case NMI_MODE_2:
                case NMI_MODE_3:
                {
                    buf = malloc(socNMIInd->length*2+128);
                    strBuf = malloc(socNMIInd->length*2+1);

                    if(buf == NULL)
                    {
                        if(strBuf)
                        {
                            free(strBuf);
                        }
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocInd_2, P_ERROR, 0, "nmEcSocInd malloc buf fail");
                        break;
                    }

                    if(strBuf == NULL)
                    {
                        if(buf)
                        {
                            free(buf);
                        }
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocInd_3, P_ERROR, 0, "nmEcSocInd malloc strBuf fail");
                        break;
                    }

                    memset(buf, 0, socNMIInd->length*2+128);
                    memset(strBuf, 0, socNMIInd->length*2+1);

                    if (cmsHexToHexStr((CHAR *)strBuf, socNMIInd->length*2+1, socNMIInd->data, socNMIInd->length) > 0)
                    {
                        if(socNMIInd->modeNMI == NMI_MODE_2)
                        {
                            CHAR remoteAddr[64];

                            if (IP_IS_V4(&socNMIInd->remoteAddr))
                            {
                                snprintf(remoteAddr, 64,
                                "%d.%d.%d.%d",
                                ip4_addr1_16(&(socNMIInd->remoteAddr.u_addr.ip4)),
                                ip4_addr2_16(&(socNMIInd->remoteAddr.u_addr.ip4)),
                                ip4_addr3_16(&(socNMIInd->remoteAddr.u_addr.ip4)),
                                ip4_addr4_16(&(socNMIInd->remoteAddr.u_addr.ip4)));

                            }
                            else
                            {
                                snprintf(remoteAddr, 64,
                                "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                                    IP6_ADDR_BLOCK1(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK2(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK3(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK4(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK5(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK6(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK7(ip_2_ip6(&socNMIInd->remoteAddr)),
                                    IP6_ADDR_BLOCK8(ip_2_ip6(&socNMIInd->remoteAddr)));
                            }
#ifdef FEATURE_REF_AT_ENABLE
                            snprintf(buf, socNMIInd->length*2+128, "%s: %d,%s,%d,%d,%s", ECSONMI_NAME, socNMIInd->socketId, remoteAddr, socNMIInd->remotePort, socNMIInd->length, strBuf);
#else
                            snprintf(buf, socNMIInd->length*2+128, "%s: %d,\"%s\",%d,%d,\"%s\"", ECSONMI_NAME, socNMIInd->socketId, remoteAddr, socNMIInd->remotePort, socNMIInd->length, strBuf);
#endif
                        }
                        else
                        {
#ifdef FEATURE_REF_AT_ENABLE
                            snprintf(buf, socNMIInd->length*2+128, "%s: %d,%d,%s", ECSONMI_NAME, socNMIInd->socketId, socNMIInd->length, strBuf);
#else
                            snprintf(buf, socNMIInd->length*2+128, "%s: %d,%d,\"%s\"", ECSONMI_NAME, socNMIInd->socketId, socNMIInd->length, strBuf);
#endif
                        }
                        ret = atcURC(channelId, (CHAR*)buf);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocInd_4, P_ERROR, 0, "nmEcSocInd atHexToString fail ");
                    }
                    free(buf);
                    free(strBuf);

                }
            }
            break;
        }
        case APPL_ECSOC_STATUS_IND:
        {
            CHAR indBuf[32];
            EcSocStatusInd *socStatusInd = (EcSocStatusInd *)paras;
            if(socStatusInd->status == EC_SOCK_BACK_OFF)
            {
                snprintf(indBuf, sizeof(indBuf), "%s: %d,%u,%u", ECSOSTATUS_NAME, socStatusInd->socketId, socStatusInd->status, socStatusInd->backOffTimer);
            }
            else
            {
                snprintf(indBuf, sizeof(indBuf), "%s: %d,%u", ECSOSTATUS_NAME, socStatusInd->socketId, socStatusInd->status);
            }
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        case APPL_ECSOC_CONNECTED_IND:
        {
            CHAR indBuf[32];
            EcSocConnectedInd *socConnectedInd = (EcSocConnectedInd *)paras;
            snprintf(indBuf, sizeof(indBuf), "%s: %d", ECSOCO_NAME, socConnectedInd->socketId);
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocInd_5, P_ERROR, 1, "nmEcSocInd invalid primId %d", primId);
            break;
        }
    }

    return ret;
}


/******************************************************************************
 * nmEcSocCnf
 * Description:
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static CmsRetId nmEcSocCnf(UINT16 reqHandle, UINT16 primId, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == APPL_RET_SUCC)
    {
        switch(primId)
        {
            case APPL_ECSOC_CREATE_CNF:
            {
                EcSocCreateResponse *socCreateResp = (EcSocCreateResponse *)paras;
                if(socCreateResp)
                {
                    snprintf(RspBuf,sizeof(RspBuf), "%d", socCreateResp->socketId);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_1, P_ERROR, 0, "nmEcSocCnf APPL_ECSOC_CREATE_CNF invalid paras");
                }
                break;
            }
            case APPL_ECSOC_UDPSEND_CNF:
            case APPL_ECSOC_UDPSENDF_CNF:
            {
                EcSocUdpSendResponse *socUdpSendResp = (EcSocUdpSendResponse *)paras;
                if(socUdpSendResp)
                {
                    snprintf(RspBuf,sizeof(RspBuf), "%d,%d", socUdpSendResp->socketId, socUdpSendResp->length);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_2, P_ERROR, 0, "nmEcSocCnf APPL_ECSOC_UDPSEND_CNF invalid paras");
                }
                break;
            }
            case APPL_ECSOC_TCPSEND_CNF:
            {
                EcSocTcpSendResponse *socTcppSendResp = (EcSocTcpSendResponse *)paras;
                if(socTcppSendResp)
                {
                    snprintf(RspBuf,sizeof(RspBuf), "%d,%d", socTcppSendResp->socketId, socTcppSendResp->length);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_3, P_ERROR, 0, "nmEcSocCnf APPL_ECSOC_TCPSEND_CNF invalid paras");
                }
                break;
            }
            case APPL_ECSOC_GNMI_CNF:
            {
                EcSocGNMIResponse *socGNMIResp = (EcSocGNMIResponse *)paras;
                if(socGNMIResp)
                {
                    snprintf(RspBuf,sizeof(RspBuf), "%s:%d,%d,%d", ECSONMI_NAME, socGNMIResp->mode, socGNMIResp->maxDlBufferSize, socGNMIResp->maxDlPkgNum);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_4, P_ERROR, 0, "nmEcSocCnf APPL_ECSOC_GNMI_CNF invalid paras");
                }
                break;
            }
            case APPL_ECSOC_QUERY_CNF:
            case APPL_ECSOC_TCPCONNECT_CNF:
            case APPL_ECSOC_CLOSE_CNF:
            case APPL_ECSOC_NMI_CNF:
            case APPL_ECSOC_NMIE_CNF:
            case APPL_ECSOC_GNMIE_CNF:
            case APPL_ECSOC_STATUS_CNF:
            {
               ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
               break;
            }
            case APPL_ECSOC_READ_CNF:
            {
                EcSocReadResponse *socReadResp = (EcSocReadResponse *)paras;
                if(socReadResp)
                {
                    if(socReadResp->length > 0)
                    {
                        CHAR remoteAddr[64];
                        CHAR *buf;
                        CHAR *strBuf;

                        if (IP_IS_V4(&socReadResp->remoteAddr))
                        {
                            snprintf(remoteAddr, 64,
                            "%d.%d.%d.%d",
                            ip4_addr1_16(&(socReadResp->remoteAddr.u_addr.ip4)),
                            ip4_addr2_16(&(socReadResp->remoteAddr.u_addr.ip4)),
                            ip4_addr3_16(&(socReadResp->remoteAddr.u_addr.ip4)),
                            ip4_addr4_16(&(socReadResp->remoteAddr.u_addr.ip4)));
                        }
                        else
                        {
                            snprintf(remoteAddr, 64,
                            "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                            IP6_ADDR_BLOCK1(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK2(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK3(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK4(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK5(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK6(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK7(ip_2_ip6(&socReadResp->remoteAddr)),
                            IP6_ADDR_BLOCK8(ip_2_ip6(&socReadResp->remoteAddr)));
                        }

                        buf = malloc(socReadResp->length*2+128);
                        strBuf = malloc(socReadResp->length*2+1);

                        if(buf == NULL)
                        {
                            if(strBuf)
                            {
                                free(strBuf);
                            }
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_5, P_ERROR, 0, "nmEcSocCnf malloc buf fail");
                            break;
                        }

                        if(strBuf == NULL)
                        {
                            if(buf)
                            {
                                free(buf);
                            }
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_6, P_ERROR, 0, "nmEcSocCnf malloc strBuf fail");
                            break;
                        }

                        memset(buf, 0, socReadResp->length*2+128);
                        memset(strBuf, 0, socReadResp->length*2+1);

                        //if(atHexToString((UINT8*)strBuf, socReadResp->data, socReadResp->length) == TRUE)
                        if (cmsHexToHexStr(strBuf, socReadResp->length*2+1, socReadResp->data, socReadResp->length) > 0)
                        {
#ifdef FEATURE_REF_AT_ENABLE
                            snprintf(buf, socReadResp->length*2+128, "%d,%s,%d,%d,%s,%d", socReadResp->socketId, remoteAddr, socReadResp->remotePort, socReadResp->length, strBuf, socReadResp->remainingLen);
#else
                            snprintf(buf, socReadResp->length*2+128, "%d,\"%s\",%d,%d,\"%s\",%d", socReadResp->socketId, remoteAddr, socReadResp->remotePort, socReadResp->length, strBuf, socReadResp->remainingLen);
#endif
                            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)buf);
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_7, P_ERROR, 0, "nmEcSocCnf atHexToString fail ");
                        }
                        free(buf);
                        free(strBuf);

                    }
                    else
                    {
//                        snprintf(RspBuf, sizeof(RspBuf), "%d,,,%d,,%d", socReadResp->socketId, socReadResp->length, socReadResp->remainingLen);
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCn_11, P_INFO, 3, "nmEcSocCnf  read socketId %d, lenghth %d, remaininglen %d", socReadResp->socketId, socReadResp->length, socReadResp->remainingLen);
                        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    }


                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCn_8, P_ERROR, 0, "nmEcSocCnf APPL_ECSOC_READ_CNF invalid paras");
                }
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCnf_9, P_ERROR, 1, "nmSKTGetCnf invalid primId %d", primId);
                break;;
            }

        }
    }
    else if(rc == APPL_RET_FAIL)
    {
        switch(primId)
        {
            case APPL_ECSOC_CREATE_CNF:
            case APPL_ECSOC_UDPSEND_CNF:
            case APPL_ECSOC_UDPSENDF_CNF:
            case APPL_ECSOC_QUERY_CNF:
            case APPL_ECSOC_READ_CNF:
            case APPL_ECSOC_TCPCONNECT_CNF:
            case APPL_ECSOC_TCPSEND_CNF:
            case APPL_ECSOC_CLOSE_CNF:
            case APPL_ECSOC_NMI_CNF:
            case APPL_ECSOC_NMIE_CNF:
            case APPL_ECSOC_GNMI_CNF:
            case APPL_ECSOC_GNMIE_CNF:
            case APPL_ECSOC_STATUS_CNF:
            {
                AtecEcSocErrCnf *socErrCnf = (AtecEcSocErrCnf *)paras;
                if(socErrCnf)
                {
                    ret = atcReply(reqHandle, AT_RC_ECSOC_ERROR, socErrCnf->errCode, NULL);
                    //(REQHANDLE, RESULT, ERRCODE, RESPSTRING)
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSocCn_10, P_ERROR, 0, "nmEcSocCnf  invalid paras");
                }
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetCnf_2, P_ERROR, 1, "nmSKTGetCnf invalid primId %d", primId);
                break;
            }

        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSKTGetCnf_3, P_ERROR, 1, "nmSKTGetCnf invalid result %d", rc);
    }

    return ret;
}

static CmsRetId nmEcSrvSocInd(UINT16 reqHandle, UINT16 primId, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocInd_1, P_ERROR, 0, "nmEcSrvSocInd invalid paras");
       return ret;
    }

    switch(primId)
    {
        case APPL_ECSRVSOC_CREATE_TCP_LISTEN_IND:
        {
            CHAR indBuf[64];
            EcSrcSocCreateTcpListenInd *srcSocCreateTcpListenInd = (EcSrcSocCreateTcpListenInd *)paras;
            if(srcSocCreateTcpListenInd)
            {
                if(srcSocCreateTcpListenInd->result == CME_SUCC)
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCCRTCP: %d,OK", srcSocCreateTcpListenInd->socketId);
                }
                else if(srcSocCreateTcpListenInd->result == CME_SOCK_MGR_ERROR_SERVER_HASE_CREATED)
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCCRTCP: %d,Already Listening", srcSocCreateTcpListenInd->socketId);
                }
                else
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCCRTCP: %d,ERROR", srcSocCreateTcpListenInd->result);
                }
                ret = atcURC(channelId, (CHAR*)indBuf);
            }
            break;
        }
        case APPL_ECSRVSOC_SERVER_ACCEPT_CLIENT_IND:
        {
            CHAR indBuf[128];
            CHAR clientAddr[64];
            EcSrvSocTcpAcceptClientReaultInd *srvSocTcpAcceptClientReaultInd =(EcSrvSocTcpAcceptClientReaultInd *)paras;

            if(srvSocTcpAcceptClientReaultInd != PNULL)
            {
                if (IP_IS_V4(&srvSocTcpAcceptClientReaultInd->clientAddr))
                {
                    snprintf(clientAddr, 64,
                        "%d.%d.%d.%d",
                        ip4_addr1_16(&(srvSocTcpAcceptClientReaultInd->clientAddr.u_addr.ip4)),
                        ip4_addr2_16(&(srvSocTcpAcceptClientReaultInd->clientAddr.u_addr.ip4)),
                        ip4_addr3_16(&(srvSocTcpAcceptClientReaultInd->clientAddr.u_addr.ip4)),
                        ip4_addr4_16(&(srvSocTcpAcceptClientReaultInd->clientAddr.u_addr.ip4)));
                }
                else
                {
                    snprintf(clientAddr, 64,
                        "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                        IP6_ADDR_BLOCK1(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK2(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK3(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK4(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK5(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK6(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK7(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)),
                        IP6_ADDR_BLOCK8(ip_2_ip6(&srvSocTcpAcceptClientReaultInd->clientAddr)));
                }
                snprintf(indBuf, sizeof(indBuf), "+ECACCEPTTCPCLIENTSOCKET: %d,%d,%s,%d", srvSocTcpAcceptClientReaultInd->serverSocketId, srvSocTcpAcceptClientReaultInd->clientSocketId, clientAddr, srvSocTcpAcceptClientReaultInd->clientPort);
                ret = atcURC(channelId, (CHAR*)indBuf);
            }
            break;
        }
        case APPL_ECSRVSOC_STATUS_TCP_LISTEN_IND:
        {
            CHAR indBuf[64];
            EcSrvSocTcpListenStatusInd *srvSocTcpListenStatusInd = (EcSrvSocTcpListenStatusInd *)paras;
            if(srvSocTcpListenStatusInd != PNULL)
            {
                if(srvSocTcpListenStatusInd->status == SOCK_CONN_STATUS_CONNECTED)
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCTCPLISTENSTATUS: %d,Listening", srvSocTcpListenStatusInd->serverSocketId);
                }
                else
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCTCPLISTENSTATUS: %d,Not Listening", srvSocTcpListenStatusInd->serverSocketId);
                }
                ret = atcURC(channelId, (CHAR*)indBuf);
            }
            break;
        }
        case APPL_ECSRVSOC_RECEIVE_TCP_CLIENT_IND:
        {
            CHAR *buf;
            CHAR *strBuf;
            EcSrvSocTcpClientReceiveInd *srvSocTcpClientReceiveInd =(EcSrvSocTcpClientReceiveInd *)paras;
            if(srvSocTcpClientReceiveInd != PNULL)
            {

                buf = malloc(srvSocTcpClientReceiveInd->length*2+128);
                if(buf != PNULL)
                {
                    strBuf = malloc(srvSocTcpClientReceiveInd->length*2+1);
                    if(strBuf != PNULL)
                    {
                        memset(buf, 0, srvSocTcpClientReceiveInd->length*2+128);
                        memset(strBuf, 0, srvSocTcpClientReceiveInd->length*2+1);

                        if (cmsHexToHexStr((CHAR *)strBuf, srvSocTcpClientReceiveInd->length*2+1, srvSocTcpClientReceiveInd->data, srvSocTcpClientReceiveInd->length) > 0)
                        {
                            snprintf(buf, srvSocTcpClientReceiveInd->length*2+128, "+ECSRVSOTCPCLTRCV: %d,%d,\"%s\"", srvSocTcpClientReceiveInd->socketId, srvSocTcpClientReceiveInd->length, strBuf);
                            ret = atcURC(channelId, (CHAR*)buf);
                        }
                        free(buf);
                        free(strBuf);
                    }
                    else
                    {
                        free(buf);
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocInd_2, P_WARNING, 0, "nmEcSrvSocInd malloc fail");
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocInd_3, P_WARNING, 0, "nmEcSrvSocInd malloc fail");
                }


            }
            break;
        }
        case APPL_ECSRVSOC_CLOSE_TCP_CONNECTION_IND:
        {
            CHAR indBuf[64];
            EcSrvSocCloseInd *srvSocCloseInd = (EcSrvSocCloseInd *)paras;
            if(srvSocCloseInd != PNULL)
            {
                if(srvSocCloseInd->bServer == TRUE)
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCLISTENTCPCLOSE: %d,%d", srvSocCloseInd->socketId, srvSocCloseInd->errCode);
                }
                else
                {
                    snprintf(indBuf, sizeof(indBuf), "+ECSRVSOCCLIENTCPCLOSE: %d,%d", srvSocCloseInd->socketId, srvSocCloseInd->errCode);
                }
            }
            ret = atcURC(channelId, (CHAR*)indBuf);
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocInd_4, P_ERROR, 1, "nmEcSrvSocInd invalid primId %d", primId);
            break;
        }
    }

    return ret;
}

/******************************************************************************
 * nmEcSrvSocCnf
 * Description:
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static CmsRetId nmEcSrvSocCnf(UINT16 reqHandle, UINT16 primId, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == APPL_RET_SUCC)
    {
        switch(primId)
        {
            case APPL_ECSRVSOC_CREATE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF:
            {
               ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
               break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocCnf_1, P_ERROR, 1, "nmEcSrvSocCnf invalid primId %d", primId);
                break;;
            }

        }
    }
    else if(rc == APPL_RET_FAIL)
    {
        switch(primId)
        {
            case APPL_ECSRVSOC_CREATE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF:
            {
                AtecEcSrvSocErrCnf *socErrCnf = (AtecEcSrvSocErrCnf *)paras;
                if(socErrCnf)
                {
                    ret = atcReply(reqHandle, AT_RC_ECSRVSOC_ERROR, socErrCnf->errCode, NULL);
                    //(REQHANDLE, RESULT, ERRCODE, RESPSTRING)
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocCnf_2, P_ERROR, 0, "nmEcSrvSocCnf  invalid paras");
                }
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocCnf_3, P_ERROR, 1, "nmEcSrvSocCnf invalid primId %d", primId);
                break;
            }

        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmEcSrvSocCnf_4, P_ERROR, 1, "nmEcSrvSocCnf invalid result %d", rc);
    }

    return ret;
}


#define __DEFINE_STATIC_VARIABLES__ //just for easy to find this position in SS

/******************************************************************************
 * nmCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList nmApplCnfHdrList[] =
{
    {NM_ATI_ASYNC_GET_DNS_CNF,          nmDNSGetCnf},

    {NM_ATI_PRIM_END, PNULL}    /* this should be the last */
};

/******************************************************************************
 * nmCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList nmApplIndHdrList[] =
{
    {NM_ATI_PING_RET_IND,               nmPingInd},
    {NM_ATI_IPERF_RET_IND,              nmIperfInd},
    {NM_ATI_NET_INFO_IND,               nmECPADDRInd},
    {NM_ATI_SNTP_RET_IND,               nmSntpInd},

    {NM_ATI_PRIM_END, PNULL}    /* this should be the last */
};


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


/*
*/
void atTcpIpProcNmApplCnf(CmsApplCnf *pApplCnf)
{
    ApplCnfHandler  applCnfHdr = PNULL;
    UINT32  primId = pApplCnf->header.primId;
    UINT32  tmpIdx = 0;

    while (nmApplCnfHdrList[tmpIdx].primId != NM_ATI_PRIM_END)
    {
        if (nmApplCnfHdrList[tmpIdx].primId == primId)
        {
            applCnfHdr = nmApplCnfHdrList[tmpIdx].applCnfHdr;
            break;
        }
        tmpIdx++;
    }

    if (applCnfHdr != PNULL)
    {
        (*applCnfHdr)(pApplCnf->header.srcHandler, pApplCnf->header.rc, pApplCnf->body);
    }

    return;
}


void atTcpIpProcNmApplInd(CmsApplInd *pApplInd)
{
    ApplIndHandler  applIndHdr = PNULL;
    UINT32  primId = pApplInd->header.primId;
    UINT32  tmpIdx = 0;

    while (nmApplIndHdrList[tmpIdx].primId != NM_ATI_PRIM_END)
    {
        if (nmApplIndHdrList[tmpIdx].primId == primId)
        {
            applIndHdr = nmApplIndHdrList[tmpIdx].applIndHdr;
            break;
        }
        tmpIdx++;
    }

    //CmsRetId (*ApplIndHandler)(UINT16 indHandle, void *paras)
    if (applIndHdr != PNULL)
    {
        (*applIndHdr)(pApplInd->header.reqHandler, pApplInd->body);
    }

    return;
}

void atTcpIpProcSktApplCnf(CmsApplCnf *pApplCnf)
{
    nmSKTGetCnf(pApplCnf->header.srcHandler, pApplCnf->header.primId, pApplCnf->header.rc, pApplCnf->body);
}

void atTcpIpProcSktApplInd(CmsApplInd *pApplInd)
{
    nmSKTGetInd(pApplInd->header.reqHandler, pApplInd->header.primId, pApplInd->body);
}

void atTcpIpProcSocApplCnf(CmsApplCnf *pApplCnf)
{
    nmEcSocCnf(pApplCnf->header.srcHandler, pApplCnf->header.primId, pApplCnf->header.rc, pApplCnf->body);
}

void atTcpIpProcSocApplInd(CmsApplInd *pApplInd)
{
    nmEcSocInd(pApplInd->header.reqHandler, pApplInd->header.primId, pApplInd->body);
}

void atTcpIpProcSrvSocApplCnf(CmsApplCnf *pApplCnf)
{
    nmEcSrvSocCnf(pApplCnf->header.srcHandler, pApplCnf->header.primId, pApplCnf->header.rc, pApplCnf->body);
}

void atTcpIpProcSrvSocApplInd(CmsApplInd *pApplInd)
{
    nmEcSrvSocInd(pApplInd->header.reqHandler, pApplInd->header.primId, pApplInd->body);
}



