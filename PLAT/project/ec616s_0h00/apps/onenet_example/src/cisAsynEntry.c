/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    cisAsynEntry.c
 * Description:  EC616 onenet cis async entry source file
 * History:      Rev1.0   2018-10-12
 *
 ****************************************************************************/
#include <cis_if_sys.h>
#include <cis_list.h>
#include <cis_def.h>
#include "cisAsynEntry.h"
#include "osasys.h"
#include "task.h"
#include "Flash_ec616s_rt.h"
#include "slpman_ec616s.h"
#include "lfs_port.h"
#include "debug_log.h"
#include "ps_event_callback.h"
#include "ps_lib_api.h"
#include "psifevent.h"
#include "lwip/netdb.h"

//#define DEBUG_SLEEP_BACKUP
#define USE_PLAIN_BOOTSTRAP         (0)
#define ONENET_TASK_STACK_SIZE      (1024*3)
#define CIS_ENABLE_CONTEXT_RESTORE  (1)

#define ONENET_MACHINESTATE(S)                                \
((S) == ONENET_INIT_STATE ? "ONENET_INIT_STATE" :      \
((S) == ONENET_IPREADY_STATE ? "ONENET_IPREADY_STATE" :      \
((S) == ONENET_HEART_BEAT_ARRIVED ? "ONENET_HEART_BEAT_ARRIVED" :        \
((S) == ONENET_HEART_BEAT_UPDATE ? "ONENET_HEART_BEAT_UPDATE" :      \
((S) == ONENET_IDLE_STATE ? "ONENET_IDLE_STATE" :  \
((S) == ONENET_RESTORE_STATE ? "ONENET_RESTORE_STATE" :  \
"Unknown"))))))


//authcode: psk: 183.230.40.39:5683 enable bootstrap
static const uint8_t config_hex[] = {
    0x13, 0x00, 0x45,
    0xf1, 0x00, 0x03,
    0xf2, 0x00, 0x37,
    0x05, 0x00 /*mtu*/, 0x11 /*Link & bind type*/, 0x80 /*BS ENABLE DTLS DISABLED*/,
    0x00, 0x05 /*apn length*/, 0x43, 0x4d, 0x49, 0x4f, 0x54 /*apn: CMIOT*/,
    0x00, 0x00 /*username length*/, /*username*/
    0x00, 0x00 /*password length*/, /*password*/
    0x00, 0x12 /*host length*/,
    0x31, 0x38, 0x33, 0x2e, 0x32, 0x33, 0x30, 0x2e,
    0x34, 0x30, 0x2e, 0x33, 0x39, 0x3a, 0x35, 0x36, 0x38, 0x33 /*host: 183.230.40.39:5683*/,
    0x00, 0x0f /*userdata length*/, 0x41, 0x75, 0x74, 0x68, 0x43, 0x6f, 0x64, 0x65, 
    0x3a, 0x3b, 0x50, 0x53, 0x4b, 0x3a, 0x3b/*userdata: AuthCode:;PSK:;*/,
    0xf3, 0x00, 0x08,0xe4 /*log config*/, 0x00, 0xc8 /*LogBufferSize: 200*/,
    0x00, 0x00 /*userdata length*//*userdata*/   
};
    
uint8_t onenetASlpHandler           = 0xff;
onenetStateMachine_t stateMachine = ONENET_INIT_STATE;
static BOOL needRestoreOnenetTask = FALSE;
static uint8 g_needUpdate = 0;
static slpManTimerID_e heartBeatTimerID = DEEPSLP_TIMER_ID0;

static StaticTask_t              onenetTask;
static osThreadId_t              onenetTaskId = NULL;
static uint8_t                   onenetTaskStack[ONENET_TASK_STACK_SIZE];
static void*                     gCisContext = NULL;
static bool                      g_shutdown = false;
static cis_time_t                g_lifetime = 36000;
static cis_time_t                g_notifyLast = 0;
static bool                      gNotifyOngoing = false;
static uint8_t                   g_fotaState = ONENET_FOTA_NO;
static struct st_callback_info*  g_callbackList = NULL;
static struct st_observe_info*   g_observeList  = NULL;
static st_sample_object          g_objectList[SAMPLE_OBJECT_MAX];
static st_instance_a             g_instList_a[SAMPLE_A_INSTANCE_COUNT];
static CHAR *defaultLocalPort    = "40962";
static onenet_context_t          gOnenetContextRunning;
extern observed_backup_t         g_observed_backup[MAX_OBSERVED_COUNT];

void cisSyncBackup(void);
void cisSyncClear(void);


//////////////////////////////////////////////////////////////////////////
//private funcation;
static void prvObserveNotify(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    uint8_t index;
    st_sample_object* object = NULL;
    cis_data_t value;
    uint32_t ackId = 2020;
    for (index = 0;index < SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        ECOMM_TRACE(UNILOG_PLA_APP, prv_observeNotify_0, P_ERROR, 0, " no find object return direct");
        return;
    }
    ECOMM_TRACE(UNILOG_PLA_APP, prv_observeNotify_1, P_INFO, 4, "uri flag(0x%x) oid(%d),iid(%d)", uri->flag,uri->objectId,uri->instanceId);

    if(!CIS_URI_IS_SET_INSTANCE(uri) && !CIS_URI_IS_SET_RESOURCE(uri))
    {
        switch(uri->objectId)
        {
            case SAMPLE_OID_A:
            {
                for(index=0;index<SAMPLE_A_INSTANCE_COUNT;index++)
                {                   
                    st_instance_a *inst = &g_instList_a[index];
                    if(inst != NULL &&  inst->enabled == true)
                    {
                        cis_data_t tmpdata[4];
                        tmpdata[0].type = cis_data_type_integer;
                        tmpdata[0].value.asInteger = inst->instance.intValue;
                        uri->instanceId = inst->instId;
                        uri->resourceId = attributeA_intValue;
                        cis_uri_update(uri);
                        cis_notify_ec(context,uri,&tmpdata[0],mid,CIS_NOTIFY_CONTINUE,ackId, PS_SOCK_RAI_NO_INFO);

                        tmpdata[2].type = cis_data_type_bool;
                        tmpdata[2].value.asBoolean = inst->instance.boolValue;
                        uri->resourceId = attributeA_boolValue;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_notify_ec(context,uri,&tmpdata[2],mid,CIS_NOTIFY_CONTINUE,ackId, PS_SOCK_RAI_NO_INFO);

                        tmpdata[3].type = cis_data_type_string;
                        tmpdata[3].asBuffer.length = strlen(inst->instance.strValue);
                        tmpdata[3].asBuffer.buffer = (uint8_t*)(inst->instance.strValue);
                        uri->resourceId = attributeA_stringValue;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_notify_ec(context,uri,&tmpdata[3],mid,CIS_NOTIFY_CONTENT,ackId, PS_SOCK_RAI_NO_INFO);
                    }
                }
            }
            break;
        }
    }
    else if(CIS_URI_IS_SET_INSTANCE(uri))
    {
        switch(object->oid)
        {
            case SAMPLE_OID_A:
            {
                if(uri->instanceId > SAMPLE_A_INSTANCE_COUNT){
                    return;
                }
                st_instance_a *inst = &g_instList_a[uri->instanceId];
                if(inst == NULL || inst->enabled == false){
                    return;
                }

                if(CIS_URI_IS_SET_RESOURCE(uri)){
                    if(uri->resourceId == attributeA_intValue)
                    {
                        value.type = cis_data_type_integer;
                        value.value.asInteger = inst->instance.intValue;
                    }
                    else if(uri->resourceId == attributeA_boolValue)
                    {
                        value.type = cis_data_type_bool;
                        value.value.asBoolean = inst->instance.boolValue;
                    }
                    else if(uri->resourceId == attributeA_stringValue)
                    {
                        value.type = cis_data_type_string;
                        value.asBuffer.length = strlen(inst->instance.strValue);
                        value.asBuffer.buffer = (uint8_t*)(inst->instance.strValue);
                    }else{
                        return;
                    }

                    cis_notify_ec(context,uri,&value,mid,CIS_NOTIFY_CONTENT,ackId, PS_SOCK_RAI_NO_INFO);

                }
                else
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, prv_observeNotify_2, P_INFO, 0, "noify three resource");
                    cis_data_t tmpdata[4];

                    tmpdata[0].type = cis_data_type_integer;
                    tmpdata[0].value.asInteger = inst->instance.intValue;
                    uri->resourceId = attributeA_intValue;
                    cis_uri_update(uri);
                    cis_notify_ec(context,uri,&tmpdata[0],mid,CIS_NOTIFY_CONTINUE,ackId, PS_SOCK_RAI_NO_INFO);


                    tmpdata[2].type = cis_data_type_bool;
                    tmpdata[2].value.asBoolean = inst->instance.boolValue;
                    uri->resourceId = attributeA_boolValue;
                    cis_uri_update(uri);
                    cis_notify_ec(context,uri,&tmpdata[2],mid,CIS_NOTIFY_CONTINUE,ackId, PS_SOCK_RAI_NO_INFO);

                    tmpdata[3].type = cis_data_type_string;
                    tmpdata[3].asBuffer.length = strlen(inst->instance.strValue);
                    tmpdata[3].asBuffer.buffer = (uint8_t*)(inst->instance.strValue);
                    uri->resourceId = attributeA_stringValue;
                    cis_uri_update(uri);
                    cis_notify_ec(context,uri,&tmpdata[3],mid,CIS_NOTIFY_CONTENT,ackId, PS_SOCK_RAI_NO_INFO);
                }
            }
            break;
        }
    }
}

