/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_http_task.c
*
*  Description: Process http(s) client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "FreeRTOS.h"
#include "semphr.h"
#include "osasys.h"
#include "cmsis_os2.h"
#include "task.h"
#include "atec_http.h"
#include "atec_http_cnf_ind.h"
#include "lwip/sockets.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

#include <stdio.h>
#include <string.h>
#include "cms_util.h"
#include "netdb.h"
#include "httpclient.h"
#include "atec_http_cnf_ind.h"
#include "at_http_task.h"

#define HTTPRECV_TASK_STACK_SIZE   5120   
#define HTTPSEND_TASK_STACK_SIZE   5632


osThreadId_t httprecv_task_handle = NULL;
osThreadId_t httpsend_task_handle = NULL;

osMessageQueueId_t http_msgqueue = NULL;

static UINT8 gHttpClientNumber = 0;
httpAtCmd gHttpClientAtcmd[HTTPCLIENT_ATCMD_MAX_NUM] = {
     {HTTP_IS_NOT_USED,HTTPSTAT_DEFAULT,0,NULL,NULL,NULL},
};
static osMutexId_t httpMutex = NULL;
 static osSemaphoreId_t flowMutex = NULL;
BOOL httpRecvTaskRunning = FALSE;
BOOL httpSendTaskRunning = FALSE;

uint8_t httpSlpHandler = 0xff;

httpProgress gRetentionProgress={0};
extern char *gCurPath;
 
extern void atCmdUartHttpFlowControl(UINT8 mode);
__WEAK void atCmdUartHttpFlowControl(UINT8 mode){};

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

void httpSaveProgress(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    httpNvmHeader nvmHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(HTTP_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSaveProgress_0_1, P_ERROR, 0, "can't open/create NVM: httpProg.nvm, save NVM failed");
        return;
    }

    /*
     * write the header
    */
    nvmHdr.fileBodySize   = sizeof(httpProgress) * HTTP_FILE_NUM;
    nvmHdr.version        = HTTP_NVM_FILE_VERSION;
    nvmHdr.checkSum       = check_sum((uint8_t *)&gRetentionProgress, sizeof(httpProgress)*HTTP_FILE_NUM);
    
    writeCount = OsaFwrite(&nvmHdr, sizeof(httpNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSaveProgress_0_2, P_ERROR, 0, "httpProg.nvm, write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&gRetentionProgress, sizeof(httpProgress), HTTP_FILE_NUM, fp);
    if (writeCount != HTTP_FILE_NUM)
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSaveProgress_0_3, P_ERROR, 0, "httpProg.nvm, write the file body failed");
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSaveProgress_1, P_INFO, 0, "success save http file download progress");
    }
    OsaFclose(fp);
    return;
}

INT8 httpReadProgress(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    httpNvmHeader    nvmHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(HTTP_NVM_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpReadProgress_0, P_INFO, 0, "can't open NVM: httpProg.nvm, start from head");
        return -1;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&nvmHdr, sizeof(httpNvmHeader), 1, fp);
    if(nvmHdr.version == HTTP_NVM_FILE_VERSION)
    {
        /*
         * read the file body retention context
        */
        readCount = OsaFread(&gRetentionProgress, sizeof(httpProgress), HTTP_FILE_NUM, fp);
    
        if (readCount != 1)
        {
            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpReadProgress_1_1, P_ERROR, 0, "httpProg.nvm, can't read body");
            OsaFclose(fp);
            return -4;
        }
        OsaFclose(fp);

        uint8_t cur_check_sum = 0xff & check_sum((uint8_t *)&gRetentionProgress, nvmHdr.fileBodySize);
        if(nvmHdr.checkSum != cur_check_sum)
        {
            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpReadProgress_1_2, P_ERROR, 2, "check sum failed, store value = 0x%x, cur value = 0x%x", nvmHdr.checkSum, cur_check_sum);
            return -5;
        }
    }
    ECOMM_STRING(UNILOG_ATCMD_HTTP, httpReadProgress_1, P_INFO, "success read http file download progress,path:%s",(uint8_t*)(gRetentionProgress.path));
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpReadProgress_2, P_INFO, 2, "head = %d whole range = %d", gRetentionProgress.rangeHead, gRetentionProgress.range);
    return 0;
}

void httpClearProgress(void)
{
    memset(&gRetentionProgress, 0, sizeof(httpProgress));
    httpSaveProgress();
}

#define HTTP_CLIENT_AT_INTERFACE_START
CmsRetId httpCreateReq(UINT32 atHandle, httpAtCmd* httpCmd, httpCmdMsg httpMsg)
{
    CmsRetId rc = CMS_FAIL;
    osStatus_t status;

    //memset(&httpMsg, 0, sizeof(httpMsg));
    httpMsg.cmd_type = HTTP_CREATE_COMMAND;
    httpMsg.httpcmd = httpCmd;
    httpMsg.reqhandle = atHandle;

    if(!httpSendTaskRunning)
    {
        httpSendTaskRunning = TRUE;
        httpSendTaskInit();
    }

    status = osMessageQueuePut(http_msgqueue, &httpMsg, 0, 0);
    if(status == osErrorResource)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpCreateReq_1, P_WARNING, 0, "http send task queue full");

    if(status!=osOK)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpCreateReq_2, P_WARNING, 1, "http send task queue error %d ",status);

    rc = CMS_RET_SUCC;

    return rc;
}

