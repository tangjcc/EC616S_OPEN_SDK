/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017- Copyrights of EigenComm Ltd.
 File name:      - psdial.c
 Description:    - DIAL functions:
                   a> power on procedure
                   b> LWIP psif management
 History:        - 04/12/2018, Originated by jcweng
 ******************************************************************************
******************************************************************************/
#include "netmgr.h"
#include "cmidev.h"
#include "cmimm.h"
#include "cmips.h"
#include "cmisim.h"
#include "cmisms.h"
#include "psdial.h"
#include "ps_ps_if.h"
#include "ps_dev_if.h"
#include "debug_log.h"
#include "psdial_plmn_cfg.h"
#include "mw_aon_info.h"


#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

/******************************************************************************
 ******************************************************************************
 GOLBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS


/*
 * PS DAIL context
*/
PsDialContext   psDialCtx =
{
    .bNetifActed    = FALSE,
    .bActIfDuringWakeup = FALSE,
    .cfunState      = CMI_DEV_MIN_FUNC,
    .bSilentReset   = FALSE,

    .actingCid      = CMI_PS_INVALID_CID,
    .actingCidCgevReason = CMI_PS_PDN_TYPE_REASON_NULL,
    .psConnStatus   = FALSE
};



/******************************************************************************
 ******************************************************************************
 static functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS

/******************************************************************************
 * psDialGetPlmnCfgByImsi
 * Description: Get PLMN configuration by IMSI
 * input: CmiSimImsiStr *pImsiStr; //IMSI
 * output: DialPlmnCfgInfo*;
 * Comment: if not found, return NULL
******************************************************************************/
static const DialPlmnCfgInfo* psDialGetPlmnCfgByImsi(CmiSimImsiStr *pImsiStr)
{
    UINT16 mcc = 0;
    UINT16 mnc = 0;
    UINT8  tmpIdx = 0;
    const DialPlmnCfgInfo *pPlmnCfg = PNULL;

    OsaCheck(pImsiStr != PNULL, pImsiStr, 0, 0);

    OsaDebugBegin(pImsiStr->length >= 6 && pImsiStr->length <= CMI_SIM_MAX_IMSI_STRING_LENGTH, pImsiStr->length, CMI_SIM_MAX_IMSI_STRING_LENGTH, 0);
    return PNULL;
    OsaDebugEnd();

    /*
     * check IMSI string
    */
    for (tmpIdx = 0; tmpIdx < pImsiStr->length; tmpIdx++)
    {
        if (pImsiStr->contents[tmpIdx] < '0' ||
            pImsiStr->contents[tmpIdx] > '9')
        {
            OsaDebugBegin(FALSE, tmpIdx, pImsiStr->contents[tmpIdx], '0');
            return PNULL;
            OsaDebugEnd();
        }
    }

    /*"4600010" */
    mcc = (((pImsiStr->contents[0]-'0')&0x0F) << 8) |
          (((pImsiStr->contents[1]-'0')&0x0F) << 4) |
          ((pImsiStr->contents[2]-'0')&0x0F);

    if (pImsiStr->bThreeDigitalMnc)
    {
        mnc = (((pImsiStr->contents[3]-'0')&0x0F) << 8) |
              (((pImsiStr->contents[4]-'0')&0x0F) << 4) |
              ((pImsiStr->contents[5]-'0')&0x0F);
    }
    else
    {
        mnc = (((pImsiStr->contents[3]-'0')&0x0F) << 4) |
              ((pImsiStr->contents[4]-'0')&0x0F);
    }

    OsaDebugBegin(mcc != 0 || mnc != 0, mcc, mnc, 0);
    return PNULL;
    OsaDebugEnd();

    pPlmnCfg = psDialGetPlmnCfgByPlmn(mcc, mnc);

    if (pPlmnCfg == PNULL)
    {
        ECOMM_TRACE(UNILOG_PS_DAIL, psDialGetPlmnCfgByImsi_1, P_WARNING, 2,
                    "Can't find DialPlmnCfg for PLMN: 0x%x, 0x%x", mcc, mnc);
    }

    return pPlmnCfg;
}

/******************************************************************************
 * psDialPlmnApnConfig
 * Description: config default APN accoding to IMSI, during start up procedure
 * input: const DialPlmnCfgInfo *pPlmnCfg
 * output: void
 * Comment:
******************************************************************************/
static void psDialPlmnApnConfig(const DialPlmnCfgInfo *pPlmnCfg)
{
    AttachedBearCtxPreSetParam attachedBearerCtx;

    memset(&attachedBearerCtx, 0x00, sizeof(AttachedBearCtxPreSetParam));

    /*
     * To set the attached bearer
     * AT+ECATTBEARER=<PDP_type>[,<eitf>[,<apn>[,<IPv4AddrAlloc>[,<NSLPI>[,<IPv4_MTU_discovery>[,<NonIP_MTU_discovery>
     *            [,<PCSCF_discovery>[,<IM_CN_Signalling_Flag_Ind>]]]]]]]]
    */
    if (pPlmnCfg)
    {
        /* etif flag */
        attachedBearerCtx.eitfPresent = TRUE;
        attachedBearerCtx.eitf        = pPlmnCfg->eitf;

        /* ip type */
        if (pPlmnCfg->ipType == DIAL_CFG_IPV4)
        {
            attachedBearerCtx.pdnType = CMI_PS_PDN_TYPE_IP_V4;
            attachedBearerCtx.ipv4AlloTypePresent = TRUE;
            attachedBearerCtx.ipv4AlloType = CMI_IPV4_ADDR_ALLOC_THROUGH_NAS_SIGNALNG;
            attachedBearerCtx.ipv4MtuDisTypePresent = TRUE;
            attachedBearerCtx.ipv4MtuDisByNas = TRUE;
        }
        else if (pPlmnCfg->ipType == DIAL_CFG_IPV6)
        {
            attachedBearerCtx.pdnType = CMI_PS_PDN_TYPE_IP_V6;
        }
        else if (pPlmnCfg->ipType == DIAL_CFG_IPV4V6)
        {
            attachedBearerCtx.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;

            attachedBearerCtx.ipv4AlloTypePresent = TRUE;
            attachedBearerCtx.ipv4AlloType = CMI_IPV4_ADDR_ALLOC_THROUGH_NAS_SIGNALNG;
            attachedBearerCtx.ipv4MtuDisTypePresent = TRUE;
            attachedBearerCtx.ipv4MtuDisByNas = TRUE;
        }
        else if (pPlmnCfg->ipType == DIAL_CFG_NON_IP)
        {
            attachedBearerCtx.pdnType = CMI_PS_PDN_TYPE_NON_IP;

            attachedBearerCtx.nonIpMtuDisTypePresent = TRUE;
            attachedBearerCtx.nonIpMtuDisByNas = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, pPlmnCfg->ipType, pPlmnCfg->plmn.mcc, pPlmnCfg->plmn.mnc);
            OsaDebugEnd();

            attachedBearerCtx.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;

            attachedBearerCtx.ipv4AlloTypePresent = TRUE;
            attachedBearerCtx.ipv4AlloType = CMI_IPV4_ADDR_ALLOC_THROUGH_NAS_SIGNALNG;
            attachedBearerCtx.ipv4MtuDisTypePresent = TRUE;
            attachedBearerCtx.ipv4MtuDisByNas = TRUE;
        }

        /* APN */
        if (strlen((const char *)(pPlmnCfg->pApn)) > 0)
        {
            if (strlen((const char *)(pPlmnCfg->pApn)) <= CMI_PS_MAX_APN_LEN)
            {
                attachedBearerCtx.apnLength = strlen((const char *)(pPlmnCfg->pApn));
                strncpy((char *)(attachedBearerCtx.apnStr), (const char *)(pPlmnCfg->pApn), strlen((const char *)(pPlmnCfg->pApn)));
            }
            else
            {
                OsaDebugBegin(FALSE, strlen((const char *)(pPlmnCfg->pApn)), pPlmnCfg->plmn.mcc, pPlmnCfg->plmn.mnc);
                OsaDebugEnd();
            }
        }

        if (attachedBearerCtx.apnLength == 0 && attachedBearerCtx.eitf == TRUE)
        {
            ECOMM_TRACE(UNILOG_PS_DAIL, psDialStartUpPlmnConfig_1, P_WARNING, 2,
                       "AT DAIL, PLMN: 0x%x, 0x%x, ESM EITF is config to TRUE, but not APN configed, change EITF to FALSE",
                       pPlmnCfg->plmn.mcc, pPlmnCfg->plmn.mnc);
            attachedBearerCtx.eitf = FALSE;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_PS_DAIL, psDialStartUpPlmnConfig_2, P_WARNING, 0,
                    "AT DAIL, start up procedure, no PLMN config found for current IMSI, use the default ATTBEARER parameters");

        /* etif flag */
        attachedBearerCtx.eitfPresent = TRUE;
        attachedBearerCtx.eitf        = FALSE;

        attachedBearerCtx.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
        attachedBearerCtx.ipv4AlloTypePresent = TRUE;
        attachedBearerCtx.ipv4AlloType = CMI_IPV4_ADDR_ALLOC_THROUGH_NAS_SIGNALNG;
        attachedBearerCtx.ipv4MtuDisTypePresent = TRUE;
        attachedBearerCtx.ipv4MtuDisByNas = TRUE;

        /* no APN set*/
    }

    psSetAttBear(PS_DIAL_REQ_HANDLER, &attachedBearerCtx);

    return;
}

