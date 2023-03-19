/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616 socket demo entry source file
 * History:      Rev1.0   2018-10-12
 *
 ****************************************************************************/
#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "queue.h"
#include "ps_event_callback.h"
//#include "psproxytask.h"
#include "app.h"
#include "cmisim.h"
#include "cmimm.h"
#include "cmips.h"
#include "psifevent.h"
#include "ps_lib_api.h"
#include "lwip/netdb.h"
#include "debug_log.h"
#include "slpman_ec616s.h"
#include "plat_config.h"
#include "hal_uart.h"
#include "ec_tcpip_api.h"


// Choose which test case to run
static socketCaseNum testCaseNum = UDP_CLIENT;
uint8_t socketSlpHandler           = 0xff;


// app task static stack and control block
#define INIT_TASK_STACK_SIZE    (1024)
#define RINGBUF_READY_FLAG      (0x06)
#define APP_EVENT_QUEUE_SIZE    (10)
#define MAX_PACKET_SIZE         (256)

static StaticTask_t             initTask;
static UINT8                    appTaskStack[INIT_TASK_STACK_SIZE];
static volatile UINT32          Event;
static QueueHandle_t            psEventQueueHandle;
static UINT8                    gImsi[16] = {0};
static UINT32                   gCellID = 0;

static AppSocketConnectionContext socContext = {-1, APP_SOCKET_CONNECTION_CLOSED};

static void sendQueueMsg(UINT32 msgId, UINT32 xTickstoWait)
{
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle)
    {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, sendQueueMsg_1, P_ERROR, 0, "xQueueSend error");
        }
    }
}

static INT32 socketRegisterPSUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;

    UINT8 rssi = 0;
    NmAtiNetifInfo *netif = NULL;

    switch(eventID)
    {
        case NB_URC_ID_SIM_READY:
        {
            imsi = (CmiSimImsiStr *)param;
            memcpy(gImsi, imsi->contents, imsi->length);
            ECOMM_STRING(UNILOG_PLA_STRING, socketRegisterPSUrcCallback_0, P_INFO, "SIM ready(imsi=%s)", (UINT8 *)imsi->contents);
            break;
        }
        case NB_URC_ID_MM_SIGQ:
        {
            rssi = *(UINT8 *)param;
            ECOMM_TRACE(UNILOG_PLA_APP, socketRegisterPSUrcCallback_1, P_INFO, 1, "RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, socketRegisterPSUrcCallback_2, P_INFO, 0, "Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, socketRegisterPSUrcCallback_3, P_INFO, 0, "Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED:
        {
            cereg = (CmiPsCeregInd *)param;
            gCellID = cereg->celId;
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_4, P_INFO, 4, "URCCallBack:CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
                sendQueueMsg(QMSG_ID_NW_IP_READY, 0);
            break;
        }
    }
    return 0;
}

void socketAppConnectionCallBack(UINT8 connectionEventType, void *bodyEvent)
{
    switch(connectionEventType)
    {
        case TCPIP_CONNECTION_STATUS_EVENT:
        {
            TcpipConnectionStatusInd *statusInd;
            statusInd = (TcpipConnectionStatusInd *)bodyEvent;
            if(statusInd != PNULL)
            {
                if(statusInd->status == TCPIP_CONNECTION_STATUS_CLOSED)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_1, P_ERROR, 2, "socketAppConnectionCallBack socket connection %u closed,cause %u", statusInd->connectionId, statusInd->cause);
                    if(statusInd->connectionId == socContext.id)
                    {
                        socContext.id = -1;
                        socContext.status = APP_SOCKET_CONNECTION_CLOSED;
                    }
                }
                else if(statusInd->status == TCPIP_CONNECTION_STATUS_CONNECTING)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_2, P_INFO, 1, "socketAppConnectionCallBack socket connection %u is connecting", statusInd->connectionId);
                    if(statusInd->connectionId == socContext.id)
                    {
                        socContext.status = APP_SOCKET_CONNECTION_CONNECTING;
                    }
                }
                else if(statusInd->status == TCPIP_CONNECTION_STATUS_CONNECTED)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_3, P_ERROR, 1, "socketAppConnectionCallBack socket connection %u is connected", statusInd->connectionId);
                    if(statusInd->connectionId == socContext.id)
                    {
                        socContext.status = APP_SOCKET_CONNECTION_CONNECTED;
                    }
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_4, P_ERROR, 0, "socketAppConnectionCallBack invalid connection status event");
            }

            break;
        }
        case TCPIP_CONNECTION_RECEIVE_EVENT:
        {
            TcpipConnectionRecvDataInd *rcvInd;
            rcvInd = (TcpipConnectionRecvDataInd *)bodyEvent;
            if(rcvInd != PNULL)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_5, P_INFO, 2, "socketAppConnectionCallBack socket connection %u receive length %u data", rcvInd->connectionId, rcvInd->length);
            }
            else
            {
                ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_6, P_ERROR, 0, "socketAppConnectionCallBack invalid connection rcv event");
            }

            break;
        }
        case TCPIP_CONNECTION_UL_STATUS_EVENT:
        {
            TcpipConnectionUlDataStatusInd *ulStatusInd;
            ulStatusInd = (TcpipConnectionUlDataStatusInd *)bodyEvent;
            if(ulStatusInd != PNULL)
            {
                if(ulStatusInd->status == Tcpip_Connection_UL_DATA_SUCCESS)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_7, P_INFO, 2, "socketAppConnectionCallBack socket connection %u sequence %u data has sent success", ulStatusInd->connectionId, ulStatusInd->sequence);
                }
                else if(ulStatusInd->status == Tcpip_Connection_UL_DATA_FAIL)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_8, P_WARNING, 2, "socketAppConnectionCallBack socket connection %u sequence %u data has sent fail", ulStatusInd->connectionId, ulStatusInd->sequence);
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_9, P_ERROR, 0, "socketAppConnectionCallBack invalid connection ul status event");
            }

            break;
        }
        default:
            ECOMM_TRACE(UNILOG_PLA_APP, socketAppConnectionCallBack_10, P_ERROR, 1, "socketAppConnectionCallBack invalid event type %u", connectionEventType);
            break;
    }
    
}


