/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616 China Telecomm NB-IOT demo app source file
 *               this demo is suitable for scenarios that require low power consumption 
 *               and need to send data to the server periodically(every day or several hours),
 *               or use key to send data to the server
 *               modify the variable:sendDataItv to set the period of sending data
 *               In this demo wakeuppad3 is the key to wakeup system and sending data.
 *               If want change the key please contact customer support.
 * History:      Rev1.0   2020-03-24
 *
 ****************************************************************************/
#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "queue.h"
#include "app.h"
#include "ct_api.h"
#include "ec_ctlwm2m_api.h"
#include "cms_api.h"
#include "debug_log.h"
#include "plat_config.h"
#include "gps_demo.h"


// app task static stack and control block
#define INIT_TASK_STACK_SIZE    (1024)
static StaticTask_t             initTask;
static UINT8                    appTaskStack[INIT_TASK_STACK_SIZE];
UINT32 sendDataItv = 1200;
UINT8 seqNumb = 0;
UINT8 retrySend = 0;

QueueHandle_t app_cmd_msg_handle = NULL;

extern QueueHandle_t ct_state_msg_handle;

static void appTask(void *arg)
{
    CT_STATUS_Q_MSG ctMsg;
    int msg_type = 0xff;
    APP_CMD_Q_MSG cmdMsg;
    memset(&cmdMsg, 0, sizeof(cmdMsg));
    while(1)
    {
        /* recv msg (block mode) */
        xQueueReceive(ct_state_msg_handle, &ctMsg, osWaitForever);
        msg_type = ctMsg.status_type;
        
        switch(msg_type)
        {
            case APP_READY_TO_SEND_PACKET:
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_1, P_INFO, 0, "can send packet");
                 //SENDMODE_CON_REL is necessay for low power consumption, seqNumb can be ignore
                 //SENDMODE_CON is mandatory if DL data immediately after UL data
                 ctiot_funcv1_send(NULL, "0356323334", SENDMODE_CON, NULL, seqNumb);
                 break;
            case APP_SEND_PACKET_DONE://recive send packet's ACK
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_2, P_INFO, 0, "packet arrived, enable sleep");
                 ctiot_funcv1_enable_sleepmode();
                 break;
            case APP_SEND_PACKET_FAILED://exceeded the maximum number of retries send packet hasn'g get ACK 
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_3, P_ERROR, 0, "packet send failed, retry it, if retry failed re-register oc");
                 if(retrySend == 0){
                     ctiot_funcv1_send(NULL, "032E323334", SENDMODE_CON_REL, NULL, seqNumb);//retry send one times
                     retrySend = 1;
                 }else{
                    cmdMsg.cmd_type = APP_CMD_DEREG_REG;//still send fail to deregister then register again
                    xQueueSend(app_cmd_msg_handle, &cmdMsg, CT_MSG_TIMEOUT);
                 }
                 break;
            case APP_UPDATE_FAILED://update failed register again
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_4, P_ERROR, 0, "packet send failed, re-register");
                 osDelay(500/portTICK_PERIOD_MS);
                 cmdMsg.cmd_type = APP_CMD_REG_AGAIN;
                 xQueueSend(app_cmd_msg_handle, &cmdMsg, CT_MSG_TIMEOUT);
                 break;
            case APP_OC_OBSV_FAILED://server cancel observe register again
                ECOMM_TRACE(UNILOG_PLA_APP, appTask_5, P_ERROR, 0, "server cancel observe, try re-register");
                cmdMsg.cmd_type = APP_CMD_DEREG_REG;
                xQueueSend(app_cmd_msg_handle, &cmdMsg, CT_MSG_TIMEOUT);
                break;
            case APP_OC_REG_FAILED://register failed retry
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_6, P_ERROR, 0, "oc reg failed, try re-register");
                 cmdMsg.cmd_type = APP_CMD_TO_IDLE;//APP_CMD_REG_AGAIN;
                 xQueueSend(app_cmd_msg_handle, &cmdMsg, CT_MSG_TIMEOUT);
                 break;
            case APP_RECV_RST_CMD://receive the RST commond from server
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_7, P_ERROR, 0, "recv rst command, do sth");
                 cmdMsg.cmd_type = APP_CMD_DEREG_REG;
                 xQueueSend(app_cmd_msg_handle, &cmdMsg, CT_MSG_TIMEOUT);
                 break;
            case APP_NETWORK_DISC://lost network connect
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_8, P_ERROR, 0, "network disconnect, try restore it");
                 cmdMsg.cmd_type = APP_CMD_REINIT_NETWORK;
                 xQueueSend(app_cmd_msg_handle, &cmdMsg, CT_MSG_TIMEOUT);
                 break;
            case APP_PARAM_ERROR:   
            case APP_INTER_ERROR:   //code error 
                 ECOMM_TRACE(UNILOG_PLA_APP, appTask_9, P_ERROR, 0, "oc parameter error or internal error check the code");
                 ctiot_funcv1_enable_sleepmode();
                 break;
			case APP_GPS_RECEIVED:   //GPS
                 //ECOMM_TRACE(UNILOG_PLA_APP, appTask_10, P_ERROR, 0, "APP_GPS_RECEIVED");
                 printf("APP_GPS_RECEIVED\r\n");
                 break;
        }
    }
}

void appInit(void *arg)
{
    if(app_cmd_msg_handle == NULL)
    {
        app_cmd_msg_handle = xQueueCreate(10, sizeof(APP_CMD_Q_MSG));
    }
    else
    {
        xQueueReset(app_cmd_msg_handle);
    }

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

    osThreadNew(appTask, NULL, &task_attr);
    
    ctApiInit();//start nb-iot client for china telecomm
    gpsApiInit();
}

extern ARM_DRIVER_USART Driver_USART0;
extern ARM_DRIVER_USART Driver_USART1;

/*
 *  set printf uart port
 *  Parameter:      port: for printf
 */
void SetPrintUart(usart_port_t port)
{
    if(port == PORT_USART_0)
    {
#if (RTE_UART0)
        UsartPrintHandle = &CREATE_SYMBOL(Driver_USART, 0);
        GPR_ClockDisable(GPR_UART0FuncClk);
        GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);
        GPR_ClockEnable(GPR_UART0FuncClk);
        GPR_SWReset(GPR_ResetUART0Func);
#endif
    }
    else if(port == PORT_USART_1)
    {
#if (RTE_UART1)
        UsartPrintHandle = &CREATE_SYMBOL(Driver_USART, 1);
        GPR_ClockDisable(GPR_UART1FuncClk);
        GPR_SetClockSrc(GPR_UART1FuncClk, GPR_UART1ClkSel_26M);
        GPR_ClockEnable(GPR_UART1FuncClk);
        GPR_SWReset(GPR_ResetUART1Func);
#endif
    }

    if(UsartPrintHandle == NULL)
        return;

    UsartPrintHandle->Initialize(NULL);
    UsartPrintHandle->PowerControl(ARM_POWER_FULL);
    UsartPrintHandle->Control(ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 |
                        ARM_USART_PARITY_NONE       | ARM_USART_STOP_BITS_1 |
                        ARM_USART_FLOW_CONTROL_NONE, 115200ul);
}


/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void) {

    BSP_CommonInit();
	SetPrintUart(PORT_USART_1);   //ѡ�񴮿ڴ�ӡ
    osKernelInitialize();
    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);
}

