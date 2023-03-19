/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: at_sock_task.h
 *
 *  Description: AT SOCKET TASK, task created by SOCKET AT COMMAND
 *
 *  History:create by xwang
 *
 *  Notes:
 *
******************************************************************************/
#ifndef __AT_SOCK_TASK_H__
#define __AT_SOCK_TASK_H__

#include "at_util.h"
#include "lwip/api.h"
#include "cms_sock_mgr.h"

/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/


#define ATECSKTREQ_MAGIC 0x1234
#define ATECSKTHANDLE_PORT 60000
#define ATECSKTREQUEST_PORT 60001
#define ATECSKTCONNECTTIMEOUT 24            //seconds
#define ATSOCHIBTCPCONTEXTMAGIC 0xABCD
#define ATSOCHIBUDPCONTEXTMAGIC 0xDCBA


/*AT SOCKET related  define*/
#define AT_SOC_FD_MAX 9
#define AT_SOC_FD_MIN 0
#define AT_SOC_FD_DEF -2
#define AT_SOC_FD_ALL 255
#define AT_SOC_UL_LENGTH_MAX 1400
#define AT_SOC_UL_LENGTH_MIN 1
#define AT_SOC_DL_LENGTH_MAX 1358
#define AT_SOC_DL_LENGTH_MIN 1
#define AT_SOC_PORT_MAX 65535
#define AT_SOC_PORT_MIN 1
#define AT_SOC_UL_DATA_SEQUENCE_MAX 255
#define AT_SOC_UL_DATA_SEQUENCE_MIN 1
#define AT_SOC_UL_SEGMENT_ID_MAX  4
#define AT_SOC_UL_SEGMENT_ID_MIN  1
#define AT_SOC_UL_SEGMENT_NUM_MAX  4
#define AT_SOC_UL_SEGMENT_NUM_MIN  2
#define AT_SOC_NOTIFY_MODE_DEF 0
#define AT_SOC_NOTIFY_MODE_MIN 0
#define AT_SOC_NOTIFY_MODE_MAX 3
#define AT_SOC_NOTIFY_MODE_IGNORE 254
#define AT_SOC_NOTIFY_MODE_PRIVATE_DISABLE 255
#define AT_SOC_PUBLIC_DL_BUFFER_DEF 2048
#define AT_SOC_PUBLIC_DL_BUFFER_MAX 3072
#define AT_SOC_PUBLIC_DL_BUFFER_MIN 1358
#define AT_SOC_PUBLIC_DL_BUFFER_IGNORE 0XFFFF
#define AT_SOC_PRIVATE_DL_BUFFER_DEF 1358
#define AT_SOC_PRIVATE_DL_BUFFER_MAX 2048
#define AT_SOC_PRIVATE_DL_BUFFER_MIN 1358
#define AT_SOC_PRIVATE_DL_BUFFER_MIN 1358
#define AT_SOC_PRIVATE_DL_BUFFER_IGNORE 0XFFFF
#define AT_SOC_PUBLIC_DL_PKG_NUM_DEF 8
#define AT_SOC_PUBLIC_DL_PKG_NUM_MAX 16
#define AT_SOC_PUBLIC_DL_PKG_NUM_MIN 8
#define AT_SOC_PUBLIC_DL_PKG_NUM_IGNORE 0XFF
#define AT_SOC_PRIVATE_DL_PKG_NUM_DEF 4
#define AT_SOC_PRIVATE_DL_PKG_NUM_MAX 8
#define AT_SOC_PRIVATE_DL_PKG_NUM_MIN 1
#define AT_SOC_PRIVATE_DL_PKG_NUM_IGNORE 0XFF
#define AT_SOC_IP_ADDR_STRING_LENGTH_MAX 63
#define AT_SOC_QUERY_UL_PENDING_LIST_RESPONSE_LENGTH_MAX 64



#define SUPPORT_MAX_SOCKET_NUM                5
#define SOCKET_TIMEOUT                        10
#define SUPPORT_MAX_SOCKET_RAW_DATA_LENGTH    1400
#define SUPPORT_MAX_SOCKET_HEX_STRING_DATA_LENGTH (SUPPORT_MAX_SOCKET_RAW_DATA_LENGTH * 2 + 1)
#define SUPPORT_MAX_TCP_SERVER_LISTEN_NUM     3