static void ecTestCaseUdpClient()
{
    eventCallbackMessage_t *queueItem = NULL;

    INT32 connectionId = -1;
    INT32 len;
    UINT8 data[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

    while (1)
    {
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_1, P_INFO, 1, "Queue receive->0x%x", queueItem->messageId);
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IP_READY:
                    connectionId = tcpipConnectionCreate(TCPIP_CONNECTION_PROTOCOL_UDP, PNULL, 0, APP_SOCKET_DAEMON_SERVER_IP, APP_SOCKET_DAEMON_SERVER_PORT, socketAppConnectionCallBack);
                    if(connectionId >= 0)
                    {
                        socContext.id = connectionId;
                        socContext.status = APP_SOCKET_CONNECTION_CONNECTED;
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_2, P_INFO, 1, "create connection %u success", socContext.id);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_3, P_ERROR, 0, "create connection fail");
                    }
                    break;
                case QMSG_ID_NW_IP_NOREACHABLE:
                case QMSG_ID_NW_IP_SUSPEND:
                    if (socContext.id >= 0 && socContext.status != APP_SOCKET_CONNECTION_CLOSED)
                    {
                        if(tcpipConnectionClose(socContext.id) < 0)
                        {
                            socContext.id = -1;
                            socContext.status = APP_SOCKET_CONNECTION_CLOSED;
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_4, P_INFO, 1, "close connection %u success", socContext.id);                            
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_5, P_ERROR, 1, "close connection %u fail", socContext.id);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_6, P_ERROR, 2, "connection %u or status invalid", socContext.id, socContext.status); 
                    }                    
                    break;
                case QMSG_ID_SOCK_SENDPKG:
                    if (socContext.id >= 0 && socContext.status == APP_SOCKET_CONNECTION_CONNECTED)
                    {
                        len = tcpipConnectionSend(socContext.id, data, 8, 0, 0, 0);
                        if(len > 0)
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_7, P_INFO, 2, "connection %u send len %u success", socContext.id, len);  
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_8, P_ERROR, 1, "connection %u sent data fail", socContext.id);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseUdpClient_9, P_ERROR, 2, "connection %u or status invalid", socContext.id, socContext.status); 
                    }
                    break;
                default:
                    break;
            }
            free(queueItem);
        }
    }
}

