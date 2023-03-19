/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* coap-client -- simple CoAP client
 *
 * Copyright (C) 2010--2016 Olaf Bergmann <bergmann@tzi.org> and others
 *
 * This file is part of the CoAP library libcoap. Please see README for terms of
 * use.
 */


#ifndef COAP_CLIENT_H
#define COAP_CLIENT_H
 
#include "cmsis_os2.h"
#include "FreeRTOS.h" 
#include "queue.h"
#include "str.h"
#include "block.h"
#include "address.h"
#include "coap2/net.h"
#include "semphr.h"
#include "coap_session.h"
#include "resource.h"
#ifdef FEATURE_MBEDTLS_ENABLE
#include "aes.h"
#endif

#define COAP_URI_LEN   64
#define COAP_HOST_URI_LEN   32

#ifdef HAVE_MBEDTLS
#define COAP_TASK_RECV_STACK_SIZE   6300
#define COAP_TASK_SEND_STACK_SIZE   4900
#else
#define COAP_TASK_RECV_STACK_SIZE   2200
#define COAP_TASK_SEND_STACK_SIZE   2200
#endif

#define COAP_CLIENT_NUMB_MAX     1
#define COAP_CLIENT_ID_DEFAULT   0xff

#define COAP_MSG_TIMEOUT        500
#define COAP_SEND_TIMEOUT       1500

#define COAP_TAG_LEN_MAX   8
#define COAP_SEND_PACKET_MAX   8

#define COAP_ERR_ABRT          (-13)
#define COAP_ERR_RST           (-14)
#define COAP_ERR_CLSD          (-15)
#define COAP_ERR_BADE          (9)

#define URI_MAX_LEN         128

#define COAP_MSGTYPE_CON    0
#define COAP_MSGTYPE_NON    1
#define COAP_MSGTYPE_ACK    2
#define COAP_MSGTYPE_RST    3

#define COAP_OPT_LEN_IF_MATCH        8 /* C, opaque, 0-8 B, (none) */
#define COAP_OPT_LEN_URI_HOST        255 /* C, String, 1-255 B, destination address */
#define COAP_OPT_LEN_ETAG            4 /* E, opaque, 1-8 B, (none) */
#define COAP_OPT_LEN_IF_NONE_MATCH   2 /* empty, 0 B, (none) */
#define COAP_OPT_LEN_URI_PORT        2 /* C, uint, 0-2 B, destination port */
#define COAP_OPT_LEN_LOCATION_PATH   255 /* E, String, 0-255 B, - */
#define COAP_OPT_LEN_URI_PATH        255 /* C, String, 0-255 B, (none) */
#define COAP_OPT_LEN_CONTENT_FORMAT  2 /* E, uint, 0-2 B, (none) */
#define COAP_OPT_LEN_CONTENT_TYPE    COAP_OPT_LEN_CONTENT_FORMAT
#define COAP_OPT_LEN_MAXAGE          4 /* E, uint, 0--4 B, 60 Seconds */
#define COAP_OPT_LEN_URI_QUERY       255 /* C, String, 1-255 B, (none) */
#define COAP_OPT_LEN_ACCEPT          2 /* C, uint,   0-2 B, (none) */
#define COAP_OPT_LEN_LOCATION_QUERY  255 /* E, String,   0-255 B, (none) */
#define COAP_OPT_LEN_SIZE2           4 /* E, uint, 0-4 B, (none) */
#define COAP_OPT_LEN_PROXY_URI       1034 /* C, String, 1-1034 B, (none) */
#define COAP_OPT_LEN_PROXY_SCHEME    255 /* C, String, 1-255 B, (none) */
#define COAP_OPT_LEN_SIZE1           4 /* E, uint, 0-4 B, (none) */

/* option types from RFC 7641 */
#define COAP_OPT_LEN_OBSERVE         3 /* E, empty/uint, 0 B/0-3 B, (none) */
#define COAP_OPT_LEN_SUBSCRIPTION    COAP_OPT_LEN_OBSERVE

/* selected option types from RFC 7959 */
#define COAP_OPT_LEN_BLOCK2          3 /* C, uint, 0--3 B, (none) */
#define COAP_OPT_LEN_BLOCK1          3 /* C, uint, 0--3 B, (none) */

#define COAP_IS_CREATE               88
#define COAP_NOT_CREATE              0
#define COAP_RECV_OPT_BUF_SIZE            512

#define COAP_DTLS_PSK_KEY_LEN            66
#define COAP_DTLS_PSK_IDENTITY_LEN       156
#define COAP_DEV_INFO1_LEN       32
#define COAP_DEV_INFO2_LEN       48

#define COAP_ALI_AUTH_TOKEN_LEN          48  /*used for ali auth token*/
#define COAP_OPTION_ALI_AUTH_TOKEN       61  /*used for ali auth token*/
#define COAP_ALI_RANDOM_LEN              48  /*used for ali auth token*/
#define COAP_ALI_RANDOM_DEF              0x7FFFFFFF
#define COAP_OPTION_ALI_SEQ              2089  /*used for ali seq*/

