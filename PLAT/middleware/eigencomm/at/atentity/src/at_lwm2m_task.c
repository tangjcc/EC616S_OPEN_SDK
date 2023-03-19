/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_lwm2m_task.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "ps_lib_api.h"

#include "at_lwm2m_task.h"
#include "cms_util.h"

#include "sockets.h"

#ifdef WITH_TINYDTLS_LWM2M
#include "shared/dtlsconnection.h"
#else
#include "shared/connection.h"
#endif

#ifdef WITH_TINYDTLS_LWM2M
#define LWM2M_TASK_STACK_SIZE 6144
#else
#define LWM2M_TASK_STACK_SIZE 4096
#endif

lwm2mClientContext gLwm2mClient[LWM2M_CLIENT_MAX_INSTANCE_NUM] = {
        {0},
};

UINT8 lwm2mTaskStart = 0;
osThreadId_t lwm2m_task_handle = NULL;

static lwm2m_client_state_t gLwm2mClientState = STATE_INITIAL;
lwm2m_object_t* lwm2m_objArray[LWM2M_CLIENT_MAX_INSTANCE_NUM][STAND_OBJ_COUNT];

static lwm2mRetentionContext gLwm2mRetainContext[LWM2M_CLIENT_MAX_INSTANCE_NUM] = {{0},};
static lwm2mObjectInfo     gLwm2mObjectInfo[LWM2M_CLIENT_MAX_INSTANCE_NUM][MAX_OBJECT_COUNT] = {0};

QueueHandle_t lwm2m_atcmd_handle = NULL;
osThreadId_t lwm2m_atcmd_task_handle = NULL;
int lwm2m_atcmd_task_flag = LWM2M_TASK_STAT_NONE;
osMutexId_t loopMutex = NULL;

uint8_t lwm2mSlpHandler = 0xff;
extern lwm2mObserveBack gLwm2mObserve[MAX_LWM2M_OBSERVE_NUM];

lwm2mClientContext* lwm2mGetFreeClient()
{
    for(INT32 i=0; i<LWM2M_CLIENT_MAX_INSTANCE_NUM; i++)
    {
        if(gLwm2mClient[i].isUsed == 0){
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mGetFreeClient_1, P_INFO, 1, "client %d is not used",i);
            if(gLwm2mClient[i].isQuit == 0){
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mGetFreeClient_2, P_INFO, 1, "and client %d not in quit",i);
                memset(&gLwm2mClient[i], 0, sizeof(lwm2mClientContext));
                gLwm2mClient[i].isUsed = 1;
                gLwm2mClient[i].lwm2mclientId= i;
                return &gLwm2mClient[i];
            }
        }
    }
    return NULL;
}

lwm2mClientContext* lwm2mFindClient(INT32 lwm2mClientId)
{
    for(INT32 i=0; i<LWM2M_CLIENT_MAX_INSTANCE_NUM; i++)
    {
        if(gLwm2mClient[i].lwm2mclientId == lwm2mClientId && gLwm2mClient[i].isUsed){
            return &gLwm2mClient[i];
        }
    }
    return NULL;
}

CHAR * lwm2mGetServerUri(lwm2m_object_t * objectP, UINT16 secObjInstID)
{
    security_instance_t * targetP = (security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        return lwm2m_strdup(targetP->uri);
    }

    return NULL;
}

UINT32 lwm2mParseAttr(const char *valueString, const char *buffer, const char *key[], char *attrList[], UINT32 attrMaxNum, const char *delim)
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



INT8 lwm2mAtcmdRetainSaveObject(UINT8 lwm2mId, UINT16 objectId, UINT16 instanceId, UINT16 resourceCount, UINT16 *resourceIds)
{
    INT8 ret = -1;
    for (INT32 i = 0; i < MAX_OBJECT_COUNT; i++) {
        lwm2mObjectInfo *objectInfo = &gLwm2mObjectInfo[lwm2mId][i];
        if (objectInfo->isUsed == 0) {
            objectInfo->isUsed = 1;
            objectInfo->objectId = objectId;
            objectInfo->instanceId = instanceId;
            if (resourceCount > MAX_RESOURCE_COUNT) {
                resourceCount = MAX_RESOURCE_COUNT;
            }
            objectInfo->resourceCount = resourceCount;
            memcpy(objectInfo->resourceIds, resourceIds, resourceCount * sizeof(UINT16));
            return 0;
        }
    }
    return ret;
}

void lwm2mAtcmdRetainDeleteObject(UINT8 lwm2mId, UINT16 objectId)
{
    for (INT32 i = 0; i < MAX_OBJECT_COUNT; i++) {
        lwm2mObjectInfo *objectInfo = &gLwm2mObjectInfo[lwm2mId][i];
        if (objectInfo->isUsed == 1 && objectInfo->objectId == objectId) {
            objectInfo->isUsed = 0;
            return;
        }
    }
}

void lwm2mAtcmdRetainDeleteAllObjects(UINT8 lwm2mId)
{
    for (INT32 i = 0; i < MAX_OBJECT_COUNT; i++) {
        lwm2mObjectInfo *objectInfo = &gLwm2mObjectInfo[lwm2mId][i];
        if (objectInfo->isUsed == 1) {
            objectInfo->isUsed = 0;
        }
    }
}

void lwm2mClearFile(void)
{
    OSAFILE fp = PNULL;
    lwm2mNvmHeader nvmHdr;   //4 bytes
    UINT32  writeCount = 0;
    lwm2mRetentionContext* retentCxt;
    for (int i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++) {
        retentCxt = &gLwm2mRetainContext[i];
        memset(retentCxt,0, sizeof(lwm2mRetentionContext));
        memset(&gLwm2mClient[i],0, sizeof(lwm2mClientContext));
        gLwm2mClientState = STATE_INITIAL;
    }
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mClearFlash_1, P_INFO, 0, "clear ctx save file begin");
    /*
     * open the NVM file
    */
    fp = OsaFopen(LWM2M_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mClearFlash_2, P_ERROR, 0,
                    "can't open/create NVM: 'lwm2mconfig.nvm', save NVM failed");

        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(lwm2mRetentionContext) * LWM2M_CLIENT_MAX_INSTANCE_NUM;;
    nvmHdr.version        = LWM2M_NVM_FILE_VERSION;
    nvmHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvmHdr, sizeof(lwm2mNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mClearFlash_3, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&gLwm2mRetainContext, sizeof(lwm2mRetentionContext), LWM2M_CLIENT_MAX_INSTANCE_NUM, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mClearFlash_4, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file body failed");
    }

    OsaFclose(fp);

    ECOMM_TRACE(UNILOG_LWM2M, lwm2mClearFlash_5, P_INFO, 0, "clear ctx save file end");
}

void lwm2mSaveFile(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    lwm2mNvmHeader nvmHdr;   //4 bytes
    lwm2mClientContext *lwm2m;
    lwm2mRetentionContext* retentCxt;
    for (int i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++) {
        lwm2m = &gLwm2mClient[i];
        retentCxt = &gLwm2mRetainContext[i];
        retentCxt->isUsed = lwm2m->isUsed;
        strncpy(retentCxt->server, lwm2m->server, MAX_SERVER_LEN);
        strncpy(retentCxt->enderpoint, lwm2m->enderpoint, MAX_NAME_LEN);
        if(lwm2m->psk != NULL)
        {
            strncpy(retentCxt->psk, lwm2m->psk, MAX_NAME_LEN);
        }
        else
        {
            memset(retentCxt->psk, 0, MAX_NAME_LEN);
        }
        if(lwm2m->pskId != NULL)
        {
            strncpy(retentCxt->pskId, lwm2m->pskId, MAX_NAME_LEN);
        }
        else
        {
            memset(retentCxt->pskId, 0, MAX_NAME_LEN);
        }
        memcpy(&retentCxt->objectInfo[0], gLwm2mObjectInfo[i], sizeof(lwm2mObjectInfo)*MAX_OBJECT_COUNT);
        retentCxt->serverPort = lwm2m->serverPort;
        retentCxt->localPort = lwm2m->localPort;
        retentCxt->lifetime = lwm2m->lifetime;
        retentCxt->lwm2mclientId = lwm2m->lwm2mclientId;
        retentCxt->lwm2mState = STATE_READY;//gLwm2mClientState;
        retentCxt->isConnected = TRUE;//lwm2m->isConnected;
        strncpy(retentCxt->location, lwm2m->context->location, LOCATION_LEN);
        retentCxt->reqhandle = lwm2m->reqHandle;
    }
    memcpy((uint8_t *)&(gLwm2mRetainContext[0].obsevInfo[0]), &(gLwm2mObserve[0]), MAX_LWM2M_OBSERVE_NUM*sizeof(lwm2mObserveBack));
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mSaveFlash_1, P_INFO, 2, "save objectInfo objid=%d, isUsed=%d",gLwm2mRetainContext[0].objectInfo[0].objectId, gLwm2mRetainContext[0].objectInfo[0].isUsed);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mSaveFlash_2, P_INFO, 2, "save to gRetention isConnected=%d gLwm2mClientState=%d",gLwm2mRetainContext[0].isConnected, gLwm2mClientState);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mSaveFlash_3, P_INFO, 1, "save flash begin, observe->resid=%d",gLwm2mRetainContext[0].obsevInfo[0].resourceId);
    ECOMM_STRING(UNILOG_LWM2M, lwm2mSaveFlash_4, P_INFO, "save location:(%s)", (UINT8*)retentCxt->location);
    if(strlen(lwm2m->pskId)>0)
        ECOMM_STRING(UNILOG_LWM2M, lwm2mSaveFlash_5, P_INFO, "save pskId:(%s)", (UINT8*)lwm2m->pskId);
    if(strlen(lwm2m->psk)>0)
        ECOMM_STRING(UNILOG_LWM2M, lwm2mSaveFlash_6, P_INFO, "save psk:(%s)", (UINT8*)lwm2m->psk);

    /*
     * open the NVM file
    */
    fp = OsaFopen(LWM2M_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mSaveFile_4, P_ERROR, 0,
                    "can't open/create NVM: 'lwm2mconfig.nvm', save NVM failed");

        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(gLwm2mRetainContext) * LWM2M_CLIENT_MAX_INSTANCE_NUM;
    nvmHdr.version        = LWM2M_NVM_FILE_VERSION;
    nvmHdr.checkSum       = 1;//TBD

    writeCount = OsaFwrite(&nvmHdr, sizeof(lwm2mNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mSaveFile_5, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&gLwm2mRetainContext, sizeof(gLwm2mRetainContext), LWM2M_CLIENT_MAX_INSTANCE_NUM, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mSaveFile_6, P_ERROR, 0,
                   "'lwm2mconfig.nvm', write the file body failed");
    }

    OsaFclose(fp);
    return;
}

