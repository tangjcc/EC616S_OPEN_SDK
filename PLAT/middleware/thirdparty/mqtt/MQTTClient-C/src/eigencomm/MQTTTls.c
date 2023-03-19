
#ifdef FEATURE_MBEDTLS_ENABLE
#include "sha1.h"
#include "sha256.h"
#include "md5.h"
#endif

#include "MQTTTls.h"
#include "MQTTFreeRTOS.h"
#include "error.h"


int mqttSslRandom(void *p_rng, unsigned char *output, size_t output_len)
{
    uint32_t rnglen = output_len;
    uint8_t   rngoffset = 0;

    while (rnglen > 0) 
    {
        *(output + rngoffset) = (unsigned char)rand();
        rngoffset++;
        rnglen--;
    }
    return 0;
}

static void mqttSslDebug(void *ctx, int level, const char *file, int line, const char *str)
{
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_00, P_SIG, 3, "%d(%d):%d", file, line, str);
//    DBG("%s", str);
}

int mqttSslNonblockRecv(void *netContext, UINT8 *buf, size_t len)
{
    int ret;
    int fd = ((mbedtls_net_context *)netContext)->fd;
    
    if(fd < 0)
    {
        return MQTT_MBEDTLS_ERR;
    }
    
    ret = (int)recv(fd, buf, len, MSG_DONTWAIT);
    if(ret<0)
    {
        if( errno == EPIPE || errno == ECONNRESET) 
        {
            return (MBEDTLS_ERR_NET_CONN_RESET);
        }
        
        if( errno == EINTR ) 
        {
            return (MBEDTLS_ERR_SSL_WANT_READ);
        }
        
        if(ret == -1 && errno == EWOULDBLOCK) 
        {
            return ret;
        }
        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }
    return (ret);
}