/******************************************************************************
 * psDialPlmnBandConfig
 * Description: config BAND accoding to IMSI, during start up procedure
 * input: const DialPlmnCfgInfo *pPlmnCfg
 * output: void
 * Comment:
******************************************************************************/
static void psDialPlmnBandConfig(const DialPlmnCfgInfo *pPlmnCfg)
{
    CmiDevSetCiotBandReq    cmiSetBandReq;

    /*
     * In current architecture, this API is useless, as protocol stack and PS dail in the same core, could call API directly
    */
    memset(&cmiSetBandReq, 0x00, sizeof(CmiDevSetCiotBandReq));
    cmiSetBandReq.bRefAT    = FALSE;
    cmiSetBandReq.nwMode    = CMI_DEV_NB_MODE;
    cmiSetBandReq.bandNum   = 0;

    devSetCiotBandModeReq(PS_DIAL_REQ_HANDLER, &cmiSetBandReq);

    return;
}


/******************************************************************************
 * psDialProcCmiPsBearerActedInd
 * Description: AT DIAL PROC "CmiPsBearerActedInd"
 * input: CmiPsBearerActedInd *pBearerActedInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsBearerActedInd(CmiPsBearerActedInd *pBearerActedInd)
{
    OsaDebugBegin(pBearerActedInd != PNULL, pBearerActedInd, 0, 0);
    return;
    OsaDebugEnd();

    if (psDialCtx.cfunState != CMI_DEV_FULL_FUNC)
    {
        OsaDebugBegin(FALSE, psDialCtx.cfunState, CMI_DEV_FULL_FUNC, 0);
        OsaDebugEnd();

        psDialCtx.cfunState = CMI_DEV_FULL_FUNC;
    }

    /*
     * try to read bearer context
    */
    if (pBearerActedInd->bearerType == CMI_PS_BEARER_DEFAULT)
    {
        /* local record the context, and get the whole context */
        /*
         * before should no recorded cid
        */
        OsaDebugBegin(psDialCtx.actingCid == CMI_PS_INVALID_CID && pBearerActedInd->cid <= CMI_PS_MAX_VALID_CID,
                      psDialCtx.actingCid, pBearerActedInd->cid, CMI_PS_INVALID_CID);
        OsaDebugEnd();

        psDialCtx.actingCid         = pBearerActedInd->cid;
        psDialCtx.actingCidCgevReason   = pBearerActedInd->pdnReason;

        psGetCGCONTRDPParam(PS_DIAL_REQ_HANDLER, pBearerActedInd->cid);
    }
    else
    {
        // other bearer not supported by NB now
        ECOMM_TRACE(UNILOG_PS_DAIL, psDialProcCmiPsBearerActedInd_1, P_WARNING, 2, "NB not support bearer type: %d; cid: %d", pBearerActedInd->bearerType, pBearerActedInd->cid);
    }

    return;
}

#if 0
/******************************************************************************
 * psDialProcCmiMmESigQInd
 * Description: AT DIAL PROC "CmiMmCesqInd"
 * input: CmiMmCesqInd *pMmCesqInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiMmEsigQInd(CmiMmCesqInd *pMmCesqInd)
{
    return;
}

/******************************************************************************
 * psDialProcCmiMmESigQInd
 * Description: AT DIAL PROC "CmiPsBearerActedInd"
 * input: CmiPsBearerActedInd *pBearerActedInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiMmCregInd(CmiMmCregInd *pMmCregInd)
{
    OsaDebugBegin(pMmCregInd != PNULL, pMmCregInd, 0, 0);
    return;
    OsaDebugEnd();

    return;
}
#endif

/******************************************************************************
 * psDialProccCmiPsBearerDeActInd
 * Description: AT DIAL PROC "CmiPsBearerDeActInd"
 * input: CmiPsBearerDeActInd *pBearerDeActedInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsBearerDeActInd(CmiPsBearerDeActInd *pBearerDeActedInd)
{
    OsaDebugBegin(pBearerDeActedInd != PNULL && pBearerDeActedInd->cid <= 10, pBearerDeActedInd, 0, 0);
    return;
    OsaDebugEnd();

    /*
     * print and notify LWIP
    */
    ECOMM_TRACE(UNILOG_PS_DAIL, psDialProccCmiPsBearerDeActInd_1, P_VALUE, 1, "CID: %d, deactivated.", pBearerDeActedInd->cid);

    //OsaPrintf(P_VALUE, "CID: %d, DEACTIVATED", pBearerDeActedInd->cid);

    if (NetMgrLinkDown(pBearerDeActedInd->cid) != NM_SUCCESS)
    {
        // print a warning
        OsaDebugBegin(FALSE, 0, 0, 0);
        OsaDebugEnd();
    }

    // only default bearer now, in fact this flag should set in LWIP net-manager side
    psDialCtx.bNetifActed = FALSE;
    return;
}

