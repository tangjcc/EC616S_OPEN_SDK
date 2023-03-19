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
#include "atec_ref_tcpip.h"
#include "ec_tcpip_api.h"
#include "ps_nm_if.h"
#include "atec_ref_tcpip_cnf_ind.h"
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
CmsRetId nmQDNSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    NmAtiGetDnsCnf *pNmCnf = (NmAtiGetDnsCnf *)paras;
    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if (rc == NM_SUCCESS)
    {
        snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+QDNS: \"%s\"", pNmCnf->response);
        ret = atcURC(channelId, (CHAR *)rspStr);
    }
    else    //NM_FAIL
    {
        snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+CME ERROR:%d", CME_SOCK_MGR_ERROR_DNS_RESOLVE_FAIL);
        ret = atcURC(channelId, (CHAR *)rspStr);
    }

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of AT+ECPING
*
****************************************************************************************/
static CmsRetId nmRfPingInd(UINT16 reqHandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    indBuf[ATEC_IND_RESP_256_STR_LEN] = {0};
    NmAtiPingResultInd *pPingInd = (NmAtiPingResultInd*)paras;

    UINT8 channelId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    memset(indBuf, 0, sizeof(indBuf));

    if (pPingInd->result == NM_PING_RET_ONE_SUCC)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+NPING: %s,%u,%d",
                 pPingInd->pingDst, pPingInd->ttl, pPingInd->minRtt);
    }
    else if (pPingInd->result == NM_PING_RET_ONE_FAIL)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+NPINGERR: 1");
    }
    else if (pPingInd->result == NM_PING_RET_ONE_SEND_FAIL)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+NPINGERR: 2");
    }
    else if(pPingInd->result != NM_PING_RET_DONE && pPingInd->result != NM_PING_ALL_FAIL)
    {
        snprintf(indBuf, ATEC_IND_RESP_256_STR_LEN,
                 "+NPINGERR: %u", pPingInd->result);
    }

    ret = atcURC(channelId, (CHAR *)indBuf);

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of +ECPADDR
*
****************************************************************************************/
static CmsRetId nmNIpInfoInd(UINT16 reqHandle, void *paras)
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
        if (pNetInfo->ipType == NM_NET_TYPE_IPV4)
        {
            if(pNetInfo->cause == NM_STATUS_CHANGE_LINK_UP_PDN_IPV4_ONLY )
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV4V6,\"%d.%d.%d.%d\",,1",
                     pNetInfo->ipv4Cid,
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[0],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[1],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[2],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[3]);
            }
            else if(pNetInfo->cause == NM_STATUS_CHANGE_LINK_UP_PDN_SINGLE_ADDRESS_ONLY )
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV4V6,\"%d.%d.%d.%d\",,3",
                     pNetInfo->ipv4Cid,
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[0],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[1],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[2],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[3]);
            }
            else
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IP,\"%d.%d.%d.%d\"",
                     pNetInfo->ipv4Cid,
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[0],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[1],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[2],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[3]);
             }
        }

        else if (pNetInfo->ipType == NM_NET_TYPE_IPV6)
        {
            if(pNetInfo->cause == NM_STATUS_CHANGE_LINK_UP_PDN_IPV6_ONLY )
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV4V6,,\"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\",2",
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
            else if(pNetInfo->cause == NM_STATUS_CHANGE_LINK_UP_PDN_SINGLE_ADDRESS_ONLY )
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV4V6,,\"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\",3",
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
            else
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV6,\"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\"",
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
        }
        else if (pNetInfo->ipType == NM_NET_TYPE_IPV4V6)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV4V6,\"%d.%d.%d.%d\", \"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\"",
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
        else if (pNetInfo->ipType == NM_NET_TYPE_IPV4_IPV6preparing)
        {
            if(pNetInfo->cause == NM_STATUS_CHANGE_RA_TIMEOUT)
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                     "+NIPINFO: %d,IPV4V6,\"%d.%d.%d.%d\",,4",
                     pNetInfo->ipv4Cid,
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[0],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[1],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[2],
                     ((const u8_t*)(&(pNetInfo->ipv4Info.ipv4Addr.addr)))[3]);
            }
        }
        else if (pNetInfo->ipType == NM_NET_TYPE_IPV6preparing)
        {
            if(pNetInfo->cause == NM_STATUS_CHANGE_RA_TIMEOUT)
            {
                snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN,
                    "+NIPINFO: %d,IPV6,,4",
                    pNetInfo->ipv4Cid);
            }
        }

        if (indBuf[0] != 0)
        {
            for (chanId = CMS_CHAN_1; chanId < CMS_CHAN_NUM; chanId++)
            {
                if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_IPINFO_RPT_MODE_CFG) == 1)
                {
                    atcURC(chanId, indBuf);
                }
            }
        }
    }

    return CMS_RET_SUCC;
}


#define __DEFINE_STATIC_VARIABLES__ //just for easy to find this position in SS

/******************************************************************************
 * nmCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList nmRefApplCnfHdrList[] =
{
    {NM_ATI_ASYNC_GET_DNS_CNF,          nmQDNSGetCnf},

    {NM_ATI_PRIM_END, PNULL}    /* this should be the last */
};

/******************************************************************************
 * nmCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList nmRefApplIndHdrList[] =
{
    {NM_ATI_PING_RET_IND,               nmRfPingInd},
    {NM_ATI_NET_INFO_IND,               nmNIpInfoInd},
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
void atTcpIpProcRefNmApplCnf(CmsApplCnf *pApplCnf)
{
    ApplCnfHandler  applCnfHdr = PNULL;
    UINT32  primId = pApplCnf->header.primId;
    UINT32  tmpIdx = 0;

    while (nmRefApplCnfHdrList[tmpIdx].primId != NM_ATI_PRIM_END)
    {
        if (nmRefApplCnfHdrList[tmpIdx].primId == primId)
        {
            applCnfHdr = nmRefApplCnfHdrList[tmpIdx].applCnfHdr;
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


void atTcpIpProcRefNmApplInd(CmsApplInd *pApplInd)
{
    ApplIndHandler  applIndHdr = PNULL;
    UINT32  primId = pApplInd->header.primId;
    UINT32  tmpIdx = 0;

    while (nmRefApplIndHdrList[tmpIdx].primId != NM_ATI_PRIM_END)
    {
        if (nmRefApplIndHdrList[tmpIdx].primId == primId)
        {
            applIndHdr = nmRefApplIndHdrList[tmpIdx].applIndHdr;
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


