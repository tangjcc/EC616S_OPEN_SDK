
/******************************************************************************
Copyright:      - 2020, All rights reserved by Eigencomm Ltd.
File name:      - esimmonitortask.c
Description:    - task process the SIG with CMS
Function List:  -
History:        - 04/12/2020, Originated by yjwang
******************************************************************************/

#ifdef SOFTSIM_FEATURE_ENABLE

#include "commonTypedef.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cms_api.h"
#include "osasys.h"
#include "ostask.h"
#include "debug_log.h"
#include "debug_trace.h"

#include "esim_ta.h"
#include "esimmonitortask.h"
#include "atc_reply.h"

#ifdef SOFTSIM_CT_ENABLE
#include "atec_cust_softsim.h"
#endif

static eSimProcTaskMsg    geSimProcTaskMgtData = {0};
osMessageQueueId_t        esimInQHandle = NULL;

static void eSimProcTaskEntry(void *arg);


/**
  \fn          esimProcCmsDataUpReq
  \brief       handle the cms at commands date request signal
  \param[in]   cmsDataUp the  SIG_ESIM_SIGNAL_SEND_REQ data
  \returns     void
*/
static void esimProcCmsDataUpReq(eSimCmsAtDataReqMsg  *cmsDataUp)
{
#ifdef SOFTSIM_CT_ENABLE
    UINT16                  send_ret   = 0;
#endif
    UINT8                  *atRecvBuf  = NULL;
    UINT16                  atrecvLen  = 2048;
    SignalBuf              *psignal     = PNULL;
    eSimCmsAtDataReqMsg    *pcmsAtreq   = cmsDataUp;
    eSimCmsAtDataCnfMsg    *pcmsAtResp = NULL;

    OsaCheck(cmsDataUp != 0, 0, 0, 0);

    OsaCreateZeroSignal(SIG_ESIM_SIGNAL_SEND_CNF, (sizeof(eSimCmsAtDataCnfMsg)), &psignal);
    OsaCheck(psignal != PNULL, SIG_ESIM_SIGNAL_SEND_CNF, ( sizeof(eSimCmsAtDataCnfMsg)), 0);

    pcmsAtResp        = (eSimCmsAtDataCnfMsg *)(psignal->sigBody);

    atRecvBuf =  (UINT8 *)malloc(atrecvLen);
    if(atRecvBuf == NULL)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, esimProcCmsDataUpReq_1, P_INFO, 0,"send ERROR cnf result");
        if(pcmsAtreq->ReqData)
        {
            free(pcmsAtreq->ReqData);
            pcmsAtreq->ReqData = NULL;
        }

        pcmsAtResp->errcode = ESIM_RET_RESULT_FAIL;
        OsaSendSignal(CMS_TASK_ID, &psignal);

        return;
    }
    memset(atRecvBuf,0,atrecvLen);

    ECOMM_TRACE(UNILOG_SOFTSIM, esimProcCmsDataUpReq_2, P_INFO, 0,"send to process_entry");

    #ifdef SOFTSIM_CT_ENABLE
    if((send_ret = esim_process_entry(pcmsAtreq->type ,pcmsAtreq->ReqData,pcmsAtreq->ReqLen,atRecvBuf,&atrecvLen)) != 0  ||
         atrecvLen == 0 )
    {
        ECOMM_TRACE(UNILOG_ATCMD_EXEC, esimProcCmsDataUpReq_1, P_INFO, 1,
                    "esim_process_entry return :%d FAIL!!! ", send_ret);
        if(pcmsAtreq->ReqData)
        {
            free(pcmsAtreq->ReqData);
            pcmsAtreq->ReqData = NULL;
        }

        if(atRecvBuf)
        {
            free(atRecvBuf);
            atRecvBuf = NULL;
        }

        pcmsAtResp->errcode = ESIM_RET_RESULT_FAIL;
        OsaSendSignal(CMS_TASK_ID, &psignal);
        return;
    }
    #endif

    ECOMM_TRACE(UNILOG_SOFTSIM, esimProcCmsDataUpReq_3, P_INFO,1,"recv from process_entry,len:%d",atrecvLen);

    pcmsAtResp->errcode    = ESIM_RET_RESULT_OK;
    pcmsAtResp->type       = pcmsAtreq->type;
    pcmsAtResp->respLen    = atrecvLen;
    pcmsAtResp->respHandle = pcmsAtreq->reqhandle;
    pcmsAtResp->respData   = (void*)atRecvBuf;

    if(pcmsAtreq->ReqData)
    {
        free(pcmsAtreq->ReqData);
        pcmsAtreq->ReqData = NULL;
    }

    OsaSendSignal(CMS_TASK_ID, &psignal);

    return;

}

