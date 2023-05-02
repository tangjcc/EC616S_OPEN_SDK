/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616 China Telecomm NB-IOT demo ct_api source file
 *               this file complete the work of the lwm2m client,the recovery work of the timer wakeup,
 *               and the work of key wakeup
 *               if want connect to ctwing platform define USE_CTWING_PLATFORM, if want connect to ocean platform no define it
 *               if client use DTLS encryption define WITH_DTLS, and modify the value of psk in ctConnectTask 
 * History:      Rev1.0   2020-03-24
 *                        2020-08-15   add DTLS 
 *
 ****************************************************************************/
#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "queue.h"
#include "ps_event_callback.h"
#include "ct_api.h"
#include "psifevent.h"
#include "ps_lib_api.h"
#include "lwip/netdb.h"
#include "ctiot_NV_data.h"
#include "ctiot_lwm2m_sdk.h"
#include "ec_ctlwm2m_api.h"
#include "debug_log.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "gps_demo.h"
#include "ps_lib_api.h"
#include "osasys.h"


#define USE_CTWING_PLATFORM   
//#define WITH_DTLS            

#define STR_MACHINESTATE(S)                                \
((S) == APP_INIT_STATE ? "APP_INIT_STATE" :      \
((S) == APP_DEACTIVE_STATE ? "APP_DEACTIVE_STATE" :      \
((S) == APP_CHECK_IPREADY_FROM_WAKE ? "APP_CHECK_IPREADY_FROM_WAKE" :  \
((S) == APP_IPREADY_STATE ? "APP_IPREADY_STATE" :        \
((S) == APP_OC_OBV_SUB_STATE ? "APP_OC_OBV_SUB_STATE" :      \
((S) == APP_OC_HEART_BEAT_UPDATE ? "APP_OC_HEART_BEAT_UPDATE" :  \
((S) == APP_OC_HEART_BEAT_ARRIVED ? "APP_OC_HEART_BEAT_ARRIVED" :  \
((S) == APP_OC_HEART_BEAT_SEND ? "APP_OC_HEART_BEAT_SEND" :  \
((S) == APP_OC_CONTEXT_RECOVER ? "APP_OC_CONTEXT_RECOVER" :  \
((S) == APP_OC_UPDATE_SUCCESS_STATE ? "APP_OC_UPDATE_SUCCESS_STATE" :  \
((S) == APP_OC_REG_SUCCESS_STATE ? "APP_OC_REG_SUCCESS_STATE" :  \
((S) == APP_SENDING_PACKET_STATE ? "APP_SENDING_PACKET_STATE" :  \
((S) == APP_WAITING_ONLINE_STATE ? "APP_WAITING_ONLINE_STATE" :  \
((S) == APP_WAIT_APP_ACT ? "APP_WAIT_APP_ACT" :  \
((S) == APP_CT_OBSV_RESP ? "APP_CT_OBSV_RESP" :  \
((S) == APP_IDLE_STATE ? "APP_IDLE_STATE" :  \
"Unknown"))))))))))))))))

#define STR_CT_REG_EVENT(S)                                \
((S) == REG_FAILED_TIMEOUT ? "REG_FAILED_TIMEOUT" :      \
((S) == REG_FAILED_AUTHORIZE_FAIL ? "REG_FAILED_AUTHORIZE_FAIL" :      \
((S) == REG_FAILED_ERROR_ENDPOINTNAME ? "REG_FAILED_ERROR_ENDPOINTNAME" :  \
((S) == REG_FAILED_PROTOCOL_NOT_SUPPORT ? "REG_FAILED_PROTOCOL_NOT_SUPPORT" :        \
((S) == REG_FAILED_OTHER ? "REG_FAILED_OTHER" :      \
"Unknown")))))

#define STR_CT_UPDATE_EVENT(S)                                \
((S) == UPDATE_FAILED_TIMEOUT ? "UPDATE_FAILED_TIMEOUT" :      \
((S) == UPDATE_FAILED_SEND ? "UPDATE_FAILED_SEND" :      \
((S) == UPDATE_FAILED_SERVER_FORBIDDEN ? "UPDATE_FAILED_SERVER_FORBIDDEN" :  \
((S) == UPDATE_FAILED_WRONG_PARAM ? "UPDATE_FAILED_WRONG_PARAM" :        \
((S) == UPDATE_FAILED_UE_OFFLINE ? "UPDATE_FAILED_UE_OFFLINE" :      \
"Unknown")))))


// app task static stack and control block
#ifdef WITH_DTLS
    #define CT_TASK_STACK_SIZE    (4096)
#else
    #define CT_TASK_STACK_SIZE    (2048)
#endif
static StaticTask_t             ctTask;
static UINT8                    ctTaskStack[CT_TASK_STACK_SIZE];


appStateMachine_t stateMachine = APP_INIT_STATE;
UINT8 recvBuff[256]                   = {0};
slpManTimerID_e heartBeatTimerID      = DEEPSLP_TIMER_ID0;
slpManTimerID_e sendDataTimerID       = DEEPSLP_TIMER_ID1;
INT32 defaultLifetime = 86400;
extern UINT32 sendDataItv;
QueueHandle_t ct_state_msg_handle = NULL;
UINT8 deactCount = 0;
UINT8 pressKey = KEY_NOT_PRESS;
UINT16 messageId = 0;
CHAR* obsvToken = NULL;
CHAR obsvUri[20]= {0};

extern QueueHandle_t app_cmd_msg_handle;

static void ct_set_context_default()
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
    pContext->nnmiFlag           = 1;
}