INT8 lwm2mReadFile(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    lwm2mNvmHeader    nvmHdr;   //4 bytes
    lwm2mClientContext *lwm2m;
    lwm2mRetentionContext* retentCxt;

    /*
     * open the NVM file
    */
    fp = OsaFopen(LWM2M_NVM_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_1, P_INFO, 0,
                    "can't open NVM: 'lwm2mconfig.nvm', use the defult value");
        return -1;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&nvmHdr, sizeof(lwm2mNvmHeader), 1, fp);
    if (readCount != 1 ||
        nvmHdr.fileBodySize != LWM2M_CLIENT_MAX_INSTANCE_NUM * sizeof(lwm2mRetentionContext))
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_2, P_INFO, 2,
                    "'lwm2mconfig.nvm', can't read header, or header not right, (%u/%u), use the defult value",
                    nvmHdr.fileBodySize, LWM2M_CLIENT_MAX_INSTANCE_NUM * sizeof(lwm2mRetentionContext));

        OsaFclose(fp);

        return -2;
    }

    /*
     * read the file body retention context
    */
    readCount = OsaFread(&gLwm2mRetainContext, sizeof(lwm2mRetentionContext), LWM2M_CLIENT_MAX_INSTANCE_NUM, fp);

    OsaFclose(fp);

    if (readCount != LWM2M_CLIENT_MAX_INSTANCE_NUM)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_3, P_ERROR, 0,"'lwm2mconfig.nvm', can't read body");
        OsaFclose(fp);
        return -3;
    }

    // restore gLwm2mClient
    for (int i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++) {
        lwm2m = &gLwm2mClient[i];
        retentCxt = &gLwm2mRetainContext[i];
        lwm2m->isUsed = retentCxt->isUsed;
        lwm2m->isConnected = retentCxt->isConnected;
        lwm2m->lwm2mclientId = retentCxt->lwm2mclientId;
        gLwm2mClientState = (lwm2m_client_state_t)retentCxt->lwm2mState;
        lwm2m->lifetime = retentCxt->lifetime;
        lwm2m->server = malloc(strlen(retentCxt->server) + 1);
        strcpy(lwm2m->server,retentCxt->server );
        lwm2m->serverPort = retentCxt->serverPort;
        lwm2m->localPort =retentCxt->localPort;
        lwm2m->enderpoint = malloc(strlen(retentCxt->enderpoint) + 1);
        strcpy(lwm2m->enderpoint,retentCxt->enderpoint );
        memcpy(gLwm2mObjectInfo[i], &retentCxt->objectInfo[0], sizeof(lwm2mObjectInfo)*MAX_OBJECT_COUNT);
        lwm2m->location = malloc(strlen(retentCxt->location) + 1);
        strcpy(lwm2m->location,retentCxt->location );
        lwm2m->reqHandle = retentCxt->reqhandle;
#ifdef WITH_TINYDTLS_LWM2M
        if(strlen(retentCxt->pskId)>0)
        {
            lwm2m->pskId = malloc(strlen(retentCxt->pskId) + 1);
            strcpy(lwm2m->pskId,retentCxt->pskId );
        }
        lwm2m->pskLen = strlen(retentCxt->psk);
        if(lwm2m->pskLen > 0)
        {
            lwm2m->psk = malloc(lwm2m->pskLen + 1);
            strcpy(lwm2m->psk,retentCxt->psk );
        }
#endif
    }
    memcpy((uint8_t *)gLwm2mObserve, (uint8_t *)&(gLwm2mRetainContext[0].obsevInfo[0]),  MAX_LWM2M_OBSERVE_NUM*sizeof(lwm2mObserveBack));
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_4, P_INFO, 3, "lwm2m->isUsed=%d, ->isConnected=%d, objid=%d",lwm2m->isUsed, lwm2m->isConnected, retentCxt->objectInfo[0].objectId);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_5, P_INFO, 2, "gLwm2mClientState=%d,observe->resid=%d",gLwm2mClientState, gLwm2mObserve[0].resourceId);
    if(strlen(lwm2m->location) == 0){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_6, 1, P_INFO, "location is NULL");
    }else{
        ECOMM_STRING(UNILOG_LWM2M, lwm2mReadFile_7, P_INFO, "read location:(%s)", (UINT8*)lwm2m->location);
    }
    if(strlen(lwm2m->pskId) == 0){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_8, 1, P_INFO, "pskid is NULL");
    }else{
        ECOMM_STRING(UNILOG_LWM2M, lwm2mReadFile_9, P_INFO, "pskid:(%s)", (UINT8*)lwm2m->pskId);
    }
    if(strlen(lwm2m->psk) == 0){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mReadFile_10, 1, P_INFO, "psk is NULL");
    }else{
        ECOMM_STRING(UNILOG_LWM2M, lwm2mReadFile_11, P_INFO, "psk:(%s)", (UINT8*)lwm2m->psk);
    }
    return 0;
}

#if 0
static void lwm2mTimerCallback(void *user_data)
{
    gLwm2mNeedUpdate = true;
}
static void lwm2mStartTimer(lwm2mClientContext *lwm2m)
{
    lwm2mRetentionContext *lwm2mSaved = &gLwm2mRetainContext[lwm2m->lwm2mclientId];
    if (lwm2mSaved->rtc_enable == false) {
        lwm2mSaved->rtc_handle = osTimerNew(lwm2mTimerCallback, (osTimerType_t)1, (void *)lwm2mSaved, PNULL);
        osStatus_t status = osTimerStart(lwm2mSaved->rtc_handle, (lwm2m->lifetime-10)*1000);
        lwm2mSaved->rtc_enable = true;
    } else {
    }
}
static void lwm2mStopTimer(lwm2mClientContext *lwm2m)
{
    lwm2mRetentionContext *lwm2mSaved = &gLwm2mRetainContext[lwm2m->lwm2mclientId];
    if (lwm2mSaved->rtc_enable == true) {
        osStatus_t status = osTimerStop(lwm2mSaved->rtc_handle);
        status = osTimerDelete(lwm2mSaved->rtc_handle);
        lwm2mSaved->rtc_enable = false;
    } else {
    }
}
#endif

void lwm2mRemoveObject(lwm2m_object_t *obj)
{
    if (obj->objID == 0) {
       clean_security_object(obj);
       free(obj);
    } else if (obj->objID == 1) {
       clean_server_object(obj);
       free(obj);
    } else if (obj->objID == 3) {
       free_object_device(obj);
       //free(obj);
    } else {
        prv_instance_t *instanceP = NULL;
        prv_instance_t *old_instanceP = NULL;
        instanceP = (prv_instance_t *)obj->instanceList;
        while (instanceP != NULL) {
            old_instanceP = instanceP;
            instanceP = instanceP->next;
            free(old_instanceP->resourceIds);
            free(old_instanceP);
        }
        free(obj);
    }
}

lwm2m_object_t* lwm2mFindObject(lwm2m_context_t* contextP, UINT16 id)
{
    lwm2m_object_t* object = contextP->objectList;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mFindObject_1, P_INFO, 1, "to find obj:%d",id);
    while(object){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mFindObject_2, P_INFO, 1, "has obj:%d",object->objID);
        if(object->objID == id)
            return object;
        object = object->next;
    }
    return NULL;
}

