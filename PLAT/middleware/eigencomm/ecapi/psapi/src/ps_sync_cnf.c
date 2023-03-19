/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ps_sync_cnf.c
 * Description:  EC616 opencpu pssynccnf source file
 * History:      Rev1.0   2018-12-10
 *
 ****************************************************************************/
#include "psdial.h"
#include "netmgr.h"
#include "cmicomm.h"
#include "cmisim.h"
#include "cmips.h"
#include "cmimm.h"
#include "cmidev.h"
#include "ps_sim_if.h"
#include "ps_ps_if.h"
#include "ps_mm_if.h"
#include "ps_dev_if.h"
#include "cms_comm.h"
#include "ps_lib_api.h"
#include "debug_log.h"
#include "ps_sync_cnf.h"
#include "cms_util.h"
#include "cms_api.h"


/******************************************************************************
 ******************************************************************************
 GOLBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

#define MAX_REQ_NUM     (10)

typedef struct {
    UINT16 requestId;
    void *cmiRequest;
} psCmiReqMapList_t;

static psCmiReqMapList_t psCmiReqMapList[MAX_REQ_NUM]= {{0, PNULL}};

/******************************************************************************
 *
 * PS blocked service request.
 *
******************************************************************************/
typedef struct AppPsBlockReqInfo_tag
{
    osTimerId_t         guardTimer;
    CmsBlockApiInfo     *pBlockInfo;
}AppPsBlockReqInfo;  //8 bytes

typedef struct AppPsBlockReqTable_tag
{
    /*
     * bit X value: 1, just means blockList[X] is used.
    */
    UINT16      freeBitMap;
    UINT16      rsvd;

    AppPsBlockReqInfo    blockList[APP_PS_BLOCK_REQ_NUM];
}AppPsBlockReqTable;

static AppPsBlockReqTable   appPsBlockReqTbl;



/******************************************************************************
 ******************************************************************************
 static functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_INTERNAL_FUNCTION__ //just for easy to find this position----Below for internal use


/**
  \fn      NBStatus_t addPsCmiReqtoList(UINT16 requestId, void *cmiRequest)
  \brief   Add PS cmi request to maplist, max support number is MAX_REQ_NUM
  \returns
*/
NBStatus_t addPsCmiReqtoList(UINT16 requestId, void *cmiRequest)
{
    INT8 i, available_id = -1;

    for (i = 0; i < MAX_REQ_NUM; i++)
    {
        if (psCmiReqMapList[i].requestId == requestId)
        {
            // one same req is ongoing
            return NBStatusBusy;
        } else if (psCmiReqMapList[i].requestId == 0)
        {
            available_id = i;
        }
    }

    if (available_id== -1)
    {
        return NBStatusListFull;
    } else {
        psCmiReqMapList[available_id].requestId = requestId;
        psCmiReqMapList[available_id].cmiRequest = cmiRequest;
        return NBStatusSuccess;
    }
}

/**
  \fn      NBStatus_t delPsCmiReqfromList(UINT16 requestId)
  \brief   Delete PS cmi request from maplist
  \returns
*/
NBStatus_t delPsCmiReqfromList(UINT16 requestId)
{
    INT8 i, available_id = -1;

    for (i = 0; i < MAX_REQ_NUM; i++)
    {
        if (psCmiReqMapList[i].requestId == requestId)
        {
            psCmiReqMapList[i].requestId = 0;
            psCmiReqMapList[i].cmiRequest = PNULL;
            available_id = i;
        }
    }
    if (available_id == -1)
        return NBStatusListNotFound;
    else
        return NBStatusSuccess;
}

/**
  \fn      void *getPsCmiReqAddr(UINT16 requestId)
  \brief   Get PS cmi request address according to request id from maplist
  \returns
*/
void *getPsCmiReqAddr(UINT16 requestId)
{
    INT8 i;
    for (i = 0; i < MAX_REQ_NUM; i++)
    {
        if (psCmiReqMapList[i].requestId == requestId)
        {
            return (void *)psCmiReqMapList[i].cmiRequest;
        }
    }

    return PNULL;
}

/**
  \fn      NBStatus_t addPsCmiReqtoList(UINT16 requestId, void *cmiRequest)
  \brief   Add PS cmi request to maplist, max support number is MAX_REQ_NUM
  \returns
*/
NBStatus_t initPsCmiReqMapList()
{
    int i;
    for (i = 0; i < MAX_REQ_NUM; i++)
    {
        psCmiReqMapList[i].requestId = 0;
        psCmiReqMapList[i].cmiRequest = PNULL;
    }
    return NBStatusSuccess;
}

