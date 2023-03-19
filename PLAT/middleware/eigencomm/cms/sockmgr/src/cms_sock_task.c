/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: cms_sock_task.c
 *
 *  Description: cms socket manager task
 *
 *  History: create by xwang
 *
 *  Notes:
 *
******************************************************************************/

#include "def.h"
#include "inet.h"
#include "sockets.h"
#include "lwip/api.h"
#include "lwip/priv/api_msg.h"
#include "cms_sock_mgr.h"
#include "task.h"
#include "ec_tcpip_api_entity.h"
#include "mw_aon_info.h"


/******************************************************************************
 *****************************************************************************
 * VARIABLES
 *****************************************************************************
******************************************************************************/


CmsSockMgrHandleDefine gCmsSockMgrStaticHandle[SOCK_SOURCE_MAX+1] =
{   //SOURCE  //reqProecss  //hibCheck //hib context store  //hib context recover  //tcp server process accept client request
    {SOCK_SOURCE_MINI, NULL, NULL, NULL, NULL, NULL},
    {0, NULL, NULL, NULL, NULL, NULL},
    {0, NULL, NULL, NULL, NULL, NULL},
    {SOCK_SOURCE_SDKAPI, tcpipApiProessReq, tcpipCheckHibMode, tcpipApiStoreConnHibContext, tcpipApiRecoverConnContext, NULL},
    {0, NULL, NULL, NULL, NULL, NULL},
    {SOCK_SOURCE_MAX, NULL, NULL, NULL, NULL, NULL},
};

CmsSockMgrDynamicHandleList *gCmsSockMgrDynamicHandleList = NULL;

TaskHandle_t gCmsSockTaskHandle = NULL;

INT32 gCmsSockServerFd = -1;

BOOL gCmsSockTaskEnableHib = TRUE;

BOOL bRecoverFromHib = FALSE;


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

/**
  \fn           CmsSockMgrHandleDefine *cmsSockMgrGetFreeDynamicHandleContext(void)

  \brief        get a free sock mgr handler context,and inset into global dynamic list
*/
CmsSockMgrHandleDefine *cmsSockMgrGetFreeDynamicHandleContext(void)
{
    CmsSockMgrDynamicHandleList *contextTmp;
    CmsSockMgrDynamicHandleList *contextNext;

    SYS_ARCH_DECL_PROTECT(lev);

    contextTmp = (CmsSockMgrDynamicHandleList *)malloc(sizeof(CmsSockMgrDynamicHandleList));

    if(contextTmp == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrGetFreeMgrContext_2, P_ERROR, 0, "CmsMgrGetFreeMgrContext malloc fail");
        return NULL;
    }
    else
    {
        //init the dynamic handler context
        memset(contextTmp, 0, sizeof(CmsSockMgrDynamicHandleList));

        SYS_ARCH_PROTECT(lev);
        contextNext = gCmsSockMgrDynamicHandleList;
        gCmsSockMgrDynamicHandleList = contextTmp;
        contextTmp->next = contextNext;
        SYS_ARCH_UNPROTECT(lev);

        return &contextTmp->handle;
    }

}

/**
  \fn           CCmsSockMgrDynamicHandleList* cmsSockMgrGetDynamicHandleContextBySource(UINT8 soure)

  \brief        get a the sock mgr handler context by the source from the dynamic list
*/
CmsSockMgrDynamicHandleList* cmsSockMgrGetDynamicHandleContextBySource(UINT8 soure)
{
    CmsSockMgrDynamicHandleList *contextTmp = NULL;

    SYS_ARCH_DECL_PROTECT(lev);

    SYS_ARCH_PROTECT(lev);
    for(contextTmp = gCmsSockMgrDynamicHandleList; contextTmp != NULL; contextTmp = contextTmp->next)
    {
        if(contextTmp->handle.source == soure)
        {
            break;
        }
    }
    SYS_ARCH_UNPROTECT(lev);

    return contextTmp;

}

/**
  \fn           void cmsSockMgrRemoveDynamicHandleContextBySource(UINT8 soure)

  \brief        remove a the sock mgr handler context by the source from the dynamic list
*/
void cmsSockMgrRemoveDynamicHandleContextBySource(UINT8 soure)
{

    CmsSockMgrDynamicHandleList *contextTmp = NULL;
    CmsSockMgrDynamicHandleList *contextPre = NULL;

    SYS_ARCH_DECL_PROTECT(lev);

    SYS_ARCH_PROTECT(lev);
    for(contextTmp = gCmsSockMgrDynamicHandleList; contextTmp != NULL; contextTmp = contextTmp->next)
    {
        if(contextTmp->handle.source == soure)
        {
            break;
        }
        else
        {
            contextPre = contextTmp;
        }
    }

    if(contextTmp != NULL)
    {
        if(contextPre == NULL)
        {
            gCmsSockMgrDynamicHandleList = contextTmp->next;
        }
        else
        {
            contextPre->next = contextTmp->next;
        }
        free(contextTmp);
    }
    SYS_ARCH_UNPROTECT(lev);

    return;

}

/**
  \fn           CmsSockMgrHandleDefine *cmsSockMgrGetHandleDefineBySource(UINT8 source)

  \brief        get athe sock mgr handler context by the source from the dynamic list and the static table
*/
CmsSockMgrHandleDefine *cmsSockMgrGetHandleDefineBySource(UINT8 source)
{
    UINT8 sourceTmp;
    CmsSockMgrDynamicHandleList* dynamicHandleList = NULL;

    //fisrt get from static handle define
    for(sourceTmp = SOCK_SOURCE_MINI ; sourceTmp < SOCK_SOURCE_MAX + 1; sourceTmp ++)
    {
        if(gCmsSockMgrStaticHandle[sourceTmp].source == source)
        {
            return &gCmsSockMgrStaticHandle[sourceTmp];
        }
    }

    //get the dynamic handle define
    dynamicHandleList = cmsSockMgrGetDynamicHandleContextBySource(source);
    if(dynamicHandleList != NULL)
    {
        return &dynamicHandleList->handle;
    }

    return NULL;
}

