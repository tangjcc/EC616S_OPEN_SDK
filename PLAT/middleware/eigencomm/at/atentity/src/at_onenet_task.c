/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_onenet_task.c
*
*  Description: Process onenet related AT commands RESP/IND
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "atec_onenet.h"
#include "cis_api.h"
#include "cis_if_sys.h"
#include "at_onenet_task.h"
#include "lfs_port.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#include "flash_ec616s_rt.h"
#endif

#include "ps_event_callback.h"
#include "arm_math.h"
#include "cmisim.h"
#include "netmgr.h"
#include "ps_lib_api.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "queue.h"
#include "cis_if_sys.h"
#include "cis_internals.h"

#define MIPLCONFIG_VALID_MAGIC 0x35331456
onenetTaskStatus gOnenetTaskRunStatus = TASK_STATUS_INIT;

onenetContext gOnenetContext[ONENET_INSTANCE_NUM]={{0}};
onenetRetentionContext gRetentionContext[ONENET_INSTANCE_NUM]={{0}};
uint8_t onenetSlpHandler           = 0xff;

OnenetSIMOTAContext gOnenetSimOtaCtx = {0};

QueueHandle_t onenet_atcmd_handle = NULL;
osThreadId_t onenet_atcmd_task_handle = NULL;
int onenet_atcmd_task_flag = ONENET_TASK_STAT_NONE;
osMutexId_t taskMutex = NULL;
static cisDLBufContext cisDlBufContex;
static osMutexId_t onenetDLBuffer_mutex = NULL;

extern observed_backup_t         g_observed_backup[MAX_OBSERVED_COUNT];

extern CHAR *defaultCisLocalPort;
extern void *g_cmiot_ota_context;

extern uint8_t    cissys_remove_psm(void);
extern uint8_t    cissys_recover_psm(void);


static INT32 onenetPsUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    switch(eventID)
    {
        case NB_URC_ID_MM_CERES_CHANGED:
        {
#ifndef FEATURE_REF_AT_ENABLE
            UINT8 celevel = 0;
            /*
             * 0 No Coverage Enhancement in the serving cell
             * 1 Coverage Enhancement level 0
             * 2 Coverage Enhancement level 1
             * 3 Coverage Enhancement level 2
             * 4 Coverage Enhancement level 3
            */
            celevel = *(UINT8 *)param;
            cissys_setCoapAckTimeout(celevel);
            ECOMM_TRACE(UNILOG_ONENET, onenetPsUrcCallback_0, P_INFO, 1, "onenetPsUrcCallback:NB_URC_ID_MM_CERES_CHANGED celevel=%d", celevel);
#endif
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            NmAtiNetifInfo *netif = (NmAtiNetifInfo *)param;
            cisnet_setNBStatus(netif->netStatus);
            ECOMM_TRACE(UNILOG_ONENET, onenetPsUrcCallback_1, P_INFO, 1, "onenetPsUrcCallback:NB_URC_ID_PS_NETINFO update to=%d", netif->netStatus);
            break;
        }

        default:
            break;
    }
    return 0;
}

static void onenetInitSleepHandler(void)
{
    slpManRet_t result=slpManFindPlatVoteHandle("ONENETSLP", &onenetSlpHandler);
    if(result==RET_HANDLE_NOT_FOUND)
        slpManApplyPlatVoteHandle("ONENETSLP",&onenetSlpHandler);
}

void onenetSleepVote(onenet_sleep_status_e bSleep)
{
    uint8_t counter = 0;
    slpManSlpState_t pstate;
    ECOMM_TRACE(UNILOG_ONENET, onenetSleepVote, P_INFO, 1, "platvote(%d)",bSleep);

    if(bSleep == SYSTEM_FREE)
    {
        if(RET_TRUE == slpManCheckVoteState(onenetSlpHandler, &pstate, &counter))
        {
            for(; counter > 0; counter -- )
            {
                slpManPlatVoteEnableSleep(onenetSlpHandler, SLP_SLP2_STATE);
            }
        }
    }
    else if(bSleep == SYSTEM_FREE_ONE)
    {
        slpManPlatVoteEnableSleep(onenetSlpHandler, SLP_SLP2_STATE);
    }
    else
    {
        if(RET_TRUE == slpManCheckVoteState(onenetSlpHandler, &pstate, &counter))
        {
            slpManPlatVoteDisableSleep(onenetSlpHandler, SLP_SLP2_STATE);
        }
    }
}


bool onenetCanSleep(void)
{
    uint8_t counter = 0;
    slpManSlpState_t pstate;

    slpManCheckVoteState(onenetSlpHandler, &pstate, &counter);

    if(counter == 0)
        return true;
    else
        return false;
}



UINT32 onenetHexToBin(UINT8 *dest, UINT8 *source, UINT32 max_dest_len)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    UINT32 i = 0;
    UINT8 lower_byte, upper_byte;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (source[i] != '\0') {
        if (source[i] >= '0' && source[i] <= '9') {
            lower_byte = source[i] - '0';
        } else if (source[i] >= 'a' && source[i] <= 'f') {
            lower_byte = source[i] - 'a' + 10;
        } else if (source[i] >= 'A' && source[i] <= 'F') {
            lower_byte = source[i] - 'A' + 10;
        } else {
            return 0;
        }

        if (source[i + 1] >= '0' && source[i + 1] <= '9') {
            upper_byte = source[i + 1] - '0';
        } else if (source[i + 1] >= 'a' && source[i + 1] <= 'f') {
            upper_byte = source[i + 1] - 'a' + 10;
        } else if (source[i + 1] >= 'A' && source[i + 1] <= 'F') {
            upper_byte = source[i + 1] - 'A' + 10;
        } else {
            return 0;
        }

        if ((i >> 1) >= max_dest_len) {
            return (i >> 1);
        }

        *(dest + (i >> 1)) = (lower_byte << 4) + upper_byte;
        i += 2;
    }

    return (i >> 1);
}

UINT32 onenetBinToHex(char *dest, const UINT8 *source, UINT32 max_dest_len)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    UINT32 i = 0, j = 0;
    UINT8 ch1, ch2;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (j + 1 < max_dest_len)
    {
        ch1 = (source[i] & 0xF0) >> 4;
        ch2 = source[i] & 0x0F;

        if (ch1 <= 9) {
            *(dest + j) = ch1 + '0';
        } else {
            *(dest + j) = ch1 + 'A' - 10;
        }

        if (ch2 <= 9) {
            *(dest + j + 1) = ch2 + '0';
        } else {
            *(dest + j + 1) = ch2 + 'A' - 10;
        }

        i++;
        j += 2;
    }

    *(dest + j) = '\0';
    return j;
}

onenetContext *onenetCreateInstance(void)
{
    /*----------------------------------------------------------------*/

    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        if (gOnenetContext[i].bUsed == false) {
            memset(&gOnenetContext[i], 0, sizeof(onenetContext));

#ifdef FEATURE_REF_AT_ENABLE
            UINT8 ackTimeout = 0;
            onenetRspTimeoutGet(0, &ackTimeout);
            cissys_setCoapAckTimeoutDirect(ackTimeout);
#endif
            cissys_setCoapMaxTransmitWaitTime();
            cissys_setCoapMinTransmitWaitTime();
            gOnenetContext[i].bUsed = true;
            gOnenetContext[i].onenetId = i;
            return &gOnenetContext[i];
        }
    }

    return NULL;
}

onenetContext *onenetSearchInstance(UINT32 onenetId)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        if (gOnenetContext[i].bUsed == true && gOnenetContext[i].onenetId == onenetId) {
            return &gOnenetContext[i];
        }
    }

    return NULL;
}

onenetContext *onenetSearchCisInstance(UINT32 onenetId)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        if (gOnenetContext[i].bUsed == true && gOnenetContext[i].onenetId == onenetId && gOnenetContext[i].cis_context != NULL) {
            return &gOnenetContext[i];
        }
    }

    return NULL;
}

onenetContext *onenetSearchCisContext(void *cis_context)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        if (gOnenetContext[i].bUsed == true && gOnenetContext[i].cis_context == cis_context) {
            return &gOnenetContext[i];
        }
    }

    return NULL;
}

void onenetDeleteInstance(onenetContext *onenet)
{
    if (onenet->bUsed == true) {
        onenet->bUsed = false;
    }
}

static void onenetDelAllObjects(onenetContext *onenet)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    onenetObjectInfo *objectInfo;
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_MAX_OBJECT_COUNT; i++) {
        objectInfo = &onenet->objectInfo[i];
        if (objectInfo->bUsed == 1) {
            objectInfo->bUsed = 0;
        }
    }
}

static void onenetSaveObject(onenetContext *onenet, cis_oid_t objectid, const cis_inst_bitmap_t *bitmap, const cis_res_count_t *resource)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    onenetObjectInfo *objectInfo;
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_MAX_OBJECT_COUNT; i++) {
        objectInfo = &onenet->objectInfo[i];
        if (objectInfo->bUsed == 0) {
            objectInfo->bUsed = 1;
            objectInfo->objectId = objectid;
            objectInfo->instCount = bitmap->instanceCount;
            memcpy(objectInfo->instBitmap, bitmap->instanceBitmap, bitmap->instanceBytes);
            objectInfo->attrCount = resource->attrCount;
            objectInfo->actCount = resource->actCount;
            return;
        }
    }
    
    ECOMM_TRACE(UNILOG_ONENET, onenetSaveObject_1, P_ERROR, 1, "No memory to store object info - no more than %d object",ONENET_MAX_OBJECT_COUNT);
    OsaCheck(0, 0, 0, 0);
}

static void onenetDelObject(onenetContext *onenet, cis_oid_t objectid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    onenetObjectInfo *objectInfo;
    UINT32 i;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    for (i = 0; i < ONENET_MAX_OBJECT_COUNT; i++) {
        objectInfo = &onenet->objectInfo[i];
        if (objectInfo->bUsed == 1 && objectInfo->objectId == objectid) {
            objectInfo->bUsed = 0;
            return;
        }
    }
}

BOOL atCheckForNumericOnlyChars(const UINT8 *password)
{
    INT16 passLen = 0;
    INT8 count = 0;
    BOOL result = TRUE;

    passLen = strlen((const CHAR*)password);
    if (passLen > 0)
    {
        /* Chech each digit */
        while ((result == TRUE) && (count < passLen))
        {
            if ((password[count] < '0') || (password[count] > '9'))
            {
                result = FALSE;
            }

            count++;
        }
    }

    return (result);
}

static BOOL prepareBuffer(UINT8* outBuf, UINT16* cursor, UINT8* inbuf, UINT8 inbuflen)
{
    UINT8 copyLen;
    UINT16 outbufCursor = *cursor;

    if(inbuflen == 0){
        return FALSE;
    }

    do {
        if((ONENET_CONFIG_MAX_BUFFER_LEN - outbufCursor) >= inbuflen) {
            copyLen = inbuflen;
        }else{
            return FALSE;
        }
        memcpy(outBuf + outbufCursor, inbuf, copyLen);
        outbufCursor += copyLen;
        inbuflen -= copyLen;
    }while(inbuflen);

    *cursor = outbufCursor;
    return TRUE;
}

static void putU16(UINT8* P, UINT16 v)
{
    UINT8 data[2] = {0};
    if(utils_checkBigendian()){
        data[0] = (v) & 0xFF;
        data[1] = (v>>8) & 0xFF;
    }else{
        data[0] = (v>>8) & 0xFF;
        data[1] = (v) & 0xFF;
    }
    memcpy(P,&data,2);
}

INT16 createConfigBuf(UINT32 onenetId, UINT8* buf)
{
    CHAR tempBuf[32];
    UINT16 bufCursor = 0;
    UINT16 netCfgLen = 0;
    UINT16 ipLen = 0;
    UINT16 portLen = 0;
    UINT16 userdataLen = 0;
    CHAR userdata[50];

    cisMIPLCfg *OnenetCfg = &gRetentionContext[onenetId].mMiplcfg;
    
    ipLen = strlen(OnenetCfg->ip);
    portLen = strlen(OnenetCfg->port);

    memset(buf, 0, ONENET_CONFIG_MAX_BUFFER_LEN);

    memset(tempBuf, 0, 32);
    tempBuf[0] = 0x13;
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 3);
    tempBuf[0] = 0xf1;
    tempBuf[1] = 0x00;
    tempBuf[2] = 0x03;
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 3);
    tempBuf[0] = 0xf2;
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 3);//let net cfg length empty first

    memset(tempBuf, 0, 32);
    putU16((UINT8*)tempBuf,MTU_SIZE);
    tempBuf[2] = 0x11;
    if(OnenetCfg->bsMode)                        //bs enable
        tempBuf[3] |= 0x80;
    if(OnenetCfg->dtls_enable)
        tempBuf[3] |= 0x40;

    putU16((UINT8*)&tempBuf[4],strlen("CMIOT"));//apn length
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 6);

    netCfgLen += 9;

    memset(tempBuf, 0, 32);
    snprintf(tempBuf, sizeof(tempBuf), "CMIOT");//apn is CMIOT
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, strlen(tempBuf));

    netCfgLen += strlen(tempBuf);

    memset(tempBuf, 0, 32);
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 4);//username and password is 0

    netCfgLen += 4;

    putU16((UINT8*)tempBuf, ipLen+portLen+1);
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 2);//host length

    netCfgLen += 2;

    memset(tempBuf, 0, 32);
    memcpy(tempBuf,OnenetCfg->ip,ipLen);
    tempBuf[ipLen] = ':';
    memcpy(&tempBuf[ipLen+1], OnenetCfg->port, portLen);
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, ipLen+portLen+1);//host

    netCfgLen = netCfgLen+ipLen+portLen+1;

    // if auth enable = 0, dtls enable = 0  auth_code or psk should be empty string
    snprintf(userdata, sizeof(userdata), "AuthCode:%s;PSK:%s;", OnenetCfg->auth_code, OnenetCfg->psk);

    userdataLen = strlen(userdata);
    memset(tempBuf, 0, 32);
    putU16((UINT8*)tempBuf,userdataLen);//userdata length
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 2);

    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)userdata, strlen(userdata));

    netCfgLen = netCfgLen+2+userdataLen;

    memset(tempBuf, 0, 32);
    tempBuf[0] = 0xf3;
    tempBuf[1] = 0x00;
    tempBuf[2] = 0x08;
    tempBuf[3] = 0xe4;  //log config
    tempBuf[4] = 0x00;
    tempBuf[5] = 0xc8;  //logBufferSize:200
    tempBuf[6] = 0x00;  //userdata length
    tempBuf[7] = 0x00;  //userdata
    prepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 8);

    putU16(buf+7, netCfgLen);
    putU16(buf+1, bufCursor);

    OsaCheck(bufCursor<=ONENET_CONFIG_MAX_BUFFER_LEN , bufCursor, 0, 0);

    return bufCursor;
}

void onenetServerIPGet(UINT32 onenetId, BOOL *pBsMode, CHAR* ip, CHAR* port)
{
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    mask = SaveAndSetIRQMask();
    *pBsMode = retContext->mMiplcfg.bsMode;
    memcpy(ip, retContext->mMiplcfg.ip, sizeof(retContext->mMiplcfg.ip));
    memcpy(port, retContext->mMiplcfg.port, sizeof(retContext->mMiplcfg.port));
    RestoreIRQMask(mask);
}

