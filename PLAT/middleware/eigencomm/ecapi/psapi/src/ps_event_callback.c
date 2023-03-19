/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ps_event_callback.c
 * Description:  API interface implementation source file for event callback service
 * History:      Rev1.0   2018-10-12
 *
 ****************************************************************************/
#include "ps_event_callback.h"
#include "cmips.h"
#include "cmisim.h"
#include "cmimm.h"
//#include "ccmsig.h"
#include "netmgr.h"


#define MAX_EVENT_NUM     (10)

typedef struct {
    groupMask_t groupMask;
    psEventCallback_t callback;
} psEventCallbackNode_t;


psEventCallbackNode_t psEventCallbackList[MAX_EVENT_NUM];
extern void mwSetTimeSyncFlag(UINT32 timeSyncTriggered);
void PSInitEventCallbackTable()
{
    memset(&psEventCallbackList, 0x00, sizeof(psEventCallbackList));
}

NBStatus_t registerPSEventCallback(groupMask_t groupMask, psEventCallback_t callback)
{
    INT8 i, available_id = -1;
    for (i = 0; i < MAX_EVENT_NUM; i++)
    {
        if (psEventCallbackList[i].callback == callback)
        {
            // Already registered
            return NBStatusSuccess;
        }
        if (available_id < 0 && psEventCallbackList[i].callback == NULL)
        {
            // Find available entry
            available_id = i;
        }
    }

    if (available_id== -1)
    {
        return NBStatusFail;
    } else {
        psEventCallbackList[available_id].groupMask = groupMask;
        psEventCallbackList[available_id].callback = callback;
        return NBStatusSuccess;
    }
}


NBStatus_t deregisterPSEventCallback(psEventCallback_t callback)
{
    UINT8 i;
    for (i = 0; i < MAX_EVENT_NUM; i++)
    {
        if (psEventCallbackList[i].callback == callback)
        {
            psEventCallbackList[i].callback = 0;
            psEventCallbackList[i].groupMask = (groupMask_t)0;
            return NBStatusSuccess;
        }
    }
    return NBStatusFail;
}

void PSTriggerEvent(urcID_t eventID, void *param, UINT32 paramLen)
{
    UINT32 groupMask = 0;
    UINT8 i;
    groupMask = (groupMask_t)(0x01<<((eventID & 0XF000) >> 12));
    for (i = 0; i < MAX_EVENT_NUM; i++)
    {
        if (psEventCallbackList[i].callback != NULL && psEventCallbackList[i].groupMask & groupMask)
        {
            psEventCallbackList[i].callback(eventID, param, paramLen);
        }
    }
}

#define __DEFINE_INTERNAL_FUNCTION__ //just for easy to find this position----Below for internal use


static void psIndToEvent(UINT16 primId, void *paras)
{
    switch (primId)
    {
        case CMI_PS_BEARER_ACTED_IND:
        {
            CmiPsBearerActedInd *pBearerActedInd = (CmiPsBearerActedInd *)paras;
            if (pBearerActedInd->bearerType == CMI_PS_BEARER_DEFAULT)
            {
                PSTriggerEvent(NB_URC_ID_PS_BEARER_ACTED, NULL, 0);
            }
            break;
        }
        case CMI_PS_BEARER_DEACT_IND:
        {
            PSTriggerEvent(NB_URC_ID_PS_BEARER_DEACTED, NULL, 0);
            break;
        }
        case CMI_PS_CEREG_IND:
        {
            CmiPsCeregInd *pPsCeregInd = (CmiPsCeregInd *)paras;
            PSTriggerEvent(NB_URC_ID_PS_CEREG_CHANGED, pPsCeregInd, sizeof(CmiPsCeregInd));
            break;
        }
    }

    return;
}

static void simIndToEvent(UINT16 primId, void *paras)
{
    switch (primId)
    {
        case CMI_SIM_UICC_STATE_IND:
        {
            CmiSimUiccStateInd *pCmiUiccStateInd = (CmiSimUiccStateInd *)paras;
            if (pCmiUiccStateInd->uiccState == CMI_SIM_UICC_ACTIVE)
            {
                PSTriggerEvent(NB_URC_ID_SIM_READY, &(pCmiUiccStateInd->imsiStr), sizeof(pCmiUiccStateInd->imsiStr));
            }
            else if (pCmiUiccStateInd->uiccState == CMI_SIM_UICC_REMOVED)
            {
                PSTriggerEvent(NB_URC_ID_SIM_REMOVED, NULL, 0);
            }
            break;
        }
        default:
            break;
    }
    return;
}