/**
  \fn           BOOL cmsSockMgrRegisterHandleDefine(CmsSockMgrHandleDefine *handleDefine)

  \brief        register sock mgr handler into dynamic liste
*/
BOOL cmsSockMgrRegisterHandleDefine(CmsSockMgrHandleDefine *handleDefine)
{
    CmsSockMgrHandleDefine *handleDefineTmp = NULL;

    if(handleDefine == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRegisterHandleDefine_1, P_ERROR, 0, "cmsSockMgrRegisterHandleDefine invalid handlefine");
        return FALSE;
    }

    handleDefineTmp = cmsSockMgrGetHandleDefineBySource(handleDefine->source);
    if(handleDefineTmp != NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRegisterHandleDefine_2, P_WARNING, 1, "cmsSockMgrRegisterHandleDefine source %d has exist", handleDefine->source);
        return FALSE;
    }

    handleDefineTmp = cmsSockMgrGetFreeDynamicHandleContext();
    if(handleDefineTmp == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRegisterHandleDefine_3, P_ERROR, 1, "cmsSockMgrRegisterHandleDefine source %d get free handledefine context fail", handleDefine->source);
        return FALSE;
    }

    //register the handler function
    handleDefineTmp->source = handleDefine->source;
    handleDefineTmp->hibCheck = handleDefine->hibCheck;
    handleDefineTmp->recoverHibContext = handleDefine->recoverHibContext;
    handleDefineTmp->reqProcess = handleDefine->reqProcess;
    handleDefineTmp->storeHibContext = handleDefine->storeHibContext;
    handleDefineTmp->tcpServerProcessAcceptClient = handleDefine->tcpServerProcessAcceptClient;

    return TRUE;

}

/**
  \fn           void cmsSockMgrUnregisterHandleDefine(UINT8 source)

  \brief        unregister sock mgr handler by the source from dynamic liste
*/
void cmsSockMgrUnregisterHandleDefine(UINT8 source)
{
    cmsSockMgrRemoveDynamicHandleContextBySource(source);

    return;
}

/**
  \fn           static BOOL cmsSockMgrCommonCheckHib(CmsSockMgrContext* context)

  \brief        check the socket whether enter hib/sleep2 mode by the common condition
*/
static BOOL cmsSockMgrCommonCheckHib(CmsSockMgrContext* context)
{

    BOOL result = TRUE;

    if(context == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCommonCheckHib_1, P_ERROR, 0, "cmsSockMgrCommonCheckHib invalid context");
        return result;
    }

    if(context->hibEnable == FALSE)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCommonCheckHib_2, P_INFO, 1, "cmsSockMgrCommonCheckHib sockId %d hib mode disable", context->sockId);
        if(cmsSockMgrEnableHibMode(context) == TRUE)
        {
            context->hibEnable = TRUE;
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCommonCheckHib_4, P_INFO, 1, "cmsSockMgrCommonCheckHib sockId %d change to hib/sleep2 mode success", context->sockId);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCommonCheckHib_5, P_INFO, 1, "cmsSockMgrCommonCheckHib sockId %d change to hib/sleep2 mode fail", context->sockId);
            result = FALSE;
        }
    }

    if(context->status != SOCK_CONN_STATUS_CONNECTED && context->status != SOCK_CONN_STATUS_CREATED)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCommonCheckHib_3, P_INFO, 2, "cmsSockMgrCommonCheckHib sockId %d status %d is not valid", context->sockId, context->status);
        result = FALSE;
    }

    return result;
}

/**
  \fn           static BOOL cmsSockMgrPriCheckHib(CmsSockMgrContext* context)

  \brief        check the socket whether enter hib/sleep2 mode by the private condition
*/
static BOOL cmsSockMgrPriCheckHib(CmsSockMgrContext* context)
{
    CmsSockMgrHandleDefine *handleDefine = NULL;

    if(context == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrPubCheckHib_1, P_ERROR, 0, "cmsSockMgrPubCheckHib invalid context");
        return FALSE;
    }

    //check source
    if(!CMSSOCKMGR_CHECK_SOURCE(context->source))
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrPubCheckHib_2, P_ERROR, 0, "cmsSockMgrPubCheckHib source is invalid");
        return TRUE;
    }

    handleDefine = cmsSockMgrGetHandleDefineBySource(context->source);

    if(handleDefine != NULL)
    {
    //call hib private check function
        if(handleDefine->hibCheck)
        {
            return handleDefine->hibCheck(context);
        }
    }
    else
    {
       ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrPubCheckHib_3, P_ERROR, 1, "cmsSockMgrPubCheckHib can not find handledefine, maybe not register %d", context->source);
    }

    return TRUE;
}

/**
  \fn           static BOOL cmsSockMgrTaskIsEnterHib(void)

  \brief        check the sock mgr task whether enter hib/sleep2 mode
*/
static BOOL cmsSockMgrTaskIsEnterHib(void)
{
    CmsSockMgrContext* gMgrContext;

    for(gMgrContext = cmsSockMgrGetContextList(); gMgrContext != NULL; gMgrContext = gMgrContext->next)
    {
        //common check
        if(cmsSockMgrCommonCheckHib(gMgrContext) == FALSE)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrTaskIsEnterHib_1, P_SIG, 0, "cmsSockMgrTaskIsEnterHib common check FALSE");
            return FALSE;
        }

        //private check
        if(cmsSockMgrPriCheckHib(gMgrContext) == FALSE)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrTaskIsEnterHib_2, P_SIG, 0, "cmsSockMgrTaskIsEnterHib private check FALSE");
            return FALSE;
        }
    }

    return TRUE;
}

