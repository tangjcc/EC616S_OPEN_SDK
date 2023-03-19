/******************************************************************************
*  Filename: at_ctlwm2m_task.c
*
*  Description: Process ctlwm2m related AT request commands 
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "cmsis_os2.h"
#include "osasys.h"
#include "queue.h"
#include "at_ctlwm2m_task.h"
#include "atec_ctlwm2m_cnf_ind.h"
#include "ec_ctlwm2m_api.h"
#include "debug_log.h"
#include "ct_liblwm2m.h"
#include "ctiot_lwm2m_sdk.h"

QueueHandle_t ctlwm2m_atcmd_handle = NULL;
osThreadId_t ctlwm2m_atcmd_task_handle = NULL;
int ctlwm2m_atcmd_task_flag = CTLWM2M_TASK_STAT_NONE;
osMutexId_t athdlTaskMutex = NULL;
extern uint16_t gRunningSendThread;


CmsRetId ctlwm2m_client_update(UINT32 reqHandle)
{
    CTLWM2M_ATCMD_Q_MSG ctMsg;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_client_update_1, P_INFO, 0, "TO SEND AT+CTM2MUPDATE");

    memset(&ctMsg, 0, sizeof(ctMsg));
    ctMsg.cmd_type = MSG_CTLWM2M_UPDATE;
    ctMsg.reqhandle = reqHandle;
    
    xQueueSend(ctlwm2m_atcmd_handle, &ctMsg, CTLWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId ctlwm2m_client_send(UINT32 reqHandle, char* data, UINT8 mode, UINT8 seqNum)
{
    CTLWM2M_ATCMD_Q_MSG ctMsg;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_client_send_1, P_INFO, 0, "TO SEND AT+CTM2MSEND");

    memset(&ctMsg, 0, sizeof(ctMsg));
    ctMsg.cmd_type = MSG_CTLWM2M_SEND;
    ctMsg.reqhandle = reqHandle;
    ctMsg.send_data.mode = mode;
    ctMsg.send_data.seqNum = seqNum;
    ctMsg.send_data.data = data;
    
    xQueueSend(ctlwm2m_atcmd_handle, &ctMsg, CTLWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId ctlwm2m_client_dereg(UINT32 reqHandle)
{
    CTLWM2M_ATCMD_Q_MSG ctMsg;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_client_dereg_1, P_INFO, 0, "TO SEND AT+CTM2MDEREG");

    memset(&ctMsg, 0, sizeof(ctMsg));
    ctMsg.cmd_type = MSG_CTLWM2M_DEREG_CLIENT;
    ctMsg.reqhandle = reqHandle;
    
    xQueueSend(ctlwm2m_atcmd_handle, &ctMsg, CTLWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

static void ctlwm2m_atcmd_processing(void *argument)
{
    int keepTask = 1;
    UINT16 ret = CTLWM2M_OK;
    int msg_type = 0xff;
    CTLWM2M_ATCMD_Q_MSG ctlwm2mMsg;
    int primSize;
    UINT16 atHandle;
    BOOL hasThread = FALSE;
    ctlwm2m_cnf_msg ctCnfMsg;
    primSize = sizeof(ctCnfMsg);
    memset(&ctCnfMsg, 0, primSize);

    ctiot_funcv1_context_t* pContext;
    pContext=ctiot_funcv1_get_context();

    while(1)
    {
        UINT8 count = 0;
        /* recv msg (block mode) */
        xQueueReceive(ctlwm2m_atcmd_handle, &ctlwm2mMsg, osWaitForever);
        msg_type = ctlwm2mMsg.cmd_type;
        atHandle = ctlwm2mMsg.reqhandle;
        ctCnfMsg.atHandle = atHandle;

        hasThread = FALSE;
        ctiot_funcv1_disable_sleepmode();
        while(gRunningSendThread != 1 && count < 40){//wait up to 40s
            ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_atcmd_process_0, P_INFO, 1, "wait send thread start count:%d", count);
            osDelay(1000);//wait 1000ms
            count += 1;
        }
        if(count < 40){
            hasThread = TRUE;
        }
        ctiot_funcv1_enable_sleepmode();

        switch(msg_type)
        {
            case MSG_CTLWM2M_DEREG_CLIENT:
                ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_atcmd_process_2, P_INFO, 2, "TO HANDLE AT+CTM2MDEREG logstatus(%d)",pContext->loginStatus);
                if(!hasThread){
                    ctCnfMsg.ret = CTIOT_EA_OPERATION_FAILED_NOSESSION;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_CTLWM2M, APPL_CT_DEREG_CNF, primSize, &ctCnfMsg);
                    break;
                }
                ret = ctiot_at_dereg(pContext);
                if(ret != CTLWM2M_OK){
                    ctCnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_CTLWM2M, APPL_CT_DEREG_CNF, primSize, &ctCnfMsg);
                }else{
                    keepTask = 0;
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_CTLWM2M, APPL_CT_DEREG_CNF, primSize, &ctCnfMsg);
                }
                break;
            
            case MSG_CTLWM2M_SEND:
                /* send packet */
                ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_atcmd_process_3, P_INFO, 2, "TO HANDLE AT+CTM2MSEND logstatus(%d)",pContext->loginStatus);
                if(!hasThread){
                    ctCnfMsg.ret = CTIOT_EA_OPERATION_FAILED_NOSESSION;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_CTLWM2M, APPL_CT_SEND_CNF, primSize, &ctCnfMsg);
                    break;
                }
                ret = ctiot_at_send(pContext,ctlwm2mMsg.send_data.data, (ctiot_funcv1_send_mode_e)ctlwm2mMsg.send_data.mode, ctlwm2mMsg.send_data.seqNum);
                if(ret != CTLWM2M_OK){
                    ctCnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_CTLWM2M, APPL_CT_SEND_CNF, primSize, &ctCnfMsg);
                }else{
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_CTLWM2M, APPL_CT_SEND_CNF, primSize, &ctCnfMsg);
                }
                ct_lwm2m_free(ctlwm2mMsg.send_data.data);
                break;
         
            case MSG_CTLWM2M_UPDATE:
                /* send update packet */
                ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_atcmd_process_4, P_INFO, 1, "TO HANDLE AT+CTM2MUPDATE logstatus(%d)",pContext->loginStatus);
                if(!hasThread){
                    ctCnfMsg.ret = CTIOT_EA_OPERATION_FAILED_NOSESSION;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_CTLWM2M, APPL_CT_UPDATE_CNF, primSize, &ctCnfMsg);
                    break;
                }
                UINT16 msgId = 0;
                ret = ctiot_at_update_reg(NULL, &msgId, false);
                if(ret != CTLWM2M_OK){
                    ctCnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_CTLWM2M, APPL_CT_UPDATE_CNF, primSize, &ctCnfMsg);
                }else{
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_CTLWM2M, APPL_CT_UPDATE_CNF, primSize, &ctCnfMsg);
                }
                break;
        }

        if(keepTask == 0)
        {
            break;
        }
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_atcmd_process_5, P_INFO, 0, "ct deregister delete the atcmd process");
    ctlwm2m_atcmd_task_flag = CTLWM2M_TASK_STAT_NONE;
    ctlwm2m_atcmd_task_handle = NULL;
    osThreadExit();
}

