
/**************************************************************

***************************************************************/
#include <stdint.h>
#include <string.h>

#include "ctiot_NV_data.h"
#include "ctiot_lwm2m_sdk.h"
#include "ctiot_parseuri.h"
#include "ctiot_fota.h"

#define PSK_LEN    33
#define PSKID_LEN  32
#define ENDPOINT_NAME_SIZE  150
#define SERVER_LEN 256

/***********************************************************
               NV ctiot params data
***********************************************************/

typedef struct
{
    // params vailid flag 
    char                           version[10];
    uint16_t                       structSize;
    uint32_t                       check_sum;

    // sdk params
    char                           serverIP[SERVER_LEN+1];  
    uint32_t                       port;
    uint32_t                       localPort;
    uint32_t                       lifetime;
    ctiot_funcv1_security_mode_e           securityMode;
    
    uint8_t                        regFlag;

    ctiot_funcv1_id_auth_mode_e           idAuthMode;
    ctiot_funcv1_on_uq_mode_e             onUqMode;
    uint8_t                               reserved1;
    ctiot_funcv1_auto_heartbeat_e         autoHeartBeat;
    ctiot_funcv1_wakeup_notify_e          wakeupNotify;
    ctiot_funcv1_protocol_mode_e          protocolMode;
#ifdef WITH_FOTA_RESUME
    char                           reserved[388];
    uint8_t                        fotaStatus;
    uint32_t                       fotaBlockNum;
    uint16_t                       fotaPort;
    char                           fotaAddress[40];
    char                           fotaUri[60];
#else
    char                           reserved[495];
#endif  
    uint8_t                        imsiValid;
    char                           imsi[16];
    uint32_t                       reqhandle;
    uint8_t                        pskValid;
    char                           psk_id[PSKID_LEN];
    uint16_t                       pskLen;
    char                           psk[PSK_LEN];
    char                           bsServerIP[SERVER_LEN+1];  
    uint8_t                        bsServerIPvalid;
    uint32_t                       bsPort;
    char                           endpoint[ENDPOINT_NAME_SIZE+1];
    uint8_t                        endpointValid;
    uint8_t                        dtlsType;
    uint8_t                        natType;
    uint8_t                        nsmiFlag;
    uint8_t                        nnmiFlag;
}NV_new_params_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    // params vailid flag 
    char                           hello[12];
    uint32_t                       check_sum;

    // sdk params
    char                           serverIP[20];  
    uint32_t                       port;
    uint32_t                       localPort;
    uint32_t                       lifetime;
    ctiot_funcv1_security_mode_e   securityMode;
    uint16_t                       reserved1;
    uint8_t                        dipFlag;
    uint8_t                        regFlag;

    ctiot_funcv1_id_auth_mode_e           idAuthMode;
    ctiot_funcv1_on_uq_mode_e             onUqMode;
    uint16_t                              reserved2;
    ctiot_funcv1_auto_heartbeat_e         autoHeartBeat;
    ctiot_funcv1_wakeup_notify_e          wakeupNotify;
    ctiot_funcv1_protocol_mode_e          protocolMode;

    char                           instanceList[512];
    uint32_t                       reqhandle;
}NV_old_params_t;
#pragma pack()

/************************************************************
            lwm2m context data
************************************************************/

#pragma pack(1)
typedef struct
{
    //lwm2m context vailid flag 
    char                           version[10];
    uint16_t                       structSize;
    uint32_t                       check_sum;

    //dip
    char                           localIP[40];

    ctiot_funcv1_boot_flag_e              bootFlag;
     
    //lwm2m context data
    uint32_t                       server_cnt;
    uint32_t                       observed_cnt;
    uint8_t                        loginStatus;
    uint8_t                        reserved;
}NV_lwm2m_context_t;
#pragma pack()

/*********************************
  1. lwm2m server list data
*********************************/
#pragma pack(1)
typedef struct 
{      
    uint16_t                shortID;      
    time_t                  lifetime;     
    lwm2m_binding_t         binding;
    uint16_t                secObjInstID;
    time_t                  registration;
    char                    location[64];
}NV_lwm2m_server_t;
#pragma pack()

/*********************************
  2. lwm2m observed list data
*********************************/
#pragma pack(1)
typedef struct 
{
    lwm2m_uri_t uri;
     
    uint32_t watcher_cnt;
}NV_lwm2m_observed_t;
#pragma pack()

#pragma pack(1)
typedef struct 
{
    bool active;
    bool update;

    uint8_t     attr_flag;
    
    //lwm2m_attributes_t
    uint8_t     toSet;
    uint8_t     toClear;
    uint32_t    minPeriod;
    uint32_t    maxPeriod;
    double      greaterThan;
    double      lessThan;
    double      step;
    
    lwm2m_media_type_t format;
    uint8_t token[8];
    size_t tokenLen;
    time_t lastTime;
    uint32_t counter;
    uint16_t lastMid;
    
    union
    {
        int64_t asInteger;
        double  asFloat;
    } lastValue;
    
    uint32_t server_cnt;
}NV_lwm2m_watcher_t;
#pragma pack()

#pragma pack(1)
typedef struct 
{      
    uint16_t                shortID;
}NV_lwm2m_watch_server_t;
#pragma pack()



/****************************** logic block int the cache **********************/
#define  FLASH_CACHE_SIZE   2000
uint8_t *flash_cache = NULL;

typedef enum
{
    block0_params = 0,
    block1_context = 1
}NV_logic_block_t;

typedef struct logic_block_entry
{
    NV_logic_block_t  logic_block_no;
    uint8_t          *address;
    uint32_t          block_size;
    uint16_t          cache_valid;
    uint16_t          block_type;  //0:old version "hellohello" 1:new version "version" 2:unknown type
    uint32_t          check_sum;
    char             *name;
}logic_block_entry_t;

#define LOGIC_BLOCK_ENTRY_CNT   2
logic_block_entry_t logic_block_entry_table[LOGIC_BLOCK_ENTRY_CNT]=
{
    {.logic_block_no = block0_params,  .address = NULL, .block_size = NULL, .cache_valid = 0, .check_sum = 0, .name = "param"},
    {.logic_block_no = block1_context, .address = NULL, .block_size = NULL, .cache_valid = 0, .check_sum = 0, .name = "context"},
};

