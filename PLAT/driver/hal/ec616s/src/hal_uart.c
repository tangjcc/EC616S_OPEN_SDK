/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    hal_uart.c
 * Description:  EC616 uart hal driver source file
 * History:      Rev1.0   2020-03-24
 *
 ****************************************************************************/

#include <string.h>
#include "hal_uart.h"
#include "ostask.h"
#include "osasys.h"
#include "debug_log.h"

#define HAL_UART_MAX_NUM        (3)

#define HAL_UART_RECV_TASK_QUEUE_CREATED            (0x1)
#define HAL_UART_SEND_TASK_QUEUE_CREATED            (0x2)

typedef struct _hal_uart_handler
{
    ARM_DRIVER_USART*   uartDriverHandler;
    uint8_t*            recvBuffPtr;
    uint32_t            recvBuffSize;
    hal_uart_recv_cb_t  recv_cb;
    hal_uart_pre_send_cb_t pre_send_cb;
    hal_uart_post_send_cb_t post_send_cb;
} hal_uart_handler_t;

static hal_uart_handler_t g_halUartHandler[HAL_UART_MAX_NUM] = {0};
static uint32_t g_halUartInitFlag = 0;

//-----------------------------------------------------------------//
// ------------------------ UART RECV PART ------------------------//
//-----------------------------------------------------------------//

// number of UART recv message queue objects
#define RECV_MSGQUEUE_OBJECTS   (8)

// UART recv message queue element typedef
typedef struct
{
    uint32_t uartIndex;
    uint32_t event;
    uint32_t value;
} recv_msgqueue_obj_t;

// message queue id
static osMessageQueueId_t mid_uart_recv_msgqueue;

// UART recv task static stack and control block
#define UART_RECV_TASK_STACK_SIZE             (1536)

#define UART_RECV_FLOW_CONTROL_FLAG           (0x1)

static StaticTask_t             uart_recv_task;
static uint8_t                  uart_recv_task_stack[UART_RECV_TASK_STACK_SIZE];
static StaticQueue_t            uart_recv_queue_cb;
static uint8_t                  uart_recv_queue_buf[RECV_MSGQUEUE_OBJECTS * sizeof(recv_msgqueue_obj_t)];
static osEventFlagsId_t         g_uartRecvFlowControlFlag;
extern osEventFlagsId_t         g_uartSendCompleteFlag;

void HAL_UART0_RecvCallBack(uint32_t event);
void HAL_UART1_RecvCallBack(uint32_t event);
void HAL_UART2_RecvCallBack(uint32_t event);

void HAL_UART_CreateRecvTaskAndQueue(void);
void HAL_UART_CreateRecvTaskAndQueue(void);


typedef void(*ARM_Driver_RecvCallBack)(uint32_t event);

static const ARM_Driver_RecvCallBack g_halUartRecvCallbacks[HAL_UART_MAX_NUM] = {HAL_UART0_RecvCallBack, HAL_UART1_RecvCallBack, HAL_UART2_RecvCallBack};

RAM_CODE2_SECTION static void HAL_UART_CommonRecvCallBack(uint32_t uartIndx, uint32_t event)
{
    recv_msgqueue_obj_t msg = {uartIndx};
    osStatus_t status;

    if(event & (ARM_USART_EVENT_RX_TIMEOUT | ARM_USART_EVENT_RECEIVE_COMPLETE | ARM_USART_EVENT_RX_OVERFLOW))
    {
        msg.event = event;
        msg.value = g_halUartHandler[uartIndx].uartDriverHandler->GetRxCount();

        if(msg.value >= g_halUartHandler[uartIndx].recvBuffSize)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, HAL_UART_CommonRecvCallBack_1, P_WARNING, 1, "UART%d recv reaches max len", uartIndx);
        }

        status = osMessageQueuePut(mid_uart_recv_msgqueue, &msg, 0, 0);

        if(status == osErrorResource)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, HAL_UART_CommonRecvCallBack_2, P_WARNING, 1, "UART%d recv queue full", uartIndx);
        }
    }

    if(event & (ARM_USART_EVENT_RX_OVERFLOW | ARM_USART_EVENT_RX_FRAMING_ERROR
             | ARM_USART_EVENT_RX_PARITY_ERROR | ARM_USART_EVENT_RX_BREAK))
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, HAL_UART_CommonRecvCallBack_3, P_DEBUG, 2, "UART%d recv err evt=0x%x", uartIndx, event);
        g_halUartHandler[uartIndx].uartDriverHandler->Control(ARM_USART_CONTROL_PURGE_COMM, 0);
    }

    if(event == ARM_USART_EVENT_AUTO_BAUDRATE_DONE)
    {
        msg.event = event;
        msg.value = g_halUartHandler[uartIndx].uartDriverHandler->GetBaudRate();

        status = osMessageQueuePut(mid_uart_recv_msgqueue, &msg, 0, 0);

        if(status == osErrorResource)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, HAL_UART_CommonRecvCallBack_2, P_WARNING, 1, "UART%d recv queue full", uartIndx);
        }
    }

    if(event & ARM_USART_EVENT_SEND_COMPLETE)
    {
        osEventFlagsSet(g_uartSendCompleteFlag, 1 << uartIndx);
    }

}