/******************************************************************************
 * psDialProcCmiPsCEREGInd
 * Description: AT DIAL PROC "CmiPsCeregInd", just notify netmgr
 * input: CmiPsCeregInd *pPsCeregInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsCEREGInd(CmiPsCeregInd *pPsCeregInd)
{
    NetMgrProcCeregInd(pPsCeregInd);
    return;
}

/******************************************************************************
 * psDialProcCmiPsConnStatusInd
 * Description: AT DIAL PROC "CMI_PS_CONN_STATUS_IND"
 * input: CmiPsBearerDeActInd *pPsConnStatusInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsConnStatusInd(CmiPsConnStatusInd *pPsConnStatusInd)
{
    OsaDebugBegin(pPsConnStatusInd != PNULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    if (CMI_Ps_CONNECTED_MODE == pPsConnStatusInd->connMode)
    {
        psDialCtx.psConnStatus = TRUE;
    }
    else
    {
        psDialCtx.psConnStatus = FALSE;
    }

    return;
}

/******************************************************************************
 * psDialProcCmiDevBandApnAutoCfgReqInd
 * Description: AT DIAL PROC "CmiDevBandApnAutoCfgReqInd", just find the local PLMN
 *              configuration, and set BAND or APN
 * input: CmiDevBandApnAutoCfgReqInd *pBandApnCfgReq
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevBandApnAutoCfgReqInd(CmiDevBandApnAutoCfgReqInd *pBandApnCfgReq)
{
    const DialPlmnCfgInfo *pPlmnCfg = PNULL;

    OsaCheck(pBandApnCfgReq != PNULL, pBandApnCfgReq, 0, 0);

    OsaDebugBegin(pBandApnCfgReq->bCfgApn || pBandApnCfgReq->bCfgBand, pBandApnCfgReq->bCfgApn, pBandApnCfgReq->bCfgBand, 0);
    return;
    OsaDebugEnd();

    pPlmnCfg = psDialGetPlmnCfgByImsi(&(pBandApnCfgReq->imsiStr));

    if (pBandApnCfgReq->bCfgApn)
    {
        psDialPlmnApnConfig(pPlmnCfg);
    }

    if (pBandApnCfgReq->bCfgBand)
    {
        /*
         * In current architecture, this branch is useless,
         *  as protocol stack and PS dail in the same core, could call API to get prefer band directly
        */

        OsaDebugBegin(FALSE, pBandApnCfgReq->bCfgBand, 0, 0);
        OsaDebugEnd();
        psDialPlmnBandConfig(pPlmnCfg);
    }

    return;
}

/******************************************************************************
 * psDialProcCmiDevPowerOnCfunInd
 * Description: AT DIAL PROC "CmiDevPowerOnCfunInd", just notify power on cfun stata
 * input:   CmiDevPowerOnCfunInd *pCmiPowerOnCfunInd
 * output:  void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevPowerOnCfunInd(CmiDevPowerOnCfunInd *pCmiPowerOnCfunInd)
{
    OsaDebugBegin(pCmiPowerOnCfunInd->func == CMI_DEV_MIN_FUNC ||
                  pCmiPowerOnCfunInd->func == CMI_DEV_FULL_FUNC ||
                  pCmiPowerOnCfunInd->func == CMI_DEV_TURN_OFF_RF_FUNC,
                  pCmiPowerOnCfunInd->func, CMI_DEV_MIN_FUNC, CMI_DEV_FULL_FUNC);
    return;
    OsaDebugEnd();

    psDialCtx.cfunState = pCmiPowerOnCfunInd->func;

    return;
}

/******************************************************************************
 * psDialProcCmiDevSilentResetInd
 * Description: PS silent reset indication
 * input:   void
 * output:  void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevSilentResetInd(void)
{
    /* Trigger a silent reset in PS side,send CFUN0 firstly */
    devSetFunc(PS_DIAL_REQ_HANDLER, CMI_DEV_MIN_FUNC, CMI_DEV_DO_NOT_RESET);
    psDialCtx.bSilentReset = TRUE;

    ECOMM_TRACE(UNILOG_PS_DAIL, psDialProcCmiDevSilentResetInd_1, P_WARNING, 0,
                "PS Silent Reset!");

    return;
}

/******************************************************************************
 * psDialProcCmiPsReadDynBearerCtxParamCnf
 * Description: AT DIAL PROC "CmiPsReadBearerDynCtxParamCnf"
 * input: uint16 rc;
 *        CmiPsReadBearerDynCtxParamCnf *pDynBearerCtxParamCnf;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsReadDynBearerCtxParamCnf(UINT16 rc, CmiPsReadBearerDynCtxParamCnf *pDynBearerCtxParamCnf)
{
    struct NmIfConfiguration_Tag ifCfg;    //124 bytes
    BOOL    bIpBearer = FALSE;
    UINT8   tmpIdx = 0;
    NmResult    ret = NM_FAIL;

    if (rc == CME_PDN_NOT_ACTIVED)
    {
        // no bearer activated, return
        psDialCtx.bActIfDuringWakeup = FALSE;
        return;
    }

    OsaDebugBegin(rc == CME_SUCC && pDynBearerCtxParamCnf != PNULL, rc, pDynBearerCtxParamCnf, 0);
    psDialCtx.bActIfDuringWakeup = FALSE;
    return;
    OsaDebugEnd();

    if (pDynBearerCtxParamCnf->bearerCtxPresent)
    {
        memset(&ifCfg, 0x00, sizeof(struct NmIfConfiguration_Tag));

        /*
         * check whether non-ip type
        */
        if (pDynBearerCtxParamCnf->ctxDynPara.ipv4Addr.addrType == CMI_IPV4_ADDR)
        {
            bIpBearer = TRUE;
            ifCfg.ipv4Addr.addrType = NM_ADDR_IPV4_ADDR;
            memcpy(ifCfg.ipv4Addr.addr, pDynBearerCtxParamCnf->ctxDynPara.ipv4Addr.addrData, 4);
        }
        if (pDynBearerCtxParamCnf->ctxDynPara.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)
        {
            bIpBearer = TRUE;
            ifCfg.ipv6Addr.addrType = NM_ADDR_FULL_IPV6_ADDR;
            memcpy(ifCfg.ipv6Addr.addr, pDynBearerCtxParamCnf->ctxDynPara.ipv6Addr.addrData, 16);
        }
        else if (pDynBearerCtxParamCnf->ctxDynPara.ipv6Addr.addrType == CMI_IPV6_INTERFACE_ADDR)
        {
            bIpBearer = TRUE;
            ifCfg.ipv6Addr.addrType = NM_ADDR_IPV6_ID;
            memcpy(ifCfg.ipv6Addr.addr, pDynBearerCtxParamCnf->ctxDynPara.ipv6Addr.addrData, 8);
        }

        /* when a default bearer activated, netif need this CGEV reason to report the URC */
        if (pDynBearerCtxParamCnf->ctxDynPara.cid == psDialCtx.actingCid)
        {
            ifCfg.cgevReason = psDialCtx.actingCidCgevReason;

            /* reset the value */
            psDialCtx.actingCid = CMI_PS_INVALID_CID;
            psDialCtx.actingCidCgevReason = CMI_PS_PDN_TYPE_REASON_NULL;
        }
        else
        {
            OsaDebugBegin(psDialCtx.actingCid == CMI_PS_INVALID_CID, psDialCtx.actingCid, psDialCtx.actingCidCgevReason, 0);
            OsaDebugEnd();
            ifCfg.cgevReason = CMI_PS_PDN_TYPE_REASON_NULL;
        }

        if (bIpBearer)
        {
            // set the DNS
            for (tmpIdx = 0; tmpIdx < pDynBearerCtxParamCnf->ctxDynPara.dnsAddrNum && tmpIdx < CMI_PDN_MAX_NW_ADDR_NUM && tmpIdx < NM_MAX_DNS_NUM; tmpIdx++)
            {
                /* "Nm_Addr_Type" just the same as "CmiIpAddrType" */
                ifCfg.dns[tmpIdx].addrType = pDynBearerCtxParamCnf->ctxDynPara.dnsAddr[tmpIdx].addrType;
                memcpy(ifCfg.dns[tmpIdx].addr, pDynBearerCtxParamCnf->ctxDynPara.dnsAddr[tmpIdx].addrData, NM_ADDR_MAX_LENGTH);
            }

            ifCfg.dnsNum = tmpIdx;

            if (pDynBearerCtxParamCnf->ctxDynPara.ipv4MtuPresent)
            {
                ifCfg.mtuPresent = TRUE;
                ifCfg.mtu = pDynBearerCtxParamCnf->ctxDynPara.ipv4Mtu;
            }

            ret = NetMgrLinkUp(pDynBearerCtxParamCnf->ctxDynPara.cid,
                               &ifCfg,
                               NM_CID_BIND_TO_NONE,
                               psDialCtx.bActIfDuringWakeup);   /*if not bind any context, the value will be 255*/

            if (ret == NM_SUCCESS)  //NM_SUCCESS
            {
                psDialCtx.bNetifActed = TRUE;
            }
            else
            {
                OsaDebugBegin(FALSE, ret, 0, 0);
                OsaDebugEnd();
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_PS_DAIL, psDialProcCmiPsReadDynBearerCtxParamCnf_1, P_WARNING, 1,
                        "CID : %d maybe a non-ip type bearer", pDynBearerCtxParamCnf->ctxDynPara.cid);
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_PS_DAIL, psDialProcCmiPsReadDynBearerCtxParamCnf_2, P_WARNING, 0,
                    "BEARER don't has valid context");
    }

    /* WAKE UP NETIF ACT done */
    if (pDynBearerCtxParamCnf->bContBearer == FALSE &&
        psDialCtx.bActIfDuringWakeup == TRUE)
    {
        psDialCtx.bActIfDuringWakeup = FALSE;
    }

    return;
}