/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/

typedef enum {
    ATECSKTREQ_CREATE   = 1,
    ATECSKTREQ_BIND     = 2,
    ATECSKTREQ_CONNECT  = 3,
    ATECSKTREQ_SEND     = 4,
    ATECSKTREQ_DELETE   = 5,
    ATECSKTREQ_STATUS   = 6,
/***************************************************/
    ECSOCREQ_CREATE     = 7,
    ECSOCREQ_UDPSEND    = 8,
    ECSOCREQ_UDPSENDF    = 9,
    ECSOCREQ_QUERY      = 10,
    ECSOCREQ_READ       = 11,
    ECSOCREQ_TCPCONNECT = 12,
    ECSOCREQ_TCPSEND    = 13,
    ECSOCREQ_CLOSE      = 14,
    ECSOCREQ_NMI        = 15,
    ECSOCREQ_NMIE       = 16,
    ECSOCREQ_GNMI       = 17,
    ECSOCREQ_GNMIE      = 18,
    ECSOCREQ_STATUS     = 19,
/***************************************************/
    ECSRVSOCREQ_CREATETCP = 20,
    ECSRVSOCREQ_CLOSELISTEN = 21,
    ECSRVSOCREQ_CLOSECLIENT = 22,
    ECSRVSOCREQ_SENDCLIENT  = 23,
    ECSRVSOCREQ_STATUSLISTEN  = 24,
}AtecSktReqId;



/*
 * APPL SGID: APPL_SOCKET/APPL_ECSOC, related PRIM ID
*/
typedef enum applSockPrimId_Enum
{
    APPL_SOCKET_PRIM_BASE = 0,

    APPL_SOCKET_CREATE_CNF,   //
    APPL_SOCKET_BIND_CNF,
    APPL_SOCKET_CONNECT_CNF,
    APPL_SOCKET_SEND_CNF,
    APPL_SOCKET_DELETE_CNF,
    APPL_SOCKET_STATUS_CNF,
    APPL_SOCKET_ERROR_IND,
    APPL_SOCKET_RCV_IND,

    APPL_ECSOC_CREATE_CNF,
    APPL_ECSOC_UDPSEND_CNF,
    APPL_ECSOC_UDPSENDF_CNF,
    APPL_ECSOC_QUERY_CNF,
    APPL_ECSOC_READ_CNF,
    APPL_ECSOC_TCPCONNECT_CNF,
    APPL_ECSOC_TCPSEND_CNF,
    APPL_ECSOC_CLOSE_CNF,
    APPL_ECSOC_NMI_CNF,
    APPL_ECSOC_NMIE_CNF,
    APPL_ECSOC_GNMI_CNF,
    APPL_ECSOC_GNMIE_CNF,
    APPL_ECSOC_STATUS_CNF,
    APPL_ECSOC_NMI_IND,
    APPL_ECSOC_CLOSE_IND,
    APPL_ECSOC_QUERY_RESULT_IND,
    APPL_ECSOC_GNMIE_IND,
    APPL_ECSOC_ULSTATUS_IND,
    APPL_ECSOC_STATUS_IND,
    APPL_ECSOC_CONNECTED_IND,

    APPL_ECSRVSOC_CREATE_TCP_LISTEN_CNF,
    APPL_ECSRVSOC_CLOSE_TCP_LISTEN_CNF,
    APPL_ECSRVSOC_CLOSE_TCP_CLIENT_CNF,
    APPL_ECSRVSOC_SEND_TCP_CLIENT_CNF,
    APPL_ECSRVSOC_STATUS_TCP_LISTEN_CNF,
    APPL_ECSRVSOC_CREATE_TCP_LISTEN_IND,
    APPL_ECSRVSOC_SERVER_ACCEPT_CLIENT_IND,
    APPL_ECSRVSOC_STATUS_TCP_LISTEN_IND,
    APPL_ECSRVSOC_RECEIVE_TCP_CLIENT_IND,
    APPL_ECSRVSOC_CLOSE_TCP_CONNECTION_IND,

    APPL_SOCKET_PRIM_MAX = 255
}ApplSockPrimId;


