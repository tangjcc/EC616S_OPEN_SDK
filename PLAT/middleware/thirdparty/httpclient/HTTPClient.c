/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "netdb.h"
#include "sockets.h"
#include "mbedtls/debug.h"
#include "cmsis_os2.h"

#include "HTTPClient.h"

#include "debug_log.h"

#define HTTP_CONNECT_TIMEOUT_EN



#define CHUNK_SIZE 2000
#define MAXHOST_SIZE 128
#define TEMPBUF_SIZE 512

#define MAX_TIMEOUT  (10 * 60)  //10 min

#define MIN(x,y) (((x)<(y))?(x):(y))

#define CHECK_CONN_ERR(ret) \
  do{ \
    if(ret) { \
      return HTTP_CONN; \
    } \
  } while(0)

#define CHECK_ERR(ret) \
        do{ \
          if(ret != HTTP_OK && ret != HTTP_CONN) { \
            return ret; \
          } \
        } while(0)

#define CHECK_HTTPS_CONN_ERR(ret) \
      do{ \
        if(ret) { \
          return HTTP_CONN_ERR; \
        } \
      } while(0)

#define PRTCL_ERR() \
  do{ \
    return HTTP_PRTCL; \
  } while(0)

#define OVERFLOW_ERR(ret) \
    do{ \
      if(ret) { \
        return HTTP_OVERFLOW; \
      } \
    } while(0)

#define DEBUG_LEVEL 0

/****************************************************************************************************************************
 ****************************************************************************************************************************
 *  static function 
 ****************************************************************************************************************************
****************************************************************************************************************************/
#define __DEFINE_STATIC_FUNCTION__//just for easy to find this position


#if 0
static INT32 getHostAddrByName(const char* host, UINT16 port, struct sockaddr_in *hostAddr)
{
    struct hostent *h = NULL;
    h = gethostbyname(host);
    if(NULL == h)
        return HTTP_ERROR;
    hostAddr->sin_family = AF_INET;
    hostAddr->sin_addr = *((struct in_addr*)h->h_addr);
    hostAddr->sin_port = htons(port);
    
    return HTTP_OK;
}
#endif

static HTTPResult parseURL(const char* url, char* scheme, INT32 maxSchemeLen, char* host, INT32 maxHostLen, UINT16* port, char* path, INT32 maxPathLen) //Parse URL
{
    ECOMM_STRING(UNILOG_HTTP_CLIENT, parseURL_1, P_INFO, "url=%s", (uint8_t*)url);
    char* schemePtr = (char*) url;
    char* hostPtr = (char*) strstr(url, "://");
    if (hostPtr == NULL) {
        HTTPWARN("Could not find host");
        return HTTP_PARSE; //URL is invalid
    }

    if ( (uint16_t)maxSchemeLen < hostPtr - schemePtr + 1 ) { //including NULL-terminating char
        HTTPWARN("Scheme str is too small (%d >= %d)", maxSchemeLen, hostPtr - schemePtr + 1);
        return HTTP_PARSE;
    }
    memcpy(scheme, schemePtr, hostPtr - schemePtr);
    scheme[hostPtr - schemePtr] = '\0';

    hostPtr += 3;

    INT32 hostLen = 0;

    char* portPtr = strchr(hostPtr, ':');
    if( portPtr != NULL ) {
        hostLen = portPtr - hostPtr;
        portPtr++;
        if( sscanf(portPtr, "%hu", port) != 1) {
            HTTPWARN("Could not find port");
            return HTTP_PARSE;
        }
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, parseURL_2, P_INFO, 2, "has port=%d, hostLen= %d", *port,hostLen);
    } else {
        hostLen = strlen(hostPtr);
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, parseURL_3, P_INFO, 1, "no port, hostLen=%d", hostLen);
        *port=0;
    }
    char* pathPtr = strchr(hostPtr, '/');
    if( pathPtr != 0 && portPtr == 0) {
        hostLen = pathPtr - hostPtr;
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, parseURL_4, P_INFO, 1, "has path, hostLen=%d", hostLen);
    }
    if( maxHostLen < hostLen + 1 ) { //including NULL-terminating char
        HTTPWARN("Host str is too small (%d >= %d)", maxHostLen, hostLen + 1);
        return HTTP_PARSE;
    }
    memcpy(host, hostPtr, hostLen);
    host[hostLen] = '\0';
    ECOMM_STRING(UNILOG_HTTP_CLIENT, parseURL_5, P_INFO, "host=%s", (uint8_t*)host);

    INT32 pathLen;
    char* fragmentPtr = strchr(hostPtr, '#');
    if(fragmentPtr != NULL) {
        pathLen = fragmentPtr - pathPtr;
    } else {
        if(pathPtr != NULL){
            pathLen = strlen(pathPtr);
        } else {
            pathLen = 0;
        }
    }

    if( maxPathLen < pathLen + 1 ) { //including NULL-terminating char
        HTTPWARN("Path str is too small (%d >= %d)", maxPathLen, pathLen + 1);
        return HTTP_PARSE;
    }
    if (pathPtr!= NULL && pathLen > 0) {
        memcpy(path, pathPtr, pathLen);
        path[pathLen] = '\0';
    }
    ECOMM_STRING(UNILOG_HTTP_CLIENT, parseURL_6, P_INFO, "path=%s", (uint8_t*)path);
    HTTPINFO("parseURL{url(%s),host(%s),maxHostLen(%d),port(%d),path(%s),maxPathLen(%d)}\r\n",
        url, host, maxHostLen, *port, path, maxPathLen);

    return HTTP_OK;
}

// Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
static int base64enc(const char *input, unsigned int length, char *output, int len)
{
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int c, c1, c2, c3;

    if ((uint16_t)len < ((((length-1)/3)+1)<<2)) return -1;
    for(unsigned int i = 0, j = 0; i<length; i+=3,j+=4) {
        c1 = ((((unsigned char)*((unsigned char *)&input[i]))));
        c2 = (length>i+1)?((((unsigned char)*((unsigned char *)&input[i+1])))):0;
        c3 = (length>i+2)?((((unsigned char)*((unsigned char *)&input[i+2])))):0;

        c = ((c1 & 0xFC) >> 2);
        output[j+0] = base64[c];
        c = ((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4);
        output[j+1] = base64[c];
        c = ((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6);
        output[j+2] = (length>i+1)?base64[c]:'=';
        c = (c3 & 0x3F);
        output[j+3] = (length>i+2)?base64[c]:'=';
    }
    output[(((length-1)/3)+1)<<2] = '\0';
    return 0;
}

static void createauth (const char *user, const char *pwd, char *buf, int len)
{
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "%s:%s", user, pwd);
    base64enc(tmp, strlen(tmp), &buf[strlen(buf)], len - strlen(buf));
}

static HTTPResult httpSslClose(HttpClientContext* context)
{
    HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
    /*context->clientCert = NULL;
    context->caCert = NULL;
    context->clientPk = NULL; let up level free it*/
    if(ssl == NULL)
        return HTTP_MBEDTLS_ERR;

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
    context->ssl = NULL;
    return HTTP_OK;
}

#if 0
static HTTPResult httpSend(HttpClientContext* context, const char* buf, UINT16 len) //0 on success, err code on failure
{
    INT32 waitToSend = len;
    INT32 hasSend = 0;
    
    do
    {
        hasSend = send(context->socket, (buf + len - waitToSend), waitToSend, 0);
        HTTPINFO("http_client: %d bytes data has sent to server\n", hasSend);
        if(hasSend > 0)
        {
            waitToSend -= hasSend;
        }
        else if(hasSend == 0)
        {
            return HTTP_OK;
        }
        else
        {
            HTTPINFO("http_client: send failed (errno:%d)\n",errno);
            return HTTP_CONN;
        }
    }while(waitToSend>0);

    return HTTP_OK;
}
#endif

static HTTPResult httpSend(HttpClientContext* context, const char* buf, UINT16 len) //0 on success, err code on failure
{
    INT32 waitToSend = len;
    INT32 hasSend = 0;
    fd_set writeFs;
    struct timeval tv;
    UINT32 preSelTime = 0,passedTime=0;
    UINT8 ret=0;

    tv.tv_sec = 0;
    tv.tv_usec =context->timeout_s*1000000;
    
    FD_ZERO(&writeFs);
    if(context && context->socket >= 0)
    FD_SET(context->socket, &writeFs);

    
    do
    {
        tv.tv_usec -= (passedTime*1000);
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSend_0, P_INFO, 2, "passedTime: %d,tv_usec %d", passedTime,tv.tv_usec);
        preSelTime=osKernelGetTickCount()/portTICK_PERIOD_MS;
        ret = select(context->socket + 1, NULL, &writeFs, NULL, &tv);
        if(ret>0)
        {
            hasSend = send(context->socket, (buf + len - waitToSend), waitToSend, MSG_DONTWAIT);
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSend_1, P_INFO, 1, "%d bytes data has sent to server", hasSend);
            //HTTPINFO("http_client: %d bytes data has sent to server\n", hasSend);
            if(hasSend > 0)
            {
                waitToSend -= hasSend;
            }
            else if(hasSend == 0)
            {
                return HTTP_OK;
            }
            else
            {
                //HTTPINFO("http_client: send failed (errno:%d)\n",errno);
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSend_2, P_INFO, 1, "http_client: send failed (errno:%d)", errno);
                return HTTP_CONN;
            }
            
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSend_3, P_INFO, 2, "preSelTime: %d,current tick %d", preSelTime,osKernelGetTickCount());
            passedTime=(osKernelGetTickCount()-preSelTime)>0?(osKernelGetTickCount() - preSelTime)/portTICK_PERIOD_MS:(0xFFFFFFFF - preSelTime + osKernelGetTickCount())/portTICK_PERIOD_MS;
            
        }
        else//slelct return <=0 select timeout or error
        {
             return HTTP_CONN;
        }

        
    }while(waitToSend>0);

    return HTTP_OK;
}


static HTTPResult httpSslSend(mbedtls_ssl_context* sslContext, const char* buf, UINT16 len)
{
    INT32 waitToSend = len;
    INT32 hasSend = 0;
    
    do
    {
        hasSend = mbedtls_ssl_write(sslContext, (unsigned char *)(buf + len - waitToSend), waitToSend);
        if(hasSend > 0)
        {
            waitToSend -= hasSend;
        }
        else if(hasSend == 0)
        {
            return HTTP_OK;
        }
        else
        {
            HTTPINFO("http_client(ssl): send failed \n");
            return HTTP_CONN;
        }
    }while(waitToSend>0);

    return HTTP_OK;
}

static int httpSslNonblockRecv(void *netContext, UINT8 *buf, size_t len)
{
    int ret;
    int fd = ((mbedtls_net_context *)netContext)->fd;
    if(fd < 0)
        return HTTP_MBEDTLS_ERR;
    ret = (int)recv(fd, buf, len, MSG_DONTWAIT);
    if(ret<0){
        if( errno == EPIPE || errno == ECONNRESET) {
            return (MBEDTLS_ERR_NET_CONN_RESET);
        }
        if( errno == EINTR ) {
            return (MBEDTLS_ERR_SSL_WANT_READ);
        }
        if(ret == -1 && errno == EWOULDBLOCK) {
            return ret;
        }
        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }
    return (ret);
}

HTTPResult httpRecv(HttpClientContext* context, char* buf, INT32 minLen, INT32 maxLen, INT32* pReadLen) //0 on success, err code on failure
{
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_1, P_INFO, 2, "Trying to read between %d and %d bytes", minLen, maxLen);
    INT32 readLen = 0;

    int ret;
    while (readLen < maxLen) 
    {
        if(!context->isHttps)
        {
            if (readLen < minLen) {
                ret = recv(context->socket, buf+readLen, minLen-readLen, 0);
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_2, P_INFO, 1, "recv [blocking] return:%d", ret);
                if(ret == 0)
                {
                    int mErr = sock_get_errno(context->socket);
                    if(socket_error_is_fatal(mErr))//maybe closed or reset by peer
                    {
                       ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_2_1, P_INFO, 0, "recv return 0 fatal error return HTTP_CLOSED");
                       return HTTP_CLOSED;
                    }
                    else
                    { 
                       ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_2_2, P_INFO, 0, "recv return 0 connect error");
                       return HTTP_CONN;
                    }
                }
            } 
            else 
            {
                ret = recv(context->socket, buf+readLen, maxLen-readLen, MSG_DONTWAIT);
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_4, P_INFO, 1, "recv [not blocking] return:%d", ret);
                if(ret == -1 && errno == EWOULDBLOCK) 
                {
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_4_1, P_INFO, 0, "recv [not blocking] errno == EWOULDBLOCK");
                    break;
                }
            }
        }
        else
        {
            HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
            if (readLen < minLen) {
                mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), mbedtls_net_send, mbedtls_net_recv, NULL);
                ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char *)(buf+readLen), minLen-readLen);
                if(ret < 0)
                {
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_8, P_INFO, 1, "mbedtls_ssl_read [blocking] return:-0x%x", -ret);
                }
                if(ret == 0)
                {
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_9, P_INFO, 0, "mbedtls_ssl_read [blocking] return 0 connect error");
                    return HTTP_CONN_ERR;
                }
            } 
            else 
            {
                mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), mbedtls_net_send, httpSslNonblockRecv, NULL);
                ret = mbedtls_ssl_read(&(ssl->sslContext), (unsigned char*)(buf+readLen), maxLen-readLen);
                
                if(ret < 0)
                {
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_a, P_INFO, 1, "mbedtls_ssl_read [not blocking] return:-0x%x", -ret);
                }
                if(ret == -1 && errno == EWOULDBLOCK) 
                {
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_b, P_INFO, 0, "mbedtls_ssl_read [not blocking] errno == EWOULDBLOCK");
                    break;
                }
            }
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
                return HTTP_CLOSED;
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
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_6, P_INFO, 1, "Connection error (recv returned -0x%x)", -ret);
            *pReadLen = readLen;
            if(context->isHttps)
            {
                return HTTP_CONN_ERR;
            }
            else
            {
                int mErr = sock_get_errno(context->socket);
                if(socket_error_is_fatal(mErr))//maybe closed or reset by peer
                {
                   ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_6_1, P_INFO, 0, "recv return 0 fatal error return HTTP_CLOSED");
                   return HTTP_CLOSED;
                }
                else
                { 
                   ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_6_2, P_INFO, 0, "recv return 0 connect error");
                   return HTTP_CONN;
                }
            }
        }
    }
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecv_7, P_INFO, 1, "Read %d bytes", readLen);
    buf[readLen] = '\0';    // DS makes it easier to see what's new.
    *pReadLen = readLen;
    return HTTP_OK;
}

