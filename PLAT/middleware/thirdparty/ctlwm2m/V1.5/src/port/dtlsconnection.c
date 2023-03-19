/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Christian Renz - Please refer to git log
 *
 *******************************************************************************/
#ifdef WITH_MBEDTLS
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dtlsconnection.h"
#include "ctiot_lwm2m_sdk.h"
#include "mbedtls/net_sockets.h"

#define COAP_PORT "5683"
#define COAPS_PORT "5684"
#define URI_LENGTH 256

osMutexId_t handshakeMutex = NULL;

int ct_create_socket(const char * portStr, int addressFamily)
{
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    {
        return -1;
    }

    for(p = res ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }

    freeaddrinfo(res);

    return s;
}

/********************* Security Obj Helpers **********************/
static char * security_get_uri(lwm2m_object_t * obj, int instanceId, char * uriBuffer, int bufferSize){
    int size = 1;
    lwm2m_data_t * dataP = ct_lwm2m_data_new(size);
    dataP->id = 0; // security server uri

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
            dataP->type == LWM2M_TYPE_STRING &&
            dataP->value.asBuffer.length > 0)
    {
        if (bufferSize > dataP->value.asBuffer.length){
            memset(uriBuffer,0,dataP->value.asBuffer.length+1);
            strncpy(uriBuffer,(char*)dataP->value.asBuffer.buffer,dataP->value.asBuffer.length);
            ct_lwm2m_data_free(size, dataP);
            return uriBuffer;
        }
    }
    ct_lwm2m_data_free(size, dataP);
    return NULL;
}

static int64_t security_get_mode(lwm2m_object_t * obj, int instanceId){
    int64_t mode;
    int size = 1;
    lwm2m_data_t * dataP = ct_lwm2m_data_new(size);
    dataP->id = 2; // security mode

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (0 != ct_lwm2m_data_decode_int(dataP,&mode))
    {
        ct_lwm2m_data_free(size, dataP);
        return mode;
    }

    ct_lwm2m_data_free(size, dataP);
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_security_get_mode_0, P_INFO, 0, "Unable to get security mode : use not secure mode");
    return LWM2M_SECURITY_MODE_NONE;
}

static char * security_get_public_id(lwm2m_object_t * obj, int instanceId, int * length){
    int size = 1;
    lwm2m_data_t * dataP = ct_lwm2m_data_new(size);
    dataP->id = 3; // public key or id

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
        dataP->type == LWM2M_TYPE_OPAQUE)
    {
        char * buff;

        buff = (char*)ct_lwm2m_malloc(dataP->value.asBuffer.length);
        if (buff != 0)
        {
            memcpy(buff, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            *length = dataP->value.asBuffer.length;
        }
        ct_lwm2m_data_free(size, dataP);

        return buff;
    } else {
        return NULL;
    }
}

static char * security_get_secret_key(lwm2m_object_t * obj, int instanceId, int * length){
    int size = 1;
    lwm2m_data_t * dataP = ct_lwm2m_data_new(size);
    dataP->id = 5; // secret key

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
        dataP->type == LWM2M_TYPE_OPAQUE)
    {
        char * buff;

        buff = (char*)ct_lwm2m_malloc(dataP->value.asBuffer.length);
        if (buff != 0)
        {
            memcpy(buff, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            *length = dataP->value.asBuffer.length;
        }
        ct_lwm2m_data_free(size, dataP);

        return buff;
    } else {
        return NULL;
    }
}

