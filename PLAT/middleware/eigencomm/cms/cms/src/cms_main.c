/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    cms_main.c
 * Description:  Communication Modem Service layer/task source file
 * History:      Rev1.0   2018-08-06
 *
 ****************************************************************************/
#include "bsp.h"
#include "cms_comm.h"
#include "cmicomm.h"
#include "ostask.h"
#include "netmgr.h"
#include "debug_log.h"
#include "psdial.h"
#include "pslpp_api.h"
#include "ps_event_callback.h"
#include "ps_lib_api.h"
#include "ps_dev_if.h"
#include "psstk_bip_api.h"
#include "mw_config.h"
#include "cms_util.h"
#include "mw_aon_info.h"
#include "ps_sync_cnf.h"
#include "cmsnetlight.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "ecpm_ec616_external.h"
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "ecpm_ec616s_external.h"
#include "slpman_ec616s.h"
#endif


/******************************************************************************
 ******************************************************************************
  GLOBAL variables
 ******************************************************************************
******************************************************************************/


/*
 * CMS task stack and queue, size defined in ostask.h
*/

CHAR     cmsTaskStack[(CMS_TASK_STACK_SIZE)/sizeof(CHAR)];
UINT32   cmsTaskQueue[(CMS_TASK_QUEUE_SIZE)];



/*
 * CMS task pending signal queue
*/
static SignalQueue gCmsPendingSigQueue; // 8 bytes
#define pCmsPendingSigQue       &gCmsPendingSigQueue

#if PS_PMUTIME_TEST
uint8_t psWakeupState = 0;
#endif

/******************************************************************************
 * cmsDeepPmuCtrlBitmap
 * Whether CMS task allow the PMU enter the hibernate/sleep2 state (only AON memory)
 * Each sub-module has a bit to indicate whether allow it:
 *  0 - allow, 1 - not allow;
 * So when all bits is set to 0 (cmsDeepPmuCtrlBitmap == 0) just means CMS task
 *  is in IDLE state, and allow to enter hibernate state
******************************************************************************/
UINT32 cmsDeepPmuCtrlBitmap = 0x00000000;




/******************************************************************************
 ******************************************************************************
  External API/Variables specification
 ******************************************************************************
******************************************************************************/
extern void atProcSignal(const SignalBuf *pSig);

extern void atInit(void);


#define __DEFINE_INTERNAL_FUNCTION__ //just for easy to find this position----Below for internal use


static void cmsProcCmiCnfSig(const SignalBuf *cnfSignalPtr)
{
    CacCmiCnf   *pCmiCnf = (CacCmiCnf *)(cnfSignalPtr->sigBody);
    UINT16      sgId = CMS_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    UINT16      primId = CMS_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);
    UINT16      srcHandler = pCmiCnf->header.srcHandler;

    /*
     * PS DAIL
     *  cfun 0/power wake up also need to notify psdail
     */
    if (srcHandler == PS_DIAL_REQ_HANDLER ||
        (sgId == CAC_DEV && primId == CMI_DEV_SET_CFUN_CNF) ||
        (sgId == CAC_DEV && primId == CMI_DEV_SET_POWER_STATE_CNF))
    {
        psDialProcCmiCnf(cnfSignalPtr);
    }

    if (srcHandler == APP_SYNC_REQ_HANDLER)
    {
        psSyncProcCmiCnf(cnfSignalPtr);
    }

    if (srcHandler >= APP_PS_BLOCK_REQ_START_HANDLER &&
        srcHandler <= APP_PS_BLOCK_REQ_END_HANDLER)
    {
        psBlockProcCmiCnf(cnfSignalPtr);
    }


    // if CMI REQ is sent from PS LPP api
    if ((srcHandler == PS_LPP_REQ_RSP_HANDLER) ||
        (sgId == CAC_DEV && primId == CMI_DEV_SET_CFUN_CNF))
    {
        psLppProcCmiCnf(cnfSignalPtr);
    }

    if (pCmiCnf->header.srcHandler == PS_STK_REQ_RSP_HANDLER)
    {
        psStkBipProcCmiCnf(cnfSignalPtr);
    }

    /*
     * check the channel ID
    */
    if (CMS_GET_HANDLER_CHAN_ID(srcHandler) == CMS_CHAN_RSVD)
    {
        if (srcHandler != CMS_REQ_HANDLER &&
            srcHandler != PS_DIAL_REQ_HANDLER &&
            srcHandler != PS_STK_REQ_RSP_HANDLER &&
            srcHandler != APP_SYNC_REQ_HANDLER &&
            srcHandler != PS_LPP_REQ_RSP_HANDLER &&
            (srcHandler < APP_PS_BLOCK_REQ_START_HANDLER ||
             srcHandler > APP_PS_BLOCK_REQ_END_HANDLER))
        {
            /*ps channel should not be 0:  CMS_CHAN_RSVD, else print a warning*/
            OsaDebugBegin(FALSE, srcHandler, CMS_CHAN_RSVD, 0);
            OsaDebugEnd();
        }
    }

    /* netlight monitor*/
    cmsNetlightMonitorCmiCnf(cnfSignalPtr);

    return;
}


