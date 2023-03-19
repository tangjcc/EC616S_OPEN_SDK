/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: cms_sock_mgr.c
 *
 *  Description: cms socket manager API
 *
 *  History:creat by xwang
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


/******************************************************************************
 *****************************************************************************
 * MACRO
 *****************************************************************************
******************************************************************************/





/******************************************************************************
 *****************************************************************************
 * structure
 *****************************************************************************
******************************************************************************/
typedef struct GMgrContext_Tag{
    UINT8 contextNum;
    CmsSockMgrContext *contextList;
}GMgrContext;

/******************************************************************************
 *****************************************************************************
 * static VARIABLES
 *****************************************************************************
******************************************************************************/

static GMgrContext gSockMgrContext = {0, NULL};


/******************************************************************************
 *****************************************************************************
 * function
 *****************************************************************************
******************************************************************************/
/**
  \fn           UINT32 cmsSockMgrGetCurrHibTicks(void)
  \brief        get sys hib ticks(econds)
*/

UINT32 cmsSockMgrGetCurrHibTicks(void)
{
    return pmuGetHibSecondCount();
}

/**
  \fn           UINT32 cmsSockMgrGetCurrentSysTicks(void)
  \brief        get sys ticks(econds)
*/
UINT32 cmsSockMgrGetCurrentSysTicks(void)
{
    UINT32 currentTicks;

    currentTicks = osKernelGetTickCount()/portTICK_PERIOD_MS;

    return currentTicks;
}

/**
  \fn           CmsSockMgrContext* cmsSockMgrGetContextList(void)
  \brief        get the global sockmgr context list
*/
CmsSockMgrContext* cmsSockMgrGetContextList(void)
{
    return gSockMgrContext.contextList;
}

/**
  \fn           CmsSockMgrContext* cmsSockMgrGetFreeMgrContext(UINT16 priLen)
  \brief        get a free sockmgr context
*/
CmsSockMgrContext* cmsSockMgrGetFreeMgrContext(UINT16 priLen)
{
    CmsSockMgrContext *contextTmp;
    CmsSockMgrContext *contextNext;

    //check the total context number
    if(gSockMgrContext.contextNum >= CMS_SOCK_MGR_CONTEXT_NUM_MAX)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrGetFreeMgrContext_1, P_WARNING, 0, "CmsMgrGetFreeMgrContext reach MAX num");
        return NULL;
    }

    //malloc sock mgr context
    contextTmp = (CmsSockMgrContext *)malloc(sizeof(CmsSockMgrContext) + priLen);

    if(contextTmp == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrGetFreeMgrContext_2, P_ERROR, 0, "CmsMgrGetFreeMgrContext malloc fail");
        return NULL;
    }
    else
    {
        //init the sock mgr context
        memset(contextTmp, 0, sizeof(CmsSockMgrContext) + priLen);
        contextTmp->sockId = -1;
        contextTmp->hibEnable = FALSE;
        contextTmp->bServer = FALSE;
        contextTmp->status = SOCK_CONN_STATUS_CLOSED;
        contextTmp->localPort = 0;

        contextNext = gSockMgrContext.contextList;
        gSockMgrContext.contextList = contextTmp;
        contextTmp->next = contextNext;
        gSockMgrContext.contextNum++;

        return contextTmp;
    }

}

/**
  \fn           CmsSockMgrContext* cmsSockMgrFindMgrContextById(INT32 id)
  \brief        find the sockmgr context by socket id
*/
CmsSockMgrContext* cmsSockMgrFindMgrContextById(INT32 id)
{
    CmsSockMgrContext *contextTmp = NULL;

    for(contextTmp = gSockMgrContext.contextList; contextTmp != NULL; contextTmp = contextTmp->next)
    {
        if(contextTmp->sockId == id)
        {
            break;
        }
    }

    return contextTmp;

}

