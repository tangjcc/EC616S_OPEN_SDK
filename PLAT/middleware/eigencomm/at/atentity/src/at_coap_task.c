/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* coap-client -- simple CoAP client
 *
 * Copyright (C) 2010--2016 Olaf Bergmann <bergmann@tzi.org> and others
 *
 * This file is part of the CoAP library libcoap. Please see README for terms of
 * use.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#include "getopt.c"
#if !defined(S_ISDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#else
#include <unistd.h>
#include <netdb.h>
#endif
#include <assert.h>
#include <coap2/coap.h>
#include "at_coap_task.h"
#include "coap_config.h"
#include "atec_coap.h"
#include "netmgr.h"
#include "debug_log.h"
#include "coap_debug.h"
#ifdef FEATURE_MBEDTLS_ENABLE
#include "sha1.h"
#include "sha256.h"
#include "md5.h"
#include "aes.h"
#endif
#include "ec_coap_api.h"
#include "coap_adapter.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "lfs_port.h"
#include "address.h"
#include "cmsis_os2.h"
#include "osasys.h"
#include "netmgr.h"
#include "resource.h"
#include "coap_mbedtls.h"

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_USER 128 /* Maximum length of a user name (i.e., PSK
                      * identity) in bytes. */
#define MAX_KEY   64 /* Maximum length of a key (i.e., PSK) in bytes. */
#define BUFSIZE 40
#define FLAGS_BLOCK 0x01
#define hexchar_to_dec(c) ((c) & 0x40 ? ((c) & 0x0F) + 9 : ((c) & 0x0F))

typedef unsigned char method_t;
UINT32 coap_reqHandle = 0;
static char *coap_cert_file = NULL; /* Combined certificate and private key in PEM */
static char *coap_ca_file = NULL;   /* CA for cert_file - for cert checking in PEM */
uint16_t coap_last_block1_tid = 0;

unsigned int coap_wait_seconds = 90;                /* default timeout in seconds */
unsigned int coap_wait_ms = 0;
int coap_wait_ms_reset = 0;
int coap_obs_started = 0;
unsigned int coap_obs_seconds = 30;          /* default observe time */
unsigned int coap_obs_ms = 0;                /* timeout for current subscription */
int coap_obs_ms_reset = 0;

coap_optlist_t *coap_optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
coap_uri_t coap_uri_info;

/* reading is done when this flag is set */
int coap_ready = 0;

coap_string_t coap_payload = { 0, NULL };       /* optional payload to send */
method_t coap_method = 1;                    /* the method we are using in our requests */
coap_block_t coap_block_cfg = { .num = 0, .m = 0, .szx = 256 };

int coap_task_recv_status_flag = COAP_TASK_STAT_DELETE;
int coap_task_send_status_flag = COAP_TASK_STAT_DELETE;
int coap_smphr_status_flag = COAP_NOT_INIT;
extern int coap_send_packet_count;
extern int coap_send_packet_count_recv;
extern int coap_send_packet_count_send;

QueueHandle_t coap_send_msg_handle = NULL;
osThreadId_t coap_recv_task_handle = NULL;

osThreadId_t coap_send_task_handle = NULL;

extern coap_mutex coap_mutex1;
extern coap_mutex coap_mutex2;
extern int ali_hmac_falg;

coap_client coapClient[COAP_CLIENT_NUMB_MAX]={0};
coap_client *coapCurrClient = NULL;
#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE)
extern coap_client autoregCoapClient;
#endif

#define COAP_NV_CLEARED     0xAABB 

extern void coapCtxRestore(void);
extern coap_client coapClient[COAP_CLIENT_NUMB_MAX];
static coapClientSlpInfo_t coapSlpInfo[COAP_CLIENT_NUMB_MAX];
static coapSlpNVMem_t coapSlpNVMem;
uint8_t coapSlpHandler = 0xff;
uint8_t coapSlpMutexWait = 0x0;

struct context_t {
  unsigned int type;
  const char *name;
};

static struct context_t contextFormats[] = {
  { 0,  "0"    },/* "text/plain" */
  { 40, "40"   },/* "application/link-format" */ 
  { 41, "41"   },/* "application/xml" */ 
  { 42, "42"   },/* "application/octet-stream" */ 
  { 47, "47"   },/* "application/exi" */ 
  { 50, "50"   },/* "application/json" */ 
  { 60, "60"   },/* "application/cbor" */ 
  { 0xffff, NULL },
};

char *aliAuthToken = NULL; /*used for ali auth token*/
char *aliRandom = NULL;    /*used for ali psk mode*/
extern int autoRegSuccFlag;

extern  NBStatus_t appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *result);

#if COAP_SLP_ENABLE
extern void coapDelSlpCtx(int id, coap_client *client);
extern void coapAddSlpCtx(coap_client *client);
#endif
#ifdef FEATURE_MBEDTLS_ENABLE
void coap_ali_sha256(const uint8_t *input, uint32_t ilen, uint8_t output[32])
{
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, input, ilen);
    mbedtls_sha256_finish(&ctx, output);
    mbedtls_sha256_free(&ctx);
}

void *coap_ali_aes128_init(const uint8_t *key, const uint8_t *iv, uint8_t dir)
{
    int ret = 0;
    platform_aes_t *p_aes128 = NULL;

    if (!key || !iv) 
        return p_aes128;

    p_aes128 = (platform_aes_t *)calloc(1, sizeof(platform_aes_t));
    if (!p_aes128) 
        return p_aes128;

    mbedtls_aes_init(&p_aes128->ctx);
    if (dir == 0) 
    {
        ret = mbedtls_aes_setkey_enc(&p_aes128->ctx, key, 128);
    } 
    else 
    {
        ret = mbedtls_aes_setkey_dec(&p_aes128->ctx, key, 128);
    }

    if (ret == 0) 
    {
        memcpy(p_aes128->iv, iv, 16);
        memcpy(p_aes128->key, key, 16);
    } 
    else 
    {
        free(p_aes128);
        p_aes128 = NULL;
    }

    return (void *)p_aes128;
}

int coap_ali_aes128_cbc_encrypt(void *aes, const void *src, size_t blockNum, void *dst)
{
    int i   = 0;
    int ret = ret;
    platform_aes_t *p_aes128 = (platform_aes_t *)aes;

    if (!aes || !src || !dst) return -1;

    for (i = 0; i < blockNum; ++i) 
    {
        ret = mbedtls_aes_crypt_cbc(&p_aes128->ctx, MBEDTLS_AES_ENCRYPT, 16, p_aes128->iv, src, dst);
        src = (unsigned char *)src + 16;
        dst = (unsigned char *)dst + 16;
    }

    return ret;
}

int coap_ali_aes128_destroy(void *aes)
{
    if (!aes) return -1;

    mbedtls_aes_free(&((platform_aes_t *)aes)->ctx);

    free(aes);
    return 0;
}

int coap_ali_aes_cbc_encrypt(const unsigned char *src, int len, unsigned char *key, void *out)
{
    char *iv = "543yhjy97ae7fyfg";

    int len1 = len & 0xfffffff0;
    int len2 = len1 + 16;
    int pad = len2 - len;
    int ret = 0;
    void *aes_e_h = NULL;

    aes_e_h = coap_ali_aes128_init((unsigned char *)key, (unsigned char *)iv, 0);
    if (len1) 
    {
        ret = coap_ali_aes128_cbc_encrypt(aes_e_h, src, len1 >> 4, out);
    }
    
    if (!ret && pad) 
    {
        char buf[16] = {0};
        memcpy(buf, src + len1, len - len1);
        memset(buf + len - len1, pad, pad);
        ret = coap_ali_aes128_cbc_encrypt(aes_e_h, buf, 1, (unsigned char *)out + len1);
    }

    coap_ali_aes128_destroy(aes_e_h);

    return ret == 0 ? len2 : 0;
}

#endif

coap_client *coap_get_curr_client(void);

static coap_pdu_t *coap_new_request(coap_client *coapCurrClient,
                                            coap_context_t *ctx,
                                             coap_session_t *session,
                                             method_t m,
                                             coap_optlist_t **options,
                                             unsigned char *data,
                                             size_t length)
{
    coap_pdu_t *pdu;
    (void)ctx;
    unsigned char buff[32] = {0};
    unsigned char seq[33] = {0};
    int len = 0;

    if(NULL == (pdu = coap_new_pdu(session)))
        return NULL;

    pdu->type = coapCurrClient->coap_msgtype;
    pdu->tid  = coap_new_message_id(session);
    if(m > 200)
    {
        pdu->code = COAP_RESPONSE_CODE(m);
    }
    else
    {
        pdu->code = m;
    }
    
    if(coapCurrClient->coap_token.s != NULL)
    {
        if(!coap_add_token(pdu, coapCurrClient->coap_token.length, coapCurrClient->coap_token.s))
        {
            coap_log(LOG_DEBUG, "cannot add token to request\n");
        }

        free(coapCurrClient->coap_token.s);
        coapCurrClient->coap_token.s = NULL;
    }
    //add for AT+COAPHEAD
    if((coapCurrClient->head_mode == 3)||(coapCurrClient->head_mode == 4)||(coapCurrClient->head_mode == 5))
    {
        pdu->tid  = coapCurrClient->msg_id;
    }
    
    if(coapCurrClient->cloud == COAP_CLOUD_TYPE_ALI)
    {
        if(aliAuthToken != NULL)
        {
            ECOMM_STRING(UNILOG_COAP, coap_msg_str1, P_INFO, "....0.aliAthToken..%s.....", (UINT8 *)aliAuthToken);
            coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_ALI_AUTH_TOKEN, strlen(aliAuthToken), (uint8_t *)aliAuthToken));
        }

        if(aliRandom != NULL)
        {
            ECOMM_STRING(UNILOG_COAP, coap_msg_str122, P_INFO, "....0.aliRandom..%s.....", (UINT8 *)aliRandom);
            
            sprintf((char *)buff, "%d", coapCurrClient->coap_dev.seq++);
            len = coap_ali_aes_cbc_encrypt(buff, strlen((char *)buff), (unsigned char *)(coapCurrClient->coap_dev.psk_key), seq);
            
            coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_ALI_SEQ, len, (uint8_t *)seq));
        }        
    }
    
    if(options)
    {
        coap_add_optlist_pdu(pdu, options);
    }
    
    if(length)
    {   
        if(ctx->resources != NULL)
        {
            ctx->resources->user_data = data;//pdu->data;
        }
        
        if((coapCurrClient->coap_block_flags & FLAGS_BLOCK) == 0)
        {
            coap_add_data(pdu, length, data);
        }
        else
        {
            coap_add_block(pdu, length, data, coapCurrClient->coap_block_cfg.num, coapCurrClient->coap_block_cfg.szx);
        }
    }

    return pdu;
}