/**
\fn          RAM_CODE2_SECTION void HAL_UART0_RecvCallBack(uint32_t event)
\brief
\return
*/
RAM_CODE2_SECTION static void HAL_UART0_RecvCallBack(uint32_t event)
{
    HAL_UART_CommonRecvCallBack(0, event);
}


/**
\fn          RAM_CODE2_SECTION void HAL_UART1_RecvCallBack(uint32_t event)
\brief
\return
*/
RAM_CODE2_SECTION static void HAL_UART1_RecvCallBack(uint32_t event)
{
    HAL_UART_CommonRecvCallBack(1, event);
}


/**
\fn          RAM_CODE2_SECTION void HAL_UART2_RecvCallBack(uint32_t event)
\brief
\return
*/
RAM_CODE2_SECTION static void HAL_UART2_RecvCallBack(uint32_t event)
{
    HAL_UART_CommonRecvCallBack(2, event);
}

/**
\fn          static void HAL_UART_RecvTaskEntry(void *arg)
\brief
\return
*/
static void HAL_UART_RecvTaskEntry(void *arg)
{
    uint32_t data_block_len, uart_recv_current_cnt, uartIndex;
    uint32_t uart_last_cnt[HAL_UART_MAX_NUM] = {0};

    while(1)
    {
        recv_msgqueue_obj_t msg;
        osStatus_t status;
        ARM_USART_STATUS uart_status;
        uint32_t mask, flags;

        flags = osEventFlagsWait(g_uartRecvFlowControlFlag, UART_RECV_FLOW_CONTROL_FLAG, osFlagsNoClear | osFlagsWaitAll, osWaitForever);

        EC_ASSERT(flags == UART_RECV_FLOW_CONTROL_FLAG, flags, 0, 0);

        // wait for message
        status = osMessageQueueGet(mid_uart_recv_msgqueue, &msg, 0, osWaitForever);

        if(status == osOK)
        {
            uartIndex = msg.uartIndex;

            if(uartIndex < 3)
            {
                if(msg.event & (ARM_USART_EVENT_RX_TIMEOUT | ARM_USART_EVENT_RECEIVE_COMPLETE | ARM_USART_EVENT_RX_OVERFLOW))
                {
                    data_block_len = msg.value - uart_last_cnt[uartIndex];

                    g_halUartHandler[uartIndex].recv_cb(msg.event, (void*)(g_halUartHandler[uartIndex].recvBuffPtr + uart_last_cnt[uartIndex]), data_block_len);

                    uart_last_cnt[uartIndex] = msg.value;

                    mask = SaveAndSetIRQMask();

                    uart_status = g_halUartHandler[uartIndex].uartDriverHandler->GetStatus();
                    uart_recv_current_cnt = g_halUartHandler[uartIndex].uartDriverHandler->GetRxCount();

                    // Trigger next round receive action when rx is free and all received data are consumed
                    if((uart_status.rx_busy != 1) && (uart_last_cnt[uartIndex] == uart_recv_current_cnt))
                    {
                        g_halUartHandler[uartIndex].uartDriverHandler->Receive(g_halUartHandler[uartIndex].recvBuffPtr, g_halUartHandler[uartIndex].recvBuffSize);
                        uart_last_cnt[uartIndex] = 0;
                    }
                    RestoreIRQMask(mask);
                }
                else if(msg.event == ARM_USART_EVENT_AUTO_BAUDRATE_DONE)
                {
                    g_halUartHandler[uartIndex].recv_cb(msg.event, (void*)&(msg.value), sizeof(msg.value));
                }
            }

        }
    }
}