/**
  \fn           void cmsSockMgrRemoveContextById(INT32 id)
  \brief        remove the sockmgr context by socket id
*/
void cmsSockMgrRemoveContextById(INT32 id)
{

    CmsSockMgrContext *contextTmp = NULL;
    CmsSockMgrContext *contextPre = NULL;

    for(contextTmp = gSockMgrContext.contextList; contextTmp != NULL; contextTmp = contextTmp->next)
    {
        if(contextTmp->sockId == id)
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
            gSockMgrContext.contextList = contextTmp->next;
        }
        else
        {
            contextPre->next = contextTmp->next;
        }
        gSockMgrContext.contextNum--;
        free(contextTmp);
    }

    return;

}

/**
  \fn           void cmsSockMgrRemoveContext(CmsSockMgrContext *context)
  \brief        remove the sockmgr context
*/
void cmsSockMgrRemoveContext(CmsSockMgrContext *context)
{

    CmsSockMgrContext *contextTmp = NULL;
    CmsSockMgrContext *contextPre = NULL;

    for(contextTmp = gSockMgrContext.contextList; contextTmp != NULL; contextTmp = contextTmp->next)
    {
        if(contextTmp == context)
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
            gSockMgrContext.contextList = contextTmp->next;
        }
        else
        {
            contextPre->next = contextTmp->next;
        }
        gSockMgrContext.contextNum--;
        free(contextTmp);
    }

    return;

}

/**
  \fn           BOOL cmsSockMgrEnableHibMode(CmsSockMgrContext *context)
  \brief        enable the socket hib/sleep2 mode of the sock mgr context
*/
BOOL cmsSockMgrEnableHibMode(CmsSockMgrContext *context)
{
    BOOL ret = FALSE;
    INT32 hibOpt = 1;

    if(context == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnableHibMode_1, P_ERROR, 0, "cmsSockMgrEnableHibMode invalid parameter");
        return FALSE;
    }

    //enable the socket hib/sleep2 option
    if(lwip_setsockopt(context->sockId, SOL_SOCKET, SO_HIB_SLEEP2, &hibOpt, sizeof(hibOpt)) < 0)
    {
        context->hibEnable = FALSE;
        ret = FALSE;
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnableHibMode_2, P_WARNING, 0, "cmsSockMgrEnableHibMode set hib/sleep2 flags fail");
    }
    else
    {
        context->hibEnable = TRUE;
        ret = TRUE;
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrEnableHibMode_3, P_INFO, 0, "cmsSockMgrEnableHibMode set hib/sleep2 flags success");
    }

    return ret;
}

/**
  \fn           INT32 cmsSockMgrCreateSocket(INT32 domain, INT32 type, INT32 protocol, INT32 expect_fd)
  \brief        create a socket
*/
INT32 cmsSockMgrCreateSocket(INT32 domain, INT32 type, INT32 protocol, INT32 expect_fd)
{
    INT32 fd = -1;

    fd = socket_expect(domain, type, protocol, expect_fd);
    if(fd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCreateSocket_1, P_ERROR, 0, "cmsSockMgrCreateSocket create socket fail");
        return -1;
    }

    INT32 flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCreateSocket_2, P_ERROR, 0, "cmsSockMgrCreateSockets get file cntrl flags fail");
        close(fd);
        return -1;
    }

    fcntl(fd, F_SETFL, flags|O_NONBLOCK);
//    lwip_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
//    lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return fd;
}

/**
  \fn           INT32 cmsSockMgrCreateTcpSrvSocket(INT32 fd, UINT8 domain, UINT16 localPort, ip_addr_t *localAddr)
  \brief        create tcp server
*/
INT32 cmsSockMgrCreateTcpSrvSocket(INT32 domain, UINT16 listenPort, ip_addr_t *bindAddr, UINT8 listenNum, INT32 expect_fd)
{
    INT32 fd = -1;

    //create socket
    fd = socket_expect(domain, SOCK_STREAM, IPPROTO_TCP, expect_fd);
    if(fd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCreateTcpSrvSocket_1, P_ERROR, 0, "cmsSockMgrCreateTcpSrvSocket create socket fail");
        return -1;
    }

    INT32 flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCreateTcpSrvSocket_2, P_ERROR, 0, "cmsSockMgrCreateTcpSrvSocket get file cntrl flags fail");
        close(fd);
        return -1;
    }

    //set the socket nonblock mode
    fcntl(fd, F_SETFL, flags|O_NONBLOCK);

    //bind listen port and listen address
    if(cmsSockMgrBindSocket(fd, domain, listenPort, bindAddr) != CME_SUCC)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCreateTcpSrvSocket_3, P_ERROR, 0, "cmsSockMgrCreateTcpSrvSocket bind listen port or lisen address fail");
        close(fd);
        return -1;
    }

    //set the listen num
    if(listen(fd, listenNum) != 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCreateTcpSrvSocket_4, P_ERROR, 0, "cmsSockMgrCreateTcpSrvSocket bind listen port or lisen address fail");
        close(fd);
        return -1;
    }

    return fd;

}