static void coap_message_handler(struct coap_context_t *ctx,
                                coap_session_t *session,
                                coap_pdu_t *sent,
                                coap_pdu_t *received,
                                const coap_tid_t id UNUSED_PARAM)
{
    coap_opt_iterator_t opt_iter;
    unsigned char buf[8] = {0};
    coap_opt_t *recv_option;
    coap_client *coapCurrClient = coap_get_curr_client();
    coap_message coap_msg;
    int msg_len = 0;
    int primSize = 0;
    int recv_opt_len = 0;
    int opt_len = 0;
    int content_format = -1;
    int i = 0;
    int currOptType = 0;
    int recvOptCnt = 0;
    char * tempPtr = NULL;
    char * tempEndPtr = NULL;
    char * coap_etag = NULL;
    
#ifndef NDEBUG
    coap_log(LOG_DEBUG, "** process incoming %d.%02d response:\n", (received->code >> 5), received->code & 0x1F);
    if (coap_get_log_level() < LOG_DEBUG)
        coap_show_pdu(LOG_INFO, received);
#endif
        switch (received->type)
        {
          case COAP_MESSAGE_ACK:
          case COAP_MESSAGE_RST:
          case COAP_MESSAGE_NON:
          case COAP_MESSAGE_CON:
              memset(&coap_msg, 0, sizeof(coap_message));
              coap_msg.coap_id = coapCurrClient->coap_id;
              coap_msg.showra = coapCurrClient->showra;
              coap_msg.showrspopt = coapCurrClient->showrspopt;
              coap_msg.msg_type = received->type;
              coap_msg.msg_method = received->code;
              coap_msg.msg_id = received->tid;
              coap_msg.token_len = received->token_length;
              if(coap_msg.token_len > 0)
              {
                  coap_msg.coap_token = malloc(coap_msg.token_len+1);
                  memset(coap_msg.coap_token, 0, coap_msg.token_len+1);
                  memcpy(coap_msg.coap_token, received->token, coap_msg.token_len);
              }
              coap_msg.server_port = session->remote_addr.addr.sin.sin_port;
              coap_msg.server_ip = session->remote_addr.addr.sin.sin_addr.s_addr;
              coap_msg.client_ptr = (void *)coapCurrClient;

              if(received->data != NULL)
              {
                  msg_len = strlen((char *)received->data);
                  coap_msg.coap_payload = malloc(msg_len+1);
                  memset(coap_msg.coap_payload, 0 ,(msg_len+1));
                  memcpy(coap_msg.coap_payload, received->data, msg_len);
              }
              primSize = (msg_len+1)+sizeof(coap_message);
              coap_msg.payload_len = msg_len;

              coap_option_iterator_init(received, &opt_iter, COAP_OPT_ALL);
              coap_msg.recv_optlist_buf = malloc(COAP_RECV_OPT_BUF_SIZE);
              memset(coap_msg.recv_optlist_buf, 0, COAP_RECV_OPT_BUF_SIZE);
              //ECOMM_TRACE(UNILOG_COAP, coap_msg_00, P_INFO, 3, ".....recv_option....0x%x..%d...%d.",recv_option,recv_opt_len,opt_iter.type);
              while ((recv_option = coap_option_next(&opt_iter)) != NULL) 
              {
                  recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                  currOptType = coap_opt_delta(recv_option)+currOptType;
                  ECOMM_TRACE(UNILOG_COAP, coap_msg_1, P_INFO, 3, ".....recv_option....0x%x..%d...%d.",recv_option,recv_opt_len,opt_iter.type);
                  switch (opt_iter.type) 
                  {
                      case COAP_OPTION_CONTENT_FORMAT:
                      case COAP_OPTION_ACCEPT:
                        recvOptCnt++;
                        content_format = (int)coap_decode_var_bytes(coap_opt_value(recv_option), coap_opt_length(recv_option));
                        while(contextFormats[i].type != 0xffff)
                        {
                            if(content_format == contextFormats[i].type)
                            {
                                opt_len = coap_opt_length(recv_option);
                                snprintf((char *)&coap_msg.recv_optlist_buf[recv_opt_len], COAP_RECV_OPT_BUF_SIZE-recv_opt_len, "%d,\"%s\",", currOptType,contextFormats[i].name);
                                break;
                            }
                            i++;
                        }
                        break;
                  
                      case COAP_OPTION_BLOCK1:
                      case COAP_OPTION_BLOCK2:
                        recvOptCnt++;
                        /* split block option into number/more/size where more is the
                         * letter M if set, the _ otherwise */
                        snprintf((char *)&coap_msg.recv_optlist_buf[recv_opt_len], COAP_RECV_OPT_BUF_SIZE-recv_opt_len, "%d,\"%u/%c/%u\"",currOptType,
                                           (unsigned int)coap_opt_block_num(recv_option), /* block number */
                                           COAP_OPT_BLOCK_MORE(recv_option) ? 'M' : '_', /* M bit */
                                           (unsigned int)(1 << (COAP_OPT_BLOCK_SZX(recv_option) + 4))); /* block size */
                        break;
                  
                      case COAP_OPTION_URI_PORT:
                      case COAP_OPTION_MAXAGE:
                      case COAP_OPTION_OBSERVE:
                      case COAP_OPTION_SIZE1:
                      case COAP_OPTION_SIZE2:
                        recvOptCnt++;
                        /* show values as unsigned decimal value */
                        opt_len = coap_opt_length(recv_option);
                        snprintf((char *)&coap_msg.recv_optlist_buf[recv_opt_len], COAP_RECV_OPT_BUF_SIZE-recv_opt_len, "%d,", currOptType);
                        recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                        coap_msg.recv_optlist_buf[recv_opt_len] = '"';
                        
                        sprintf((char *)buf,  "%u", coap_decode_var_bytes(coap_opt_value(recv_option), coap_opt_length(recv_option)));
                        strcat(coap_msg.recv_optlist_buf, (CHAR *)buf);
                        
                        recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                        coap_msg.recv_optlist_buf[recv_opt_len] = '"';
                        coap_msg.recv_optlist_buf[recv_opt_len+1] = ',';
                        break;
                  
                      case COAP_OPTION_URI_PATH:
                      case COAP_OPTION_PROXY_URI:
                      case COAP_OPTION_PROXY_SCHEME:
                      case COAP_OPTION_URI_HOST:
                      case COAP_OPTION_LOCATION_PATH:
                      case COAP_OPTION_LOCATION_QUERY:
                      case COAP_OPTION_URI_QUERY:
                        recvOptCnt++;
                        /* generic output function for all other option types */
                        opt_len = coap_opt_length(recv_option);
                        snprintf((char *)&coap_msg.recv_optlist_buf[recv_opt_len], COAP_RECV_OPT_BUF_SIZE-recv_opt_len, "%d,", currOptType);
                        recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                        coap_msg.recv_optlist_buf[recv_opt_len] = '"';
                        memcpy((char *)&coap_msg.recv_optlist_buf[recv_opt_len+1], coap_opt_value(recv_option), opt_len);
                        recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                        coap_msg.recv_optlist_buf[recv_opt_len] = '"';
                        coap_msg.recv_optlist_buf[recv_opt_len+1] = ',';
                        break;

                      case COAP_OPTION_IF_MATCH:
                      case COAP_OPTION_ETAG:
                      case COAP_OPTION_IF_NONE_MATCH:
                        recvOptCnt++;
                        /* generic output function for all other option types */
                        opt_len = coap_opt_length(recv_option);
                        snprintf((char *)&coap_msg.recv_optlist_buf[recv_opt_len], COAP_RECV_OPT_BUF_SIZE-recv_opt_len, "%d,", currOptType);
                        recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                        coap_msg.recv_optlist_buf[recv_opt_len] = '"';
                        coap_etag = malloc(opt_len*2+2);
                        memset(coap_etag, 0, (opt_len*2+2));
                        cmsHexToHexStr(coap_etag, opt_len*2+1, (const UINT8 *)coap_opt_value(recv_option), opt_len);
                        opt_len = strlen(coap_etag);

                        memcpy((char *)&coap_msg.recv_optlist_buf[recv_opt_len+1], coap_etag, opt_len);
                        free(coap_etag);
                        recv_opt_len = strlen(coap_msg.recv_optlist_buf);
                        coap_msg.recv_optlist_buf[recv_opt_len] = '"';
                        coap_msg.recv_optlist_buf[recv_opt_len+1] = ',';
                        break;
                      }
                    ECOMM_STRING(UNILOG_COAP, coap_msg_str2, P_INFO, ".....recv coap opt..%s.....", (UINT8 *)coap_msg.recv_optlist_buf);
              }
              coap_msg.recv_opt_cnt = recvOptCnt;
              ECOMM_TRACE(UNILOG_COAP, coap_msg_2, P_INFO, 0, ".....recv coap packet.......");
              ECOMM_STRING(UNILOG_COAP, coap_msg_str4, P_INFO, ".....recv coap packet..%s.....", (UINT8 *)received->data);
              //ECOMM_STRING(UNILOG_COAP, coap_msg_str1, P_INFO, ".....recv coap opt..%s.....", (UINT8 *)coap_msg.recv_optlist_buf);
              if(coapCurrClient->cloud == COAP_CLOUD_TYPE_ALI)
              {
                if((tempPtr=strstr((const char *)received->data,"\"token\":\"")) != NULL)
                {
                    if(aliAuthToken != NULL)
                    {
                        free(aliAuthToken);
                        aliAuthToken = NULL;
                    }
                    aliAuthToken = malloc(COAP_ALI_AUTH_TOKEN_LEN);
                    memset(aliAuthToken, 0, COAP_ALI_AUTH_TOKEN_LEN);
                    tempPtr = tempPtr+9;
                    tempEndPtr = strstr((const char *)tempPtr,"\"");
                    memcpy(aliAuthToken, tempPtr, (tempEndPtr-tempPtr));
                    ECOMM_STRING(UNILOG_COAP, coap_msg_str5, P_INFO, ".....aliAthToken..%s.....", (UINT8 *)aliAuthToken);
                }

                if((tempPtr=strstr((const char *)received->data,"\"random\":\"")) != NULL)
                {
                    if(aliRandom != NULL)
                    {
                        free(aliRandom);
                        aliRandom = NULL;
                    }
                    aliRandom = malloc(COAP_ALI_RANDOM_LEN);
                    memset(aliRandom, 0, COAP_ALI_RANDOM_LEN);
                    tempPtr = tempPtr+10;
                    tempEndPtr = strstr((const char *)tempPtr,"\"");
                    memcpy(aliRandom, tempPtr, (tempEndPtr-tempPtr));
                    ECOMM_STRING(UNILOG_COAP, coap_msg_str5, P_INFO, ".....aliAthToken..%s.....", (UINT8 *)aliAuthToken);
                }                
              }
              
              if(strstr((const char *)received->data, "\"resultDesc\":\"Success\"") != NULL)
              {
                  autoRegSuccFlag = 1;
              }
              applSendCmsInd(coapCurrClient->coap_reqhandle, APPL_COAP, APPL_COAP_RECV_IND, primSize, (void *)&coap_msg);

              #if COAP_SLP_ENABLE
              coapMaintainSlpInfo(coapCurrClient->coap_id,received->type);
              #endif
            break;
        }

}

/* Called after processing the options from the commandline to set
 * Block1 or Block2 depending on method. */
static void set_blocksize(void)
{
    static unsigned char buf[4];        /* hack: temporarily take encoded bytes */
    uint16_t opt;
    unsigned int opt_length;

    if (coap_method != COAP_REQUEST_DELETE)
    {
        opt = coap_method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 : COAP_OPTION_BLOCK1;

        coap_block_cfg.m = (opt == COAP_OPTION_BLOCK1) && ((1u << (coap_block_cfg.szx + 4)) < coap_payload.length);

        opt_length = coap_encode_var_safe(buf, sizeof(buf), (coap_block_cfg.num << 4 | coap_block_cfg.m << 3 | coap_block_cfg.szx));

        coap_insert_optlist(&coap_optlist, coap_new_optlist(opt, opt_length, buf));
    }
}

static int verify_cn_callback(const char *cn,
                           const uint8_t *asn1_public_cert UNUSED_PARAM,
                           size_t asn1_length UNUSED_PARAM,
                           coap_session_t *session UNUSED_PARAM,
                           unsigned depth,
                           int validated UNUSED_PARAM,
                           void *arg UNUSED_PARAM
                        )
{
  coap_log(LOG_INFO, "CN '%s' presented by server (%s)\n", cn, depth ? "CA" : "Certificate");
  return 1;
}

static coap_dtls_pki_t *setup_pki(void)
{
    static coap_dtls_pki_t dtls_pki;
    static char client_sni[256];

    memset (&dtls_pki, 0, sizeof(dtls_pki));
    dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
    if(coap_ca_file)
    {
        /*
        * Add in additional certificate checking.
        * This list of enabled can be tuned for the specific
        * requirements - see 'man coap_encryption'.
        */
        dtls_pki.verify_peer_cert        = 1;
        dtls_pki.require_peer_cert       = 1;
        dtls_pki.allow_self_signed       = 1;
        dtls_pki.allow_expired_certs     = 1;
        dtls_pki.cert_chain_validation   = 1;
        dtls_pki.cert_chain_verify_depth = 2;
        dtls_pki.check_cert_revocation   = 1;
        dtls_pki.allow_no_crl            = 1;
        dtls_pki.allow_expired_crl       = 1;
        dtls_pki.validate_cn_call_back   = verify_cn_callback;
        dtls_pki.cn_call_back_arg        = NULL;
        dtls_pki.validate_sni_call_back  = NULL;
        dtls_pki.sni_call_back_arg       = NULL;
        memset(client_sni, 0, sizeof(client_sni));
        if (coap_uri_info.host.length)
            memcpy(client_sni, coap_uri_info.host.s, min(coap_uri_info.host.length, sizeof(client_sni)));
        else
            memcpy(client_sni, "localhost", 9);
        dtls_pki.client_sni = client_sni;
    }
    dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM;
    dtls_pki.pki_key.key.pem.public_cert = coap_cert_file;
    dtls_pki.pki_key.key.pem.private_key = coap_cert_file;
    dtls_pki.pki_key.key.pem.ca_file = coap_ca_file;
    return &dtls_pki;
}

static coap_session_t *get_session(
                                      coap_context_t *ctx,
                                      const char *local_addr,
                                      const char *local_port,
                                      coap_proto_t proto,
                                      coap_address_t *dst,
                                      const char *identity,
                                      const uint8_t *key,
                                      unsigned key_len
                                    )
{
    coap_session_t *session = NULL;
#if 0
    /* If general root CAs are defined */
    if (root_ca_file)
    {
        struct stat stbuf;
        if ((stat(root_ca_file, &stbuf) == 0) && S_ISDIR(stbuf.st_mode))
        {
            coap_context_set_pki_root_cas(ctx, NULL, root_ca_file);
        }
        else
        {
            coap_context_set_pki_root_cas(ctx, root_ca_file, NULL);
        }
    }
#endif

    if(local_addr)
    {
        int s;
        struct addrinfo hints;
        struct addrinfo *result = NULL, *rp;

        memset(&hints, 0, sizeof( struct addrinfo ));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = COAP_PROTO_RELIABLE(proto) ? SOCK_STREAM : SOCK_DGRAM; /* Coap uses UDP */
        hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;

        s = getaddrinfo(local_addr, local_port, &hints, &result);
        if(s != 0)
        {
            //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            return NULL;
        }

        /* iterate through results until success */
        for(rp = result; rp != NULL; rp = rp->ai_next)
        {
            coap_address_t bind_addr;
            if(rp->ai_addrlen <= sizeof(bind_addr.addr))
            {
                coap_address_init( &bind_addr );
                bind_addr.size = rp->ai_addrlen;
                memcpy(&bind_addr.addr, rp->ai_addr, rp->ai_addrlen);
                if(coap_cert_file && (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS))
                {
                    coap_dtls_pki_t *dtls_pki = setup_pki();
                    session = coap_new_client_session_pki(ctx, &bind_addr, dst, proto, dtls_pki);
                }
                else if((identity || key) && (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS))
                {
                    session = coap_new_client_session_psk(ctx, &bind_addr, dst, proto, identity, key, key_len);
                }
                else
                {
                    session = coap_new_client_session(ctx, &bind_addr, dst, proto);
                }

                if(session)
                break;
            }
        }
        freeaddrinfo( result );
    }
    else
    {
        if(coap_cert_file && (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS))
        {
            coap_dtls_pki_t *dtls_pki = setup_pki();
            session = coap_new_client_session_pki(ctx, NULL, dst, proto, dtls_pki);
        }
        else if((identity || key) && (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS))
        {
            session = coap_new_client_session_psk(ctx, NULL, dst, proto, identity, key, key_len);
        }
        else
        {
            session = coap_new_client_session(ctx, NULL, dst, proto);
        }
    }
    return session;
}
int coap_delete_optlist_node(coap_optlist_t ** coap_optlist, int type)
{
    coap_optlist_t * coap_optlist_temp = NULL;

    if(*coap_optlist != NULL)
    {
        if((*coap_optlist)->number == type)
        {
            coap_optlist_temp = (*coap_optlist)->next;
            free(*coap_optlist);
            *coap_optlist = coap_optlist_temp;
        }
        else
        {
            while((*coap_optlist)->next != NULL)
            {
                if((*coap_optlist)->next->number == type)
                {
                    coap_optlist_temp = (*coap_optlist)->next->next;
                    free((*coap_optlist)->next);
                    (*coap_optlist)->next = coap_optlist_temp;
                    break;
                }
            }
        }
    }
    return 0;
}
int eccoap_loghandler(coap_log_t level, char *message)
{
    uint8_t loglevel;

    if(level<=LOG_ERR)
        loglevel = P_ERROR;
    else if(level<=LOG_WARNING)
        loglevel = P_WARNING;
    else if(level<=LOG_NOTICE)
        loglevel = P_SIG;
    else
        loglevel = P_VALUE;

    ECOMM_STRING(UNILOG_ONENET, ec_coap_0, loglevel, "Coap:%s", (uint8_t*)message);
    return 0;
}

UINT8 *coap_token_decode(char* data,int datalen)
{
    int bufferlen = datalen/2;
    UINT8* buffer = NULL;
    int i=0;
    
    for(i=0; i<datalen; i++)
    {
        if((data[i]<'0')||((data[i]>'9')&&(data[i]<'A'))||((data[i]>'F')&&(data[i]<'a'))||(data[i]>'f'))
        {
            return NULL;
        }
    }
    buffer=malloc(bufferlen+1);
        
    if(buffer!=NULL)
    {
        char tmpdata;
        UINT8 tmpValue=0;
        i = 0;
        memset(buffer, 0, bufferlen+1);
        while(i<datalen)
        {
            tmpdata=data[i];
            if(tmpdata<='9' && tmpdata>='0')
            {
                tmpValue=tmpdata-'0';
            }
            else if(tmpdata<='F' && tmpdata>='A')
            {
                tmpValue=tmpdata-'A'+10;
            }
            else if(tmpdata<='f' && tmpdata>='a')
            {
                tmpValue=tmpdata-'a'+10;
            }
            if(i%2==0)
            {
                buffer[i/2] = tmpValue << 4;
            }
            else
            {
                buffer[i/2] |= tmpValue;
            }
            i++;
            tmpdata=data[i];
            if(tmpdata<='9' && tmpdata>='0')
            {
                tmpValue=tmpdata-'0';
            }
            else if(tmpdata<='F' && tmpdata>='A')
            {
                tmpValue=tmpdata-'A'+10;
            }
            else if(tmpdata<='f' && tmpdata>='a')
            {
                tmpValue=tmpdata-'a'+10;
            }
            if(i%2==0)
            {
                buffer[i/2] = tmpValue << 4;
            }
            else
            {
                buffer[i/2] |= tmpValue;
            }
            i++;
        }
    }
    return buffer;
}

#define COAP_CLIENT_AT_INTERFACE