static void HAL_UART_CreateRecvTaskAndQueue(void)
{
    if(g_halUartInitFlag & HAL_UART_RECV_TASK_QUEUE_CREATED)
    {
        return;
    }

    osThreadId_t threadId;
    osThreadAttr_t task_attr;
    osMessageQueueAttr_t queue_attr;

    g_uartRecvFlowControlFlag = osEventFlagsNew(NULL);

    EC_ASSERT(g_uartRecvFlowControlFlag, g_uartRecvFlowControlFlag, 0, 0);

    memset(&queue_attr, 0, sizeof(queue_attr));

    queue_attr.mq_size = sizeof(uart_recv_queue_buf);
    queue_attr.mq_mem = uart_recv_queue_buf;
    queue_attr.cb_mem = &uart_recv_queue_cb;
    queue_attr.cb_size = sizeof(uart_recv_queue_cb);

    // init message queue
    mid_uart_recv_msgqueue = osMessageQueueNew(RECV_MSGQUEUE_OBJECTS, sizeof(recv_msgqueue_obj_t), &queue_attr);

    EC_ASSERT(mid_uart_recv_msgqueue, mid_uart_recv_msgqueue, 0, 0);

    memset(&task_attr,0,sizeof(task_attr));
    memset(uart_recv_task_stack, 0xA5, UART_RECV_TASK_STACK_SIZE);
    task_attr.name = "uartrecv";
    task_attr.stack_mem = uart_recv_task_stack;
    task_attr.stack_size = UART_RECV_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &uart_recv_task;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    threadId = osThreadNew(HAL_UART_RecvTaskEntry, NULL, &task_attr);

    EC_ASSERT(threadId, threadId, 0, 0);

    g_halUartInitFlag |= HAL_UART_RECV_TASK_QUEUE_CREATED;

}

//-----------------------------------------------------------------//
// -------------------------UART SEND PART ------------------------//
//-----------------------------------------------------------------//

// number of UART send message queue objects
#define SEND_MSGQUEUE_OBJECTS   (32)

// UART send message queue element typedef
typedef struct
{
    uint16_t msgType;
    uint16_t msgLen;
    uint8_t * msgPtr;
} send_msgqueue_obj_t;

// message queue id
static osMessageQueueId_t mid_uart_send_msgqueue;

// UART send task static stack and control block
#define UART_SEND_TASK_STACK_SIZE             (1536)

static StaticTask_t             uart_send_task;
static uint8_t                  uart_send_task_stack[UART_SEND_TASK_STACK_SIZE];
static StaticQueue_t            uart_send_queue_cb;
static uint8_t                  uart_send_queue_buf[SEND_MSGQUEUE_OBJECTS * sizeof(send_msgqueue_obj_t)];
static osEventFlagsId_t         g_uartSendCompleteFlag;
static uint32_t                 g_uartIsSendBlock;

void HAL_UART_SendMsg(uint16_t type, uint8_t uartIdx, uint8_t* msgPtr, uint16_t msgLen)
{
    uint8_t*   pHeapStr = NULL;
    send_msgqueue_obj_t sendMsg;
    osStatus_t status;

    OsaDebugBegin(msgLen < 4096, msgPtr, msgLen, 0);
    return;
    OsaDebugEnd();

    if(msgLen > 0)
    {
        pHeapStr = OsaAllocMemory(msgLen);
        OsaDebugBegin(pHeapStr != PNULL, pHeapStr, msgLen, 0);
        return;
        OsaDebugEnd();
        memcpy(pHeapStr, msgPtr, msgLen);
    }

    sendMsg.msgPtr = pHeapStr;
    sendMsg.msgType = ((type << HAL_UART_SEND_MSG_TYPE_POS) & HAL_UART_SEND_MSG_TYPE_MASK) | (uartIdx & HAL_UART_SEND_MSG_UART_IDX_MASK);
    sendMsg.msgLen = msgLen;

    // drop on message queue full
    status = osMessageQueuePut(mid_uart_send_msgqueue, &sendMsg, 0, 0);

    if(status == osErrorResource)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, HAL_UART_SendMsg_0, P_WARNING, 0, "UART send queue full");

        if(pHeapStr)
        {
            free(pHeapStr);
        }
    }
}