static INT32 ocPSUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    UINT8 rssi = 0;
    CmiPsCeregInd *cereg = NULL;
    NmAtiNetifInfo *netif = NULL;
    ctiot_funcv1_context_t* pContext=ctiot_funcv1_get_context();
    ctiot_funcv1_chip_info_ptr pChipInfo=pContext->chipInfo;

    switch(eventID)
    {
        case NB_URC_ID_MM_SIGQ:
        {
            rssi = *(UINT8 *)param;
            ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_1, P_INFO, 1, "URCCallBack:RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_2, P_INFO, 0, "URCCallBack:Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED:
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_3, P_INFO, 0, "URCCallBack:Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED:
        {
            cereg = (CmiPsCeregInd *)param;
            ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_4, P_INFO, 4, "URCCallBack:CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
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
            ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_5, P_INFO, 1, "URCCallBack:NB_URC_ID_MM_CERES_CHANGED celevel=%d", pChipInfo->cSignalLevel);
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                stateMachine = APP_IPREADY_STATE;
                ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_6, P_INFO, 0, "URCCallBack:PSIF network active");
            }
            else if (netif->netStatus == NM_NO_NETIF_OR_DEACTIVATED || netif->netStatus == NM_NO_NETIF_NOT_DIAL)
            {
                stateMachine = APP_DEACTIVE_STATE;
                ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_7, P_INFO, 0, "URCCallBack:PSIF network deactive");
            }
            else
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ocPSUrcCallback_8, P_INFO, 1, "network status=%d",netif->netStatus);
            }
            break;

        }
    }
    return 0;
}

#define CT_IOT_CMD_RECORD_FILE "ct_iot.dat"
#define CT_IOT_CMD_RECORD_MAX_NUM 5