/********************* Security Obj Helpers Ends **********************/
#if 0
static int ct_dtls_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
    int ret = mbedtls_ssl_write(ssl, (unsigned char *) buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return 0;
    }
    else if (ret < 0)
    {
        return -1;
    }

    return ret;
}
int ct_send_data(mbedtls_connection_t *connP,
                    uint8_t * buffer,
                    size_t length)
{
    int nbSent;
    size_t offset;

    offset = 0;
        do nbSent = mbedtls_ssl_write( connP->ssl,  buffer, length );
    while( nbSent == MBEDTLS_ERR_SSL_WANT_READ ||
           nbSent == MBEDTLS_ERR_SSL_WANT_WRITE );

    if( nbSent < 0 )return -1;
    connP->lastSend = ct_lwm2m_gettime();
    return 0;
}
#endif
int ct_send_plain_data(mbedtls_connection_t *connP,
                    uint8_t * buffer,
                    size_t length,uint8_t sendOption)
{
    int nbSent;
    size_t offset;
    offset = 0;
    while (offset != length)
    {
        nbSent = ps_sendto(connP->sock, buffer + offset, length - offset, 0, (struct sockaddr *)&(connP->addr), connP->addrLen, sendOption, false);
        if (nbSent == -1) return -1;
        offset += nbSent;
    }
    return 0;
}

static int get_port(struct sockaddr *x)
{
   if (x->sa_family == AF_INET)
   {
       return ((struct sockaddr_in *)x)->sin_port;
   } else if (x->sa_family == AF_INET6) {
       return ((struct sockaddr_in6 *)x)->sin6_port;
   } else {
       ECOMM_TRACE(UNILOG_CTLWM2M, get_port, P_INFO, 0, "non IPV4 or IPV6 address");
       return  -1;
   }
}

static int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y)
{
    int portX = get_port(x);
    int portY = get_port(y);

    // if the port is invalid of different
    if (portX == -1 || portX != portY)
    {
        return 0;
    }

    // IPV4?
    if (x->sa_family == AF_INET && y->sa_family == AF_INET)
    {
        // compare V4 with V4
        return ((struct sockaddr_in *)x)->sin_addr.s_addr == ((struct sockaddr_in *)y)->sin_addr.s_addr;
    } else if (x->sa_family == AF_INET6 && y->sa_family == AF_INET6) {
        // IPV6 with IPV6 compare
        return memcmp(((struct sockaddr_in6 *)x)->sin6_addr.s6_addr, ((struct sockaddr_in6 *)y)->sin6_addr.s6_addr, 16) == 0;
    } else {
        // unknown address type
        ECOMM_TRACE(UNILOG_CTLWM2M, sockaddr_cmp, P_INFO, 0, "non IPV4 or IPV6 address");
        return 0;
    }
}
#if 0
mbedtls_ssl_context * dtls_ssl_new(void)
{
    int ret;
    mbedtls_ssl_context *ssl = NULL;
    mbedtls_ssl_config *conf = NULL;
    mbedtls_entropy_context *entropy = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg = NULL;
    mbedtls_timing_delay_context *timer = NULL;

    const char *pers = "dtls_client";
    ssl       = ct_lwm2m_malloc(sizeof(mbedtls_ssl_context));
    conf      = ct_lwm2m_malloc(sizeof(mbedtls_ssl_config));
    entropy   = ct_lwm2m_malloc(sizeof(mbedtls_entropy_context));
    ctr_drbg  = ct_lwm2m_malloc(sizeof(mbedtls_ctr_drbg_context));
    
    if (NULL == ssl || NULL == conf || NULL == entropy || NULL == ctr_drbg)
    {
        goto exit_fail;
    }

    timer = ct_lwm2m_malloc(sizeof(mbedtls_timing_delay_context));
    if (NULL == timer) goto exit_fail;
    
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
    mbedtls_entropy_init(entropy);
    
    if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0)
    {
        mbedtls_printf("mbedtls_ctr_drbg_seed failed: -0x%x", -ret);
        goto exit_fail;
    }
    
    mbedtls_printf("setting up the SSL structure");

    ret = mbedtls_ssl_config_defaults(conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT);

    if (ret != 0)
    {
        goto exit_fail;
    }
    
    mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);

    if ((ret = mbedtls_ssl_conf_psk(conf,
                                    psk,
                                    pskLen,
                                    pskId,
                                    strlen(pskId))) != 0)
    {
        goto exit_fail;
    }
    
    mbedtls_ssl_conf_dtls_cookies(conf, NULL, NULL,NULL);
    if(mbedtls_ssl_conf_max_frag_len(conf, 1) != 0)
    {
        printf("mbedtls_ssl_conf_max_frag_len error!!\n");
        goto exit_fail;
    }
    if ((ret = mbedtls_ssl_setup(ssl, conf)) != 0)
    {
        goto exit_fail;
    }
    
    mbedtls_ssl_set_timer_cb(ssl, timer, mbedtls_timing_set_delay,
                             mbedtls_timing_get_delay);
    
    return ssl;
