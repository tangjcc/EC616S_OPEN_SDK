/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: tcpip_lib_event.c
 *
 *  Description: tcpip SDK API related request process function
 *
 *  History:create by xwang
 *
 *  Notes:
 *
******************************************************************************/

#include "def.h"
#include "inet.h"
#include "sockets.h"
#include "lwip/api.h"
#include "lwip/priv/api_msg.h"
#include "ec_tcpip_api_entity.h"
#include "ec_tcpip_api.h"



/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

INT32 tcpipApiSendResponse(INT32 rcvRequestFd, CmsSockMgrResponse *response, UINT16 responseLen,ip_addr_t *remoteAddr, UINT16 remotePort)
{
    struct sockaddr_storage to;

    if(rcvRequestFd < 0 || response == NULL || remoteAddr == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendResponse_1, P_ERROR, 0, "TcpipApiSendResponse invalid parameter");
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    if (IP_IS_V4(remoteAddr))
    {
        struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
        to4->sin_len    = sizeof(struct sockaddr_in);
        to4->sin_family = AF_INET;
        to4->sin_port = htons(remotePort);
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(remoteAddr));
    }
    else if(IP_IS_V6(remoteAddr))
    {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6*)&to;
        to6->sin6_len    = sizeof(struct sockaddr_in6);
        to6->sin6_family = AF_INET6;
        to6->sin6_port = htons(remotePort);
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(remoteAddr));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendResponse_2, P_ERROR, 0, "TcpipApiCreateConnection atskt is full");
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    //send request mesage to socket handler socket
    if(sendto(rcvRequestFd, response, responseLen, 0, (struct sockaddr *)&to, sizeof(to)) == responseLen)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendResponse_3, P_INFO, 0, "send response to socket handler success");
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendResponse_4, P_ERROR, 0, "send response to socket handler fail");
        return CME_SOCK_MGR_ERROR_SEND_FAIL;
    }

    return CME_SUCC;
}

CmsSockMgrResponse *tcpipApiMakeResponse(UINT16 bodyLen, UINT8 source, UINT8 reqId)
{
    CmsSockMgrResponse *response = NULL;

    response = (CmsSockMgrResponse *)malloc(sizeof(CmsSockMgrResponse) + bodyLen);
    GosCheck(response != PNULL, response, 0, 0);
    if(response)
    {
        response->reqId = reqId;
        response->source = source;
        response->bodyLen = bodyLen;
    }

    return response;
}

void tcpipApiFreeResponse(CmsSockMgrResponse *response)
{
    if(response)
    {
        free(response);
    }

    return;
}


