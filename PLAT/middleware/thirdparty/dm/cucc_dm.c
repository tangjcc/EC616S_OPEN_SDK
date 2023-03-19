/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    plat_config.c
 * Description:  platform configuration source file
 * History:      Rev1.0   2019-01-18
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "commonTypedef.h"
#include "lfs_port.h"
#include "autoreg_common.h"
#include "cucc_dm.h"
#include "osasys.h"
#include "ostask.h"
#include "debug_log.h"
#include "netmgr.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "mw_config.h"
#include "ps_lib_api.h"
#include "at_util.h"
#include "at_coap_task.h"

extern int autoRegSuccFlag;
extern UINT8 cuccDmSlpHandler;

int autoRegTimerIsOut = AR_REGISTER_TIME_WAIT;
int autoRegTimerOutRang = 0;
int AutoRegTimerStatus = 0;
int AutoRegTimerStatusCount = 0;
int AutoRegReSendCountMax = 0;

osTimerId_t autoRegTimer = NULL;

UINT8 coapPayloadPack[DST_DATA_LEN] = {0};
CHAR autoRegImei[IMEI_LEN+1] = {0};
CHAR autoRegImsi[IMSI_LEN+1] = {0};
CHAR autoRegFlashCCID[CCID_LEN+1] = {0};
CHAR autoRegSimCCID[CCID_LEN+1] = {0};

extern  UINT32 mwGetSimCcidValue(UINT8 *ccid, UINT8 len);
extern  UINT32 mwSetSimCcidValue(UINT8 *ccid, UINT8 len);
extern  UINT8 mwGetCuccAutoRegFlag(void);
extern  void mwSetCuccAutoRegFlag(UINT8 autoReg);
extern  NBStatus_t appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *result);
extern  int mbedtls_base64_encode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen );

void autoRegTimerCallback(void)
{
    autoRegTimerIsOut = AR_REGISTER_TIME_OUT;
}
INT32 autoRegSetStatus(INT32 status, INT32 count)
{
    AutoRegTimerStatus = status;
    AutoRegTimerStatusCount = count;
    autoRegTimerIsOut = AR_REGISTER_TIME_WAIT;
    autoRegTimerOutRang = mwGetCuccAutoRegRang();
    return 0;
}

#define CUCC_AUTO_REG_TASK_API