int coap_get_dtls_key(UINT32 cloudType, UINT32 encrypType, char* deviceId, char* deviceName, char*deviceScret, char* productKey, char* psk_key, char* psk_identity)
{
    char *ali_clientID = NULL;
    char *ali_username = NULL;
    char *ali_signature = NULL;
    char *hmac_source = NULL;
    int ali_signature_len = 0;
    char *timestamp = "2524608000000";
    int ret = COAP_OK;
    
    if(cloudType == COAP_CLOUD_TYPE_ALI)
    {
        if(ali_hmac_falg == ALI_HMAC_NOT_USED)
        {
            return COAP_CLIENT_ERR;
        }
    
        hmac_source = malloc(256);
        if(hmac_source == NULL)
        {
            return COAP_CLIENT_ERR;
        }
        ali_clientID = malloc(128);
        if(ali_clientID == NULL)
        {
            free(hmac_source);
            return COAP_CLIENT_ERR;
        }
        ali_username = malloc(64);
        if(ali_username == NULL)
        {
            free(hmac_source);
            free(ali_clientID);
            return COAP_CLIENT_ERR;
        }
        ali_signature = malloc(64);
        if(ali_signature == NULL)
        {
            free(hmac_source);
            free(ali_clientID);
            free(ali_username);
            return COAP_CLIENT_ERR;
        }
        
        memset(ali_clientID, 0, 128);
        memset(ali_username, 0, 64);
        memset(ali_signature, 0, 64);
        
        snprintf(hmac_source, 256, "id%s&%stimestamp%s", productKey, deviceName, timestamp);
        
        #ifdef FEATURE_MBEDTLS_ENABLE
        switch(encrypType)
        {
            case COAP_ENCRYP_TYPE_MD5:
                atAliHmacMd5((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)deviceScret, strlen(deviceScret));
                snprintf(psk_identity, COAP_DTLS_PSK_IDENTITY_LEN, "devicename|hmacmd5|%s&%s|%s", productKey, deviceName, timestamp);               
                break;
                
            case COAP_ENCRYP_TYPE_SHA1:
                atAliHmacSha1((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)deviceScret, strlen(deviceScret));
                snprintf(psk_identity, COAP_DTLS_PSK_IDENTITY_LEN, "devicename|hmacsha1|%s&%s|%s", productKey, deviceName, timestamp);               
                break;
                
            case COAP_ENCRYP_TYPE_SHA256:
                atAliHmacSha256((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)deviceScret, strlen(deviceScret));
                snprintf(psk_identity, COAP_DTLS_PSK_IDENTITY_LEN, "devicename|hmacsha256|%s&%s|%s", productKey, deviceName, timestamp); 
                break;

            default:
                ret = COAP_ERR;
                break;

        }
        #endif
            
        ali_signature_len = strlen(ali_signature);
        if(cmsHexToHexStr((CHAR *)psk_key, ali_signature_len*2+1, (UINT8 *)ali_signature, ali_signature_len) < 0)
        {
            ret = COAP_ERR;
        }
    
        free(hmac_source);
        free(ali_clientID);
        free(ali_username);
        free(ali_signature);
    }
    else if(cloudType == COAP_CLOUD_TYPE_ONENET)
    {


    }
    else
    {

    }
    
    return ret;
}

int coap_check_ip_type(char *coapIp, int *rec)
{
    char *ip_ptr = coapIp;
    int i=0;
    int count=0;
    int len = 0;

    len = strlen(coapIp);

    for(i=0;i<len;i++)
    {
        if(*(ip_ptr+i) == '.')
        {
            count++;
        }
    }

    if(count == 0)
    {
        for(i=0;i<len;i++)
        {
            if(*(ip_ptr+i) == ':')
            {
                count++;
            }
        }
        if(count == 7)
        {
            *rec = AF_INET6;
        }
        else
        {
            return COAP_ERR;
        }

    }
    else if(count == 3)
    {
        *rec = AF_INET;
    }
    else
    {
        return COAP_ERR;
    }

    return COAP_OK;
}


coap_client *coap_new_client(unsigned int localPort)
{
    int i = 0;

    for(i=0; i<COAP_CLIENT_NUMB_MAX; i++)
    {
        if(coapClient[i].is_used == COAP_CLIENT_NOT_USED)
        {
            memset(&coapClient[i], 0, sizeof(coap_client));

            coapClient[i].coap_msgtype = COAP_MESSAGE_CON;
            coapClient[i].coap_method = 1;
            coapClient[i].coap_block_flags = 0;
            coapClient[i].coap_block_cfg.num = 0;
            coapClient[i].coap_block_cfg.m = 0;
            coapClient[i].coap_block_cfg.szx = 6;
            coapClient[i].coap_token.length = 0;
            coapClient[i].coap_token.s = NULL;
            coapClient[i].coap_payload.length = 0;
            coapClient[i].coap_payload.s = NULL;
            coapClient[i].coap_optlist = NULL;
            coapClient[i].coap_proxy.length = 0;
            coapClient[i].coap_proxy.s = NULL;
            coapClient[i].coap_proxy_port = COAP_DEFAULT_PORT;
            coapClient[i].is_used = COAP_CLIENT_USED;
            coapClient[i].coap_id = 0;
            coapClient[i].coap_port = 0;
            coapClient[i].coap_local_port = localPort;
            coapClient[i].coap_ip = NULL;

            //coapClient[i].coap_resource= coap_resource_init(NULL, 0);
            return &coapClient[i];
        }
    }

    return NULL;
}

int coap_delete_client(coap_client *coapClient)
{
    int ret = COAP_ERR;

    if(coapClient == NULL)
    {
        return COAP_ERR;
    }
    coapClient->is_used      = COAP_CLIENT_NOT_USED;
    coapClient->coap_id      = COAP_CLIENT_ID_DEFAULT;

    if(coapClient->coap_ip != NULL)
    {
        free(coapClient->coap_ip);
        coapClient->coap_ip = NULL;
        ret = COAP_OK;
    }

    if(coapClient->coap_optlist != NULL)
    {
        coap_delete_optlist(coapClient->coap_optlist);
        coapClient->coap_optlist = NULL;
        ret = COAP_OK;
    }
    if(coapClient->coap_resource != NULL)
    {
        //coap_delete_all_resources(coapClient->coap_ctx);
        //free(coapClient->coap_resource);
        //coapClient->coap_resource = NULL;
        ret = COAP_OK;
    }

    if(coapClient->coap_session != NULL)
    {
        coap_session_release(coapClient->coap_session);
        coapClient->coap_session = NULL;
        ret = COAP_OK;
    }

    if(coapClient->coap_ctx != NULL)
    {
        coap_free_context(coapClient->coap_ctx);
        coapClient->coap_ctx = NULL;
        ret = COAP_OK;
    }

    coap_cleanup();

    return ret;
}

coap_client *coap_find_client(int coapId)
{
    int i = 0;

    for(i=0; i<COAP_CLIENT_NUMB_MAX; i++)
    {
        if((coapClient[i].coap_id == coapId)&&(coapClient[i].is_used == COAP_CLIENT_USED))
        {
            return &coapClient[i];
        }
    }
    return NULL;
}


int coap_check_client(int localPort)
{
    int i = 0;

    for(i=0; i<COAP_CLIENT_NUMB_MAX; i++)
    {
        if(coapClient[i].is_used == COAP_CLIENT_USED)
        {
            if(coapClient[i].coap_local_port == localPort)
            {
                return COAP_ERR;
            }
        }
    }

    return COAP_OK;
}