static HTTPResult prepareBuffer(HttpClientContext* context, char* sendBuf, int* cursor, char* buf, int len)
{
    int copyLen;
    int sendbufCursor = *cursor;
    
    if(len == 0){
        len = strlen(buf);
    }

    do {
        if((CHUNK_SIZE - sendbufCursor) >= len) {
            copyLen = len;
        }else{
            HTTPERR("send buffer overflow");
            return HTTP_OVERFLOW;
        }
        memcpy(sendBuf + sendbufCursor, buf, copyLen);
        sendbufCursor += copyLen;
        len -= copyLen;
    }while(len);
    
    *cursor = sendbufCursor;
    return HTTP_OK;
}

char *httpSendBuf = NULL;
char *httpSendBufTemp = NULL;

void httpFreeBuff(HTTPResult ret)
{
    if(ret != 0)
    {
        if(httpSendBuf != NULL)
        {
            free(httpSendBuf);
            httpSendBuf = NULL;
        }

        if(httpSendBufTemp != NULL)
        {
            free(httpSendBufTemp);
            httpSendBufTemp = NULL;
        }
    }
}


static HTTPResult httpSendHeader(HttpClientContext* context, const char * url, HTTP_METH method, HttpClientData * data)
{
    char scheme[8];
    uint16_t port;
    //char buf[CHUNK_SIZE];
    //char tempBuf[TEMPBUF_SIZE];
    char host[MAXHOST_SIZE];
    char path[MAXPATH_SIZE];
    HTTPResult ret = HTTP_OK;
    int bufCursor = 0;
    //memset(buf, 0, CHUNK_SIZE);
    memset(host, 0, MAXHOST_SIZE);
    memset(path, 0, MAXPATH_SIZE);
    //memset(tempBuf, 0, TEMPBUF_SIZE);
    context->method = method;

    HTTPResult res = parseURL(url, scheme, sizeof(scheme), host, sizeof(host), &port, path, sizeof(path));
    if(res != HTTP_OK) {
        HTTPERR("parseURL returned %d", res);
        return res;
    }
    
    httpSendBuf = malloc(CHUNK_SIZE);
    memset(httpSendBuf, 0, CHUNK_SIZE);
    
    httpSendBufTemp = malloc(TEMPBUF_SIZE);
    
    const char* meth = (method==HTTP_GET)?"GET":(method==HTTP_POST)?"POST":(method==HTTP_PUT)?"PUT":(method==HTTP_HEAD)?"HEAD":(method==HTTP_DELETE)?"DELETE":"";
    snprintf(httpSendBufTemp, TEMPBUF_SIZE, "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); //Write request
    ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
    httpFreeBuff(ret);
    OVERFLOW_ERR(ret);
    
    // send authorization
    if (context->basicAuthUser && context->basicAuthPassword) {
        memset(httpSendBufTemp, 0, TEMPBUF_SIZE);
        HTTPINFO("send auth (if defined)");
        strcpy(httpSendBufTemp, "Authorization: Basic ");
        createauth(context->basicAuthUser, context->basicAuthPassword, httpSendBufTemp+strlen(httpSendBufTemp), TEMPBUF_SIZE-strlen(httpSendBufTemp));
        strcat(httpSendBufTemp, "\r\n");
        HTTPINFO(" (%s,%s) => (%s)", context->basicAuthUser, context->basicAuthPassword, httpSendBufTemp);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);
    }
    
    // Send custom header
    HTTPINFO("httpSendHeader Send custom header(s) %d (if any)", context->headerNum);
    for (INT32 nh = 0; nh < context->headerNum * 2; nh+=2) {
        HTTPINFO("hdr[%2d] %s: %s", nh, context->customHeaders[nh], context->customHeaders[nh+1]);
        snprintf(httpSendBufTemp, TEMPBUF_SIZE, "%s: %s\r\n", context->customHeaders[nh], context->customHeaders[nh+1]);
        HTTPINFO("httpSendHeader custom header:{%s}", httpSendBufTemp);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);
    }

    if(context->custHeader) {
        snprintf(httpSendBufTemp, TEMPBUF_SIZE, "%s\r\n", context->custHeader);
        HTTPINFO("httpSendHeader custheader:{%s}", httpSendBufTemp);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);
    }

    // set range
    if(data->isRange)
    {
       ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSendHeader_1, P_INFO, 2, "Range:bytes=%d-%d",data->rangeHead, data->rangeTail);
       if(data->rangeTail == -1){
            snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Range:bytes=%d-\r\n", data->rangeHead);
        }else{
           snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Range:bytes=%d-%d\r\n", data->rangeHead, data->rangeTail);
       }
       ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
       httpFreeBuff(ret);
       OVERFLOW_ERR(ret);
    }
    
    //Send default headers
    if(data->postBuf != NULL)
    {
        snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Content-Length: %d\r\n", data->postBufLen);
        ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
        HTTPDBG("httpSendHeader Sending default len headers:{%s}", httpSendBufTemp);
        httpFreeBuff(ret);
        OVERFLOW_ERR(ret);

        if(data->postContentType != NULL)
        {
            snprintf(httpSendBufTemp, TEMPBUF_SIZE, "Content-Type: %s\r\n", data->postContentType);
            ret = prepareBuffer(context, httpSendBuf, &bufCursor, httpSendBufTemp, strlen(httpSendBufTemp));
            HTTPDBG("httpSendHeader Sending default type headers:{%s}", httpSendBufTemp);
            httpFreeBuff(ret);
            OVERFLOW_ERR(ret);
        }
    }
    
    //Close headers
    ret = prepareBuffer(context, httpSendBuf, &bufCursor, "\r\n", strlen("\r\n"));
    httpFreeBuff(ret);
    OVERFLOW_ERR(ret);
    HTTPINFO("httpSendHeader send head:%s, headlen:%d", httpSendBuf,strlen(httpSendBuf));

    if(context->isHttps){
        HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
        ret = httpSslSend(&(ssl->sslContext), httpSendBuf, strlen(httpSendBuf));
        httpFreeBuff(ret);
        CHECK_HTTPS_CONN_ERR(ret);
    }
    else {
        ret = httpSend(context, httpSendBuf, strlen(httpSendBuf));
        httpFreeBuff(ret);
        CHECK_CONN_ERR(ret);
    }
    httpFreeBuff((HTTPResult)1);
    return ret;
}

