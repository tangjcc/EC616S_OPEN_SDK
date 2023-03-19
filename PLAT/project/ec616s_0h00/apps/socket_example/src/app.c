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
#include "sockets.h"
#include "psifevent.h"
#include "ps_lib_api.h"
#include "lwip/netdb.h"
#include "debug_log.h"
#include "slpman_ec616s.h"
#include "plat_config.h"
#include "hal_uart.h"

typedef enum {
    UDP_CLIENT,
    UDP_SERVER,
    TCP_CLIENT,
    TCP_SERVER
} socketCaseNum;

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
static INT32                    sockfd = -1;
static UINT32                   gCellID = 0;


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

    UINT8 rssi = 0, index = 0;
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



static void testCaseUdpClient()
{
    eventCallbackMessage_t *queueItem = NULL;

    UINT8  enable = 1;
    CHAR   serverip[] = "47.105.44.99";
    UINT16 serverport = 5684;
    struct sockaddr_in servaddr;
    memset( &servaddr, 0, sizeof(servaddr) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(serverport);
    servaddr.sin_addr.s_addr = inet_addr(serverip);

    while (1)
    {
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpClient_1, P_INFO, 1, "Queue receive->0x%x", queueItem->messageId);
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IP_READY:
                    if ( (sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0 )
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpClient_2, P_ERROR, 0, "socket create error!");
                        break;
                    }
                    ioctlsocket(sockfd, FIONBIO, (unsigned long *)(&enable));

                    sendQueueMsg(QMSG_ID_SOCK_SENDPKG, 0);

                    break;
                case QMSG_ID_NW_IP_NOREACHABLE:
                case QMSG_ID_NW_IP_SUSPEND:
                    if (sockfd > 0)
                    {
                       close(sockfd);
                       sockfd = -1;
                    }
                    break;
                case QMSG_ID_SOCK_SENDPKG:
                    sendto( sockfd, "hello world", strlen("hello world") + 1, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
                    osDelay(2000);
                    sendQueueMsg(QMSG_ID_SOCK_SENDPKG, 0);
                    break;
                default:
                    break;
            }
            free(queueItem);
        }
    }
}

static void testCaseUdpServer()
{
    eventCallbackMessage_t *queueItem = NULL;
    struct sockaddr_in cliAddr, servAddr;
    INT32 recvTimeout = 10;
    INT32  result;
    UINT32 cliLen;
    fd_set readfds;
    struct timeval tv;

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(5000);

    while(1){
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_1, P_INFO, 1, "Queue receive->0x%x", queueItem->messageId);
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IP_READY:
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_2, P_INFO, 0, "IP got ready");
                    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                    if (sockfd < 0)
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_3, P_ERROR, 0, "socket create error");
                    result = bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr));
                    if (result < 0)
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_4, P_ERROR, 0, "socket bind error");
                    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&recvTimeout, sizeof(recvTimeout));
                    sendQueueMsg(QMSG_ID_SOCK_RECVPKG, 0);
                    break;
                case QMSG_ID_NW_IP_NOREACHABLE:
                case QMSG_ID_NW_IP_SUSPEND:
                    if (sockfd > 0)
                    {
                       close(sockfd);
                       sockfd = -1;
                    }
                    break;
                case QMSG_ID_SOCK_RECVPKG:
                    FD_ZERO(&readfds);
                    FD_SET(sockfd, &readfds);

                    tv.tv_sec = 60;
                    tv.tv_usec = 0;

                    result = select(FD_SETSIZE, &readfds, 0, 0, &tv);
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_5, P_INFO, 1, "select result %d",result);
                    if (result < 0)
                    {
                        if (errno != EINTR)
                        {
                            ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_6, P_INFO, 1, "Error in select():%d",errno);
                        }
                    } else if (result > 0) {
                        UINT8 buffer[MAX_PACKET_SIZE]= {0};
                        INT32 numBytes;
                        if (FD_ISSET(sockfd,&readfds))
                        {
                            cliLen = sizeof(cliAddr);

                            numBytes = recvfrom(sockfd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&cliAddr, &cliLen);
                            if (numBytes == -1)
                            {
                                ECOMM_TRACE(UNILOG_PLA_APP, testCaseUdpServer_7, P_INFO, 0, "Error in recvfrom()");
                            } else {
                                ECOMM_STRING(UNILOG_PLA_STRING, testCaseUdpServer_8, P_INFO, "recvbuffer:%s", buffer);
                                ECOMM_STRING(UNILOG_PLA_STRING, testCaseUdpServer_9, P_INFO, "recv from client:%s", inet_ntoa(cliAddr.sin_addr));
                            }
                        }
                    }
                    osDelay(1000);
                    sendQueueMsg(QMSG_ID_SOCK_RECVPKG, 0);
                    break;
            }
            free(queueItem);
        }
    }

}


static void testCaseTcpClient()
{
    eventCallbackMessage_t *queueItem = NULL;
    INT32 recvTimeout = 10;
    INT32  result;
    UINT32 cliLen;
    fd_set readfds;
    struct timeval tv;

    struct addrinfo hints, *server_res;

    CHAR   serverip[] = "47.95.193.246";
    CHAR serverport[] = "6000";
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    while(1){
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_1, P_INFO, 1, "Queue receive->0x%x", queueItem->messageId);
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IP_READY:
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_2, P_INFO, 0, "IP got ready");
                    if (getaddrinfo( serverip, serverport , &hints, &server_res ) != 0 ) {
                       ECOMM_TRACE(UNILOG_HTTP_CLIENT, testCaseTcpClient_21, P_WARNING, 0, "TCP connect unresolved dns");
                    }
                    sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    if (sockfd < 0)
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_3, P_ERROR, 0, "socket create error");
                    if (connect(sockfd, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) 
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_4, P_ERROR, 0, "socket connect fail");
                        close(sockfd);
                        break;
                    }
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_5, P_INFO, 0, "socket connect success");

                    sendQueueMsg(QMSG_ID_SOCK_SENDPKG, 0);
                    break;
                case QMSG_ID_NW_IP_NOREACHABLE:
                case QMSG_ID_NW_IP_SUSPEND:
                    if (sockfd > 0)
                    {
                       close(sockfd);
                       sockfd = -1;
                    }
                    break;
                case QMSG_ID_SOCK_SENDPKG:
                    send( sockfd, "hello", 6, 0 );
                    osDelay(2000);
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_6, P_INFO, 0, "socket send success");
                    sendQueueMsg(QMSG_ID_SOCK_SENDPKG, 0);
                    break;
            }
            free(queueItem);
        }
        }


}

static void socketAppTask(void *arg)
{
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, socketAppTask_1, P_ERROR, 0, "psEventQueue create error!");
        return;
    }

    switch(testCaseNum)
    {
        case UDP_SERVER:
            testCaseUdpServer();
            break;
        case UDP_CLIENT:
            testCaseUdpClient();
            break;
        case TCP_CLIENT:
            testCaseTcpClient();
            break;
        case TCP_SERVER:
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

    osThreadNew(socketAppTask, NULL, &task_attr);
    
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