int coap_config_client(int coapId, coap_config *coapCnf)
{
    int i = 0;
    coap_client *coapCurrClient = NULL;
    coapCurrClient = coap_find_client(coapId);
    int subUriLen = 0;
    int subUriLenTemp = 0;
    int subUriNumb = 1;
    char *chrFind = NULL;
    coap_optlist_t *coap_optlist_temp = NULL;
    int optLen = 0;
    int mode = 0;
    int msgId = 0;
    char *token = NULL;
    int tokenLenTemp = 0;
    int ret = COAP_OK;
    coap_resource_t *coap_resource = NULL;
    char *optValue = coapCnf->str_para1;
    UINT8 optBuf2[2] = {0};
    UINT8 optBuf3[3] = {0};
    UINT8 optBuf4[4] = {0};
    coap_optlist_t *optNode = NULL;
    UINT32 optNodeValue = 0;

    ECOMM_TRACE(UNILOG_COAP, coap_msg_3, P_INFO, 3, ".....coap_config_client..%d.%d...%d.",coapId,coapCnf->mode,coapCnf->dec_para1);
    ECOMM_STRING(UNILOG_COAP, coap_msg_str7, P_INFO, ".....coapCnf->str_para1 name :%s", (UINT8 *)coapCnf->str_para1);
    if(coapCurrClient != NULL)
    {
        #if COAP_SLP_ENABLE 
        coapSlpWaitMutex(COAPSLP_WAIT_CONFIG);
        #endif
        switch(coapCnf->mode)
        {
            case COAP_CFG_ADDRES:
                if(coapCnf->is_from == COAP_CREATE_FROM_AT)
                {
                    optLen = strlen(coapCnf->str_para1);
                    coapSlpNVMem.coapSlpCtx.body[coapId].coap_resource.res_len = optLen;
                    memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_resource.resource, coapCnf->str_para1, optLen);
                }
                coap_resource = coap_resource_init(coap_make_str_const(coapCnf->str_para1), 0);
                coap_add_attr(coap_resource, coap_make_str_const(coapCnf->str_para1), coap_make_str_const(coapCnf->str_para1), 0);

                //coap_resource = coap_resource_init(coap_make_str_const("test"), 0);
                //coap_add_attr(coap_resource, coap_make_str_const("if"), coap_make_str_const("\"clock\""), 0);
                
                coap_add_resource(coapCurrClient->coap_ctx, coap_resource);
            
                break;

            case COAP_CFG_HEAD:
                mode = coapCnf->dec_para1;
                msgId = coapCnf->dec_para2;
                //tokenLen = coapCnf->dec_para3;
                token = coapCnf->str_para1;
                coapCurrClient->head_mode = mode;
                if(coapCnf->is_from == COAP_CREATE_FROM_AT)
                {
                    coapSlpNVMem.coapSlpCtx.body[coapId].coap_head.msg_id = msgId;
                    coapSlpNVMem.coapSlpCtx.body[coapId].coap_head.mode = mode;
                    if(token != NULL)
                    {
                        optLen = strlen(token);
                        memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_head.token, coapCnf->str_para1, optLen);
                    }
                }
                
                if(token != NULL)
                {
                    tokenLenTemp = strlen(token);
                }
                switch(mode)
                {
                    case 1:
                        tokenLenTemp = 8;
                        coapCurrClient->coap_token.length = tokenLenTemp;
                        if(coapCurrClient->coap_token.s != NULL)
                        {
                            free(coapCurrClient->coap_token.s);
                            coapCurrClient->coap_token.s = NULL;
                        }
                        coapCurrClient->coap_token.s = malloc(tokenLenTemp+1);
                        memset(coapCurrClient->coap_token.s, 0, (tokenLenTemp+1));
                        srand(1);
                        for(i=0; i<tokenLenTemp; i++)
                        {
                            coapCurrClient->coap_token.s[i]=rand()%255;
                        }
                        ECOMM_STRING(UNILOG_COAP, coap_head, P_INFO, "....token.. :%s", (UINT8 *)coapCurrClient->coap_token.s);
                        break;
                
                    case 2:
                        coapCurrClient->coap_token.length = tokenLenTemp;
                        if(coapCurrClient->coap_token.s != NULL)
                        {
                            free(coapCurrClient->coap_token.s);
                            coapCurrClient->coap_token.s = NULL;
                        }
                        
                        coapCurrClient->coap_token.s = malloc(tokenLenTemp+1);
                        memset(coapCurrClient->coap_token.s, 0, (tokenLenTemp+1));
                        cmsHexStrToHex(coapCurrClient->coap_token.s, tokenLenTemp/2, (CHAR *)token, tokenLenTemp);                      
                        coapCurrClient->coap_token.length = tokenLenTemp/2;
                        break;
                
                    case 3:
                        coapCurrClient->msg_id = msgId;
                        break;
                
                    case 4:
                        coapCurrClient->msg_id = msgId;
                        tokenLenTemp = 8;
                        coapCurrClient->coap_token.length = tokenLenTemp;
                        if(coapCurrClient->coap_token.s != NULL)
                        {
                            free(coapCurrClient->coap_token.s);
                            coapCurrClient->coap_token.s = NULL;
                        }
                        coapCurrClient->coap_token.s = malloc(tokenLenTemp+1);
                        memset(coapCurrClient->coap_token.s, 0, (tokenLenTemp+1));
                        srand(9);
                        for(i=0; i<tokenLenTemp; i++)
                        {
                            coapCurrClient->coap_token.s[i]=rand()%255;
                        }
                        ECOMM_STRING(UNILOG_COAP, coap_head1, P_INFO, "....token.. :%s", (UINT8 *)coapCurrClient->coap_token.s);
                        break;
                
                    case 5:
                        coapCurrClient->msg_id = msgId;
                        coapCurrClient->coap_token.length = tokenLenTemp;
                        if(coapCurrClient->coap_token.s != NULL)
                        {
                            free(coapCurrClient->coap_token.s);
                            coapCurrClient->coap_token.s = NULL;
                        }
                        coapCurrClient->coap_token.s = malloc(tokenLenTemp+1);
                        memset(coapCurrClient->coap_token.s, 0, (tokenLenTemp+1));
                        cmsHexStrToHex(coapCurrClient->coap_token.s, tokenLenTemp/2, (CHAR *)token, tokenLenTemp);
                        coapCurrClient->coap_token.length = tokenLenTemp/2;
                        break;
                }
                break;

            case COAP_CFG_OPTION:
                coap_optlist_temp = coapCurrClient->coap_optlist;
                if(coapCnf->is_from == COAP_CREATE_FROM_AT)
                {
                    //coap_delete_optlist_node(&coapCurrClient->coap_optlist, coapCnf->dec_para1);
                    ECOMM_TRACE(UNILOG_COAP, coap_msg_4, P_INFO, 2, ".....coap_config_client...0x%x...0x%x.",coap_optlist_temp, coapCurrClient->coap_optlist);
                    optLen = strlen(optValue);
                    for(i=0; i<19; i++)
                    {
                        if(coapCnf->dec_para1 == coapSlpNVMem.coapSlpCtx.body[coapId].coap_opt[i].opt_name)
                        {
                            //coapSlpNVMem.coapSlpCtx.body[coapId].coap_opt[i].opt_value = malloc(optLen+1);
                            //memset(coapSlpNVMem.coapSlpCtx.body[coapId].coap_opt[i].opt_value, 0, (optLen+1));
                            memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_opt[i].opt_value, optValue, optLen);
                            coapSlpNVMem.coapSlpCtx.body[coapId].coap_opt[i].opt_used = 1;
                            ECOMM_STRING(UNILOG_COAP, coap_msg_str8, P_INFO, ".....opt_value name :%s", (UINT8 *)coapSlpNVMem.coapSlpCtx.body[coapId].coap_opt[i].opt_value);
                        }
                    }
                }
                switch(coapCnf->dec_para1)
                {
                    case COAP_OPTION_IF_MATCH: /*if-Match  opaque, 0-8 B*/
                        if(strlen(optValue) > 8)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_IF_MATCH, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_URI_HOST: /*Uri-Host  String, 1-255 B*/
                        if(strlen(optValue) > 255)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_URI_HOST, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_ETAG: /*ETag  opaque, 1-8 B*/
                        if(strlen(optValue) > 8)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_ETAG, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_IF_NONE_MATCH: /*if-None-Match  empty, 0 B*/
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_IF_NONE_MATCH, 0, (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_OBSERVE: /*Observe  empty/uint, 0 B/0-3 B*/
                        if(strlen(optValue) > 3)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen);
                        {
                            optNode = coap_new_optlist(COAP_OPTION_OBSERVE, coap_encode_var_safe(optBuf4, sizeof(optBuf4), optNodeValue), optBuf4);
                            coap_insert_optlist(&coapCurrClient->coap_optlist, optNode); 
                        }
                        break;

                    case COAP_OPTION_URI_PORT: /*Uri-Port  uint, 0-2 B*/
                        if(strlen(optValue) > 2)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen);
                        optNode = coap_new_optlist(COAP_OPTION_URI_PORT, coap_encode_var_safe(optBuf2, sizeof(optBuf2), optNodeValue), optBuf2);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);
                        break;

                    case COAP_OPTION_LOCATION_PATH: /*Location-Path  String, 0-255 B*/
                        if(strlen(optValue) > 255)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_LOCATION_PATH, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_URI_PATH: /*Uri-Path  String, 0-255 B*/
                        if(strlen(optValue) > 255)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        subUriLen = strlen(optValue);
                        if(subUriLen > 0)
                        {
                            for(int i=0; i<subUriLen; i++)
                            {
                                if(optValue[i] == '/')
                                {
                                    subUriNumb++;
                                }
                            }
                            
                            if(optValue[0] == '/')
                            {
                                subUriNumb--;
                            }
                            if(optValue[subUriLen-1] == '/')
                            {
                                subUriNumb--;
                                optValue[subUriLen-1] = 0; 
                            }

                            for(int i=0; i<subUriNumb; i++)
                            {
                                if(optValue[0] == '/')
                                {
                                    optValue++;
                                }
                                if(subUriNumb == 1)
                                {
                                    coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_URI_PATH, strlen(optValue), (uint8_t *)optValue));
                                }
                                else
                                {
                                    chrFind = strchr(optValue, '/');
                                    if(chrFind != NULL)
                                    {
                                        subUriLenTemp = chrFind - optValue;
                                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_URI_PATH, subUriLenTemp, (uint8_t *)optValue));
                                        optValue = chrFind+1;
                                    }
                                    else
                                    {
                                        subUriLenTemp = strlen(optValue);
                                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_URI_PATH, subUriLenTemp, (uint8_t *)optValue));
                                        break;
                                    }
                                }
                            }           
                        }
                        break;

                    case COAP_OPTION_CONTENT_FORMAT: /*Content-Format  uint, 0-2 B*/
                        if(strlen(optValue) > 2)
                        {
                            ret = COAP_ERR;
                            break;
                        }

                        optLen = strlen(optValue);
                        for(i=0; i<optLen; i++)
                        {
                            if((optValue[i] < '0')||(optValue[i] > '9'))
                            {
                                ret = COAP_ERR;
                                break;
                            }
                        }
                        if(ret == COAP_ERR)
                        {
                            break;
                        }
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen);
                        if((optNodeValue!=0)&&(optNodeValue!=40)&&(optNodeValue!=41)&&(optNodeValue!=42)&&(optNodeValue!=47)&&(optNodeValue!=50))
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optNode = coap_new_optlist(COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(optBuf2, sizeof(optBuf2), optNodeValue), optBuf2);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);
                        break;

                    case COAP_OPTION_MAXAGE: /*Max-Age  uint, 0--4 B*/
                        if(strlen(optValue) > 4)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen);
                        optNode = coap_new_optlist(COAP_OPTION_MAXAGE, coap_encode_var_safe(optBuf4, sizeof(optBuf4), optNodeValue), optBuf4);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);
                        break;

                    case COAP_OPTION_URI_QUERY: /*Uri-Query  String, 1-255 B*/
                        if(strlen(optValue) > 255)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_URI_QUERY, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_ACCEPT: /*Accept  uint, 0-2 B*/
                        if(strlen(optValue) > 2)
                        {
                            ret = COAP_ERR;
                            break;
                        }

                        optLen = strlen(optValue);
                        for(i=0; i<optLen; i++)
                        {
                            if((optValue[i] < '0')||(optValue[i] > '9'))
                            {
                                ret = COAP_ERR;
                                break;
                            }
                        }
                        if(ret == COAP_ERR)
                        {
                            break;
                        }
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen);
                        if((optNodeValue!=0)&&(optNodeValue!=40)&&(optNodeValue!=41)&&(optNodeValue!=42)&&(optNodeValue!=47)&&(optNodeValue!=50))
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optNode = coap_new_optlist(COAP_OPTION_ACCEPT, coap_encode_var_safe(optBuf2, sizeof(optBuf2), optNodeValue), optBuf2);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);
                        break;

                    case COAP_OPTION_LOCATION_QUERY: /*Location-Query  String, 0-255 B*/
                        if(strlen(optValue) > 255)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_LOCATION_QUERY, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_BLOCK2: /*Block2  uint, 0-3 B*/
                        if(strlen(optValue) > 3)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen); //optNodeValue format is: (block.num << 4 | block.m << 3 | block.szx)
                        optNode = coap_new_optlist(COAP_OPTION_BLOCK2, coap_encode_var_safe(optBuf3, sizeof(optBuf3), optNodeValue), optBuf3);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);                    
                        break;

                    case COAP_OPTION_BLOCK1: /*Block1  uint, 0-3 B*/
                        if(strlen(optValue) > 3)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen); //optNodeValue format is: (block.num << 4 | block.m << 3 | block.szx)
                        optNode = coap_new_optlist(COAP_OPTION_BLOCK1, coap_encode_var_safe(optBuf3, sizeof(optBuf3), optNodeValue), optBuf3);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);                    
                        break;

                    case COAP_OPTION_SIZE2: /*SIZE  uint, 0-4 B*/
                        if(strlen(optValue) > 4)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen); //
                        optNode = coap_new_optlist(COAP_OPTION_SIZE2, coap_encode_var_safe(optBuf4, sizeof(optBuf4), optNodeValue), optBuf4);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);                    
                        break;

                    case COAP_OPTION_PROXY_URI: /*Proxy-Uri  String, 1-1034 B*/
                        if(strlen(optValue) > 1034)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_PROXY_URI, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_PROXY_SCHEME: /*Proxy-Scheme  String, 1-255 B*/
                        if(strlen(optValue) > 255)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        coap_insert_optlist(&coapCurrClient->coap_optlist, coap_new_optlist(COAP_OPTION_PROXY_SCHEME, strlen(optValue), (uint8_t *)optValue));
                        break;

                    case COAP_OPTION_SIZE1: /*Size1  uint, 0-4 B*/
                        if(strlen(optValue) > 4)
                        {
                            ret = COAP_ERR;
                            break;
                        }
                        optLen = strlen(optValue);
                        cmsDecStrToUInt(&optNodeValue, (const UINT8 *)optValue, optLen); //
                        optNode = coap_new_optlist(COAP_OPTION_SIZE1, coap_encode_var_safe(optBuf4, sizeof(optBuf4), optNodeValue), optBuf4);
                        coap_insert_optlist(&coapCurrClient->coap_optlist, optNode);                    
                        break;

                    default:
                        ret = COAP_ERR;
                        break;
                }
                break;

            case COAP_CFG_SHOW:
                if(coapCnf->is_from == COAP_CREATE_FROM_AT)
                {
                    if(coapCnf->dec_para1 == 1)
                    {
                        coapSlpNVMem.coapSlpCtx.body[coapId].showra = coapCnf->dec_para2;
                    }
                    else if(coapCnf->dec_para1 == 2)
                    {
                        coapSlpNVMem.coapSlpCtx.body[coapId].showrspopt = coapCnf->dec_para2;
                    }
                }
                if(coapCnf->dec_para1 == 1)
                {
                    coapCurrClient->showra = coapCnf->dec_para2;
                }
                else if(coapCnf->dec_para1 == 2)
                {
                    coapCurrClient->showrspopt = coapCnf->dec_para2;
                }
                break;
                
            case COAP_CFG_CLOUD:
                if(coapCnf->is_from == COAP_CREATE_FROM_AT)
                {
                    coapSlpNVMem.coapSlpCtx.body[coapId].dtls_flag = coapCnf->dec_para1;
                    coapSlpNVMem.coapSlpCtx.body[coapId].cloud = coapCnf->dec_para2;
                    coapSlpNVMem.coapSlpCtx.body[coapId].encrypt = coapCnf->dec_para3;
                }
                coapCurrClient->dtls_flag = coapCnf->dec_para1;  /*dtls=1, no dtls=0*/
                coapCurrClient->cloud = coapCnf->dec_para2;      /*COAP_CLOUD_TYPE_ONENET=1, COAP_CLOUD_TYPE_ALI=2*/
                coapCurrClient->encrypt = coapCnf->dec_para3;    /*COAP_ENCRYP_TYPE_MD5=1,COAP_ENCRYP_TYPE_SHA1=2, COAP_ENCRYP_TYPE_SHA256=3*/
                break;
                
            case COAP_CFG_ALISIGN:
                if(coapCnf->is_from == COAP_CREATE_FROM_AT)
                {
                    memset(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.device_id, 0, COAP_DEV_INFO1_LEN);
                    memset(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.device_name, 0, COAP_DEV_INFO1_LEN);
                    memset(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.device_scret, 0, COAP_DEV_INFO2_LEN);
                    memset(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.product_key, 0, COAP_DEV_INFO1_LEN);
                    
                    memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.device_id, coapCurrClient->coap_dev.device_id, strlen(coapCurrClient->coap_dev.device_id));
                    memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.device_name, coapCurrClient->coap_dev.device_name, strlen(coapCurrClient->coap_dev.device_name));
                    memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.device_scret, coapCurrClient->coap_dev.device_scret, strlen(coapCurrClient->coap_dev.device_scret));
                    memcpy(coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.product_key, coapCurrClient->coap_dev.product_key, strlen(coapCurrClient->coap_dev.product_key));
                    coapSlpNVMem.coapSlpCtx.body[coapId].coap_dev.seq = coapCurrClient->coap_dev.seq;
                }
                break;

            default:
                ret = COAP_ERR;
                break;

        }
        coapSlpReleaseMutex(COAPSLP_WAIT_CONFIG);

    }

#if COAP_SLP_ENABLE
    if((coapCnf->is_from != COAP_CREATE_FROM_RESTORE) && (ret == COAP_OK))
    {
        coapSlpNVMem.coapSlpCtx.needRestoreConfig = 1;
        coapCtxStoreNVMem(false);
    }
#endif

    return ret;
}

coap_client *coap_get_curr_client(void)
{
    return coapCurrClient;
}

int coap_create_client(coap_client *coapClient)
{
    coap_client *coapNewClient = coapClient;
    coap_log_t log_level = LOG_NOTICE;

    coap_startup();
    coap_dtls_set_log_level(log_level);
    coap_set_log_level(log_level);

    coapNewClient->coap_ctx = coap_new_context(NULL);             // coap_ctx
    if(coapNewClient->coap_ctx == NULL)
    {
        coap_delete_client(coapNewClient);
        return COAP_CONTEXT_ERR;
    }

    coap_context_set_keepalive(coapNewClient->coap_ctx, coapNewClient->coap_ping_seconds);

    coapNewClient->coap_wait_ms = 2*1000;

    return COAP_OK;
}

int coap_open_client(coap_client *coapClient, coap_send_msg msg)//int port, char *ipAddr)
{
    coap_client *coapNewClient = coapClient;
    ssize_t key_length = 0;
    unsigned char psk_key[COAP_DTLS_PSK_KEY_LEN] = {0};
    unsigned char psk_identity[COAP_DTLS_PSK_IDENTITY_LEN] = {0};
    coap_proto_t coap_pro;
    char aliBuff[128] = {0};
    char aliKey[32] = {0};
    
    ECOMM_TRACE(UNILOG_COAP, coap_msg_5, P_VALUE, 0, ".....coap_open_client..");           

    if(coapNewClient->dtls_flag == 1)
    {
        coap_pro = COAP_PROTO_DTLS;
        //coap_get_dtls_key(coapNewClient->cloud, coapNewClient->encrypt, "eigencomm", "ec_smoke", "sWZtNMYkODMxvauyaSiGeJrVEp9jZ4Tg", "a1xFDTv3InR", psk_key, psk_identity);   
        coap_get_dtls_key(coapNewClient->cloud, coapNewClient->encrypt, coapNewClient->coap_dev.device_id, coapNewClient->coap_dev.device_name, coapNewClient->coap_dev.device_scret, coapNewClient->coap_dev.product_key, (char *)psk_key, (char *)psk_identity);      
    }
    else
    {
        coap_pro = COAP_PROTO_UDP;
    }

    if(coapNewClient->coap_dev.seq != COAP_ALI_RANDOM_DEF)
    {
        snprintf((char *)aliBuff, sizeof(aliBuff), "%s,%s", coapNewClient->coap_dev.device_scret,  aliRandom);

        coap_ali_sha256((const uint8_t *)aliBuff,  strlen((char *)aliBuff), (uint8_t *)aliKey);
        memcpy(coapNewClient->coap_dev.psk_key, aliKey + 8, 16);
        
    }
    
    key_length = strlen((const char *)psk_key);

    if(coapNewClient->coap_session == NULL)
    {
        if(coap_pro == COAP_PROTO_DTLS)
        {
            coapNewClient->coap_session = get_session(coapNewClient->coap_ctx,
                                          NULL, NULL,
                                          coap_pro,
                                          &msg.coap_dst,
                                          (const char *)psk_identity,
                                          psk_key, (unsigned)key_length
                                        );
        }
        else 
        {
            coapNewClient->coap_session = get_session(coapNewClient->coap_ctx,
                                          NULL, NULL,
                                          COAP_PROTO_UDP,
                                          &msg.coap_dst,
                                          NULL,
                                          NULL, (unsigned)key_length
                                        );
        }
    }
    else
    {
        if(coapNewClient->coap_port != msg.port)
        {
            coapNewClient->coap_port = msg.port;
            coap_session_release(coapNewClient->coap_session);
            coapNewClient->coap_session = NULL;
        }

        if(strcmp((char *)&coapNewClient->coap_dst.addr.sin.sin_addr, (char *)&msg.coap_dst.addr.sin.sin_addr) != 0)
        {
            memcpy((char *)&coapNewClient->coap_dst, (char *)&msg.coap_dst, sizeof(coapNewClient->coap_dst));

            if(coapNewClient->coap_session != NULL)
            {
                coap_session_release(coapNewClient->coap_session);
                coapNewClient->coap_session = NULL;
            }
        }

        if(coapNewClient->coap_session == NULL)
        {
            if(coap_pro == COAP_PROTO_DTLS)
            {
                coapNewClient->coap_session = get_session(coapNewClient->coap_ctx,
                                              NULL, NULL,
                                              coap_pro,
                                              &msg.coap_dst,
                                              (const char *)psk_identity,
                                              psk_key, (unsigned)key_length
                                            );
            }
            else 
            {
                coapNewClient->coap_session = get_session(coapNewClient->coap_ctx,
                                              NULL, NULL,
                                              COAP_PROTO_UDP,
                                              &msg.coap_dst,
                                              NULL,
                                              NULL, (unsigned)key_length
                                            );
            }
        }
    }

    if(!coapNewClient->coap_session)
    {
        //coap_delete_client(coapNewClient);
        coap_log( LOG_EMERG, "cannot create client session\n" );
        return COAP_SESSION_ERR;
    }

    /* add Uri-Host if server address differs from uri.host */

    switch(msg.coap_dst.addr.sa.sa_family)
    {
        case AF_INET:
            //addrptr = &coapNewClient->coap_dst.addr.sin.sin_addr;
            /* create context for IPv4 */
            break;

        case AF_INET6:
            //addrptr = &coapNewClient->coap_dst.addr.sin6.sin6_addr;
            break;

        default:
            break;
    }

    coap_register_option(coapNewClient->coap_ctx, COAP_OPTION_BLOCK2);
    coap_register_response_handler(coapNewClient->coap_ctx, coap_message_handler);

    /* construct CoAP message */
    if(coapNewClient->coap_block_flags & FLAGS_BLOCK)
    {
        set_blocksize();
    }

    coapNewClient->coap_wait_ms = 2*1000;

    return COAP_OK;
}