/**
  \fn           static void cmsSockMgrProcessRequest(CmsSockMgrRequest *request, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 fd)
  \brief        process the request
*/
static void cmsSockMgrProcessRequest(CmsSockMgrRequest *request, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 fd)
{
    CmsSockMgrHandleDefine *handleDefine = NULL;

    //check parameter
    if(request == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_1, P_ERROR, 0, "cmsSockMgrProcessRequest invalid request");
        return;
    }

    //check source
    if(!CMSSOCKMGR_CHECK_SOURCE(request->source))
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_2, P_ERROR, 1, "cmsSockMgrProcessRequest invalid request source %d", request->source);
        return;
    }

    //get the handler context by the source
    handleDefine = cmsSockMgrGetHandleDefineBySource(request->source);
    if(handleDefine == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_3, P_ERROR, 1, "cmsSockMgrProcessRequest can not find handledefine, maybe not register %d", request->source);
        return ;
    }

    //process sync request
    if(request->magic == CMS_SOCK_MGR_ASYNC_REQUEST_MAGIC)
    {
        // call request process function
        if(handleDefine->reqProcess)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_4, P_INFO, 1, "cmsSockMgrProcessRequest process sync request %d", request->reqId);
            handleDefine->reqProcess(request, sourceAddr, sourcePort, fd);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_5, P_ERROR, 0, "cmsSockMgrProcessRequest invalid request process function");
        }
    }
    else if(request->magic == CMS_SOCK_MGR_SYNC_REQUEST_MAGIC)
    {
        //sync reqeust need check whether the request have timeout
        if(cmsSockMgrGetCurrHibTicks() > request->reqTicks + request->timeout)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_6, P_ERROR, 0, "cmsSockMgrProcessRequest sync request has timeout");
            free(request->reqBody);
            return;
        }

        //call the request process function
        if(handleDefine->reqProcess)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_7, P_INFO, 1, "cmsSockMgrProcessRequest process async request %d", request->reqId);
            handleDefine->reqProcess(request, sourceAddr, sourcePort, fd);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_8, P_ERROR, 0, "cmsSockMgrProcessRequest invalid request process function");
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessRequest_9, P_ERROR, 0, "cmsSockMgrProcessRequest invalid request magic");
        return;
    }

    if(request->reqBody)
    {
        free(request->reqBody);
    }
    return;
}

/**
  \fn           static void cmsSockMgrProcessUlStatusRequest(CmsSockMgrUlStatusRequest *request)
  \brief        process the UL status indication request
*/
static void cmsSockMgrProcessUlStatusRequest(CmsSockMgrUlStatusRequest *request)
{
    UINT8 sequence;
    CmsSockMgrContext* gMgrContext;
    CmsSockMgrUlStatusInd *ulStatusInd = NULL;

    //check parameter
    if(request == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessUlStatusRequest_1, P_ERROR, 0, "cmsSockMgrProcessUlStatusRequest invalid request");
        return;
    }

    //check source & magic
    if(request->requestSource != CMS_SOCK_MGR_UL_STATE_INDICATION_REQ_SOURCE_ID || request->magic != CMS_SOCK_MGR_UL_STATE_INDICATION_REQ_MAGIC)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessUlStatusRequest_2, P_ERROR, 0, "cmsSockMgrProcessUlStatusRequest invalid source or magic");
        return;
    }

    ulStatusInd = (CmsSockMgrUlStatusInd *)&request->requestBody;
    GosCheck(ulStatusInd != PNULL, ulStatusInd, 0, 0);

    //get the socket mgr context by the socket id
    gMgrContext = cmsSockMgrFindMgrContextById(ulStatusInd->sockId);

    if(gMgrContext)
    {
        for(sequence = CMS_SOCK_MGR_UL_DATA_SEQUENCE_MIN; sequence <= CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX; sequence ++)
        {
            //check the UL sequence status indication bitmap
            if(cmsSockMgrGetUlPendingSequenceState(ulStatusInd->sequence_state_bitmap, sequence) == TRUE)
            {
                if(ulStatusInd->ulStatus == SOCK_MGR_UL_SEQUENCE_STATE_SENT || ulStatusInd->ulStatus == SOCK_MGR_UL_SEQUENCE_STATE_DISCARD)
                {
                    //call the ul sequence status event callback
                    cmsSockMgrCallUlStatusEventCallback(gMgrContext, sequence, ulStatusInd->ulStatus);
                }
            }

            if(sequence == CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX)
            {
                break;
            }
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessUlStatusRequest_5, P_ERROR, 0, "cmsSockMgrProcessUlStatusRequest invalid mgr context");
    }

    return;
}

/**
  \fn           void cmsSockMgrCallErrorEventCallback(CmsSockMgrContext* gMgrContext, INT32 errorCode)
  \brief        call the error event callback function
*/
void cmsSockMgrCallErrorEventCallback(CmsSockMgrContext* gMgrContext, INT32 errorCode)
{
    CmsSockMgrConnErrArg errArg;
    CmsSockMgrEventCallback eventCallback;

    if(gMgrContext == NULL)
    {
        return;
    }

    eventCallback = (CmsSockMgrEventCallback)gMgrContext->eventCallback;

    if(eventCallback)
    {
        //init the error event argument
        errArg.errCode = errorCode;
        eventCallback(gMgrContext, SOCK_EVENT_CONN_ERROR, &errArg);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallErrorEventCallback_1, P_INFO, 1, "cmsSockMgrCallErrorEventCallback %d", errorCode);
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallErrorEventCallback_2, P_WARNING, 2, "cmsSockMgrCallErrorEventCallback %d fd, but eventcallback is invalid", errorCode, gMgrContext->sockId);
    }

    return;
}

/**
  \fn           void cmsSockMgrCallStatusEventCallback(CmsSockMgrContext* gMgrContext, UINT8 oldStatus, UINT8 newStatus, UINT16 cause)
  \brief        call the socket status change event callback function
*/
void cmsSockMgrCallStatusEventCallback(CmsSockMgrContext* gMgrContext, UINT8 oldStatus, UINT8 newStatus, UINT16 cause)
{
    CmsSockMgrConnStatusArg statusArg;
    CmsSockMgrEventCallback eventCallback;

    if(gMgrContext == NULL)
    {
        return;
    }

    eventCallback = (CmsSockMgrEventCallback)gMgrContext->eventCallback;

    if(eventCallback)
    {
        //init the status evnet argument
        statusArg.oldStatus = oldStatus;
        statusArg.newStatus = newStatus;
        statusArg.cause = cause;
        eventCallback(gMgrContext, SOCK_EVENT_CONN_STATUS, &statusArg);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallStatusEventCallback_1, P_INFO, 1, "cmsSockMgrCallStatusEventCallback cause %d", cause);
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallStatusEventCallback_2, P_WARNING, 1, "cmsSockMgrCallStatusEventCallback %d fd, but eventcallback is invalid", gMgrContext->sockId);
    }
}