/**
  \fn           INT32 cmsSockMgrBindSocket(INT32 fd, UINT8 domain, UINT16 localPort, ip_addr_t *localAddr)
  \brief        bind local socket (port or address)
*/
INT32 cmsSockMgrBindSocket(INT32 fd, UINT8 domain, UINT16 localPort, ip_addr_t *localAddr)
{
    struct sockaddr_in addr;
    struct sockaddr_in6 servaddr6;
    INT32 ret = CME_SUCC;

    if(fd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrBindSocket_1, P_ERROR, 0, "cmsSockMgrBindSocket invalid param");
        ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
        return ret;
    }

    switch(domain){
        case AF_INET:
            memset(&addr, 0, sizeof(struct sockaddr_in));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(localPort);

            if(IP_IS_V6_VAL(*localAddr) && (!ip_addr_isany_val(*localAddr)))
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrBindSocket_2, P_ERROR, 0, "cmsSockMgrBindSocket bind fail,because local address is ipv6 address");
                return CME_SOCK_MGR_ERROR_PARAM_ERROR;
            }

            inet_addr_from_ip4addr(&addr.sin_addr, ip_2_ip4(localAddr));
            if(bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrBindSocket_3, P_ERROR, 0, "cmsSockMgrBindSocket bind fail");
                ret = CME_SOCK_MGR_ERROR_BIND_FAIL;
            }
            break;
        case AF_INET6:
            memset(&servaddr6, 0, sizeof(struct sockaddr_in6));
            servaddr6.sin6_family = AF_INET6;
            servaddr6.sin6_port = htons(localPort);

            /*only to check the ipv6 address is not a NULL scenario,as when it's a NULL, type is IPADDR_TYPE_V4 default */
            if(IP_IS_V4_VAL(*localAddr) && (!ip_addr_isany_val(*localAddr)))
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrBindSocket_4, P_ERROR, 0, "cmsSockMgrBindSocket bind fail,because local address is ipv4 address");
                return CME_SOCK_MGR_ERROR_PARAM_ERROR;
            }
            inet6_addr_from_ip6addr(&servaddr6.sin6_addr, ip_2_ip6(localAddr));

            if(bind(fd, (struct sockaddr *)&servaddr6, sizeof(struct sockaddr_in6)) != 0)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrBindSocket_5, P_ERROR, 0, "cmsSockMgrBindSocket bind fail");
                ret = CME_SOCK_MGR_ERROR_BIND_FAIL;
            }
            break;
        default:
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrBindSocket_6, P_ERROR, 0, "cmsSockMgrBindSocket invalid domain type");
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
            break;
    }

    return ret;

}

/**
  \fn           UINT32 cmsSockMgrConnectTimeout(INT32 connectFd, UINT32 timeout)
  \brief        the socket connect with or without timeout
*/
UINT32 cmsSockMgrConnectTimeout(INT32 connectFd, UINT32 timeout)
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
    INT32 ret;

    ret = select(connectFd+1, NULL, &writeSet, &errorSet, &tv);
    if(ret < 0)
    {
        int mErr = sock_get_errno(connectFd);
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectTimeout_1, P_WARNING, 1, "connect result=-1 get errno=115(EINPROGRESS) select<0 get errno=%d", mErr);
        return CME_SOCK_MGR_ERROR_CONNECT_FAIL;
    }
    else if(ret == 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectTimeout_2, P_WARNING, 1, "connect result=-1 get select== 0, timeout");
        return CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT;
    }
    else
    {
        if(FD_ISSET(connectFd, &errorSet))
        {
            int mErr = sock_get_errno(connectFd);
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectTimeout_3, P_WARNING, 1, "connect result=-1 get select<0 get errno=%d", mErr);
            if(mErr)
            {
                return CME_SOCK_MGR_ERROR_CONNECT_FAIL;
            }
        }
        else if(FD_ISSET(connectFd, &writeSet))
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectTimeout_4, P_WARNING, 0, "connect result=-1 get errno=115(EINPROGRESS) select>=0 connect success");
        }
    }

    return CME_SUCC;

}