void coap_task_recv_process(void *argument)
{
    int i = 0;
    int hasBusyClient = 0;

    coap_task_recv_status_flag = COAP_TASK_STAT_CREATE;

    #if COAP_SLP_ENABLE
    coapSlpInit();
    #endif
    
    while(1)
    {
        hasBusyClient = 0;
        for(i=0; i<COAP_CLIENT_NUMB_MAX; i++)
        {
            coap_mutex_lock(&coap_mutex1);
            if(coapClient[i].is_used == COAP_CLIENT_USED)
            {
                hasBusyClient = 1;
                coapCurrClient = &coapClient[i];

                ECOMM_TRACE(UNILOG_COAP, coap_msg_6, P_INFO, 0, ".....coap_run_once.......");
                coap_run_once(coapClient[i].coap_ctx, coapClient[i].coap_wait_ms, coapCurrClient);
            }

            coap_mutex_unlock(&coap_mutex1);
            ECOMM_TRACE(UNILOG_COAP, coap_msg_7, P_INFO, 0, ".....coap_run_once..release.....");
            osDelay(200);
        }

        if(hasBusyClient == 0)
        {
            break;
        }
    }

    #if COAP_SLP_ENABLE
    coapSlpDeInit();
    #endif
    ECOMM_TRACE(UNILOG_COAP, coap_msg_8, P_INFO, 0, ".....coap_delete_recv..task....");

    coap_task_recv_status_flag = COAP_TASK_STAT_DELETE;

    coap_recv_task_handle = NULL;

    osThreadExit();
}


void coap_task_send_process(void *argument)
{
    int i = 0;
    int ret = COAP_ERR;
    int hasBusyClient = 0;
    int msg_type = 0xff;
    coap_send_msg coapMsg;
    coap_client *coapNewClient = NULL;
    int primSize;
    int socket_stat = -1;
    coap_pdu_t  *pdu;
    int coapId = 0xff;
    int check_time = 0;
    int network_ready_flag = 0;
    UINT16 atHandle;
    coap_cnf_msg coapCnfMsg;
    UINT8 cnfType = 0xff;
    coap_task_send_status_flag = COAP_TASK_STAT_CREATE;

    while(1)
    {
        /* recv msg (block mode) */
        xQueueReceive(coap_send_msg_handle, &coapMsg, osWaitForever);
        msg_type = coapMsg.cmd_type;

        switch(msg_type)
        {
            case MSG_COAP_CREATE_CLIENT:
                #if COAP_SLP_ENABLE 
                coapSlpWaitMutex(COAPSLP_WAIT_CREATE);
                #endif
                atHandle = coapMsg.reqhandle;
                coapId = coapMsg.id;
                if(coapMsg.isRestore == true)
                {
                    coapNewClient = coap_find_client(coapId);
                }
                else
                {
                    coapNewClient = coap_new_client(coapMsg.local_port);
                }
                if((coapNewClient == NULL))
                {
                    ret = COAP_CLIENT_ERR;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_COAP, coap_msg_9, P_INFO, 0, "...coap_client_create..1...");
                
                    ret = coap_create_client(coapNewClient);
                    if(ret != COAP_OK)
                    {
                        ret = COAP_CLIENT_ERR;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_COAP, coap_msg_10, P_INFO, 0, "...coap_client_create..2...");
                    
                        coapNewClient->is_used = COAP_CLIENT_USED;
                        coapNewClient->coap_reqhandle = coapMsg.reqhandle;
                        if(coap_recv_task_handle == NULL)
                        {
                            coap_recv_task_Init();
                        }
                    }
                }
                #if COAP_SLP_ENABLE
                coapSlpReleaseMutex(COAPSLP_WAIT_CREATE);
                if((ret == COAP_OK) && (coapMsg.isRestore == false))
                {
                    coapSlpNVMem.coapSlpCtx.body[coapId].handle = coap_reqHandle;
                    coapAddSlpCtx(coapNewClient);
                    coapCtxStoreNVMem(false);
                }
                #endif
                if(ret == COAP_OK)
                {            
                    if(coapMsg.isRestore != true)
                    {
                        primSize = sizeof(coapCnfMsg);
                        coapCnfMsg.atHandle = atHandle;
                        applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_COAP, APPL_COAP_CREATE_CNF, primSize, &coapCnfMsg);
                    }
                }
                else
                {
                    if(coapMsg.isRestore != true)
                    {
                        primSize = sizeof(coapCnfMsg);
                        coapCnfMsg.atHandle = atHandle;
                        applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_COAP, APPL_COAP_CREATE_CNF, primSize, &coapCnfMsg);
                    }
                }
                break;
                
            case MSG_COAP_DELETE_CLIENT:
                #if COAP_SLP_ENABLE
                coapSlpWaitMutex(COAPSLP_WAIT_DELETE);
                #endif
                ECOMM_TRACE(UNILOG_COAP, coap_msg_11, P_INFO, 0, ".....start delete coap client.......");
                coapNewClient = (coap_client*)coapMsg.client_ptr;
                coapId = coapMsg.id;
                atHandle = coapMsg.reqhandle;
            
                if(coapNewClient != NULL)
                {               
                    coap_mutex_lock(&coap_mutex1);
                    ret = coap_delete_client(coapNewClient);
                    ECOMM_TRACE(UNILOG_COAP, coap_msg_12, P_INFO, 0, ".....coap_delete_client.......");
                    #if COAP_SLP_ENABLE
                    if(ret == COAP_OK)
                    {
                        coapDelSlpCtx(coapId, coapNewClient);          // delete slp ctx
                        coapCtxStoreNVMem(false);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_COAP, coap_msg_13, P_INFO, 0, ".....coap delete failed.......");
            
                    }
                    ECOMM_TRACE(UNILOG_COAP, coap_msg_14, P_INFO, 0, ".....coap_delete_client...2....");
                    #endif
                    coap_mutex_unlock(&coap_mutex1);
                }
                /* send result to at task */
                #if COAP_SLP_ENABLE
                coapSlpReleaseMutex(COAPSLP_WAIT_DELETE);
                #endif
                if(ret == COAP_OK)
                {
                    for(i=0; i<10; i++)
                    {
                        if(coap_task_recv_status_flag == COAP_TASK_STAT_DELETE)
                        {
                            ECOMM_TRACE(UNILOG_COAP, coap_msg_15, P_INFO, 0, ".....delete coap client ok.......");
                            break;
                        }
                        osDelay(200);
                    }
                    primSize = sizeof(coapCnfMsg);
                    coapCnfMsg.atHandle = atHandle;
                    applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_COAP, APPL_COAP_DELETE_CNF, primSize, &coapCnfMsg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_COAP, coap_msg_16, P_INFO, 0, ".....delete coap client fail.......");
                    primSize = sizeof(coapCnfMsg);
                    coapCnfMsg.atHandle = atHandle;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_COAP, APPL_COAP_DELETE_CNF, primSize, &coapCnfMsg);
                }
                break;
            
            case MSG_COAP_SEND:
                /* send packet */
                ECOMM_TRACE(UNILOG_COAP, coap_msg_17, P_INFO, 0, ".....start send coap publish packet.......");
                coapNewClient = (coap_client*)coapMsg.client_ptr;
                atHandle = coapMsg.reqhandle;

                /*the network status check should be done, when coap sends*/
                /*becase it can not do the check, during coap creates client. for if the status is always not active, coap client create fail*/
                /*so coap sends will always fail, when the chip wake up from sleep*/
                #if COAP_SLP_ENABLE
                coapSlpWaitMutex(COAPSLP_WAIT_NETWORK);
                #endif
                NmAtiSyncRet netStatus;
                do
                {
                    check_time++;
                    appGetNetInfoSync(0, &netStatus);
                    if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
                    {
                        network_ready_flag =1;
                        break;
                    }
                    osDelay(1000);

                }while(check_time<7);

                if(network_ready_flag == 0)
                {
                    ECOMM_TRACE(UNILOG_COAP, coap_msg_18, P_INFO, 0, ".....send coap publish packet fail,network err.......");

                    primSize = sizeof(coapCnfMsg);
                    coapCnfMsg.atHandle = atHandle;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_COAP, APPL_COAP_SEND_CNF, primSize, &coapCnfMsg);
                }
                else
                {
                    coap_mutex_lock(&coap_mutex1);
                    ret = coap_open_client(coapNewClient, coapMsg);
                    coap_mutex_unlock(&coap_mutex1);

                    if(ret == COAP_OK)
                    {
                        #if 0
                        if(coap_recv_task_handle == NULL)
                        {
                            coap_recv_task_Init();
                        }
                        #endif
                        if(NULL == (pdu = coap_new_request(coapNewClient, coapNewClient->coap_ctx, coapNewClient->coap_session, coapMsg.coap_method, &coapNewClient->coap_optlist, coapMsg.coap_payload.s, coapMsg.coap_payload.length)))
                        {
                            ret = COAP_ERR;
                        }
                        else
                        {
                            coapNewClient->coap_session->raiFlag = (uint8_t)coapMsg.raiFlag;

                            ret = coap_send(coapNewClient->coap_session, pdu);
                            if(coapNewClient->coap_optlist != NULL)
                            {
                                coap_delete_optlist(coapNewClient->coap_optlist); //add for 
                                coapNewClient->coap_optlist = NULL;
                            }
                            coapNewClient->dtls_flag = 0;
                            coapNewClient->coap_port = coapMsg.port;
                            memcpy((char *)&coapNewClient->coap_dst, (char *)&coapMsg.coap_dst, sizeof(coapNewClient->coap_dst));
                            
                            /* send result to at task */
                            if(ret != COAP_INVALID_TID)
                            {
                                ECOMM_TRACE(UNILOG_COAP, coap_msg_19, P_INFO, 0, ".....send coap publish packet ok.......");                                
                                primSize = sizeof(coapCnfMsg);
                                coapCnfMsg.atHandle = atHandle;
                                applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_COAP, APPL_COAP_SEND_CNF, primSize, &coapCnfMsg);
                            }
                            else
                            {
                                socket_stat = sock_get_errno(coapNewClient->coap_session->sock.fd);
                                socket_error_is_fatal(socket_stat);
                                primSize = sizeof(coapCnfMsg);
                                coapCnfMsg.atHandle = atHandle;
                                applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_COAP, APPL_COAP_SEND_CNF, primSize, &coapCnfMsg);
                                ECOMM_TRACE(UNILOG_COAP, coap_msg_20, P_INFO, 0, ".....send coap publish packet fail.......");
                            }
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_COAP, coap_msg_21, P_INFO, 0, ".....send coap publish packet fail,network err.......");
                        
                        primSize = sizeof(coapCnfMsg);
                        coapCnfMsg.atHandle = atHandle;
                        applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_COAP, APPL_COAP_SEND_CNF, primSize, &coapCnfMsg);
                    }
                }
                #if COAP_SLP_ENABLE
                coapSlpNVMem.coapSlpCtx.needRestoreConfig = 0;
                coapCtxStoreNVMem(false);
                coapSlpReleaseMutex(COAPSLP_WAIT_NETWORK);
                #endif

                if(coapMsg.coap_payload.s != NULL)
                {
                    free(coapMsg.coap_payload.s);
                    coapMsg.coap_payload.s = NULL;
                }
                if(coapNewClient->coap_token.s != NULL)
                {
                    free(coapNewClient->coap_token.s);
                    coapNewClient->coap_token.s = NULL;
                }
                break;
        
            case MSG_COAP_ADD_RES:
            case MSG_COAP_HEAD:
            case MSG_COAP_OPTION:
            case MSG_COAP_CONFIG:
            case MSG_COAP_ALISIGN:                    
                coapNewClient = (coap_client*)coapMsg.client_ptr;
                coapId = coapMsg.id;
                atHandle = coapMsg.reqhandle;
            
                switch(msg_type)
                {
                    case MSG_COAP_ADD_RES:
                        cnfType = APPL_COAP_ADDRES_CNF;
                        break;
                    case MSG_COAP_HEAD:
                        cnfType = APPL_COAP_HEAD_CNF;
                        break;
                    case MSG_COAP_OPTION:
                        cnfType = APPL_COAP_OPTION_CNF;
                        break;
                    case MSG_COAP_CONFIG:
                        cnfType = APPL_COAP_CNF_CNF;
                        break;
                }
                ret = coap_config_client(coapId, &coapMsg.coapCnf);

                if(ret == COAP_OK)
                {
                    primSize = sizeof(coapCnfMsg);
                    coapCnfMsg.atHandle = atHandle;
                    //msg_type = MSG_COAP_ADD_RES;
                    if(msg_type == MSG_COAP_OPTION)
                    {
                        if(coapMsg.cmd_end_flag == 1)
                        {
                            applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_COAP, cnfType, primSize, &coapCnfMsg);
                        }
                    }
                    else
                    {
                        applSendCmsCnf(atHandle, APPL_RET_SUCC, APPL_COAP, cnfType, primSize, &coapCnfMsg);
                    }
                }
                else
                {
                    primSize = sizeof(coapCnfMsg);
                    coapCnfMsg.atHandle = atHandle;
                    applSendCmsCnf(atHandle, APPL_RET_FAIL, APPL_COAP, cnfType, primSize, &coapCnfMsg);
                }
                if(coapMsg.coapCnf.str_para1 != NULL)
                {
                    free(coapMsg.coapCnf.str_para1);
                    coapMsg.coapCnf.str_para1 = NULL;
                }
                break;
        }

        hasBusyClient = 0;
        for(i=0; i<COAP_CLIENT_NUMB_MAX; i++)
        {
            if(coapClient[i].is_used == COAP_CLIENT_USED)
            {
                hasBusyClient = 1;
            }
        }

        if(hasBusyClient == 0)
        {
            break;
        }
    }
    ECOMM_TRACE(UNILOG_COAP, coap_msg_22, P_INFO, 0, ".....coap_delete_send..task....");
    coap_task_send_status_flag = COAP_TASK_STAT_DELETE;
    coap_send_task_handle = NULL;
    osThreadExit();
}

int coap_recv_task_Init(void)
{
    int ret = COAP_OK;

    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "coapRecv";
    task_attr.stack_size = COAP_TASK_RECV_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal6;

    coap_recv_task_handle = osThreadNew(coap_task_recv_process, NULL,&task_attr);
    if(coap_recv_task_handle == NULL)
    {
        ret = COAP_TASK_ERR;
    }

    return ret;
}