/*
 * value of "ApplRetCnf->header.rc"
*/
typedef enum ApplSockRetCode_enum
{
    SOCKET_ACTION_OK   = 0,
    SOCKET_PARAM_ERROR = 1,                             //parameter error
    SOCKET_TOO_MUCH_INST = 2,                           //too much socket instance
    SOCKET_CREATE_SOCK_ERROR = 3,                       //create socket error
    SOCKET_OPERATION_NOT_SUPPORT = 4,                   //operation not support
    SOCKET_NO_FIND_CLIENT = 5,                          //operation not support
    SOCKET_CONNECT_FAIL = 6,                            //connect failed
    SOCKET_BIND_FAIL = 7,                               //bind failed
    SOCKET_SEND_FAIL = 8,                               //send failed
    SOCKET_NO_CONNECTED = 9,                            //connect failed
    SOCKET_IS_CONNECTED = 10,                           //already connected
    SOCKET_INVALID_STATUS,
    SOCKET_CONNECT_TIMEOUT,
    SOCKET_DELETE_FAIL,
    SOCKET_FATAL_ERROR,                                //fatal error
    SOCKET_NO_MEMORY,
    SOCKET_NO_MORE_DL_BUFFER_RESOURCE,
    SOCKET_CONNECT_IS_ONGOING,
    SOCKET_UL_SEQUENCE_INVALID,
    SOCKET_UNKNOWN,
}ApplSockRetCode;



typedef enum AtecSktDataRelAssistInd_Tag
{
    ATEC_SKT_DATA_RAI_NO_INFO = 0,
    ATEC_SKT_DATA_RAI_NO_UL_DL_FOLLOWED = 1,
    ATEC_SKT_DATA_RAI_ONLY_DL_FOLLOWED = 2,
    ATEC_SKT_DATA_RAI_RESERVED = 3
}AtecSktDataRelAssistIndEnum;

typedef enum {
    ATECECSOC_FAIL =    0,
    ATECECSOC_SUCCESS = 1,
}AtecEcSocUlStatus;

typedef enum AtSocketType_Tag
{
    AT_SOCKET_ATSKT = 0,
    AT_SOCKET_ECSOC = 1,
    AT_SOCKET_SDKSOC = 2,
}AtSocketTypeEnum;

typedef enum {
    SOCK_CLOSED = 0, //socket delete
    SOCK_INIT = 1, //socket create
    SOCK_CONNECTING = 2, //socket is connecting
    SOCK_CONNECTED = 3, //socket connected
}sockStatus;

typedef enum {
    MODE_PUBLIC = 0,
    MODE_PRIVATE = 1,
}EcSocModeFlag;

typedef enum {
    RCV_DISABLE = 0, //diabel receive DL packet
    RCV_ENABLE  = 1, //enable receive DL packet
}EcSocRcvControlFlag;

typedef enum {
    NMI_MODE_0 = 0, //disable notify
    NMI_MODE_1  = 1, //notify with socketid and length
    NMI_MODE_2  = 2, //notify with socketid, server address&port, length and data
    NMI_MODE_3  = 3, //notify with socketid, length and data
}EcSocNotifyIndMode;

typedef enum {
    EC_SOCK_VALID = 0, //EC socket available
    EC_SOCK_INVALID = 1, //EC socket unavailable
    EC_SOCK_FLOW_CONTROL = 2, //flow control
    EC_SOCK_BACK_OFF = 3, //back off
}EcSockStatus;



/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/

/******************************ATSKT related**********************************/

typedef struct AtecSktCreateReq_Tag{
    INT32 domain;
    INT32 type;
    INT32 protocol;
    UINT32 reqSource;
    void *eventCallback;
}AtecSktCreateReq;

typedef struct AtecSktSendReq_Tag{
    INT32 fd;
    UINT32 sendLen;
    UINT8 dataRai;
    BOOL dataExpect;
    UINT16 rsvd;
    UINT32 reqSource;
    UINT8 data[];
}AtecSktSendReq;

typedef struct AtecSktBindReq_Tag{
    INT32 fd;
    UINT16 localPort;
    UINT16 reserved;
    UINT32 reqSource;
    ip_addr_t localAddr;
}AtecSktBindReq;