CmsRetId httpConnectReq(UINT32 atHandle,httpAtCmd* httpCmd )
{
    httpCmdMsg httpMsg;
    CmsRetId rc = CMS_FAIL;
    osStatus_t status;

    memset(&httpMsg, 0, sizeof(httpMsg));
    httpMsg.cmd_type = HTTP_CONNECT_COMMAND;
    httpMsg.httpcmd = httpCmd;
    httpMsg.reqhandle = atHandle;

    status = osMessageQueuePut(http_msgqueue, &httpMsg, 0, 0);
    if(status == osErrorResource)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpConnectReq_1, P_WARNING, 0, "http send task queue full");

    if(status!=osOK)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpConnectReq_2, P_WARNING, 1, "http send task queue error %d ",status);

    rc = CMS_RET_SUCC;

    return rc;
}

CmsRetId httpSendReq(UINT32 atHandle,httpAtCmd* httpCmd)
{
    httpCmdMsg httpMsg;
    CmsRetId rc = CMS_FAIL;
    osStatus_t status;

    memset(&httpMsg, 0, sizeof(httpMsg));
    httpMsg.cmd_type = HTTP_SEND_COMMAND;
    httpMsg.httpcmd = httpCmd;
    httpMsg.reqhandle = atHandle;

    status = osMessageQueuePut(http_msgqueue, &httpMsg, 0, 0);
    if(status == osErrorResource)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSendReq_1, P_WARNING, 0, "http send task queue full");

    if(status!=osOK)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSendReq_2, P_WARNING, 1, "http send task queue error %d ",status);

    rc = CMS_RET_SUCC;

    return rc;
}

CmsRetId httpDestroyReq(UINT32 atHandle, httpAtCmd* httpCmd,int httpclientId)
{
    httpCmdMsg httpMsg;
    CmsRetId rc = CMS_FAIL;
    osStatus_t status;

    memset(&httpMsg, 0, sizeof(httpMsg));
    httpMsg.cmd_type = HTTP_DESTROY_COMMAND;
    httpMsg.httpcmd = httpCmd;
    httpMsg.reqhandle = atHandle;
    httpMsg.clinetId = httpclientId;

    status = osMessageQueuePut(http_msgqueue, &httpMsg, 0, 0);
    if(status == osErrorResource)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpDestroyReq_1, P_WARNING, 0, "http send task queue full");

    if(status!=osOK)
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpDestroyReq_2, P_WARNING, 1, "http send task queue error %d ",status);

    rc = CMS_RET_SUCC;

    return rc;
}

#define HTTP_CLIENT_AT_INTERFACE_END

URLError httpAtIsUrlValid(char* url)
{
    char defaultPort[4]={0};
    char *host = NULL;
    char *port = NULL;
    INT32 hostLen = 0;
    INT32 portLen = 0;

    URLError result = URL_FORMAT;

    if(!strncmp(url, "http", strlen("http")))
    {
        strcpy(defaultPort, "80");
    }
    else if(!strncmp(url, "https", strlen("https")))
    {
        strcpy(defaultPort, "443");
    }
    else
    {
        return result;//URL is invalid
    }

    char* hostPtr = (char*) strstr(url, "://");
    if (hostPtr == NULL) 
    {
        return result; //URL is invalid
    }

    hostPtr+=3;

    char* portPtr = strchr(hostPtr, ':');
    if( portPtr != NULL ) 
    {
        hostLen = portPtr - hostPtr;
        portPtr++;
        char* tempPtr = strchr(portPtr, '/');
        if(tempPtr != NULL)
        {
            portLen = tempPtr - portPtr;
        }
        else
        {
            portLen = strlen(portPtr);
        }
    } 
    else 
    {
        char* pathPtr = strchr(hostPtr, '/');
        if( pathPtr != 0) 
        {
            hostLen = pathPtr - hostPtr;
        } 
        else 
        {
            hostLen = strlen(hostPtr);
        }
    }

    if(hostLen != 0) 
    {
        host = malloc(hostLen + 1);
        memcpy(host, hostPtr, hostLen);
        host[hostLen] = '\0';
    } 
    else 
    {
        return result;
    }

    if(portLen != 0) 
    {
        port = malloc(portLen + 1);
        memcpy(port, portPtr, portLen);
        port[portLen] = '\0';
    }
    #if 0//remove block API from create command,since we will get address when connect
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    getAddrRet = getaddrinfo(host, port == NULL ? defaultPort : port, &hints, &addrList);
    if(getAddrRet != 0) {
        result = URL_NETWORK;
    }
    else {
        result = URL_OK;
        freeaddrinfo(addrList);
    }
    #endif

    result = URL_OK;

    if(host)
    {
        free(host);
    }
    if(port)
    {
        free(port);
    }
    return result;
}

BOOL httpAtConnect(httpAtCmd* httpCmd)
{
    HTTPResult result = httpConnect(httpCmd->clientContext, httpCmd->host);
    
    if(result != HTTP_OK) 
    {
        return FALSE;
    }
    return TRUE;
}