INT32 tcpipApiCreateConnection(TcpipApiCreateConnectionReq *tcpipApiCreateReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd, UINT32 reqTicks, UINT16 timeout)
{
    INT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    TcpipApiPriMgrContext *priMgrcontext;
    CmsSockMgrResponse *response;
    TcpipCreateConnResult *result;
    INT32 protocol;
    UINT16 responseLen = 0;
    UINT8 domain;

    //init response
    response = tcpipApiMakeResponse(sizeof(TcpipCreateConnResult), SOCK_SOURCE_SDKAPI, TCPIP_API_REQ_CREATE_CONNECTION);
    if(response == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_1, P_ERROR, 0, "TcpipApiCreateConnection invalid param");
        return CME_SOCK_MGR_ERROR_NO_MEMORY;
    }
    result = (TcpipCreateConnResult *)response->body;
    result->cause = CME_SUCC;
    result->connectionId = -1;
    responseLen = sizeof(CmsSockMgrResponse) + sizeof(TcpipCreateConnResult);

    if(tcpipApiCreateReq == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_2, P_ERROR, 0, "TcpipApiCreateConnection invalid param");
        ret = CME_SOCK_MGR_ERROR_UNKNOWN;
        goto fail;
    }

    //get free mgr context
    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(TcpipApiPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_3, P_ERROR, 0, "TcpipApiCreateConnection atskt is full");
        ret = CME_SOCK_MGR_ERROR_TOO_MUCH_INST;
        goto fail;
    }

    //anlaysis domain
    if(IP_IS_V6(&tcpipApiCreateReq->localAddr) || IP_IS_V6(&tcpipApiCreateReq->remoteAddr))
    {
        domain = AF_INET6;
    }
    else if(IP_IS_V4(&tcpipApiCreateReq->localAddr) || IP_IS_V4(&tcpipApiCreateReq->remoteAddr))
    {
        domain = AF_INET;
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_4, P_ERROR, 0, "TcpipApiCreateConnection invalid address info");
        ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        cmsSockMgrRemoveContext(sockMgrContext);
        goto fail;
    }

    if(tcpipApiCreateReq->type == SOCK_DGRAM_TYPE)
    {
        sockMgrContext->type = SOCK_CONN_TYPE_UDP;
        protocol = IPPROTO_UDP;
    }
    else if(tcpipApiCreateReq->type == SOCK_STREAM_TYPE)
    {
        sockMgrContext->type = SOCK_CONN_TYPE_TCP;
        protocol = IPPROTO_TCP;
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_5, P_ERROR, 1, "TcpipApiCreateConnection invalid type %d", tcpipApiCreateReq->type);
        ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        cmsSockMgrRemoveContext(sockMgrContext);
        goto fail;
    }

    //config cms mgr context
    priMgrcontext = (TcpipApiPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);
    sockMgrContext->domain = domain;
    sockMgrContext->source = SOCK_SOURCE_SDKAPI;
    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)tcpipApiCreateReq->eventCallback;
    priMgrcontext->tcpipApiConnEventCallback = tcpipApiCreateReq->tcpipApiConnEventCallback;
    priMgrcontext->creatTimeout = timeout;
    priMgrcontext->startCreateTicks = reqTicks;
    priMgrcontext->rcvRequestFd = rcvRequestFd;

    //create socket
    sockMgrContext->sockId = cmsSockMgrCreateSocket(domain, tcpipApiCreateReq->type, protocol, -1);
    if(sockMgrContext->sockId < 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_6, P_ERROR, 0, "TcpipApiCreateConnection create socket fail");
        cmsSockMgrRemoveContext(sockMgrContext);
        ret = CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
        goto fail;
    }

    //enable hib mode
    cmsSockMgrEnableHibMode(sockMgrContext);

    //bind socket
    ret = cmsSockMgrBindSocket(sockMgrContext->sockId, domain, tcpipApiCreateReq->localPort, &tcpipApiCreateReq->localAddr);
    if(ret != CME_SUCC)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_7, P_ERROR, 0, "TcpipApiCreateConnection create socket fail");
        cmsSockMgrRemoveContext(sockMgrContext);
        goto fail;
    }
    else
    {
        sockMgrContext->localPort = tcpipApiCreateReq->localPort;
        ip_addr_copy(sockMgrContext->localAddr, tcpipApiCreateReq->localAddr);
    }

    //connect socket
    ret = cmsSockMgrConnectSocket(sockMgrContext->sockId, domain, tcpipApiCreateReq->remotePort, &tcpipApiCreateReq->remoteAddr, FALSE);
    if(ret == CME_SUCC)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_8, P_INFO, 0, "TcpipApiCreateConnection connect success");
        sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
        result->connectionId = sockMgrContext->sockId;

        if(cmsSockMgrGetCurrHibTicks() < reqTicks + timeout)
        {
            tcpipApiSendResponse(rcvRequestFd, response, responseLen, sourceAddr, sourcePort);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_9, P_ERROR, 3, "TcpipApiCreateConnection request has timeout,can not send response, reqticks %u, timeout %u, now %u", reqTicks, timeout, cmsSockMgrGetCurrHibTicks());
            cmsSockMgrCloseSocket(sockMgrContext->sockId);
            cmsSockMgrRemoveContext(sockMgrContext);
        }
        tcpipApiFreeResponse(response);
        return ret;
    }
    else if(ret == CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING)
    {
        sockMgrContext->status = SOCK_CONN_STATUS_CONNECTING;
        sockMgrContext->connectStartTicks = cmsSockMgrGetCurrHibTicks();
        priMgrcontext->createSourcePort = sourcePort;
        ip_addr_copy(priMgrcontext->createSourceAddr, *sourceAddr);
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_10, P_INFO, 2, "TcpipApiCreateConnection connect is oning,fd %d,current tick %u", sockMgrContext->sockId, sockMgrContext->connectStartTicks);
        tcpipApiFreeResponse(response);
        return ret;
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_11, P_ERROR, 0, "TcpipApiCreateConnection connect fail");
        cmsSockMgrRemoveContext(sockMgrContext);
        goto fail;
    }



fail:
    result->cause = ret;
    result->connectionId = -1;
    if(cmsSockMgrGetCurrHibTicks() < reqTicks + timeout)
    {
        tcpipApiSendResponse(rcvRequestFd, response, responseLen, sourceAddr, sourcePort);
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCreateConnection_12, P_ERROR, 3, "TcpipApiCreateConnection request has timeout,can not send response, reqticks %u, timeout %u, now %u", reqTicks, timeout, cmsSockMgrGetCurrHibTicks());
        cmsSockMgrCloseSocket(sockMgrContext->sockId);
        cmsSockMgrRemoveContext(sockMgrContext);
    }
    tcpipApiFreeResponse(response);
    return ret;

}