typedef struct AtecSktConnectReq_Tag{
    INT32 fd;
    UINT16 remotePort;
    UINT16 reserved;
    UINT32 reqSource;
    ip_addr_t remoteAddr;
}AtecSktConnectReq;

typedef struct AtecSktDeleteReq_Tag{
    INT32 fd;
    UINT32 reqSource;
}AtecSktDeleteReq;

typedef struct AtecSktStatusReq_Tag{
    INT32 fd;
    UINT32 reqSource;
}AtecSktStatusReq;

typedef struct AtecSktDlInd_Tag{
    INT32 fd;
    UINT16 len;
    UINT16 rsvd;
    UINT8 data[];
}AtecSktDlInd;

typedef struct AtecSktCnfTag
{
    union {
        UINT32 errCode;
        INT32 fd;
        INT32 status;
    }cnfBody;
}AtecSktCnf;

typedef struct AtecSktPriMgrContext_Tag{
    UINT16 createReqHandle;
    UINT16 connectReqHandle;
}AtecSktPriMgrContext;

/******************************ECSOC related**********************************/

typedef struct EcSocCreateReq_Tag{
    INT32 type;
    INT32 protocol;
    INT32 domain;
    UINT8 receiveControl;
    UINT8 reserved;
    UINT16 listenPort;
    UINT32 reqSource;
    void *eventCallback;
    ip_addr_t localAddr;
}EcSocCreateReq;


typedef struct EcSocUdpSendReq_Tag{
    INT32 socketId;
    UINT16 remotePort;
    UINT16 length;
    UINT8 sequence;
    UINT8 segmentId;
    UINT8 segmentNum;
    UINT8 exceptionFlag;
    UINT8 raiInfo;
    UINT8 reserved1;
    UINT16 reserved2;
    UINT32 reqSource;
    ip_addr_t remoteAddr;
    UINT8 data[];
}EcSocUdpSendReq;

typedef struct EcSocQueryReq_Tag{
    UINT32 reqSource;
    BOOL bQueryAll;
    UINT8 rsvd1;
    UINT16 rsvd2;
    INT32 socketId[SUPPORT_MAX_SOCKET_NUM];
}EcSocQueryReq;

typedef struct EcSocReadReq_Tag{
    INT32 socketId;
    UINT16 length;
    UINT16 reserved;
    UINT32 reqSource;
}EcSocReadReq;

typedef struct EcSocTcpConnectReq_Tag{
    INT32 socketId;
    UINT16 remotePort;
    UINT16 reserved;
    UINT32 reqSource;
    ip_addr_t remoteAddr;
}EcSocTcpConnectReq;

typedef struct EcSocTcpSendReq_Tag{
    INT32 socketId;
    UINT16 length;
    UINT8 sequence;
    UINT8 expectionFlag;
    UINT8 raiInfo;
    UINT8 reserved1;
    UINT16 reserved2;
    UINT32 reqSource;
    UINT8 data[];
}EcSocTcpSendReq;

typedef struct EcSocCloseReq_Tag{
    INT32 socketId;
    UINT32 reqSource;
}EcSocCloseReq;

typedef struct EcSocNMIReq_Tag{
    UINT8 mode;
    UINT8 maxPublicDlPkgNum;
    UINT16 maxPublicDlBuffer;
    UINT32 reqSource;
}EcSocNMIReq;

typedef struct EcSocNMIEReq_Tag{
    INT32 socketId;
    UINT8 mode;
    UINT8 maxPublicDlPkgNum;
    UINT16 maxPublicDlBuffer;
    UINT32 reqSource;
}EcSocNMIEReq;

typedef struct EcSocNMIGetReq_Tag{
    UINT32 reqSource;
}EcSocNMIGetReq;

typedef struct EcSocNMIEGetReq_Tag{
    UINT32 reqSource;
}EcSocNMIEGetReq;

typedef struct EcSocStatusReq_Tag{
    BOOL bQuryAll;
    UINT8 rsvd1;
    UINT16 rsvd2;
    INT32 socketId;
    UINT32 reqSource;
}EcSocStatusReq;

typedef struct EcSocUlStatusReq_Tag{
    INT32 socketId;
    INT32 ulStatus;
    UINT32 sequenceBitMap[8];
}EcSocUlStatusReq;

typedef struct EcSocCreateResponse_Tag{
    INT32 socketId;
}EcSocCreateResponse;