void HAL_UART_SendStr(uint8_t uartIdx, uint8_t* strPtr, uint16_t strLen)
{
    HAL_UART_SendMsg(HAL_UART_SEND_MSG_TYPE_STR, uartIdx, strPtr, strLen);
}

void HAL_UART_DirectSendStr(uint8_t uartIdx, uint8_t* strPtr, uint16_t strLen)
{
    if(uartIdx < HAL_UART_MAX_NUM)
    {
        g_halUartHandler[uartIdx].uartDriverHandler->SendPolling(strPtr, strLen);
    }
}

/**
  \fn          static void HAL_UART_SendTaskEntry(void *arg)
  \brief
  \return
 */
static void HAL_UART_SendTaskEntry(void *arg)
{
    while(1)
    {
        send_msgqueue_obj_t msg;
        hal_uart_send_msg_type_t msgType;
        uint8_t uartIndex = 0;
        osStatus_t status;

        // wait for message
        status = osMessageQueueGet(mid_uart_send_msgqueue, &msg, 0, osWaitForever);

#if(WDT_FEATURE_ENABLE==1)
        extern void WDT_Kick(void);
        WDT_Kick();
#endif

        if(status == osOK)
        {
            msgType = (hal_uart_send_msg_type_t)((msg.msgType & HAL_UART_SEND_MSG_TYPE_MASK) >> HAL_UART_SEND_MSG_TYPE_POS);
            uartIndex = ((msg.msgType & HAL_UART_SEND_MSG_UART_IDX_MASK) >> HAL_UART_SEND_MSG_UART_IDX_POS);

            EC_ASSERT(uartIndex < HAL_UART_MAX_NUM, uartIndex, 0, 0);

            switch(msgType)
            {
                case HAL_UART_SEND_MSG_TYPE_CNF:

                    if(msg.msgPtr)
                    {
                        if(g_halUartHandler[uartIndex].pre_send_cb)
                        {
                            g_halUartHandler[uartIndex].pre_send_cb(HAL_UART_SEND_MSG_TYPE_CNF, msg.msgPtr, msg.msgLen);
                        }

                        g_halUartHandler[uartIndex].uartDriverHandler->Send(msg.msgPtr, msg.msgLen);

                        if((g_uartIsSendBlock & (1 << uartIndex)) == 0)
                        {
                            osEventFlagsWait(g_uartSendCompleteFlag, 1 << uartIndex, osFlagsWaitAll, osWaitForever);
                        }

                        if(g_halUartHandler[uartIndex].post_send_cb)
                        {
                            g_halUartHandler[uartIndex].post_send_cb(HAL_UART_SEND_MSG_TYPE_CNF, msg.msgPtr, msg.msgLen);
                        }
                    }
                    break;

                case HAL_UART_SEND_MSG_TYPE_URC:

                    if(msg.msgPtr)
                    {
                        if(g_halUartHandler[uartIndex].pre_send_cb)
                        {
                            g_halUartHandler[uartIndex].pre_send_cb(HAL_UART_SEND_MSG_TYPE_URC, msg.msgPtr, msg.msgLen);
                        }

                        g_halUartHandler[uartIndex].uartDriverHandler->Send(msg.msgPtr, msg.msgLen);

                        if((g_uartIsSendBlock & (1 << uartIndex)) == 0)
                        {
                            osEventFlagsWait(g_uartSendCompleteFlag, 1 << uartIndex, osFlagsWaitAll, osWaitForever);
                        }

                        if(g_halUartHandler[uartIndex].post_send_cb)
                        {
                            g_halUartHandler[uartIndex].post_send_cb(HAL_UART_SEND_MSG_TYPE_URC, msg.msgPtr, msg.msgLen);
                        }
                    }
                    break;

                case HAL_UART_SEND_MSG_TYPE_STR:

                    if(msg.msgPtr)
                    {
                        g_halUartHandler[uartIndex].uartDriverHandler->Send(msg.msgPtr, msg.msgLen);
                        if((g_uartIsSendBlock & (1 << uartIndex)) == 0)
                        {
                            osEventFlagsWait(g_uartSendCompleteFlag, 1 << uartIndex, osFlagsWaitAll, osWaitForever);
                        }
                    }
                    break;

                case HAL_UART_ACT_CHANGE_CONFIG:

                    if(g_halUartHandler[uartIndex].post_send_cb)
                    {
                        if((g_uartIsSendBlock & (1 << uartIndex)) == 0)
                        {
                            //delay for a while to output all characters in uart fifo
                            delay_us(20000);
                        }
                        g_halUartHandler[uartIndex].post_send_cb(msgType, msg.msgPtr, msg.msgLen);
                    }

                    if(osMessageQueueGetCount(mid_uart_send_msgqueue) != 0)
                    {
                        // Add some delay here for next output after setting changed
                    }
                    break;

                case HAL_UART_ACT_RESET_CHIP:
                case HAL_UART_ACT_SLEEP_MODE_SWITCH:
                case HAL_UART_HTTP_RECVIND_FLOW_CONTROL:

                    if(g_halUartHandler[uartIndex].post_send_cb)
                    {
                        g_halUartHandler[uartIndex].post_send_cb(msgType, msg.msgPtr, msg.msgLen);
                    }

                    break;

                default:
                    // Just ignore other undefined messages
                    break;
            }

            if(msg.msgPtr)
            {
                free(msg.msgPtr);
            }
        }
    }
}