static void cmsProcCmiIndSig(const SignalBuf *indSignalPtr)
{
    /*
     * broadcast to all related modules
    */
    /* PS DIAL */
    psDialProcCmiInd(indSignalPtr);

    /* PS STK */
    psStkBipProcCmiInd(indSignalPtr);

    /* APP */
    psCmiIndToAppEventTrigger(indSignalPtr);

    /* PS LPP */
    psLppProcCmiInd(indSignalPtr);

    /* netlight monitor*/
    cmsNetlightMonitorCmiInd(indSignalPtr);

    return;
}

static void cmsProcApplIndSig(CmsApplInd *pAppInd)
{
    applIndToAppEventTrigger(pAppInd);
}


static void cmsProcAppSeq(const SignalBuf *SignalPtr)
{
    appFunCb    *pAppFunCb = (appFunCb *)(SignalPtr->sigBody);
    pAppFunCb->function(pAppFunCb->ctx);
    return;
}

/**
  \fn          void cmsProcSynApiReqSig(CmsSynApiReq *pSynApiReq)
  \brief
  \returns     void
*/
static void cmsProcSynApiReqSig(CmsSynApiReq *pSynApiReq)
{
    CmsSynApiInfo   *pApiInfo = pSynApiReq->pApiInfo;
    UINT16          inputSize4 = 0;
    UINT16          outputSize4 = 0;
    UINT32          *pInMagic = PNULL, *pOutMagic = PNULL;
    UINT8           *pInAddr = PNULL, *pOutAddr = PNULL;

    OsaDebugBegin(pApiInfo != PNULL, pSynApiReq, 0, 0);
    return;
    OsaDebugEnd();

    inputSize4  = (pApiInfo->inputSize+3)&0xFFFFFFFC;       /* 4 bytes aligned */
    outputSize4 = (pApiInfo->outputSize+3)&0xFFFFFFFC;      /* 4 bytes aligned */

    /*
     * +----------------------+--------------+---------+--------------+-------+
     * | CmsSynApiInfo        | input PARAM  | MAGIC   | output PARAM | MAGIC |
     * +----------------------+--------------+---------+--------------+-------+
     * ^                      ^              ^         ^              ^
     * pApiInfo               pInAddr        pInMagic  pOutAddr       pOutMagic
    */

    pInAddr     = pApiInfo->param;
    pInMagic    = (UINT32 *)(pApiInfo->param + inputSize4);
    pOutAddr    = pApiInfo->param + inputSize4 + CMS_MAGIC_WORDS_SIZE;
    pOutMagic   = (UINT32 *)(pApiInfo->param + inputSize4 + CMS_MAGIC_WORDS_SIZE + outputSize4);

    OsaCheck(pApiInfo->hdrMagic == CMS_MAGIC_WORDS && *pInMagic == CMS_MAGIC_WORDS && *pOutMagic == CMS_MAGIC_WORDS,
             pApiInfo->hdrMagic, *pInMagic, *pOutMagic);

    if (pApiInfo->cmsSynFunc != PNULL)
    {
        if (pApiInfo->inputSize == 0 && pApiInfo->outputSize == 0)
        {
            //INT32 (*CmsSynCallback)(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput);
            pApiInfo->retCode = pApiInfo->cmsSynFunc(0, PNULL, 0, PNULL);
        }
        else if (pApiInfo->inputSize == 0)
        {
            pApiInfo->retCode = pApiInfo->cmsSynFunc(0, PNULL, pApiInfo->outputSize, pOutAddr);
        }
        else if (pApiInfo->outputSize == 0)
        {
            pApiInfo->retCode = pApiInfo->cmsSynFunc(pApiInfo->inputSize, pInAddr, 0, PNULL);
        }
        else
        {
            pApiInfo->retCode = pApiInfo->cmsSynFunc(pApiInfo->inputSize, pInAddr, pApiInfo->outputSize, pOutAddr);
        }
    }
    else
    {
        OsaCheck(FALSE, pApiInfo->retCode, inputSize4, outputSize4);
    }

    if (pApiInfo->sem)
    {
        osSemaphoreRelease(pApiInfo->sem);
    }
    else
    {
        OsaDebugBegin(FALSE, pApiInfo->sem, pApiInfo->retCode, 0);
        OsaDebugEnd();
    }

    return;
}