BOOL httpMutexCreate(void)
{
    if(httpMutex == NULL) 
    {
        httpMutex = osMutexNew(NULL);
    }
    if(httpMutex == NULL)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL httpMutexAcquire(void)
{
    if (osMutexAcquire(httpMutex, osWaitForever) != CMS_RET_SUCC)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void httpMutexRelease(void)
{
    osMutexRelease(httpMutex);
}

void httpMutexDelete(void)
{
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpMutexDelete_1, P_INFO, 0, "ok");
    if(httpMutex != NULL)
    {
        osMutexDelete(httpMutex);
        httpMutex = NULL;
    }
}

BOOL flowMutexCreate(void)
{
    if(flowMutex == NULL) 
    {
        flowMutex = osSemaphoreNew(1U, 0, NULL);
    }
    if(flowMutex == NULL)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void flowMutexDelete(void)
{
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, flowMutexDelete_1, P_INFO, 0, "......flowMutexDelete");
    if(flowMutex != NULL){
        osSemaphoreDelete(flowMutex);
        flowMutex = NULL;
    }
}


httpAtCmd* httpGetFreeClient()
{
    for(int i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
    {
        if(gHttpClientAtcmd[i].isUsed == 0)
        {
            memset(&gHttpClientAtcmd[i], 0, sizeof(httpAtCmd));
            gHttpClientAtcmd[i].isUsed = HTTP_IS_USED;
            gHttpClientAtcmd[i].httpclientId = i;
            gHttpClientNumber++;
            return &gHttpClientAtcmd[i];
        }
    }
    return NULL;
}

void httpCreateClientData(httpAtCmd* httpCmd)
{
    HttpClientData* clientData = NULL;
    httpCmd->clientData = malloc(sizeof(HttpClientData));
    clientData = httpCmd->clientData;
    memset(clientData, 0, sizeof(HttpClientData));
    clientData->headerBufLen = MAX_HEAD_BUFFER_SIZE;
    clientData->headerBuf = malloc(MAX_HEAD_BUFFER_SIZE);
    clientData->respBufLen = MAX_RSP_BUFFER_SIZE;
    clientData->respBuf = malloc(MAX_RSP_BUFFER_SIZE);
}

void httpReleaseClientData(httpAtCmd* httpCmd)
{
    if(httpCmd && httpCmd->clientData)
    {
        if(httpCmd->clientData->postBuf)
        {
            free(httpCmd->clientData->postBuf);
        }
        if(httpCmd->clientData->postContentType)
        {
            free(httpCmd->clientData->postContentType);
        }
        if(httpCmd->clientData->respBuf)
        {
            free(httpCmd->clientData->respBuf);
        }
        if(httpCmd->clientData->headerBuf)
        {
            free(httpCmd->clientData->headerBuf);
        }
        memset(httpCmd->clientData, 0, sizeof(httpCmd->clientData));
        free(httpCmd->clientData);
        httpCmd->clientData = NULL;
    }
}

httpAtCmd* httpFindClient(int httpclientId)
{
    for(int i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
    {
        if(gHttpClientAtcmd[i].httpclientId == httpclientId && gHttpClientAtcmd[i].isUsed)
        {
            return &gHttpClientAtcmd[i];
        }
    }
    return NULL;
}

void httpFreeClient(int httpclientId)
{
    for(int i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
    {
        httpAtCmd* httpContext = &gHttpClientAtcmd[i];
        if(httpContext->httpclientId == httpclientId && httpContext->isUsed)
        {
            if(httpContext->host)
            {
                free(httpContext->host);
                httpContext->host = NULL;
            }
            if(httpContext->url)
            {
                free(httpContext->url);
                httpContext->url = NULL;
            }
            
            if(httpContext->clientContext)
            {
                if(httpContext->clientContext->basicAuthUser)
                {
                    free(httpContext->clientContext->basicAuthUser);
                    httpContext->clientContext->basicAuthUser = NULL;
                }
                if(httpContext->clientContext->basicAuthPassword)
                {
                    free(httpContext->clientContext->basicAuthPassword);
                    httpContext->clientContext->basicAuthPassword = NULL;
                }
                if(httpContext->clientContext->caCert)
                {
                    free((char*)httpContext->clientContext->caCert);
                    httpContext->clientContext->caCert = NULL;
                }
                if(httpContext->clientContext->clientCert)
                {
                    free((char*)httpContext->clientContext->clientCert);
                    httpContext->clientContext->clientCert = NULL;
                }
                if(httpContext->clientContext->clientPk)
                {
                    free((char*)httpContext->clientContext->clientPk);
                    httpContext->clientContext->clientPk = NULL;
                }
                if(httpContext->clientContext->ssl)
                {
                    free((char*)httpContext->clientContext->ssl); 
                    httpContext->clientContext->ssl = NULL;
                }
                if(httpContext->clientContext)
                {
                    free(httpContext->clientContext);
                    httpContext->clientContext = NULL;
                }
            }
            httpReleaseClientData(httpContext);
            memset(httpContext, 0, sizeof(httpAtCmd));
            return;
        }
    }
}

int httpGetMaxSocketId()
{
    int maxFs = -1;
    for(int i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
    {
        if(gHttpClientAtcmd[i].isUsed && gHttpClientAtcmd[i].clientContext)
        {
            if(maxFs < gHttpClientAtcmd[i].clientContext->socket)
            {
                maxFs = gHttpClientAtcmd[i].clientContext->socket;
            }
        }
    }
    return maxFs;
}

int httpGetSocketId(int index)
{
    int maxFs = -1;
    
    maxFs = gHttpClientAtcmd[index].clientContext->socket;

    return maxFs;
}

void httpSENDIND(UINT16 indBodyLen, void *indBody, UINT32 reqhandle)
{
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSENDIND_1, P_INFO, 0, "send indication sig");

    if(indBodyLen == 0 || indBody == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSENDIND_2, P_ERROR, 2, "invalid indBodyLen %u, indBody 0x%x ", indBodyLen, indBody);
        return;
    }

    applSendCmsInd(reqhandle, APPL_HTTP, APPL_HTTP_IND, indBodyLen+1, indBody);

    return;
}
void httpOutflowContrl()
{
    //ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpOutflowContrl, P_INFO, 0, "release the flowMutex");
    osSemaphoreRelease(flowMutex);
}

void httpRecvInd(httpAtCmd* httpCmd, UINT8 flag)
{
    CHAR* pIndBuf = NULL;
    httpIndMsg  indMsg;
    UINT16 respLen = 0;
    UINT16 headerLen = 0;
    static uint16_t seqnum = 0;
    headerLen = strlen(httpCmd->clientData->headerBuf);
    //contentLen  = strlen(httpCmd->clientData->respBuf);//removed, should not only check strlen, since content may be raw data

    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpRecvInd_1, P_INFO, 2, "httpRecvInd %d %d",headerLen,httpCmd->clientData->blockContentLen);
    if(headerLen > 0) 
    {
        pIndBuf = malloc(MAX_ATCMD_SIZE);
        memset(pIndBuf, 0, (MAX_ATCMD_SIZE));
        indMsg.pHttpInd = pIndBuf;
        snprintf(pIndBuf, (MAX_ATCMD_SIZE), "\r\n+HTTPRESPH: %d,%d,%d,",
            httpCmd->httpclientId,
            httpCmd->clientContext->httpResponseCode,
            strlen(httpCmd->clientData->headerBuf));
        respLen = strlen(pIndBuf);
        memcpy(pIndBuf+respLen, httpCmd->clientData->headerBuf, headerLen);
        httpSENDIND(sizeof(indMsg), &indMsg, httpCmd->reqhandle);
        seqnum = gRetentionProgress.seqnum;
        osSemaphoreRelease(flowMutex);
        if(httpCmd->clientData->isRange){
            if(httpCmd->clientData->contentRange == 0){
                gRetentionProgress.range = httpCmd->clientData->recvContentLength;
            }else{
                gRetentionProgress.range = httpCmd->clientData->contentRange;
            }
            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpRecvInd_1_1, P_INFO, 1, "save the path and the whole length %d",gRetentionProgress.range);
            memset(gRetentionProgress.path, 0 ,MAXPATH_SIZE);
            strncpy(gRetentionProgress.path, gCurPath, MAXPATH_SIZE-1);
            httpSaveProgress();
        }
    }
    if(httpCmd->clientData->blockContentLen > 0) 
    {
        seqnum += 1;
        pIndBuf = malloc(MAX_ATCMD_SIZE);
        memset(pIndBuf, 0, (MAX_ATCMD_SIZE));
        indMsg.pHttpInd = pIndBuf;
        snprintf(pIndBuf, (MAX_ATCMD_SIZE), "\r\n+HTTPRESPC: %d,%d,%d,%d,",
            httpCmd->httpclientId,
            flag,
#if HTTP_DISP_SEQNUM
            seqnum,
#endif
            httpCmd->clientData->recvContentLength,
            httpCmd->clientData->blockContentLen*2);
        
        respLen = strlen(pIndBuf);
        cmsHexToHexStr(pIndBuf+respLen, httpCmd->clientData->blockContentLen*2, (const UINT8 *)httpCmd->clientData->respBuf, httpCmd->clientData->blockContentLen);
        respLen += httpCmd->clientData->blockContentLen*2;
        pIndBuf[respLen]='\r';
        pIndBuf[respLen+1]='\n';
        if( osSemaphoreAcquire(flowMutex,  portMAX_DELAY) == osOK ){
            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpRecvInd_2, P_INFO, 0, "acquire the flowMutex");
            httpSENDIND(sizeof(indMsg), &indMsg, httpCmd->reqhandle);
            atCmdUartHttpFlowControl(1);
        }
        if(httpCmd->clientData->isRange){
            if(flag == 1){
                gRetentionProgress.rangeHead += (MAX_RSP_BUFFER_SIZE-1);
                gRetentionProgress.seqnum = seqnum;
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpRecvInd_2_1, P_INFO, 2, "save the rangehead=%d", gRetentionProgress.rangeHead);
            }else{
                gRetentionProgress.rangeHead = 0;
                gRetentionProgress.seqnum = 0;
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpRecvInd_2_2, P_INFO, 0, "last range, rangehead back to zero");
            }
            httpSaveProgress();
        }
    }
}