exit_fail:  
    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

    if (ssl)
    {
        mbedtls_ssl_free(ssl);
        mbedtls_free(ssl);
    }

    return NULL;
}
#endif

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    ECOMM_STRING(UNILOG_CTLWM2M, my_debug_1, P_INFO, "mbedtls/%s/", (uint8_t*)file);
    ECOMM_TRACE(UNILOG_CTLWM2M, my_debug_2, P_INFO, 1,"line:%d",line);
    ECOMM_STRING(UNILOG_CTLWM2M, my_debug_3, P_INFO, "str/%s/", (uint8_t*)str);
}
                      
int ct_connection_init( mbedtls_connection_t * connP, char * host, char * port, void * userData)
{
    int ret = 0;
    const char *pers = "dtls_client";

    if(handshakeMutex == NULL){
      handshakeMutex = osMutexNew(NULL);
    }

    /*
     * Make sure memory references are valid.
     */
    mbedtls_net_init( &connP->server_fd );
    mbedtls_ssl_init( &connP->ssl );
    mbedtls_ssl_config_init( &connP->conf );
    mbedtls_ctr_drbg_init( &connP->ctr_drbg );

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold( 4 );
#endif

    /*
     * 0. Initialize the RNG and the session data
     */
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_0, P_INFO, 0, "0. Initialize the RNG and the session data");

    mbedtls_entropy_init( &connP->entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &connP->ctr_drbg, mbedtls_entropy_func, &connP->entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_0_1, P_INFO, 1, " failed  ! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret );
        goto exit;
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_0_2, P_INFO, 0, "ok");

    /*
     * 2. Start the connection
     */
    ECOMM_STRING(UNILOG_CTLWM2M, ct_connection_init_2, P_INFO, "  . Connecting to udp host/%s...", (uint8_t*)host);
    ECOMM_STRING(UNILOG_CTLWM2M, ct_connection_init_2_0, P_INFO, "  . Connecting to udp port/%s...", (uint8_t*)port);

    if( ( ret = mbedtls_net_connect( &connP->server_fd, host,
                                         port, MBEDTLS_NET_PROTO_UDP ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_2_1, P_INFO, 1, " failed  ! mbedtls_net_connect returned -0x%x\n\n", -ret );
        goto exit;
    }

    ret = mbedtls_net_set_block( &connP->server_fd );
    if( ret != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_2_2, P_INFO, 1, "failed  ! net_set_block() returned -0x%x\n\n", -ret );
        goto exit;
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_2_3, P_INFO, 0, "ok");

    /*
     * 3. Setup stuff
     */
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_3, P_INFO, 0, "  . Setting up the DTLS structure..." );

    if( ( ret = mbedtls_ssl_config_defaults( &connP->conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_3_1, P_INFO, 1, "  failed  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret );
        goto exit;
    }

    /* OPTIONAL is usually a bad choice for security, but makes interop easier
     * in this simplified example, in which the ca chain is hardcoded.
     * Production code should set a proper ca chain and use REQUIRED. */
    mbedtls_ssl_conf_authmode( &connP->conf, MBEDTLS_SSL_VERIFY_NONE );//ct is MBEDTLS_SSL_VERIFY_NONE
    
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    mbedtls_ssl_conf_handshake_timeout( &connP->conf, 10000, 30000 );
#endif /* MBEDTLS_SSL_PROTO_DTLS */

    mbedtls_ssl_conf_rng( &connP->conf, mbedtls_ctr_drbg_random, &connP->ctr_drbg );
    mbedtls_ssl_conf_dbg( &connP->conf, NULL/*my_debug*/, NULL);

    mbedtls_ssl_conf_read_timeout( &connP->conf, 5000 );

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
    int keyLen;
    char * key;
    key = security_get_secret_key(connP->securityObj, connP->securityInstId, &keyLen);

    if (MBEDTLS_PSK_MAX_LEN < keyLen)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_3_2, P_INFO, 2, "buffer_length=%d, keyLen=%d, psk too long\n", MBEDTLS_PSK_MAX_LEN, keyLen);
        ct_lwm2m_free(key);
        goto exit;
    }

    int idLen;
    char * id;
    id = security_get_public_id(connP->securityObj, connP->securityInstId, &idLen);

    if( ( ret = mbedtls_ssl_conf_psk( &connP->conf, (unsigned char*)key, keyLen, (unsigned char*)id, idLen ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_3_3, P_INFO, 1, " failed  ! mbedtls_ssl_conf_psk returned %d\r\n", ret );
        ct_lwm2m_free(key);
        ct_lwm2m_free(id);
        goto exit;
    }

    ct_lwm2m_free(key);
    ct_lwm2m_free(id);
#endif

    if( ( ret = mbedtls_ssl_setup( &connP->ssl, &connP->conf ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_3_4, P_INFO, 1, " failed  ! mbedtls_ssl_setup returned %d\r\n", ret );
        goto exit;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if( ( ret = mbedtls_ssl_set_hostname( &connP->ssl, host ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_init_3_5, P_INFO, 1, " failed  ! mbedtls_ssl_set_hostname returned %d\r\n", ret);
        goto exit;
    }
#endif

    mbedtls_ssl_set_bio( &connP->ssl, &connP->server_fd,
                         mbedtls_net_send, NULL, mbedtls_net_recv_timeout );

    mbedtls_ssl_set_timer_cb( &connP->ssl, &connP->timer, mbedtls_timing_set_delay,
                                            mbedtls_timing_get_delay );

    /*
     * 5. Verify the server certificate
     */
     
    /*
     * 6. Write the echo request
     */

    /*
     * 7. Read the echo response
     */

    /*
     * 8. Done, cleanly close the connection
     */

    /*
     * 9. Final clean-ups and exit
     */
exit:

    return( ret );
}

mbedtls_connection_t * ct_connection_find(mbedtls_connection_t * connList,
                               const struct sockaddr_storage * addr,
                               size_t addrLen)
{
    mbedtls_connection_t * connP;

    connP = connList;
    while (connP != NULL)
    {

       if (sockaddr_cmp((struct sockaddr*) (&connP->addr),(struct sockaddr*) addr)) {
            return connP;
       }

       connP = connP->next;
    }

    return connP;
}

mbedtls_connection_t * ct_connection_new_incoming(mbedtls_connection_t * connList,
                                       int sock,
                                       const struct sockaddr * addr,
                                       size_t addrLen)
{
    mbedtls_connection_t * connP;

    connP = (mbedtls_connection_t *)ct_lwm2m_malloc(sizeof(mbedtls_connection_t));
    
    if (connP != NULL)
    {
        memset(connP, 0, sizeof(mbedtls_connection_t));
        connP->sock = sock;
        memcpy(&(connP->addr), addr, addrLen);
        connP->addrLen = addrLen;
        connP->next = connList;
        connP->dtlsSession = 1;
        connP->lastSend = ct_lwm2m_gettime();
        connP->hasHS = FALSE;
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_new_incoming_1, P_INFO, 1, "new connP = 0x%x",connP);
    return connP;
}

mbedtls_connection_t * ct_connection_create(mbedtls_connection_t * connList,
                                 int sock,
                                 lwm2m_object_t * securityObj,
                                 int instanceId,
                                 lwm2m_context_t * lwm2mH,
                                 int addressFamily,
                                 void * userData)
{
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
    
    
    struct sockaddr *sa = NULL;
    socklen_t sl = 0;
    mbedtls_connection_t * connP = NULL;
    char uriBuf[URI_LENGTH];
    char * uri = "";
    char * host = "";
    char * port = "";
    /* Do name resolution with both IPv6 and IPv4 */
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;


    uri = security_get_uri(securityObj, instanceId, uriBuf, URI_LENGTH);
    if (uri == NULL) return NULL;
    // parse uri in the form "coaps://[host]:[port]"
    char * defaultport;
    if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
    {
        host = uri+strlen("coaps://");
        defaultport = COAPS_PORT;
    }
    else if (0 == strncmp(uri, "coap://", strlen("coap://")))
    {
        host = uri+strlen("coap://");
        defaultport = COAP_PORT;
    }
    else
    {
        return NULL;
    }
    port = strrchr(host, ':');
    if (port == NULL)
    {
        port = defaultport;
    }
    else
    {
        // remove brackets
        if (host[0] == '[')
        {
            host++;
            if (*(port - 1) == ']')
            {
                *(port - 1) = 0;
            }
            else
            {
                return NULL;
            }
        }
        // split strings
        *port = 0;
        port++;
    }
         
    if(  0 != getaddrinfo(host, port, &hints, &servinfo) ||  servinfo == NULL )
        return NULL;

    /* Try the sockaddrs until a connection succeeds */
    s = -1;
    for( p = servinfo; p != NULL && s == -1; p = p->ai_next )
    {
        s = socket( p->ai_family, p->ai_socktype,p->ai_protocol );
        if( s >= 0 )
        {
        
            sa = p->ai_addr;
            sl = p->ai_addrlen;
            if( connect( s, p->ai_addr, (int)p->ai_addrlen ) == -1 )
            {
                close( s );
                s = -1;
            }
        }
    }

    if (s >= 0)
    {
        //ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_create_1, P_INFO, 1, " connList = 0x%x", connList);
        connP = ct_connection_new_incoming(connList, sock, sa, sl);
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_create_2, P_INFO, 3, "connList = 0x%x new connP = 0x%x next = 0x%x",connList, connP, connP->next);
        close(s);
        
        if (connP != NULL)
        {
            connP->securityObj = securityObj;
            connP->securityInstId = instanceId;
            connP->lwm2mH = lwm2mH;

            if(security_get_mode(connP->securityObj, connP->securityInstId) != LWM2M_SECURITY_MODE_NONE)
            {
                if(ct_connection_init(connP, host, port, userData) != 0){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_create_3, P_INFO, 1, "dtls init failed free connP:0x%x", connP);
                    ct_connection_free(connP);
                    if (NULL != servinfo) freeaddrinfo(servinfo);
                    return NULL;
                }
            }
            else
            {
                connP->dtlsSession = 0;
            }
        }
    }

    if (NULL != servinfo) freeaddrinfo(servinfo);
    return connP;
}

void ct_connection_free(mbedtls_connection_t * connList)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_free_1, P_INFO, 1, "connList = 0x%x",connList);
    while (connList != NULL)
    {
        mbedtls_connection_t * nextP;

        nextP = connList->next;
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_free_2, P_INFO, 2, "connList->dtlsSession = %d, next = 0x%x",connList->dtlsSession,nextP);
        if (connList->dtlsSession != 0) {
            mbedtls_net_free( &connList->server_fd );
            mbedtls_ssl_free( &connList->ssl );
            mbedtls_ssl_config_free( &connList->conf );
            mbedtls_ctr_drbg_free( &connList->ctr_drbg );
            mbedtls_entropy_free( &connList->entropy );
        }
        ct_lwm2m_free(connList);

        connList = nextP;
    }
    if(handshakeMutex != NULL){
        osMutexDelete(handshakeMutex);
        handshakeMutex = NULL;
    }
}