void lwm2mRemoveClient(UINT8 id)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mRemoveClient_1, P_INFO, 0, "enter");
    for( INT32 i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++){
        lwm2mClientContext *lwm2m = lwm2mFindClient(i);
        if (lwm2m != NULL && lwm2m->lwm2mclientId == id) {
            lwm2m_context_t * lwm2mH = lwm2m->context;
            lwm2mClientData* data = NULL;
            lwm2m_object_t *object = NULL;
            lwm2m_object_t *old_object = NULL;

            lwm2m->isUsed = 0;
            //save to nv TBD
            //lwm2mStopTimer(lwm2m);
            lwm2mAtcmdRetainDeleteAllObjects(lwm2m->lwm2mclientId);

            // remove lwm2mClientData* data and lwm2mH
            if (lwm2mH) {
                data = (lwm2mClientData*)lwm2mH->userData;
                if(data){
                    object = lwm2mH->objectList;
                    while (object != NULL) {
                        old_object = object;
                        object = object->next;
                        lwm2mRemoveObject(old_object);
                    }
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mRemoveClient_5, P_INFO, 0, "to delete all list and close socket");
                    lwm2m_close(lwm2mH);
                    connection_free(data->connList);
                    close(data->socket);
                }
            }
            free(lwm2m->clientData);
            free(lwm2m->server);
            free(lwm2m->enderpoint);
            if(lwm2m->pskId){
               free(lwm2m->pskId);
            }
            if(lwm2m->psk){
                free(lwm2m->psk);
            }
            if (lwm2m->readWriteSemph != NULL) {
                vSemaphoreDelete(lwm2m->readWriteSemph);
            }
            memset(lwm2m, 0, sizeof(lwm2mClientContext));
            return;
        }
    }
}

void lwm2mSetQuitFlag(UINT8 type,UINT32 code)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mSetQuitFlag_1, P_INFO, 2, "type=%d, code=%d",type,code);
    for( INT32 i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++){
        lwm2mClientContext *lwm2m = lwm2mFindClient(i);
        lwm2m->isQuit = type;
        lwm2m->codeResaon = code;
    }
}

void lwm2mSendInd(void *indBody, UINT32 reqHandle)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mSendInd_1, P_INFO, 0, "send indication sig");

    UINT16 indBodyLen = strlen((CHAR*)indBody);
    if(indBodyLen == 0 || indBody == NULL)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mSendInd_2, P_ERROR, 2, "invalid indBodyLen %u, indBody 0x%x ", indBodyLen, indBody);
        return;
    }
    applSendCmsInd(reqHandle, APPL_LWM2M, APPL_LWM2M_IND, indBodyLen+1, indBody);

    return;

}

UINT8 lwm2mObserveCallback (UINT16 oper, UINT16 msgId, lwm2m_uri_t * uriP, void* userData)
{
    CHAR * indBuf = NULL;
    lwm2mClientContext * lwm2m = NULL;
    UINT8 ret = 0;

    lwm2m = (lwm2mClientContext*)userData;
    if (NULL == lwm2m)
        return COAP_404_NOT_FOUND;

    if (lwm2m->isUsed == 0)
        return COAP_404_NOT_FOUND;

    indBuf = malloc(100);

    if (uriP->flag & LWM2M_URI_FLAG_OBJECT_ID) {
        snprintf(indBuf, 100, "+LWM2MOBSERVE: %d,%d,%d,%d,%d", lwm2m->lwm2mclientId, oper,
                uriP->objectId,
                (uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID) ? uriP->instanceId : -1,
                (uriP->flag & LWM2M_URI_FLAG_RESOURCE_ID) ? uriP->resourceId : -1);

        lwm2mSendInd(indBuf,lwm2m->reqHandle);
        lwm2mSaveFile();
        ret = COAP_NO_ERROR;
    } else {
        ret = COAP_400_BAD_REQUEST;
    }

    if (indBuf) {
        free(indBuf);
    }
    return ret;
}

