/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    cisAsynEntry.c
 * Description:  EC616 CMCC cis dm entry source file
 * History:      Rev1.0   2019-3-6
 *
 ****************************************************************************/
#include <cis_def.h>
#if CIS_ENABLE_DM
#include <stdio.h>
#include <stdlib.h>
#include <cis_if_sys.h>
#include <cis_log.h>
#include <cis_list.h>
#include <cis_api.h>
#include "cmcc_dm.h"
#include <string.h>
#include "osasys.h"
#include "task.h"
#include "debug_log.h"
#include "dm_utils/dm_endpoint.h"
#include "mw_config.h"
#include "netmgr.h"
#include "ps_event_callback.h"
#include "ps_lib_api.h"
#include "cmips.h"
#include "cis_def.h"

#define DMCONN_TASK_STACK_SIZE 1024

#define DM_MACHINESTATE(S)                                \
((S) == DM_INIT_STATE ? "DM_INIT_STATE" :      \
((S) == DM_IPREADY_STATE ? "DM_IPREADY_STATE" :      \
((S) == DM_HEART_BEAT_ARRIVED ? "DM_HEART_BEAT_ARRIVED" :        \
((S) == DM_HEART_BEAT_UPDATE ? "DM_HEART_BEAT_UPDATE" :      \
((S) == DM_IDLE_STATE ? "DM_IDLE_STATE" :  \
"Unknown")))))

static const uint8_t dm_config_hex[] = {
0x13,0x00,0x5f,0xf1,0x00,0x03,0xf2,0x00,0x48,0x05,0x00,0x11,0x00,0x00,0x05,0x43,
0x4d,0x49,0x4f,0x54,0x00,0x04,0x73,0x64,0x6b,0x55,0x00,0x04,0x73,0x64,0x6b,0x50,
0x00,0x10,0x31,0x31,0x37,0x2e,0x31,0x36,0x31,0x2e,0x32,0x2e,0x37,0x3a,0x35,0x36,
0x38,0x33,0x00,0x1a,0x41,0x75,0x74,0x68,0x43,0x6f,0x64,0x65,0x3a,0x3b,0x55,0x73,
0x65,0x72,0x64,0x61,0x74,0x61,0x3a,0x61,0x62,0x63,0x31,0x32,0x33,0x3b,0xf3,0x00,
0x11,0xe4,0x00,0xc8,0x00,0x09,0x73,0x79,0x73,0x55,0x44,0x3a,0x31,0x32,0x33
};

static const uint8_t dm_m_config_hex[] = {
0x13,0x00,0x60,0xf1,0x00,0x03,0xf2,0x00,0x49,0x05,0x00,0x11,0x00,0x00,0x05,0x43,
0x4d,0x49,0x4f,0x54,0x00,0x04,0x73,0x64,0x6b,0x55,0x00,0x04,0x73,0x64,0x6b,0x50,
0x00,0x10,0x31,0x31,0x37,0x2e,0x31,0x36,0x31,0x2e,0x32,0x2e,0x34,0x31,0x3a,0x35,0x36,
0x38,0x33,0x00,0x1a,0x41,0x75,0x74,0x68,0x43,0x6f,0x64,0x65,0x3a,0x3b,0x55,0x73,
0x65,0x72,0x64,0x61,0x74,0x61,0x3a,0x61,0x62,0x63,0x31,0x32,0x33,0x3b,0xf3,0x00,
0x11,0xe4,0x00,0xc8,0x00,0x09,0x73,0x79,0x73,0x55,0x44,0x3a,0x31,0x32,0x33
};

#ifdef FEATURE_REF_AT_ENABLE
uint8_t *cmcc_config_hex = NULL;

static CHAR cmccUserdata[16] = "AuthCode:;PSK:;";
#endif

extern UINT8 cmccDmSlpHandler;
dmStateMachine_t stateMachine = DM_INIT_STATE;
BOOL needRestoreDMTask = FALSE;
static StaticTask_t dmconnTask;
static UINT8 dmconnTaskStack[DMCONN_TASK_STACK_SIZE];

MidWareDmConfig *ecDMconfig = NULL;

static char constDeviceInfo[20] = " ";
static char constAppInfo[20] = " ";
static char constMAC[20] = " ";
static char constROM[20] = "4M";
static char constRAM[20] = "256K";
static char constCPU[20] = "ARM Cortex-M3";
static char constSysVer[20] = "FreeRTOS V9.0.0";
static char constSoftVer[20] = "v1.1.1";
static char constSoftNam[20] = "EC616";
static char constVolte[20] = "0";
static char constNetType[20] = "NB-IoT";
static char constAccount[20] = "Account";
static char constPhoneNumber[20] = " ";
static char constvl[20] = " ";

//static void     prv_dm_observeNotify(void* context, cis_uri_t* uri, cis_mid_t mid);

static void     prv_dm_readResponse(void* context, cis_uri_t* uri, cis_mid_t mid);
static void     prv_dm_discoverResponse(void* context, cis_uri_t* uri, cis_mid_t mid);
static void     prv_dm_writeResponse(void* context, cis_uri_t* uri, const cis_data_t* value, cis_attrcount_t count, cis_mid_t mid);
static void     prv_dm_execResponse(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length, cis_mid_t mid);
static void     prv_dm_paramsResponse(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid);

static cis_coapret_t dm_onRead(void* context, cis_uri_t* uri, cis_mid_t mid);
static cis_coapret_t dm_onWrite(void* context, cis_uri_t* uri, const cis_data_t* value, cis_attrcount_t attrcount, cis_mid_t mid);
static cis_coapret_t dm_onExec(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length, cis_mid_t mid);
static cis_coapret_t dm_onObserve(void* context, cis_uri_t* uri, bool flag, cis_mid_t mid);
static cis_coapret_t dm_onParams(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid);
static cis_coapret_t dm_onDiscover(void* context, cis_uri_t* uri, cis_mid_t mid);
static void        dm_onEvent(void* context, cis_evt_t eid, void* param);

static struct dm_callback_info* g_dmcallbackList = NULL;
static struct dm_observe_info*  g_dmobserveList = NULL;
static dm_device_object          g_objectList[DM_SAMPLE_OBJECT_MAX];
static dm_instance             g_instList_a[DM_SAMPLE_A_INSTANCE_COUNT];

static Options    dm_config = { "","", "","","",4,"" };
static void*      g_dmcontext = NULL;
static bool       g_dmshutdown = false;
static bool       g_doUnregisterdm = false;
static bool       g_doRegisterdm = false;
static TaskHandle_t resp_taskhandle;
static CHAR *defaultDmLocalPort = "49260";

