#ifndef NM_HDR_H
#define NM_HDR_H
/******************************************************************************
 ******************************************************************************
 Copyright:      - 2018- Copyrights of EigenComm Ltd.
 File name:      - netmgr.h
 Description:    - ps network data path linkup/linkdown function header
 History:        - 01/02/2018, Originated by xwang
 ******************************************************************************
******************************************************************************/

#include "commontypedef.h"
#include "lwip/ip_addr.h"
#include "cmips.h"

/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/
#define NM_PDN_TYPE_MAX_DNS_NUM     2
#define NM_MAX_DNS_NUM              4
#define NM_CID_BIND_TO_NONE         255
#define NM_ADDR_MAX_LENGTH          16
#define NM_PS_DEFAULT_CID           0
#define NM_PS_CID_MIN               0       //1, attached EPS bearer CID is changed to 0.
#define NM_PS_CID_MAX               10
#define NM_PS_INVALID_CID           255

/*
 * set bit B to 1 in D
 * example:
 *  NmBit1Set(0x00, 0) = 0x01
 *  NmBit1Set(0x01, 4) = 0x11
*/
#define NmBit1Set(D, B)   ((D) |= (1<<(B)))

/*
 * set bit B to 0 in D
 * example:
 *  NmBit0Set(0xFF, 0) = 0xFE
 *  NmBit0Set(0xFE, 4) = 0xEE
*/
#define NmBit0Set(D, B)   ((D) &= (~(1<<(B))))


/*
 * default setting for ping
*/
#define NM_PING_DEFAULT_COUNT       15
#define NM_PING_KEEP_COUNT          255
#define NM_PING_DEFAULT_PAYLOAD_SIZE    32
#define NM_PING_DEFAULT_TIMEOUT     20  //20s



#define NM_MAGIC_WORD               0xA5B7

/******************************************************************************
 *****************************************************************************
 * STRUCT/ENUM
 *****************************************************************************
******************************************************************************/

#if 0
/*
 * 0x0920 - 0x0930, SIGID used for netmanager
*/
typedef enum
{
    SIG_NM_BASE = 0x0920,

    SIG_NM_ATI_CNF, //NmAtiCnf
    SIG_NM_ATI_IND, //NmAtiInd

    SIG_NM_END  = 0x0927
}NetMgrSigId;
#endif

/*
 * the action result
*/
typedef enum
{
    NM_SUCCESS = 0,
    NM_FAIL = 1,
}NmResult;

#if 0
/*
 * ReqId & CnfId in "SIG_NM_ATI_CNF"
*/
typedef enum NM_ATI_ACTION_ID_TAG
{
    NM_ATI_ACTION_BASE = 0x1000,

/****************************************************/
    NM_ATI_ASYNC_BASE = 0X1001,

    NM_ATI_ASYNC_GET_DNS_REQ, //ASYNC reqeust
    NM_ATI_ASYNC_GET_DNS_CNF, //ASYNC cnf

    NM_ATI_ASYNC_END = 0X10FF,

/*****************************************************/

    NM_ATI_SYNC_BASE = 0X1100,

    NM_ATI_SYNC_GET_NET_INFO_REQ,

    NM_ATI_SYNC_IS_ANY_ACTIVE_SOCKET_REQ,

    NM_ATI_SYNC_END = 0X11FF,

/*****************************************************/

    NM_ATI_ACTION_END = 0xffff,
}NM_ATI_ACTION_ID;

/*
 * IndId in "SIG_NM_ATI_IND"
*/
typedef enum NM_ATI_INDICATION_ID_TAG
{
    NM_ATI_IND_BASE = 0x2000,

    NM_ATI_PING_RET_IND,    //NmAtiPingResultInd
    NM_ATI_IPERF_RET_IND,   //NmAtiIperfResultInd

    NM_ATI_NET_INFO_IND,    //NmAtiNetInfoInd


    NM_ATI_IND_END = 0xffff,
}NM_ATI_INDICATION_ID;
#endif