UINT8 lwm2mObjectDiscoverCallback (UINT16 instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP)
{
    UINT8 result;
    INT32 i, j;
    prv_instance_t *instanceP;

    result = COAP_205_CONTENT;
    instanceP = (prv_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, instanceId);

    ECOMM_TRACE(UNILOG_LWM2M, lwm2mObjectDiscoverCallback_1, P_INFO, 1, "lwm2mObjectDiscoverCallback numDataP = %d", *numDataP);
    if (instanceP != NULL) {
        // is the server asking for the full object ?
        if (*numDataP == 0)
        {
            INT32 nbRes = instanceP->resourceCount;

            *dataArrayP = lwm2m_data_new(nbRes);
            if (*dataArrayP == NULL) {
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            *numDataP = nbRes;
            for (i = 0; i < nbRes; i++)
            {
                (*dataArrayP)[i].id = instanceP->resourceIds[i];
            }
        }
        else
        {
            for (i = 0; i < *numDataP && result == COAP_205_CONTENT; i++)
            {
                result = COAP_404_NOT_FOUND;
                for (j = 0; j < instanceP->resourceCount; j++) {
                    if (instanceP->resourceIds[j] == (*dataArrayP)[i].id) {
                        result = COAP_205_CONTENT;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

UINT8 lwm2mParameterCallback (lwm2m_uri_t * uriP, lwm2m_attributes_t * attrP, void* userData)
{
    CHAR * indBuf = NULL;
    lwm2mClientContext * lwm2m = NULL;
    UINT8 ret = 0;

    lwm2m = (lwm2mClientContext*)userData;
    if (NULL == lwm2m)
        return COAP_404_NOT_FOUND;

    if (lwm2m->isUsed == 0)
        return COAP_404_NOT_FOUND;

    indBuf = malloc(150);

    if (uriP->flag & LWM2M_URI_FLAG_OBJECT_ID) {
        snprintf(indBuf, 150, "+LWM2MPARAMETER: %d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f",
                lwm2m->lwm2mclientId,
                uriP->objectId,
                (uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID) ? uriP->instanceId : -1,
                (uriP->flag & LWM2M_URI_FLAG_RESOURCE_ID) ? uriP->resourceId : -1,
                attrP->toSet, attrP->toClear, (INT32)attrP->minPeriod, (INT32)attrP->maxPeriod,
                attrP->greaterThan, attrP->lessThan, attrP->step);

        lwm2mSendInd(indBuf,lwm2m->reqHandle);
        ret = COAP_NO_ERROR;
    } else {
        ret = COAP_400_BAD_REQUEST;
    }

    if (indBuf) {
        free(indBuf);
    }
    return ret;
}

UINT8 lwm2mObjectReadCallback (UINT16 instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    CHAR* indBuf = NULL;
    lwm2mClientContext* lwm2m = NULL;
    UINT8 ret = 0;
    INT32 i, j;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    lwm2m = (lwm2mClientContext*)objectP->userData;
    if (NULL == lwm2m)
        return COAP_404_NOT_FOUND;

    if (lwm2m->isUsed == 0)
        return COAP_404_NOT_FOUND;

    if(*numDataP != 1){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mObjectReadCallback_1, P_INFO, 0, "AT not support multiple object instances read or whole object instance read yet");
        return COAP_404_NOT_FOUND;
    }

    indBuf = malloc(100);

    snprintf(indBuf, 100, "+LWM2MREAD: %d,%d,%d,%d", lwm2m->lwm2mclientId, objectP->objID, targetP->shortID, (*dataArrayP)->id);

    lwm2mSendInd(indBuf,lwm2m->reqHandle);

    if (indBuf) {
        free(indBuf);
    }

    if (xSemaphoreTake(lwm2m->readWriteSemph, READ_WRITE_TIMEOUT / portTICK_PERIOD_MS) == pdTRUE) {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mObjectReadCallback_2, P_INFO, 1, "lwm2mObjectReadCallback take readWriteSemph, %d", *numDataP);
        if (*numDataP > 0) {
            if (lwm2m->readResultCount == *numDataP) {
                for (i = 0; i < *numDataP; i++) {
                    for (j = 0; j < lwm2m->readResultCount; j++) {
                        if (((*dataArrayP) + i)->id == (lwm2m->readResultList + j)->id) {
                            memcpy((*dataArrayP) + i, lwm2m->readResultList + j, sizeof(lwm2m_data_t));
                            // because content is copied out, clean old
                            memset(lwm2m->readResultList + j, 0, sizeof(lwm2m_data_t));
                            break;
                        }
                    }
                    if (j == lwm2m->readResultCount) {
                        ECOMM_TRACE(UNILOG_LWM2M, lwm2mObjectReadCallback_3, P_INFO, 2, "No matched id for read i=%d, id=%d\n", i, ((*dataArrayP) + i)->id);
                    }
                }
            }
            lwm2m_data_free(lwm2m->readResultCount, lwm2m->readResultList);
        } else {
            *dataArrayP = lwm2m->readResultList;
            *numDataP = lwm2m->readResultCount;
        }
        lwm2m->readResultCount = 0;
        lwm2m->readResultList = NULL;
        ret = COAP_205_CONTENT;
    } else {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mObjectReadCallback_4, P_INFO, 0, "take lwm2m->readWriteSemph failed, timeout");
        ret = COAP_412_PRECONDITION_FAILED;
    }
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mObjectReadCallback_5, P_INFO, 0, "read has finished enable slp2");
    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);

    return ret;
}

UINT8 lwm2mObjectWriteCallback(UINT16 instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    CHAR* indBuf = NULL;
    CHAR *numStr = NULL;
    CHAR *opaqStr = NULL;
    INT32 pos = 0;
    lwm2mClientContext* lwm2m = NULL;
    INT32 i;
    UINT8 ret;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    lwm2m = (lwm2mClientContext*)objectP->userData;
    if (NULL == lwm2m)
        return COAP_404_NOT_FOUND;

    if (lwm2m->isUsed == 0)
        return COAP_404_NOT_FOUND;

    indBuf = malloc(IND_BUFFER);
    memset(indBuf, 0, IND_BUFFER);
    numStr = malloc(50);
    memset(numStr, 0, 50);

    snprintf(indBuf, IND_BUFFER, "+LWM2MWRITE: %d,%d,%d,%d", lwm2m->lwm2mclientId, objectP->objID, instanceId, numData);
    pos = strlen(indBuf);
    for (i = 0; i < numData; i++) {
        snprintf(indBuf + pos, IND_BUFFER - pos, ",%d", dataArray->id);
        pos += strlen(indBuf + pos);
        switch (dataArray->type) {
            case LWM2M_TYPE_STRING:
                snprintf(indBuf + pos, IND_BUFFER - pos, ",S,%d,\"%s\"",
                        dataArray->value.asBuffer.length,
                        dataArray->value.asBuffer.buffer);
                pos += strlen(indBuf + pos);
                break;
            case LWM2M_TYPE_OPAQUE:
                opaqStr = (CHAR *)malloc(dataArray->value.asBuffer.length * 2 + 1);
                memset(opaqStr, 0, dataArray->value.asBuffer.length * 2 + 1);
                cmsHexToHexStr(opaqStr, dataArray->value.asBuffer.length * 2 + 1, (UINT8*)dataArray->value.asBuffer.buffer, dataArray->value.asBuffer.length);
                snprintf(indBuf + pos, IND_BUFFER - pos, ",O,%d,\"%s\"",
                        dataArray->value.asBuffer.length,
                        opaqStr);
                pos += strlen(indBuf + pos);
                free(opaqStr);
                break;
            case LWM2M_TYPE_INTEGER:
                snprintf(numStr, 50, "%d", (INT32)dataArray->value.asInteger);
                snprintf(indBuf + pos, IND_BUFFER - pos, ",I,%d,%s",
                                            strlen(numStr),
                                            numStr);
                pos += strlen(indBuf + pos);
                break;
            case LWM2M_TYPE_FLOAT:
                snprintf(numStr, 50, "%f", dataArray->value.asFloat);
                snprintf(indBuf + pos, IND_BUFFER - pos, ",F,%d,%s",
                                            strlen(numStr),
                                            numStr);
                pos += strlen(indBuf + pos);
                break;
        }
    }

    lwm2mSendInd(indBuf,lwm2m->reqHandle);

    if (xSemaphoreTake(lwm2m->readWriteSemph, READ_WRITE_TIMEOUT / portTICK_PERIOD_MS) == pdTRUE) {
        ret = lwm2m->writeResult;
    } else {
        ret = COAP_412_PRECONDITION_FAILED;
    }

    if (indBuf) {
        free(indBuf);
    }
    if (numStr) {
        free(numStr);
    }
    return ret;
}

UINT8 lwm2mObjectExecuteCallback (UINT16 instanceId, UINT16 resourceId, UINT8 * buffer, int length, lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    CHAR* indBuf = NULL;
    CHAR *bufStr = NULL;
    INT32 pos = 0;
    lwm2mClientContext* lwm2m = NULL;
    UINT8 ret;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    lwm2m = (lwm2mClientContext*)objectP->userData;
    if (NULL == lwm2m)
        return COAP_404_NOT_FOUND;

    if (lwm2m->isUsed == 0)
        return COAP_404_NOT_FOUND;

    indBuf = malloc(IND_BUFFER);
    if (indBuf != NULL)
    {
        memset(indBuf, 0, IND_BUFFER);
    }
    if (indBuf == NULL) {
        ret = COAP_412_PRECONDITION_FAILED;
    } else {
        snprintf(indBuf, IND_BUFFER, "+LWM2MEXECUTE: %d,%d,%d,%d", lwm2m->lwm2mclientId,
                            objectP->objID, instanceId, resourceId);
        pos = strlen(indBuf);

        if (length > 0) {
            bufStr = malloc(length + 1);
            if(bufStr != NULL)
            {
                memset(bufStr,0,length+1);
            }
        }
        if (bufStr == NULL) {
            ret = COAP_412_PRECONDITION_FAILED;
        } else {
            memcpy((UINT8*)bufStr, (UINT8*)buffer, length);

            snprintf(indBuf + pos, IND_BUFFER - pos, ",%d,\"%s\"", length,
                    bufStr);
            pos = strlen(indBuf);

            lwm2mSendInd(indBuf,lwm2m->reqHandle);

            if (xSemaphoreTake(lwm2m->readWriteSemph, READ_WRITE_TIMEOUT / portTICK_PERIOD_MS) == pdTRUE) {
                ret = lwm2m->writeResult;
            } else {
                ret = COAP_412_PRECONDITION_FAILED;
            }
        }
    }

    if (indBuf) {
        free(indBuf);
    }
    if (bufStr) {
        free(bufStr);
    }
    return ret;
}

UINT8 lwm2mObjectCreateCallback (UINT16 instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP)
{
    return COAP_405_METHOD_NOT_ALLOWED;
}

UINT8 lwm2mObjectDeleteCallback (UINT16 instanceId, lwm2m_object_t * objectP)
{
    return COAP_405_METHOD_NOT_ALLOWED;
}

INT32 lwm2mAddobj(lwm2m_context_t* contextP, lwm2m_object_t* objectP, UINT8* resourceIds, UINT8 resourceCount, UINT8 lwm2mId, INT32 objectId, INT32 instanceId)
{
    prv_instance_t* tempInstanceP = NULL;
    prv_instance_t* instanceP = NULL;
    int result = COAP_NO_ERROR;
    CHAR* buffer = NULL;
    CHAR *resrList[MAX_RESOURCE_COUNT + 1];

    buffer = malloc(strlen((CHAR*)resourceIds)+10);
    UINT32 attrNum = lwm2mParseAttr((const CHAR*)resourceIds, buffer, NULL, resrList, MAX_RESOURCE_COUNT + 1, ";");
    if(attrNum != resourceCount)
    {
        result = COAP_UNKOWN_ERROR;
    }
    else
    {
        tempInstanceP = malloc(sizeof(prv_instance_t));
        tempInstanceP->shortID = instanceId;
        tempInstanceP->next = NULL;
        tempInstanceP->resourceIds = malloc(sizeof(UINT16) * resourceCount);
        for(UINT8 i=0; i<attrNum; i++)
        {
            *(tempInstanceP->resourceIds + i) = (UINT16)atoi(resrList[i]);
        }

        instanceP = (prv_instance_t*)lwm2m_list_find(objectP->instanceList, instanceId);
        if(instanceP == NULL){
            objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, tempInstanceP);
        }
        else
        {
            instanceP->resourceCount = tempInstanceP->resourceCount;
            free(instanceP->resourceIds);
            instanceP->resourceIds = tempInstanceP->resourceIds;
            free(tempInstanceP);
        }
        INT8 ret = lwm2mAtcmdRetainSaveObject(lwm2mId, objectId, instanceId, resourceCount,tempInstanceP->resourceIds);
        if(ret < 0)
            result = COAP_UNKOWN_ERROR;
    }

    if(buffer)
    {
        free(buffer);
    }

    return result;
}

INT32 addObjtoClient( lwm2mClientContext* lwm2m, lwm2m_object_t* objectP, INT32 objectId, BOOL updateReg)
{
    lwm2m_context_t* contextP = lwm2m->context;
    int result = COAP_NO_ERROR;
    objectP->readFunc = lwm2mObjectReadCallback;
    objectP->writeFunc = lwm2mObjectWriteCallback;
    objectP->executeFunc = lwm2mObjectExecuteCallback;
    objectP->createFunc = lwm2mObjectCreateCallback;
    objectP->deleteFunc = lwm2mObjectDeleteCallback;
    objectP->discoverFunc = lwm2mObjectDiscoverCallback;
    objectP->objID = objectId;
    objectP->userData = (void *)lwm2m;
    result = lwm2m_add_object_ec(contextP, objectP);
    ECOMM_TRACE(UNILOG_LWM2M, addObjtoClient_1, P_INFO, 2, "add object:%d to client result:%d", objectId, result);
    if(result == COAP_NO_ERROR)
    {
        if(updateReg)
        {
            ECOMM_TRACE(UNILOG_LWM2M, addObjtoClient_2, P_INFO, 1, P_INFO,"contextP->state:%d", contextP->state);
            if(contextP->state == STATE_READY)
            {
                ECOMM_TRACE(UNILOG_LWM2M, addObjtoClient_3, P_INFO, 0, P_INFO,"call lwm2m_update_registration");
                contextP->updateMode = 1;
                lwm2m_update_registration(contextP, 0, TRUE);
            }
            result = COAP_NO_ERROR;
        }
    }

    return result;
}


#ifdef WITH_TINYDTLS_LWM2M
void * lwm2mConnectServer(UINT16 secObjInstID, void * userData)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mConnectServer_1, P_INFO, 0, "enter");
    lwm2mClientData * dataP;
    lwm2m_list_t * instance;
    dtls_connection_t * newConnP = NULL;
    dataP = (lwm2mClientData *)userData;
    lwm2m_object_t  * securityObj = dataP->securityObject;

    instance = LWM2M_LIST_FIND(dataP->securityObject->instanceList, secObjInstID);
    if (instance == NULL) return NULL;

    newConnP = connection_create(dataP->connList, dataP->socket, securityObj, instance->id, dataP->lwm2mCxt, dataP->addressFamily);
    if (newConnP == NULL)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mConnectServer_2, P_INFO, 0, "Connection create failed.");
        return NULL;
    }
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mConnectServer_3, P_INFO, 0, "Connection create success.");

    dataP->connList = newConnP;
    return (void *)newConnP;
}
#else
void * lwm2mConnectServer(UINT16 secObjInstID, void * userData)
{
    lwm2mClientData * dataP;
    CHAR * uri;
    CHAR * host;
    CHAR * port;
    connection_t * newConnP = NULL;

    dataP = (lwm2mClientData *)userData;

    uri = lwm2mGetServerUri(dataP->securityObject, secObjInstID);
    if (uri == NULL) return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    if (0==strncmp(uri, "coaps://", strlen("coaps://"))) {
        host = uri+strlen("coaps://");
    }
    else if (0==strncmp(uri, "coap://",  strlen("coap://"))) {
        host = uri+strlen("coap://");
    }
    else {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL) goto exit;
    // remove brackets
    if (host[0] == '[')
    {
        host++;
        if (*(port - 1) == ']')
        {
            *(port - 1) = 0;
        }
        else goto exit;
    }
    // split strings
    *port = 0;
    port++;

    newConnP = connection_create(dataP->connList, dataP->socket, host, port, dataP->addressFamily);
    if (newConnP == NULL) {
    }
    else {
        dataP->connList = newConnP;
    }

exit:
    lwm2m_free(uri);
    return (void *)newConnP;
}
#endif