osThreadId_t cmccAutoRegisterTaskHandle = NULL;
static cmccDmRetentionContext g_retentContext;
static bool g_needUpdate = false;
static INT32 g_lifeTime = 3600;

void slpManInternalDeepSlpTimerRegisterExpCb(slpManUsrDeepSlpTimerCb_Func cb, slpManTimerID_e id);


#ifdef FEATURE_REF_AT_ENABLE
static BOOL cmccPrepareBuffer(UINT8* outBuf, UINT16* cursor, UINT8* inbuf, UINT8 inbuflen)
{
    UINT8 copyLen;
    UINT16 outbufCursor = *cursor;

    if(inbuflen == 0){
        return FALSE;
    }

    do {
        if((CMCCDM_CONFIG_MAX_BUFFER_LEN - outbufCursor) >= inbuflen) {
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

static void cmccPutU16(UINT8* P, UINT16 v)
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

static INT16 cmccCreateConfigBuf(UINT8* buf, CHAR* ip, CHAR* port)
{
    CHAR tempBuf[32];
    UINT16 bufCursor = 0;
    UINT16 netCfgLen = 0;
    UINT16 ipLen = 0;
    UINT16 portLen = 0;
    UINT16 userdataLen = 0;
    ipLen = strlen(ip);
    portLen = strlen(port);

    memset(buf, 0, CMCCDM_CONFIG_MAX_BUFFER_LEN);

    memset(tempBuf, 0, 32);
    tempBuf[0] = 0x13;
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 3);
    tempBuf[0] = 0xf1;
    tempBuf[1] = 0x00;
    tempBuf[2] = 0x03;
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 3);
    tempBuf[0] = 0xf2;
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 3);//let net cfg length empty first

    memset(tempBuf, 0, 32);
    cmccPutU16((UINT8*)tempBuf,CMCCDM_MTU_SIZE);
    tempBuf[2] = 0x11;
    tempBuf[3] = 0x00;

    cmccPutU16((UINT8*)&tempBuf[4],strlen("CMIOT"));//apn length
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 6);

    netCfgLen += 9;

    memset(tempBuf, 0, 32);
    snprintf(tempBuf, sizeof(tempBuf), "CMIOT");//apn is CMIOT
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, strlen(tempBuf));

    netCfgLen += strlen(tempBuf);

    memset(tempBuf, 0, 32);
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 4);//username and password is 0

    netCfgLen += 4;

    cmccPutU16((UINT8*)tempBuf, ipLen+portLen+1);
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 2);//host length

    netCfgLen += 2;

    memset(tempBuf, 0, 32);
    memcpy(tempBuf,ip,ipLen);
    tempBuf[ipLen] = ':';
    memcpy(&tempBuf[ipLen+1], port, portLen);
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, ipLen+portLen+1);//host

    netCfgLen = netCfgLen+ipLen+portLen+1;

    userdataLen = strlen(cmccUserdata);
    memset(tempBuf, 0, 32);
    cmccPutU16((UINT8*)tempBuf,userdataLen);//userdata length
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 2);

    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)cmccUserdata, strlen(cmccUserdata));//userdata now default:"AuthCode:;PSK:;" expand later

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
    cmccPrepareBuffer((UINT8*)buf, &bufCursor, (UINT8*)tempBuf, 8);


    cmccPutU16(buf+7, netCfgLen);
    cmccPutU16(buf+1, bufCursor);

    return bufCursor;
}

#endif

static void cmccDmSaveFile(INT8 regstat)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cmccDmNvmHeader nvmHdr;   //4 bytes
    g_retentContext.bConnected = regstat;
    g_retentContext.pumpState = PUMP_STATE_READY;
    strncpy(g_retentContext.location, ((st_context_t*)g_dmcontext)->location, DM_LOCATION_LEN);

    ECOMM_TRACE(UNILOG_DM, cmccDmSaveFile_1, P_INFO, 0, "save the connected status to flash");
    ECOMM_STRING(UNILOG_DM,cmccDmSaveFile_1_1, P_INFO, "save location=%s",(uint8_t*)g_retentContext.location);

    /*
     * open the NVM file
    */
    fp = OsaFopen(CMCCDM_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmSaveFile_2, P_ERROR, 0,
                    "can't open/create NVM: 'lwm2mconfig.nvm', save NVM failed");

        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(g_retentContext);
    nvmHdr.version        = 0;
    nvmHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvmHdr, sizeof(cmccDmNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmSaveFile_3, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&g_retentContext, sizeof(g_retentContext), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmSaveFile_4, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file body failed");
    }

    OsaFclose(fp);
    return;
}

static INT8 cmccDmReadFile(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    cmccDmNvmHeader    nvmHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(CMCCDM_NVM_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmReadFile_1, P_ERROR, 0,
                    "can't open NVM: 'cmccdmconfig.nvm', use the defult value");
        return -1;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&nvmHdr, sizeof(cmccDmNvmHeader), 1, fp);
    if (readCount != 1 ||
        nvmHdr.fileBodySize != sizeof(cmccDmRetentionContext))
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmReadFile_2, P_ERROR, 2,
                    "'cmccdmconfig.nvm', can't read header, or header not right, (%u/%u), use the defult value",
                    nvmHdr.fileBodySize, sizeof(cmccDmRetentionContext));

        OsaFclose(fp);

        return -2;
    }

    /*
     * read the file body retention context
    */
    readCount = OsaFread(&g_retentContext, sizeof(cmccDmRetentionContext), 1, fp);

    if (readCount != 1)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmReadFile_3, P_ERROR, 0,"'cmccdmconfig.nvm', can't read body");
        OsaFclose(fp);
        return -3;
    }
    OsaFclose(fp);

    ECOMM_TRACE(UNILOG_DM, cmccDmReadFile_4, P_INFO, 2, "isConnected=%d, pumpState=%d",g_retentContext.bConnected, g_retentContext.pumpState);
    ECOMM_STRING(UNILOG_DM, cmccDmReadFile_5, P_INFO, "read location=%s",(uint8_t*)g_retentContext.location);
    return 0;
}