int mqttSslConn(mqttsClientContext* context, char* host)
{
    INT32 value;
    mqttsClientSsl *ssl;
    const char *custom = "mqtts";
    char port[10] = {0};
    INT32 authmode = MBEDTLS_SSL_VERIFY_NONE;
    UINT32 flag;
    
    context->ssl = malloc(sizeof(mqttsClientSsl));
    ssl = context->ssl;
    
    /*
     * 0. Initialize the RNG and the session data
     */
#if defined(MBEDTLS_DEBUG_C)
    //mbedtls_debug_set_threshold((int)DEBUG_LEVEL);
#endif
    mbedtls_net_init(&ssl->netContext);
    mbedtls_ssl_init(&ssl->sslContext);
    mbedtls_ssl_config_init(&ssl->sslConfig);
    mbedtls_x509_crt_init(&ssl->caCert);
    mbedtls_x509_crt_init(&ssl->clientCert);
    mbedtls_pk_init(&ssl->pkContext);
    mbedtls_ctr_drbg_init(&ssl->ctrDrbgContext);
    mbedtls_entropy_init(&ssl->entropyContext);
    if((context->psk_key != NULL)&&(context->psk_identity != NULL))
    {
        mbedtls_ssl_conf_psk(&ssl->sslConfig, (const unsigned char *)context->psk_key, strlen(context->psk_key),
                             (const unsigned char *)context->psk_identity, strlen(context->psk_identity));
    }
    if((value = mbedtls_ctr_drbg_seed(&ssl->ctrDrbgContext,
                             mbedtls_entropy_func,
                             &ssl->entropyContext,
                             (const unsigned char*)custom,
                             strlen(custom))) != 0) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_0, P_SIG, 1, "mbedtls_ctr_drbg_seed failed, value:-0x%x.", -value);
        return MQTT_MBEDTLS_ERR;
    }
    snprintf(port, sizeof(port), "%d", context->port);
    /*
     * 0. Initialize certificates
     */
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_1, P_SIG, 0, "STEP 0. Loading the CA root certificate ...");
    if (NULL != context->caCert) 
    {
        authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
        if (0 != (value = mbedtls_x509_crt_parse(&(ssl->caCert), (const unsigned char *)context->caCert, context->caCertLen))) 
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_2, P_SIG, 1, "failed ! value:-0x%04x", -value);
            return MQTT_MBEDTLS_ERR;
        }
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_3, P_SIG, 1, " ok (%d skipped)", value);
    }

    /* Setup Client Cert/Key */
    if (context->clientCert != NULL && context->clientPk != NULL) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_4, P_SIG, 0, "STEP 0. start prepare client cert ...");
        value = mbedtls_x509_crt_parse(&(ssl->clientCert), (const unsigned char *) context->clientCert, context->clientCertLen);
        if (value != 0) 
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_5, P_SIG, 1, " failed!  mbedtls_x509_crt_parse returned -0x%x\n", -value);
            return MQTT_MBEDTLS_ERR;
        }

        ECOMM_TRACE(UNILOG_MQTT, mqttTls_6, P_SIG, 1, "STEP 0. start mbedtls_pk_parse_key[%s]", context->clientPk);
        value = mbedtls_pk_parse_key(&ssl->pkContext, (const unsigned char *) context->clientPk, context->clientPkLen, NULL, 0);
        if (value != 0) 
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_7, P_SIG, 1, " failed\n  !  mbedtls_pk_parse_key returned -0x%x\n", -value);
            return MQTT_MBEDTLS_ERR;
        }
    }
   
    /*
     * 1. Start the connection
     */
    snprintf(port, sizeof(port), "%d", context->port);
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_8, P_SIG, 2, "STEP 1. Connecting to /%s/%s...", host, port);
    if (0 != (value = mbedtls_net_connect(&ssl->netContext, host, port, MBEDTLS_NET_PROTO_TCP))) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_9, P_SIG, 1, " failed ! mbedtls_net_connect returned -0x%04x", -value);
        return MQTT_MBEDTLS_ERR;
    }
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_10, P_SIG, 0, " ok");
    
    /*
     * 2. Setup stuff
     */
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_11, P_SIG, 0, "STEP 2. Setting up the SSL/TLS structure...");
    if ((value = mbedtls_ssl_config_defaults(&(ssl->sslConfig), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_12, P_SIG, 1, " failed! mbedtls_ssl_config_defaults returned %d", value);
        return MQTT_MBEDTLS_ERR;
    }

    mbedtls_ssl_conf_max_version(&ssl->sslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl->sslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

    memcpy(&(ssl->crtProfile), ssl->sslConfig.cert_profile, sizeof(mbedtls_x509_crt_profile));
    mbedtls_ssl_conf_authmode(&(ssl->sslConfig), authmode);

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    if ((mbedtls_ssl_conf_max_frag_len(&(ssl->sslConfig), MBEDTLS_SSL_MAX_FRAG_LEN_1024)) != 0) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_13, P_SIG, 0, "mbedtls_ssl_conf_max_frag_len returned\r\n");       
        return MQTT_MBEDTLS_ERR;
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_ssl_conf_cert_profile(&ssl->sslConfig, &ssl->crtProfile);
    mbedtls_ssl_conf_ca_chain(&(ssl->sslConfig), &(ssl->caCert), NULL);
    if(context->clientCert) 
    {
        if ((value = mbedtls_ssl_conf_own_cert(&(ssl->sslConfig), &(ssl->clientCert), &(ssl->pkContext))) != 0) 
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_14, P_SIG, 1, " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n", value);  
            return MQTT_MBEDTLS_ERR;
        }
    }