/**
  \fn           void cmsSockMgrCallDlEventCallback(CmsSockMgrContext* gMgrContext,CmsSockMgrDataContext *dataContext, ip_addr_t *remoteAddr, UINT16 remotePort)
  \brief        call the socket DL event callback function
*/
void cmsSockMgrCallDlEventCallback(CmsSockMgrContext* gMgrContext,CmsSockMgrDataContext *dataContext, ip_addr_t *remoteAddr, UINT16 remotePort)
{
    CmsSockMgrConnDlArg dlArg;
    CmsSockMgrEventCallback eventCallback;

    if(gMgrContext == NULL)
    {
        return;
    }

    eventCallback = (CmsSockMgrEventCallback)gMgrContext->eventCallback;

    if(eventCallback)
    {
        //init the DL event argument
        dlArg.dataContext = dataContext;
        dlArg.fromPort = remotePort;
        ip_addr_copy(dlArg.fromAddr, *remoteAddr);
        eventCallback(gMgrContext, SOCK_EVENT_CONN_DL, &dlArg);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallDlEventCallback_1, P_INFO, 2, "cmsSockMgrCallDlEventCallback dl %d, fd", dataContext->Length, gMgrContext->sockId);
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallStatusEventCallback_2, P_WARNING, 1, "cmsSockMgrCallStatusEventCallback %d fd, but eventcallback is invalid", gMgrContext->sockId);
    }


}

/**
  \fn           void cmsSockMgrCallUlStatusEventCallback(CmsSockMgrContext* gMgrContext, UINT8 sequence, UINT8 status)
  \brief        call the ULsequence status event callback function
*/
void cmsSockMgrCallUlStatusEventCallback(CmsSockMgrContext* gMgrContext, UINT8 sequence, UINT8 status)
{
    CmsSockMgrConnUlStatusArg ulStatusArg;
    CmsSockMgrEventCallback eventCallback;

    if(gMgrContext == NULL)
    {
        return;
    }

    eventCallback = (CmsSockMgrEventCallback)gMgrContext->eventCallback;

    if(eventCallback)
    {
        //init the UL sequence status vent argument
        ulStatusArg.status = status;
        ulStatusArg.sequence = sequence;
        eventCallback(gMgrContext, SOCK_EVENT_CONN_UL_STATUS, &ulStatusArg);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallUlStatusEventCallback_1, P_INFO, 3, "the sockeId %d ul sequence %u, status %u status event callback compelete ", gMgrContext->sockId, sequence, status);
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallUlStatusEventCallback_2, P_WARNING, 1, "the sockeId %d event callback function is invalid", gMgrContext->sockId);
    }
}

void cmsSockMgrCallTcpServerAcceptClientEventCallback(CmsSockMgrContext* gMgrContext, INT32 clientSocketId, ip_addr_t *clientAddr, UINT16 clientPort)
{
    CmsSockMgrConnAcceptClientArg acceptClientArg;
    CmsSockMgrEventCallback eventCallback;

    if(gMgrContext == NULL)
    {
        return;
    }

    eventCallback = (CmsSockMgrEventCallback)gMgrContext->eventCallback;

    if(eventCallback)
    {
        //init the UL sequence status vent argument
        acceptClientArg.serverSocketId = gMgrContext->sockId;
        acceptClientArg.clientSocketId = clientSocketId;
        acceptClientArg.clientPort = clientPort;
        ip_addr_copy(acceptClientArg.clientAddr, *clientAddr);
        eventCallback(gMgrContext, SOCK_EVENT_CONN_TCP_ACCEPT_CLIENT, &acceptClientArg);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallTcpServerAcceptClientEventCallback_1, P_INFO, 2, "the sockeId %d accept client socketId %d event callback compelete ", gMgrContext->sockId, clientSocketId);
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCallTcpServerAcceptClientEventCallback_2, P_WARNING, 1, "the sockeId %d event callback function is invalid", gMgrContext->sockId);
    }
}


void cmsSockMgrProcessTcpServerAcceptClientRequest(CmsSockMgrContext* gMgrContext, INT32 clientSocketId, ip_addr_t *clientAddr, UINT16 clientPort)
{
    CmsSockMgrHandleDefine *handleDefine;

        //check source
    if(!CMSSOCKMGR_CHECK_SOURCE(gMgrContext->source))
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessTcpServerAcceptClientRequest_check_resource, P_ERROR, 1, "cmsSockMgrProcessTcpServerAcceptClientRequest invalid source %d", gMgrContext->source);
        return;
    }

        //get the handler context by the source
    handleDefine = cmsSockMgrGetHandleDefineBySource(gMgrContext->source);

    if(handleDefine == PNULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessTcpServerAcceptClientRequest_1, P_ERROR, 1, "cmsSockMgrProcessTcpServerAcceptClientRequest can not find handledefine, maybe not register %d", gMgrContext->source);
        cmsSockMgrCloseSocket(clientSocketId);
        return ;
    }

    if(handleDefine->tcpServerProcessAcceptClient != PNULL)
    {
        if(handleDefine->tcpServerProcessAcceptClient(gMgrContext, clientSocketId, clientAddr, clientPort) == TRUE)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessTcpServerAcceptClientRequest_2, P_SIG, 1, "cmsSockMgrProcessTcpServerAcceptClientRequest accept the client %d", clientSocketId);
            cmsSockMgrCallTcpServerAcceptClientEventCallback(gMgrContext, clientSocketId, clientAddr, clientPort);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessTcpServerAcceptClientRequest_3, P_SIG, 1, "cmsSockMgrProcessTcpServerAcceptClientRequest did not accept the client %d", clientSocketId);
            cmsSockMgrCloseSocket(clientSocketId);
        }
    }
    else
    {
        cmsSockMgrCloseSocket(clientSocketId);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrProcessTcpServerAcceptClientRequest_4, P_WARNING, 1, "cmsSockMgrProcessTcpServerAcceptClientRequest can not find tcperveracceptclient process, maybe not register %d", gMgrContext->source);
    }
}