static void cmccDmClearFile()
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cmccDmNvmHeader nvmHdr;   //4 bytes
    g_retentContext.bConnected = 0;
    g_retentContext.pumpState = PUMP_STATE_INITIAL;
    memset(g_retentContext.location, 0, DM_LOCATION_LEN);

    ECOMM_TRACE(UNILOG_DM, cmccDmClearFile_1, P_INFO, 0, "clear the connected status");

    /*
     * open the NVM file
    */
    fp = OsaFopen(CMCCDM_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmClearFile_2, P_ERROR, 0,
                    "can't open/create NVM: 'lwm2mconfig.nvm', save NVM failed");

        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(g_retentContext);
    nvmHdr.version        = 0;
    nvmHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvmHdr, sizeof(cmccDmNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmClearFile_3, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&g_retentContext, sizeof(g_retentContext), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmClearFile_4, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file body failed");
    }

    OsaFclose(fp);
    return;
}

void prvMakeUserdata()
{
    int i = 0;
    cis_instcount_t instIndex;
    cis_instcount_t instCount;
    for (i= 0;i < DM_SAMPLE_OBJECT_MAX; i++)
    {
        dm_device_object* obj = &g_objectList[i];
        switch(i){
            case 0:
            {
                obj->oid = DM_SAMPLE_OID_A;
                obj->instBitmap = DM_SAMPLE_A_INSTANCE_BITMAP;
                instCount = DM_SAMPLE_A_INSTANCE_COUNT;
                for (instIndex = 0;instIndex < instCount;instIndex++)
                {
                    if(obj->instBitmap[instIndex] != '1'){
                        g_instList_a[instIndex].instId = instIndex;
                        g_instList_a[instIndex].enabled = false;
                    }else
                    {
                        g_instList_a[instIndex].instId = instIndex;
                        g_instList_a[instIndex].enabled = true;

                        g_instList_a[instIndex].instance.boolVal = true;
                        g_instList_a[instIndex].instance.intVal = cissys_rand() % 100;
                        strcpy(g_instList_a[instIndex].instance.strVal,"temp test");
                    }
                }
                obj->attrCount = sizeof(const_AttrIds_a) / sizeof(cis_rid_t);
                obj->attrListPtr = const_AttrIds_a;

                obj->actCount = sizeof(const_ActIds_a)/sizeof(cis_rid_t);
                obj->actListPtr = const_ActIds_a;
            }
            break;

        }
    }
}

void dmTaskThread(void* arg)
{
    while (!g_dmshutdown)
    {
        struct dm_callback_info* node;
        if (g_dmcallbackList == NULL) {
            cissys_sleepms(1000);
            continue;
        }
        node = g_dmcallbackList;
        ECOMM_TRACE(UNILOG_DM, dmTaskThread_1, P_INFO, 1, "g_dmcallbackList has sth. get a node and flag is %d", node->flag);
        g_dmcallbackList = g_dmcallbackList->next;

        switch (node->flag)
        {
        case 0:
            break;
        case DM_SAMPLE_CALLBACK_READ:
        {
            ECOMM_TRACE(UNILOG_DM, dmTaskThread_2, P_INFO, 0, "call prv_dm_readResponse(),after readresponse enable sleep");
            cis_uri_t uriLocal;
            uriLocal = node->uri;
            prv_dm_readResponse(g_dmcontext, &uriLocal, node->mid);
            slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
        }
        break;
        case DM_SAMPLE_CALLBACK_DISCOVER:
        {
            cis_uri_t uriLocal;
            uriLocal = node->uri;
            prv_dm_discoverResponse(g_dmcontext, &uriLocal, node->mid);
        }
        break;
        case DM_SAMPLE_CALLBACK_WRITE:
        {
            //write
            prv_dm_writeResponse(g_dmcontext, &node->uri, node->param.toWrite.value, node->param.toWrite.count, node->mid);
            cis_data_t* data = node->param.toWrite.value;
            cis_attrcount_t count = node->param.toWrite.count;

            for (int i = 0; i < count; i++)
            {
                if (data[i].type == cis_data_type_string || data[i].type == cis_data_type_opaque)
                {
                    if (data[i].asBuffer.buffer != NULL)
                        cis_free(data[i].asBuffer.buffer);
                }
            }
            cis_free(data);
        }
        break;
        case DM_SAMPLE_CALLBACK_EXECUTE:
        {
            //exec and notify
            prv_dm_execResponse(g_dmcontext, &node->uri, node->param.toExec.buffer, node->param.toExec.length, node->mid);
            cis_free(node->param.toExec.buffer);
        }
        break;
        case DM_SAMPLE_CALLBACK_SETPARAMS:
        {
            //set parameters and notify
            prv_dm_paramsResponse(g_dmcontext, &node->uri, node->param.toObserveParam.params, node->mid);
        }
        break;
        case DM_SAMPLE_CALLBACK_OBSERVE:
        {
            if (node->param.toObserve.flag)
            {
                struct dm_observe_info* observe_new = (struct dm_observe_info*)cis_malloc(sizeof(struct dm_observe_info));
                cissys_assert(observe_new != NULL);
                observe_new->mid = node->mid;
                observe_new->uri = node->uri;
                observe_new->next = NULL;

                g_dmobserveList = (struct dm_observe_info*)cis_list_add((cis_list_t*)g_dmobserveList, (cis_list_t*)observe_new);
                /*
                LOGD("cis_on_observe set(%d): %d/%d/%d",
                count,
                uri->objectId,
                CIS_URI_IS_SET_INSTANCE(uri)?uri->instanceId:-1,
                CIS_URI_IS_SET_RESOURCE(uri)?uri->resourceId:-1);
                */

                cis_response(g_dmcontext, NULL, NULL, node->mid, CIS_RESPONSE_OBSERVE, 0);
            }
            else
            {
                struct dm_observe_info* delnode = NULL;

                g_dmobserveList = (struct dm_observe_info *)cis_list_remove((cis_list_t *)g_dmobserveList, node->mid, (cis_list_t **)&delnode);
                if (delnode == NULL)
                {
                    cis_free(node);
                    continue;
                }

                cis_free(delnode);
                /*
                LOGD("cis_on_observe cancel: %d/%d/%d\n",
                uri->objectId,
                CIS_URI_IS_SET_INSTANCE(uri)?uri->instanceId:-1,
                CIS_URI_IS_SET_RESOURCE(uri)?uri->resourceId:-1);
                */
                cis_response(g_dmcontext, NULL, NULL, node->mid, CIS_RESPONSE_OBSERVE, 0);
            }
        }
        default:
            break;
        }

        cis_free(node);
    }
}