typedef enum NmCmsPrimId_enum
{
    NM_ATI_PRIM_BASE    = 0x00,

    /*
     * ASYN PRIM ID, REQ/CNF
    */
    NM_ATI_ASYNC_BASE   = 0x00,

    NM_ATI_ASYNC_GET_DNS_REQ, //ASYNC reqeust
    NM_ATI_ASYNC_GET_DNS_CNF, //NmAtiGetDnsCnf

    NM_ATI_ASYNC_END    = 0X1F,

    /*
     * SYN PRIM ID
    */
    NM_ATI_SYNC_BASE    = 0x20,
    NM_ATI_SYNC_GET_NET_INFO_REQ,
    NM_ATI_SYNC_CLEAR_DNS_CAHCE,
    NM_ATI_SYNC_GET_DNS_SERVER,
    NM_ATI_SYNC_SET_DNS_SERVER,
    NM_ATI_SYNC_IS_ANY_ACTIVE_SOCKET_REQ,
    NM_ATI_SYNC_END     = 0x3F,

    /*
     * NM IND PRIM ID
    */
    NM_ATI_PING_RET_IND = 0x40,    //NmAtiPingResultInd
    NM_ATI_IPERF_RET_IND,   //NmAtiIperfResultInd
    NM_ATI_NET_INFO_IND,    //NmAtiNetInfoInd
    NM_ATI_SNTP_RET_IND,

    NM_ATI_PRIM_END = 0XFF,
}NmCmsPrimId;


/*
 * NM NET type
*/
typedef enum
{
    NM_NET_TYPE_INVALID = 0,    //net not available
    NM_NET_TYPE_IPV4 = 1,       //ipv4 available
    NM_NET_TYPE_IPV6 = 2,       //ipv6 available
    NM_NET_TYPE_IPV4V6 = 3,     //ipv4&ipv6 available
    NM_NET_TYPE_IPV6preparing = 4, //ipv6 preparing
    NM_NET_TYPE_IPV4_IPV6preparing = 5, //ipv4 available &ipv6 preparing
}NmNetIpType;

/*
 * the ip address type
*/
typedef enum
{
    NM_ADDR_INVALID_ADDR = 0,
    NM_ADDR_IPV4_ADDR,          // 4 bytes length
    NM_ADDR_FULL_IPV6_ADDR,     //16 bytes length
    NM_ADDR_IPV6_ID             //8 bytes length
}NmAddrType;

#if 0
/*
 * Signal: NM_ATI_SIGNAL_CNF
 * signal to AT CMD SRV task,
 *  just the confirmation for "NM_ATI_ACTION_ID" request
*/
typedef struct NmAtiCnf_Tag
{
    struct {
        UINT16  cnfId;        //NM_ATI_ACTION_ID
        UINT16  srcHandler;   //Handler
        UINT16  rc;           //NmResult, return code
        UINT16  reserved;
    }header;

    UINT8   body[];
}NmAtiCnf;

/*
 * Signal: SIG_NM_ATI_IND
 * signal to AT CMD SRV task,
 *  just the result indication for some TCP/IP app (ping/iperf, etc)
*/
typedef struct NmAtiIndTag
{
    struct {
        UINT16  indId;      //NM_ATI_INDICATION_ID
        UINT16  reserved;
    }header;

    UINT8   body[];
}NmAtiInd;
#endif

/******************************************************************************
 * Net manager entity
******************************************************************************/

/*
 * PS NET status, status from CEREG
*/
typedef enum NmPsNetStatus_Tag
{
    /*
     * PS is not registered, and UE is not under dialing
    */
    NM_PS_NET_NOT_DIAL,

    /*
     * 1> PS is not registered, but under registration
     * 2> and no ACT NETIF (bearer)
    */
    NM_PS_NET_NOT_REG,

    /*
     * 1> PS is OOS (out of service)
     * 2> and must have ACT NETIF (bearer);
     * 3> If no ACT NETIF (bearer), status need to change to "NM_PS_NET_NOT_REG"
    */
    NM_PS_NET_OOS,

    /*
     * PS is registered, and NETIF maybe comes later
    */
    NM_PS_NET_REG
}NmPsNetStatus;