#ifdef WITH_FOTA_RESUME
extern CTIOT_URI fotaUrl;
extern ctiotFotaManage fotaStateManagement;
extern uint32_t blockNumber;
#endif
static logic_block_entry_t *NV_get_logic_block(NV_logic_block_t logic_block)
{
    if(logic_block == block0_params)
        return &logic_block_entry_table[0];
    else
        return &logic_block_entry_table[1];
}



/**********************************  utils check sum ****************************************/

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


/******************************** NV api *********************************/

static void NV_init_cache()
{
    int                  ret;
    uint32_t             sum;
    NV_old_params_t     *check_oldParams;
    NV_new_params_t     *check_newParams;
    NV_lwm2m_context_t  *check_context;
    logic_block_entry_t *pentry;
    uint8_t             *pcache = NULL;
    BOOL                 block_valid = FALSE;
    if( flash_cache == NULL )
    {
        flash_cache = slpManGetUsrNVMem();
        logic_block_entry_table[0].address = flash_cache;

        check_oldParams = (NV_old_params_t *)flash_cache;
        ret = memcmp(check_oldParams->hello, "hellohello", 10);
        if( ret == 0 ){
            ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_0_1, P_INFO, 0, "it's old params");
            logic_block_entry_table[0].block_type = 0;
            logic_block_entry_table[0].block_size = sizeof(NV_old_params_t);
            ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_0_1_1, P_INFO, 2, "params p=0x%x,size=0x%x", logic_block_entry_table[0].address, logic_block_entry_table[0].block_size);
            logic_block_entry_table[1].block_type = 0;
            logic_block_entry_table[1].address = flash_cache+logic_block_entry_table[0].block_size;
            logic_block_entry_table[1].block_size = FLASH_CACHE_SIZE-sizeof(NV_old_params_t);
            block_valid = TRUE;
            ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_0_1_2, P_INFO, 2, "context p=0x%x,size=0x%x", logic_block_entry_table[1].address, logic_block_entry_table[1].block_size);
        }else{
            check_newParams = (NV_new_params_t *)flash_cache;
            ret = memcmp(check_newParams->version, "version", 7);
            if( ret == 0 ){
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_0_2, P_INFO, 0, "it's new params");
                logic_block_entry_table[0].block_type = 1;
                logic_block_entry_table[0].block_size = check_newParams->structSize;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_0_2_1, P_INFO, 2, "params p=0x%x,size=0x%x", logic_block_entry_table[0].address, check_newParams->structSize);
                logic_block_entry_table[1].block_type = 1;
                logic_block_entry_table[1].address = flash_cache+logic_block_entry_table[0].block_size;
                check_context = (NV_lwm2m_context_t *)logic_block_entry_table[1].address;
                logic_block_entry_table[1].block_size = check_context->structSize;
                block_valid = TRUE;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_0_2_2, P_INFO, 2, "context p=0x%x,size=0x%x", logic_block_entry_table[1].address, check_context->structSize);
            }else{
                ECOMM_STRING(UNILOG_CTLWM2M, NV_init_cache_0_3, P_INFO, "block invalid,maybe erased NVM,unknown params!!!%s", (uint8_t*)check_newParams->version);
                logic_block_entry_table[0].block_type = 2;
                logic_block_entry_table[0].cache_valid = 0;
                logic_block_entry_table[0].block_size = sizeof(NV_new_params_t);
                logic_block_entry_table[1].block_type = 2;
                logic_block_entry_table[1].address = flash_cache+sizeof(NV_new_params_t);
                logic_block_entry_table[1].cache_valid = 0;
                block_valid = FALSE;
            }
        }
        
    }

    if(block_valid)
    {
        //check entry 1
        pentry = NV_get_logic_block(block0_params);
        pcache = pentry->address;
        if(pentry->block_type == 0){//old params
            pentry->cache_valid = 1;
            check_oldParams = (NV_old_params_t *)pcache;
            sum = check_sum(pentry->address+sizeof(check_oldParams->hello)+sizeof(check_oldParams->check_sum), pentry->block_size-sizeof(check_oldParams->hello)-sizeof(check_oldParams->check_sum));
            if( sum == check_oldParams->check_sum )
            {
                pentry->cache_valid = 1;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_1_1, P_INFO, 0, "block0 checksum ok param valid");
            }
            else
            {
                pentry->cache_valid = 0;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_1_2, P_INFO, 0, "old block0 checksum failed");
            }
        }else{//new params
            check_newParams = (NV_new_params_t *)pcache;
            sum = check_sum(pentry->address+sizeof(check_newParams->version)+sizeof(check_newParams->structSize)+sizeof(check_newParams->check_sum), pentry->block_size-sizeof(check_newParams->version)-sizeof(check_newParams->structSize)-sizeof(check_newParams->check_sum));
            if( sum == check_newParams->check_sum )
            {
                pentry->cache_valid = 1;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_1_3, P_INFO, 0, "block0 checksum ok param valid");
            }
            else
            {
                pentry->cache_valid = 0;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_1_4, P_INFO, 0, "block0 checksum failed, return FALSE");
            }
        }
        //check entry 2
        pentry = NV_get_logic_block(block1_context);
        pcache        = pentry->address;
        check_context = (NV_lwm2m_context_t *)pcache;

        if(pentry->block_type == 0){
            ret = memcmp(check_context->version, "hellohello", 10);
            if(ret == 0)
            {
                if(pentry->block_size > FLASH_CACHE_SIZE || pentry->block_size == 0){//sth wrong
                    ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_2_1_1, P_INFO, 0, "old block1 sth wrong block invalid");
                }else{
                    pentry->cache_valid = 1;
                    ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_2_1, P_INFO, 0, "old block1 version valid");
                }
            }
            else
            {
                pentry->cache_valid = 0;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_2_2, P_INFO, 0, "old block1 version mismatch");
            }
        }else if(pentry->block_type == 1){
            ret = memcmp(check_context->version, "version", 7);
            if(ret == 0)
            {
                if(pentry->block_size > FLASH_CACHE_SIZE || pentry->block_size == 0){//sth wrong
                    ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_2_3_1, P_INFO, 0, "new block1 sth wrong block invalid");
                }else{
                    pentry->cache_valid = 1;
                    ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_2_3, P_INFO, 0, "new block1 version ok context valid");
                }
            }
            else
            {
                pentry->cache_valid = 0;
                ECOMM_TRACE(UNILOG_CTLWM2M, NV_init_cache_2_4, P_INFO, 0, "new block1 version mismatch");
            }
        }
    }
}