/******************************************************************************
 * psDialProcCmiPsGetCeregCnf
 * Description: AT DIAL PROC "CmiPsGetCeregCnf"
 * input: uint16 rc;
 *        CmiPsGetCeregCnf *pGetCeregCnf;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsGetCeregCnf(UINT16 rc, CmiPsGetCeregCnf *pGetCeregCnf)
{
    OsaDebugBegin(rc == CME_SUCC && pGetCeregCnf != PNULL, rc, pGetCeregCnf, 0);
    return;
    OsaDebugEnd();

    NetMgrProcCeregCnf(pGetCeregCnf);

    return;
}

/******************************************************************************
 * psDialProcCmiSimUiccStateInd
 * Description: AT DIAL PROC "CmiSimUiccStateInd"
 * input: CmiSimUiccStateInd *pCmiUiccStateInd
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiSimUiccStateInd(CmiSimUiccStateInd *pCmiUiccStateInd)
{
    OsaDebugBegin(pCmiUiccStateInd != PNULL, pCmiUiccStateInd, 0, 0);
    return;
    OsaDebugEnd();

    /*copy the IMSI*/
    if (pCmiUiccStateInd->imsiStr.length > 0)
    {
        memcpy(&(psDialCtx.ueImsi), &(pCmiUiccStateInd->imsiStr), sizeof(CmiSimImsiStr));
    }

    #if 0
    if (pCmiUiccStateInd->uiccState == CMI_SIM_UICC_PD_DONE_CONT_INIT ||
        pCmiUiccStateInd->uiccState == CMI_SIM_UICC_AWAIT_PIN_VER)
    {
        /*
         * SIM init OK, (maybe need to verify PIN)
         * But IMSI not read, need to send cfun1 to power on the protocol
         * AT+CFUN = 1
         */
        devSetFunc(PS_DIAL_REQ_HANDLER, CMI_DEV_FULL_FUNC, CMI_DEV_DO_NOT_RESET);
    }
    else if (pCmiUiccStateInd->uiccState == CMI_SIM_UICC_ACTIVE)
    {
        //UICC activated, IMSI is read, need to find the APN table to descide which default APN need to set
        psDialStartUpPlmnConfig(&(pCmiUiccStateInd->imsiStr));

        // SIM init power on procedure done
        psDialCtx.bUnderPowerOn = FALSE;

    }
    else if (psDialCtx.cfunState != CMI_DEV_MIN_FUNC)
    {
        // UICC another abnormal state during power on procedure
        ECOMM_TRACE(UNILOG_PS_DAIL, psDialProcCmiSimUiccStateInd_2, P_WARNING, 1, "During power on; abnormal SIM state: %d", pCmiUiccStateInd->uiccState);
    }
    #endif

    return;
}


/******************************************************************************
 * psDialProcCmiDevSetCfunCnf
 * Description: AT DIAL PROC "CmiDevSetCfunCnf"
 * input: uint16 rc;
 *        CmiDevSetCfunCnf *pSetCfunCnf;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevSetCfunCnf(UINT16 rc, CmiDevSetCfunCnf *pSetCfunCnf)
{
    OsaDebugBegin(rc == CME_SUCC, rc, 0, 0);
    OsaDebugEnd();

    psDialCtx.cfunState = pSetCfunCnf->func;

    /* PS silent reset procedure CFUN0 done,perform CFUN1 */
    if (psDialCtx.bSilentReset && (CMI_DEV_MIN_FUNC == psDialCtx.cfunState))
    {
        devSetFunc(PS_DIAL_REQ_HANDLER, CMI_DEV_FULL_FUNC, CMI_DEV_DO_NOT_RESET);
        psDialCtx.bSilentReset = FALSE;
    }

    return;
}

