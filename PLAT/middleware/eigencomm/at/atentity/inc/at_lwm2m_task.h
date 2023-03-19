/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_lwm2m_task.h
*
*  Description: Process LWM2M client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _AT_LWM2M_TASK_H
#define _AT_LWM2M_TASK_H

#include "at_util.h"
#include "liblwm2m.h"
#include "client/lwm2mclient.h"
#include "internals.h"

#include "sockets.h"
#include "inet.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#ifdef WITH_TINYDTLS_LWM2M
#include "shared/dtlsconnection.h"
#else
#include "shared/connection.h"
#endif

/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/
#define MAX_SERVER_LEN                     40
#define MAX_NAME_LEN                       32
#define LOCATION_LEN                   14
#define DEFAULT_LIFE_TIME           60
#define STAND_OBJ_COUNT             5
#define MAX_LWM2M_PACKET_SIZE       1024
#define READ_WRITE_TIMEOUT          5000
#define IND_BUFFER                  512
#define LWM2M_CLIENT_MAX_INSTANCE_NUM  1
#define MAX_OBJECT_COUNT              5
#define MAX_RESOURCE_COUNT            10

#define LWM2M_ERRID_OK                      (0)

#define LWM2M_ERRID_CREATE_THREAD_FAIL    (LWM2M_ERRID_OK + 1)
#define LWM2M_ERRID_CREATE_CLIENT_FAIL    (LWM2M_ERRID_OK + 2)
#define LWM2M_ERRID_NO_CONTEXT            (LWM2M_ERRID_OK + 3)
#define LWM2M_ERRID_NO_NETWORK    (LWM2M_ERRID_OK + 4)

/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/
/*
 * APPL SGID: APPL_LWM2M, related PRIM ID
*/
typedef enum applLwm2mPrimId_Enum
{
    APPL_LWM2M_PRIM_ID_BASE = 0,
    APPL_LWM2M_CREATE_CNF,
    APPL_LWM2M_ADDOBJ_CNF,
    APPL_LWM2M_DELOBJ_CNF,
    APPL_LWM2M_NOTIFY_CNF,
    APPL_LWM2M_UPDATE_CNF,
    APPL_LWM2M_DEL_CNF,
    APPL_LWM2M_IND,

    APPL_LWM2M_PRIM_ID_END = 0xFF
}applLwm2mPrimId;

typedef enum
{
    VALUE_TYPE_STRING = 0,
    VALUE_TYPE_OPAQUE,
    VALUE_TYPE_INTEGER,
    VALUE_TYPE_FLOAT,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_UNKNOWN,
}valuetype;


//AT HANDLE 
#define LWM2M_ATHDL_TASK_STACK_SIZE   2560
#define LWM2M_MSG_TIMEOUT 500

enum LWM2M_MSG_CMD
{
    MSG_LWM2M_NOTIFY,
    MSG_LWM2M_ADDOBJ,
    MSG_LWM2M_DELOBJ,
    MSG_LWM2M_DELETE, 
    MSG_LWM2M_UPDATE, 
};

enum LWM2M_TASK_STATUS
{
    LWM2M_TASK_STAT_NONE, 
    LWM2M_TASK_STAT_CREATE
};
//AT HANDLE



/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/

//AT HANDLE
typedef struct
{
    INT32 objectid;
    INT32 instanceId;
    UINT8* resourceIds;
    UINT16 resourceCount;
}lwm2mAddobjCmd;

typedef struct
{
    uint8_t cmd_type;
    UINT16 reqhandle;
    UINT32 lwm2mId;
    UINT16 objId;
    UINT8 withobj;
    lwm2m_uri_t uri;
    void* cmd;
}LWM2M_ATCMD_Q_MSG;

typedef struct
{
    INT32 id;
    INT32 ret;
}lwm2m_cnf_msg;

//AT HANDLE


typedef struct {
    BOOL isUsed;
    UINT16 objectId;
    UINT16 instanceId;
    UINT8 resourceCount;
    UINT16 resourceIds[MAX_RESOURCE_COUNT];
} lwm2mObjectInfo;

typedef struct {
    BOOL isUsed;
    BOOL isConnected;
    UINT8 isQuit;   //0: not quit  1: lwm2mdelete quit  2: lwm2mcreate reg fail quit
    UINT32 codeResaon;  //lwm2m_notify_code_t
    UINT32 reqHandle;
    UINT8 lwm2mclientId;
    CHAR* server;
    UINT16 serverPort;
    UINT16 localPort;
    CHAR* enderpoint;
    CHAR* pskId;
    CHAR* psk;
    UINT16 pskLen;
    INT32 lifetime;

    int addressFamily;
    lwm2m_context_t* context;
    lwm2mClientData* clientData;
#ifdef WITH_TINYDTLS_LWM2M
    dtls_connection_t * connP;
#else
    connection_t * connP;
#endif
    SemaphoreHandle_t readWriteSemph;
    lwm2m_data_t * readResultList;
    INT32  readResultCount;
    UINT8  writeResult;
    CHAR* location;
} lwm2mClientContext;

typedef struct _security_instance_
{
    struct _security_instance_ * next;        // matches lwm2m_list_t::next
    uint16_t                     instanceId;  // matches lwm2m_list_t::id
    char *                       uri;
    bool                         isBootstrap;
    uint8_t                      securityMode;
    char *                       publicIdentity;
    uint16_t                     publicIdLen;
    char *                       serverPublicKey;
    uint16_t                     serverPublicKeyLen;
    char *                       secretKey;
    uint16_t                     secretKeyLen;
    uint8_t                      smsSecurityMode;
    char *                       smsParams; // SMS binding key parameters
    uint16_t                     smsParamsLen;
    char *                       smsSecret; // SMS binding secret key
    uint16_t                     smsSecretLen;
    uint16_t                     shortID;
    uint32_t                     clientHoldOffTime;
} security_instance_t;