/**
  \fn           static void cmsSockMgrLoop(void *arg)
  \brief        the sock mgr main task function
*/
static void cmsSockMgrLoop(void *arg)
{
    INT32 result;
    fd_set readfds;
    fd_set writefds;
    fd_set errorfds;
    INT32 maxfd;
    CmsSockMgrDataContext* dataBuf = NULL;
    UINT32 dataBufLen = CMS_SOCK_MGR_DL_LENGTH_MAX;
    CmsSockMgrContext* gMgrContext;
    struct timeval tv;
    tv.tv_sec  = 3;
    tv.tv_usec = 0;
    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 cmsSockMgrPmuHdr = 0;
    UINT32 currentHibTicks;
    int mErr;
    BOOL anyConnectingConnection = FALSE;
    BOOL *bWakeup;

    bWakeup = (BOOL *)arg;


    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_1, P_INFO, 1, "cmsSockMgrLoop create!, wakeup flag %u", *bWakeup);

    /*
     * start atskt,first disable enter HIB mode
    */
    pmuRet = slpManApplyPlatVoteHandle("cmssockmgr", &cmsSockMgrPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);
    if(gCmsSockTaskEnableHib == TRUE)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_2, P_INFO, 0, "cmsSockMgr task disable enter hib/sleep2 mode");

        pmuRet = slpManPlatVoteDisableSleep(cmsSockMgrPmuHdr, SLP_SLP2_STATE);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        gCmsSockTaskEnableHib = FALSE;
    }

    //malloc DL buffer
    dataBuf = (CmsSockMgrDataContext *)malloc(dataBufLen + sizeof(CmsSockMgrDataContext));

    if(dataBuf == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_3, P_WARNING, 0, "exit cmsSockMgr, because malloc rcv buffer fail");
        gCmsSockTaskHandle = NULL;
        vTaskDelete(NULL);
    }

    while(1) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&errorfds);

        anyConnectingConnection = FALSE;

        //init select fdset
        maxfd = -1;
        for(gMgrContext = cmsSockMgrGetContextList(); gMgrContext != NULL; gMgrContext = gMgrContext->next){
            if(gMgrContext->status == SOCK_CONN_STATUS_CONNECTED) {
                if(maxfd < gMgrContext->sockId) {
                    maxfd = gMgrContext->sockId;
                }
                FD_SET(gMgrContext->sockId, &readfds);
                FD_SET(gMgrContext->sockId, &errorfds);
            }
            else if(gMgrContext->status == SOCK_CONN_STATUS_CONNECTING)
            {
                if(maxfd < gMgrContext->sockId) {
                    maxfd = gMgrContext->sockId;
                }
                FD_SET(gMgrContext->sockId, &writefds);
                FD_SET(gMgrContext->sockId, &errorfds);
                anyConnectingConnection = TRUE;
            }
        }

        if(gCmsSockServerFd >= 0)
        {
            FD_SET(gCmsSockServerFd, &readfds);
            FD_SET(gCmsSockServerFd, &errorfds);
            if(maxfd < gCmsSockServerFd) {
                maxfd = gCmsSockServerFd;
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_4, P_WARNING, 1, "cmsSockMgrLoop gCmsSockServerFd is invalid!");
            break;
        }

        //start select with timeout
        if(anyConnectingConnection == TRUE)
        {
            result = select(maxfd + 1, &readfds, &writefds, &errorfds, &tv);
        }
        else
        {
            if(*bWakeup == FALSE)
            {
                result = select(maxfd + 1, &readfds, &writefds, &errorfds, 0);
            }
            else
            {
                tv.tv_sec  = CMS_SOCK_MGR_WAKEUP_SELECT_TIMEOUT;
                tv.tv_usec = 0;
                result = select(maxfd + 1, &readfds, &writefds, &errorfds, &tv);
                *bWakeup = FALSE;
            }
        }

        if(result > 0) {
            for(gMgrContext = cmsSockMgrGetContextList(); gMgrContext != NULL; gMgrContext = gMgrContext->next)
            {
                //check error
                if(FD_ISSET(gMgrContext->sockId, &errorfds))
                {
                    mErr = sock_get_errno(gMgrContext->sockId);
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_5, P_WARNING, 2, "selcet>0 error happen %d, fd %u", mErr, gMgrContext->sockId);
                    if(gMgrContext->status == SOCK_CONN_STATUS_CONNECTED)
                    {
                        if(mErr != 0)
                        {
                            //call conn error event callback
                            cmsSockMgrCallErrorEventCallback(gMgrContext, mErr);
                        }
                    }
                    else if(gMgrContext->status == SOCK_CONN_STATUS_CONNECTING)
                    {
                        gMgrContext->status = SOCK_CONN_STATUS_CREATED;
                        //call conn status change event callback
                        cmsSockMgrCallStatusEventCallback(gMgrContext, SOCK_CONN_STATUS_CONNECTING, SOCK_CONN_STATUS_CREATED, CME_SOCK_MGR_ERROR_SOCK_ERROR);
                        if(mErr != 0)
                        {
                            //call conn error event callback
                            cmsSockMgrCallErrorEventCallback(gMgrContext, mErr);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_24, P_WARNING, 1, "selcet>0 error %d fd, but status is invalid", gMgrContext->sockId);
                    }
                    break;
                }

                //check connect success
                if(FD_ISSET(gMgrContext->sockId, &writefds))
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_8, P_INFO, 1, "selcet>0 sth to write at sockId:%d", gMgrContext->sockId);
                    if(gMgrContext->status == SOCK_CONN_STATUS_CONNECTING)
                    {
                        gMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
                        //call conn status change event callback
                        cmsSockMgrCallStatusEventCallback(gMgrContext, SOCK_CONN_STATUS_CONNECTING, SOCK_CONN_STATUS_CONNECTED, CME_SUCC);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_11, P_WARNING, 0, "fd %d can write now ,but invalid status", gMgrContext->sockId);
                    }
                }

                //check rcv
                if(FD_ISSET(gMgrContext->sockId, &readfds))
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_12, P_INFO, 1, "selcet>0 sth to read at sockId:%d", gMgrContext->sockId);

                    struct sockaddr_storage from;
                    int fromlen = sizeof(struct sockaddr_storage);
                    ip_addr_t remoteAddr;
                    UINT16 remotePort;

                    memset(&from, 0, sizeof(struct sockaddr_storage));

                    if(gMgrContext->bServer == TRUE)
                    {
                        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_tcp_server, P_SIG, 1, "sockId:%d, tcp server", gMgrContext->sockId);
                        if(gMgrContext->type == SOCK_CONN_TYPE_TCP)
                        {
                            INT32 clientSocketId = -1;
                            clientSocketId = accept(gMgrContext->sockId, (struct sockaddr*)&from, (socklen_t*)&fromlen);
                            if(clientSocketId < 0)
                            {
                                mErr = sock_get_errno(gMgrContext->sockId);

                                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsSockMgrLoop_tcp_accept_1, P_WARNING, 2, "tcp server socket id %u accept client error %u", gMgrContext->sockId, mErr);
                            }
                            else
                            {
                                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsSockMgrLoop_tcp_accept_2, P_SIG, 2, "tcp server socket id %u accept client success socket id %u", gMgrContext->sockId, clientSocketId);
                                sockaddr_to_ipaddr_port((const struct sockaddr*)&from, &remoteAddr, &remotePort);
                                cmsSockMgrProcessTcpServerAcceptClientRequest(gMgrContext, clientSocketId, &remoteAddr, remotePort);
                                break;
                            }
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsSockMgrLoop_tcp_accept_3, P_WARNING, 1, "tcp server socket id %u is not tcp", gMgrContext->sockId);
                        }
                    }
                    else
                    {

                        memset(dataBuf, 0, dataBufLen + sizeof(CmsSockMgrDataContext));

                        result = recvfrom(gMgrContext->sockId, dataBuf->data, dataBufLen, MSG_DONTWAIT, (struct sockaddr*)&from, (socklen_t*)&fromlen);

                        if (result <= 0)
                        {
                            mErr = sock_get_errno(gMgrContext->sockId);
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsSockMgrLoop_13, P_WARNING, 2, "recv result=%d, errno=%d", result, mErr);
                            if(mErr != 0)
                            {
                                //call conn error event callback
                                cmsSockMgrCallErrorEventCallback(gMgrContext, mErr);
                                break;
                            }
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_16, P_WARNING, 1, "recv bytes len=%d",result);

                            sockaddr_to_ipaddr_port((const struct sockaddr*)&from, &remoteAddr, &remotePort);

                            //call conn dl event callback
                            dataBuf->Length = result;
                            dataBuf->type = SOCK_MGR_DATA_TYPE_RAW;
                            cmsSockMgrCallDlEventCallback(gMgrContext, dataBuf, &remoteAddr, remotePort);

                        }
                    }
                }
            }

            // read the sock mgr server socket
            if(FD_ISSET(gCmsSockServerFd, &readfds))
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_18, P_INFO, 0, "selcet>0 sth to read sockhandler fd");
                CmsSockMgrRequest *cmsSockMgrReq;
                CmsSockMgrUlStatusRequest *cmsSockMgrUlstatusReq;
                struct sockaddr_storage from;
                int fromlen = sizeof(struct sockaddr_storage);
                ip_addr_t remoteAddr;
                UINT16 remotePort;

                memset(&from, 0, sizeof(struct sockaddr_storage));
                memset(dataBuf, 0, dataBufLen + sizeof(CmsSockMgrDataContext));

                result = recvfrom(gCmsSockServerFd, dataBuf->data, dataBufLen, MSG_DONTWAIT, (struct sockaddr*)&from, (socklen_t*)&fromlen);
                if(result <= 0)
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_19, P_WARNING, 0, "read atSktReqFd fail ");
                }
                else
                {
                    sockaddr_to_ipaddr_port((const struct sockaddr*)&from, &remoteAddr, &remotePort);

                    //process the reqeust
                    if(result == sizeof(CmsSockMgrRequest))
                    {
                        cmsSockMgrReq = (CmsSockMgrRequest *)dataBuf->data;
                        cmsSockMgrProcessRequest(cmsSockMgrReq, &remoteAddr, remotePort, gCmsSockServerFd);
                    }
                    else if(result == sizeof(CmsSockMgrUlStatusRequest))
                    {
                        cmsSockMgrUlstatusReq = (CmsSockMgrUlStatusRequest *)dataBuf->data;
                        cmsSockMgrProcessUlStatusRequest(cmsSockMgrUlstatusReq);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_20, P_WARNING, 1, "read atSktReqFd fail, check rcv len %u fail", result);
                    }
                }
            }
        }
        else if(result == 0)
        {
            //check the connection whether has connect timeout
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_21, P_INFO, 0, "select timeout ");
            currentHibTicks = cmsSockMgrGetCurrHibTicks();
            for(gMgrContext = cmsSockMgrGetContextList(); gMgrContext != NULL; gMgrContext = gMgrContext->next)
            {
                if(gMgrContext->status == SOCK_CONN_STATUS_CONNECTING)
                {
                    if(currentHibTicks >= (gMgrContext->connectStartTicks + CMS_SOCK_MGR_CONNECT_TIMEOUT))
                    {
                        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_22, P_WARNING, 1, "fd %d connect timeout ", gMgrContext->sockId);
                        gMgrContext->status = SOCK_CONN_STATUS_CREATED;
                        //call conn status change event callback
                        cmsSockMgrCallStatusEventCallback(gMgrContext, SOCK_CONN_STATUS_CONNECTING, SOCK_CONN_STATUS_CREATED, CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT);
                        //call conn error event callback
                        cmsSockMgrCallErrorEventCallback(gMgrContext, mErr);
                        break;
                    }
                }
            }
        }


    // check wether need continue select

        if(cmsSockMgrGetContextList() == NULL)
        {
            break;
        }

        //check wether can enter hib/sleep2 mode
        if(cmsSockMgrTaskIsEnterHib() == TRUE)
        {
            if(gCmsSockTaskEnableHib == FALSE)
            {
                //enable enter hib/sleep2 mode
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_25, P_INFO, 0, "cmsSockMgr task enable enter hib/sleep2 mode");

                pmuRet = slpManPlatVoteEnableSleep(cmsSockMgrPmuHdr, SLP_SLP2_STATE);
                OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

                gCmsSockTaskEnableHib = TRUE;
            }
        }
        else
        {
            if(gCmsSockTaskEnableHib == TRUE)
            {
                //disable enter hib/sleep2 mode
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_26, P_INFO, 0, "cmsSockMgr task disable enter hib/sleep2 mode");

                pmuRet = slpManPlatVoteDisableSleep(cmsSockMgrPmuHdr, SLP_SLP2_STATE);
                OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

                gCmsSockTaskEnableHib = FALSE;
            }
        }

    }
    free(dataBuf);
    if(gCmsSockServerFd >= 0)
    {
        close(gCmsSockServerFd);
        gCmsSockServerFd = -1;
    }
    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_27, P_INFO, 0, "exit cmsSockMgr task");
    gCmsSockTaskHandle = NULL;

    //before atskt task exit, enable enter hib/sleep2  mode
    if(gCmsSockTaskEnableHib == FALSE)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrLoop_28, P_INFO, 0, "cmsSockMgr task enable enter hib/sleep2 mode");

        pmuRet = slpManPlatVoteEnableSleep(cmsSockMgrPmuHdr, SLP_SLP2_STATE);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        gCmsSockTaskEnableHib = TRUE;
    }
    pmuRet = slpManGivebackPlatVoteHandle(cmsSockMgrPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    vTaskDelete(NULL);
}