/******************************************************************************
 * psDialProcCmiDevSetPowerStateCnf
 * Description: AT DIAL PROC "CmiDevSetPowerStateCnf"
 * input: uint16 rc;
 *        CmiDevSetPowerStateCnf *pSetPowerCnf;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevSetPowerStateCnf(UINT16 rc, CmiDevSetPowerStateCnf *pSetPowerCnf)
{
    /*
     * now, only power wake up
    */
    OsaDebugBegin(rc == CME_SUCC && pSetPowerCnf->powerState == CMI_DEV_POWER_WAKE_UP,
                  rc, pSetPowerCnf->powerState, CMI_DEV_POWER_WAKE_UP);
    return;
    OsaDebugEnd();


    psDialCtx.cfunState = pSetPowerCnf->func;

    memcpy(&(psDialCtx.ueImsi),
           &(pSetPowerCnf->imsiStr),
           sizeof(CmiSimImsiStr));

    /*
     * get CEREG state, as netmgr need it to update net state
    */
    psGetERegStatus(PS_DIAL_REQ_HANDLER);

    /*
     * need to read all activated default/dedicated bearer info, and create the netif
     * As for NB, only default bearer supported, AT+CGCONTRDP? to read all PDP info;
    */
    psDialCtx.bActIfDuringWakeup = TRUE;

    psGetAllCGCONTRDPParam(PS_DIAL_REQ_HANDLER);

    return;
}


/******************************************************************************
 * psDialProcCmiSimInd
 * Description: AT DIAL PROC the CMI IND from CAC SIM module
 * input: UINT16 primId;
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiSimInd(UINT16 primId, void *paras)
{
    OsaDebugBegin(paras != PNULL, primId, paras, 0);
    return;
    OsaDebugEnd();

    switch (primId)
    {
        case CMI_SIM_UICC_STATE_IND:
            psDialProcCmiSimUiccStateInd((CmiSimUiccStateInd *)paras);
            break;

        case CMI_SIM_UICC_PIN_STATE_IND:
            //TBD, indicated the sim pin state
            break;

        default:
            break;
    }
    return;
}


/******************************************************************************
 * psDialProcCmiPsInd
 * Description: AT DIAL PROC the CMI IND from CAC PS module
 * input: UINT16 primId;
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsInd(UINT16 primId, void *paras)
{
    switch (primId)
    {
        case CMI_PS_BEARER_ACTED_IND:
            psDialProcCmiPsBearerActedInd((CmiPsBearerActedInd *)paras);
            break;

        case CMI_PS_BEARER_DEACT_IND:
            psDialProcCmiPsBearerDeActInd((CmiPsBearerDeActInd *)paras);
            break;
        case CMI_PS_CEREG_IND:
            psDialProcCmiPsCEREGInd((CmiPsCeregInd *)paras);
            break;
        case CMI_PS_CONN_STATUS_IND:
            psDialProcCmiPsConnStatusInd((CmiPsConnStatusInd *)paras);
            break;
    }

    return;
}


/******************************************************************************
 * psDialProcCmiMmInd
 * Description: AT DIAL PROC the CMI IND from CAC MM module
 * input: UINT16 primId;
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiMmInd(UINT16 primId, void *paras)
{
#if 0
    //OsaPrintf(P_VALUE, "psDialProcCmiMmInd primID=%d", primId);
    switch (primId)
    {
        case CMI_MM_EXTENDED_SIGNAL_QUALITY_IND:
            psDialProcCmiMmEsigQInd((CmiMmCesqInd *)paras);
            break;
        case CMI_MM_CREG_IND:
            psDialProcCmiMmCregInd((CmiMmCregInd *)paras);
            break;
    }
#endif
    return;
}

/******************************************************************************
 * psDialProcCmiMmInd
 * Description: AT DIAL PROC the CMI IND from CAC DEV module
 * input: UINT16 primId;
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevInd(UINT16 primId, void *paras)
{

    switch (primId)
    {
        case CMI_DEV_BAND_APN_AUTO_CONFIG_REQ_IND:
            psDialProcCmiDevBandApnAutoCfgReqInd((CmiDevBandApnAutoCfgReqInd *)paras);
            break;

        case CMI_DEV_POWER_ON_CFUN_IND:
            psDialProcCmiDevPowerOnCfunInd((CmiDevPowerOnCfunInd *)paras);
            break;

        case CMI_DEV_SILENT_RESET_IND:
            psDialProcCmiDevSilentResetInd();
            break;

        default:
            break;
    }

    return;
}


/******************************************************************************
 * psDialProcCmiPsCnf
 * Description: AT DIAL PROC the CMI CNF from CAC PS module
 * input: UINT16 primId;
 *        UINT16 rc;
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiPsCnf(UINT16 primId, UINT16 rc, void *paras)
{
    switch (primId)
    {
        case CMI_PS_READ_BEARER_DYN_CTX_CNF:
            psDialProcCmiPsReadDynBearerCtxParamCnf(rc, (CmiPsReadBearerDynCtxParamCnf *)paras);
            break;

        case CMI_PS_SET_ATTACHED_BEARER_CTX_CNF:
            //maybe need to check the confirm result, - TBD
            break;

        case CMI_PS_GET_CEREG_CNF:
            psDialProcCmiPsGetCeregCnf(rc, (CmiPsGetCeregCnf *)paras);
            break;

        default:
            break;
    }

    return;
}


/******************************************************************************
 * psDialProcCmiDevCnf
 * Description: AT DIAL PROC the CMI CNF from CAC DEV module
 * input: UINT16 primId;
 *        UINT16 rc;
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
static void psDialProcCmiDevCnf(UINT16 primId, UINT16 rc, void *paras)
{
    switch (primId)
    {
        case CMI_DEV_SET_CFUN_CNF:
            psDialProcCmiDevSetCfunCnf(rc, (CmiDevSetCfunCnf *)paras);
            break;

        case CMI_DEV_SET_POWER_STATE_CNF:
            psDialProcCmiDevSetPowerStateCnf(rc, (CmiDevSetPowerStateCnf *)paras);
            break;

        case CMI_DEV_SET_CIOT_BAND_CNF:
            /* AUTO BAND*/
            break;

        default:
            break;
    }

    return;
}


/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS




/******************************************************************************
 * psDialProcCmiInd
 * Description: AT DIAL process the CMI INDICATION
 * input: UINT8 sgId;  //service group ID
 *        UINT16 primId;   //PRIM ID
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
// Just avoid warning, will do SIMBIP later

CmsRetId psDialProcCmiInd(const SignalBuf *indSignalPtr)
{
    CmsRetId ret = CMS_RET_SUCC;
    CacCmiInd   *pCmiInd = PNULL;
    UINT16    primId = 0, sgId = 0;

    pCmiInd = (CacCmiInd *)(indSignalPtr->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiInd->header.indId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiInd->header.indId);
    switch(sgId)
    {
        case CAC_DEV:
            psDialProcCmiDevInd(primId, pCmiInd->body);
            break;

        case CAC_MM:
            psDialProcCmiMmInd(primId, pCmiInd->body);
            break;

        case CAC_PS:
            psDialProcCmiPsInd(primId, pCmiInd->body);
            break;

        case CAC_SIM:
            psDialProcCmiSimInd(primId, pCmiInd->body);
            break;

        case CAC_SMS:
            break;

        default:
            break;
    }

    return ret;
}


/******************************************************************************
 * psDialProcCmiCnf
 * Description: AT DIAL process the CMI CNF
 * input: const SignalBuf *cnfSignalPtr;    //signal
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psDialProcCmiCnf(const SignalBuf *cnfSignalPtr)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT16    sgId = 0;
    UINT16    primId = 0;
    CacCmiCnf   *pCmiCnf = PNULL;

    pCmiCnf = (CacCmiCnf *)(cnfSignalPtr->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    //OsaPrintf(P_VALUE, "psDialProcCmiCnf sgId=%d primId=%d", sgId, primId);
    switch(sgId)
    {
        case CAC_DEV:
            psDialProcCmiDevCnf(primId, pCmiCnf->header.rc, pCmiCnf->body);
            break;

        case CAC_MM:
            break;

        case CAC_PS:
            psDialProcCmiPsCnf(primId, pCmiCnf->header.rc, pCmiCnf->body);
            break;

        case CAC_SIM:
            break;

        case CAC_SMS:
            break;

        default:
            break;
    }

    return ret;
}

/******************************************************************************
 * psDialLwipNetIfIsAct
 * Description: Whether NETIF is activated
 * input: void
 * output: BOOL
 * Comment:
 * Called by APP, to get the NETIF state, whetehr NETIF/PDP is activated
******************************************************************************/
BOOL psDialLwipNetIfIsAct(void)
{
    return psDialCtx.bNetifActed;
}