/**
  \fn           UINT32 cmsSockMgrConnectTimeout(INT32 connectFd, UINT32 timeout)
  \brief        the socket connect
*/
INT32 cmsSockMgrConnectSocket(INT32 fd, UINT8 domain, UINT16 remotePort, ip_addr_t *remoteAddr, BOOL withTimeout)
{

    UINT32 ret = CME_SUCC;
    INT32 errCode;
    struct sockaddr_in addr;
    struct sockaddr_in6 servaddr6;

    switch(domain){
        case AF_INET:
        {
            memset(&addr, 0, sizeof(struct sockaddr_in));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(remotePort);
            inet_addr_from_ip4addr(&addr.sin_addr, ip_2_ip4(remoteAddr));

            if(connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == 0)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_1, P_INFO, 0, "cmsSockMgrConnectSocket connect success");
            }
            else
            {
                errCode = sock_get_errno(fd);
                if(errCode == EINPROGRESS)
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_2, P_ERROR, 0, "cmsSockMgrConnectSocket connect is ongoing");
                    if(withTimeout == TRUE)
                    {
                        ret = cmsSockMgrConnectTimeout(fd, 9);
                        if(ret == CME_SUCC)
                        {
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_3, P_INFO, 0, "cmsSockMgrConnectSocket connect success");
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_4, P_ERROR, 1, "cmsSockMgrConnectSocket connect fail,error code %d", ret);
                        }
                    }
                    else
                    {
                        ret = CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING;
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_5, P_ERROR, 0, "cmsSockMgrConnectSocket connect fail");
                    ret = CME_SOCK_MGR_ERROR_CONNECT_FAIL;
                }
            }
            break;
        }
        case AF_INET6:
        {
            memset(&servaddr6, 0, sizeof(struct sockaddr_in6));
            servaddr6.sin6_family = AF_INET6;
            servaddr6.sin6_port = htons(remotePort);
            inet6_addr_from_ip6addr(&servaddr6.sin6_addr, ip_2_ip6(remoteAddr));

            if(connect(fd, (struct sockaddr *)&servaddr6, sizeof(struct sockaddr_in6)) == 0)
            {
                ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_6, P_INFO, 0, "cmsSockMgrConnectSocket connect success");
            }
            else
            {
                errCode = sock_get_errno(fd);
                if(errCode == EINPROGRESS)
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_7, P_ERROR, 0, "cmsSockMgrConnectSocket connect is ongoing");
                    if(withTimeout == TRUE)
                    {
                        ret = cmsSockMgrConnectTimeout(fd, 9);
                        if(ret == CME_SUCC)
                        {
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_8, P_INFO, 0, "cmsSockMgrConnectSocket connect success");
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_9, P_ERROR, 1, "cmsSockMgrConnectSocket connect fail,error code %d", errCode);
                            if(socket_error_is_fatal(errCode) == TRUE)
                            {
                                ret = CME_SOCK_MGR_ERROR_SOCK_ERROR;
                            }
                        }
                    }
                    else
                    {
                        ret = CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING;
                    }

                }
                else
                {
                    ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_10, P_ERROR, 0, "cmsSockMgrConnectSocket connect fail");
                    ret = CME_SOCK_MGR_ERROR_CONNECT_FAIL;
                }
            }
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrConnectSocket_11, P_ERROR, 0, "cmsSockMgrConnectSocket invalid domain type");
            ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
            break;
        }
    }

   return ret;
}