/**
  \fn      static void psSyncProcCmiSimCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
  \brief   Process PS cmi SIM branch's confirm response from APP_SYNC_REQ_HANDLER
  \returns
*/
static void psSyncProcCmiSimCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
{
    OsaDebugBegin(pCmiCnf != PNULL, primId, pCmiCnf, 0);
    return;
    OsaDebugEnd();

    switch (primId)
    {
        case CMI_SIM_GET_SUBSCRIBER_ID_CNF:
        {
            CmiSimGetSubscriberIdCnf *Conf = (CmiSimGetSubscriberIdCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct imsiApiMsg *msg = (struct imsiApiMsg *)cmiRequest;
                memcpy(msg->imsi, Conf->imsiStr.contents, Conf->imsiStr.length);
                msg->len = Conf->imsiStr.length;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_SIM_GET_ICCID_CNF:
        {
            CmiSimGetIccIdCnf *Conf = (CmiSimGetIccIdCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct iccidApiMsg *msg = (struct iccidApiMsg *)cmiRequest;
                memcpy(msg->iccid, Conf->iccidStr.data, CMI_SIM_ICCID_LEN);
                msg->len = CMI_SIM_ICCID_LEN;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_SIM_OPEN_LOGICAL_CHANNEL_CNF:
        {
            CmiSimOpenLogicalChannelCnf *Conf = (CmiSimOpenLogicalChannelCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct cchoApiMsg *msg = (struct cchoApiMsg *)cmiRequest;
                *(msg->sessionID) = Conf->sessionId;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_SIM_CLOSE_LOGICAL_CHANNEL_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct cchcApiMsg *msg = (struct cchcApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_SIM_GENERIC_LOGICAL_CHANNEL_ACCESS_CNF:
        {
            CmiSimGenLogicalChannelAccessCnf *Conf = (CmiSimGenLogicalChannelAccessCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct cglaApiMsg *msg = (struct cglaApiMsg *)cmiRequest;
                *(msg->respLen) = (Conf->cmdRspLen * 2);
                if (cmsHexToHexStr((CHAR *)msg->response, (*(msg->respLen) + 1), Conf->cmdRspData, Conf->cmdRspLen) <= 0)
                {
                    ECOMM_TRACE(UNILOG_ATCMD, psSyncProcCmiSimCnf_1, P_ERROR, 0, "psSyncProcCmiSimCnf atHexToString fail");
                }
                msg->result = pCmiCnf->header.rc;
                if (Conf->cmdRspData != PNULL)
                {
                    OsaFreeMemory(&Conf->cmdRspData);
                }
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_SIM_RESTRICTED_ACCESS_CNF:
        {
            CmiSimRestrictedAccessCnf *Conf = (CmiSimRestrictedAccessCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct crsmApiMsg *msg = (struct crsmApiMsg *)cmiRequest;
                msg->responseParam->sw1 = Conf->sw1;
                msg->responseParam->sw2 = Conf->sw2;
                msg->responseParam->respLen = (Conf->cmdRspLen * 2);
                if (cmsHexToHexStr((CHAR *)msg->responseParam->responseData, (msg->responseParam->respLen + 1), Conf->cmdRspData, Conf->cmdRspLen) <= 0)
                {
                    ECOMM_TRACE(UNILOG_ATCMD, psSyncProcCmiSimCnf_2, P_ERROR, 0, "psSyncProcCmiSimCnf atHexToString fail");
                }
                msg->result = pCmiCnf->header.rc;
                if (Conf->cmdRspData != PNULL)
                {
                    OsaFreeMemory(&Conf->cmdRspData);
                }
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }

        default:
            OsaDebugBegin(FALSE, primId, pCmiCnf, 0);
            OsaDebugEnd();
            break;
    }
    return;
}

/**
  \fn      static void psSyncProcCmiPSCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
  \brief   Process PS cmi PS branch's confirm response from APP_SYNC_REQ_HANDLER
  \returns
*/
static void psSyncProcCmiPSCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
{
    UINT8       i = 0;
    OsaDebugBegin(pCmiCnf != PNULL, primId, pCmiCnf, 0);
    return;
    OsaDebugEnd();

    switch (primId)
    {
        case CMI_PS_GET_ALL_BEARERS_CIDS_INFO_CNF:
        {
            CmiPsGetAllBearersCidsInfoCnf *pGetCidConf = (CmiPsGetAllBearersCidsInfoCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct cgcontrdpApiMsg *msg = (struct cgcontrdpApiMsg *)cmiRequest;
                *(msg->len) = pGetCidConf->validNum;
                msg->result = pCmiCnf->header.rc;

                for(i = 0; i < pGetCidConf->validNum; i++)
                {
                    if(pGetCidConf->basicInfoList[i].bearerState == CMI_PS_BEARER_ACTIVATED)    //invalid - 0/  defined - 1/  actived - 2/
                    {
                        msg->cid[i] = pGetCidConf->basicInfoList[i].cid;
                    }
                }
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_PS_GET_CEREG_CNF:
        {
            CmiPsGetCeregCnf *pGetCeregCnf = (CmiPsGetCeregCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct ceregApiMsg *msg = (struct ceregApiMsg *)cmiRequest;
                if (msg->state != NULL)
                    *(msg->state) = pGetCeregCnf->state;
                if (pGetCeregCnf->locPresent)
                {
                    if (msg->tac != NULL) *(msg->tac) = pGetCeregCnf->tac;
                    if (msg->cellId != NULL) *(msg->cellId) = pGetCeregCnf->celId;
                }
                // First check extTauTimePresent then tauTimerPresent
                if (pGetCeregCnf->extTauTimePresent)
                {
                    if (msg->tauTimeS != NULL) *(msg->tauTimeS) = pGetCeregCnf->extPeriodicTauS;
                }
                else if (pGetCeregCnf->tauTimerPresent)
                {
                    if (msg->tauTimeS != NULL) *(msg->tauTimeS) = pGetCeregCnf->periodicTauS;
                }
                if (pGetCeregCnf->activeTimePresent)
                {
                    if (msg->activeTimeS != NULL) *(msg->activeTimeS) = pGetCeregCnf->activeTimeS;
                }
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_PS_READ_BEARER_DYN_CTX_CNF:
        {
            CmiPsReadBearerDynCtxParamCnf *pGetDynCtxconf = (CmiPsReadBearerDynCtxParamCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct cgcontrdpApiMsg *msg = (struct cgcontrdpApiMsg *)cmiRequest;
                if (pGetDynCtxconf->bearerCtxPresent)
                {
                    if(pGetDynCtxconf->ctxDynPara.apnLength > 0)
                    {
                        memcpy(msg->APN, pGetDynCtxconf->ctxDynPara.apnStr,pGetDynCtxconf->ctxDynPara.apnLength);
                    }
                }
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        default:
            OsaDebugBegin(FALSE, primId, pCmiCnf, 0);
            OsaDebugEnd();
            break;
    }
    return;
}

/**
  \fn      static void psSyncProcCmiMMCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
  \brief   Process PS cmi mm branch's confirm response from APP_SYNC_REQ_HANDLER
  \returns
*/
static void psSyncProcCmiMMCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
{
    OsaDebugBegin(pCmiCnf != PNULL, primId, pCmiCnf, 0);
    return;
    OsaDebugEnd();

    switch (primId)
    {
        case CMI_MM_GET_REQUESTED_PSM_PARM_CNF:
        {
            CmiMmGetRequestedPsmParmCnf *pPsmConf = (CmiMmGetRequestedPsmParmCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct psmApiMsg *msg = (struct psmApiMsg *)cmiRequest;
                *(msg->psmMode) = pPsmConf->mode;
                *(msg->tauTimeS) = pPsmConf->reqPeriodicTauS;
                *(msg->activeTimeS) = pPsmConf->reqActiveTimeS;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_MM_SET_REQUESTED_PSM_PARM_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct psmApiMsg *msg = (struct psmApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_MM_READ_EDRX_DYN_PARM_CNF:
        {
            CmiMmReadEdrxDynParmCnf *pEdrxConf = (CmiMmReadEdrxDynParmCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct edrxApiMsg *msg = (struct edrxApiMsg *)cmiRequest;
                if (pEdrxConf->nwEdrxPresent)
                {
                    *(msg->actType) = pEdrxConf->actType;
                    *(msg->edrxValueMs) = pEdrxConf->nwEdrxValueMs;
                    *(msg->share.nwPtwMs) = pEdrxConf->nwPtwMs;
                } else {
                    *(msg->actType) = 0;
                    *(msg->edrxValueMs) = 0;
                    *(msg->share.nwPtwMs) = 0;
                }
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_MM_SET_REQUESTED_EDRX_PARM_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct edrxApiMsg *msg = (struct edrxApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_MM_GET_COVERAGE_ENHANCEMENT_STATUS_CNF:
        {
            CmiMmGetCEStatusCnf *pCoverageConf = (CmiMmGetCEStatusCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct celevelApiMsg *msg = (struct celevelApiMsg *)cmiRequest;
                msg->act = pCoverageConf->act;
                msg->cc = pCoverageConf->ccLevel;
                msg->celevel = pCoverageConf->ceLevel;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_MM_GET_EXTENDED_SIGNAL_QUALITY_CNF:
        {
            CmiMmGetCesqCnf *pCmiGetCesqCnf = (CmiMmGetCesqCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct signalApiMsg *msg = (struct signalApiMsg *)cmiRequest;

                msg->result = pCmiCnf->header.rc;
                if (msg->result == CME_SUCC)
                {
                    *(msg->snr) = pCmiGetCesqCnf->snr;
                    *(msg->rsrp) = pCmiGetCesqCnf->rsrp;
                    *(msg->csq) = mmGetCsqRssiFromCesq(pCmiGetCesqCnf->rsrp, pCmiGetCesqCnf->rsrq);
                }
                else
                {
                    *(msg->snr) = 0;
                    *(msg->rsrp) = 127; // not valid value
                    *(msg->csq) = 99;   //99       not known or not detectable
                }

                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }

        default:
            OsaDebugBegin(FALSE, primId, pCmiCnf, 0);
            OsaDebugEnd();
            break;
    }
    return;
}

/**
  \fn      static void psSyncProcCmiDevCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
  \brief   Process PS cmi dev branch's confirm response from APP_SYNC_REQ_HANDLER
  \returns
*/
static void psSyncProcCmiDevCnf(UINT16 primId, CacCmiCnf *pCmiCnf , void *cmiRequest)
{
    OsaDebugBegin(pCmiCnf != PNULL, primId, pCmiCnf, 0);
    return;
    OsaDebugEnd();

    switch (primId)
    {
        case CMI_DEV_SET_POWER_STATE_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct powerStateApiMsg *msg = (struct powerStateApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
            }
            break;
        }
        case CMI_DEV_GET_EXT_CFG_CNF:
        {
            CmiDevGetExtCfgCnf *pGetEcfgCnf = (CmiDevGetExtCfgCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct extCfgApiMsg *msg = (struct extCfgApiMsg *)cmiRequest;
                msg->powerlevel = pGetEcfgCnf->plmnSearchPowerLevel;
                msg->powerCfun = pGetEcfgCnf->powerCfun;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_DEV_SET_EXT_CFG_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct extCfgApiMsg *msg = (struct extCfgApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_DEV_SET_CFUN_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct cfunApiMsg *msg = (struct cfunApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_DEV_SET_CIOT_BAND_CNF:
        {
            if (cmiRequest != PNULL)
            {
                struct ecbandApiMsg *msg = (struct ecbandApiMsg *)cmiRequest;
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        case CMI_DEV_GET_CIOT_BAND_CNF:
        {
            CmiDevGetCiotBandCnf *pGetCiotBandCnf = (CmiDevGetCiotBandCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct ecbandApiMsg *msg = (struct ecbandApiMsg *)cmiRequest;
                msg->networkMode = pGetCiotBandCnf->nwMode;
                msg->bandNum = pGetCiotBandCnf->bandNum;
                memcpy(msg->orderBand,pGetCiotBandCnf->orderedBand,CMI_DEV_SUPPORT_MAX_BAND_NUM);
                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        #if 0
        case CMI_DEV_GET_BASIC_CELL_LIST_INFO_CNF:
        {
            CmiDevGetBasicCellListInfoCnf   *pGetBasicCellListInfoCnf = (CmiDevGetBasicCellListInfoCnf *)(pCmiCnf->body);
            if (cmiRequest != PNULL)
            {
                struct ecbcInfoApiMsg *msg = (struct ecbcInfoApiMsg *)cmiRequest;
                if (pGetBasicCellListInfoCnf->sCellPresent == TRUE)
                {
                    msg->bcInfo->sCellPresent = TRUE;
                    msg->bcInfo->sCellInfo.mcc = pGetBasicCellListInfoCnf->sCellInfo.plmn.mcc;
                    msg->bcInfo->sCellInfo.mncWithAddInfo = pGetBasicCellListInfoCnf->sCellInfo.plmn.mncWithAddInfo;
                    msg->bcInfo->sCellInfo.earfcn = pGetBasicCellListInfoCnf->sCellInfo.earfcn;
                    msg->bcInfo->sCellInfo.cellId = pGetBasicCellListInfoCnf->sCellInfo.cellId;
                    msg->bcInfo->sCellInfo.tac    = pGetBasicCellListInfoCnf->sCellInfo.tac;
                    msg->bcInfo->sCellInfo.phyCellId = pGetBasicCellListInfoCnf->sCellInfo.phyCellId;
                    msg->bcInfo->sCellInfo.snrPresent = pGetBasicCellListInfoCnf->sCellInfo.snrPresent;
                    msg->bcInfo->sCellInfo.snr = pGetBasicCellListInfoCnf->sCellInfo.snr;
                    msg->bcInfo->sCellInfo.rsrp = pGetBasicCellListInfoCnf->sCellInfo.rsrp;
                    msg->bcInfo->sCellInfo.rsrq = pGetBasicCellListInfoCnf->sCellInfo.rsrq;
                    if (pGetBasicCellListInfoCnf->nCellNum > NCELL_INFO_CELL_NUM)
                        pGetBasicCellListInfoCnf->nCellNum = NCELL_INFO_CELL_NUM;
                    msg->bcInfo->nCellNum = pGetBasicCellListInfoCnf->nCellNum;
                    memcpy(msg->bcInfo->nCellList, pGetBasicCellListInfoCnf->nCellList, sizeof(CmiDevNCellBasicInfo)*msg->bcInfo->nCellNum);
                }
                else
                {
                    msg->bcInfo->sCellPresent = FALSE;
                    msg->bcInfo->nCellNum = 0;
                    if (pCmiCnf->header.rc == CME_SUCC)
                    {
                        pCmiCnf->header.rc = CME_NO_NW_SERVICE;
                    }
                }

                msg->result = pCmiCnf->header.rc;
                if (msg->sem != PNULL)
                    osSemaphoreRelease(*(msg->sem));
            }
            break;
        }
        #endif
        default:
            OsaDebugBegin(FALSE, primId, pCmiCnf, 0);
            OsaDebugEnd();
            break;
    }
    return;
}


/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position---Below for external use

/**
  \fn      void psSyncProcCmiCnf(const SignalBuf *cnfSignalPtr)
  \brief   Process PS cmi confirm response from APP_SYNC_REQ_HANDLER
  \returns
*/
void psSyncProcCmiCnf(const SignalBuf *cnfSignalPtr)
{
    UINT16      sgId = 0;
    UINT16      primId = 0;
    CacCmiCnf   *pCmiCnf = PNULL;
    void        *cmiRequest = PNULL;

    pCmiCnf = (CacCmiCnf *)(cnfSignalPtr->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);
    cmiRequest = (void *)getPsCmiReqAddr(pCmiCnf->header.cnfId);

    switch(sgId)
    {
        case CAC_SIM:
            psSyncProcCmiSimCnf(primId, pCmiCnf, cmiRequest);
            break;
        case CAC_PS:
            psSyncProcCmiPSCnf(primId, pCmiCnf, cmiRequest);
            break;
        case CAC_MM:
            psSyncProcCmiMMCnf(primId, pCmiCnf, cmiRequest);
            break;
        case CAC_DEV:
            psSyncProcCmiDevCnf(primId, pCmiCnf, cmiRequest);
            break;
        default:
            break;
     }

     delPsCmiReqfromList(pCmiCnf->header.cnfId);
}

NBStatus_t psSyncProcErrCode(UINT16 result)
{
    NBStatus_t status = NBStatusSuccess;

    switch (result)
    {
        case CME_SUCC:
            status = NBStatusSuccess;
            break;

        case CME_OPERATION_NOT_SUPPORT:
            status = NBStatusOperNotSupported;
            break;

        case CME_SIM_WRONG:
            status = NBStatusSimWrong;
            break;

        case CME_INCORRECT_PARAM:
            status = NBStatusInvalidParam;
            break;

        case CME_SIM_NOT_INSERT:
            status = NBStatusSimNotInserted;
            break;

        case CME_NOT_POWER_ON:
            status = NBStatusSimNotPowerOn;
            break;

        case CME_SIM_BUSY:
            status = NBStatusSimBusy;
            break;

        default:
            if (result > CME_SUCC)
            {
                status = NBStatusFail;
            }
            else
            {
                status = (NBStatus_t)result;
            }
            break;
    }

    return status;
}


/**
  \fn       void  psBlockCmiReqCallback(void *pArg)
  \brief    Process the "CacCmiReq" request from APP layer. Just the callback
  \         called in func: "CmsRetId appPsCmiReq(AppPsCmiReqData *pReqData, UINT32 timeOutMs)"
  \         This API called in CMS task
  \returns
*/
void psBlockCmiReqCallback(void *pArg)
{
    UINT8   subHdrId = 0;
    UINT16  reqHandler = 0;

    CmsBlockApiInfo     *pApiInfo = (CmsBlockApiInfo *)pArg;
    AppPsCmiReqData     *pReqData = (AppPsCmiReqData *)(pApiInfo->pInput);
    AppPsBlockReqTable  *pBlockReqTbl = &appPsBlockReqTbl;
    AppPsBlockReqInfo   *pBlockReqInfo = PNULL;

    // check the input
    OsaCheck(pApiInfo->sem != PNULL && pApiInfo->inputSize == sizeof(AppPsCmiReqData) &&
             pReqData != PNULL && pApiInfo->timeOutMs != 0,
             pApiInfo, pApiInfo->inputSize, pReqData);

    // check the request parameters
    OsaDebugBegin(pReqData->sgId >= CAC_DEV && pReqData->sgId <= CAC_SMS &&
                  pReqData->reqPrimId < 0xFFF && pReqData->reqParamLen > 0 && pReqData->pReqParam != PNULL,
                  pReqData->sgId, pReqData->reqPrimId, pReqData->reqParamLen);
    pApiInfo->retCode = CMS_INVALID_PARAM;
    osSemaphoreRelease(pApiInfo->sem);
    return;
    OsaDebugEnd();


    /*
     * find a valid "AppPsBlockReqInfo" in "AppPsBlockReqTable"
    */
    subHdrId = OsaUintBit0Search(pBlockReqTbl->freeBitMap);

    if (subHdrId > 15)  // valid
    {
        OsaDebugBegin(FALSE, subHdrId, pBlockReqTbl->freeBitMap, 0);
        OsaDebugEnd();

        ECOMM_TRACE(UNILOG_EC_API, appPsCmiReqBlockCallback_warn_1, P_WARNING, 1,
                    "EC API, APP CMI request is busy, can't proc new, freeBitmap: 0x%x", pBlockReqTbl->freeBitMap);

        pApiInfo->retCode = CMS_BUSY;
        osSemaphoreRelease(pApiInfo->sem);

        return;
    }

    reqHandler = APP_PS_BLOCK_REQ_START_HANDLER + subHdrId;
    pBlockReqInfo = &(pBlockReqTbl->blockList[subHdrId]);

    OsaCheck(pBlockReqInfo->guardTimer == OSA_TIMER_NOT_CREATE && pBlockReqInfo->pBlockInfo == PNULL,
             pBlockReqInfo->guardTimer, pBlockReqInfo->pBlockInfo, 0);

    /* send the CMI Request */
    CacCmiRequest(reqHandler,
                  pReqData->sgId,
                  pReqData->reqPrimId,
                  pReqData->reqParamLen,
                  pReqData->pReqParam);

    if (pApiInfo->timeOutMs < CMS_MAX_DELAY_MS)
    {
        pBlockReqInfo->guardTimer = OsaTimerNew(CMS_TASK_ID,
                                                APP_SET_BLOCK_REQ_GUARD_TIMER_ID(subHdrId),
                                                osTimerOnce);
        OsaCheck(pBlockReqInfo->guardTimer != OSA_TIMER_NOT_CREATE,
                 subHdrId, pReqData->sgId, pReqData->reqPrimId);

        OsaTimerStart(pBlockReqInfo->guardTimer, MILLISECONDS_TO_TICKS(pApiInfo->timeOutMs));
    }

    pBlockReqInfo->pBlockInfo = pApiInfo;

    OsaBit1Set(pBlockReqTbl->freeBitMap, subHdrId);

    return;
}

/**
  \fn      void psBlockProcCmiCnf(const SignalBuf *pSig)
  \brief   Process PS cmi confirm response for hander: APP_PS_BLOCK_REQ_START_HANDLER ~ APP_PS_BLOCK_REQ_END_HANDLER
  \returns
*/
void psBlockProcCmiCnf(const SignalBuf *pSig)
{
    UINT16      sgId = 0;
    UINT16      primId = 0;
    UINT32      subHdrId = 0;

    CacCmiCnf           *pCmiCnf = PNULL;
    AppPsBlockReqTable  *pBlockReqTbl = &appPsBlockReqTbl;
    AppPsBlockReqInfo   *pBlockReqInfo = PNULL;
    CmsBlockApiInfo     *pBlockApiInfo = PNULL;
    AppPsCmiReqData     *pAppCmiReqData = PNULL;

    pCmiCnf = (CacCmiCnf *)(pSig->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    // find the block info
    subHdrId = pCmiCnf->header.srcHandler - APP_PS_BLOCK_REQ_START_HANDLER;
    OsaCheck(subHdrId < APP_PS_BLOCK_REQ_NUM, subHdrId, pCmiCnf->header.srcHandler, APP_PS_BLOCK_REQ_START_HANDLER);

    pBlockReqInfo = &(pBlockReqTbl->blockList[subHdrId]);

    if (OsaIsBit1(pBlockReqTbl->freeBitMap, subHdrId) == FALSE)
    {
        OsaCheck(pBlockReqInfo->guardTimer == OSA_TIMER_NOT_CREATE && pBlockReqInfo->pBlockInfo == PNULL,
                 pBlockReqInfo->guardTimer, pBlockReqInfo->pBlockInfo, 0);

        ECOMM_TRACE(UNILOG_EC_API, psBlockProcCmiCnf_warning_1, P_WARNING, 3,
                    "EC API, blocked psCmiReq, guard timer maybe expiried before, sgId: %d, primId: %d, subHdrId: %d",
                    sgId, primId, subHdrId);
        return;
    }

    pBlockApiInfo = pBlockReqInfo->pBlockInfo;

    OsaCheck(pBlockApiInfo != PNULL && pBlockApiInfo->sem != PNULL && pBlockApiInfo->cmsBlockFunc == psBlockCmiReqCallback &&
             pBlockApiInfo->pInput != PNULL && pBlockApiInfo->inputSize == sizeof(AppPsCmiReqData),
             pBlockApiInfo, pBlockReqInfo->guardTimer, pBlockApiInfo->inputSize);

    pAppCmiReqData = (AppPsCmiReqData *)(pBlockApiInfo->pInput);

    /* whether the same CMI REQ */
    OsaDebugBegin(pAppCmiReqData->cnfPrimId == primId && pAppCmiReqData->sgId == sgId,
                  pAppCmiReqData->cnfPrimId, primId, sgId);
    return;
    OsaDebugEnd();

    /* fill the output */
    pBlockApiInfo->retCode = CMS_RET_SUCC;

    pAppCmiReqData->cnfRc = pCmiCnf->header.rc;

    if (pAppCmiReqData->cnfBufLen > 0 && pAppCmiReqData->pCnfBuf != PNULL)
    {
        memcpy(pAppCmiReqData->pCnfBuf,
               pCmiCnf->body,
               pAppCmiReqData->cnfBufLen);
    }

    /* stop guard timer */
    if (OsaTimerIsRunning(pBlockReqInfo->guardTimer))
    {
        OsaTimerStop(pBlockReqInfo->guardTimer);
    }
    if (pBlockReqInfo->guardTimer != OSA_TIMER_NOT_CREATE)
    {
        OsaTimerDelete(&(pBlockReqInfo->guardTimer));
    }

    OsaBit0Set(pBlockReqTbl->freeBitMap, subHdrId);

    /* release semphore */
    osSemaphoreRelease(pBlockReqInfo->pBlockInfo->sem);

    /*free block list*/
    pBlockReqInfo->pBlockInfo = PNULL;  //memory freed in SDK caller

    return;
}

/**
  \fn      psGetNetInfoSynCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
  \brief   Process the syn requset, called by: appGetNetInfoSync()
  \param[in]   inputSize        4 bytes
  \param[in]   pInput           CID
  \param[in]   outputSize       output buffer size
  \param[out]  pOutput          output buffer
  \returns
*/
CmsRetId psGetNetInfoSynCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
{
    NmAtiGetNetInfoReq  getNetInfoReq;
    /*
     * input is CID value, size: 4
     * output: NmAtiSyncRet
    */
    OsaCheck(inputSize == 4 && pInput != PNULL && outputSize == sizeof(NmAtiSyncRet) && pOutput != PNULL,
             inputSize, outputSize, pOutput);

    memset(&getNetInfoReq, 0, sizeof(NmAtiGetNetInfoReq));

    getNetInfoReq.cid = (UINT8)(*(UINT32 *)pInput);

    if (NetmgrAtiSyncReq(NM_ATI_SYNC_GET_NET_INFO_REQ, (void *)&getNetInfoReq, (NmAtiSyncRet *)pOutput) != 0)
    {
        OsaDebugBegin(FALSE, getNetInfoReq.cid, 0, 0);
        OsaDebugEnd();

        return CMS_FAIL;
    }

    return CMS_RET_SUCC;
}


/**
  \fn      void psBlockProcTimerExpiry(OsaTimerExpiry *pTimerExpiry)
  \brief   Process PS cmi confirm response for hander: APP_PS_BLOCK_REQ_START_HANDLER ~ APP_PS_BLOCK_REQ_END_HANDLER
  \returns
*/
void psBlockProcTimerExpiry(OsaTimerExpiry *pTimerExpiry)
{
    UINT16      timeId = pTimerExpiry->timerEnum;
    UINT32      subHdrId = 0;

    AppPsBlockReqTable  *pBlockReqTbl = &appPsBlockReqTbl;
    AppPsBlockReqInfo   *pBlockReqInfo = PNULL;
    CmsBlockApiInfo     *pBlockApiInfo = PNULL;
    AppPsCmiReqData     *pAppCmiReqData = PNULL;


    if (CMS_GET_OSA_TIMER_SUB_MOD_ID(timeId) != CMS_TIMER_APP_PS_BLOCK_REQ_SUB_MOD_ID)
    {
        OsaDebugBegin(FALSE, timeId, CMS_TIMER_APP_PS_BLOCK_REQ_SUB_MOD_ID, 0);
        OsaDebugEnd();

        return;
    }

    OsaCheck((timeId&0xFFF) < APP_PS_BLOCK_REQ_NUM, timeId, APP_PS_BLOCK_REQ_NUM, 0x00);
    subHdrId = APP_GET_BLOCK_REQ_GUARD_TIMER_SUB_HDR_ID(timeId);

    pBlockReqInfo = &(pBlockReqTbl->blockList[subHdrId]);

    if (OsaIsBit1(pBlockReqTbl->freeBitMap, subHdrId) == FALSE)
    {
        OsaCheck(pBlockReqInfo->guardTimer == OSA_TIMER_NOT_CREATE && pBlockReqInfo->pBlockInfo == PNULL,
                 pBlockReqInfo->guardTimer, pBlockReqInfo->pBlockInfo, 0);

        ECOMM_TRACE(UNILOG_EC_API, psBlockProcTimerExpiry_warning_1, P_WARNING, 1,
                    "EC API, block timer expiry, but block info not exist, maybe confirmed before, subHdrId: %d",
                    subHdrId);
        return;
    }

    pBlockApiInfo = pBlockReqInfo->pBlockInfo;

    OsaCheck(pBlockApiInfo != PNULL && pBlockApiInfo->sem != PNULL,
             pBlockApiInfo, pBlockReqInfo->guardTimer, pBlockApiInfo->inputSize);

    pBlockApiInfo->retCode  = CMS_TIME_OUT;

    if (pBlockApiInfo->cmsBlockFunc == psBlockCmiReqCallback)
    {
        OsaCheck(pBlockApiInfo->pInput != PNULL && pBlockApiInfo->inputSize == sizeof(AppPsCmiReqData),
                 pBlockApiInfo, pBlockReqInfo->guardTimer, pBlockApiInfo->inputSize);

        pAppCmiReqData = (AppPsCmiReqData *)(pBlockApiInfo->pInput);

        /* fill the output */
        pAppCmiReqData->cnfRc   = CME_UE_FAIL;
    }
    else
    {
        OsaDebugBegin(FALSE, pBlockApiInfo->cmsBlockFunc, psBlockCmiReqCallback, 0);
        OsaDebugEnd();
    }

    /* delete timer */
    if (pBlockReqInfo->guardTimer != OSA_TIMER_NOT_CREATE)
    {
        OsaTimerDelete(&(pBlockReqInfo->guardTimer));
    }

    OsaBit0Set(pBlockReqTbl->freeBitMap, subHdrId);

    /* release semphore */
    osSemaphoreRelease(pBlockReqInfo->pBlockInfo->sem);

    /*free block list*/
    pBlockReqInfo->pBlockInfo = PNULL;  //memory freed in SDK caller

    return;
}