typedef struct NmEntity_Tag
{
    UINT8   psNetStatus;    //NmPsNetStatus
    UINT8   rsvd0;

    /*
     * CID valid value: 1 - 15;
     * If one CID (bearer) activated, related bit set to 1
    */
    UINT16  actCidsBitmap;
}NmEntity;


/******************************************************************************
 * A blocked callback designation, which called in LWIP task
******************************************************************************/
typedef enum NmBlockCallbackMsgId_Tag
{
    NM_BLOCK_CB_INVALID_ID,
    NM_NET_LINK_UP,
    NM_NET_LINK_DOWN,
    NM_GET_NET_INFO,
    NM_CLEAR_DNS_CACHE,
    NM_GET_DNS_SERVER,
    NM_SET_DNS_SERVER,
    NM_START_SNTP,
    NM_STOP_SNTP,
}NmBlockCallbackMsgId;

typedef struct NmBlockCallbackHdr_Tag
{
    UINT8   blcokMsgId;
    UINT8   rsvd0;
    UINT16  inputParaLen;
    UINT16  outputParaLen;
    UINT16  magicWord;      //2 bytes magic
}NmBlockCallbackHdr;    //8 bytes


/*
 * The ip address structure
 * NM_ADDR_IPV4_ADDR: 12~15:0 0~3:address
 * NM_ADDR_FULL_IPV6_ADDR: 0~15 prefix:id
 * NM_ADDR_IPV6_ID: 8~15:0 0~7:id
*/
typedef struct NmIpAddr_Tag
{
    UINT8   addrType;   //NmAddrType
    UINT8   reserved0;
    UINT16  reserved1;
    UINT8   addr[NM_ADDR_MAX_LENGTH];
}NmIpAddr;  //20 bytes


/******************************************************************************
 * ECIPERF related
******************************************************************************/
/*
 * iperf mode type
*/
typedef enum NmIperfAction_Enum
{
    NM_IPERF_STOP_ALL = 0,
    NM_IPERF_START_CLIENT = 1,
    NM_IPERF_STOP_CLIENT = 2,
    NM_IPERF_START_SERVER = 3,
    NM_IPERF_START_UDP_NAT_SERVER = 4,   //special mode
    NM_IPERF_STOP_SERVER = 5
}NmIperfAction;

/*
 * iperf network domain type
*/
typedef enum NmIperfProtocol_Enum
{
    NM_IPERF_PROTOCOL_UDP = 0,
    NM_IPERF_PROTOCOL_TCP = 1,
}NmIperfProtocol;

typedef struct NmIperfReq_Tag
{
    UINT8       reqAct;     //NmIperfAction
    UINT8       protocol;   //NmIperfProtocol
    UINT16      port;       //client/server UDP/TCP port

    BOOL        destAddrPresent;
    BOOL        tptPresent;
    BOOL        payloadSizePresent;
    BOOL        pkgNumPresent;

    ip_addr_t   destAddr;   //client, or UDP NAT server DEST address, 20 bytes
    UINT32      tpt;        //client through put setting, in bps, such as: 20000, just means 20000 bps

    UINT16      payloadSize;//client parameter
    UINT16      pkgNum;     //client parameter

    BOOL        durationPresent;
    BOOL        rptIntervalPresent;
    UINT16      rsvd0;

    UINT16      durationS;      //client/server parameter, in seconds
    UINT8       rptIntervalS;   //report internal in seconds
    UINT8       rsvd1;

    UINT16      reqHandle;
    UINT16      rsvd2;
}NmIperfReq;    //46 bytes