/**
  \fn           UINT32 cmsSockMgrConnectTimeout(INT32 connectFd, UINT32 timeout)
  \brief        close the socket
*/
INT32 cmsSockMgrCloseSocket(INT32 fd)
{
    INT32 ret = CME_SUCC;

    if(fd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCloseSocket_1, P_ERROR, 0, "cmsSockMgrCloseSocket invalid fd");
        ret = CME_SOCK_MGR_ERROR_PARAM_ERROR;
    }

    if(close(fd) == 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCloseSocket_2, P_INFO, 0, "cmsSockMgrCloseSocket close socket success");
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrCloseSocket_3, P_ERROR, 0, "cmsSockMgrCloseSocket close socket fail");
        ret = CME_SOCK_MGR_ERROR_DELETE_FAIL;
    }

    return ret;
}


/**
  \fn           CmsSockMgrConnStatus cmsSockMgrRebuildSocket(INT32 fd, ip_addr_t *localAddr, ip_addr_t *remoteAddr, UINT16 *localPort, UINT16 *remotePort, INT32 type)
  \brief        rebuild the hib/sleep2 socket context
*/
CmsSockMgrConnStatus cmsSockMgrRebuildSocket(INT32 fd, ip_addr_t *localAddr, ip_addr_t *remoteAddr, UINT16 *localPort, UINT16 *remotePort, INT32 type)
{
    INT32 rebuildFd = -1;
    INT32 domain;
    INT32 protocol;
    INT32 sockType;
    CmsSockMgrConnStatus ret = SOCK_CONN_STATUS_CLOSED;

    if(IP_IS_V6(localAddr) || IP_IS_V6(remoteAddr))
    {
        domain = AF_INET6;
    }
    else if(IP_IS_V4(localAddr) || IP_IS_V4(remoteAddr))
    {
        domain = AF_INET;
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_1, P_ERROR, 0, "cmsSockMgrRebuildSocket invalid domain");
        return SOCK_CONN_STATUS_CLOSED;
    }

    switch(type)
    {
        case SOCK_DGRAM_TYPE:
        {
            protocol = IPPROTO_UDP;
            sockType = SOCK_DGRAM;
            break;
        }
        case SOCK_STREAM_TYPE:
        {
            protocol = IPPROTO_TCP;
            sockType = SOCK_STREAM;
            break;
        }
        default:
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_2, P_ERROR, 0, "cmsSockMgrRebuildSocket invalid type");
            return SOCK_CONN_STATUS_CLOSED;
    }

    rebuildFd = cmsSockMgrCreateSocket(domain, sockType, protocol, fd);

    if(rebuildFd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_3, P_ERROR, 0, "cmsSockMgrRebuildSocket create socket fail");
        return SOCK_CONN_STATUS_CLOSED;
    }
    else
    {
        ret = SOCK_CONN_STATUS_CREATED;
    }

    if(*localPort != 0 || !ip_addr_isany_val(*localAddr))
    {
        if(cmsSockMgrBindSocket(rebuildFd, domain, *localPort, localAddr) != 0)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_4, P_ERROR, 0, "cmsSockMgrRebuildSocket bind socket fail");
            close(rebuildFd);
            return SOCK_CONN_STATUS_CLOSED;
        }
        else
        {
            if(type == SOCK_DGRAM_TYPE)
            {
                ret = SOCK_CONN_STATUS_CONNECTED;
            }
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_5, P_INFO, 0, "cmsSockMgrRebuildSocket bind socket success");
        }
    }

    if(!ip_addr_isany_val(*remoteAddr))
    {
        if(cmsSockMgrConnectSocket(rebuildFd, domain, *remotePort, remoteAddr, TRUE) != CME_SUCC)
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_6, P_ERROR, 0, "cmsSockMgrRebuildSocket connect socket fail");
            close(rebuildFd);
            return SOCK_CONN_STATUS_CLOSED;
        }
        else
        {
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_7, P_INFO, 0, "cmsSockMgrRebuildSocket connect socket success");
            ret = SOCK_CONN_STATUS_CONNECTED;
        }
    }

    return ret;

}