static void CtIotRecordCmdMsg(void)
{
    OSAFILE fp = PNULL;
    int32_t ret = 0;

    // read current count
    uint32_t cmdRecordNum = 0;

    /*
     * open the config file
    */
    fp = OsaFopen(CT_IOT_CMD_RECORD_FILE, "wb+");   //read & write & create
    if (fp == PNULL)
    {
        printf("Can't open/create 'ct_iot.dat' file\r\n");
        return;
    }

    /*
     * check file size
     */
    ret = OsaFsize(fp);
	ECOMM_TRACE(UNILOG_PLA_APP, CtIotRecordCmdMsg_0, P_INFO, 1, "ct_iot.dat file size:%d", ret);
    if(ret == 0)
    {
        printf("First creation of 'ct_iot.dat' file\r\n");
    }
    else if(ret != -1)
    {
        /*
         * read file
         */
        ret = OsaFread(&cmdRecordNum, sizeof(cmdRecordNum), 1, fp);
        if(ret != 1)
        {
            printf("Can't read 'ct_iot.dat' file\r\n");
            OsaFclose(fp);
            return;
        }
		ECOMM_TRACE(UNILOG_PLA_APP, CtIotRecordCmdMsg_1, P_INFO, 1, "ct_iot.dat read:%d", cmdRecordNum);
    }
    else
    {
        printf("Can't get 'ct_iot.dat' file size\r\n");
        OsaFclose(fp);
        return;
    }

	// update boot count
    cmdRecordNum += 1;

    ret = OsaFseek(fp, 0, SEEK_SET);
    if(ret != 0)
    {
        printf("Seek 'ct_iot.dat' file failed\r\n");
        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    ret = OsaFwrite(&cmdRecordNum, sizeof(cmdRecordNum), 1, fp);
    if (ret != 1)
    {
        printf("Write 'ct_iot.dat' file failed\r\n");
    }

	ECOMM_TRACE(UNILOG_PLA_APP, CtIotRecordCmdMsg_2, P_INFO, 1, "ct_iot.dat write done:%d", cmdRecordNum);

    OsaFclose(fp);
}

extern uint8_t* prv_at_decode(char* data,int datalen);

static void CtIotBuildSendMsg(CtWingMsgHead_s *msgHead, uint8_t dataLen, uint8_t *data, uint8_t sendbuf[], uint16_t sendbufMax)
{
	if (msgHead == NULL || data == NULL || sendbuf == NULL || sendbufMax > CT_SEND_BUFF_MAX_LEN) {
		return;
	}
	sprintf((char *)sendbuf, "%02x%04x%04x%04x", msgHead->cmdType, msgHead->serviceID, msgHead->taskId, dataLen);
	strcat((char *)sendbuf, (char *)data);
}

static void CtIotBuildReportMsg(CtWingMsgHead_s *msgHead, uint8_t dataLen, uint8_t *data, uint8_t sendbuf[], uint16_t sendbufMax)
{
	if (msgHead == NULL || data == NULL || sendbuf == NULL || sendbufMax < dataLen) {
		return;
	}
	sprintf((char *)sendbuf, "%02x%04x%04x", msgHead->cmdType, msgHead->serviceID, dataLen);
	strcat((char *)sendbuf, (char *)data);
}

static void CtIotRecvProcessing(uint8_t *atbuff, uint32_t arg_len)
{
	uint32_t msgHeadLen = 1 + 2 * 3;
	uint32_t headLen = strlen("+CTM2MRECV: ");
	ECOMM_STRING(UNILOG_PLA_APP, RecvProcessing_0, P_INFO, "atbuff:%s", (uint8_t *)atbuff);
	if (arg_len < headLen + msgHeadLen * 2) {
		ECOMM_TRACE(UNILOG_PLA_APP, RecvProcessing_0_1, P_INFO, 2, "error Len: arg_len:%d; except:%d", arg_len, headLen + msgHeadLen * 2);
		return;
	}
	uint8_t * buff = prv_at_decode((char *)atbuff + headLen, arg_len - headLen);
	uint32_t i = 0;
	uint8_t cmdType = buff[i];
	i++;
	uint16_t srvId = buff[i] << 8 | buff[i+1];
	i += 2;
	uint16_t taskId = buff[i] << 8 | buff[i+1];
	i += 2;
	uint16_t dataLen = buff[i] << 8 | buff[i+1];
	i += 2;
	//UINT8 *data = &buff[i];
	uint8_t  sendbuf[CT_SEND_BUFF_MAX_LEN]={0};

	CtWingMsgHead_s msgHead = {0};
	msgHead.cmdType = CTWING_CMD_TYPE_RESP;
	msgHead.serviceID = CTWING_SVR_ID_RESP(srvId);
	msgHead.taskId = taskId;
	/* times buff */
	uint16_t timesLen = 12;
	uint8_t times[32]={0};
	uint32_t lllen = (timesLen + 1) * 2;

	uint8_t rawDate[20] = {0};
	uint32_t rawDateLen = 0;

	OsaUtcTimeTValue    timeUtc;       //24 bytes
	uint32_t ret = appGetSystemTimeUtcSync(&timeUtc);
    ECOMM_TRACE(UNILOG_PLA_APP, RecvProcessing_0_2, P_INFO, 2, "System time=0x%x..0x%x", timeUtc.UTCtimer1,timeUtc.UTCtimer2);
	if (ret == NBStatusSuccess) {
		uint16_t yy = (timeUtc.UTCtimer1 >> 16) & 0xFFFF;
		uint8_t mm = (timeUtc.UTCtimer1 >> 8) & 0xFF;
		uint8_t dd = (timeUtc.UTCtimer1) & 0xFF;
		uint8_t hh = (timeUtc.UTCtimer2 >> 24) & 0xFF;
		uint8_t MM = (timeUtc.UTCtimer2 >> 16) & 0xFF;
		uint8_t ss = (timeUtc.UTCtimer2 >> 8) & 0xFF;
		int8_t tz = (timeUtc.UTCtimer2) & 0xFF;
 		sprintf((char *)rawDate, "%04d%02d%02d%02d%02d%02d\r\n", yy, mm, dd, hh + tz, MM, ss);
		rawDateLen = 14;
	} else {
		GpsGetDateTimeString((char *)rawDate, 20, &rawDateLen);
	}
	if (rawDateLen > 0) {
		ctiot_funcv1_str_to_hex(rawDate,12,times,&lllen);
	} else {
		ctiot_funcv1_str_to_hex("202205121810",12,times,&lllen);
	}

	/* location buff */
	uint16_t locLen = 20;
	uint8_t locs[40]={0};
	uint32_t locllen = (locLen + 1) * 2;

	uint8_t rawLoc[24] = {0};
	uint32_t rawLocLen = 0;
	GpsGetLocationString((char *)rawLoc, 24, &rawLocLen);
	if (rawLocLen > 0) {
		ctiot_funcv1_str_to_hex(rawLoc,20,locs,&locllen);
	} else {
		ctiot_funcv1_str_to_hex("E180.22.33N044.55.11", 20 ,locs,&locllen);
	}

	ct_lwm2m_free(buff);

	ECOMM_TRACE(UNILOG_PLA_APP, RecvProcessing_1, P_INFO, 4, "cmdType:%d; srvId:%x; taskId:%d; dataLen:%d", cmdType, srvId, taskId, dataLen);
	switch (cmdType) {
		case CTWING_CMD_TYPE_REQ:
			if (srvId == CTWING_SVR_ID_GET_TIME) { // get_devtime
				ECOMM_STRING(UNILOG_PLA_APP, RecvProcessing_2_0, P_INFO, "times:%s", (uint8_t *)times);
				CtIotBuildSendMsg(&msgHead, timesLen, times, sendbuf, sizeof(sendbuf));
				ECOMM_STRING(UNILOG_PLA_APP, RecvProcessing_2, P_INFO, "resp:%s", (uint8_t *)sendbuf);
				ctiot_funcv1_send(NULL, (char *)sendbuf, SENDMODE_CON, NULL, 0);
			} else if (srvId == CTWING_SVR_ID_GET_LOC) { // get_dev_location
				CtIotBuildSendMsg(&msgHead, locLen, locs, sendbuf, sizeof(sendbuf));
				ECOMM_STRING(UNILOG_PLA_APP, RecvProcessing_3, P_INFO, "resp:%s", (uint8_t *)sendbuf);
				ctiot_funcv1_send(NULL, (char *)sendbuf, SENDMODE_CON, NULL, 0);
			}
		break;
	}
	CtIotRecordCmdMsg();
	return;
}

void CtIotReportDevTimes(void)
{
	/* times buff */
	uint16_t timesLen = 14;
	uint8_t times[32]={0};
	uint32_t lllen = (timesLen + 1) * 2;

	uint8_t rawDate[20] = {0};
	uint32_t rawDateLen = 0;

	OsaUtcTimeTValue    timeUtc;       //24 bytes
	uint32_t ret = appGetSystemTimeUtcSync(&timeUtc);
    ECOMM_TRACE(UNILOG_PLA_APP, CtIotReportDevTimes_0, P_INFO, 2, "System time=0x%x..0x%x", timeUtc.UTCtimer1,timeUtc.UTCtimer2);
	if (ret == NBStatusSuccess) {
		uint16_t yy = (timeUtc.UTCtimer1 >> 16) & 0xFFFF;
		uint8_t mm = (timeUtc.UTCtimer1 >> 8) & 0xFF;
		uint8_t dd = (timeUtc.UTCtimer1) & 0xFF;
		uint8_t hh = (timeUtc.UTCtimer2 >> 24) & 0xFF;
		uint8_t MM = (timeUtc.UTCtimer2 >> 16) & 0xFF;
		uint8_t ss = (timeUtc.UTCtimer2 >> 8) & 0xFF;
		int8_t tz = (timeUtc.UTCtimer2) & 0xFF;
 		sprintf((char *)rawDate, "%04d%02d%02d%02d%02d%02d\r\n", yy, mm, dd, hh + tz, MM, ss);
		rawDateLen = 14;
	} else {
		GpsGetDateTimeString((char *)rawDate, 20, &rawDateLen);
	}
	if (rawDateLen > 0) {
		ctiot_funcv1_str_to_hex(rawDate,14,times,&lllen);
	} else {
		ctiot_funcv1_str_to_hex("20220512181000",14,times,&lllen);
	}
	uint8_t  sendbuf[CT_SEND_BUFF_MAX_LEN]={0};
	CtWingMsgHead_s msgHead = {0};
	msgHead.cmdType = CTWING_CMD_TYPE_RPT;
	msgHead.serviceID = CTWING_SVR_ID_TIME_RPT;
	// msgHead.taskId = taskId;
	CtIotBuildReportMsg(&msgHead, timesLen, times, sendbuf, sizeof(sendbuf));
	ECOMM_STRING(UNILOG_PLA_APP, CtIotReportDevTimes_1, P_INFO, "rpt:%s", (uint8_t *)sendbuf);
	ctiot_funcv1_send(NULL, (char *)sendbuf, SENDMODE_CON, NULL, 0);
}

void CtIotReportDevLocation(void)
{
	uint16_t locLen = 20;
	uint8_t locs[40]={0};
	uint32_t locllen = (locLen + 1) * 2;

	uint8_t rawLoc[24] = {0};
	uint32_t rawLocLen = 0;
	GpsGetLocationString((char *)rawLoc, 24, &rawLocLen);
	ECOMM_STRING(UNILOG_PLA_APP, CtIotReportDevLocation_0, P_INFO, "rpt:%s", (uint8_t *)rawLoc);
	if (rawLocLen > 0) {
		ctiot_funcv1_str_to_hex(rawLoc,20,locs,&locllen);
	} else {
		ctiot_funcv1_str_to_hex("E180.22.33N044.55.11", 20 ,locs,&locllen);
	}

	uint8_t  sendbuf[CT_SEND_BUFF_MAX_LEN]={0};
	CtWingMsgHead_s msgHead = {0};
	msgHead.cmdType = CTWING_CMD_TYPE_RPT;
	msgHead.serviceID = CTWING_SVR_ID_LOC_RPT;
	CtIotBuildReportMsg(&msgHead, locllen, locs, sendbuf, sizeof(sendbuf));
	ECOMM_STRING(UNILOG_PLA_APP, CtIotReportDevLocation_1, P_INFO, "rpt:%s", (uint8_t *)sendbuf);
	ctiot_funcv1_send(NULL, (char *)sendbuf, SENDMODE_CON, NULL, 0);
}


#define CTIOT_DEV_INFO_MAX_LEN 20

static void CtIotStrToHexWithLen(const char * src, char *dest, int destMax)
{
	char dataBuff[CT_SEND_BUFF_MAX_LEN] = {0};
	int srcLen = strlen(src);
	int lllen = srcLen * 2 + 1;
	if (lllen > destMax) {
		return;
	}
	ctiot_funcv1_str_to_hex(src, srcLen, dataBuff, &lllen);
	sprintf((char *)dest, "%04x", srcLen);
	strcat((char *)dest, dataBuff);
}

void CtIotReportDevInfos(void)
{
	char *mfrName = "M01_XXX";
	char *termType = BOARD_NAME;
	char *modType = BOARD_NAME;
	char *hwVer = EVB_VERSION;
	char *swVer = SOFTVERSION;
	char imei[CTIOT_DEV_INFO_MAX_LEN] = {0};
	char iccid[CTIOT_DEV_INFO_MAX_LEN] = {0};
	uint32_t ret = appGetImeiNumSync(imei);
	ret |= appGetIccidNumSync(iccid);
	if (ret != NBStatusSuccess) {
		return;
	}

	char infoBuff[240] = {0};
	char tmpBuff[CT_SEND_BUFF_MAX_LEN] = {0};
	CtIotStrToHexWithLen(mfrName, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);
	memset(tmpBuff, 0, sizeof(tmpBuff));
	CtIotStrToHexWithLen(termType, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);
	memset(tmpBuff, 0, sizeof(tmpBuff));
	CtIotStrToHexWithLen(modType, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);
	memset(tmpBuff, 0, sizeof(tmpBuff));
	CtIotStrToHexWithLen(hwVer, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);
	memset(tmpBuff, 0, sizeof(tmpBuff));
	CtIotStrToHexWithLen(swVer, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);
	memset(tmpBuff, 0, sizeof(tmpBuff));
	CtIotStrToHexWithLen(imei, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);
	memset(tmpBuff, 0, sizeof(tmpBuff));
	CtIotStrToHexWithLen(iccid, tmpBuff, CT_SEND_BUFF_MAX_LEN);
	strcat((char *)infoBuff, tmpBuff);

	ECOMM_STRING(UNILOG_PLA_APP, CtIotReportDevInfos_0, P_INFO, "infoBuff:%s", (uint8_t *)infoBuff);

	uint16_t infoLen = strlen(infoBuff) / 2;
	uint8_t sendbuf[240]={0};

	CtWingMsgHead_s msgHead = {0};
	msgHead.cmdType = CTWING_CMD_TYPE_RPT;
	msgHead.serviceID = CTWING_SVR_ID_DEV_INFO_RPT;
	// msgHead.taskId = taskId;
	CtIotBuildReportMsg(&msgHead, infoLen, infoBuff, sendbuf, sizeof(sendbuf));
	ECOMM_STRING(UNILOG_PLA_APP, CtIotReportDevInfos_1, P_INFO, "rpt:%s", (uint8_t *)sendbuf);
	ctiot_funcv1_send(NULL, (char *)sendbuf, SENDMODE_CON, NULL, 0);
}


/*WARNING NO TIME-CONSUMING OPERATIONS CAN BE USED IN THIS FUNCTION!!!!!!!!!!!!!!!!!!!!!!!!!!*/
static void ctEventHandler(module_type_t type, INT32 code, const void* arg, INT32 arg_len)
{
    char logbuf[64] = {0};
    CT_STATUS_Q_MSG ctMsg;
    memset(&ctMsg, 0, sizeof(ctMsg));
    
    switch (type)
    {
        case MODULE_LWM2M:
        {
            if (code == REG_SUCCESS)
            {
                stateMachine = APP_OC_REG_SUCCESS_STATE;//register ok still need waiting server observe
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_0, P_INFO, 0, "ctiot register successful");
            }
            else if ( code >= REG_FAILED_TIMEOUT && code <= REG_FAILED_OTHER)
            {
                stateMachine = APP_WAIT_APP_ACT;//register fail notify app
                snprintf(logbuf,64,"%s",STR_CT_REG_EVENT(code));
                ECOMM_STRING(UNILOG_PLA_APP, ctEventHandler_1, P_INFO, "ctiot register failed:%s", (uint8_t *)logbuf);
                ctMsg.status_type = APP_OC_REG_FAILED;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
            }
            else if (code == OBSERVE_UNSUBSCRIBE)
            {
                stateMachine = APP_WAIT_APP_ACT;//observe cancel by server notify app
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_2, P_INFO, 0, "ctiot observe unsubscribe");
                ctMsg.status_type = APP_OC_OBSV_FAILED;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
            }
            else if (code == OBSERVE_SUBSCRIBE)//recv observe msg need save context
            {
                stateMachine = APP_OC_OBV_SUB_STATE;//observe success can send data
                ctiot_funcv1_save_context();
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_3, P_INFO, 0, "ctiot observe subscribe");
            }
            else if (code == SEND_CON_DONE)
            {
                stateMachine = APP_IDLE_STATE;//send data success goto idle state
                if(pressKey == KEY_RESPONSING){
                    pressKey = KEY_NOT_PRESS;//send data success clear key status, can response next key press
                }
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_4, P_INFO, 0, "ctiot send con arrived");
                ctMsg.status_type = APP_SEND_PACKET_DONE;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
            }
            else if (code == SEND_FAILED)//send failed notify app to retry or re-register
            {
                stateMachine = APP_WAIT_APP_ACT;
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_5, P_INFO, 0, "ctiot send failed");
                ctMsg.status_type = APP_SEND_PACKET_FAILED;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
            }
            else if (code == UPDATE_SUCCESS)//
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_6, P_INFO, 0, "ctiot update success");
                if(stateMachine == APP_IDLE_STATE){
                    stateMachine = APP_OC_UPDATE_SUCCESS_STATE;//if no need send data, enable sleep
                }
            }
            else if (code >= UPDATE_FAILED_TIMEOUT && code <= UPDATE_FAILED_UE_OFFLINE)
            {
                stateMachine = APP_WAIT_APP_ACT;//update failed, notify app to re-register
                snprintf(logbuf,64,"%s",STR_CT_UPDATE_EVENT(code));
                ECOMM_STRING(UNILOG_PLA_APP, ctEventHandler_7, P_INFO, "ctiot update failed:%s", (uint8_t *)logbuf);
                ctiot_funcv1_disable_sleepmode();
                ctMsg.status_type = APP_UPDATE_FAILED;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                stateMachine = APP_WAIT_APP_ACT;
            }
            else if (code == RECV_RST_CMD)
            {
                stateMachine = APP_WAIT_APP_ACT;//recevie server's rst cmd, notify app to re-register 
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_8, P_INFO, 0, "ctiot recv RST command notify app");
                ctMsg.status_type = APP_RECV_RST_CMD;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
            }
            else if(code == FOTA_BEGIN)//begin FOTA need disable sleep
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_10, P_INFO, 0, "begin FOTA disable sleep");
                ctiot_funcv1_disable_sleepmode();
            }
            else if(code == FOTA_DOWNLOAD_SUCCESS)//download complete, system will reset restart soon
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_11, P_INFO, 0, "FOTA firmware download success");
            }
            else if(code == FOTA_DOWNLOAD_FAIL)//download failed, platform will re-initiate a new upgrade if config
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_11_1, P_INFO, 0, "FOTA firmware download fail");
                ctiot_funcv1_enable_sleepmode();
            }
            else if(code == FOTA_OVER)//FOTA finished enable sleep
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_12, P_INFO, 0, "FOTA complete enble sleep");
                ctiot_funcv1_save_context();
                ctiot_funcv1_enable_sleepmode();
            }
            else if(code == OBSV_CMD)//CTWING need obsv object just reponse it
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctEventHandler_13, P_INFO, 0, "CTWING need obsv object");
                ctiot_funcv1_object_operation_t* pTmpCommand = (ctiot_funcv1_object_operation_t*) arg;
                messageId = pTmpCommand->msgId;
                obsvToken = prv_at_encode(pTmpCommand->token, pTmpCommand->tokenLen);
                strcpy(obsvUri, (CHAR*)pTmpCommand->uri);
                stateMachine = APP_CT_OBSV_RESP;
            }
            break;
        }
        case MODULE_COMM:
        {
            if (code == RECV_DATA_MSG)//recive data from server user can extended implementation
            {
                memset(recvBuff, 0, sizeof(recvBuff));
                memcpy(recvBuff, (CHAR*)arg, ((arg_len > 256)?256:arg_len));
                ECOMM_STRING(UNILOG_PLA_APP, ctEventHandler_9, P_INFO, "%s", recvBuff);
				//printf("recvBuff:%s\n",recvBuff);
                stateMachine = APP_CT_REQUEST_RECV;//APP_IDLE_STATE;
                //ctiot_funcv1_sleep_vote(SYSTEM_STATUS_FREE);
            }
            break;
        }
    }
}