static void ecTestCaseTcpClient()
{
    eventCallbackMessage_t *queueItem = NULL;

    INT32 connectionId = -1;
    INT32 len;
    UINT8 data[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

    while (1)
    {
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_1, P_INFO, 1, "Queue receive->0x%x", queueItem->messageId);
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IP_READY:
                    connectionId = tcpipConnectionCreate(TCPIP_CONNECTION_PROTOCOL_TCP, PNULL, 0, APP_SOCKET_DAEMON_SERVER_IP, APP_SOCKET_DAEMON_SERVER_PORT, socketAppConnectionCallBack);
                    if(connectionId >= 0)
                    {
                        socContext.id = connectionId;
                        socContext.status = APP_SOCKET_CONNECTION_CONNECTED;
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_2, P_INFO, 1, "create connection %u success", socContext.id);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_3, P_ERROR, 0, "create connection fail");
                    }
                    break;
                case QMSG_ID_NW_IP_NOREACHABLE:
                case QMSG_ID_NW_IP_SUSPEND:
                    if (socContext.id >= 0 && socContext.status != APP_SOCKET_CONNECTION_CLOSED)
                    {
                        if(tcpipConnectionClose(socContext.id) < 0)
                        {
                            socContext.id = -1;
                            socContext.status = APP_SOCKET_CONNECTION_CLOSED;
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_4, P_INFO, 1, "close connection %u success", socContext.id);                            
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_5, P_ERROR, 1, "close connection %u fail", socContext.id);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_6, P_ERROR, 2, "connection %u or status invalid", socContext.id, socContext.status); 
                    }                    
                    break;
                case QMSG_ID_SOCK_SENDPKG:
                    if (socContext.id >= 0 && socContext.status == APP_SOCKET_CONNECTION_CONNECTED)
                    {
                        len = tcpipConnectionSend(socContext.id, data, 8, 0, 0, 0);
                        if(len > 0)
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_7, P_INFO, 2, "connection %u send len %u success", socContext.id, len);  
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_8, P_ERROR, 1, "connection %u sent data fail", socContext.id);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ecTestCaseTcpClient_9, P_ERROR, 2, "connection %u or status invalid", socContext.id, socContext.status); 
                    }
                    break;
                default:
                    break;
            }
            free(queueItem);
        }
    }
}


static void ecSocketAppTask(void *arg)
{
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ecSocketAppTask_1, P_ERROR, 0, "psEventQueue create error!");
        return;
    }

    switch(testCaseNum)
    {
        case UDP_CLIENT:
            ecTestCaseUdpClient();
            break;
        case TCP_CLIENT:
            ecTestCaseTcpClient();
            break;
        default:
            break;
    }
}

static void appInit(void *arg)
{
    osThreadAttr_t task_attr;

    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_CONTROL) != 0)
    {
        HAL_UART_RecvFlowControl(false);
    }

    slpManApplyPlatVoteHandle("SOCKET",&socketSlpHandler);
    registerPSEventCallback(NB_GROUP_ALL_MASK, socketRegisterPSUrcCallback);
    memset(&task_attr,0,sizeof(task_attr));
    memset(appTaskStack, 0xA5,INIT_TASK_STACK_SIZE);
    task_attr.name = "app";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(ecSocketAppTask, NULL, &task_attr);
    
    //abupfotaInit();
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

