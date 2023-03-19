/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_sock_task.c
*
*  Description:
*
*  History: create by xwang
*
*  Notes:
*
******************************************************************************/
#include "at_util.h"
#include "def.h"
#include "inet.h"
#include "sockets.h"
#include "lwip/api.h"
#include "lwip/priv/api_msg.h"
#include "at_sock_entity.h"
#include "mw_config.h"
#include "mw_aon_info.h"



/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS
#define AT_SOCK_MEMBER_OFFSET(STRUCT, MEMBER) (&(((STRUCT *)0)->MEMBER))

EcSocPublicDlConfig gecSocPubDlSet ={AT_SOC_PUBLIC_DL_BUFFER_DEF, 0, AT_SOC_PUBLIC_DL_PKG_NUM_DEF, 0, NMI_MODE_1, RCV_ENABLE};




/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS

/****************************************************************************************
 *
 *
****************************************************************************************/
static void atecSktSendCnf(UINT16 handle, ApplRetCode result, UINT16 primId, UINT32 errCode, INT32 fd, INT32 status)
{
    AtecSktCnf sktCnf;

    memset(&sktCnf, 0, sizeof(AtecSktCnf));

    if (result == APPL_RET_SUCC)
    {
        switch(primId)
        {
            case APPL_SOCKET_BIND_CNF:
            case APPL_SOCKET_CONNECT_CNF:
            case APPL_SOCKET_SEND_CNF:
            case APPL_SOCKET_DELETE_CNF:
            {
                break;
            }
            case APPL_SOCKET_CREATE_CNF:
            {
               sktCnf.cnfBody.fd = fd;
               break;
            }
            case APPL_SOCKET_STATUS_CNF:
            {
                sktCnf.cnfBody.status = status;
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktSendCnf_1, P_ERROR, 1, "AtecSktSendCnf invalid primId %d", primId);
                return ;
            }

        }
        applSendCmsCnf(handle, APPL_RET_SUCC, APPL_SOCKET, primId, sizeof(AtecSktCnf), &sktCnf);
    }
    else if(result == APPL_RET_FAIL)
    {
        switch(primId)
        {
            case APPL_SOCKET_BIND_CNF:
            case APPL_SOCKET_CONNECT_CNF:
            case APPL_SOCKET_SEND_CNF:
            case APPL_SOCKET_DELETE_CNF:
            case APPL_SOCKET_CREATE_CNF:
            case APPL_SOCKET_STATUS_CNF:
            {
                sktCnf.cnfBody.errCode = errCode;
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktSendCnf_2, P_ERROR, 1, "AtecSktSendCnf invalid primId %d", primId);
                return ;
            }

        }
        applSendCmsCnf(handle, APPL_RET_FAIL, APPL_SOCKET, primId, sizeof(AtecSktCnf), &sktCnf);
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktSendCnf_3, P_ERROR, 1, "AtecSktSendCnf invalid result %d", result);
    }

    return;
}


static void atecSocSendCnf(UINT16 handle, ApplRetCode result, UINT16 primId, UINT32 errCode, void *responseBody)
{

    INT32 padding;

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_1, P_INFO, 3, "AtecSocSendCnf primId %u, result %u, errCode %u", primId, result, errCode);

    if(result == APPL_RET_SUCC)
    {
        switch(primId)
        {
            case APPL_ECSOC_QUERY_CNF:
            case APPL_ECSOC_CLOSE_CNF:
            case APPL_ECSOC_NMI_CNF:
            case APPL_ECSOC_NMIE_CNF:
            case APPL_ECSOC_TCPCONNECT_CNF:
            case APPL_ECSOC_GNMIE_CNF:
            case APPL_ECSOC_STATUS_CNF:
            {
                applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSOC, primId, sizeof(INT32), &padding);
                break;
            }
            case APPL_ECSOC_CREATE_CNF:
            {
                if(responseBody)
                {
                    applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSOC, primId, sizeof(EcSocCreateResponse), responseBody);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_2, P_ERROR, 0, "AtecSocSendCnf APPL_ECSOC_CREATE_CNF invalid responseBody");
                }
                break;
            }
            case APPL_ECSOC_UDPSEND_CNF:
            case APPL_ECSOC_UDPSENDF_CNF:
            {
                if(responseBody)
                {
                    applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSOC, primId, sizeof(EcSocUdpSendResponse), responseBody);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_3, P_ERROR, 0, "AtecSocSendCnf APPL_ECSOC_UDPSEND_CNF invalid responseBody");
                }
                break;
            }
            case APPL_ECSOC_READ_CNF:
            {
                EcSocReadResponse *socreadResponse = (EcSocReadResponse *)responseBody;
                if(socreadResponse)
                {

                    applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSOC, primId, sizeof(EcSocReadResponse) + socreadResponse->length, responseBody);

                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_4, P_ERROR, 0, "AtecSocSendCnf APPL_ECSOC_READ_CNF invalid responseBody");
                }
                break;
            }
            case APPL_ECSOC_TCPSEND_CNF:
            {
                if(responseBody)
                {
                    applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSOC, primId, sizeof(EcSocTcpSendResponse), responseBody);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_5, P_ERROR, 0, "AtecSocSendCnf APPL_ECSOC_TCPSEND_CNF invalid responseBody");
                }
                break;
            }
            case APPL_ECSOC_GNMI_CNF:
            {
                if(responseBody)
                {
                    applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSOC, primId, sizeof(EcSocGNMIResponse), responseBody);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_6, P_ERROR, 0, "AtecSocSendCnf APPL_ECSOC_GNMI_CNF invalid responseBody");
                }
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_7, P_ERROR, 1, "AtecSocSendCnf invalid primId %d", primId);
                return ;
            }

        }

    }
    else if(result == APPL_RET_FAIL)
    {
        switch(primId)
        {
            case APPL_ECSOC_QUERY_CNF:
            case APPL_ECSOC_CLOSE_CNF:
            case APPL_ECSOC_NMI_CNF:
            case APPL_ECSOC_NMIE_CNF:
            case APPL_ECSOC_TCPCONNECT_CNF:
            case APPL_ECSOC_CREATE_CNF:
            case APPL_ECSOC_UDPSEND_CNF:
            case APPL_ECSOC_UDPSENDF_CNF:
            case APPL_ECSOC_READ_CNF:
            case APPL_ECSOC_TCPSEND_CNF:
            case APPL_ECSOC_GNMI_CNF:
            case APPL_ECSOC_GNMIE_CNF:
            case APPL_ECSOC_STATUS_CNF:
            {
                AtecEcSocErrCnf socErrCnf;
                socErrCnf.errCode = errCode;
                applSendCmsCnf(handle, APPL_RET_FAIL, APPL_ECSOC, primId, sizeof(AtecEcSocErrCnf), &socErrCnf);
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_8, P_ERROR, 1, "AtecSocSendCnf invalid primId %d", primId);
                return ;
            }

        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendCnf_9, P_ERROR, 1, "AtecSocSendCnf invalid result %d", result);
    }

    return;
}

static void atecSrvSocSendCnf(UINT16 handle, ApplRetCode result, UINT16 primId, UINT32 errCode, void *responseBody)
{

    INT32 padding;

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendCnf_1, P_INFO, 3, "atecSrvSocSendCnf primId %u, result %u, errCode %u", primId, result, errCode);

    if(result == APPL_RET_SUCC)
    {
        switch(primId)
        {
            case APPL_ECSRVSOC_CREATE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF:
            {
                applSendCmsCnf(handle, APPL_RET_SUCC, APPL_ECSRVSOC, primId, sizeof(INT32), &padding);
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendCnf_2, P_ERROR, 1, "atecSrvSocSendCnf invalid primId %d", primId);
                return ;
            }

        }

    }
    else if(result == APPL_RET_FAIL)
    {
        switch(primId)
        {
            case APPL_ECSRVSOC_CREATE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF:
            case APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF:
            case APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF:
            {
                AtecEcSrvSocErrCnf socErrCnf;
                socErrCnf.errCode = errCode;
                applSendCmsCnf(handle, APPL_RET_FAIL, APPL_ECSRVSOC, primId, sizeof(AtecEcSrvSocErrCnf), &socErrCnf);
                break;
            }
            default:
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendCnf_3, P_ERROR, 1, "AtecSocSendCnf invalid primId %d", primId);
                return ;
            }

        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendCnf_4, P_ERROR, 1, "AtecSocSendCnf invalid result %d", result);
    }

    return;
}

static void atecSktSendInd(UINT16 reqSource, UINT8 primId, UINT16 indBodyLen, void *indBody)
{

    if(indBodyLen == 0 || indBody == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktSendInd_1, P_ERROR, 2, "AtecSktSendInd invalid indBodyLen %u, indBody 0x%x ", indBodyLen, indBody);
        return;
    }

    switch(primId)
    {
        case APPL_SOCKET_ERROR_IND:
        case APPL_SOCKET_RCV_IND:
        {
            applSendCmsInd(reqSource, APPL_SOCKET, primId, indBodyLen, indBody);
            //applSendCmsInd(BROADCAST_IND_HANDLER, UINT8 appId, UINT8 primId, UINT16 primSize, void *pPrimBody)
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktSendInd_2, P_ERROR, 1, "AtecSktSendInd invalid primId %d", primId);
            break;
        }
    }

    return;

}

static void atecSocSendInd(UINT16 reqSource, UINT8 primId, UINT16 indBodyLen, void *indBody)
{

    if(indBodyLen == 0 || indBody == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_1, P_ERROR, 2, "AtecSktSendInd invalid indBodyLen %u, indBody 0x%x ", indBodyLen, indBody);
        return;
    }

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_2, P_INFO, 1, "AtecSktSendInd primId %u", primId);

    switch(primId)
    {
        case APPL_ECSOC_NMI_IND:
        {
            EcSocNMInd *socNMI = (EcSocNMInd *)indBody;
            if(socNMI)
            {
                switch(socNMI->modeNMI)
                {
                    case NMI_MODE_0:
                    {
                        break;
                    }
                    case NMI_MODE_1:
                    {
                        applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocNMInd), indBody);
                        break;
                    }
                    case NMI_MODE_2:
                    case NMI_MODE_3:
                    {
                        applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocNMInd) + socNMI->length, indBody);
                        break;
                    }
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_3, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_NMI_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSOC_CLOSE_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocCloseInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_4, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_CLOSE_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSOC_QUERY_RESULT_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocQueryInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_5, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_QUERY_RESULT_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSOC_GNMIE_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocGNMIEInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_6, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_GNMIE_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSOC_ULSTATUS_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocUlStatusInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_7, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_ULSTATUS_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSOC_STATUS_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocStatusInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_9, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_STATUS_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSOC_CONNECTED_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSOC, primId, sizeof(EcSocConnectedInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_10, P_ERROR, 0, "AtecSocSendInd APPL_ECSOC_CONNECTED_IND invalid indBody");
            }
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendInd_8, P_ERROR, 1, "AtecSocSendInd invalid primId %d", primId);
            break;
        }
    }

    return;

}

static void atecSrvSocSendInd(UINT16 reqSource, UINT8 primId, UINT16 indBodyLen, void *indBody)
{

    if(indBodyLen == 0 || indBody == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_1, P_ERROR, 2, "atecSrvSocSendInd invalid indBodyLen %u, indBody 0x%x ", indBodyLen, indBody);
        return;
    }

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_2, P_INFO, 1, "atecSrvSocSendInd primId %u", primId);

    switch(primId)
    {
        case APPL_ECSRVSOC_CREATE_TCP_LISTEN_IND:
        {
            if(indBody)
            {

                applSendCmsInd(reqSource, APPL_ECSRVSOC, primId, sizeof(EcSrcSocCreateTcpListenInd), indBody);
                break;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_3, P_ERROR, 0, "atecSrvSocSendInd APPL_ECSRVSOC_CREATE_TCP_LISTEN_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSRVSOC_SERVER_ACCEPT_CLIENT_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSRVSOC, primId, sizeof(EcSrvSocTcpAcceptClientReaultInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_4, P_ERROR, 0, "atecSrvSocSendInd APPL_ECSRVSOC_SERVER_ACCEPT_CLIENT_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSRVSOC_STATUS_TCP_LISTEN_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSRVSOC, primId, sizeof(EcSrvSocTcpListenStatusInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_5, P_ERROR, 0, "atecSrvSocSendInd APPL_ECSRVSOC_STATUS_TCP_LISTEN_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSRVSOC_RECEIVE_TCP_CLIENT_IND:
        {
            EcSrvSocTcpClientReceiveInd *srvSocTcpClientReceiveInd =(EcSrvSocTcpClientReceiveInd *)indBody;
            if(srvSocTcpClientReceiveInd)
            {
                applSendCmsInd(reqSource, APPL_ECSRVSOC, primId, sizeof(EcSrvSocTcpClientReceiveInd) + srvSocTcpClientReceiveInd->length, indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_6, P_ERROR, 0, "atecSrvSocSendInd APPL_ECSRVSOC_RECEIVE_TCP_CLIENT_IND invalid indBody");
            }
            break;
        }
        case APPL_ECSRVSOC_CLOSE_TCP_CONNECTION_IND:
        {
            if(indBody)
            {
                applSendCmsInd(reqSource, APPL_ECSRVSOC, primId, sizeof(EcSrvSocCloseInd), indBody);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_7, P_ERROR, 0, "atecSrvSocSendInd APPL_ECSRVSOC_CLOSE_TCP_CONNECTION_IND invalid indBody");
            }
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocSendInd_8, P_ERROR, 1, "atecSrvSocSendInd invalid primId %d", primId);
            break;
        }
    }

    return;

}


static void nmSocketNetworkErrorInd(UINT16 reqSource, int fd, int err)
{
    CHAR RespBuf[32];

    memset(RespBuf, 0, sizeof(RespBuf));
    snprintf(RespBuf, sizeof(RespBuf), "+SKTERR: %d,%d", fd, err);
    atecSktSendInd(reqSource, APPL_SOCKET_ERROR_IND, strlen(RespBuf)+1, RespBuf);
}

/****************************************************************************************
*  display receive data from SOCKET
*
****************************************************************************************/
static void nmSocketNetworkRecvInd(UINT16 reqSource, int fd, CmsSockMgrDataContext *dataContext)
{

    AtecSktDlInd *dlInd;

    if(dataContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSocketNetworkRecvInd_1, P_ERROR, 0, "nmSocketNetworkRecvInd invalid buf or len");
        return;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSocketNetworkRecvInd_2, P_INFO, 2, "nmSocketNetworkRecvInd fd %d, len %d", fd, dataContext->Length);
    }

    if(sizeof(EcSocNMInd) > CMS_SOCK_MGR_DATA_HEADER_LENGTH_MAX)
    {
        dlInd = (AtecSktDlInd *)malloc(sizeof(AtecSktDlInd) + dataContext->Length);
        if(dlInd == NULL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSocketNetworkRecvInd_3, P_ERROR, 0, "nmSocketNetworkRecvInd malloc fail");
            return;
        }
        else
        {
            dlInd->fd = fd;
            dlInd->len = dataContext->Length;
            memcpy(dlInd->data, dataContext->data, dataContext->Length);
            atecSktSendInd(reqSource, APPL_SOCKET_RCV_IND, sizeof(AtecSktDlInd) + dataContext->Length, dlInd);
            free(dlInd);
        }
    }
    else
    {
        dlInd = (AtecSktDlInd *)((UINT8 *)dataContext->data - (UINT32)&(((AtecSktDlInd *)0)->data));
        if(dlInd == NULL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, nmSocketNetworkRecvInd_4, P_ERROR, 0, "AtecEcSocProcessDlData dlNMI point is invalid");
            return;
        }
        else
        {
            dlInd->fd = fd;
            dlInd->len = dataContext->Length;
            atecSktSendInd(reqSource, APPL_SOCKET_RCV_IND, sizeof(AtecSktDlInd) + dataContext->Length, dlInd);
        }
    }

}

static UINT32 atecSocketHandleSendReq(AtecSktSendReq *sktSendReq)
{
    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;

    if(sktSendReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_1, P_ERROR, 0, "AtecSocketHandleSendReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(sktSendReq->fd);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_2, P_INFO, 0, "AtecSocketHandleSendReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source != SOCK_SOURCE_ATSKT)
        {
           ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_3, P_ERROR, 1, "AtecSocketHandleSendReq the seq socket %d is not atskt", sktSendReq->fd);
           ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else if(sockMgrContext->status != SOCK_CONN_STATUS_CONNECTED)
        {
           ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_4, P_ERROR, 1, "AtecSocketHandleSendReq the seq socket %d is not connected", sktSendReq->fd);
           ret = CME_SOCK_MGR_ERROR_NO_CONNECTED;
        }
        else
        {
            if(sktSendReq->sendLen > 0)
            {
                if(ps_send(sockMgrContext->sockId, sktSendReq->data, sktSendReq->sendLen, 0, sktSendReq->dataRai, sktSendReq->dataExpect) == sktSendReq->sendLen)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_5, P_INFO, 1, "AtecSocketHandleSendReq send packet success %u", sktSendReq->sendLen);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_6, P_ERROR, 0, "AtecSocketHandleSendReq send fail");
                    ret = CME_SOCK_MGR_ERROR_SEND_FAIL;
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleSendReq_7, P_ERROR, 0, "AtecSocketHandleSendReq invalid sendLen param");
                ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
            }
        }
    }

    return ret;

}

static UINT32 atecSocketHandleCreateReq(AtecSktCreateReq *sktCreateReq, INT32 *fd)
{

    CmsSockMgrContext* sockMgrContext = NULL;
    AtecSktPriMgrContext *priMgrcontext;
    UINT32 ret = CME_SUCC;

    if(sktCreateReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleCreateReq_1, P_ERROR, 0, "AtecSocketHandleCreateReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(AtecSktPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleCreateReq_2, P_ERROR, 0, "AtecSocketHandleCreateReq atskt is full");
        return CME_SOCK_MGR_ERROR_TOO_MUCH_INST;
    }

    sockMgrContext->sockId = cmsSockMgrCreateSocket(sktCreateReq->domain, sktCreateReq->type, sktCreateReq->protocol, -1);
    if(sockMgrContext->sockId < 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleCreateReq_3, P_ERROR, 0, "AtecSocketHandleCreateReq create socket fail");
        cmsSockMgrRemoveContext(sockMgrContext);
        return CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
    }

    cmsSockMgrEnableHibMode(sockMgrContext);

    priMgrcontext = (AtecSktPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    priMgrcontext->createReqHandle = sktCreateReq->reqSource;

    if(sktCreateReq->type == SOCK_DGRAM_TYPE)
    {
        sockMgrContext->type = SOCK_CONN_TYPE_UDP;
    }
    else if(sktCreateReq->type == SOCK_STREAM_TYPE)
    {
        sockMgrContext->type = SOCK_CONN_TYPE_TCP;
    }
    sockMgrContext->domain = sktCreateReq->domain;
    sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
    sockMgrContext->source = SOCK_SOURCE_ATSKT;
    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)sktCreateReq->eventCallback;

    if(fd)
    {
        *fd = sockMgrContext->sockId;
    }

    return ret;
}


