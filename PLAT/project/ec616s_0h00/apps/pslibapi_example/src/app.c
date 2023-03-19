/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616S pslibapi demo app source file
 * History:      Rev1.0   2018-10-12
 *
 ****************************************************************************/
#include "bsp.h"
#include "bsp_custom.h"
#include "ps_event_callback.h"
#include "hal_uart.h"
#include "cms_def.h"
#include "app.h"
#include "cmisim.h"
#include "cmimm.h"
#include "cmips.h"
#include "psifevent.h"
#include "ps_lib_api.h"
#include "lwip/netdb.h"
#include "debug_log.h"
#include "slpman_ec616s.h"
#include "time.h"
#include "plat_config.h"

// app task static stack and control block
#define INIT_TASK_STACK_SIZE    (2048)
#define RINGBUF_READY_FLAG      (0x06)
#define APP_EVENT_QUEUE_SIZE    (10)
#define MAX_PACKET_SIZE         (256)

static StaticTask_t             initTask;
static UINT8                    appTaskStack[INIT_TASK_STACK_SIZE];
static volatile UINT32          Event;
//static CHAR                     gImsi[16] = {0};
static appRunningState_t        stateMachine = APP_INIT_STATE;
uint8_t pslibapiSlpHandler      = 0xff;


static INT32 pslibPSUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    UINT8 rssi = 0;
    UINT8 timerState = 0;
    UINT8 celevel = 0;
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;
    NmAtiNetifInfo *netif = NULL;

    switch(eventID)
    {
        case NB_URC_ID_SIM_READY:
        {
            imsi = (CmiSimImsiStr *)param;
            ECOMM_STRING(UNILOG_PLA_APP, pslibPSUrcCallback_0, P_INFO, "URCCallBack:imsi=%s", (uint8_t*)imsi->contents);
            break;
        }
        case NB_URC_ID_MM_SIGQ:
        {
            CmiMmCesqInd *pMmCesqInd = (CmiMmCesqInd *)param;
            if(pMmCesqInd->rsrp < 28)
                rssi = 0;
            else if((pMmCesqInd->rsrp > 28) && (pMmCesqInd->rsrp < 90))
                rssi = (pMmCesqInd->rsrp - 28)/2;
            else if((pMmCesqInd->rsrp > 90) && (pMmCesqInd->rsrp <= 97))
                rssi = 31;
            else if(pMmCesqInd->rsrp == 255)
                rssi = 99;
            else
                rssi = 99;
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_1, P_INFO, 1, "URCCallBack:RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_2, P_INFO, 0, "URCCallBack:Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_3, P_INFO, 0, "URCCallBack:Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED:
        {
            cereg = (CmiPsCeregInd *)param;
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_4, P_INFO, 4, "URCCallBack:CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                stateMachine = APP_IPREADY_STATE;
                ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_5, P_INFO, 0, "URCCallBack:PSIF network active");
            }
            else if (netif->netStatus == NM_NO_NETIF_OR_DEACTIVATED)
            {
                stateMachine = APP_DEACTIVE_STATE;
                ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_7, P_INFO, 0, "URCCallBack:PSIF network deactive");
            }
            else if (netif->netStatus == NM_NETIF_ACTIVATED_INFO_CHNAGED)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_8, P_INFO, 0, "URCCallBack:PSIF network info changed");
            }
            break;

        }
        case NB_URC_ID_MM_TAU_EXPIRED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_9, P_INFO, 0, "URCCallBack:NB_URC_ID_MM_TAU_EXPIRED");
            break;
        }
        case NB_URC_ID_MM_CERES_CHANGED:
        {
            /*
             * 0 No Coverage Enhancement in the serving cell
             * 1 Coverage Enhancement level 0
             * 2 Coverage Enhancement level 1
             * 3 Coverage Enhancement level 2
             * 4 Coverage Enhancement level 3
            */
            celevel = *(UINT8 *)param;
            ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_10, P_INFO, 1, "URCCallBack:NB_URC_ID_MM_CERES_CHANGED celevel=%d", celevel);
            break;
        }
        case NB_URC_ID_MM_BLOCK_STATE_CHANGED:
        {
            timerState = *(UINT8 *)param;
            if (timerState == 0)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_11, P_INFO, 0, "URCCallBack:NB_URC_ID_MM_BLOCK_STATE_CHANGED in block state");
            }
            else
            {
                ECOMM_TRACE(UNILOG_PLA_APP, pslibPSUrcCallback_12, P_INFO, 0, "URCCallBack:NB_URC_ID_MM_BLOCK_STATE_CHANGED in unblock state");
            }
            break;
        }
    }
    return 0;
}