void lwm2mCloseConnection(void * sessionH, void * userData)
{
    lwm2mClientData * dataP;
#ifdef WITH_TINYDTLS_LWM2M
    dtls_connection_t * targetP;
#else
    connection_t * targetP;
#endif

    dataP = (lwm2mClientData *)userData;
#ifdef WITH_TINYDTLS_LWM2M
    targetP = (dtls_connection_t *)sessionH;
#else
    targetP = (connection_t *)sessionH;
#endif

    if (targetP == dataP->connList)
    {
        dataP->connList = targetP->next;
        lwm2m_free(targetP);
    }
    else
    {
#ifdef WITH_TINYDTLS_LWM2M
        dtls_connection_t * parentP;
#else
        connection_t * parentP;
#endif

        parentP = dataP->connList;
        while (parentP != NULL && parentP->next != targetP)
        {
            parentP = parentP->next;
        }
        if (parentP != NULL)
        {
            parentP->next = targetP->next;
            lwm2m_free(targetP);
        }
    }
}

void lwm2mNotifyCallback(lwm2m_notify_type_t type, lwm2m_notify_code_t code, UINT16 mid)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_0, P_INFO, 3, "lwm2mNotifyCallback type = %d, code = %d, mid = %d", (INT32)type, (INT32)code, (INT32)mid);
    UINT16 clientId = mid < LWM2M_CLIENT_MAX_INSTANCE_NUM ? mid : LWM2M_CLIENT_MAX_INSTANCE_NUM-1;
    lwm2m_cnf_msg cnfMsg;

    if (type == LWM2M_NOTIFY_TYPE_PING){
        if(code == LWM2M_NOTIFY_CODE_SUCCESS){
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_1, P_INFO, 0, "update register SUCCESS enable slp2");
            lwm2mSaveFile();
            if(mid == 0){
                applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_SUCC, APPL_LWM2M, APPL_LWM2M_DELOBJ_CNF, 0, PNULL);
            }else if(mid == 1){
                applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_SUCC, APPL_LWM2M, APPL_LWM2M_ADDOBJ_CNF, 0, PNULL);
            }else{
                applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_SUCC, APPL_LWM2M, APPL_LWM2M_UPDATE_CNF, 0, PNULL);
            }
        }else{
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_2, P_INFO, 0, "update register fail enable slp2");
            switch(code)
            {
                case LWM2M_NOTIFY_CODE_BAD_REQUEST:
                    cnfMsg.ret = LWM2M_REG_BAD_REQUEST;
                    break;
                case LWM2M_NOTIFY_CODE_FORBIDDEN:
                    cnfMsg.ret = LWM2M_REG_FORBIDEN;
                    break;
                case LWM2M_NOTIFY_CODE_PRECONDITION_FAILED:
                    cnfMsg.ret = LWM2M_REG_PRECONDITION;
                    break;
                case LWM2M_NOTIFY_CODE_TIMEOUT:
                    cnfMsg.ret = LWM2M_REG_TIMEOUT;
                    break;
                default:
                    cnfMsg.ret = LWM2M_REG_TIMEOUT;
                    break;
            }
            if(mid == 0){
                applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_DELOBJ_CNF, sizeof(cnfMsg), &cnfMsg);
            }else if(mid == 1){
                applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_ADDOBJ_CNF, sizeof(cnfMsg), &cnfMsg);
            }else{
                applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_UPDATE_CNF, sizeof(cnfMsg), &cnfMsg);
            }
        }
        //slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    }
    if(type == LWM2M_NOTIFY_TYPE_REG){
        if(code == LWM2M_NOTIFY_CODE_SUCCESS){
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_3, P_INFO, 0, "LWM2M_NOTIFY_TYPE_REG SUCCESS save flash and enable slp2");
            lwm2mSaveFile();
            cnfMsg.id = clientId;
            applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_SUCC, APPL_LWM2M, APPL_LWM2M_CREATE_CNF, sizeof(cnfMsg), &cnfMsg);
        }else{
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_3_1, P_INFO, 0, "LWM2M_NOTIFY_TYPE_REG fail enable slp2");
            lwm2mSetQuitFlag(2,(UINT32)code);
        }
        //slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    }
    if(type == LWM2M_NOTIFY_TYPE_DEREG){
        if(code == LWM2M_NOTIFY_CODE_SUCCESS){
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_4, P_INFO, 0, "LWM2M_NOTIFY_TYPE_DEREG SUCCESS continue destroy other things");
            lwm2mSetQuitFlag(1, (UINT32)code);
        }else{
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_4_1, P_INFO, 0, "LWM2M_NOTIFY_TYPE_DEREG NO SESSION delete AT cnf");
            cnfMsg.ret = LWM2M_SESSION_INVALID;
            applSendCmsCnf(gLwm2mClient[clientId].reqHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_DEL_CNF, sizeof(cnfMsg), &cnfMsg);
        }
    }
    if(type == LWM2M_NOTIFY_TYPE_SEND_CONFIRM){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotifyCallback_5, P_INFO, 1, "LWM2M_NOTIFY_TYPE_SEND_CONFIRM result:%d,enable slp2",code);
        slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    }
}

INT8 lwm2mCreate(lwm2mClientContext* lwm2m)
{
    lwm2mClientData *data;
    INT8 result;
    lwm2m_context_t* lwm2mCxt = NULL;
    CHAR localPort[6] = {0};

    data = lwm2m->clientData;
    memset(data, 0, sizeof(lwm2mClientData));
    data->addressFamily = lwm2m->addressFamily;
    sprintf(localPort, "%d", lwm2m->localPort);
    data->socket = create_socket(localPort, data->addressFamily);
    if(data->socket < 0)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mCreate_0, P_INFO, 0, "Failed to create socket");
        return -1;
    }
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCreate_0_1, P_INFO, 0, "get objects like security, server, device");
    CHAR serverUri[50] = {0};
    INT32 serverId = 123;
    if(lwm2m->psk){
        sprintf(serverUri, "coaps://%s:%d", lwm2m->server, lwm2m->serverPort);
    }else{
        sprintf(serverUri, "coap://%s:%d", lwm2m->server, lwm2m->serverPort);
    }
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCreate_0_2, P_INFO, 0, "call get_security_object");
    lwm2m_objArray[lwm2m->lwm2mclientId][0] = get_security_object(serverId, serverUri, lwm2m->pskId, lwm2m->psk, lwm2m->pskLen, false);
    if (NULL == lwm2m_objArray[lwm2m->lwm2mclientId][0])
    {
        return -1;
    }
    data->securityObject = lwm2m_objArray[lwm2m->lwm2mclientId][0];

    lwm2m_objArray[lwm2m->lwm2mclientId][1] = get_server_object(serverId, "U", lwm2m->lifetime, FALSE);
    if (NULL == lwm2m_objArray[lwm2m->lwm2mclientId][1])
    {
        return -1;
    }
    data->serverObject = lwm2m_objArray[lwm2m->lwm2mclientId][1];

    lwm2m_objArray[lwm2m->lwm2mclientId][2] = get_object_device();
    if (NULL == lwm2m_objArray[lwm2m->lwm2mclientId][2])
    {
        return -1;
    }

    lwm2mCxt = lwm2m_init(data);
    if(lwm2mCxt == NULL){
        return -1;
    }
    lwm2mCxt->clientId = lwm2m->lwm2mclientId;
    lwm2mCxt->state = gLwm2mClientState;
    lwm2mCxt->mode = CLIENT_MODE;
    lwm2m->context = lwm2mCxt;