int coap_send_task_Init(void)
{
    int ret = COAP_OK;

    if(coap_smphr_status_flag == COAP_NOT_INIT)
    {
        coap_smphr_status_flag = COAP_HAVE_INIT;
        if(coap_mutex1.sem == NULL)
        {
            coap_mutex_init(&coap_mutex1);
        }
        if(coap_mutex2.sem == NULL)
        {
            coap_mutex_init(&coap_mutex2);
        }

        if(coap_send_msg_handle == NULL)
        {
            coap_send_msg_handle = xQueueCreate(24, sizeof(coap_send_msg));
        }
        else
        {
            xQueueReset(coap_send_msg_handle);
        }
    }

    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "coapSend";
    task_attr.stack_size = COAP_TASK_SEND_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;

    coap_send_task_handle = osThreadNew(coap_task_send_process, NULL,&task_attr);
    if(coap_send_task_handle == NULL)
    {
        ret = COAP_TASK_ERR;
    }

    return ret;
}
#define COAP_CLIENT_AUTOREG_INTERFACE
#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE)
extern int coap_autoreg_run_once(coap_context_t *ctx, unsigned timeout_ms, void *pClient);
static void coap_autoreg_message_handler(struct coap_context_t *ctx,
                                coap_session_t *session,
                                coap_pdu_t *sent,
                                coap_pdu_t *received,
                                const coap_tid_t id UNUSED_PARAM)
{
    coap_client *coapCurrClient = &autoregCoapClient;
    coap_message coap_msg;
    int msg_len = 0;
    int primSize = 0;
    
    switch (received->type)
    {
        case COAP_MESSAGE_ACK:
        case COAP_MESSAGE_RST:
        case COAP_MESSAGE_NON:
        case COAP_MESSAGE_CON:
          memset(&coap_msg, 0, sizeof(coap_message));
          coap_msg.coap_id = coapCurrClient->coap_id;
          coap_msg.showra = coapCurrClient->showra;
          coap_msg.showrspopt = coapCurrClient->showrspopt;
          coap_msg.msg_type = received->type;
          coap_msg.msg_method = received->code;
          coap_msg.msg_id = received->tid;
          coap_msg.token_len = received->token_length;
          if(coap_msg.token_len > 0)
          {
              coap_msg.coap_token = malloc(coap_msg.token_len+1);
              memset(coap_msg.coap_token, 0, coap_msg.token_len+1);
              memcpy(coap_msg.coap_token, received->token, coap_msg.token_len);
          }
          coap_msg.server_port = session->remote_addr.addr.sin.sin_port;
          coap_msg.server_ip = session->remote_addr.addr.sin.sin_addr.s_addr;
          coap_msg.client_ptr = (void *)coapCurrClient;

          if(received->data != NULL)
          {
              msg_len = strlen((char *)received->data);
              coap_msg.coap_payload = malloc(msg_len+1);
              memset(coap_msg.coap_payload, 0 ,(msg_len+1));
              memcpy(coap_msg.coap_payload, received->data, msg_len);
          }
          primSize = (msg_len+1)+sizeof(coap_message);
          coap_msg.payload_len = msg_len;              
          
          if(strstr((const char *)received->data, "\"resultDesc\":\"Success\"") != NULL)
          {
              autoRegSuccFlag = 1;
          }
          applSendCmsInd(BROADCAST_IND_HANDLER, APPL_COAP, APPL_COAP_RECV_IND, primSize, (void *)&coap_msg);

        break;
    }
}

int coap_autoreg_create(coap_client *coapClient, int localPort, CoapCreateInfo coapInfo)
{
    int ret = COAP_OK;
    
    ret = coap_create_client(coapClient);
    if(ret != COAP_OK)
    {
        ret = COAP_CLIENT_ERR;
    }

    return ret;
}

int coap_autoreg_recv(coap_client *coapClient)
{
    int ret = COAP_OK;

    ret = coap_autoreg_run_once(coapClient->coap_ctx, coapClient->coap_wait_ms, coapClient);

    return ret;
}

int coap_autoreg_send(coap_client *coapClient, int coapMsgType, int coapMethod, char *ipAddr, int port, int coapPayloadLen, char *coapPayload)
{
    int ret = -1;
    coap_send_msg coapMsg;
    coap_pdu_t  *pdu;
    int ipType = 0;

    memset(&coapMsg, 0, sizeof(coapMsg));
    ret = coap_check_ip_type(ipAddr, &ipType);
    if(ret == COAP_ERR)
    {
        return COAP_IP_ERR;
    }

    if(ipType == AF_INET)
    {
        coapMsg.coap_dst.size = sizeof(coapMsg.coap_dst.addr.sin);
        coapMsg.coap_dst.addr.sin.sin_len = coapMsg.coap_dst.size;
        coapMsg.coap_dst.addr.sin.sin_family = AF_INET;
        inet_aton(ipAddr, &coapMsg.coap_dst.addr.sin.sin_addr.s_addr);
        coapMsg.coap_dst.addr.sin.sin_port = htons(port);
    }
    else if(ipType == AF_INET6)
    {
        coapMsg.coap_dst.size = sizeof(coapMsg.coap_dst.addr.sin6);
        coapMsg.coap_dst.addr.sin6.sin6_len = coapMsg.coap_dst.size;
        coapMsg.coap_dst.addr.sin6.sin6_family = AF_INET6;

        //inet6_aton(coapIp, &coapNewClient->coap_dst.addr.sin6.sin6_addr.un.u8_addr)
    }

    coapClient->coap_method = coapMethod;
    coapClient->coap_msgtype = coapMsgType;
    coapMsg.coap_method = coapMethod;
    coapMsg.coap_msgtype = coapMsgType;
    
    coapMsg.cmd_type = MSG_COAP_SEND;
    coapMsg.client_ptr = (void *)coapClient;
    coapMsg.port = port;

    //if(coapSendMode == COAP_SEND_AT)/* for at mode, the payload is ascii */
    {
        if(coapPayload != NULL)
        {
            if(coapPayload[0] == '"')
            {
                if(coapPayload[coapPayloadLen-1] == '"')
                {
                    coapPayload[coapPayloadLen-1] = 0;
                }
                coapMsg.coap_payload.length = strlen(&coapPayload[1]);
                coapMsg.coap_payload.s = malloc(coapMsg.coap_payload.length +1);
                memset(coapMsg.coap_payload.s, 0, (coapMsg.coap_payload.length +1));
                memcpy(coapMsg.coap_payload.s, &coapPayload[1], coapMsg.coap_payload.length);
            }
            else
            {
                coapMsg.coap_payload.length = strlen(coapPayload);
                coapMsg.coap_payload.s = malloc(coapMsg.coap_payload.length +1);
                memset(coapMsg.coap_payload.s, 0, (coapMsg.coap_payload.length +1));
                memcpy(coapMsg.coap_payload.s, coapPayload, coapMsg.coap_payload.length);
            }
        
        }
    }

    ret = coap_open_client(coapClient, coapMsg);
    coapClient->coap_ctx->response_handler = coap_autoreg_message_handler;
    if(ret == COAP_OK)
    {
        if(NULL == (pdu = coap_new_request(coapClient, coapClient->coap_ctx, coapClient->coap_session, coapMsg.coap_method, &coapClient->coap_optlist, coapMsg.coap_payload.s, coapMsg.coap_payload.length)))
        {
            ret = COAP_ERR;
        }
        else
        {        
            ret = coap_send(coapClient->coap_session, pdu);
        }
    }
    
    return ret;
}

int coap_autoreg_delete(coap_client *coapClient)
{
    int ret = COAP_OK;

    ret = coap_delete_client(coapClient);

    return ret;
}

#endif
#define COAP_CLIENT_AT_INTERFACE

int coap_client_create(UINT32 reqHandle, int localPort, CoapCreateInfo coapInfo)
{
    int ret = COAP_ERR;
    coap_client *coapNewClient = NULL;
    coap_send_msg coapMsg;
    
    ECOMM_TRACE(UNILOG_COAP, coap_msg_23, P_INFO, 0, "...coap_client_create.....");

    if(coapInfo.from == COAP_CREATE_FROM_AT)
    {
        ret = coap_check_client(localPort);
        if(ret == COAP_ERR)
        {
            ECOMM_TRACE(UNILOG_COAP, coap_msg_24, P_SIG, 0, "...coap client..is.already..ex");
            return COAP_CLIENT_ERR;
        }
    }
    coap_reqHandle = reqHandle;

    if((coap_task_recv_status_flag == COAP_TASK_STAT_DELETE)&&(coap_task_send_status_flag == COAP_TASK_STAT_DELETE))
    {
        if(coap_send_task_Init() != COAP_OK)
        {
            ECOMM_TRACE(UNILOG_COAP, coap_msg_25, P_INFO, 0, "...coap_task create fail...\r\n");

            coap_delete_client(coapNewClient);
            coap_recv_task_handle = NULL;
            coap_send_task_handle = NULL;
            coap_task_recv_status_flag = COAP_TASK_STAT_DELETE;
            coap_task_send_status_flag = COAP_TASK_STAT_DELETE;
            return COAP_TASK_ERR;
        }
        else
        {
            memset(&coapMsg, 0, sizeof(coapMsg));
            if(coapInfo.from == COAP_CREATE_FROM_RESTORE)
            {
                coapMsg.isRestore = true;
            }
            coapMsg.cmd_type = MSG_COAP_CREATE_CLIENT;
            coapMsg.reqhandle = reqHandle;
            coapMsg.local_port = localPort;
            coapMsg.id = coapInfo.coapId;
            
            xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);

            ret = COAP_OK;
        }
    }

    return ret;
}

int coap_client_delete(UINT32 reqHandle, int coapId)
{
    int ret = COAP_OK;
    coap_client *coapNewClient = NULL;
    coap_send_msg coapMsg;

    coapNewClient = coap_find_client(coapId);

    if(coapNewClient == NULL)
    {
        ECOMM_TRACE(UNILOG_COAP, coap_msg_26, P_SIG, 0, "can not find coap client");
        return COAP_CLIENT_ERR;
    }

    memset(&coapMsg, 0, sizeof(coapMsg));
    coapMsg.cmd_type = MSG_COAP_DELETE_CLIENT;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.id = coapId;
    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);

    return ret;
}

/*
{  0, "plain" },
{  0, "text/plain" },
{ 40, "link" },
{ 40, "link-format" },
{ 40, "application/link-format" },
{ 41, "xml" },
{ 41, "application/xml" },
{ 42, "binary" },
{ 42, "octet-stream" },
{ 42, "application/octet-stream" },
{ 47, "exi" },
{ 47, "application/exi" },
{ 50, "json" },
{ 50, "application/json" },
{ 60, "cbor" },
{ 60, "application/cbor" },
{ 255, NULL }
*/
//#define paylaodtt    "{\"id\":\"1\",\"version\":\"1.0\",\"params\":[{\"attrKey\":\"SYS_LP_SDK_VERSION\",\"attrValue\":\"3.2.0\",\"domain\":\"SYSTEM\"},{\"attrKey\":\"SYS_SDK_LANGUAGE\",\"attrValue\":\"C\",\"domain\":\"SYSTEM\"}],\"method\":\"thing.deviceinfo.update\"}"
//#define paylaodtt "{\"productKey\":\"a1xFDTv3InR\",\"deviceName\":\"ec_smoke\",\"clientId\":\"a1xFDTv3InR.ec_smoke\",\"sign\":\"e6813e664c1a50b9f4ca4e848cb188a5\"}"

int coap_client_send(UINT32 reqHandle, int coapId, int coapMsgType, int coapMethod, char *ipAddr,
                           int port, int coapPayloadLen, char *coapPayload, int coapSendMode, UINT8 coapRai)
{
    int ret = COAP_ERR;
    coap_client *coapNewClient = NULL;
    coap_send_msg coapMsg;
    int ipType = 0;
    int coapPort = port;
    
    ECOMM_TRACE(UNILOG_COAP, coap_msg_27, P_INFO, 1, "...coap_client_send..port.0x%x...",coapPort);
    memset(&coapMsg, 0, sizeof(coapMsg));

    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }

    if(coap_send_packet_count > COAP_SEND_PACKET_MAX)
    {
        return COAP_SEND_NOT_END_ERR;
    }

    ret = coap_check_ip_type(ipAddr, &ipType);
    if(ret == COAP_ERR)
    {
        return COAP_IP_ERR;
    }
    coap_reqHandle = reqHandle;

    if(ipType == AF_INET)
    {
        coapMsg.coap_dst.size = sizeof(coapMsg.coap_dst.addr.sin);
        coapMsg.coap_dst.addr.sin.sin_len = coapMsg.coap_dst.size;
        coapMsg.coap_dst.addr.sin.sin_family = AF_INET;
        inet_aton(ipAddr, &coapMsg.coap_dst.addr.sin.sin_addr.s_addr);
        coapMsg.coap_dst.addr.sin.sin_port = htons(coapPort);
    }
    else if(ipType == AF_INET6)
    {
        coapMsg.coap_dst.size = sizeof(coapMsg.coap_dst.addr.sin6);
        coapMsg.coap_dst.addr.sin6.sin6_len = coapMsg.coap_dst.size;
        coapMsg.coap_dst.addr.sin6.sin6_family = AF_INET6;

        //inet6_aton(coapIp, &coapNewClient->coap_dst.addr.sin6.sin6_addr.un.u8_addr)
    }

    coapNewClient->coap_method = coapMethod;
    coapNewClient->coap_msgtype = coapMsgType;
    coapMsg.coap_method = coapMethod;
    coapMsg.coap_msgtype = coapMsgType;
    coapMsg.raiFlag = coapRai;
    ECOMM_TRACE(UNILOG_COAP, coap_msg_28, P_INFO, 0, "...coap_client_send.....");

    coapMsg.cmd_type = MSG_COAP_SEND;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.port = coapPort;

    if(coapSendMode == COAP_SEND_AT)/* for at mode, the payload is ascii */
    {
        if(coapPayload != NULL)
        {
            if(coapPayload[0] == '"')
            {
                if(coapPayload[coapPayloadLen-1] == '"')
                {
                    coapPayload[coapPayloadLen-1] = 0;
                }
                coapMsg.coap_payload.length = strlen(&coapPayload[1]);
                coapMsg.coap_payload.s = malloc(coapMsg.coap_payload.length +1);
                memset(coapMsg.coap_payload.s, 0, (coapMsg.coap_payload.length +1));
                memcpy(coapMsg.coap_payload.s, &coapPayload[1], coapMsg.coap_payload.length);
            }
            else
            {
                coapMsg.coap_payload.length = strlen(coapPayload);
                coapMsg.coap_payload.s = malloc(coapMsg.coap_payload.length +1);
                memset(coapMsg.coap_payload.s, 0, (coapMsg.coap_payload.length +1));
                memcpy(coapMsg.coap_payload.s, coapPayload, coapMsg.coap_payload.length);
            }
        
        }
    }
    else if(coapSendMode == COAP_SEND_CTRLZ)/* for at mode, the payload is hex or ascii, if it is hex string, maybe it is 1122330044AABB */
    {
        coapMsg.coap_payload.length = coapPayloadLen;
        if(coapPayloadLen > 0)
        {
            coapMsg.coap_payload.s = malloc(coapMsg.coap_payload.length +1);
            memset(coapMsg.coap_payload.s, 0, (coapMsg.coap_payload.length +1));
            memcpy(coapMsg.coap_payload.s, coapPayload, coapMsg.coap_payload.length);
        }
    }
    
    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);

#if COAP_SLP_ENABLE
    coapMaintainSlpInfo(coapNewClient->coap_id,coapMsgType);
#endif

    return COAP_OK;
}

int coap_client_addres(UINT32 reqHandle, int coapId, int resLen, char *resource)
{
    coap_client *coapNewClient = NULL;
    int ret = COAP_OK;
    int len = 0;
    coap_send_msg coapMsg;

    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    coap_reqHandle = reqHandle;

    memset(&coapMsg, 0, sizeof(coapMsg));
    coapMsg.cmd_type = MSG_COAP_ADD_RES;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.id = coapId;
    coapMsg.coapCnf.mode = COAP_CFG_ADDRES;
    coapMsg.coapCnf.is_from = COAP_CREATE_FROM_AT;
    coapMsg.coapCnf.dec_para1 = resLen;
    if(resource != NULL)
    {
        len = strlen(resource);
        coapMsg.coapCnf.str_para1 = malloc(len+1);
        memset(coapMsg.coapCnf.str_para1, 0, len+1);
        memcpy(coapMsg.coapCnf.str_para1, resource, len);
    }
    
    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);

    return ret;
}