/**
  \fn           static INT32 cmsSockMgrSockServerInit(void)
  \brief        setup the sock mgr server socket
*/
static INT32 cmsSockMgrSockServerInit(void)
{
    INT32 socketHandleFd;
    struct sockaddr_in addr;
    int seqHanbler = 1;

    //create loopback udp socket with expect socket id first
    socketHandleFd = socket_expect(AF_INET, SOCK_DGRAM, IPPROTO_UDP, CMSSOCKMGR_EXPEXT_SERVER_SOCKET_ID);

    if(socketHandleFd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrSockServerInit_expect_socket, P_WARNING, 1, "cmsSockMgrSockServerInit create loopback udp socket with expect id %u fail", CMSSOCKMGR_EXPEXT_SERVER_SOCKET_ID);

        socketHandleFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if(socketHandleFd < 0)
        {

            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrSockServerInit_1, P_ERROR, 0, "cmsSockMgrSockServerInit create loopback udp socket fail");
            return SOCK_MGR_ACTION_FAIL;
        }
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CMS_SOCK_MGR_SERVER_PORT);
    addr.sin_addr.s_addr = htonl(IPADDR_LOOPBACK);

    //bind local port and loopback address
    if(bind(socketHandleFd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, cmsSockMgrSockServerInit_2, P_ERROR, 0, "cmsSockMgrSockServerInit bind loopback udp server socket fail");
        close(socketHandleFd);
        return -1;
    }

    //enable UL sequence inication server socket
    if(lwip_setsockopt(socketHandleFd, SOL_SOCKET, SO_SEQUENCE_HANDLER, &seqHanbler, sizeof(seqHanbler)) < 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, cmsSockMgrSockServerInit_3, P_WARNING, 0, "cmsSockMgrSockServerInit enable ul seqstatus handler fail");
    }

    return socketHandleFd;
}