INT32 AutoRegInit(void)
{
    uint8_t ctRegisterFlag = NON_REGISTER;
    NmAtiSyncRet netStatus;
    UINT32 autoRegRangTime = 0;
    UINT32 currTimeSecs = 0;
    UINT32 lastRegTimeSecs = 0;

    autoRegSetStatus(AR_TIMER_OUT_RETRY_MODE, 0);

    memset(autoRegImei, 0, IMEI_LEN+1);
    memset(autoRegImsi, 0, IMSI_LEN+1);
    memset(autoRegFlashCCID, 0, CCID_LEN+1);
    memset(autoRegSimCCID, 0, CCID_LEN+1);

    for(int i=0; i<20; i++)
    {
        osDelay(3000);
        appGetNetInfoSync(0, &netStatus);
        if((netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)&&(mwGetTimeSyncFlag() == 1))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg04, P_SIG, 0, "autoReg network is ready");
            break;
        }
    }

    if(netStatus.body.netInfoRet.netifInfo.netStatus != NM_NETIF_ACTIVATED)
    {
        return AR_RC_FAIL;
    }

    if((netStatus.body.netInfoRet.netifInfo.ipType != NM_NET_TYPE_IPV6)&&(netStatus.body.netInfoRet.netifInfo.ipType != NM_NET_TYPE_INVALID))
    {
        osaGetImeiNumSync(autoRegImei);

        ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg05, P_SIG, "autoReg get imei ok, imei is %s", (UINT8 *)autoRegImei);
        /* if imei is not set, use default imei, but the api will return false */
        {
            //CcmSimGetImsiOut(imsi, IMSI_LEN);
            appGetImsiNumSync(autoRegImsi);
            appGetIccidNumSync(autoRegSimCCID);
            mwGetSimCcidValue((UINT8 *)autoRegFlashCCID, CCID_LEN);
        }

        ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg06, P_SIG, "autoReg get imsi is %s", (UINT8 *)autoRegImsi);
        ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg07, P_SIG, "autoReg get current iccid is %s", (UINT8 *)autoRegSimCCID);
        ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg08, P_SIG, "autoReg get previous iccid is %s", (UINT8 *)autoRegFlashCCID);

        /* check ccid, and start auto register */
        if(strcmp(autoRegFlashCCID, autoRegSimCCID) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg09, P_SIG, 0, "autoReg find new sim iccid, start auto register");
            /* read curr time secs */
            currTimeSecs = OsaSystemTimeReadSecs();

            /* read last regis time secs */
            lastRegTimeSecs = mwGetCuccAutoRegLastRegTime();

            /* compare time*/
            autoRegRangTime = currTimeSecs - lastRegTimeSecs;

            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg51, P_SIG, 3, "AutoRegInit currTimeSecs:%d, lastRegTimeSecs:%d, autoRegRangTime:%d", currTimeSecs, lastRegTimeSecs, autoRegRangTime);

            return AR_RC_OK;
        }
        else if(strcmp(autoRegFlashCCID, autoRegSimCCID) == 0)
        {
            ctRegisterFlag = mwGetCuccAutoRegFlag();
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg10, P_SIG, 0, "autoReg find same sim iccid");
            if(ctRegisterFlag != ALREADY_REGISTER)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg11, P_SIG, 0, "autoReg not register before, start register");

                return AR_RC_OK;
            }
            else
            {
                /* read curr time secs */
                currTimeSecs = OsaSystemTimeReadSecs();
                if(currTimeSecs == 0)
                {
                    return AR_RC_FAIL;
                }
                /* read last regis time secs */
                lastRegTimeSecs = mwGetCuccAutoRegLastRegTime();

                /* compare time*/
                if(currTimeSecs >= lastRegTimeSecs)
                {
                    autoRegRangTime = currTimeSecs - lastRegTimeSecs;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg61, P_SIG, 3, "AutoRegInit currTimeSecs:%d is smaller than lastRegTimeSecs:%d", currTimeSecs, lastRegTimeSecs);
                    return AR_RC_FAIL;
                }

                ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg52, P_SIG, 3, "AutoRegInit currTimeSecs:%d, lastRegTimeSecs:%d, autoRegRangTime:%d", currTimeSecs, lastRegTimeSecs, autoRegRangTime);
                if(autoRegRangTime >= autoRegTimerOutRang)
                {
                    /* start register flow */
                    return AR_RC_OK;
                }
                else
                {
                    autoRegSetStatus(AR_TIMER_OUT_NORM_MODE, 0);
                    /* start timer, turn to wait for timeout*/
                    return AR_RC_WAIT_TIMER;
                }

            }
        }
    }
    else
    {
        return AR_RC_FAIL;
        //ECOMM_TRACE(UNILOG_PLA_APP, autoReg66, P_DEBUG, 0, "autoReg..ip..is..ipv6 only..disable..atuoreg..");
    }
    return AR_RC_OK;
}


INT32 AutoRegCreateClient(void)
{
    int ret = 1;
    CoapCreateInfo coapInfo = {COAP_CREATE_FROM_AT, 0};

    /*create client*/
    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg14, P_SIG, 0, "autoReg create client");
    //ret = coapClientCreate(0xff00, 0, "cu_coap", "114.255.193.236",5683, (CHAR *)PNULL, (CHAR *)PNULL, (CHAR *)PNULL, (CHAR *)PNULL, &coapId, info);
    ret = coap_client_create(0, CUCC_COAP_LOCAL_PORT, coapInfo);

    if (ret == COAP_OK) /* COAP create OK */
    {
        return AR_RC_OK;
    }
    else if(ret == 1)
    {
        coap_client_delete(0, 0);
        return AR_RC_CLIENT_FAIL;
    }
    else
    {
        return AR_RC_CLIENT_RECREATE;
    }
}