INT32 onenetServerIPConfig(UINT32 onenetId, BOOL bsMode, CHAR* ip, CHAR* port)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];

    mask = SaveAndSetIRQMask();
    retContext->mMiplcfg.bsMode = bsMode;
    memset(retContext->mMiplcfg.ip, 0, sizeof(retContext->mMiplcfg.ip));
    memset(retContext->mMiplcfg.port, 0, sizeof(retContext->mMiplcfg.port));
    memcpy(retContext->mMiplcfg.ip, ip, strlen(ip));
    memcpy(retContext->mMiplcfg.port, port, strlen(port));
    RestoreIRQMask(mask);
    onenetSaveFlash();

    return cis_error;
}


INT32 onenetRspTimeoutConfig(UINT32 onenetId, UINT8 rsp_timeout)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    if((rsp_timeout <= 20) && (rsp_timeout >= 2))
    {
        mask = SaveAndSetIRQMask();
        retContext->mMiplcfg.rsp_timeout = rsp_timeout;
        RestoreIRQMask(mask);
        onenetSaveFlash();
    }
    else
    {
        cis_error = CIS_ERRID_CONFIG_FAIL;
    }

    return cis_error;
}


void onenetRspTimeoutGet(UINT32 onenetId, UINT8 *rsp_timeout)
{
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    mask = SaveAndSetIRQMask();
    *rsp_timeout = retContext->mMiplcfg.rsp_timeout;
    RestoreIRQMask(mask);
}


INT32 onenetObsAutoackConfig(UINT32 onenetId, UINT8 obs_autoack)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;

    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    if(obs_autoack <= 1)
    {
        mask = SaveAndSetIRQMask();
        retContext->mMiplcfg.obs_autoack = obs_autoack;
        RestoreIRQMask(mask);
        onenetSaveFlash();
    }
    else
    {
        cis_error = CIS_ERRID_CONFIG_FAIL;
    }
    return cis_error;
}


void onenetObsAutoackGet(UINT32 onenetId, UINT8 *obs_autoack)
{
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    mask = SaveAndSetIRQMask();
    *obs_autoack = retContext->mMiplcfg.obs_autoack;
    RestoreIRQMask(mask);
}



INT32 onenetObsAuthConfig(UINT32 onenetId, BOOL auth_enable, UINT8 *auth_code)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;

    onenetRetentionContext *retContext = &gRetentionContext[onenetId];

    mask = SaveAndSetIRQMask();
    retContext->mMiplcfg.auth_enable = auth_enable;
    if(auth_enable)
    {
        memcpy(retContext->mMiplcfg.auth_code, auth_code, 17);
    }
    else
    {
        memset(retContext->mMiplcfg.auth_code, 0, 17);
    }
    RestoreIRQMask(mask);
    
    onenetSaveFlash();

    return cis_error;
}


void onenetObsAuthGet(UINT32 onenetId, UINT8 *auth_enable, UINT8 *auth_code)
{
    uint32_t mask;
    OsaCheck(auth_code != NULL, 0, 0, 0);
    OsaCheck(auth_enable != NULL, 0, 0, 0);
    
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    mask = SaveAndSetIRQMask();
    *auth_enable = retContext->mMiplcfg.auth_enable;
    if(retContext->mMiplcfg.auth_enable)
    {
        memcpy(auth_code, retContext->mMiplcfg.auth_code, 17);
    }
    RestoreIRQMask(mask);
}


INT32 onenetObsDTLSConfig(UINT32 onenetId, BOOL dtls_enable, UINT8 *psk)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;

    onenetRetentionContext *retContext = &gRetentionContext[onenetId];

    mask = SaveAndSetIRQMask();
    retContext->mMiplcfg.dtls_enable = dtls_enable;
    if(dtls_enable)
    {
        memcpy(retContext->mMiplcfg.psk, psk, 17);
    }
    else
    {
        memset(retContext->mMiplcfg.psk, 0, 17);
    }
    RestoreIRQMask(mask);
    
    onenetSaveFlash();

    return cis_error;
}


void onenetObsDTLSGet(UINT32 onenetId, UINT8 *dtls_enable, UINT8 *psk)
{
    uint32_t mask;
    OsaCheck(dtls_enable != NULL, 0, 0, 0);
    OsaCheck(psk != NULL, 0, 0, 0);
    
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    mask = SaveAndSetIRQMask();
    *dtls_enable = retContext->mMiplcfg.dtls_enable;
    if(retContext->mMiplcfg.dtls_enable)
    {
        memcpy(psk, retContext->mMiplcfg.psk , 17);
    }
    RestoreIRQMask(mask);
}


INT32 onenetWriteFormatConfig(UINT32 onenetId, UINT8 write_format)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;

    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    if(write_format <= 1)
    {
        mask = SaveAndSetIRQMask();
        retContext->mMiplcfg.write_format = write_format;
        RestoreIRQMask(mask);
        onenetSaveFlash();
    }
    else
    {
        cis_error = CIS_ERRID_CONFIG_FAIL;
    }
    return cis_error;
}


void onenetObsFormatGet(UINT32 onenetId, UINT8 *write_format)
{
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    mask = SaveAndSetIRQMask();
    *write_format = retContext->mMiplcfg.write_format;
    RestoreIRQMask(mask);
}



INT32 onenetMIPLRDConfig(UINT32 onenetId, UINT8 buf_cfg, UINT8 buf_urc_mode)
{
    INT32 cis_error = CIS_ERRID_OK;
    uint32_t mask;

    onenetRetentionContext *retContext = &gRetentionContext[onenetId];

    mask = SaveAndSetIRQMask();
    retContext->mMiplcfg.buf_cfg = (cisDLBufCfg)buf_cfg;
    retContext->mMiplcfg.buf_urc_mode = (cisDLBufURCMode)buf_urc_mode;
    RestoreIRQMask(mask);

    onenetSaveFlash();

    return cis_error;
}


void onenetMIPLRDCfgGet(UINT32 onenetId, UINT8 *buf_cfg, UINT8 *buf_urc_mode)
{
    uint32_t mask;
    cissys_assert(onenetId<ONENET_INSTANCE_NUM);
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    mask = SaveAndSetIRQMask();
    *buf_cfg = retContext->mMiplcfg.buf_cfg;
    *buf_urc_mode = retContext->mMiplcfg.buf_urc_mode;
    RestoreIRQMask(mask);
}


static void onenetSetDefaultCfg(UINT32 onenetId)
{
    uint32_t mask;
    onenetRetentionContext *retContext = &gRetentionContext[onenetId];
    
    mask = SaveAndSetIRQMask();
    retContext->mMiplcfg.bsMode = 1;
    memset(retContext->mMiplcfg.ip, 0, sizeof(retContext->mMiplcfg.ip));
    memset(retContext->mMiplcfg.port, 0, sizeof(retContext->mMiplcfg.port));
    snprintf(retContext->mMiplcfg.ip, sizeof(retContext->mMiplcfg.ip), "183.230.40.39");
    snprintf(retContext->mMiplcfg.port, sizeof(retContext->mMiplcfg.port), "5683");
#ifdef FEATURE_REF_AT_ENABLE
    retContext->mMiplcfg.rsp_timeout = 2;
    retContext->mMiplcfg.obs_autoack = 1;
#else
    retContext->mMiplcfg.rsp_timeout = 0;
    retContext->mMiplcfg.obs_autoack = 0;
#endif
    retContext->mMiplcfg.auth_enable = 0;
    memset(retContext->mMiplcfg.auth_code, 0, sizeof(retContext->mMiplcfg.auth_code));
    retContext->mMiplcfg.dtls_enable = 0;
    memset(retContext->mMiplcfg.psk, 0, sizeof(retContext->mMiplcfg.psk));
    retContext->mMiplcfg.write_format = 0;
    retContext->mMiplcfg.buf_cfg = CIS_DL_BUFCFG_BUF_NONE;
    retContext->mMiplcfg.buf_urc_mode = CIS_DL_URC_MODE_ON;
    retContext->mMiplcfg.valid_magic = MIPLCONFIG_VALID_MAGIC;
    RestoreIRQMask(mask);
}


INT32 onenetDelete(UINT32 onenetId)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else if(onenet->cis_context != NULL ){
        ECOMM_TRACE(UNILOG_ONENET, onenetDelete_1, P_INFO, 1, "cis_deinit start: 0x%x", onenet->cis_context);
        cis_ret_t cis_ret = cis_deinit(&onenet->cis_context);
        ECOMM_TRACE(UNILOG_ONENET, onenetDelete_2, P_INFO, 1, "cis_deinit end: %d", cis_ret);
        if (cis_ret != CIS_RET_OK) {
            cis_error = CIS_ERRID_SDK_ERROR;
        } else {
            onenetDelAllObjects(onenet);
            onenetDeleteInstance(onenet);
            onenet->bRestoreFlag = 0;
        }
    }
    return cis_error;
}

static INT32 onenetClose(UINT32 onenetId, psEventCallback_t urcHandler)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        cis_ret_t cis_ret = cis_unregister(onenet->cis_context);
        if (cis_ret != CIS_RET_OK) {
            cis_error = CIS_ERRID_SDK_ERROR;
        } else {
            onenet->bConnected = false;
            if (urcHandler != NULL)
                deregisterPSEventCallback(urcHandler);
        }
    }

    return cis_error;
}

static INT32 onenetDeinit(UINT32 onenetId)
{
    return onenetClose(onenetId, onenetPsUrcCallback);
}

static INT32 onenetUpdate(UINT32 onenetId, UINT32 lifetime, BOOL withObjectFlag, UINT8 raiflag)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        ECOMM_TRACE(UNILOG_ONENET, onenetUpdate_1, P_INFO, 2, "cis_update_reg start: 0x%x, %d, %d",
                       (UINT32)onenet->cis_context, (INT32)lifetime, (INT32)withObjectFlag);
        cis_ret_t cis_ret = cis_update_reg(onenet->cis_context, lifetime, withObjectFlag, raiflag);
        if (cis_ret != CIS_RET_OK) {
            cis_error = CIS_ERRID_SDK_ERROR;
        }
    }

    return cis_error;
}

static bool onenetSetCisData(cis_data_t *cis_data, UINT32 resourceid, UINT32 valuetype, UINT32 len, UINT8 *value)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    bool ret = true;
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    cis_data->id = (uint16_t)resourceid;
    cis_data->type = (cis_datatype_t)valuetype;
    switch (cis_data->type) {
        case cis_data_type_string:
            cis_data->asBuffer.length = len;
            cis_data->asBuffer.buffer = (UINT8 *)malloc(cis_data->asBuffer.length);
            memcpy(cis_data->asBuffer.buffer, value, cis_data->asBuffer.length);
            break;
        case cis_data_type_opaque:
            cissys_assert( len%2==0 );
            cis_data->asBuffer.buffer = (UINT8 *)pvPortMalloc(len/2 + 1);
            memset(cis_data->asBuffer.buffer, 0, len/2+1);
            cis_data->asBuffer.length = onenetHexToBin(cis_data->asBuffer.buffer, value, len/2);
            if(cis_data->asBuffer.length == 0)
            {
                ret = false;
                free(cis_data->asBuffer.buffer);
                cis_data->asBuffer.buffer = NULL;
            }
            break;
        case cis_data_type_integer:
            cis_data->value.asInteger = atoi((const char*)value);
            break;
        case cis_data_type_float:
            cis_data->value.asFloat = atof((const char*)value);
            break;
        case cis_data_type_bool:
            cis_data->value.asBoolean = atoi((const char*)value);
            break;
        default:
            cissys_assert(0);
    }
    return ret;
}

static void onenetFreeCisData(cis_data_t *cis_data)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    switch (cis_data->type) {
        case cis_data_type_string:
        case cis_data_type_opaque:
            free(cis_data->asBuffer.buffer);
            break;
        default:
            break;
    }
}

static void onenetSENDIND(UINT16 indBodyLen, void *indBody, UINT32 reqhandle)
{
    ECOMM_TRACE(UNILOG_ONENET, onenetSENDIND_1, P_INFO, 0, "send indication sig");

    if(indBodyLen == 0 || indBody == NULL)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetSENDIND_2, P_ERROR, 2, "invalid indBodyLen %u, indBody 0x%x ", indBodyLen, indBody);
        return;
    }

    applSendCmsInd(reqhandle, APPL_ONENET, APPL_ONENET_IND, indBodyLen+1, indBody);

    return;

}