static logic_block_entry_t * NV_get_cache(NV_logic_block_t block)
{
    logic_block_entry_t *pentry;
    if(flash_cache == NULL){
        NV_init_cache();
    }
    pentry = NV_get_logic_block(block);
    return pentry;
}

void NV_updata_cache(void)
{
    slpManUpdateUserNVMem();
}

/*
*  Flush the User none volatile memory immediately to File System. 
*/
void NV_flush_cache(void)
{
    slpManFlushUsrNVMem();
}


/******************************** list *********************************/

struct c2f_list_t
{
    struct c2f_list_t *next;    
};

static int c2f_get_list_count(void *c2f_list)
{
    uint32_t count=0;
    struct c2f_list_t *plist = c2f_list;

    while(plist)
    {
        count++;
        plist = plist->next;
    }

    return count;
}


/**********************************  api ****************************************/

int32_t c2f_encode_params(ctiot_funcv1_context_t *pContext)
{
    NV_new_params_t          *ctiot_funcv1_params;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;
    
    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block0_params);
    
    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;

    //fill top
    memset(ctiot_funcv1_params->version,0,sizeof(ctiot_funcv1_params->version));
    strcpy(ctiot_funcv1_params->version, "version");
    ctiot_funcv1_params->structSize = sizeof(NV_new_params_t);
    
    if(pContext->serverIP)
    {
        strcpy(ctiot_funcv1_params->serverIP, pContext->serverIP);

    }
    else  
    {
        ctiot_funcv1_params->serverIP[0] = 0;
    }
    
#ifdef  FEATURE_REF_AT_ENABLE
    if(pContext->bsServerIP)
    {
        strcpy(ctiot_funcv1_params->bsServerIP, pContext->bsServerIP);
        ctiot_funcv1_params->bsServerIPvalid = 0xA5;
    }
    else  
    {
        ctiot_funcv1_params->bsServerIP[0] = 0;
    }
    ctiot_funcv1_params->bsPort             = pContext->bsPort;
    if(pContext->endpoint != NULL)
    {
        int len = strlen(pContext->endpoint);
        if(len > 0 && len <= ENDPOINT_NAME_SIZE){
            strcpy(ctiot_funcv1_params->endpoint, pContext->endpoint);
            ctiot_funcv1_params->endpointValid = 0xA5;
        }
    }
    ctiot_funcv1_params->dtlsType           =  pContext->dtlsType;
    ctiot_funcv1_params->nsmiFlag           =  pContext->nsmiFlag;
#endif
    ctiot_funcv1_params->nnmiFlag           =  pContext->nnmiFlag;
    ctiot_funcv1_params->natType            =  pContext->natType;
    //fill sdk params
    ctiot_funcv1_params->port               = pContext->port;
    ctiot_funcv1_params->localPort          = pContext->localPort;
    ctiot_funcv1_params->lifetime           = pContext->lifetime;
    ctiot_funcv1_params->securityMode       = pContext->securityMode;
    ctiot_funcv1_params->idAuthMode         = pContext->idAuthMode;
    ctiot_funcv1_params->autoHeartBeat      = pContext->autoHeartBeat;
    ctiot_funcv1_params->onUqMode           = pContext->onUqMode;
    ctiot_funcv1_params->wakeupNotify       = pContext->wakeupNotify;
    ctiot_funcv1_params->protocolMode       = pContext->protocolMode;
    ctiot_funcv1_params->regFlag            = pContext->regFlag; 

#ifdef WITH_MBEDTLS

    if(pContext->pskid != 0 && pContext->psk != 0 )//user has set pskid or psk
    {
        if( strlen(pContext->pskid)+1 > PSKID_LEN || strlen(pContext->psk)+1 > PSK_LEN )
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_params_0, P_INFO, 0, "psk information err!!!!!");
            return NV_ERR_PARAMS;
        }
       
        memcpy(ctiot_funcv1_params->psk_id, pContext->pskid, strlen(pContext->pskid)+1);
        memcpy(ctiot_funcv1_params->psk,    pContext->psk,   strlen(pContext->psk)+1);
        ctiot_funcv1_params->pskLen = pContext->pskLen;
        ctiot_funcv1_params->pskValid = 0xA5;
    }
    else if(pContext->pskid == 0 && pContext->psk == 0 )
    {
        memset(ctiot_funcv1_params->psk_id, 0, PSKID_LEN);
        memset(ctiot_funcv1_params->psk,  0,   PSK_LEN);
        ctiot_funcv1_params->pskLen = 0;
        ctiot_funcv1_params->pskValid = 0;
    }
#endif

    ctiot_funcv1_params->reqhandle            = pContext->reqhandle; 

    uint8_t len = strlen(pContext->chipInfo->cImsi);
    if(len>0 && len<16)//imsi is a mazimum 15 decimal digits
    {
        ctiot_funcv1_params->imsiValid = 0xA5;
        strcpy(ctiot_funcv1_params->imsi, pContext->chipInfo->cImsi);
    }

    pentry->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), ctiot_funcv1_params->structSize-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    if( pentry->check_sum != ctiot_funcv1_params->check_sum )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_params_1, P_INFO, 0, "params change, then updata flash");
        ctiot_funcv1_params->check_sum = pentry->check_sum;
        /* Flush the User none volatile memory immediately to File System.  */
        NV_flush_cache();

    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_params_2, P_INFO, 0, "params not change");
    }

    return NV_OK;
}

int32_t c2f_clean_params(ctiot_funcv1_context_t *pContext)
{
    NV_new_params_t          *ctiot_funcv1_params;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;
    
    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block0_params);
    
    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;

    //fill top
    memset(ctiot_funcv1_params,0,sizeof(NV_new_params_t));
    strcpy(ctiot_funcv1_params->version, "version");
    ctiot_funcv1_params->idAuthMode = AUTHMODE_IMEI;
    ctiot_funcv1_params->onUqMode = DISABLE_UQ_MODE;
    ctiot_funcv1_params->autoHeartBeat = AUTO_HEARTBEAT;
    ctiot_funcv1_params->wakeupNotify = NOTICE_MCU;
    ctiot_funcv1_params->protocolMode = PROTOCOL_MODE_NORMAL;
    ctiot_funcv1_params->lifetime = 86400;
        