int coap_client_head(UINT32 reqHandle, int coapId, int mode, int msgId, int tokenLen, char *token)
{
    coap_client *coapNewClient = NULL;
    int ret = COAP_OK;
    int len = 0;
    coap_send_msg coapMsg;
    UINT8 *coapToken = NULL;
    
    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    coap_reqHandle = reqHandle;

    memset(&coapMsg, 0, sizeof(coapMsg));
    coapMsg.cmd_type = MSG_COAP_HEAD;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.id = coapId;
    coapMsg.coapCnf.mode = COAP_CFG_HEAD;
    coapMsg.coapCnf.is_from = COAP_CREATE_FROM_AT;
    coapMsg.coapCnf.dec_para1 = mode;
    coapMsg.coapCnf.dec_para2 = msgId;
    coapMsg.coapCnf.dec_para3 = tokenLen;

    if(token != NULL)
    {
        len = strlen(token);

        coapToken = malloc(len+1);//coap_token_decode(token, len); 
        memset(coapToken, 0, (len+1));
        if(coapToken == NULL)
        {
            return COAP_ERR;
        }
        memcpy(coapToken, token, len);
        len = strlen((CHAR *)coapToken);
        coapMsg.coapCnf.dec_para3 = len;
        //len = strlen(coapToken);
        //coapMsg.send_data.coapCnf.str_para1 = malloc(len+1);
        //memset(coapMsg.send_data.coapCnf.str_para1, 0, len+1);
        //memcpy(coapMsg.send_data.coapCnf.str_para1, coapToken, len);
        coapMsg.coapCnf.str_para1 = (CHAR *)coapToken;
    }
        
    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);
    
    return ret;
}
int coap_client_option(UINT32 reqHandle, int coapId, int mode, char *optValue, int cmdEndFlag)
{
    coap_client *coapNewClient = NULL;
    coap_send_msg coapMsg;
    int ret = COAP_OK;
    int len = 0;

    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    coap_reqHandle = reqHandle;

    memset(&coapMsg, 0, sizeof(coapMsg));
    coapMsg.cmd_type = MSG_COAP_OPTION;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.id = coapId;
    coapMsg.coapCnf.mode = COAP_CFG_OPTION;
    coapMsg.coapCnf.is_from = COAP_CREATE_FROM_AT;
    coapMsg.coapCnf.dec_para1 = mode;
    coapMsg.cmd_end_flag = cmdEndFlag;

    if(optValue != NULL)
    {
        len = strlen(optValue);
        coapMsg.coapCnf.str_para1 = malloc(len+1);
        memset(coapMsg.coapCnf.str_para1, 0, len+1);
        memcpy(coapMsg.coapCnf.str_para1, optValue, len);
    }    
    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);

    return ret;
}

int coap_client_status(UINT32 reqHandle, int coapId, INT32 *outValue)
{
    coap_client *coapNewClient = NULL;

    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    if(coap_send_packet_count == 0)
    {
        *outValue = 4;
    }
    else
    {
        *outValue = 1;
    }

    return COAP_OK;
}

int coap_client_config(UINT32 reqHandle, int coapId, int mode, int value1,int value2,int value3)
{
    coap_client *coapNewClient = NULL;
    coap_send_msg coapMsg;
    int ret = COAP_OK;

    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    coap_reqHandle = reqHandle;

    memset(&coapMsg, 0, sizeof(coapMsg));
    coapMsg.cmd_type = MSG_COAP_CONFIG;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.id = coapId;
    coapMsg.coapCnf.mode = mode;
    coapMsg.coapCnf.is_from = COAP_CREATE_FROM_AT;
    coapMsg.coapCnf.dec_para1 = value1;
    coapMsg.coapCnf.dec_para2 = value2;
    coapMsg.coapCnf.dec_para3 = value3;

    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);

    return ret;
}