typedef struct EcSocUdpSendResponse_Tag{
    INT32 socketId;
    UINT16 length;
    UINT16 reserved;
}EcSocUdpSendResponse;

typedef struct EcSocQueryInd_Tag{
    INT32 socketId;
    UINT8 sequence;
    UINT8 reserved1;
    UINT16 reserved2;
}EcSocQueryInd;

typedef struct EcSocReadResponse_Tag{
    INT32 socketId;
    ip_addr_t remoteAddr;
    UINT16 remotePort;
    UINT16 length;
    UINT16 remainingLen;
    UINT8 reserved;
    UINT8 data[];
}EcSocReadResponse;

typedef struct EcSocNMInd_Tag{
    INT32 socketId;
    ip_addr_t remoteAddr;
    UINT16 remotePort;
    UINT16 length;
    UINT8 modeNMI;
    UINT8 reserved1;
    UINT16 reserved2;
    UINT8 data[];
}EcSocNMInd;

typedef struct EcSocTcpSendResponse_Tag{
    INT32 socketId;
    UINT16 length;
    UINT16 reserved;
}EcSocTcpSendResponse;

typedef struct EcSocGNMIResponse_Tag{
    UINT8 mode;
    UINT8 maxDlPkgNum;
    UINT16 maxDlBufferSize;
}EcSocGNMIResponse;

typedef struct EcSocGNMIEInd_Tag{
    INT32 socketId;
    UINT8 mode;
    UINT8 maxDlPkgNum;
    UINT16 maxDlBufferSize;
}EcSocGNMIEInd;

typedef struct EcSocCloseInd_Tag{
    INT32 socketId;
    INT32 errCode;
}EcSocCloseInd;

typedef struct EcSocUlStatusInd_Tag{
    INT32 socketId;
    UINT8 sequence;
    UINT8 status;  //AtecEcSocUlStatus
    UINT16 reserved;
}EcSocUlStatusInd;

typedef struct EcSocStatusInd_Tag{
    INT32 socketId;
    UINT8 status;//EcSockStatus
    UINT8 rsvd;
    UINT16 backOffTimer;
}EcSocStatusInd;

typedef struct EcSocConnectedInd_Tag{
    INT32 socketId;
}EcSocConnectedInd;

typedef struct AtecSocCnfTag
{
    union {
        UINT32 errCode;
        INT32 fd;
        INT32 status;
    }cnfBody;
}AtecSocCnf;

typedef struct AtecEcSocErrCnfTag
{
    UINT32 errCode;
}AtecEcSocErrCnf;

typedef struct EcSocUlBuffer_Tag{
    UINT8 segmentId;
    UINT8 reserved;
    UINT16 length;
    struct EcSocUlBuffer_Tag *next;
    UINT8 data[];
}EcSocUlBuffer;

typedef struct EcSocUlList_Tag{
    UINT8 sequence;
    UINT8 segmentNum;
    UINT16 remotePort;
    UINT8 segmentAlready;
    UINT8 reserved1;
    UINT16 reserved2;
    ip_addr_t remoteAddr;
    struct EcSocUlList_Tag *next;
    EcSocUlBuffer *ulData;
}EcSocUlList;

typedef struct EcSocDlBufferList_Tag{
    BOOL isPrivate; //default FALSE
    UINT8 reserved1;
    UINT16 totalLen;
    UINT16 reserved;
    UINT16 length;
    UINT16 offSet;
    UINT16 remotePort;
    ip_addr_t remoteAddr;
    struct EcSocDlBufferList_Tag *next;
    UINT8 data[];
}EcSocDlBufferList;

typedef struct EcSocModeSet_Tag{
    UINT8 flag:1; //EcSocModeFlag
    UINT8 mode:2; //EcSocNotifyIndMode
    UINT8 receiveControl:1; //EcSocRcvControlFlag
    UINT8 reserved:4;
}EcSocModeSet;

typedef struct EcSocUlSequenceStatus_Tag{
    UINT32 bitmap[8];
}EcSocUlSequenceStatus;

typedef struct EcSocDlPrivateSet_Tag{
    UINT16 privateDlBufferToalSize;
    UINT16 privateDlBufferTotalUsage;
    UINT8 privateDlPkgNumMax;
    UINT8 privateDlPkgNumTotalUsage;
    UINT16 reserved;
}EcSocDlPrivateSet;