static UINT32 tcpipApiSendData(TcpipApiSendDataReq *tcpipApiSendReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd, UINT32 reqTicks, UINT16 timeout)
{
    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    TcpipApiPriMgrContext *priMgrcontext;
    CmsSockMgrResponse *response;
    TcpipSendDataResult *result;
    UINT16 responseLen = 0;


    if(tcpipApiSendReq == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_2, P_ERROR, 0, "TcpipApiSendData request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }


    //init response
    response = tcpipApiMakeResponse(sizeof(TcpipSendDataResult), SOCK_SOURCE_SDKAPI, TCPIP_API_REQ_SEND_DATA);
    if(response == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_1, P_ERROR, 0, "TcpipApiSendData invalid param");
        return CME_SOCK_MGR_ERROR_NO_MEMORY;
    }
    result = (TcpipSendDataResult *)response->body;
    result->cause = CME_SUCC;
    result->sendLen = -1;
    responseLen = sizeof(CmsSockMgrResponse) + sizeof(TcpipSendDataResult);

    //find the cms mgr context
    sockMgrContext = cmsSockMgrFindMgrContextById(tcpipApiSendReq->connectionId);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_3, P_INFO, 0, "TcpipApiSendData can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
        goto fail;
    }
    else
    {
        priMgrcontext = (TcpipApiPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

        if(sockMgrContext->source != SOCK_SOURCE_SDKAPI)
        {
           ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_4, P_ERROR, 1, "TcpipApiSendData the seq socket %d is not skapi socket", tcpipApiSendReq->connectionId);
           ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
           goto fail;
        }
        else if(sockMgrContext->status != SOCK_CONN_STATUS_CONNECTED)
        {
           ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_5, P_ERROR, 1, "TcpipApiSendData the seq socket %d is not connected", tcpipApiSendReq->connectionId);
           ret = CME_SOCK_MGR_ERROR_NO_CONNECTED;
           goto fail;
        }
        else
        {
            if(tcpipApiSendReq->sequence != 0)
            {
                if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->bitmap, tcpipApiSendReq->sequence) == TRUE)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_6, P_WARNING, 2, "TcpipApiSendData the socket %d sequence %u is reusing ", tcpipApiSendReq->connectionId, tcpipApiSendReq->sequence);
                    ret = CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID;
                    goto fail;
                }
            }

            if(tcpipApiSendReq->dataLen > 0)
            {
                result->sendLen = ps_send_with_sequence(sockMgrContext->sockId, tcpipApiSendReq->data, tcpipApiSendReq->dataLen, 0, tcpipApiSendReq->raiInfo, tcpipApiSendReq->expectFlag, tcpipApiSendReq->sequence);
                if(result->sendLen > 0)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_7, P_INFO, 1, "TcpipApiSendData send packet success %u", result->sendLen);
                    //send response
                    if(cmsSockMgrGetCurrHibTicks() < reqTicks + timeout)
                    {
                        tcpipApiSendResponse(rcvRequestFd, response, responseLen, sourceAddr, sourcePort);

                        //process UL sequence status
                        if(tcpipApiSendReq->sequence != 0)
                        {
                            //send UDP UL sequence status
                            if(sockMgrContext->type == SOCK_CONN_TYPE_UDP)
                            {
                                cmsSockMgrCallUlStatusEventCallback(sockMgrContext, tcpipApiSendReq->sequence, SOCK_CONN_UL_STATUS_SUCCESS);
                            }
                            else
                            {
                                //add to UL pending sequence status bit map
                                cmsSockMgrUpdateUlPendingSequenceBitMapState(priMgrcontext->bitmap, tcpipApiSendReq->sequence, TRUE);
                            }
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_8, P_ERROR, 3, "TcpipApiSendData request has timeout,can not send response, reqticks %u, timeout %u, now %u", reqTicks, timeout, cmsSockMgrGetCurrHibTicks());
                    }

                    tcpipApiFreeResponse(response);

                    return ret;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_9, P_ERROR, 0, "TcpipApiSendData send fail");
                    ret = CME_SOCK_MGR_ERROR_SEND_FAIL;
                    goto fail;
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_10, P_ERROR, 0, "TcpipApiSendData invalid sendLen param");
                ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
                goto fail;
            }
        }
    }


fail:
    ECOMM_TRACE(UNILOG_ATCMD_SOCK, TcpipApiSendData_11, P_INFO, 1, "TcpipApiSendData send packet fail %u", result->sendLen);
    //send response
    result->cause = ret;
    if(cmsSockMgrGetCurrHibTicks() < reqTicks + timeout)
    {
        tcpipApiSendResponse(rcvRequestFd, response, responseLen, sourceAddr, sourcePort);

        //send UL sequence status
        if(tcpipApiSendReq->sequence != 0)
        {
            cmsSockMgrCallUlStatusEventCallback(sockMgrContext, tcpipApiSendReq->sequence, SOCK_CONN_UL_STATUS_FAIL);
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSendData_12, P_ERROR, 3, "TcpipApiSendData request has timeout,can not send response, reqticks %u, timeout %u, now %u", reqTicks, timeout, cmsSockMgrGetCurrHibTicks());
    }

    tcpipApiFreeResponse(response);

    return ret;
}