static void prvReadResponse(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    uint8_t index;
    st_sample_object* object = NULL;
    cis_data_t value;
    for (index = 0;index < SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        return;
    }

    if(!CIS_URI_IS_SET_INSTANCE(uri) && !CIS_URI_IS_SET_RESOURCE(uri)) // one object
    {
        switch(uri->objectId)
        {
            case SAMPLE_OID_A:
            {
                for(index=0;index<SAMPLE_A_INSTANCE_COUNT;index++)
                {                   
                    st_instance_a *inst = &g_instList_a[index];
                    if(inst != NULL &&  inst->enabled == true)
                    {
                        cis_data_t tmpdata[4];
                        tmpdata[0].type = cis_data_type_integer;
                        tmpdata[0].value.asInteger = inst->instance.intValue;
                        uri->instanceId = inst->instId;
                        uri->resourceId = attributeA_intValue;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata[0],mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);


                        tmpdata[2].type = cis_data_type_bool;
                        tmpdata[2].value.asBoolean = inst->instance.boolValue;
                        uri->resourceId = attributeA_boolValue;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata[2],mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);

                        tmpdata[3].type = cis_data_type_string;
                        tmpdata[3].asBuffer.length = strlen(inst->instance.strValue);
                        tmpdata[3].asBuffer.buffer = (uint8_t*)strdup(inst->instance.strValue);
                        uri->resourceId = attributeA_stringValue;
                        uri->instanceId = inst->instId;
                        cis_uri_update(uri);
                        cis_response(context,uri,&tmpdata[3],mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);
                    }
                }
            }
            break;
        }
        cis_response(context,NULL,NULL,mid,CIS_RESPONSE_READ, PS_SOCK_RAI_NO_INFO);

    }else
    {
        switch(object->oid)
        {
            case SAMPLE_OID_A:
            {
                if(uri->instanceId > SAMPLE_A_INSTANCE_COUNT){
                    return;
                }
                st_instance_a *inst = &g_instList_a[uri->instanceId];
                if(inst == NULL || inst->enabled == false){
                    return;
                }

                if(CIS_URI_IS_SET_RESOURCE(uri)){
                    if(uri->resourceId == attributeA_intValue)
                    {
                        value.type = cis_data_type_integer;
                        value.value.asInteger = inst->instance.intValue;
                    }
                    else if(uri->resourceId == attributeA_boolValue)
                    {
                        value.type = cis_data_type_bool;
                        value.value.asBoolean = inst->instance.boolValue;
                    }
                    else if(uri->resourceId == attributeA_stringValue)
                    {
                        value.type = cis_data_type_string;
                        value.asBuffer.length = strlen(inst->instance.strValue);
                        value.asBuffer.buffer = (uint8_t*)strdup(inst->instance.strValue);
                    }else{
                        return;
                    }

                    cis_response(context,uri,&value,mid,CIS_RESPONSE_READ, PS_SOCK_RAI_NO_INFO);
                }else{
                    cis_data_t tmpdata[4];

                    tmpdata[0].type = cis_data_type_integer;
                    tmpdata[0].value.asInteger = inst->instance.intValue;
                    uri->resourceId = attributeA_intValue;
                    cis_uri_update(uri);
                    cis_response(context,uri,&tmpdata[0],mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);

                    tmpdata[2].type = cis_data_type_bool;
                    tmpdata[2].value.asBoolean = inst->instance.boolValue;
                    uri->resourceId = attributeA_boolValue;
                    cis_uri_update(uri);
                    cis_response(context,uri,&tmpdata[2],mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);

                    tmpdata[3].type = cis_data_type_string;
                    tmpdata[3].asBuffer.length = strlen(inst->instance.strValue);
                    tmpdata[3].asBuffer.buffer = (uint8_t*)strdup(inst->instance.strValue);
                    uri->resourceId = attributeA_stringValue;
                    cis_uri_update(uri);
                    cis_response(context,uri,&tmpdata[3],mid,CIS_RESPONSE_READ, PS_SOCK_RAI_NO_INFO);
                }
            }
            break;
        }
    }
}


static void prvDiscoverResponse(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    uint8_t index;
    st_sample_object* object = NULL;

    for (index = 0;index < SAMPLE_OBJECT_MAX;index++)
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
        case SAMPLE_OID_A:
        {
            uri->objectId = SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_intValue;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);

            uri->objectId = SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_boolValue;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);

            uri->objectId = SAMPLE_OID_A;
            uri->instanceId = 0;
            uri->resourceId = attributeA_stringValue;
            cis_uri_update(uri);
            cis_response(context,uri,NULL,mid,CIS_RESPONSE_CONTINUE, PS_SOCK_RAI_NO_INFO);

        }
        break;
    }
    cis_response(context,NULL,NULL,mid,CIS_RESPONSE_DISCOVER, PS_SOCK_RAI_NO_INFO);
}