#ifdef WITH_TINYDTLS_LWM2M
    data->lwm2mCxt = lwm2mCxt;
#endif


    lwm2m->context->observeCallback = lwm2mObserveCallback;
    lwm2m->context->observeUserdata = (void*)lwm2m;
    lwm2m->context->parameterCallback = lwm2mParameterCallback;
    lwm2m->context->connectServerCallback = lwm2mConnectServer;
    lwm2m->context->closeConnectionCallback = lwm2mCloseConnection;
    lwm2m->context->notifyCallback = lwm2mNotifyCallback;

    if(strlen(lwm2m->location) == LOCATION_LEN){
        lwm2m->context->location = lwm2m->location;
        ECOMM_STRING(UNILOG_LWM2M, lwm2mCreate_1, P_INFO, "restore contextP->location:(%s)", (UINT8*)lwm2m->context->location);
    }
    result = lwm2m_configure(lwm2mCxt, lwm2m->enderpoint, NULL, NULL, 3, lwm2m_objArray[lwm2m->lwm2mclientId]);

    if (result != 0)
    {
        lwm2m_close(lwm2mCxt);
        lwm2m->context = NULL;
        return -1;
    }

    ECOMM_STRING(UNILOG_LWM2M, lwm2mCreate_2, P_INFO, "LWM2M Client:%s ", (UINT8*)lwm2m->enderpoint);
    //lwm2mStartTimer(lwm2m);

    return 0;
}

INT32 checkNetworkReady(UINT8 waitTime)
{
    INT32 result = LWM2M_ERRID_OK;
    NmAtiSyncRet netStatus;
    UINT32 start, end;
    
    start = lwm2m_gettime()/osKernelGetTickFreq();
    end=start;
    // Check network for waitTime*2 seconds
    UINT8 network=0;
    while(end-start < waitTime)
    {
        appGetNetInfoSync(0, &netStatus);
        if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
        {
            ECOMM_TRACE(UNILOG_LWM2M, checkNetworkReady_1, P_INFO, 0, "network ready");
            network = 1;
            break;
        }
        osDelay(500);
        end = lwm2m_gettime()/osKernelGetTickFreq();
    }
    if(network == 0){
        ECOMM_TRACE(UNILOG_LWM2M, checkNetworkReady_2, P_INFO, 0, "no network return 655");
        result = LWM2M_NO_NETWORK;
    }
    return result;

}

static INT32 lwm2mInitClient()
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_1, P_INFO, 0, "to init lwm2m client");
    INT32 result=LWM2M_ERRID_OK;
    UINT8 needMaintask = 0;
    
    for (INT32 i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++) {
        lwm2mClientContext *lwm2m = &gLwm2mClient[i];
        if (lwm2m->isConnected) {
            needMaintask = 1;
            lwm2m->clientData = malloc(sizeof(lwm2mClientData));
            lwm2m->addressFamily = AF_INET;
            lwm2m->readWriteSemph = xSemaphoreCreateBinary();
            if (lwm2m->readWriteSemph == NULL) {
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_1_1, P_ERROR, 0, "lwm2m->readWriteSemph == NULL");
            }
            if (lwm2mCreate(lwm2m) != 0) {
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_1_2, P_ERROR, 0, "lwm2mcreate failed, remove client");
                lwm2mRemoveClient(lwm2m->lwm2mclientId);
                result = LWM2M_ERRID_CREATE_CLIENT_FAIL;
                return result;
            }
            for (int j = 0; j < MAX_OBJECT_COUNT; j++) {
                lwm2mObjectInfo *objectInfo = &gLwm2mRetainContext[lwm2m->lwm2mclientId].objectInfo[j];
                if (objectInfo->isUsed) {
                    lwm2m_object_t * objectP = NULL;
                    prv_instance_t * instanceP = NULL;
                    prv_instance_t * tempInstanceP = NULL;
                    objectP = lwm2mFindObject(lwm2m->context, objectInfo->objectId);
                    if(objectP == NULL){
                        objectP = malloc(sizeof(lwm2m_object_t));
                        memset(objectP,0,sizeof(lwm2m_object_t));
                        objectP->objID = objectInfo->objectId;
                    }
                    tempInstanceP = malloc(sizeof(prv_instance_t));
                    tempInstanceP->shortID = objectInfo->instanceId;
                    tempInstanceP->next = NULL;
                    tempInstanceP->resourceIds = malloc(sizeof(uint16_t) * objectInfo->resourceCount);
                    memcpy(tempInstanceP->resourceIds, objectInfo->resourceIds, sizeof(uint16_t) * objectInfo->resourceCount);

                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_2, P_INFO, 2, "prepare tempInstanceP instanceId:%d resourceId[0]:%d", tempInstanceP->shortID,tempInstanceP->resourceIds[0]);

                    instanceP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, objectInfo->instanceId);
                    if (instanceP == NULL) {
                        ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_3, P_INFO, 0, "no find this instacne add it");
                        objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, tempInstanceP);
                    } else {
                        instanceP->resourceCount = tempInstanceP->resourceCount;
                        instanceP->resourceIds = tempInstanceP->resourceIds;
                        free(tempInstanceP);
                    }
                    addObjtoClient(lwm2m, objectP, objectInfo->objectId, FALSE);

                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_4, P_INFO, 1, "has add saved objectId:%d", objectInfo->objectId);
                }
            }
        }
    }
    
    if(needMaintask)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mInitClinet_5, P_SIG, 0, "needMaintask, start the mainLoop");
        result = lwm2mCreateMainLoop();
    }
    else
    {
        result = LWM2M_ERRID_OK;
    }
    return result;
}

static void lwm2mMainLoop(void *arg)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_1, P_INFO, 0, "lwm2mMainLoop enter !!!");
    INT32 lwm2mClientNum = 0;
    lwm2m_cnf_msg cnfMsg;
    int primSize = sizeof(cnfMsg);
    INT32 reqhandle = 0;
    
    while (lwm2mTaskStart)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_2, P_INFO, 0, "lwm2mTaskStart");
        lwm2mClientNum = 0;
        for(INT32 i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++) {
            INT32 result;
            struct timeval tv;
            lwm2mClientContext* lwm2m = lwm2mFindClient(i);
            lwm2m_context_t * lwm2mH = NULL;
            fd_set readfds;
            lwm2mClientData * data = NULL;

            if (lwm2m == NULL){
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_3, P_INFO, 0, "lwm2m == NULL exit the lwm2mMainLoop");
                continue;
            }

            if(lwm2m->isQuit != 0){
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_3_1, P_INFO, 1, "lwm2m->isQuit=%d",lwm2m->isQuit);
                if(lwm2m->isQuit == 1){
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_3_2, P_INFO, 0, "has delete client call del cnf");
                    cnfMsg.id = i;
                    reqhandle = lwm2m->reqHandle;
                    lwm2mRemoveClient(i);
                    lwm2mClearFile();
                    applSendCmsCnf(reqhandle, APPL_RET_SUCC, APPL_LWM2M, APPL_LWM2M_DEL_CNF, primSize, &cnfMsg);
                }else if(lwm2m->isQuit == 2){
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_3_3, P_INFO, 0, "AT+MIPLCREATE register failed call create cnf");
                    memset(&cnfMsg, 0, primSize);
                    cnfMsg.id = i;
                    switch(lwm2m->codeResaon)
                    {
                        case LWM2M_NOTIFY_CODE_BAD_REQUEST:
                            cnfMsg.ret = LWM2M_REG_BAD_REQUEST;
                            break;
                        case LWM2M_NOTIFY_CODE_FORBIDDEN:
                            cnfMsg.ret = LWM2M_REG_FORBIDEN;
                            break;
                        case LWM2M_NOTIFY_CODE_PRECONDITION_FAILED:
                            cnfMsg.ret = LWM2M_REG_PRECONDITION;
                            break;
                        case LWM2M_NOTIFY_CODE_TIMEOUT:
                            cnfMsg.ret = LWM2M_REG_TIMEOUT;
                            break;
                        case LWM2M_NOTIFY_CODE_LWM2M_SESSION_INVALID:
                            cnfMsg.ret = LWM2M_SESSION_INVALID;
                            break;
                        default:
                            cnfMsg.ret = LWM2M_REG_TIMEOUT;
                            break;
                    }
                    reqhandle = lwm2m->reqHandle;
                    lwm2mRemoveClient(i);
                    applSendCmsCnf(reqhandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_CREATE_CNF, primSize, &cnfMsg);
                }
                continue;
            }

            lwm2mClientNum++;
            lwm2mH = lwm2m->context;

            if (lwm2mH == NULL) {
                break;
            }


            tv.tv_sec = 1;
            tv.tv_usec = 0;

            /*
             * This function does two things:
             *  - first it does the work needed by liblwm2m (eg. (re)sending some packets).
             *  - Secondly it adjusts the timeout value (default 60s) depending on the state of the transaction
             *    (eg. retransmission) and the time between the next operation
             */

            result = lwm2m_step(lwm2mH, (time_t*)&(tv.tv_sec));

            if (result != 0)
            {
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_4, P_INFO, 1, "lwm2m_step() failed: 0x%x", result);
            }

            gLwm2mClientState = lwm2mH->state;
            if(gLwm2mClientState == STATE_READY){
                lwm2m->isConnected = TRUE;
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_4_1, P_INFO, 1, "gLwm2mClientState:%d", gLwm2mClientState);
            }else{
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_4_2, P_INFO, 1, "gLwm2mClientState:%d", gLwm2mClientState);
                lwm2m->isConnected = FALSE;
            }
            data = (lwm2mClientData*)lwm2mH->userData;

            FD_ZERO(&readfds);
            FD_SET(data->socket, &readfds);

            /*
             * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
             * with the precedent function)
             */
            result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

            if (result < 0)
            {
                if (errno != EINTR)
                {
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_5, P_INFO, 1, "Error in select(): %d ", errno);
                }
            }
            else if (result > 0)
            {
                UINT8* buffer = (UINT8*)malloc(MAX_LWM2M_PACKET_SIZE);
                memset(buffer, 0, MAX_LWM2M_PACKET_SIZE);
                INT32 numBytes;
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_6, P_INFO, 0, "select return >0");

                /*
                 * If an event happens on the socket
                 */
                if (FD_ISSET(data->socket, &readfds))
                {
                    struct sockaddr_storage addr;
                    socklen_t addrLen;

                    addrLen = sizeof(addr);

                    /*
                     * We retrieve the data received
                     */
                    numBytes = recvfrom(data->socket, buffer, MAX_LWM2M_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                    if (0 > numBytes)
                    {
                        ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_7, P_INFO, 1, "Error in recvfrom(): %d ", errno);
                    }
                    else if (0 < numBytes)
                    {
                        CHAR s[INET6_ADDRSTRLEN];

                    #ifdef WITH_TINYDTLS_LWM2M
                        dtls_connection_t * connP;
                    #else
                        connection_t * connP;
                    #endif
                        if (AF_INET == addr.ss_family)
                        {
                            struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                            ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_8, P_INFO, 1, "AF_INET saddr = %x", (INT32)saddr->sin_addr.s_addr);

                            inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
                        }
                        else if (AF_INET6 == addr.ss_family)
                        {
                            struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&addr;
                            inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
                        }
                        ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_9, P_INFO, 1, "%d bytes received", numBytes);

                        connP = connection_find(data->connList, &addr, addrLen);

                        if (connP != NULL)
                        {
                            /*
                             * Let liblwm2m respond to the query depending on the context
                             */
                            lwm2m->connP = connP;
                        #ifdef WITH_TINYDTLS_LWM2M
                            INT32 result = connection_handle_packet(connP, buffer, numBytes);
                            ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_10, P_INFO, 1, "call connection_handle_packet return:%d ", result);
                        #else
                            lwm2m_handle_packet(lwm2mH, buffer, numBytes, connP);
                        #endif
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_11, P_INFO, 0, "no find connP can't handle recv data!!!");
                        }
                    }else{
                        ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_12, P_INFO, 0, "socket closed!!!");
                    }
                }
                free(buffer);
            }else{
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_13, P_INFO, 0, "select return 0");
            }
        }
        if (lwm2mClientNum == 0) {
            lwm2mTaskStart = 0;
            gLwm2mClientState = STATE_INITIAL;
        }
    }
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mMainLoop_14, P_INFO, 0, "no lwm2m client exist lwm2m_atcmd task deleted");

    vTaskDelete(NULL);

    return;
}