#ifdef  FEATURE_REF_AT_ENABLE
    ctiot_funcv1_params->regFlag = 1;
#endif
    ctiot_funcv1_params->structSize = sizeof(NV_new_params_t);
    ctiot_funcv1_params->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), sizeof(NV_new_params_t)-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clean_params_1, P_INFO, 1, "params change, then updata flash,sizeof it=%d",ctiot_funcv1_params->structSize);
    /* Flush the User none volatile memory immediately to File System.  */
    NV_flush_cache();
    
    return NV_OK;
}

#ifdef  FEATURE_REF_AT_ENABLE
int32_t c2f_clear_serverIP(ctiot_funcv1_context_t *pContext)
{
    NV_new_params_t              *ctiot_funcv1_params;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;
    
    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block0_params);
    
    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;

    memset(ctiot_funcv1_params->serverIP, 0, SERVER_LEN);//clear serverIP
    //recalculate sum
    pentry->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), sizeof(NV_new_params_t)-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    if( pentry->check_sum != ctiot_funcv1_params->check_sum )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_serverIP_1, P_INFO, 0, "params change, then updata flash");
        ctiot_funcv1_params->check_sum = pentry->check_sum;
        /* Flush the User none volatile memory immediately to File System.  */
        NV_flush_cache();

    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_serverIP_2, P_INFO, 0, "params not change");
    }

    return NV_OK;
}

int32_t c2f_clear_bsServerIP(ctiot_funcv1_context_t *pContext)
{
    NV_new_params_t          *ctiot_funcv1_params;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;
    
    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block0_params);

    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;

    memset(ctiot_funcv1_params->bsServerIP, 0, SERVER_LEN);//clear bsServerIP
    ctiot_funcv1_params->bsServerIPvalid = 0;
    
    //recalculate sum
    pentry->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), sizeof(NV_new_params_t)-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    if( pentry->check_sum != ctiot_funcv1_params->check_sum )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_bsServerIP_1, P_INFO, 0, "params change, then updata flash");
        ctiot_funcv1_params->check_sum = pentry->check_sum;
        /* Flush the User none volatile memory immediately to File System.  */
        NV_flush_cache();

    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_bsServerIP_2, P_INFO, 0, "params not change");
    }

    return NV_OK;
}

int32_t cache_get_loginstatus(uint8_t *status)
{
    NV_lwm2m_context_t   *lwm2m_context;
    logic_block_entry_t  *pentry;
    uint8_t              *pcache;
    
    pentry = NV_get_cache(block1_context);
    
    if(pentry->cache_valid == 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, cache_get_loginstatus_1, P_INFO, 0, "block1_context invalid");
        return NV_ERR_CACHE_INVALID;
    }
    pcache = pentry->address;        
    lwm2m_context = (NV_lwm2m_context_t *)pcache;
    
    *status = lwm2m_context->loginStatus;
    
    return NV_OK;
}

#endif

#ifdef WITH_FOTA_RESUME
void c2f_encode_fotaurl_params()
{
    NV_new_params_t          *ctiot_funcv1_params;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;

    pentry = NV_get_cache(block0_params);
    
    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;

    ctiot_funcv1_params->fotaPort = fotaUrl.port;
    ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_fotaurl_params_0, P_INFO, 1, "fotaPort = %d",fotaUrl.port);

    if(fotaUrl.address[0] && strlen(fotaUrl.address) < 40)
    {
        strcpy(ctiot_funcv1_params->fotaAddress, fotaUrl.address);
    }
    
    if(fotaUrl.uri[0] !=0 && strlen(fotaUrl.uri) < 60)
    {
        strcpy(ctiot_funcv1_params->fotaUri, fotaUrl.uri);
    }

    pentry->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), ctiot_funcv1_params->structSize-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    if( pentry->check_sum != ctiot_funcv1_params->check_sum )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_fotaurl_params_1, P_INFO, 0, "params change, then updata flash");
        ctiot_funcv1_params->check_sum = pentry->check_sum;
        NV_updata_cache();

    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_fotaurl_params_2, P_INFO, 0, "params not change");
    }

}

void c2f_encode_fotarange_params()
{
    NV_new_params_t          *ctiot_funcv1_params;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;

    pentry = NV_get_cache(block0_params);
    
    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;

    ctiot_funcv1_params->fotaStatus = (uint8_t)fotaStateManagement.fotaState;
    ctiot_funcv1_params->fotaBlockNum = blockNumber;

    pentry->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), ctiot_funcv1_params->structSize-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    if( pentry->check_sum != ctiot_funcv1_params->check_sum )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_fotarange_params_1, P_INFO, 0, "params change, then updata flash");
        ctiot_funcv1_params->check_sum = pentry->check_sum;
        NV_updata_cache();

    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_fotarange_params_2, P_INFO, 0, "params not change");
    }

}