/**
  \fn          void cmsProcHighPriSynApiReqSig(CmsSynApiReq *pSynApiReq)
  \brief
  \returns     void
*/
static void cmsProcHighPriSynApiReqSig(CmsSynApiReq *pSynApiReq)
{
    cmsProcSynApiReqSig(pSynApiReq);
    return;
}


/**
  \fn          void cmsProcBlockApiReqSig(CmsBlockApiReq *pBlockApiReq)
  \brief
  \returns     void
*/
static void cmsProcBlockApiReqSig(CmsBlockApiReq *pBlockApiReq)
{
    CmsBlockApiInfo *pBlockApiInfo = pBlockApiReq->pApiInfo;

    OsaDebugBegin(pBlockApiInfo != PNULL, pBlockApiInfo, 0, 0);
    return;
    OsaDebugEnd();

    OsaCheck(pBlockApiInfo->cmsBlockFunc != PNULL, pBlockApiInfo->cmsBlockFunc, pBlockApiInfo->inputSize, pBlockApiInfo->timeOutMs);

    //semphore release in: "cmsBlockFunc()"
    pBlockApiInfo->cmsBlockFunc(pBlockApiInfo);

    return;
}

/**
  \fn          void cmsProcNonBlockApiReqSig(CmsNonBlockApiReq *pNonBlockApiReq)
  \brief
  \returns     void
*/
static void cmsProcNonBlockApiReqSig(CmsNonBlockApiReq *pNonBlockApiReq)
{
    OsaCheck(pNonBlockApiReq->cmsNonBlockFunc != PNULL, pNonBlockApiReq->cmsNonBlockFunc, pNonBlockApiReq->paramSize, 0);

    if (pNonBlockApiReq->paramSize > 0)
    {
        pNonBlockApiReq->cmsNonBlockFunc(pNonBlockApiReq->paramSize, pNonBlockApiReq->param);
    }
    else
    {
        pNonBlockApiReq->cmsNonBlockFunc(0, PNULL);
    }

    return;
}


/**
  \fn          void cmsProcBlockApiReqSig(CmsBlockApiReq *pBlockApiReq)
  \brief
  \returns     void
*/
static void cmsProcTimerExpirySig(OsaTimerExpiry *pExpiry)
{
    UINT16  timeId = pExpiry->timerEnum;

    switch (CMS_GET_OSA_TIMER_SUB_MOD_ID(timeId))
    {
        case CMS_TIMER_AT_SUB_MOD_ID:
            /* PROC in at_entity.c*/
            break;

        case CMS_TIMER_PS_LPP_SUB_MOD_ID:
            psLppProcTimerExpiry(pExpiry);
            break;

        case CMS_TIMER_APP_PS_BLOCK_REQ_SUB_MOD_ID:
            psBlockProcTimerExpiry(pExpiry);
            break;

        default:
            OsaDebugBegin(FALSE, timeId, 0, 0);
            OsaDebugEnd();
            break;
    }

    return;
}


static CmsRetId cmsProcSignal(const SignalBuf *pSignalPtr)
{
    BOOL bProced = TRUE;

    switch(pSignalPtr->sigId)
    {
        case SIG_APP_MSG_CALLBACK:
            cmsProcAppSeq(pSignalPtr);
            break;

        case SIG_CAC_CMI_CNF:
            cmsProcCmiCnfSig(pSignalPtr);
            break;

        case SIG_CAC_CMI_IND:
            cmsProcCmiIndSig(pSignalPtr);
            break;

        case SIG_CMS_APPL_IND:
            cmsProcApplIndSig((CmsApplInd *)(pSignalPtr->sigBody));
            break;

        case SIG_CMS_SYN_API_REQ:
            cmsProcSynApiReqSig((CmsSynApiReq *)(pSignalPtr->sigBody));
            break;

        case SIG_CMS_HIGH_PRI_SYN_API_REQ:
            cmsProcHighPriSynApiReqSig((CmsSynApiReq *)(pSignalPtr->sigBody));
            break;

        case SIG_CMS_BLOCK_API_REQ:
            cmsProcBlockApiReqSig((CmsBlockApiReq *)(pSignalPtr->sigBody));
            break;

        case SIG_CMS_NON_BLOCK_API_REQ:
            cmsProcNonBlockApiReqSig((CmsNonBlockApiReq *)(pSignalPtr->sigBody));
            break;

        case SIG_TIMER_EXPIRY:
            cmsProcTimerExpirySig((OsaTimerExpiry *)(pSignalPtr->sigBody));
            break;

        default:
            bProced = FALSE;
            break;
    }

    if (bProced == FALSE)
    {
        // maybe signal to SIM STK
        psStkBipProcSimBipSig(pSignalPtr);
    }

    return CMS_RET_SUCC;
}