INT32 lwm2mCreateMainLoop()
{
    INT32 ret = LWM2M_ERRID_OK;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCreateMainLoop_1, P_INFO, 1, "lwm2mTaskStart = %d",lwm2mTaskStart);
    
    osStatus_t result = osMutexAcquire(loopMutex, osWaitForever);
    
    if(lwm2mTaskStart == 0)
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mCreateMainLoop_2, P_INFO, 0, "start lwm2m task");
        lwm2mTaskStart = 1;
        osThreadAttr_t task_attr;
        memset(&task_attr, 0, sizeof(task_attr));
        task_attr.name = "lwm2mloop";
        task_attr.stack_size = LWM2M_TASK_STACK_SIZE;
        task_attr.priority = osPriorityBelowNormal7;

        lwm2m_task_handle= osThreadNew(lwm2mMainLoop, NULL, &task_attr);
        if(lwm2m_task_handle == NULL)
        {
            ret = LWM2M_ERRID_CREATE_THREAD_FAIL;
        }
    }
    osMutexRelease(loopMutex);

    return ret;
}

CmsRetId lwm2mClientNotify(UINT32 reqHandle, UINT32 id, lwm2m_uri_t* pUri)
{
    LWM2M_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_LWM2M, onenet_client_notify_1, P_INFO, 0, "TO SEND AT+LWM2MNOTIFY");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_LWM2M_NOTIFY;
    msg.lwm2mId = id;
    msg.reqhandle = reqHandle;
    memcpy(&msg.uri, pUri, sizeof(lwm2m_uri_t));
    
    xQueueSend(lwm2m_atcmd_handle, &msg, LWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId lwm2mClientAddobj(UINT32 reqHandle, UINT32 id, lwm2mAddobjCmd* pCmd)
{
    LWM2M_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientAddobj_1, P_INFO, 0, "TO SEND AT+LWM2MADDOBJ");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_LWM2M_ADDOBJ;
    msg.lwm2mId = id;
    msg.reqhandle = reqHandle;
    msg.cmd = (void*)pCmd;
    
    xQueueSend(lwm2m_atcmd_handle, &msg, LWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId lwm2mClientDelobj(UINT32 reqHandle, UINT32 id, INT32 objId)
{
    LWM2M_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientDelobj_1, P_INFO, 0, "TO SEND AT+LWM2MDELOBJ");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_LWM2M_DELOBJ;
    msg.lwm2mId = id;
    msg.reqhandle = reqHandle;
    msg.objId = objId;
    
    xQueueSend(lwm2m_atcmd_handle, &msg, LWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId lwm2mClientUpdate(UINT32 reqHandle, UINT32 id, UINT8 withobj)
{
    LWM2M_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientUpdate_1, P_INFO, 0, "TO SEND AT+LWM2MUPDATE");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_LWM2M_UPDATE;
    msg.lwm2mId = id;
    msg.reqhandle = reqHandle;
    msg.withobj = withobj;
    
    xQueueSend(lwm2m_atcmd_handle, &msg, LWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

CmsRetId lwm2mClientDelete(UINT32 reqHandle, UINT32 id)
{
    LWM2M_ATCMD_Q_MSG msg;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientDelete_1, P_INFO, 0, "TO SEND AT+LWM2MDELETE");

    memset(&msg, 0, sizeof(msg));
    msg.cmd_type = MSG_LWM2M_DELETE;
    msg.lwm2mId = id;
    msg.reqhandle = reqHandle;
    
    xQueueSend(lwm2m_atcmd_handle, &msg, LWM2M_MSG_TIMEOUT);

    return CMS_RET_SUCC;
}

static INT32 lwm2mClientReady(UINT32 clientId)
{
    /*----------------------------------------------------------------*/
    lwm2mClientContext *lwm2m = lwm2mFindClient(clientId);
    if (lwm2m == NULL ||lwm2m->context == NULL){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientReady_1, P_INFO, 0, "context == NULL");
        return LWM2M_ERRID_NO_CONTEXT;
    }
    
    lwm2m_context_t* ctx = (lwm2m_context_t*)lwm2m->context;
    UINT8 waitTime = 0;
    while(ctx->state != STATE_READY && waitTime < 8){
        waitTime += 1;
        osDelay(500);
    }
    if(waitTime == 8){
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientReady_2, P_INFO, 0, "wait 4s statestep not ready");
        return LWM2M_ERRID_NO_NETWORK;
    }else{
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mClientReady_3, P_INFO, 0, "statestep ready");
        return LWM2M_ERRID_OK;
    }
}

extern void lwm2m_observe_clean_obj(lwm2m_context_t * contextP, uint16_t objId);