static cis_coapret_t onenetREADCALLBACK(void *context, cis_uri_t *uri, cis_mid_t mid)
{
    char response_data[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    onenetContext *onenet = onenetSearchCisContext(context);
    if (onenet == NULL) {
        return CIS_CALLBACK_BAD_REQUEST;
    } else {
        // +MIPLREAD: <ref>, <msgid>, <objectid>, <instanceid>, <resourceid>
        if (CIS_URI_IS_SET_RESOURCE(uri)) {
            sprintf(response_data, "+MIPLREAD: %d,%d,%d,%d,%d",
                    (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)uri->resourceId);
        } else if (CIS_URI_IS_SET_INSTANCE(uri)) {
            sprintf(response_data, "+MIPLREAD: %d,%d,%d,%d,-1",
                    (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId);
        } else {
            sprintf(response_data, "+MIPLREAD: %d,%d,%d,-1,-1",
                    (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId);
        }
        onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
    }

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetWRITECALLBACK(void *context, cis_uri_t *uri, const cis_data_t *value, cis_attrcount_t attrcount, cis_mid_t mid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    char *response_data = NULL;
    char *string = NULL;
    UINT32 length;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchCisContext(context);
    if (onenet == NULL) {
        return CIS_CALLBACK_BAD_REQUEST;
    } else {
        UINT32 i, flag = ONENET_FLAG_LAST;
        // +MIPLWRITE: <ref>, <msgid>, <objectid>, <instanceid>, <resourceid>, <valuetype>, <len>, <value>, <flag>, <index>
        for (i = 0; i < attrcount; i++) {
            switch (value[i].type) {
                case cis_data_type_string:
                    length = value[i].asBuffer.length;
                    response_data = (char *)malloc(ONENET_AT_RESPONSE_DATA_LEN + length);
                    memset(response_data, 0, ONENET_AT_RESPONSE_DATA_LEN + length);
                    string = (char *)malloc(length + 1);
                    memset(string, 0, length + 1);
                    memcpy(string, value[i].asBuffer.buffer, length);
                    sprintf(response_data, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,\"%s\",%d,%d",
                            (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)uri->resourceId,
                            (INT32)value[i].type, (INT32)value[i].asBuffer.length, string,
                            (INT32)flag, (INT32)(attrcount - 1 - i));
                    free(string);
                    break;
                case cis_data_type_opaque:
                    length = value[i].asBuffer.length;
                    string = (char *)malloc(length * 2 + 1);
                    if (string == NULL) return CIS_CALLBACK_BAD_REQUEST;
                    onenetBinToHex(string, value[i].asBuffer.buffer, length * 2);
                    response_data = (char *)malloc(ONENET_AT_RESPONSE_DATA_LEN + length * 2);
                    if (response_data == NULL) {
                        free(string);
                        return CIS_CALLBACK_BAD_REQUEST;
                    }
                    sprintf(response_data, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d",
                            (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)value[i].id,
                            (INT32)value[i].type, (INT32)length, string,
                            (INT32)flag, (INT32)(attrcount - 1 - i));
                    if (string != NULL)
                        free(string);
                    break;
                case cis_data_type_integer:
                    response_data = (char *)malloc(ONENET_AT_RESPONSE_DATA_LEN);
                    if (response_data == NULL) return CIS_CALLBACK_BAD_REQUEST;
                    sprintf(response_data, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,1,%d,%d,%d",
                            (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)value[i].id,
                            (INT32)value[i].type, (INT32)value[i].value.asInteger,
                            (INT32)flag, (INT32)(attrcount - 1 - i));
                    break;
                case cis_data_type_float:
                    response_data = (char *)malloc(ONENET_AT_RESPONSE_DATA_LEN);
                    if (response_data == NULL) return CIS_CALLBACK_BAD_REQUEST;
                    sprintf(response_data, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,1,%lf,%d,%d",
                            (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)value[i].id,
                            (INT32)value[i].type, value[i].value.asFloat,
                            (INT32)flag, (INT32)(attrcount - 1 - i));
                    break;
                case cis_data_type_bool:
                    response_data = (char *)malloc(ONENET_AT_RESPONSE_DATA_LEN);
                    if (response_data == NULL) return CIS_CALLBACK_BAD_REQUEST;
                    sprintf(response_data, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,1,%d,%d,%d",
                            (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)value[i].id,
                            (INT32)value[i].type, (INT32)value[i].value.asBoolean,
                            (INT32)flag, (INT32)(attrcount - 1 - i));
                    break;
                default:
                    configASSERT(0);
                    //break;
            }
            if (response_data != NULL) 
            {
                if(onenetURCSendMode() == false)
                {
                    onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
                }
                else
                {
                    atcURC(AT_GET_HANDLER_CHAN_ID(onenet->reqhandle),(CHAR*)response_data);
                }
            }
            
            if (response_data != NULL) {
                free(response_data);
            }
        }
    }

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetEXECUTECALLBACK(void *context, cis_uri_t *uri, const uint8_t *buffer, uint32_t length, cis_mid_t mid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    char *response_data = NULL;
    char *display_buffer = NULL;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchCisContext(context);
    if (onenet == NULL) {
        return CIS_CALLBACK_BAD_REQUEST;
    } else {
        // +MIPLEXECUTE: <ref>, <msgid>, <objectid>, <instanceid>, <resourceid>[, <len>, <arguments>]
        display_buffer = (char *)malloc(length + 1);
        if (display_buffer == NULL) return CIS_CALLBACK_BAD_REQUEST;
        memcpy(display_buffer, buffer, length);
        display_buffer[length] = '\0';
        response_data = (char *)malloc(ONENET_AT_RESPONSE_DATA_LEN + length);
        if (response_data == NULL) {
            free(display_buffer);
            return CIS_CALLBACK_BAD_REQUEST;
        }
        sprintf(response_data, "+MIPLEXECUTE: %d,%d,%d,%d,%d,%d,\"%s\"",
                (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)uri->resourceId, (INT32)length, display_buffer);

        if(onenetURCSendMode() == false)
        {   
            onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
        }
        else
        {
            atcURC(AT_GET_HANDLER_CHAN_ID(onenet->reqhandle),(CHAR*)response_data);
        }

        free(display_buffer);
        free(response_data);
    }

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetOBSERVECALLBACK(void *context, cis_uri_t *uri, bool flag, cis_mid_t mid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    char response_data[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchCisContext(context);

    UINT8 obs_autoack = 0;
    if (onenet == NULL) {
        return CIS_CALLBACK_BAD_REQUEST;
    } else {
        onenetObsAutoackGet(onenet->onenetId, &obs_autoack);
        if(obs_autoack == 0)
        {
            // +MIPLOBSERVE: <ref>, <msgid>, <flag>, <objectid>, <instanceid>, <resourceid>
            if (CIS_URI_IS_SET_RESOURCE(uri)) {
                sprintf(response_data, "+MIPLOBSERVE: %d,%d,%d,%d,%d,%d",
                        (INT32)onenet->onenetId, (INT32)mid, (INT32)flag, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)uri->resourceId);
            } else if (CIS_URI_IS_SET_INSTANCE(uri)) {
                sprintf(response_data, "+MIPLOBSERVE: %d,%d,%d,%d,%d,-1",
                        (INT32)onenet->onenetId, (INT32)mid, (INT32)flag, (INT32)uri->objectId, (INT32)uri->instanceId);
            } else {
                sprintf(response_data, "+MIPLOBSERVE: %d,%d,%d,%d,-1,-1",
                        (INT32)onenet->onenetId, (INT32)mid, (INT32)flag, (INT32)uri->objectId);
            }
            onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
        }
        else
        {
            INT32 cis_error;
            cis_error = onenetResponse(onenet->onenetId, (UINT32)mid, NULL, NULL, CIS_RESPONSE_OBSERVE, 0);
            
            ECOMM_TRACE(UNILOG_ONENET, onenetOBSERVECALLBACK_1, P_SIG, 1, "Send AutoResponse: ret = %d",cis_error);
        }
    }

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetDISCOVERCALLBACK(void *context, cis_uri_t *uri, cis_mid_t mid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    char response_data[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchCisContext(context);
    if (onenet == NULL) {
        return CIS_CALLBACK_BAD_REQUEST;
    } else {
        // +MIPLDISCOVER: <ref>, <msgid>, <objectid>
        if (CIS_URI_IS_SET_RESOURCE(uri)) {
            return CIS_COAP_405_METHOD_NOT_ALLOWED;
        } else if (CIS_URI_IS_SET_INSTANCE(uri)) {
            return CIS_COAP_405_METHOD_NOT_ALLOWED;
        } else {
            sprintf(response_data, "+MIPLDISCOVER: %d,%d,%d", (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId);
        }
        onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
    }

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetSETPARAMCALLBACK(void *context, cis_uri_t *uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    char response_data[2 * ONENET_AT_RESPONSE_DATA_LEN] = {0};
    char parameter[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchCisContext(context);
    if (onenet == NULL) {
        return CIS_CALLBACK_BAD_REQUEST;
    } else {
        // +MIPLPARAMETER: <ref>, <msgid>, <objectid>, <instanceid>, <resourceid>, <len>, <parameter>
//        snprintf(parameter, ONENET_AT_RESPONSE_DATA_LEN, "pmin=%d;pmax=%d;gt=%d.0;lt=%d.0;st=%d.0",
//                (INT32)parameters.minPeriod, (INT32)parameters.maxPeriod, (int)parameters.greaterThan, (int)parameters.lessThan, (int)parameters.step);
        snprintf(parameter, ONENET_AT_RESPONSE_DATA_LEN, "pmin=%d;pmax=%d;gt=%.1f;lt=%.1f;st=%.1f",
                (INT32)parameters.minPeriod, (INT32)parameters.maxPeriod, (float)parameters.greaterThan, (float)parameters.lessThan, (float)parameters.step);
        snprintf(response_data, 2 * ONENET_AT_RESPONSE_DATA_LEN, "+MIPLPARAMETER: %d,%d,%d,%d,%d,%d,\"%s\"",
                (INT32)onenet->onenetId, (INT32)mid, (INT32)uri->objectId, (INT32)uri->instanceId, (INT32)uri->resourceId,
                (INT32)strlen(parameter), parameter);
        onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
    }

    return CIS_CALLBACK_CONFORM;
}

static void onenetEVENTCALLBACK(void *context, cis_evt_t id, void* param)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    char response_data[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/

    onenetContext *onenet = onenetSearchCisContext(context);
    st_context_t* ctx = (st_context_t*)onenet->cis_context;
    if (onenet == NULL) {
        return;
    } else {
        // +MIPLEVENT: <ref>, <evtid>[, <extend>]
        switch (id)
        {
            case CIS_EVENT_NETWORK_FAILED:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                break;
            case CIS_EVENT_BOOTSTRAP_FAILED:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                ctx->registerEnabled = false;
                onenetSleepVote(SYSTEM_FREE);
                break;
            case CIS_EVENT_NOTIFY_FAILED:
            case CIS_EVENT_NOTIFY_SUCCESS:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_1, P_SIG, 1, "CMD_NOTIFY enable slp2 event:%d",id);
                onenetSleepVote(SYSTEM_FREE_ONE);
                sprintf(response_data, "+MIPLEVENT: %d,%d,%d", (INT32)onenet->onenetId, (INT32)id, (INT32)param);
                break;

            case CIS_EVENT_NOTIFY_SEND:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_2, P_SIG, 0, "no ack notify has send enable slp2");
                onenetSleepVote(SYSTEM_FREE_ONE);
                break;

            case CIS_EVENT_UPDATE_NEED://still need update register
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_3, P_SIG, 0, "need update register");
                cis_update_reg(onenet->cis_context, onenet->lifeTime, false, 0);
                sprintf(response_data, "+MIPLEVENT: %d,%d,%d", (INT32)onenet->onenetId, (INT32)id, (INT32)param);
#if ONENET_AUTO_UPDATE_ENABLE
                cis_lifeTimeStart(onenet->lifeTime);
#endif
                break;

            case CIS_EVENT_RESPONSE_FAILED:
                sprintf(response_data, "+MIPLEVENT: %d,%d,%d", (INT32)onenet->onenetId, (INT32)id, (INT32)param);
                break;

            case CIS_EVENT_REG_SUCCESS:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
#if ONENET_AUTO_UPDATE_ENABLE
                cis_lifeTimeStart(onenet->lifeTime);
#endif
                onenet->bConnected = true;
                onenetSaveFlash();
                onenetSleepVote(SYSTEM_FREE);
                break;
                
            case CIS_EVENT_UNREG_DONE:
                applSendCmsCnf(onenet->closeHandle, APPL_RET_SUCC, APPL_ONENET, APPL_ONENET_CLOSE_CNF, 0, PNULL);
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
#if ONENET_AUTO_UPDATE_ENABLE
                cis_lifeTimeStop();
#endif
                onenetSaveFlash();
                onenetSleepVote(SYSTEM_FREE);
                break;

            case CIS_EVENT_UPDATE_SUCCESS:
            case CIS_EVENT_UPDATE_FAILED:
            case CIS_EVENT_UPDATE_TIMEOUT:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_4, P_SIG, 1, "CMD_UPDATE enable slp2",id);
                onenetSleepVote(SYSTEM_FREE_ONE);
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                break;

            case CIS_EVENT_OBSERVE_CANCEL:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_5, P_SIG, 0, "observe del changed");
                break;
            case CIS_EVENT_OBSERVE_ADD:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_6, P_SIG, 0, "observe add changed");
                onenetSaveFlash();
                break;
            case CIS_EVENT_FIRMWARE_DOWNLOADING:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                onenetSleepVote(SYSTEM_BUSY);
                break;
            case CIS_EVENT_FIRMWARE_UPDATING:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);                
                onenet->FotaState = 1;//set  OTA state for reset
                onenetSaveFlash();
                break;
            case CIS_EVENT_FIRMWARE_DOWNLOAD_DISCONNECT:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id); 
                onenet->FotaState = 0;
                onenetSaveFlash();
                onenetSleepVote(SYSTEM_FREE);
                break;
            case CIS_EVENT_FIRMWARE_UPDATE_SUCCESS:
            case CIS_EVENT_FIRMWARE_UPDATE_FAILED:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_7, P_VALUE, 1, "FOTA result(%d) clear FotaState",id);
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);                
                onenet->FotaState = 0;//clear OTA state
                onenetSaveFlash();//save the OTA state to flash
                
                #if ONENET_AUTO_UPDATE_ENABLE
                cis_lifeTimeStart(onenet->lifeTime);
                #endif
                break;
            case CIS_EVENT_MIPLRD_RECV:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                break;
            case CIS_EVENT_REG_TIMEOUT:
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_0, P_SIG, 1, "Network failed onenet vote sleep",id);
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                ctx->registerEnabled = false;
                onenetSaveFlash();
                onenetSleepVote(SYSTEM_FREE);
                break;
            default:
                sprintf(response_data, "+MIPLEVENT: %d,%d", (INT32)onenet->onenetId, (INT32)id);
                break;
        }
        if (strlen(response_data) != 0)
            onenetSENDIND(strlen(response_data),  response_data, onenet->reqhandle);
    }
}




static void onenetDLMutexAcquire(void)
{
    if(onenetDLBuffer_mutex == NULL)
        onenetDLBuffer_mutex = osMutexNew(NULL);

    OsaCheck(onenetDLBuffer_mutex != NULL, 0, 0, 0);

    osStatus_t result = osMutexAcquire(onenetDLBuffer_mutex, osWaitForever);

    if(result != osOK)
        ECOMM_TRACE(UNILOG_ONENET, onenetDLMutexAcquire_1, P_ERROR, 1, "MIPLRD->onenetDLMutexAcquire failed, result = %d",result);
}


static void onenetDLMutexRelease(void)
{
    OsaCheck(onenetDLBuffer_mutex != NULL, 0, 0, 0);

    osStatus_t result = osMutexRelease(onenetDLBuffer_mutex);

    if(result != osOK)
        ECOMM_TRACE(UNILOG_ONENET, onenetDLMutexRelease_1, P_ERROR, 1, "MIPLRD->onenetDLMutexRelease failed, result = %d",result);
}

static void onenetMIPLRDSlpSet(bool slp_enable)
{
    static uint8_t miplrd_slp = 0xff;
    uint8_t count;
    slpManSlpState_t state;
    slpManRet_t ret = RET_UNKNOW_FALSE;

    if(slp_enable)          
    {
        if(miplrd_slp == 0xff)
            return;

        ret = slpManPlatVoteEnableSleep(miplrd_slp, SLP_SLP2_STATE);

        OsaCheck(ret == RET_TRUE, 0, 0, 0);
        
        ret = slpManCheckVoteState(miplrd_slp, &state, &count);

        OsaCheck(ret == RET_TRUE, 0, 0, 0);

        OsaCheck(count == 0, state, count, 0);

        ret = slpManGivebackPlatVoteHandle(miplrd_slp);

        OsaCheck(ret == RET_TRUE, 0, 0, 0);

        miplrd_slp = 0xff;
        
    }
    else
    {
        if(miplrd_slp == 0xff)
        {
            ret = slpManApplyPlatVoteHandle("MIPLRD", &miplrd_slp);

            OsaCheck(ret == RET_TRUE, ret, miplrd_slp, 0);
        }

        ret = slpManPlatVoteDisableSleep(miplrd_slp, SLP_SLP2_STATE);

        OsaCheck(ret == RET_TRUE, 0, 0, 0);

        ret = slpManCheckVoteState(miplrd_slp, &state, &count);

        OsaCheck(ret == RET_TRUE, 0, 0, 0);

        OsaCheck(count == 1, state, count, 0);

    }

}


