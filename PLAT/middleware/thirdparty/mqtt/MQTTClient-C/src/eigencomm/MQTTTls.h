
#ifndef MQTT_DTLS_H
#define MQTT_DTLS_H

#include "commonTypedef.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/certs.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"




#define MQTT_MAX_TIMEOUT  (10 * 60)  //10 min


typedef struct mqttsClientSslTag
{
    mbedtls_ssl_context       sslContext;
    mbedtls_net_context       netContext;
    mbedtls_ssl_config        sslConfig;
    mbedtls_entropy_context   entropyContext;
    mbedtls_ctr_drbg_context  ctrDrbgContext;
    mbedtls_x509_crt_profile  crtProfile;
    mbedtls_x509_crt          caCert;
    mbedtls_x509_crt          clientCert;
    mbedtls_pk_context        pkContext;
}mqttsClientSsl;

typedef struct mqttsClientContextTag
{
    int socket;
    int timeout_s;
    int timeout_r;
    int isMqtts;
    int method;
    UINT16 port;
    size_t sendBufSize;
    size_t readBufSize;
    unsigned char *sendBuf;
    unsigned char *readBuf;
    
    mqttsClientSsl * ssl;
    const char *caCert;
    const char *clientCert;
    const char *clientPk;
    const char *hostName;
    char *psk_key;
    char *psk_identity;
    int caCertLen;
    int clientCertLen;
    int clientPkLen;
}mqttsClientContext;



int mqttSslConn(mqttsClientContext* context, char* host);
int mqttSslSend(mqttsClientContext* context, unsigned char* buf, int len);
int mqttSslRecv(mqttsClientContext* context, unsigned char* buf, int minLen, int maxLen, int* pReadLen);
int mqttSslRead(mqttsClientContext* context, unsigned char *buffer, int len, int timeout_ms);
int mqttSslClose(mqttsClientContext* context);

#endif