static UINT32 atecSocketHandleBindReq(AtecSktBindReq *sktBindReq)
{
    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    AtecSktPriMgrContext *priMgrcontext;

    if(sktBindReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleBindReq_1, P_ERROR, 0, "AtecSocketHandleBindReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(sktBindReq->fd);
    if(sktBindReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleBindReq_2, P_INFO, 0, "AtecSocketHandleBindReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        priMgrcontext = (AtecSktPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

        if(sockMgrContext->source == SOCK_SOURCE_ATSKT)
        {
            ret = cmsSockMgrBindSocket(sktBindReq->fd, sockMgrContext->domain, sktBindReq->localPort, &sktBindReq->localAddr);
            if(ret == CME_SUCC)
            {
                sockMgrContext->localPort = sktBindReq->localPort;
                ip_addr_copy(sockMgrContext->localAddr, sktBindReq->localAddr);
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleBindReq_4, P_ERROR, 1, "AtecSocketHandleBindReq the socket fd %d is not atskt", sktBindReq->fd);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
    }

    return ret;

}

static UINT32 atecSocketHandleConnectReq(AtecSktConnectReq *sktConnectReq)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    AtecSktPriMgrContext *priMgrcontext;

    if(sktConnectReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_1, P_ERROR, 0, "AtecSocketHandleConnectReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(sktConnectReq->fd);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_2, P_INFO, 0, "AtecSocketHandleConnectReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        priMgrcontext = (AtecSktPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

        if(sockMgrContext->source != SOCK_SOURCE_ATSKT)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_4, P_ERROR, 1, "AtecSocketHandleConnectReq the seq socket %d is not atskt", sktConnectReq->fd);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else if(sockMgrContext->status != SOCK_CONN_STATUS_CREATED)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_5, P_ERROR, 1, "AtecSocketHandleConnectReq the seq socket %d is not init and type is not dgram", sktConnectReq->fd);
            ret = CME_SOCK_MGR_ERROR_INVALID_STATUS;
        }
        else
        {
            ret = cmsSockMgrConnectSocket(sktConnectReq->fd, sockMgrContext->domain, sktConnectReq->remotePort, &sktConnectReq->remoteAddr, FALSE);

            if(ret == CME_SUCC)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_6, P_INFO, 0, "AtecSocketHandleConnectReq connect success");
                sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
            }
            else if(ret == CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING)
            {
                sockMgrContext->status = SOCK_CONN_STATUS_CONNECTING;
                priMgrcontext->connectReqHandle = sktConnectReq->reqSource;
                sockMgrContext->connectStartTicks = cmsSockMgrGetCurrHibTicks();
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_7, P_INFO, 2, "AtecSocketHandleConnectReq connect is oning,fd %d,current tick %u", sktConnectReq->fd, sockMgrContext->connectStartTicks);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleConnectReq_8, P_ERROR, 0, "AtecSocketHandleConnectReq connect fail");
            }
        }
    }

    return ret;

}

static UINT32 atecSocketHandleDeleteReq(AtecSktDeleteReq *sktDeleteReq)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;


    if(sktDeleteReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleDeleteReq_1, P_ERROR, 0, "AtecSocketHandleDeleteReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(sktDeleteReq->fd);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleDeleteReq_2, P_INFO, 0, "AtecSocketHandleDeleteReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source == SOCK_SOURCE_ATSKT)
        {
            ret = cmsSockMgrCloseSocket(sktDeleteReq->fd);
            if(ret == CME_SUCC)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleDeleteReq_3, P_INFO, 0, "AtecSocketHandleDeleteReq close socket success");
                cmsSockMgrRemoveContextById(sktDeleteReq->fd);
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleDeleteReq_4, P_ERROR, 1, "AtecSocketHandleDeleteReq socket fd %d is not atskt", sktDeleteReq->fd);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
    }

    return ret;
}


static UINT32 atecSocketHandleStatusReq(AtecSktStatusReq *sktStatusReq, INT32 *status)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;

    if(sktStatusReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleStatusReq_1, P_ERROR, 0, "AtecSocketHandleStatusReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(sktStatusReq->fd);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleStatusReq_2, P_INFO, 0, "AtecSocketHandleStatusReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source == SOCK_SOURCE_ATSKT)
        {
            if(status)
            {
                *status = sockMgrContext->status;
            }
            else
            {
                ret = CME_SOCK_MGR_ERROR_UNKNOWN;
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleStatusReq_3, P_ERROR, 1, "AtecSocketHandleStatusReq socket %d is not atskt", sktStatusReq->fd);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
    }

    return ret;
}

static void atecEcSocAddPublicDlPkgNumUsage(UINT8 num)
{
    gecSocPubDlSet.publicDlPkgNumTotalUsage += num;
}

static void atecEcSocReducePublicDlPkgNumUsage(UINT8 num)
{
    gecSocPubDlSet.publicDlPkgNumTotalUsage -= num;
}

static void atecEcSocAddPublicDlBufferUsage(UINT16 len)
{
    gecSocPubDlSet.publicDlBufferTotalUsage += len;
}

static void atecEcSocReducePublicDlBufferUsage(UINT16 len)
{
    gecSocPubDlSet.publicDlBufferTotalUsage -= len;
}

static void atecEcSocModifyPublicDlBufferTotalSize(UINT16 len)
{
    if(len == AT_SOC_PUBLIC_DL_BUFFER_IGNORE)
    {
        return;
    }
    gecSocPubDlSet.publicDlBufferToalSize = len;
}

static void atecEcSocModifyPublicDlPkgNumMax(UINT8 num)
{
    if(num == AT_SOC_PUBLIC_DL_PKG_NUM_IGNORE)
    {
        return;
    }
    gecSocPubDlSet.publicDlPkgNumMax = num;
}

static UINT8 atecEcSocGetPublicDlPkgNumUsage(void)
{
    return gecSocPubDlSet.publicDlPkgNumTotalUsage;
}

static UINT8 atecEcSocGetPublicDlPkgNumMax(void)
{
    return gecSocPubDlSet.publicDlPkgNumMax;
}

static UINT16 atecEcSocGetPublicDlBufferUsage(void)
{
    return gecSocPubDlSet.publicDlBufferTotalUsage;
}

static UINT16 atecEcSocGetPublicDlBufferTotalSize(void)
{
    return gecSocPubDlSet.publicDlBufferToalSize;
}

static UINT8 atecEcSocGetPublicMode(void)
{
    return gecSocPubDlSet.mode;
}

static UINT32 atecSocGetNMInfo(INT32 socketId, EcSocNMInd **socNMInd)
{
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    EcSocNMInd *socNMIndTmp;
    UINT32 ret = CME_SUCC;

    if(socNMInd == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_1, P_ERROR, 0, "AtecSocGetNMInfo parameter invalid");
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(socketId);

    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_2, P_ERROR, 0, "AtecSocGetNMInfo socketId not found");
        return CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }

    if(sockMgrContext->source != SOCK_SOURCE_ECSOC)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_3, P_ERROR, 1, "AtecSocketHandleEcSocReadReq socket %d is not ecsoc", sockMgrContext->sockId);
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    switch(priMgrcontext->modeSet.mode)
    {
        case NMI_MODE_0:
        {
            *socNMInd = NULL;
            break;
        }
        case NMI_MODE_1:
        {
            if(priMgrcontext->dlList == NULL)
            {
                *socNMInd = NULL;
            }
            else
            {
                socNMIndTmp = (EcSocNMInd *)malloc(sizeof(EcSocNMInd));
                if(socNMIndTmp == NULL)
                {
                    ret = CME_SOCK_MGR_ERROR_NO_MEMORY;
                    *socNMInd = NULL;
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_4, P_ERROR, 0, "AtecSocGetNMInfo malloc fail");
                }
                else
                {
                     socNMIndTmp->length = priMgrcontext->dlList->length;
                     socNMIndTmp->modeNMI = priMgrcontext->modeSet.mode;
                     socNMIndTmp->socketId = sockMgrContext->sockId;
                     socNMIndTmp->remotePort = priMgrcontext->dlList->remotePort;
                     memcpy(&socNMIndTmp->remoteAddr, &(priMgrcontext->dlList->remoteAddr), sizeof(ip_addr_t));
                     *socNMInd = socNMIndTmp;
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_5, P_INFO, 0, "AtecSocGetNMInfo get mode 1 NMI info");
                }
            }
            break;
        }
        case NMI_MODE_2:
        case NMI_MODE_3:
        {
            if(priMgrcontext->dlList == NULL)
            {
                *socNMInd = NULL;
            }
            else
            {
                socNMIndTmp = (EcSocNMInd *)malloc(sizeof(EcSocNMInd) + priMgrcontext->dlList->length);
                if(socNMIndTmp == NULL)
                {
                    ret = CME_SOCK_MGR_ERROR_NO_MEMORY;
                    *socNMInd = NULL;
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_6, P_ERROR, 0, "AtecSocGetNMInfo malloc fail");
                }
                else
                {
                     socNMIndTmp->length = priMgrcontext->dlList->length;
                     socNMIndTmp->modeNMI = priMgrcontext->modeSet.mode;
                     socNMIndTmp->socketId = sockMgrContext->sockId;
                     socNMIndTmp->remotePort = priMgrcontext->dlList->remotePort;
                     memcpy(&socNMIndTmp->remoteAddr, &(priMgrcontext->dlList->remoteAddr), sizeof(ip_addr_t));
                     memcpy(socNMIndTmp->data, priMgrcontext->dlList->data + priMgrcontext->dlList->offSet, priMgrcontext->dlList->length);
                     *socNMInd = socNMIndTmp;
                     ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocGetNMInfo_7, P_INFO, 0, "AtecSocGetNMInfo get mode 2 or 3 NMI info");
                }
            }
            break;
        }
    }

    return ret;
}

static void atecEcSocRemoveUlList(EcsocPriMgrContext *priMgrcontext)
{
    EcSocUlList *ulListTmp = NULL;
    EcSocUlList *ulListNext = NULL;
    EcSocUlBuffer *ulDataTmp = NULL;
    EcSocUlBuffer *ulDataNext = NULL;

    if(priMgrcontext == NULL)
    {
        return;
    }

    ulListTmp = priMgrcontext->ulList;
    while(ulListTmp != NULL)
    {
        ulDataTmp = ulListTmp->ulData;
        while( ulDataTmp != NULL)
        {
            ulDataNext = ulDataTmp->next;
            free(ulDataTmp);
            ulDataTmp = ulDataNext;
        }
        ulListNext = ulListTmp->next;
        free(ulListTmp);
        ulListTmp = ulListNext;
    }

    return;
}

static void atecEcSocConfigPublicMode(UINT8 mode)
{
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;


    if(mode == AT_SOC_NOTIFY_MODE_IGNORE)
    {
        return;
    }

    for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != NULL; sockMgrContext = sockMgrContext->next)
    {
        if(sockMgrContext->source == SOCK_SOURCE_ECSOC)
        {
            priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
            GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

            if(priMgrcontext->modeSet.flag == MODE_PUBLIC)
            {
                priMgrcontext->modeSet.mode = mode;
            }
        }
    }

    gecSocPubDlSet.mode = mode;

    return;
}

static void atecEcSocRemoveDlList(EcsocPriMgrContext *priMgrcontext)
{
    EcSocDlBufferList *dlList;
    EcSocDlBufferList *dlListNext;

    if(priMgrcontext == NULL)
    {
        return;
    }

    dlList = priMgrcontext->dlList;
    while( dlList != NULL)
    {
        dlListNext = dlList->next;
        if(dlList->isPrivate == TRUE)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocRemoveDlList_1, P_INFO, 1, "AtecEcSocRemoveDlList reduce DL private usage buffer %u", dlList->totalLen);
            priMgrcontext->dlPriSet.privateDlBufferTotalUsage -= dlList->totalLen;
            priMgrcontext->dlPriSet.privateDlPkgNumTotalUsage --;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocRemoveDlList_2, P_INFO, 1, "AtecEcSocRemoveDlList reduce DL public usage buffer %u", dlList->totalLen);
            atecEcSocReducePublicDlBufferUsage(dlList->totalLen);
            atecEcSocReducePublicDlPkgNumUsage(1);
        }
        free(dlList);
        dlList = dlListNext;
    }

    return;
}

static void atecEcSocAddDlList(EcSocDlBufferList *dlList, EcSocDlBufferList *dl)
{
    EcSocDlBufferList *dlListPre;
    EcSocDlBufferList *dlListTmp;

    if(dlList == NULL || dl == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocAddDlList_1, P_ERROR, 0, "AtecEcSocAddDlList parameter invalid");
        return;
    }

    for(dlListTmp = dlList, dlListPre = dlList; dlListTmp;)
    {
        dlListPre = dlListTmp;
        dlListTmp = dlListTmp->next;
    }

    if(dlListPre)
    {
        dlListPre->next = dl;
        dl->next = NULL;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocAddDlList_2, P_ERROR, 0, "AtecEcSocAddDlList parameter invalid");
    }

    return;
}

static void atecSocSendUlStatusInd(UINT32 reqSource, INT32 socketId, UINT8 sequence, UINT8 status)
{
    EcSocUlStatusInd socUlStatusInd;

    if(socketId < 0 || sequence == 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendUlStatusInd_1, P_INFO, 0, "AtecSocSendUlStatusInd parameter invalid,no need send indicate");
        return;
    }

    socUlStatusInd.socketId = socketId;
    socUlStatusInd.sequence = sequence;
    socUlStatusInd.status = status;

    atecSocSendInd(reqSource, APPL_ECSOC_ULSTATUS_IND, sizeof(EcSocUlStatusInd), &socUlStatusInd);

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocSendUlStatusInd_2, P_INFO, 3, "AtecSocSendUlStatusInd send UL status indicate socketid %u, sequence %u, status %u", socketId, sequence, status);
}

static UINT32 atecEcSocAddDlContext(CmsSockMgrContext *mgrContext, CmsSockMgrDataContext *dataContext, ip_addr_t *remoteAddr, UINT16 remotePort)
{
    EcSocDlBufferList *dlListTmp = NULL;
    int ret = CME_SUCC;
    EcsocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || dataContext == NULL || remoteAddr == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecEcSocAddDlContext_1, P_ERROR, 0, "atecEcSocAddDlContext parameter invalid");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    dlListTmp = (EcSocDlBufferList *)malloc(sizeof(EcSocDlBufferList) + dataContext->Length);
    if(dlListTmp == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecEcSocAddDlContext_2, P_ERROR, 0, "atecEcSocAddDlContext malloc DL buffer fail");
        return CME_SOCK_MGR_ERROR_NO_MEMORY;
    }
    else
    {
        dlListTmp->next = NULL;
        dlListTmp->length = dataContext->Length;
        dlListTmp->offSet = 0;
        dlListTmp->totalLen = dataContext->Length;
        dlListTmp->remotePort = remotePort;
        memcpy(&dlListTmp->remoteAddr, remoteAddr, sizeof(ip_addr_t));
        memcpy(dlListTmp->data, dataContext->data, dataContext->Length);

        priMgrcontext->dlTotalLen += dataContext->Length;
        if(priMgrcontext->modeSet.flag == MODE_PUBLIC)
        {
            dlListTmp->isPrivate = FALSE;
            atecEcSocAddPublicDlPkgNumUsage(1);
            atecEcSocAddPublicDlBufferUsage(dataContext->Length);
        }
        else
        {
            dlListTmp->isPrivate = TRUE;
            priMgrcontext->dlPriSet.privateDlBufferTotalUsage += dataContext->Length;
            priMgrcontext->dlPriSet.privateDlPkgNumTotalUsage ++;
        }

        if(priMgrcontext->dlList != NULL)
        {
            atecEcSocAddDlList(priMgrcontext->dlList, dlListTmp);
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecEcSocAddDlContext_3, P_SIG, 1, "atecEcSocAddDlContext add dl packet to DL list %u", mgrContext->sockId);
        }
        else
        {
            priMgrcontext->dlList = dlListTmp;
        }
    }

    return ret;

}

static UINT32 atecEcSocProcessDlData(CmsSockMgrContext *mgrContext, CmsSockMgrDataContext *dataContext, ip_addr_t *remoteAddr, UINT16 remotePort)
{
    EcSocDlBufferList *dlListTmp = NULL;
    EcSocNMInd *socNMInd = NULL;
    int ret = CME_SUCC;
    EcsocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || dataContext == NULL || remoteAddr == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_1, P_ERROR, 0, "AtecEcSocProcessDlData parameter invalid");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    //case 1: DL list is not NULL,insert DL list

    if(priMgrcontext->modeSet.flag == MODE_PUBLIC)
    {
        if((atecEcSocGetPublicDlBufferUsage() + dataContext->Length > atecEcSocGetPublicDlBufferTotalSize()) || (atecEcSocGetPublicDlPkgNumUsage() + 1 > atecEcSocGetPublicDlPkgNumMax()))
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_2, P_WARNING, 2, "AtecEcSocProcessDlData reach public DL resource, has use %u or has use num %u", atecEcSocGetPublicDlBufferUsage(), atecEcSocGetPublicDlPkgNumUsage());
            return SOCKET_NO_MORE_DL_BUFFER_RESOURCE;
        }
    }
     else
    {
       if((priMgrcontext->dlPriSet.privateDlBufferTotalUsage + dataContext->Length > priMgrcontext->dlPriSet.privateDlBufferToalSize) || (priMgrcontext->dlPriSet.privateDlPkgNumTotalUsage +1 > priMgrcontext->dlPriSet.privateDlPkgNumMax))
       {
           ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_3, P_WARNING, 0, "AtecEcSocProcessDlData reach private DL buffer size or num max,discard this packet");
           return SOCKET_NO_MORE_DL_BUFFER_RESOURCE;
        }
    }

    //if receive control flag is zero, just add data to DL list
    if(priMgrcontext->modeSet.receiveControl == RCV_DISABLE)
    {
        return atecEcSocAddDlContext(mgrContext, dataContext, remoteAddr, remotePort);
    }

    switch(priMgrcontext->modeSet.mode)
    {
        case NMI_MODE_0:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_4, P_INFO, 0, "AtecEcSocProcessDlData modset is mode 0");
            ret = atecEcSocAddDlContext(mgrContext, dataContext, remoteAddr, remotePort);
            break;
        }
        case NMI_MODE_2:
        case NMI_MODE_3:
        {
            if(sizeof(EcSocNMInd) > CMS_SOCK_MGR_DATA_HEADER_LENGTH_MAX)
            {
                socNMInd = (EcSocNMInd *)malloc(sizeof(EcSocNMInd) + dataContext->Length);
                if(socNMInd == NULL)
                {
                    ret = CME_SOCK_MGR_ERROR_NO_MEMORY;
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_5, P_ERROR, 0, "AtecEcSocProcessDlData malloc NMI fail");
                }
                else
                {
                    socNMInd->length = dataContext->Length;
                    socNMInd->modeNMI = priMgrcontext->modeSet.mode;
                    socNMInd->socketId = mgrContext->sockId;
                    socNMInd->remotePort = remotePort;
                    memcpy(&socNMInd->remoteAddr, remoteAddr, sizeof(ip_addr_t));
                    memcpy(socNMInd->data, dataContext->data, dataContext->Length);
                    atecSocSendInd(priMgrcontext->createReqHandle, APPL_ECSOC_NMI_IND, sizeof(EcSocNMInd) + sizeof(socNMInd->length), socNMInd);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_6, P_SIG, 1, "AtecEcSocProcessDlData send a NMI indication %u, mode 1 or mode 2", mgrContext->sockId);
                    free(socNMInd);
                }
            }
            else
            {
                socNMInd = (EcSocNMInd *)((UINT8 *)dataContext->data - (UINT32)&(((EcSocNMInd *)0)->data));
                if(socNMInd == NULL)
                {
                    ret = CME_SOCK_MGR_ERROR_UNKNOWN;
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_7, P_ERROR, 0, "AtecEcSocProcessDlData malloc NMI fail");
                }
                else
                {
                    socNMInd->length = dataContext->Length;
                    socNMInd->modeNMI = priMgrcontext->modeSet.mode;
                    socNMInd->socketId = mgrContext->sockId;
                    socNMInd->remotePort = remotePort;
                    memcpy(&socNMInd->remoteAddr, remoteAddr, sizeof(ip_addr_t));
                    atecSocSendInd(priMgrcontext->createReqHandle, APPL_ECSOC_NMI_IND, sizeof(EcSocNMInd) + sizeof(socNMInd->length), socNMInd);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_8, P_SIG, 1, "AtecEcSocProcessDlData send a NMI indication %u, mode 1 or mode 2", mgrContext->sockId);
                }
            }
            break;
        }
        case NMI_MODE_1:
        {

                dlListTmp = (EcSocDlBufferList *)malloc(sizeof(EcSocDlBufferList) + dataContext->Length);
                if(dlListTmp == NULL)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_9, P_ERROR, 0, "AtecEcSocProcessDlData malloc DL buffer fail");
                    return CME_SOCK_MGR_ERROR_NO_MEMORY;
                }
                else
                {
                    dlListTmp->next = NULL;
                    dlListTmp->length = dataContext->Length;
                    dlListTmp->offSet = 0;
                    dlListTmp->totalLen = dataContext->Length;
                    dlListTmp->remotePort = remotePort;
                    memcpy(&dlListTmp->remoteAddr, remoteAddr, sizeof(ip_addr_t));
                    memcpy(dlListTmp->data, dataContext->data, dataContext->Length);

                    priMgrcontext->dlTotalLen += dataContext->Length;

                    if(priMgrcontext->modeSet.flag == MODE_PUBLIC)
                    {
                        dlListTmp->isPrivate = FALSE;
                        atecEcSocAddPublicDlPkgNumUsage(1);
                        atecEcSocAddPublicDlBufferUsage(dataContext->Length);
                    }
                    else
                    {
                        dlListTmp->isPrivate = TRUE;
                        priMgrcontext->dlPriSet.privateDlBufferTotalUsage += dataContext->Length;
                        priMgrcontext->dlPriSet.privateDlPkgNumTotalUsage ++;
                    }

                    if(priMgrcontext->dlList != NULL)
                    {
                        atecEcSocAddDlList(priMgrcontext->dlList, dlListTmp);
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_10, P_SIG, 1, "AtecEcSocProcessDlData add dl packet to DL list %u", mgrContext->sockId);
                    }
                    else
                    {
                        priMgrcontext->dlList = dlListTmp;

                        if(sizeof(EcSocNMInd) > CMS_SOCK_MGR_DATA_HEADER_LENGTH_MAX)
                        {
                            socNMInd = (EcSocNMInd *)malloc(sizeof(EcSocNMInd));
                            if(socNMInd == NULL)
                            {
                                ret = CME_SOCK_MGR_ERROR_NO_MEMORY;
                                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_11, P_ERROR, 0, "AtecEcSocProcessDlData malloc NMI fail");
                            }
                            else
                            {
                                socNMInd->length = dataContext->Length;
                                socNMInd->modeNMI = priMgrcontext->modeSet.mode;
                                socNMInd->socketId = mgrContext->sockId;
                                socNMInd->remotePort = remotePort;
                                memcpy(&socNMInd->remoteAddr, remoteAddr, sizeof(ip_addr_t));
                                atecSocSendInd(priMgrcontext->createReqHandle, APPL_ECSOC_NMI_IND, sizeof(EcSocNMInd) + sizeof(socNMInd->length), socNMInd);
                                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_12, P_SIG, 1, "AtecEcSocProcessDlData send a NMI indication %u, mode 1 or mode 2", mgrContext->sockId);
                                free(socNMInd);
                            }
                        }
                        else
                        {
                            socNMInd = (EcSocNMInd *)((UINT8 *)dataContext->data - (UINT32)&(((EcSocNMInd *)0)->data));
                            if(socNMInd == NULL)
                            {
                                ret = CME_SOCK_MGR_ERROR_UNKNOWN;
                                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_13, P_ERROR, 0, "AtecEcSocProcessDlData malloc NMI fail");
                            }
                            else
                            {
                                socNMInd->length = dataContext->Length;
                                socNMInd->modeNMI = priMgrcontext->modeSet.mode;
                                socNMInd->socketId = mgrContext->sockId;
                                socNMInd->remotePort = remotePort;
                                memcpy(&socNMInd->remoteAddr, remoteAddr, sizeof(ip_addr_t));
                                atecSocSendInd(priMgrcontext->createReqHandle, APPL_ECSOC_NMI_IND, sizeof(EcSocNMInd) + sizeof(socNMInd->length), socNMInd);
                                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_14, P_SIG, 1, "AtecEcSocProcessDlData send a NMI indication %u, mode 1 or mode 2", mgrContext->sockId);
                            }
                        }
                    }

               }
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessDlData_15, P_ERROR, 0, "AtecEcSocProcessDlData invalid mode set");
            break;
        }
    }
    return ret;
}