static void onenetDLFreeBuffer(uint8_t posIndex)
{
    OsaCheck(cisDlBufContex.buf[posIndex].valid == true, posIndex, 0, 0);

    cisDLBufType msg_type = cisDlBufContex.buf[posIndex].type;

    if(msg_type == CIS_DL_BUFFER_WRITE)
    {
        cis_data_t *pWriteBuf = (cis_data_t *)cisDlBufContex.buf[posIndex].data;
        cis_rescount_t resCount,resIndex;
        resCount = cisDlBufContex.buf[posIndex].resCount;

        ECOMM_TRACE(UNILOG_ONENET, onenetDLFreeBuffer_0, P_VALUE, 2, "MIPLRD->free write packet, Malloc Buf free = 0x%d, MsgId = %d", pWriteBuf, cisDlBufContex.buf[posIndex].mid);

        for (resIndex = 0;resIndex< resCount;resIndex++)
        {
            if(pWriteBuf[resIndex].type == cis_data_type_opaque || pWriteBuf[resIndex].type == cis_data_type_string)
            {
                cis_free(pWriteBuf[resIndex].asBuffer.buffer);
            }
        }
        cis_free(pWriteBuf);

        cisDlBufContex.buf[posIndex].valid = false;

        cisDlBufContex.total_len -= cisDlBufContex.buf[posIndex].len;
    }
    else if(msg_type == CIS_DL_BUFFER_EXCUTE)
    {
        void *pExecuteBuf = cisDlBufContex.buf[posIndex].data;

        ECOMM_TRACE(UNILOG_ONENET, onenetDLFreeBuffer_1, P_VALUE, 2, "MIPLRD->free excute packet, Malloc Buf free = 0x%d, MsgId = %d", pExecuteBuf, cisDlBufContex.buf[posIndex].mid);

        cis_free(pExecuteBuf);

        cisDlBufContex.buf[posIndex].valid = false;

        cisDlBufContex.total_len -= cisDlBufContex.buf[posIndex].len;
    }
    else
    {
        OsaCheck(0, msg_type, 0, 0);
    }

    ECOMM_TRACE(UNILOG_ONENET, onenetDLFreeBuffer_2, P_VALUE, 3, "MIPLRD->Free onenet DL Buffer, Index = %d, Type = %d, FreeLen = %d", posIndex, msg_type, cisDlBufContex.buf[posIndex].len);

}


/*
void onenetDLBufCfg(uint8_t buf_cfg, uint8_t buf_urc_mode)
{
    uint32_t mask = SaveAndSetIRQMask();
    cisDlBufContex.buf_cfg = (cisDLBufCfg)buf_cfg;
    cisDlBufContex.buf_urc_mode = (cisDLBufURCMode)buf_urc_mode;
    RestoreIRQMask(mask);

    ECOMM_TRACE(UNILOG_ONENET, onenetDLBufCfg_1, P_VALUE, 1, "MIPLRD->onenetDLBufCfg, buf_cfg=%d, buf_urc_mode=%d", buf_cfg, buf_urc_mode);
}
*/


void onenetDLPush2Buffer(cisDLBufferInfo bufInfo)
{
    uint8_t next_input_pos;
    uint8_t tmp_pos;
    uint16_t tmp_total_len;
    uint8_t drop_cnt = 0;
    uint8_t tmp_buffer_cnt = 0;
    UINT8 buf_cfg, buf_urc_mode;

    OsaCheck(bufInfo.len <= CIS_MAX_DL_BUFFER_LEN, bufInfo.len, bufInfo.mid, bufInfo.type);

    onenetDLMutexAcquire();

    for(uint8_t i=0; i<CIS_MAX_DL_BUFFER_NUM; i++)
    {
        if(cisDlBufContex.buf[i].valid == true)
        {
            tmp_buffer_cnt++;
        }
    }

    ECOMM_TRACE(UNILOG_ONENET, onenetDLPush2Buffer_0, P_VALUE, 2, "MIPLRD->cache malloc header, Malloc Buf Cached = 0x%d, MsgId = %d", bufInfo.data, bufInfo.mid);

    if((cisDlBufContex.fifo_header == cisDlBufContex.fifo_ending) && (tmp_buffer_cnt == 0))     //empty
    {
        OsaCheck(tmp_buffer_cnt == 0, cisDlBufContex.fifo_header, cisDlBufContex.fifo_ending, 0);

        next_input_pos = (cisDlBufContex.fifo_ending + 1) % CIS_MAX_DL_BUFFER_NUM;

        ECOMM_TRACE(UNILOG_ONENET, onenetDLPush2Buffer_1, P_VALUE, 1, "MIPLRD->Buffer Empty, New data put to = %d", next_input_pos);

        tmp_total_len = cisDlBufContex.total_len + bufInfo.len;
    }
    else
    {
        next_input_pos = (cisDlBufContex.fifo_ending + 1) % CIS_MAX_DL_BUFFER_NUM;

        if(next_input_pos == ((cisDlBufContex.fifo_header+1) % CIS_MAX_DL_BUFFER_NUM))  // full and several data may drop, according to data len
        {
            tmp_total_len = cisDlBufContex.total_len + bufInfo.len - cisDlBufContex.buf[next_input_pos].len;

            onenetDLFreeBuffer(next_input_pos);

            drop_cnt = 1;

            cisDlBufContex.drop_cnt++;

            cisDlBufContex.fifo_header = (cisDlBufContex.fifo_header + 1) % CIS_MAX_DL_BUFFER_NUM;


            ECOMM_TRACE(UNILOG_ONENET, onenetDLPush2Buffer_2, P_VALUE, 1, "MIPLRD->Buffer Full, Drop data @ Index = %d", next_input_pos);
        }
        else
        {
            tmp_total_len = cisDlBufContex.total_len + bufInfo.len;
        }

    }

    while(tmp_total_len > CIS_MAX_DL_BUFFER_LEN)
    {
        tmp_pos = (cisDlBufContex.fifo_header + 1) % CIS_MAX_DL_BUFFER_NUM;

        tmp_total_len = tmp_total_len - cisDlBufContex.buf[tmp_pos].len;

        onenetDLFreeBuffer(tmp_pos);

        drop_cnt++;

        cisDlBufContex.drop_cnt++;

        cisDlBufContex.fifo_header = tmp_pos;

        ECOMM_TRACE(UNILOG_ONENET, onenetDLPush2Buffer_3, P_VALUE, 2, "MIPLRD->Buffer Length full, Limit to %d, Drop data @ Index = %d", CIS_MAX_DL_BUFFER_LEN, tmp_pos);
    }

    OsaCheck(drop_cnt <= CIS_MAX_DL_BUFFER_NUM, drop_cnt, next_input_pos, 0);

    cisDlBufContex.total_len = tmp_total_len;

    cisDlBufContex.fifo_ending = next_input_pos;

    OsaCheck(cisDlBufContex.buf[next_input_pos].valid == false, cisDlBufContex.fifo_header, cisDlBufContex.fifo_ending, cisDlBufContex.total_len);

    memcpy(&cisDlBufContex.buf[next_input_pos], &bufInfo, sizeof(bufInfo));

    cisDlBufContex.buf[next_input_pos].valid = true;

    cisDlBufContex.recv_cnt++;

    onenetMIPLRDCfgGet(0, &buf_cfg, &buf_urc_mode);

    onenetDLMutexRelease();

    if(tmp_buffer_cnt == 0)
    {
        if(buf_urc_mode == 1)
            core_callbackEvent(bufInfo.context, CIS_EVENT_MIPLRD_RECV, NULL);

        onenetMIPLRDSlpSet(false);
    }

    ECOMM_TRACE(UNILOG_ONENET, onenetDLPush2Buffer_4, P_VALUE, 8, "MIPLRD->PUSH, InputPos = %d, Previous BufCnt = %d, Current header = %d, ending = %d, PushSize = %d, totalSize = %d, recvCnt = %d, dropCnt = %d",
                                next_input_pos, tmp_buffer_cnt, cisDlBufContex.fifo_header, cisDlBufContex.fifo_ending, bufInfo.len, cisDlBufContex.total_len, cisDlBufContex.recv_cnt, cisDlBufContex.drop_cnt);

}


bool onenetDLPopout(void)
{
    uint8_t posIndex = 0;
    uint8_t i;
    uint8_t tmp_buffer_cnt;
    cisDLBufType type;
    st_context_t *contextP;
    st_uri_t m_uri;

    onenetDLMutexAcquire();

    tmp_buffer_cnt = 0;
    
    for(i=0; i<CIS_MAX_DL_BUFFER_NUM; i++)
    {
        if(cisDlBufContex.buf[i].valid == true)
        {
            tmp_buffer_cnt++;
        }
    }
//  ECOMM_TRACE(UNILOG_ONENET, onenetDLPopout_2, P_SIG, 3, "MIPLRD->DLPopout = %d, %d, %d",tmp_buffer_cnt,cisDlBufContex.fifo_header,posIndex);

    if(tmp_buffer_cnt == 0)
    {
        onenetDLMutexRelease();
        
        return false;
    }
    posIndex = (cisDlBufContex.fifo_header + 1) % CIS_MAX_DL_BUFFER_NUM;

    OsaCheck(cisDlBufContex.buf[posIndex].valid == true, posIndex, 0, 0);

    type = cisDlBufContex.buf[posIndex].type;

    contextP = (st_context_t *)cisDlBufContex.buf[posIndex].context;

    m_uri.flag = cisDlBufContex.buf[posIndex].uri_flag;
    m_uri.objectId = cisDlBufContex.buf[posIndex].uri_objectId;
    m_uri.instanceId = cisDlBufContex.buf[posIndex].uri_instanceId;
    m_uri.resourceId = cisDlBufContex.buf[posIndex].uri_resourceId;

    if(type == CIS_DL_BUFFER_WRITE)
    {
        cisDlBufContex.fifo_header = posIndex;
        cisDlBufContex.urc_direct_send = true;
        contextP->callback.onWrite(contextP,&m_uri,cisDlBufContex.buf[posIndex].data,cisDlBufContex.buf[posIndex].resCount,cisDlBufContex.buf[posIndex].mid);
        cisDlBufContex.urc_direct_send = false;
        onenetDLFreeBuffer(posIndex);
    }
    else if(type == CIS_DL_BUFFER_EXCUTE)
    {
        cisDlBufContex.fifo_header = posIndex;
        cisDlBufContex.urc_direct_send = true;
        contextP->callback.onExec(contextP,&m_uri,cisDlBufContex.buf[posIndex].data,cisDlBufContex.buf[posIndex].len,cisDlBufContex.buf[posIndex].mid);
        cisDlBufContex.urc_direct_send = false;
        onenetDLFreeBuffer(posIndex);
    }
    else
        OsaCheck(0, type, 0, 0);

    onenetDLMutexRelease();

    ECOMM_TRACE(UNILOG_ONENET, onenetDLPopout_1, P_VALUE, 8, "MIPLRD->POP, OutputPos = %d, Previous BufCnt = %d, Current header = %d, ending = %d, PopSize = %d, totalSize = %d, recvCnt = %d, dropCnt = %d",
                                posIndex, tmp_buffer_cnt, cisDlBufContex.fifo_header, cisDlBufContex.fifo_ending, cisDlBufContex.buf[posIndex].len, cisDlBufContex.total_len, cisDlBufContex.recv_cnt, cisDlBufContex.drop_cnt);

    tmp_buffer_cnt = 0;

    for(i=0; i<CIS_MAX_DL_BUFFER_NUM; i++)
    {
        if(cisDlBufContex.buf[i].valid == true)
        {
            tmp_buffer_cnt++;
        }
    }
    if(tmp_buffer_cnt == 0)
    {
        onenetMIPLRDSlpSet(true);
    }

    return true;

}


bool onenetURCSendMode(void)
{
    return cisDlBufContex.urc_direct_send;
}


void onenetDLBufferInfoCheck(uint8_t *pBuffered, uint16_t *pBufferedSize, uint32_t *pRecvCnt, uint32_t *pDropCnt)
{
    uint8_t i;
    uint8_t tmp_buffer_cnt = 0;

    uint32_t mask = SaveAndSetIRQMask();

    for(i=0; i<CIS_MAX_DL_BUFFER_NUM; i++)
    {
        if(cisDlBufContex.buf[i].valid == true)
        {
            tmp_buffer_cnt++;
        }
    }

    *pBuffered = tmp_buffer_cnt;
    *pBufferedSize = cisDlBufContex.total_len;
    *pRecvCnt = cisDlBufContex.recv_cnt;
    *pDropCnt = cisDlBufContex.drop_cnt;

    RestoreIRQMask(mask);

}

/*
void onenetDLBufferCfgGet(cisDLBufCfg *p_buf_cfg, cisDLBufURCMode *p_buf_urc_mode)
{
    uint32_t mask = SaveAndSetIRQMask();

    *p_buf_cfg = cisDlBufContex.buf_cfg;
    *p_buf_urc_mode = cisDlBufContex.buf_urc_mode;

    RestoreIRQMask(mask);
}
*/

bool onenetDLBufferIsEnable(UINT32 onenetId, cisDLBufType type)
{
    UINT8 buf_cfg, buf_urc_mode;
    
    onenetMIPLRDCfgGet(onenetId, &buf_cfg, &buf_urc_mode);

    ECOMM_TRACE(UNILOG_ONENET, onenetDLBufferIsEnable_1, P_VALUE, 3, "MIPLRD->Get Setting, OnenetId=%d, type=%d, buf_cfg=%d", onenetId, type, buf_cfg);

    if((CIS_DL_BUFFER_WRITE == type) && ((buf_cfg == CIS_DL_BUFCFG_BUF_WRITE) || (buf_cfg == CIS_DL_BUFCFG_BUF_WRITE_AND_EXCUTE)))
    {
        return true;
    }

    if((CIS_DL_BUFFER_EXCUTE == type) && ((buf_cfg == CIS_DL_BUFCFG_BUF_EXECUTE) || (buf_cfg == CIS_DL_BUFCFG_BUF_WRITE_AND_EXCUTE)))
    {
        return true;
    }

    return false;
}


#if ONENET_AUTO_UPDATE_ENABLE

static bool cisDeepSlpTimerExpFlag = false;

void cis_lifeTimeStart(uint32_t timeout_second)
{
    uint32_t notifytime;
    if(timeout_second > cissys_getCoapMaxTransmitWaitTime() * 2)
    {
        notifytime = cissys_getCoapMaxTransmitWaitTime();
    }
    else if(timeout_second > cissys_getCoapMaxTransmitWaitTime())
    {
        notifytime = cissys_getCoapMinTransmitWaitTime();
    }
    else
    {
        notifytime = (timeout_second * (1 - ((float)timeout_second / 120)));
    }

    notifytime = notifytime + 1;    // make it 1 second earlier to avoid CIS_EVENT_UPDATE_NEED trigger

    if(timeout_second >= ONENET_AUTO_UPDATE_THD)
    {
        slpManDeepSlpTimerStart(CIS_LIFETIMEOUT_ID, timeout_second*1000-notifytime*1000);
 
        cisDeepSlpTimerExpFlag = false;

        ECOMM_TRACE(UNILOG_PLA_APP, cis_lifeTimeoutStart_1, P_VALUE, 3, "Onenet LifeTime start Deepsleep Timer Id = %u, Duration = %d second, NotifyTime = %d", CIS_LIFETIMEOUT_ID, timeout_second, notifytime);
    }
}