static HTTPResult httpSendUserdata(HttpClientContext* context, HttpClientData * data) 
{
    HTTPResult ret = HTTP_OK;
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSendUserdata_1, P_INFO, 0, "begin send content");
    if(data->postBuf && data->postBufLen)
    {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSendUserdata_2, P_INFO, 1, "data->postBufLen=%d",data->postBufLen);
        if(context->isHttps)
        {
            HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
            ret = httpSslSend(&(ssl->sslContext), data->postBuf, data->postBufLen);
            CHECK_HTTPS_CONN_ERR(ret);
        }
        else
        {
            ECOMM_STRING(UNILOG_HTTP_CLIENT, httpSendUserdata_3, P_INFO, "data->postBuf=%s",(uint8_t*)data->postBuf);
            ret = httpSend(context, data->postBuf, strlen(data->postBuf));
            CHECK_CONN_ERR(ret);
        }
    }
    return ret;
}
static HTTPResult check_timeout_ret(HTTPResult ret){
    static uint8_t count = 0;
    if(ret == HTTP_OK){
        count = 0;
    }else if(ret == HTTP_CONN ){
        if(count < 3){
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, check_timeout_ret_1, P_INFO, 1, "wait %d x 20s", count);
            count += 1;
        }else{
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, check_timeout_ret_2, P_INFO, 0, "give up return HTTP_TIMEOUT");
            ret = HTTP_TIMEOUT;
        }
    }
    return ret; 
}

static HTTPResult httpParseContent(HttpClientContext* context, char * buf, INT32 trfLen, HttpClientData * data)
{
    INT32 crlfPos = 0;
    HTTPResult ret = HTTP_OK;
    int maxlen;
    int total = 0;
    int templen = 0;
    static int seqNum = 0;
    //Receive data
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_1, P_INFO, 0, "begin parse content");

    data->isMoreContent = TRUE;

    if(data->recvContentLength == -1 && data->isChunked == FALSE)
    {
        while(true)
        {
            if(total + trfLen < data->respBufLen - 1)
            {
                memcpy(data->respBuf + total, buf, trfLen);
                total += trfLen;
                data->respBuf[total] = '\0';
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_2, P_INFO, 0, "all data are here");
            }
            else
            {
                memcpy(data->respBuf + total, buf, data->respBufLen - 1 - total);
                data->respBuf[data->respBufLen-1] = '\0';
                data->blockContentLen = data->respBufLen-1;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_3, P_INFO, 0, "still has more data on the way");
                return HTTP_MOREDATA;
            }

            maxlen = MIN(CHUNK_SIZE - 1, data->respBufLen - 1 - total);
            ret = httpRecv(context, buf, 1, maxlen, &trfLen);

            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_4, P_INFO, 2, "receive data len:%d, total:%d", trfLen, total);

            if(ret != HTTP_OK)
            {
                data->blockContentLen = total;
                data->isMoreContent = false;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_5, P_INFO, 1, "ret:%d", ret);
                return ret;
            }
            if(trfLen == 0)
            {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_6, P_INFO, 0, "no more data read");
                data->isMoreContent = false;
                return HTTP_OK;
            }
        }
    }
    
    while(true) {
        INT32 readLen = 0;
    
        if( data->isChunked ) {//content is chunked code
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_7, P_INFO, 0, "content is chunked code");
            //Read chunk header
            bool foundCrlf;
            do {
                foundCrlf = false;
                crlfPos=0;
                buf[trfLen]=0;
                if(trfLen >= 2) {
                    for(; crlfPos < trfLen - 2; crlfPos++) {
                        if( buf[crlfPos] == '\r' && buf[crlfPos + 1] == '\n' ) {
                            foundCrlf = true;
                            break;
                        }
                    }
                }
                if(!foundCrlf) { //Try to read more
                    if( trfLen < CHUNK_SIZE ) {
                        INT32 newTrfLen = 0;
                        ret = httpRecv(context, buf + trfLen, 0, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                        trfLen += newTrfLen;
                        CHECK_CONN_ERR(ret);
                        continue;
                    } else {
                        PRTCL_ERR();
                    }
                }
            } while(!foundCrlf);
            buf[crlfPos] = '\0';
            int n = sscanf(buf, "%x", &readLen);
            data->needObtainLen = readLen;
            data->recvContentLength += readLen;
            if(n!=1) {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_7_1, P_INFO, 0, "Could not read chunk length");
                PRTCL_ERR();
            }
    
            memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2)); //Not need to move NULL-terminating char any more
            trfLen -= (crlfPos + 2);
    
            if( readLen == 0 ) {
                //Last chunk
                data->isMoreContent = false;
                break;
            }
        } else {
            readLen = data->needObtainLen;
        }
    
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_8, P_INFO, 2, "need to obtaining %d bytes trfLen=%d", readLen,trfLen);
    
        do {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9, P_INFO, 2, "trfLen=%d, readLen=%d", trfLen, readLen);
            templen = MIN(trfLen, readLen);
            if(total+templen < data->respBufLen - 1){
                memcpy(data->respBuf+total, buf, templen);
                total += templen;
                data->respBuf[total] = '\0';
                data->needObtainLen -= templen;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_1, P_INFO, 2, "templen=%d data->needObtainLen=%d", templen,data->needObtainLen);
            } else {
                memcpy(data->respBuf + total, buf, data->respBufLen - 1 - total);
                data->respBuf[data->respBufLen - 1] = '\0';
                data->needObtainLen -= data->respBufLen - 1 - total;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_2, P_INFO, 2, "data->needObtainLen=%d total=%d", data->needObtainLen,total);
                if(readLen > trfLen){
                    data->blockContentLen = data->respBufLen -1;
                    seqNum += 1;
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_3, P_INFO, 2, "return 12 moredata data->blockContentLen=%d, seqNum=%d", data->blockContentLen, seqNum);
                    return HTTP_MOREDATA;
                }else{
                    total += templen;
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_4, P_INFO, 2, "templen=%d total=%d", templen,total);
                }
            }
            
            if( trfLen >= readLen ) {
                memmove(buf, &buf[readLen], trfLen - readLen);
                trfLen -= readLen;
                readLen = 0;
                data->needObtainLen = 0;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_5, P_INFO, 1, "trfLen=%d data->needObtainLen and readLen set 0", trfLen);
            } else {
                readLen -= trfLen;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_6, P_INFO, 1, "readLen=%d", readLen);
            }
    
            if(readLen) {
                maxlen = MIN(MIN(CHUNK_SIZE-1, data->respBufLen-1-total),readLen);
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_7, P_INFO, 1, "to read maxlen=%d", maxlen);
                ret = httpRecv(context, buf, 1, maxlen, &trfLen);
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_9_8, P_INFO, 2, "httpRecv return: %d,trfLen:%d", ret,trfLen);
                ret = check_timeout_ret(ret);
                CHECK_ERR(ret);
            }
        } while(readLen);
    
        if( data->isChunked ) {
            if(trfLen < 2) {
                INT32 newTrfLen;
                //Read missing chars to find end of chunk
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_10, P_INFO, 0, "search end of chunk");
                ret = httpRecv(context, buf + trfLen, 2 - trfLen, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                CHECK_CONN_ERR(ret);
                trfLen += newTrfLen;
            }
            if( (buf[0] != '\r') || (buf[1] != '\n') ) {
                HTTPERR("Format error");
                PRTCL_ERR();
            }
            memmove(buf, &buf[2], trfLen - 2);
            trfLen -= 2;
        } else {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_11, P_INFO, 0, "no more content");
            data->isMoreContent = false;
            break;
        }
    }
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseContent_12, P_INFO, 1, "all content over, seqNum=%d", seqNum);
    data->blockContentLen = total;
    return ret;
}