UINT32 tcpipApiCloseConnection(TcpipApiCloseConnectionReq *tcpipApiCloseReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd, UINT32 reqTicks, UINT16 timeout)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    TcpipApiPriMgrContext *priMgrcontext;
    UINT8 sequence;
    CmsSockMgrResponse *response;
    TcpipCloseResult *result;
    UINT16 responseLen = 0;


    //init response
    response = tcpipApiMakeResponse(sizeof(TcpipCloseResult), SOCK_SOURCE_SDKAPI, TCPIP_API_REQ_CLOSE_CONNECTION);
    if(response == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_1, P_ERROR, 0, "TcpipApiCloseConnection invalid param");
        return CME_SOCK_MGR_ERROR_NO_MEMORY;
    }
    result = (TcpipCloseResult *)response->body;
    result->cause = CME_SUCC;
    result->result = 0;
    responseLen = sizeof(CmsSockMgrResponse) + sizeof(TcpipCloseResult);

    if(tcpipApiCloseReq == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_2, P_ERROR, 0, "TcpipApiCloseConnection request point is null");
        ret = CME_SOCK_MGR_ERROR_UNKNOWN;
        goto fail;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(tcpipApiCloseReq->connectionId);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_3, P_INFO, 0, "TcpipApiCloseConnection can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
        goto fail;
    }
    else
    {
        if(sockMgrContext->source != SOCK_SOURCE_SDKAPI)
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_4, P_ERROR, 1, "TcpipApiCloseConnection socket %d is not ecsoc", tcpipApiCloseReq->connectionId);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
            goto fail;
        }

        priMgrcontext = (TcpipApiPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

        ret = cmsSockMgrCloseSocket(sockMgrContext->sockId);
        if(ret == CME_SUCC)
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_5, P_INFO, 0, "TcpipApiCloseConnection close socket success");

            // send the pending UL sequence status indicate
            for(sequence = CMS_SOCK_MGR_UL_DATA_SEQUENCE_MIN; sequence <= CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX; sequence ++)
            {
                if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->bitmap, sequence) == TRUE)
                {
                    cmsSockMgrCallUlStatusEventCallback(sockMgrContext, sequence, SOCK_CONN_UL_STATUS_FAIL);

                }

                if(sequence == CMS_SOCK_MGR_UL_DATA_SEQUENCE_MIN)
                {
                    break;
                }

            }
            cmsSockMgrCallStatusEventCallback(sockMgrContext, sockMgrContext->status, SOCK_CONN_STATUS_CLOSED, ret);
            cmsSockMgrRemoveContext(sockMgrContext);
            if(cmsSockMgrGetCurrHibTicks() < reqTicks + timeout)
            {
                tcpipApiSendResponse(rcvRequestFd, response, responseLen, sourceAddr, sourcePort);
            }
            else
            {
                ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_6, P_ERROR, 3, "TcpipApiCloseConnection request has timeout,can not send response, reqticks %u, timeout %u, now %u", reqTicks, timeout, cmsSockMgrGetCurrHibTicks());
            }

            tcpipApiFreeResponse(response);
            return ret;

        }
    }

fail:
    result->cause = ret;
    result->result = -1;
    if(cmsSockMgrGetCurrHibTicks() < reqTicks + timeout)
    {
        tcpipApiSendResponse(rcvRequestFd, response, responseLen, sourceAddr, sourcePort);
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiCloseConnection_7, P_ERROR, 3, "AtecSocketHandleConnectReq request has timeout,can not send response, reqticks %u, timeout %u, now %u", reqTicks, timeout, cmsSockMgrGetCurrHibTicks());
    }

    tcpipApiFreeResponse(response);
    return ret;
}

void tcpipApiProessReq(CmsSockMgrRequest *tcpipApiReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd)
{

    if(tcpipApiReq == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiProessReq_1, P_ERROR, 0, "TcpipApiProessReq invalid param");
        return;
    }

    //check magic
    if(tcpipApiReq->magic != CMS_SOCK_MGR_ASYNC_REQUEST_MAGIC && tcpipApiReq->magic != CMS_SOCK_MGR_SYNC_REQUEST_MAGIC)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiProessReq_2, P_ERROR, 0, "TcpipApiProessReq invalid param");
        return;
    }

    if(tcpipApiReq->source == SOCK_SOURCE_SDKAPI) {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiProessReq_3, P_INFO, 3, "read tcpipApiReqFd success, req_id %d, reqbody 0x%x, sourcePort %d ", tcpipApiReq->reqId, tcpipApiReq->reqBody, sourcePort);
        switch(tcpipApiReq->reqId) {
            case TCPIP_API_REQ_CREATE_CONNECTION:{
                TcpipApiCreateConnectionReq *tcpipApiCreateReq;
                tcpipApiCreateReq = (TcpipApiCreateConnectionReq *)tcpipApiReq->reqBody;
                if(tcpipApiCreateReq != NULL)
                {
                    tcpipApiCreateConnection(tcpipApiCreateReq, sourceAddr, sourcePort, rcvRequestFd, tcpipApiReq->reqTicks, tcpipApiReq->timeout);
                }
                break;
            }
            case TCPIP_API_REQ_SEND_DATA:{
                TcpipApiSendDataReq *tcpipApiSendReq;
                tcpipApiSendReq = (TcpipApiSendDataReq *)tcpipApiReq->reqBody;
                if(tcpipApiSendReq != NULL)
                {
                    tcpipApiSendData(tcpipApiSendReq, sourceAddr, sourcePort, rcvRequestFd, tcpipApiReq->reqTicks, tcpipApiReq->timeout);
                }
                break;
            }
            case TCPIP_API_REQ_CLOSE_CONNECTION:{
                TcpipApiCloseConnectionReq *tcpipApiCloseReq;
                tcpipApiCloseReq = (TcpipApiCloseConnectionReq *)tcpipApiReq->reqBody;
                if(tcpipApiCloseReq != NULL)
                {
                    tcpipApiCloseConnection(tcpipApiCloseReq, sourceAddr, sourcePort, rcvRequestFd, tcpipApiReq->reqTicks, tcpipApiReq->timeout);
                }
                break;
            }
            default:
                ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiProessReq_4, P_WARNING, 1, "process tcpipApi reqeust fail, invalid reqId %d", tcpipApiReq->reqId);
                break;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiProessReq_5, P_WARNING, 1, "read tcpipApi fail, source %u check fail", tcpipApiReq->source);
    }
}


void tcpipApiStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnStatusArg *statusArg)
{
    TcpipApiPriMgrContext *priMgrcontext;
    CmsSockMgrResponse *response;
    TcpipCreateConnResult *result;
    UINT16 responseLen = 0;

    //init response
    response = tcpipApiMakeResponse(sizeof(TcpipCreateConnResult), SOCK_SOURCE_SDKAPI, TCPIP_API_REQ_CREATE_CONNECTION);
    if(response == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStatusEventProcess_1, P_ERROR, 0, "TcpipApiStatusEventProcess invalid param");
        return ;
    }
    result = (TcpipCreateConnResult *)response->body;
    result->cause = CME_SUCC;
    result->connectionId = -1;
    responseLen = sizeof(CmsSockMgrResponse) + sizeof(TcpipCreateConnResult);

    if(mgrContext == NULL || statusArg == NULL)
    {
        tcpipApiFreeResponse(response);
        return;
    }

    priMgrcontext = (TcpipApiPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    switch(statusArg->oldStatus)
    {
        case SOCK_CONN_STATUS_CONNECTING:{
            if(statusArg->newStatus == SOCK_CONN_STATUS_CREATED)
            {
                //connect timeout
                if(statusArg->cause == CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT)
                {
                    result->cause = CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT;
                    if(cmsSockMgrGetCurrHibTicks() < priMgrcontext->startCreateTicks + priMgrcontext->creatTimeout)
                    {
                        tcpipApiSendResponse(priMgrcontext->rcvRequestFd, response, responseLen, &priMgrcontext->createSourceAddr, priMgrcontext->createSourcePort);
                    }
                    else
                    {
                        cmsSockMgrCloseSocket(mgrContext->sockId);
                        cmsSockMgrRemoveContext(mgrContext);
                        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStatusEventProcess_2, P_ERROR, 3, "TcpipApiStatusEventProcess request has timeout,can not send response, reqticks %u, timeout %u, now %u", priMgrcontext->startCreateTicks, priMgrcontext->creatTimeout, cmsSockMgrGetCurrHibTicks());
                    }
                }
                else if(statusArg->cause == CME_SOCK_MGR_ERROR_SOCK_ERROR) //connect fail
                {
                    result->cause = CME_SOCK_MGR_ERROR_SOCK_ERROR;
                    if(cmsSockMgrGetCurrHibTicks() < priMgrcontext->startCreateTicks + priMgrcontext->creatTimeout)
                    {
                        tcpipApiSendResponse(priMgrcontext->rcvRequestFd, response, responseLen, &priMgrcontext->createSourceAddr, priMgrcontext->createSourcePort);
                    }
                    else
                    {
                        cmsSockMgrCloseSocket(mgrContext->sockId);
                        cmsSockMgrRemoveContext(mgrContext);
                        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStatusEventProcess_3, P_ERROR, 3, "TcpipApiStatusEventProcess request has timeout,can not send response, reqticks %u, timeout %u, now %u", priMgrcontext->startCreateTicks, priMgrcontext->creatTimeout, cmsSockMgrGetCurrHibTicks());
                    }
                }
            }
            else if(statusArg->newStatus == SOCK_CONN_STATUS_CONNECTED)
            {
                //connect success
                result->connectionId = mgrContext->sockId;
                if(cmsSockMgrGetCurrHibTicks() < priMgrcontext->startCreateTicks + priMgrcontext->creatTimeout)
                {
                    tcpipApiSendResponse(priMgrcontext->rcvRequestFd, response, responseLen, &priMgrcontext->createSourceAddr, priMgrcontext->createSourcePort);
                }
                else
                {
                    cmsSockMgrCloseSocket(mgrContext->sockId);
                    cmsSockMgrRemoveContext(mgrContext);
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStatusEventProcess_4, P_ERROR, 3, "TcpipApiStatusEventProcess request has timeout,can not send response, reqticks %u, timeout %u, now %u", priMgrcontext->startCreateTicks, priMgrcontext->creatTimeout, cmsSockMgrGetCurrHibTicks());
                }

            }
            break;
        }
        case SOCK_CONN_STATUS_CONNECTED:{
            break;
        }
        default:
            break;
    }

    tcpipApiFreeResponse(response);

    return;
}


void tcpipDlEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnDlArg *dlArg)
{
    TcpipApiPriMgrContext *priMgrcontext;
    TcpipConnectionRecvDataInd rcvDataInd;
    tcpipConnectionCallBack tcpipApiCallback;


    if(mgrContext == NULL || dlArg == NULL)
    {
        return;
    }

    priMgrcontext = (TcpipApiPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    tcpipApiCallback = (tcpipConnectionCallBack)priMgrcontext->tcpipApiConnEventCallback;

    if(tcpipApiCallback != NULL)
    {
        rcvDataInd.connectionId = mgrContext->sockId;
        rcvDataInd.length = dlArg->dataContext->Length;
        rcvDataInd.data = dlArg->dataContext->data;
        tcpipApiCallback(TCPIP_CONNECTION_RECEIVE_EVENT, &rcvDataInd);
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipDlEventProcess_1, P_ERROR, 0, "TcpipDlEventProcess callback is invalid");
    }

    return;
}

void tcpipApiSockErrorEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnErrArg *errArg)
{
    TcpipApiPriMgrContext *priMgrcontext;
    TcpipConnectionStatusInd statusInd;
    tcpipConnectionCallBack tcpipApiCallback;

    if(mgrContext == NULL || errArg == NULL)
    {
        return;
    }

    priMgrcontext = (TcpipApiPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(errArg->errCode != 0)
    {
        tcpipApiCallback = (tcpipConnectionCallBack)priMgrcontext->tcpipApiConnEventCallback;

        cmsSockMgrCloseSocket(mgrContext->sockId);
        if(tcpipApiCallback != NULL)
        {
            statusInd.connectionId = mgrContext->sockId;
            statusInd.status = TCPIP_CONNECTION_STATUS_CLOSED;
            statusInd.cause = errArg->errCode;
            tcpipApiCallback(TCPIP_CONNECTION_STATUS_EVENT, &statusInd);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiSockErrorEventProcess_1, P_ERROR, 0, "TcpipApiSockErrorEventProcess callback is invalid");
        }

        cmsSockMgrRemoveContextById(mgrContext->sockId);
    }

    return;

}


void tcpipApiUlStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnUlStatusArg *ulStatusArg)
{
    TcpipApiPriMgrContext *priMgrcontext;
    TcpipConnectionUlDataStatusInd ulStatusInd;
    tcpipConnectionCallBack tcpipApiCallback;


    if(mgrContext == NULL || ulStatusArg == NULL)
    {
        return;
    }

    priMgrcontext = (TcpipApiPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    tcpipApiCallback = (tcpipConnectionCallBack)priMgrcontext->tcpipApiConnEventCallback;
    if(tcpipApiCallback == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiUlStatusEventProcess_1, P_ERROR, 0, "TcpipApiUlStatusEventProcess callback is invalid");
        return;
    }

    if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->bitmap, ulStatusArg->sequence) == TRUE)
    {
        cmsSockMgrUpdateUlPendingSequenceBitMapState(priMgrcontext->bitmap, ulStatusArg->sequence, FALSE);
        ulStatusInd.sequence = ulStatusArg->sequence;
        ulStatusInd.connectionId = mgrContext->sockId;
        if(ulStatusArg->status == SOCK_MGR_UL_SEQUENCE_STATE_SENT)
        {
            ulStatusInd.status = Tcpip_Connection_UL_DATA_SUCCESS;
            tcpipApiCallback(TCPIP_CONNECTION_UL_STATUS_EVENT, &ulStatusInd);
        }
        else if(ulStatusArg->status == SOCK_MGR_UL_SEQUENCE_STATE_DISCARD)
        {
            ulStatusInd.status = Tcpip_Connection_UL_DATA_FAIL;
            tcpipApiCallback(TCPIP_CONNECTION_UL_STATUS_EVENT, &ulStatusInd);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiUlStatusEventProcess_2, P_WARNING, 2, "TcpipApiUlStatusEventProcess the socket %d, sequence %d status invalid", mgrContext->sockId, ulStatusArg->sequence);
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiUlStatusEventProcess_3, P_WARNING, 2, "TcpipApiUlStatusEventProcess the socket %d, sequence %d status invalid", mgrContext->sockId, ulStatusArg->sequence);
    }

    return;

}


void tcpipApiEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg)
{

    if(mgrContext == NULL)
    {
        return;
    }

    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiEventCallback_1, P_INFO, 2, "TcpipApiEventCallback socketid %d, event %d",mgrContext->sockId, eventType);

    switch(eventType)
    {
        case SOCK_EVENT_CONN_STATUS:{
            CmsSockMgrConnStatusArg *statusArg = (CmsSockMgrConnStatusArg *)eventArg;
            tcpipApiStatusEventProcess(mgrContext, statusArg);
            break;
        }
        case SOCK_EVENT_CONN_DL:{
            CmsSockMgrConnDlArg *dlArg = (CmsSockMgrConnDlArg *)eventArg;
            tcpipDlEventProcess(mgrContext, dlArg);
            break;
        }
        case SOCK_EVENT_CONN_ERROR:{
            CmsSockMgrConnErrArg *errArg = (CmsSockMgrConnErrArg *)eventArg;
            tcpipApiSockErrorEventProcess(mgrContext, errArg);
            break;
        }
        case SOCK_EVENT_CONN_UL_STATUS:{
            CmsSockMgrConnUlStatusArg *ulStatusArg = (CmsSockMgrConnUlStatusArg *)eventArg;
            tcpipApiUlStatusEventProcess(mgrContext, ulStatusArg);
            break;
        }
        default:
            break;
    }

    return;
}