static void lwm2mAtcmdProcessing(void *argument)
{
    int keepTask = 1;
    INT32 ret = LWM2M_ERRID_OK;
    INT32 result = COAP_NO_ERROR;
    int msg_type = 0xff;
    LWM2M_ATCMD_Q_MSG msg;
    lwm2mAddobjCmd* pCmd;
    UINT16 atHandle;
    UINT32 lwm2mId;
    int primSize;
    BOOL hasLoop = FALSE;
    BOOL clientReady = FALSE;
    lwm2m_uri_t uri = {0};
    INT32 objectId,instanceId;
    UINT8 * resourceIds = NULL;
    UINT8 resourceCount;
    lwm2m_object_t* objectP = FALSE;
    lwm2m_object_t* obj = FALSE;
    lwm2m_cnf_msg cnfMsg;
    primSize = sizeof(cnfMsg);
    memset(&cnfMsg, 0, primSize);

    while(1)
    {
        /* recv msg (block mode) */
        xQueueReceive(lwm2m_atcmd_handle, &msg, osWaitForever);
        msg_type = msg.cmd_type;
        lwm2mId = msg.lwm2mId;
        atHandle = msg.reqhandle;
        lwm2mClientContext *pContext = &gLwm2mClient[lwm2mId];
  
        hasLoop = FALSE;
        clientReady = FALSE;
        
        if(lwm2mTaskStart == 0){
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_0, P_INFO, 0, "no main task, need to start mainLoop first");
            ret = lwm2mCreateMainLoop();
            if(ret == LWM2M_ERRID_OK){
                hasLoop = TRUE;
            }
        }else{
            hasLoop = TRUE;
        }
        if(hasLoop){
            ret = lwm2mClientReady(lwm2mId);
            if(ret == LWM2M_ERRID_OK){
                clientReady = TRUE;
            }
        }
        switch(msg_type)
        {
            case MSG_LWM2M_NOTIFY:
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_1, P_INFO, 0, "TO HANDLE AT+LWM2MNOTIFY");
                memcpy(&uri, &msg.uri, sizeof(lwm2m_uri_t));
                if(!hasLoop || !clientReady){
                    cnfMsg.ret = LWM2M_INTER_ERROR;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_NOTIFY_CNF, primSize, &cnfMsg);
                    break;
                }
                lwm2m_resource_value_changed(pContext->context, &uri);
                applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_LWM2M, APPL_LWM2M_NOTIFY_CNF, 0, PNULL);
                break;
                
            case MSG_LWM2M_ADDOBJ:
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_2, P_INFO, 0, "TO HANDLE AT+LWM2MADDOBJ");
                if(!hasLoop || !clientReady){
                    cnfMsg.ret = LWM2M_INTER_ERROR;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_ADDOBJ_CNF, primSize, &cnfMsg);
                    free(((lwm2mAddobjCmd*)(msg.cmd))->resourceIds);
                    free(msg.cmd);
                    break;
                }
                pCmd = (lwm2mAddobjCmd*)msg.cmd;
                objectId = pCmd->objectid ;
                instanceId = pCmd->instanceId;
                resourceIds = pCmd->resourceIds;
                resourceCount = pCmd->resourceCount;
                objectP = lwm2mFindObject(pContext->context, objectId);
                if(objectP == NULL)
                {
                    objectP = malloc(sizeof(lwm2m_object_t));
                    memset(objectP, 0, sizeof(lwm2m_object_t));
                    objectP->objID = objectId;
                    result = lwm2mAddobj(pContext->context, objectP, resourceIds, resourceCount, lwm2mId, objectId, instanceId);
                    if(result == COAP_NO_ERROR){
                        result = addObjtoClient(pContext, objectP, objectId, TRUE);
                    }
                }
                else
                {
                    cnfMsg.ret = LWM2M_ALREADY_ADD;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_ADDOBJ_CNF, primSize, &cnfMsg);
                }
                if(result != COAP_NO_ERROR){
                    cnfMsg.ret = LWM2M_INTER_ERROR;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_ADDOBJ_CNF, primSize, &cnfMsg);
                }
                free(((lwm2mAddobjCmd*)(msg.cmd))->resourceIds);
                free(msg.cmd);
                
                break;
            
            case MSG_LWM2M_DELOBJ:
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_3, P_INFO, 0, "TO HANDLE AT+LWM2MDELOBJ");
                if(!hasLoop || !clientReady){
                    cnfMsg.ret = LWM2M_INTER_ERROR;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_DELOBJ_CNF, primSize, &cnfMsg);
                    break;
                }
                
                obj = lwm2mFindObject(pContext->context, msg.objId);
                if(obj != NULL)
                {
                    lwm2mAtcmdRetainDeleteObject(lwm2mId, msg.objId);
                    result = lwm2m_remove_object(pContext->context, msg.objId);
                    lwm2mRemoveObject(obj);
                    if(result != 0){
                        ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_3_1, P_INFO, 1, "result=%d",result);
                        ret = LWM2M_INTER_ERROR;
                    }else{
                        lwm2m_observe_clean_obj(pContext->context,msg.objId);
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_3_2, P_INFO, 0, "no find obj");
                    ret = LWM2M_NO_FIND_OBJ;
                }
                if(ret != LWM2M_ERRID_OK){
                    cnfMsg.ret = ret;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_DELOBJ_CNF, primSize, &cnfMsg);
                    break;
                }
                break;
                
            case MSG_LWM2M_UPDATE:
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_4, P_INFO, 0, "TO HANDLE AT+LWM2MUPDATE");
                if(!hasLoop || !clientReady){
                    cnfMsg.ret = LWM2M_INTER_ERROR;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_UPDATE_CNF, primSize, &cnfMsg);
                    break;
                }
                pContext->context->updateMode = 2;
                result = lwm2m_update_registration(pContext->context, 0, (BOOL)msg.withobj);
                if(result != 0){
                    cnfMsg.ret = result;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_UPDATE_CNF, primSize, &cnfMsg);
                }
                break;

            case MSG_LWM2M_DELETE:
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mAtcmdProcessing_5, P_INFO, 0, "TO HANDLE AT+LWM2MDELETE");
                if(!hasLoop || !clientReady){
                    cnfMsg.ret = LWM2M_INTER_ERROR;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_LWM2M, APPL_LWM2M_DEL_CNF, primSize, &cnfMsg);
                    break;
                }
                lwm2m_deregister(pContext->context);
                break;
         }
         if(keepTask == 0)
         {
             break;
         }
     }

    ECOMM_TRACE(UNILOG_LWM2M, onenet_atcmd_processing_6, P_INFO, 0, "MIPLDELETE delete the atcmd process");
    lwm2m_atcmd_task_flag = LWM2M_TASK_STAT_NONE;
    lwm2m_atcmd_task_handle = NULL;
    osThreadExit();
}

static INT32 lwm2mHandleATTaskInit(void)
{
    INT32 ret = LWM2M_ERRID_OK;

    if(lwm2m_atcmd_handle == NULL)
    {
        lwm2m_atcmd_handle = xQueueCreate(24, sizeof(LWM2M_ATCMD_Q_MSG));
    }
    else
    {
        xQueueReset(lwm2m_atcmd_handle);
    }

    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "lwAThdl";
    task_attr.stack_size = LWM2M_ATHDL_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;

    lwm2m_atcmd_task_handle = osThreadNew(lwm2mAtcmdProcessing, NULL,&task_attr);
    if(lwm2m_atcmd_task_handle == NULL)
    {
        ret = LWM2M_ERRID_CREATE_THREAD_FAIL;
    }

    return ret;
}

INT32 lwm2mAthandleCreate(void)
{
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mAthandleCreate_1, P_INFO, 0, "to start lwm2m handleAT task");

    if(lwm2m_atcmd_task_flag == LWM2M_TASK_STAT_NONE)
    {
        lwm2m_atcmd_task_flag = LWM2M_TASK_STAT_CREATE;
        if(lwm2mHandleATTaskInit() != LWM2M_ERRID_OK)
        {
            ECOMM_TRACE(UNILOG_LWM2M, lwm2mAthandleCreate_0, P_INFO, 0, "lwm2m handleAT task create failed");
            return LWM2M_ERRID_CREATE_THREAD_FAIL;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mAthandleCreate_2, P_INFO, 0, "lwm2m handleAT task already start");
    }

    return LWM2M_ERRID_OK;
}

static BOOL ifNeedClient(UINT8 autoLoginFlag)
{
    INT8 i,ret=0;
    BOOL needClient = FALSE;
    slpManSlpState_t slpState;

    ret = lwm2mReadFile();
    if(ret<0){//read file failed cant restore task
        ECOMM_TRACE(UNILOG_LWM2M, ifNeedClient_1, P_INFO, 0, "read file failed can't restore client");
        return needClient;
    }
    for (i = 0; i < LWM2M_CLIENT_MAX_INSTANCE_NUM; i++) {
        lwm2mClientContext *lwm2m = &gLwm2mClient[i];
        if (lwm2m->isConnected){
            ECOMM_TRACE(UNILOG_LWM2M, ifNeedClient_2, P_INFO, 0, "before has lwm2m connected, need restore client");
            needClient = TRUE;
        }
    }

    slpState = slpManGetLastSlpState();
    if (slpState == SLP_ACTIVE_STATE)
    {
        ECOMM_TRACE(UNILOG_LWM2M, ifNeedClient_3, P_INFO, 1, "reset boot with autoLoginFlag=%d", autoLoginFlag);
        if (autoLoginFlag == 0)
        {
            if(needClient){
                ECOMM_TRACE(UNILOG_LWM2M, ifNeedClient_4, P_INFO, 0, "clear last lwm2m context");
                lwm2mClearFile();
                needClient = FALSE;
            }
        }
    }
    return needClient;
}

void lwm2mEngineInit(UINT8 autoLoginFlag)
{
    BOOL needRestoreClinet = FALSE;
    slpManApplyPlatVoteHandle("LWM2MHIB",&lwm2mSlpHandler);
    needRestoreClinet = ifNeedClient(autoLoginFlag);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mEngineInit_1, P_INFO, 1, "needStartTask:%d",needRestoreClinet);
    if(needRestoreClinet){
        if(loopMutex ==  NULL){
            loopMutex = osMutexNew(NULL);
        }
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mEngineInit_2, P_INFO, 0, "create has call,need restore. call onenet_athandle_create");
        INT32 result = lwm2mAthandleCreate();
        result = lwm2mInitClient();
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mEngineInit_3, P_INFO, 1, "lwm2mInitClient ret=%d", result);
    }
}


