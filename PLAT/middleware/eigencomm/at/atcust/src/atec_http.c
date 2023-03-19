/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_http.c
*
*  Description: Process http(s) client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "osasys.h"
#include "at_util.h"
#include "cmsis_os2.h"
#include "task.h"
#include "atec_http.h"
#include "atec_http_cnf_ind.h"
#include "lwip/sockets.h"
#include "netmgr.h"
#include "ps_lib_api.h"
#include "at_util.h"
#include "at_http_task.h"

extern BOOL httpRecvTaskRunning;
extern BOOL httpSendTaskRunning;

extern osMessageQueueId_t http_msgqueue;
static INT32 currentLenLast = 0;
UINT8 *serverCertTotal = NULL;

#define _DEFINE_AT_REQ_FUNCTION_LIST_

char *serverCertTemp = NULL;

char *gCurPath = NULL;

/**
  \fn          CmsRetId httpCREATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  httpCREATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 flag=0;
    UINT8* url = NULL;
    UINT8* authUser = NULL;
    UINT8* authPassword = NULL;
    INT32 totalServerCertLen = 0;
    INT32 currentServerCertLen = 0;
    UINT8* serverCert = NULL;
    INT32 clientCertLen = 0;
    UINT8* clientCert = NULL;
    INT32 clientPkLen=0;
    UINT8* clientPk = NULL;
    URLError validUrl = URL_NETWORK;
    httpAtCmd* httpContext = NULL;
    UINT16 length = 0;
    httpCmdMsg httpMsg;
    CHAR certSubBuf[128] = {0};
    int count = 0;
    int subSeqCount = 0;
    char *caKeyTemp = NULL;
    
    BOOL validParam = FALSE;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+HTTPCREATE=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+HTTPCREATE: (0,1),\"<host>\",\"<authuser>\",\"<authpasswd>\"");
            break;
        }

        case AT_SET_REQ:      /* AT+HTTPCREATE= */
        {
            do{
                if((atGetNumValue(pAtCmdReq->pParamList, 0, &flag, HTTPCREATE_0_FLAG_MIN, HTTPCREATE_0_FLAG_MAX, HTTPCREATE_0_FLAG_DEF)) == AT_PARA_ERR){
                    break;
                }
                url = malloc(HTTPCREATE_1_URL_STR_MAX_LEN+1);
                memset(url, 0, (HTTPCREATE_1_URL_STR_MAX_LEN+1));
                if((atGetStrValue(pAtCmdReq->pParamList, 1, url, HTTPCREATE_1_URL_STR_MAX_LEN+1, &length , (CHAR *)HTTPCREATE_1_URL_STR_DEF)) == AT_PARA_ERR){
                    break;
                }
                if(!pParamList[2].bDefault){
                    atGetStrLength(pAtCmdReq->pParamList, 2, &length);
                    authUser = (UINT8*)malloc(length+1);
                    memset(authUser, 0, (length+1));
                    if((atGetStrValue(pAtCmdReq->pParamList, 2, authUser, HTTPCREATE_2_AUTHUSER_STR_MAX_LEN+1, &length , (CHAR *)HTTPCREATE_2_AUTHUSER_STR_DEF)) == AT_PARA_ERR){
                        break;
                    }
                }
                if(!pParamList[3].bDefault){
                    atGetStrLength(pAtCmdReq->pParamList, 3, &length);
                    authPassword = (UINT8*)malloc(length+1);
                    memset(authPassword, 0, (length+1));
                    if((atGetStrValue(pAtCmdReq->pParamList, 3, authPassword, HTTPCREATE_3_AUTHPASSWD_STR_MAX_LEN+1, &length , (CHAR *)HTTPCREATE_3_AUTHPASSWD_STR_DEF)) == AT_PARA_ERR){
                        break;
                    }
                }
                if((atGetNumValue(pAtCmdReq->pParamList, 4, &totalServerCertLen, HTTPCREATE_4_TOTALSERCERLEN_MIN, HTTPCREATE_4_TOTALSERCERLEN_MAX, HTTPCREATE_4_TOTALSERCERLEN_DEF)) == AT_PARA_ERR){
                    break;
                }
                if((atGetNumValue(pAtCmdReq->pParamList, 5, &currentServerCertLen, HTTPCREATE_5_CUEESERCERLEN_MIN, HTTPCREATE_5_CUEESERCERLEN_MAX, HTTPCREATE_5_CUEESERCERLEN_DEF)) == AT_PARA_ERR){
                    break;
                }
                if(totalServerCertLen < currentServerCertLen){
                    break;
                }
                atGetStrLength(pAtCmdReq->pParamList, 6, &length);
                if(length != currentServerCertLen){
                    break;
                }
                if(!pParamList[6].bDefault){
                    serverCert = malloc(currentServerCertLen+1);
                    memset(serverCert, 0, (currentServerCertLen+1));
                    if((atGetStrValue(pAtCmdReq->pParamList, 6, serverCert, currentServerCertLen+1, &length , (CHAR *)HTTPCREATE_6_SERCERT_STR_DEF)) == AT_PARA_ERR){
                        break;
                    }
                }
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpCREATE_1, P_INFO, 2, "totalServerCertLen=%d, currentServerCertLen=%d",totalServerCertLen,currentServerCertLen);
                if((atGetNumValue(pAtCmdReq->pParamList, 7, &clientCertLen, HTTPCREATE_7_CLIENTCERTLEN_MIN, HTTPCREATE_7_CLIENTCERTLEN_MAX, HTTPCREATE_7_CLIENTCERTLEN_DEF)) == AT_PARA_ERR){
                    break;
                }
                if(!pParamList[8].bDefault){
                    atGetStrLength(pAtCmdReq->pParamList, 8, &length);
                    if(length != clientCertLen){
                        break;
                    }else{
                        clientCert = (UINT8*)malloc(clientCertLen+1);
                        if((atGetStrValue(pAtCmdReq->pParamList, 8, clientCert, clientCertLen+1, &length , (CHAR *)HTTPCREATE_8_CLIENTCERT_STR_DEF)) == AT_PARA_ERR){
                            break;
                        }
                    }
                }
                if((atGetNumValue(pAtCmdReq->pParamList, 9, &clientPkLen, HTTPCREATE_9_CLIENTPKLEN_MIN, HTTPCREATE_9_CLIENTPKLEN_MAX, HTTPCREATE_9_CLIENTPKLEN_DEF)) == AT_PARA_ERR){
                    break;
                }
                if(!pParamList[10].bDefault){
                    atGetStrLength(pAtCmdReq->pParamList, 10, &length);
                    if(length != clientPkLen){
                        break;
                    }else{
                        clientCert = (UINT8*)malloc(clientPkLen+1);
                        if((atGetStrValue(pAtCmdReq->pParamList, 10, clientPk, clientPkLen+1, &length , (CHAR *)HTTPCREATE_10_CLIENTPK_STR_DEF)) == AT_PARA_ERR){
                            break;
                        }
                    }
                }
                ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpCREATE_2, P_INFO, 1, "length=%d validParam = TRUE",length);
                validParam = TRUE;
            }while(0);

            if(validParam == FALSE)
            {
                if(url != NULL)
                {
                    free(url);
                }
                if(authUser != NULL)
                {
                    free(authUser);
                }
                if(authPassword != NULL)
                {
                    free(authPassword);
                }
                if(clientCert != NULL)
                {
                    free(clientCert);
                }
                if(clientPk != NULL)
                {
                    free(clientPk);
                }         
                if(serverCertTotal != NULL)
                {
                    free(serverCertTotal);
                }         
                rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_PARAM_ERROR), NULL);
                break;
            }
            else
            {
                validUrl = httpAtIsUrlValid((char*)url);//judge url
                if(validUrl != URL_OK){
                    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpCREATE_3, P_INFO, 0, "URL not valid free all");
                    free(url);
                    if(authUser != NULL)
                    {
                        free(authUser);
                    }
                    if(authPassword != NULL)
                    {
                        free(authPassword);
                    }
                    if(clientCert != NULL)
                    {
                        free(clientCert);
                    }
                    if(clientPk != NULL)
                    {
                        free(clientPk);
                    }                       
                    if(serverCertTotal != NULL)
                    {
                        free(serverCertTotal);
                    }         
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_URL_FORMAT_ERROR), NULL);
                    break;
                }
                
                /*there is continue server cert*/
                if(flag == 1)
                {
                    if(serverCert != NULL)
                    {
                        if(serverCertTotal == NULL)
                        {
                            serverCertTotal = malloc(totalServerCertLen+1);
                            memset(serverCertTotal, 0, (totalServerCertLen+1));
                        } 
                        memcpy(&serverCertTotal[currentLenLast], serverCert, currentServerCertLen);  
                        currentLenLast = currentLenLast + currentServerCertLen;
                    }
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);//next to assemble serverCert                        
                }
                else
                {
                    httpContext = httpGetFreeClient();
                    if(NULL == httpContext)
                    {
                        if(url != NULL)
                        {
                            free(url);
                        }
                        if(authUser != NULL)
                        {
                            free(authUser);
                        }
                        if(authPassword != NULL)
                        {
                            free(authPassword);
                        }
                        if(clientCert != NULL)
                        {
                            free(clientCert);
                        }
                        if(clientPk != NULL)
                        {
                            free(clientPk);
                        }
                        free(serverCertTotal);
                        serverCertTotal = NULL;
                        currentLenLast = 0;
                        rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_CLIENTS_EXHAOUST), NULL);
                        break;
                    }  
                    if(serverCert != NULL)
                    {
                        if(serverCertTotal == NULL)//serverCert in one AT commmand no need assemble
                        {
                            serverCertTotal = malloc(totalServerCertLen+1);
                            memset(serverCertTotal, 0, (totalServerCertLen+1));
                        } 
                        memcpy(&serverCertTotal[currentLenLast], serverCert, currentServerCertLen);
                        
                        length = strlen((char *)serverCertTotal);
                        if(length >= HTTP_TLS_CA_SUB_SEQ_LEN)
                        {
                            count = (length/HTTP_TLS_CA_SUB_SEQ_LEN);
                        }
                        
                        serverCertTemp = malloc(length+count*6+80);
                        memset(serverCertTemp, 0, (length+count*6+80));
                        
                        sprintf((char*)serverCertTemp, "-----BEGIN CERTIFICATE-----\r\n");
                        
                        for(subSeqCount=0; subSeqCount<count; subSeqCount++)
                        {
                            caKeyTemp = (char *)serverCertTotal+HTTP_TLS_CA_SUB_SEQ_LEN*subSeqCount;
                        
                            snprintf((char*)certSubBuf, HTTP_TLS_CA_SUB_SEQ_LEN+1, "%s", caKeyTemp);
                            
                            strcat(serverCertTemp, certSubBuf);
                            memset(certSubBuf, 0, 128);
                            strcat(serverCertTemp, "\r\n");
                        }
                        if((length%HTTP_TLS_CA_SUB_SEQ_LEN) > 0)
                        {
                            memset(certSubBuf, 0, 128);
                            caKeyTemp = (char *)serverCertTotal+HTTP_TLS_CA_SUB_SEQ_LEN*subSeqCount;
                            length = (length%HTTP_TLS_CA_SUB_SEQ_LEN);
                            snprintf((char*)certSubBuf, length+1, "%s", caKeyTemp);
                            strcat(serverCertTemp, certSubBuf);
                            strcat(serverCertTemp, "\r\n");
                        }
                        memset(certSubBuf, 0, 128);
                        sprintf((char*)certSubBuf, "-----END CERTIFICATE-----");
                        strcat(serverCertTemp, certSubBuf);
                        currentLenLast = 0;
                        free(serverCertTotal);
                        serverCertTotal = NULL;
                    }
                    memset((char *)&httpMsg,0, sizeof(httpCmdMsg));
                    httpMsg.httpCreateParam.authUser = authUser;
                    httpMsg.httpCreateParam.authPassword = authPassword;
                    httpMsg.httpCreateParam.serverCert = (UINT8 *)serverCertTemp;
                    httpMsg.httpCreateParam.clientCert = clientCert;
                    httpMsg.httpCreateParam.clientPk = clientPk;
                    if(httpContext->host){
                        ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpCREATE_4, P_INFO, 0, "URL not valid free all");
                        free(httpContext->host);
                    }
                    httpContext->host = (CHAR*)url;
                    httpCreateReq(reqHandle, httpContext, httpMsg);
                    
                    rc = CMS_RET_SUCC;
                }
            }   
            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    if(serverCert != NULL)
    {
        free(serverCert);
    }

    return rc;
}