int dm_sample_dm_entry(MidWareDmConfig* config, BOOL needRestore)
{
    int index = 0;
    et_client_state_t curStatus = PUMP_STATE_INITIAL;
    

    cis_callback_t callback;
    callback.onRead = dm_onRead;
    callback.onWrite = dm_onWrite;
    callback.onExec = dm_onExec;
    callback.onObserve = dm_onObserve;
    callback.onSetParams = dm_onParams;
    callback.onEvent = dm_onEvent;
    callback.onDiscover = dm_onDiscover;

    g_lifeTime = config->lifeTime;
    ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_1, P_SIG, 2, "lifetime:%d, testmode:%d", config->lifeTime, config->test);
    memset(dm_config.szDMv, 0, 16);
    strcpy(dm_config.szDMv, "v2.0");
    if(strlen(config->appKey)!=0){
        memset(dm_config.szAppKey, 0, 64);
        memcpy(dm_config.szAppKey, config->appKey, strlen(config->appKey));
    }
    if(strlen(config->secret)!=0){
        memset(dm_config.szPwd, 0, 64);
        memcpy(dm_config.szPwd, config->secret, strlen(config->secret));
    }

    prvMakeUserdata();

#ifdef  FEATURE_REF_AT_ENABLE
    int len = 0;
    if(cmcc_config_hex == NULL)
    {
        cmcc_config_hex = malloc(CMCCDM_CONFIG_MAX_BUFFER_LEN);
    }
    len = cmccCreateConfigBuf(cmcc_config_hex, config->serverIP, config->serverPort);
    if (cis_init(&g_dmcontext, (void *)cmcc_config_hex, len, &dm_config, defaultDmLocalPort) != CIS_RET_OK) {
        if (g_dmcontext != NULL)
            cis_deinit(&g_dmcontext);
        ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_308, P_INFO, 0, "cis entry init failed.");
        if(cmcc_config_hex != NULL)
        {
            free(cmcc_config_hex);
            cmcc_config_hex = NULL;
        }
        return -1;
    }
#else
    if(config->test == 1){
        if (cis_init(&g_dmcontext, (void *)dm_config_hex, sizeof(dm_config_hex), &dm_config, defaultDmLocalPort) != CIS_RET_OK) {
            if (g_dmcontext != NULL)
                cis_deinit(&g_dmcontext);
            ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_1_1, P_INFO, 0, "cis entry init failed.");
            return -1;
        }
    }else{
        if (cis_init(&g_dmcontext, (void *)dm_m_config_hex, sizeof(dm_m_config_hex), &dm_config, defaultDmLocalPort) != CIS_RET_OK) {
            if (g_dmcontext != NULL)
                cis_deinit(&g_dmcontext);
            ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_1_2, P_INFO, 0, "cis entry init failed.");
            return -1;
        }       
    }
#endif
    
    //log_config(false, 1, 10, 1024);//For debug
    st_context_t* ctx = (st_context_t*)g_dmcontext;
    ctx->regTimeout = 120;//set register timeout 120 seconds
    
    if (needRestore) {
        ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_2, P_INFO, 0, "needRestore ignoreRegistration");
        ((st_context_t*)g_dmcontext)->stateStep = PUMP_STATE_CONNECTING;
        ((st_context_t*)g_dmcontext)->ignoreRegistration = TRUE;
        ((st_context_t*)g_dmcontext)->location = malloc(strlen(g_retentContext.location) + 1);
        strcpy(((st_context_t*)g_dmcontext)->location,g_retentContext.location );
    }

    for (index = 0; index < DM_SAMPLE_OBJECT_MAX; index++)
    {
        cis_inst_bitmap_t bitmap;
        cis_res_count_t  rescount;
        cis_instcount_t instCount, instBytes;
        const char* instAsciiPtr;
        uint8_t * instPtr;
        cis_oid_t oid;
        int16_t i;
        dm_device_object* obj = &g_objectList[index];

        oid = obj->oid;
        instCount = utils_strlen(obj->instBitmap);//实例的个数包括不使能的实例
        instBytes = (instCount - 1) / 8 + 1;//要用多少个字节来表示实例，每个实例占用一位
        instAsciiPtr = obj->instBitmap;
        instPtr = (uint8_t*)cis_malloc(instBytes);
        cissys_assert(instPtr != NULL);
        cis_memset(instPtr, 0, instBytes);

        for (i = 0; i < instCount; i++)//这一段代码的意思是把类似"1101"的二进制字符串转换为二进制数据1101
        {
            cis_instcount_t instBytePos = i / 8;
            cis_instcount_t instByteOffset = 7 - (i % 8);
            if (instAsciiPtr[i] == '1') {
                instPtr[instBytePos] += 0x01 << instByteOffset;
            }
        }

        bitmap.instanceCount = instCount;
        bitmap.instanceBitmap = instPtr;
        bitmap.instanceBytes = instBytes;

        rescount.attrCount = obj->attrCount;
        rescount.actCount = obj->actCount;

        cis_addobject(g_dmcontext, oid, &bitmap, &rescount);
        ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_3, P_INFO, 1, "cis_addobject obj=%d",oid);
        cis_free(instPtr);
    }

    g_doUnregisterdm = false;

    //register enabled
    g_doRegisterdm = true;

    xTaskCreate(dmTaskThread,
                "dm task thread",
                1024 / sizeof(portSTACK_TYPE),
                NULL,
                osPriorityBelowNormal7,
                &resp_taskhandle);
    while (1)
    {
        /*
        *wait press keyboard for register test;
        *do register press 'o'
        *do unregister press 'r'
        **/
        if (g_doRegisterdm)
        {
            g_doRegisterdm = false;
            ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_4, P_INFO, 0, "open client");
            cis_register(g_dmcontext, config->lifeTime, &callback);//将回调函数注册进SDK内
        }
        if (g_doUnregisterdm) {
            g_doUnregisterdm = false;
            ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_4_2, P_INFO, 0, "close client");
            cis_unregister(g_dmcontext);
            struct dm_observe_info* delnode;
            while (g_dmobserveList != NULL) {
                g_dmobserveList = (struct dm_observe_info *)CIS_LIST_RM((cis_list_t *)g_dmobserveList, g_dmobserveList->mid, (cis_list_t **)&delnode);
                cis_free(delnode);
            }
            cissys_sleepms(1000);
            g_doRegisterdm = true;
        }
        /*pump function*/
        cis_pump(g_dmcontext);
        curStatus = ((st_context_t*)g_dmcontext)->stateStep;
        if(g_needUpdate && curStatus == PUMP_STATE_READY)
        {
            ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_5, P_INFO, 1, "g_needUpdate = %d,PUMP_STATE_READY call cis_update_reg", g_needUpdate);
            int ret = cis_update_reg(g_dmcontext,config->lifeTime,FALSE,2);
            g_needUpdate = FALSE;
            if(ret != 0){
                ECOMM_TRACE(UNILOG_DM, _dm_sample_dm_entry_5_1, P_INFO, 1, "cis_update_reg failed return %x go sleep and wakeup next update", ret);
                cmccDmSaveFile(DM_REG_FAIL_STATE);
                slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID8, (g_lifeTime-10)*1000);
                slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
            }
        }
        cissys_sleepms(200);
    }

    /*
    cis_deinit(&g_dmcontext);

    struct dm_observe_info* delnode;
    while (g_dmobserveList != NULL) {
        g_dmobserveList = (struct dm_observe_info *)CIS_LIST_RM((cis_list_t *)g_dmobserveList, g_dmobserveList->mid, (cis_list_t **)&delnode);
        cis_free(delnode);
    }
    cissys_sleepms(2000);
    return 0;
    */
}