void httpErrInd(UINT8 httpClientId, HTTPResult errorCode, UINT32 reqhandle, UINT32 rspCode)
{
    CHAR* pIndBuf = malloc(48);
    httpIndMsg  indMsg;
    indMsg.pHttpInd = pIndBuf;
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpErrInd, P_WARNING, 2, "clientID: %d, ERROR CODE:%d", httpClientId, errorCode);
    if(errorCode==HTTP_ERROR)
        snprintf(pIndBuf, 48, "+HTTPERR: %d,%d,%d", httpClientId,errorCode,rspCode);
    else
        snprintf(pIndBuf, 48, "+HTTPERR: %d,%d", httpClientId,errorCode);
    
    httpSENDIND(sizeof(indMsg), &indMsg, reqhandle);
}

BOOL httpIsConnectExist()
{
    BOOL exist = FALSE;
    for(int i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
    {
        if(gHttpClientAtcmd[i].status != HTTPSTAT_CLOSED)
        {
            exist = TRUE;
        }
    }
    return exist;
}

BOOL httpIsAnyClientExist()
{
    BOOL exist = FALSE;
    for(int i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
    {
        if(gHttpClientAtcmd[i].isUsed!= 0)
        {
            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpIsAnyClientExist_1, P_WARNING, 1, "clientID: %d exsit", i);
            exist = TRUE;
        }
    }
    return exist;
}

BOOL httpRecvTaskInit()
{
    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));

    task_attr.name = "httpRecv";
    //task_attr.stack_mem = NULL;
    task_attr.stack_size = HTTPRECV_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal6;

    //task_attr.cb_mem = NULL;//task control block
    //task_attr.cb_size = sizeof(StaticTask_t);//size of task control block
    //memset(httprecvTaskStack, 0xA5,HTTPRECV_TASK_STACK_SIZE);
    if(httprecv_task_handle == NULL)
    {
        httprecv_task_handle = osThreadNew(httpClientRecvTask, NULL,&task_attr);
        if(httprecv_task_handle == NULL)
        {
            EC_ASSERT(FALSE,0,0,0);
        }
    }
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpRecvTaskInit_1, P_INFO, 0, "httpRecvTaskInit success");
    return TRUE;
}