static int ct_connection_send(mbedtls_connection_t *connP, uint8_t * buffer, size_t length,uint8_t sendOption, void* userdata)
{
#ifdef  FEATURE_REF_AT_ENABLE
    ctiot_funcv1_status_t atStatus[1] = {0};
#endif
    ctiot_funcv1_client_data_t * dataP = (ctiot_funcv1_client_data_t *)userdata;
    int result = 0;
    if(connP->dtlsSession == 0){
        result = ct_send_plain_data(connP, buffer, length, sendOption);
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_1, P_INFO, 1, "no dtlsSession send plain data result=%d",result);
        return result;
    }else{
        osMutexAcquire(handshakeMutex, osWaitForever);
#ifndef  FEATURE_REF_AT_ENABLE
        if(connP->hasHS == FALSE || (dataP->natType == 0) && ((ct_lwm2m_gettime() - connP->lastSend) > DTLS_NAT_TIMEOUT))
#else
        if(connP->hasHS == FALSE || (dataP->natType == 0) && ((ct_lwm2m_gettime() - connP->lastSend) > DTLS_NAT_TIMEOUT || dataP->resetHandshake))
#endif
        {
#ifdef  FEATURE_REF_AT_ENABLE
            if(dataP->resetHandshake){
                dataP->resetHandshake = FALSE;
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_2_1, P_INFO, 0, "user control rehandshake");
            }
            // we need to rehandhake because our source IP/port probably changed for the server
            dataP->handshakeResult = HANDSHAKE_ING;
#endif
            ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_2, P_INFO, 2, "need to rehandshake.connP->hasHS=%d,dataP->natType=%d",connP->hasHS,dataP->natType);
            result = ct_connection_rehandshake(connP, false);
            if ( result != 0 )
            {
#ifdef  FEATURE_REF_AT_ENABLE
                dataP->handshakeResult = HANDSHAKE_FAIL;
                atStatus->baseInfo=ct_lwm2m_strdup(HS3);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, (void *) atStatus, 0);