void appTimerExpFunc(uint8_t id)
{
    ECOMM_TRACE(UNILOG_PLA_APP, appTimerExpFunc_1, P_SIG, 1, "User DeepSleep Timer Expired: TimerID = %u",id);
    ctiot_funcv1_disable_sleepmode();
    if (heartBeatTimerID == id){
        ECOMM_TRACE(UNILOG_PLA_APP, appTimerExpFunc_2, P_SIG, 0, "it's heartbeat time up");
#ifdef USE_CTWING_PLATFORM
        stateMachine = APP_OC_HEART_BEAT_ARRIVED;
#else
        if(stateMachine != APP_CHECK_IPREADY_FROM_WAKE){//if not in sending data, to heartbeat, if sending data no need heartbeat
            stateMachine = APP_OC_HEART_BEAT_ARRIVED;
        }else{
            ECOMM_TRACE(UNILOG_PLA_APP, appTimerExpFunc_2_1, P_SIG, 0, "already in send data time");
        }
#endif
        slpManDeepSlpTimerStart(heartBeatTimerID, defaultLifetime*1000); 
    }else if (sendDataTimerID == id){
        ECOMM_TRACE(UNILOG_PLA_APP, appTimerExpFunc_3, P_SIG, 1, "it's send data time up (%d)s",sendDataItv);
        stateMachine = APP_CHECK_IPREADY_FROM_WAKE;
        slpManDeepSlpTimerStart(sendDataTimerID, sendDataItv*1000);
    }
}