BOOL httpSendTaskInit()
{
    osThreadAttr_t task_attr;

    // init message queue
    if(http_msgqueue == NULL)
    {
        http_msgqueue = osMessageQueueNew(16, sizeof(httpCmdMsg), NULL);
    }
    else
    {
        osMessageQueueReset(http_msgqueue);
    }
    if(http_msgqueue == NULL)
    {
        EC_ASSERT(FALSE,0,0,0);
    }

    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "httpSend";
    task_attr.stack_size = HTTPSEND_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;
    if(httpsend_task_handle == NULL)
    {       
        httpsend_task_handle = osThreadNew(httpClientSendTask, NULL,&task_attr);
        if(httpsend_task_handle == NULL)
        {
            EC_ASSERT(FALSE,0,0,0);
        }
    }
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpSendTaskInit_1, P_INFO, 0, "httpSendTaskInit success");
    return TRUE;
}

void httpClientRecvTask(void* arg)
{
    HTTPResult result;
    httpAtCmd* httpCmd = NULL;
    httpCmdMsg httpMsg;
    osStatus_t status;
    fd_set readFs,errorFs;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int maxFs = -1, ret;
    UINT8 i;
    int httpStatus = -1;
    int httpRecvTaskStatus = FALSE;

    for(;;)
    {
        httpRecvTaskStatus = FALSE;
        for(i=0; i<HTTPCLIENT_ATCMD_MAX_NUM; i++)
        {
            if(gHttpClientAtcmd[i].isUsed == HTTP_IS_USED)
            {
                httpRecvTaskRunning = TRUE;
                httpStatus = gHttpClientAtcmd[i].status;
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_0, P_INFO, 2, "....isUsed:%d, httpStatus:%d", gHttpClientAtcmd[i].isUsed, httpStatus);
                switch(httpStatus)
                {
                    case HTTPSTAT_CONNECTED:
                    {
                        httpRecvTaskStatus = TRUE;
                        maxFs = httpGetSocketId(i);
                        if(maxFs >= 0)
                        {
                            httpMutexAcquire();
                            FD_ZERO(&readFs);
                            FD_ZERO(&errorFs);
                            httpCmd = &gHttpClientAtcmd[i];
                            
                            if(httpCmd->isUsed && httpCmd->clientContext && httpCmd->clientContext->socket >= 0)
                            {
                                FD_SET(httpCmd->clientContext->socket, &readFs);
                                FD_SET(httpCmd->clientContext->socket, &errorFs);
                            }
                            ret = select(maxFs + 1, &readFs, NULL, &errorFs, &tv);
                            httpMutexRelease();
                            if(ret>0)
                            {
                                if(httpCmd->isUsed && httpCmd->clientContext && httpCmd->clientContext->socket >= 0 && FD_ISSET(httpCmd->clientContext->socket, &readFs))
                                {
                                    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_1, P_INFO, 2, "to read client:%d, socket:%d", i, httpCmd->clientContext->socket);
                                    do
                                    {
                                        httpMutexAcquire();
                                        
                                        if(httpCmd->status != HTTPSTAT_CONNECTED)
                                        {
                                            httpMutexRelease();
                                            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_1_1, P_INFO, 0, "status changed break while");
                                            break;
                                        }

                                        memset(httpCmd->clientData->headerBuf, 0, httpCmd->clientData->headerBufLen);
                                        memset(httpCmd->clientData->respBuf, 0, httpCmd->clientData->respBufLen);
                                        result = httpRecvResponse(httpCmd->clientContext, httpCmd->clientData);
            
                                        if(result == HTTP_OK)
                                        {
                                            httpRecvInd(httpCmd,0);
                                            httpCmd->status = HTTPSTAT_CONNECTED;
                                        }
                                        else if(result == HTTP_MOREDATA)
                                        {
                                            httpRecvInd(httpCmd,1);
                                        }
                                        else if(result == HTTP_CONN)
                                        {
                                            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_2, P_INFO, 0, "httpRecvResponse return HTTP_CONN release mutex try again");
                                        }
                                        else
                                        {
                                            int mErr = sock_get_errno(httpCmd->clientContext->socket);
                                            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_3, P_WARNING, 2, "httpRecvResponse return %d error=%d, close it", result,mErr);
                                            memset(&httpMsg, 0, sizeof(httpMsg));
                                            httpMsg.cmd_type = HTTP_CLOSE_TCP_COMMAND;
                                            httpMsg.httpcmd = httpCmd;
                                            httpMsg.reqhandle = httpCmd->reqhandle;
                                            httpMsg.result = result;
                                            httpMsg.clinetId = httpCmd->httpclientId;
                                            
                                            status = osMessageQueuePut(http_msgqueue, &httpMsg, 0, 0);
                                            if(status != osOK)
                                            {
                                                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_3_1, P_INFO, 0, "send msg fail, status:%d", status);
                                            }
                                        }
                                        httpMutexRelease();
                                    }
                                    while(result == HTTP_MOREDATA);
                                    httpCmd->isReceiving = FALSE;                     
                                }
                                
                                if(httpCmd->isUsed && httpCmd->clientContext && httpCmd->clientContext->socket >= 0 && FD_ISSET(httpCmd->clientContext->socket, &errorFs))
                                {
                                    int mErr = sock_get_errno(httpCmd->clientContext->socket);
                                    if(socket_error_is_fatal(mErr))
                                    {
                                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_7, P_INFO, 3, "selected fatal error client:%d, socket:%d, errno:%d", i, httpCmd->clientContext->socket, mErr);
                                        memset(&httpMsg, 0, sizeof(httpMsg));
                                        httpMsg.cmd_type = HTTP_CLOSE_TCP_COMMAND;
                                        httpMsg.httpcmd = httpCmd;
                                        httpMsg.reqhandle = httpCmd->reqhandle;
                                        httpMsg.result = HTTP_CLOSED;
                                        httpMsg.clinetId = httpCmd->httpclientId;
                                        
                                        status = osMessageQueuePut(http_msgqueue, &httpMsg, 0, 0);
                                        if(status != osOK)
                                        {
                                            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_8, P_INFO, 0, "send msg fail, status:%d", status);
                                        }
                                    }
                                    else
                                    {
                                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_9, P_INFO, 3, "selected not fatal error client:%d, socket:%d, errno:%d", i, httpCmd->clientContext->socket, mErr);
                                    }
                                }
                            }
                            else
                            {
                                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_10, P_INFO, 2, "recv select return:%d, errno:%d", ret, errno);
                                //vTaskDelay(1000);
                            }
                        }

                        break;
                    }

                    case HTTPSTAT_CREATED:
                    case HTTPSTAT_CLOSED:
                    case HTTPSTAT_CONNECTING:
                    {
                        httpRecvTaskStatus = TRUE;
                        osDelay(5000);
                        break;
                    }                       

                    case HTTPSTAT_DEFAULT:
                    {
                        httpRecvTaskStatus = TRUE;
                        osDelay(5000);
                        break;
                    }

                    case HTTPSTAT_DESTROYING:
                    case HTTPSTAT_DESTROYED:
                    default:
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_12_1, P_INFO, 1, "httpStatus:%d", httpStatus);
                        httpRecvTaskStatus = FALSE;
                        break;
                    }
                }
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_11, P_INFO, 2, "........isUsed:%d, httpStatus:%d", gHttpClientAtcmd[i].isUsed, httpStatus);
            }
        }
        if(httpRecvTaskStatus == FALSE)
        {
            break;
        }
    }
    
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientRecvTask_12, P_INFO, 0, "no http client connecting httpClientRecvTask task deleted");
    osThreadSetPriority(osThreadGetId(), osPriorityNormal); //should be higher than http send task
    ostaskENTER_CRITICAL();
    httprecv_task_handle = NULL;
    httpRecvTaskRunning = FALSE;
    ostaskEXIT_CRITICAL();

    vTaskDelete(NULL);
}