static HTTPResult httpParseHeader(HttpClientContext* context, char * buf, INT32 trfLen, HttpClientData * data)
{
    HTTPResult ret;
    INT32 crlfPos = 0;
    int temp1 = 0, temp2 = 0;
    int headerBufLen = data->headerBufLen;
    char *headerBuf = data->headerBuf;
    
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_0, P_INFO, 1, "begin parse header trfLen=%d",trfLen);
    memset(headerBuf, 0, headerBufLen);
    
    data->recvContentLength = -1;
    data->isChunked = false;
    
    char* crlfPtr = strstr(buf, "\r\n");
    if( crlfPtr == NULL) {
        PRTCL_ERR();
    }

    crlfPos = crlfPtr - buf;
    buf[crlfPos] = '\0';
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_0_1, P_INFO, 1, "crlfPos=%d",crlfPos);

    //Parse HTTP response
    if( sscanf(buf, "HTTP/%*d.%*d %d %*[^\r\n]", &(context->httpResponseCode)) != 1 ) {
        //Cannot match string, error
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_1, P_INFO, 0, "Not a correct HTTP answer");
        PRTCL_ERR();
    }

    if( (context->httpResponseCode < 200) || (context->httpResponseCode >= 400) ) {
        //Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_2, P_INFO, 1, "Response code %d", context->httpResponseCode);
        if(context->httpResponseCode==404)
           return HTTP_NOTFOUND;
        else if(context->httpResponseCode==403)
           return HTTP_REFUSED;
        else if(context->httpResponseCode==400)
        {
            if(!context->isHttps)
            {
                return HTTP_ERROR;
            }
        }
        else
           return HTTP_ERROR;
    }
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_2_1, P_INFO, 1, "context->httpResponseCode = %d", context->httpResponseCode);

    memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
    trfLen -= (crlfPos + 2);

    //Now get headers
    while( true ) {
        char *colonPtr, *keyPtr, *valuePtr;
        int keyLen, valueLen;

        crlfPtr = strstr(buf, "\r\n");
        if(crlfPtr == NULL) {
            if( trfLen < (CHUNK_SIZE - 1) ) 
            {
                INT32 newTrfLen = 0;
                ret = httpRecv(context, buf + trfLen, 1, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                trfLen += newTrfLen;
                buf[trfLen] = '\0';
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_3, P_INFO, 2, "trfLen = %d, new recv:%d", trfLen, newTrfLen);
                CHECK_ERR(ret);
                continue;
            } 
            else if( trfLen == (CHUNK_SIZE - 1) )
            {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_3_hh, P_INFO, 1, "trfLen = %d", trfLen);
            }
            else 
            {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_4, P_INFO, 1, "trfLen = %d > CHUNK_SIZE",trfLen);
                PRTCL_ERR();
            }
        }
        if(crlfPtr != NULL)
        {
            crlfPos = crlfPtr - buf;
        }
        else
        {
            crlfPos = trfLen;
        }
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_4_1, P_INFO, 2, "crlfPos = %d...trfLen=%d",crlfPos,trfLen);

        if(crlfPos == 0) { //End of headers
            memmove(buf, &buf[2], trfLen - 2 + 1); //Be sure to move NULL-terminating char as well
            trfLen -= 2;
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_5, P_INFO, 1, "End of headers,trfLen=%d",trfLen);
            break;
        }
        
        colonPtr = strstr(buf, ": ");        
        if (colonPtr) {             
            if (headerBufLen >= crlfPos + 2) {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_5_1, P_INFO, 2, "headerBufLen=%d crlfPos=%d",headerBufLen,crlfPos);
                /* copy response header to caller buffer */
                memcpy(headerBuf, buf, crlfPos + 2);                                
                headerBuf += crlfPos + 2;
                headerBufLen -= crlfPos + 2;
            }
            
            keyLen = colonPtr - buf;
            valueLen = crlfPtr - colonPtr - strlen(": ");            
            keyPtr = buf;
            valuePtr = colonPtr + strlen(": ");

            //printf("Read header : %.*s: %.*s\r\n", keyLen, keyPtr, valueLen, valuePtr); 
            if (0 == strncasecmp(keyPtr, "Content-Length", keyLen)) {
                sscanf(valuePtr, "%d[^\r]", &(data->recvContentLength));                
                data->needObtainLen = data->recvContentLength;
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_6, P_INFO, 1, "data->needObtainLen=%d",data->needObtainLen);
            } else if (0 == strncasecmp(keyPtr, "Transfer-Encoding", keyLen)) {
                if (0 == strncasecmp(valuePtr, "Chunked", valueLen)) {
                    data->isChunked = true;
                    data->recvContentLength = 0;
                    data->needObtainLen = 0;
                    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_7, P_INFO, 0, "data->isChunked = true");
                }
            } else if (0 == strncasecmp(keyPtr, "Content-Range", keyLen)) {
                sscanf(valuePtr, "%*[^/]/%d[^\r]", &(data->contentRange));                
                sscanf(valuePtr, "%*[^ ] %d-[^\-]", &(temp1));                
                sscanf(valuePtr, "%*[^\-]-%d[^/]", &(temp2));                
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_6_1, P_INFO, 3, "data->contentRange=%d,current head=%d tail=%d",data->contentRange,temp1,temp2);
            } 
           
            memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
            trfLen -= (crlfPos + 2);
        } else {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_8, P_INFO, 0, "Could not parse header");
            PRTCL_ERR();
        }
    }// get headers
    
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_9, P_INFO, 0, "Completed HTTP header parse");
    if(context->method == HTTP_HEAD)
        return HTTP_OK;
    else{
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpParseHeader_10, P_INFO, 1, "continue parse content, trfLen=%d",trfLen);
        ret = httpParseContent(context, buf, trfLen, data);
    }
    
    return ret;
}