static void ctConnectTask(void *arg)
{
    UINT16 ret;
#ifdef USE_CTWING_PLATFORM
    UINT8* serverIP = "221.229.214.202";
#else
    UINT8* serverIP = "180.101.147.115";
#endif

#ifdef WITH_DTLS
    CHAR* psk = "32303230";
    UINT32 psklen = strlen(psk)/2;
    UINT8* pskBuf = malloc(psklen+1);
    memset(pskBuf, 0, psklen+1);
    cmsHexStrToHex(pskBuf, psklen, psk, psklen*2);
    UINT16 portNum  = 5684;
#else
    UINT16 portNum  = 5683;
#endif
    
    CT_STATUS_Q_MSG ctMsg;
    memset(&ctMsg, 0, sizeof(ctMsg));
    
    ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();

    while (1)
    {
        char logbuf[64] = {0};
        snprintf(logbuf,64,"%s",STR_MACHINESTATE(stateMachine));
        ECOMM_STRING(UNILOG_PLA_APP, ctConnectTask_0, P_INFO, "handle stateMachine:%s", (uint8_t *)logbuf);
        switch(stateMachine)
        {
           case APP_INIT_STATE:
                osDelay(500/portTICK_PERIOD_MS);
                break;
           case APP_OC_CONTEXT_RECOVER://wake up to recover nb-iot's context
           case APP_OC_HEART_BEAT_UPDATE://to heartbeat
           case APP_IPREADY_STATE://poweron to register
           {
                pContext->chipInfo->cState = NETWORK_CONNECTED;
                pContext->autoHeartBeat = AUTO_HEARTBEAT;
                pContext->lifetime = defaultLifetime;
                if(appGetImeiNumSync(pContext->chipInfo->cImei) != NBStatusSuccess)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_1, P_INFO, 0, "appGetImeiNumSync error");
                }
                ECOMM_STRING(UNILOG_PLA_APP, ctConnectTask_2, P_INFO, "%s", (uint8_t*)pContext->chipInfo->cImei);
                ctiot_funcv1_register_event_handler(ctEventHandler);
                if((pContext->bootFlag == BOOT_LOCAL_BOOTUP) || (pContext->bootFlag == BOOT_FOTA_REBOOT))
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_3, P_INFO, 0, "call ctiot_session_init");
                    if(stateMachine == APP_IPREADY_STATE){
                        pContext->regFlag = 1;//if network attach need re-register to platform
                    }
                    ret = ctiot_funcv1_session_init(pContext);
                    pContext->regFlag = 0;//need clear the flag avoid register everytime
                }
                else
                {
#ifdef WITH_DTLS
                    ret = ctiot_funcv1_set_psk(NULL, 2, (uint8_t*)pContext->chipInfo->cImei, pskBuf);
                    if (ret == CTIOT_NB_SUCCESS)
                    {
                        ECOMM_DUMP(UNILOG_PLA_APP, ctConnectTask_4_2, P_INFO, "set psk:", strlen((CHAR*)pskBuf),pskBuf);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_4_3, P_INFO, 1, "set psk fail ret=%d", ret);
                        ctMsg.status_type = APP_PARAM_ERROR;
                        xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                        stateMachine = APP_WAIT_APP_ACT;
                        break;
                    }