/**
  \fn           CmsSockMgrConnStatus cmsSockMgrReCreateSocket(INT32 fd, INT32 domain, INT32 type)
  \brief        recreate socket
*/
CmsSockMgrConnStatus cmsSockMgrReCreateSocket(INT32 fd, INT32 domain, INT32 type)
{
    INT32 rebuildFd = -1;
    INT32 protocol;
    INT32 sockType;
    CmsSockMgrConnStatus ret = SOCK_CONN_STATUS_CLOSED;

    switch(type)
    {
        case SOCK_CONN_TYPE_UDP:
        {
            protocol = IPPROTO_UDP;
            sockType = SOCK_DGRAM;
            break;
        }
        case SOCK_CONN_TYPE_TCP:
        {
            protocol = IPPROTO_TCP;
            sockType = SOCK_STREAM;
            break;
        }
        default:
            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_2, P_ERROR, 0, "cmsSockMgrRebuildSocket invalid type");
            return SOCK_CONN_STATUS_CLOSED;
    }

    rebuildFd = cmsSockMgrCreateSocket(domain, sockType, protocol, fd);

    if(rebuildFd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrRebuildSocket_3, P_ERROR, 0, "cmsSockMgrRebuildSocket create socket fail");
        return SOCK_CONN_STATUS_CLOSED;
    }
    else
    {
        ret = SOCK_CONN_STATUS_CREATED;
    }

    return ret;

}


/**
  \fn           CmsSockMgrContext* cmsSockMgrGetHibContext(CmsSockMgrConnType type)
  \brief        get the sock mgr context by connection type
*/
CmsSockMgrContext* cmsSockMgrGetHibContext(CmsSockMgrConnType type)
{
    CmsSockMgrContext *context = NULL;

    for(context = gSockMgrContext.contextList; context != NULL; context = context->next)
    {
        if(context->hibEnable == TRUE && (context->status == SOCK_CONN_STATUS_CONNECTED || context->status == SOCK_CONN_STATUS_CREATED))
        {
            if(context->type == type)
            {
                return context;
            }
        }
    }

    return context;
}

/**
  \fn           BOOL cmsSockMgrGetUlPendingSequenceState(UINT32 bitmap[8], UINT8 sequence)
  \brief        check wether the sequence exist of the bitmap
*/
BOOL cmsSockMgrGetUlPendingSequenceState(UINT32 bitmap[8], UINT8 sequence)
{
    UINT8 arrayNum = 0;
    UINT8 i = 0;


    //check parameter
    if(bitmap == NULL || sequence == 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrGetUlPendingSequenceState_1, P_ERROR, 0, "cmsSockMgrGetUlPendingSequenceState parameter invalid");
        return FALSE;
    }

    if(sequence%32 == 0)
    {
        i = 31;
    }
    else
    {
        i = ((sequence%32) -1);
    }

    if(sequence > 1)
    {
        arrayNum = ((sequence - 1)/32);
    }
    else
    {
        arrayNum = 0;
    }


    if((bitmap[arrayNum] & (1 << i)) != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
  \fn           void cmsSockMgrUpdateUlPendingSequenceBitMapState(UINT32 bitmap[8], UINT8 sequence, BOOL bActive)
  \brief        update the sequence bitmap status
*/
void cmsSockMgrUpdateUlPendingSequenceBitMapState(UINT32 bitmap[8], UINT8 sequence, BOOL bActive)
{
    UINT8 arrayNum = 0;
    UINT8 i = 0;

    //check parameter
    if(bitmap == NULL || sequence == 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrUpdateUlPendingSequenceBitMapState_1, P_ERROR, 0, "cmsSockMgrUpdateUlPendingSequenceBitMapStates parameter invalid");
        return ;
    }

    if(sequence%32 == 0)
    {
        i = 31;
    }
    else
    {
        i = ((sequence%32) -1);
    }

    if(sequence > 1)
    {
        arrayNum = ((sequence - 1)/32);
    }
    else
    {
        arrayNum = 0;
    }

    if(bActive == TRUE)
    {
        bitmap[arrayNum] |= (1 << i);
    }
    else
    {
        bitmap[arrayNum] &= (~(1 << i));
    }

    ECOMM_TRACE(UNILOG_ATCMD_SOCK, cmsSockMgrUpdateUlPendingSequenceBitMapState_2, P_INFO, 2, "AtecEcSocUpdateUlPendingSequenceState sequence %u, bactive %u", sequence, bActive);
}

