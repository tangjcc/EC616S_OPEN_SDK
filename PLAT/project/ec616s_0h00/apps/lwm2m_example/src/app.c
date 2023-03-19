/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616 lwm2m demo entry source file
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
#include "client/lwm2mclient.h"

// TinyDTLS need such large stack
#define INIT_TASK_STACK_SIZE    (1024*4)
#define RINGBUF_READY_FLAG      (0x06)
#define APP_EVENT_QUEUE_SIZE    (10)
#define MAX_PACKET_SIZE         (256)

static StaticTask_t             initTask;
static UINT8                    appTaskStack[INIT_TASK_STACK_SIZE];
static volatile UINT32          Event;
static QueueHandle_t            psEventQueueHandle;
static UINT8                    gImsi[16] = {0};
static UINT32                   gCellID = 0;
static NmAtiSyncRet             gNetworkInfo;
static UINT8                    lwm2mSlpHandler = 0xff;



trace_add_module(APP, P_INFO);


static void sendQueueMsg(UINT32 msgId, UINT32 xTickstoWait)
{
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle)
    {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait))
        {
            DEBUG_TRACE(APP, P_ERROR,"xQueueSend error");
        }
    }
}

static INT32 registerPSUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;
    //UINT8 rssi = 0;
    NmAtiNetifInfo *netif = NULL;

    switch(eventID)
    {
        case NB_URC_ID_SIM_READY:
        {
            imsi = (CmiSimImsiStr *)param;
            memcpy(gImsi, imsi->contents, imsi->length);
            DEBUG_TRACE(APP, P_INFO,"SIM ready(imsi=%s len=%d)", imsi->contents, imsi->length);
            break;
        }
        case NB_URC_ID_MM_SIGQ:
        {
            //rssi = *(UINT8 *)param;
            //DEBUG_TRACE(APP, P_INFO,"RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED:
        {
            DEBUG_TRACE(APP, P_INFO,"Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED:
        {
            DEBUG_TRACE(APP, P_INFO,"Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED:
        {
            cereg = (CmiPsCeregInd *)param;
            gCellID = cereg->celId;
            DEBUG_TRACE(APP, P_INFO,"CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
                sendQueueMsg(QMSG_ID_NW_IPV4_READY, 0);
            break;
        }
    }
    return 0;
}



static void lwm2mAppTask(void *arg)
{
    //UINT8 cid[CMI_PS_CID_NUM] = {0};
    //UINT8 cidlen = 0;
    //INT8 ret;
    UINT8 psmMode = 0, actType = 0;//edrxModeValue = 0;
    UINT16 tac = 0;
    UINT32 tauTime = 0, activeTime = 0, cellID = 0, nwEdrxValueMs = 0, nwPtwMs = 0;// reqEdrxValueMs = 0;

    eventCallbackMessage_t *queueItem = NULL;

    registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback);
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL)
    {
        DEBUG_TRACE(APP, P_ERROR,"psEventQueue create error!");
        return;
    }

    slpManApplyPlatVoteHandle("lwm2m",&lwm2mSlpHandler);
    slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_HIB_STATE);
    DEBUG_TRACE(APP, P_INFO,"first time run lwm2m example");

    while (1)
    {
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IPV4_READY:
                case QMSG_ID_NW_IPV6_READY:
                case QMSG_ID_NW_IPV4_6_READY:
                    appGetImsiNumSync((CHAR*)gImsi);
                    DEBUG_TRACE(APP, P_INFO,"IMSI = %s", gImsi);
                    appGetNetInfoSync(gCellID, &gNetworkInfo);
                    if ( NM_NET_TYPE_IPV4 == gNetworkInfo.body.netInfoRet.netifInfo.ipType)
                        DEBUG_TRACE(APP, P_INFO,"IP:\"%u.%u.%u.%u\"", ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);
                    appGetLocationInfoSync(&tac, &cellID);
                    DEBUG_TRACE(APP, P_INFO,"tac=%d, cellID=%d ret=%d", tac, cellID);
                    //edrxModeValue = CMI_MM_ENABLE_EDRX_AND_ENABLE_IND;
                    actType = CMI_MM_EDRX_NB_IOT;
                    //reqEdrxValueMs = 20480;
                   // appSetEDRXSettingSync(edrxModeValue, actType, reqEdrxValueMs);
                    appGetEDRXSettingSync(&actType, &nwEdrxValueMs, &nwPtwMs);
                    DEBUG_TRACE(APP, P_INFO,3,"actType=%d, nwEdrxValueMs=%d nwPtwMs=%d", actType, nwEdrxValueMs, nwPtwMs);

                    //ret = appGetActedCidSync(cid, &cidlen);
                    //DEBUG_TRACE(APP, P_INFO,"cid=%d, len=%d ret=%d", cid[0], cidlen, ret);
                   // ret = appGetPSMSettingSync(&psmMode, &tauTime, &activeTime);
                   // DEBUG_TRACE(APP, P_INFO,"Get PSM info mode=%d, TAU=%d, ActiveTime=%d", psmMode, tauTime, activeTime);
                    psmMode = 1;
                    tauTime = 4000;
                    activeTime = 30;
                    //ret = appSetPSMSettingSync(psmMode, tauTime,activeTime);
                    //if (ret == NBStatusSuccess)
                    {
                        appGetPSMSettingSync(&psmMode, &tauTime, &activeTime);
                        DEBUG_TRACE(APP, P_INFO,"Get PSM info mode=%d, TAU=%d, ActiveTime=%d", psmMode, tauTime, activeTime);
                    }
                    startLwm2mClient();
                    break;
                case QMSG_ID_NW_DISCONNECT:
                    break;

                default:
                    break;
            }
            free(queueItem);
        }
    }

}

static void appInit(void *arg)
{
    osThreadAttr_t task_attr;
    memset(&task_attr,0,sizeof(task_attr));
    memset(appTaskStack, 0xA5,INIT_TASK_STACK_SIZE);
    task_attr.name = "lwm2mapp";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(lwm2mAppTask, NULL, &task_attr);
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