/******************************************************************************
 * PS netif configuration info
 * Use to link up one netif
******************************************************************************/
typedef struct NmIfConfiguration_Tag
{
    UINT16      dnsNum : 3;
    UINT16      mtuPresent : 1; //if mtu is invalid ,this value will be zero
    UINT16      cgevReason: 4;  //CmiPsPdnTypeReason
    UINT16      rsvd0 : 8;

    UINT16      mtu;
    NmIpAddr    ipv4Addr;
    NmIpAddr    ipv6Addr;
    NmIpAddr    dns[NM_MAX_DNS_NUM];
}NmIfConfiguration; //124 bytes


/******************************************************************************
 * NM_ATI_PRIM_GT_DNS_REQ
 * DNS query to get URL IP ADDR
******************************************************************************/
#define NM_MAX_URL_STR_LEN  256

typedef struct NmAtiGetDnsReqTag
{
    BOOL bDisableCache;
    UINT8 rsdv1;
    UINT8 rsdv2;
    CHAR        url[NM_MAX_URL_STR_LEN];
}NmAtiGetDnsReq;

/*
 * NM_ATI_PRIM_GT_DNS_CNF
 * Suggest carry the UINT8 IP ADDR, - TBD
*/
typedef struct NmAtiGetDnsCnfTag
{
    CHAR response[64]; //need support ipv6 dns resolve
}NmAtiGetDnsCnf;


/******************************************************************************
 * NM_ATI_PRIM_PING_RESULT_IND
******************************************************************************/
typedef enum NmPingResultEnum_Tag
{
    NM_PING_RET_DONE,       //ping succ, and ping done
    NM_PING_RET_ONE_SUCC,   //One ping is succ, and maybe ping is still ongoing
    NM_PING_RET_ONE_FAIL,   //One ping is fail, and maybe ping is still ongoing
    NM_PING_RET_ONE_SEND_FAIL,  //One ping send fail
    NM_PING_PARAM_ERROR,    //input parameters error
    NM_PING_DNS_ERROR,      //DNS failed
    NM_PING_SOCKET_ERROR,   //socket error
    NM_PING_ALL_FAIL,
    NM_PING_ERROR
}NmPingResultEnum;

#define NM_ATI_PING_DEST_IP_STR_LEN    40

typedef struct NmAtiPingResultIndTag
{
    UINT8   result;         //NmPingResultEnum
    UINT8   ttl;            //the TTL value from ping reply packet
    UINT16  packetLossRate; //valid when "NM_PING_RET_DONE", RATE*100, example: 23 just means: 23%

    UINT16  requestNum;     //if "NM_PING_RET_ONE_SUCC/NM_PING_RET_ONE_FAIL", set to 1
    UINT16  responseNum;    //if "NM_PING_RET_ONE_SUCC" set 1, if "NM_PING_RET_ONE_FAIL", set to 0

    /*
     * ms,
     * if "NM_PING_RET_ONE_SUCC", set RTT at this time,
     * if "NM_PING_RET_ONE_FAIL", set max wait time
    */
    UINT32  minRtt;
    UINT32  maxRtt;         //ms
    UINT32  avgRtt;         //ms

    /*ping pkg payload len*/
    UINT16  payloadLen;
    UINT16  rsvd;

    /*
     * Dest IP address, in string format;
     * For Ipv4, example: "10.10.10.10"
     * For Ipv6, example: "1234:1234:1234:1234:1234:1234:1234:1234"
    */
    CHAR    pingDst[NM_ATI_PING_DEST_IP_STR_LEN];
}NmAtiPingResultInd;  //60 bytes


typedef enum NmSntpResultEnum_Tag{
    SNTP_RESULT_OK = 0,
    SNTP_RESULT_CREATE_CLIENT_SOCK_FAIL,
    SNTP_RESULT_PARAMTER_INVALID,
    SNTP_RESULT_URL_RESOLVE_FAIL,
    SNTP_RESULT_NETWORK_NOT_READY,
    SNTP_RESULT_RETRY_REACH_MAX_TIMES,
    SNTP_RESULT_FAIL,
}NmSntpResultEnum;



typedef struct NmAtiSntpResultInd_Tag{
    UINT16 result;
    UINT16 autoSync;
    UINT32 time;
    UINT32 us;
}NmAtiSntpResultInt;