static UINT32 atecEcSocProcessUdpUlList(EcsocPriMgrContext *priMgrcontext, EcSocUdpSendReq *socSendReqIn, EcSocUdpSendReq **socSendReqOut)
{
    UINT32 ret = CME_SUCC;
    EcSocUdpSendReq *socSendReqTmp = NULL;
    EcSocUlList *ulListTmp = NULL;
    EcSocUlBuffer *ulDataTmp = NULL;
    EcSocUlList *ulListPre = NULL;

    //check parameter
    if(priMgrcontext == NULL || socSendReqIn == NULL || socSendReqOut == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_1, P_ERROR, 0, "AtecEcSocProcessUdpUlList parameter invalid");
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    *socSendReqOut = NULL;

    //case 1: the UDP UL req is not a segment
    if(socSendReqIn->segmentId == 0 || socSendReqIn->segmentNum == 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_2, P_INFO, 0, "AtecEcSocProcessUdpUlList the udp UL req is not a segment");
        //check the sequence first
        if(socSendReqIn->sequence != 0)
        {
            if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, socSendReqIn->sequence) == TRUE)
            {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_3, P_WARNING, 2, "AtecEcSocProcessUdpUlList the socket %d sequence %u is reusing ", socSendReqIn->socketId, socSendReqIn->sequence);
                    return CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID;
            }
        }

        *socSendReqOut = socSendReqIn;
        return ret;
    }
    else
    {
        ulListTmp = priMgrcontext->ulList;
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_4, P_INFO, 0, "AtecEcSocProcessUdpUlListfind the correct ullist");
        while(ulListTmp)
        {
            if(ulListTmp->sequence == socSendReqIn->sequence ) // find the correct ul list
            {
                if(ulListTmp->segmentNum == socSendReqIn->segmentNum)
                {
                    ulDataTmp = ulListTmp->ulData;
                    while(ulDataTmp)
                    {
                        if(ulDataTmp->segmentId == socSendReqIn->segmentId) //check the segment id
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_5, P_ERROR, 0, "AtecEcSocProcessUdpUlList repeat segmentId");
                            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
                        }
                        else
                        {
                            ulDataTmp = ulDataTmp->next;
                        }
                    }
                    break;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_6, P_ERROR, 0, "AtecEcSocProcessUdpUlList segmentNum not match");
                    return CME_SOCK_MGR_ERROR_PARAM_ERROR;
                }
            }
            else
            {
                ulListPre = ulListTmp;
                ulListTmp = ulListTmp->next;
            }

        }

        //case 2: the UDP UL req is a new segment
        if(ulListTmp == NULL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_7, P_INFO, 3, "AtecEcSocProcessUdpUlListfind can not find the adapt ullist, sequence %u, segmentind %u, segmentnum %u", socSendReqIn->sequence, socSendReqIn->segmentId, socSendReqIn->segmentNum);
            ulListTmp = (EcSocUlList *)malloc(sizeof(EcSocUlList));
            if(ulListTmp)
            {
                ulDataTmp = (EcSocUlBuffer *)malloc(sizeof(EcSocUlBuffer) + socSendReqIn->length);
                if(ulDataTmp)
                {
                    ulListTmp->segmentNum = socSendReqIn->segmentNum;
                    ulListTmp->sequence = socSendReqIn->sequence;
                    ulListTmp->remotePort = socSendReqIn->remotePort;
                    memcpy(&ulListTmp->remoteAddr, &socSendReqIn->remoteAddr,sizeof(ip_addr_t));
                    ulListTmp->ulData = ulDataTmp;
                    ulListTmp->segmentAlready = 1;

                    ulDataTmp->length = socSendReqIn->length;
                    ulDataTmp->segmentId = socSendReqIn->segmentId;
                    ulDataTmp->next = NULL;
                    memcpy(ulDataTmp->data, socSendReqIn->data, socSendReqIn->length);

                    ulListTmp->next = priMgrcontext->ulList;
                    priMgrcontext->ulList = ulListTmp;

                    *socSendReqOut = NULL;

                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_8, P_INFO, 3, "AtecEcSocProcessUdpUlList insert a new segment id %u, segment num %u, sequence %u", ulDataTmp->segmentId, ulListTmp->segmentNum, ulListTmp->sequence);
                    return ret;
                }
                else
                {
                    free(ulListTmp);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_9, P_ERROR, 0, "AtecEcSocProcessUdpUlList malloc UL data buffer fail");
                    return CME_SOCK_MGR_ERROR_NO_MEMORY;
                }
            }
            else
            {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_10, P_ERROR, 0, "AtecEcSocProcessUdpUlList malloc UL list fail");
                    return CME_SOCK_MGR_ERROR_NO_MEMORY;
            }

        }
        else //case 3: the UDP UL req is not a new segment
        {
            EcSocUlBuffer *ulDataTmpNext, *ulDataTmpPre;
            UINT16 lenTotal = 0;
            UINT8 *dataWithOffset;

            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_11, P_INFO, 3, "AtecEcSocProcessUdpUlListfind find the adapt ullist, sequence %u, segmentind %u, segmentnum %u", socSendReqIn->sequence, socSendReqIn->segmentId, socSendReqIn->segmentNum);

            //first check whether the remode info is adpat
            if(ulListTmp->remotePort != socSendReqIn->remotePort || !ip_addr_cmp(&ulListTmp->remoteAddr, &socSendReqIn->remoteAddr))
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_12, P_ERROR, 0, "AtecEcSocProcessUdpUlList remote info is not correct");
                return CME_SOCK_MGR_ERROR_PARAM_ERROR;
            }

            //case 3.1 :the UDP UL req is not the last segment
            if(ulListTmp->segmentAlready != ulListTmp->segmentNum - 1)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_13, P_INFO, 3, "AtecEcSocProcessUdpUlListfind find the adapt ullist and not the last segment, sequence %u, segmentind %u, segmentnum %u", socSendReqIn->sequence, socSendReqIn->segmentId, socSendReqIn->segmentNum);
                ulDataTmp = (EcSocUlBuffer *)malloc(sizeof(EcSocUlBuffer) + socSendReqIn->length);
                if(ulDataTmp)
                {
                    ulListTmp->segmentAlready ++;

                    ulDataTmp->length = socSendReqIn->length;
                    ulDataTmp->segmentId = socSendReqIn->segmentId;
                    ulDataTmp->next = NULL;
                    memcpy(ulDataTmp->data, socSendReqIn->data, socSendReqIn->length);

                    ulDataTmpPre = ulListTmp->ulData;
                    if(ulDataTmpPre->segmentId > ulDataTmp->segmentId)
                    {
                        ulDataTmp->next = ulDataTmpPre;
                        ulListTmp->ulData = ulDataTmp;
                    }
                    else
                    {
                        for(ulDataTmpNext = ulDataTmpPre; ulDataTmpNext; ulDataTmpNext = ulDataTmpNext->next)
                        {
                            if(ulDataTmpNext->segmentId > ulDataTmp->segmentId)
                            {
                                break;
                            }
                            else
                            {
                                ulDataTmpPre = ulDataTmpNext;
                            }
                        }

                        ulDataTmp->next = ulDataTmpNext;
                        ulDataTmpPre->next = ulDataTmp;
                    }

                    *socSendReqOut = NULL;

                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_14, P_INFO, 3, "AtecEcSocProcessUdpUlList insert a new segment id %u, segment num %u, sequence %u", ulDataTmp->segmentId, ulListTmp->segmentNum, ulListTmp->sequence);
                    return ret;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_15, P_ERROR, 0, "AtecEcSocProcessUdpUlList malloc ul data buffer fail");
                    return CME_SOCK_MGR_ERROR_NO_MEMORY;
                }
            }
            else //case 3.2 the UDP UL req is the last segemnt
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_16, P_INFO, 3, "AtecEcSocProcessUdpUlListfind find the adapt ullist and is the last segment, sequence %u, segmentind %u, segmentnum %u", socSendReqIn->sequence, socSendReqIn->segmentId, socSendReqIn->segmentNum);
                  //firt calculate the total packet size
                for(ulDataTmpPre = ulListTmp->ulData; ulDataTmpPre; ulDataTmpPre = ulDataTmpPre->next)
                {
                    lenTotal += ulDataTmpPre->length;
                }
                lenTotal += socSendReqIn->length;

                socSendReqTmp = (EcSocUdpSendReq *)malloc(sizeof(EcSocUdpSendReq) + lenTotal);
                if(socSendReqTmp)
                {
                    socSendReqTmp->length = lenTotal;
                    socSendReqTmp->raiInfo = socSendReqIn->raiInfo;
                    socSendReqTmp->exceptionFlag = socSendReqIn->exceptionFlag;
                    socSendReqTmp->sequence = socSendReqIn->sequence;
                    socSendReqTmp->remotePort = socSendReqIn->remotePort;
                    memcpy(&socSendReqTmp->remoteAddr, &socSendReqIn->remoteAddr, sizeof(ip_addr_t));
                    socSendReqTmp->segmentId = 0;
                    socSendReqTmp->segmentNum = 0;
                    socSendReqTmp->socketId = socSendReqIn->socketId;

                    //init new data
                    ulDataTmpPre = ulListTmp->ulData;
                    if(ulDataTmpPre->segmentId > socSendReqIn->segmentId)
                    {
                        dataWithOffset = socSendReqTmp->data;
                        memcpy(dataWithOffset, socSendReqIn->data, socSendReqIn->length);
                        dataWithOffset += socSendReqIn->length;

                        while(ulDataTmpPre)
                        {
                            memcpy(dataWithOffset, ulDataTmpPre->data, ulDataTmpPre->length);
                            dataWithOffset += ulDataTmpPre->length;
                            ulDataTmpNext = ulDataTmpPre->next;
                            free(ulDataTmpPre);
                            ulDataTmpPre = ulDataTmpNext;
                        }
                    }
                    else
                    {
                        dataWithOffset = socSendReqTmp->data;

                        while(ulDataTmpPre)
                        {
                            if(ulDataTmpPre->segmentId < socSendReqIn->segmentId)
                            {
                                memcpy(dataWithOffset, ulDataTmpPre->data, ulDataTmpPre->length);
                                dataWithOffset += ulDataTmpPre->length;
                                ulDataTmpNext = ulDataTmpPre->next;
                                free(ulDataTmpPre);
                                ulDataTmpPre = ulDataTmpNext;
                            }
                            else
                            {
                                break;
                            }
                        }

                        memcpy(dataWithOffset, socSendReqIn->data, socSendReqIn->length);
                        dataWithOffset += socSendReqIn->length;

                        while(ulDataTmpPre)
                        {
                            memcpy(dataWithOffset, ulDataTmpPre->data, ulDataTmpPre->length);
                            dataWithOffset += ulDataTmpPre->length;
                            ulDataTmpNext = ulDataTmpPre->next;
                            free(ulDataTmpPre);
                            ulDataTmpPre = ulDataTmpNext;
                        }
                    }

                    *socSendReqOut = socSendReqTmp;

                    //free the UL List
                    if(ulListPre)
                    {
                        ulListPre->next = ulListTmp->next;
                    }
                    else
                    {
                        priMgrcontext->ulList = ulListTmp->next;
                    }
                    free(ulListTmp);

                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecEcSocProcessUdpUlList_17, P_ERROR, 0, "AtecEcSocProcessUdpUlList malloc new UDP SEND REQ fail");
                    return CME_SOCK_MGR_ERROR_NO_MEMORY;
                }
            }
        }

    }

    return ret;
}

static UINT32 atecSocketHandleEcSocCreateReq(EcSocCreateReq *socCreateReq, EcSocCreateResponse *socCreateResponse)
{

    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    UINT32 ret = CME_SUCC;

    if(socCreateReq == NULL || socCreateResponse == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_1, P_ERROR, 0, "AtecSocketHandleEcSocCreateReq request parameter invalid");
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(EcsocPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_2, P_ERROR, 0, "AtecSocketHandleEcSocCreateReq atskt is full");
        return CME_SOCK_MGR_ERROR_TOO_MUCH_INST;
    }

    sockMgrContext->sockId = cmsSockMgrCreateSocket(socCreateReq->domain, socCreateReq->type, socCreateReq->protocol, -1);
    if(sockMgrContext->sockId < 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_3, P_ERROR, 0, "AtecSocketHandleEcSocCreateReq create socket fail");
        cmsSockMgrRemoveContext(sockMgrContext);
        return CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
    }

    cmsSockMgrEnableHibMode(sockMgrContext);

    if(socCreateReq->protocol == IPPROTO_UDP)
    {
        ret = cmsSockMgrBindSocket(sockMgrContext->sockId, socCreateReq->domain, socCreateReq->listenPort, &socCreateReq->localAddr);
        if(ret != CME_SUCC)
        {
            close(sockMgrContext->sockId);
            cmsSockMgrRemoveContext(sockMgrContext);
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_4, P_WARNING, 0, "AtecSocketHandleEcSocCreateReq bind fail");
            return CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
        }
        else
        {
            sockMgrContext->localPort = socCreateReq->listenPort;
            ip_addr_copy(sockMgrContext->localAddr, socCreateReq->localAddr);
            sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
         }
    }
    else if(socCreateReq->protocol == IPPROTO_TCP)
    {
        if(!ip_addr_isany_val(socCreateReq->localAddr) || socCreateReq->listenPort)
        {
            ret = cmsSockMgrBindSocket(sockMgrContext->sockId, socCreateReq->domain, socCreateReq->listenPort, &socCreateReq->localAddr);
            if(ret != CME_SUCC)
            {
                close(sockMgrContext->sockId);
                cmsSockMgrRemoveContext(sockMgrContext);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_5, P_WARNING, 0, "AtecSocketHandleEcSocCreateReq bind fail");
                return CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_6, P_INFO, 0, "AtecSocketHandleEcSocCreateReq bind sucess");
                sockMgrContext->localPort = socCreateReq->listenPort;
                ip_addr_copy(sockMgrContext->localAddr, socCreateReq->localAddr);
                sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
            }
        }
        else
        {
            sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCreateReq_7, P_WARNING, 0, "AtecSocketHandleEcSocCreateReq wrong protocol");
        close(sockMgrContext->sockId);
        cmsSockMgrRemoveContext(sockMgrContext);
        return CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
    }

    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    priMgrcontext->createReqHandle = socCreateReq->reqSource;
    priMgrcontext->modeSet.receiveControl = socCreateReq->receiveControl;
    priMgrcontext->modeSet.flag = MODE_PUBLIC;
    priMgrcontext->modeSet.mode = gecSocPubDlSet.mode;
    priMgrcontext->dlPriSet.privateDlBufferToalSize = AT_SOC_PRIVATE_DL_BUFFER_DEF;
    priMgrcontext->dlPriSet.privateDlPkgNumMax = AT_SOC_PRIVATE_DL_PKG_NUM_DEF;
    priMgrcontext->dlTotalLen = 0;

    if(socCreateReq->type == SOCK_DGRAM_TYPE)
    {
        sockMgrContext->type = SOCK_CONN_TYPE_UDP;
    }
    else if(socCreateReq->type == SOCK_STREAM_TYPE)
    {
        sockMgrContext->type = SOCK_CONN_TYPE_TCP;
    }
    sockMgrContext->domain = socCreateReq->domain;
    sockMgrContext->source = SOCK_SOURCE_ECSOC;
    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)socCreateReq->eventCallback;

    socCreateResponse->socketId = sockMgrContext->sockId;

    return ret;
}


