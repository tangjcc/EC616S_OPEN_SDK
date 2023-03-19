/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616S onenet demo app source file
 * History:      Rev1.0   2018-10-12
 *
 ****************************************************************************/
#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "queue.h"
#include "app.h"
#include "debug_log.h"
#include "slpman_ec616s.h"
#include "plat_config.h"

// app task static stack and control block
#define INIT_TASK_STACK_SIZE    (1024)
#define RINGBUF_READY_FLAG      (0x06)
#define APP_EVENT_QUEUE_SIZE    (10)
#define MAX_PACKET_SIZE         (256)

static StaticTask_t             initTask;
static UINT8                    appTaskStack[INIT_TASK_STACK_SIZE];
static volatile UINT32          Event;
static appRunningState_t        stateMachine = APP_INIT_STATE;
static UINT8                    notifyTimes  = 0;  
extern uint8_t onenetASlpHandler;

#if 0
static void sendQueueMsg(UINT32 msgId, UINT32 xTickstoWait)
{
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle)
    {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, sendQueueMsg_0, P_ERROR, 0, "xQueueSend error");
        }
    }
}
#endif



static void onenetAppTask(void *arg)
{
    onenetFotaState_t fota = ONENET_FOTA_NO;
    while (1)
    {
        switch(stateMachine)
        {
            case APP_INIT_STATE:
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_1, P_INFO, 0, "INIT STATE disable sleep go REPORT STATE");
                    slpManPlatVoteDisableSleep(onenetASlpHandler, SLP_SLP2_STATE);
                    stateMachine = APP_REPORT_STATE;
                    osDelay(5000/portTICK_PERIOD_MS);
                    break;
                }
            case APP_REPORT_STATE:
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_2, P_INFO, 0, "check cis if ready");
                    if (cisAsyncCheckNotificationReady())
                    {
                        fota = cisAsynGetFotaStatus();
                        ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_3, P_INFO, 1, "cis state ready and observe ready,fota=%d",fota);
                        if (fota != ONENET_FOTA_NO)
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_4, P_INFO, 0, "in fota go idle");
                            stateMachine = APP_IDLE_STATE;
                            break;
                        }
                        else if(!cisAsynGetNotifyStatus())
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_5, P_INFO, 0, "idling to notify");
                            if(notifyTimes < 2){
                                notifyTimes += 1;
                                prvMakeUserdata();
                                cisDataObserveReport();
                            }else{
                                ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_6, P_INFO, 0, "has notify 2 times go to idle");
                                stateMachine = APP_IDLE_STATE;
                            }
                        }
                    }
                    osDelay(10000/portTICK_PERIOD_MS);
                    break;
                }
             case  APP_IDLE_STATE:
                {
                    slpManPlatVoteEnableSleep(onenetASlpHandler, SLP_SLP2_STATE);
                    ECOMM_TRACE(UNILOG_PLA_APP, onenetAppTask_7, P_INFO, 0, "app enter idle state");
                    osDelay(5000/portTICK_PERIOD_MS);
                    break;
                }

        }

    }

}

void appInit(void *arg)
{
    osThreadAttr_t task_attr;
    
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_CONTROL) != 0)
    {
        HAL_UART_RecvFlowControl(false);
    }
    memset(&task_attr,0,sizeof(task_attr));
    memset(appTaskStack, 0xA5, INIT_TASK_STACK_SIZE);
    task_attr.name = "app";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(onenetAppTask, NULL, &task_attr);
    
    slpManSlpState_t slpState = slpManGetLastSlpState();
    if(slpState == SLP_HIB_STATE || slpState == SLP_SLP2_STATE)//wakeup from deep sleep
    {
        stateMachine = APP_IDLE_STATE;
    }
    cisOnenetInit();

}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void) {

    BSP_CommonInit();
    osKernelInitialize();
    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);
}

