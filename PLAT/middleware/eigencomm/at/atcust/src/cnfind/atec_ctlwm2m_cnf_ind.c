/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_onenet.c
*
*  Description: Process onenet related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "FreeRTOS.h"
#include "semphr.h"
#include "ctiot_lwm2m_sdk.h"
#include "er-coap-13.h"
#include "ctiot_NV_data.h"
#include "ec_ctlwm2m_api.h"
#include "at_util.h"
#include "at_ctlwm2m_task.h"
#ifdef WITH_FOTA_RESUME
#include "ctiot_fota.h"
#endif
thread_handle_t nInittid;
SemaphoreHandle_t networkReadySemph = NULL;
extern osMutexId_t athdlTaskMutex;
extern osMutexId_t startThreadMutex;
#ifdef WITH_FOTA_RESUME
extern ctiotFotaManage fotaStateManagement;
#endif
void prv_athandleRegistrationUpdateReply(lwm2m_transaction_t * transacP,void * message);

static INT32 prv_PSTriggerEvent_callback(urcID_t eventID, void *param, UINT32 paramLen)
{
    ctiot_funcv1_context_t* pContext=ctiot_funcv1_get_context();
    ctiot_funcv1_chip_info_ptr pChipInfo=pContext->chipInfo;
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;
    NmAtiNetifInfo *netif = NULL;
#ifdef WITH_FOTA_RESUME
    ctiot_funcv1_status_t atStatus[1] = {0};
#endif
    switch(eventID)
    {
        case NB_URC_ID_SIM_READY://SIM 卡探测成功事�?
        {
            imsi = (CmiSimImsiStr *)param;
            memset(pChipInfo->cImsi,0,20);
            memcpy(pChipInfo->cImsi,imsi->contents, imsi->length);
            break;
        }
        case NB_URC_ID_SIM_REMOVED://SIM 卡被移除事件
        {
            memset(pChipInfo->cImsi,0,20);
            pChipInfo->cState = NETWORK_UNAVAILABLE;//无卡
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED://默认 BEARER 使能事件
        {
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED://默认 BEARER 去使能事�?
        {
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED://网络注册状�?�变化事�?包含 CellID 信息)
        {
            cereg = (CmiPsCeregInd *)param;
            pChipInfo->cCellID=cereg->celId;
            break;
        }
        case NB_URC_ID_PS_NETINFO://网络连接状�?�变化事�?
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                pChipInfo->cState = NETWORK_CONNECTED;
                xSemaphoreGive(networkReadySemph);
                ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_0_0, P_INFO, 2, "network connected!!! regFlag(%d), logstat(%d)",pContext->regFlag,pContext->loginStatus);
                
#ifdef  FEATURE_REF_AT_ENABLE
                // when AT+QREGSWT=1 need show REGISTERNOTIFY after reboot
                if(pContext->regFlag == 0 && pContext->loginStatus == UE_NOT_LOGINED)
                {
                    ctiot_funcv1_status_t notify[1]={0};
                    notify->baseInfo=ct_lwm2m_strdup(REGNOTIFY);
                    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, notify, 0);
                }
#ifdef WITH_FOTA_RESUME
                if(pContext->bootFlag == BOOT_FOTA_BREAK_RESUME){
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_0_2, P_INFO, 0, "FOTA is downloading fail to register again for break resume");
                    if(pContext->lwm2mContext != NULL){
                        ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_0_3, P_INFO, 0, "clean session first");
                        ctiot_funcv1_clear_session(FALSE); 
                    }else{
                        pContext->loginStatus = UE_NO_IP;
                    }
                }
#endif              
                if((pContext->loginStatus == UE_NO_IP) || (pContext->regFlag == 1 && (pContext->loginStatus != UE_LOGINED || pContext->loginStatus != UE_LOGINING)))
                {
                    //re-register to platform
                    if(appGetImeiNumSync(pContext->chipInfo->cImei) != NBStatusSuccess){
                        ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_0_1, P_ERROR, 0, "appGetImeiNumSync error");
                    }
                    pContext->ipReadyRereg = TRUE;
                    ctiot_funcv1_session_init(pContext);
                }