static void prvWriteResponse(void* context,cis_uri_t* uri,const cis_data_t* value,cis_attrcount_t count,cis_mid_t mid)
{

    uint8_t index;
    st_sample_object* object = NULL;
    

    if(!CIS_URI_IS_SET_INSTANCE(uri))
    {
        return;
    }

    for (index = 0;index < SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        return;
    }

    switch(object->oid)
    {
        case SAMPLE_OID_A:
        {
            if(uri->instanceId > SAMPLE_A_INSTANCE_COUNT){
                return;
            }
            st_instance_a *inst = &g_instList_a[uri->instanceId];
            if(inst == NULL || inst->enabled == false){
                return;
            }

            for (int i=0;i<count;i++)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, prvWriteResponse_1, P_INFO, 3, "prvWriteResponse:write %d/%d/%d", uri->objectId,uri->instanceId,value[i].id);
                switch(value[i].id)
                {
                case attributeA_intValue:
                    {
                        if(value[i].type == cis_data_type_string){
                            inst->instance.intValue = atoi((const char*)value[i].asBuffer.buffer);
                        }else{
                            inst->instance.intValue = value[i].value.asInteger;
                        }
                    }
                    break;
                case attributeA_boolValue:
                    {
                        if(value[i].type == cis_data_type_string){
                            inst->instance.boolValue = atoi((const char*)value[i].asBuffer.buffer);
                        }else{
                            inst->instance.boolValue = value[i].value.asBoolean;
                        }
                    }
                    break;
                case  attributeA_stringValue:
                    {
                        memset(inst->instance.strValue,0,sizeof(inst->instance.strValue));
                        strncpy(inst->instance.strValue,(char*)value[i].asBuffer.buffer,value[i].asBuffer.length);
                    }
                    break;
                }
            }
        }
        break;
    }

    cis_response(context,NULL,NULL,mid,CIS_RESPONSE_WRITE, PS_SOCK_RAI_NO_INFO);
}


static void prvExecResponse(void* context,cis_uri_t* uri,const uint8_t* value,uint32_t length,cis_mid_t mid)
{

    uint8_t index;
    st_sample_object* object = NULL;

    for (index = 0;index < SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        return;
    }


    switch(object->oid)
    {
    case SAMPLE_OID_A:
        {
            if(uri->instanceId > SAMPLE_A_INSTANCE_COUNT){
                return;
            }
            st_instance_a *inst = &g_instList_a[uri->instanceId];
            if(inst == NULL || inst->enabled == false){
                return;
            }

            if(uri->resourceId == actionA_1)
            {
                /*
                *\call action;
                */
                ECOMM_TRACE(UNILOG_PLA_APP, prvExecResponse_1, P_INFO, 0, "exec actionA_action");
                cis_response(context,NULL,NULL,mid,CIS_RESPONSE_EXECUTE, PS_SOCK_RAI_NO_INFO);
            }else{
                return;
            }
        }
        break;
    }
};


static void prvParamsResponse (void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    uint8_t index;
    st_sample_object* object = NULL;

    if(CIS_URI_IS_SET_RESOURCE(uri)){
        ECOMM_TRACE(UNILOG_PLA_APP, prvParamsResponse_1, P_INFO, 3, "prvParamsResponse (%d/%d/%d)", uri->objectId,uri->instanceId,uri->resourceId);
    }

    if(!CIS_URI_IS_SET_INSTANCE(uri))
    {
        return;
    }

    for (index = 0;index < SAMPLE_OBJECT_MAX;index++)
    {
        if(g_objectList[index].oid ==  uri->objectId){
            object = &g_objectList[index];
        }
    }

    if(object == NULL){
        return;
    }

    /*set parameter to observe resource*/
    ECOMM_TRACE(UNILOG_PLA_APP, prvParamsResponse_2, P_INFO, 2, "prvParamsResponse set:%x,clr:%x", parameters.toSet, parameters.toClear);
    ECOMM_TRACE(UNILOG_PLA_APP, prvParamsResponse_3, P_INFO, 5, "prvParamsResponse min:%d,max:%d,gt:%f,lt:%f,st:%f", parameters.minPeriod,parameters.maxPeriod,parameters.greaterThan,parameters.lessThan,parameters.step);

    cis_response(context,NULL,NULL,mid,CIS_RESPONSE_OBSERVE_PARAMS, PS_SOCK_RAI_NO_INFO);

}

static cis_data_t* prvDataDup(const cis_data_t* value,cis_attrcount_t attrcount)
{
    cis_attrcount_t index;
    cis_data_t* newData;
    newData =(cis_data_t*)cissys_malloc(attrcount * sizeof(cis_data_t));
    if(newData == NULL)
    {
        return NULL;
    }
    for (index =0;index < attrcount;index++)
    {
        newData[index].id = value[index].id;
        newData[index].type = value[index].type;
        newData[index].asBuffer.length = value[index].asBuffer.length;
        newData[index].asBuffer.buffer = (uint8_t*)cissys_malloc(value[index].asBuffer.length);
        memcpy(newData[index].asBuffer.buffer,value[index].asBuffer.buffer,value[index].asBuffer.length);
        memcpy(&newData[index].value.asInteger,&value[index].value.asInteger,sizeof(newData[index].value));
    }
    return newData;
}

static void prvUpdateObserveContext()
{
    struct st_observe_info* node = g_observeList;
    uint8_t i = 0;
    while(node != NULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, prvUpdateObserveContext_0, P_INFO, 1, "prvUpdateObserveContext mid=%d", node->mid);
        node = node->next;
        i++;
    }
    gOnenetContextRunning.observeObjNum = i;
    
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cisNvHeader nvHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NV_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, prvUpdateObserveContext_1, P_ERROR, 0, "can't open/create NVM: cisExample.nvm, save NVM failed");
        return;
    }

    /*
     * write the header
    */
    nvHdr.fileBodySize   = sizeof(onenet_context_t);
    nvHdr.version        = CIS_NV_FILE_VERSION;
    nvHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvHdr, sizeof(cisNvHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, prvUpdateObserveContext_2, P_ERROR, 0, "cisExample.nvm, write the file header failed");
        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    memcpy((uint8_t *)&(gOnenetContextRunning.gObservedBackup[0]), &(g_observed_backup[0]), MAX_OBSERVED_COUNT*sizeof(observed_backup_t));

    writeCount = OsaFwrite((void*)&gOnenetContextRunning, sizeof(onenet_context_t), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, prvUpdateObserveContext_3, P_ERROR, 0, "cisconfig.nvm, write the file body failed");
    }

    OsaFclose(fp);
    return;
}

static char* prvMakeDeviceName()
{
    char* name = NULL;
    name = (char*)malloc(NBSYS_IMEI_MAXLENGTH + NBSYS_IMSI_MAXLENGTH + 2);
    memset(name, 0, NBSYS_IMEI_MAXLENGTH + NBSYS_IMSI_MAXLENGTH + 2);
    uint8_t imei = cissys_getIMEI(name, NBSYS_IMEI_MAXLENGTH);
    *((char*)(name + imei)) = ';';
    uint8_t imsi = cissys_getIMSI(name + imei + 1, NBSYS_IMSI_MAXLENGTH, true);
    if (imei <= 0 || imsi <= 0 || strlen(name) <= 0)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, prvMakeDeviceName_0, P_INFO, 0, "ERROR:Get IMEI/IMSI ERROR.");
        return 0;
    }

    ECOMM_STRING(UNILOG_PLA_APP, prvMakeDeviceName_1, P_INFO, "get enderpoint=%s",(uint8_t*)name);

    return name;
}