static UINT32 atecSocketHandleEcSocUdpSendReq(EcSocUdpSendReq *socSendReq, EcSocUdpSendResponse *socUdpSendResponse, BOOL *bNeedUlInd)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    struct sockaddr_storage to;
    int sendLen = -1;
    EcSocUdpSendReq *socSendReqNew = NULL;
    *bNeedUlInd = FALSE;

    if(socSendReq == NULL || socUdpSendResponse == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_1, P_ERROR, 0, "AtecSocketHandleEcSocUdpSendReq invalid param");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    socUdpSendResponse->socketId = socSendReq->socketId;
    socUdpSendResponse->length = 0;

    if (IP_IS_V4(&socSendReq->remoteAddr))
    {
        struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
        to4->sin_len    = sizeof(struct sockaddr_in);
        to4->sin_family = AF_INET;
        to4->sin_port = htons(socSendReq->remotePort);
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(&socSendReq->remoteAddr));
    }
    else if(IP_IS_V6(&socSendReq->remoteAddr))
    {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6*)&to;
        to6->sin6_len    = sizeof(struct sockaddr_in6);
        to6->sin6_family = AF_INET6;
        to6->sin6_port = htons(socSendReq->remotePort);
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(&socSendReq->remoteAddr));
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_2, P_ERROR, 0, "AtecSocketHandleEcSocUdpSendReq remote info is not correct");
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(socSendReq->socketId);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_3, P_INFO, 0, "AtecSocketHandleEcSocUdpSendReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source == SOCK_SOURCE_ECSOC)
        {
            priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
            GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

            ret = atecEcSocProcessUdpUlList(priMgrcontext, socSendReq, &socSendReqNew);

            if(ret != CME_SUCC)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_5, P_ERROR, 1, "AtecSocketHandleSendReq the socket %d process UL list fail", socSendReq->socketId);
            }
            else
            {
                if(sockMgrContext->type != SOCK_CONN_TYPE_UDP)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_6, P_ERROR, 1, "AtecSocketHandleEcSocUdpSendReq the seq socket %d is not UDP socket", socSendReq->socketId);
                    ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
                }
                else
                {
                    if(socSendReqNew)
                    {
                        sendLen = ps_sendto(socSendReqNew->socketId, socSendReqNew->data, socSendReqNew->length, 0, (struct sockaddr*)&to, sizeof(to), socSendReqNew->raiInfo, socSendReqNew->exceptionFlag);
                        if(sendLen > 0)
                        {
                            socUdpSendResponse->length = socSendReqNew->length;
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_7, P_INFO, 1, "AtecSocketHandleEcSocUdpSendReq send packet success %u", socSendReqNew->length);
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_8, P_ERROR, 0, "AtecSocketHandleEcSocUdpSendReq send fail");
                            ret = CME_SOCK_MGR_ERROR_SEND_FAIL;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_9, P_ERROR, 0, "AtecSocketHandleEcSocUdpSendReq add UL list success");
                    }
                }
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocUdpSendReq_10, P_ERROR, 1, "AtecSocketHandleEcSocUdpSendReq socket %d is not ecsoc", socSendReq->socketId);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
    }


    //send UL status indicate
    if(socSendReqNew != NULL)
    {
        *bNeedUlInd = TRUE;
    }

    if(socSendReqNew && socSendReq != socSendReqNew)
    {
        free(socSendReqNew);
    }

    return ret;

}

static UINT32 atecSocketHandleEcSocQueryReq(EcSocQueryReq *socQueryReq)
{
    int i;
    UINT8 sequence;
    INT32 ret = CME_SUCC;
    EcSocQueryInd socQueryInd;
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;

    if(socQueryReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocQueryReq_1, P_ERROR, 0, "AtecSocketHandleEcSocQueryReq invalid param");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    if(socQueryReq->bQueryAll == TRUE)
    {

        for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != PNULL; sockMgrContext = sockMgrContext->next)
        {

            for(sequence = AT_SOC_UL_DATA_SEQUENCE_MIN; sequence <= AT_SOC_UL_DATA_SEQUENCE_MAX; sequence ++)
            {
               if(sockMgrContext != NULL && sockMgrContext->source == SOCK_SOURCE_ECSOC)
               {
                    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
                    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

                    if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, sequence) == TRUE)
                    {
                        socQueryInd.socketId = sockMgrContext->sockId;
                        socQueryInd.sequence = sequence;
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocQueryReq_2, P_INFO, 2, "AtecSocketHandleEcSocQueryReq senq query result indication id %u, sequence %u", socQueryReq->socketId[i], sequence);
                        atecSocSendInd(socQueryReq->reqSource, APPL_ECSOC_QUERY_RESULT_IND, sizeof(EcSocQueryInd), &socQueryInd);
                    }
                }

                if(sequence == AT_SOC_UL_DATA_SEQUENCE_MAX)
                {
                    break;
                }
            }
        }
    }
    else
    {

        for(i = 0; i < SUPPORT_MAX_SOCKET_NUM; i++)
        {
            if(socQueryReq->socketId[i] != AT_SOC_FD_DEF)
            {
                if((sockMgrContext = cmsSockMgrFindMgrContextById(socQueryReq->socketId[i])) == NULL)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocQueryReq_3, P_ERROR, 1, "AtecSocketHandleEcSocQueryReq can not find socket id %u", socQueryReq->socketId[i]);
                    ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
                    break;
                }

                for(sequence = AT_SOC_UL_DATA_SEQUENCE_MIN; sequence <= AT_SOC_UL_DATA_SEQUENCE_MAX; sequence ++)
                {
                    if(sockMgrContext != NULL && sockMgrContext->source == SOCK_SOURCE_ECSOC)
                    {
                        priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
                        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

                        if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, sequence) == TRUE)
                        {
                            socQueryInd.socketId = socQueryReq->socketId[i];
                            socQueryInd.sequence = sequence;
                            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocQueryReq_4, P_INFO, 2, "AtecSocketHandleEcSocQueryReq senq query result indication id %u, sequence %u", socQueryReq->socketId[i], sequence);
                            atecSocSendInd(socQueryReq->reqSource, APPL_ECSOC_QUERY_RESULT_IND, sizeof(EcSocQueryInd), &socQueryInd);
                        }
                    }

                    if(sequence == AT_SOC_UL_DATA_SEQUENCE_MAX)
                    {
                        break;
                    }
                }
            }
        }
    }

    return ret;
}


static UINT32 atecSocketHandleEcSocReadReq(EcSocReadReq *socReadReq, EcSocReadResponse *socReadResponse, BOOL *bNeedNMI)
{

    EcSocDlBufferList *dlbufferPre = NULL;
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;


    if(socReadReq == NULL || socReadResponse == NULL || bNeedNMI == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_1, P_ERROR, 0, "AtecSocketHandleEcSocReadReq invalid paramter");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    socReadResponse->socketId = socReadReq->socketId;
    *bNeedNMI = FALSE;

    sockMgrContext = cmsSockMgrFindMgrContextById(socReadReq->socketId);

    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_2, P_INFO, 0, "AtecSocketHandleEcSocReadReq can not get the socketatcmd");
        return CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source != SOCK_SOURCE_ECSOC)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_3, P_ERROR, 1, "AtecSocketHandleEcSocReadReq socket %d is not ecsoc", sockMgrContext->sockId);
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
    }

    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    //case 1: no dl pkg
    if(priMgrcontext->dlList == NULL)
    {
        socReadResponse->length = 0;
        socReadResponse->remainingLen = 0;
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_5, P_INFO, 0, "AtecSocketHandleEcSocReadReq the dl list is NULL");
    }
    else //case 2: exsit DL pkg
    {
        dlbufferPre = priMgrcontext->dlList;
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_6, P_INFO, 2, "AtecSocketHandleEcSocReadReq the first dl remaining len %u, and read %u", dlbufferPre->length, socReadReq->length);
        if(dlbufferPre->length > socReadReq->length) // case 2.1 the first DL pkg will be not read total
        {
            //calculate the remain dl total len
            priMgrcontext->dlTotalLen -= socReadReq->length;

            memcpy(socReadResponse->data, dlbufferPre->data + dlbufferPre->offSet, socReadReq->length);
            dlbufferPre->offSet += socReadReq->length;
            dlbufferPre->length -= socReadReq->length;
            socReadResponse->length = socReadReq->length;
            socReadResponse->remainingLen = priMgrcontext->dlTotalLen;
            socReadResponse->remotePort = dlbufferPre->remotePort;
            memcpy(&socReadResponse->remoteAddr, &dlbufferPre->remoteAddr, sizeof(ip_addr_t));

        }
        else // case 2.2 the first DL pkg will be read total
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_7, P_INFO, 2, "AtecSocketHandleEcSocReadReq the first dl remaining len %u, and read %u", dlbufferPre->length, socReadReq->length);

            //calculate the remain dl total len
            priMgrcontext->dlTotalLen -= dlbufferPre->length;

            memcpy(socReadResponse->data, dlbufferPre->data + dlbufferPre->offSet, dlbufferPre->length);
            socReadResponse->length = dlbufferPre->length;
            socReadResponse->remainingLen = priMgrcontext->dlTotalLen;
            socReadResponse->remotePort = dlbufferPre->remotePort;
            dlbufferPre->length = 0;
            dlbufferPre->offSet += dlbufferPre->length;

            memcpy(&socReadResponse->remoteAddr, &dlbufferPre->remoteAddr, sizeof(ip_addr_t));

            //remove the dl list
            priMgrcontext->dlList = dlbufferPre->next;
            if(dlbufferPre->isPrivate == FALSE) //modify public DL manager info
            {
                atecEcSocReducePublicDlBufferUsage(dlbufferPre->totalLen);
                atecEcSocReducePublicDlPkgNumUsage(1);
                free(dlbufferPre);
            }
            else   //modify priavet DL manager info
            {
                priMgrcontext->dlPriSet.privateDlBufferTotalUsage -= dlbufferPre->totalLen;
                priMgrcontext->dlPriSet.privateDlPkgNumTotalUsage --;
                free(dlbufferPre);
            }

            if(priMgrcontext->modeSet.mode > NMI_MODE_0 && priMgrcontext->modeSet.receiveControl == RCV_ENABLE)
            {
                if(priMgrcontext->dlList != NULL)
                {
                    *bNeedNMI = TRUE;
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_8, P_SIG, 0, "AtecSocketHandleEcSocReadReq the dl list is not null, need NMI");
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocReadReq_9, P_SIG, 0, "AtecSocketHandleEcSocReadReq the dl list is not null, but no need NMI");
            }

        }
    }

    return CME_SUCC;
}


static UINT32 atecSocketHandleEcSocTcpConnectReq(EcSocTcpConnectReq *socTcpConnectReq)
{
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    UINT32 ret = CME_SUCC;

    if(socTcpConnectReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_1, P_ERROR, 0, "AtecSocketHandleEcSocTcpConnectReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(socTcpConnectReq->socketId);
    if(sockMgrContext == NULL)
    {
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source != SOCK_SOURCE_ECSOC)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_2, P_ERROR, 1, "AtecSocketHandleEcSocTcpConnectReq socket %d is not ecsoc", sockMgrContext->sockId);
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else if(sockMgrContext->type != SOCK_CONN_TYPE_TCP)
        {
           ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_3, P_ERROR, 1, "AtecSocketHandleEcSocTcpConnectReq the seq socket %d is not init and type is not tcp socket", sockMgrContext->sockId);
           ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else
        {
            if(sockMgrContext->status != SOCK_CONN_STATUS_CREATED)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_4, P_ERROR, 1, "AtecSocketHandleEcSocTcpConnectReq the seq socket %d is not init and type is not init status", sockMgrContext->sockId);
                ret = CME_SOCK_MGR_ERROR_INVALID_STATUS;
            }
            else
            {
                priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
                GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

                ret = cmsSockMgrConnectSocket(socTcpConnectReq->socketId, sockMgrContext->domain, socTcpConnectReq->remotePort, &socTcpConnectReq->remoteAddr, FALSE);

                if(ret == CME_SUCC)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_6, P_INFO, 0, "AtecSocketHandleEcSocTcpConnectReq connect success");
                    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
                }
                else if(ret == CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING)
                {
                    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTING;
                    priMgrcontext->connectReqHandle = socTcpConnectReq->reqSource;
                    sockMgrContext->connectStartTicks = cmsSockMgrGetCurrHibTicks();
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_7, P_INFO, 2, "AtecSocketHandleEcSocTcpConnectReq connect is oning,fd %d,current tick %u", sockMgrContext->sockId, sockMgrContext->connectStartTicks);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpConnectReq_8, P_ERROR, 0, "AtecSocketHandleEcSocTcpConnectReq connect fail");
                }
            }
        }
    }

    return ret;

}

static UINT32 atecSocketHandleEcSocTcpSendReq(EcSocTcpSendReq *socTcpSendReq, EcSocTcpSendResponse *socTcpSendResponse)
{
    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    int sendLen = -1;

    if(socTcpSendReq == NULL || socTcpSendResponse == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_1, P_ERROR, 0, "AtecSocketHandleEcSocTcpSendReq invalid param");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    socTcpSendResponse->socketId = socTcpSendReq->socketId;
    socTcpSendResponse->length = 0;

    sockMgrContext = cmsSockMgrFindMgrContextById(socTcpSendReq->socketId);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_2, P_INFO, 0, "AtecSocketHandleEcSocTcpSendReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source != SOCK_SOURCE_ECSOC)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_3, P_WARNING, 1, "AtecSocketHandleEcSocTcpSendReq the socket %u is not ecsoc", socTcpSendReq->socketId);
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }

        priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

        //check the sequence first
        if(socTcpSendReq->sequence != 0)
        {
            if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, socTcpSendReq->sequence) == TRUE)
            {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_5, P_WARNING, 2, "AtecSocketHandleEcSocTcpSendReq the socket %u sequence %u is reusing ", socTcpSendReq->socketId, socTcpSendReq->sequence);
                    return CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID;
            }
        }

        if(sockMgrContext->type != SOCK_CONN_TYPE_TCP)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_6, P_ERROR, 1, "AtecSocketHandleEcSocTcpSendReq the seq socket %d is not TCP socket", socTcpSendReq->socketId);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else
        {
            if(sockMgrContext->status != SOCK_CONN_STATUS_CONNECTED)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_7, P_ERROR, 1, "AtecSocketHandleEcSocTcpSendReq the seq socket %d is not connected", socTcpSendReq->socketId);
                ret = CME_SOCK_MGR_ERROR_NO_CONNECTED;
            }
            else
            {
                if(socTcpSendReq->length > 0)
                {
                    sendLen = ps_send_with_sequence(sockMgrContext->sockId, socTcpSendReq->data, socTcpSendReq->length, 0, socTcpSendReq->raiInfo, socTcpSendReq->expectionFlag, socTcpSendReq->sequence);
                    if(sendLen > 0)
                    {
                        socTcpSendResponse->length = sendLen;
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_8, P_INFO, 1, "AtecSocketHandleEcSocTcpSendReq send packet success %u", socTcpSendReq->length);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_9, P_ERROR, 0, "AtecSocketHandleEcSocTcpSendReq send fail");
                        ret = CME_SOCK_MGR_ERROR_SEND_FAIL;
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocTcpSendReq_10, P_ERROR, 0, "AtecSocketHandleEcSocTcpSendReq invalid sendLen param");
                    ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
                }
            }
        }
    }

    if(ret == CME_SUCC)
    {
        if(socTcpSendReq->sequence > 0)
        {
            cmsSockMgrUpdateUlPendingSequenceBitMapState(priMgrcontext->ulSeqStatus.bitmap, socTcpSendReq->sequence, TRUE);
        }
    }

    return ret;

}

static void atecSocketHandleEcSocCloseSocket(UINT16 reqSource, CmsSockMgrContext *mgrContext)
{
    UINT8 sequence;
    EcsocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCloseSocket_1, P_ERROR, 0, "AtecSocketHandleEcSocCloseSocket request point is null");
        return ;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    close(mgrContext->sockId);

    atecEcSocRemoveUlList(priMgrcontext);
    atecEcSocRemoveDlList(priMgrcontext);

        // send the pending UL sequence status indicate
    for(sequence = AT_SOC_UL_DATA_SEQUENCE_MIN; sequence <= AT_SOC_UL_DATA_SEQUENCE_MAX; sequence ++)
    {
        if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, sequence) == TRUE)
        {

            atecSocSendUlStatusInd(reqSource, mgrContext->sockId, sequence, ATECECSOC_FAIL);

        }

        if(sequence == AT_SOC_UL_DATA_SEQUENCE_MAX)
        {
            break;
        }

    }
    cmsSockMgrRemoveContextById(mgrContext->sockId);

}


static UINT32 atecSocketHandleEcSocCloseReq(EcSocCloseReq *socCloseReq)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    UINT8 sequence;

    if(socCloseReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCloseReq_1, P_ERROR, 0, "AtecSocketHandleEcSocCloseReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(socCloseReq->socketId);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCloseReq_2, P_INFO, 0, "AtecSocketHandleEcSocCloseReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {
        if(sockMgrContext->source != SOCK_SOURCE_ECSOC)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCloseReq_3, P_ERROR, 1, "AtecSocketHandleEcSocCloseReq socket %d is not ecsoc", socCloseReq->socketId);
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }

        priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

        ret = cmsSockMgrCloseSocket(sockMgrContext->sockId);
        if(ret == CME_SUCC)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocCloseReq_5, P_INFO, 0, "AtecSocketHandleEcSocCloseReq close socket success");
            atecEcSocRemoveUlList(priMgrcontext);
            atecEcSocRemoveDlList(priMgrcontext);

            // send the pending UL sequence status indicate
            for(sequence = AT_SOC_UL_DATA_SEQUENCE_MIN; sequence <= AT_SOC_UL_DATA_SEQUENCE_MAX; sequence ++)
            {
                if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, sequence) == TRUE)
                {

                    atecSocSendUlStatusInd(socCloseReq->reqSource, socCloseReq->socketId, sequence, ATECECSOC_FAIL);

                }

                if(sequence == AT_SOC_UL_DATA_SEQUENCE_MAX)
                {
                    break;
                }

            }
            cmsSockMgrRemoveContext(sockMgrContext);
        }
    }

    return ret;
}

static UINT32 atecHandleEcSocNMIReq(EcSocNMIReq *socNMIReq)
{

    if(socNMIReq == NULL)
    {
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    atecEcSocConfigPublicMode(socNMIReq->mode);

    atecEcSocModifyPublicDlBufferTotalSize(socNMIReq->maxPublicDlBuffer);

    atecEcSocModifyPublicDlPkgNumMax(socNMIReq->maxPublicDlPkgNum);

#ifdef FEATURE_REF_AT_ENABLE
    mwSetAonEcSocPublicConfig(socNMIReq->mode, socNMIReq->maxPublicDlPkgNum, socNMIReq->maxPublicDlBuffer);
#else
    mwSetEcSocPublicConfig(socNMIReq->mode, socNMIReq->maxPublicDlPkgNum, socNMIReq->maxPublicDlBuffer);
#endif

    return 0;
}


static UINT32 atecHandleEcSocNMIEReq(EcSocNMIEReq *socNMIEReq)
{

    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;


    if(socNMIEReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecHandleEcSocNMIEReq_1, P_INFO, 0, "AtecHandleEcSocNMIEReq invalid request");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(socNMIEReq->socketId);

    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecHandleEcSocNMIEReq_2, P_INFO, 0, "AtecHandleEcSocNMIEReq can not get the socketatcmd");
        return CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }

    if(sockMgrContext->source != SOCK_SOURCE_ECSOC)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecHandleEcSocNMIEReq_3, P_ERROR, 1, "AtecSocketHandleEcSocCloseReq socket %d is not ecsoc", socNMIEReq->socketId);
        return CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(socNMIEReq->mode == AT_SOC_NOTIFY_MODE_PRIVATE_DISABLE)
    {
        priMgrcontext->modeSet.flag = MODE_PUBLIC;
        priMgrcontext->modeSet.mode =gecSocPubDlSet.mode;
        priMgrcontext->dlPriSet.privateDlBufferToalSize = AT_SOC_PRIVATE_DL_BUFFER_DEF;
        priMgrcontext->dlPriSet.privateDlPkgNumMax = AT_SOC_PRIVATE_DL_PKG_NUM_DEF;
    }
    else if(socNMIEReq->mode != AT_SOC_NOTIFY_MODE_IGNORE)
    {
        if(socNMIEReq->mode > NMI_MODE_3)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecHandleEcSocNMIEReq_5, P_ERROR, 1, "AtecHandleEcSocNMIEReq invalid mode %u", priMgrcontext->modeSet.mode);
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        priMgrcontext->modeSet.flag = MODE_PRIVATE;
        priMgrcontext->modeSet.mode = socNMIEReq->mode;
        if(socNMIEReq->maxPublicDlBuffer != AT_SOC_PRIVATE_DL_BUFFER_IGNORE)
        {
            priMgrcontext->dlPriSet.privateDlBufferToalSize = socNMIEReq->maxPublicDlBuffer;
        }
        if(socNMIEReq->maxPublicDlPkgNum != AT_SOC_PUBLIC_DL_PKG_NUM_IGNORE)
        {
            priMgrcontext->dlPriSet.privateDlPkgNumMax = socNMIEReq->maxPublicDlPkgNum;
        }
    }
    else
    {
        if(socNMIEReq->maxPublicDlBuffer != AT_SOC_PRIVATE_DL_BUFFER_IGNORE)
        {
            priMgrcontext->dlPriSet.privateDlBufferToalSize = socNMIEReq->maxPublicDlBuffer;
        }
        if(socNMIEReq->maxPublicDlPkgNum != AT_SOC_PUBLIC_DL_PKG_NUM_IGNORE)
        {
            priMgrcontext->dlPriSet.privateDlPkgNumMax = socNMIEReq->maxPublicDlPkgNum;
        }
    }

    return CME_SUCC;
}