HTTPResult httpConnectSocket(INT32 socket,struct sockaddr *addr, INT32 addrlen);

static HTTPResult httpConn(HttpClientContext* context, char* host)
{
    HTTPResult ret=HTTP_OK;
    //struct timeval timeout_send;
    struct timeval timeout_recv;
    struct addrinfo hints, *addr_list, *p;
    char port[10] = {0};

    //timeout_send.tv_sec = context->timeout_s > MAX_TIMEOUT ? MAX_TIMEOUT : context->timeout_s;
    //timeout_send.tv_usec = 0;
    timeout_recv.tv_sec = context->timeout_r > MAX_TIMEOUT ? MAX_TIMEOUT : context->timeout_r;
    timeout_recv.tv_usec = 0;

    memset( &hints, 0, sizeof( hints ) );
    //hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;


    snprintf(port, sizeof(port), "%d", context->port) ;
    
    if (getaddrinfo( host, port , &hints, &addr_list ) != 0 ) {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConn_1, P_INFO, 0, "HTTP connect unresolved dns");
        return HTTP_CONN;
    }

    //try address one by one until sucess
    for ( p = addr_list; p != NULL; p = p->ai_next ) {
        context->socket = (int) socket( p->ai_family, p->ai_socktype,p->ai_protocol);
        if ( context->socket < 0 ) {
            ret = HTTP_CONN;
            continue;//try new one
        }
        
        /* set timeout for both tx removed since lwip not support tx timeout */
        //if ( context->timeout_s > 0) {
        //    setsockopt(context->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout_send, sizeof(timeout_send));
        //}

        /* set timeout for both rx */
        if ( context->timeout_r > 0) {
            setsockopt(context->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_recv, sizeof(timeout_recv));;
        }

        #ifdef HTTP_CONNECT_TIMEOUT_EN

        INT32 flags = fcntl( context->socket, F_GETFL, 0);
        if(flags < 0)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, httpConn_3, P_ERROR, 0, "httpCreateSocket get file cntrl flags fail");      
            close(context->socket);
            context->socket = -1;
            continue;//try new one
        }
        
        fcntl(context->socket, F_SETFL, flags|O_NONBLOCK); //set socket as nonblock for connect timeout
        
        if ( httpConnectSocket( context->socket, p->ai_addr, (INT32)p->ai_addrlen ) == HTTP_OK ) {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConn_2, P_INFO, 0, "HTTP connect success");
            ret = HTTP_OK;//connect success
            fcntl(context->socket, F_SETFL, flags&~O_NONBLOCK); //connect success recover to block mode
            break;
        }

        fcntl(context->socket, F_SETFL, flags&~O_NONBLOCK); //connect fail recover to block mode

        #else
        if ( connect( context->socket, p->ai_addr, (int)p->ai_addrlen ) == 0 ) {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConn_2, P_INFO, 0, "HTTP connect success");
            ret = HTTP_OK;//connect success
            break;
        }
        #endif
        
        close( context->socket );
        context->socket = -1;
        ret = HTTP_CONN;
    }

    freeaddrinfo( addr_list );

    return ret;

}


static int sslRandom(void *p_rng, unsigned char *output, size_t output_len)
{
    uint32_t rnglen = output_len;
    uint8_t   rngoffset = 0;

    while (rnglen > 0) {
        *(output + rngoffset) = (unsigned char)rand();
        rngoffset++;
        rnglen--;
    }
    return 0;
}

static void sslDebug(void *ctx, int level, const char *file, int line, const char *str)
{
    HTTPDBG("%s(%d):%s", file, line, str);
//    DBG("%s", str);
}