/******************************************************************************
 * psDailBeCfun1State
 * Description: whether PS is under cfun1 (power on) state
 * input: void
 * output: BOOL
 * Comment:
 * Called by APP, if PS is not in cfun1 state, APP need to stop (ping/iperf, etc)
******************************************************************************/
BOOL psDailBeCfun1State(void)
{
    return (psDialCtx.cfunState == CMI_DEV_FULL_FUNC);
}


/******************************************************************************
 * psDailGetCfgDnsByUeImsi
 * Description: get configured DNS by UE IMSI
 * input:   PsDialDnsCfg *pDnsCfg
 * output:  void
 * Comment:
 * Called by Netmgr
******************************************************************************/
void psDailGetCfgDnsByUeImsi(PsDialDnsCfg *pDnsCfg)
{
    const DialPlmnCfgInfo *pPlmnCfg = PNULL;

    OsaCheck(pDnsCfg != PNULL, pDnsCfg, 0, 0);

    memset(pDnsCfg, 0x00, sizeof(PsDialDnsCfg));

    /*
     * get PLMN CFG
    */
    pPlmnCfg = psDialGetPlmnCfgByImsi(&(psDialCtx.ueImsi));

    /*
     * pPlmnCfg->pDefIp4Dns1/pDefIp4Dns2/pDefIp6Dns1/pDefIp6Dns2
    */
    if(pPlmnCfg == PNULL)
    {
        ECOMM_TRACE(UNILOG_PS_DAIL, psDailGetCfgDnsByUeImsi_5, P_WARNING, 0, "psDailGetCfgDnsByUeImsi get plmn dns fail");
        return;
    }

    if(pPlmnCfg->pDefIp4Dns1)
    {
        if(ip4addr_aton((const char*)pPlmnCfg->pDefIp4Dns1, &pDnsCfg->ipv4Dns[0]) == 0)
        {
            ECOMM_TRACE(UNILOG_PS_DAIL, psDailGetCfgDnsByUeImsi_1, P_WARNING, 0, "psDailGetCfgDnsByUeImsi parse ipv4Dns0 fail");
        }
        else
        {
            if(!ip4_addr_isany(&pDnsCfg->ipv4Dns[0]))
            {
                pDnsCfg->ipv4DnsNum ++;
            }
        }
    }

    if(pPlmnCfg->pDefIp4Dns2)
    {
        if(ip4addr_aton((const char*)pPlmnCfg->pDefIp4Dns2, &pDnsCfg->ipv4Dns[1]) == 0)
        {
            ECOMM_TRACE(UNILOG_PS_DAIL, psDailGetCfgDnsByUeImsi_2, P_WARNING, 0, "psDailGetCfgDnsByUeImsi parse ipv4Dns1 fail");
        }
        else
        {
            if(!ip4_addr_isany(&pDnsCfg->ipv4Dns[1]))
            {
                pDnsCfg->ipv4DnsNum ++;
            }
        }
    }

    if(pPlmnCfg->pDefIp6Dns1)
    {
        if(ip6addr_aton((const char*)pPlmnCfg->pDefIp6Dns1, &pDnsCfg->ipv6Dns[0]) == 0)
        {
            ECOMM_TRACE(UNILOG_PS_DAIL, psDailGetCfgDnsByUeImsi_3, P_WARNING, 0, "psDailGetCfgDnsByUeImsi parse ipv6Dns0 fail");
        }
        else
        {
            if(!ip6_addr_isany(&pDnsCfg->ipv6Dns[0]))
            {
                pDnsCfg->ipv6DnsNum ++;
            }
        }
    }

    if(pPlmnCfg->pDefIp6Dns2)
    {
        if(ip6addr_aton((const char*)pPlmnCfg->pDefIp6Dns2, &pDnsCfg->ipv6Dns[1]) == 0)
        {
            ECOMM_TRACE(UNILOG_PS_DAIL, psDailGetCfgDnsByUeImsi_4, P_WARNING, 0, "psDailGetCfgDnsByUeImsi parse ipv6Dns1 fail");
        }
        else
        {
            if(!ip6_addr_isany(&pDnsCfg->ipv6Dns[1]))
            {
                pDnsCfg->ipv6DnsNum ++;
            }
        }
    }

    return;
}

/******************************************************************************
 * psDailGetPlmnPreferBandList
 * Description: Get prefer BAND list by PLMN
 * input:   pOutBand        //output
 *          pInOutBandNum   //in and output
 *          mcc
 *          mnc
 * output:  void
 * Comment:
 * Called by CE PLMN
******************************************************************************/
void psDailGetPlmnPreferBandList(UINT8 *pOutBand, UINT8 *pInOutBandNum, UINT16 mcc, UINT16 mnc)
{
    const DialPlmnCfgInfo *pPlmnCfg = PNULL;
    UINT8   maxBandNum = *pInOutBandNum;
    UINT8   bandIdx = 0;


    OsaCheck(pOutBand != PNULL && pInOutBandNum != PNULL && *pInOutBandNum > 0,
             pOutBand, pInOutBandNum, *pInOutBandNum);

    memset(pOutBand, 0x00, maxBandNum);
    *pInOutBandNum = 0;

    pPlmnCfg = psDialGetPlmnCfgByPlmn(mcc, mnc);

    if (pPlmnCfg == PNULL)
    {
        ECOMM_TRACE(UNILOG_PS_DAIL, psDailGetPlmnPreferBandList_1, P_WARNING, 2,
                    "Can't find DialPlmnCfg for PLMN: 0x%x, 0x%x, can't get prefer band", mcc, mnc);

        return;
    }

    for (bandIdx = 0; bandIdx < DIAL_CFG_PRFER_BAND_NUM && bandIdx < maxBandNum; bandIdx++)
    {
        if (pPlmnCfg->preBand[bandIdx] != 0)
        {
            pOutBand[bandIdx] = pPlmnCfg->preBand[bandIdx];
        }
        else
        {
            break;
        }
    }

    *pInOutBandNum = bandIdx;

    return;
}


