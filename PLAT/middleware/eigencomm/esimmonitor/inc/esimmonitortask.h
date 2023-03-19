/******************************************************************************
Copyright:      - 2020, All rights reserved by Eigencomm Ltd.
File name:      - esimMonitorTask.h
Description:    - task process the SIG with CMS
Function List:  -
History:        - 04/12/2020, Originated by yjwang
******************************************************************************/

#ifndef  _ESIM_PROC_TASK_H
#define  _ESIM_PROC_TASK_H

#ifdef SOFTSIM_FEATURE_ENABLE
#include <stdint.h>
#include "cms_api.h"
#include "commonTypedef.h"



#define ESIM_TASK_QUEUE_SIZE                       8

typedef enum ESIMPROCTASK_SIG_ID_Tag
{
    SIG_ESIM_SIGNAL_BASE =      0x0930,

    /*CMS  <--->ESIM TASK*/
    SIG_ESIM_SIGNAL_SEND_REQ,   /*CMS ->ESIM TASK  ->CMS */
    SIG_ESIM_SIGNAL_SEND_CNF,   /*ESIM TASK  ->CMS */

    SIG_ESIM_SIGNAL_RECV_IND,    /*OTHERS APP  ->ESIM TASK */
    SIG_ESIM_SIGNAL_RECV_IND_RSP,    /*ESIM TASK  ->OTHERS APP */

    SIG_ESIM_SIGNAL_RECV_END  = 0x093F,
}ESIMPROCTASK_SIG_ID;


typedef enum ESIMSIG_MSG_ITEM_ID_Tag
{
    ESIM_MSG_SEND_DATA_BASE = 0,
    ESIM_MSG_SEND_DATA_REQ,
    ESIM_MSG_SEND_DATA_RSP,
    ESIM_MSG_SEND_EVENT_IND,
    ESIM_MSG_SEND_EVENT_RSP
}ESIMSIG_MSG_ITEM_ID;

typedef enum ESIMSIG_EVENT_IND_ITEM_ID_Tag
{
    ESIM_MSG_IND_BASE = 0,

    ESIM_MSG_IND_FINISHED,
    ESIM_MSG_IND_ERROR,

    ESIM_MSG_IND_MAX = 255
}ESIMSIG_EVENT_IND_ITEM_ID;


typedef enum ESIM_AT_RESULT_E
{
    ESIM_RET_RESULT_OK   = 0,
    ESIM_RET_RESULT_FAIL = 1
}ESIM_AT_RESULT;

typedef enum ESIM_TASK_STATE_E
{
    ESIM_TASK_STATE_INIT = 0,
    ESIM_TASK_STATE_IDLE,
    ESIM_TASK_STATE_WORKING,
    ESIM_TASK_STATE_FINISHED
}ESIM_TASK_STATE;


/*
 *MSGID: ESIM_CSM_SEND_DATA_REQ
*/
typedef struct esimSendDataReq_Tag
{
    UINT16 reqId;
    UINT16 reserved;
    UINT8  reqBody[];
}esimSendDataReq;

/*
 *MSGTYPE: ESIM_CSM_SEND_DATA_REQ
*/
typedef struct  eSimCmsAtDataReqMsg_Tag
{
    UINT8     type;
    UINT16    ReqLen;
    UINT32    reqhandle;
    void      *ReqData;
} eSimCmsAtDataReqMsg;

/*
 *MSGTYPE: ESIM_CSM_SEND_DATA_RSP
*/
typedef struct  eSimCmsAtDataCnfMsg_Tag
{
    UINT8     errcode;
    UINT8     type;
    UINT16    respLen;
    UINT32    respHandle;
    void      *respData;
} eSimCmsAtDataCnfMsg;


/*
 *MSGID: SIG_ESIM_SIGNAL_SEND_CNF
*/
typedef struct esimSendDataCnf_Tag
{
    UINT16 reqId;
    UINT16 reserved;
    UINT8  reqBody[];
}esimSendDataCnf;


/*
 *MSGID: SIG_ESIM_SIGNAL_RECV_IND
*/
typedef struct esimRecvIndReq_Tag
{
    UINT16 reqId;
    UINT16 reserved;
    UINT8  reqBody[];
}esimRecvIndReq;

/*
 *MSGID: SIG_ESIM_SIGNAL_RECV_IND_RSP
*/
typedef struct esimRecvIndResp_Tag
{
    UINT16 reqId;
    UINT16 reserved;
    UINT8  reqBody[];
}esimRecvIndResp;


/*
 *MSGTYPE: ESIM_CSM_SEND_EVENT_IND
*/
typedef struct  eSimCmsAtIndReqMsg_Tag
{
    UINT8     event;
} eSimCmsAtIndReqMsg;

/*
 *MSGTYPE: ESIM_CSM_SEND_DATA_RSP
*/
typedef struct  eSimCmsAtIndRespMsg_Tag
{
    UINT8     event;
    UINT8     result;
} eSimCmsAtIndRespMsg;


typedef struct eSimProcSendOutMsg_Tag
{
    UINT8                  init;
    UINT8                  state;
}eSimProcTaskMsg;


extern void     eSimCreateProcTaskInQ(void);
extern void     eSimCreateProcTask(void);
extern UINT8    eSimProcInit(void);
extern UINT8    eSimProcGetWorkFlag(void);
extern UINT8    eSimProcSetWorkFlag(ESIM_TASK_STATE          state);
extern CmsRetId        eSimSendAtDataReq(UINT32 atHandle,UINT8 type, UINT8 *sendData, UINT16 sendLen);
extern eSimProcTaskMsg *eSimTaskGetInstance(void);
extern CmsRetId         eSimSendIndSigReq(osMessageQueueId_t mq_id,UINT32 atHandle,UINT8 event);
extern  CmsRetId  esimProcessRespMsg(UINT16 indHandle,void *pHexData);
#endif

#endif