static void HAL_UART_CreateSendTaskAndQueue(void)
{
    if(g_halUartInitFlag & HAL_UART_SEND_TASK_QUEUE_CREATED)
    {
        return;
    }

    osThreadId_t threadId;
    osThreadAttr_t task_attr;
    osMessageQueueAttr_t queue_attr;

    g_uartSendCompleteFlag = osEventFlagsNew(NULL);

    EC_ASSERT(g_uartSendCompleteFlag, g_uartSendCompleteFlag, 0, 0);

    g_uartIsSendBlock = 0;

    memset(&queue_attr, 0, sizeof(queue_attr));

    queue_attr.mq_size = sizeof(uart_send_queue_buf);
    queue_attr.mq_mem = uart_send_queue_buf;
    queue_attr.cb_mem = &uart_send_queue_cb;
    queue_attr.cb_size = sizeof(uart_send_queue_cb);

    mid_uart_send_msgqueue = osMessageQueueNew(SEND_MSGQUEUE_OBJECTS, sizeof(send_msgqueue_obj_t), &queue_attr);

    EC_ASSERT(mid_uart_send_msgqueue, mid_uart_send_msgqueue, 0, 0);

    memset(&task_attr,0,sizeof(task_attr));
    memset(uart_send_task_stack, 0xA5, UART_SEND_TASK_STACK_SIZE);
    task_attr.name = "uartsend";
    task_attr.stack_mem = uart_send_task_stack;
    task_attr.stack_size = UART_SEND_TASK_STACK_SIZE;
    task_attr.priority = osPriorityLow;
    task_attr.cb_mem = &uart_send_task;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    threadId = osThreadNew(HAL_UART_SendTaskEntry, NULL, &task_attr);

    EC_ASSERT(threadId, threadId, 0, 0);

    g_halUartInitFlag |= HAL_UART_SEND_TASK_QUEUE_CREATED;

}

//-----------------------------------------------------------------//
// ---------------------------PUBLIC API---------------------------//
//-----------------------------------------------------------------//
int32_t HAL_UART_InitHandler(uint8_t uartIndex, hal_uart_config_t* config, hal_uart_hardware_config_t* hwConfig, hal_uart_task_create_flag_t taskCreatFlag)
{
    int32_t ret;

    ARM_USART_STATUS uart_status;

    EC_ASSERT((uartIndex < HAL_UART_MAX_NUM) && config && hwConfig, uartIndex, config, hwConfig);
    EC_ASSERT(config->uartDriverHandler, config->uartDriverHandler, 0, 0);

    g_halUartHandler[uartIndex].uartDriverHandler = config->uartDriverHandler;
    g_halUartHandler[uartIndex].recvBuffPtr = config->recvBuffPtr;
    g_halUartHandler[uartIndex].recvBuffSize = config->recvBuffSize;
    g_halUartHandler[uartIndex].recv_cb = config->recv_cb;
    g_halUartHandler[uartIndex].pre_send_cb = config->pre_send_cb;
    g_halUartHandler[uartIndex].post_send_cb = config->post_send_cb;

    ret = config->uartDriverHandler->Initialize(g_halUartRecvCallbacks[uartIndex]);

    if(ret != ARM_DRIVER_OK)
    {
        return ret;
    }

    ret = config->uartDriverHandler->PowerControl(hwConfig->powerMode);

    if(ret != ARM_DRIVER_OK)
    {
        return ret;
    }

    ret = config->uartDriverHandler->Control(hwConfig->controlSetting, hwConfig->baudRate);

    if(ret != ARM_DRIVER_OK)
    {
        return ret;
    }

    if(taskCreatFlag & HAL_UART_TASK_CREATE_FLAG_RECV)
    {
        EC_ASSERT(config->recvBuffPtr && config->recvBuffSize, taskCreatFlag, config->recvBuffPtr, config->recvBuffSize);

        HAL_UART_CreateRecvTaskAndQueue();

        ret = config->uartDriverHandler->Receive(config->recvBuffPtr, config->recvBuffSize);

        if(ret != ARM_DRIVER_OK)
        {
            return ret;
        }

    }

    if(taskCreatFlag & HAL_UART_TASK_CREATE_FLAG_SEND)
    {
        HAL_UART_CreateSendTaskAndQueue();
    }

    uart_status = g_halUartHandler[uartIndex].uartDriverHandler->GetStatus();

    g_uartIsSendBlock |= (uart_status.is_send_block << uartIndex);

    return 0;

}