#ifdef WITH_FOTA_RESUME
                if (pContext->bootFlag == BOOT_FOTA_BREAK_RESUME)
                {
                    if(pContext->inBreakFOTA == 0){
                        pContext->inBreakFOTA = 1;
                        ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_0_4, P_INFO, 0, "fota in breakpoint resume begin downing");
                        atStatus->baseInfo=ct_lwm2m_strdup(FOTA0);
                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, atStatus, 0);
                        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_BEGIN, NULL, 0);
                        fota_resume();//continue fota download
                    }
                }
#endif                
#else
#ifdef WITH_FOTA_RESUME
                if (pContext->bootFlag == BOOT_FOTA_BREAK_RESUME)
                {
                    if(pContext->inBreakFOTA == 0){
                        pContext->inBreakFOTA = 1;
                        ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_0_5, P_INFO, 0, "fota in breakpoint resume begin downing");
                        //ctiot_funcv1_fota_state_changed();
                        atStatus->baseInfo=ct_lwm2m_strdup(FOTA0);
                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, atStatus, 0);
                        ct_lwm2m_free(atStatus->baseInfo);
                        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_BEGIN, NULL, 0);
                        fota_resume();//continue fota download
                    }
                }
#endif                
                if(netif->ipType == NM_NET_TYPE_IPV4)
                {
                    ip4_addr_t ip4addr;
                    ip4addr.addr=netif->ipv4Info.ipv4Addr.addr;
                    uint8_t* tmpaddr=(uint8_t *)&ip4addr.addr;
                    char ip4[CTIOT_MAX_IP_LEN]={0};
                    sprintf(ip4,"%u.%u.%u.%u",tmpaddr[0],tmpaddr[1],tmpaddr[2],tmpaddr[3]);
                    if(strcmp(ip4,pContext->localIP)!=0)
                    {
                        memset(pContext->localIP,0,CTIOT_MAX_IP_LEN);
                        strcpy(pContext->localIP,ip4);
                    }
                }
#endif
            }
            else if (netif->netStatus == NM_NETIF_OOS)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_1, P_INFO, 0, "URCCallBack:PSIF network oos");
                pChipInfo->cState = NETWORK_UNAVAILABLE;
            }
            else if (netif->netStatus == NM_NO_NETIF_OR_DEACTIVATED || netif->netStatus == NM_NO_NETIF_NOT_DIAL)
            {
                pChipInfo->cState =NETWORK_UNCONNECTED;//通信故障
                ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_2, P_INFO, 1, "URCCallBack:PSIF network deactive logstat=%d",pContext->loginStatus);
#ifdef  FEATURE_REF_AT_ENABLE
                if(pContext->loginStatus == UE_LOGINED)//if in logined status change to NO_UE_IP status
                {
#ifdef WITH_FOTA_RESUME
                    if(fotaStateManagement.fotaState == FOTA_STATE_DOWNLOADING){
                        if(pContext->bootFlag == BOOT_FOTA_BREAK_RESUME && pContext->lwm2mContext != NULL){
                            ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_2_1, P_INFO, 0, "FOTA is in break resume status, clear session");
                            ctiot_funcv1_clear_session(FALSE); 
                        }else{
                            ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_2_2, P_INFO, 0, "FOTA is still try download not in break resume status");
                        }
                    }
                    else
#endif
                    {
                        ctiot_funcv1_clear_session(TRUE);      
                    }
                }