/**
  \fn          esimProcCmsIndMsgReq
  \brief       API: handle the IND MSG from CMS task
  \param[int]  cmsEvent : the cms send event
*/
static void esimProcCmsIndMsgReq(eSimCmsAtIndReqMsg  *cmsEvent)
{
    eSimCmsAtIndReqMsg  *pEvent = cmsEvent;

    OsaCheck(pEvent != PNULL, 0, 0,0);
    if(pEvent->event == ESIM_MSG_IND_FINISHED)
    {
        eSimProcSetWorkFlag(ESIM_TASK_STATE_FINISHED);
    }
    return;
}

/**
  \fn          eSimCreateProcTaskInQ
  \brief       create a INPUT queue for processing the AT+esim data to TA
*/
void eSimCreateProcTaskInQ(void)
{
    esimInQHandle  = osMessageQueueNew(ESIM_TASK_QUEUE_SIZE, sizeof(void *), NULL);
    OsaCheck(esimInQHandle  != PNULL, 0, 0,0);
}


/**
  \fn          eSimCreateProcTask
  \brief       create a task received the AT+esim data from cms TASK
*/

void eSimCreateProcTask(void)
{
    osThreadAttr_t task_attr;
    osThreadId_t   osDynTaskId = PNULL;

    memset(&task_attr, 0, sizeof(osThreadAttr_t));

    task_attr.name       = "eSimProcTask";
    task_attr.stack_mem  = PNULL;
    task_attr.stack_size = 5120;
    task_attr.priority   = osPriorityNormal;

    osDynTaskId = osThreadNew(eSimProcTaskEntry, NULL, &task_attr);

    EC_ASSERT(osDynTaskId, osDynTaskId, 0, 0);
    return;
}


/**
  \fn          eSimCreateIndSigMsg
  \brief       Create a IND msg from CMS task to esimproceTask that notify all profiles downloaded is finishe
  \param[int]  pSendSignal  the send signal buf
  \param[int]  event  the send indication event ID
  \param[out]  CmsRetId result
*/

static  void eSimCreateIndSigMsg(UINT8 event,SignalBuf **pSendSignal)
{
    eSimCmsAtIndReqMsg     *pEventMsg = NULL;
    SignalBuf              *pTempSignal  = NULL;

    OsaCheck(pSendSignal != PNULL  && *pSendSignal == NULL, 0, 0, 0);
    OsaCreateZeroSignal(SIG_ESIM_SIGNAL_RECV_IND, ( sizeof(eSimCmsAtIndReqMsg)), &pTempSignal);
    OsaCheck(pTempSignal != PNULL, SIG_ESIM_SIGNAL_RECV_IND, ( sizeof(eSimCmsAtIndReqMsg)), 0);

    pEventMsg         = (eSimCmsAtIndReqMsg *)(pTempSignal->sigBody);
    pEventMsg->event  =  event;

    *pSendSignal = pTempSignal;

    return;

}


/**
  \fn          eSimSendIndSigReq
  \brief       Create a IND msg from CMS task to esimproceTask that notify all profiles downloaded is finishe
  \param[int]  mq_id  the esimprocTask INQUEU handle ID
  \param[int]  event  the indication event ID
  \param[out]  CmsRetId result
*/

CmsRetId eSimSendIndSigReq(osMessageQueueId_t mq_id,UINT32 atHandle,UINT8 event)
{
    CmsRetId               rc           = CMS_FAIL;
    SignalBuf             *pSendSignal  = PNULL;
    osStatus_t             status;

    eSimCreateIndSigMsg(event,&pSendSignal);
    status = osMessageQueuePut(mq_id, (const void*)&pSendSignal, 0, 0);
    if(status == osErrorResource)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, eSimSendDataConfigReq_1, P_WARNING, 0, "task queue full");
        return  (rc);
    }

    if(status != osOK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, eSimSendDataConfigReq_2, P_WARNING, 1, "send task queue error %d ",status);
        return  (rc);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, eSimSendDataConfigReq_3, P_INFO, 0, " eSimSendAtDataReq SUCC");
    rc = CMS_RET_SUCC;

    return rc;
}