static UINT32 atecSocketHandleEcSocGNMIEReq(UINT16 reqHandle)
{
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;

    EcSocGNMIEInd socGNMIEInd;


    for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != NULL; sockMgrContext = sockMgrContext->next)
    {
        if(sockMgrContext->source == SOCK_SOURCE_ECSOC)
        {
            priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
            GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

            if(priMgrcontext->modeSet.flag == MODE_PRIVATE)
            {
                socGNMIEInd.socketId = sockMgrContext->sockId;
                socGNMIEInd.mode = priMgrcontext->modeSet.mode;
                socGNMIEInd.maxDlBufferSize = priMgrcontext->dlPriSet.privateDlBufferToalSize;
                socGNMIEInd.maxDlPkgNum = priMgrcontext->dlPriSet.privateDlPkgNumMax;
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketHandleEcSocGNMIEReq_1, P_INFO, 2, "AtecSocketHandleEcSocGNMIEReq senq GNMIE ind,socket id %u, mode %d", sockMgrContext->sockId, priMgrcontext->modeSet.mode);
                atecSocSendInd(reqHandle, APPL_ECSOC_GNMIE_IND, sizeof(EcSocGNMIEInd), &socGNMIEInd);
            }
        }
    }

    return CME_SUCC;
}


static UINT32 atecSocketHandleEcSocStatusReq(EcSocStatusReq *socStatusReq)
{
    INT32 ret = CME_SUCC;
    EcSocStatusInd socStatusInd;
    CmsSockMgrContext* sockMgrContext = NULL;

    if(socStatusReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSocketHandleEcSocStatusReq_1, P_ERROR, 0, "atecSocketHandleEcSocStatusReq invalid param");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    if(socStatusReq->bQuryAll == FALSE)
    {
        if(socStatusReq->socketId < 0)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSocketHandleEcSocStatusReq_1, P_ERROR, 0, "atecSocketHandleEcSocStatusReq invalid param");
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else
        {
            if((sockMgrContext = cmsSockMgrFindMgrContextById(socStatusReq->socketId)) == NULL)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSocketHandleEcSocStatusReq_2, P_ERROR, 1, "atecSocketHandleEcSocStatusReq can not find socket id %u", socStatusReq->socketId);
                return CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
            }
            else
            {
                if(sockMgrContext->status == SOCK_CONNECTED)
                {
                    socStatusInd.socketId = socStatusReq->socketId;
                    socStatusInd.status = EC_SOCK_VALID;

                    //ToDo: backoff and flow control check
                }
                else
                {
                    socStatusInd.socketId = socStatusReq->socketId;
                    socStatusInd.status = EC_SOCK_INVALID;
                }

                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSocketHandleEcSocStatusReq_3, P_INFO, 2, "atecSocketHandleEcSocStatusReq senq status indication id %u, status %u", socStatusInd.socketId, socStatusInd.status);
                atecSocSendInd(socStatusReq->reqSource, APPL_ECSOC_STATUS_IND, sizeof(EcSocStatusInd), &socStatusInd);
            }
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSocketHandleEcSocStatusReq_4, P_INFO, 0, "atecSocketHandleEcSocStatusReq query all ec socket status");

        for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != PNULL; sockMgrContext = sockMgrContext->next)
        {
            if(sockMgrContext->source == SOCK_SOURCE_ECSOC)
            {
                if(sockMgrContext->status == SOCK_CONNECTED)
                {
                    socStatusInd.socketId = sockMgrContext->sockId;
                    socStatusInd.status = EC_SOCK_VALID;

                    //ToDo: backoff and flow control check
                }
                else
                {
                    socStatusInd.socketId = sockMgrContext->sockId;
                    socStatusInd.status = EC_SOCK_INVALID;
                }

                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSocketHandleEcSocStatusReq_5, P_INFO, 2, "atecSocketHandleEcSocStatusReq senq status indication id %u, status %u", socStatusInd.socketId, socStatusInd.status);
                atecSocSendInd(socStatusReq->reqSource, APPL_ECSOC_STATUS_IND, sizeof(EcSocStatusInd), &socStatusInd);
            }
        }
    }

    return ret;
}


void atEcsocSendConnectedInd(UINT32 reqSource, INT32 socketId)
{
    EcSocConnectedInd connectedInd;

    if(socketId < 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcsocSendConnectedInd_1, P_ERROR, 0, "atEcsocSendConnectedInd socketId invalid");
        return;
    }

    connectedInd.socketId = socketId;

    atecSocSendInd(reqSource, APPL_ECSOC_CONNECTED_IND, sizeof(EcSocConnectedInd), &connectedInd);
}

CmsSockMgrContext* atecSrvSocCheckTcpServerHasCreate(ip_addr_t *bindAddr, UINT16 listenPort, INT32 domain)
{
    CmsSockMgrContext* sockMgrContext = PNULL;
    EcSrvSocPriMgrContext *priMgrcontext;

    if(bindAddr == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocCheckTcpServerHasCreate_1, P_ERROR, 0, "atecSrvSocCheckTcpServerHasCreate bind address is null");
        return PNULL;
    }

    for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != PNULL; sockMgrContext = sockMgrContext->next)
    {
        if(sockMgrContext->source == SOCK_SOURCE_ECSRVSOC && sockMgrContext->type == SOCK_CONN_TYPE_TCP)
        {
            priMgrcontext = (EcSrvSocPriMgrContext *)sockMgrContext->priContext;
            GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

            if(sockMgrContext->bServer == TRUE)
            {
                if((listenPort == priMgrcontext->listenPort) && ip_addr_cmp(bindAddr, &priMgrcontext->bindAddr) && (domain == sockMgrContext->domain))
                {
                    return sockMgrContext;
                }
            }
        }
    }

    return PNULL;
}


static void atecSrvSocCloseAllTcpCLient(INT32 fSocket)
{
    CmsSockMgrContext* sockMgrContext = NULL;
    CmsSockMgrContext* sockMgrContextTmp = NULL;    
    EcSrvSocPriMgrContext *priMgrcontext;

    for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != PNULL;)
    {
        if(sockMgrContext->source == SOCK_SOURCE_ECSRVSOC && sockMgrContext->bServer == FALSE)
        {
            priMgrcontext = (EcSrvSocPriMgrContext *)sockMgrContext->priContext;
            GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

            if(priMgrcontext->fSocketId == fSocket)
            {
                cmsSockMgrCloseSocket(sockMgrContext->sockId);
                sockMgrContextTmp = sockMgrContext->next;
                cmsSockMgrRemoveContext(sockMgrContext);
                sockMgrContext = sockMgrContextTmp;
            }
            else
            {
                sockMgrContext = sockMgrContext->next;
            }
        }
        else
        {
            sockMgrContext = sockMgrContext->next;
        }
    }    
}


static UINT32 atecSrvSocHandleCreateTcpListenReq(EcSrvSocCreateTcpReq *srvSocCreateTcpReq, INT32 *fd)
{

    CmsSockMgrContext* sockMgrContext = NULL;    
    EcSrvSocPriMgrContext *priMgrcontext;
    UINT32 ret = CME_SUCC;

    if(srvSocCreateTcpReq == NULL || fd == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocHandleCreateTcpListenReq_1, P_ERROR, 0, "atecSrvSocHandleCreateTcpListenReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    *fd = -1;

    //check whether has create
    sockMgrContext = atecSrvSocCheckTcpServerHasCreate(&srvSocCreateTcpReq->bindAddr, srvSocCreateTcpReq->listenPort, srvSocCreateTcpReq->domain);
    if(sockMgrContext != PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocHandleCreateTcpListenReq_2, P_WARNING, 1, "atecSrvSocHandleCreateTcpListenReq has already create, socket id %u", sockMgrContext->sockId);
        return CME_SOCK_MGR_ERROR_SERVER_HASE_CREATED;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(EcSrvSocPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocHandleCreateTcpListenReq_3, P_ERROR, 0, "atecSrvSocHandleCreateTcpListenReq cms sock mgr context is full");
        return CME_SOCK_MGR_ERROR_TOO_MUCH_INST;
    }

    sockMgrContext->sockId = cmsSockMgrCreateTcpSrvSocket(srvSocCreateTcpReq->domain, srvSocCreateTcpReq->listenPort, &srvSocCreateTcpReq->bindAddr, SUPPORT_MAX_TCP_SERVER_LISTEN_NUM, -1);
    if(sockMgrContext->sockId < 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocHandleCreateTcpListenReq_4, P_ERROR, 0, "atecSrvSocHandleCreateTcpListenReq create socket fail");
        cmsSockMgrRemoveContext(sockMgrContext);
        return CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    priMgrcontext->createReqHandle = srvSocCreateTcpReq->reqSource;
    priMgrcontext->listenPort = srvSocCreateTcpReq->listenPort;
    priMgrcontext->fSocketId = -1;
    ip_addr_copy(priMgrcontext->bindAddr, srvSocCreateTcpReq->bindAddr);

    sockMgrContext->domain = srvSocCreateTcpReq->domain;
    sockMgrContext->type = SOCK_CONN_TYPE_TCP;
    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
    sockMgrContext->source = SOCK_SOURCE_ECSRVSOC;
    sockMgrContext->bServer = TRUE;
    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)srvSocCreateTcpReq->eventCallback;

    if(fd)
    {
        *fd = sockMgrContext->sockId;
    }

    return ret;
}



static UINT32 atecSrvSocketHandleCloseTcpListenReq(EcSrvSocCloseTcpServerReq *srvSocCloseTcpServerReq)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    CmsSockMgrContext* sockMgrContextTmp = NULL;

    if(srvSocCloseTcpServerReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpListenReq_1, P_ERROR, 0, "atecSrvSocketHandleCloseTcpListenReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    if(srvSocCloseTcpServerReq->socketId != AT_SOC_FD_ALL)
    {

        sockMgrContext = cmsSockMgrFindMgrContextById(srvSocCloseTcpServerReq->socketId);
        if(sockMgrContext == NULL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpListenReq_2, P_INFO, 0, "atecSrvSocketHandleCloseTcpListenReq can not get the socketatcmd");
            ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
        }
        else
        {
            if(sockMgrContext->source != SOCK_SOURCE_ECSRVSOC || sockMgrContext->bServer != TRUE)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpListenReq_3, P_ERROR, 1, "atecSrvSocketHandleCloseTcpListenReq socket %d is not ecsrvsoc or server", srvSocCloseTcpServerReq->socketId);
                return CME_SOCK_MGR_ERROR_PARAM_ERROR;
            }

            //if need, close client sockets
            if(srvSocCloseTcpServerReq->bCloseClient == TRUE)
            {
                atecSrvSocCloseAllTcpCLient(srvSocCloseTcpServerReq->socketId);
            }            

            ret = cmsSockMgrCloseSocket(sockMgrContext->sockId);
            if(ret == CME_SUCC)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpListenReq_4, P_INFO, 0, "atecSrvSocketHandleCloseTcpListenReq close socket success");
                cmsSockMgrRemoveContext(sockMgrContext);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpListenReq_5, P_WARNING, 0, "atecSrvSocketHandleCloseTcpListenReq close socket fail");
                return ret;
            }

        }
    }
    else
    {
        for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != PNULL;)
        {
            if(sockMgrContext->source == SOCK_SOURCE_ECSRVSOC && sockMgrContext->bServer == TRUE)
            {
                //if need, close client sockets
                if(srvSocCloseTcpServerReq->bCloseClient == TRUE)
                {
                    atecSrvSocCloseAllTcpCLient(sockMgrContext->sockId);
                }
                
                cmsSockMgrCloseSocket(sockMgrContext->sockId);
                sockMgrContextTmp = sockMgrContext->next;
                cmsSockMgrRemoveContext(sockMgrContext);
                sockMgrContext = sockMgrContextTmp;             
            }
            else
            {
                sockMgrContext = sockMgrContext->next;
            }
        } 

    }

    return ret;
}

static UINT32 atecSrvSocketHandleCloseTcpClientReq(EcSrvSocCloseTcpClientReq *srvSocCloseTcpClientReq)
{

    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;    
    EcSrvSocPriMgrContext *priMgrcontext;

    if(srvSocCloseTcpClientReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpClientReq_1, P_ERROR, 0, "atecSrvSocketHandleCloseTcpClientReq request point is null");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    //if need, close client sockets
    if(srvSocCloseTcpClientReq->socketClientId == AT_SOC_FD_ALL)
    {
        atecSrvSocCloseAllTcpCLient(srvSocCloseTcpClientReq->socketServerId);
    }
    else
    {
        sockMgrContext = cmsSockMgrFindMgrContextById(srvSocCloseTcpClientReq->socketClientId);
        if(sockMgrContext == NULL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpClientReq_2, P_INFO, 0, "atecSrvSocketHandleCloseTcpClientReq can not get the socketatcmd");
            ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
        }
        else
        {

            if(sockMgrContext->source == SOCK_SOURCE_ECSRVSOC && sockMgrContext->bServer == FALSE)
            {
                priMgrcontext = (EcSrvSocPriMgrContext *)sockMgrContext->priContext;
                GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

                if(priMgrcontext->fSocketId == srvSocCloseTcpClientReq->socketServerId)
                {
                    cmsSockMgrCloseSocket(sockMgrContext->sockId);
                    cmsSockMgrRemoveContext(sockMgrContext);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpClientReq_3, P_ERROR, 0, "atecSrvSocketHandleCloseTcpClientReq request point is null");
                    ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;                    
                }
            }
            else
            {
                 ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleCloseTcpClientReq_4, P_ERROR, 1, "atecSrvSocketHandleCloseTcpClientReq client fd %u is nor ECSRVSOC or client", srvSocCloseTcpClientReq->socketClientId);
                 ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;             
            }
        }            
    }

    return ret;
}

static UINT32 atecSrvSocketHandleTcpClientSendReq(EcSrvSocSendTcpClientReq *srvSocSendTcpClientReq)
{
    UINT32 ret = CME_SUCC;
    CmsSockMgrContext* sockMgrContext = NULL;
    EcsocPriMgrContext *priMgrcontext;
    int sendLen = -1;

    if(srvSocSendTcpClientReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_1, P_ERROR, 0, "atecSrvSocketHandleTcpClientSendReq invalid param");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    sockMgrContext = cmsSockMgrFindMgrContextById(srvSocSendTcpClientReq->socketClientId);
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_2, P_INFO, 0, "atecSrvSocketHandleTcpClientSendReq can not get the socketatcmd");
        ret = CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
    }
    else
    {

        priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
        GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);


        if(sockMgrContext->source != SOCK_SOURCE_ECSRVSOC || sockMgrContext->type != SOCK_CONN_TYPE_TCP)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_3, P_ERROR, 1, "atecSrvSocketHandleTcpClientSendReq the seq socket %d is not TCP socket", srvSocSendTcpClientReq->socketClientId);
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else
        {
            if(sockMgrContext->status != SOCK_CONN_STATUS_CONNECTED || sockMgrContext->bServer != FALSE)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_4, P_ERROR, 1, "atecSrvSocketHandleTcpClientSendReq the seq socket %d is not connected or is server", srvSocSendTcpClientReq->socketClientId);
                ret = CME_SOCK_MGR_ERROR_NO_CONNECTED;
            }
            else
            {
                if(srvSocSendTcpClientReq->sendLen > 0)
                {
                    sendLen = ps_send_with_sequence(sockMgrContext->sockId, srvSocSendTcpClientReq->data, srvSocSendTcpClientReq->sendLen, 0, srvSocSendTcpClientReq->dataRai, srvSocSendTcpClientReq->dataExpect, 0);
                    if(sendLen > 0)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_5, P_INFO, 1, "atecSrvSocketHandleTcpClientSendReq send packet success %u", srvSocSendTcpClientReq->socketClientId);
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_6, P_ERROR, 0, "atecSrvSocketHandleTcpClientSendReq send fail");
                        ret = CME_SOCK_MGR_ERROR_SEND_FAIL;
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpClientSendReq_7, P_ERROR, 0, "atecSrvSocketHandleTcpClientSendReq invalid sendLen param");
                    ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
                }
            }
        }
    }

    return ret;

}

static UINT32 atecSrvSocketHandleTcpListenStatusReq(EcSrvSocStatusTcpServerReq *srvSocStatusTcpServerReq)
{
    INT32 ret = CME_SUCC;
    EcSrvSocTcpListenStatusInd srvSocTcpListenStatusInd;
    CmsSockMgrContext* sockMgrContext = NULL;

    if(srvSocStatusTcpServerReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_1, P_ERROR, 0, "atecSrvSocketHandleTcpListenStatusReq invalid param");
        return CME_SOCK_MGR_ERROR_UNKNOWN;
    }

    if(srvSocStatusTcpServerReq->socketId != AT_SOC_FD_ALL)
    {
        if(srvSocStatusTcpServerReq->socketId < 0)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_2, P_ERROR, 0, "atecSrvSocketHandleTcpListenStatusReq invalid param");
            return CME_SOCK_MGR_ERROR_PARAM_ERROR;
        }
        else
        {
            sockMgrContext = cmsSockMgrFindMgrContextById(srvSocStatusTcpServerReq->socketId);
            if(sockMgrContext == PNULL)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_3, P_ERROR, 1, "atecSrvSocketHandleTcpListenStatusReq can not find socket id %u", srvSocStatusTcpServerReq->socketId);
                return CME_SOCK_MGR_ERROR_NO_FIND_CLIENT;
            }
            else
            {
                if(sockMgrContext->source == SOCK_SOURCE_ECSRVSOC && sockMgrContext->bServer == TRUE)
                {
                    srvSocTcpListenStatusInd.serverSocketId = srvSocStatusTcpServerReq->socketId;
                    srvSocTcpListenStatusInd.status = sockMgrContext->status;

                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_4, P_INFO, 2, "atecSrvSocketHandleTcpListenStatusReq senq status indication id %u, status %u", srvSocTcpListenStatusInd.serverSocketId, srvSocTcpListenStatusInd.status);
                    atecSrvSocSendInd(srvSocStatusTcpServerReq->reqSource, APPL_ECSRVSOC_STATUS_TCP_LISTEN_IND, sizeof(EcSrvSocTcpListenStatusInd), &srvSocTcpListenStatusInd);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_5, P_ERROR, 0, "atecSrvSocketHandleTcpListenStatusReq invalid param");
                    return CME_SOCK_MGR_ERROR_PARAM_ERROR;                   
                }
            }
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_6, P_INFO, 0, "atecSrvSocketHandleTcpListenStatusReq query all ec srv listen socket status");

        for(sockMgrContext = cmsSockMgrGetContextList(); sockMgrContext != PNULL; sockMgrContext = sockMgrContext->next)
        {
            if(sockMgrContext->source == SOCK_SOURCE_ECSRVSOC && sockMgrContext->bServer == TRUE)
            {
                srvSocTcpListenStatusInd.serverSocketId = sockMgrContext->sockId;
                srvSocTcpListenStatusInd.status = sockMgrContext->status;
            
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atecSrvSocketHandleTcpListenStatusReq_7, P_INFO, 2, "atecSrvSocketHandleTcpListenStatusReq senq status indication id %u, status %u", srvSocTcpListenStatusInd.serverSocketId, srvSocTcpListenStatusInd.status);
                atecSrvSocSendInd(srvSocStatusTcpServerReq->reqSource, APPL_ECSRVSOC_STATUS_TCP_LISTEN_IND, sizeof(EcSrvSocTcpListenStatusInd), &srvSocTcpListenStatusInd);
            }

        }
    }

    return ret;
}


void atSktStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnStatusArg *statusArg)
{
    AtecSktPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || statusArg == NULL)
    {
        return;
    }

    priMgrcontext = (AtecSktPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    switch(statusArg->oldStatus)
    {
        case SOCK_CONN_STATUS_CONNECTING:{
            if(statusArg->newStatus == SOCK_CONN_STATUS_CREATED)
            {
                //connect timeout
                if(statusArg->cause == CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT)
                {
                    atecSktSendCnf(priMgrcontext->connectReqHandle, APPL_RET_FAIL, APPL_SOCKET_CONNECT_CNF, CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT, 0, 0);
                }
                else if(statusArg->cause == CME_SOCK_MGR_ERROR_SOCK_ERROR) //connect fail
                {
                    atecSktSendCnf(priMgrcontext->connectReqHandle, APPL_RET_FAIL, APPL_SOCKET_CONNECT_CNF, CME_SOCK_MGR_ERROR_SOCK_ERROR, 0, 0);
                }
            }
            else if(statusArg->newStatus == SOCK_CONN_STATUS_CONNECTED)
            {
                //connect success
                atecSktSendCnf(priMgrcontext->connectReqHandle, APPL_RET_SUCC, APPL_SOCKET_CONNECT_CNF, 0, 0, 0);
            }
            break;
        }
        case SOCK_CONN_STATUS_CONNECTED:{
            break;
        }
        default:
            break;
    }

    return;
}

void atSktDlEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnDlArg *dlArg)
{
    AtecSktPriMgrContext *priMgrcontext;
    CmsSockMgrDataContext *dataContext;

    if(mgrContext == NULL || dlArg == NULL)
    {
        return;
    }

    priMgrcontext = (AtecSktPriMgrContext *)mgrContext->priContext;
    dataContext = dlArg->dataContext;
    GosCheck(priMgrcontext != PNULL && dataContext != PNULL, priMgrcontext, dataContext, 0);

    nmSocketNetworkRecvInd(priMgrcontext->createReqHandle, mgrContext->sockId, dataContext);

    return;
}

void atSktSockErrorEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnErrArg *errArg)
{
    AtecSktPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || errArg == NULL)
    {
        return;
    }

    priMgrcontext = (AtecSktPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(errArg->errCode != 0)
    {
        nmSocketNetworkErrorInd(priMgrcontext->createReqHandle, mgrContext->sockId, errArg->errCode);
        cmsSockMgrCloseSocket(mgrContext->sockId);
        cmsSockMgrRemoveContextById(mgrContext->sockId);
    }

    return;

}

void atEcsocStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnStatusArg *statusArg)
{
    EcsocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || statusArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    switch(statusArg->oldStatus)
    {
        case SOCK_CONN_STATUS_CONNECTING:{
            if(statusArg->newStatus == SOCK_CONN_STATUS_CREATED)
            {
                //connect fail
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcsocStatusEventProcess_1, P_WARNING, 0, "atEcsocStatusEventProcess connect fail");
            }
            else if(statusArg->newStatus == SOCK_CONN_STATUS_CONNECTED)
            {
                //connect success
                atEcsocSendConnectedInd(priMgrcontext->createReqHandle, mgrContext->sockId);
            }
            break;
        }
        case SOCK_CONN_STATUS_CONNECTED:{
            break;
        }
        default:
            break;
    }

    return;
}

void atEcsocDlEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnDlArg *dlArg)
{
    EcsocPriMgrContext *priMgrcontext;
    CmsSockMgrDataContext *dataContext;

    if(mgrContext == NULL || dlArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    dataContext = dlArg->dataContext;
    GosCheck(priMgrcontext != PNULL && dataContext != PNULL, priMgrcontext, dataContext, 0);

#if 0
    if(priMgrcontext->modeSet.receiveControl == RCV_DISABLE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocDlEventProcess_2, P_INFO, 0, "recv control flag is disable, ignore this packet");
    }
    else
    {
#endif
        if(atecEcSocProcessDlData(mgrContext, dataContext, &dlArg->fromAddr, dlArg->fromPort) != CME_SUCC)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocDlEventProcess_3, P_WARNING, 0, "ignore this packet");
        }
#if 0
    }
#endif

    return;
}


void atEcsocErrorEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnErrArg *errArg)
{
    EcsocPriMgrContext *priMgrcontext;
    EcSocCloseInd socCloseInd;
    UINT16 reqHandle;

    if(mgrContext == NULL || errArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(errArg->errCode != 0)
    {
        socCloseInd.socketId = mgrContext->sockId;
        socCloseInd.errCode = errArg->errCode;
        reqHandle = priMgrcontext->createReqHandle;
        atecSocketHandleEcSocCloseSocket(reqHandle, mgrContext);
        atecSocSendInd(reqHandle, APPL_ECSOC_CLOSE_IND, sizeof(socCloseInd), &socCloseInd);
    }

    return;

}

void atEcsocUlStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnUlStatusArg *ulStatusArg)
{
    EcsocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || ulStatusArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, ulStatusArg->sequence) == TRUE)
    {
        cmsSockMgrUpdateUlPendingSequenceBitMapState(priMgrcontext->ulSeqStatus.bitmap, ulStatusArg->sequence, FALSE);
        if(ulStatusArg->status == SOCK_MGR_UL_SEQUENCE_STATE_SENT)
        {
            atecSocSendUlStatusInd(priMgrcontext->createReqHandle, mgrContext->sockId, ulStatusArg->sequence, ATECECSOC_SUCCESS);
        }
        else if(ulStatusArg->status == SOCK_MGR_UL_SEQUENCE_STATE_DISCARD)
        {
            atecSocSendUlStatusInd(priMgrcontext->createReqHandle, mgrContext->sockId, ulStatusArg->sequence, ATECECSOC_FAIL);
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocUlStatusEventProcess_2, P_WARNING, 2, "AtEcsocUlStatusEventProcess the socket %d, sequence %d status invalid", mgrContext->sockId, ulStatusArg->sequence);
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocUlStatusEventProcess_3, P_WARNING, 2, "AtEcsocUlStatusEventProcess the socket %d, sequence %d status invalid", mgrContext->sockId, ulStatusArg->sequence);
    }


    return;

}

void atEcSrvSocStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnStatusArg *statusArg)
{
    EcSrvSocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || statusArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    return;
}


void atEcSrvSocDlEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnDlArg *dlArg)
{
    EcSrvSocPriMgrContext *priMgrcontext;
    CmsSockMgrDataContext *dataContext;
    EcSrvSocTcpClientReceiveInd *srvSocTcpClientReceiveInd;

    if(mgrContext == NULL || dlArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)mgrContext->priContext;
    dataContext = dlArg->dataContext;
    GosCheck(priMgrcontext != PNULL && dataContext != PNULL, priMgrcontext, dataContext, 0);

    if(mgrContext->bServer == FALSE)
    {
        if(sizeof(EcSrvSocTcpClientReceiveInd) > CMS_SOCK_MGR_DATA_HEADER_LENGTH_MAX)
        {
            srvSocTcpClientReceiveInd = (EcSrvSocTcpClientReceiveInd *)malloc(sizeof(EcSrvSocTcpClientReceiveInd) + dataContext->Length);
            if(srvSocTcpClientReceiveInd == NULL)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocDlEventProcess_1, P_ERROR, 0, "atEcSrvSocDlEventProcess malloc tcp client rcv indication fail");
            }
            else
            {
                srvSocTcpClientReceiveInd->length = dataContext->Length;
                srvSocTcpClientReceiveInd->socketId = mgrContext->sockId;
                memcpy(srvSocTcpClientReceiveInd->data, dataContext->data, dataContext->Length);
                atecSrvSocSendInd(priMgrcontext->createReqHandle, APPL_ECSRVSOC_RECEIVE_TCP_CLIENT_IND, sizeof(EcSrvSocTcpClientReceiveInd) + sizeof(srvSocTcpClientReceiveInd->length), srvSocTcpClientReceiveInd);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocDlEventProcess_2, P_SIG, 1, "atEcSrvSocDlEventProcess send a tcp client %u rcv indication", mgrContext->sockId);
                free(srvSocTcpClientReceiveInd);
            }
        }
        else
        {
            srvSocTcpClientReceiveInd = (EcSrvSocTcpClientReceiveInd *)((UINT8 *)dataContext->data - (UINT32)&(((EcSrvSocTcpClientReceiveInd *)0)->data));
            if(srvSocTcpClientReceiveInd == NULL)
            {
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocDlEventProcess_3, P_ERROR, 0, "atEcSrvSocDlEventProcess memory unknow issue");
            }
            else
            {
                srvSocTcpClientReceiveInd->length = dataContext->Length;
                srvSocTcpClientReceiveInd->socketId = mgrContext->sockId;
                atecSrvSocSendInd(priMgrcontext->createReqHandle, APPL_ECSRVSOC_RECEIVE_TCP_CLIENT_IND, sizeof(EcSrvSocTcpClientReceiveInd) + sizeof(srvSocTcpClientReceiveInd->length), srvSocTcpClientReceiveInd);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocDlEventProcess_4, P_SIG, 1, "atEcSrvSocDlEventProcess send a tcp client %u rcv indication", mgrContext->sockId);
            }
        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocDlEventProcess_5, P_WARNING, 1, "atEcSrvSocDlEventProcess socket %u is not a tcp client  ", mgrContext->sockId);
    }


    return;
}

void atEcSrvSocErrorEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnErrArg *errArg)
{
    EcSrvSocPriMgrContext *priMgrcontext;
    EcSrvSocCloseInd srvSocCloseInd;
    UINT16 reqHandle;

    if(mgrContext == NULL || errArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(errArg->errCode != 0)
    {
        srvSocCloseInd.socketId = mgrContext->sockId;
        srvSocCloseInd.errCode = errArg->errCode;
        srvSocCloseInd.bServer = mgrContext->bServer;
        reqHandle = priMgrcontext->createReqHandle;
        cmsSockMgrCloseSocket(mgrContext->sockId);
        cmsSockMgrRemoveContext(mgrContext);
        atecSrvSocSendInd(reqHandle, APPL_ECSRVSOC_CLOSE_TCP_CONNECTION_IND, sizeof(EcSrvSocCloseInd), &srvSocCloseInd);
    }

    return;

}

void atEcSrcSocUlStatusEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnUlStatusArg *ulStatusArg)
{
    EcSrvSocPriMgrContext *priMgrcontext;

    if(mgrContext == NULL || ulStatusArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    //not used now

    return;
}

void atEcSrcSocTcpClientAcceptEventProcess(CmsSockMgrContext *mgrContext, CmsSockMgrConnAcceptClientArg *acceptClientArg)
{
    EcSrvSocPriMgrContext *priMgrcontext;
    EcSrvSocTcpAcceptClientReaultInd srvSocTcpAcceptClientReaultInd;

    if(mgrContext == NULL || acceptClientArg == NULL)
    {
        return;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    srvSocTcpAcceptClientReaultInd.serverSocketId = acceptClientArg->serverSocketId;
    srvSocTcpAcceptClientReaultInd.clientSocketId = acceptClientArg->clientSocketId;
    srvSocTcpAcceptClientReaultInd.clientPort = acceptClientArg->clientPort;
    ip_addr_copy(srvSocTcpAcceptClientReaultInd.clientAddr, acceptClientArg->clientAddr);
    atecSrvSocSendInd(priMgrcontext->createReqHandle, APPL_ECSRVSOC_SERVER_ACCEPT_CLIENT_IND, sizeof(EcSrvSocTcpAcceptClientReaultInd), &srvSocTcpAcceptClientReaultInd);
    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrcSocTcpClientAcceptEventProcess_1, P_INFO, 2, "atEcSrcSocTcpClientAcceptEventProcess send a server accept client ind, server id %u, client id %u", acceptClientArg->serverSocketId, acceptClientArg->clientSocketId);  

    return;

}


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


void atecSktProessReq(CmsSockMgrRequest *atecSktReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd)
{
    INT32 result;
    CHAR RespBuf[32];

    if(atecSktReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_1, P_ERROR, 0, "AtecSktProessReq invalid param");
        return;
    }

    //check magic
    if(atecSktReq->magic != CMS_SOCK_MGR_ASYNC_REQUEST_MAGIC && atecSktReq->magic != CMS_SOCK_MGR_SYNC_REQUEST_MAGIC)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_2, P_ERROR, 0, "AtecSocketProessReq invalid param");
        return;
    }

    memset(RespBuf, 0, sizeof(RespBuf));
    if(atecSktReq->source == SOCK_SOURCE_ATSKT) {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_3, P_INFO, 3, "read atSktReqFd success, req_id %d, reqbody 0x%x, sourcePort %d ", atecSktReq->reqId, atecSktReq->reqBody, sourcePort);
        switch(atecSktReq->reqId) {
            case ATECSKTREQ_CREATE:{
                AtecSktCreateReq *sktCreateReq;
                INT32 createfd = -1;
                sktCreateReq = (AtecSktCreateReq *)atecSktReq->reqBody;
                if(sktCreateReq != NULL)
                {
                    result = atecSocketHandleCreateReq(sktCreateReq, &createfd);
                    if(result == CME_SUCC)
                    {
                        atecSktSendCnf(sktCreateReq->reqSource, APPL_RET_SUCC, APPL_SOCKET_CREATE_CNF, 0, createfd, 0);
                    }
                    else
                    {
                        atecSktSendCnf(sktCreateReq->reqSource, APPL_RET_FAIL, APPL_SOCKET_CREATE_CNF, result, 0, 0);
                    }
                }
                break;
            }
            case ATECSKTREQ_BIND:{
                AtecSktBindReq *sktBindReq;
                sktBindReq = (AtecSktBindReq *)atecSktReq->reqBody;
                if(sktBindReq != NULL)
                {
                    result = atecSocketHandleBindReq(sktBindReq);
                    if(result == CME_SUCC)
                    {
                        atecSktSendCnf(sktBindReq->reqSource, APPL_RET_SUCC, APPL_SOCKET_BIND_CNF, 0, 0, 0);
                    }
                    else
                    {
                        atecSktSendCnf(sktBindReq->reqSource, APPL_RET_FAIL, APPL_SOCKET_BIND_CNF, result, 0, 0);
                    }
                }
                break;
            }
            case ATECSKTREQ_CONNECT:{
                AtecSktConnectReq *sktConnectReq;
                sktConnectReq = (AtecSktConnectReq *)atecSktReq->reqBody;
                if(sktConnectReq != NULL)
                {
                    result = atecSocketHandleConnectReq(sktConnectReq);
                    if(result == CME_SUCC)
                    {
                        atecSktSendCnf(sktConnectReq->reqSource, APPL_RET_SUCC, APPL_SOCKET_CONNECT_CNF, 0, 0, 0);
                    }
                    else if(result == CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_4, P_INFO, 0, "AtecSocketProessReq connect is on going, delay send atskt cnf");
                    }
                    else
                    {
                         atecSktSendCnf(sktConnectReq->reqSource, APPL_RET_FAIL, APPL_SOCKET_CONNECT_CNF, result, 0, 0);
                    }
                }
                break;
            }
            case ATECSKTREQ_SEND:{
                AtecSktSendReq *sktSendReq;
                sktSendReq = (AtecSktSendReq *)atecSktReq->reqBody;
                if(sktSendReq != NULL)
                {
                    result = atecSocketHandleSendReq(sktSendReq);
                    if(result == CME_SUCC)
                    {
                        atecSktSendCnf(sktSendReq->reqSource, APPL_RET_SUCC, APPL_SOCKET_SEND_CNF, 0, 0, 0);
                    }
                    else
                    {
                        atecSktSendCnf(sktSendReq->reqSource, APPL_RET_FAIL, APPL_SOCKET_SEND_CNF, result, 0, 0);
                    }
                }
                break;
            }
            case ATECSKTREQ_DELETE:{
                AtecSktDeleteReq *sktDeleteReq;
                sktDeleteReq = (AtecSktDeleteReq *)atecSktReq->reqBody;
                if(sktDeleteReq != NULL)
                {
                    result = atecSocketHandleDeleteReq(sktDeleteReq);
                    if(result == CME_SUCC)
                    {
                        atecSktSendCnf(sktDeleteReq->reqSource, APPL_RET_SUCC, APPL_SOCKET_DELETE_CNF, 0, 0, 0);
                    }
                    else
                    {
                        atecSktSendCnf(sktDeleteReq->reqSource, APPL_RET_FAIL, APPL_SOCKET_DELETE_CNF, result, 0, 0);
                    }
                }
                break;
            }
            case ATECSKTREQ_STATUS:{
                AtecSktStatusReq *sktStatusReq;
                sktStatusReq = (AtecSktStatusReq *)atecSktReq->reqBody;
                INT32 status = SOCK_CLOSED;
                if(sktStatusReq != NULL)
                {
                    result = atecSocketHandleStatusReq(sktStatusReq, &status);
                    if(result == CME_SUCC)
                    {
                        atecSktSendCnf(sktStatusReq->reqSource, APPL_RET_SUCC, APPL_SOCKET_STATUS_CNF, 0, 0, status);
                    }
                    else
                    {
                        atecSktSendCnf(sktStatusReq->reqSource, APPL_RET_FAIL, APPL_SOCKET_STATUS_CNF, result, 0, 0);
                    }
                }
                break;
            }
            default:
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_5, P_WARNING, 1, "process atSktReqFd fail, invalid reqId %d", atecSktReq->reqId);
                break;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_6, P_WARNING, 1, "read atSktReqFd fail, source %u check fail", atecSktReq->source);
    }
}