typedef struct NmNetIfDnsCfg_Tag{
    UINT8 dnsNum;
    UINT8 rsvd0;
    UINT16 rsvd1;
    ip_addr_t  dns[NM_MAX_DNS_NUM];
}NmNetIfDnsCfg; //84 bytes

/******************************************************************************
 * NM_ATI_IPERF_RET_IND
******************************************************************************/
typedef enum NmIperfModeEnum_Tag
{
    NM_IPERF_MODE_CLIENT,       //ECIPERF tmp report for every rtpInterval, iperf task is on goging
    NM_IPERF_MODE_SERVER,   //ECIPERF the last report, iperf task end
}NmIperfModeEnum;

typedef enum NmIperfResultEnum_Tag
{
    NM_IPERF_END_REPORT_SUCCESS,   //ECIPERF the last report, iperf task end
    NM_IPERF_ONE_REPORT_SUCCESS,   //ECIPERF tmp report for every rtpInterval, iperf task is on goging
    NM_IPERF_PARAM_ERROR,   //iperf parameter error, iperf task end
    NM_IPERF_SOCKET_ERROR,  //iperf socket error, iperf task end
    NM_IPERF_MALLOC_ERROR,  //iperf task malloc fail, iperf task end
}NmIperfResultEnum;

typedef struct NmAtiIperfResultIndTag
{
    UINT8   result;   //NmIperfResultEnum
    UINT8   mode;     //NmIperfModeEnum
    UINT16  reserved;

    /*
     * "dataNum"/"bandwidth" only valid when
     * 1> "result" == NM_IPERF_END_REPORT_SUCCESS, total pkg bytes and average bandwith (send/recv) during the whole duration;
     * 2> "result" == NM_IPERF_ONE_REPORT_SUCCESS, total pkg bytes and bandwith (send/recv) during report interval
    */
    UINT32  dataNum;    //client mode, total send packet bytes; for server mode, total receive packet bytes
    UINT32  bandwidth;  //bps
}NmAtiIperfResultInd;

/******************************************************************************
 * NM_ATI_SYNC_GET_NET_INFO_REQ
******************************************************************************/
typedef struct NmAtiGetNetInfoReq_Tag
{
    /*
     * 1> If user want to get the default netif info, CID could set to "NM_PS_DEFAULT_CID", CID 0 is used for attached bearer (initial PDP context)
     * 2> Valid value: [NM_PS_CID_MIN - NM_PS_CID_MAX]/[0-15]
    */
    UINT8       cid;
    UINT8       rsvd0;
    UINT16      rsvd1;
}NmAtiGetNetInfoReq;

/******************************************************************************
 * NM_ATI_SYNC_GET_NET_INFO_REQ
******************************************************************************/
typedef struct NmAtiClearDnsCacheReq_Tag
{

    BOOL       bAll;
    UINT8       rsvd0;
    UINT16      rsvd1;
    char        name[DNS_MAX_NAME_LENGTH];
}NmAtiClearDnsCacheReq;


typedef struct NmAtiGetDnsServerReq_Tag
{
    UINT8       cid;
    UINT8       rsvd0;
    UINT16      rsvd1;
}NmAtiGetDnsServerReq;

typedef struct NmAtiSetDnsServerReq_Tag
{
    UINT8       cid;
    NmNetIfDnsCfg dnsCfg;
}NmAtiSetDnsServerReq;


/*
 * the netif related ip info
*/
typedef struct NmNetIpv6Info_Tag{
    ip6_addr_t  ipv6Addr;

    UINT8       dnsNum;
    UINT8       rsvd0;
    UINT16      ipv6GetPrefixDelay;
    ip6_addr_t  dns[NM_PDN_TYPE_MAX_DNS_NUM];
}NmNetIpv6Info;   //52 bytes