static int ctlwm2m_handleAT_task_Init(void)
{
    int ret = CTLWM2M_OK;

    if(ctlwm2m_atcmd_handle == NULL)
    {
        ctlwm2m_atcmd_handle = xQueueCreate(24, sizeof(CTLWM2M_ATCMD_Q_MSG));
    }
    else
    {
        xQueueReset(ctlwm2m_atcmd_handle);
    }

    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "ctAThdl";
    task_attr.stack_size = CTLWM2M_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal6;

    ctlwm2m_atcmd_task_handle = osThreadNew(ctlwm2m_atcmd_processing, NULL,&task_attr);
    if(ctlwm2m_atcmd_task_handle == NULL)
    {
        ret = CTLWM2M_TASK_ERR;
    }

    return ret;
}

int ctlwm2m_client_create(void)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_client_create_1, P_INFO, 0, "to start ctlwm2m handleAT task");

    osStatus_t result = osMutexAcquire(athdlTaskMutex, osWaitForever);
    
    if(ctlwm2m_atcmd_task_flag == CTLWM2M_TASK_STAT_NONE)
    {
        ctlwm2m_atcmd_task_flag = CTLWM2M_TASK_STAT_CREATE;
        if(ctlwm2m_handleAT_task_Init() != CTLWM2M_OK)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_client_create_0, P_INFO, 0, "ctlwm2m handleAT task create failed");
            return CTLWM2M_TASK_ERR;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_client_create_2, P_INFO, 0, "ctlwm2m handleAT task already start");
    }
    osMutexRelease(athdlTaskMutex);

    return CTLWM2M_OK;
}