static HTTPResult httpSslConn(HttpClientContext* context, char* host)
{
    int value;
    HttpClientSsl *ssl;
    const char *custom = "https";
    char port[10] = {0};
    int authmode = MBEDTLS_SSL_VERIFY_NONE;
    UINT32 flag;
    
    context->ssl = malloc(sizeof(HttpClientSsl));
    ssl = context->ssl;
    
    /*
     * 0. Initialize the RNG and the session data
     */
#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold((int)DEBUG_LEVEL);
#endif
    mbedtls_net_init(&ssl->netContext);
    mbedtls_ssl_init(&ssl->sslContext);
    mbedtls_ssl_config_init(&ssl->sslConfig);
    mbedtls_x509_crt_init(&ssl->caCert);
    mbedtls_x509_crt_init(&ssl->clientCert);
    mbedtls_pk_init(&ssl->pkContext);
    mbedtls_ctr_drbg_init(&ssl->ctrDrbgContext);
    mbedtls_entropy_init(&ssl->entropyContext);

    if((value = mbedtls_ctr_drbg_seed(&ssl->ctrDrbgContext,
                             mbedtls_entropy_func,
                             &ssl->entropyContext,
                             (const unsigned char*)custom,
                             strlen(custom))) != 0) {
        HTTPDBG("mbedtls_ctr_drbg_seed failed, value:-0x%x.", -value);
        return HTTP_MBEDTLS_ERR;
    }
    /*
     * 0. Initialize certificates
     */

    HTTPDBG("STEP 0. Loading the CA root certificate ...");
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_1, P_INFO, 0, "STEP 0. Loading the CA root certificate ...");
    if (NULL != context->caCert) {
        authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
        if (0 != (value = mbedtls_x509_crt_parse(&(ssl->caCert), (const unsigned char *)context->caCert, context->caCertLen))) {
            HTTPDBG("failed ! value:-0x%04x", -value);
            return HTTP_MBEDTLS_ERR;
        }
        HTTPDBG(" ok (%d skipped)", value);
    }

    /* Setup Client Cert/Key */
    if (context->clientCert != NULL && context->clientPk != NULL) {
        HTTPDBG("STEP 0. start prepare client cert ...");
        value = mbedtls_x509_crt_parse(&(ssl->clientCert), (const unsigned char *) context->clientCert, context->clientCertLen);
        if (value != 0) {
            HTTPDBG(" failed!  mbedtls_x509_crt_parse returned -0x%x\n", -value);
            return HTTP_MBEDTLS_ERR;
        }

        HTTPDBG("STEP 0. start mbedtls_pk_parse_key[%s]", context->clientPk);
        value = mbedtls_pk_parse_key(&ssl->pkContext, (const unsigned char *) context->clientPk, context->clientPkLen, NULL, 0);

        if (value != 0) {
            HTTPDBG(" failed\n  !  mbedtls_pk_parse_key returned -0x%x\n", -value);
            return HTTP_MBEDTLS_ERR;
        }
    }

    
    /*
     * 1. Start the connection
     */
    snprintf(port, sizeof(port), "%d", context->port);
    HTTPDBG("STEP 1. Connecting to /%s/%s...", host, port);
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_2, P_INFO, 1, "STEP 1. Connecting to PORT:%d",context->port);
    if (0 != (value = mbedtls_net_connect(&ssl->netContext, host, port, MBEDTLS_NET_PROTO_TCP))) {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_2_1, P_INFO, 1, " failed ! mbedtls_net_connect returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }
    HTTPDBG(" ok");

    
    /*
     * 2. Setup stuff
     */
    HTTPDBG("STEP 2. Setting up the SSL/TLS structure...");
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_3, P_INFO, 0, "STEP 2. Setting up the SSL/TLS structure...");
    if ((value = mbedtls_ssl_config_defaults(&(ssl->sslConfig), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_3_1, P_INFO, 1, " failed! mbedtls_ssl_config_defaults returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }

    mbedtls_ssl_conf_max_version(&ssl->sslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl->sslConfig, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

    memcpy(&(ssl->crtProfile), ssl->sslConfig.cert_profile, sizeof(mbedtls_x509_crt_profile));
    mbedtls_ssl_conf_authmode(&(ssl->sslConfig), authmode);

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    if ((value = mbedtls_ssl_conf_max_frag_len(&(ssl->sslConfig), MBEDTLS_SSL_MAX_FRAG_LEN_1024)) != 0) {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_3_2, P_INFO, 1, " mbedtls_ssl_conf_max_frag_len returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_ssl_conf_cert_profile(&ssl->sslConfig, &ssl->crtProfile);
    mbedtls_ssl_conf_ca_chain(&(ssl->sslConfig), &(ssl->caCert), NULL);
    if(context->clientCert) {
        if ((value = mbedtls_ssl_conf_own_cert(&(ssl->sslConfig), &(ssl->clientCert), &(ssl->pkContext))) != 0) {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_3_3, P_INFO, 1, "  failed! mbedtls_ssl_conf_own_cert returned -0x%x", -value);
            return HTTP_MBEDTLS_ERR;
        }
    }
#endif
    mbedtls_ssl_conf_rng(&(ssl->sslConfig), sslRandom, &(ssl->ctrDrbgContext));
    mbedtls_ssl_conf_dbg(&(ssl->sslConfig), sslDebug, NULL);

#if defined(MBEDTLS_SSL_ALPN)
    const char *alpn_list[] = { "http/1.1", NULL };
    mbedtls_ssl_conf_alpn_protocols(&(ssl->sslConfig),alpn_list);
#endif

    if(context->timeout_r > 0) {
        UINT32 recvTimeout;
        recvTimeout = context->timeout_r > MAX_TIMEOUT ? MAX_TIMEOUT * 1000 : context->timeout_r * 1000;
        mbedtls_ssl_conf_read_timeout(&(ssl->sslConfig), recvTimeout);
    }
    if ((value = mbedtls_ssl_setup(&(ssl->sslContext), &(ssl->sslConfig))) != 0) {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_3_4, P_INFO, 1, " failed! mbedtls_ssl_setup returned -0x%x", -value);
        return HTTP_MBEDTLS_ERR;
    }
    mbedtls_ssl_set_hostname(&(ssl->sslContext), host);
    mbedtls_ssl_set_bio(&(ssl->sslContext), &(ssl->netContext), mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
    
    HTTPDBG(" ok");

    /*
     * 3. Handshake
     */
    HTTPDBG("STEP 3. Performing the SSL/TLS handshake...");
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_4, P_INFO, 0, "STEP 3. Performing the SSL/TLS handshake...");
    
    while ((value = mbedtls_ssl_handshake(&(ssl->sslContext))) != 0) {
        if ((value != MBEDTLS_ERR_SSL_WANT_READ) && (value != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_4_1, P_INFO, 1, "failed  ! mbedtls_ssl_handshake returned -0x%x", -value);
            return HTTP_MBEDTLS_ERR;
        }
    }
    HTTPDBG(" ok");

    /*
     * 4. Verify the server certificate
     */
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_5, P_INFO, 0, "STEP 4. Verifying peer X.509 certificate..");
    flag = mbedtls_ssl_get_verify_result(&(ssl->sslContext));
    if (flag != 0) {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_5_1, P_INFO, 0, " failed  ! verify result not confirmed.");
        return HTTP_MBEDTLS_ERR;
    }
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSslConn_6, P_INFO, 0, "caCert varification ok");

    return HTTP_OK;
}

/****************************************************************************************************************************
 ****************************************************************************************************************************
 *  EXTERNAL/GLOBAL FUNCTION 
 ****************************************************************************************************************************
****************************************************************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__//just for easy to find this position

const char * GetErrorMessage(HTTPResult res)
{
    const char * msg[HTTP_CLOSED+1] = {
        "HTTP OK",            ///<Success
        "HTTP Processing",    ///<Processing
        "HTTP URL Parse error",         ///<url Parse error
        "HTTP DNS error",           ///<Could not resolve name
        "HTTP Protocol error",         ///<Protocol error
        "HTTP 404 Not Found",      ///<HTTP 404 Error
        "HTTP 403 Refused",       ///<HTTP 403 Error
        "HTTP ### Error",         ///<HTTP xxx error
        "HTTP Timeout",       ///<Connection timeout
        "HTTP Connection error",          ///<Connection error
        "HTTP fatal error",           //fatal error when connect with timeout
        "HTTP Closed by remote host"         ///<Connection was closed by remote host
    };
    if (res <= HTTP_CLOSED)
        return msg[res];
    else
        return "HTTP Unknown Code";
};

void basicAuth(HttpClientContext* context, const char* user, const char* password) //Basic Authentification
{
    if (context->basicAuthUser)
        free(context->basicAuthUser);
    context->basicAuthUser = (char *)malloc(strlen(user)+1);
    strcpy(context->basicAuthUser, user);
    if (context->basicAuthPassword)
        free(context->basicAuthPassword);
    context->basicAuthPassword = (char *)malloc(strlen(password)+1);
    strcpy(context->basicAuthPassword, password);//not free yet!!!
}

void customHeaders(HttpClientContext* context, char **headers, int pairs)
{
    context->customHeaders = headers;
    context->headerNum = pairs;
}

void custHeader(HttpClientContext* context, char *header)
{
    context->custHeader = header;
}

HTTPResult httpConnect(HttpClientContext* context, const char* url) //Execute request
{
    HTTPResult ret;
    
    char scheme[8]= {0};
    UINT16 port;
    char host[MAXHOST_SIZE] = {0};
    char path[MAXPATH_SIZE] = {0};
    
    HTTPINFO("httpConnect parse url: [%s]", url);
    //First we need to parse the url (http[s]://host[:port][/[path]]) -- HTTPS not supported (yet?)
    HTTPResult res = parseURL(url, scheme, sizeof(scheme), host, sizeof(host), &port, path, sizeof(path));
    if(res != HTTP_OK) {
        HTTPERR("parseURL returned %d", res);
        return res;
    }

    if(strcmp(scheme, "https") == 0)
        context->isHttps = TRUE;
    else
        context->isHttps = FALSE;
    
    if(port == 0) { 
        if(context->isHttps)
            port = 443;
        else
            port = 80;
    }
    context->port = port;
    
    HTTPDBG("httpConnect Scheme: %s", scheme);
    HTTPDBG("httpConnect Host: %s", host);
    HTTPDBG("httpConnect Port: %d", port);
    HTTPDBG("httpConnect Path: %s", path);
    
    if(context->isHttps) {
        ret = httpSslConn(context, host);
        if(HTTP_OK == ret) {
            HttpClientSsl *ssl = (HttpClientSsl *)context->ssl;
            context->socket = ssl->netContext.fd;
        }
    } else {
        ret = httpConn(context, host);
    }
    return ret;
}

HTTPResult httpSendRequest(HttpClientContext* context, const char* url, HTTP_METH method,  HttpClientData * data) 
{
    HTTPResult ret = HTTP_CONN;
    if(context->socket <0)
    {
        return ret;
    }
    ret = httpSendHeader(context, url, method, data);
    if(ret != HTTP_OK)
        return ret;
    if(method == HTTP_GET || method == HTTP_POST)
    {
        ret = httpSendUserdata(context, data);
    }
    HTTPDBG("httpSendRequest ret:%d",ret);
    return ret;
}

HTTPResult httpClose(HttpClientContext* context)
{
    HTTPResult ret = HTTP_OK;
    if(context->isHttps)
    {
        ret = httpSslClose(context);
    }
    else
    {
        if(context->socket >= 0)
            close(context->socket);
    }
    /*if(context->basicAuthUser){
        free(context->basicAuthUser);
    if(context->basicAuthPassword)
        free(context->basicAuthPassword);let up level free it*/
    context->socket = -1;
    HTTPDBG("httpClose");
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpClose, P_INFO, 1, "httpClose,ret=%d",ret);
    return ret;
}
char *httpRecvRespBuf = NULL;