static void UeExtStatusInfoTest(void)
{
#define STR_BUFFER_SIZE    512
    UeExtStatusType statusType = UE_EXT_STATUS_ALL;
    UeExtStatusInfo *pStatusInfo = PNULL;
    CHAR    *pStrBuf = PNULL;

    pStrBuf = (CHAR *)OsaAllocZeroMemory(STR_BUFFER_SIZE);
    OsaCheck(pStrBuf != PNULL, STR_BUFFER_SIZE, 0, 0);

    pStatusInfo = (UeExtStatusInfo *)OsaAllocZeroMemory(sizeof(UeExtStatusInfo));
    OsaCheck(pStatusInfo != PNULL, sizeof(UeExtStatusInfo), 0, 0);

    for (statusType = UE_EXT_STATUS_ALL; statusType <= UE_EXT_STATUS_CCM; statusType++)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, UeExtStatusInfoTest_0, P_INFO, 1, "request status type %d", statusType);
        appGetUeExtStatusInfoSync(statusType, pStatusInfo);

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_PHY)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: PHY, DlEarfcn:%d, UlEarfcn:%d, PCI:%d, Band:%d, RSRP:%d, RSRQ:%d, SNR:%d, AnchorFreqOfst:%d, NonAnchorFreqOfst:%d, CeLevel:%d, DlBler:%d/100, UlBler:%d/100, DataInactTimerS:%d, RetxBSRTimerP:%d, TAvalue:%d, TxPower %d",
                     pStatusInfo->phyStatus.dlEarfcn, pStatusInfo->phyStatus.ulEarfcn, pStatusInfo->phyStatus.phyCellId, pStatusInfo->phyStatus.band,
                     pStatusInfo->phyStatus.sRsrp/100, pStatusInfo->phyStatus.sRsrq/100, pStatusInfo->phyStatus.snr, pStatusInfo->phyStatus.anchorFreqOffset, pStatusInfo->phyStatus.nonAnchorFreqOffset, ((pStatusInfo->phyStatus.ceLevel == 0xFF)? -2 : pStatusInfo->phyStatus.ceLevel),
                     pStatusInfo->phyStatus.dlBler, pStatusInfo->phyStatus.ulBler, pStatusInfo->phyStatus.dataInactTimerS, pStatusInfo->phyStatus.retxBSRTimerP, pStatusInfo->phyStatus.taValue, pStatusInfo->phyStatus.txPower);
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_phypStatusInfo, P_INFO, "phystatusInfo: %s", (UINT8 *)pStrBuf);
        }

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_L2)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: L2, SrbNum:%d, DrbNum:%d",
                     pStatusInfo->l2Status.srbNum, pStatusInfo->l2Status.drbNum);
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_L2statusInfo, P_INFO, "L2statusInfo %s", (UINT8 *)pStrBuf);
        }

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_ERRC)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: RRC, State:%d, TAC:%d, CellId:%d", pStatusInfo->rrcStatus.rrcState, pStatusInfo->rrcStatus.tac, pStatusInfo->rrcStatus.cellId);
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_ErrcstatusInfo, P_INFO, "ErrcstatusInfo: %s", (UINT8 *)pStrBuf);
        }

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_EMM)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: EMM, EmmState:\"%d\", EmmMode:\"%d\", PTWMs:%d, EDRXPeriodMs:%d, PsmExT3412TimerS:%d, T3324TimerS:%d, T3346RemainTimeS:%d",
                     pStatusInfo->nasStatus.emmState, pStatusInfo->nasStatus.emmMode, pStatusInfo->nasStatus.ptwMs, pStatusInfo->nasStatus.eDRXPeriodMs,
                     pStatusInfo->nasStatus.psmExT3412TimerS, pStatusInfo->nasStatus.T3324TimerS,pStatusInfo->nasStatus.T3346RemainTimeS);
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_EmmstatusInfo, P_INFO, "EmmstatusInfo %s", (UINT8 *)pStrBuf);
        }

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_PLMN)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: PLMN, PlmnState:\"%d\", PlmnType:\"%d\", SelectPlmn:\"0x%x,0x%x\"",
                     pStatusInfo->nasStatus.plmnState, pStatusInfo->nasStatus.plmnType, pStatusInfo->nasStatus.selectPlmn.mcc, pStatusInfo->nasStatus.selectPlmn.mncWithAddInfo);
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_PlmnstatusInfo, P_INFO, "PlmnstatusInfo %s", (UINT8 *)pStrBuf);
        }

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_ESM)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: ESM, ActBearerNum:%d", pStatusInfo->nasStatus.actBearerNum);
            if (strlen((CHAR*)(pStatusInfo->nasStatus.apnStr)) > 0)
            {
                snprintf(pStrBuf+strlen(pStrBuf), STR_BUFFER_SIZE-strlen(pStrBuf),
                         ", APN:\"%s\"", (CHAR*)(pStatusInfo->nasStatus.apnStr));
            }
            if (pStatusInfo->nasStatus.ipv4Addr.addrType == CMI_IPV4_ADDR)
            {
                snprintf(pStrBuf+strlen(pStrBuf), STR_BUFFER_SIZE-strlen(pStrBuf),
                         ", IPv4:\"%d.%d.%d.%d\"",
                         pStatusInfo->nasStatus.ipv4Addr.addrData[0], pStatusInfo->nasStatus.ipv4Addr.addrData[1],
                         pStatusInfo->nasStatus.ipv4Addr.addrData[2], pStatusInfo->nasStatus.ipv4Addr.addrData[3]);
            }
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_EsmstatusInfo, P_INFO, "EsmstatusInfo %s", (UINT8 *)pStrBuf);
        }

        if (statusType == UE_EXT_STATUS_ALL || statusType == UE_EXT_STATUS_CCM)
        {
            memset(pStrBuf, 0, STR_BUFFER_SIZE);
            snprintf(pStrBuf, STR_BUFFER_SIZE,
                     "+ECSTATUS: CCM, Cfun:%d, IMSI:\"%s\"", pStatusInfo->ccmStatus.cfunValue, pStatusInfo->ccmStatus.imsi);
            ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_CcmstatusInfo, P_INFO, "CcmstatusInfo %s", (UINT8 *)pStrBuf);
        }
    }

    if (pStrBuf != PNULL)
    {
        OsaFreeMemory(&pStrBuf);
    }

    if (pStatusInfo != PNULL)
    {
        OsaFreeMemory(&pStatusInfo);
    }

    return;
}