#endif              
                    ret = ctiot_funcv1_set_pm(NULL, (char *)serverIP, portNum, defaultLifetime, NULL);
                    if (ret == CTIOT_NB_SUCCESS)
                    {
                        ECOMM_STRING(UNILOG_PLA_APP, ctConnectTask_4, P_INFO, "OC set platform parameters success ip:%s",(uint8_t *)serverIP);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_4_1, P_INFO, 1, "OC set platform parameters fail ret=%d", ret);
                        ctMsg.status_type = APP_PARAM_ERROR;
                        xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                        stateMachine = APP_WAIT_APP_ACT;
                        break;
                    }
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_5, P_INFO, 0, "direct call register function");
                    ret = ctiot_funcv1_reg(NULL);
                }
                
                if(ret == CTIOT_NB_SUCCESS)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_6, P_INFO, 0, "OC register/restore call success");
                    if (stateMachine == APP_OC_CONTEXT_RECOVER)//after recover context it can send data
                    {
                        stateMachine = APP_SENDING_PACKET_STATE;
                        ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_6_1, P_INFO, 0, "notify app to send data");
                        ctMsg.status_type = APP_READY_TO_SEND_PACKET;
                        xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                    }
                    else if (stateMachine == APP_OC_HEART_BEAT_UPDATE)//can heartbeat
                    {
                        stateMachine = APP_OC_HEART_BEAT_SEND;
                    }
                    else if (stateMachine == APP_IPREADY_STATE)//power on wait to online server
                    {
                        stateMachine = APP_WAITING_ONLINE_STATE;
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_7, P_INFO, 1, "OC register fail ret=%d", ret);
                    stateMachine = APP_WAIT_APP_ACT;
                    ctMsg.status_type = APP_INTER_ERROR;//it can't go here check code
                    xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                    break;
                }

                break;
            }
           case APP_OC_HEART_BEAT_ARRIVED:
           {
                // Here check actively can't got it from URC callback
                if(ctiot_funcv1_wait_cstate(pContext->chipInfo) != 0)
                {
                    pContext->chipInfo->cState = NETWORK_MANUAL_RESTART;
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_8_1, P_INFO, 0, "app update network connect error");
                    stateMachine = APP_WAIT_APP_ACT;
                    ctMsg.status_type = APP_NETWORK_DISC;
                    xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                }
                else
                {
                    stateMachine = APP_OC_HEART_BEAT_UPDATE;
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_8, P_INFO, 0, "network ok send heartbeat");
                }
                break;
           }
           case APP_OC_HEART_BEAT_SEND://check the ct client complete login and update for heartbeat 
           {
                //RegisterUpdate;
                if(pContext->loginStatus==UE_LOGINED)
                {
                    lwm2m_server_t* serverP=pContext->lwm2mContext->serverList;
                    while(serverP!=NULL)
                    {
                        extern int updateRegistration(lwm2m_context_t * contextP,lwm2m_server_t * server);
                        updateRegistration(pContext->lwm2mContext, serverP);
                        serverP=serverP->next;
                    }
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_9, P_INFO, 0, "has send update package");
                    stateMachine = APP_IDLE_STATE;
                }
                else
                {
                    osDelay(1000/portTICK_PERIOD_MS);
                }
                break;
           }
           case APP_OC_OBV_SUB_STATE://complete observe start heartbeat timer and send data timer
           {
                ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_10, P_INFO, 0, "start heartbeat timer, notify app to send package");
                if (slpManDeepSlpTimerIsRunning(heartBeatTimerID) == FALSE)
                    slpManDeepSlpTimerStart(heartBeatTimerID, defaultLifetime*1000); 
                if (slpManDeepSlpTimerIsRunning(sendDataTimerID) == FALSE){
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_10_1, P_INFO, 1, "start send data timer (%d)s",sendDataItv);
                    slpManDeepSlpTimerStart(sendDataTimerID, sendDataItv*1000);
                }
                stateMachine = APP_SENDING_PACKET_STATE;
                ctMsg.status_type = APP_READY_TO_SEND_PACKET;
                xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                break;
           }
           case APP_CHECK_IPREADY_FROM_WAKE: //after wakeup check the ip ready
           {
                if(ctiot_funcv1_wait_cstate(pContext->chipInfo) != 0)//connection timed out
                {
                    //处理无线连接超时
                    pContext->chipInfo->cState = NETWORK_MANUAL_RESTART;
                    stateMachine = APP_WAIT_APP_ACT;
                    ctMsg.status_type = APP_NETWORK_DISC;//notify app network can't connect
                    xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_11_1, P_INFO, 0, "network connect failed");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_11, P_INFO, 0, "from wakeup ip ready, to restore oc client");
                    stateMachine = APP_OC_CONTEXT_RECOVER;//network available to recover nb-iot's context
                }
                break;
           }
           case APP_OC_UPDATE_SUCCESS_STATE://heartbeat complete go to idle
           {
               ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_12, P_INFO, 0, "update success enable sleep and app enter idle");
               ctiot_funcv1_enable_sleepmode();
               stateMachine = APP_IDLE_STATE;
               break;
           }
           case APP_DEACTIVE_STATE:
           {
               deactCount += 1;
               if(deactCount<181){//wait 180 second for the network to recover
                   osDelay(1000/portTICK_PERIOD_MS);
               }else{
                   ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_13, P_INFO, 0, "confirm network disconnect");//network can't recover notify app to manual recover
                   stateMachine = APP_WAIT_APP_ACT;
                   ctMsg.status_type = APP_NETWORK_DISC;
                   xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
               }
               break;
           }
           case APP_OC_REG_SUCCESS_STATE://register success wait server observe
           {
               ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_14, P_INFO, 0, "reg success wait observe ready");
               stateMachine = APP_WAITING_ONLINE_STATE;
               break;
           }
           case APP_WAIT_APP_ACT://wait app's command
           {
               ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15, P_INFO, 0, "wait app's act");
               APP_CMD_Q_MSG cmdMsg;
               int cmd_type = 0xff;
               xQueueReceive(app_cmd_msg_handle, &cmdMsg, osWaitForever);
               cmd_type = cmdMsg.cmd_type;
               switch(cmd_type)
               {
                   case APP_CMD_REINIT_NETWORK://cfun0->clear preferance frequency->cfun1
                   {
                       ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15_1, P_INFO, 0, "reinit network");
                       stateMachine = APP_WAITING_ONLINE_STATE;
                       appSetCFUN(0);
                       osDelay(5000/portTICK_PERIOD_MS);
                       CiotSetFreqParams para;
                       memset(&para, 0, sizeof(CiotSetFreqParams));
                       para.mode = 3;
                       appSetCiotFreqSync(&para);
                       appSetCFUN(1);
                       break;
                   }
                   case APP_CMD_REG_AGAIN:
                   {
                       ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15_2, P_INFO, 0, "register ct again");
                       stateMachine = APP_IPREADY_STATE;//to register again
                       break;
                   }
                   case APP_CMD_TO_IDLE:
                   {
                       ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15_2_1, P_INFO, 0, "do nothing to idle");
                       stateMachine = APP_IDLE_STATE;
                       break;
                   }
                   case APP_CMD_DEREG_REG:
                   {
                       ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15_3, P_INFO, 0, "deregister ct");
                       ret = ctiot_funcv1_dereg(NULL, NULL);
                       if(ret == CTIOT_NB_SUCCESS)
                       {
                           ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15_4, P_INFO, 0, "then register ct again");
                           stateMachine = APP_IPREADY_STATE;//after dereg goto reg again
                       }
                       else
                       {
                           ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_15_5, P_INFO, 0, "not logined or log failed check code");
                           stateMachine = APP_IDLE_STATE;
                       }
                       break;
                   }
               }
               break;
           }
           case  APP_IDLE_STATE://nothing to do can handle key event
           {
               ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_16, P_INFO, 0, "app enter idle");
               if(pressKey == KEY_PRESS){
                   ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_16_1, P_INFO, 0, "key pressed when active or sleep1 mode, notify app to send data");
                   pressKey = KEY_RESPONSING;
                   ctMsg.status_type = APP_READY_TO_SEND_PACKET;
                   xQueueSend(ct_state_msg_handle, &ctMsg, CT_MSG_TIMEOUT);
               }
               osDelay(500/portTICK_PERIOD_MS);
               break;
           }
           case APP_CT_OBSV_RESP://recv obseve command response it
           {
               ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_17, P_INFO, 0, "recv obseve command response it");
               ret = ctiot_funcv1_cmdrsp(NULL, messageId, obsvToken, 205, obsvUri, 0, 0, NULL);
               if(ret == CTIOT_NB_SUCCESS)
               {
                   ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_17_1, P_INFO, 0, "has response, goto APP_WAITING_ONLINE_STATE");
                   stateMachine = APP_WAITING_ONLINE_STATE;//wait platform obseve 19/0/0
               }
               else
               {
                   ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_17_2, P_INFO, 0, "response failed, wait 1 second try again");
                   osDelay(500/portTICK_PERIOD_MS);
               }
               break;
           }
		   case APP_CT_REQUEST_RECV:
		   {
		   	   CtIotRecvProcessing(recvBuff, strlen(recvBuff));
			   memset(recvBuff, 0, sizeof(recvBuff));
			   stateMachine = APP_IDLE_STATE;
		   	   break;
		   }
           case  APP_SENDING_PACKET_STATE://send the data waiting ack
           case  APP_WAITING_ONLINE_STATE://waiting nb-iot client online
           default:
           {
               ECOMM_TRACE(UNILOG_PLA_APP, ctConnectTask_18, P_INFO, 0, "app enter waiting state");
               osDelay(500/portTICK_PERIOD_MS);
               break;
           }

        }

    }
}