/*add this func for some works which need to do when power on reset triggered
and should after AT init done, the work should not to do when wakeup case*/
static void porHandleFunc(void)
{
    //check if power on reset triggered
    if(slpManGetLastSlpState() == SLP_ACTIVE_STATE)
    {
        //add a flag for NITZ,clear to 0 when power on reset.set to 1 when NITZ triggered

        if(mwGetTimeSyncFlag()==TRUE)
           mwSetTimeSyncFlag(FALSE);//clear to 0 when power on init


         //send ECRDY URC to AT port only when power on init and will be better after at init
        if (UsartAtCmdHandle != NULL)//now use AT UART handler directly,since it is not a real AT indication for using AT resp API
        {
//            UsartAtCmdHandle->SendPolling((CHAR *)"\r\nECRDY\r\n", strlen("\r\nECRDY\r\n"));
			UsartAtCmdHandle->SendPolling((CHAR *)"\r\nREBOOT_CAUSE_APPLICATION_POWER_ON_RESET\r\n",
				strlen("\r\nREBOOT_CAUSE_APPLICATION_POWER_ON_RESET\r\n"));
		}


        //sth else todo in future

    }

    return;

}

/**
  \fn          void cmsWaitPhyWakeupReady(void)
  \brief       CMS task just waiting for wake up indication from PHY, then could continue normal jobs
  \returns     void
*/
static void cmsWaitPhyWakeupReady(void)
{
    SignalBuf   *pSig   = PNULL;

    if (pmuBWakeupFromHib() == FALSE &&
        pmuBWakeupFromSleep2() == FALSE)
    {
        return;
    }

    /*
     * wait for "SIG_PROXY_PHY_WAKEUP_READY_IND" from PHY
    */
    while (TRUE)
    {
        pSig = PNULL;

        OsaReceiveSignal(CMS_TASK_ID, &pSig);
        OsaCheck(pSig != PNULL, CMS_TASK_ID, pSig, 0);

        if (pSig->sigId == SIG_CMS_PHY_WAKEUP_READY_IND)
        {
            ECOMM_TRACE(UNILOG_CMS, cmsWaitPhyWakeupInd_2, P_VALUE, 0,
                        "CMS, recv: SIG_PROXY_PHY_WAKEUP_READY_IND");

            /* destory signal, and break*/
            OsaDestroySignal(&pSig);
            break;
        }
        else if (pSig->sigId == SIG_CMS_HIGH_PRI_SYN_API_REQ)
        {
            /* high priority, need to process immediately */
            //ECOMM_TRACE(UNILOG_CMS, cmsWaitPhyWakeupReady_1, P_DEBUG, 1, "SIG received pointer:%x", pSig);

            cmsProcHighPriSynApiReqSig((CmsSynApiReq *)(pSig->sigBody));

            OsaDestroySignal(&pSig);

            /*can't break, still need to wait: SIG_CMS_PHY_WAKEUP_READY_IND*/
        }
        else
        {
            /*As during wake up, maybe some CMI IND send to CMS task*/
            ECOMM_TRACE(UNILOG_CMS, cmsWaitPhyWakeupInd_3, P_WARNING, 1,
                        "CMS received sig: 0x%x, while waiting for PHY ready ind, just enqueue", pSig->sigId);


            /*signal not processed, need to enqueue*/
            OsaSigEnQueue(pCmsPendingSigQue, &pSig);
        }
    }

    return;
}