void prvMakeUserdata()
{
    int i = 0;
    cis_instcount_t instIndex;
    cis_instcount_t instCount;
    for (i= 0;i < SAMPLE_OBJECT_MAX; i++)
    {
        st_sample_object* obj = &g_objectList[i];
        switch(i){
            case 0:
            {
                obj->oid = SAMPLE_OID_A;
                obj->instBitmap = SAMPLE_A_INSTANCE_BITMAP;
                instCount = SAMPLE_A_INSTANCE_COUNT;
                for (instIndex = 0;instIndex < instCount;instIndex++)
                {
                    if(obj->instBitmap[instIndex] != '1'){
                        g_instList_a[instIndex].instId = instIndex;
                        g_instList_a[instIndex].enabled = false;
                    }
                    else
                    {
                        g_instList_a[instIndex].instId = instIndex;
                        g_instList_a[instIndex].enabled = true;

                        g_instList_a[instIndex].instance.boolValue = true;
                        g_instList_a[instIndex].instance.intValue = cissys_rand() % 100;
                        strcpy(g_instList_a[instIndex].instance.strValue,"temp test");
                    }
                }
                obj->attrCount = sizeof(const_AttrIds_a) / sizeof(cis_rid_t);
                obj->attrListPtr = const_AttrIds_a;

                obj->actCount = 0;
                obj->actListPtr = NULL;
            }
            break;

        }
    }
}

//////////////////////////////////////////////////////////////////////////
cis_coapret_t cisAsynOnRead(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    struct st_callback_info* newNode = (struct st_callback_info*)cissys_malloc(sizeof(struct st_callback_info));
    newNode->next = NULL;
    newNode->flag = (et_callback_type_t)CIS_CALLBACK_READ;
    newNode->mid = mid;
    memcpy(&(newNode->uri), uri, sizeof(cis_uri_t));
    
    g_callbackList = (struct st_callback_info*)CIS_LIST_ADD(g_callbackList,newNode);

    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnRead_1, P_SIG, 3, "cisAsynOnRead (%d/%d/%d)", uri->objectId,uri->instanceId,uri->resourceId);
    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t cisAsynOnDiscover(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    struct st_callback_info* newNode = (struct st_callback_info*)cissys_malloc(sizeof(struct st_callback_info));
    newNode->next = NULL;
    newNode->flag = (et_callback_type_t)CIS_CALLBACK_DISCOVER;
    newNode->mid = mid;
    memcpy(&(newNode->uri), uri, sizeof(cis_uri_t));
    
    g_callbackList = (struct st_callback_info*)CIS_LIST_ADD(g_callbackList,newNode);

    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnDiscover_1, P_SIG, 3, "cisAsynOnDiscover (%d/%d/%d)", uri->objectId,uri->instanceId,uri->resourceId);
    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t cisAsynOnWrite(void* context,cis_uri_t* uri,const cis_data_t* value,cis_attrcount_t attrcount,cis_mid_t mid)
{

    if(CIS_URI_IS_SET_RESOURCE(uri)){      
        ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnWrite_1, P_SIG, 3, "cisAsynOnWrite (%d/%d/%d)", uri->objectId,uri->instanceId,uri->resourceId);
    }
    else{     
        ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnWrite_2, P_SIG, 2, "cisAsynOnWrite (%d/%d)", uri->objectId,uri->instanceId);
    }

    struct st_callback_info* newNode = (struct st_callback_info*)cissys_malloc(sizeof(struct st_callback_info));
    newNode->next = NULL;
    newNode->flag = (et_callback_type_t)CIS_CALLBACK_WRITE;
    newNode->mid = mid;
    memcpy(&(newNode->uri), uri, sizeof(cis_uri_t));
    newNode->param.asWrite.count = attrcount;
    newNode->param.asWrite.value = prvDataDup(value,attrcount);
    g_callbackList = (struct st_callback_info*)CIS_LIST_ADD(g_callbackList,newNode);


    return CIS_CALLBACK_CONFORM;
   
}


cis_coapret_t cisAsynOnExec(void* context,cis_uri_t* uri,const uint8_t* value,uint32_t length,cis_mid_t mid)
{
    if(CIS_URI_IS_SET_RESOURCE(uri))
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnExec_1, P_SIG, 3, "cisAsynOnExec (%d/%d/%d)", uri->objectId,uri->instanceId,uri->resourceId);
    }
    else{
        return CIS_CALLBACK_METHOD_NOT_ALLOWED;
    }

    if(!CIS_URI_IS_SET_INSTANCE(uri))
    {
        return CIS_CALLBACK_BAD_REQUEST;
    }

    struct st_callback_info* newNode = (struct st_callback_info*)cissys_malloc(sizeof(struct st_callback_info));
    newNode->next = NULL;
    newNode->flag = (et_callback_type_t)CIS_CALLBACK_EXECUTE;
    newNode->mid = mid;
    memcpy(&(newNode->uri), uri, sizeof(cis_uri_t));
    newNode->param.asExec.buffer = (uint8_t*)cissys_malloc(length);
    newNode->param.asExec.length = length;
    memcpy(newNode->param.asExec.buffer,value,length);
    g_callbackList = (struct st_callback_info*)CIS_LIST_ADD(g_callbackList,newNode);


    return CIS_CALLBACK_CONFORM;
}


cis_coapret_t cisAsynOnObserve(void* context,cis_uri_t* uri,bool flag,cis_mid_t mid)
{

    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnObserve_1, P_SIG, 3, "cisAsynOnObserve mid:%d uri:(%d/%d/%d)",mid,uri->objectId,uri->instanceId,uri->resourceId);
    if(!CIS_URI_IS_SET_INSTANCE(uri))
    {
        return CIS_CALLBACK_BAD_REQUEST;
    }
    
    struct st_callback_info* newNode = (struct st_callback_info*)cissys_malloc(sizeof(struct st_callback_info));
    newNode->next = NULL;
    newNode->flag = (et_callback_type_t)CIS_CALLBACK_OBSERVE;
    newNode->mid = mid;
    memcpy(&(newNode->uri), uri, sizeof(cis_uri_t));
    newNode->param.asObserve.flag = flag;
    
    g_callbackList = (struct st_callback_info*)CIS_LIST_ADD(g_callbackList,newNode);

    return CIS_CALLBACK_CONFORM;
}


cis_coapret_t cisAsynOnParams(void* context,cis_uri_t* uri,cis_observe_attr_t parameters,cis_mid_t mid)
{
    if(CIS_URI_IS_SET_RESOURCE(uri)){
        ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnParams_0, P_SIG, 3, "cisAsynOnParams=(%d/%d/%d)", uri->objectId,uri->instanceId,uri->resourceId); 
    }
    
    if(!CIS_URI_IS_SET_INSTANCE(uri))
    {
        return CIS_CALLBACK_BAD_REQUEST;
    }

    struct st_callback_info* newNode = (struct st_callback_info*)cissys_malloc(sizeof(struct st_callback_info));
    newNode->next = NULL;
    newNode->flag = (et_callback_type_t)CIS_CALLBACK_SETPARAMS;
    newNode->mid = mid;
    memcpy(&(newNode->uri), uri, sizeof(cis_uri_t));
    newNode->param.asObserveParam.params = parameters;
    g_callbackList = (struct st_callback_info*)CIS_LIST_ADD(g_callbackList,newNode);

    return CIS_CALLBACK_CONFORM;
}



