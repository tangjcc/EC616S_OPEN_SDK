/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ec_tcpip_api.c
 * Description:  API interface implementation source file for socket service
 * History:      create by xwang
 *
 ****************************************************************************/

#include "cms_sock_mgr.h"
#include "ec_tcpip_api_entity.h"
#include "def.h"
#include "inet.h"
#include "sockets.h"
#include "lwip/api.h"
#include "lwip/priv/api_msg.h"
#include "ec_tcpip_api.h"



/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

/******************************************************************************
 * TcpipConnectionCreate
 * Description: create a tcpip connection
 * input: type(mandatory) 
 *        localIp(option) stringlen <= 63
 *        localPort(option)
 *        destIp(mandatory)"183.100.1.123" or"2001::2"   
 *        destPort(mandatory)
 *        callBack -> the tcpip connection related event(TcpipConnectionEvent) callback handler
 * output: >=0 connectionId, <0 fail
 * Comment:
******************************************************************************/

INT32 tcpipConnectionCreate(TcpipConnectionProtocol protocol, CHAR *localIp, UINT16 localPort, CHAR *destIp, UINT16 destPort, tcpipConnectionCallBack callBack)
{
    TcpipApiCreateConnectionReq *tcpipApiCreateReq;
    CmsSockMgrResponse *response;
    TcpipCreateConnResult *result;
    INT32 ret = -1;

    //check paramter
    if(destIp == NULL || callBack == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_1, P_ERROR, 0, "TcpipConnectionCreates invalid param");
        return -1;
    }

    tcpipApiCreateReq =(TcpipApiCreateConnectionReq *)malloc(sizeof(TcpipApiCreateConnectionReq));
    if(tcpipApiCreateReq)
    {
        memset(tcpipApiCreateReq, 0 ,sizeof(TcpipApiCreateConnectionReq));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_2, P_ERROR, 0, "TcpipConnectionCreates invalid param");
        return -1;        
    }

    response =(CmsSockMgrResponse *)malloc(sizeof(TcpipCreateConnResult) + sizeof(CmsSockMgrResponse));
    if(response)
    {
        memset(response, 0 , sizeof(TcpipCreateConnResult) + sizeof(CmsSockMgrResponse));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_3, P_ERROR, 0, "TcpipConnectionCreates invalid param");
        free(tcpipApiCreateReq);
        return -1;        
    }     

    if(localIp)
    {
        if(ipaddr_aton((const char*)localIp, &tcpipApiCreateReq->localAddr) == 0)
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_4, P_ERROR, 0, "TcpipConnectionCreates local address invalid");        
            free(tcpipApiCreateReq);
            free(response);
            return -1;              
        }
    }

    if(destIp)
    {
        if(ipaddr_aton((const char*)destIp, &tcpipApiCreateReq->remoteAddr) == 0)
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_5, P_ERROR, 0, "TcpipConnectionCreates destAddr invalid");
            free(tcpipApiCreateReq);
            free(response);
            return -1;           
        }
    }

    tcpipApiCreateReq->tcpipApiConnEventCallback = (void *)callBack;
    tcpipApiCreateReq->eventCallback = (void *)tcpipApiEventCallback;
    tcpipApiCreateReq->remotePort = destPort;
    tcpipApiCreateReq->localPort = localPort;
    if(protocol == TCPIP_CONNECTION_PROTOCOL_UDP)
    {
        tcpipApiCreateReq->type = SOCK_DGRAM_TYPE;
    }
    else if(protocol == TCPIP_CONNECTION_PROTOCOL_TCP)
    {
        tcpipApiCreateReq->type = SOCK_STREAM_TYPE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_6, P_ERROR, 0, "TcpipConnectionCreates protocol invalid");
        free(tcpipApiCreateReq);
        free(response);
        return -1;     
    }
    
    ret = cmsSockMgrSendsyncRequest(TCPIP_API_REQ_CREATE_CONNECTION, (void *)tcpipApiCreateReq, SOCK_SOURCE_SDKAPI, response, TCPIP_SDK_API_TIMEOUT);
    if(ret == SOCK_MGR_ACTION_FAIL)    
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_7, P_ERROR, 0, "TcpipConnectionCreates send request fail");
        free(tcpipApiCreateReq);
        free(response);
        ret = -1;
    }
    else if(ret == SOCK_MGR_ACTION_TIMEOUT)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_8, P_ERROR, 0, "TcpipConnectionCreates send request timeout");
        free(response);
        ret = -1;
    }else {
        //check source and reqId
        if(response->source == SOCK_SOURCE_SDKAPI && response->reqId == TCPIP_API_REQ_CREATE_CONNECTION)
        {
            result = (TcpipCreateConnResult *)response->body;
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_9, P_INFO, 2, "TcpipConnectionCreates recv response success,result %d, cause %d", result->connectionId, result->cause);
            ret = result->connectionId;
            free(response);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionCreate_10, P_ERROR, 0, "TcpipConnectionCreates recv invalid response");
            free(response);            
        }
    }

    return ret;
    
}