#endif
            }
            break;
        }
        case NB_URC_ID_MM_CERES_CHANGED://网络覆盖等级变化事件
        {
            /*
             * 0 No Coverage Enhancement in the serving cell
             * 1 Coverage Enhancement level 0
             * 2 Coverage Enhancement level 1
             * 3 Coverage Enhancement level 2
             * 4 Coverage Enhancement level 3
            */
            pChipInfo->cSignalLevel = (ctiot_funcv1_signal_level_e)(*(uint8_t *)param);
            ctiot_setCoapAckTimeout(pChipInfo->cSignalLevel);
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_PSTriggerEvent_callback_3, P_INFO, 1, "URCCallBack:NB_URC_ID_MM_CERES_CHANGED celevel=%d", pChipInfo->cSignalLevel);
            break;
        }
        case NB_URC_ID_MM_SIGQ://
        {
            break;
        }
        case NB_URC_ID_MM_TAU_EXPIRED://终端 TAU 到期通知事件
        {
            break;
        }
        case NB_URC_ID_MM_BLOCK_STATE_CHANGED://终端拥塞控制事件
        {
            //pChipInfo->cState= 3;
            uint8_t timerState=*(uint8_t *)param;
            if(timerState==0)
            {
                pChipInfo->cTrafficFlag=1;
                pChipInfo->cTrafficTime=ct_lwm2m_gettime();
                pChipInfo->cState=NETWORK_JAM;
            }
            else
            {
                pChipInfo->cTrafficFlag=0;
                pChipInfo->cState=NETWORK_CONNECTED;
            }
            break;
        }

        default:break;
    }
    return 0;
}

static void prv_set_context_default()
{
    ctiot_funcv1_context_t* pContext;
    pContext=ctiot_funcv1_get_context();
    pContext->securityMode       = NO_SEC_MODE;
    pContext->idAuthMode         = AUTHMODE_IMEI;
    pContext->onUqMode           = DISABLE_UQ_MODE;
    pContext->autoHeartBeat      = AUTO_HEARTBEAT;
    pContext->wakeupNotify       = NOTICE_MCU;
    pContext->protocolMode       = PROTOCOL_MODE_NORMAL;
    pContext->bootFlag           = BOOT_NOT_LOAD;
    pContext->reqhandle          = 0x100;
    pContext->nnmiFlag           = 1;
#ifdef  FEATURE_REF_AT_ENABLE
    pContext->nsmiFlag           = 0;
    pContext->regFlag            = 1;
    pContext->lifetime           = 86400;
#endif
}

static void prv_init_thread(void* arg)
{
    //uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_context_t* pContext=(ctiot_funcv1_context_t*)arg;
    
    if(pContext==NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(appGetImeiNumSync(pContext->chipInfo->cImei) != NBStatusSuccess)
    {
        //result = CTIOT_EB_OTHER;
        ECOMM_TRACE(UNILOG_CTLWM2M, prv_init_thread_2, P_ERROR, 0, "appGetImeiNumSync error");
        //处理imei获取错误
    }

    memset(pContext->chipInfo->cSDKVersion,0,30);
    memset(pContext->chipInfo->cFirmwareVersion,0,20);
    strncpy(pContext->chipInfo->cSDKVersion,"EC_SW_V001.000.001",30);
    strncpy(pContext->chipInfo->cFirmwareVersion,"EC_HW_V1.0",20);

    if (pContext->protocolMode == PROTOCOL_MODE_ENHANCE)
    {
        if(appGetAPNSettingSync(0,(UINT8 *)(pContext->chipInfo->cApn))!=NBStatusSuccess)
        {
            //result = CTIOT_EB_OTHER;
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_init_thread_3, P_ERROR, 0, "appGetAPNSettingSync error");
        }
    }
    
    NmAtiSyncRet netStatus;
    appGetNetInfoSync(0, &netStatus);
    if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
    {
       ECOMM_TRACE(UNILOG_CTLWM2M, prv_init_thread_4_1, P_INFO, 0, "network connected, call ctiot_session_init");
       ctiot_funcv1_session_init(pContext);
    }
    else
    {
        //wait network eventcallback
        ECOMM_TRACE(UNILOG_CTLWM2M, prv_init_thread_4_2, P_INFO, 0, "wait network connect");
        pContext->chipInfo->cState = NETWORK_UNCONNECTED;
        
        if (xSemaphoreTake(networkReadySemph, portMAX_DELAY) == pdTRUE) {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_init_thread_4_3, P_INFO, 0, "network ready, call ctiot_session_init");
            ctiot_funcv1_session_init(pContext);
        }
    }

    thread_exit(NULL);
}

