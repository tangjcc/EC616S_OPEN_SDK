#ifndef _OBJECT_DEFS_H_
#define _OBJECT_DEFS_H_

#include "cis_api.h"
#include "cis_internals.h"

#define SAMPLE_OBJECT_MAX              1
#define SAMPLE_OID_A                   (3306)
#define SAMPLE_A_INSTANCE_COUNT        1
#define SAMPLE_A_INSTANCE_BITMAP       "1"

#define CIS_NV_FILE_NAME      "cisExample.nvm"
#define CIS_NV_FILE_VERSION   0

typedef struct st_sample_object
{
    cis_oid_t         oid;
    cis_instcount_t   instCount;
    const char*       instBitmap;
    const cis_rid_t*  attrListPtr;
    uint16_t          attrCount;
    const cis_rid_t*  actListPtr;
    uint16_t          actCount;
} st_sample_object;


//////////////////////////////////////////////////////////////////////////
//a object

typedef struct st_object_a{
    int32_t intValue;
    float   floatValue;
    bool    boolValue;
    char    strValue[1024];
    uint8_t update;
} st_object_a;


typedef struct st_instance_a
{
    cis_iid_t   instId;
    bool        enabled;
    st_object_a instance;
} st_instance_a;


enum{
    attributeA_intValue     = 5851,
    attributeA_boolValue    = 5850,
    attributeA_stringValue  = 5750,
};

enum{
    actionA_1           = 100,
};

                        
//at+mipldiscoverrsp=0,msgid1,1,14,"5750;5850;5851" string bool integer
static const cis_rid_t const_AttrIds_a[] = {
    attributeA_intValue,
    attributeA_boolValue,
    attributeA_stringValue,
};


static const cis_rid_t const_ActIds_a[] = {
    actionA_1,
};

typedef enum{
    CIS_CALLBACK_UNKNOWN = 0,
    CIS_CALLBACK_READ,
    CIS_CALLBACK_WRITE,
    CIS_CALLBACK_EXECUTE,
    CIS_CALLBACK_OBSERVE,
    CIS_CALLBACK_SETPARAMS,
    CIS_CALLBACK_DISCOVER,
} et_callback_type_t1;



struct st_observe_info
{
    struct st_observe_info* next;
    cis_listid_t    mid;
    cis_uri_t       uri;
    cis_observe_attr_t params;
};

struct store_observe_info
{
    cis_listid_t    mid;
    cis_uri_t       uri;
    cis_observe_attr_t params;
};

struct st_callback_info
{
    struct st_callback_info* next;
    cis_listid_t       mid;
    et_callback_type_t flag; 
    cis_uri_t          uri;

    union
    {
        struct{
            cis_data_t* value;
            cis_attrcount_t count;
        }asWrite;

        struct{
            uint8_t* buffer;
            uint32_t length;
        }asExec;
        
        struct  
        {
            bool flag;
        }asObserve;
        
        struct  
        {
            cis_observe_attr_t params;
        }asObserveParam;
    }param;
    
};

typedef enum
{
    ONENET_INIT_STATE,
    ONENET_IPREADY_STATE,
    ONENET_RESTORE_STATE,
    ONENET_HEART_BEAT_ARRIVED,
    ONENET_HEART_BEAT_UPDATE,
    ONENET_IDLE_STATE
} onenetStateMachine_t;

typedef enum
{
    ONENET_FOTA_NO,
    ONENET_FOTA_BEGIN,
    ONENET_FOTA_UPDATING,
    ONENET_FOTA_SUCCESS,
    ONENET_FOTA_FAIL
} onenetFotaState_t;

typedef struct {
    et_client_state_t pumpState;
    uint8_t host[16];
    uint8_t observeObjNum;
    observed_backup_t gObservedBackup[MAX_OBSERVED_COUNT]; // For CIS core save use
    uint8_t fotaState;
} onenet_context_t;

typedef struct cisNvHeader_Tag
{
    UINT16 fileBodySize; //file body size, not include size of header;
    UINT8  version;
    UINT8  checkSum;
}cisNvHeader;

bool cisAsynGetShutdown(void);
void cisAsynSetShutdown(bool shutdown);
bool cisAsynGetNotifyStatus(void);
uint8_t cisAsynGetFotaStatus(void);

bool cisAsyncCheckNotificationReady(void);

void prvMakeUserdata(void);
void cisOnenetInit(void);
void cisDataObserveReport(void);

void onenetDeepSleepTimerCbRegister(void);
void onenetDeepSleepCb(uint8 id);

#endif//_OBJECT_DEFS_H_