/**
  \fn           BOOL cmsSockMgrHandleInit(BOOL bWakeup)
  \brief        setup sock mgr main task
*/
BOOL cmsSockMgrHandleInit(BOOL bWakeup)
{


    if(gCmsSockServerFd == -1)
    {
        gCmsSockServerFd = cmsSockMgrSockServerInit();
        if(gCmsSockServerFd < 0)
        {
           return FALSE;
        }
    }

    if(gCmsSockTaskHandle == NULL)
    {
        if(bWakeup == TRUE)
        {
            bRecoverFromHib = TRUE;
            xTaskCreate(cmsSockMgrLoop,
                "cms_sock_mgr",
                1024 * 2 / sizeof(portSTACK_TYPE),
                &bRecoverFromHib,
                osPriorityNormal,
                &gCmsSockTaskHandle);
        }
        else
        {
            bRecoverFromHib =FALSE;
            xTaskCreate(cmsSockMgrLoop,
                "cms_sock_mgr",
                1024 * 2 / sizeof(portSTACK_TYPE),
                &bRecoverFromHib,
                osPriorityNormal,
                &gCmsSockTaskHandle);
        }
    }

    return TRUE;
}

/**
  \fn           void cmsSockMgrEnterHibCallback(void *data, LpState state)
  \brief        cms sock mgr callback function when UE enter hib/sleep2 mode
*/
void cmsSockMgrEnterHibCallback(void *data, LpState state)
{
    MidWareSockAonInfo socHibContext;
    CmsSockMgrHandleDefine *handleDefine = NULL;

    CmsSockMgrContext *udpContext = NULL;
    CmsSockMgrContext *tcpContext = NULL;

    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_1, P_INFO, 0, "cmsSockMgrEnterHibCallback enter");

    memset(&socHibContext, 0, sizeof(MidWareSockAonInfo));
    socHibContext.sockContext.tcpContext.sockId = -1;
    socHibContext.sockContext.udpContext.sockId = -1;

    //get udp hib context
    udpContext = cmsSockMgrGetHibContext(SOCK_CONN_TYPE_UDP);
    tcpContext = cmsSockMgrGetHibContext(SOCK_CONN_TYPE_TCP);
    if(udpContext || tcpContext)
    {
        socHibContext.sockContext.magic = CMS_SOCK_MGR_CONTEXT_MAGIC;

        if(udpContext)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_2, P_INFO, 0, "cmsSockMgrEnterHibCallback store udp context");
            socHibContext.sockContext.flag |= 1 << SOCK_CONN_TYPE_UDP;
            handleDefine = cmsSockMgrGetHandleDefineBySource(udpContext->source);
            if(handleDefine == NULL)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_3, P_ERROR, 1, "cmsSockMgrEnterHibCallback can not find handledefine, maybe not register %d", udpContext->source);
            }
            else
            {
                if(handleDefine->storeHibContext)
                {
                    handleDefine->storeHibContext(udpContext, &socHibContext.sockContext.udpContext);
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_4, P_INFO, 0, "cmsSockMgrEnterHibCallback store udp context compelete");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_5, P_WARNING, 1, "cmsSockMgrEnterHibCallback sockId %d no storeHibContext function",udpContext->sockId);
                }
            }

        }

        if(tcpContext)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_6, P_INFO, 0, "cmsSockMgrEnterHibCallback store tcp context");
            socHibContext.sockContext.flag |= 1 << SOCK_CONN_TYPE_TCP;
            handleDefine = cmsSockMgrGetHandleDefineBySource(tcpContext->source);
            if(handleDefine == NULL)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_7, P_ERROR, 1, "cmsSockMgrEnterHibCallback can not find handledefine, maybe not register %d", tcpContext->source);
            }
            else
            {
                if(handleDefine->storeHibContext)
                {
                    handleDefine->storeHibContext(tcpContext, &socHibContext.sockContext.tcpContext);
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_8, P_INFO, 0, "cmsSockMgrEnterHibCallback store tcp context complete");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnterHibCallback_9, P_WARNING, 1, "cmsSockMgrEnterHibCallback sockId %d no storeHibContext function",tcpContext->sockId);
                }
            }

        }
    }

    mwSetSockAonInfo(&socHibContext);

}