int coap_client_alisign(UINT32 reqHandle, int coapId, char* deviceId, char* deviceName, char*deviceScret, char* productKey, INT32 seq, char* signature)
{
    char *ali_clientID = NULL;
    char *ali_username = NULL;
    char *ali_signature = NULL;
    char *hmac_source = NULL;
    int ali_signature_len = 0;
    coap_client *coapNewClient = NULL;
    int deviceId_len = 0;
    int deviceName_len = 0;
    int deviceScret_len = 0;
    int productKey_len = 0;
    coap_send_msg coapMsg;

    if((deviceId == NULL)||(deviceName == NULL)||(deviceScret == NULL)||(productKey == NULL))
    {
        return COAP_CLIENT_ERR;
    }

    deviceId_len = strlen(deviceId);
    deviceName_len = strlen(deviceName);
    deviceScret_len = strlen(deviceScret);
    productKey_len = strlen(productKey);
    if((deviceId_len > COAP_DEV_INFO1_LEN)||(deviceName_len > COAP_DEV_INFO1_LEN)||(deviceScret_len > COAP_DEV_INFO2_LEN)||(productKey_len > COAP_DEV_INFO1_LEN))
    {
        return COAP_CLIENT_ERR;
    }

    coapNewClient = coap_find_client(coapId);
    if(coapNewClient == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    if(ali_hmac_falg == ALI_HMAC_NOT_USED)
    {
        return COAP_CLIENT_ERR;
    }

    hmac_source = malloc(256);
    if(hmac_source == NULL)
    {
        return COAP_CLIENT_ERR;
    }
    ali_clientID = malloc(128);
    if(ali_clientID == NULL)
    {
        free(hmac_source);
        return COAP_CLIENT_ERR;
    }
    ali_username = malloc(64);
    if(ali_username == NULL)
    {
        free(hmac_source);
        free(ali_clientID);
        return COAP_CLIENT_ERR;
    }
    ali_signature = malloc(64);
    if(ali_signature == NULL)
    {
        free(hmac_source);
        free(ali_clientID);
        free(ali_username);
        return COAP_CLIENT_ERR;
    }

    memset(coapNewClient->coap_dev.device_id, 0, COAP_DEV_INFO1_LEN);
    memset(coapNewClient->coap_dev.device_name, 0, COAP_DEV_INFO1_LEN);
    memset(coapNewClient->coap_dev.device_scret, 0, COAP_DEV_INFO2_LEN);
    memset(coapNewClient->coap_dev.product_key, 0, COAP_DEV_INFO1_LEN);

    memcpy(coapNewClient->coap_dev.device_id, deviceId, deviceId_len);
    memcpy(coapNewClient->coap_dev.device_name, deviceName, deviceName_len);
    memcpy(coapNewClient->coap_dev.device_scret, deviceScret, deviceScret_len);
    memcpy(coapNewClient->coap_dev.product_key, productKey, productKey_len);
    coapNewClient->coap_dev.seq = seq;
    
    memset(ali_clientID, 0, 128);
    memset(ali_username, 0, 64);
    memset(ali_signature, 0, 64);

    if(seq == COAP_ALI_RANDOM_DEF)
    {
        snprintf(hmac_source, 256, "clientId%sdeviceName%sproductKey%s", deviceId, deviceName, productKey);
    }
    else
    {
        snprintf(hmac_source, 256, "clientId%sdeviceName%sproductKey%sseq%d", deviceId, deviceName, productKey, seq);
    }
    
#ifdef FEATURE_MBEDTLS_ENABLE
    atAliHmacMd5((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)deviceScret, strlen(deviceScret));
#endif
    
    sprintf(ali_clientID,"%s|securemode=3,signmethod=hmacsha1", deviceId);
    sprintf(ali_username,"%s&%s",deviceName,productKey);

    ali_signature_len = strlen(ali_signature);
    memcpy(signature, ali_signature, ali_signature_len);

    free(hmac_source);
    free(ali_clientID);
    free(ali_username);
    free(ali_signature);

    memset(&coapMsg, 0, sizeof(coapMsg));
    coapMsg.cmd_type = MSG_COAP_ALISIGN;
    coapMsg.reqhandle = reqHandle;
    coapMsg.client_ptr = (void *)coapNewClient;
    coapMsg.id = coapId;
    coapMsg.coapCnf.mode = COAP_CFG_ALISIGN;
    coapMsg.coapCnf.is_from = COAP_CREATE_FROM_AT;

    xQueueSend(coap_send_msg_handle, &coapMsg, COAP_MSG_TIMEOUT);
    
    return COAP_OK;
}

#define COAP_CLIENT_SLEEP_INTERFACE

void coapSlpWaitMutex(CoapSlpWait_e state)
{
    coapSlpMutexWait |= state;
    ECOMM_TRACE(UNILOG_COAP, coapSlpWaitMutex0, P_VALUE, 2, "coapSlpWaitMutex=0x%x,%d",coapSlpMutexWait,state);
    coapCheckSafe2Sleep();
}

void coapSlpReleaseMutex(CoapSlpWait_e state)
{
    coapSlpMutexWait &= (~state);
    ECOMM_TRACE(UNILOG_COAP, coapSlpReleaseMutex0, P_VALUE, 2, "coapSlpWaitMutex=0x%x,%d",coapSlpMutexWait,state);
    coapCheckSafe2Sleep();
}

void coapSlpInit(void)
{
    uint8_t handle;
    
    ECOMM_TRACE(UNILOG_COAP, coapSlpInit0, P_SIG, 0, "coapSlp Init");
    if(slpManFindPlatVoteHandle("COAP",&handle) == RET_TRUE)
    {
        ECOMM_TRACE(UNILOG_COAP, coapSlpInit_1, P_ERROR, 0, "coapSlp find vote handle failed");
    }
    
    slpManApplyPlatVoteHandle("COAP",&coapSlpHandler);
}

void coapSlpDeInit(void)
{
    if(slpManGivebackPlatVoteHandle(coapSlpHandler) != RET_TRUE)
    {
        ECOMM_TRACE(UNILOG_COAP, coapSlpDeInit_1, P_ERROR, 0, "coapSlp vote handle give back failed");
    }
    else
    {
        coapSlpHandler = 0xff;
        ECOMM_TRACE(UNILOG_COAP, coapSlpDeInit_2, P_VALUE, 0, "coapSlp vote handle give back success");
    }
}

UINT32 coapCtxStoreNVMem(bool to_clear)
{
    UINT32 Count;
    OSAFILE file = NULL;

    file = OsaFopen("coap_nvm","wb");

    if(file == NULL)
    {
        ECOMM_TRACE(UNILOG_COAP, coapCtxStoreNVMem_1, P_VALUE, 0, "coapSlp open file fail");
        file = OsaFopen("coap_nvm","wb");
        if(file == NULL)
        {
            return COAP_ERR;
        }
    }

    coapSlpNVMem.coapSlpCtx.body_len = sizeof(coapSlpNVMem.coapSlpCtx.body);
    if(to_clear == true)
        coapSlpNVMem.coapSlpCtx.memcleared = COAP_NV_CLEARED;
    else
        coapSlpNVMem.coapSlpCtx.memcleared = 0;

    Count = OsaFwrite(&coapSlpNVMem.coapSlpCtx, 1, sizeof(coapSlpNVMem.coapSlpCtx), file);
    if(Count != (sizeof(coapSlpNVMem.coapSlpCtx)))
    {
        ECOMM_TRACE(UNILOG_COAP, coapCtxStoreNVMem_2, P_VALUE, 2, "coapSlp write file fail, Count=%d, coapSlpCtx size=%d", Count, (sizeof(coapSlpNVMem.coapSlpCtx)));
        OsaFclose(file);
        return COAP_ERR;
    }
    OsaFclose(file);
    
    ECOMM_TRACE(UNILOG_COAP, coapCtxStoreNVMem_3, P_VALUE, 0, "coapSlp store Context to FileSys");
    return COAP_OK;
}

UINT32 coapCtxRestoreNVMem(void)
{
    OSAFILE fp = PNULL;
    UINT32 Count;

    if(coapSlpNVMem.ctx_valid == 1)
    { 
        ECOMM_TRACE(UNILOG_COAP, coapCtxRestoreNVMem_0, P_SIG, 0, "coapSlp Context already restored");
        return COAP_OK;
    }

    fp = OsaFopen("coap_nvm", "rb+");   //read and write

    if(fp == NULL)
    {
        ECOMM_TRACE(UNILOG_COAP, coapCtxRestoreNVMem_1, P_SIG, 0, "coapSlp open fail");
        fp = OsaFopen("coap_nvm", "rb+");   //read and write
        if(fp == NULL)
        {
            memset(&coapSlpNVMem,0,sizeof(coapSlpNVMem));
            coapSlpNVMem.coapSlpCtx.body_len = sizeof(coapSlpNVMem.coapSlpCtx.body);
            coapSlpNVMem.coapSlpCtx.memcleared = COAP_NV_CLEARED;
            return COAP_ERR;
        }    
    }

    Count = OsaFread(&coapSlpNVMem.coapSlpCtx, 1, sizeof(coapSlpNVMem.coapSlpCtx), fp);
    if((sizeof(coapSlpNVMem.coapSlpCtx.body) != coapSlpNVMem.coapSlpCtx.body_len)||(Count != (sizeof(coapSlpNVMem.coapSlpCtx))))            // len error, use default value
    {
        memset(&coapSlpNVMem,0,sizeof(coapSlpNVMem));
        coapSlpNVMem.coapSlpCtx.body_len = sizeof(coapSlpNVMem.coapSlpCtx.body);
        coapSlpNVMem.coapSlpCtx.memcleared = COAP_NV_CLEARED;

        Count = OsaFwrite(&coapSlpNVMem.coapSlpCtx, 1, sizeof(coapSlpNVMem.coapSlpCtx), fp);
        if(Count != (sizeof(coapSlpNVMem.coapSlpCtx)))
        {
            ECOMM_TRACE(UNILOG_COAP, coapCtxRestoreNVMem_2, P_VALUE, 2, "coapSlp write file fail, Count=%d, coapSlpCtx size=%d", Count, (sizeof(coapSlpNVMem.coapSlpCtx)));
            OsaFclose(fp);
            return COAP_ERR;
        }

        ECOMM_TRACE(UNILOG_COAP, coapCtxRestoreNVMem_3, P_WARNING, 0, "coapSlp NVMem invalid, use default value");
    }
    OsaFclose(fp);

    ECOMM_TRACE(UNILOG_COAP, coapCtxRestoreNVMem_4, P_VALUE, 0, "coapSlp Restore Context");

    coapSlpNVMem.ctx_valid = 1;
    return COAP_OK;
}

bool coapSlpIsNVMemValid(void)
{
    if(coapSlpNVMem.ctx_valid == 1)
        return true;
    else
        return false;
}

UINT32 coapSlpNVMemInit(void *param)
{
    UINT32 Count;
    OSAFILE file = NULL;
    UINT32 slpSize;

    file = OsaFopen("coap_nvm","rb");       // read only

    // file doesn't exist, create new file when need write
    if(file == NULL)
    {
        ECOMM_TRACE(UNILOG_COAP, coapSlpNVMemInit_0, P_VALUE, 0, "coapSlp does not exist");
        return COAP_ERR;
    }
    // file already exists
    else
    {
        slpSize = sizeof(coapSlpNVMem.coapSlpCtx.restoreFlag)+sizeof(coapSlpNVMem.coapSlpCtx.body_len)+sizeof(coapSlpNVMem.coapSlpCtx.memcleared);
        Count = OsaFread(&coapSlpNVMem.coapSlpCtx, 1, slpSize, file);

        if(Count != slpSize)
        {
            ECOMM_TRACE(UNILOG_COAP, coapSlpNVMemInit_1, P_VALUE, 0, "coapSlp nv read fail");
            OsaFclose(file);
            return COAP_ERR;
        }

        OsaFclose(file);
        if(coapSlpNVMem.coapSlpCtx.memcleared != COAP_NV_CLEARED)
        {
            if(slpManGetLastSlpState() == SLP_ACTIVE_STATE)
            {
                coapSlpNVMem.ctx_valid = 1;
                memset(&coapSlpNVMem.coapSlpCtx.body,0,sizeof(coapSlpNVMem.coapSlpCtx.body));
                coapCtxStoreNVMem(true);
            }
            ECOMM_TRACE(UNILOG_COAP, coapSlpNVMemInit_2, P_VALUE, 0, "coapSlp NVMem Clear");
        }
    }
    return COAP_OK;
}

void coapSlpCheck2RestoreCtx(void)
{
    int stat = 0xff;

    if(coapSlpIsNVMemValid() == false)
    {
        stat = slpManGetLastSlpState();
        if((stat == SLP_SLP2_STATE) || (stat == SLP_HIB_STATE))
        {
            if(coapSlpNVMem.coapSlpCtx.restoreFlag == COAP_IS_CREATE)
            {
                coapCtxRestore();
            }
        }     
    }
}

void coapEngineInit(uint32_t autoreg_flag)
{
    bool need_clear = true;
    UINT32 ret = 0;

    if(need_clear)
    {
        ret = coapSlpNVMemInit(NULL);
        if(ret == COAP_ERR)
        {
            coapSlpNVMemInit(NULL);
        }
    }
    coapSlpCheck2RestoreCtx();
}

void coapMaintainSlpInfo(uint8_t id, uint8_t msgType)
{
    if(id >= COAP_CLIENT_NUMB_MAX)
    {
        ECOMM_TRACE(UNILOG_COAP, coapMaintainSlpInfo_0, P_VALUE, 0, "coapSlp client id error");
        return;
    }

    ECOMM_TRACE(UNILOG_COAP, coapMaintainSlpInfo_1, P_INFO, 5, "coapSlp message add CON Packet, CoapId = %u, msgType=%u, waitmsg=%u,%u, timeoutvalid=%u", id, msgType, coapSlpInfo[id].wait_msg, coapSlpInfo[id].last_msg_type, coapSlpInfo[id].timeout_vaild);

    if(msgType == COAP_MSGTYPE_CON)
    {
        if(coapSlpInfo[id].wait_msg == 1)
        {
            ECOMM_TRACE(UNILOG_COAP, coapMaintainSlpInfo_2, P_WARNING, 1, "coapSlp Last packet not finish, CoapId = %u", id);
        }

        coapSlpInfo[id].last_msg_type = msgType;
        coapSlpInfo[id].wait_msg = 1;
    }
    else if((msgType == COAP_MSGTYPE_RST) || (msgType == COAP_MSGTYPE_ACK))
    {
        if(coapSlpInfo[id].last_msg_type == COAP_MSGTYPE_CON)
        {
            coapSlpInfo[id].wait_msg = 0;
        }
        else
        {
            ECOMM_TRACE(UNILOG_COAP, coapMaintainSlpInfo_3, P_VALUE, 2, "coapSlp last_msg_type error, id=%d, last_msg_type=%d", id, coapSlpInfo[id].last_msg_type);
        }
    }
    coapCheckSafe2Sleep();
}

void coapDelSlpCtx(int id, coap_client *client)
{
    if(id >= COAP_CLIENT_NUMB_MAX)
    {
        ECOMM_TRACE(UNILOG_COAP, coapDelSlpCtx_0, P_VALUE, 0, "coapSlp client id error");
        return;
    }

    // make sure client has been delete 
    if((client->is_used == 0) && (client->coap_id == COAP_CLIENT_ID_DEFAULT))
    {
        memset(&coapSlpNVMem.coapSlpCtx.body[id],0,sizeof(coapSlpCtxBody_t));
    }
    else
    {
        ECOMM_TRACE(UNILOG_COAP, coapDelSlpCtx_1, P_VALUE, 1, "coapSlp is_used error, is_used=%d", client->is_used);
        return;
    }

    coapSlpNVMem.coapSlpCtx.restoreFlag = COAP_NOT_CREATE;
    coapSlpInfo[id].wait_msg = 0;
    coapSlpInfo[id].timeout_vaild = 0;

    ECOMM_TRACE(UNILOG_COAP, coapDelSlpCtx_2, P_SIG, 1, "coapSlp Delete Slp Context, CoapId = %u", id);
    coapCheckSafe2Sleep();
}

void coapAddSlpCtx(coap_client *client)
{
    int id = 0xff;
    // make sure client has been delete 
    if(client->is_used == 1)
    {
        id = client->coap_id;
        if(id >= COAP_CLIENT_NUMB_MAX)
        {
            ECOMM_TRACE(UNILOG_COAP, coapAddSlpCtx_0, P_VALUE, 0, "coapSlp client id error");
            return;
        }
        coapSlpNVMem.coapSlpCtx.restoreFlag = COAP_IS_CREATE;
        coapSlpNVMem.coapSlpCtx.body[id].valid = 1;
        coapSlpNVMem.coapSlpCtx.body[id].coap_id = client->coap_id;
        coapSlpNVMem.coapSlpCtx.body[id].coap_local_port = client->coap_local_port;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[0].opt_name = 1;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[1].opt_name = 3;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[2].opt_name = 4;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[3].opt_name = 5;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[4].opt_name = 6;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[5].opt_name = 7;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[6].opt_name = 8;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[7].opt_name = 11;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[8].opt_name = 12;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[9].opt_name = 14;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[10].opt_name = 15;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[11].opt_name = 17;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[12].opt_name = 20;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[13].opt_name = 23;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[14].opt_name = 27;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[15].opt_name = 28;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[16].opt_name = 35;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[17].opt_name = 39;
        coapSlpNVMem.coapSlpCtx.body[id].coap_opt[18].opt_name = 60;
        
    }
    else
    {
        ECOMM_TRACE(UNILOG_COAP, coapAddSlpCtx_1, P_VALUE, 1, "coapSlp is_used error, is_used=%d", client->is_used);
    }

    ECOMM_TRACE(UNILOG_COAP, coapAddSlpCtx_2, P_SIG, 1, "coapSlp Add Slp Context, CoapId = %u", id);
}

bool coapGetDstAddr(coap_client *client)
{
    bool ret = false;
    int id;
    
    if(coapSlpNVMem.ctx_valid != 1)
        return ret;

    if(client->is_used != COAP_CLIENT_USED)
    {
        ECOMM_TRACE(UNILOG_COAP, coapGetDstAddr_0, P_VALUE, 1, "coapSlp is_used error, is_used=%d", client->is_used);
        return false;
    }
    id = client->coap_id;
    if(id >= COAP_CLIENT_NUMB_MAX)
    {
        ECOMM_TRACE(UNILOG_COAP, coapGetDstAddr_1, P_VALUE, 0, "coapSlp client id error");
        return false;
    }

    if(coapSlpNVMem.coapSlpCtx.body[id].valid == 1)
    {   
        ECOMM_TRACE(UNILOG_COAP, coapGetDstAddr_2, P_SIG, 1, "coapSlp Dst addr Valid, CoapId = %u", id); 

        ret = true;
    }
    else
    {
        ECOMM_TRACE(UNILOG_COAP, coapGetDstAddr_3, P_SIG, 1, "coapSlp Dst addr Invalid, CoapId = %u", id); 

        ret = false;
    }

    return ret;
}

void coapCtxRestore(void)
{
    uint8_t i;
    int result;
    coapCtxRestoreNVMem();
    CoapCreateInfo coapInfo;
    coap_config coapCnf;
    
    for(i=0; i<COAP_CLIENT_NUMB_MAX; i++)
    {
        if(coapSlpNVMem.coapSlpCtx.body[i].valid == 1)
        {
            coapInfo.coapId = coapSlpNVMem.coapSlpCtx.body[i].coap_id;
            coapInfo.from = COAP_CREATE_FROM_RESTORE;
            
            coap_new_client(coapSlpNVMem.coapSlpCtx.body[i].coap_local_port);
            result = coap_client_create(0, coapSlpNVMem.coapSlpCtx.body[i].coap_local_port, coapInfo);

            if(coapSlpNVMem.coapSlpCtx.needRestoreConfig == 1)
            {
                if(coapSlpNVMem.coapSlpCtx.body[i].coap_resource.res_used != 0)
                {
                    memset(&coapCnf, 0 , sizeof(coapCnf));
                    
                    coapCnf.mode = COAP_CFG_ADDRES;
                    coapCnf.is_from = COAP_CREATE_FROM_RESTORE;
                    coapCnf.dec_para1 = coapSlpNVMem.coapSlpCtx.body[i].coap_resource.res_len;
                    coapCnf.str_para1 = coapSlpNVMem.coapSlpCtx.body[i].coap_resource.resource;
                    ECOMM_TRACE(UNILOG_COAP, coapCtxRestore_0, P_VALUE, 1, "coapSlp auto create, opt_name = %d", coapCnf.dec_para1);            
                    coap_config_client(coapInfo.coapId, &coapCnf);
                }
                if(coapSlpNVMem.coapSlpCtx.body[i].coap_head.mode != 0)
                {
                    memset(&coapCnf, 0 , sizeof(coapCnf));
                    
                    coapCnf.mode = COAP_CFG_HEAD;
                    coapCnf.is_from = COAP_CREATE_FROM_RESTORE;
                    coapCnf.dec_para1 = coapSlpNVMem.coapSlpCtx.body[i].coap_head.mode;
                    coapCnf.dec_para2 = coapSlpNVMem.coapSlpCtx.body[i].coap_head.msg_id;
                    coapCnf.str_para1 = coapSlpNVMem.coapSlpCtx.body[i].coap_head.token;
                    ECOMM_TRACE(UNILOG_COAP, coapCtxRestore_1, P_VALUE, 1, "coapSlp auto create, opt_name = %d", coapCnf.dec_para1);            
                    coap_config_client(coapInfo.coapId, &coapCnf);
                }
                
                coapClient[coapInfo.coapId].showra = coapSlpNVMem.coapSlpCtx.body[i].showra;
                coapClient[coapInfo.coapId].showrspopt = coapSlpNVMem.coapSlpCtx.body[i].showrspopt;
                coapClient[coapInfo.coapId].dtls_flag = coapSlpNVMem.coapSlpCtx.body[i].dtls_flag;
                coapClient[coapInfo.coapId].cloud = coapSlpNVMem.coapSlpCtx.body[i].cloud;
                coapClient[coapInfo.coapId].encrypt = coapSlpNVMem.coapSlpCtx.body[i].encrypt;
                memcpy(coapClient[coapInfo.coapId].coap_dev.device_id, coapSlpNVMem.coapSlpCtx.body[i].coap_dev.device_id, strlen(coapSlpNVMem.coapSlpCtx.body[i].coap_dev.device_id));
                memcpy(coapClient[coapInfo.coapId].coap_dev.device_name, coapSlpNVMem.coapSlpCtx.body[i].coap_dev.device_name, strlen(coapSlpNVMem.coapSlpCtx.body[i].coap_dev.device_name));
                memcpy(coapClient[coapInfo.coapId].coap_dev.device_scret, coapSlpNVMem.coapSlpCtx.body[i].coap_dev.device_scret, strlen(coapSlpNVMem.coapSlpCtx.body[i].coap_dev.device_scret));
                memcpy(coapClient[coapInfo.coapId].coap_dev.product_key, coapSlpNVMem.coapSlpCtx.body[i].coap_dev.product_key, strlen(coapSlpNVMem.coapSlpCtx.body[i].coap_dev.product_key));
                
                memset(&coapCnf, 0 , sizeof(coapCnf));           
                for(int j=0; j<19; j++)
                {
                    if(coapSlpNVMem.coapSlpCtx.body[i].coap_opt[j].opt_used != 0)
                    {
                        memset(&coapCnf, 0 , sizeof(coapCnf));
                        
                        coapCnf.mode = COAP_CFG_OPTION;
                        coapCnf.is_from = COAP_CREATE_FROM_RESTORE;
                        coapCnf.dec_para1 = coapSlpNVMem.coapSlpCtx.body[i].coap_opt[j].opt_name;
                        coapCnf.str_para1 = coapSlpNVMem.coapSlpCtx.body[i].coap_opt[j].opt_value;
                        ECOMM_TRACE(UNILOG_COAP, coapCtxRestore_2, P_VALUE, 1, "coapSlp auto create, opt_name = %d", coapCnf.dec_para1);            
                        coap_config_client(coapInfo.coapId, &coapCnf);
                    }
                }
                coap_reqHandle = coapSlpNVMem.coapSlpCtx.body[i].handle;

            }

            if(COAP_OK == result)
            {
    //                OsaCheck(id == coapSlpNVMem.coapSlpCtx.body[i].coap_id,id,coapSlpNVMem.coapSlpCtx.body[i].coap_id,0);
            }
            else
            {
                ECOMM_TRACE(UNILOG_COAP, coapCtxRestore_3, P_ERROR, 1, "coapSlp auto create failed, result = %u", result);
            }
        }
    }
}

void coapAddTimeoutInfo(void *client, uint32_t timeout)
{
    int id;
    coap_client *pClient = (coap_client *)client;
    
    if(pClient == NULL)
    {
        ECOMM_TRACE(UNILOG_COAP, coapAddTimeoutInfo_0, P_VALUE, 1, "coapSlp add timeout fail");
        return;
    }

    if(pClient->is_used != COAP_CLIENT_USED)
    {
        ECOMM_TRACE(UNILOG_COAP, coapAddTimeoutInfo_1, P_VALUE, 1, "coapSlp is_used error, is_used=%d", pClient->is_used);
        return;
    }

    id = pClient->coap_id;

    ECOMM_TRACE(UNILOG_COAP, coapAddTimeoutInfo_2, P_VALUE, 2, "coapSlp Wait CON Timeout id=%d, timout=%d",id, timeout);

    if(timeout != 0)
    {
        coapSlpInfo[id].timeout_vaild = 1;
        coapSlpInfo[id].timeout = timeout;
    }
    else
    {
        coapSlpInfo[id].timeout_vaild = 0;
        coapSlpInfo[id].wait_msg = 0;
    }

    coapCheckSafe2Sleep();
}

bool coapCheckSafe2Sleep(void)
{
    uint8_t reason = 0;
    uint8_t i;
    uint8_t cnt;
    slpManSlpState_t State;

    if(coapSlpHandler == 0xff)
    {
        ECOMM_TRACE(UNILOG_COAP, coapCheckSafe2Sleep_0, P_VALUE, 1, "coapSlp still in recovery process, skip vote");
        return false;
    }
    for(i=0;i<COAP_CLIENT_NUMB_MAX;i++)
    {
        if(coapSlpInfo[i].wait_msg == 1)
        {
            reason = 1;
            break;
        }

        if((coapSlpInfo[i].timeout_vaild == 1) && (coapSlpInfo[i].timeout <= COAP_DEEPSLP_THD))
        {
            reason = 2;
            break;
        }

        if(coapSlpMutexWait != 0)
        {
            reason = 3;
            break;
        }
    }

    if(reason == 0)         // can sleep
    {
        slpManPlatVoteEnableSleep(coapSlpHandler,SLP_SLP2_STATE);
        return true;
    }
    else
    {
        ECOMM_TRACE(UNILOG_COAP, coapCheckSafe2Sleep_1, P_VALUE, 1, "coapSlp can not sleep reason = %u", reason);

        if(slpManCheckVoteState(coapSlpHandler, &State, &cnt) == RET_TRUE)
        {
            if(cnt >= 1)
            {
                ECOMM_TRACE(UNILOG_COAP, coapCheckSafe2Sleep_2, P_VALUE, 0, "coapSlp already voted not sleep");
            }
            else
            {
                slpManPlatVoteDisableSleep(coapSlpHandler,SLP_SLP2_STATE);
            }
        }
        
        return false;
    }
}