void cis_lifeTimeStop(void)
{
    slpManDeepSlpTimerDel(CIS_LIFETIMEOUT_ID);
    
    cisDeepSlpTimerExpFlag = false;
    
    ECOMM_TRACE(UNILOG_PLA_APP, cis_lifeTimeStop_1, P_VALUE, 0, "Onenet LifeTime Stop");

}


bool cis_DeepSlpTimerHasExp(void)
{
    return cisDeepSlpTimerExpFlag;
}


void cis_lifeTimeExpCallback(void)
{
    cisDeepSlpTimerExpFlag = true;
    
    onenetSleepVote(SYSTEM_BUSY);
}

#endif

static INT32 onenetNotify(UINT32 onenetId, onenetResuourcCmd* cmd)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;
    cis_data_t cis_data;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        cis_uri_t uri;
        uri.objectId = cmd->objectid;
        uri.instanceId = cmd->instanceid;
        uri.resourceId = cmd->resourceid;
        cis_uri_update(&uri);
        memset(&cis_data, 0, sizeof(cis_data_t));
        if(onenetSetCisData(&cis_data, cmd->resourceid, cmd->valuetype, cmd->len, cmd->value) == true)
        {
            cis_ret_t cis_ret;
            cis_ret = cis_notify_ec(onenet->cis_context, &uri, &cis_data, (cis_mid_t)(cmd->msgid), cmd->result, cmd->ackid, cmd->raiflag);
            onenetFreeCisData(&cis_data);
            if (cis_ret < CIS_RET_OK) {
                cis_error = CIS_ERRID_SDK_ERROR;
            }
        }
        else
            cis_error = CIS_ERRID_PARAMETER_ERROR;
    }

    return cis_error;
}

INT32 onenetReadResp(UINT32 onenetId, UINT32 msgid, UINT32 objectid, UINT32 instanceid, UINT32 resourceid, UINT32 valuetype, UINT32 len, UINT8 *value, cis_coapret_t result, UINT8 raiflag)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;
    cis_data_t cis_data;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        cis_uri_t uri;
        uri.objectId = objectid;
        uri.instanceId = instanceid;
        uri.resourceId = resourceid;
        cis_uri_update(&uri);
        memset(&cis_data, 0, sizeof(cis_data_t));
        if(onenetSetCisData(&cis_data, resourceid, valuetype, len, value) == true)
        {
            cis_ret_t cis_ret;
            cis_ret = cis_response(onenet->cis_context, &uri, &cis_data, (cis_mid_t)msgid, result, raiflag);
            onenetFreeCisData(&cis_data);
            if (cis_ret < CIS_RET_OK) {
                cis_error = CIS_ERRID_SDK_ERROR;
            }
        }
        else
            cis_error = CIS_ERRID_PARAMETER_ERROR;
    }

    return cis_error;
}

INT32 onenetAddObj(UINT32 onenetId, UINT32 objectid, UINT32 instancecount, UINT8 *instancebitmap, UINT32 attributecount, UINT32 actioncount)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        UINT32 i;
        UINT8 *instancebitmap_bytes;
        cis_inst_bitmap_t bitmap;
        cis_res_count_t resource;
        onenetObjectInfo *objectInfo;

        for (i = 0; i < ONENET_MAX_OBJECT_COUNT; i++) 
        {
            objectInfo = &onenet->objectInfo[i];
            if (objectInfo->bUsed == 0)
                break;
        }
        if( i == ONENET_MAX_OBJECT_COUNT )
            return CIS_ERRID_NO_OBJMEM;

        if (instancebitmap == NULL || instancecount == 0 || strlen((const char*)instancebitmap) != instancecount) {
            ECOMM_TRACE(UNILOG_ONENET, onenetAddObj_erro1, P_WARNING, 3,
                        "ONENET, ADD invalid OBJ, instanceBitmapAddr: 0x%lx, instancecount: %d, strlen(instanceBitmap) = %d",
                        instancebitmap, instancecount, strlen((const char*)instancebitmap));

            return CIS_ERRID_PARAMETER_ERROR;
        }
        bitmap.instanceCount = instancecount;
        bitmap.instanceBytes = (bitmap.instanceCount + 7) / 8;
        instancebitmap_bytes = (UINT8 *)malloc(bitmap.instanceBytes + 1);
        if (instancebitmap_bytes == NULL) {
            cis_error = CIS_ERRID_MALLOC_FAIL;
        } else {
            memset(instancebitmap_bytes, 0, bitmap.instanceBytes+1);
            for (i = 0; i < bitmap.instanceCount; i++) {
                UINT8 pos = i / 8;
                UINT8 offset = 7 - (i % 8);
                if (instancebitmap[bitmap.instanceCount - i - 1] == '1') {
                    instancebitmap_bytes[pos] += 0x01 << offset;
                }
            }
            bitmap.instanceBitmap = instancebitmap_bytes;
            resource.attrCount = attributecount;
            resource.actCount = actioncount;
            #if 0
            UINT8 waitTimes = 0;
            while(onenet->cis_context == NULL){
                osDelay(200);
                waitTimes++;
                if(waitTimes>5)
                    break;
                ECOMM_TRACE(UNILOG_ONENET, onenetAddObj_0, P_INFO, 1, "cis_init hasn't complete wait %d*200ms", waitTimes);
            }
            #endif
            if(onenet->cis_context == NULL){
                cis_error = CIS_ERRID_SDK_ERROR;
            }else{
                cis_ret_t cis_ret = cis_addobject(onenet->cis_context, (cis_oid_t)objectid, &bitmap, &resource);
                if (cis_ret != CIS_RET_OK) {
                    cis_error = CIS_ERRID_SDK_ERROR;
                } else {
                    onenetSaveObject(onenet, (cis_oid_t)objectid, &bitmap, &resource);
                    ECOMM_TRACE(UNILOG_ONENET, onenetAddObj_1, P_INFO, 0, "save flash");
                    onenetSaveFlash();//save the add object info to flash
                }
            }
        }

        if (instancebitmap_bytes != NULL) {
            free(instancebitmap_bytes);
        }
    }

    return cis_error;
}

INT32 onenetDelObj(UINT32 onenetId, UINT32 objectid)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        cis_ret_t cis_ret = cis_delobject(onenet->cis_context, (cis_oid_t)objectid);
        if (cis_ret != CIS_RET_OK) {
            cis_error = CIS_ERRID_SDK_ERROR;
        } else {
            onenetDelObject(onenet, (cis_oid_t)objectid);
        }
    }

    return cis_error;
}

static INT32 onenetOpen(UINT32 onenetId, cis_time_t lifetime, cis_time_t timeout)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;
    st_context_t* ctx = NULL;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        ECOMM_TRACE(UNILOG_ONENET, onenetOpen_1, P_INFO, 1, "cis_register start: 0x%x", (UINT32)onenet->cis_context);
        cis_callback_t callback;
        callback.onRead = onenetREADCALLBACK;
        callback.onWrite = onenetWRITECALLBACK;
        callback.onExec = onenetEXECUTECALLBACK;
        callback.onObserve = onenetOBSERVECALLBACK;
        callback.onDiscover = onenetDISCOVERCALLBACK;
        callback.onSetParams = onenetSETPARAMCALLBACK;
        callback.onEvent = onenetEVENTCALLBACK;
        ctx = (st_context_t*)onenet->cis_context;
        ctx->regTimeout = timeout;
        cis_ret_t cis_ret = cis_register(onenet->cis_context, lifetime, &callback);
        ECOMM_TRACE(UNILOG_ONENET, onenetOpen_2, P_INFO, 1, "cis_register end: %d", (INT32)cis_ret);
        if (cis_ret != CIS_RET_OK) {
            cis_error = CIS_ERRID_SDK_ERROR;
        } else {
            onenet->lifeTime = lifetime;
            onenet->timeout = timeout;
        }
    }

    return cis_error;
}

BOOL onenetResult2Coapret(UINT32 cmdid, onenet_at_result_t result, cis_coapret_t *coapret)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (coapret == NULL) {
        return false;
    }

    switch (result) {
        case ONENET_OP_RESULT_205_CONTENT:
            if (cmdid == ONENET_CMD_READRESP || cmdid == ONENET_CMD_OBSERVERESP || cmdid == ONENET_CMD_DISCOVERRESP) {
                *coapret = CIS_COAP_205_CONTENT;
                return true;
            }
            break;

        case ONENET_OP_RESULT_204_CHANGED:
            if (cmdid == ONENET_CMD_WRITERESP || cmdid == ONENET_CMD_EXECUTERESP || cmdid == ONENET_CMD_PARAMRESP) {
                *coapret = CIS_COAP_204_CHANGED;
                return true;
            }
            break;

        case ONENET_OP_RESULT_400_BAD_REQUEST:
            *coapret = CIS_COAP_400_BAD_REQUEST;
            return true;

        case ONENET_OP_RESULT_401_UNAUTHORIZED:
            *coapret = CIS_COAP_401_UNAUTHORIZED;
            return true;

        case ONENET_OP_RESULT_404_NOT_FOUND:
            *coapret = CIS_COAP_404_NOT_FOUND;
            return true;

        case ONENET_OP_RESULT_405_METHOD_NOT_ALLOWED:
            *coapret = CIS_COAP_405_METHOD_NOT_ALLOWED;
            return true;

        case ONENET_OP_RESULT_406_NOT_ACCEPTABLE:
            if (cmdid == ONENET_CMD_READRESP || cmdid == ONENET_CMD_OBSERVERESP || cmdid == ONENET_CMD_DISCOVERRESP) {
                *coapret = CIS_COAP_406_NOT_ACCEPTABLE;
                return true;
            }
            break;

        default:
            return false;
    }

    return false;
}

INT32 onenetResponse(UINT32 onenetId, UINT32 msgid, const cis_uri_t *uri, const cis_data_t *cis_data, cis_coapret_t result, uint8_t raiflag)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 cis_error = CIS_ERRID_OK;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if (onenet == NULL) {
        cis_error = CIS_ERRID_NO_INSTANCE;
    } else {
        cis_ret_t cis_ret = cis_response(onenet->cis_context, uri, cis_data, (cis_mid_t)msgid, result, raiflag);
        if (cis_ret < CIS_RET_OK) {
            cis_error = CIS_ERRID_SDK_ERROR;
        }
    }

    return cis_error;
}

UINT32 onenetParseAttr(const char *valueString, const char *buffer, const char *key[], char *attrList[], UINT32 attrMaxNum, const char *delim)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    INT32 i;
    UINT32 attrNum = 0;
    char *p = strtok((char *)valueString, delim);
    char *field = (char *)buffer;
    UINT32 fieldLen;
    char *attrValue = NULL;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    while (p != NULL && *p != '\0' && attrNum < attrMaxNum) {
        strcpy(field, p);
        // remove leading blanks
        while (*field == 0x20) {
            field++;
        }
        // remove post blanks
        fieldLen = strlen(field);
        for (i = fieldLen - 1; i >= 0; i--) {
            if (*(field + i) == 0x20) {
                *(field + i) = '\0';
            } else {
                break;
            }
        }
        if (key != NULL) {
            // search '='
            attrValue = strchr(field, '=');
            if (attrValue == NULL || strncmp(field, key[attrNum], strlen(key[attrNum])) != 0) {
                break;
            }
            attrValue++;
            // remove leading blanks
            while (*attrValue == 0x20) {
                attrValue++;
            }
        } else {
            attrValue = field;
        }
        // add to attrList
        attrList[attrNum++] = attrValue;

        field = field + strlen(field) + 1;
        p = strtok(NULL, delim);
    }

    for (i = 0; i < attrNum; i++) {
    }

    return attrNum;
}


static uint32_t check_sum(uint8_t *buffer, uint32_t size)
{
    uint32_t i   = 0;
    uint32_t sum = 0;
    
    for( i=0; i<size; i++ )
    {
        sum += buffer[i];
    }
    return sum;
}


void onenetClearFlash(bool ToClearCfg)
{
    onenetRetentionContext* retentCxt;
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cisNvmHeader nvmHdr;   //4 bytes
    UINT8 tmpOtaState = gRetentionContext[0].otaFinishState;

    cisMIPLCfg tmpMiplCfg;
        
    for (int i = 0; i < ONENET_INSTANCE_NUM; i++) {
        retentCxt = &gRetentionContext[i];

        if(!ToClearCfg)
            tmpMiplCfg = retentCxt->mMiplcfg;

        memset(retentCxt,0,sizeof(onenetRetentionContext));

        if(!ToClearCfg)
            retentCxt->mMiplcfg = tmpMiplCfg;           //don't clear miplconfig state
    }
    gRetentionContext[0].otaFinishState = tmpOtaState; //don't clear otaFinishState
    ECOMM_TRACE(UNILOG_ONENET, onenetClearFlash_0, P_SIG, 0, "clear ctx save flash begin");

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetClearFlash_1, P_ERROR, 0, "can't open/create NVM: cisconfig.nvm, save NVM failed");
        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(onenetRetentionContext) * ONENET_INSTANCE_NUM;
    nvmHdr.version        = CIS_NVM_FILE_VERSION_1;
    nvmHdr.checkSum       = check_sum((uint8_t *)gRetentionContext, sizeof(onenetRetentionContext)*ONENET_INSTANCE_NUM);

    writeCount = OsaFwrite(&nvmHdr, sizeof(cisNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetClearFlash_2, P_ERROR, 0, "cisconfig.nvm, write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(gRetentionContext, sizeof(onenetRetentionContext), ONENET_INSTANCE_NUM, fp);
    if (writeCount != ONENET_INSTANCE_NUM)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetClearFlash_3, P_ERROR, 0, "cisconfig.nvm, write the file body failed");
    }

    OsaFclose(fp);
}