HTTPResult httpRecvResponse(HttpClientContext* context, HttpClientData * data) 
{
    //Receive response
    HTTPResult ret = HTTP_CONN;
    INT32 trfLen = 0;
    //char buf[CHUNK_SIZE+2] = {0};

    HTTPDBG("Receiving response");
   
    ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecvResponse_1, P_INFO, 0, "Receiving response");

    if(context->socket < 0)
        return ret;
    
    httpRecvRespBuf = malloc(CHUNK_SIZE+2);
    memset(httpRecvRespBuf, 0, (CHUNK_SIZE+2));
   
    if(data->isMoreContent)
    {
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecvResponse_2, P_INFO, 0, "data->isMoreContent is true continue parseContent");
        data->respBuf[0] = '\0';
        ret = httpParseContent(context, httpRecvRespBuf, trfLen, data);
    }
    else
    {
         ret = httpRecv(context, httpRecvRespBuf, 1, CHUNK_SIZE - 1, &trfLen);    // recommended by Rob Noble to avoid timeout wait
         if(ret != HTTP_OK)
         {      
            if(httpRecvRespBuf != NULL)
            {
                free(httpRecvRespBuf);
                httpRecvRespBuf = NULL;
            }
            return ret;
         }
         ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpRecvResponse_3, P_INFO, 1, "has read %d bytes", trfLen);
         httpRecvRespBuf[trfLen] = '\0';
         if(trfLen)
         {
             HTTPDBG("Received \r\n(%s\r\n)", httpRecvRespBuf);
             ret = httpParseHeader(context, httpRecvRespBuf, trfLen, data);
         }
     }

     if(httpRecvRespBuf != NULL)
     {
         free(httpRecvRespBuf);
         httpRecvRespBuf = NULL;
     }
     return ret;
}

HTTPResult httpGet(HttpClientContext* context, const char* url,  HttpClientData * data,  int timeout) //Blocking
{
    context->timeout_s = timeout;
    HTTPResult ret = HTTP_CONN;
    ret = httpConnect(context, url);
    if(ret == HTTP_OK)
    {
        ret = httpSendRequest(context, url, HTTP_GET, data);
        if(ret == HTTP_OK)
        {
            ret = httpRecvResponse(context, data);
        }
    }
    return ret;
}

HTTPResult httpPost(HttpClientContext* context, const char* url,  HttpClientData * data,  int timeout) //Blocking
{
    context->timeout_s = timeout;
    HTTPResult ret = HTTP_CONN;
    ret = httpConnect(context, url);
    if(ret == HTTP_OK)
    {
        ret = httpSendRequest(context, url, HTTP_POST, data);
        if(ret == HTTP_OK)
        {
            ret = httpRecvResponse(context, data);
        }
    }
    return ret;
}


HTTPResult httpConnectTimeout(INT32 connectFd, UINT32 timeout)
{
    fd_set writeSet;
    fd_set errorSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(connectFd,&writeSet);
    FD_SET(connectFd,&errorSet);
    struct timeval tv;
    tv.tv_sec  = timeout;
    tv.tv_usec = 0;
    
    if(select(connectFd+1, NULL, &writeSet, &errorSet, &tv)<=0)
    {
        int mErr = sock_get_errno(connectFd);
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectTimeout_1, P_INFO, 1, "connect select<0 get errno=%d", mErr);
        if(mErr)
        {
            return HTTP_CONN;
        }
        else
        {
            return HTTP_TIMEOUT;
        }
    }
    else
    {
        if(FD_ISSET(connectFd, &errorSet))
        {
            int mErr = sock_get_errno(connectFd);
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectTimeout_2, P_INFO, 1, "select error fd set get errno=%d", mErr);
            if(mErr)
            {
                return HTTP_CONN;
            }
        }
        else if(FD_ISSET(connectFd, &writeSet))
        {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectTimeout_3, P_INFO, 0, "errno=115(EINPROGRESS) connect success in time(10s)");
        }
    }

    return HTTP_OK;

}



HTTPResult httpConnectSocket(INT32 socket,struct sockaddr *addr, INT32 addrlen)
{

    HTTPResult ret = HTTP_OK;
    INT32 errCode;

    if(connect(socket,addr,addrlen) == 0)
    {                                         
        ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectSocket_1, P_INFO, 0, "httpConnectSocket connect success");
    }
    else
    {
        errCode = sock_get_errno(socket);
        if(errCode == EINPROGRESS)
        {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectSocket_2, P_ERROR, 0, "httpConnectSocket connect is ongoing");

            ret = httpConnectTimeout(socket, 25);//from 10s to 25s 
            if(ret == 0)
            {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, AhttpConnectSocket_3, P_INFO, 0, "httpConnectSocket connect success");
            }
            else
            {
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectSocket_4, P_ERROR, 1, "httpConnectSocket connect fail,error code %d", errCode);
                if(socket_error_is_fatal(errCode))
                {
                    ret = HTTP_FATAL_ERROR;
                }
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpConnectSocket_5, P_ERROR, 1, "httpConnectSocket connect fail %d",errCode);
            ret = HTTP_CONN;
        }
    }

   return ret;
}