//private funcation;
//static void prv_dm_observeNotify(void* context, cis_uri_t* uri, cis_mid_t mid)
//{

//}



static void prv_dm_readResponse(void* context, cis_uri_t* uri, cis_mid_t mid)
{
    uint8_t index;
    dm_device_object* object = NULL;
    for (index = 0;index < DM_SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        ECOMM_TRACE(UNILOG_DM, prv_dm_readResponse_0, P_INFO, 0, "object == NULL!!!");
        return;
    }

    if(!CIS_URI_IS_SET_RESOURCE(uri)) // one object
    {
        ECOMM_TRACE(UNILOG_DM, prv_dm_readResponse_1, P_INFO, 1, "read all object:%d", uri->objectId);
        switch(uri->objectId)
        {
            case DM_SAMPLE_OID_A:
            {
                for(index=0;index<DM_SAMPLE_A_INSTANCE_COUNT;index++)
                {
                    dm_instance *inst = &g_instList_a[index];
                    if(inst != NULL &&  inst->enabled == true)
                    {
                        cis_data_t tmpdata;
                        tmpdata.type = cis_data_type_string;

                        tmpdata.asBuffer.buffer = genEncodeValue(constDeviceInfo, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constDeviceInfo);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constDeviceInfo;
                        uri->resourceId = attributeA_deviceInfo;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constAppInfo, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constAppInfo);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constAppInfo;
                        uri->resourceId = attributeA_appInfo;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constMAC, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constMAC);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constMAC;
                        uri->resourceId = attributeA_MAC;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constROM, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constROM);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constROM;
                        uri->resourceId = attributeA_ROM;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constRAM, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constRAM);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constRAM;
                        uri->resourceId = attributeA_RAM;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constCPU, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constCPU);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constCPU;
                        uri->resourceId = attributeA_CPU;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constSysVer, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constSysVer);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constSysVer;
                        uri->resourceId = attributeA_sysVersion;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constSoftVer, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constSoftVer);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constSoftVer;
                        uri->resourceId = attributeA_softwareVer;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constSoftNam, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constSoftNam);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constSoftNam;
                        uri->resourceId = attributeA_softwareName;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constVolte, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constVolte);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constVolte;
                        uri->resourceId = attributeA_volte;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constNetType, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constNetType);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constNetType;
                        uri->resourceId = attributeA_netType;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);
#ifndef FEATURE_REF_AT_ENABLE
                        tmpdata.asBuffer.buffer = genEncodeValue(constAccount, &(tmpdata.asBuffer.length), &dm_config);
#else
                        tmpdata.asBuffer.buffer = genEncodeValue(ecDMconfig->account, &(tmpdata.asBuffer.length), &dm_config);
#endif
                        //tmpdata.asBuffer.length = strlen(constAccount);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constAccount;
                        uri->resourceId = attributeA_account;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);
#ifndef FEATURE_REF_AT_ENABLE
                        tmpdata.asBuffer.buffer = genEncodeValue(constPhoneNumber, &(tmpdata.asBuffer.length), &dm_config);
#else
                        tmpdata.asBuffer.buffer = genEncodeValue(ecDMconfig->phoneNum, &(tmpdata.asBuffer.length), &dm_config);