void atecEcsocProessReq(CmsSockMgrRequest *atecSktReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd)
{
    INT32 result;
    CHAR RespBuf[32];

    if(atecSktReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_1, P_ERROR, 0, "AtecSocketProessReq invalid param");
        return;
    }

    //check magic
    if(atecSktReq->magic != CMS_SOCK_MGR_ASYNC_REQUEST_MAGIC && atecSktReq->magic != CMS_SOCK_MGR_SYNC_REQUEST_MAGIC)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_2, P_ERROR, 0, "AtecSocketProessReq invalid param");
        return;
    }

    memset(RespBuf, 0, sizeof(RespBuf));
    if(atecSktReq->source == SOCK_SOURCE_ECSOC) {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_3, P_INFO, 3, "read atEcsocReqFd success, req_id %d, reqbody 0x%x, sourcePort %d", atecSktReq->reqId, atecSktReq->reqBody, sourcePort);
        switch(atecSktReq->reqId) {
            case ECSOCREQ_CREATE:{
                EcSocCreateReq *socCreateReq;
                EcSocCreateResponse socCreateResponse;
                socCreateReq = (EcSocCreateReq *)atecSktReq->reqBody;
                if(socCreateReq != NULL)
                {
                    result = atecSocketHandleEcSocCreateReq(socCreateReq, &socCreateResponse);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socCreateReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_CREATE_CNF, 0, &socCreateResponse);
                    }
                    else
                    {
                        atecSocSendCnf(socCreateReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_CREATE_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_UDPSEND:
                {
                EcSocUdpSendReq *socUdpSendReq;
                EcSocUdpSendResponse socUdpSendResponse;
                INT32 socUdpSendSocketid = 0;
                UINT8 socUdpSendSequence = 0;
                BOOL bNeedUlInd = FALSE;
                socUdpSendReq = (EcSocUdpSendReq *)atecSktReq->reqBody;
                if(socUdpSendReq != NULL)
                {
                    socUdpSendSocketid = socUdpSendReq->socketId;
                    socUdpSendSequence = socUdpSendReq->sequence;
                    result = atecSocketHandleEcSocUdpSendReq(socUdpSendReq, &socUdpSendResponse, &bNeedUlInd);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socUdpSendReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_UDPSEND_CNF, 0, &socUdpSendResponse);
                        if(bNeedUlInd == TRUE)
                        {
                            atecSocSendUlStatusInd(socUdpSendReq->reqSource, socUdpSendSocketid, socUdpSendSequence, ATECECSOC_SUCCESS);
                        }
                    }
                    else
                    {
                        atecSocSendCnf(socUdpSendReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_UDPSEND_CNF, result, NULL);
                        if(bNeedUlInd == TRUE)
                        {
                            atecSocSendUlStatusInd(socUdpSendReq->reqSource, socUdpSendSocketid, socUdpSendSequence, ATECECSOC_FAIL);
                        }
                    }
                }
                break;
            }
            case ECSOCREQ_UDPSENDF:{
                EcSocUdpSendReq *socUdpSendReq;
                EcSocUdpSendResponse socUdpSendResponse;
                INT32 socUdpSendSocketid = 0;
                UINT8 socUdpSendSequence = 0;
                BOOL bNeedUlInd = FALSE;
                socUdpSendReq = (EcSocUdpSendReq *)atecSktReq->reqBody;
                if(socUdpSendReq != NULL)
                {
                    socUdpSendSocketid = socUdpSendReq->socketId;
                    socUdpSendSequence = socUdpSendReq->sequence;
                    result = atecSocketHandleEcSocUdpSendReq(socUdpSendReq, &socUdpSendResponse, &bNeedUlInd);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socUdpSendReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_UDPSENDF_CNF, 0, &socUdpSendResponse);
                        if(bNeedUlInd == TRUE)
                        {
                            atecSocSendUlStatusInd(socUdpSendReq->reqSource, socUdpSendSocketid, socUdpSendSequence, ATECECSOC_SUCCESS);
                        }
                    }
                    else
                    {
                        atecSocSendCnf(socUdpSendReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_UDPSENDF_CNF, result, NULL);
                        if(bNeedUlInd == TRUE)
                        {
                            atecSocSendUlStatusInd(socUdpSendReq->reqSource, socUdpSendSocketid, socUdpSendSequence, ATECECSOC_FAIL);
                        }
                    }
                }
                break;
            }
            case ECSOCREQ_QUERY:{
                EcSocQueryReq *socQueryReq;
                socQueryReq = (EcSocQueryReq *)atecSktReq->reqBody;
                if(socQueryReq != NULL)
                {
                    result = atecSocketHandleEcSocQueryReq(socQueryReq);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socQueryReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_QUERY_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socQueryReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_QUERY_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_READ:{
                EcSocReadReq *socReadReq;
                EcSocReadResponse *socReadResponse;
                BOOL bNeedNMI;
                INT32 readFd;
                socReadReq = (EcSocReadReq *)atecSktReq->reqBody;
                if(socReadReq != NULL)
                {
                    readFd = socReadReq->socketId;
                    socReadResponse = (EcSocReadResponse *)malloc(sizeof(EcSocReadResponse) + socReadReq->length);
                    if(socReadResponse)
                    {
                        result = atecSocketHandleEcSocReadReq(socReadReq, socReadResponse, &bNeedNMI);
                        if(result == CME_SUCC)
                        {
                            atecSocSendCnf(socReadReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_READ_CNF, 0, socReadResponse);

                            //if need NMI, do it
                            if(bNeedNMI == TRUE)
                            {
                                EcSocNMInd *socNMInd = PNULL;
                                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_4, P_SIG, 1, "read compelete, socket %u need NMI", readFd);
                                if(atecSocGetNMInfo(readFd, &socNMInd) == CME_SUCC)
                                {
                                    if(socNMInd != PNULL)
                                    {
                                        if(socNMInd->modeNMI == NMI_MODE_1)
                                        {
                                            atecSocSendInd(socReadReq->reqSource, APPL_ECSOC_NMI_IND, sizeof(EcSocNMInd), socNMInd);
                                        }
                                        else if(socNMInd->modeNMI == NMI_MODE_2 || socNMInd->modeNMI == NMI_MODE_3)
                                        {
                                            atecSocSendInd(socReadReq->reqSource, APPL_ECSOC_NMI_IND, sizeof(EcSocNMInd) + sizeof(socNMInd->length), socNMInd);
                                        }
                                        free(socNMInd);
                                    }
                                }

                            }
                        }
                        else
                        {
                            atecSocSendCnf(socReadReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_READ_CNF, result, NULL);
                        }
                        free(socReadResponse);
                    }
                    else
                    {
                        atecSocSendCnf(socReadReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_READ_CNF, CME_SOCK_MGR_ERROR_NO_MEMORY, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_TCPCONNECT:{
                EcSocTcpConnectReq *socTcpConnectReq;
                socTcpConnectReq = (EcSocTcpConnectReq *)atecSktReq->reqBody;
                if(socTcpConnectReq != NULL)
                {
                    result = atecSocketHandleEcSocTcpConnectReq(socTcpConnectReq);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socTcpConnectReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_TCPCONNECT_CNF, 0, NULL);
                        
                        //connect success indication
                        atEcsocSendConnectedInd(socTcpConnectReq->reqSource, socTcpConnectReq->socketId);                        
                    }
                    else if(result == CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_5, P_INFO, 0, "AtecSocketProessReq tcpconnect is on going");
                        atecSocSendCnf(socTcpConnectReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_TCPCONNECT_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socTcpConnectReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_TCPCONNECT_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_TCPSEND:{
                EcSocTcpSendReq *socTcpSendReq;
                EcSocTcpSendResponse socTcpSendResponse;
                socTcpSendReq = (EcSocTcpSendReq *)atecSktReq->reqBody;
                if(socTcpSendReq != NULL)
                {
                    result = atecSocketHandleEcSocTcpSendReq(socTcpSendReq, &socTcpSendResponse);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socTcpSendReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_TCPSEND_CNF, 0, &socTcpSendResponse);
                    }
                    else
                    {
                        atecSocSendCnf(socTcpSendReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_TCPSEND_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_CLOSE:{
                EcSocCloseReq *socCloseReq;
                socCloseReq = (EcSocCloseReq *)atecSktReq->reqBody;
                if(socCloseReq != NULL)
                {
                    result = atecSocketHandleEcSocCloseReq(socCloseReq);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socCloseReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_CLOSE_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socCloseReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_CLOSE_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_NMI:{
                EcSocNMIReq *socNMIReq;
                socNMIReq = (EcSocNMIReq *)atecSktReq->reqBody;
                if(socNMIReq != NULL)
                {
                    result = atecHandleEcSocNMIReq(socNMIReq);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socNMIReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_NMI_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socNMIReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_NMI_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_NMIE:{
                EcSocNMIEReq *socNMIEReq;
                socNMIEReq = (EcSocNMIEReq *)atecSktReq->reqBody;
                if(socNMIEReq != NULL)
                {
                    result = atecHandleEcSocNMIEReq(socNMIEReq);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socNMIEReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_NMIE_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socNMIEReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_NMIE_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_GNMI:{
                EcSocNMIGetReq *socNmiGetReq;
                socNmiGetReq = (EcSocNMIGetReq *)atecSktReq->reqBody;
                if(socNmiGetReq != NULL)
                {
                    EcSocGNMIResponse socGNMIResponse;
                    socGNMIResponse.mode = atecEcSocGetPublicMode();
                    socGNMIResponse.maxDlBufferSize = atecEcSocGetPublicDlBufferTotalSize();
                    socGNMIResponse.maxDlPkgNum = atecEcSocGetPublicDlPkgNumMax();

                    atecSocSendCnf(socNmiGetReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_GNMI_CNF, 0, &socGNMIResponse);
                }
                break;
            }
            case ECSOCREQ_GNMIE:{
                EcSocNMIEGetReq *socNmieGetReq;
                socNmieGetReq = (EcSocNMIEGetReq *)atecSktReq->reqBody;
                if(socNmieGetReq != NULL)
                {
                    result = atecSocketHandleEcSocGNMIEReq(socNmieGetReq->reqSource);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socNmieGetReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_GNMIE_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socNmieGetReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_GNMIE_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSOCREQ_STATUS:{
                EcSocStatusReq *socStatusReq;
                socStatusReq = (EcSocStatusReq *)atecSktReq->reqBody;
                if(socStatusReq != NULL)
                {
                    result = atecSocketHandleEcSocStatusReq(socStatusReq);
                    if(result == CME_SUCC)
                    {
                        atecSocSendCnf(socStatusReq->reqSource, APPL_RET_SUCC, APPL_ECSOC_STATUS_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSocSendCnf(socStatusReq->reqSource, APPL_RET_FAIL, APPL_ECSOC_STATUS_CNF, result, NULL);
                    }
                }
                break;
            }
            default:
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_6, P_WARNING, 1, "process atSktReqFd fail, invalid reqId %d", atecSktReq->reqId);
                break;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSocketProessReq_7, P_WARNING, 1, "read atSktReqFd fail, source %u check fail", atecSktReq->source);
    }
}



void atecEcSrvSocProessReq(CmsSockMgrRequest *atecEcSrvSocReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd)
{
    INT32 result;
    CHAR RespBuf[32];

    if(atecEcSrvSocReq == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_1, P_ERROR, 0, "AtecSktProessReq invalid param");
        return;
    }

    //check magic
    if(atecEcSrvSocReq->magic != CMS_SOCK_MGR_ASYNC_REQUEST_MAGIC && atecEcSrvSocReq->magic != CMS_SOCK_MGR_SYNC_REQUEST_MAGIC)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_2, P_ERROR, 0, "AtecSocketProessReq invalid param");
        return;
    }

    memset(RespBuf, 0, sizeof(RespBuf));
    if(atecEcSrvSocReq->source == SOCK_SOURCE_ECSRVSOC) {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_3, P_INFO, 3, "read atSktReqFd success, req_id %d, reqbody 0x%x, sourcePort %d ", atecEcSrvSocReq->reqId, atecEcSrvSocReq->reqBody, sourcePort);
        switch(atecEcSrvSocReq->reqId) {
            case ECSRVSOCREQ_CREATETCP:{
                EcSrvSocCreateTcpReq *srvSocCreateTcpReq;
                INT32 createfd = -1;
                srvSocCreateTcpReq = (EcSrvSocCreateTcpReq *)atecEcSrvSocReq->reqBody;
                if(srvSocCreateTcpReq != NULL)
                {
                    atecSrvSocSendCnf(srvSocCreateTcpReq->reqSource, APPL_RET_SUCC, APPL_ECSRVSOC_CREATE_TCP_LISTEN_CNF, 0, NULL);

                    result = atecSrvSocHandleCreateTcpListenReq(srvSocCreateTcpReq, &createfd);
                    EcSrcSocCreateTcpListenInd srvSocCreateTcpListenInd;
                    srvSocCreateTcpListenInd.result = result;
                    srvSocCreateTcpListenInd.socketId = createfd;
                    atecSrvSocSendInd(srvSocCreateTcpReq->reqSource, APPL_ECSRVSOC_CREATE_TCP_LISTEN_IND, sizeof(EcSrcSocCreateTcpListenInd), &srvSocCreateTcpListenInd);
                }
                break;
            }
            case ECSRVSOCREQ_CLOSELISTEN:{
                EcSrvSocCloseTcpServerReq *srvSocCloseTcpServerReq;
                srvSocCloseTcpServerReq = (EcSrvSocCloseTcpServerReq *)atecEcSrvSocReq->reqBody;
                if(srvSocCloseTcpServerReq != NULL)
                {
                    result = atecSrvSocketHandleCloseTcpListenReq(srvSocCloseTcpServerReq);
                    if(result == CME_SUCC)
                    {
                        atecSrvSocSendCnf(srvSocCloseTcpServerReq->reqSource, APPL_RET_SUCC, APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSrvSocSendCnf(srvSocCloseTcpServerReq->reqSource, APPL_RET_FAIL, APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSRVSOCREQ_CLOSECLIENT:{
                EcSrvSocCloseTcpClientReq *srvSocCloseTcpClientReq;
                srvSocCloseTcpClientReq = (EcSrvSocCloseTcpClientReq *)atecEcSrvSocReq->reqBody;
                if(srvSocCloseTcpClientReq != NULL)
                {
                    result = atecSrvSocketHandleCloseTcpClientReq(srvSocCloseTcpClientReq);
                    if(result == CME_SUCC)
                    {
                        atecSrvSocSendCnf(srvSocCloseTcpClientReq->reqSource, APPL_RET_SUCC, APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSrvSocSendCnf(srvSocCloseTcpClientReq->reqSource, APPL_RET_FAIL, APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSRVSOCREQ_SENDCLIENT:{
                EcSrvSocSendTcpClientReq *srvSocSendTcpClientReq;
                srvSocSendTcpClientReq = (EcSrvSocSendTcpClientReq *)atecEcSrvSocReq->reqBody;
                if(srvSocSendTcpClientReq != NULL)
                {
                    result = atecSrvSocketHandleTcpClientSendReq(srvSocSendTcpClientReq);
                    if(result == CME_SUCC)
                    {
                        atecSrvSocSendCnf(srvSocSendTcpClientReq->reqSource, APPL_RET_SUCC, APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSrvSocSendCnf(srvSocSendTcpClientReq->reqSource, APPL_RET_FAIL, APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF, result, NULL);
                    }
                }
                break;
            }
            case ECSRVSOCREQ_STATUSLISTEN:{
                EcSrvSocStatusTcpServerReq *srvSocStatusTcpServerReq;
                srvSocStatusTcpServerReq = (EcSrvSocStatusTcpServerReq *)atecEcSrvSocReq->reqBody;
                if(srvSocStatusTcpServerReq != NULL)
                {
                    result = atecSrvSocketHandleTcpListenStatusReq(srvSocStatusTcpServerReq);
                    if(result == CME_SUCC)
                    {
                        atecSrvSocSendCnf(srvSocStatusTcpServerReq->reqSource, APPL_RET_SUCC, APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF, 0, NULL);
                    }
                    else
                    {
                        atecSrvSocSendCnf(srvSocStatusTcpServerReq->reqSource, APPL_RET_FAIL, APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF, result, NULL);
                    }
                }
                break;
            }
            default:
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_5, P_WARNING, 1, "process atSktReqFd fail, invalid reqId %d", atecEcSrvSocReq->reqId);
                break;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtecSktProessReq_6, P_WARNING, 1, "read atSktReqFd fail, source %u check fail", atecEcSrvSocReq->source);
    }
}


void atSktEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg)
{

    if(mgrContext == NULL)
    {
        return;
    }

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktEvnetCallback_1, P_INFO, 2, "AtSktEvnetCallback socketid %d, event %d",mgrContext->sockId, eventType);

    switch(eventType)
    {
        case SOCK_EVENT_CONN_STATUS:{
            CmsSockMgrConnStatusArg *statusArg = (CmsSockMgrConnStatusArg *)eventArg;
            atSktStatusEventProcess(mgrContext, statusArg);
            break;
        }
        case SOCK_EVENT_CONN_DL:{
            CmsSockMgrConnDlArg *dlArg = (CmsSockMgrConnDlArg *)eventArg;
            atSktDlEventProcess(mgrContext, dlArg);
            break;
        }
        case SOCK_EVENT_CONN_ERROR:{
            CmsSockMgrConnErrArg *errArg = (CmsSockMgrConnErrArg *)eventArg;
            atSktSockErrorEventProcess(mgrContext, errArg);
            break;
        }
        case SOCK_EVENT_CONN_UL_STATUS:
        default:
            break;
    }

    return;
}

void atEcsocEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg)
{

    if(mgrContext == NULL)
    {
        return;
    }

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocEventCallback_1, P_INFO, 2, "AtEcsocEventCallback socketid %d, event %d",mgrContext->sockId, eventType);

    switch(eventType)
    {
        case SOCK_EVENT_CONN_STATUS:{
            CmsSockMgrConnStatusArg *statusArg = (CmsSockMgrConnStatusArg *)eventArg;
            atEcsocStatusEventProcess(mgrContext, statusArg);
            break;
        }
        case SOCK_EVENT_CONN_DL:{
            CmsSockMgrConnDlArg *dlArg = (CmsSockMgrConnDlArg *)eventArg;
            atEcsocDlEventProcess(mgrContext, dlArg);
            break;
        }
        case SOCK_EVENT_CONN_ERROR:{
            CmsSockMgrConnErrArg *errArg = (CmsSockMgrConnErrArg *)eventArg;
            atEcsocErrorEventProcess(mgrContext, errArg);
            break;
        }
        case SOCK_EVENT_CONN_UL_STATUS:{
            CmsSockMgrConnUlStatusArg *ulStatusArg = (CmsSockMgrConnUlStatusArg *)eventArg;
            atEcsocUlStatusEventProcess(mgrContext, ulStatusArg);
            break;
        }
        default:
            break;
    }

    return;
}

void atEcSrvSocEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg)
{

    if(mgrContext == NULL)
    {
        return;
    }

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocEventCallback_1, P_INFO, 2, "atEcSrvSocEventCallback socketid %d, event %d",mgrContext->sockId, eventType);

    switch(eventType)
    {
        case SOCK_EVENT_CONN_STATUS:{
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocEventCallback_2, P_INFO, 0, "atEcSrvSocEventCallback conn status event, do nothing");
            CmsSockMgrConnStatusArg *statusArg = (CmsSockMgrConnStatusArg *)eventArg;
            atEcSrvSocStatusEventProcess(mgrContext, statusArg);            
            break;
        }
        case SOCK_EVENT_CONN_DL:{
            CmsSockMgrConnDlArg *dlArg = (CmsSockMgrConnDlArg *)eventArg;
            atEcSrvSocDlEventProcess(mgrContext, dlArg);
            break;
        }
        case SOCK_EVENT_CONN_ERROR:{
            CmsSockMgrConnErrArg *errArg = (CmsSockMgrConnErrArg *)eventArg;
            atEcSrvSocErrorEventProcess(mgrContext, errArg);
            break;
        }
        case SOCK_EVENT_CONN_UL_STATUS:{
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrvSocEventCallback_3, P_INFO, 0, "atEcSrvSocEventCallback ul sequence status event, do nothing");
            CmsSockMgrConnUlStatusArg *ulStatusArg = (CmsSockMgrConnUlStatusArg *)eventArg;
            atEcSrcSocUlStatusEventProcess(mgrContext, ulStatusArg);
            break;
        }
        case SOCK_EVENT_CONN_TCP_ACCEPT_CLIENT:{
            CmsSockMgrConnAcceptClientArg *acceptClientArg = (CmsSockMgrConnAcceptClientArg *)eventArg;
            atEcSrcSocTcpClientAcceptEventProcess(mgrContext, acceptClientArg);
        }
        default:
            break;
    }

    return;
}


void atSktStoreConnHibContext(CmsSockMgrContext *sockMgrContext, CmsSockMgrConnHibContext *hibContext)
{
    AtsktConnHibPriContext *sktPriContext;
    AtecSktPriMgrContext *priMgrcontext;

    if(sockMgrContext == NULL || hibContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktStoreConnHibContext_1, P_WARNING, 0, "AtSktStoreConnHibContext context is invalid");
        return;
    }

    if(sizeof(AtsktConnHibPriContext) > CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktStoreConnHibContext_2, P_WARNING, 2, "AtSktStoreConnHibContext private hib context len %d bit than %d", sizeof(AtsktConnHibPriContext), CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH);
        return;
    }

    sktPriContext = (AtsktConnHibPriContext *)&hibContext->priContext;
    priMgrcontext = (AtecSktPriMgrContext *)sockMgrContext->priContext;

    if(sktPriContext && priMgrcontext)
    {
        memset(hibContext, 0, sizeof(CmsSockMgrConnHibContext));

        hibContext->sockId = sockMgrContext->sockId;
        hibContext->source = sockMgrContext->source;
        hibContext->type = sockMgrContext->type;
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
        hibContext->domain = sockMgrContext->domain;
        hibContext->status = sockMgrContext->status;

        sktPriContext->createReqHandle = priMgrcontext->createReqHandle;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktStoreConnHibContext_3, P_WARNING, 0, "AtSktStoreConnHibContext private context is invalid");
    }

    return;

}

void atEcsocStoreConnHibContext(CmsSockMgrContext *sockMgrContext, CmsSockMgrConnHibContext *hibContext)
{
    EcsocConnHibPriContext *sktPriContext;
    EcsocPriMgrContext *priMgrcontext;

    if(sockMgrContext == NULL || hibContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocStoreConnHibContext_1, P_WARNING, 0, "AtEcsocStoreConnHibContext context is invalid");
        return;
    }

    if(sizeof(EcsocConnHibPriContext) > CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocStoreConnHibContext_2, P_WARNING, 2, "AtEcsocStoreConnHibContext private hib context len %d bit than %d", sizeof(EcsocConnHibPriContext), CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH);
        return;
    }

    sktPriContext = (EcsocConnHibPriContext *)&hibContext->priContext;
    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;

    if(sktPriContext && priMgrcontext)
    {
        memset(hibContext, 0, sizeof(CmsSockMgrConnHibContext));

        hibContext->sockId = sockMgrContext->sockId;
        hibContext->source = sockMgrContext->source;
        hibContext->type = sockMgrContext->type;
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
        hibContext->domain = sockMgrContext->domain;
        hibContext->status = sockMgrContext->status;

        sktPriContext->modeSet = priMgrcontext->modeSet;
        sktPriContext->createReqHandle = priMgrcontext->createReqHandle;
        memcpy(&sktPriContext->dlPriSet, &priMgrcontext->dlPriSet, sizeof(EcSocDlPrivateSet));
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocStoreConnHibContext_3, P_WARNING, 0, "AtEcsocStoreConnHibContext private context is invalid");
    }

    return;

}

void atSktRecoverConnContext(CmsSockMgrConnHibContext *hibContext)
{
    AtsktConnHibPriContext *sktPriContext;
    AtecSktPriMgrContext *priMgrcontext;
    CmsSockMgrContext* sockMgrContext = NULL;
    ip_addr_t localAddr;
    ip_addr_t remoteAddr;
    UINT16 localPort;
    UINT16 remotePort;
    int rebuildType;
    CmsSockMgrConnStatus connStatus = SOCK_CONN_STATUS_CLOSED;

    if(hibContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_1, P_WARNING, 0, "AtSktRecoverConnContexts context is invalid");
        return;
    }

    if(sizeof(AtsktConnHibPriContext) > CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_2, P_WARNING, 2, "AtSktRecoverConnContext private hib context len %d bit than %d", sizeof(AtsktConnHibPriContext), CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH);
        return;
    }

    sktPriContext = (AtsktConnHibPriContext *)&hibContext->priContext;
    if(sktPriContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_3, P_WARNING, 0, "AtSktRecoverConnContexts private hib context is invalid");
        return;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(AtecSktPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_4, P_ERROR, 0, "AtSktRecoverConnContexts atskt is full");
        return;
    }

    priMgrcontext = (AtecSktPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(hibContext->sockId >= 0)
    {
        if(hibContext->status == SOCK_CONN_STATUS_CONNECTED)
        {
            if(netconn_get_sock_info(hibContext->sockId, &localAddr, &remoteAddr, &localPort, &remotePort, &rebuildType) == ERR_OK)
            {
                connStatus = cmsSockMgrRebuildSocket(hibContext->sockId, &localAddr, &remoteAddr, &localPort, &remotePort, rebuildType);
                if(connStatus == SOCK_CONN_STATUS_CLOSED)
                {
                    cmsSockMgrRemoveContext(sockMgrContext);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_6, P_ERROR, 1, "AtSktRecoverConnContexts rebuild socket id %d fail", hibContext->sockId);
                }
                else if(connStatus == SOCK_CONN_STATUS_CREATED)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_7, P_ERROR, 1, "AtSktRecoverConnContexts rebuild socket id %d success,but status is not valid", hibContext->sockId);
                    cmsSockMgrCloseSocket(hibContext->sockId);
                    cmsSockMgrRemoveContext(sockMgrContext);
                }else if(connStatus == SOCK_CONN_STATUS_CONNECTED)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_8, P_INFO, 1, "AtSktRecoverConnContexts rebuild socket id %d success", hibContext->sockId);
                    sockMgrContext->sockId = hibContext->sockId;
                    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                    sockMgrContext->hibEnable = TRUE;
                    sockMgrContext->source = hibContext->source;
                    sockMgrContext->type = hibContext->type;
                    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
                    sockMgrContext->domain = hibContext->domain;
                    sockMgrContext->localPort = localPort;
                    ip_addr_copy(sockMgrContext->localAddr, localAddr);

                    priMgrcontext->createReqHandle = sktPriContext->createReqHandle;
                }
            }
        }
        else if(hibContext->status == SOCK_CONN_STATUS_CREATED)
        {
            connStatus = cmsSockMgrReCreateSocket(hibContext->sockId, hibContext->domain, hibContext->type);
            if(connStatus == SOCK_CONN_STATUS_CLOSED)
            {
                cmsSockMgrRemoveContext(sockMgrContext);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_9, P_ERROR, 1, "AtSktRecoverConnContexts recreate socket id %d fail", hibContext->sockId);
            }
            else if(connStatus == SOCK_CONN_STATUS_CREATED)
            {
                if(hibContext->localPort || !ip_addr_isany_val(hibContext->localAddr))
                {
                    if(cmsSockMgrBindSocket(hibContext->sockId, hibContext->domain, hibContext->localPort, &hibContext->localAddr) == CME_SUCC)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_success_1, P_INFO, 1, "AtSktRecoverConnContexts recreate socket id %d success", hibContext->sockId);
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
                sockMgrContext->domain = hibContext->domain;
                        sockMgrContext->localPort = hibContext->localPort;
                        ip_addr_copy(sockMgrContext->localAddr, hibContext->localAddr);
                priMgrcontext->createReqHandle = sktPriContext->createReqHandle;

                cmsSockMgrEnableHibMode(sockMgrContext);
                    }
                    else
                    {
                        cmsSockMgrCloseSocket(hibContext->sockId);
                        cmsSockMgrRemoveContext(sockMgrContext);
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_bind_fail, P_ERROR, 1, "AtSktRecoverConnContexts recreate socket id %d bind fail", hibContext->sockId);
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_success_2, P_INFO, 1, "AtSktRecoverConnContexts recreate socket id %d success", hibContext->sockId);
                    sockMgrContext->sockId = hibContext->sockId;
                    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                    sockMgrContext->source = hibContext->source;
                    sockMgrContext->type = hibContext->type;
                    sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
                    sockMgrContext->domain = hibContext->domain;
                    priMgrcontext->createReqHandle = sktPriContext->createReqHandle;
                    cmsSockMgrEnableHibMode(sockMgrContext);
                }
            }
            else
            {
                cmsSockMgrRemoveContext(sockMgrContext);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_11, P_ERROR, 2, "AtSktRecoverConnContexts recreate socket id %d status %d is invalid", hibContext->sockId, connStatus);
            }
        }
        else
        {
            cmsSockMgrRemoveContext(sockMgrContext);
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_12, P_ERROR, 2, "AtSktRecoverConnContexts hib socket id %d status %d is invalid", hibContext->sockId, hibContext->status);      
        }
    }
    else
    {
        cmsSockMgrRemoveContext(sockMgrContext);
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_13, P_ERROR, 1, "AtSktRecoverConnContexts rebuild socket id %d is invalid", hibContext->sockId);
    }

    return;

}