static void mmIndToEvent(UINT16 primId, void *paras)
{
    //UINT32 NITZ_Flag = 0;

    switch (primId)
    {
        case CMI_MM_EXTENDED_SIGNAL_QUALITY_IND:
        {
            CmiMmCesqInd *pMmCesqInd = (CmiMmCesqInd *)paras;
            PSTriggerEvent(NB_URC_ID_MM_SIGQ, pMmCesqInd, sizeof(CmiMmCesqInd));
            break;
        }
        case CMI_MM_COVERAGE_ENHANCEMENT_STATUS_IND:
        {
            UINT8 celevel = 0;
            CmiMmCEStatusInd *pMmCesInd = (CmiMmCEStatusInd *)paras;
            celevel = pMmCesInd->ceLevel;
            PSTriggerEvent(NB_URC_ID_MM_CERES_CHANGED, &celevel, 1);
            break;
        }
        case CMI_MM_EMM_TIMER_STATE_IND:
        {
            UINT8 timerState = 0;
            CmiMmEmmTimerStateInd *pTimerInd = (CmiMmEmmTimerStateInd *)paras;
            if ((pTimerInd->emmTimer == CMI_MM_EMM_TAU_TIMER) && (pTimerInd->timerState == CMI_MM_EMM_TIMER_EXPIRY))
                PSTriggerEvent(NB_URC_ID_MM_TAU_EXPIRED, NULL, 0);
            if (pTimerInd->emmTimer == CMI_MM_EMM_T3346 || pTimerInd->emmTimer == CMI_MM_EMM_T3448)
            {
                timerState = pTimerInd->timerState;
                PSTriggerEvent(NB_URC_ID_MM_BLOCK_STATE_CHANGED, &timerState, 1);
            }
            break;
        }

        case CMI_MM_NITZ_IND:
        {
            CmiMmNITZInd *pMmNitzInd = (CmiMmNITZInd *)paras;
            PSTriggerEvent(NB_URC_ID_MM_NITZ_REPORT, pMmNitzInd, sizeof(CmiMmNITZInd));
            if (pMmNitzInd->utcInfoPst)
            {
                #ifdef FEATURE_REF_AT_ENABLE
                //NITZ_Flag = mwAonGetAtChanConfigItemValue(AT_CHAN_DEFAULT, MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG);
                if(mwAonGetAtChanConfigItemValue(AT_CHAN_DEFAULT, MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG) == 0)  /* add for ref nitz cmd*/
                {
                    break;
                }
                #endif
                if (OsaTimerSync(0,
                                 SET_LOCAL_TIME,
                                 (pMmNitzInd->utcInfo.year<<16)|(pMmNitzInd->utcInfo.mon<<8)|(pMmNitzInd->utcInfo.day),
                                 (pMmNitzInd->utcInfo.hour<<24)|(pMmNitzInd->utcInfo.mins<<16)|(pMmNitzInd->utcInfo.sec<<8),
                                 (((INT32)pMmNitzInd->utcInfo.tz)&0xff)
                                 ) == CMS_FAIL)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, synctime100, P_INFO, 0, "sync NITZ time fail");
                }
                else
                {
                    //OsaTimerSync has set the flag
                    //if(mwGetTimeSyncFlag()==FALSE)
                    //{
                    //    mwSetTimeSyncFlag(TRUE);
                    //}
                }
            }
            break;
        }

        case CMI_MM_PSM_CHANGE_IND:
        {
            /*
             * Enter/exit PSM state indication
            */
            CmiMmPsmChangeInd *pCmiInd = (CmiMmPsmChangeInd *)paras;
            UINT32  psmState = pCmiInd->psmMode;    // 0 - Exit PSM, 1 - Enter PSM

            PSTriggerEvent(NB_URC_ID_MM_PSM_STATE_CHANGED, &psmState, sizeof(UINT32));

            break;
        }

        case CMI_MM_PLMN_SELECT_STATE_IND:
        {
            CmiMmPlmnSelectStateInd *pMmPlmnSelectStateInd = (CmiMmPlmnSelectStateInd *)paras;
            PSTriggerEvent(NB_URC_ID_MM_PLMN_SELECT_STATE_CHANGED, pMmPlmnSelectStateInd, sizeof(CmiMmPlmnSelectStateInd));
            break;
        }

        default:
            break;
    }
    return;
}

void nmIndToEvent(UINT16 primId, void *paras)
{
    switch(primId)
    {
        case NM_ATI_NET_INFO_IND:
        {
            NmAtiNetInfoInd *pNetInfoInd = (NmAtiNetInfoInd *)paras;
            PSTriggerEvent(NB_URC_ID_PS_NETINFO, &(pNetInfoInd->netifInfo), sizeof(pNetInfoInd->netifInfo));
            break;
        }

        default:
            break;
    }
}

void devIndToEvent(UINT16 primId, void *paras)
{
    switch(primId)
    {
        default:
            break;
    }
}


#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position---Below for external use

void psCmiIndToAppEventTrigger(const SignalBuf *indSignalPtr)
{
    CacCmiInd   *pCmiInd = PNULL;
    UINT16    primId = 0, sgId = 0;

    pCmiInd = (CacCmiInd *)(indSignalPtr->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiInd->header.indId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiInd->header.indId);
    switch(sgId)
    {
        case CAC_PS:
            psIndToEvent(primId, pCmiInd->body);
            break;
        case CAC_SIM:
            simIndToEvent(primId, pCmiInd->body);
            break;
        case CAC_MM:
            mmIndToEvent(primId, pCmiInd->body);
            break;
        case CAC_DEV:
            devIndToEvent(primId, pCmiInd->body);
        case CAC_SMS:
        default:
            break;
    }

    return;
}

void applIndToAppEventTrigger(CmsApplInd *pAppInd)
{
    switch(pAppInd->header.appId)
    {
        case APPL_NM:
            nmIndToEvent(pAppInd->header.primId, pAppInd->body);
            break;

        default:
            break;
    }

    return;
}