static void pslibapiAppTask(void *arg)
{
    UINT8   *pApn = PNULL;
    INT8    ret = 0;
    UINT16  tac = 0;
    UINT32  tauTime = 0, activeTime = 0, cellID = 0;
    UINT8   csq = 0, regstate = 0;
    INT8    snr = 0;
    INT8    rsrp = 0;
    time_t  systemTimeS = 0;
    NmAtiSyncRet netInfo = {0}; //84 bytes
    UINT8   sessionId = 0;
    UINT8   dfName[] = {"A00000004374506173732E496F54"};
    //UINT8   cglaCmd[] = {"81F100000E0051010A11223344556677889900"};
    //UINT8   *pCglaResp = PNULL;    //
    //UINT8   cglaRespLen = 0;
    UINT8   imsiNum[16] = {0};
    OsaUtcTimeTValue    timeUtc;       //24 bytes
    BasicCellListInfo   *pBcListInfo = PNULL;

    while (1)
    {
        switch(stateMachine)
        {
            case APP_INIT_STATE:
                //test get UE status information
                UeExtStatusInfoTest();
                osDelay(2000/portTICK_PERIOD_MS);
                break;
            case APP_DEACTIVE_STATE:
                break;
            case APP_IPREADY_STATE:
               {
                    // 0. Get NB version info
                    ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_GetNBVersionInfo, P_INFO, "%s", (const UINT8 *)appGetNBVersionInfo());
                    // 1. Get IMSI
                    appGetImsiNumSync((CHAR *)imsiNum);
                    ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_GetImsiNum, P_INFO, "IMSI = %s", (UINT8 *)imsiNum);
                    // 2. Get TAC and CellID
                    ret = appGetLocationInfoSync(&tac, &cellID);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetLocationInfo, P_INFO, 3, "tac=%d, cellID=%d ret=%d", tac, cellID, ret);
                    // 3. Get SNR
                    ret = appGetSignalInfoSync(&csq, &snr, &rsrp);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetSignalInfo, P_INFO, 3, "SNR=%d csq=%d, rsrp=%d", snr, csq, rsrp);
                    // 5. Get coverage level
                    ret = appGetCELevelSync();
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetCELevel, P_INFO, 1, "Coverage level=%d", ret);
                    // 6. Get TAU info
                    ret = appGetTAUInfoSync(&tauTime, &activeTime);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetTAUInfo, P_INFO, 3, "TAU time=%d active timer=%d ret=%d", tauTime, activeTime, ret);
                    // 7. Get APN info
                    pApn = (UINT8 *)OsaAllocZeroMemory(PS_APN_MAX_SIZE);
                    OsaCheck(pApn != PNULL, PS_APN_MAX_SIZE, 0, 0);
                    ret = appGetAPNSettingSync(0, pApn);
                    ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_GetAPNSetting, P_INFO, "APN = %s", pApn);
                    OsaFreeMemory(&pApn);
                    // 8. Get system time
                    ret = appGetSystemTimeSecsSync(&systemTimeS);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetSystemTimeSecs, P_INFO, 1, "System time=%d", systemTimeS);
                    // 9. Get system time
                    ret = appGetSystemTimeUtcSync(&timeUtc);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetSystemTimeUtc, P_INFO, 2, "System time=0x%x..0x%x", timeUtc.UTCtimer1,timeUtc.UTCtimer2);
                    // 10. Get system time
                    ret = appCheckSystemTimeSync();
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_CheckSystemTime, P_INFO, 1, "System ret=%d", ret);
                    // 11. Get netinfo status
                    ret = appGetNetInfoSync(0, &netInfo);
                    if ( NM_NET_TYPE_IPV4 == netInfo.body.netInfoRet.netifInfo.ipType)
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetNetInfo,P_INFO, 4,"IP:%d.%d.%d.%d", ((UINT8 *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                                                                      ((UINT8 *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                                                                      ((UINT8 *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                                                                      ((UINT8 *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);

                    //12. open SIM logical channel
                    ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_CCHO_1, P_INFO, "Open SIM logical channel, DF name: %s", dfName);
                    ret = appSetSimLogicalChannelOpenSync(&sessionId, dfName);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_CCHO_2, P_INFO, 1, "retrun sessionId: %d", sessionId);

                    //13. Generic SIM logical channel access
                    //ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_CGLA_1, P_INFO, 1, "Generic SIM logical channel %d access", sessionId);
                    //ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_CGLA_2, P_INFO, "CGLA command: %s", cglaCmd);
                    //ret = appSetSimGenLogicalChannelAccessSync(sessionId, cglaCmd, 38, cglaResp, &cglaRespLen);
                    //ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_CGLA_3, P_INFO, 1, "cgla return reponse length: %d", cglaRespLen);
                    //ECOMM_STRING(UNILOG_PLA_STRING, pslibapiAppTask_CGLA_4, P_INFO, "cgla return reponse: %s", cglaResp);

                    //14. close SIM logical channel
                    ret = appSetSimLogicalChannelCloseSync(sessionId);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_CCHC, P_INFO, 2, "close logical channel %d, return value %d", sessionId, ret);

                    //15. get basic cell info
                    pBcListInfo = (BasicCellListInfo *)OsaAllocZeroMemory(sizeof(BasicCellListInfo));
                    OsaCheck(pBcListInfo != PNULL, sizeof(BasicCellListInfo), 0, 0);
                    ret = appGetECBCInfoSync(GET_NEIGHBER_CELL_ID_INFO, 2, 10, pBcListInfo);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetECBCInfo_1, P_INFO, 2, "sCellPresent %d, nCellNum %d", pBcListInfo->sCellPresent, pBcListInfo->nCellNum);
                    ECOMM_DUMP(UNILOG_PLA_DUMP, pslibapiAppTask_GetECBCInfo_2, P_INFO, "sCellInfo: ", sizeof(CmiDevSCellBasicInfo), (UINT8 *)(&(pBcListInfo->sCellInfo)));
                    ECOMM_DUMP(UNILOG_PLA_DUMP, pslibapiAppTask_GetECBCInfo_3, P_INFO, "nCellList: ", sizeof(CmiDevNCellBasicInfo)*4, (UINT8 *)(pBcListInfo->nCellList));
                    OsaFreeMemory(&pBcListInfo);

                    //test get UE status information
                    UeExtStatusInfoTest();
                    stateMachine = APP_IDLE_STATE;
                    break;
                }
            case APP_REPORT_STATE:
                {
                    break;
                }
             case  APP_IDLE_STATE:
                {
                    appGetCeregStateSync(&regstate);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetCeregState, P_INFO, 1, "CEREG state=%d", regstate);
                    osDelay(5000/portTICK_PERIOD_MS);
                    ret = appGetSignalInfoSync(&csq, &snr, &rsrp);
                    ECOMM_TRACE(UNILOG_PLA_APP, pslibapiAppTask_GetSignalInfo, P_INFO, 3, "SNR=%d csq=%d, rsrp=%d", snr, csq, rsrp);
                    //test get UE status information
                    UeExtStatusInfoTest();
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
    registerPSEventCallback(NB_GROUP_ALL_MASK, pslibPSUrcCallback);

    memset(&task_attr,0,sizeof(task_attr));
    memset(appTaskStack, 0xA5, INIT_TASK_STACK_SIZE);
    task_attr.name = "app";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(pslibapiAppTask, NULL, &task_attr);

}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void) {

    BSP_CommonInit();
    osKernelInitialize();
    // Apply own right power policy according to design
    slpManSetPmuSleepMode(true, SLP_HIB_STATE, false);
    slpManApplyPlatVoteHandle("pslibapi",&pslibapiSlpHandler);

    slpManPlatVoteDisableSleep(pslibapiSlpHandler, SLP_SLP2_STATE);

    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);
}