enum COAP_RC
{
    COAP_OK = 100,
    COAP_ERR,
    COAP_RECREATE,
    COAP_FULL,
    COAP_CLIENT_ERR,
    COAP_URI_ERR,
    COAP_IP_ERR,
    COAP_NETWORK_ERR,
    COAP_DNS_ERR,  
    COAP_CONTEXT_ERR,
    COAP_SESSION_ERR,
    COAP_TASK_ERR,
    COAP_RECONNECT,
    COAP_SEND_NOT_END_ERR,
    COAP_SEND_CONTINUE,
    
    COAP_NULL,
};
enum COAP_MSG_CMD
{
    MSG_COAP_RESERVE,
    MSG_COAP_CREATE_CLIENT,
    MSG_COAP_DELETE_CLIENT, 
    MSG_COAP_SEND, 
    MSG_COAP_ADD_RES, 
    MSG_COAP_HEAD, 
    MSG_COAP_OPTION, 
    MSG_COAP_CONFIG, 
    MSG_COAP_ALISIGN, 
    
};

enum COAP_CLIENT
{
    COAP_CLIENT_NOT_USED,
    COAP_CLIENT_USED,  
    COAP_CLIENT_IS_CREATING,         
};

enum COAP_CONNECT
{
    COAP_CONN_NOT_CONNECTED,
    COAP_CONN_CONNECTED, 
    COAP_CONN_IS_CONNECTING, 
};

enum COAP_INIT
{
    COAP_NOT_INIT, 
    COAP_HAVE_INIT, 
};

enum COAP_STATUS
{
    COAP_TASK_STAT_NULL, 
    COAP_TASK_STAT_CREATE, 
    COAP_TASK_STAT_DELETE
};

enum COAP_OP
{
    COAP_TASK_OP_NULL, 
    COAP_TASK_OP_DELETE, 
    COAP_TASK_OP_SEND
};

enum COAP_CFG
{
    COAP_CFG_ADDRES, 
    COAP_CFG_HEAD, 
    COAP_CFG_OPTION,
    COAP_CFG_SHOW,
    COAP_CFG_CLOUD,
    COAP_CFG_ALISIGN,
};

enum COAP_SHOW_CFG
{
    COAP_SHOW_SHOWRA=1, 
    COAP_SHOW_SHOWRSPOPT, 
};

enum COAP_CLOUD_TYPE
{
    COAP_CLOUD_TYPE_ONENET = 1,
    COAP_CLOUD_TYPE_ALI,
    COAP_CLOUD_TYPE_ECLIPSE,  /*not use client id, user name, passwd*/
    COAP_CLOUD_TYPE_NORMAL,   /*need client id, user name, passwd*/
    COAP_CLOUD_TYPE_MAX
};

enum COAP_ENCRYP_TYPE
{
    COAP_ENCRYP_TYPE_MD5 = 1,
    COAP_ENCRYP_TYPE_SHA1,
    COAP_ENCRYP_TYPE_SHA256,   
    COAP_ENCRYP_TYPE_MAX
};

enum COAP_SEND_TYPE
{
    COAP_SEND_CTRLZ = 0,
    COAP_SEND_AT = 1,
};

#ifdef FEATURE_MBEDTLS_ENABLE
typedef struct {
    mbedtls_aes_context ctx;
    uint8_t iv[16];
    uint8_t key[16];
} platform_aes_t;
#endif

typedef struct
{
    int mode;
    int is_from;
    int dec_para1;
    int dec_para2;
    int dec_para3;
    char *str_para1;  
}coap_config;
 
typedef struct
{
     uint8_t cmd_type;
     bool isRestore;
     void * client_ptr;
     unsigned char coap_msgtype;
     int coap_method;
     char data_type;
     uint8_t raiFlag;
     unsigned int reqhandle;
     int id;
     char *ip;
     int port;
     int local_port;
     char *token;
     coap_config coapCnf;     
     coap_string_t coap_payload;
     coap_address_t coap_dst;
     int cmd_end_flag;
}coap_send_msg;

typedef struct
{
    int coap_id;
    int ret;
    int atHandle;
}coap_cnf_msg;

typedef struct
{
    int coap_id;
    int msg_type;
    int msg_method;
    int msg_id;
    int mode;
    unsigned int showra;
    unsigned int showrspopt;
    int server_port;
    unsigned int server_ip;    
    int payload_len;
    char *coap_payload;
    int token_len;
    char *coap_token;    
    void * client_ptr;
    int recv_opt_cnt;
    char *recv_optlist_buf;
}coap_message;

typedef struct
{
    char device_id[COAP_DEV_INFO1_LEN];
    char device_name[COAP_DEV_INFO1_LEN];
    char device_scret[COAP_DEV_INFO2_LEN];
    char product_key[COAP_DEV_INFO1_LEN];
    unsigned int seq;
    char psk_key[COAP_DEV_INFO2_LEN];
}coap_dev_info;