/******************************************************************************
 * TcpipConnectionSend
 * Description: send data by a tcpip connection
 * input: connectionId(mandatory)
 *        data(mandatory) -> the data which need send, 
 *        dataLen(mandatory) -> data length
 *        raiInfo(option)
 *        exceptFlag(option)
 *        sequence(option) -> the sequence number of the UL data. 1~255: a UL status event will be indicate with the socket event callback 0:no UL status event
 * output:
 * Comment:
******************************************************************************/
INT32 tcpipConnectionSend(INT32 connectionId, UINT8 *data, UINT16 dataLen, UINT8 raiInfo, UINT8 expectFlag, UINT8 sequence)
{
    TcpipApiSendDataReq *tcpipApiSendDataReq;
    CmsSockMgrResponse *response;
    TcpipSendDataResult *result;
    INT32 ret = -1;

    //check paramter

    if(data == NULL || connectionId < 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_1, P_ERROR, 0, "TcpipConnectionSend invalid param");
        return -1;
    }

    tcpipApiSendDataReq =(TcpipApiSendDataReq *)malloc(sizeof(TcpipApiSendDataReq)+ dataLen);
    if(tcpipApiSendDataReq)
    {
        memset(tcpipApiSendDataReq, 0, sizeof(TcpipApiSendDataReq)+ dataLen);
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_2, P_ERROR, 0, "TcpipConnectionSend invalid param");
        return -1;        
    }

    response =(CmsSockMgrResponse *)malloc(sizeof(TcpipSendDataResult) + sizeof(CmsSockMgrResponse));
    if(response)
    {
        memset(response, 0 , sizeof(TcpipSendDataResult) + sizeof(CmsSockMgrResponse));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_3, P_ERROR, 0, "TcpipConnectionSend invalid param");
        free(tcpipApiSendDataReq);
        return -1;        
    }     


    tcpipApiSendDataReq->connectionId = connectionId;
    tcpipApiSendDataReq->dataLen = dataLen;
    tcpipApiSendDataReq->raiInfo = raiInfo;
    tcpipApiSendDataReq->expectFlag = expectFlag;
    tcpipApiSendDataReq->sequence = sequence;
    memcpy(tcpipApiSendDataReq->data, data, dataLen);
    
    ret = cmsSockMgrSendsyncRequest(TCPIP_API_REQ_SEND_DATA, (void *)tcpipApiSendDataReq, SOCK_SOURCE_SDKAPI, response, TCPIP_SDK_API_TIMEOUT);
    if(ret == SOCK_MGR_ACTION_FAIL)    
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_4, P_ERROR, 0, "TcpipConnectionSend send request fail");
        free(tcpipApiSendDataReq);
        free(response);
        ret = -1;
    }
    else if(ret == SOCK_MGR_ACTION_TIMEOUT)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_5, P_ERROR, 0, "TcpipConnectionSend send request timeout");
        free(response);
        ret = -1;
    }else {
        //check source and reqId
        if(response->source == SOCK_SOURCE_SDKAPI && response->reqId == TCPIP_API_REQ_SEND_DATA)
        {
            result = (TcpipSendDataResult *)response->body;
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_6, P_INFO, 2, "TcpipConnectionSend recv response success,result %d, cause %d", result->sendLen, result->cause);
            ret = result->sendLen;
            free(response);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionSend_7, P_ERROR, 0, "TcpipConnectionSend recv invalid response");
            free(response);            
        }
    }

    return ret;
    
}


/******************************************************************************
 * TcpipConnectionClose
 * Description: close a tcpip connection
 * input: connectionId(mandatory) 
 * output:
 * Comment:
******************************************************************************/
INT32 tcpipConnectionClose(INT32 connectionId)
{
    TcpipApiCloseConnectionReq *tcpipApiCloseReq;
    CmsSockMgrResponse *response;
    TcpipCloseResult *result;
    INT32 ret = -1;

    //check paramter

    if(connectionId < 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_1, P_ERROR, 0, "TcpipConnectionClose invalid param");
        return -1;
    }

    tcpipApiCloseReq =(TcpipApiCloseConnectionReq *)malloc(sizeof(TcpipApiCloseConnectionReq));
    if(tcpipApiCloseReq)
    {
        memset(tcpipApiCloseReq, 0, sizeof(TcpipApiCloseConnectionReq));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_2, P_ERROR, 0, "TcpipConnectionClose invalid param");
        return -1;        
    }

    response =(CmsSockMgrResponse *)malloc(sizeof(TcpipCloseResult) + sizeof(CmsSockMgrResponse));
    if(response)
    {
        memset(response, 0 , sizeof(TcpipCloseResult) + sizeof(CmsSockMgrResponse));
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_3, P_ERROR, 0, "TcpipConnectionClose invalid param");
        free(tcpipApiCloseReq);
        return -1;        
    }     

    tcpipApiCloseReq->connectionId = connectionId;
    
    ret = cmsSockMgrSendsyncRequest(TCPIP_API_REQ_CLOSE_CONNECTION, (void *)tcpipApiCloseReq, SOCK_SOURCE_SDKAPI, response, TCPIP_SDK_API_TIMEOUT);
    if(ret == SOCK_MGR_ACTION_FAIL)    
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_4, P_ERROR, 0, "TcpipConnectionClose send request fail");
        free(tcpipApiCloseReq);
        free(response);
        ret = -1;
    }
    else if(ret == SOCK_MGR_ACTION_TIMEOUT)
    {
        ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_5, P_ERROR, 0, "TcpipConnectionClose send request timeout");
        free(response);
        ret = -1;
    }else {
        //check source and reqId
        if(response->source == SOCK_SOURCE_SDKAPI && response->reqId == TCPIP_API_REQ_CLOSE_CONNECTION)
        {
            result = (TcpipCloseResult *)response->body;
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_6, P_INFO, 0, "TcpipConnectionClose recv response success,result %d, cause %d", result->result, result->cause);
            ret = result->result;
            free(response);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_SDK_API, TcpipConnectionClose_7, P_ERROR, 0, "TcpipConnectionClose recv invalid response");
            free(response);            
        }
    }

    return ret;
    
}