#endif
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_2_2, P_INFO, 0, "can't send due to rehandshake error");
                osMutexRelease(handshakeMutex);
                return result;
            }
            else
            {
#ifdef  FEATURE_REF_AT_ENABLE
                dataP->handshakeResult = HANDSHAKE_OVER;
                atStatus->baseInfo=ct_lwm2m_strdup(HS0);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, (void *) atStatus, 0);
#endif
                connP->hasHS = TRUE;
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_2_3, P_INFO, 0, "rehandshake success");
            }
        }
        int ret;
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_3, P_INFO, 1, "  > Write to server: RAI(%d)", sendOption);

        connP->ssl.rai = sendOption;
        do ret = mbedtls_ssl_write( &connP->ssl, (unsigned char *) buffer, length );
        while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
               ret == MBEDTLS_ERR_SSL_WANT_WRITE );
        if( ret < 0 )
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_3_1, P_INFO, 1, " failed  ! mbedtls_ssl_write returned %d", length );
            osMutexRelease(handshakeMutex);
            return ret;
        }
        length = ret;
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_3_2, P_INFO, 1, " %d bytes written\n\n", length );
        connP->ssl.rai = 0;
        osMutexRelease(handshakeMutex);

        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_send_3_3, P_INFO, 2, " ct_lwm2m_gettime() = %d, connP->lastSend = %d", ct_lwm2m_gettime(), connP->lastSend );
        connP->lastSend = ct_lwm2m_gettime();
    }

    return 0;
}