/**
  \fn          eSimSendAtDataReq
  \brief       Create a data REQ msg from CMS task to esimproc task
  \param[int]  atHandle  AT handle from at module
  \param[int]  type   the message id
  \param[int]  sendData  the send data
  \param[int]  sendData  the send data  len
  \param[out]  CmsRetId result
*/
CmsRetId eSimSendAtDataReq(UINT32 atHandle,UINT8 type, UINT8 *sendData, UINT16 sendLen)
{
    CmsRetId               rc           = CMS_FAIL;
    SignalBuf             *pSendSignal  = PNULL;
    osStatus_t             status;
    eSimCmsAtDataReqMsg   *pAtDataReqMsg     = NULL;

    OsaCreateZeroSignal(SIG_ESIM_SIGNAL_SEND_REQ, (sizeof(eSimCmsAtDataReqMsg)), &pSendSignal);

    pAtDataReqMsg            = (eSimCmsAtDataReqMsg *)(pSendSignal->sigBody);
    pAtDataReqMsg->type      =  type;
    pAtDataReqMsg->ReqLen    =  sendLen;
    pAtDataReqMsg->reqhandle =  atHandle;
    pAtDataReqMsg->ReqData   =  sendData;

    EC_ASSERT(pSendSignal != NULL && pSendSignal->sigId == SIG_ESIM_SIGNAL_SEND_REQ , pSendSignal, 0, 0);

    pAtDataReqMsg        = (eSimCmsAtDataReqMsg *)(pSendSignal->sigBody);

    status = osMessageQueuePut(esimInQHandle, (const void*)&pSendSignal, 0, 0);
    if(status == osErrorResource)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, eSimSendAtDataReq_0, P_WARNING, 0, "task queue full");
        return  (rc);
    }

    if(status != osOK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, eSimSendAtDataReq_1, P_WARNING, 1, "send task queue error %d ",status);
        return  (rc);
    }

    rc = CMS_RET_SUCC;

    return rc;
}



/**
  \fn          eSimProcInit
  \brief       init the task state
  \param[int]  state
*/
UINT8  eSimProcInit(void)
{
    memset(&geSimProcTaskMgtData,0x0, sizeof(eSimProcTaskMsg));

    geSimProcTaskMgtData.state   = ESIM_TASK_STATE_IDLE;
    geSimProcTaskMgtData.init    = 1;

    return ESIM_RET_RESULT_OK;
}

/**
  \fn          eSimProcSetWorkFlag
  \brief       set the task state flag
  \param[int]  state
*/
UINT8  eSimProcSetWorkFlag(ESIM_TASK_STATE          state)
{
    geSimProcTaskMgtData.state = state;
    return 0;
}

/**
  \fn          eSimProcGetWorkFlag
  \brief       get the task state flag
  \param[out]  state
*/
UINT8  eSimProcGetWorkFlag(void)
{
    UINT8 state = geSimProcTaskMgtData.state ;

    return state;
}

/**
  \fn          eSimTaskGetInstance
  \brief       get the global instance
  \param[in]   void
*/
eSimProcTaskMsg *eSimTaskGetInstance(void)
{
    return (&geSimProcTaskMgtData);
}

/**
  \fn          eSimProcTaskEntry
  \brief       esimProc task MainEntry
  \param[in]   void *arg
*/
void eSimProcTaskEntry(void *arg)
{
    osStatus_t           retStatus = osOK;
    SignalBuf           *psignal =NULL;
    eSimCmsAtDataReqMsg *pAtDataReqMsg = NULL;
    eSimProcTaskMsg     *pInstance = eSimTaskGetInstance();

    ECOMM_TRACE(UNILOG_SOFTSIM, eSimProcTaskEntry_0, P_INFO, 0, "eSimProcTaskEntry starting...");

    while (1)
    {
        retStatus = osMessageQueueGet(esimInQHandle, (void *)&psignal, PNULL, osWaitForever);
        OsaCheck(retStatus == osOK, 0 , 0, 0);
        eSimProcSetWorkFlag(ESIM_TASK_STATE_WORKING);
        switch (psignal->sigId)
        {
            case SIG_ESIM_SIGNAL_SEND_REQ:
            {
                pAtDataReqMsg = (eSimCmsAtDataReqMsg *)psignal->sigBody;

                esimProcCmsDataUpReq(pAtDataReqMsg);
                break;
            }
            case SIG_ESIM_SIGNAL_RECV_IND:
            {
                esimProcCmsIndMsgReq((eSimCmsAtIndReqMsg *)psignal->sigBody);

                break;
            }
            default:
                break;
        }

        if (psignal != PNULL)
        {
            OsaDestroySignal(&psignal);
        }

        if (ESIM_TASK_STATE_FINISHED == eSimProcGetWorkFlag())
        {
            break;
        }

        eSimProcSetWorkFlag(ESIM_TASK_STATE_IDLE);

    }

    //delete the esimProceTask queue
    osMessageQueueDelete(esimInQHandle);
    esimInQHandle = PNULL;

    ECOMM_TRACE(UNILOG_SOFTSIM, eSimProcTaskEntry_4, P_WARNING, 0, "All finished, EXIT!!!");
    osThreadExit();

}

#endif