typedef struct EcSocPublicDlConfig_Tag{
    UINT16 publicDlBufferToalSize;
    UINT16 publicDlBufferTotalUsage;
    UINT8 publicDlPkgNumMax;
    UINT8 publicDlPkgNumTotalUsage;
    UINT8 mode;
    UINT8 receiveControl;
}EcSocPublicDlConfig;

typedef struct EcsocPriMgrContext_Tag{
    UINT16 rsvd1;
    UINT16 dlTotalLen;
    UINT16 createReqHandle;
    UINT16 connectReqHandle;
    EcSocModeSet modeSet;
    EcSocDlPrivateSet dlPriSet;
    EcSocUlSequenceStatus ulSeqStatus;
    EcSocUlList *ulList; //only for udp socket
    EcSocDlBufferList *dlList;
}EcsocPriMgrContext;

/******************************SDKAPI related**********************************/

typedef struct SdkCreateConnectionReq_Tag{
    UINT32 protocol;
    UINT32 reqTicks;
    UINT16 localPort;
    UINT16 destPort;
    ip_addr_t localAddr;
    ip_addr_t destAddr;
    void *callback;
}SdkCreateConnectionReq;

typedef struct SdkSendDataReq_Tag{
    INT32 connectionId;
    UINT32 reqTicks;
    UINT16 length;
    UINT8 sequence;
    UINT8 raiInfo;
    UINT8 data[];
}SdkSendDataReq;

typedef struct SdkCloseReq_Tag{
    INT32 connectionId;
}SdkCloseReq;

typedef struct SdkSktResult_Tag{
    INT32 result;
    union {
        INT32 connectionId;
        UINT16 sendLen;
    }body;
}SdkSktResult;
/*****************************ec server soc related****************************/
typedef struct EcSrvSocCreateTcpReq_Tag{
    INT32 domain;
    UINT8 rsvd1;
    UINT8 rsvd2;
    UINT16 listenPort;
    UINT32 reqSource;
    void *eventCallback;
    ip_addr_t bindAddr;
}EcSrvSocCreateTcpReq;

typedef struct EcSrvSocCloseTcpServerReq_Tag{
    INT32 socketId;
    BOOL bCloseClient;
    UINT8 rsvd1;
    UINT8 rsvd2;
    UINT32 reqSource;
}EcSrvSocCloseTcpServerReq;

typedef struct EcSrvSocCloseTcpClientReq_Tag{
    INT32 socketServerId;
    INT32 socketClientId;
    UINT32 reqSource;
}EcSrvSocCloseTcpClientReq;

typedef struct EcSrvSocSendTcpClientReq_Tag{
    INT32 socketClientId;
    UINT32 sendLen;
    UINT8 dataRai;
    BOOL dataExpect;
    UINT8 rsvd;
    UINT8 sequence;
    UINT32 reqSource;
    UINT8 data[];
}EcSrvSocSendTcpClientReq;

typedef struct EcSrvSocStatusTcpServerReq_Tag{
    INT32 socketId;
    UINT32 reqSource;
}EcSrvSocStatusTcpServerReq;

typedef struct EcSrcSocCreateTcpListenInd_Tag{
    INT32 result;
    INT32 socketId;
}EcSrcSocCreateTcpListenInd;

typedef struct EcSrvSocTcpAcceptClientReaultInd_Tag{
    INT32 serverSocketId;
    INT32 clientSocketId;
    UINT16 clientPort;
    UINT16 rsvd;
    ip_addr_t clientAddr;
}EcSrvSocTcpAcceptClientReaultInd;

typedef struct EcSrvSocTcpListenStatusInd_Tag{
    INT32 serverSocketId;
    INT32 status;
}EcSrvSocTcpListenStatusInd;

typedef struct EcSrvSocTcpClientReceiveInd_Tag{
    INT32 socketId;
    UINT16 length;
    UINT16 reserved;
    UINT8 data[];
}EcSrvSocTcpClientReceiveInd;

typedef struct EcSrvSocCloseInd_Tag{
    INT32 socketId;
    INT32 errCode;
    BOOL bServer;
    UINT8 rsvd1;
    UINT16 rsvd2;
}EcSrvSocCloseInd;