int32_t c2f_clear_fotaparam()
{
    NV_new_params_t          *ctiot_funcv1_params;
    NV_lwm2m_context_t       *lwm2m_context;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;
    BOOL                     needFlush = FALSE;
    pentry = NV_get_cache(block0_params);
    
    pcache       = pentry->address;
    ctiot_funcv1_params = (NV_new_params_t *)pcache;
    
    ctiot_funcv1_params->fotaStatus = 0;
    ctiot_funcv1_params->fotaBlockNum = 0;
    ctiot_funcv1_params->fotaPort = 0;

    memset(ctiot_funcv1_params->fotaAddress, 0, 40);//clear fotaAddress
    memset(ctiot_funcv1_params->fotaUri, 0, 60);//clear fotaUri
    //recalculate sum
    pentry->check_sum = check_sum(pentry->address+sizeof(ctiot_funcv1_params->version)+sizeof(ctiot_funcv1_params->structSize)+sizeof(ctiot_funcv1_params->check_sum), sizeof(NV_new_params_t)-sizeof(ctiot_funcv1_params->version)-sizeof(ctiot_funcv1_params->structSize)-sizeof(ctiot_funcv1_params->check_sum));
    if( pentry->check_sum != ctiot_funcv1_params->check_sum )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_fotaparam_1, P_INFO, 0, "params change, then updata flash");
        ctiot_funcv1_params->check_sum = pentry->check_sum;
        
        needFlush = TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_fotaparam_2, P_INFO, 0, "params not change");
    }
    
    pentry = NV_get_cache(block1_context);
    
    pcache        = pentry->address;
    lwm2m_context = (NV_lwm2m_context_t *)pcache;

    lwm2m_context->bootFlag     = BOOT_NOT_LOAD;

    pentry->check_sum = check_sum(pentry->address+sizeof(lwm2m_context->version)+sizeof(lwm2m_context->structSize)+sizeof(lwm2m_context->check_sum), lwm2m_context->structSize-sizeof(lwm2m_context->version)-sizeof(lwm2m_context->structSize)-sizeof(lwm2m_context->check_sum));
    if( pentry->check_sum != lwm2m_context->check_sum )
    {
        lwm2m_context->check_sum = pentry->check_sum;
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_fotaparam_3, P_INFO, 0, "re calculation check sum and updata");

        needFlush = TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clear_fotaparam_4, P_INFO, 0, "context not change no need updata");
    }

    if(needFlush){
        NV_flush_cache();
    }
    return NV_OK;
}
#endif

int32_t c2f_encode_context(ctiot_funcv1_context_t *pContext)
{
    NV_lwm2m_context_t       *lwm2m_context;
    NV_lwm2m_server_t        *server_data;
    NV_lwm2m_observed_t      *observed_data;
    NV_lwm2m_watcher_t       *watcher_data;
    NV_lwm2m_watch_server_t  *watcher_server;
    lwm2m_server_t           *pser;
    lwm2m_observed_t         *pob;
    lwm2m_watcher_t          *pwat;
    uint8_t                  *pcache;

    logic_block_entry_t      *pentry;
    
    uint32_t i,ii,iii;
    
    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block1_context);
    
    pcache = pentry->address;   
    lwm2m_context = (NV_lwm2m_context_t *)pcache;

    //fill valid flag
    memset(lwm2m_context->version,0,sizeof(lwm2m_context->version));
    strcpy(lwm2m_context->version, "version");

    //fill local ip
    memcpy(lwm2m_context->localIP, pContext->localIP, sizeof(lwm2m_context->localIP));

    //fill bootflag
    lwm2m_context->bootFlag           = pContext->bootFlag;

    //fill loginStatus
    lwm2m_context->loginStatus        =  pContext->loginStatus;
    
    ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_context_0, P_INFO, 1, "loginStatus:%d",lwm2m_context->loginStatus);
    
    if( pContext->lwm2mContext == NULL )
    {
        lwm2m_context->server_cnt   = 0;
        lwm2m_context->observed_cnt = 0;
        pcache += sizeof(NV_lwm2m_context_t);
    }
    else
    {
        //fill lwm2m context
        lwm2m_context->server_cnt         = c2f_get_list_count(pContext->lwm2mContext->serverList);
        lwm2m_context->observed_cnt       = c2f_get_list_count(pContext->lwm2mContext->observedList);
    
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_context_1, P_INFO, 2, "server_cnt:%d, observed_cnt:%d",lwm2m_context->server_cnt,lwm2m_context->observed_cnt);
    
        //goto next block
        pcache += sizeof(NV_lwm2m_context_t);
        
        //fill server data
        pser = pContext->lwm2mContext->serverList;
        for(i = 0; i<lwm2m_context->server_cnt; i++)
        {
            server_data                 = (NV_lwm2m_server_t *)pcache;
            
            server_data->binding        = pser->binding;
            server_data->lifetime       = pser->lifetime;
            server_data->shortID        = pser->shortID;
            server_data->registration   = pser->registration;
            server_data->secObjInstID   = pser->secObjInstID;
    
            if( pser->location != NULL )
            {
                strcpy(server_data->location, pser->location);
            }
            else
            {
                server_data->location[0] = 0;
            }
    
            //goto next serverlist
            pcache += sizeof(NV_lwm2m_server_t);
            pser    = pser->next;
        }
        
        //goto next block
    
        //fill observed data
        pob = pContext->lwm2mContext->observedList;
        for(i=0; i<lwm2m_context->observed_cnt; i++)
        {
            observed_data                 = (NV_lwm2m_observed_t *)pcache;
       
            observed_data->uri.flag       = pob->uri.flag;
            observed_data->uri.instanceId = pob->uri.instanceId;
            observed_data->uri.objectId   = pob->uri.objectId;
            observed_data->uri.resourceId = pob->uri.resourceId;
            observed_data->watcher_cnt    = c2f_get_list_count( pob->watcherList);
            
            //goto next block
            pcache += sizeof(NV_lwm2m_observed_t);
    
            //fill watcher data
            pwat   = pob->watcherList;
            for(ii = 0; ii<observed_data->watcher_cnt; ii++)
            {
                watcher_data                  = (NV_lwm2m_watcher_t *)pcache;
    
                //params 0
                watcher_data->active          = pwat->active;
                watcher_data->update          = pwat->update;
    
                ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_context_1_0, P_INFO, 2, "pwat->update=%d,observed_data->uri.objectId=%d",pwat->update,observed_data->uri.objectId);
                //params 1
                if(pwat->parameters!=NULL)
                {
                    watcher_data->attr_flag   = 1;
                    watcher_data->toClear     = pwat->parameters->toClear;
                    watcher_data->toSet       = pwat->parameters->toSet;
                    watcher_data->minPeriod   = pwat->parameters->minPeriod;
                    watcher_data->maxPeriod   = pwat->parameters->maxPeriod;
                    watcher_data->greaterThan = pwat->parameters->greaterThan;
                    watcher_data->lessThan    = pwat->parameters->lessThan;
                    watcher_data->step        = pwat->parameters->step;
                }
                else
                {
                    watcher_data->attr_flag   = 0;
                }
                
                //params 2
                watcher_data->format          = pwat->format;
                memcpy(watcher_data->token,     pwat->token, pwat->tokenLen);
                watcher_data->tokenLen        = pwat->tokenLen;
                watcher_data->lastTime        = pwat->lastTime;
                watcher_data->counter         = pwat->counter;
                //params 3
                watcher_data->lastMid         = pwat->lastMid;
                watcher_data->lastValue.asInteger = pwat->lastValue.asInteger;
                watcher_data->server_cnt      = c2f_get_list_count(pwat->server);
    
                //goto next block
                pcache += sizeof(NV_lwm2m_watcher_t);
                
                //fill watcher server data
                pser = pwat->server;
                for(iii = 0; iii<watcher_data->server_cnt; iii++)
                {   
                    watcher_server                 = (NV_lwm2m_watch_server_t *)pcache;
                    
                    watcher_server->shortID        = pser->shortID;
                    
                    //goto next serverlist
                    pcache += sizeof(NV_lwm2m_watch_server_t);
                    pser    = pser->next;
                }
    
                //goto next watcherlist
                pwat = pwat->next;
            }
            
            //goto next observedList
            pob = pob->next;
        }
    }

    lwm2m_context->structSize = pcache-pentry->address;
    ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_context_1_1, P_INFO, 1, "block size:0x%x",lwm2m_context->structSize);
    
    pentry->check_sum = check_sum(pentry->address+sizeof(lwm2m_context->version)+sizeof(lwm2m_context->structSize)+sizeof(lwm2m_context->check_sum), lwm2m_context->structSize-sizeof(lwm2m_context->version)-sizeof(lwm2m_context->structSize)-sizeof(lwm2m_context->check_sum));
    if( pentry->check_sum != lwm2m_context->check_sum )
    {
        lwm2m_context->check_sum = pentry->check_sum;

        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_encode_context_2, P_SIG, 0, "context change, then update flash");
        NV_updata_cache();

    }
   
    return NV_OK;   
}