void prv_print_at_rsp(uint32_t reqhandle, char* at_str)
{
    applSendCmsInd(reqhandle, APPL_CTLWM2M, APPL_CT_IND, strlen(at_str)+1, at_str);
}


#if CTLWM2M_AUTO_UPDATE_ENABLE

static bool ctLifeTimerExpFlag = false;

static void ctlwm2m_lifeTimeStart(uint32_t timeout_second)
{
    if(timeout_second == 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_lifeTimeStart_0, P_VALUE, 0, "CTLWM2M LifeTime = 0");
        ctLifeTimerExpFlag = false;
        return;
    }
    if(timeout_second >= CTLWM2M_AUTO_UPDATE_THD)
    {
        slpManDeepSlpTimerStart(CTLWM2M_LIFETIMEOUT_ID, timeout_second*1000);

        ctLifeTimerExpFlag = false;

        ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_lifeTimeStart_1, P_VALUE, 2, "CTLWM2M LifeTime start Deepsleep Timer Id = %u, Duration = %d second", CTLWM2M_LIFETIMEOUT_ID, timeout_second);
    }
}

static void ctlwm2m_lifeTimeStop(void)
{
    slpManDeepSlpTimerDel(CTLWM2M_LIFETIMEOUT_ID);
    
    ctLifeTimerExpFlag = false;
    
    ECOMM_TRACE(UNILOG_CTLWM2M, ctlwm2m_lifeTimeStop_1, P_VALUE, 0, "CTLWM2M LifeTime Stop");
}

static bool ctlwm2m_DeepSlpTimerHasExp(void)
{
    return ctLifeTimerExpFlag;
}

void ctlwm2m_lifeTimeExpCallback(void)
{
    ctLifeTimerExpFlag = true;
    
    ctiot_funcv1_sleep_vote(SYSTEM_STATUS_BUSY);
}

void ct_send_loop_callback(ctiot_funcv1_context_t* pContext)
{
    uint16_t result;
    
    if(ctlwm2m_DeepSlpTimerHasExp() == true)            // send an auto update
    {
        uint16_t msgId = 0;
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_send_loop_callback_1, P_WARNING, 1, "ct send task auto-update, LifeTime = %d", pContext->lifetime);

        result = ctiot_funcv1_update_reg( NULL, &msgId, false, prv_athandleRegistrationUpdateReply);
        if (result != CTIOT_NB_SUCCESS){
            ECOMM_TRACE(UNILOG_CTLWM2M, ct_send_loop_callback_2, P_WARNING, 1, "ctiot_funcv1_update_reg failed return = %d", result);
            ctiot_funcv1_enable_sleepmode();
        }

        ctlwm2m_lifeTimeStart(pContext->lifetime*9/10);
    }
}


#endif


static void ctiot_event_handler(module_type_t type, INT32 code, const void* arg, INT32 arg_len)
{
    switch (type)
    {
        case MODULE_LWM2M:
        {
            if (code == OBSERVE_SUBSCRIBE)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_1, P_INFO, 0, "observe subscribed save context and enable sleep");
                ctiot_funcv1_save_context();
                ctiot_funcv1_enable_sleepmode();
            }
            else if ((code == SEND_CON_DONE) || (code == SEND_FAILED))
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_2, P_INFO, 0, "send done or send failed enable sleep");
                ctiot_funcv1_enable_sleepmode();
            }
            else if(code == FOTA_BEGIN)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_3, P_INFO, 0, "begin FOTA disable sleep");
                ctiot_funcv1_disable_sleepmode();
            }
            else if(code == FOTA_DOWNLOAD_FAIL || code == FOTA_UPDATE_FAIL)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_3_1, P_INFO, 0, "FOTA download or update fail enable sleep");
                ctiot_funcv1_enable_sleepmode();
            }
            else if(code == FOTA_OVER)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_4, P_INFO, 0, "FOTA complete enble sleep");
                ctiot_funcv1_save_context();
                ctiot_funcv1_enable_sleepmode();
            }

            else if(code == REG_SUCCESS)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_5, P_INFO, 0, "CT event, reg success");