int ct_connection_handle_packet(uint8_t* buf, int length, mbedtls_connection_t *connP){
    int ret, len;
    //ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_1, P_INFO, 0, "  < Read from server:" );
    len = length - 1;

    osMutexAcquire(handshakeMutex, osWaitForever);
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_1_1, P_INFO, 0, ">>mbedtls_ssl_read" );
    do{
        ret = mbedtls_ssl_read( &connP->ssl, buf, len );

    }
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );
    osMutexRelease(handshakeMutex);
    if( ret <= 0 )
    {
        switch( ret )
        {
            case MBEDTLS_ERR_SSL_TIMEOUT:
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_2, P_INFO, 0, "<<mbedtls_ssl_read timeout");
                return ret;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_3, P_INFO, 0, " connection was closed gracefully\r\n . Closing the connection...");

                /* No error checking, the connection might be closed already */
                do ret = mbedtls_ssl_close_notify( &connP->ssl );
                while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );
                ret = 0;

                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_3_1, P_INFO, 0, " done");
                return ret;

            default:
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_4, P_INFO, 1, " mbedtls_ssl_read returned -0x%x\r\n", -ret);
                return ret;
        }
    }

    len = ret;
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_handle_packet_5, P_INFO, 1, "<<mbedtls_ssl_read %d bytes", len);
    
    ct_handle_packet(connP->lwm2mH, buf, len, (void*)connP);
    return len;
}