/**
  \fn           CmsSockMgrActionResult cmsSockMgrSendAsyncRequest(UINT16 reqId, void *reqBody, CmsSockMgrSource source)
  \brief        send the async sock mgr request
*/
CmsSockMgrActionResult cmsSockMgrSendAsyncRequest(UINT16 reqId, void *reqBody, CmsSockMgrSource source)
{
    INT32 sendFd;
    CmsSockMgrRequest request;
    struct sockaddr_in to;
    struct sockaddr_in src;
    CmsSockMgrActionResult ret = SOCK_MGR_ACTION_OK;

    //check reqBody parameter
    if(reqBody == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_5, P_ERROR, 0, "cmsSockMgrSendAsyncRequest invalid param");
        return SOCK_MGR_ACTION_FAIL;
    }


    //init CmsSockMgr related resource
    if(cmsSockMgrHandleInit(FALSE) == FALSE)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_6, P_ERROR, 0, "init CMS SOCK MGR resource fail");
        ret = SOCK_MGR_ACTION_FAIL;
    }

    //create loopback udp socket with expect socket id first
    sendFd = socket_expect(AF_INET, SOCK_DGRAM, IPPROTO_UDP, CMSSOCKMGR_EXPECT_CLIENT_SOCKET_ID);

    if(sendFd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_expect, P_WARNING, 1, "CmsMgrSendAsyncRequest create loopback udp socket with expect id %u fail", CMSSOCKMGR_EXPECT_CLIENT_SOCKET_ID);

        sendFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if(sendFd < 0)
        {

            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_1, P_ERROR, 0, "CmsMgrSendAsyncRequest create loopback udp socket fail");
            return SOCK_MGR_ACTION_FAIL;
        }
    }

    //bind local port
    memset(&src, 0, sizeof(struct sockaddr_in));
    src.sin_family = AF_INET;
    src.sin_port = htons(CMS_SOCK_MGR_CLIENT_PORT);

    if(bind(sendFd, (struct sockaddr *)&src, sizeof(struct sockaddr_in)) != 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_2, P_ERROR, 0, "CmsMgrSendAsyncRequest bind fail");
        close(sendFd);
        return SOCK_MGR_ACTION_FAIL;
    }

    //init socket reqeust structure
    request.magic = CMS_SOCK_MGR_ASYNC_REQUEST_MAGIC;
    request.source = source;
    request.reqId = reqId;
    request.reqBody = reqBody;
    request.reqTicks = cmsSockMgrGetCurrHibTicks();
    request.timeout = 0;

    //init dst info
    memset(&to, 0, sizeof(struct sockaddr_in));
    to.sin_family = AF_INET;
    to.sin_port = htons(CMS_SOCK_MGR_SERVER_PORT);
    to.sin_addr.s_addr = htonl(IPADDR_LOOPBACK);

    //send request mesage to socket handler socket
    if(sendto(sendFd, &request, sizeof(CmsSockMgrRequest), 0, (struct sockaddr *)&to, sizeof(to)) == sizeof(CmsSockMgrRequest))
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_3, P_INFO, 0, "send socket request to socket handler success");
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendAsyncRequest_4, P_ERROR, 0, "send socket request to socket handler fail");
        ret = SOCK_MGR_ACTION_FAIL;
    }

    close(sendFd);

    return ret;

}