/**
  \fn          void cmsInit()
  \brief       CMS task initialization
  \returns     void
*/
static void cmsInit(void)
{
    /*
     * init pending signal queue
    */
    OsaSigQueueInit(pCmsPendingSigQue);

    /*
     * check when wake up from HIB/Sleep2 state
    */
    if (pmuBWakeupFromHib() || pmuBWakeupFromSleep2())
    {
        /* wait for PHY ready ind */
        ECOMM_TRACE(UNILOG_CMS, cmsInit_phy_ready_1, P_VALUE, 0,
                    "CMS wake up from Hib/sleep2, just waiting for PHY ready Ind");
        cmsWaitPhyWakeupReady();

        /* wake up PS firstly */
        ECOMM_TRACE(UNILOG_CMS, cmsInit_1, P_VALUE, 0,
                    "CMS wake up from Hib/sleep2, wake up PS");
        cmsWakeupPs(PNULL);

        /*
         * Here, restore/recovery the HIB timer firstly, before wake up PHY, refer bug: 1330
         * If wake up PHY before restore the HIB timer, then, in case:
         * 1> PHY recevied paging, and wake up from HIB/Sleep2,
         * 2> As PHY & PS task have high priority than CMS task, EMM task maybe try to stop T3324, before HIB timer restored.
         *    then a bug/assert will be happened.
        */

        /* restore HIB timer */
        ECOMM_TRACE(UNILOG_CMS, cmsInit_2, P_VALUE, 0,
                    "CMS wake up from Hib/sleep2, restore HIB timer");
        extern void pmuRestoreHibTimer(void);
        pmuRestoreHibTimer();

        /* wake up PHY task*/
        ECOMM_TRACE(UNILOG_CMS, cmsInit_3, P_VALUE, 0,
                    "CMS wake up from Hib/sleep2, wake up PHY task");
        extern void PhySendSignalAfterWakeupFullImage(void);
        PhySendSignalAfterWakeupFullImage();
    }

    /*
     * init ps lpp
    */
    PslppInitialise();

    return;
}



#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position---Below for external use



/**
  \fn          UINT32 cmsGetTaskStackSize(void)
  \brief       cms task  is created in ostask.c which is not open source, customer may modify
               CMS_TASK_STACK_SIZE macro in ostask.h to customerize its queue size
  \param[in]   void
  \returns     UINT32
*/

UINT32 cmsGetTaskStackSize(void)
{
    return CMS_TASK_STACK_SIZE;
}


/**
  \fn          UINT32 cmsGetQueueSize(void)
  \brief       cms task queue is created in ostask.c which is not open source, customer may modify
               CMS_TASK_QUEUE_SIZE macro in ostask.h to customerize its queue size
  \param[in]   void
  \returns     UINT32
*/
UINT32 cmsGetQueueSize(void)
{
    return CMS_TASK_QUEUE_SIZE;
}


NBStatus_t appCallbackWithNoBlock(app_callback_fn function, void *ctx)
{
    SignalBuf   *pSig = PNULL;
    appFunCb    *pAppFunCb = PNULL;

    OsaCreateZeroSignal(SIG_APP_MSG_CALLBACK, sizeof(appFunCb), &pSig);

    pAppFunCb = (appFunCb *)(pSig->sigBody);
    pAppFunCb->function = function;
    pAppFunCb->ctx   = ctx;

    OsaSendSignal(CMS_TASK_ID, &pSig);
    return NBStatusSuccess;

}




/**
  \fn          void cmsPhyWakeupReadyInd(void)
  \brief       When wake from Sleep2/Hib state, PHY need to prepare some works (such as pre-syn) before the
                MODEM real starting, after PHY done preparation, just call this API, to let PS start to work.
               This API should called in task.
  \param[in]   void
  \returns     void
*/
void cmsPhyWakeupReadyInd(void)
{
    SignalBuf   *pSig = PNULL;

    OsaCreateZeroSignal(SIG_CMS_PHY_WAKEUP_READY_IND, sizeof(OsaEmptySignal), &pSig);

    OsaSendSignal(CMS_TASK_ID, &pSig);

    return;
}


