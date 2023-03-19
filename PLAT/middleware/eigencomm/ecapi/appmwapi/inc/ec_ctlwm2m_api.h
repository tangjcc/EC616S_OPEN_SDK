/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ec_ctlwm2m_api.h
 * Description:  API interface implementation header file for ChinaNet lwm2m
 * History:      Rev1.0   2018-11-27
 *
 ****************************************************************************/
#ifndef __EC_CTLWM2M_API_H__
#define __EC_CTLWM2M_API_H__
#include "cms_util.h"
#include "port/platform.h"
#include "ps_event_callback.h"
#include "ctiot_lwm2m_sdk.h"

#define MAX_PSKID_LEN         (32)
#define MAX_PSK_LEN           (32)
#define MAX_SERVER_LEN        (256)
#define MAX_IMSI_LEN          (15)

#define ENDPOINT_NAME_SIZE 150

/*
 * APPL SGID: APPL_CTLWM2M, related PRIM ID
*/
typedef enum applCtPrimId_Enum
{
    APPL_CT_PRIM_ID_BASE = 0,
    APPL_CT_DEREG_CNF,
    APPL_CT_SEND_CNF,
    APPL_CT_UPDATE_CNF,
    APPL_CT_IND,
    APPL_CT_PRIM_ID_END = 0xFF
}applCtPrimId;

typedef INT32 (*ps_event_callback_t) (urcID_t eventID, void *param, UINT32 paramLen);

uint16_t ctiot_funcv1_set_dfota_mode(ctiot_funcv1_context_t* pTContext, uint8_t mode);

void ctiot_funcv1_save_context(void);
uint16_t ctiot_funcv1_set_mod(ctiot_funcv1_context_t* pTContext, char idAuthMode, char autoTAUUpdate, char onUQMode, char autoHeartBeat, char wakeupNotify, char protocolMode);
uint16_t ctiot_funcv1_get_mod(ctiot_funcv1_context_t* pContext,uint8_t* idAuthMode,uint8_t* autoTAUUpdate,uint8_t* onUQMode,uint8_t* autoHeartBeat,uint8_t* wakeupNotify, uint8_t* protocolMode);
uint16_t ctiot_funcv1_set_pm(ctiot_funcv1_context_t* pTContext,char* serverIP,uint16_t port, int32_t lifeTime, uint32_t reqhandle);
uint16_t ctiot_funcv1_get_pm(ctiot_funcv1_context_t* pContext,char* serverIP,uint16_t* port,uint32_t* lifeTime);
uint16_t ctiot_funcv1_session_init(ctiot_funcv1_context_t* pContext);
uint16_t ctiot_funcv1_reg(ctiot_funcv1_context_t* pTContext);
uint16_t ctiot_funcv1_dereg(ctiot_funcv1_context_t* pContext, ps_event_callback_t eventcallback);
ctiot_funcv1_NM_status_e ctiot_funcv1_get_reg_status(ctiot_funcv1_context_t* pContext);
uint16_t ctiot_funcv1_update_reg(ctiot_funcv1_context_t* pTContext,uint16_t*msgId,bool withObjects, lwm2m_transaction_callback_t updateCallback);
uint16_t ctiot_funcv1_cmdrsp(ctiot_funcv1_context_t* pTContext,uint16_t msgId,char* token,uint16_t responseCode,char* uriStr,uint8_t observe,uint8_t dataFormat,char* dataS);
uint16_t ctiot_funcv1_notify(ctiot_funcv1_context_t* pTContext,char* token,char* uriStr,uint8_t dataFormat,char* dataS,uint8_t sendMode);
uint16_t ctiot_funcv1_get_status(ctiot_funcv1_context_t* pTContext,uint8_t queryType,uint16_t msgId);
uint16_t ctiot_funcv1_send(ctiot_funcv1_context_t* pTContext,char* data,ctiot_funcv1_send_mode_e sendMode, ctiot_callback_notify notifyCallback, UINT8 seqNum);
void ctiot_funcv1_clean_params(void);
void ctiot_funcv1_clean_context(void);
uint16_t ctiot_funcv1_set_psk(ctiot_funcv1_context_t* pContext,uint8_t securityMode,uint8_t* pskId,uint8_t* psk);
uint16_t ctiot_funcv1_get_pskid(ctiot_funcv1_context_t* pContext, char* pskid);
uint16_t ctiot_funcv1_get_sec_mode(ctiot_funcv1_context_t* pContext,uint8_t* securityMode);
uint16_t ctiot_funcv1_set_idauth_pm(ctiot_funcv1_context_t* pTContext,char* idAuthStr);