/**
  \fn           CmsSockMgrActionResult cmsSockMgrSendsyncRequest(UINT16 reqId, void *reqBody, CmsSockMgrSource source, CmsSockMgrResponse *response, UINT16 timeout)
  \brief        send the sync sock mgr request
*/
CmsSockMgrActionResult cmsSockMgrSendsyncRequest(UINT16 reqId, void *reqBody, CmsSockMgrSource source, CmsSockMgrResponse *response, UINT16 timeout)
{
    INT32 sendFd;
    CmsSockMgrRequest request;
    struct sockaddr_in to;
    struct sockaddr_in src;
    CmsSockMgrActionResult ret = SOCK_MGR_ACTION_OK;
    INT32 err;

    //init timeout value
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int rcvTimeout = timeout;
#else
    struct timeval rcvTimeout;
    rcvTimeout.tv_sec = timeout;
    rcvTimeout.tv_usec = 0;
#endif

    if(reqBody == NULL || response == NULL)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_1, P_ERROR, 0, "cmsSockMgrSendsyncRequest invalid param");
        return SOCK_MGR_ACTION_FAIL;
    }

    //init CmsSockMgr related resource
    if(cmsSockMgrHandleInit(FALSE) == FALSE)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrSendsyncRequest_9, P_ERROR, 0, "init CMS SOCK MGR resource fail");
        ret = SOCK_MGR_ACTION_FAIL;
    }


    //create loopback udp socket with expect socket id first
    sendFd = socket_expect(AF_INET, SOCK_DGRAM, IPPROTO_UDP, CMSSOCKMGR_EXPECT_CLIENT_SOCKET_ID);

    if(sendFd < 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrSendsyncRequest_expect, P_WARNING, 1, "cmsSockMgrSendsyncRequest create loopback udp socket with expect id %u fail", CMSSOCKMGR_EXPECT_CLIENT_SOCKET_ID);

        sendFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if(sendFd < 0)
        {

            ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, cmsSockMgrSendsyncRequest_2, P_ERROR, 0, "cmsSockMgrSendsyncRequest create loopback udp socket fail");
            return SOCK_MGR_ACTION_FAIL;
        }
    }

    //bind local port
    memset(&src, 0, sizeof(struct sockaddr_in));
    src.sin_family = AF_INET;
    src.sin_port = htons(CMS_SOCK_MGR_CLIENT_PORT);

    if(bind(sendFd, (struct sockaddr *)&src, sizeof(struct sockaddr_in)) != 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_3, P_ERROR, 0, "SdkSendSocketRequestWithResponse bind fail");
        close(sendFd);
        return SOCK_MGR_ACTION_FAIL;
    }

    lwip_setsockopt(sendFd, SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));

    //init socket reqeust structure
    request.magic = CMS_SOCK_MGR_SYNC_REQUEST_MAGIC;
    request.source = source;
    request.reqId = reqId;
    request.reqBody = reqBody;
    request.reqTicks = cmsSockMgrGetCurrHibTicks();
    request.timeout = timeout;


    //init dst info
    memset(&to, 0, sizeof(struct sockaddr_in));
    to.sin_family = AF_INET;
    to.sin_port = htons(CMS_SOCK_MGR_SERVER_PORT);
    to.sin_addr.s_addr = htonl(IPADDR_LOOPBACK);

    //send request mesage to socket handler socket
    if(sendto(sendFd, &request, sizeof(CmsSockMgrRequest), 0, (struct sockaddr *)&to, sizeof(to)) == sizeof(CmsSockMgrRequest))
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_4, P_INFO, 0, "send socket request to socket handler success");
    }
    else
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_5, P_ERROR, 0, "send socket request to socket handler fail");
        close(sendFd);
        return SOCK_MGR_ACTION_FAIL;
    }

    if(recv(sendFd, response, CMS_SOCK_MGR_RESPONSE_LEN_MAX, 0) > 0)
    {
        ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_6, P_INFO, 0, "SdkSendSocketRequestWithResponse recv responses");
    }
    else
    {
       err = sock_get_errno(sendFd);
       if(err == ETIME)
       {
           ret = SOCK_MGR_ACTION_TIMEOUT;
           ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_7, P_ERROR, 0, "SdkSendSocketRequestWithResponse timeout");
       }
       else
       {
           ret = SOCK_MGR_ACTION_FAIL;
           ECOMM_TRACE(UNILOG_CMS_SOCK_MGR, CmsMgrSendsyncRequest_8, P_ERROR, 0, "SdkSendSocketRequestWithResponse recv response fail");
       }
    }


    close(sendFd);

    return ret;

}