void onenetSaveFlash(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cisNvmHeader nvmHdr;   //4 bytes
    onenetContext *onenet;
    onenetRetentionContext* retentCxt;
    for (int i = 0; i < ONENET_INSTANCE_NUM; i++) {
        onenet = &gOnenetContext[i];
        retentCxt = &gRetentionContext[i];
        retentCxt->bUsed = onenet->bUsed;
        retentCxt->bConnected = onenet->bConnected;
        retentCxt->onenetId = onenet->onenetId;
        retentCxt->lifeTime = onenet->lifeTime;
        retentCxt->FotaState = onenet->FotaState;        
        retentCxt->bRestoreFlag = onenet->bRestoreFlag;
        memcpy(&retentCxt->objectInfo[0], &onenet->objectInfo[0], ONENET_MAX_OBJECT_COUNT * sizeof(onenetObjectInfo));
        if(onenet->cis_context != NULL){
            memcpy(&retentCxt->host[0], ((st_context_t*)onenet->cis_context)->hostRestore, ONENET_HOST_NAME_LEN );
            if(((st_context_t*)onenet->cis_context)->identify != NULL){
                memcpy(&retentCxt->identify[0], ((st_context_t*)onenet->cis_context)->identify, ONENET_PSKID_LEN );
                memcpy(&retentCxt->secret[0], ((st_context_t*)onenet->cis_context)->secret, ONENET_PSKID_LEN );
                retentCxt->isPsk = 0xA5;
                ECOMM_STRING(UNILOG_ONENET, onenetSaveFlash_4, P_INFO, "save identify=%s",retentCxt->identify);
            }
        }
        retentCxt->reqhandle = onenet->reqhandle;
    }
    memcpy((uint8_t *)&(gRetentionContext[0].gObservedBackup[0]), &(g_observed_backup[0]), MAX_OBSERVED_COUNT*sizeof(observed_backup_t));
    //save sim ota finish state
    gRetentionContext[0].otaFinishState = gOnenetSimOtaCtx.otaFinishState;
    ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_0, P_INFO, 1, "save to gRetention otaFinishState=%d",gRetentionContext[0].otaFinishState);

    ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_1, P_INFO, 2, "onenet->bUsed[%d],onenet->bConnected[%d]", onenet->bUsed, onenet->bConnected);

    for(int i = 0; i<MAX_OBSERVED_COUNT; i++){
        if(onenet->objectInfo[i].bUsed){
            ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_2, P_INFO, 2, "onenet->objectInfo[%d]: objid=%d", i, onenet->objectInfo[i].objectId);
        }
        if(g_observed_backup[i].is_used == 1){
            ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_3, P_INFO, 3, "g_observed_backup[%d]: msgid = %d,objid=%d", i, g_observed_backup[i].msgid, g_observed_backup[i].uri.objectId);
        }
    }

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_0_1, P_ERROR, 0, "can't open/create NVM: cisconfig.nvm, save NVM failed");
        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(onenetRetentionContext) * ONENET_INSTANCE_NUM;
    nvmHdr.version        = CIS_NVM_FILE_VERSION_2;
    nvmHdr.checkSum       = check_sum((uint8_t *)gRetentionContext, sizeof(onenetRetentionContext)*ONENET_INSTANCE_NUM);
    
    writeCount = OsaFwrite(&nvmHdr, sizeof(cisNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_0_2, P_ERROR, 0, "cisconfig.nvm, write the file header failed");

        OsaFclose(fp);
        return;
    }


    if(gRetentionContext[0].mMiplcfg.valid_magic != MIPLCONFIG_VALID_MAGIC)
    {
        OsaCheck(0, gRetentionContext[0].mMiplcfg.valid_magic, 0, 0);
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(gRetentionContext, sizeof(onenetRetentionContext), ONENET_INSTANCE_NUM, fp);
    if (writeCount != ONENET_INSTANCE_NUM)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetSaveFlash_0_3, P_ERROR, 0, "cisconfig.nvm, write the file body failed");
    }

    OsaFclose(fp);
    return;
}

INT8 onenetRestoreContext(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    INT32 i;
    cisNvmHeader    nvmHdr;   //4 bytes
    onenetContext *onenet;
    onenetRetentionContext* retentCxt;

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NVM_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetRestore_0, P_INFO, 0, "can't open NVM: cisconfig.nvm, use the defult value");
        return -1;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&nvmHdr, sizeof(cisNvmHeader), 1, fp);
    if(nvmHdr.version == CIS_NVM_FILE_VERSION) //nv version should upgrade
    {
        if (readCount != 1 ||
            nvmHdr.fileBodySize != ONENET_INSTANCE_NUM * sizeof(onenetRetentionContext_v0))
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_1_1, P_ERROR, 2,
                        "cisconfig.nvm, can't read header, or header not right, (%u/%u), use the defult value",nvmHdr.fileBodySize, ONENET_INSTANCE_NUM * sizeof(onenetRetentionContext_v0));
            OsaFclose(fp);

            return -2;
        }

        onenetRetentionContext_v0 *v0_context = pvPortMalloc(sizeof(onenetRetentionContext_v0));

        /*
         * read the file body retention context
        */
        readCount = OsaFread(v0_context, sizeof(onenetRetentionContext_v0), ONENET_INSTANCE_NUM, fp);
    
        if (readCount != ONENET_INSTANCE_NUM)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_1_2, P_ERROR, 0,"cisconfig.nvm, can't read body");
            OsaFclose(fp);
            free(v0_context);
            return -3;
        }
        OsaFclose(fp);
        
        gRetentionContext[0].bUsed = v0_context->bUsed;
        gRetentionContext[0].bConnected = v0_context->bConnected;
        gRetentionContext[0].bRestoreFlag = v0_context->bRestoreFlag;
        gRetentionContext[0].FotaState = v0_context->FotaState;
        gRetentionContext[0].lifeTime = v0_context->lifeTime;
        gRetentionContext[0].otaFinishState = v0_context->otaFinishState;
        gRetentionContext[0].onenetId = v0_context->onenetId;
        gRetentionContext[0].reserved0 = 0;
        gRetentionContext[0].reqhandle = v0_context->reqhandle;
        memcpy(gRetentionContext[0].host, v0_context->host, sizeof(gRetentionContext[0].host));
        gRetentionContext[0].reserved1 = 0;
        gRetentionContext[0].isPsk = v0_context->isPsk;
        gRetentionContext[0].observeObjNum = v0_context->observeObjNum;
        gRetentionContext[0].reserved2 = 0;
        memcpy(gRetentionContext[0].identify, v0_context->identify, sizeof(gRetentionContext[0].identify));
        memcpy(gRetentionContext[0].secret, v0_context->secret, sizeof(gRetentionContext[0].secret));
        memcpy(&gRetentionContext[0].mMiplcfg, &v0_context->mMiplcfg, sizeof(gRetentionContext[0].mMiplcfg));
        memcpy(&gRetentionContext[0].objectInfo, &v0_context->objectInfo, sizeof(onenetObjectInfo) * 2);
        memcpy(&gRetentionContext[0].gObservedBackup, &v0_context->gObservedBackup, sizeof(observed_backup_t) * 2);

        free(v0_context);

        ECOMM_TRACE(UNILOG_ONENET, onenetRestore_1, P_WARNING, 0,"cisconfig.nvm, upgrade from 0->1");

    }
    else if(nvmHdr.version == CIS_NVM_FILE_VERSION_1)//nv version should upgrade
    {
        if (readCount != 1 ||
            nvmHdr.fileBodySize != ONENET_INSTANCE_NUM * sizeof(onenetRetentionContext_v1))
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_2_1, P_ERROR, 2,
                        "cisconfig.nvm, can't read header, or header not right, (%u/%u), use the defult value",nvmHdr.fileBodySize, ONENET_INSTANCE_NUM * sizeof(onenetRetentionContext_v1));
            OsaFclose(fp);

            return -2;
        }

        onenetRetentionContext_v1 *v1_context = pvPortMalloc(sizeof(onenetRetentionContext_v1));

        /*
         * read the file body retention context
        */
        readCount = OsaFread(v1_context, sizeof(onenetRetentionContext_v1), ONENET_INSTANCE_NUM, fp);
    
        if (readCount != ONENET_INSTANCE_NUM)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_2_2, P_ERROR, 0,"cisconfig.nvm, can't read body");
            OsaFclose(fp);
            free(v1_context);
            return -4;
        }
        OsaFclose(fp);

        uint8_t cur_check_sum = 0xff & check_sum((uint8_t *)v1_context, nvmHdr.fileBodySize);
        if(nvmHdr.checkSum != cur_check_sum)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_2_3, P_ERROR, 2, "check sum failed, store value = 0x%x, cur value = 0x%x", nvmHdr.checkSum, cur_check_sum);
            free(v1_context);
            return -5;
        }
        
        gRetentionContext[0].bUsed = v1_context->bUsed;
        gRetentionContext[0].bConnected = v1_context->bConnected;
        gRetentionContext[0].bRestoreFlag = v1_context->bRestoreFlag;
        gRetentionContext[0].FotaState = v1_context->FotaState;
        gRetentionContext[0].lifeTime = v1_context->lifeTime;
        gRetentionContext[0].otaFinishState = v1_context->otaFinishState;
        gRetentionContext[0].onenetId = v1_context->onenetId;
        gRetentionContext[0].reserved0 = 0;
        gRetentionContext[0].reqhandle = v1_context->reqhandle;
        memcpy(gRetentionContext[0].host, v1_context->host, sizeof(gRetentionContext[0].host));
        gRetentionContext[0].reserved1 = 0;
        gRetentionContext[0].isPsk = v1_context->isPsk;
        gRetentionContext[0].observeObjNum = v1_context->observeObjNum;
        gRetentionContext[0].reserved2 = 0;
        memcpy(gRetentionContext[0].identify, v1_context->identify, sizeof(gRetentionContext[0].identify));
        memcpy(gRetentionContext[0].secret, v1_context->secret, sizeof(gRetentionContext[0].secret));
        memcpy(&gRetentionContext[0].mMiplcfg, &v1_context->mMiplcfg, sizeof(gRetentionContext[0].mMiplcfg));
        memcpy(&gRetentionContext[0].objectInfo, &v1_context->objectInfo, sizeof(onenetObjectInfo) * 2);
        memcpy(&gRetentionContext[0].gObservedBackup, &v1_context->gObservedBackup, sizeof(observed_backup_t) * 2);
        
        free(v1_context);
        ECOMM_TRACE(UNILOG_ONENET, onenetRestore_2, P_WARNING, 0,"cisconfig.nvm, upgrade from 1->2");
    }
    else if(nvmHdr.version == CIS_NVM_FILE_VERSION_2)
    {
        /*
         * read the file body retention context
        */
        readCount = OsaFread(gRetentionContext, sizeof(onenetRetentionContext), ONENET_INSTANCE_NUM, fp);
    
        if (readCount != ONENET_INSTANCE_NUM)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_3_1, P_ERROR, 0,"cisconfig.nvm, can't read body");
            OsaFclose(fp);
            return -4;
        }
        OsaFclose(fp);

        uint8_t cur_check_sum = 0xff & check_sum((uint8_t *)gRetentionContext, nvmHdr.fileBodySize);
        if(nvmHdr.checkSum != cur_check_sum)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetRestore_3_2, P_ERROR, 2, "check sum failed, store value = 0x%x, cur value = 0x%x", nvmHdr.checkSum, cur_check_sum);
            return -5;
        }
        
    }

    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        onenet = &gOnenetContext[i];
        retentCxt = &gRetentionContext[i];
        onenet->bUsed = retentCxt->bUsed;
        onenet->bConnected = retentCxt->bConnected;
        onenet->bRestoreFlag = retentCxt->bRestoreFlag;
        onenet->FotaState = retentCxt->FotaState;
        onenet->onenetId = retentCxt->onenetId;
        onenet->lifeTime = retentCxt->lifeTime;
        memcpy(&onenet->objectInfo[0], &retentCxt->objectInfo[0], ONENET_MAX_OBJECT_COUNT * sizeof(onenetObjectInfo));
        memcpy(&onenet->host[0], &retentCxt->host[0], ONENET_HOST_NAME_LEN);
        if(retentCxt->isPsk == 0xA5){
            onenet->isPsk = TRUE;
            memcpy(&onenet->secret[0], &retentCxt->secret[0], ONENET_PSK_LEN);
            memcpy(&onenet->identify[0], &retentCxt->identify[0], ONENET_PSKID_LEN);
            ECOMM_STRING(UNILOG_ONENET, onenetRestore_6, P_INFO, "restore identify=%s",onenet->identify);
        }

        onenet->configLen = createConfigBuf(i, onenet->configBuf);
        onenet->reqhandle = retentCxt->reqhandle;
    }
    // Restore for cis core using/Now only support one instance
    memcpy((uint8_t *)g_observed_backup, (uint8_t *)&(gRetentionContext[0].gObservedBackup[0]),  MAX_OBSERVED_COUNT*sizeof(observed_backup_t));
    ECOMM_TRACE(UNILOG_ONENET, onenetRestore_4, P_INFO, 3, "used =%d onenet->bConnected=%d, bRestoreFlag=%d",onenet->bUsed,onenet->bConnected, onenet->bRestoreFlag);

    //restore sim ota finish state flag
    gOnenetSimOtaCtx.otaFinishState = gRetentionContext[0].otaFinishState;
    ECOMM_TRACE(UNILOG_ONENET, onenetRestore_5, P_VALUE, 1, "restore from NVM, otaFinishState:%d",gOnenetSimOtaCtx.otaFinishState);

    return 0;
}