void atEcsocRecoverConnContext(CmsSockMgrConnHibContext *hibContext)
{
    EcsocConnHibPriContext *sktPriContext;
    EcsocPriMgrContext *priMgrcontext;

    CmsSockMgrContext* sockMgrContext = NULL;
    ip_addr_t localAddr;
    ip_addr_t remoteAddr;
    UINT16 localPort;
    UINT16 remotePort;
    int rebuildType;
    CmsSockMgrConnStatus connStatus = SOCK_CONN_STATUS_CLOSED;

    if(hibContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_1, P_WARNING, 0, "AtEcsocRecoverConnContext context is invalid");
        return;
    }

    if(sizeof(EcsocConnHibPriContext) > CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_2, P_WARNING, 2, "AtEcsocRecoverConnContext private hib context len %d bit than %d", sizeof(EcsocConnHibPriContext), CMS_SOCK_MGR_HIB_PRIVATE_CONTEXT_MAX_LENGTH);
        return;
    }

    sktPriContext = (EcsocConnHibPriContext *)&hibContext->priContext;
    if(sktPriContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_3, P_WARNING, 0, "AtEcsocRecoverConnContext private hib context is invalid");
        return;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(EcsocPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_4, P_ERROR, 0, "AtEcsocRecoverConnContext atskt is full");
        return;
    }

    priMgrcontext = (EcsocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    if(hibContext->sockId >= 0)
    {
        if(hibContext->status == SOCK_CONN_STATUS_CONNECTED)
        {
            if(netconn_get_sock_info(hibContext->sockId, &localAddr, &remoteAddr, &localPort, &remotePort, &rebuildType) == ERR_OK)
            {
                connStatus = cmsSockMgrRebuildSocket(hibContext->sockId, &localAddr, &remoteAddr, &localPort, &remotePort, rebuildType);
                if(connStatus == SOCK_CONN_STATUS_CLOSED)
                {
                    cmsSockMgrRemoveContext(sockMgrContext);
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_6, P_ERROR, 1, "AtEcsocRecoverConnContext rebuild socket id %d fail", hibContext->sockId);
                }
                else if(connStatus == SOCK_CONN_STATUS_CREATED)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_7, P_ERROR, 1, "AtEcsocRecoverConnContext rebuild socket id %d success,but status is not valid", hibContext->sockId);
                    cmsSockMgrCloseSocket(hibContext->sockId);
                    cmsSockMgrRemoveContext(sockMgrContext);
                }else if(connStatus == SOCK_CONN_STATUS_CONNECTED)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_8, P_INFO, 1, "AtEcsocRecoverConnContext rebuild socket id %d success", hibContext->sockId);
                    sockMgrContext->sockId = hibContext->sockId;
                    sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                    sockMgrContext->hibEnable = TRUE;
                    sockMgrContext->source = hibContext->source;
                    sockMgrContext->type = hibContext->type;
                    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
                    sockMgrContext->domain = hibContext->domain;
                    sockMgrContext->localPort = localPort;
                    ip_addr_copy(sockMgrContext->localAddr, localAddr);

                    priMgrcontext->createReqHandle = sktPriContext->createReqHandle;
                    priMgrcontext->modeSet = sktPriContext->modeSet;
                    memcpy(&priMgrcontext->dlPriSet, &sktPriContext->dlPriSet, sizeof(EcSocDlPrivateSet));
                }
            }
        }
        else if(hibContext->status == SOCK_CONN_STATUS_CREATED)
        {
            connStatus = cmsSockMgrReCreateSocket(hibContext->sockId, hibContext->domain, hibContext->type);
            if(connStatus == SOCK_CONN_STATUS_CLOSED)
            {
                cmsSockMgrRemoveContext(sockMgrContext);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_9, P_ERROR, 1, "AtEcsocRecoverConnContext recreate socket id %d fail", hibContext->sockId);
            }
            else if(connStatus == SOCK_CONN_STATUS_CREATED)
            {
                if(hibContext->localPort || !ip_addr_isany_val(hibContext->localAddr))
                {
                    if(cmsSockMgrBindSocket(hibContext->sockId, hibContext->domain, hibContext->localPort, &hibContext->localAddr) == CME_SUCC)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtSktRecoverConnContext_success_1, P_INFO, 1, "AtSktRecoverConnContexts recreate socket id %d success", hibContext->sockId);
                sockMgrContext->sockId = hibContext->sockId;
                sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                sockMgrContext->source = hibContext->source;
                sockMgrContext->type = hibContext->type;
                        sockMgrContext->domain = hibContext->domain;
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
                        priMgrcontext->createReqHandle = sktPriContext->createReqHandle;
                        priMgrcontext->modeSet = sktPriContext->modeSet;
                        memcpy(&priMgrcontext->dlPriSet, &sktPriContext->dlPriSet, sizeof(EcSocDlPrivateSet));
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
                    ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_rebuild_success_2, P_INFO, 1, "AtEcsocRecoverConnContext recreate socket id %d success", hibContext->sockId);
                     sockMgrContext->sockId = hibContext->sockId;
                     sockMgrContext->eventCallback = (CmsSockMgrEventCallback)hibContext->eventCallback;
                     sockMgrContext->source = hibContext->source;
                     sockMgrContext->type = hibContext->type;
                     sockMgrContext->status = SOCK_CONN_STATUS_CREATED;
                sockMgrContext->domain = hibContext->domain;
                
                priMgrcontext->createReqHandle = sktPriContext->createReqHandle;
                priMgrcontext->modeSet = sktPriContext->modeSet;
                memcpy(&priMgrcontext->dlPriSet, &sktPriContext->dlPriSet, sizeof(EcSocDlPrivateSet));

                cmsSockMgrEnableHibMode(sockMgrContext);
                }
            }
            else
            {
                cmsSockMgrRemoveContext(sockMgrContext);
                ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_11, P_ERROR, 2, "AtEcsocRecoverConnContext recreate socket id %d status %d is invalid", hibContext->sockId, connStatus);
            }        
        }
        else
        {
            cmsSockMgrRemoveContext(sockMgrContext);
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_12, P_ERROR, 2, "AtEcsocRecoverConnContext hib socket id %d status %d is invalid", hibContext->sockId, hibContext->status);
        }
    }
    else
    {
        cmsSockMgrRemoveContext(sockMgrContext);
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, AtEcsocRecoverConnContext_13, P_ERROR, 1, "AtEcsocRecoverConnContext rebuild socket id %d is invalid", hibContext->sockId);
    }

    return;

}

BOOL atEcSrcSocTcpServerProcessAcceptClient(CmsSockMgrContext* gMgrContext, INT32 clientSocketId, ip_addr_t *clientAddr, UINT16 clientPort)
{
    CmsSockMgrContext* sockMgrContext = NULL;
    EcSrvSocPriMgrContext *priMgrcontext;
    EcSrvSocPriMgrContext *fPriMgrcontext;

    if(gMgrContext == PNULL || clientAddr == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrcSocTcpServerProcessAcceptClient_1, P_ERROR, 1, "atEcSrcSocTcpServerProcessAcceptClient cms mgr context or client address is invalid");
        return FALSE;
    }

    sockMgrContext = cmsSockMgrGetFreeMgrContext(sizeof(EcSrvSocPriMgrContext));
    if(sockMgrContext == NULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atEcSrcSocTcpServerProcessAcceptClient_2, P_ERROR, 0, "atEcSrcSocTcpServerProcessAcceptClient cms sock mgr context is full");
        return FALSE;
    }

    priMgrcontext = (EcSrvSocPriMgrContext *)sockMgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);
    
    fPriMgrcontext = (EcSrvSocPriMgrContext *)gMgrContext->priContext;
    GosCheck(fPriMgrcontext != PNULL, fPriMgrcontext, 0, 0);


    sockMgrContext->bServer = FALSE;
    sockMgrContext->eventCallback = gMgrContext->eventCallback;
    sockMgrContext->hibEnable = gMgrContext->hibEnable;
    sockMgrContext->source = gMgrContext->source;
    sockMgrContext->type = gMgrContext->type;
    sockMgrContext->sockId = clientSocketId;
    sockMgrContext->status = SOCK_CONN_STATUS_CONNECTED;
    sockMgrContext->domain = gMgrContext->domain;

    priMgrcontext->fSocketId = gMgrContext->sockId;
    priMgrcontext->createReqHandle = fPriMgrcontext->createReqHandle;  

    return TRUE;

}


BOOL EcsocCheckHibMode(CmsSockMgrContext *mgrContext)
{
    EcsocPriMgrContext *priMgrcontext;
    UINT8 sequence;

    if(mgrContext == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, EcsocCheckHibMode_1, P_WARNING, 0, "EcsocCheckHibMode invalid cms sock mgr context");
        return TRUE;
    }

    priMgrcontext = (EcsocPriMgrContext *)mgrContext->priContext;
    GosCheck(priMgrcontext != PNULL, priMgrcontext, 0, 0);

    //first check any pending UL list
    if(priMgrcontext->ulList != PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, EcsocCheckHibMode_2, P_INFO, 1, "EcsocCheckHibMode socket id %u exist pending UL", mgrContext->sockId);
        return FALSE;
    }

    //first check any pending DL list
    if(priMgrcontext->dlList != PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, EcsocCheckHibMode_3, P_INFO, 1, "EcsocCheckHibMode socket id %u exist pending DL", mgrContext->sockId);
        return FALSE;
    }

    //check any pending UL sequence status
    for(sequence = CMS_SOCK_MGR_UL_DATA_SEQUENCE_MIN; sequence <= CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX; sequence ++)
    {
        //check the UL sequence status indication bitmap
        if(cmsSockMgrGetUlPendingSequenceState(priMgrcontext->ulSeqStatus.bitmap, sequence) == TRUE)
        {
            ECOMM_TRACE(UNILOG_ATCMD_SOCK, EcsocCheckHibMode_4, P_INFO, 2, "EcsocCheckHibMode socket id %u exist pending UL sequence %ustatus", mgrContext->sockId, sequence);
            return FALSE;
        }

        if(sequence == CMS_SOCK_MGR_UL_DATA_SEQUENCE_MAX)
        {
            break;
        }
    }

    return TRUE;

}

BOOL EcSrvsocCheckHibMode(CmsSockMgrContext *mgrContext)
{

    if(mgrContext == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, EcSrvsocCheckHibMode_1, P_WARNING, 0, "EcSrvsocCheckHibMode invalid cms sock mgr context");
        return TRUE;
    }

    //server mode disable enter PSM mode
    return FALSE;

}



void atSocketInit(void)
{
    UINT8 mode = NMI_MODE_1;
    UINT8 publicDlPkgNumMax = AT_SOC_PUBLIC_DL_PKG_NUM_DEF;
    UINT16 publicDlBufferToalSize = AT_SOC_PUBLIC_DL_BUFFER_DEF;

    CmsSockMgrHandleDefine atskthandleDefine;
    CmsSockMgrHandleDefine ecsochandleDefine;
    CmsSockMgrHandleDefine ecsrvsochandleDefine;

    atskthandleDefine.source = SOCK_SOURCE_ATSKT;
    atskthandleDefine.hibCheck = NULL;
    atskthandleDefine.recoverHibContext = atSktRecoverConnContext;
    atskthandleDefine.reqProcess = atecSktProessReq;
    atskthandleDefine.storeHibContext = atSktStoreConnHibContext;
    atskthandleDefine.tcpServerProcessAcceptClient = PNULL;

    ecsochandleDefine.source = SOCK_SOURCE_ECSOC;
    ecsochandleDefine.hibCheck = EcsocCheckHibMode;
    ecsochandleDefine.recoverHibContext = atEcsocRecoverConnContext;
    ecsochandleDefine.reqProcess = atecEcsocProessReq;
    ecsochandleDefine.storeHibContext = atEcsocStoreConnHibContext;
    ecsochandleDefine.tcpServerProcessAcceptClient = PNULL;

    ecsrvsochandleDefine.source = SOCK_SOURCE_ECSRVSOC;
    ecsrvsochandleDefine.hibCheck = EcSrvsocCheckHibMode;
    ecsrvsochandleDefine.recoverHibContext = PNULL;
    ecsrvsochandleDefine.reqProcess = atecEcSrvSocProessReq;
    ecsrvsochandleDefine.storeHibContext = PNULL;
    ecsrvsochandleDefine.tcpServerProcessAcceptClient = atEcSrcSocTcpServerProcessAcceptClient;


    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atSocketInit_1, P_SIG, 0, "AtSocketInit INIT");

    if(cmsSockMgrRegisterHandleDefine(&atskthandleDefine) != TRUE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atSocketInit_2, P_ERROR, 0, "AtSocketInit ATSKT register fail");
    }

    if(cmsSockMgrRegisterHandleDefine(&ecsochandleDefine) != TRUE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atSocketInit_3, P_ERROR, 0, "AtSocketInit ECSOC register fail");
    }

    if(cmsSockMgrRegisterHandleDefine(&ecsrvsochandleDefine) != TRUE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_SOCK, atSocketInit_4, P_ERROR, 0, "AtSocketInit ECSRVSOC register fail");
    }

    //load ec soc public config
#ifdef FEATURE_REF_AT_ENABLE
    mwGetAonEcSocPublicConfig(&mode, &publicDlPkgNumMax, &publicDlBufferToalSize);
#else
    mwGetEcSocPublicConfig(&mode, &publicDlPkgNumMax, &publicDlBufferToalSize);
#endif

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, atSocketInit_5, P_INFO, 3, "AtSocketInit ECSOC public setting mode %u, publicDlPkgNumMax %u, publicDlBufferToalSize %u",
        mode, publicDlPkgNumMax, publicDlBufferToalSize);


    gecSocPubDlSet.mode = mode;
    gecSocPubDlSet.publicDlPkgNumMax = publicDlPkgNumMax;
    gecSocPubDlSet.publicDlBufferToalSize = publicDlBufferToalSize;

    return;
}

