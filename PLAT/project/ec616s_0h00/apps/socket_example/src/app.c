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
#include "gps_demo.h"
#include "cJSON.h"

typedef enum {
    UDP_CLIENT,
    UDP_SERVER,
    TCP_CLIENT,
    TCP_SERVER
} socketCaseNum;

// Choose which test case to run
static socketCaseNum testCaseNum = TCP_CLIENT;
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
static char g_local_ips[16] = {0};
static char g_imei[20] = {0};
static char g_iccid[24] = {0};
static UINT32 g_rssi = 0;

static UINT32 g_send_data_interval = 300; // s

void soket_exp_dev_info_init(void)
{
	uint32_t ret = appGetImeiNumSync(g_imei);
	ret |= appGetIccidNumSync(g_iccid);
	if (ret != NBStatusSuccess) {
		return;
	}
}


void soket_exp_conn_localIP(char* localIP)
{
    if(localIP==NULL)
    {
        return;
    }

    NmAtiSyncRet netInfo;
    NBStatus_t nbstatus=appGetNetInfoSync(0 ,&netInfo);
	printf("gCellID =%d; ipType=%d\r\n", gCellID , netInfo.body.netInfoRet.netifInfo.ipType);
    if ( NM_NET_TYPE_IPV4 == netInfo.body.netInfoRet.netifInfo.ipType ||
		NM_NET_TYPE_IPV4V6 == netInfo.body.netInfoRet.netifInfo.ipType)
    {
        uint8_t* ips=(uint8_t *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr;
        sprintf(localIP,"%u.%u.%u.%u",ips[0],ips[1],ips[2],ips[3]);
        ECOMM_STRING(UNILOG_PLA_APP, soket_exp_conn_localIP_1, P_INFO, "local IP:%s", (const uint8_t *)localIP);
    }
}


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
			printf("rssi:%d\r\n", rssi);
			g_rssi = rssi;
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


UINT32 g_seqNum = 0;

extern char g_GPS_raw_data[];

static void ZmBuildLoginMsg()
{
	cJSON* cjson_obj = NULL;
    char* str = NULL;

	OsaUtcTimeTValue    timeUtc;       //24 bytes
	(void)appGetSystemTimeUtcSync(&timeUtc);

    /* 创建一个JSON数据对象(链表头结点) */
    cjson_obj = cJSON_CreateObject();
#if 0
    /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
    cJSON_AddStringToObject(cjson_obj, "id", g_imei);
#endif
    /* 添加一条整数类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_obj, "zmd", 1);
#if 0
    /* 添加一条浮点类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_obj, "sq", (g_seqNum % 0xFFFF));
	if (g_seqNum >= 0xFFFF - 1) {
		g_seqNum = 0;
	} else {
		g_seqNum++;
	}

	cJSON_AddStringToObject(cjson_obj, "cid", g_iccid);
#endif
	cJSON_AddNumberToObject(cjson_obj, "date", timeUtc.UTCsecs);
#if 0
	cJSON_AddNumberToObject(cjson_obj, "tick", g_send_data_interval);
#endif
	cJSON_AddNumberToObject(cjson_obj, "ver", 10);
	cJSON_AddNumberToObject(cjson_obj, "hwr", 10);

    /* 打印JSON对象(整条链表)的所有数据 */
    str = cJSON_PrintUnformatted(cjson_obj);
    printf("%s\n", str);
	send( sockfd, str, strlen(str), 0 );
	free(str);
	cJSON_Delete(cjson_obj);
}

static void ZmBuildLocInfoMsg()
{
	cJSON* cjson_obj = NULL;
    char* str = NULL;

	OsaUtcTimeTValue    timeUtc;       //24 bytes
	(void)appGetSystemTimeUtcSync(&timeUtc);

    /* 创建一个JSON数据对象(链表头结点) */
    cjson_obj = cJSON_CreateObject();

    /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
    cJSON_AddStringToObject(cjson_obj, "id", g_imei);

    /* 添加一条整数类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_obj, "zmd", 7);
#if 0
    /* 添加一条浮点类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_obj, "sq", (g_seqNum % 0xFFFF));
	if (g_seqNum >= 0xFFFF - 1) {
		g_seqNum = 0;
	} else {
		g_seqNum++;
	}
#endif
	//cJSON_AddStringToObject(cjson_test, "date", "190819151045");
	cJSON_AddNumberToObject(cjson_obj, "bat", 50);
#if 0
	cJSON_AddNumberToObject(cjson_obj, "rf", g_rssi);
	cJSON_AddNumberToObject(cjson_obj, "date", timeUtc.UTCsecs);
#endif
	GPS_Date *pdate = NULL;
	GPS_Location *pLoc = NULL;
	GpsGetData(&pdate, &pLoc);
	char buff[40] = {0};
#if 0
	sprintf(buff, "%02d.%02d%04d,%03d.%02d%04d,%.2f,%.2f", pLoc->weidu, pLoc->weiduMin, pLoc->weiduSec, pLoc->jingdu, pLoc->jingduMin, pLoc->jingduSec, pdate->spd, pdate->cog);
#endif
	sprintf(buff, "%d,%05.2f,%06.2f", timeUtc.UTCsecs, pLoc->weidu + pLoc->weiduMin / 60.0, pLoc->jingdu + pLoc->jingduMin / 60.0);
	char *gpsstr = buff;
	if (strlen(buff) == 0) {
		gpsstr = "null";
	}
	cJSON_AddStringToObject(cjson_obj, "gps", gpsstr);

    /* 打印JSON对象(整条链表)的所有数据 */
    str = cJSON_PrintUnformatted(cjson_obj);
    printf("%s\n", str);
	send( sockfd, str, strlen(str), 0 );
	free(str);
	cJSON_Delete(cjson_obj);
}


UINT8 g_send_switch = 0;

static void testCaseTcpClient()
{
    eventCallbackMessage_t *queueItem = NULL;
    INT32 recvTimeout = 10;
    INT32  result;
    UINT32 cliLen;
    fd_set readfds;
    struct timeval tv;

    struct addrinfo hints, *server_res;
#if 0
#define SEND_INTV 30
    CHAR   serverip[] = "120.76.100.197";//"47.95.193.246";
    CHAR serverport[] = "10002";//"6000";
#else
#define SEND_INTV 300
	CHAR   serverip[] = "121.41.164.214";//"120.76.100.197";//"47.95.193.246";
    CHAR serverport[] = "50005";//"10002";//"6000";
#endif
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    while(1){
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
        	printf("Queue receive->0x%x\r\n", queueItem->messageId);
            ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_1, P_INFO, 1, "Queue receive->0x%x", queueItem->messageId);
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IP_READY:
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_2, P_INFO, 0, "IP got ready");
                    if (getaddrinfo( serverip, serverport , &hints, &server_res ) != 0 ) {
                       ECOMM_TRACE(UNILOG_HTTP_CLIENT, testCaseTcpClient_21, P_WARNING, 0, "TCP connect unresolved dns");
                    }
                    sockfd = socket(AF_INET, SOCK_STREAM, 0);
					printf("sockfd=%d\r\n", sockfd);
                    if (sockfd < 0)
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_3, P_ERROR, 0, "socket create error");
                    if (connect(sockfd, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) 
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_4, P_ERROR, 0, "socket connect fail");
                        close(sockfd);
                        break;
                    }
                    ECOMM_TRACE(UNILOG_PLA_APP, testCaseTcpClient_5, P_INFO, 0, "socket connect success");

					soket_exp_dev_info_init();
					soket_exp_conn_localIP(g_local_ips);
					printf("IP:%s\r\n", g_local_ips);

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
					printf("send data\r\n");
					UINT32 delay_s = SEND_INTV;
					//UINT32 gpslen = strlen(g_GPS_raw_data);
					if (g_send_switch == 0) {
						g_send_switch = 1;
						delay_s = 2;
                    	//send( sockfd, "SLM:hello", 10, 0 );
                    	ZmBuildLoginMsg();
						//osDelay(1 * 1000/portTICK_PERIOD_MS);
					} else {
						//g_send_switch = 0;
						//send( sockfd, g_GPS_raw_data, gpslen + 1, 0 );
						ZmBuildLocInfoMsg();
						//osDelay(30 * 1000/portTICK_PERIOD_MS);
					}
                    osDelay(delay_s * 1000/portTICK_PERIOD_MS);
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
	SetPrintUart(PORT_USART_1);
    osKernelInitialize();
    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);

}