typedef struct
{
    int is_used;
    int is_connected;
    int coap_id;
    unsigned int coap_reqhandle;
    coap_string_t coap_proxy;   
    int coap_proxy_port;    
    int coap_ping_seconds;
    unsigned int coap_wait_ms;
    int coap_reliable;
    unsigned char coap_msgtype;
    int coap_method;
    int coap_block_flags;
    coap_block_t coap_block_cfg;    
    char *coap_ip; 
    unsigned int coap_port;
    unsigned int coap_local_port;
    int head_mode;
    int msg_id;
    coap_binary_t coap_token;
    coap_address_t coap_dst;
    coap_context_t *coap_ctx;
    coap_session_t *coap_session;
    coap_uri_t coap_uri_info;
    coap_optlist_t *coap_optlist;
    coap_string_t coap_payload;
    unsigned int showra;
    unsigned int showrspopt;
    unsigned int dtls_flag;
    unsigned int cloud;
    unsigned int encrypt;
    coap_resource_t *coap_resource;
    coap_dev_info coap_dev;
}coap_client;

typedef enum
{
    COAP_CREATE_FROM_AT,
    COAP_CREATE_FROM_RESTORE,
}CoapCreateFrom;

typedef struct
{
    CoapCreateFrom from;
    uint8_t coapId;
}CoapCreateInfo;

#define COAP_SLP_ENABLE         1       // enable coap sleep

#define COAP_DEEPSLP_THD        10000

typedef enum
{
    COAPSLP_WAIT_CREATE = 1,
    COAPSLP_WAIT_DELETE = 2,
    COAPSLP_WAIT_CONFIG = 4,
    COAPSLP_WAIT_NETWORK = 8,
}CoapSlpWait_e;

typedef struct
{
    uint8_t opt_used;
    uint8_t opt_name;
    char opt_value[32];    
}coapOption_t;

typedef struct
{
    uint8_t msg_used;
    uint8_t token_used;
    uint8_t mode;
    uint32_t msg_id;
    char token[10];    
}coapHead_t;

typedef struct
{
    uint8_t res_used;
    uint8_t res_len;
    char resource[50];    
}coapResource_t;

typedef struct
{
    uint8_t valid;
    uint8_t coap_id;
    uint32_t handle;
    uint32_t coap_local_port;
    uint32_t showra;
    uint32_t showrspopt;
    uint32_t dtls_flag;
    uint32_t cloud;
    uint32_t encrypt;
    coapResource_t coap_resource;
    coapHead_t coap_head;
    coapOption_t coap_opt[20];
    coap_dev_info coap_dev;
}coapSlpCtxBody_t;


typedef struct
{
    uint16_t restoreFlag;
    uint16_t body_len;
    uint16_t memcleared;
    uint16_t needRestoreConfig;
    coapSlpCtxBody_t body[COAP_CLIENT_NUMB_MAX];
}coapSlpCtx_t;



typedef struct
{
    uint32_t ctx_valid;
    coapSlpCtx_t coapSlpCtx;
}coapSlpNVMem_t;

typedef struct
{
    uint8_t wait_msg;
    uint8_t last_msg_type;
    uint8_t timeout_vaild;
    uint32_t timeout;
}coapClientSlpInfo_t;

int coap_client_create(UINT32 reqHandle, int localPort, CoapCreateInfo coapInfo);
int coap_client_delete(UINT32 reqHandle, int coapId);
int coap_client_send(UINT32 reqHandle, int coapId, int coapMsgType, int coapMethod, char *ipAddr,
                           int port, int coapPayloadLen, char *coapPayload, int coapSendMode, UINT8 coapRai);
int coap_client_addres(UINT32 reqHandle, int coapId, int resLen, char *resource);
int coap_client_head(UINT32 reqHandle, int coapId, int mode, int msgId, int tokenLen, char *token);
int coap_client_option(UINT32 reqHandle, int coapId, int mode, char *optValue, int cmdEndFlag);
int coap_client_status(UINT32 reqHandle, int coapId, INT32 *outValue);
int coap_client_config(UINT32 reqHandle, int coapId, int mode, int value1,int value2,int value3);
int coap_client_alisign(UINT32 reqHandle, int coapId, char* deviceId, char* deviceName, char*deviceScret, char* productKey, INT32 seq, char* signature);
int coap_recv_task_Init(void);

int coap_autoreg_create(coap_client *coapClient, int localPort, CoapCreateInfo coapInfo);
int coap_autoreg_recv(coap_client *coapClient);
int coap_autoreg_send(coap_client *coapClient, int coapMsgType, int coapMethod, char *ipAddr, int port, int coapPayloadLen, char *coapPayload);
int coap_autoreg_delete(coap_client *coapClient);

void coapSlpCheck2RestoreCtx(void);
void coapSlpInit(void);
void coapSlpDeInit(void);
void coapMaintainSlpInfo(uint8_t id, uint8_t msgType);
UINT32 coapCtxStoreNVMem(bool to_clear);
void coapAddTimeoutInfo(void *client, uint32_t timeout);
void coapSlpWaitMutex(CoapSlpWait_e state);
void coapSlpReleaseMutex(CoapSlpWait_e state);
bool coapCheckSafe2Sleep(void);

#endif

