/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.h
 * Description:  EC616 socket demo entry header file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/
#ifndef APP_EC_SOCKET_DAEMON_H
#define APP_EC_SOCKET_DAEMON_H


#include "commontypedef.h"

#define QMSG_ID_BASE               (0x160) 
#define QMSG_ID_NW_IP_READY        (QMSG_ID_BASE)
#define QMSG_ID_NW_IP_SUSPEND      (QMSG_ID_BASE + 1)
#define QMSG_ID_NW_IP_NOREACHABLE  (QMSG_ID_BASE + 2)
#define QMSG_ID_SOCK_SENDPKG       (QMSG_ID_BASE + 4)
#define QMSG_ID_SOCK_RECVPKG       (QMSG_ID_BASE + 5)

#define APP_SOCKET_DAEMON_SERVER_IP "47.105.44.99"
#define APP_SOCKET_DAEMON_SERVER_PORT 5684


typedef enum 
{
    UDP_CLIENT,
    TCP_CLIENT,
} socketCaseNum;

typedef enum
{
    APP_SOCKET_CONNECTION_CLOSED,
    APP_SOCKET_CONNECTION_CONNECTING,
    APP_SOCKET_CONNECTION_CONNECTED,    
}AppSocketConnectionStatus;

typedef struct AppSocketConnectionContext_Tag
{
    INT32 id;
    INT32 status;
}AppSocketConnectionContext;

#endif