#endif
    mbedtls_ssl_conf_rng(&(ssl->sslConfig), mqttSslRandom, &(ssl->ctrDrbgContext));
    mbedtls_ssl_conf_dbg(&(ssl->sslConfig), mqttSslDebug, NULL);

    if(context->timeout_r > 0) 
    {
        UINT32 recvTimeout;
        recvTimeout = context->timeout_r > MQTT_MAX_TIMEOUT ? MQTT_MAX_TIMEOUT * 1000 : context->timeout_r * 1000;
        mbedtls_ssl_conf_read_timeout(&(ssl->sslConfig), recvTimeout);
    }
    if ((value = mbedtls_ssl_setup(&(ssl->sslContext), &(ssl->sslConfig))) != 0) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_15, P_SIG, 1, "failed! mbedtls_ssl_setup returned %d", value);
        return MQTT_MBEDTLS_ERR;
    }
    
    if(context->hostName != NULL)
    {
        mbedtls_ssl_set_hostname(&(ssl->sslContext), context->hostName);
        //mbedtls_ssl_set_hostname(&(ssl->sslContext), "OneNET MQTTS");
    }
    else
    {
        mbedtls_ssl_set_hostname(&(ssl->sslContext), host);
    }
    
    mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
    
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_16, P_SIG, 0, " ok");

    /*
     * 3. Handshake
     */
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_17, P_SIG, 0, "STEP 3. Performing the SSL/TLS handshake...");    
    while ((value = mbedtls_ssl_handshake(&(ssl->sslContext))) != 0) 
    {
        if ((value != MBEDTLS_ERR_SSL_WANT_READ) && (value != MBEDTLS_ERR_SSL_WANT_WRITE)) 
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_18, P_SIG, 1, "failed  ! mbedtls_ssl_handshake returned 0x%04x", value);
            return MQTT_MBEDTLS_ERR;
        }
    }
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_19, P_SIG, 0, " ok");

    /*
     * 4. Verify the server certificate
     */
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_20, P_SIG, 0, "STEP 4. Verifying peer X.509 certificate..");
    flag = mbedtls_ssl_get_verify_result(&(ssl->sslContext));
    if (flag != 0) 
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTls_21, P_SIG, 0, " failed  ! verify result not confirmed.");
        return MQTT_MBEDTLS_ERR;
    }
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_22, P_SIG, 0, "caCert varification ok");

    return MQTT_CONN_OK;
}

//INT32 mqttSslSend(mbedtls_ssl_context* sslContext, const char* buf, UINT16 len)
int mqttSslSend(mqttsClientContext* context, unsigned char* buf, int len)
{
    INT32 waitToSend = len;
    INT32 hasSend = 0;

    do
    {
        hasSend = mbedtls_ssl_write(&(context->ssl->sslContext), (unsigned char *)(buf + len - waitToSend), waitToSend);
        if(hasSend > 0)
        {
            waitToSend -= hasSend;
        }
        else if(hasSend == 0)
        {
            return MQTT_CONN_OK;
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_23, P_SIG, 0, "mqtt_client(ssl): send failed \n");
            return MQTT_CONN;
        }
    }while(waitToSend>0);

    return MQTT_CONN_OK;
}

int mqttSslRecv(mqttsClientContext* context, unsigned char* buf, int minLen, int maxLen, int* pReadLen) //0 on success, err code on failure
{
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_24, P_INFO, 2, "Trying to read between %d and %d bytes", minLen, maxLen);
    INT32 readLen = 0;
    INT32 ret;
    
    while (readLen < maxLen) 
    {
        mqttsClientSsl *ssl = (mqttsClientSsl *)context->ssl;
        if (readLen < minLen) 
        {
            mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), mbedtls_net_send, mbedtls_net_recv, NULL);
            ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char *)(buf+readLen), minLen-readLen);
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_30, P_INFO, 1, "mbedtls_ssl_read [blocking] return:0x%x", ret);
            if(ret == 0)
            {
                ECOMM_TRACE(UNILOG_MQTT, mqttTls_31, P_INFO, 0, "mbedtls_ssl_read [blocking] return 0 connect error");
                return MQTT_CONN;
            }
        } 
        else 
        {
            mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), mbedtls_net_send, mqttSslNonblockRecv, NULL);
            ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char*)(buf+readLen), maxLen-readLen);
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_32, P_INFO, 1, "mbedtls_ssl_read [not blocking] return:0x%x", ret);
            if(ret == -1 && errno == EWOULDBLOCK) 
            {
                ECOMM_TRACE(UNILOG_MQTT, mqttTls_33, P_INFO, 0, "mbedtls_ssl_read [not blocking] errno == EWOULDBLOCK");
                break;
            }
        }
        if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            return MQTT_CLOSED;
        }

        if (ret > 0) 
        {
            readLen += ret;
        } 
        else if ( ret == 0 ) 
        {
            break;
        } 
        else 
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttTls_34, P_INFO, 1, "Connection error (recv returned %d)", ret);
            *pReadLen = readLen;
            return MQTT_CONN;
        }
    }
    ECOMM_TRACE(UNILOG_MQTT, mqttTls_35, P_INFO, 1, "Read %d bytes", readLen);
    buf[readLen] = '\0';    // DS makes it easier to see what's new.
    *pReadLen = readLen;
    return MQTT_CONN_OK;
}