#if CTLWM2M_AUTO_UPDATE_ENABLE
                ctiot_funcv1_context_t* pTmpContext=ctiot_funcv1_get_context();
                if(pTmpContext != NULL){
                    uint32_t updatetime = (pTmpContext->lifetime * 9) / 10;
                    ctlwm2m_lifeTimeStart(updatetime);
                }
#endif
            }
            else if(code == DEREG_SUCCESS)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_event_handler_6, P_INFO, 0, "CT event, dereg success");
#if CTLWM2M_AUTO_UPDATE_ENABLE
                ctlwm2m_lifeTimeStop();
#endif
            }
            
            
            break;
        }
    }
}

void prv_athandleRegistrationUpdateReply(lwm2m_transaction_t * transacP,
                                              void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    lwm2m_server_t * targetP = (lwm2m_server_t *)(transacP->userData);
    ctiot_funcv1_status_t atStatus[1]={0};
    ctiot_funcv1_context_t* pTmpContext=ctiot_funcv1_get_context();
    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        targetP->registration = ct_lwm2m_gettime();
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
            atStatus->baseInfo=ct_lwm2m_strdup(SU0);
            atStatus->extraInfo=&transacP->mID;
#ifndef  FEATURE_REF_AT_ENABLE
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
#else
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL,2);
#endif
            ct_lwm2m_free(atStatus->baseInfo);
        }
        else
        {
            if(packet == NULL)//update timeout
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU2);
                atStatus->extraInfo=&transacP->mID;
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                targetP->status = STATE_REG_FAILED;
            }
            if(packet->code == COAP_403_FORBIDDEN/*COAP_403*/)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU4);
                atStatus->extraInfo=&transacP->mID;
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                targetP->status = STATE_REG_FAILED;
            }
            else if(packet->code == COAP_400_BAD_REQUEST/*COAP_400*/)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU3);
                atStatus->extraInfo=&transacP->mID;
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                targetP->status = STATE_REG_FAILED;
            }
            else if(packet->code == COAP_404_NOT_FOUND || packet->code == COAP_401_UNAUTHORIZED/*COAP_404 or COAP_401*/)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU1);
                atStatus->extraInfo=&transacP->mID;
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                targetP->status = STATE_REG_FAILED;
            }
            else
            {
                targetP->status = STATE_REG_FAILED;
            }
        }
        if(targetP->status == STATE_REGISTERED)
        {
            pTmpContext->loginStatus = UE_LOGINED;
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_athandleRegistrationUpdateReply_2, P_INFO, 0, "Login Status change to logined");
        }
        else if(targetP->status == STATE_REG_FAILED)
        {
            pTmpContext->loginStatus = UE_LOG_FAILED;
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_athandleRegistrationUpdateReply_3, P_INFO, 0, "Login Status change to unlogined");
        }
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_athandleRegistrationUpdateReply_4, P_INFO, 0, "update over enable sleep");
    ctiot_funcv1_save_context();
    ctiot_funcv1_enable_sleepmode();
}