int ct_connection_rehandshake(mbedtls_connection_t *connP, bool sendCloseNotify)
{
    int ret = 0;
    // if not a dtls connection we do nothing
    if (connP->dtlsSession == 0) {
        return 0;
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_rehandshake_1, P_INFO, 0, "  . Restarting connection from same port...");

    if( ( ret = mbedtls_ssl_session_reset( &connP->ssl ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_rehandshake_2, P_INFO, 1, " failed  ! mbedtls_ssl_session_reset returned -0x%x", -ret);
        return ret;
    }

    while( ( ret = mbedtls_ssl_handshake( &connP->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ct_connection_rehandshake_3, P_INFO, 1, " failed  ! mbedtls_ssl_handshake returned -0x%x", -ret);
            return ret;
        }
        osDelay(100);//wait 100ms
    }

    ECOMM_STRING(UNILOG_CTLWM2M, ct_connection_rehandshake_4, P_INFO, "  [ Ciphersuite is %s ] handshake ok.", (uint8_t*)mbedtls_ssl_get_ciphersuite( &connP->ssl ));
    return ret;
}

uint8_t ct_lwm2m_buffer_send(void * sessionH,
                          uint8_t * buffer,
                          size_t length,
                          void * userdata,uint8_t sendOption)
{
    mbedtls_connection_t * connP = (mbedtls_connection_t*) sessionH;

    if (connP == NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_lwm2m_buffer_send_1, P_INFO, 1, "#> failed sending %d bytes, missing connection\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR ;
    }

    if (ct_connection_send(connP, buffer, length,sendOption, userdata) < 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_lwm2m_buffer_send_2, P_INFO, 1, "#> failed sending %lu bytes\r\n", length);
        return MBEDTLS_FAILED ;
    }

    return COAP_NO_ERROR;
}

bool ct_lwm2m_session_is_equal(void * session1,
                            void * session2,
                            void * userData)
{
    return (session1 == session2);
}

#if 0
void lwm2m_dtls_destroy(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_config           *conf = NULL;
    mbedtls_ctr_drbg_context     *ctr_drbg = NULL;
    mbedtls_entropy_context      *entropy = NULL;
    mbedtls_net_context          *server_fd = NULL;
    mbedtls_timing_delay_context *timer = NULL;

    if (ssl == NULL)
    {
        return;
    }

    conf       = (mbedtls_ssl_config *)ssl->conf;
    server_fd  = (mbedtls_net_context *)ssl->p_bio;
    timer      = (mbedtls_timing_delay_context *)ssl->p_timer;

    if (conf)
    {
        ctr_drbg   = conf->p_rng;

        if (ctr_drbg)
        {
            entropy =  ctr_drbg->p_entropy;
        }
    }

    if (server_fd)
    {
        mbedtls_net_free(server_fd);
    }

    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
        ssl->conf = NULL; //  need by mbedtls_debug_print_msg(), see mbedtls_ssl_free(ssl)
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

    mbedtls_ssl_free(ssl);
    mbedtls_free(ssl);
}
#endif

#endif