int mqttSslRead(mqttsClientContext* context, unsigned char *buffer, int len, int timeout_ms)
{
    UINT32          readLen = 0;
    static int      net_status = 0;
    INT32             ret = -1;
    char            err_str[33];
    mqttsClientSsl *ssl = (mqttsClientSsl *)context->ssl;

    mbedtls_ssl_conf_read_timeout(&(ssl->sslConfig), timeout_ms);
    while (readLen < len) {
        ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char *)(buffer + readLen), (len - readLen));
        if (ret > 0) {
            readLen += ret;
            net_status = 0;
        } else if (ret == 0) {
            /* if ret is 0 and net_status is -2, indicate the connection is closed during last call */
            return (net_status == -2) ? net_status : readLen;
        } else {
            if (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == ret) {
                mbedtls_strerror(ret, err_str, sizeof(err_str));
                printf("ssl recv error: code = -0x%04X, err_str = '%s'\n", -ret, err_str);
                net_status = -2; /* connection is closed */
                break;
            } else if ((MBEDTLS_ERR_SSL_TIMEOUT == ret)
                       || (MBEDTLS_ERR_SSL_CONN_EOF == ret)
                       || (MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED == ret)
                       || (MBEDTLS_ERR_SSL_NON_FATAL == ret)) {
                /* read already complete */
                /* if call mbedtls_ssl_read again, it will return 0 (means EOF) */

                return readLen;
            } else {
                mbedtls_strerror(ret, err_str, sizeof(err_str));
                printf("ssl recv error: code = -0x%04X, err_str = '%s'\n", -ret, err_str);
                net_status = -1;
                return -1; /* Connection error */
            }
        }
    }

    return (readLen > 0) ? readLen : net_status;
}

int mqttSslClose(mqttsClientContext* context)
{
    mqttsClientSsl *ssl = (mqttsClientSsl *)context->ssl;
    /*context->clientCert = NULL;
    context->caCert = NULL;
    context->clientPk = NULL; let up level free it*/
    if(ssl == NULL)
        return MQTT_MBEDTLS_ERR;

    mbedtls_ssl_close_notify(&(ssl->sslContext));
    mbedtls_net_free(&(ssl->netContext));
    mbedtls_x509_crt_free(&(ssl->caCert));
    mbedtls_x509_crt_free(&(ssl->clientCert));
    mbedtls_pk_free(&(ssl->pkContext));
    mbedtls_ssl_free(&(ssl->sslContext));
    mbedtls_ssl_config_free(&(ssl->sslConfig));
    mbedtls_ctr_drbg_free(&(ssl->ctrDrbgContext));
    mbedtls_entropy_free(&(ssl->entropyContext));

    free(ssl);

    ECOMM_TRACE(UNILOG_MQTT, mqttTls_36, P_INFO, 0, "mqtt tls close ok");
    return MQTT_CONN_OK;
}

 