void  ctiotEngineInit()
{
    int32_t  ret;
    ctiot_funcv1_context_t* pContext;

    if(networkReadySemph == NULL){
        networkReadySemph = xSemaphoreCreateBinary();
    }
    
    registerPSEventCallback(NB_GROUP_ALL_MASK,prv_PSTriggerEvent_callback);
    ctiot_funcv1_register_at_handler(prv_print_at_rsp);
    ctiot_funcv1_register_event_handler(ctiot_event_handler);
    ctiot_funcv1_init_sleep_handler();
    pContext=ctiot_funcv1_get_context();

    ret=f2c_encode_params(pContext);
    if( ret != NV_OK )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_0, P_INFO, 0, "f2c_encode_params fail,use default params and clear context");
        c2f_clean_params(NULL);
        c2f_clean_context(NULL);
        prv_set_context_default();
        NV_reinit_cache();
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_1, P_INFO, 0, "f2c_encode_params ok");
        uint8_t cache_boot_flag = 0;
        ret = cache_get_bootflag(&cache_boot_flag);
        if( ret == NV_OK )
        {
            pContext->bootFlag = (ctiot_funcv1_boot_flag_e)cache_boot_flag;
#ifdef  FEATURE_REF_AT_ENABLE
            cache_get_loginstatus(&pContext->loginStatus);
#endif
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_2_0, P_INFO, 3, "get form nv bootflag=%d, regFlag=%d, logstatus=%d",cache_boot_flag, pContext->regFlag, pContext->loginStatus);
        }
        else
        {
            c2f_clean_context(NULL);
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_2_1, P_INFO, 0, "can't get bootFlag from nv, clean context");
        }
        
        slpManSlpState_t slpState = slpManGetLastSlpState();
        if(slpState == SLP_HIB_STATE || slpState == SLP_SLP2_STATE)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_3, P_INFO, 0, "recover from HIB and sleep2");
            pContext->restoreFlag = 1;
        }
        else
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_4, P_INFO, 0, "system reboot");
#ifdef  FEATURE_REF_AT_ENABLE
            BOOL saveParams = FALSE;
            // restore nnmi flag to 1(default) nsmi every reboot
            if(pContext->nnmiFlag != 1 || pContext->nsmiFlag != 0 ){
                pContext->nnmiFlag = 1;
                pContext->nsmiFlag = 0;
                saveParams = TRUE;
            }
            if(pContext->loginStatus != UE_NOT_LOGINED && cache_boot_flag != BOOT_FOTA_REBOOT
#ifdef WITH_FOTA_RESUME
                && cache_boot_flag != BOOT_FOTA_BREAK_RESUME)//when bc28 reset goto no reg status
#else                
                )//when bc28 reset goto no reg status
#endif               
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_4_4, P_INFO, 0, "bc28 clear reg status");
                pContext->loginStatus = UE_NOT_LOGINED;
                pContext->bootFlag = BOOT_NOT_LOAD;
                c2f_encode_context(pContext);
                slpManFlushUsrNVMem();
            }
            if(pContext->regFlag == 2)
            {
                pContext->regFlag = 3;
                saveParams = TRUE;
            }
            if(saveParams)
            {
                c2f_encode_params(pContext);
            }
#else //ctm2m if reboot clear
            if (ret == NV_OK)
            {
                if(cache_boot_flag == BOOT_LOCAL_BOOTUP)
                {
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_4_1, P_INFO, 0, "bootflag=BOOT_LOCAL_BOOTUP clear context");
                    c2f_clean_context(NULL);
                    c2f_clean_params(NULL);
                    prv_set_context_default();
                }else if(cache_boot_flag == BOOT_FOTA_REBOOT){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_4_2, P_INFO, 0, "bootflag=BOOT_FOTA_REBOOT need to restore ctlwm2m");
                }else{
                    if(pContext->pskid != 0 || pContext->psk != 0){
                        ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_4_3, P_INFO, 0, "before has set psk, clear it");
                        pContext->psk = NULL;
                        pContext->pskid = NULL;
                        pContext->pskLen = 0;
                        pContext->securityMode = NO_SEC_MODE;
                        c2f_clean_params(NULL);
                    }
                }
                if(pContext->nnmiFlag != 1){//restore nnmi flag to 1(default) every reboot
                    pContext->nnmiFlag = 1;
                    c2f_encode_params(pContext);
                }
            }
#endif
        }
    }
    if(athdlTaskMutex == NULL){
        athdlTaskMutex = osMutexNew(NULL);
    }
    if(startThreadMutex == NULL){
        startThreadMutex = osMutexNew(NULL);
    }
    if(pContext->bootFlag == BOOT_LOCAL_BOOTUP || pContext->bootFlag == BOOT_FOTA_REBOOT || pContext->regFlag == 1
#ifdef WITH_FOTA_RESUME
        || pContext->bootFlag == BOOT_FOTA_BREAK_RESUME)