#endif
                        //tmpdata.asBuffer.length = strlen(constPhoneNumber);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constPhoneNumber;
                        uri->resourceId = attributeA_phoneNumber;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);

                        tmpdata.asBuffer.buffer = genEncodeValue(constvl, &(tmpdata.asBuffer.length), &dm_config);
                        //tmpdata.asBuffer.length = strlen(constvl);
                        //tmpdata.asBuffer.buffer = (uint8_t*)constvl;
                        uri->resourceId = attributeA_location;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata,mid,CIS_RESPONSE_CONTINUE,0);
                        free(tmpdata.asBuffer.buffer);
                    }
                }
            }
            break;
        }
        cis_response(context,NULL,NULL,mid,CIS_RESPONSE_READ,0);

    }else{
        ECOMM_TRACE(UNILOG_DM, prv_dm_readResponse_2, P_INFO, 1, "read object:%d;instance:%d", object->oid, uri->instanceId);
        switch(object->oid)
        {
            case DM_SAMPLE_OID_A:
            {
                if(uri->instanceId > DM_SAMPLE_A_INSTANCE_COUNT){
                    return;
                }
                dm_instance *inst = &g_instList_a[uri->instanceId];
                if(inst == NULL || inst->enabled == false){
                    ECOMM_TRACE(UNILOG_DM, prv_dm_readResponse_3, P_INFO, 2, "inst:%x,enabled:%d", inst, inst->enabled);
                    return;
                }

                if(CIS_URI_IS_SET_RESOURCE(uri)){
                    cis_data_t value;
                    value.type = cis_data_type_string;
                    ECOMM_TRACE(UNILOG_DM, prv_dm_readResponse_4, P_INFO, 1, "resourceid:%d", uri->resourceId);
                    if(uri->resourceId == attributeA_deviceInfo)
                    {
                        //value.asBuffer.length = strlen(constDeviceInfo);
                        //value.asBuffer.buffer = (uint8_t*)constDeviceInfo;
                        value.asBuffer.buffer = genEncodeValue(constDeviceInfo, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_appInfo)
                    {
                        value.asBuffer.buffer = genEncodeValue(constAppInfo, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_MAC)
                    {
                        value.asBuffer.buffer = genEncodeValue(constMAC, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_ROM)
                    {
                        value.asBuffer.buffer = genEncodeValue(constROM, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_RAM)
                    {
                        value.asBuffer.buffer = genEncodeValue(constRAM, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_CPU)
                    {
                        value.asBuffer.buffer = genEncodeValue(constCPU, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_sysVersion)
                    {
                        value.asBuffer.buffer = genEncodeValue(constSysVer, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_softwareVer)
                    {
                        value.asBuffer.buffer = genEncodeValue(constSoftVer, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_softwareName)
                    {
                        value.asBuffer.buffer = genEncodeValue(constSoftNam, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_volte)
                    {
                        value.asBuffer.buffer = genEncodeValue(constVolte, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_netType)
                    {
                        value.asBuffer.buffer = genEncodeValue(constNetType, &(value.asBuffer.length), &dm_config);
                    }
                    else if(uri->resourceId == attributeA_account)
                    {
#ifndef FEATURE_REF_AT_ENABLE
                        value.asBuffer.buffer = genEncodeValue(constAccount, &(value.asBuffer.length), &dm_config);
#else
                        value.asBuffer.buffer = genEncodeValue(ecDMconfig->account, &(value.asBuffer.length), &dm_config);
#endif
                    }
                    else if(uri->resourceId == attributeA_phoneNumber)
                    {
#ifndef FEATURE_REF_AT_ENABLE
                        value.asBuffer.buffer = genEncodeValue(constPhoneNumber, &(value.asBuffer.length), &dm_config);
#else
                        value.asBuffer.buffer = genEncodeValue(ecDMconfig->phoneNum, &(value.asBuffer.length), &dm_config);
#endif
                    }
                    else if(uri->resourceId == attributeA_location)
                    {
                        value.asBuffer.buffer = genEncodeValue(constvl, &(value.asBuffer.length), &dm_config);
                    }
                    cis_response(context,uri,&value,mid,CIS_RESPONSE_READ,0);
                    free(value.asBuffer.buffer);
                }
            }
            break;
        }
    }
}


static void prv_dm_discoverResponse(void* context, cis_uri_t* uri, cis_mid_t mid)
{
    uint8_t index;
    dm_device_object* object = NULL;

    for (index = 0;index < DM_SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        return;
    }


    switch(uri->objectId)
    {
        case DM_SAMPLE_OID_A:
        {
            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_deviceInfo;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_appInfo;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_MAC;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_ROM;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_RAM;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_CPU;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_sysVersion;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_softwareVer;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_softwareName;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_volte;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_netType;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_account;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_phoneNumber;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);

            uri->objectId = DM_SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_location;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE,0);
        }
        break;
    }
    cis_response(context,NULL,NULL,mid,CIS_RESPONSE_DISCOVER,0);
}

static void prv_dm_writeResponse(void* context, cis_uri_t* uri, const cis_data_t* value, cis_attrcount_t count, cis_mid_t mid)
{

}

static void prv_dm_execResponse(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length, cis_mid_t mid)
{

}

static void prv_dm_paramsResponse(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid)
{

}

static cis_coapret_t dm_onRead(void* context, cis_uri_t* uri, cis_mid_t mid)
{
    struct dm_callback_info* newNode = (struct dm_callback_info*)cis_malloc(sizeof(struct dm_callback_info));
    newNode->next = NULL;
    newNode->flag = DM_SAMPLE_CALLBACK_READ;
    newNode->mid = mid;
    newNode->uri = *uri;
    g_dmcallbackList = (struct dm_callback_info*)CIS_LIST_ADD(g_dmcallbackList, newNode);

    ECOMM_TRACE(UNILOG_DM, dm_onRead, P_INFO, 3, "Read:(%d/%d/%d)", uri->objectId, uri->instanceId, uri->resourceId);
    LOGD("cis_onRead:(%d/%d/%d)\n", uri->objectId, uri->instanceId, uri->resourceId);
    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t dm_onDiscover(void* context, cis_uri_t* uri, cis_mid_t mid)
{
    struct dm_callback_info* newNode = (struct dm_callback_info*)cis_malloc(sizeof(struct dm_callback_info));
    newNode->next = NULL;
    newNode->flag = DM_SAMPLE_CALLBACK_DISCOVER;
    newNode->mid = mid;
    newNode->uri = *uri;
    g_dmcallbackList = (struct dm_callback_info*)CIS_LIST_ADD(g_dmcallbackList, newNode);

    ECOMM_TRACE(UNILOG_DM, dm_onDiscover, P_INFO, 3, "Discover:(%d/%d/%d)", uri->objectId, uri->instanceId, uri->resourceId);
    LOGD("cis_onDiscover:(%d/%d/%d)\n", uri->objectId, uri->instanceId, uri->resourceId);
    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t dm_onWrite(void* context, cis_uri_t* uri, const cis_data_t* value, cis_attrcount_t attrcount, cis_mid_t mid)
{
    return CIS_CALLBACK_CONFORM;
}


static cis_coapret_t dm_onExec(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length, cis_mid_t mid)
{
    return CIS_CALLBACK_CONFORM;
}


static cis_coapret_t dm_onObserve(void* context, cis_uri_t* uri, bool flag, cis_mid_t mid)
{
    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t dm_onParams(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    return CIS_CALLBACK_CONFORM;
}

#ifndef FEATURE_REF_AT_ENABLE
void dm_onEvent(void* context, cis_evt_t eid, void* param)
{
    switch (eid)
    {
    case CIS_EVENT_UPDATE_NEED:
        ECOMM_TRACE(UNILOG_DM, _dm_onEvent_1, P_SIG, 1, "DM need to update,reserve time:%ds", (int32_t)param);
        if(!g_needUpdate){
            ECOMM_TRACE(UNILOG_DM, _dm_onEvent_1_1, P_INFO, 0, "no deepSleepTimer auto update reg", (int32_t)param);
            cis_update_reg(g_dmcontext, LIFETIME_INVALID, false, 2);
        }else{
            ECOMM_TRACE(UNILOG_DM, _dm_onEvent_1_2, P_INFO, 0, "deepSleepTimer is work not update reg", (int32_t)param);
        }
        break;
    case CIS_EVENT_REG_SUCCESS:
        ECOMM_TRACE(UNILOG_DM, _dm_onEvent_2, P_SIG, 0,"DM register success. save context, start deep sleep timer");
        cmccDmSaveFile(DM_REG_SUC_STATE);
        slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID8, (g_lifeTime-10)*1000);
        break;
    case CIS_EVENT_REG_FAILED:
    case CIS_EVENT_REG_TIMEOUT:
        ECOMM_TRACE(UNILOG_DM, _dm_onEvent_3, P_SIG, 1,"DM register failed(%d). save regstat DM_REG_FAIL_STATE, enable sleeep",eid);
        cmccDmSaveFile(DM_REG_FAIL_STATE);
        break;
    case CIS_EVENT_UPDATE_SUCCESS:
        ECOMM_TRACE(UNILOG_DM, _dm_onEvent_4, P_SIG, 0, "DM update success, start deep sleep timer, enable sleep");
        slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID8, (g_lifeTime-10)*1000);
        slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
        break;
    case CIS_EVENT_UPDATE_FAILED:
    case CIS_EVENT_UPDATE_TIMEOUT:
        cmccDmSaveFile(DM_REG_FAIL_STATE);
        slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID8, (g_lifeTime-10)*1000);
        slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
        ECOMM_TRACE(UNILOG_DM, _dm_onEvent_5, P_SIG, 0, "DM update failed, save regstat DM_REG_FAIL_STATE, enable sleep");
    default:
        ECOMM_TRACE(UNILOG_DM, _dm_onEvent_6, P_SIG, 2, "event:%d,param:%d", eid, (int32_t)param);
        break;
    }
}
#else
void dm_onEvent(void* context, cis_evt_t eid, void* param)
{
    switch (eid)
    {
    case CIS_EVENT_UPDATE_NEED:
        LOGD("cis_on_event need to update,reserve time:%ds\n", (int32_t)param);
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_1, P_INFO, 1, "DM need to update,reserve time:%ds", (int32_t)param);
        if(!g_needUpdate){
            ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_1_1, P_INFO, 0, "no deepSleepTimer auto update reg", (int32_t)param);
            cis_update_reg(g_dmcontext, LIFETIME_INVALID, false, 2);
        }else{
            ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_1_2, P_INFO, 0, "deepSleepTimer is work not update reg", (int32_t)param);
        }
        break;
        
    case CIS_EVENT_REG_SUCCESS:
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_2, P_SIG, 0,"DM register success. save context, start deep sleep timer");
        cmccDmSaveFile(DM_REG_SUC_STATE);
        slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID8, (g_lifeTime-10)*1000);
        break;
        
    case CIS_EVENT_REG_FAILED:
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_3, P_SIG, 0,"DM register failed. save regstat DM_REG_FAIL_STATE, enable sleeep");
        cmccDmSaveFile(DM_REG_FAIL_STATE);
        break;
        
    case CIS_EVENT_REG_TIMEOUT:
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_4, P_SIG, 0,"DM register failed. save regstat DM_REG_FAIL_STATE, enable sleeep");
        cmccDmSaveFile(DM_REG_TIMEOUT_STATE);
        break;
        
    case CIS_EVENT_UPDATE_SUCCESS:
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_5, P_INFO, 0, "DM update success, start deep sleep timer, enable sleep");
        cmccDmSaveFile(DM_REG_HB_SUC_STATE);
        slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID8, (g_lifeTime-10)*1000);
        slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
        break;
        
    case CIS_EVENT_UPDATE_FAILED:
        cmccDmSaveFile(DM_REG_HB_FAIL_STATE);
        slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_6, P_INFO, 0, "DM update failed, save regstat DM_REG_FAIL_STATE, enable sleep");
        break;
        
    case CIS_EVENT_UPDATE_TIMEOUT:
        cmccDmSaveFile(DM_REG_HB_TIMEOUT_STATE);
        slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_7, P_INFO, 0, "DM update failed, save regstat DM_REG_FAIL_STATE, enable sleep");
        break;
        
    default:
        ECOMM_TRACE(UNILOG_DM, ref_dm_onEvent_8, P_INFO, 2, "event:%d,param:%d", eid, (int32_t)param);
        break;
    }
}
#endif