void cisAsynOnEvent(void* context,cis_evt_t eid,void* param)
{
    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_0, P_SIG, 1, "event = %d", eid); 
    switch(eid)
    {
        case CIS_EVENT_RESPONSE_FAILED:
        case CIS_EVENT_NOTIFY_FAILED:       
            gNotifyOngoing = false;
            ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_2, P_INFO, 1, "con notify or response failed mid:%d", (int32_t)param); 
            break;
        case CIS_EVENT_NOTIFY_SUCCESS:
            gNotifyOngoing = false;
            ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_2_1, P_INFO, 1, "con notify success ackid=%d", (int32_t)param); 
            break;
        case CIS_EVENT_NOTIFY_SEND:
            gNotifyOngoing = false;
            ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_2_2, P_INFO, 0, "non notify success send"); 
            break;
        case CIS_EVENT_UPDATE_NEED:
            ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_3, P_INFO, 1, "need to update,reserve time:%ds", (int32_t)param); 
            cis_update_reg(gCisContext,LIFETIME_INVALID,false, PS_SOCK_RAI_NO_INFO);
            break;
        case CIS_EVENT_REG_SUCCESS:
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_4, P_INFO, 1, "register success, will clean g_observeList"); 
                struct st_observe_info* delnode;
                while(g_observeList != NULL){
                    g_observeList =(struct st_observe_info *)CIS_LIST_RM((cis_list_t *)g_observeList,g_observeList->mid,(cis_list_t **)&delnode);
                    cissys_free(delnode);
                }
                cisSyncBackup();
                if (slpManDeepSlpTimerIsRunning(heartBeatTimerID) == FALSE)
                    slpManDeepSlpTimerStart(heartBeatTimerID, (g_lifetime*9/10)*1000); 
            }
            break;
        case CIS_EVENT_UPDATE_SUCCESS:
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_5, P_INFO, 0, "update success "); 
                if(g_needUpdate == 2){
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_5_1, P_INFO, 0, "deeptimer update success can go to sleep"); 
                    slpManPlatVoteEnableSleep(onenetASlpHandler, SLP_SLP2_STATE);
                }
            }
            break;
        case CIS_EVENT_UPDATE_FAILED:
        case CIS_EVENT_UPDATE_TIMEOUT:
            {
                static uint8_t retrytime = 0;
                if(retrytime < 3){
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_6_1, P_INFO, 2, "update result=%d, update again %d time", eid, retrytime); 
                    cis_update_reg(gCisContext,LIFETIME_INVALID,false, PS_SOCK_RAI_NO_INFO);
                }else{
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_6_2, P_INFO, 1, "update result=%d, go sleep register next wakeup", eid); 
                    cisSyncClear();
                    slpManPlatVoteEnableSleep(onenetASlpHandler, SLP_SLP2_STATE);
                }
            }
            break;
        case CIS_EVENT_OBSERVE_ADD:
        case CIS_EVENT_OBSERVE_CANCEL:
            {
                prvUpdateObserveContext();
            }
            break;
        case CIS_EVENT_FIRMWARE_DOWNLOADING:
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_7, P_INFO, 0, "FOTA:begin downloading"); 
                g_fotaState = ONENET_FOTA_BEGIN;
                slpManPlatVoteDisableSleep(onenetASlpHandler, SLP_SLP2_STATE);
            }
            break;
        case CIS_EVENT_FIRMWARE_UPDATING:
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_8, P_INFO, 0, "FOTA:begin updating"); 
                gOnenetContextRunning.fotaState = 1;//set  OTA state for reset
                cisSyncBackup();
            }
            break;
        case CIS_EVENT_FIRMWARE_DOWNLOAD_DISCONNECT:
            {
                ECOMM_TRACE(UNILOG_PLA_APP, cisAsynOnEvent_9, P_INFO, 0, "FOTA:update error"); 
                g_fotaState = ONENET_FOTA_FAIL;
                gOnenetContextRunning.fotaState = 0;
                cisSyncBackup();
                slpManPlatVoteEnableSleep(onenetASlpHandler, SLP_SLP2_STATE);
            }
            break;
        case CIS_EVENT_FIRMWARE_UPDATE_SUCCESS:
            {
                g_fotaState = ONENET_FOTA_SUCCESS;
            }
        break;
        case CIS_EVENT_FIRMWARE_UPDATE_FAILED:
            {
                g_fotaState = ONENET_FOTA_FAIL;
            }
        break;
        case CIS_EVENT_FIRMWARE_UPDATE_OVER:
            {
                ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_10, P_VALUE, 1, "FOTA OVER");
                gOnenetContextRunning.fotaState = 0;//clear OTA state
                cisSyncBackup();//save the FOTA state to flash
            }
            break;
            
        default:
            ECOMM_TRACE(UNILOG_ONENET, onenetEVENTCALLBACK_11, P_VALUE, 1, "get event:%d",eid);
            break;
    }
    
}

bool cisAsynGetShutdown(void)
{
    return g_shutdown;
}
void cisAsynSetShutdown(bool shutdown)
{
    g_shutdown = shutdown;
}

bool cisAsynGetNotifyStatus(void)
{
    return gNotifyOngoing;
}

uint8_t cisAsynGetFotaStatus(void)
{
    return g_fotaState;
}