#else
        )
#endif        
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_5, P_INFO, 2,"need to restore ctlwm2m. bootFlag:%d regFlag:%d",pContext->bootFlag, pContext->regFlag);
        ret = (int32_t)thread_create(&nInittid, NULL, prv_init_thread, (void*)pContext, LWM2M_TEMP_INIT_TASK);
        if (ret != 0)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_5_1, P_ERROR, 0, "create ctiot temp init task failed");
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiotEngineInit_6, P_INFO, 0, "no need to restore ctlwm2m");
    }
    

}


uint16_t ctiot_at_set_mod(ctiot_funcv1_context_t* pContext, uint8_t modeId, char modeValue)
{
    switch(modeId)
    {
        case 1:
            return ctiot_funcv1_set_mod(pContext,modeValue,(char)-1,(char)-1,(char)-1,(char)-1,(char)-1);
        case 2:
            return ctiot_funcv1_set_mod(pContext,(char)-1,modeValue,(char)-1,(char)-1,(char)-1,(char)-1);
        case 3:
            return ctiot_funcv1_set_mod(pContext,(char)-1,(char)-1,modeValue,(char)-1,(char)-1,(char)-1);
        case 4:
            return ctiot_funcv1_set_mod(pContext,(char)-1,(char)-1,(char)-1,modeValue,(char)-1,(char)-1);
        case 5:
            return ctiot_funcv1_set_mod(pContext,(char)-1,(char)-1,(char)-1,(char)-1,modeValue,(char)-1);
        case 6:
            return ctiot_funcv1_set_mod(pContext,(char)-1,(char)-1,(char)-1,(char)-1,(char)-1,modeValue);
        default:
            return CTIOT_ED_MODE_ERROR;
    }
}


uint16_t ctiot_at_reg(ctiot_funcv1_context_t* pTContext)
{
    uint16_t result;
    ctiot_funcv1_disable_sleepmode();
    result = ctiot_funcv1_reg(pTContext);
    if (result != CTIOT_NB_SUCCESS)
        ctiot_funcv1_enable_sleepmode();
    return result;
}

uint16_t ctiot_at_dereg(ctiot_funcv1_context_t* pContext)
{
    uint16_t result;
    ctiot_funcv1_disable_sleepmode();
    result = ctiot_funcv1_dereg(pContext, prv_PSTriggerEvent_callback);
    if (result != CTIOT_NB_SUCCESS)
        ctiot_funcv1_enable_sleepmode();
    return result;
}

uint16_t ctiot_at_update_reg(ctiot_funcv1_context_t* pTContext,uint16_t*msgId,bool withObjects)
{
    uint16_t result;
    ctiot_funcv1_disable_sleepmode();
    result = ctiot_funcv1_update_reg( pTContext, msgId,withObjects, prv_athandleRegistrationUpdateReply);
    if (result != CTIOT_NB_SUCCESS)
        ctiot_funcv1_enable_sleepmode();
    return result;
}

uint16_t ctiot_at_send(ctiot_funcv1_context_t* pTContext,char* data,ctiot_funcv1_send_mode_e sendMode, UINT8 seqNum)
{
    uint16_t result;
    ctiot_funcv1_disable_sleepmode();
    result = ctiot_funcv1_send( pTContext, data, sendMode, ctiot_funcv1_notify_nb_info, seqNum);
    if (result != CTIOT_NB_SUCCESS)
        ctiot_funcv1_enable_sleepmode();
    return result;
}


uint16_t ctiot_at_cmdrsp(ctiot_funcv1_context_t* pTContext,uint16_t msgId,char* token,uint16_t responseCode,char* uriStr,uint8_t observe,uint8_t dataFormat,char* dataS)
{
    uint16_t result;
    ctiot_funcv1_disable_sleepmode();
    result = ctiot_funcv1_cmdrsp( pTContext, msgId, token, responseCode, uriStr, observe, dataFormat, dataS);
    if (result != CTIOT_NB_SUCCESS)
        ctiot_funcv1_enable_sleepmode();
    return result;
}