BOOL ifNeedRestoreDMTask(void)
{
    INT8 ret=0;
    BOOL needRestoreTask = FALSE;

    ret = cmccDmReadFile();
    if(ret<0){//read file failed cant restore task
        ECOMM_TRACE(UNILOG_DM, ifNeedTask_1, P_INFO, 0, "read file failed start new task");
        return needRestoreTask;
    }
    if(g_retentContext.bConnected == DM_REG_SUC_STATE && g_retentContext.pumpState == PUMP_STATE_READY){
        ECOMM_TRACE(UNILOG_DM, ifNeedTask_2, P_INFO, 0, "before has dm connected, need restore task");
        needRestoreTask = TRUE;
    }

    return needRestoreTask;
}

static INT32 dmPSUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    NmAtiNetifInfo *netif = NULL;
    switch(eventID)
    {
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                ECOMM_TRACE(UNILOG_DM, dmPSUrcCallback_1, P_INFO, 0, "PSIF network active change statemachine to DM_IPREADY_STATE");
                stateMachine = DM_IPREADY_STATE;
            }
            else if (netif->netStatus == NM_NETIF_OOS)
            {
                ECOMM_TRACE(UNILOG_DM, dmPSUrcCallback_2, P_INFO, 0, "PSIF network OOS");
            }
            else if (netif->netStatus == NM_NO_NETIF_OR_DEACTIVATED ||
                     netif->netStatus == NM_NO_NETIF_NOT_DIAL)
            {
                ECOMM_TRACE(UNILOG_DM, dmPSUrcCallback_3, P_INFO, 0, "PSIF network deactive");
            }
            break;
        }
        case NB_URC_ID_MM_CERES_CHANGED:
        {
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
            cissys_setCoapMaxTransmitWaitTime();
            ECOMM_TRACE(UNILOG_DM, dmPSUrcCallback_4, P_INFO, 1, "celevel=%d", celevel);
            break;
        }
        default:break;
    }
    return 0;
}

static BOOL dmCheckNetworkReady(UINT8 waitTime)
{
    BOOL result = TRUE;
    NmAtiSyncRet netStatus;
    UINT32 start, end;
    
    start = cissys_gettime()/osKernelGetTickFreq();
    end = start;
    // Check network for waitTime*2 seconds
    UINT8 network=0;
    while(end-start < waitTime)
    {
        appGetNetInfoSync(0, &netStatus);
        if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
        {
            ECOMM_TRACE(UNILOG_ONENET, checkNetworkReady_1, P_INFO, 0, "network ready");
            network = 1;
            break;
        }
        cissys_sleepms(500);
        end = cissys_gettime()/osKernelGetTickFreq();
    }
    if(network == 0){
        ECOMM_TRACE(UNILOG_ONENET, checkNetworkReady_2, P_INFO, 0, "no network ");
        result = FALSE;
    }
    return result;

}

