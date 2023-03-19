/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ps_lib_api.h
 * Description:  EC616 opencpu pslibapi header file
 * History:      Rev1.0   2018-12-10
 *
 ****************************************************************************/

#ifndef __PS_LIB_API_H__
#define __PS_LIB_API_H__
#include "task.h"
#include "cmsis_os2.h"
#include "cms_util.h"
#include "netmgr.h"
#include "cmidev.h"
#include "cmips.h"


/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/
#define NV9_DATA_LEN               96       /*not suitable to put here, -TBD*/
#define NV9_DATA_IMEI_LEN          32
#define NV9_DATA_SN_LEN            32
#define NV9_DATA_IMEI_LOCK_LEN     16
#define NV9_DATA_SN_LOCK_LEN       16

#define NV9_DATA_IMEI_OFFSET        0
#define NV9_DATA_SN_OFFSET          32
#define NV9_DATA_IMEI_LOCK_OFFSET   64
#define NV9_DATA_SN_LOCK_OFFSET     80


#define PS_APN_MAX_SIZE             (CMI_PS_MAX_APN_LEN+1)

/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/



/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/

/*
 * APP request PS service
*/
typedef struct AppPsCmiReqData_Tag
{
    /* request input */
    UINT8   sgId;       //PS service group ID: CacSgIdEnum
    UINT8   rsvd0;

    UINT16  reqPrimId;  //request prim ID.
    UINT16  cnfPrimId;

    UINT16  reqParamLen;
    void    *pReqParam;

    /* confirm output */
    UINT16  cnfRc;      //confirm return code: MtErrorResultCode/SmsErrorResultCode
    UINT16  cnfBufLen;
    void    *pCnfBuf;   //filled in callback API
}AppPsCmiReqData;   //20 bytes