CmsRetId onenet_client_open(UINT32 reqHandle, UINT32 id, INT32 lifetime, INT32 timeout)
{
    ONENET_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_ONENET, onenet_client_open_1, P_INFO, 0, "TO SEND AT+MIPLOPEN");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_ONENET_OPEN;
    msg.reqhandle = reqHandle;
    msg.onenetId = id;
    msg.lifetime = lifetime;
    msg.timeout = timeout;
    
    xQueueSend(onenet_atcmd_handle, &msg, ONENET_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId onenet_client_close(UINT32 reqHandle, UINT32 id)
{
    ONENET_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_ONENET, onenet_client_close_1, P_INFO, 0, "TO SEND AT+MIPLCLOSE");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_ONENET_CLOSE;
    msg.reqhandle = reqHandle;
    msg.onenetId = id;
    
    xQueueSend(onenet_atcmd_handle, &msg, ONENET_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}
CmsRetId onenet_client_update(UINT32 reqHandle, UINT32 id, INT32 lifetime, UINT8 oflag, UINT8 raiflag)
{
    ONENET_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_ONENET, onenet_client_update_1, P_INFO, 0, "TO SEND AT+MIPLUPDATE");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_ONENET_UPDATE;
    msg.reqhandle = reqHandle;
    msg.onenetId = id;
    msg.lifetime = lifetime;
    msg.oflag = oflag;
    msg.raiflag = raiflag;
    
    xQueueSend(onenet_atcmd_handle, &msg, ONENET_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId onenet_client_notify(UINT32 reqHandle, UINT32 id, void* cmd)
{
    ONENET_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_ONENET, onenet_client_notify_1, P_INFO, 0, "TO SEND AT+MIPLNOTIFY");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_ONENET_NOTIFY;
    msg.reqhandle = reqHandle;
    msg.cmd = cmd;
    
    xQueueSend(onenet_atcmd_handle, &msg, ONENET_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

UINT16 check_network_ready(UINT8 waitTime)
{
    UINT16 result = CIS_ERRID_OK;
    NmAtiSyncRet netStatus;
    UINT32 start, end;
    
    start = cissys_gettime()/osKernelGetTickFreq();
    end=start;
    // Check network for waitTime*2 seconds
    UINT8 network=0;
    while(end-start < waitTime)
    {
        appGetNetInfoSync(0, &netStatus);
        if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
        {
            ECOMM_TRACE(UNILOG_ONENET, check_network_ready_1, P_INFO, 0, "network ready");
            network = 1;
            break;
        }
        cissys_sleepms(500);
        end = cissys_gettime()/osKernelGetTickFreq();
    }
    if(network == 0){
        ECOMM_TRACE(UNILOG_ONENET, check_network_ready_2, P_INFO, 0, "no network return 655");
        result = CIS_ERRID_NO_NETWORK;
    }
    return result;

}

static char* onenetMakeDeviceName()
{
    char* name = NULL;
    name = (char*)malloc(NBSYS_IMEI_MAXLENGTH + NBSYS_IMSI_MAXLENGTH + 2);
    memset(name, 0, NBSYS_IMEI_MAXLENGTH + NBSYS_IMSI_MAXLENGTH + 2);
    uint8_t imei = cissys_getIMEI(name, NBSYS_IMEI_MAXLENGTH);
    *((char*)(name + imei)) = ';';
    uint8_t imsi = cissys_getIMSI(name + imei + 1, NBSYS_IMSI_MAXLENGTH, true);
    if (imei <= 0 || imsi <= 0 || strlen(name) <= 0)
    {
        LOGE("ERROR:Get IMEI/IMSI ERROR.\n");
        ECOMM_TRACE(UNILOG_ONENET, onenetMakeDeviceName_0, P_INFO, 0, "ERROR:Get IMEI/IMSI ERROR.");
        return 0;
    }

    ECOMM_STRING(UNILOG_ONENET, onenetMakeDeviceName_1, P_INFO, "get enderpoint=%s",(uint8_t*)name);

    return name;
}


static bool onenetHasActiveInstance(void)
{
    onenetContext *onenet;
    UINT32 i;

    for (i = 0; i < ONENET_INSTANCE_NUM; i++)
    {
        onenet = &gOnenetContext[i];
        if (onenet->bUsed)
        {
            return true;
        }
    }
    return false;
}


static void onenetTASK(void *arg)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    onenetContext *onenet;
    UINT32 i;
    
    ECOMM_TRACE(UNILOG_ONENET, onenetTASK_1, P_INFO, 0, "onenetTASK enter");

    UINT16 cisError = check_network_ready(20);
    if (cisError == CIS_ERRID_OK){
        ECOMM_TRACE(UNILOG_ONENET, onenetTASK_1_1, P_INFO, 0, "network ready");
    }else{
        ECOMM_TRACE(UNILOG_ONENET, onenetTASK_1_2, P_INFO, 0, "network not ready");
    }

    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        onenet = &gOnenetContext[i];
        if (onenet->bUsed) {
            st_context_t* cisContext = (st_context_t*)(onenet->cis_context);
            if(cisContext->endpointName == NULL){
                cisContext->endpointName = onenetMakeDeviceName();
                ECOMM_STRING(UNILOG_ONENET, onenetTASK_4, P_VALUE, "endpoint = %s", (uint8_t *)cisContext->endpointName);
            }
        }
    }
    
    
    while (onenetHasActiveInstance()) {
        for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
            onenet = &gOnenetContext[i];
            UINT32 ret = cis_pump(onenet->cis_context);
            if(ret == PUMP_RET_FAULT)
            {
                onenet->bUsed = false;
                onenet->bConnected = false;
                onenet->bRestoreFlag = false;
                onenetDelAllObjects(onenet);
                onenetSaveFlash();
                goto exit;
            }
#if ONENET_AUTO_UPDATE_ENABLE
            if(cis_DeepSlpTimerHasExp() == true)            // send an auto update
            {
                ECOMM_TRACE(UNILOG_ONENET, onenetTASK_3, P_WARNING, 2, "Onenet task auto-update, Id = %d, LifeTime = %d", onenet->onenetId, onenet->lifeTime);
                onenetUpdate(onenet->onenetId, onenet->lifeTime, false, 0);
                cis_lifeTimeStart(onenet->lifeTime);
            }
#endif
            }
            if(onenetCanSleep() == true)
                osDelay(200);
            else
                osDelay(20);
    }

exit:
    gOnenetTaskRunStatus = TASK_STATUS_CLOSE;

    ECOMM_TRACE(UNILOG_ONENET, onenetTASK_2, P_INFO, 0, "onenetTASK exit");

    onenetSleepVote(SYSTEM_FREE);

    vTaskDelete(NULL);
}

INT32 onenet_create_maintask()
{
    INT32 ret = CIS_ERRID_OK;
    
    osStatus_t result = osMutexAcquire(taskMutex, osWaitForever);
    
    if(gOnenetTaskRunStatus != TASK_STATUS_OPEN)
    {
        gOnenetTaskRunStatus = TASK_STATUS_OPEN;
        osThreadAttr_t task_attr;
        memset(&task_attr, 0, sizeof(task_attr));
        task_attr.name = "oneTask";
        task_attr.stack_size = ONENET_MAIN_TASK_STACK_SIZE;
        task_attr.priority = osPriorityBelowNormal7;
    
        ECOMM_TRACE(UNILOG_ONENET, onenet_create_maintask_2, P_INFO, 0, "create onenetTASK");
        osThreadId_t task_handle = osThreadNew(onenetTASK, NULL,&task_attr);
        if(task_handle == NULL)
        {
            ret = CIS_ERRID_TASK_ERR;
        }
    }
    osMutexRelease(taskMutex);

    return ret;
}


static UINT16 onenet_init_client()
{
    uint16_t result=CIS_ERRID_TASK_ERR;
    onenetContext *onenet;
    onenetObjectInfo *objectInfo;
    UINT32 i, j;
    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_1, P_INFO, 0, "to init onenet client");
        onenet = &gOnenetContext[i];
        onenet->cis_context = NULL;
        ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_2, P_INFO, 1, "cis_init start bConnected=%d",onenet->bConnected);
        cis_ret_t cis_ret = cis_init(&onenet->cis_context, onenet->configBuf, onenet->configLen, NULL, defaultCisLocalPort);
        ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_3, P_INFO, 1, "cis_init end: %d",(UINT32)cis_ret);
        if(cis_ret != CIS_RET_OK){
            result = CIS_ERRID_SDK_ERROR;
            return result;
        }
        
        if(onenet->bConnected)
        {
            ((st_context_t*)onenet->cis_context)->stateStep = PUMP_STATE_CONNECTING;
            ((st_context_t*)onenet->cis_context)->ignoreRegistration = TRUE;
            
            memcpy(((st_context_t*)onenet->cis_context)->hostRestore, (CHAR*)onenet->host, ONENET_HOST_NAME_LEN);

            if(onenet->isPsk)
            {
                ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_3_1, P_INFO, 0, "restore cis context's identify");
                ((st_context_t*)onenet->cis_context)->identify = (char*)malloc(strlen((CHAR*)onenet->identify)+1);
                memset(((st_context_t*)onenet->cis_context)->identify, 0, strlen((CHAR*)onenet->identify)+1);
                memcpy(((st_context_t*)onenet->cis_context)->identify, (CHAR*)onenet->identify, ONENET_PSKID_LEN);
                
                ((st_context_t*)onenet->cis_context)->secret = (CHAR*)malloc(strlen((CHAR*)onenet->secret) + 1);
                memset(((st_context_t*)onenet->cis_context)->secret, 0, strlen((CHAR*)onenet->secret) + 1);
                memcpy(((st_context_t*)onenet->cis_context)->secret, (CHAR*)onenet->secret, ONENET_PSK_LEN);
            }
            if (onenet->FotaState) 
            {
                ((st_context_t*)onenet->cis_context)->fotaNeedUpdate = 1;
                ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_fota_1, P_INFO, 0, "cis_init fota ");
            }
            observe_read_retention_data((st_context_t*)onenet->cis_context);
        }
        else
        {
            ((st_context_t*)onenet->cis_context)->ignoreRegistration = FALSE;
            ((st_context_t*)onenet->cis_context)->stateStep = PUMP_STATE_INITIAL;
        }

        for (j = 0; j < ONENET_MAX_OBJECT_COUNT; j++) {
            objectInfo = &onenet->objectInfo[j];
            if (objectInfo->bUsed) {
                cis_inst_bitmap_t bitmap;
                cis_res_count_t resource;
                bitmap.instanceCount = objectInfo->instCount;
                bitmap.instanceBytes = (bitmap.instanceCount + 7) / 8;
                bitmap.instanceBitmap = objectInfo->instBitmap;
                resource.attrCount = objectInfo->attrCount;
                resource.actCount = objectInfo->actCount;
                ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_4, P_INFO, 2, "cis_addobject start: objid=%d, rescount=%d",
                               (INT32)objectInfo->objectId,(INT32)resource.attrCount);
                cis_ret = cis_addobject(onenet->cis_context, objectInfo->objectId, &bitmap, &resource);
                ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_5, P_INFO, 1, "cis_addobject end: %d", (INT32)cis_ret);
                if(cis_ret != CIS_RET_OK){
                    result = CIS_ERRID_SDK_ERROR;
                    return result;
                }
            }
        }
        if (onenet->bConnected) {
            ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_6, P_INFO, 2, "bConnected call onenetOpen:lifetime %d, timeout %d",onenet->lifeTime, onenet->timeout);
            INT32 cisError = onenetOpen(onenet->onenetId, onenet->lifeTime, onenet->timeout);
            if(cisError != CIS_ERRID_OK){
                result = CIS_ERRID_SDK_ERROR;
                return result;
            }
        }
    }

    ECOMM_TRACE(UNILOG_ONENET, onenet_init_client_7, P_SIG, 0, "needMaintask, start the maintask");
    result = onenet_create_maintask();

    return result;
}

static INT16 onenetCisReady(UINT32 onenetId)
{
    /*----------------------------------------------------------------*/
    onenetContext *onenet = onenetSearchInstance(onenetId);
    if ((onenet == NULL) ||(onenet->cis_context == NULL)){
        ECOMM_TRACE(UNILOG_ONENET, onenetCisReady_1, P_INFO, 0, "cis_context == NULL");
        return CIS_ERRID_SDK_ERROR;
    }
    
    st_context_t* ctx = (st_context_t*)onenet->cis_context;
    UINT8 waitTime = 0;
    while(ctx->stateStep != PUMP_STATE_READY && waitTime < 8){
        waitTime += 1;
        cissys_sleepms(500);
    }
    if(waitTime == 8){
        ECOMM_TRACE(UNILOG_ONENET, onenetCisReady_2, P_INFO, 0, "wait 4s statestep not ready");
        return CIS_ERRID_NO_NETWORK;
    }else{
        ECOMM_TRACE(UNILOG_ONENET, onenetCisReady_3, P_INFO, 0, "statestep ready");
        return CIS_ERRID_OK;
    }
}

static void onenet_atcmd_processing(void *argument)
{
    int keepTask = 1;
    INT16 ret = CIS_ERRID_OK;
    INT32 msg_type = 0xff;
    ONENET_ATCMD_Q_MSG msg;
    int primSize;
    UINT16 atHandle;
    UINT32 onenetId;
    BOOL hasTask = FALSE;
    BOOL hasClient = FALSE;
    onenetResuourcCmd* notifycmd = NULL;
    onenet_cnf_msg cnfMsg;
    primSize = sizeof(cnfMsg);
    memset(&cnfMsg, 0, primSize);

    while(1)
    {
        /* recv msg (block mode) */
        xQueueReceive(onenet_atcmd_handle, &msg, osWaitForever);
        msg_type = msg.cmd_type;
        onenetId = msg.onenetId;
        atHandle = msg.reqhandle;
  
        hasTask = FALSE;
        hasClient = FALSE;
        if(gOnenetTaskRunStatus != TASK_STATUS_OPEN){
            ECOMM_TRACE(UNILOG_ONENET, onenet_atcmd_processing_0, P_INFO, 0, "no main task, need to start main task first");
            ret = onenet_create_maintask();
            if(ret == CIS_ERRID_OK){
                hasTask = TRUE;
            }
        }else{
            hasTask = TRUE;
        }
        if(hasTask){
            if(msg_type == MSG_ONENET_NOTIFY || msg_type == MSG_ONENET_UPDATE || msg_type == MSG_ONENET_CLOSE){
                ret = onenetCisReady(onenetId);
                if(ret == CIS_ERRID_OK){
                    hasClient = TRUE;
                }
            }
        }
        
        switch(msg_type)
        {
            case MSG_ONENET_OPEN:
                ECOMM_TRACE(UNILOG_ONENET, onenet_atcmd_processing_1, P_INFO, 0, "TO HANDLE AT+MIPLOPEN");
                if(!hasTask){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_OPEN_CNF, primSize, &cnfMsg);
                    break;
                }
                ret = onenetOpen(onenetId, msg.lifetime, msg.timeout);
                if(ret != CIS_ERRID_OK){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_OPEN_CNF, primSize, &cnfMsg);
                }else{
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_ONENET, APPL_ONENET_OPEN_CNF, primSize, &cnfMsg);
                }
                break;
                
            case MSG_ONENET_NOTIFY:
                ECOMM_TRACE(UNILOG_ONENET, onenet_atcmd_processing_2, P_INFO, 0, "TO HANDLE AT+MIPLNOTIFY");
                notifycmd = (onenetResuourcCmd*)msg.cmd;
                if(!hasTask || !hasClient){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_NOTIFY_CNF, primSize, &cnfMsg);
                    if(notifycmd->value != NULL)
                        free(notifycmd->value);
                    free(notifycmd);
                    break;
                }
                ret = onenetNotify(onenetId, notifycmd);
                if(notifycmd->value != NULL)
                    free(notifycmd->value);
                free(notifycmd);
                if(ret != CIS_ERRID_OK){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_NOTIFY_CNF, primSize, &cnfMsg);
                }else{
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_ONENET, APPL_ONENET_NOTIFY_CNF, primSize, &cnfMsg);
                }
                break;
                
            case MSG_ONENET_CLOSE:
                ECOMM_TRACE(UNILOG_ONENET, onenet_atcmd_processing_3, P_INFO, 0, "TO HANDLE AT+MIPLCLOSE");
                if(!hasTask){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_CLOSE_CNF, primSize, &cnfMsg);
                    break;
                }
                ret = onenetDeinit(onenetId);
                if(ret != CIS_ERRID_OK){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_CLOSE_CNF, primSize, &cnfMsg);
                }
                break;
            
            case MSG_ONENET_UPDATE:
                /* send update packet */
                ECOMM_TRACE(UNILOG_ONENET, onenet_atcmd_processing_5, P_INFO, 0, "TO HANDLE AT+MIPLUPDATE");
                if(!hasTask || !hasClient){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_UPDATE_CNF, primSize, &cnfMsg);
                    break;
                }
                ret = onenetUpdate(onenetId, msg.lifetime, msg.oflag, msg.raiflag);
                if(ret != CIS_ERRID_OK){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_ONENET, APPL_ONENET_UPDATE_CNF, primSize, &cnfMsg);
                }else{
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_ONENET, APPL_ONENET_UPDATE_CNF, primSize, &cnfMsg);
                }
                break;
         }
         if(keepTask == 0)
         {
             break;
         }
     }

    ECOMM_TRACE(UNILOG_ONENET, onenet_atcmd_processing_6, P_INFO, 0, "MIPLDELETE delete the atcmd process");
    onenet_atcmd_task_flag = ONENET_TASK_STAT_NONE;
    onenet_atcmd_task_handle = NULL;
    osThreadExit();
}