typedef struct _prv_instance_
{
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (UINT8 test in this case)
     */
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    UINT16 shortID;               // matches lwm2m_list_t::id
    UINT16 resourceCount;
    UINT16 *resourceIds;
} prv_instance_t;

typedef enum {
    LWM2M_CMD_NONE   = 0,
    LWM2M_CMD_UPDATE = 1,
    LWM2M_CMD_NOTIFY = 2,
    LWM2M_CMD_ADDOBJ = 3,
} lwm2m_cmd_type_t;

typedef struct {
    lwm2m_cmd_type_t cmdType;
    INT32 objectId;
    INT32 instanceId;
    INT32 resId;
    UINT8 withObj;
    lwm2m_object_t* objectP;
} lwm2mCmdContext;


typedef struct {
    BOOL isUsed;
    BOOL isConnected;
    CHAR server[MAX_SERVER_LEN+1];
    UINT16 serverPort;
    UINT16 localPort;
    CHAR enderpoint[MAX_NAME_LEN+1];
    CHAR pskId[MAX_NAME_LEN+1];
    CHAR psk[MAX_NAME_LEN+1];
    INT32 lifetime;
    UINT8 lwm2mclientId;
    UINT32 lwm2mState;
    lwm2mObjectInfo objectInfo[MAX_OBJECT_COUNT];
    lwm2mObserveBack obsevInfo[MAX_LWM2M_OBSERVE_NUM];
    CHAR location[LOCATION_LEN + 1];
    UINT32 reqhandle;
} lwm2mRetentionContext;

typedef struct lwm2mNvmHeader_Tag
{
    UINT16 fileBodySize; //file body size, not include size of header;
    UINT8  version;
    UINT8  checkSum;
}lwm2mNvmHeader;

#define LWM2M_NVM_FILE_NAME      "lwm2mconfig.nvm"

#define LWM2M_NVM_FILE_VERSION   0

lwm2mClientContext* lwm2mGetFreeClient(void);
lwm2mClientContext* lwm2mFindClient(INT32 lwm2mClientId);

INT8 lwm2mAtcmdRetainSaveObject(UINT8 lwm2mId, UINT16 objectId, UINT16 instanceId, UINT16 resourceCount, UINT16 *resourceIds);
void lwm2mAtcmdRetainDeleteObject(UINT8 lwm2mId, UINT16 objectId);
void lwm2mAtcmdRetainDeleteAllObjects(UINT8 lwm2mId);
//static void lwm2mTimerCallback(void *user_data);
//static void lwm2mStartTimer(lwm2mAtCmd *lwm2m);
///static void lwm2mStopTimer(lwm2mAtCmd *lwm2m);
void lwm2mRemoveObject(lwm2m_object_t *obj);
lwm2m_object_t* lwm2mFindObject(lwm2m_context_t* contextP, UINT16 id);

void lwm2mRemoveClient(UINT8 id);
void lwm2mClearFile(void);

INT32 lwm2mAddobj(lwm2m_context_t* contextP, lwm2m_object_t* objectP, UINT8* resourceIds, UINT8 resourceCount, UINT8 lwm2mId, INT32 objectId, INT32 instanceId);
INT32 addObjtoClient( lwm2mClientContext* lwm2m, lwm2m_object_t* objectP, INT32 objectId, UINT8 objectIsExist);


UINT8 lwm2mObserveCallback (UINT16 code, UINT16 msgId, lwm2m_uri_t * uriP, void* userData);
UINT8 lwm2mObjectDiscoverCallback (UINT16 instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP);
UINT8 lwm2mParameterCallback (lwm2m_uri_t * uriP, lwm2m_attributes_t * attrP, void* userData);
UINT8 lwm2mObjectReadCallback (UINT16 instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP);
UINT8 lwm2mObjectWriteCallback(UINT16 instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP);
UINT8 lwm2mObjectExecuteCallback (UINT16 instanceId, UINT16 resourceId, UINT8 * buffer, int length, lwm2m_object_t * objectP);
UINT8 lwm2mObjectCreateCallback (UINT16 instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP);
UINT8 lwm2mObjectDeleteCallback (UINT16 instanceId, lwm2m_object_t * objectP);

#ifdef WITH_TINYDTLS_LWM2M
void * lwm2mConnectServer(UINT16 secObjInstID, void * userData);
#else
void * lwm2mConnectServer(UINT16 secObjInstID, void * userData);
#endif
void lwm2mCloseConnection(void * sessionH, void * userData);
void lwm2mNotifyCallback(lwm2m_notify_type_t type, lwm2m_notify_code_t code, UINT16 mid);
INT8 lwm2mCreate(lwm2mClientContext* lwm2m);
INT32 lwm2mCreateMainLoop(void);

CmsRetId lwm2mClientNotify(UINT32 reqHandle, UINT32 id, lwm2m_uri_t* pUri);
CmsRetId lwm2mClientAddobj(UINT32 reqHandle, UINT32 id, lwm2mAddobjCmd* pCmd);
CmsRetId lwm2mClientDelobj(UINT32 reqHandle, UINT32 id, INT32 objId);
CmsRetId lwm2mClientUpdate(UINT32 reqHandle, UINT32 id, UINT8 withobj);
CmsRetId lwm2mClientDelete(UINT32 reqHandle, UINT32 id);
INT32 lwm2mAthandleCreate(void);


#endif