uint16_t ctiot_at_dfota_mode(uint8_t mode)
{
    extern void prv_firmware_fota_start(uint8_t mode);
    extern void prv_firmware_fota_udate(uint8_t mode);

    if(mode == 0 || mode == 1)
        return ctiot_funcv1_set_dfota_mode(NULL, mode);
    // notify to start downloading
    else if (mode == 2)
    {
        prv_firmware_fota_start(1);
    }
    // notify to cancel downloading
    else if (mode == 3)
    {
        prv_firmware_fota_start(0);
    }
    // notify to start updating
    else if (mode == 4)
    {
        prv_firmware_fota_udate(1);
    }
    // notify to cancel updating
    else if (mode == 5)
    {
        prv_firmware_fota_udate(0);
    }
    return CTIOT_NB_SUCCESS;
}

ctiot_funcv1_boot_flag_e ctiot_at_get_bootflag()
{
   ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();
   return pContext->bootFlag;
}

ctiot_funcv1_login_status_e ctiot_at_get_loginstatus()
{
   ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();
   return pContext->loginStatus;
}

#define _DEFINE_AT_CNF_IND_INTERFACE_

CmsRetId ctiotDeregCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiotDeregCnf_0, P_INFO, 1, "rc : %d", rc);
    ctlwm2m_cnf_msg     *pConf = (ctlwm2m_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CTM2M_ERROR, pConf->ret, NULL);
    }
    return ret;
}

CmsRetId ctiotUpdateCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiotUpdateCnf_0, P_INFO, 1, "rc : %d", rc);
    ctlwm2m_cnf_msg     *pConf = (ctlwm2m_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CTM2M_ERROR, pConf->ret, NULL);
    }
    return ret;
}

CmsRetId ctiotSendCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiotSendCnf_0, P_INFO, 1, "rc : %d", rc);
    ctlwm2m_cnf_msg     *pConf = (ctlwm2m_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CTM2M_ERROR, pConf->ret, NULL);
    }
    return ret;
}


CmsRetId ctiot_at_ind(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_at_ind_1, P_INFO, 0, "recv indication");

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_at_ind_2, P_INFO, 0, "invalid paras");
       return ret;
    }
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_at_ind_1_1, P_INFO, 2, "reqhandle=0x%x, chanId=%d",reqhandle,chanId);
    if(chanId >= CMS_CHAN_1 && chanId < CMS_CHAN_NUM){
        ret = atcURC(chanId, (CHAR*)paras);
    }else{
        ret = atcURC(CMS_CHAN_DEFAULT, (CHAR*)paras);
    }
    return ret;
}

/******************************************************************************
 * ctCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList ctCmsCnfHdrList[] =
{
    {APPL_CT_DEREG_CNF,            ctiotDeregCnf},
    {APPL_CT_UPDATE_CNF,           ctiotUpdateCnf},
    {APPL_CT_SEND_CNF,             ctiotSendCnf},
    
    {APPL_CT_PRIM_ID_END, PNULL}    /* this should be the last */
};

/******************************************************************************
 * onenetCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList ctCmsIndHdrList[] =
{
    {APPL_CT_IND, ctiot_at_ind},
    {APPL_CT_PRIM_ID_END, PNULL}    /* this should be the last */
};

void atApplCtProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (ctCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (ctCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = ctCmsCnfHdrList[hdrIndex].applCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (applCnfHdr != PNULL)
    {
        (*applCnfHdr)(pCmsCnf->header.srcHandler, pCmsCnf->header.rc, pCmsCnf->body);
    }
    else
    {
        OsaDebugBegin(applCnfHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}


void atApplCtProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);

    while (ctCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (ctCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = ctCmsIndHdrList[hdrIndex].applIndHdr;
            break;
        }
        hdrIndex++;
    }

    if (applIndHdr != PNULL)
    {
        (*applIndHdr)(pCmsInd->header.reqHandler, pCmsInd->body);
    }
    else
    {
        OsaDebugBegin(applIndHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}