int32_t HAL_UART_ResetUartSetting(uint8_t uartIndex, hal_uart_hardware_config_t* newHwConfig, bool startRecv)
{
    int32_t ret;

    EC_ASSERT((uartIndex < HAL_UART_MAX_NUM) && newHwConfig && g_halUartHandler[uartIndex].uartDriverHandler, uartIndex, newHwConfig, g_halUartHandler[uartIndex].uartDriverHandler);


    ret = g_halUartHandler[uartIndex].uartDriverHandler->PowerControl(ARM_POWER_OFF);

    if(ret != ARM_DRIVER_OK)
    {
        return ret;
    }

    ret = g_halUartHandler[uartIndex].uartDriverHandler->PowerControl(newHwConfig->powerMode);

    if(ret != ARM_DRIVER_OK)
    {
        return ret;
    }

    ret = g_halUartHandler[uartIndex].uartDriverHandler->Control(newHwConfig->controlSetting, newHwConfig->baudRate);

    if(ret != ARM_DRIVER_OK)
    {
        return ret;
    }

    if(startRecv)
    {
        EC_ASSERT(g_halUartHandler[uartIndex].recvBuffPtr && g_halUartHandler[uartIndex].recvBuffSize, startRecv, g_halUartHandler[uartIndex].recvBuffPtr, g_halUartHandler[uartIndex].recvBuffSize);

        ret = g_halUartHandler[uartIndex].uartDriverHandler->Receive(g_halUartHandler[uartIndex].recvBuffPtr, g_halUartHandler[uartIndex].recvBuffSize);

        if(ret != ARM_DRIVER_OK)
        {
            return ret;
        }
    }

    return 0;

}

void HAL_UART_DeInitHandler(uint8_t uartIndex)
{
    EC_ASSERT((uartIndex < HAL_UART_MAX_NUM), uartIndex, HAL_UART_MAX_NUM, 0);

    g_halUartHandler[uartIndex].uartDriverHandler->PowerControl(ARM_POWER_OFF);
    g_halUartHandler[uartIndex].uartDriverHandler->Uninitialize();

    memset(&g_halUartHandler[uartIndex], 0, sizeof(hal_uart_handler_t));
}

void HAL_UART_RecvFlowControl(bool on)
{
    EC_ASSERT(g_uartRecvFlowControlFlag, g_uartRecvFlowControlFlag, 0, 0);

    if(on == true)
    {
        osEventFlagsClear(g_uartRecvFlowControlFlag, UART_RECV_FLOW_CONTROL_FLAG);
    }
    else
    {
        osEventFlagsSet(g_uartRecvFlowControlFlag, UART_RECV_FLOW_CONTROL_FLAG);
    }
}


ARM_USART_MODEM_STATUS HAL_UART_GetModemStatus(uint8_t uartIdx)
{
    if(uartIdx < HAL_UART_MAX_NUM)
    {
        return g_halUartHandler[uartIdx].uartDriverHandler->GetModemStatus();
    }
    else
    {
        return (ARM_USART_MODEM_STATUS){0};
    }

}

int32_t HAL_UART_SetModemControl(uint8_t uartIdx, ARM_USART_MODEM_CONTROL control)
{
    if(uartIdx < HAL_UART_MAX_NUM)
    {
        return g_halUartHandler[uartIdx].uartDriverHandler->SetModemControl(control);
    }
    else
    {
        return -2;
    }
}