BOOL tcpipCheckHibMode(CmsSockMgrContext *mgrContext)
{
    TcpipApiPriMgrContext *priMgrcontext;
    UINT8 sequence;

    if(mgrContext == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, tcpipCheckHibMode_1, P_WARNING, 0, "tcpipCheckHibMode invalid cms sock mgr context");
        return TRUE;
    }

    priMgrcontext = (TcpipApiPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    //add status check
    if(mgrContext->status != SOCK_CONN_STATUS_CONNECTED)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, tcpipCheckHibMode_2, P_INFO, 2, "tcpipCheckHibMode socket id %u status %d, can not enter hib/sleep2 mode", mgrContext->sockId, mgrContext->status);
        return FALSE;
    }

    //check any pending UL sequence status
    for(sequence = CMS_SOCK_MGR_UL_DATA_SEQUENCE_MIN; sequence <= CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX; sequence ++)
    {
        //check the UL sequence status indication bitmap
        if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->bitmap, sequence) == TRUE)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, tcpipCheckHibMode_3, P_INFO, 2, "tcpipCheckHibMode socket id %u exist pending UL sequence %ustatus", mgrContext->sockId, sequence);
            return FALSE;
        }

        if(sequence == CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX)
        {
            break;
        }
    }

    return TRUE;

}



void tcpipApiStoreConnHibContext(CmsSockMgrContext *sockMgrContext, CmsSockMgrConnHibContext *hibContext)
{
    TcpipApiConnHibPriContext *tcpipApiPriContext;
    TcpipApiPriMgrContext *priMgrcontext;

    if(sockMgrContext == NULL || hibContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStoreConnHibContext_1, P_WARNING, 0, "TcpipApiStoreConnHibContext context is invalid");
        return;
    }

    if(sizeof(TcpipApiConnHibPriContext) > CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStoreConnHibContext_2, P_WARNING, 2, "TcpipApiStoreConnHibContext private hib context len %d bit than %d", sizeof(TcpipApiConnHibPriContext), CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH);
        return;
    }

    tcpipApiPriContext = (TcpipApiConnHibPriContext *)&hibContext->priContext;
    priMgrcontext = (TcpipApiPriMgrContext *)sockMgrContext->priContext;

    if(tcpipApiPriContext && priMgrcontext)
    {
        memset(hibContext, 0, sizeof(CmsSockMgrConnHibContext));

        hibContext->sockId = sockMgrContext->sockId;
        hibContext->source = sockMgrContext->source;
        hibContext->type = sockMgrContext->type;
        hibContext->status = sockMgrContext->status;
        hibContext->localPort = sockMgrContext->localPort;
        ip_addr_copy(hibContext->localAddr, sockMgrContext->localAddr);
        if(sockMgrContext->type == SOCK_CONN_TYPE_UDP)
        {
            hibContext->magic = CMS_SOCK_MGR_UDP_CONTEXT_MAGIC;
        }
        else if(sockMgrContext->type == SOCK_CONN_TYPE_TCP)
        {
            hibContext->magic = CMS_SOCK_MGR_TCP_CONTEXT_MAGIC;
        }

        hibContext->eventCallback =(void *)sockMgrContext->eventCallback;

        tcpipApiPriContext->tcpipApiConnEventCallback = priMgrcontext->tcpipApiConnEventCallback;
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiStoreConnHibContext_3, P_WARNING, 0, "TcpipApiStoreConnHibContext private context is invalid");
    }

    return;

}