void httpClientSendTask(void* arg)
{
    osStatus_t status;
    httpCmdMsg msg;
    httpCnfCmdMsg cnfMsg;
    HTTPResult result;
    BOOL ret;
    httpAtCmd* httpContext = NULL;
    int primSize=0;
    uint8_t waitloop = 10;
    BOOL needCloseSocket = FALSE;

    slpManRet_t vote_ret = slpManApplyPlatVoteHandle("ECHTTP", &httpSlpHandler);
    OsaCheck(RET_TRUE == vote_ret,vote_ret,0,0);
    vote_ret = slpManPlatVoteDisableSleep(httpSlpHandler,SLP_SLP2_STATE);
    OsaCheck(RET_TRUE == vote_ret,vote_ret,0,0);
    httpMutexCreate();
    flowMutexCreate();

    while(httpSendTaskRunning)
    {
        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_1, P_INFO, 0, "send task wait for signal");
        status = osMessageQueueGet(http_msgqueue, &msg, 0, osWaitForever);
        EC_ASSERT(status==osOK,status,0,0);
        httpContext = msg.httpcmd;
        memset(&cnfMsg, 0, sizeof(httpCnfCmdMsg));

        switch(msg.cmd_type)
        {
            case HTTP_CREATE_COMMAND:
            {
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_2, P_INFO, 0, "recv create command");
                
                httpContext->clientContext = malloc(sizeof(HttpClientContext));
                memset(httpContext->clientContext, 0, sizeof(HttpClientContext));
                
                httpContext->clientContext->timeout_s = 2;  //default send timeout 2 second,no need to wait for a long time for TCP
                httpContext->clientContext->timeout_r = 20;  //default recv timeout 20 second
                httpContext->clientContext->socket = -1;
                
                if(msg.httpCreateParam.authUser != NULL)
                {
                    //httpContext->clientContext->basicAuthUser= malloc(strlen((CHAR*)msg.httpCreateParam.authUser)+1);
                    //strcpy((CHAR*)httpContext->clientContext->basicAuthUser, (CHAR*)msg.httpCreateParam.authUser);
                    httpContext->clientContext->basicAuthUser = (CHAR *)msg.httpCreateParam.authUser;
                }
                if(msg.httpCreateParam.authPassword!= NULL)
                {
                    //httpContext->clientContext->basicAuthPassword= malloc(strlen((CHAR*)msg.httpCreateParam.authPassword)+1);
                    //strcpy((CHAR*)httpContext->clientContext->basicAuthPassword, (CHAR*)msg.httpCreateParam.authPassword);
                    httpContext->clientContext->basicAuthPassword = (CHAR *)msg.httpCreateParam.authPassword;
                }
                if(msg.httpCreateParam.serverCert != NULL)
                {
                    UINT32 serverCertLen = strlen((CHAR*)msg.httpCreateParam.serverCert);
                    //UINT8* realCaCert = malloc(serverCertLen+1);
                    //memset(realCaCert, 0, (serverCertLen+1));
                    //memcpy(realCaCert, msg.httpCreateParam.serverCert, serverCertLen);
                    //getStrFromHex(realCaCert, (CHAR*)serverCert, serverCertLen);
                    //cmsHexStrToHex(realCaCert, serverCertLen, (CHAR *)msg.httpCreateParam.serverCert, serverCertLen*2);
                    httpContext->clientContext->caCertLen = serverCertLen;
                    httpContext->clientContext->caCert = (CHAR*)msg.httpCreateParam.serverCert;
                    //free(msg.httpCreateParam.serverCert);
                }
                
                if(msg.httpCreateParam.clientCert!= NULL)
                {
                    UINT32 clientCertLen = strlen((CHAR*)msg.httpCreateParam.clientCert)/2;
                    UINT8* realClientCert= malloc(clientCertLen+1);
                    //getStrFromHex(realClientCert, (CHAR*)clientCert, clientCertLen);
                    cmsHexStrToHex(realClientCert, clientCertLen, (CHAR *)msg.httpCreateParam.clientCert, clientCertLen*2);
                    httpContext->clientContext->clientCertLen = clientCertLen;
                    httpContext->clientContext->clientCert = (CHAR*)realClientCert;
                    free(msg.httpCreateParam.clientCert);
                }
                if(msg.httpCreateParam.clientPk!= NULL)
                {
                    UINT32 clientPkLen = strlen((CHAR*)msg.httpCreateParam.clientPk)/2;
                    UINT8* realClientPk = malloc(clientPkLen/2+1);
                    //getStrFromHex(realClientPk, (CHAR*)clientPk, strlen((CHAR*)clientPk)/2);
                    cmsHexStrToHex(realClientPk, clientPkLen, (CHAR *)msg.httpCreateParam.clientPk, clientPkLen*2);
                    httpContext->clientContext->clientPkLen = clientPkLen;
                    httpContext->clientContext->clientPk = (CHAR*)msg.httpCreateParam.clientPk;
                    free(msg.httpCreateParam.clientPk);
                }
                httpCreateClientData(httpContext);          
                
                httpContext->reqhandle = msg.reqhandle;
                httpContext->status = HTTPSTAT_CREATED;
                primSize=sizeof(cnfMsg);
                cnfMsg.clinetId = msg.clinetId;
                applSendCmsCnf(msg.reqhandle, APPL_RET_SUCC, APPL_HTTP, APPL_HTTP_CREATE_CNF, primSize, &cnfMsg);
                break;
            }
            
            case HTTP_CONNECT_COMMAND:
            {
                httpMutexAcquire();
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_3, P_INFO, 0, "recv connect command");
                httpContext->status = HTTPSTAT_CONNECTING;
               
                ret= httpAtConnect(httpContext);
                if(ret == TRUE)
                {
                    httpContext->status = HTTPSTAT_CONNECTED;
                    if(httpRecvTaskRunning == FALSE)
                    {
                        httpRecvTaskRunning = TRUE;
                        httpRecvTaskInit();
                    }

                    primSize=sizeof(cnfMsg);
                    applSendCmsCnf(msg.reqhandle, APPL_RET_SUCC, APPL_HTTP, APPL_HTTP_CONNECT_CNF, primSize, &cnfMsg);
                }
                else
                {
                    httpContext->status = HTTPSTAT_CREATED;
                    primSize=sizeof(cnfMsg);
                    applSendCmsCnf(msg.reqhandle, APPL_RET_FAIL, APPL_HTTP, APPL_HTTP_CONNECT_CNF, primSize, &cnfMsg);
                }
                httpMutexRelease();
                break;
            }
                
            case HTTP_SEND_COMMAND:
            {
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_4, P_INFO, 0, "recv send command");
                if(httpContext->clientData->isRange){
                    INT8 readPro = httpReadProgress();
                    if(readPro == 0){//read progress success
                        if(gRetentionProgress.rangeHead == 0 || (gCurPath && strcmp(gCurPath, gRetentionProgress.path)!=0)){
                            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_4_1, P_INFO, 0, "from head or diff path");
                            httpContext->clientData->rangeHead = 0;
                            httpContext->clientData->rangeTail = -1;
                        }else if(gCurPath && strcmp(gCurPath, gRetentionProgress.path)==0){
                            httpContext->clientData->rangeHead = gRetentionProgress.rangeHead;
                            httpContext->clientData->rangeTail = -1;
                            ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_4_2, P_INFO, 1, "same path at head:%d",httpContext->clientData->rangeHead);
                        }else{
                            ECOMM_STRING(UNILOG_ATCMD_HTTP, httpClientSendTask_4_3, P_INFO, "why here gRetentionProgress.path:%s", (uint8_t*)(gRetentionProgress.path));
                            httpContext->clientData->rangeHead = 0;
                            httpContext->clientData->rangeTail = -1;
                        }
                    }else{//no save progress or read progress fail, range start at 0
                        httpContext->clientData->rangeHead = 0;
                        httpContext->clientData->rangeTail = -1;
                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_4_4, P_INFO, 0,"no save progress from head");
                    }
                }
                result = httpSendRequest(httpContext->clientContext, httpContext->url,(HTTP_METH)httpContext->method, httpContext->clientData);
                if(result == HTTP_OK)
                {
                    primSize=sizeof(cnfMsg);
                    applSendCmsCnf(msg.reqhandle, APPL_RET_SUCC, APPL_HTTP, APPL_HTTP_SEND_CNF, primSize, &cnfMsg);
                    //httpContext->status = HTTPSTAT_RECVDATA;
                    httpContext->isReceiving=TRUE;
                }
                else
                {
                    //atcReply(msg.reqhandle, AT_RC_HTTP_ERROR, ( HTTP_SEND_FAILED), NULL);
                    primSize=sizeof(cnfMsg);
                    applSendCmsCnf(msg.reqhandle, APPL_RET_FAIL, APPL_HTTP, APPL_HTTP_SEND_CNF, primSize, &cnfMsg);
                }
                if(httpContext->clientContext->custHeader)
                {
                    free(httpContext->clientContext->custHeader);
                    httpContext->clientContext->custHeader=NULL;
                }
                if(httpContext->clientData->postBuf)
                {
                    free(httpContext->clientData->postBuf);
                    httpContext->clientData->postBuf=NULL;
                }
                if(httpContext->clientData->postContentType)
                {
                    free(httpContext->clientData->postContentType);
                    httpContext->clientData->postContentType=NULL;
                }                
                break;
            }
         
            case HTTP_CLOSE_TCP_COMMAND:
            {
                httpMutexAcquire();
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_5, P_WARNING, 0, "recv close command");
                httpAtCmd* atCmd = (httpAtCmd*)msg.httpcmd;
                int rspCode = atCmd->clientContext->httpResponseCode;

                httpContext->isReceiving=FALSE;
                if(httpContext->status != HTTPSTAT_CLOSED)
                {   
                    httpErrInd(msg.clinetId, msg.result, msg.reqhandle, rspCode);
                    //peer addr close the socket
                    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_6, P_WARNING, 0, "socketclose by peer addr or network reason");
                    httpContext->status = HTTPSTAT_CLOSED;
                    httpClose(httpContext->clientContext);
                }

                httpMutexRelease();
                break;
            }
            
            case HTTP_DESTROY_COMMAND:
            {
                if(httpContext->status == HTTPSTAT_CONNECTED)
                {
                    needCloseSocket = TRUE;
                }
                httpContext->status = HTTPSTAT_DESTROYING;
                httpMutexAcquire();
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_7, P_INFO, 0, "recv destroy command");
                result = HTTP_OK;
                if(httpContext != NULL)
                {
                    httpContext->isReceiving = FALSE;
                    if(needCloseSocket)
                    {
                        result = httpClose(httpContext->clientContext);
                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_8, P_INFO, 1, "http close 0x%x",result);
                    }
                    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_9, P_INFO, 1, "destroy clinet %d",msg.clinetId);
                }
                
                httpMutexRelease();

                //wait for recv task really deleted
                if(httpRecvTaskRunning == TRUE)//no conenct recv task should delete itself
                {
                    while(waitloop)//max 3s
                    {
                        if(httpRecvTaskRunning == FALSE)//deteled,break
                        {
                            break;
                        }
                        vTaskDelay(1000);
                        waitloop--;
                    }
                    if(waitloop==0)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_10, P_ERROR, 0, " wait recv task deleted tiemout");
                    }
                }
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_11, P_ERROR, 1, " wait recv task deleted %d",httpRecvTaskRunning);

                if((result == HTTP_OK)||(result == HTTP_MBEDTLS_ERR))
                {
                    if(http_msgqueue != NULL)
                    {
                        osMessageQueueDelete(http_msgqueue);//delete the memory for queue
                        http_msgqueue = NULL;
                    }
                    
                    httpMutexDelete();
                    flowMutexDelete();
                    vote_ret = slpManPlatVoteEnableSleep(httpSlpHandler,SLP_SLP2_STATE);
                    OsaCheck(RET_TRUE == vote_ret,vote_ret,0,0);
                    vote_ret = slpManGivebackPlatVoteHandle(httpSlpHandler);
                    OsaCheck(RET_TRUE == vote_ret,vote_ret,0,0);
                    
                    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_12, P_INFO, 0, "no http client connecting httpClientSendTask task deleted");
                    
                    primSize = sizeof(cnfMsg);                    
                    httpContext->status = HTTPSTAT_DEFAULT;
                    httpFreeClient(msg.clinetId);
                    osThreadSetPriority(osThreadGetId(), osPriorityNormal2); //should be higher than cms task
                    ostaskENTER_CRITICAL();
                    httpSendTaskRunning = FALSE;
                    httpsend_task_handle = NULL;
                    ostaskEXIT_CRITICAL();
                    applSendCmsCnf(msg.reqhandle, APPL_RET_SUCC, APPL_HTTP, APPL_HTTP_DESTORY_CNF, primSize, &cnfMsg);
                    vTaskDelete(NULL);
                }
                else
                {
                    primSize=sizeof(cnfMsg);
                    applSendCmsCnf(msg.reqhandle, APPL_RET_FAIL, APPL_HTTP, APPL_HTTP_DESTORY_CNF, primSize, &cnfMsg);
                }
                break;
            }

            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpClientSendTask_13, P_ERROR, 0, " recv error cmd");
                break;
            }
        }
    }

}