typedef struct AtecEcSrvSocErrCnf_Tag
{
    UINT32 errCode;
}AtecEcSrvSocErrCnf;


typedef struct EcSrvSocPriMgrContext_Tag{
    UINT16 createReqHandle;
    UINT16 listenPort;
    INT32 fSocketId; //father socket
    ip_addr_t bindAddr;
}EcSrvSocPriMgrContext;

typedef struct AtSocketSendInfo_Tag
{
    INT32   socketId;
    UINT16  reqHander;  /*if set to zero, just means no sending request */
    UINT16  dataOffset;
    INT32  dataLen;    
    BOOL    expectData;
    UINT8   raiInfo;
    UINT8   sequence;
    UINT8   source;
    UINT16  requestId;
    UINT16  remotePort;
    ip_addr_t remoteAddr;
    UINT8   data[SUPPORT_MAX_SOCKET_HEX_STRING_DATA_LENGTH];  
}AtSocketSendInfo;



/******************************common related**********************************/
typedef struct AtecSktReq_Tag{
    UINT16 magic; //ATECSKTREQ_MAGIC
    UINT16 reqId;
    UINT32 reqSource;
    void* reqBody;
}AtecSktReq;

typedef struct SdkSktResponse_Tag{
    UINT16 magic; //ATECSKTREQ_MAGIC
    UINT16 reqId;
    SdkSktResult reqResult;
}SdkSktResponse;


typedef struct socketAtcmd_Tag
{
    UINT8 status;
    UINT8 atSockType; //AtSocketTypeEnum
    UINT16 createReqHandle;
    UINT16 connectReqHandle;
    UINT16 reserved;
    INT32 socketId;
    UINT8 domain;
    UINT8 type;
    UINT8 protocol;
    BOOL hibEnable;
    UINT32 connectTime;
    EcSocModeSet modeSet;
    EcSocDlPrivateSet dlPriSet;
    EcSocUlSequenceStatus ulSeqStatus;
    EcSocUlList *ulList; //only for udp socket
    EcSocDlBufferList *dlList;
    struct socketAtcmd_Tag *next;
}socketAtcmd;

typedef struct GsocketAtcmd_Tag
{
    UINT8 socketNum;
    socketAtcmd *socketList;
}GsocketAtcmd;


typedef struct EcsocConnHibPriContext_Tag{
    UINT8 domain;
    UINT8 rsvd1;
    UINT16 rsvd2;
    EcSocModeSet modeSet;
    UINT16 createReqHandle;
    EcSocDlPrivateSet dlPriSet;
}EcsocConnHibPriContext;

typedef struct AtsktConnHibPriContext_Tag{
    UINT8 domain;
    UINT8 rsvd;
    UINT16 createReqHandle;
}AtsktConnHibPriContext;


/******************************************************************************
 *****************************************************************************
 * FUNCTION/API
 *****************************************************************************
******************************************************************************/

void atecSktProessReq(CmsSockMgrRequest *atecSktReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd);
void atecEcsocProessReq(CmsSockMgrRequest *atecSktReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd);
void atecEcSrvSocProessReq(CmsSockMgrRequest *atecEcSrvSocReq, ip_addr_t *sourceAddr, UINT16 sourcePort, INT32 rcvRequestFd);
void atSktEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg);
void atEcsocEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg);
void atEcSrvSocEventCallback(CmsSockMgrContext *mgrContext, CmsSockMgrEventType eventType, void *eventArg);
void atSktStoreConnHibContext(CmsSockMgrContext *sockMgrContext, CmsSockMgrConnHibContext *hibContext);
void atEcsocStoreConnHibContext(CmsSockMgrContext *sockMgrContext, CmsSockMgrConnHibContext *hibContext);
void atSktRecoverConnContext(CmsSockMgrConnHibContext *hibContext);
void atEcsocRecoverConnContext(CmsSockMgrConnHibContext *hibContext);
BOOL atEcSrcSocTcpServerProcessAcceptClient(CmsSockMgrContext* gMgrContext, INT32 clientSocketId, ip_addr_t *clientAddr, UINT16 clientPort);


void atSocketInit(void);





#endif