typedef struct NmNetIpv4Info_Tag{
    ip4_addr_t  ipv4Addr;

    UINT8       dnsNum;
    UINT8       rsvd0;
    UINT16      rsvd1;
    ip4_addr_t  dns[NM_PDN_TYPE_MAX_DNS_NUM];
}NmNetIpv4Info;   //16 bytes

/*
 * netif type
*/
typedef enum NmNetifType_Tag
{
    NM_INVALID_NETIF,
    NW_DEFAULT_NETIF,   //default netif, all socket data default to send via this netif
    //NW_IMS_NETIF      //FFS
}NmNetifType;


/*
 * netif status
*/
typedef enum NmNetifStatus_Tag
{
    /*
     * no netif created, and UE is not under dialing
    */
    NM_NO_NETIF_NOT_DIAL,

    /*
     * No netif created
     * 1> maybe UE is not registered, but already under dialing, and NETIF maybe be OK later
     * 2> or APP request one specific NETIF (by CID), which not created,
     * 3> when a NETIF deactivated
    */
    NM_NO_NETIF_OR_DEACTIVATED,

    /*
     * netif is created, but NETIF is OOS
    */
    NM_NETIF_OOS,

    /*
     * NETIF is suspend
     * When NETIF is OOS, must be also suspend, in such case, state should be set to OOS
    */
    //NM_NETIF_SUSPEND,

    /*
     * NETIF is activated
    */
    NM_NETIF_ACTIVATED,

    /*
     * NETIF ACTED, but some info changed, such as:
     * 1> ipv4v6 type, ipv6 RS success later, need to report NETIF changed;
     * 2> ipv4v6 two bearers, if one bearer deactivated, but another one is still exist, also need to report NETIF changed;
     * 3> This status only used for event indication report;
    */
    NM_NETIF_ACTIVATED_INFO_CHNAGED
}NmNetifStatus;

/*
*net status change cause
*
*/
typedef enum NmNetifStatusChangeCause_Tag
{
    NM_STATUS_CHANGE_LINK_UP,
    NM_STATUS_CHANGE_LINK_DOWN,
    NM_STATUS_CHANGE_RA_SUCCESS,
    NM_STATUS_CHANGE_RA_TIMEOUT,
    NM_STATUS_CHANGE_ENTER_OOS,
    NM_STATUS_CHANGE_EXIT_OOS,
    NM_STATUS_CHANGE_LINK_UP_PDN_IPV4_ONLY,
    NM_STATUS_CHANGE_LINK_UP_PDN_IPV6_ONLY,
    NM_STATUS_CHANGE_LINK_UP_PDN_SINGLE_ADDRESS_ONLY,
    NM_STATUS_CHANGE_LINK_UP_PDN_SINGLE_ADDR_ONLY_ALLOWED_AND_SECOND_BEARER_FAILED,
    NM_STATUS_CHANGE_OTHERS,
}NmNetifStatusChangeCause;

/*
 * One NETIF info
*/
typedef struct NmAtiNetifInfo_Tag
{
    UINT8               netStatus;  //NmNetifStatus
    UINT8               netifType;  //NmNetifType
    UINT8               ipType;     //NmNetIpType
    UINT8               cause;   //NmNetifStatusChangeCause

    /*
     * 1> if NETIF is ipv4v6 type:
     *    a> if two bearers created, "ipv4Cid" is for IPV4 bearer, and "ipv6Cid" is for IPV6 bearer
     *    b> if one bearer (ipv4v6) created, "ipv4Cid" = "ipv6Cid"
     * 2> if NETIF is ipv4 type
     *    a> "ipv6Cid" should set to NM_PS_INVALID_CID
     * 3> if NETIF is ipv6 type
     *    a> "ipv4Cid" should set to NM_PS_INVALID_CID
    */
    UINT8               ipv4Cid;
    UINT8               ipv6Cid;
    UINT16              rsvd1;

    NmNetIpv4Info       ipv4Info;   //first need to check "ipType", if "ipType" indicate ipv4 or ipv4v6, then this info must be valid
    NmNetIpv6Info       ipv6Info;   //first need to check "ipType", if "ipType" indicate ipv6 or ipv4v6, then this info must be valid
}NmAtiNetifInfo;    // 76 bytes