int32_t c2f_clean_context(ctiot_funcv1_context_t *pContext)
{
    NV_lwm2m_context_t       *lwm2m_context;
    uint8_t                  *pcache;
    logic_block_entry_t      *pentry;
    ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clean_context_1, P_INFO, 1, "clean context sizeof it=%d",sizeof(NV_lwm2m_context_t));
    
    pentry = NV_get_cache(block1_context);
    
    pcache        = pentry->address;
    lwm2m_context = (NV_lwm2m_context_t *)pcache;

    //fill valid flag
    memset(lwm2m_context->version,0,sizeof(lwm2m_context->version));
    strcpy(lwm2m_context->version, "version");

    lwm2m_context->structSize = sizeof(NV_lwm2m_context_t);
    memset(lwm2m_context->localIP, 0, sizeof(lwm2m_context->localIP));
    lwm2m_context->bootFlag     = BOOT_NOT_LOAD;
    lwm2m_context->server_cnt   = 0;
    lwm2m_context->observed_cnt = 0;
    lwm2m_context->reserved = 0;

    pentry->check_sum = check_sum(pentry->address+sizeof(lwm2m_context->version)+sizeof(lwm2m_context->structSize)+sizeof(lwm2m_context->check_sum), lwm2m_context->structSize-sizeof(lwm2m_context->version)-sizeof(lwm2m_context->structSize)-sizeof(lwm2m_context->check_sum));
    if( pentry->check_sum != lwm2m_context->check_sum )
    {
        lwm2m_context->check_sum = pentry->check_sum;
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clean_context_2, P_INFO, 0, "re calculation check sum and updata");

        /* Flush the User none volatile memory immediately to File System.  */
        NV_flush_cache();
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, c2f_clean_context_3, P_INFO, 0, "context not change no need updata");
    }

    return NV_OK;
}