INT32 AutoRegCreatePacket(void)
{
    UINT8 srcPayload[SRC_DATA_LEN] = {0};
    int srcLen = 0;
    size_t outLen = 0;
    int rc = 0;
    char *model = NULL;
    char *swver = NULL;

    /*send data*/
    memset(coapPayloadPack, 0, DST_DATA_LEN);
    model = (CHAR *)mwGetEcAutoRegModel();
    swver = (CHAR *)mwGetEcAutoRegSwver();

    /*ccid 89860435191890112882    */
    //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"%s\",\"MODEL\":\"eigen-Ec616\",\"SWVER\":\"SW_V10\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", IMEI, ICCID, IMSI);
    //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"356700082680757\",\"MODEL\":\"EiGENCOMM-EC616\",\"SWVER\":\"SW_V1.0\",\"SIM1ICCID\":\"89860435191890112842\",\"SIM1LTEIMSI\":\"460045500202842\"}");
    //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"356700082680757\",\"MODEL\":\"EiGENCOMM-EC616\",\"SWVER\":\"SW_V1.0\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", ICCID, IMSI);
    //sprintf((char*)srcPayload, "{\"REGVER\":\"1\",\"MEID\":\"%s\",\"MODELSMS\":\"EGN-YX EC616\",\"SWVER\":\"EC616_SW_V1.0\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", autoRegImei, autoRegSimCCID, autoRegImsi);
    sprintf((char*)srcPayload, "{\"REGVER\":\"1\",\"MEID\":\"%s\",\"MODELSMS\":\"%s\",\"SWVER\":\"%s\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", autoRegImei, model, swver, autoRegSimCCID, autoRegImsi);

    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg15, P_SIG, "autoReg url is: %s", "coap://114.255.193.236:5683");
    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg16, P_SIG, "autoReg port is: %s", "5683");
    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg17, P_SIG, "autoReg register protocol is: %s", "COAP");
    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg18, P_SIG, "autoReg register message type is: %s", "POST");
    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg19, P_SIG, "autoReg register message Content-Format is: %s", "application/json (50)");
    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg88, P_SIG, "autoReg packet is: %s", srcPayload);
    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg20, P_SIG, 0, "prepare register data as JSON format");
    ECOMM_STRING(UNILOG_PLA_APP, cucc_autoReg88, P_SIG, "autoReg packet is: %s", srcPayload);

    srcLen = strlen((CHAR *)srcPayload);
    rc = mbedtls_base64_encode(coapPayloadPack, (size_t)DST_DATA_LEN, &outLen, srcPayload, (size_t)srcLen);
    if(rc != 0)
    {
        return AR_RC_FAIL;
    }
    else
    {
        return AR_RC_OK;
    }
}

INT32 AutoRegSendPacket(void)
{
    int ret = 1;
    int len = 0;
    
    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg21, P_SIG, 0, "autoReg send register Data");
    len = strlen((char *)coapPayloadPack);

    //ret = coapClientSend(0, 0, "cu_coap", (CHAR *)NULL, 2, 0, 0, 0, 0xff, (CHAR *)NULL, (CHAR *)coapPayloadPack, 0, 0);
    ret = coap_client_send(0x1, 0, 0, 2, "114.255.193.236", 5683, len, (char *)coapPayloadPack, COAP_SEND_AT, 0);
    if (ret != COAP_OK)
    {
        AutoRegReSendCountMax++;
        if(AutoRegReSendCountMax == AR_RESEND_COUNT_MAX)
        {
            return AR_RC_OK;
        }
        else
        {
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg22, P_WARNING, 1, "AutoReg, send register Data fail: %d", ret);
            return AR_RC_SEND_FAIL;
        }
    }
    else
    {
        AutoRegReSendCountMax = 0;
        return AR_RC_OK;
    }
}