/*
 * Result for: NM_ATI_SYNC_GET_NET_INFO_REQ, just return NET info info
*/
typedef struct NmAtiGetNetInfoRet_Tag
{
    NmAtiNetifInfo      netifInfo;
}NmAtiGetNetInfoRet;    //76 bytes


/*
 * NM_ATI_SYNC_IS_ANY_ACTIVE_SOCKET_REQ,
 *  return whether any activted socket
*/
typedef struct NmAtiIsAnyActiveSockRet_Tag
{
    BOOL                bAnyActSock;
    UINT8               rsvd0;
    UINT16              rsvd1;
}NmAtiIsAnyActiveSockRet;


/*
 * NM ATI SYNC result
*/
typedef struct NmAtiSyncRet_Tag
{
    union {
        NmAtiGetNetInfoRet      netInfoRet;     //76 bytes
        NmAtiIsAnyActiveSockRet anyActSockRet;  //4 bytes
        NmNetIfDnsCfg           getDnsServer;   //84 bytes
    }body;
}NmAtiSyncRet;  // 84 bytes


/******************************************************************************
 * NM_ATI_NET_INFO_IND
 * NmAtiNetInfoInd
******************************************************************************/
typedef struct NmAtiNetInfoInd_Tag
{
    NmAtiNetifInfo  netifInfo;
}NmAtiNetInfoInd;   // 76 bytes


/******************************************************************************
 *****************************************************************************
 * Functions
 *****************************************************************************
******************************************************************************/

/******************************************************************************
 * NetMgrLinkUp
 * Description: net manager pdn linkup function
 * input: UINT8 cid     //which CID (CAC layer ID)
 *        NmIfConfiguration *ifConf //default bearer configuration
 *        UINT8 bindToCid   //if NW assign two bearer (one for IPV4, and another for IPV6)
 *                          // when second bearer setup, need to bind to orig "netif"
 *                          // if set to "255", just means not such case
 *        BOOL bWakeup; //whether this NETIF/bearer is linked up during sleep2/hib wake up procedure
 * output: NmResult;
 * Comment:
******************************************************************************/
NmResult NetMgrLinkUp(UINT8 cid, NmIfConfiguration *pIfCfg, UINT8 bindToCid, BOOL bWakeUp);

/******************************************************************************
 * NetMgrLinkDown
 * Description: netmngr pdn linkdown function
 * input: UINT8 cid
 * output: NmResult
 * Comment:
******************************************************************************/
NmResult NetMgrLinkDown(UINT8 cid);


/******************************************************************************
 * NetMgrPingRequest
 * Description: netmngr ip ping request function
 * input: NmIpAddr *pDest   //ping DEST IP address info
 *        UINT8 tFlag       //ping count, if set "255", just means keep ping, default value 15
 *        UINT16 payloadLen     //ping payload byte len, default: 32; ping pkg size: 28(ping ICMP header)+32 = 60
 *        UINT32 timeout    //set wait time for one ping pkg
 *        BOOL   raiFlag    //whether enable rai info flag
 * output: NmResult
 * Comment:
******************************************************************************/
NmResult NetMgrPingRequest(ip_addr_t *pDest, UINT8 count, UINT16 payloadLen, UINT32 timeout, BOOL raiFlag, UINT16 reqHandle);

/******************************************************************************
 * NetMgrPingUrlRequest
 * Description: netmngr url ping request function
 * input: const CHAR* pingTarget,
 *        UINT8 count,
 *        UINT16 payloadLen,
 *        UINT32 timeout
 *        BOOL   raiFlag    //whether enable rai info flag
 * output:
 * Comment:
******************************************************************************/
NmResult NetMgrPingUrlRequest(const CHAR* pingTarget, UINT8 count, UINT16 payloadLen, UINT32 timeout, BOOL raiFlag, UINT16 reqHandle);