static INT32 onenet_handleAT_task_Init(void)
{
    INT32 ret = CIS_ERRID_OK;

    if(onenet_atcmd_handle == NULL)
    {
        onenet_atcmd_handle = xQueueCreate(24, sizeof(ONENET_ATCMD_Q_MSG));
    }
    else
    {
        xQueueReset(onenet_atcmd_handle);
    }

    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "oneAThdl";
    task_attr.stack_size = ONENET_ATHDL_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;

    onenet_atcmd_task_handle = osThreadNew(onenet_atcmd_processing, NULL,&task_attr);
    if(onenet_atcmd_task_handle == NULL)
    {
        ret = CIS_ERRID_TASK_ERR;
    }

    return ret;
}

INT32 onenet_athandle_create(void)
{
    ECOMM_TRACE(UNILOG_ONENET, onenet_athandle_create_1, P_INFO, 0, "to start onenet handleAT task");

    if(onenet_atcmd_task_flag == ONENET_TASK_STAT_NONE)
    {
        onenet_atcmd_task_flag = ONENET_TASK_STAT_CREATE;
        if(onenet_handleAT_task_Init() != CIS_ERRID_OK)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenet_athandle_create_0, P_INFO, 0, "onenet handleAT task create failed");
            return CIS_ERRID_TASK_ERR;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ONENET, onenet_athandle_create_2, P_INFO, 0, "onenet handleAT task already start");
    }

    return CIS_ERRID_OK;
}


void onenetEngineInit()
{
    INT8 ret = 0, i, needClearFlash = 0, needRestore = 0, needClearCfg = 0;
    INT8 FotaState = 0;
    onenetContext *pContext;
    slpManSlpState_t slpState;
    NmAtiSyncRet netStatus;
    
    onenetInitSleepHandler();
    registerPSEventCallback(NB_GROUP_ALL_MASK, onenetPsUrcCallback);

    appGetNetInfoSync(0, &netStatus);
    cisnet_setNBStatus(netStatus.body.netInfoRet.netifInfo.netStatus);

    ret = onenetRestoreContext();
    if (ret < 0)
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetEngineInit_0_1, P_INFO, 0, "onenetRestoreContext fail,use default params");
        for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
            pContext = &gOnenetContext[i];
            memset(&gOnenetContext[i], 0, sizeof(onenetContext));
        }
    }
    else
    {
        slpState = slpManGetLastSlpState();
        if (slpState == SLP_HIB_STATE || slpState == SLP_SLP2_STATE)
        {
            ECOMM_TRACE(UNILOG_ONENET, onenetEngineInit_1, P_INFO, 0, "wakeup from hib or slp2");
        }
        else
        {
        
            for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
                pContext = &gOnenetContext[i];
                if (pContext->FotaState == 1)
                {
                    FotaState = 1;
                    needRestore = 1;
                    //when fota state, onenet must restore, need not clear 
                    break;
                }                    
            }
            ECOMM_TRACE(UNILOG_ONENET, onenetEngineInit_fota_1, P_INFO, 1, "onenet_init_client FotaState=%d", FotaState);

            
            if (FotaState == 0)
            {
                ECOMM_TRACE(UNILOG_ONENET, onenetEngineInit_2, P_INFO, 0, "reset boot");
                for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
                    pContext = &gOnenetContext[i];
                    
                    if (pContext->bRestoreFlag == 1)
                    {
                        pContext->bRestoreFlag = 0;
                        needClearFlash = 1;
                        memset(&gOnenetContext[i], 0, sizeof(onenetContext));
                    }
                    pContext->bUsed = FALSE;
                }
                for (i = 0; i < ONENET_INSTANCE_NUM; i++)
                {
                    if(gRetentionContext[i].mMiplcfg.valid_magic == MIPLCONFIG_VALID_MAGIC)
                    {
                        needClearCfg = 1;           
                    }
                }
                if((needClearFlash == 1) || (needClearCfg == 1))    // clear cfg
                {
                    needClearFlash = 0;
                    onenetClearFlash(true);
                }
            }
        }
    }
    for (i = 0; i < ONENET_INSTANCE_NUM; i++) {
        pContext = &gOnenetContext[i];
        needRestore += pContext->bRestoreFlag;

        if(gRetentionContext[i].mMiplcfg.valid_magic != MIPLCONFIG_VALID_MAGIC)
        {
            onenetSetDefaultCfg(i);
        }
    }
    
    if (needRestore > 0)
    {
        if(taskMutex == NULL){
            taskMutex = osMutexNew(NULL);
        }
        ECOMM_TRACE(UNILOG_ONENET, onenetEngineInit_3, P_SIG, 0, "create has call,need restore. call onenet_athandle_create");
        INT32 result = onenet_athandle_create();
        result = onenet_init_client();
        ECOMM_TRACE(UNILOG_ONENET, onenetEngineInit_4, P_INFO, 1, "onenet_init_client ret=%d", result);
    }
}

static cis_coapret_t onenetOTAReadCallBack(void* context, cis_uri_t* uri, cis_mid_t mid)
{
    ECOMM_TRACE(UNILOG_ONENET, onenetOTAReadCallBack_0, P_INFO, 0, "No READ command process for OTA");

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetOTADiscoverCallBack(void* context, cis_uri_t* uri, cis_mid_t mid)
{

    ECOMM_TRACE(UNILOG_ONENET, onenetOTADiscoverCallBack_0, P_INFO, 0, "No DISCOVER command process for OTA");

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetOTAWriteCallBack(void* context, cis_uri_t* uri, const cis_data_t* value, cis_attrcount_t attrcount, cis_mid_t mid)
{

    ECOMM_TRACE(UNILOG_ONENET, onenetOTAWriteCallBack_0, P_INFO, 0, "No WRITE command process for OTA");

    return CIS_CALLBACK_CONFORM;
}


static cis_coapret_t onenetOTAExecCallBack(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length, cis_mid_t mid)
{
    ECOMM_TRACE(UNILOG_ONENET, onenetOTAExecCallBack_0, P_INFO, 0, "No EXEC command process for OTA");

    return CIS_CALLBACK_CONFORM;
}


static cis_coapret_t onenetOTAObserveCallBack(void* context, cis_uri_t* uri, bool flag, cis_mid_t mid)
{
    ECOMM_TRACE(UNILOG_ONENET, onenetOTAObserveCallBack_0, P_INFO, 0, "No OBSERVE command process for OTA");

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onenetOTAParamsCallBack(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    ECOMM_TRACE(UNILOG_ONENET, onenetOTAParamsCallBack_0, P_INFO, 0, "No PARAMS command process for OTA");

    return CIS_CALLBACK_CONFORM;
}

#define SIM_OTA_START_TIME_OUT_MSEC  (180*1000)

static void onenetOTAEventCallBack(void* context, cis_evt_t eid, void* param)
{
    st_context_t* ctx = (st_context_t*)context;

    cissys_assert(context != NULL);
    //LOGD("cis_on_event(%d):%s\n", eid, STR_EVENT_CODE(eid));
    ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_0, P_INFO, 1, "cis_on_event Id:%d", eid);
    ECOMM_STRING(UNILOG_ONENET, onenetOTAEventCallBack_1, P_VALUE, "cis_on_event: %s", (uint8_t *)STR_EVENT_CODE(eid));

    switch (eid)
    {
        case CIS_EVENT_UPDATE_NEED:
            LOGD("cis_on_event need to update,reserve time:%ds\n", (int32_t)param);
            ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_10, P_WARNING, 0, "need update register");
            cis_update_reg(ctx, ctx->ota_timeout_duration, false, PS_SOCK_RAI_NO_INFO);
            break;

        case CIS_EVENT_UNREG_DONE:
            if (gOnenetSimOtaCtx.otaFinishState == OTA_HISTORY_STATE_FINISHED)
            {
                cis_deinit(&g_cmiot_ota_context);
                ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_11, P_WARNING, 0, "OTA finshed and dereg done, so deinit cis_context");
            }
            break;

        case CIS_EVENT_CMIOT_OTA_START:
            LOGD("cis_on_event CMIOT OTA start:%d\n", (int32_t)param);
            ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_2, P_WARNING, 1, "onenet event: SIM OTA start, %d", *(uint32_t *)param);
            if (ctx->cmiotOtaState == CMIOT_OTA_STATE_IDIL)
            {

                ctx->cmiotOtaState = CMIOT_OTA_STATE_START;
                ctx->ota_timeout_base = cissys_gettime();
                ctx->ota_timeout_duration = SIM_OTA_START_TIME_OUT_MSEC;
                gOnenetSimOtaCtx.otaAtReturnCode = OTA_FINISH_AT_RETURN_TIMEOUT_SECOND;
            }
            break;

        case CIS_EVENT_CMIOT_OTA_SUCCESS:
            if (ctx->cmiotOtaState == CMIOT_OTA_STATE_START)
            {
                LOGD("cis_on_event CMIOT OTA success %d\n", (int32_t)param);
                ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_3, P_WARNING, 1, "onenet event: SIM OTA success, %d", *(uint32_t *)param);
                ctx->cmiotOtaState = CMIOT_OTA_STATE_IDIL;
                cissys_recover_psm();
            }
            break;

        case CIS_EVENT_CMIOT_OTA_FAIL:
            if (ctx->cmiotOtaState == CMIOT_OTA_STATE_START)
            {
                ctx->cmiotOtaState = CMIOT_OTA_STATE_IDIL;
                LOGD("cis_on_event CMIOT OTA fail %d\n", (int32_t)param);
                ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_4, P_WARNING, 1, "onenet event: SIM OTA fail, %d", *(uint32_t *)param);
                cissys_recover_psm();
            }
            break;

        case CIS_EVENT_CMIOT_OTA_FINISH:
            if (ctx->cmiotOtaState == CMIOT_OTA_STATE_START || ctx->cmiotOtaState == CMIOT_OTA_STATE_IDIL)
            {
                ctx->cmiotOtaState = CMIOT_OTA_STATE_FINISH;
                LOGD("cis_on_event CMIOT OTA finish %d\n", *((int8_t *)param));
                ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_5, P_WARNING, 1, "onenet event: SIM OTA finish, %d", *(uint8_t *)param);

                cissys_recover_psm();
                switch (*((int8_t *)param))
                {
                case OTA_FINISH_COMMAND_SUCCESS_CODE:
                    gOnenetSimOtaCtx.otaFinishState = OTA_HISTORY_STATE_FINISHED; //indicate ota procedure has finished
                    gOnenetSimOtaCtx.otaAtReturnCode = OTA_FINISH_AT_RETURN_SUCCESS;
                    LOGD("OTA finish Success!\n");
                    ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_6, P_WARNING, 0, "SIM OTA finish Success");
                    break;
                case OTA_FINISH_COMMAND_UNREGISTER_CODE:
                    gOnenetSimOtaCtx.otaAtReturnCode = OTA_FINISH_AT_RETURN_NO_REQUEST;
                    LOGD("OTA finish fail: no OTA register on platform!\n");//indicates there is no ota request on the CMIOT platform
                    ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_7, P_WARNING, 0, "SIM OTA finish fail: no OTA register on platform!");
                    break;
                case OTA_FINISH_COMMAND_FAIL_CODE:
                    gOnenetSimOtaCtx.otaAtReturnCode = OTA_FINISH_AT_RETURN_FAIL;
                    LOGD("OTA finish fail: target NUM error!\n");//indicates the IMSI is not changed or the new IMSI is illegal
                    ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_8, P_WARNING, 0, "SIM OTA finish fail: target NUM error!");
                    break;
                default://unkown error from the platform
                    LOGD("OTA finish fail: unknow error!\n");
                    ECOMM_TRACE(UNILOG_ONENET, onenetOTAEventCallBack_9, P_WARNING, 0, "SIM OTA finish fail: unknow error!");
                }
            }
            break;

        default:
            break;
    }

}

void onenetSimOtaTaskEntry(void *arg)
{
    //UINT32 pumpRet;
    cis_ret_t cis_ret;
    UINT32 nowtime;
    cis_callback_t callback;
    cis_time_t g_lifetime = 100;
    char response_data[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    callback.onRead = onenetOTAReadCallBack;
    callback.onWrite = onenetOTAWriteCallBack;
    callback.onExec = onenetOTAExecCallBack;
    callback.onObserve = onenetOTAObserveCallBack;
    callback.onSetParams = onenetOTAParamsCallBack;
    callback.onEvent = onenetOTAEventCallBack;
    callback.onDiscover = onenetOTADiscoverCallBack;

    //gOnenetSimOtaCtx.otaAtReturnCode = 99;//the return AT result init value
    gOnenetSimOtaCtx.otaAtReturnCode = OTA_FINISH_AT_RETURN_TIMEOUT_FIRST;

    //disable hib/sleep2 mode
    slpManPlatVoteDisableSleep(onenetSlpHandler, SLP_SLP2_STATE);

    //initialize
    gOnenetSimOtaCtx.configLen = (UINT16)onenetHexToBin(gOnenetSimOtaCtx.configBuf, ONENET_CONFIG_BUFFER_BS, 69);
    cis_ret = cis_init_ota(&g_cmiot_ota_context, gOnenetSimOtaCtx.configBuf, gOnenetSimOtaCtx.configLen, gOnenetSimOtaCtx.timeoutValue, defaultCisLocalPort);
    if (cis_ret == CIS_RET_OK)
    {
        cis_register(g_cmiot_ota_context, g_lifetime, &callback);
        while (1)
        {

            /*pump function*/
            /*pumpRet = */cis_pump(g_cmiot_ota_context);
            /*data observe data report*/
            nowtime = cissys_gettime();

            //if (gOnenetSimOtaCtx.otaFinishState == OTA_HISTORY_STATE_FINISHED)
            if (g_cmiot_ota_context == PNULL)
            {
                ECOMM_TRACE(UNILOG_ONENET, onenetSimOtaTaskEntry_0, P_WARNING, 0, "onenet simota cis_context is NULL, onenetSimOtaTask will exit");
                break;
            }

            /*ota timeout detection*/
            if (nowtime - ((st_context_t *)g_cmiot_ota_context)->ota_timeout_base > ((st_context_t *)g_cmiot_ota_context)->ota_timeout_duration)
            {
                //OTA timeout : Terminal has not received otastart command in specific duration
                cis_unregister(g_cmiot_ota_context);
                LOGD("OTA Timeout detected. Unresgister from OneNET \r\n");
                ECOMM_TRACE(UNILOG_ONENET, onenetSimOtaTaskEntry_1, P_ERROR, 0, "OTA Timeout detected. Unresgister from OneNET");
                cis_deinit(&g_cmiot_ota_context);
                cissys_sleepms(2000);
                break;
            }

        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ONENET, onenetSimOtaTaskEntry_2, P_ERROR, 1, "cis_init_ota ERROR=%d", cis_ret);
    }

    //return at result code
    sprintf(response_data, "+OTAFINISH: %d", (INT32)gOnenetSimOtaCtx.otaAtReturnCode);
    onenetSENDIND(strlen(response_data),  response_data, gOnenetSimOtaCtx.reqhandle);

    //enable hib/sleep2 mode
    slpManPlatVoteEnableSleep(onenetSlpHandler, SLP_SLP2_STATE);

    //save otaFinishState
    onenetSaveFlash();
    ECOMM_TRACE(UNILOG_ONENET, onenetSimOtaTaskEntry_3, P_WARNING, 0, "onenetSimOta Task delete.");

    vTaskDelete(NULL);
}