INT32 AutoRegCheckRet(void)
{
    time_t currTimeSecs = 0;
    for(int i=0; i<6; i++)
    {
        osDelay(1000);
        if(autoRegSuccFlag == AR_REGISTER_SUCC)
        {
            break;
        }
    }
    if(autoRegSuccFlag == AR_REGISTER_SUCC)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg23, P_SIG, 0, "autoReg recv register ack success");

        currTimeSecs = OsaSystemTimeReadSecs();
        autoRegSetStatus(AR_TIMER_OUT_NORM_MODE, 0);

        mwSetSimCcidValue((UINT8 *)autoRegSimCCID, CCID_LEN);
        mwSetCuccAutoRegFlag(ALREADY_REGISTER);
        mwSetCuccAutoRegLastRegTime(currTimeSecs);
        autoRegSuccFlag = AR_REGISTER_NO_SUCC;
        ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg26, P_SIG, 0, "autoReg done sucess,record current iccid to NV");
        return AR_RC_OK;
    }
    else
    {
        if((AutoRegTimerStatus == AR_TIMER_OUT_RETRY_MODE)&&(AutoRegTimerStatusCount < AR_RETRY_TIME_MAX))
        {
            AutoRegTimerStatusCount++;
            autoRegSetStatus(AR_TIMER_OUT_RETRY_MODE, AutoRegTimerStatusCount);
        }
        else if((AutoRegTimerStatus == AR_TIMER_OUT_RETRY_MODE)&&(AutoRegTimerStatusCount == AR_RETRY_TIME_MAX))
        {
            autoRegSetStatus(AR_TIMER_OUT_NORM_MODE, 0);
        }
        else if(AutoRegTimerStatus == AR_TIMER_OUT_NORM_MODE)
        {
            autoRegSetStatus(AR_TIMER_OUT_RETRY_MODE, 1);
        }

        ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg24, P_SIG, 0, "autoReg recv register ack fail");
        return AR_RC_RECV_FAIL;
    }
}

INT32 AutoRegSetTimer(void)
{
    UINT32 autoRegRangTime = 0;
    UINT32 currTimeSecs = 0;
    UINT32 lastRegTimeSecs = 0;

    AutoRegReSendCountMax = 0;
    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg27, P_SIG, 2, "AutoRegSetTimer, mode is %d, count is %d", AutoRegTimerStatus, AutoRegTimerStatusCount);
    if(autoRegTimer == NULL)
    {
        autoRegTimer = osTimerNew((osTimerFunc_t)autoRegTimerCallback, osTimerOnce, NULL, NULL);
    }
    currTimeSecs = OsaSystemTimeReadSecs(); /* read curr time secs */
    lastRegTimeSecs = mwGetCuccAutoRegLastRegTime(); /* read last regis time secs */
    if(currTimeSecs >= lastRegTimeSecs)
    {
        autoRegRangTime = currTimeSecs - lastRegTimeSecs;
    }
    else
    {

    }

    autoRegTimerOutRang = mwGetCuccAutoRegRang();

    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg28, P_SIG, 3, "AutoRegSetTimer, regPastTime: %d, curr_time: %d, lastRegTime: %d", autoRegRangTime, currTimeSecs, lastRegTimeSecs);

    if(AutoRegTimerStatus == AR_TIMER_OUT_RETRY_MODE)
    {
        /* compare time*/
        if(autoRegRangTime > AR_RETRY_TIME_OUT)
        {
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg56, P_SIG, 0, "AutoRegSetTimer is larger than retry timeout");
            /* start register flow */
            autoRegTimerIsOut = AR_REGISTER_TIME_OUT;
            return AR_RC_FAIL;
        }
        else
        {
            autoRegRangTime = AR_RETRY_TIME_OUT - autoRegRangTime + 1;
            /* get curr time secs, and get last register time from flsh, sub them */
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg29, P_SIG, 1, "AutoRegSetTimer, regWaitTime: %d", autoRegRangTime);
            osTimerStart(autoRegTimer, autoRegRangTime*1000);
            return AR_RC_OK;
        }
    }
    else if(AutoRegTimerStatus == AR_TIMER_OUT_NORM_MODE)
    {
        /* compare time*/
        if(autoRegRangTime > autoRegTimerOutRang)
        {
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg57, P_SIG, 0, "AutoRegSetTimer is larger than rang timeout");
            /* start register flow */
            autoRegTimerIsOut = AR_REGISTER_TIME_OUT;
            return AR_RC_FAIL;
        }
        else
        {
            autoRegRangTime = autoRegTimerOutRang - autoRegRangTime + 1;  /*autoRegTimerOutRang is secs*/
            /* get curr time secs, and get last register time from flsh, sub them */
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg30, P_SIG, 1, "AutoRegSetTimer, regWaitTime: %d", autoRegRangTime);
            osTimerStart(autoRegTimer, autoRegRangTime*1000);
            return AR_RC_OK;
        }
    }
    else
    {
        return AR_RC_RECV_FAIL;
    }
}