int32_t f2c_encode_params(ctiot_funcv1_context_t *pContext)
{
    NV_new_params_t          *ctiot_funcv1_params;
    NV_old_params_t          *ctiot_funcv1_old_params;
    uint8_t              *pcache;
    logic_block_entry_t  *pentry;

    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block0_params);

    if(pentry->cache_valid == 0)
        return NV_ERR_CACHE_INVALID;
    
    pcache = pentry->address;

    if(pContext->serverIP != NULL) 
        ct_lwm2m_free(pContext->serverIP);
    
    if(pentry->block_type == 0)//old type
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, f2c_encode_params_0, P_INFO, 0, "encode old type params");
        ctiot_funcv1_old_params = (NV_old_params_t *)pcache;
    
        if( ctiot_funcv1_old_params->serverIP[0] != 0 )
        {
            pContext->serverIP = ct_lwm2m_strdup(ctiot_funcv1_old_params->serverIP);
            ECOMM_STRING(UNILOG_CTLWM2M, f2c_encode_params_0_1, P_INFO, "pContext->serverIP:%s",(uint8_t*)pContext->serverIP);
        }
        else
        {
            pContext->serverIP = NULL;
        }
    
        pContext->port         = ctiot_funcv1_old_params->port;
        pContext->localPort    = ctiot_funcv1_old_params->localPort;
        pContext->lifetime     = ctiot_funcv1_old_params->lifetime;
        pContext->securityMode = ctiot_funcv1_old_params->securityMode;
    
        pContext->idAuthMode         = ctiot_funcv1_old_params->idAuthMode;
        pContext->autoHeartBeat      = ctiot_funcv1_old_params->autoHeartBeat;
        pContext->onUqMode           = ctiot_funcv1_old_params->onUqMode;
        pContext->wakeupNotify       = ctiot_funcv1_old_params->wakeupNotify;
        pContext->protocolMode       = ctiot_funcv1_old_params->protocolMode;
        pContext->regFlag            = ctiot_funcv1_old_params->regFlag;
        
        pContext->reqhandle = ctiot_funcv1_old_params->reqhandle;
    }
    else//new type
    {
        ctiot_funcv1_params = (NV_new_params_t *)pcache;
    
        if( ctiot_funcv1_params->serverIP[0] != 0 )
        {
            pContext->serverIP = ct_lwm2m_strdup(ctiot_funcv1_params->serverIP);
            ECOMM_STRING(UNILOG_CTLWM2M, f2c_encode_params_1, P_INFO, "pContext->serverIP:%s",(uint8_t*)pContext->serverIP);
        }
        else
        {
            pContext->serverIP = NULL;
        }
        
#ifdef  FEATURE_REF_AT_ENABLE
        if(pContext->bsServerIP != NULL) 
            ct_lwm2m_free(pContext->bsServerIP);
    
        if(ctiot_funcv1_params->bsServerIPvalid == 0xA5 && strlen(ctiot_funcv1_params->bsServerIP) < SERVER_LEN)
        {
            pContext->bsServerIP = ct_lwm2m_strdup(ctiot_funcv1_params->bsServerIP);
            ECOMM_STRING(UNILOG_CTLWM2M, f2c_encode_params_2, P_INFO, "pContext->bsServerIP:%s",(uint8_t*)pContext->bsServerIP);
        }
        else
        {
            pContext->bsServerIP = NULL;
        }
        pContext->bsPort = ctiot_funcv1_params->bsPort;
        if(ctiot_funcv1_params->endpointValid == 0xA5)
        {
            pContext->endpoint = ct_lwm2m_strdup(ctiot_funcv1_params->endpoint);
            ECOMM_STRING(UNILOG_CTLWM2M, f2c_encode_params_3, P_INFO, "pContext->endpoint:%s",(uint8_t*)pContext->endpoint);
        }
        pContext->dtlsType     = ctiot_funcv1_params->dtlsType;
        pContext->nsmiFlag     = ctiot_funcv1_params->nsmiFlag;
#endif
        pContext->nnmiFlag     = ctiot_funcv1_params->nnmiFlag;
        pContext->natType      = ctiot_funcv1_params->natType;
        pContext->port         = ctiot_funcv1_params->port;
        pContext->localPort    = ctiot_funcv1_params->localPort;
        pContext->lifetime     = ctiot_funcv1_params->lifetime;
        pContext->securityMode = ctiot_funcv1_params->securityMode;
    
        pContext->idAuthMode         = ctiot_funcv1_params->idAuthMode;
        pContext->autoHeartBeat      = ctiot_funcv1_params->autoHeartBeat;
        pContext->onUqMode           = ctiot_funcv1_params->onUqMode;
        pContext->wakeupNotify       = ctiot_funcv1_params->wakeupNotify;
        pContext->protocolMode       = ctiot_funcv1_params->protocolMode;
        pContext->regFlag            = ctiot_funcv1_params->regFlag;
        
#ifdef WITH_MBEDTLS
        if( ctiot_funcv1_params->pskValid == 0xA5)
        {
            pContext->pskid = ct_lwm2m_strdup(ctiot_funcv1_params->psk_id);
            pContext->psk   = ct_lwm2m_strdup(ctiot_funcv1_params->psk);
            pContext->pskLen = strlen(pContext->psk);
        }
        else
        {
            pContext->pskid  = NULL;
            pContext->psk    = NULL;
            pContext->pskLen = 0;
        }
#endif
        
        pContext->reqhandle = ctiot_funcv1_params->reqhandle;

        if(ctiot_funcv1_params->imsiValid == 0xA5)//imsi has saved
        {
            strcpy(pContext->chipInfo->cImsi, ctiot_funcv1_params->imsi);
        }

#ifdef WITH_FOTA_RESUME
        if(ctiot_funcv1_params->fotaStatus == 3){
            fotaStateManagement.fotaState = 0;
        }else{
            fotaStateManagement.fotaState = ctiot_funcv1_params->fotaStatus;
        }
        blockNumber = ctiot_funcv1_params->fotaBlockNum;
        fotaUrl.port = ctiot_funcv1_params->fotaPort;
        ECOMM_TRACE(UNILOG_CTLWM2M, f2c_encode_params_4_1, P_INFO, 3, "fotaStateManagement.fotaState=%d, blockNumber=%d, fotaUrl.port=%d",fotaStateManagement.fotaState, blockNumber, fotaUrl.port);
        if( ctiot_funcv1_params->fotaAddress[0] != 0 )
        {
            strcpy(fotaUrl.address, ctiot_funcv1_params->fotaAddress);
            ECOMM_STRING(UNILOG_CTLWM2M, f2c_encode_params_4_2, P_INFO, "fotaUrl.fotaAddress:%s",(uint8_t*)fotaUrl.address);
        }
        
        if( ctiot_funcv1_params->fotaUri[0] != 0 )
        {
            strcpy(fotaUrl.uri, ctiot_funcv1_params->fotaUri);
            ECOMM_STRING(UNILOG_CTLWM2M, f2c_encode_params_4_3, P_INFO, "fotaUrl.fotaUri:%s",(uint8_t*)fotaUrl.uri);
        }
#endif
    }
    
    return NV_OK;
}

int32_t cache_get_bootflag(uint8_t *bootflag)
{
    NV_lwm2m_context_t   *lwm2m_context;
    logic_block_entry_t  *pentry;
    uint8_t              *pcache;
    
    pentry = NV_get_cache(block1_context);
    
    if(pentry->cache_valid == 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, cache_get_bootflag_1, P_INFO, 0, "cache_get_bootflag block1_context data invalid");
        return NV_ERR_CACHE_INVALID;
    }
    pcache = pentry->address;        
    lwm2m_context = (NV_lwm2m_context_t *)pcache;
    
    *bootflag = lwm2m_context->bootFlag;
    
    return NV_OK;
}

static lwm2m_server_t * utils_findServer(lwm2m_context_t * contextP,
                                  uint16_t shortID)
{
    lwm2m_server_t * targetP;

    targetP = contextP->serverList;
    while (targetP != NULL
        && shortID != targetP->shortID)
    {
        targetP = targetP->next;
    }

    return targetP;
}
                                  
int32_t f2c_encode_context(ctiot_funcv1_context_t *pContext)
{  
    NV_lwm2m_context_t       *lwm2m_context;
    NV_lwm2m_server_t        *server_data;
    NV_lwm2m_observed_t      *observed_data;
    NV_lwm2m_watcher_t       *watcher_data;
    NV_lwm2m_watch_server_t  *watch_server;
    uint8_t                  *pcache;
    lwm2m_server_t           *pser;
    lwm2m_observed_t         *pob;
    lwm2m_watcher_t          *pwat;

    logic_block_entry_t      *pentry;
    
    uint32_t i,ii,iii;

    if(pContext == NULL) 
        pContext=ctiot_funcv1_get_context();

    pentry = NV_get_cache(block1_context);

    
    if(pentry->cache_valid == 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, f2c_encode_context_0_0, P_ERROR, 0, "block1_context data invalid. should not go here!!!");
        return NV_ERR_CACHE_INVALID;
    }
    
    pcache = pentry->address;        
    lwm2m_context = (NV_lwm2m_context_t *)pcache;

    memcpy(pContext->localIP, lwm2m_context->localIP, sizeof(lwm2m_context->localIP));

    pContext->bootFlag      = lwm2m_context->bootFlag;