/******************************************************************************
 * psDailBeActNetifDoneDuringWakeup
 * Description: Whether netif activated after wake up from HIB/Sleep2
 * input:   void
 * output:  BOOL
 * Comment:
******************************************************************************/
BOOL psDailBeActNetifDoneDuringWakeup(void)
{
    return (psDialCtx.bActIfDuringWakeup == FALSE);
}


/******************************************************************************
 * psDailSetAdptDnsServerInfo
 * Description: Set DNS server address
 * input:   UINT8 cid
 *          UINT8 type  //PDP IP type, PsDialNetType
 *          PsDialDnsCfg *pPcoDns,  //DNS address in NW configed in PCO
 *          PsDialDnsCfg *pAdptDns  //output
 *          BOOL bWakeUp  //indicate is or not recover from hib/sleep2 mode
 * output:  BOOL
 * Comment:
******************************************************************************/
BOOL psDailSetAdptDnsServerInfo(UINT8 cid, UINT8 type, PsDialDnsCfg *pPcoDns, PsDialDnsCfg *pAdptDns, BOOL bWakeUp)
{
    UINT8 dnsIdx;

    if (pPcoDns == PNULL || pAdptDns == PNULL)
    {
        if(pAdptDns != PNULL)
        {
            pAdptDns->ipv4DnsNum = 0;
            pAdptDns->ipv6DnsNum = 0;
        }
        return FALSE;
    }

    pAdptDns->ipv4DnsNum = 0;
    pAdptDns->ipv6DnsNum = 0;

    /*
     * PRIORITY:
     * 1> DNS, default AON DNS CFG by QIDNSCFG // not store when reboot
     * 2> DNS, NW configed in PCO
     * 3> DNS, user configed by AT+ECDNSCFG  // store when reboot
     * 4> DNS, static configed in psdail_plmn_cfg.c according to IMSI
    */

    //if there is not any the ipv4 and ipv6 dns conf, use default AON setting
    if ((pAdptDns->ipv4DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6)) ||
        (pAdptDns->ipv6DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6)))
    {
        MidWareDefaultAonDnsCfg dnsDefaultAonCfg;

        memset(&dnsDefaultAonCfg, 0x00, sizeof(MidWareDefaultDnsCfg));
        mwAonGetDefaultAonDnsCfg(&dnsDefaultAonCfg);      //DNS in NVM. configed by AT+ECDNSCFG

        if (pAdptDns->ipv4DnsNum == 0 &&
            (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            for (dnsIdx = 0; dnsIdx < MID_WARE_DEFAULT_DNS_NUM; dnsIdx++)
            {
                if(*((UINT32 *)&(dnsDefaultAonCfg.ipv4Dns[dnsIdx][0])) != 0x00000000)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_aon_1, P_INFO, 4,
                                "psdial, NW not config DNS, use AON default IPv4 DNS ADDR: %u.%u.%u.%u",
                                dnsDefaultAonCfg.ipv4Dns[dnsIdx][0], dnsDefaultAonCfg.ipv4Dns[dnsIdx][1],
                                dnsDefaultAonCfg.ipv4Dns[dnsIdx][2], dnsDefaultAonCfg.ipv4Dns[dnsIdx][3]);

                    memcpy((CHAR *)&(pAdptDns->ipv4Dns[pAdptDns->ipv4DnsNum]),
                           (CHAR *)&(dnsDefaultAonCfg.ipv4Dns[dnsIdx][0]),
                           MID_WARE_IPV4_ADDR_LEN);
                    pAdptDns->ipv4DnsNum++;
                }
            }
        }

        if (pAdptDns->ipv6DnsNum == 0 &&
            (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            for (dnsIdx = 0; dnsIdx < MID_WARE_DEFAULT_DNS_NUM; dnsIdx++)
            {
                if ((*((UINT32 *)&(dnsDefaultAonCfg.ipv6Dns[dnsIdx][0])) != 0x00000000) ||
                    (*((UINT32 *)&(dnsDefaultAonCfg.ipv6Dns[dnsIdx][4])) != 0x00000000) ||
                    (*((UINT32 *)&(dnsDefaultAonCfg.ipv6Dns[dnsIdx][8])) != 0x00000000) ||
                    (*((UINT32 *)&(dnsDefaultAonCfg.ipv6Dns[dnsIdx][12])) != 0x00000000))
                {
                    ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_aon_2, P_WARNING, 16,
                                "psdial, NW not config DNS, use AON default IPv6 DNS ADDR: %u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u",
                                dnsDefaultAonCfg.ipv6Dns[dnsIdx][0], dnsDefaultAonCfg.ipv6Dns[dnsIdx][1], dnsDefaultAonCfg.ipv6Dns[dnsIdx][2], dnsDefaultAonCfg.ipv6Dns[dnsIdx][3],
                                dnsDefaultAonCfg.ipv6Dns[dnsIdx][4], dnsDefaultAonCfg.ipv6Dns[dnsIdx][5], dnsDefaultAonCfg.ipv6Dns[dnsIdx][6], dnsDefaultAonCfg.ipv6Dns[dnsIdx][7],
                                dnsDefaultAonCfg.ipv6Dns[dnsIdx][8], dnsDefaultAonCfg.ipv6Dns[dnsIdx][9], dnsDefaultAonCfg.ipv6Dns[dnsIdx][10], dnsDefaultAonCfg.ipv6Dns[dnsIdx][11],
                                dnsDefaultAonCfg.ipv6Dns[dnsIdx][12], dnsDefaultAonCfg.ipv6Dns[dnsIdx][13], dnsDefaultAonCfg.ipv6Dns[dnsIdx][14], dnsDefaultAonCfg.ipv6Dns[dnsIdx][15]);

                    memcpy((CHAR *)&(pAdptDns->ipv6Dns[pAdptDns->ipv6DnsNum]),
                           (CHAR *)&(dnsDefaultAonCfg.ipv6Dns[dnsIdx][0]),
                           MID_WARE_IPV6_ADDR_LEN);
                    pAdptDns->ipv6DnsNum++;
                }
            }
        }
    }

    //check pco DNS
    if ((pAdptDns->ipv4DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6)) ||
        (pAdptDns->ipv6DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6)))
    {

        if (pAdptDns->ipv4DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV4 || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            for (dnsIdx = 0; dnsIdx < PS_DAIL_DEFAULT_DNS_NUM && dnsIdx < pPcoDns->ipv4DnsNum ; dnsIdx++)
            {
                if (!ip4_addr_isany_val(pPcoDns->ipv4Dns[dnsIdx]))
                {
                   ip4_addr_copy(pAdptDns->ipv4Dns[pAdptDns->ipv4DnsNum], pPcoDns->ipv4Dns[dnsIdx]);
                    pAdptDns->ipv4DnsNum++;
                }
            }
        }

        if (pAdptDns->ipv6DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            for (dnsIdx = 0; dnsIdx < PS_DAIL_DEFAULT_DNS_NUM && dnsIdx < pPcoDns->ipv6DnsNum ; dnsIdx++)
            {
               ip6_addr_copy(pAdptDns->ipv6Dns[pAdptDns->ipv6DnsNum], pPcoDns->ipv6Dns[dnsIdx]);
                pAdptDns->ipv6DnsNum ++;
            }
        }
    }

    //if there is not any the ipv4 and ipv6 dns conf, use default setting
    //mayebe enahnce default setting with cid info
    if ((pAdptDns->ipv4DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6)) ||
        (pAdptDns->ipv6DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6)))
    {
        MidWareDefaultDnsCfg dnsDefaultCfg;

        memset(&dnsDefaultCfg, 0x00, sizeof(MidWareDefaultDnsCfg));
        mwGetDefaultDnsConfig(&dnsDefaultCfg);      //DNS in NVM. configed by AT+ECDNSCFG

        if (pAdptDns->ipv4DnsNum == 0 &&
            (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            for (dnsIdx = 0; dnsIdx < MID_WARE_DEFAULT_DNS_NUM; dnsIdx++)
            {
                if(*((UINT32 *)&(dnsDefaultCfg.ipv4Dns[dnsIdx][0])) != 0x00000000)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_1, P_INFO, 4,
                                "psdial, NW not config DNS, use default IPv4 DNS ADDR: %u.%u.%u.%u",
                                dnsDefaultCfg.ipv4Dns[dnsIdx][0], dnsDefaultCfg.ipv4Dns[dnsIdx][1],
                                dnsDefaultCfg.ipv4Dns[dnsIdx][2], dnsDefaultCfg.ipv4Dns[dnsIdx][3]);

                    memcpy((CHAR *)&(pAdptDns->ipv4Dns[pAdptDns->ipv4DnsNum]),
                           (CHAR *)&(dnsDefaultCfg.ipv4Dns[dnsIdx][0]),
                           MID_WARE_IPV4_ADDR_LEN);
                    pAdptDns->ipv4DnsNum++;
                }
            }
        }

        if (pAdptDns->ipv6DnsNum == 0 &&
            (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            for (dnsIdx = 0; dnsIdx < MID_WARE_DEFAULT_DNS_NUM; dnsIdx++)
            {
                if ((*((UINT32 *)&(dnsDefaultCfg.ipv6Dns[dnsIdx][0])) != 0x00000000) ||
                    (*((UINT32 *)&(dnsDefaultCfg.ipv6Dns[dnsIdx][4])) != 0x00000000) ||
                    (*((UINT32 *)&(dnsDefaultCfg.ipv6Dns[dnsIdx][8])) != 0x00000000) ||
                    (*((UINT32 *)&(dnsDefaultCfg.ipv6Dns[dnsIdx][12])) != 0x00000000))
                {
                    ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_2, P_WARNING, 16,
                                "psdial, NW not config DNS, use default IPv6 DNS ADDR: %u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u",
                                dnsDefaultCfg.ipv6Dns[dnsIdx][0], dnsDefaultCfg.ipv6Dns[dnsIdx][1], dnsDefaultCfg.ipv6Dns[dnsIdx][2], dnsDefaultCfg.ipv6Dns[dnsIdx][3],
                                dnsDefaultCfg.ipv6Dns[dnsIdx][4], dnsDefaultCfg.ipv6Dns[dnsIdx][5], dnsDefaultCfg.ipv6Dns[dnsIdx][6], dnsDefaultCfg.ipv6Dns[dnsIdx][7],
                                dnsDefaultCfg.ipv6Dns[dnsIdx][8], dnsDefaultCfg.ipv6Dns[dnsIdx][9], dnsDefaultCfg.ipv6Dns[dnsIdx][10], dnsDefaultCfg.ipv6Dns[dnsIdx][11],
                                dnsDefaultCfg.ipv6Dns[dnsIdx][12], dnsDefaultCfg.ipv6Dns[dnsIdx][13], dnsDefaultCfg.ipv6Dns[dnsIdx][14], dnsDefaultCfg.ipv6Dns[dnsIdx][15]);

                    memcpy((CHAR *)&(pAdptDns->ipv6Dns[pAdptDns->ipv6DnsNum]),
                           (CHAR *)&(dnsDefaultCfg.ipv6Dns[dnsIdx][0]),
                           MID_WARE_IPV6_ADDR_LEN);
                    pAdptDns->ipv6DnsNum++;
                }
            }
        }
    }

    //if there is not any the ipv4 and ipv6 dns conf, use IMSI DNS
    //mayebe enahnce IMSI setting with cid info
    if ((pAdptDns->ipv4DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6)) ||
        (pAdptDns->ipv6DnsNum == 0 && (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6)))
    {
        PsDialDnsCfg dnsImsiCfg;

        memset(&dnsImsiCfg, 0x00, sizeof(PsDialDnsCfg));

        psDailGetCfgDnsByUeImsi(&dnsImsiCfg);

        if (pAdptDns->ipv4DnsNum == 0 &&
            (type == PS_DIAL_NET_TYPE_IPV4  || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            if (dnsImsiCfg.ipv4DnsNum > 0)
            {
                ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_3, P_INFO, 0,
                            "psdial load ipv4 IMSI DNS");
                for (dnsIdx = 0; dnsIdx < dnsImsiCfg.ipv4DnsNum && dnsIdx < NM_PDN_TYPE_MAX_DNS_NUM; dnsIdx++)
                {
                    memcpy(&(pAdptDns->ipv4Dns[pAdptDns->ipv4DnsNum]), &dnsImsiCfg.ipv4Dns[dnsIdx], sizeof(ip4_addr_t));
                    pAdptDns->ipv4DnsNum ++;
                }
            }
        }

        if (pAdptDns->ipv6DnsNum == 0 &&
            (type == PS_DIAL_NET_TYPE_IPV6 || type == PS_DIAL_NET_TYPE_IPV4V6))
        {
            if (dnsImsiCfg.ipv6DnsNum > 0)
            {
                ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_4, P_INFO, 0,
                            "psdial load ipv6 IMSI DNS");
                for (dnsIdx = 0; dnsIdx < dnsImsiCfg.ipv6DnsNum && dnsIdx < NM_PDN_TYPE_MAX_DNS_NUM; dnsIdx++)
                {
                    memcpy(&(pAdptDns->ipv6Dns[pAdptDns->ipv6DnsNum]), &dnsImsiCfg.ipv6Dns[dnsIdx], sizeof(ip6_addr_t));
                    pAdptDns->ipv6DnsNum ++;
                }
            }
        }
    }

    if (pAdptDns->ipv4DnsNum == 0 && pAdptDns->ipv6DnsNum == 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_NETMGR, psDailSetAdptDnsServerInfo_5, P_WARNING, 1,
                    "PS DAIL, Get adpt DNS server address failed, bWakeUp %u", bWakeUp);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * psDialGetCfunState
 * Description: Get PS CFUN state
 * input: void
 * output: UINT8 CMI_DEV_MIN_FUNC/CMI_DEV_FULL_FUNC/CMI_DEV_TURN_OFF_RF_FUNC
 * Comment:
 * Called
******************************************************************************/
UINT8 psDialGetCfunState(void)
{
    return  psDialCtx.cfunState;
}

/******************************************************************************
 * psDialGetPsConnStatus
 * Description: Get PS CONN STATUS
 * input: void
 * output: UINT8 psConnStatus
 * Comment:
 * Called
******************************************************************************/
BOOL psDialGetPsConnStatus(void)
{
    return  psDialCtx.psConnStatus;
}