static void cisAsyncProcessTask(void* lpParam)
{

    while(1)
    {
        struct st_callback_info* node;
        if(g_callbackList == NULL || g_shutdown == TRUE)
        {
            cissys_sleepms(1000);
            continue;
        }
        node = g_callbackList;
        
        switch (node->flag)
        {
            case 0:
                break;
            case CIS_CALLBACK_READ:
                {               
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_0, P_SIG, 0, "handle CIS_CALLBACK_READ");
                    cis_uri_t uriLocal;
                    uriLocal = node->uri;
                    prvReadResponse(gCisContext,&uriLocal,node->mid);
                }
                break;
            case CIS_CALLBACK_DISCOVER:
                {                    
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_1, P_SIG, 0, "handle CIS_CALLBACK_DISCOVER");
                    cis_uri_t uriLocal;
                    uriLocal = node->uri;
                    prvDiscoverResponse(gCisContext,&uriLocal,node->mid);
                }
                break;
            case CIS_CALLBACK_WRITE:
                {                   
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_2, P_SIG, 0, "handle CIS_CALLBACK_WRITE");
                    prvWriteResponse(gCisContext,&node->uri,node->param.asWrite.value,node->param.asWrite.count,node->mid);
                    cis_data_t* data = node->param.asWrite.value;
                    cis_attrcount_t count = node->param.asWrite.count;

                    for (int i=0;i<count;i++)
                    {
                        if(data[i].type == cis_data_type_string || data[i].type == cis_data_type_opaque)
                        {
                            if(data[i].asBuffer.buffer != NULL)
                                cissys_free(data[i].asBuffer.buffer);
                        }
                    }
                    cissys_free(data);
                }
                break;
            case CIS_CALLBACK_EXECUTE:
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_3, P_SIG, 0, "handle CIS_CALLBACK_EXECUTE");
                    prvExecResponse(gCisContext,&node->uri,node->param.asExec.buffer,node->param.asExec.length,node->mid);
                    cissys_free(node->param.asExec.buffer);
                }
                break;
            case CIS_CALLBACK_SETPARAMS:
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_4, P_SIG, 0, "handle CIS_CALLBACK_SETPARAMS");
                    //set parameters and notify
                    prvParamsResponse(gCisContext,&node->uri,node->param.asObserveParam.params,node->mid);
                }
                break;
            case CIS_CALLBACK_OBSERVE:
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_5, P_INFO, 2, "handle observe callback mid=%d flag=%d\r\n", node->mid, node->param.asObserve.flag);
                    if(node->param.asObserve.flag)
                    {
                        uint16_t count = 0;
                        struct st_observe_info* observe_new = (struct st_observe_info*)cissys_malloc(sizeof(struct st_observe_info));
                        observe_new->mid = node->mid;
                        memcpy(&observe_new->uri, &node->uri, sizeof(cis_uri_t));
                        observe_new->next = NULL;
                        // mid change every time once register
                        g_observeList = (struct st_observe_info*)cis_list_add((cis_list_t*)g_observeList,(cis_list_t*)observe_new);
                        
                        cis_response(gCisContext,NULL,NULL,node->mid,CIS_RESPONSE_OBSERVE, PS_SOCK_RAI_NO_INFO);

                        ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_5_1, P_INFO, 4, "handle CIS_CALLBACK_OBSERVE set(%d): %d/%d/%d",
                                        count,
                                        observe_new->uri.objectId,
                                        CIS_URI_IS_SET_INSTANCE(&observe_new->uri)?observe_new->uri.instanceId:-1,
                                        CIS_URI_IS_SET_RESOURCE(&observe_new->uri)?observe_new->uri.resourceId:-1);

                    }
                    else
                    {
                        struct st_observe_info* delnode = g_observeList;

                        while (delnode) {
                            if (node->uri.flag == delnode->uri.flag && node->uri.objectId == delnode->uri.objectId) {
                                if (node->uri.instanceId == delnode->uri.instanceId) {
                                    if (node->uri.resourceId == delnode->uri.resourceId) {
                                        break;
                                    }
                                }
                            }
                            delnode = delnode->next;
                        }
                        if (delnode != NULL) {
                            g_observeList = (struct st_observe_info *)cis_list_remove((cis_list_t *)g_observeList, delnode->mid, (cis_list_t **)&delnode);

                            ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_5_2, P_INFO, 3, "cis_on_observe cancel: %d/%d/%d\n",
                                        delnode->uri.objectId,
                                        CIS_URI_IS_SET_INSTANCE(&delnode->uri) ? delnode->uri.instanceId : -1,
                                        CIS_URI_IS_SET_RESOURCE(&delnode->uri) ? delnode->uri.resourceId : -1);

                            cis_free(delnode);
                            cis_response(gCisContext, NULL, NULL, node->mid, CIS_RESPONSE_OBSERVE, PS_SOCK_RAI_NO_INFO);
                        }
                        else {
                            return;
                        }
                    }                    
                }
                break;
            default:
                break;
        }
        
        g_callbackList = (struct st_callback_info*)CIS_LIST_RM(g_callbackList,node->mid,NULL);
        cissys_free(node);
        ECOMM_TRACE(UNILOG_PLA_APP, cisAsyncProcessTask_6, P_INFO, 1, "g_callbackList = %x", g_callbackList);
    }
}


void cisAsyncUpdatePumpState(et_client_state_t state)
{
    if(gOnenetContextRunning.pumpState != state)
        gOnenetContextRunning.pumpState = state;
}

void cisAsyncAddObject()
{
    int index = 0;
    for (index= 0;index < SAMPLE_OBJECT_MAX ; index++)
    {
        cis_inst_bitmap_t bitmap;
        cis_res_count_t  rescount;
        cis_instcount_t instCount,instBytes;
        const char* instAsciiPtr;
        uint8_t * instPtr;
        cis_oid_t oid;
        int16_t i;
        st_sample_object* obj = &g_objectList[index];

        oid = obj->oid;
        instCount = strlen(obj->instBitmap);
        instBytes = (instCount - 1) / 8 + 1;
        instAsciiPtr = obj->instBitmap;

        instPtr = (uint8_t*)cissys_malloc(instBytes);
        memset(instPtr,0,instBytes);
        
        for (i = 0;i < instCount;i++)
        {
            cis_instcount_t instBytePos = i / 8;
            cis_instcount_t instByteOffset = 7 - (i % 8);
            if(instAsciiPtr[i] == '1'){
                instPtr[instBytePos] += 0x01 << instByteOffset;
            }
        }

        bitmap.instanceCount = instCount;
        bitmap.instanceBitmap = instPtr;
        bitmap.instanceBytes = instBytes;

        rescount.attrCount = obj->attrCount;
        rescount.actCount = obj->actCount;

        cis_addobject(gCisContext,oid,&bitmap,&rescount);
        cissys_free(instPtr);
    }
}

void cisAsyncRegister()
{
    cis_callback_t callback;
    callback.onRead = cisAsynOnRead;
    callback.onWrite = cisAsynOnWrite;
    callback.onExec = cisAsynOnExec;
    callback.onObserve = cisAsynOnObserve;
    callback.onSetParams = cisAsynOnParams;
    callback.onEvent = cisAsynOnEvent;
    callback.onDiscover = cisAsynOnDiscover;
    cis_register(gCisContext,g_lifetime,&callback);
}

bool cisAsyncCheckNotificationReady()
{
    if (gCisContext == NULL)
        return FALSE;
    else
        return ((((st_context_t*)gCisContext)->stateStep == PUMP_STATE_READY) &&
                (((st_context_t*)gCisContext)->observedList != NULL));
}

void cisSyncBackup(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cisNvHeader nvHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NV_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncBackup_1, P_ERROR, 0, "can't open/create NVM: cisExample.nvm, save NVM failed");
        return;
    }

    /*
     * write the header
    */
    nvHdr.fileBodySize   = sizeof(onenet_context_t);
    nvHdr.version        = CIS_NV_FILE_VERSION;
    nvHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvHdr, sizeof(cisNvHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncBackup_2, P_ERROR, 0, "cisExample.nvm, write the file header failed");
        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    if(gCisContext != NULL){
        memcpy(&gOnenetContextRunning.host[0], ((st_context_t*)gCisContext)->hostRestore, 16 );
    }
    memcpy((uint8_t *)&(gOnenetContextRunning.gObservedBackup[0]), &(g_observed_backup[0]), MAX_OBSERVED_COUNT*sizeof(observed_backup_t));

    writeCount = OsaFwrite((void*)&gOnenetContextRunning, sizeof(onenet_context_t), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncBackup_3, P_ERROR, 0, "cisconfig.nvm, write the file body failed");
    }

    OsaFclose(fp);
    return;
}