static void dmTaskProcessEntry(void * arg)
{
    ecDMconfig = malloc(sizeof(MidWareDmConfig));
    memset(ecDMconfig, 0, sizeof(MidWareDmConfig));

    ECOMM_TRACE(UNILOG_DM, _dmTaskProcessEntry_1, P_SIG, 4, "DM task start! needrestore--->%d",needRestoreDMTask);
    
    mwGetAllDMConfig(ecDMconfig);
    dm_sample_dm_entry(ecDMconfig, needRestoreDMTask);
    free(ecDMconfig);
#ifdef  FEATURE_REF_AT_ENABLE
    if(cmcc_config_hex != NULL)
    {
        free(cmcc_config_hex);
        cmcc_config_hex = NULL;
    }   
#endif
}

static void dmConnectTask(void *arg)
{
    while (1)
    {
        char logbuf[64] = {0};
        snprintf(logbuf,64,"%s",DM_MACHINESTATE(stateMachine));
        ECOMM_STRING(UNILOG_DM, dmConnectTask_0, P_INFO, "handle stateMachine:%s", (uint8_t *)logbuf);
        switch(stateMachine)
        {
           case DM_INIT_STATE:
                osDelay(500/portTICK_PERIOD_MS);
                break;
           case DM_HEART_BEAT_UPDATE://to heartbeat
           case DM_IPREADY_STATE://poweron to register
           {
                ECOMM_TRACE(UNILOG_DM, dmConnectTask_1, P_INFO, 0, "start dmTask and exit dmConnectTask");
                osThreadAttr_t task_attr;
                memset(&task_attr, 0, sizeof(task_attr));
                task_attr.name = "dmtask";
                task_attr.stack_mem = NULL;
                task_attr.stack_size = DM_TASK_STACK_SIZE;
                task_attr.priority = osPriorityBelowNormal7;
                task_attr.cb_mem = NULL;
                task_attr.cb_size = 0;
                
                osThreadNew(dmTaskProcessEntry, NULL, &task_attr);
                osThreadExit();
                
                break;
           }
           case DM_HEART_BEAT_ARRIVED:
           {
                // wait 120s for network
                if(dmCheckNetworkReady(120) != TRUE)
                {
                    ECOMM_TRACE(UNILOG_DM, dmConnectTask_2_1, P_ERROR, 0, "app update network connect error enable sleep go idle");
                    slpManPlatVoteEnableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
                    stateMachine = DM_IDLE_STATE;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_DM, dmConnectTask_2, P_INFO, 0, "network ok send heartbeat");
                    stateMachine = DM_HEART_BEAT_UPDATE;
                }
                break;
           }
           case  DM_IDLE_STATE://nothing to do can handle key event
           {
               ECOMM_TRACE(UNILOG_DM, dmConnectTask_3, P_INFO, 0, "app enter idle");
               osDelay(500/portTICK_PERIOD_MS);
               break;
           }
           default:
           {
               ECOMM_TRACE(UNILOG_DM, dmConnectTask_4, P_INFO, 0, "app enter waiting state");
               osDelay(500/portTICK_PERIOD_MS);
               break;
           }

        }

    }
}

void dmDeepSleepCb(uint8 id)
{
    ECOMM_TRACE(UNILOG_DM, deepSleepCb_1, P_SIG, 1, "callback ID:%d come, disable slp2", id);
    slpManPlatVoteDisableSleep(cmccDmSlpHandler, SLP_SLP2_STATE);
    stateMachine = DM_HEART_BEAT_ARRIVED;
    g_needUpdate = TRUE;
}

void dmDeepSleepTimerCbRegister(void)
{
    slpManInternalDeepSlpTimerRegisterExpCb(dmDeepSleepCb,DEEPSLP_TIMER_ID8);
}

INT8 cmccDmReadRegstat(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    cmccDmNvmHeader    nvmHdr;   //4 bytes
    cmccDmRetentionContext m_retentContext;

    /*
     * open the NVM file
    */
    fp = OsaFopen(CMCCDM_NVM_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmReadRegstat_1, P_INFO, 0,
                    "can't open NVM: 'cmccdmconfig.nvm', maybe not start dm");
        return -1;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&nvmHdr, sizeof(cmccDmNvmHeader), 1, fp);
    if (readCount != 1 ||
        nvmHdr.fileBodySize != sizeof(cmccDmRetentionContext))
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmReadRegstat_2, P_ERROR, 2,
                    "'cmccdmconfig.nvm', can't read header, or header not right, (%u/%u), use the defult value",
                    nvmHdr.fileBodySize, sizeof(cmccDmRetentionContext));

        OsaFclose(fp);

        return -2;
    }

    /*
     * read the file body retention context
    */
    readCount = OsaFread(&m_retentContext, sizeof(cmccDmRetentionContext), 1, fp);

    OsaFclose(fp);

    if (readCount != 1)
    {
        ECOMM_TRACE(UNILOG_DM, cmccDmReadRegstat_3, P_ERROR, 0,"'cmccdmconfig.nvm', can't read body");
        OsaFclose(fp);
        return -3;
    }

    ECOMM_TRACE(UNILOG_DM, cmccDmReadRegstat_4, P_INFO, 2, "isConnected=%d, pumpState=%d",m_retentContext.bConnected);
    return m_retentContext.bConnected;
}

void cmccAutoRegisterInit(slpManSlpState_t slpState)
{
    registerPSEventCallback(NB_GROUP_ALL_MASK, dmPSUrcCallback);
    
    
    osThreadAttr_t task_attr;
    memset(&task_attr,0,sizeof(task_attr));
    memset(dmconnTaskStack, 0xA5, DMCONN_TASK_STACK_SIZE);
    task_attr.name = "dmConnTask";
    task_attr.stack_mem = dmconnTaskStack;
    task_attr.stack_size = DMCONN_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &dmconnTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(dmConnectTask, NULL, &task_attr);

    needRestoreDMTask = ifNeedRestoreDMTask();
    
    ECOMM_TRACE(UNILOG_DM, cmccAutoRegisterInit_0, P_INFO, 2, "slpState = %d,needRestoreDMTask=%d",slpState,needRestoreDMTask);
    if(slpState == SLP_HIB_STATE || slpState == SLP_SLP2_STATE)//wakeup from deep sleep
    {
        ECOMM_TRACE(UNILOG_DM, cmccAutoRegisterInit_0_1, P_INFO, 0, "wakeup from deep sleep");
    }
    else
    {
        ECOMM_TRACE(UNILOG_DM, cmccAutoRegisterInit_0_2, P_INFO, 0, "system reset, register DM");
        if(needRestoreDMTask == TRUE){
            cmccDmClearFile();
            needRestoreDMTask = FALSE;
        }
    }
    
    
}

#endif