struct imsiApiMsg {
  CHAR *imsi;
  UINT8 len;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct iccidApiMsg {
  CHAR *iccid;
  UINT8 len;
  osSemaphoreId_t *sem;
  UINT16 result;
};


struct cgcontrdpApiMsg {
  UINT8 *cid;
  UINT8 *len;
  UINT8 index;
  UINT8 *APN;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct celevelApiMsg {
  UINT8 act;
  UINT8 celevel;
  UINT8 cc;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct signalApiMsg {
  INT8 *snr;
  INT8 *rsrp;
  UINT8 *csq;
  osSemaphoreId_t *sem;
  UINT16 result;
};



struct psmApiMsg {
  UINT8 *psmMode;
  UINT32 *tauTimeS;
  UINT32 *activeTimeS;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct edrxApiMsg {
  UINT8 *actType;
  UINT32 *edrxValueMs;
  union {
    UINT32 *nwPtwMs;
    UINT8 *modeVal;
  } share;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct ceregApiMsg {
  UINT8 *state;
  UINT16 *tac;
  UINT32 *cellId;
  UINT32 *tauTimeS;
  UINT32 *activeTimeS;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct extCfgApiMsg {
  UINT8 powerlevel;
  UINT8 powerCfun;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct cfunApiMsg {
  UINT8 fun;
  UINT8 rst;
  osSemaphoreId_t *sem;
  UINT16 result;
};


struct powerStateApiMsg {
  UINT8 powerState;
  UINT16 result;
};

struct cchoApiMsg {
  UINT8 *dfName;
  UINT8 *sessionID;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct cchcApiMsg {
  UINT8 sessionID;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct cglaApiMsg {
  UINT8 sessionID;
  UINT8 *command;
  UINT16 cmdLen;
  UINT8 *response;
  UINT16 *respLen;
  osSemaphoreId_t *sem;
  UINT16 result;
};

#define SIM_FILE_PATH_MAX_LEN 8
#define SIM_APDU_DATA_MAX_LEN 256
typedef struct CrsmCmdParam_Tag
{
    UINT8       command;
    UINT16      fileId;
    UINT8       p1;
    UINT8       p2;
    UINT8       p3;
    UINT16      dataLen;
    UINT8       data[SIM_APDU_DATA_MAX_LEN];
    UINT8       pathLen;
    UINT8       filePath[SIM_FILE_PATH_MAX_LEN];
}
CrsmCmdParam;

typedef struct CrsmRspParam_Tag
{
    UINT8    sw1;
    UINT8    sw2;
    UINT8    respLen;
    UINT8    responseData[SIM_APDU_DATA_MAX_LEN];

}
CrsmRspParam;

struct crsmApiMsg {
  CrsmCmdParam  *cmdParam;
  CrsmRspParam  *responseParam;
  osSemaphoreId_t *sem;
  UINT16 result;
};

struct ecbandApiMsg {
    UINT8 networkMode;
    UINT8 bandNum;
    UINT16 result;
    UINT8 *orderBand;
    osSemaphoreId_t *sem;
};


/******************************************************************************
 * Neighber cell info/measurment
 * Note: this API will trigger neighber cell search/measurement, which maybe
 *        cost some power
******************************************************************************/
typedef enum NeighCellReqType_enum
{
    GET_NEIGHBER_CELL_MEAS      = CMI_DEV_GET_BASIC_CELL_MEAS,  /* Only measure neighber cell, no cellID/TAC returned */
    GET_NEIGHBER_CELL_ID_INFO   = CMI_DEV_GET_BASIC_CELL_ID     /* Search neighber cell, neighber cellID & TAC also returned, this cost more time */
}NeighCellReqType;

#define PS_LIB_BASIC_CELL_REQ_MAX_CELL_NUM  5       //MAX cell measured: 1 serving cell + 4 neighber cell

typedef CmiDevGetBasicCellListInfoCnf BasicCellListInfo;


/******************************************************************************
 * CELL lock/unlock,
 * perfer FREQ setting/clear
******************************************************************************/
#define SUPPORT_MAX_FREQ_NUM    8 //== CMI_DEV_SUPPORT_MAX_FREQ_NUM
typedef struct CiotSetFreqParams_Tag
{
    UINT8   mode;       // CmiDevSetFreqModeEnum
    UINT8   cellPresent;// indicate whether phyCellId present
    UINT16  phyCellId; // phyCell ID, 0 - 503

    UINT8   arfcnNum; // 0 is not allowed for mode is CMI_DEV_SET_PREFER_FREQ (1);
                      // max number is CMI_DEV_SUPPORT_MAX_FREQ_NUM
    UINT8   reserved0;
    UINT16  reserved1;
    UINT32  lockedArfcn;//locked EARFCN
    UINT32  arfcnList[SUPPORT_MAX_FREQ_NUM];
}CiotSetFreqParams; // total 44 bytes

typedef struct CiotGetFreqParams_Tag
{
    UINT8   mode;       // CmiDevGetFreqModeEnum  3:means UE has set preferred EARFCN list and has locked EARFCN
    UINT8   cellPresent;// indicate whether phyCellId present
    UINT16  phyCellId; // phyCell ID, 0 - 503

    UINT8   arfcnNum; // 0 is not allowed for mode is CMI_DEV_SET_PREFER_FREQ (1);
                      // max number is CMI_DEV_SUPPORT_MAX_FREQ_NUM
    UINT8   reserved0;
    UINT16  reserved1;
    UINT32  lockedArfcn;//locked EARFCN
    UINT32  arfcnList[SUPPORT_MAX_FREQ_NUM];
}CiotGetFreqParams; // total 44 bytes


/******************************************************************************
 * Get PS extended status info
******************************************************************************/
typedef enum UeExtStatusType_TAG
{
    UE_EXT_STATUS_ALL   = CMI_DEV_GET_ECSTATUS,
    UE_EXT_STATUS_PHY   = CMI_DEV_GET_ECSTATUS_PHY,
    UE_EXT_STATUS_L2    = CMI_DEV_GET_ECSTATUS_L2,
    UE_EXT_STATUS_ERRC  = CMI_DEV_GET_ECSTATUS_RRC,
    UE_EXT_STATUS_EMM   = CMI_DEV_GET_ECSTATUS_EMM,
    UE_EXT_STATUS_PLMN  = CMI_DEV_GET_ECSTATUS_PLMN,
    UE_EXT_STATUS_ESM   = CMI_DEV_GET_ECSTATUS_ESM,
    UE_EXT_STATUS_CCM   = CMI_DEV_GET_ECSTATUS_CCM
}UeExtStatusType;

typedef CmiDevGetExtStatusCnf   UeExtStatusInfo;

/******************************************************************************
 * Set attach bearer parameter
******************************************************************************/
typedef CmiPsSetAttachedBearerCtxReq    SetAttachBearerParams;

/******************************************************************************
 * Get attach bearer setting parameter
******************************************************************************/
typedef CmiPsGetAttachedBearerCtxCnf    GetAttachBearerSetting;

/******************************************************************************
 * Set UE extended configuration
******************************************************************************/
typedef CmiDevSetExtCfgReq   SetExtCfgParams;

/******************************************************************************
 * Get UE extended configuration
******************************************************************************/
typedef CmiDevGetExtCfgCnf   GetExtCfgSetting;

/******************************************************************************
 * Set PS sleep configuration
******************************************************************************/
typedef CmiDevSetPsSleepCfgReq   SetPsSlpCfgParams;

/******************************************************************************
 * Get PS sleep configuration
******************************************************************************/
typedef CmiDevGetPsSleepCfgCnf   GetPsSlpCfgParams;


/******************************************************************************
 *****************************************************************************
 * API
 *****************************************************************************
******************************************************************************/

//void appCheckTcpipReady(void);
void psSyncProcCmiCnf(const SignalBuf *cnfSignalPtr);

CmsRetId initPsCmiReqMapList(void);
CmsRetId appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *pNmNetInfo);
CmsRetId appGetImsiNumSync(CHAR *imsi);
CmsRetId appGetIccidNumSync(CHAR *iccid);
CmsRetId appGetImeiNumSync(CHAR *imei);
CmsRetId appGetPSMSettingSync(UINT8 *psmmode, UINT32 *tauTime, UINT32 *activeTime);
CmsRetId appSetPSMSettingSync(UINT8 psmMode, UINT32 tauTime, UINT32 activeTime);
CmsRetId appGetCeregStateSync(UINT8 *state);
CmsRetId appGetEDRXSettingSync(UINT8 *actType, UINT32 *nwEdrxValueMs, UINT32 *nwPtwMs);
CmsRetId appSetEDRXSettingSync(UINT8 modeVal, UINT8 actType, UINT32 reqEdrxValueMs);
CmsRetId appGetLocationInfoSync(UINT16 *tac, UINT32 *cellId);
CmsRetId appGetTAUInfoSync(UINT32 *tauTimeS, UINT32 *activeTimeS);
CmsRetId appGetAPNSettingSync(UINT8 cid, UINT8 *apn);
CmsRetId appCheckSystemTimeSync(void);
CmsRetId appGetSystemTimeSecsSync(time_t *time);
CmsRetId appGetSystemTimeUtcSync(OsaUtcTimeTValue *time);
CmsRetId appSetSystemTimeUtcSync(UINT32 time1, UINT32 time2, INT32 timeZone);
CmsRetId appGetActedCidSync(UINT8 *cid, UINT8 *num);

/**
  \fn          appSetSimLogicalChannelOpenSync
  \brief       Send cmi request to open SIM logical channel
  \param[out]  *sessionID: Pointer to a new logical channel number returned by SIM
  \param[in]   *dfName: Pointer to DFname selected on the new logical channel
  \returns     CMS_RET_SUCC: success
  \            CMS_SIM_NOT_INSERT: SIM not inserted
  \            CMS_OPER_NOT_SUPPROT: operation not supported
*/
CmsRetId appSetSimLogicalChannelOpenSync(UINT8 *sessionID, UINT8 *dfName);

/**
  \fn          appSetSimLogicalChannelCloseSync
  \brief       Send cmi request to close SIM logical channel
  \param[in]   sessionID: the logical channel number to be closed
  \returns     CMS_RET_SUCC---success
  \            CMS_SIM_NOT_INSERT: SIM not inserted
  \            CMS_INVALID_PARAM: input invalid parameters
*/
CmsRetId appSetSimLogicalChannelCloseSync(UINT8 sessionID);

/**
  \fn          appSetSimGenLogicalChannelAccessSync
  \brief       Send cmi request to get generic SIM logical channel access
  \param[in]   sessionID: the logical channel number
  \param[in]   *command: Pointer to command apdu, HEX string
  \param[in]   cmdLen: the length of command apdu, max value is CMI_SIM_MAX_CMD_APDU_LEN * 2 (522)
  \param[out]  *response: Pointer to response apdu, HEX string
  \param[out]  respLen: the length of command apdu, max value is 4KB
  \returns     CmsRetId
*/
CmsRetId appSetSimGenLogicalChannelAccessSync(UINT8 sessionID, UINT8 *command, UINT16 cmdLen,
                                                UINT8 *response, UINT16 *respLen);

/**
  \fn          appSetRestrictedSimAccessSync
  \brief       Send cmi request to get generic SIM access
  \param[in]   *pCmdParam: Pointer to command parameters
  \param[out]  *pRspParam: Pointer to response parameters
  \returns     CMS_RET_SUCC---success
  \            CMS_SIM_NOT_INSERT: SIM not inserted
  \            CMS_INVALID_PARAM: input invalid parameters
*/
CmsRetId appSetRestrictedSimAccessSync(CrsmCmdParam *pCmdParam, CrsmRspParam *pRspParam);

/**
  \fn          CmsRetId appSetCFUN(UINT8 fun)
  \brief       Send cfun request to NB
  \param[in]   fun: 0 minimum function and 1 full function
  \returns     CmsRetId
*/
CmsRetId appSetCFUN(UINT8 fun);
/**
  \fn          CmsRetId appGetCFUN(UINT8 *pOutCfun)
  \brief       Get current CFUN state
  \param[out]  *pOutCfun      //refer "CmiFuncValueEnum" CMI_DEV_MIN_FUNC(0)/CMI_DEV_FULL_FUNC(1)/CMI_DEV_TURN_OFF_RF_FUNC(4)
  \returns     CmsRetId
*/
CmsRetId appGetCFUN(UINT8 *pOutCfun);

CmsRetId appSetBootCFUNMode(UINT8 mode);
UINT8 appGetBootCFUNMode(void);
UINT8 appGetSearchPowerLevelSync(void);
UINT8 appGetCELevelSync(void);
CmsRetId appGetSignalInfoSync(UINT8 *csq, INT8 *snr, INT8 *rsrp);
CHAR* appGetNBVersionInfo(void);
void drvSetPSToWakeup(void);
BOOL appSetImeiNumSync(CHAR* imei);
BOOL appGetSNNumSync(CHAR* sn);
BOOL appSetSNNumSync(CHAR* sn, UINT8 len);

BOOL appGetImeiLockSync(CHAR* imeiLock);
BOOL appSetImeiLockSync(CHAR* imeiLock);
BOOL appGetSNLockSync(CHAR* snLock);
BOOL appSetSNLockSync(CHAR* snLock);
BOOL appSetNV9LockCleanSync(void);

/**
  \fn          CmsRetId appSetBandModeSync(UINT8 networkMode, UINT8 bandNum,  UINT8 *orderBand)
  \brief
  \param[out] networkMode: network mode
  \param[out] bandNum: valid num of orderBand
  \param[out] orderBand is an array of size 32 bytes

  \returns     CmsRetId
*/
CmsRetId appSetBandModeSync(UINT8 networkMode, UINT8 bandNum,  UINT8 *orderBand);
/**
  \fn          CmsRetId appGetBandModeSync(UINT8 *networkMode, UINT8 *bandNum,  UINT8 *orderBand)
  \brief
  \param[out] networkMode: network mode
  \param[out] bandNum: valid num of orderBand
  \param[out] orderBand is an array of size 32 bytes
  \returns     CmsRetId
*/
CmsRetId appGetBandModeSync(UINT8 *networkMode, UINT8 *bandNum,  UINT8 *orderBand);

/**
  \fn          CmsRetId appGetSupportedBandModeSync(INT8 *networkMode, UINT8 *bandNum,  UINT8 *orderBand)
  \brief
  \param[out] networkMode: network mode
  \param[out] bandNum: valid num of orderBand
  \param[out] orderBand is an array of size 16 bytes
  \returns     CmsRetId
*/
CmsRetId appGetSupportedBandModeSync(INT8 *networkMode, UINT8 *bandNum,  UINT8 *orderBand);


/**
  \fn           CmsRetId appGetECBCInfoSync(NeighCellReqType reqType,
  \                                         UINT8 reqMaxCellNum,
  \                                         UINT16 maxMeasTimeSec,
  \                                         BasicCellListInfo *pBcListInfo)
  \brief        Request PS to measure/search neighber cell
  \param[in]    reqType         Measurement/Search.
  \param[in]    reqMaxCellNum   How many cell info need to returned, including serving cell, range: (1-5)
  \param[in]    maxMeasTimeSec  Measure/search guard timer, range: (4-300)
  \param[out]   pBcListInfo     Pointer to store the result basic cell info
  \returns      CmsRetId
*/
CmsRetId appGetECBCInfoSync(NeighCellReqType reqType, UINT8 reqMaxCellNum, UINT16 maxMeasTimeSec, BasicCellListInfo *pBcListInfo);

/**
  \fn          CmsRetId appSetCiotFreqSync
  \brief       Send cmi request to set prefer EARFCN list, lock or unlock cell
  \param[in]   CiotSetFreqParams *pCiotFreqParams, the pointer to the CiotSetFreqParams
  \returns     CmsRetId
  \NTOE:       Set EARFCN must be restricted to execute in power off or air plane state.
*/
CmsRetId appSetCiotFreqSync(CiotSetFreqParams *pCiotFreqParams);

/**
  \fn          CmsRetId appGetCiotFreqSync
  \brief       Send cmi request to get the current EARFCN setting.
  \param[out]  CiotGetFreqParams *pCiotFreqParams, the pointer to the CiotGetFreqParams
  \returns     CmsRetId
*/
CmsRetId appGetCiotFreqSync(CiotGetFreqParams *pCiotFreqParams);

/**
  \fn          CmsRetId appGetPSMModeSync(uint8_t *pMode)
  \brief       Send cmi request to get current psm mode
  \param[out]  *pMode     Pointer to store the result of psm mode
  \returns     CmsRetId
*/
CmsRetId appGetPSMModeSync(UINT8 *pMode);


/**
  \fn          appGetUeExtStatusInfoSync
  \brief       Send cmi request to get the extended status infomation of UE.
  \param[in]   UeExtStatusType statusType, the request type of status information
  \param[out]  UeExtStatusInfo *pStatusInfo, the pointer to the status information returned.
  \returns     CmsRetId
*/
CmsRetId appGetUeExtStatusInfoSync(UeExtStatusType statusType, UeExtStatusInfo *pStatusInfo);

/**
  \fn          appSetEDRXPtwSync
  \brief       Send cmi request to set paging time window
  \param[in]   reqPtwValue      PTW value encoded in 24.008
  \returns     CmsRetId
  \NOTE:
*/
CmsRetId appSetEDRXPtwSync(UINT8 reqPtwValue);

/**
  \fn          CmsRetId appGetPtwSettingSync(uint8_t *pMode)
  \brief       Send cmi request to get current PTW setting
  \param[out]  *pPtw     Pointer to store the result of paging time window
  \returns     CmsRetId
*/
CmsRetId appGetPtwSettingSync(UINT8 *pPtw);


/**
  \fn          appSetAttachBearerSync
  \brief       Send cmi request to set attach bearer info, such as ip type and APN
  \param[in]   pAttachBearerParams      INPUT, request attach bearer parameter
  \returns     CmsRetId
  \NOTE:       This attach bearer setting take effect in next attach procedure, refer AT: AT+ECATTBEARER
*/
CmsRetId appSetAttachBearerSync(SetAttachBearerParams *pAttachBearerParams);

/**
  \fn          appGetAttachBearerSettingSync
  \brief       Send cmi request to get attach bearer setting
  \param[out]  pAttachBearerSettingParams   OUTPUT, return current attach bearer setting info
  \returns     CmsRetId
  \NOTE:       Return attach bearer setting, refer AT: AT+ECATTBEARER?
*/
CmsRetId appGetAttachBearerSettingSync(GetAttachBearerSetting *pAttachBearerSettingParams);


/**
  \fn          appSetEccfgSync
  \brief       Send cmi request to set the UE extended configuration
  \param[in]   pSetExtCfgParams     the pointer to set the extended status infomation of UE
  \returns     CmsRetId
  \NOTE:       refer: AT+ECCFG
*/
CmsRetId appSetEccfgSync(SetExtCfgParams *pSetExtCfgParams);

/**
  \fn          appGetEccfgSettingSync
  \brief       Send cmi request to get the extended status infomation of UE.
  \param[out]  pExtCfgSetting   the pointer to the extended status infomation of UE returned.
  \returns     CmsRetId
  \NOTE:       refer: AT+ECCFG
*/
CmsRetId appGetEccfgSettingSync(GetExtCfgSetting *pExtCfgSetting);

/**
  \fn          appGetCsconStateSync
  \brief       Get RRC connection state: idle (0) or connected (1)
  \param[out]  pCsconState      RRC connection state: idle (0) / connected (1)
  \returns     CmsRetId
  \NOTE:       refer: AT+CSCON?
*/
CmsRetId appGetCsconStateSync(UINT8 *pCsconState);

/**
  \fn           appNBRAIReleaseConnection
  \brief        If RRC in connection state, send R13 RAI info to let network release connection
  \param[in]    void
  \returns      CmsRetId
  \NOTE:        refer: AT+ECNBIOTRAI
  \             a) APP must make sure no following UL & DL packet are expected.
  \             b) if in connected state, this API will simulate a UL packet (useless) with the RAI: 1,
  \                to let network release connection
*/
CmsRetId appNBRAIReleaseConnection(void);

/**
  \fn          appSetEcPsSlpCfgSync
  \brief       Send cmi request to set the protocol stack sleep2/hibernate configuration
  \param[in]   SetPsSlpCfgParams     the pointer to set the set parameters
  \returns     CmsRetId
  \NOTE:       refer: AT+ECPSSLPCFG =
*/
CmsRetId appSetEcPsSlpCfgSync(SetPsSlpCfgParams *pSetPsSlpCfgParams);
/**
  \fn           appGetEcPsSlpCfgSync
  \brief        Send cmi request to get the protocol stack sleep2/hibernate config.
  \param[out]   pExtCfgSetting   the pointer to the parameters returned.
  \returns      CmsRetId
  \NOTE:        refer: AT+ECPSSLPCFG?
*/
CmsRetId appGetEcPsSlpCfgSync(GetPsSlpCfgParams *pPsSlpCfgSetting);


#endif