INT8 cisSyncRestore(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    cisNvHeader    nvmHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NV_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncRestore_1, P_INFO, 0, "can't open NVM: cisExample.nvm, use the defult value");
        return -1;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&nvmHdr, sizeof(cisNvHeader), 1, fp);
    if (readCount != 1 ||
        nvmHdr.fileBodySize != sizeof(onenet_context_t))
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncRestore_2, P_ERROR, 2,
                    "cisExample.nvm, can't read header, or header not right, (%u/%u), use the defult value", nvmHdr.fileBodySize, sizeof(onenet_context_t));
        OsaFclose(fp);
        return -2;
    }

    /*
     * read the file body retention context
    */
    readCount = OsaFread((void*)&gOnenetContextRunning, sizeof(onenet_context_t), 1, fp);

    if (readCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncRestore_3, P_ERROR, 0, "cisExample.nvm, can't read body");
        OsaFclose(fp);
        return -3;
    }
    OsaFclose(fp);
    
    // Restore for cis core using
    memcpy((uint8_t *)(&(g_observed_backup[0])), (uint8_t *)&(gOnenetContextRunning.gObservedBackup[0]),  MAX_OBSERVED_COUNT*sizeof(observed_backup_t));
    // Restore for cis app using
    ECOMM_TRACE(UNILOG_PLA_APP, cisSyncRestore_4, P_INFO, 1, "observeObjNum =%d", gOnenetContextRunning.observeObjNum);
    for (int i = 0; i < gOnenetContextRunning.observeObjNum; i++)
    {
        struct st_observe_info* observe_new = (struct st_observe_info*)cissys_malloc(sizeof(struct st_observe_info));
        memset((uint8_t *)observe_new, 0, sizeof(struct st_observe_info));
        observe_new->mid = g_observed_backup[i].msgid;
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncRestore_5, P_INFO, 1, "mid=%d", observe_new->mid);
        cis_memcpy(&(observe_new->uri), &(g_observed_backup[i].uri), sizeof(st_uri_t));
        cis_memcpy(&(observe_new->params), &(g_observed_backup[i].params), sizeof(cis_observe_attr_t));
        observe_new->next = NULL;
        g_observeList = (struct st_observe_info*)cis_list_add((cis_list_t*)g_observeList,(cis_list_t*)observe_new);
    }
    
    return 0;
}

void cisSyncClear(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    cisNvHeader nvHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(CIS_NV_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncClear_1, P_ERROR, 0, "can't open/create NVM: cisExample.nvm, save NVM failed");
        return;
    }

    /*
     * write the header
    */
    nvHdr.fileBodySize   = sizeof(onenet_context_t);
    nvHdr.version        = CIS_NV_FILE_VERSION;
    nvHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvHdr, sizeof(cisNvHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncClear_2, P_ERROR, 0, "cisExample.nvm, write the file header failed");
        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    memset((uint8_t *)&(gOnenetContextRunning), 0, sizeof(onenet_context_t));

    writeCount = OsaFwrite((void*)&gOnenetContextRunning, sizeof(onenet_context_t), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisSyncClear_3, P_ERROR, 0, "cisconfig.nvm, write the file body failed");
    }

    OsaFclose(fp);
    return;
}


void cisDataObserveReport()
{
    uint32_t nowtime;
    /*data observe data report*/
    nowtime = cissys_gettime();
    struct st_observe_info* node = g_observeList;
    if (node == NULL)
        return;
    ECOMM_TRACE(UNILOG_PLA_APP, cisDataObserveReport_0, P_INFO, 2, "g_notifyLast=%d nowtime=%d", g_notifyLast, nowtime);
    // if diff time is more than 120S, continue to allow to report
    g_notifyLast = nowtime;
    while(node != NULL)
    {
         if(node->mid == 0)
         {
            ECOMM_TRACE(UNILOG_PLA_APP, cisDataObserveReport_1, P_INFO, 0, "mid = 0");
            node = node->next;
            continue;
         }
         if(node->uri.flag == 0)
         {
            ECOMM_TRACE(UNILOG_PLA_APP, cisDataObserveReport_2, P_INFO, 0, "uri.flag = 0");
            node = node->next;
            continue;
         }
         cis_uri_t uriLocal;
         memcpy(&uriLocal, &(node->uri), sizeof(cis_uri_t));
         ECOMM_TRACE(UNILOG_PLA_APP, cisDataObserveReport_3, P_INFO, 0, "to notify this node");
         prvObserveNotify(gCisContext,&uriLocal,node->mid);
         node = node->next;
    }
}

static BOOL ifNeedRestoreOnenetTask(void)
{
    INT8 ret=0;
    BOOL needRestoreTask = FALSE;

    ret = cisSyncRestore();
    if(ret<0){//read file failed cant restore task
        ECOMM_TRACE(UNILOG_PLA_APP, ifNeedRestoreOnenetTask_1, P_INFO, 0, "read file failed start new task");
        return needRestoreTask;
    }
    if(gOnenetContextRunning.pumpState == PUMP_STATE_READY){
        ECOMM_TRACE(UNILOG_PLA_APP, ifNeedRestoreOnenetTask_2, P_INFO, 0, "before has ONENET connected, need restore task");
        needRestoreTask = TRUE;
    }

    return needRestoreTask;
}

static INT32 onenetPSUrcCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    NmAtiNetifInfo *netif = NULL;
    switch(eventID)
    {
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, onenetPSUrcCallback_1, P_INFO, 0, "PSIF network active change statemachine to DM_IPREADY_STATE");
                stateMachine = ONENET_IPREADY_STATE;
            }
            else if (netif->netStatus == NM_NETIF_OOS)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, onenetPSUrcCallback_2, P_INFO, 0, "PSIF network OOS");
            }
            else if (netif->netStatus == NM_NO_NETIF_OR_DEACTIVATED ||
                     netif->netStatus == NM_NO_NETIF_NOT_DIAL)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, onenetPSUrcCallback_3, P_INFO, 0, "PSIF network deactive");
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
            ECOMM_TRACE(UNILOG_PLA_APP, onenetPSUrcCallback_4, P_INFO, 1, "celevel=%d", celevel);
            break;
        }
        default:break;
    }
    return 0;
}

static BOOL onenetCheckNetworkReady(UINT8 waitTime)
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
            ECOMM_TRACE(UNILOG_PLA_APP, checkNetworkReady_1, P_INFO, 0, "network ready");
            network = 1;
            break;
        }
        cissys_sleepms(500);
        end = cissys_gettime()/osKernelGetTickFreq();
    }
    if(network == 0){
        ECOMM_TRACE(UNILOG_PLA_APP, checkNetworkReady_2, P_INFO, 0, "no network ");
        result = FALSE;
    }
    return result;

}



