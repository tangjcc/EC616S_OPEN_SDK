/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.h
 * Description:  EC616S onenet demo entry header file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

#define QMSG_ID_BASE               (0x160) 
#define QMSG_ID_NW_IPV4_READY      (QMSG_ID_BASE)
#define QMSG_ID_NW_IPV6_READY      (QMSG_ID_BASE + 1)
#define QMSG_ID_NW_IPV4_6_READY    (QMSG_ID_BASE + 2)
#define QMSG_ID_NW_DISCONNECT      (QMSG_ID_BASE + 3)
#define QMSG_ID_SOCK_SENDPKG       (QMSG_ID_BASE + 4)
#define QMSG_ID_SOCK_RECVPKG       (QMSG_ID_BASE + 5)

typedef enum
{
    APP_INIT_STATE,
    APP_DEACTIVE_STATE,
    APP_IPREADY_STATE,
    APP_REPORT_STATE,
    APP_IDLE_STATE
} appRunningState_t;