/**
  \fn           void cmsSockMgrRecoverHibCallback(CmsSockMgrHibContext *hibContext)
  \brief        cms sock mgr callback function when UE recover from hib/sleep2 mode
*/
void cmsSockMgrRecoverHibCallback(CmsSockMgrHibContext *hibContext)
{
    CmsSockMgrConnHibContext *connHibContext = NULL;
    CmsSockMgrHandleDefine *handleDefine = NULL;

    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_1, P_INFO, 0, "cmsSockMgrRecoverHibCallback enter");

    if(hibContext == NULL || hibContext->magic != CMS_SOCK_MGR_CONTEXT_MAGIC)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_2, P_INFO, 0, "cmsSockMgrRecoverHibCallback invalid hib context, can not recover");
        return;
    }

    if(hibContext->flag == 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_3, P_INFO, 0, "cmsSockMgrRecoverHibCallback no hibcontext need recover");
        return;
    }

    if(hibContext->flag & (1 << SOCK_CONN_TYPE_UDP))
    {
        connHibContext = &hibContext->udpContext;
        if(!CMSSOCKMGR_CHECK_SOURCE(connHibContext->source) || connHibContext->magic != CMS_SOCK_MGR_UDP_CONTEXT_MAGIC)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_4, P_WARNING, 3, "cmsSockMgrRecoverHibCallback udp sockId %d or source %d or magic %d is invalid", connHibContext->sockId, connHibContext->source, connHibContext->magic);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_5, P_INFO, 1, "cmsSockMgrRecoverHibCallback recover udp context, sockId %d", connHibContext->sockId);
            handleDefine = cmsSockMgrGetHandleDefineBySource(connHibContext->source);
            if(handleDefine == NULL)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_6, P_ERROR, 1, "cmsSockMgrRecoverHibCallback can not find handledefine, maybe not register %d", connHibContext->source);
            }
            else
            {
                if(handleDefine->recoverHibContext)
                {
                    handleDefine->recoverHibContext(connHibContext);
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_7, P_INFO, 1, "cmsSockMgrRecoverHibCallback recover udp context, sockId %d compelete", connHibContext->sockId);
                }
            }
        }
    }

    if(hibContext->flag & (1 << SOCK_CONN_TYPE_TCP))
    {
        connHibContext = &hibContext->tcpContext;
        if(!CMSSOCKMGR_CHECK_SOURCE(connHibContext->source) || connHibContext->magic != CMS_SOCK_MGR_TCP_CONTEXT_MAGIC)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_8, P_WARNING, 3, "cmsSockMgrRecoverHibCallback tcp sockId %d or source %d or magic %d is invalid", connHibContext->sockId, connHibContext->source, connHibContext->magic);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_9, P_INFO, 1, "cmsSockMgrRecoverHibCallback recover tcp context, sockId %d", connHibContext->sockId);
             handleDefine = cmsSockMgrGetHandleDefineBySource(connHibContext->source);
            if(handleDefine == NULL)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_10, P_ERROR, 1, "cmsSockMgrRecoverHibCallback can not find handledefine, maybe not register %d", connHibContext->source);
            }
            else
            {
                if(handleDefine->recoverHibContext)
                {
                    handleDefine->recoverHibContext(connHibContext);
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_11, P_INFO, 1, "cmsSockMgrRecoverHibCallback recover tcp context, sockId %d compelete", connHibContext->sockId);
                }
            }
        }
    }

    if(cmsSockMgrHandleInit(TRUE) == TRUE)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_12, P_INFO, 0, "cmsSockMgrRecoverHibCallback INIT CMS SOCK MGR success");
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRecoverHibCallback_13, P_ERROR, 0, "cmsSockMgrRecoverHibCallback INIT CMS SOCK MGR fail");
    }
}