void cisProcessEntry(void *arg)
{
    if(cis_init(&gCisContext,(void *)config_hex,sizeof(config_hex),NULL, defaultLocalPort) != CIS_RET_OK){
        ECOMM_TRACE(UNILOG_PLA_APP, cisProcessEntry_0, P_ERROR, 0, "cis entry init failed.");
        LOGD("cis entry init failed.\n");
        if (gCisContext != NULL)
            cis_deinit(&gCisContext);

        return;
    }
    
    st_context_t* cisContext = gCisContext;
    if(cisContext->endpointName == NULL){
        cisContext->endpointName = prvMakeDeviceName();
        ECOMM_STRING(UNILOG_PLA_APP, cisProcessEntry_1, P_VALUE, "endpoint = %s", (uint8_t *)cisContext->endpointName);
    }
    
    cisContext->regTimeout = 120;//set register timeout 120 seconds
    
    prvMakeUserdata();
    
    if (needRestoreOnenetTask)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisProcessEntry_2, P_INFO, 1, "wakeup from hib continue pumpState=%d", gOnenetContextRunning.pumpState);
        ((st_context_t*)gCisContext)->ignoreRegistration = true;
        ((st_context_t*)gCisContext)->stateStep = PUMP_STATE_CONNECTING;
        memcpy(((st_context_t*)gCisContext)->hostRestore, &gOnenetContextRunning.host[0], 16);
        if (gOnenetContextRunning.fotaState) 
        {
            ((st_context_t*)gCisContext)->fotaNeedUpdate = 1;
            ECOMM_TRACE(UNILOG_PLA_APP, cisProcessEntry_2_1, P_INFO, 0, "need fota after reset");
        }
        cisAsyncUpdatePumpState(((st_context_t*)gCisContext)->stateStep);
        observe_read_retention_data(gCisContext);    
    }
    cisAsyncAddObject();
    cisAsyncRegister();
    
    g_shutdown = false;
    xTaskCreate(cisAsyncProcessTask,"cisAsyncProc", 512, NULL, osPriorityBelowNormal7, NULL);
    while(1)
    {
        if (g_shutdown)
        {
            cissys_sleepms(3000);
            continue;
        }
           
        /*pump function*/
        cis_pump(gCisContext);
        cisAsyncUpdatePumpState(((st_context_t*)gCisContext)->stateStep);
        cissys_sleepms(200);
        /*update lifetime*/
        if(g_needUpdate == 1 && ((st_context_t*)gCisContext)->stateStep == PUMP_STATE_READY){
            g_needUpdate = 2;
            int ret = cis_update_reg(gCisContext,LIFETIME_INVALID,false, PS_SOCK_ONLY_DL_FOLLOWED);
            if(ret != 0){
                ECOMM_TRACE(UNILOG_PLA_APP, cisProcessEntry_3, P_INFO, 1, "cis_update_reg failed return %x ", ret);
            }
        }
    }
}

static void onenetConnectTask(void *arg)
{
    while (1)
    {
        char logbuf[64] = {0};
        snprintf(logbuf,64,"%s",ONENET_MACHINESTATE(stateMachine));
        ECOMM_STRING(UNILOG_PLA_APP, onenetConnectTask_0, P_INFO, "handle stateMachine:%s", (uint8_t *)logbuf);
        switch(stateMachine)
        {
           case ONENET_INIT_STATE:
                osDelay(500/portTICK_PERIOD_MS);
                break;
           case ONENET_HEART_BEAT_UPDATE://to heartbeat
           case ONENET_IPREADY_STATE://poweron to register
           case ONENET_RESTORE_STATE://wakeup from deep sleep to restore client's context
           {
                ECOMM_TRACE(UNILOG_PLA_APP, onenetConnectTask_1, P_INFO, 0, "start dmTask for register/heartbeat/restore");
                osThreadAttr_t task_attr;
                memset(&task_attr, 0, sizeof(task_attr));
                task_attr.name = "dmtask";
                task_attr.stack_mem = NULL;
                task_attr.stack_size = ONENET_TASK_STACK_SIZE;
                task_attr.priority = osPriorityBelowNormal7;
                task_attr.cb_mem = NULL;
                task_attr.cb_size = 0;
                
                osThreadNew(cisProcessEntry, NULL, &task_attr);
                
                stateMachine = ONENET_IDLE_STATE;
                
                break;
           }
           case ONENET_HEART_BEAT_ARRIVED:
           {
                // wait 120s for network
                if(onenetCheckNetworkReady(120) != TRUE)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, onenetConnectTask_2_1, P_ERROR, 0, "app update network connect error enable sleep go idle");
                    slpManPlatVoteEnableSleep(onenetASlpHandler, SLP_SLP2_STATE);
                    stateMachine = ONENET_IDLE_STATE;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, onenetConnectTask_2, P_INFO, 0, "network ok send heartbeat");
                    stateMachine = ONENET_HEART_BEAT_UPDATE;
                }
                break;
           }
           case  ONENET_IDLE_STATE://nothing to do can handle key event
           {
               osDelay(10000/portTICK_PERIOD_MS);
               break;
           }
           default:
           {
               osDelay(10000/portTICK_PERIOD_MS);
               break;
           }

        }

    }
}

void cisOnenetInit()
{
    // Apply own right power policy according to design
    slpManSetPmuSleepMode(true, SLP_HIB_STATE, false);
    slpManApplyPlatVoteHandle("onenetA",&onenetASlpHandler);
    
    registerPSEventCallback(NB_GROUP_ALL_MASK, onenetPSUrcCallback);
    
    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(osThreadAttr_t));
    task_attr.name        = "cisAsyncTask";
    task_attr.stack_mem   = onenetTaskStack;
    task_attr.stack_size  = ONENET_TASK_STACK_SIZE;
    task_attr.priority    = osPriorityBelowNormal7;
    task_attr.cb_mem      = &onenetTask;
    task_attr.cb_size     = sizeof(StaticTask_t);
    memset(onenetTaskStack, 0xA5, ONENET_TASK_STACK_SIZE);
    onenetTaskId = osThreadNew(onenetConnectTask, NULL,&task_attr);
    
    needRestoreOnenetTask = ifNeedRestoreOnenetTask();

    slpManSlpState_t slpState = slpManGetLastSlpState();
    ECOMM_TRACE(UNILOG_PLA_APP, cisOnenetInit_1, P_INFO, 1, "slpState = %d",slpState);
    if(slpState == SLP_HIB_STATE || slpState == SLP_SLP2_STATE)//wakeup from deep sleep
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisOnenetInit_2, P_INFO, 0, "wakeup from deep sleep");
        stateMachine = ONENET_RESTORE_STATE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, cisOnenetInit_3, P_INFO, 0, "system reboot");
        if(needRestoreOnenetTask == TRUE && gOnenetContextRunning.fotaState != 1){
            ECOMM_TRACE(UNILOG_PLA_APP, cisOnenetInit_3_1, P_INFO, 0, "not in fota clear nv start from scratch");
            cisSyncClear();
            needRestoreOnenetTask = FALSE;
        }
        if(gOnenetContextRunning.fotaState == 1){
            g_fotaState = ONENET_FOTA_UPDATING;
        }
    }
}

void cisOnenetDeinit()
{
    osThreadTerminate(onenetTaskId);
    onenetTaskId = NULL;
}

void onenetDeepSleepCb(uint8 id)
{
    ECOMM_TRACE(UNILOG_PLA_APP, onenetDeepSleepCb_1, P_SIG, 1, "callback ID:%d come, disable slp2", id);
    slpManPlatVoteDisableSleep(onenetASlpHandler, SLP_SLP2_STATE);
    stateMachine = ONENET_HEART_BEAT_ARRIVED;
    g_needUpdate = 1;
    slpManDeepSlpTimerStart(heartBeatTimerID, (g_lifetime*9/10)*1000); 
}

void onenetDeepSleepTimerCbRegister(void)
{
    slpManDeepSlpTimerRegisterExpCb(onenetDeepSleepCb);
}