INT32 AutoRegWaitTimer(void)
{
    INT32 i = 0;

    while(1)
    {
        if(autoRegTimerIsOut == AR_REGISTER_TIME_OUT)
        {
            ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg55, P_SIG, 0, "AutoRegWaitTimer...out");
            //coapClientDelete(0, "cu_coap");
            return AR_RC_OK;
        }
        else
        {
            i++;
            if(i == 80)//2 min print one time
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg31, P_SIG, 0, "AutoRegWaitTimer.........");
                i = 0;
            }
            osDelay(1500);
        }
    }
}


AutoRegProcSeqTbl AutoRegProcFuncTbl[] =
{
    /*index      func                        nextSeq/ok         nextSeq/fail     nextSeq     */
    {INDEX_01, AutoRegInit,            {INDEX_02,     INDEX_01,     INDEX_06}},
    {INDEX_02, AutoRegCreateClient,    {INDEX_03,     INDEX_02,     INDEX_02}},
    {INDEX_03, AutoRegCreatePacket,    {INDEX_04,     INDEX_03,     INDEX_03}},
    {INDEX_04, AutoRegSendPacket,      {INDEX_05,     INDEX_04,     INDEX_04}},
    {INDEX_05, AutoRegCheckRet,        {INDEX_06,     INDEX_06,     INDEX_06}},
    {INDEX_06, AutoRegSetTimer,        {INDEX_07,     INDEX_07,     INDEX_07}},
    {INDEX_07, AutoRegWaitTimer,       {INDEX_02,     INDEX_03,     INDEX_03}},
};
//volatile UINT32 seqNmb = INDEX_01;

void cuccAutoRegisterTask(void *argument)
{
    INT32 ret = AR_RC_FAIL;
    volatile UINT32 seqNmb = INDEX_01;
    UINT32 index = INDEX_01;

    slpManPlatVoteDisableSleep(cuccDmSlpHandler,SLP_SLP2_STATE);  //close sleep2 gate now, it can go to active or sleep1

    while(1)
    {
        ret = AutoRegProcFuncTbl[seqNmb].ProcFunc();
        index = seqNmb + 1;
        ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg60, P_SIG, 2, "...........AutoRegProcFuncTbl..indexNmb = %d...ret = %d", index, ret);
        switch(ret)
        {
            case AR_RC_OK:
            case AR_RC_CLIENT_RECREATE:
                seqNmb = AutoRegProcFuncTbl[seqNmb].nextFuncSeqNumb[0];
                break;

            case AR_RC_FAIL:
                seqNmb = AutoRegProcFuncTbl[seqNmb].nextFuncSeqNumb[1];
                break;

            case AR_RC_INIT_FAIL:
            case AR_RC_CLIENT_FAIL:
            case AR_RC_SEND_FAIL:
            case AR_RC_RECV_FAIL:
            case AR_RC_WAIT_TIMER:
                seqNmb = AutoRegProcFuncTbl[seqNmb].nextFuncSeqNumb[2];
                break;
        }
    }
}


/*auto reg task static stack and control block*/
osThreadId_t cuccAutoRegisterTaskHandle = NULL;

void cuccAutoRegisterInit(void)
{
    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));

    slpManApplyPlatVoteHandle("CUCC_DM",&cuccDmSlpHandler);

    task_attr.name = "cuccAutoReg";
    task_attr.stack_size = CUCC_AUTO_REG_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal5;

    ECOMM_TRACE(UNILOG_PLA_APP, cucc_autoReg25, P_SIG, 1, "autoReg..starting task ");

    cuccAutoRegisterTaskHandle= osThreadNew(cuccAutoRegisterTask, NULL,&task_attr);
    if(cuccAutoRegisterTaskHandle == NULL)
    {
        if(slpManGivebackPlatVoteHandle(cuccDmSlpHandler) != RET_TRUE)
        {
            ECOMM_TRACE(UNILOG_DM, CUCC_DM0, P_ERROR, 0, "CUCC_DM vote handle give back failed");
        }
        else
        {
            ECOMM_TRACE(UNILOG_DM, CUCC_DM1, P_VALUE, 0, "CUCC_DM vote handle give back success");
        }
    }
}


