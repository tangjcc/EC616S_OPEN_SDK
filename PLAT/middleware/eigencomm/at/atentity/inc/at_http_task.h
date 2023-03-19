/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_http_task.h
*
*  Description: Process http(s) client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _AT_HTTP_TASK_H_
#define _AT_HTTP_TASK_H_

#include "at_util.h"
#include "httpclient.h"

/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/


#define HTTPCLIENT_ATCMD_MAX_NUM   1
#define MAX_BASIC_AUTHUSER_SIZE 128
#define MAX_BASIC_AUTHPASSWD_SIZE 128
#define MAX_HEAD_BUFFER_SIZE 800
#define MAX_RSP_BUFFER_SIZE  1501
#define MAX_URC_FORMAT_BUFFER_SIZE  100
#define MAX_ATCMD_SIZE (MAX_RSP_BUFFER_SIZE*2+MAX_URC_FORMAT_BUFFER_SIZE)// should be at least 2*MAX_RSP_BUFFER_SIZE+1
#define MAX_CONTENT_TYPE_SIZE 64

#define HTTP_IS_NOT_USED   0
#define HTTP_IS_USED       1

#define HTTP_NVM_FILE_NAME      "httpProg.nvm"
#define HTTP_NVM_FILE_VERSION   0
#define HTTP_FILE_NUM           1

#define HTTP_DISP_SEQNUM  0

enum HTTP_MSG_CMD
{
    HTTP_CREATE_COMMAND,
    HTTP_CONNECT_COMMAND,
    HTTP_SEND_COMMAND,
    HTTP_CLOSE_TCP_COMMAND,
    HTTP_DESTROY_COMMAND
};


/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/

/*
 * APPL SGID: APPL_HTTP, related PRIM ID
*/
typedef enum applHttpPrimId_Enum
{
    APPL_HTTP_PRIM_ID_BASE = 0,
        
    APPL_HTTP_CREATE_CNF,
    APPL_HTTP_CONNECT_CNF,
    APPL_HTTP_SEND_CNF,
    APPL_HTTP_DESTORY_CNF,
    APPL_HTTP_IND,

    APPL_HTTP_PRIM_ID_END = 0xFF
}applHttpPrimId;


typedef enum {
    URL_OK = 0,         ///<Success
    URL_FORMAT = 3,     ///<url format error
    URL_NETWORK = 4,    ///<url Parse error
    URL_HTTPS = 14,
}URLError;

typedef enum {
    HTTPSTAT_DEFAULT = 0,
    HTTPSTAT_CREATED,
    HTTPSTAT_CLOSED,
    HTTPSTAT_CONNECTING,
    HTTPSTAT_CONNECTED,
    HTTPSTAT_RECVDATA,
    HTTPSTAT_DESTROYING,   
    HTTPSTAT_DESTROYED, 
}HTTPStatus;

/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/
typedef struct {
    BOOL isUsed;
    HTTPStatus status;
    BOOL isReceiving;// only support one receive ongoing for a connection
    UINT8 httpclientId;
    CHAR* host;
    UINT8 method;
    CHAR* url;
    HttpClientContext* clientContext;
    HttpClientData* clientData;
    UINT32 reqhandle;
} httpAtCmd;

// http message queue element typedef
typedef struct
{
    UINT8* authUser;
    UINT8* authPassword;
    INT32 serverCertLen;
    UINT8* serverCert;
    INT32 currentLenLast;
    INT32 clientCertLen;
    UINT8* clientCert;
    INT32 clientPkLen;
    UINT8* clientPk;    
} httpCreatePara;

typedef struct
{
    uint32_t cmd_type;
    uint32_t clinetId;
    //void * client_context;
    //void * client_data;
    void * httpcmd;
    uint32_t reqhandle;
    HTTPResult result;
    httpCreatePara  httpCreateParam;
} httpCmdMsg;

typedef struct
{
    uint32_t clinetId;
} httpCnfCmdMsg;

typedef struct
{
    char *pHttpInd;
} httpIndMsg;

typedef struct httpNvmHeader_Tag
{
    UINT16 fileBodySize; //file body size, not include size of header;
    UINT8  version;
    UINT8  checkSum;
}httpNvmHeader;

typedef struct httpProgress_Tag
{
    CHAR path[MAXPATH_SIZE];
    UINT32  rangeHead;
    UINT32  range;
    UINT16  seqnum;
}httpProgress;

CmsRetId httpCreateReq(UINT32 atHandle, httpAtCmd* httpCmd, httpCmdMsg httpMsg);
CmsRetId httpConnectReq(UINT32 atHandle,httpAtCmd* httpCmd );
CmsRetId httpSendReq(UINT32 atHandle,httpAtCmd* httpCmd);
CmsRetId httpDestroyReq(UINT32 atHandle, httpAtCmd* httpCmd,int httpclientId);

URLError httpAtIsUrlValid(char* url);
BOOL httpMutexCreate(void);
BOOL httpMutexAcquire(void);
void httpMutexRelease(void);
httpAtCmd* httpGetFreeClient(void);
void httpCreateClientData(httpAtCmd* httpCmd);
void httpReleaseClientData(httpAtCmd* httpCmd);
httpAtCmd* httpFindClient(int httpclientId);
void httpFreeClient(int httpclientId);
int httpGetMaxSocketId(void);
void httpRecvInd(httpAtCmd* httpCmd, UINT8 flag);
void httpErrInd(UINT8 httpClientId, HTTPResult errorCode, UINT32 reqhandle, UINT32 rspCode);
BOOL httpRecvTaskInit(void);
BOOL httpSendTaskInit(void);
//BOOL checkHttpRecvTaskDeleted();
void httpClientRecvTask(void* arg);
void httpClientSendTask(void* arg);

BOOL httpIsConnectExist(void);
void httpOutflowContrl(void);

void httpClearProgress(void);

#endif