/******************************************************************************
 * NetMgrPingTerminate
 * Description: netmngr ping request terminate function
 * input:
 * output:
 * Comment:
******************************************************************************/
void NetMgrPingTerminate(void);

/*
*the reference AT command format: AT^ECIPERF:<action>,[protocol],["ipaddr"],[port],[tpt],[pkg_payload_size],[pkg_num],[duration],[rpt_interval]
*1 action:NmIperfAction
*1.1 client mode: must param <action>,<"ipaddr">,<tpt>
*1.2 server mode: must param <action>
*1.3 nat server mode: must param <action>,<"ipaddr">
*2 protocol: default udp
*3 "ipaddr": ipv4 or ipv6
*3.1 dst addr for client mode [must]
*3.2 remote client address for nat server mode [must]
*4 port: default 5001
*4.1 server rcv port for server mode
*4.2 dst port for client mode
*5 tpt: throughput
*6 pkg_payload_size: max -> MTU-28
*7 pkg_num
*8 duration
*9 rtp_interval: default 10 seconds
*/

/******************************************************************************
 * NetMgrIperfRequest
 * Description: netmngr iperf request function
 * input: NmIperfReq *pIperfReq
 * output: NmResult
 * Comment:
******************************************************************************/
NmResult NetMgrIperfRequest(NmIperfReq *pIperfReq);

/******************************************************************************
 * NetMgrSntpStart
 * Description: netmngr sntp start function
 * input: sntp server address and server port
 * output: NmResult
 * Comment:
******************************************************************************/
NmResult NetMgrSntpStart(CHAR *server, UINT16 port, UINT16 reqHandler, BOOL autoSync);

/******************************************************************************
 * NetMgrSntpStop
 * Description: netmngr stop sntp function
 * input:
 * output:
 * Comment:
******************************************************************************/
void NetMgrSntpStop(void);

/******************************************************************************
 * NetMgrAtiAsyncReq
 * Description: netmngr at srv async request function
 * input:   UINT16 srcHdr,
 *          UINT16 nmReqId,
 *          void *reqBody
 * output: NmResult
 * Comment:
 *  Async request API, return:
 *  1> NM_SUCCESS, just means a signal "SIG_NM_ATI_CNF" will be sent to ATCMDSRV late
 *  2> others, just means this request it failed, and no "SIG_NM_ATI_CNF" late
******************************************************************************/
NmResult NetMgrAtiAsyncReq(UINT16 srcHdr, UINT16 nmReqId, void *reqBody);

/******************************************************************************
 * NetmgrAtiSyncReq
 * Description: netmngr sync request API.
 * input:   UINT16 reqId, void *reqBody, void *result
 * output:
 * Comment:
******************************************************************************/
NmResult NetmgrAtiSyncReq(UINT16 reqId, void *reqBody, NmAtiSyncRet *pRet);

/*
 * Send URL DNS result
*/
void NetMgrSendGetDnsCnf(INT32 result, ip_addr_t *pIpInfo, UINT16 srcHdr);


/*
 * Send PING result
*/
void NetMgrSendPingResultInd(NmAtiPingResultInd *pPingRet, UINT16 reqHandle);

/*
 * Send ECIPERF result
*/
void NetMgrSendIperfResultInd(NmAtiIperfResultInd *pIperfRet, UINT16 reqHandle);

/*
 * Send SNTP result
*/
void NetMgrSendSntpResultInd(NmAtiSntpResultInt *pSntpRet, UINT16 reqHandle);

/*
 * Send NET (NETIF) info (status)
*/
void NetMgrSendNetInfoInd(NmAtiNetInfoInd *pNetInfoInd);

/*
 * Proc "CMI_PS_GET_CEREG_CNF"
*/
void NetMgrProcCeregCnf(CmiPsGetCeregCnf *pCregCnf);

/*
 * Proc "CMI_PS_CEREG_IND"
*/
void NetMgrProcCeregInd(CmiPsCeregInd *pCregInd);


#endif