void ctApiInit(void)
{
    // Apply own right power policy according to design
    slpManSetPmuSleepMode(true, SLP_HIB_STATE, false);
    ctiot_funcv1_init_sleep_handler();
    
    registerPSEventCallback(NB_GROUP_ALL_MASK, ocPSUrcCallback);
    
    slpManRestoreUsrNVMem();

    if(ct_state_msg_handle == NULL)
    {
        ct_state_msg_handle = xQueueCreate(10, sizeof(CT_STATUS_Q_MSG));
    }
    else
    {
        xQueueReset(ct_state_msg_handle);
    }

    osThreadAttr_t task_attr;
    memset(&task_attr,0,sizeof(task_attr));
    memset(ctTaskStack, 0xA5, CT_TASK_STACK_SIZE);
    task_attr.name = "ct_task";
    task_attr.stack_mem = ctTaskStack;
    task_attr.stack_size = CT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &ctTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(ctConnectTask, NULL, &task_attr);
    
    INT32   ret;
    UINT8 cache_boot_flag = 0;
    ctiot_funcv1_context_t* pContext = NULL;
    pContext=ctiot_funcv1_get_context();
    ret=f2c_encode_params(pContext);
    if( ret != NV_OK )//can't read flash
    {
        ct_set_context_default();
        ECOMM_TRACE(UNILOG_PLA_APP, ctApiInit_0, P_INFO, 0, "f2c_encode_params fail,set default params");
    }
    slpManSlpState_t slpState = slpManGetLastSlpState();
    ECOMM_TRACE(UNILOG_PLA_APP, ctApiInit_1, P_INFO, 1, "slpState = %d",slpState);
    if(slpState == SLP_HIB_STATE || slpState == SLP_SLP2_STATE)//wakeup from deep sleep
    {
        //stateMachine = APP_CHECK_IPREADY_FROM_WAKE;//notes: if you are in edrx state and want recv data every wakeup, this code is needed!!!!
        if(pressKey == KEY_PRESS){//wakeup by key
            ECOMM_TRACE(UNILOG_PLA_APP, ctApiInit_2, P_INFO, 0, "wakeup by pad3 key. set machinestate and disable sleep");
            pressKey = KEY_RESPONSING;
            stateMachine = APP_CHECK_IPREADY_FROM_WAKE;
            ctiot_funcv1_disable_sleepmode();
        }
        ECOMM_TRACE(UNILOG_PLA_APP, ctApiInit_3, P_INFO, 0, "recover from HIB and sleep2");
        ret = cache_get_bootflag(&cache_boot_flag);
        if( ret == NV_OK)
        {
            pContext->bootFlag = (ctiot_funcv1_boot_flag_e)cache_boot_flag;
        }
        else
        {
            pContext->bootFlag = BOOT_NOT_LOAD;
        }
        ECOMM_TRACE(UNILOG_PLA_APP, ctApiInit_4, P_INFO, 1, "OC example bootflag=%d", pContext->bootFlag);
    }
    else
    {
        pContext->bootFlag = BOOT_NOT_LOAD;
        ret = cache_get_bootflag(&cache_boot_flag);
        if (ret == NV_OK)
        {
            if (cache_boot_flag == BOOT_FOTA_REBOOT)
            {
                pContext->bootFlag = (ctiot_funcv1_boot_flag_e)cache_boot_flag;
                ECOMM_TRACE(UNILOG_CTLWM2M, ctApiInit_5, P_INFO, 0, "reboot because FOTA so need to restore context");
            }
            else
            {
                if(pContext->pskid != 0 || pContext->psk != 0){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctApiInit_6, P_INFO, 0, "before has set psk, clear it");
                    c2f_clean_context(NULL);
                    c2f_clean_params(NULL);
                }
            }
            if(pContext->nnmiFlag != 1){//restore nnmi flag to 1(default) every reboot
                pContext->nnmiFlag = 1;
                c2f_encode_params(pContext);
            }
        }
    }
}