void ctiot_funcv1_init_sleep_handler(void);
void ctiot_funcv1_disable_sleepmode(void);
void ctiot_funcv1_enable_sleepmode(void);
void ctiot_funcv1_sleep_vote(system_status_e status);
uint16_t ctiot_funcv1_get_localIP(char* localIP);
uint16_t ctiot_funcv1_wait_cstate(ctiot_funcv1_chip_info* pChipInfo);
uint16_t ctiot_funcv1_set_imsi(ctiot_funcv1_context_t* pTContext, char* imsi);
uint16_t ctiot_funcv1_get_imsi(ctiot_funcv1_context_t* pContext, char* imsi);
void ctiot_funcv1_clear_session(BOOL saveToNV);

#ifdef  FEATURE_REF_AT_ENABLE
uint16_t ctiot_funcv1_set_regswt_mode(ctiot_funcv1_context_t* pTContext, uint8_t type);
uint16_t ctiot_funcv1_get_regswt_mode(ctiot_funcv1_context_t* pContext,uint8_t* type);
uint16_t ctiot_funcv1_reset_dtls(ctiot_funcv1_context_t* pTContext);
uint16_t ctiot_funcv1_get_dtls_status(ctiot_funcv1_context_t* pTContext, uint8_t * status);
uint16_t ctiot_funcv1_get_bs_server(ctiot_funcv1_context_t* pContext,char* serverIP,uint16_t* port);
uint16_t ctiot_funcv1_set_bs_server(ctiot_funcv1_context_t* pTContext,char* serverIP,uint16_t port, uint32_t reqHandle);
uint16_t ctiot_funcv1_del_serverIP(ctiot_funcv1_context_t* pTContext,char* serverIP,uint16_t port);
uint16_t ctiot_funcv1_get_ep(ctiot_funcv1_context_t* pContext, char* endpoint);
uint16_t ctiot_funcv1_set_ep(ctiot_funcv1_context_t* pContext, char* endpoint);
uint16_t ctiot_funcv1_get_lifetime(ctiot_funcv1_context_t* pContext, uint32_t* lifetime);
uint16_t ctiot_funcv1_set_lifetime(ctiot_funcv1_context_t* pContext, uint32_t lifeTime);
uint16_t ctiot_funcv1_get_dlpm(ctiot_funcv1_context_t* pContext, uint32_t* buffered, uint32_t* received, uint32_t* dropped);
uint16_t ctiot_funcv1_get_ulpm(ctiot_funcv1_context_t* pContext, uint32_t* pending, uint32_t*sent, uint32_t*error);
uint16_t ctiot_funcv1_get_dtlstype(ctiot_funcv1_context_t* pContext, uint8_t* type, uint8_t* natType);
uint16_t ctiot_funcv1_set_dtlstype(ctiot_funcv1_context_t* pContext, uint8_t type, uint8_t natType);
uint16_t ctiot_funcv1_set_nsmi_mode(ctiot_funcv1_context_t* pTContext, uint8_t type);
uint16_t ctiot_funcv1_get_nsmi_mode(ctiot_funcv1_context_t* pTContext, uint8_t* type);
BOOL ctiot_funcv1_get_data_status(ctiot_funcv1_context_t* pTContext,uint8_t* status,uint8_t* seqNum);
#endif
uint16_t ctiot_funcv1_set_nnmi_mode(ctiot_funcv1_context_t* pTContext, uint8_t type);
uint16_t ctiot_funcv1_get_nnmi_mode(ctiot_funcv1_context_t* pContext, uint8_t* type);
uint16_t ctiot_funcv1_get_recvData(ctiot_funcv1_context_t* pTContext, uint16_t* datalen, uint8_t** dataStr);
#endif