/**
  \fn          void cmsWakeupPs(void *arg)
  \brief       Wake up PS (modem), PS can't process any other signal/command before wake up confiramtion,
  \            here, all signal need to pending (enqueue) before wake up confiramtion from PS
  \returns     void
*/
void cmsWakeupPs(void *arg)
{
    SignalBuf   *pSig   = PNULL;
    CacCmiCnf   *pCmiCnf = PNULL;
    UINT16      sgId    = 0;
    UINT16      primId  = 0;

    #if PS_PMUTIME_TEST
    psWakeupState = 1;
    #endif

    /*
     * send wake up signal to PS
    */
    devSetPowerState(CMS_REQ_HANDLER, CMI_DEV_POWER_WAKE_UP);

    /*
     * wait for confirmation from PS
    */
    while (TRUE)
    {
        pSig = PNULL;

        OsaReceiveSignal(CMS_TASK_ID, &pSig);
        OsaCheck(pSig != PNULL, CMS_TASK_ID, pSig, 0);

        if (pSig->sigId == SIG_CAC_CMI_CNF)
        {
            pCmiCnf = (CacCmiCnf *)(pSig->sigBody);
            sgId = CMS_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
            primId = CMS_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

            if ((sgId == CAC_DEV && primId == CMI_DEV_SET_POWER_STATE_CNF) ||
                (sgId == CAC_PS && primId == CMI_PS_GET_CEREG_CNF) ||
                (sgId == CAC_PS && primId == CMI_PS_READ_BEARER_DYN_CTX_CNF))
            {
                OsaDebugBegin(pCmiCnf->header.rc == CME_SUCC, pCmiCnf->header.rc, 0, 0);
                OsaDebugEnd();

                /* PS DAIL need also process it */
                psDialProcCmiCnf(pSig);

                #if PS_PMUTIME_TEST
                psWakeupState = 2;
                #endif

                /* destory signal, and break*/
                OsaDestroySignal(&pSig);    //pSig = PNULL in this API

                /* need to check whether netif recovery done */
                if (psDailBeActNetifDoneDuringWakeup())
                {
                    break;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_CMS, cmsWakeupPs_value_1, P_VALUE, 2,
                                "CMS recv CMI CNF, SgId: %d, PrimId: %d, netif not recovery, still need pending",
                                sgId, primId);
                }
            }
        }
        else if (pSig->sigId == SIG_CMS_HIGH_PRI_SYN_API_REQ)
        {
            /* high priority, need to process immediately */
            cmsProcHighPriSynApiReqSig((CmsSynApiReq *)(pSig->sigBody));

            OsaDestroySignal(&pSig);    //pSig = PNULL in this API

            /*can't break, still need to wait PS wake up done*/
        }

        if (pSig != PNULL)
        {
            /*As during wake up, maybe some CMI IND send to PS PROXY task*/
            if (pSig->sigId != SIG_CAC_CMI_IND)
            {
                ECOMM_TRACE(UNILOG_CMS, cmsWakeupPs_1, P_WARNING, 1,
                            "CMS received sig: 0x%x, while waiting for PS waking up, just enqueue", pSig->sigId);
            }

            /*signal not processed, need to enqueue*/
            OsaSigEnQueue(pCmsPendingSigQue, &pSig);
        }
    }

    return;
}



/**
  \fn          void CmsTaskEntry(void *pvParameters)
  \brief       Main entity for CMS task
  \param[in]   pvParameters         useless now
  \returns     void
*/
void CmsTaskEntry(void *pvParameters)
{
    SignalBuf   *pSig = PNULL;

    initPsCmiReqMapList();

    mwLoadNvmConfig();
    mwAonInfoInit();

    porHandleFunc();

    /* init AT */
    atInit();

    cmsInit();

    /*MW AON task recovery here*/
    if (pmuBWakeupFromHib() || pmuBWakeupFromSleep2())
    {
        mwAonTaskRecovery();
    }

    while (TRUE)
    {
        /*
         * vote to HIB/Sleep2 by default
        */
        if (cmsDeepPmuCtrlBitmap == 0x0)
        {
            pmuVoteToHIBState(PMU_DEEPSLP_CMS_MOD, TRUE);
            pmuVoteToSleep2State(PMU_DEEPSLP_CMS_MOD, TRUE);
        }
        else
        {
            pmuVoteToHIBState(PMU_DEEPSLP_CMS_MOD, FALSE);
            pmuVoteToSleep2State(PMU_DEEPSLP_CMS_MOD, FALSE);
        }


        pSig = PNULL;
        // check from any signal enqueue
        if (OsaSigOnQueue(pCmsPendingSigQue))
        {
            OsaSigDeQueue(pCmsPendingSigQue, &pSig);
        }
        else
        {
            OsaReceiveSignal(CMS_TASK_ID, &pSig);
        }
        OsaCheck(pSig != PNULL, CMS_TASK_ID, pSig, 0);

        atProcSignal(pSig);

        cmsProcSignal(pSig);

        OsaDestroySignal(&pSig);
    }

}