#ifdef FEATURE_REF_AT_ENABLE
    pContext->loginStatus   = (ctiot_funcv1_login_status_e)lwm2m_context->loginStatus;
#endif
    //ECOMM_TRACE(UNILOG_CTLWM2M, f2c_encode_context_0, P_INFO, 2, "lwm2m_context->server_cnt:%d observed_cnt=%d", lwm2m_context->server_cnt,lwm2m_context->observed_cnt);
    //goto next block
    pcache += sizeof(NV_lwm2m_context_t);

    //fill server list
    for(i = 0; i<lwm2m_context->server_cnt; i++)
    {
        pser = ct_lwm2m_malloc(sizeof(lwm2m_server_t));
        memset(pser, 0, sizeof(lwm2m_server_t));
        server_data        =(NV_lwm2m_server_t*)pcache;

        pser->next         = NULL;
        pser->secObjInstID = server_data->secObjInstID;
        pser->binding      = server_data->binding;
        pser->lifetime     = server_data->lifetime;
        pser->shortID      = server_data->shortID;
        pser->registration = server_data->registration;
        pser->sessionH     = NULL;
        
        if(server_data->location[0] != 0)
        {
            pser->location = ct_lwm2m_strdup(server_data->location);
        }
        else
        {
            pser->location = NULL;
        }
        
        pContext->lwm2mContext->serverList = (lwm2m_server_t *)LWM2M_LIST_ADD(pContext->lwm2mContext->serverList,pser);

        pcache += sizeof(NV_lwm2m_server_t);
    }

    //fill observed list
    for(i=0; i<lwm2m_context->observed_cnt; i++)
    {
        observed_data = (NV_lwm2m_observed_t *)pcache;
        pob           = ct_lwm2m_malloc(sizeof(lwm2m_observed_t));
        memset(pob, 0, sizeof(lwm2m_observed_t));
        
        pob->uri.flag       = observed_data->uri.flag;
        pob->uri.objectId   = observed_data->uri.objectId;
        pob->uri.instanceId = observed_data->uri.instanceId;
        pob->uri.resourceId = observed_data->uri.resourceId;
        pob->next           = NULL;
        pob->watcherList    = NULL;

        //ECOMM_TRACE(UNILOG_CTLWM2M, f2c_encode_context_1, P_INFO, 2, "observed_data->watcher_cnt=%d observed_data->uri.objectId=%d", observed_data->watcher_cnt,observed_data->uri.objectId);

        //goto next block
        pcache += sizeof(NV_lwm2m_observed_t);
        
        //fill watcher list
        for(ii = 0; ii<observed_data->watcher_cnt; ii++)
        {
            watcher_data     = (NV_lwm2m_watcher_t *)pcache;

            pwat             = ct_lwm2m_malloc(sizeof(lwm2m_watcher_t));
            memset(pwat, 0, sizeof(lwm2m_watcher_t));
            
            //param 0
            pwat->active                  = watcher_data->active;
            pwat->update                  = watcher_data->update;
            
            //param 1
            if( watcher_data->attr_flag == 1 )
            {
                pwat->parameters = ct_lwm2m_malloc(sizeof(lwm2m_attributes_t));
                memset(pwat->parameters, 0, sizeof(lwm2m_attributes_t));
            
                pwat->parameters->toClear     = watcher_data->toClear;
                pwat->parameters->toSet       = watcher_data->toSet;
                pwat->parameters->minPeriod   = watcher_data->minPeriod;
                pwat->parameters->maxPeriod   = watcher_data->maxPeriod;
                pwat->parameters->greaterThan = watcher_data->greaterThan;
                pwat->parameters->lessThan    = watcher_data->lessThan;
                pwat->parameters->step        = watcher_data->step;
            }
            
            //param 2
            pwat->format                  = watcher_data->format;
            pwat->tokenLen                = watcher_data->tokenLen;
            pwat->lastTime                = watcher_data->lastTime;
            pwat->counter                 = watcher_data->counter;
            memcpy(pwat->token, watcher_data->token, watcher_data->tokenLen);
            if(observed_data->uri.objectId == 19)
                ECOMM_TRACE(UNILOG_CTLWM2M, f2c_encode_context_2, P_INFO, 2, "update=%d,counter=%d", pwat->update,pwat->counter);
            //param 3
            pwat->lastMid                 = watcher_data->lastMid;
            pwat->lastValue.asInteger     = watcher_data->lastValue.asInteger;

            lwm2m_printf("watcher_data->server_cnt=%u\r\n", watcher_data->server_cnt);
            
            //goto next block
            pcache += sizeof(NV_lwm2m_watcher_t);
            
            //fill watcher server list
            for(iii = 0; iii<watcher_data->server_cnt; iii++)
            {   
                lwm2m_server_t  *serverP;
                watch_server = (NV_lwm2m_watch_server_t *)pcache;
             
                serverP = utils_findServer(pContext->lwm2mContext,  watch_server->shortID);
                if( serverP != NULL )
                {
                    pwat->server = (lwm2m_server_t *)LWM2M_LIST_ADD(pwat->server,serverP);
                }
                
                //goto next block
                pcache += sizeof(NV_lwm2m_watch_server_t);
            }

            pob->watcherList = (lwm2m_watcher_t *)LWM2M_LIST_ADD(pob->watcherList,pwat);
        }

        pContext->lwm2mContext->observedList = (lwm2m_observed_t *)LWM2M_LIST_ADD(pContext->lwm2mContext->observedList,pob);
    }

    return NV_OK;   
}

void NV_reinit_cache()
{
    ECOMM_TRACE(UNILOG_CTLWM2M, NV_reinit_cache, P_INFO, 0, "reinit cache");
    flash_cache = NULL;
    NV_init_cache();
}