void tcpipApiRecoverConnContext(CmsSockMgrConnHibContext *hibContext)
{
    TcpipApiConnHibPriContext *tcpipApiPriContext;
    TcpipApiPriMgrContext *priMgrcontext;

    CmsSockMgrContext* sockMgrContext = NULL;
    ip_addr_t localAddr;
    ip_addr_t remoteAddr;
    UINT16 localPort;
    UINT16 remotePort;
    INT32 rebuildType;
    CmsSockMgrConnStatus connStatus = SOCK_CONN_STATUS_CLOSED;

    if(hibContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_1, P_WARNING, 0, "TcpipApiRecoverConnContext context is invalid");
        return;
    }

    if(sizeof(TcpipApiConnHibPriContext) > CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_2, P_WARNING, 2, "TcpipApiRecoverConnContext private hib context len %d bit than %d", sizeof(TcpipApiConnHibPriContext), CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH);
        return;
    }

    tcpipApiPriContext = (TcpipApiConnHibPriContext *)&hibContext->priContext;
    if(tcpipApiPriContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_3, P_WARNING, 0, "TcpipApiRecoverConnContext private hib context is invalid");
        return;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(TcpipApiPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_4, P_ERROR, 0, "TcpipApiRecoverConnContext atskt is full");
        return;
    }

    priMgrcontext = (TcpipApiPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(hibContext->sockId >= 0)
    {
        if(hibContext->status == SOCK_CONN_STATUS_CONNECTED)
        {
            if(netconn_get_sock_info(hibContext->sockId, &localAddr, &remoteAddr, &localPort, &remotePort, (int *)&rebuildType) == ERR_OK)
            {
                connStatus = cmsSockMgrRebuildSocket(hibContext->sockId, &localAddr, &remoteAddr, &localPort, &remotePort, rebuildType);
                if(connStatus == SOCK_CONN_STATUS_CLOSED)
                {
                    cmsSockMgrRemoveContext(sockMgrContext);
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_5, P_ERROR, 1, "TcpipApiRecoverConnContext rebuild socket id %d fail", hibContext->sockId);
                }
                else if(connStatus == SOCK_CONN_STATUS_CREATED)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_6, P_ERROR, 1, "TcpipApiRecoverConnContext rebuild socket id %d success,but status is not valid", hibContext->sockId);
                    cmsSockMgrCloseSocket(hibContext->sockId);
                    cmsSockMgrRemoveContext(sockMgrContext);
                }else if(connStatus == SOCK_CONN_STATUS_CONNECTED)
                {
                    ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_7, P_INFO, 1, "TcpipApiRecoverConnContext rebuild socket id %d success", hibContext->sockId);
                    sockMgrContext->sockId = hibContext->sockId;
                    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                    sockMgrContext->hibEnable = TRUE;
                    sockMgrContext->source = hibContext->source;
                    sockMgrContext->type = hibContext->type;
                    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
                    sockMgrContext->localPort = localPort;
                    ip_addr_copy(sockMgrContext->localAddr, localAddr);

                    priMgrcontext->tcpipApiConnEventCallback = tcpipApiPriContext->tcpipApiConnEventCallback;

                }
            }
            else if(hibContext->status == SOCK_CONN_STATUS_CREATED)
            {
                connStatus = cmsSockMgrReCreateSocket(hibContext->sockId, hibContext->domain, hibContext->type);
                if(connStatus == SOCK_CONN_STATUS_CLOSED)
                {
                    cmsSockMgrRemoveContext(sockMgrContext);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, TcpipApiRecoverConnContext_8, P_ERROR, 1, "TcpipApiRecoverConnContext recreate socket id %d fail", hibContext->sockId);
                }
                else if(connStatus == SOCK_CONN_STATUS_CREATED)
                {
                    if(hibContext->localPort || !ip_addr_isany_val(hibContext->localAddr))
                    {
                        if(cmsSockMgrBindSocket(hibContext->sockId, hibContext->domain, hibContext->localPort, &hibContext->localAddr) == CME_SUCC)
                        {
                            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_91, P_INFO, 1, "TcpipApiRecoverConnContext rebuild socket id %d success", hibContext->sockId);
                    sockMgrContext->sockId = hibContext->sockId;
                    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                    sockMgrContext->source = hibContext->source;
                    sockMgrContext->type = hibContext->type;
                            if(hibContext->type == SOCK_CONN_TYPE_UDP)
                            {
                                sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
                            }
                            else
                            {
                    sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
                            }

                            sockMgrContext->localPort = hibContext->localPort;
                            ip_addr_copy(sockMgrContext->localAddr, hibContext->localAddr);
                    priMgrcontext->tcpipApiConnEventCallback = tcpipApiPriContext->tcpipApiConnEventCallback;

                    cmsSockMgrEnableHibMode(sockMgrContext);
                        }
                        else
                        {
                            cmsSockMgrCloseSocket(hibContext->sockId);
                            cmsSockMgrRemoveContext(sockMgrContext);
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_bind_fail, P_ERROR, 1, "AtEcsocRecoverConnContext recreate socket id %d bind fail", hibContext->sockId);
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_92, P_INFO, 1, "TcpipApiRecoverConnContext rebuild socket id %d success", hibContext->sockId);
                        sockMgrContext->sockId = hibContext->sockId;
                        sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                        sockMgrContext->source = hibContext->source;
                        sockMgrContext->type = hibContext->type;
                        sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
                        priMgrcontext->tcpipApiConnEventCallback = tcpipApiPriContext->tcpipApiConnEventCallback;
                        cmsSockMgrEnableHibMode(sockMgrContext);
                    }
                }
                else
                {
                    cmsSockMgrRemoveContext(sockMgrContext);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, TcpipApiRecoverConnContext_10, P_ERROR, 2, "TcpipApiRecoverConnContext recreate socket id %d status %d is invalid", hibContext->sockId, connStatus);                    
                }
            }
            else
            {
                cmsSockMgrRemoveContext(sockMgrContext);
                ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_11, P_ERROR, 2, "TcpipApiRecoverConnContext hib socket id %d status %dis invalid", hibContext->sockId, hibContext->status);                
            }
        }
    }
    else
    {
        cmsSockMgrRemoveContext(sockMgrContext);
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipApiRecoverConnContext_12, P_ERROR, 1, "TcpipApiRecoverConnContext rebuild socket id %d is invalid", hibContext->sockId);
    }

    return;

}