/**
  \fn          CmsRetId httpCONNECT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  httpCONNECT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 httpclientId;
    httpAtCmd* httpContext = NULL;
    NmAtiSyncRet netStatus;
    NmAtiGetNetInfoReq  getNetInfoReq;
    
    memset(&getNetInfoReq, 0, sizeof(NmAtiGetNetInfoReq));

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+HTTPCON=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+HTTPCON: (0)");
            break;
        }

        case AT_SET_REQ:      /* AT+HTTPCONNECT                                                                                                                                                                             = */
        {
            NetmgrAtiSyncReq(NM_ATI_SYNC_GET_NET_INFO_REQ, (void *)&getNetInfoReq, &netStatus);
            if(netStatus.body.netInfoRet.netifInfo.netStatus != NM_NETIF_ACTIVATED)
            {
                rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_NO_NETWORK), NULL);
                break;
            }
        
            if((rc = atGetNumValue(pAtCmdReq->pParamList, 0, &httpclientId, HTTPCONNECT_0_CLIENTID_MIN, HTTPCONNECT_0_CLIENTID_MAX, HTTPCONNECT_0_CLIENTID_DEF)) != AT_PARA_ERR)
            {
                httpContext = httpFindClient(httpclientId);
                if(httpContext == NULL)
                {
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_NO_FIND_CLIENT_ID), NULL);
                    break;
                }
                if(httpContext->status == HTTPSTAT_CONNECTED)
                {
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_ALREADY_CONNECTED), NULL);
                    break;
                }
                if((httpContext->status != HTTPSTAT_CREATED)&&(httpContext->status != HTTPSTAT_CLOSED))
                {
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_OPERATION_NOT_SUPPORT), NULL);
                    break;
                }

                rc=httpConnectReq(reqHandle,httpContext);
            }
            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId httpSEND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  httpSEND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 clientId;
    INT32 method;
    UINT8 path[HTTPSEND_3_PATH_STR_MAX_LEN+1]={0};
    INT32 pathLen = 0, isRange = 0;
    UINT8* customHeader = NULL;
    INT32 customHeaderLen = 0;
    UINT8 contentType[HTTPSEND_7_CONTENTTYPE_STR_MAX_LEN+1]={0};
    INT32 contentTypeLen = 0;
    UINT8* content = NULL;
    INT32 contentLen = 0;
    CHAR* url = NULL;
    BOOL validParam = FALSE;
    httpAtCmd* httpContext = NULL;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+HTTPSEND=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+HTTPSEND: (0),(0-4),(0-260),\"<path>\",(0-255),\"<customheader>\",(0-64),\"<contentType>\",(0-1024),\"<content>\"");
            break;
        }

        case AT_SET_REQ:      /* AT+HTTPSEND= */
        {                    
            do{
                if(atGetNumValue(pAtCmdReq->pParamList, 0, &clientId, HTTPSEND_0_CLIENTID_MIN, HTTPSEND_0_CLIENTID_MAX, HTTPSEND_0_CLIENTID_DEF) == AT_PARA_ERR){
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &method, HTTPSEND_1_METHOD_MIN, HTTPSEND_1_METHOD_MAX, HTTPSEND_1_METHOD_DEF) == AT_PARA_ERR){
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 2, &pathLen, HTTPSEND_2_PATHLEN_MIN, HTTPSEND_2_PATHLEN_MAX+1, HTTPSEND_2_PATHLEN_DEF) == AT_PARA_ERR){
                    break;
                }
                if(atGetStrValue(pAtCmdReq->pParamList, 3, path, HTTPSEND_3_PATH_STR_MAX_LEN, &length , (CHAR *)HTTPSEND_3_PATH_STR_DEF) == AT_PARA_ERR){
                    break;
                }
                if(pathLen != length){
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 4, &customHeaderLen, HTTPSEND_4_HEADERLEN_MIN, HTTPSEND_4_HEADERLEN_MAX+1, HTTPSEND_4_HEADERLEN_DEF) == AT_PARA_ERR){
                    break;
                }
                if(customHeaderLen > 0){
                    atGetStrLength(pAtCmdReq->pParamList, 5, &length);
                    if(customHeaderLen != length){
                        break;
                    }else{
                        customHeader = malloc(customHeaderLen+1);
                        if(atGetStrValue(pAtCmdReq->pParamList, 5, customHeader, customHeaderLen+1, &length , (CHAR *)HTTPSEND_5_HEADER_STR_DEF) == AT_PARA_ERR){
                            break;
                        }
                    }
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 6, &contentTypeLen, HTTPSEND_6_CONTENTTYPELEN_MIN, HTTPSEND_6_CONTENTTYPELEN_MAX+1, HTTPSEND_0_CLIENTID_DEF) == AT_PARA_ERR){
                    break;
                }
                if(atGetStrValue(pAtCmdReq->pParamList, 7, contentType, HTTPSEND_7_CONTENTTYPE_STR_MAX_LEN+1, &length , (CHAR *)HTTPSEND_7_CONTENTTYPE_STR_DEF) == AT_PARA_ERR){
                    break;
                }
                if(contentTypeLen != length){
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 8, &contentLen, HTTPSEND_8_CONTENTLEN_MIN, HTTPSEND_8_CONTENTLEN_MAX+1, HTTPSEND_8_CONTENTLEN_DEF) == AT_PARA_ERR){
                    break;
                }
                if(contentLen > 0)
                {
                    atGetStrLength(pAtCmdReq->pParamList, 9, &length);
                    if(contentLen != length){
                        break;
                    }else{
                        content = malloc(contentLen+1);
                        if(atGetStrValue(pAtCmdReq->pParamList, 9, content, contentLen+1, &length , (CHAR *)HTTPSEND_9_CONTENT_STR_DEF) == AT_PARA_ERR){
                            break;
                        }
                    }
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 10, &isRange, 0, 2, 0) == AT_PARA_ERR){
                    break;
                }
                validParam = TRUE;
            }while(0);

            if(validParam == TRUE)
            {
                if(isRange == 2){
                    httpClearProgress();
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    break;
                }
                httpContext = httpFindClient(clientId);
                if(!httpContext)
                {
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_NO_FIND_CLIENT_ID), NULL);
                    break;
                }
                if(httpContext->status != HTTPSTAT_CONNECTED)
                {
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_CLIENT_NOT_OPEN), NULL);
                    break;
                }

                if(httpContext->isReceiving)
                {
                    rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_SEND_FAILED), NULL);
                    break;
                }
                
                if(isRange == 0){
                    httpContext->clientData->isRange = FALSE;
                }else if(isRange == 1){
                    httpContext->clientData->isRange = TRUE;
                }
                ECOMM_TRACE(UNILOG_HTTP_CLIENT, httpSEND_1, P_INFO, 1, "isRange = %d",isRange);
                
                UINT8 hostLen = 0;
                hostLen = strlen(httpContext->host);
                pathLen = strlen((CHAR*)path);
                url = malloc(hostLen+pathLen+2);
                memset(url, 0, hostLen+pathLen+2);
                memcpy(url, httpContext->host, hostLen);
                if(path[0] != '/')
                {
                    for(int i=pathLen; i>0; i--)
                    {
                        path[i] = path[i-1];
                    }
                    path[0] = '/';
                }
                strcat(url,(CHAR*)path);

                httpContext->method = method;
                if(httpContext->url){
                    free(httpContext->url);
                }
                httpContext->url = url;
                
                if(gCurPath != NULL){
                    free(gCurPath);
                }
                gCurPath = malloc(strlen((CHAR*)url)+1);
                strcpy(gCurPath,(CHAR*)url);
                
                UINT8* strCustomHeadr = NULL;
                httpContext->clientContext->custHeader=NULL;
                if(customHeaderLen>0)
                {
                    strCustomHeadr = malloc(customHeaderLen/2+1);
                    memset(strCustomHeadr, 0, customHeaderLen/2+1);
                    //getStrFromHex(strCustomHeadr, (CHAR*)customHeader, customHeaderLen/2);
                    cmsHexStrToHex(strCustomHeadr, customHeaderLen/2, (CHAR *)customHeader, customHeaderLen);
                    httpContext->clientContext->custHeader = (CHAR*)strCustomHeadr;
                }
                
                UINT8* strContent = NULL;
                if(contentLen>0)
                {
                    strContent = malloc(contentLen/2+1);
                    memset(strContent, 0, contentLen/2+1);
                    //getStrFromHex(strContent, (CHAR*)content, contentLen/2);
                    cmsHexStrToHex(strContent, contentLen/2, (CHAR *)content, contentLen);
                }
                httpContext->clientData->isChunked = FALSE;
                httpContext->clientData->isMoreContent = FALSE;
                httpContext->clientData->needObtainLen = 0;
                httpContext->clientData->recvContentLength = 0;
                if(method == HTTP_POST || method == HTTP_PUT)
                {
                    httpContext->clientData->postBuf = (CHAR*)strContent;
                    httpContext->clientData->postBufLen = strlen((CHAR*)strContent);//contentLen/2 + 1;
                    if(contentTypeLen > 0)
                    {
                        contentTypeLen = strlen((CHAR*)contentType);
                        httpContext->clientData->postContentType = malloc(contentTypeLen+1);
                        memset(httpContext->clientData->postContentType, 0, (contentTypeLen+1));
                        memcpy(httpContext->clientData->postContentType, (CHAR*)contentType, contentTypeLen);
                    }
                    else
                        httpContext->clientData->postContentType = NULL;
                }
                else
                {
                    httpContext->clientData->postBuf = NULL;
                    httpContext->clientData->postBufLen = 0;
                    httpContext->clientData->postContentType = NULL;
                    if(strContent)
                    free(strContent);
                }

                rc=httpSendReq(reqHandle, httpContext);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_PARAM_ERROR), NULL);
            }
            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }
    if(customHeader)
    {
        free(customHeader);
    }
    if(content)
    {
        free(content);
    }
    #if 0
    if(url)
    {
        free(url);
    }
    #endif

    return rc;
}

/**
  \fn          CmsRetId httpDESTROY(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  httpDESTROY(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 httpclientId;
    httpAtCmd* httpContext = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+HTTPDESTROY =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+HTTPDESTROY: (0)");
            break;
        }

        case AT_SET_REQ:      /* AT+HTTPDESTROY */
        {
            if((rc = atGetNumValue(pAtCmdReq->pParamList, 0, &httpclientId, HTTPDESTROY_0_CLIENTID_MIN, HTTPDESTROY_0_CLIENTID_MAX, HTTPDESTROY_0_CLIENTID_DEF)) != AT_PARA_ERR)
            {
                httpContext = httpFindClient(httpclientId);

                if(httpContext != NULL)
                {
                    if((httpContext->status == HTTPSTAT_CLOSED)||(httpContext->status == HTTPSTAT_CONNECTED)||(httpContext->status == HTTPSTAT_CREATED))
                    {
                        rc=httpDestroyReq(reqHandle,httpContext,httpclientId);
                    }
                    else
                    {
                        rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_PARAM_ERROR), NULL);
                    }
                }
                else
                {
                    rc=atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_NO_FIND_CLIENT_ID), NULL);
                }
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, (HTTP_PARAM_ERROR), NULL);
            }

            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}


