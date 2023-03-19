/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    pslibapi.c
 * Description:  EC616 opencpu pslibapi source file
 * History:      Rev1.0   2018-12-10
 *
 ****************************************************************************/
#include "bsp.h"
#include "psdial.h"
#include "netmgr.h"
#include "cmicomm.h"
#include "cmisim.h"
#include "cmips.h"
#include "cmimm.h"
#include "cmidev.h"
#include "ps_sim_if.h"
#include "ps_ps_if.h"
#include "ps_mm_if.h"
#include "ps_dev_if.h"
#include "cms_comm.h"
#include "ps_lib_api.h"
#include "ps_sync_cnf.h"
#include "debug_log.h"
#include "nvram.h"


#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position---Below for external use



/**
  \fn           CmsRetId appPsCmiReq(AppPsCmiReqData *pReqData, UINT32 timeOutMs)
  \brief        Send cmi request to PS, to get/set PS service
  \param[in]    pReqData        pointer to app ps cmi request with structure AppPsCmiReqData of 20 bytes
  \param[in]    timeOutMs       timeout in miniseconds, if there is no timeout setting, the parameter is set as CMS_MAX_DELAY_MS
  \returns      CmsRetId
*/
CmsRetId appPsCmiReq(AppPsCmiReqData *pReqData, UINT32 timeOutMs)
{
    CmsRetId    cmsRet = CMS_RET_SUCC;

    /*
     * check the parameters
    */
    OsaDebugBegin(pReqData != PNULL, pReqData, timeOutMs, 0);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin(pReqData->sgId >= CAC_DEV && pReqData->sgId <= CAC_SMS &&
                  pReqData->reqPrimId > 0 && pReqData->reqPrimId < 0x0FFF &&
                  pReqData->reqParamLen > 0 && pReqData->pReqParam != PNULL &&
                  pReqData->cnfPrimId > 0 && pReqData->cnfPrimId < 0x0FFF,
                  pReqData->sgId, pReqData->reqPrimId, pReqData->reqParamLen);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();


    if (timeOutMs == 0)
    {
        OsaDebugBegin(FALSE, timeOutMs, pReqData->sgId, pReqData->reqPrimId);
        OsaDebugEnd();
        timeOutMs = CMS_MAX_DELAY_MS;
    }

    cmsRet = cmsBlockApiCall(psBlockCmiReqCallback,
                             sizeof(AppPsCmiReqData),
                             pReqData,
                             0,
                             PNULL,
                             timeOutMs);

    return cmsRet;
}



/**
  \fn          CmsRetId appGetImsiNumSync(CHAR* imsi)
  \brief       Send cmi request to get IMSI number
  \param[out]  *imsi  Pointer to stored IMSI(max length 16 bytes) result
  \returns     CmsRetId
*/
CmsRetId appGetImsiNumSync(CHAR *imsi)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct imsiApiMsg msg = {0};

    msg.sem = &sem;
    msg.imsi = imsi;
    msg.len = 0;
    msg.result = 0;

    OsaCheck((imsi != NULL)&&(sem != NULL), imsi, sem, 0);

    appCallbackWithNoBlock((app_callback_fn)simGetImsiSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    return ret;
}

/**
  \fn          CmsRetId appGetIccidNumSync(CHAR* iccid)
  \brief       Send cmi request to get USIM iccid number
  \param[out]  *iccid  Pointer to stored USIM ICCID(max len 20 bytes) result
  \returns     CmsRetId
*/
CmsRetId appGetIccidNumSync(CHAR *iccid)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct iccidApiMsg msg = {0};

    msg.sem = &sem;
    msg.iccid = iccid;
    msg.len = 0;
    msg.result = 0;

    OsaCheck((iccid != NULL)&&(sem != NULL), iccid, sem, 0);

    appCallbackWithNoBlock((app_callback_fn)simGetIccidSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    return ret;
}

/**
  \fn          CmsRetId appGetActedCidSync(UINT8 *cid, UINT8 *num)
  \brief       Send cmi request to get cid information
  \param[out]  *cid  Pointer to stored result cid(1~15)
  \param[out]  *num  Pointer to stored result cid num(1~15)
  \returns     CmsRetId
*/
CmsRetId appGetActedCidSync(UINT8 *cid, UINT8 *num)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct cgcontrdpApiMsg msg = {0};
    msg.sem = &sem;
    msg.cid = cid;
    msg.len = num;

    OsaCheck((cid != NULL)&&(sem != NULL)&&(num != NULL), cid, sem, num );

    appCallbackWithNoBlock((app_callback_fn)psGetCGCONTRDPCapsSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appGetPSMSettingSync(UINT8 *psmmode, UINT32 *tauTime, UINT32 *activeTime)
  \brief       Send cmi request to get PSM setting information
  \param[out]  *psmmode     Pointer to store the result mode--psmmode with the Enum "CmiMmPsmReqModeEnum"
               \CMI_MM_DISABLE_PSM(0)/CMI_MM_ENABLE_PSM(1)/CMI_MM_DISCARD_PSM(2)
  \param[out]  *tauTimeS     Pointer to store the result TAU time(unit:S)---related to T3412
  \param[out]  *activeTimeS  Pointer to store the result active time(unit:S)---related to T3324
  \returns     CmsRetId
*/
CmsRetId appGetPSMSettingSync(UINT8 *psmMode, UINT32 *tauTimeS, UINT32 *activeTimeS)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct psmApiMsg msg = {0};
    msg.sem = &sem;
    msg.psmMode = psmMode;
    msg.tauTimeS = tauTimeS;
    msg.activeTimeS = activeTimeS;

    OsaCheck((sem != NULL)&&(psmMode != NULL)&&(tauTimeS != NULL)&&(activeTimeS != NULL), psmMode, tauTimeS, activeTimeS);

    appCallbackWithNoBlock((app_callback_fn)mmGetCpsmsSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }

    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appGetCeregStateSync(UINT8 *state)
  \brief       Send cmi request to get cereg state information
  \param[out]  *state     Pointer to stored result state with Enum "CmiCeregStateEnum"
               \CMI_PS_NOT_REG(0)/CMI_PS_REG_HOME (1)/CMI_PS_NOT_REG_SEARCHING(2)/CMI_PS_REG_DENIED(3)
               \CMI_PS_REG_UNKNOWN(4)/ CMI_PS_REG_ROAMING(5)/  CMI_PS_REG_SMS_ONLY_HOME(6) (not applicable)
               \CMI_PS_REG_SMS_ONLY_ROAMING(7)(not applicable)/CMI_PS_REG_EMERGENCY(8)
               \CMI_PS_REG_CSFB_NOT_PREFER_HOME(9)(not applicable)/CMI_PS_REG_CSFB_NOT_PREFER_ROAMING(10)(not applicable)
  \returns     CmsRetId
*/
CmsRetId appGetCeregStateSync(UINT8 *state)
{
    CmsRetId ret = CMS_RET_SUCC;

    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct ceregApiMsg msg = {0};
    msg.sem = &sem;
    msg.state = state;


    OsaCheck((sem != NULL)&&(state != NULL), state, 0, 0);

    appCallbackWithNoBlock((app_callback_fn)psGetERegStatusSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }

    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appGetTAUInfoSync(UINT32 *tauTime, UINT32 *activeTime)
  \brief       Send cmi request to get TAU information
  \param[out]  *tauTimeS     Pointer to stored result TAU time(unit:S)---related to T3412
  \param[out]  *activeTimeS  Pointer to stored result active time(unit:S)---related to T3324
  \returns     CmsRetId
*/
CmsRetId appGetTAUInfoSync(UINT32 *tauTimeS, UINT32 *activeTimeS)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT16 tac = 0;
    UINT32 cellId = 0;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct ceregApiMsg msg = {0};
    msg.sem = &sem;
    msg.tauTimeS = tauTimeS;
    msg.activeTimeS = activeTimeS;
    msg.tac = &tac;
    msg.cellId = &cellId;

    OsaCheck((sem != NULL)&&(tauTimeS != NULL)&&(activeTimeS != NULL), tauTimeS, activeTimeS, 0);

    appCallbackWithNoBlock((app_callback_fn)psGetERegStatusSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }

    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appGetLocationInfoSync(UINT16 *tac, UINT32 *cellId)
  \brief       Send cmi request to get location setting information through CEREG command
  \param[out]  *tac         Pointer to stored tac value(UINT16)
  \param[out]  *cellId      Pointer to stored result cell ID(UINT32)
  \returns     CmsRetId
*/
CmsRetId appGetLocationInfoSync(UINT16 *tac, UINT32 *cellId)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    UINT32 tauTime = 0, activeTime = 0;
    struct ceregApiMsg msg = {0};

    msg.sem = &sem;
    msg.tac = tac;
    msg.cellId = cellId;
    msg.tauTimeS = &tauTime;
    msg.activeTimeS = &activeTime;

    OsaCheck((tac != NULL)&&(cellId != NULL), tac, cellId, 0);

    appCallbackWithNoBlock((app_callback_fn)psGetERegStatusSync, &msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appSetPSMSettingSync(UINT8 psmMode, UINT32 tauTime, UINT32 activeTime)
  \brief       Send cmi request to set PSM setting information
  \param[out]  psmMode     Value to input psm mode--psmmode with the Enum "CmiMmPsmReqModeEnum"
               \CMI_MM_DISABLE_PSM(0)/CMI_MM_ENABLE_PSM(1)/CMI_MM_DISCARD_PSM(2)
  \param[out]  tauTimeS     Value to input TAU time(unit: S)---related to T3412
  \param[out]  activeTimeS  Value to input active time(unit: S)---related to T3324
  \returns     CmsRetId
*/
CmsRetId appSetPSMSettingSync(UINT8 psmMode, UINT32 tauTimeS, UINT32 activeTimeS)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct psmApiMsg msg = {0};
    UINT8 tempPsmMode = psmMode;
    UINT32 tempTauTimeS = tauTimeS;
    UINT32 tempActiveTimeS = activeTimeS;

    msg.sem = &sem;
    msg.psmMode = &tempPsmMode;
    msg.tauTimeS = &tempTauTimeS;
    msg.activeTimeS = &tempActiveTimeS;

    OsaCheck((sem != NULL)&&(psmMode <= CMI_MM_DISCARD_PSM)&&(tauTimeS <= 320*60*60*31)&&(activeTimeS <= 6*60*31),
             psmMode, tauTimeS, activeTimeS);

    appCallbackWithNoBlock((app_callback_fn)mmSetCpsmsSync, &msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appGetEDRXSettingSync(UINT8 *actType, UINT32 *nwEdrxValueMs, UINT32 *nwPtwMs)
  \brief       Send cmi request to get EDRX setting information
  \param[out]  *actType     Pointer to stored result act type--refer Enum "CmiMmEdrxActTypeEnum"
               CMI_MM_EDRX_NO_ACT_OR_NOT_USE_EDRX(0)/  CMI_MM_EDRX_EC_GSM_IOT(1)(useless)
               CMI_MM_EDRX_GSM(2)(useless)/CMI_MM_EDRX_UMTS(3)(useless)/CMI_MM_EDRX_LTE(4)(useless)
               CMI_MM_EDRX_NB_IOT(5)
  \param[out]  *nwEdrxValueMs     Pointer to stored result nw edrx value(ms unit)
  \param[out]  *nwPtwMs  Pointer to stored result nw paging time window
  \returns     CmsRetId
*/
CmsRetId appGetEDRXSettingSync(UINT8 *actType, UINT32 *nwEdrxValueMs, UINT32 *nwPtwMs)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct edrxApiMsg msg = {0};
    msg.sem = &sem;
    msg.actType = actType;
    msg.edrxValueMs = nwEdrxValueMs;
    msg.share.nwPtwMs = nwPtwMs;

    OsaCheck((sem != NULL)&&(actType != NULL)&&(nwEdrxValueMs != NULL)&&(nwPtwMs != NULL), actType, nwEdrxValueMs, nwPtwMs);
    appCallbackWithNoBlock((app_callback_fn)mmReadEdrxDynSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appSetEDRXSettingSync(UINT8 modeVal, UINT8 actType, UINT32 reqEdrxValueMs)
  \brief       Send cmi request to set EDRX setting
  \param[out]  modeVal     Value to input mode--refer to #CmiMmEdrxModeEnum
                           \CMI_MM_DISABLE_EDRX(0)/CMI_MM_ENABLE_EDRX_AND_DISABLE_IND(1)
                           \CMI_MM_ENABLE_EDRX_AND_ENABLE_IND(2)/CMI_MM_DISCARD_EDRX(3)
  \param[out]  actType     Value to input act Type---refer to #CmiMmEdrxActTypeEnum
                           \CMI_MM_EDRX_NO_ACT_OR_NOT_USE_EDRX(0)/  CMI_MM_EDRX_EC_GSM_IOT(1)(useless)
                           \CMI_MM_EDRX_GSM(2)(useless)/CMI_MM_EDRX_UMTS(3)(useless)
                           \CMI_MM_EDRX_LTE(4)(useless)/CMI_MM_EDRX_NB_IOT(5)
  \param[out]  reqEdrxValueMs  Value to input request EDRX value---Unit:MS
  \returns     CmsRetId
*/
CmsRetId appSetEDRXSettingSync(UINT8 modeVal, UINT8 actType, UINT32 reqEdrxValueMs)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct edrxApiMsg msg = {0};
    UINT8 tempModeVal = modeVal;
    UINT8 tempActType = actType;
    UINT32 tempReqEdrxValueMs = reqEdrxValueMs;

    msg.sem = &sem;
    msg.actType = &tempActType;
    msg.share.modeVal = &tempModeVal;

    if (tempReqEdrxValueMs != 0)
    {
        msg.edrxValueMs = &tempReqEdrxValueMs;
    }
    else
    {
        msg.edrxValueMs = PNULL;
    }
    /*
     * <Requested_eDRX_value>: string type; half a byte in a 4 bit format.
     *  The eDRX value refers to bit 4 to 1 of octet 3 of the Extended DRX parameters information element
     *  for NB, available value: 0x01 - 0x0F
     * S1 mode
     *  The field contains the eDRX value for S1 mode. The E-UTRAN eDRX cycle length
     *  duration value and the eDRX cycle parameter 'TeDRX' as defined in
     *  3GPP TS 36.304 [121] are derived from the eDRX value as follows:
     * bit
     * 4 3 2 1      E-UTRAN eDRX cycle length
     * 0 0 0 0      5,12 seconds (NOTE 4)
     * 0 0 0 1      10,24 seconds (NOTE 4)
     * 0 0 1 0      20,48 seconds
     * 0 0 1 1      40,96 seconds
     * 0 1 0 0      61,44 seconds (NOTE 5)
     * 0 1 0 1      81,92 seconds
     * 0 1 1 0      102,4 seconds (NOTE 5)
     * 0 1 1 1      122,88 seconds (NOTE 5)
     * 1 0 0 0      143,36 seconds (NOTE 5)
     * 1 0 0 1      163,84 seconds
     * 1 0 1 0      327,68 seconds
     * 1 0 1 1      655,36 seconds
     * 1 1 0 0      1310,72 seconds
     * 1 1 0 1      2621,44 seconds
     * 1 1 1 0      5242,88 seconds (NOTE 6)
     * 1 1 1 1      10485,76 seconds (NOTE 6)
     * All other values shall be interpreted as 0000 by this version of the protocol.
     * NOTE 4: The value is applicable only in WB-S1 mode. If received in NB-S1 mode it is
     *         interpreted as if the Extended DRX parameters IE were not included in the
     *         message by this version of the protocol.
     * NOTE 5: The value is applicable only in WB-S1 mode. If received in NB-S1 mode it is
     *         interpreted as 0010 by this version of the protocol.
     * NOTE 6: The value is applicable only in NB-S1 mode. If received in WB-S1 mode it is
     *         interpreted as 1101 by this version of the protocol */
    if (modeVal == CMI_MM_DISABLE_EDRX || modeVal == CMI_MM_ENABLE_EDRX_AND_ENABLE_IND || modeVal == CMI_MM_ENABLE_EDRX_AND_DISABLE_IND)
    {
        OsaCheck((reqEdrxValueMs >= 5120), reqEdrxValueMs, 0, 0);
    }

    OsaCheck((sem != NULL) && (modeVal <= CMI_MM_DISCARD_EDRX) && (actType == CMI_MM_EDRX_NB_IOT), modeVal, actType, reqEdrxValueMs);
    appCallbackWithNoBlock((app_callback_fn)mmSetEdrxSync, &msg);

    if (osSemaphoreAcquire(sem,  portMAX_DELAY) == osOK)
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          appSetEDRXPtwSync
  \brief       Send cmi request to set paging time window
  \param[in]   reqPtwValue      PTW value encoded in 24.008
  \returns     CmsRetId
  \NOTE:
*/
/*
 * reqPtwValue:
 * NB-S1 mode
 * The field contains the PTW value in seconds for NB-S1 mode.The PTW value is used
 * as specified in 3GPP TS 23.682 [133a].The PTW value is derived as follows:
 * bit
 *          Paging Time Window length
 * 0 0 0 0  2,56 seconds
 * 0 0 0 1  5,12 seconds
 * 0 0 1 0  7,68 seconds
 * 0 0 1 1  10,24 seconds
 * 0 1 0 0  12,8 seconds
 * 0 1 0 1  15,36 seconds
 * 0 1 1 0  17,92 seconds
 * 0 1 1 1  20,48 seconds
 * 1 0 0 0  23,04 seconds
 * 1 0 0 1  25,6 seconds
 * 1 0 1 0  28,16 seconds
 * 1 0 1 1  30,72 seconds
 * 1 1 0 0  33,28 seconds
 * 1 1 0 1  35,84 seconds
 * 1 1 1 0  38,4 seconds
 * 1 1 1 1  40,96 seconds
*/
CmsRetId appSetEDRXPtwSync(UINT8 reqPtwValue)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiMmSetPtwEdrxParmReq  cmiReq;
    CmiMmSetEdrxParmCnf     cmiCnf;

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiMmSetPtwEdrxParmReq));
    memset(&cmiCnf, 0x00, sizeof(CmiMmSetEdrxParmCnf));

    cmiReq.edrxMode = CMI_MM_ENABLE_EDRX_AND_DISABLE_IND;
    cmiReq.actType = 5; /* 5 is NB-IOT */
    cmiReq.ptwValuePresent = TRUE;
    cmiReq.reqPtwValue = reqPtwValue;

    psCmiReqData.sgId        = CAC_MM;
    psCmiReqData.reqPrimId   = CMI_MM_SET_REQUESTED_PTW_EDRX_PARM_REQ;
    psCmiReqData.cnfPrimId   = CMI_MM_SET_REQUESTED_PTW_EDRX_PARM_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC || psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}

/**
  \fn          CmsRetId appGetPtwSettingSync(uint8_t *pMode)
  \brief       Send cmi request to get current PTW setting
  \param[out]  *pPtw     Pointer to store the result of paging time window
  \returns     CmsRetId
*/
CmsRetId appGetPtwSettingSync(UINT8 *pPtw)
{
    /*
     * Return value:
     * NB-S1 mode
     * The field contains the PTW value in seconds for NB-S1 mode.The PTW value is used
     * as specified in 3GPP TS 23.682 [133a].The PTW value is derived as follows:
     * bit      Paging Time Window length
     * 0 0 0 0  2,56 seconds
     * 0 0 0 1  5,12 seconds
     * 0 0 1 0  7,68 seconds
     * 0 0 1 1  10,24 seconds
     * 0 1 0 0  12,8 seconds
     * 0 1 0 1  15,36 seconds
     * 0 1 1 0  17,92 seconds
     * 0 1 1 1  20,48 seconds
     * 1 0 0 0  23,04 seconds
     * 1 0 0 1  25,6 seconds
     * 1 0 1 0  28,16 seconds
     * 1 0 1 1  30,72 seconds
     * 1 1 0 0  33,28 seconds
     * 1 1 0 1  35,84 seconds
     * 1 1 1 0  38,4 seconds
     * 1 1 1 1  40,96 seconds
    */

    CmsRetId                            cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData                     psCmiReqData;
    CmiMmGetRequestedPtwEdrxParmReq     cmiReq;
    CmiMmGetRequestedEdrxParmCnf        cmiCnf;

    /* check the input */
    OsaCheck((pPtw != NULL), pPtw, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiMmGetRequestedPtwEdrxParmReq));
    memset(&cmiCnf, 0x00, sizeof(CmiMmGetRequestedEdrxParmCnf));

    psCmiReqData.sgId       = CAC_MM;
    psCmiReqData.reqPrimId  = CMI_MM_GET_REQUESTED_PTW_EDRX_PARM_REQ;
    psCmiReqData.cnfPrimId  = CMI_MM_GET_REQUESTED_PTW_EDRX_PARM_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam  = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen  = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf    = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        *pPtw = cmiCnf.reqPtwValue;
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
        *pPtw = 0xff;
    }
    return cmsRet;
}



/**
  \fn          UINT8 appGetSearchPowerLevelSync(void)
  \brief       Send cmi request to get PLMN power level information
  \returns     UINT8 range{0,3} and if error, return 0xff
*/
UINT8 appGetSearchPowerLevelSync(void)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct extCfgApiMsg msg = {0};
    msg.sem = &sem;

    appCallbackWithNoBlock((app_callback_fn)devGetExtCfgSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }

    osSemaphoreDelete(sem);
    if (ret == NBStatusSuccess)
    {
        return msg.powerlevel;
    }
    else
    {
        return 0xFF;
    }
}

/**
  \fn          CmsRetId appCheckSystemTimeSync(void)
  \brief       check if have get NITZ time
  \returns     NBStatusSuccess---success else return CMS_FAIL
*/
CmsRetId appCheckSystemTimeSync(void)
{
    extern UINT32 mwGetTimeSyncFlag(void);
    UINT32 syncFlag = mwGetTimeSyncFlag();

    if (syncFlag == FALSE)
    {
        return CMS_FAIL;
    }
    else
    {
        return CMS_RET_SUCC;
    }
}

/**
  \fn          CmsRetId appGetSystemTimeSync(time_t *time)
  \brief       Get system time, the defaut time is 2000.1.1 00:00:00,
               and when the NITZ time is got, the time will be updated
  \returns     CmsRetId
*/
CmsRetId appGetSystemTimeSecsSync(time_t *time)
{
    extern time_t OsaSystemTimeReadSecs(void);
    time_t temp = OsaSystemTimeReadSecs();

    if (temp == 0)
    {
        return CMS_FAIL;
    }
    else
    {
        *time = temp;
        return CMS_FAIL;
    }
}

/**
  \fn          CmsRetId appGetSystemTimeUtcSync(time_t *time)
  \brief       Get system time, the format is blow. the defaut time is 2000.1.1 00:00:00,
               and when the NITZ time is got, the time will be updated
               typedef __PACKED_STRUCT _utc_timer_value
               {
                   // UTCtimer1   ---UINT16 year--UINT8 mon--UINT8 day           //
                   // UTCtimer2   ---UINT8 hour--UINT8 mins--UINT8 sec--INT8 tz  //
                   unsigned int UTCtimer1;
                   unsigned int UTCtimer2;
                   unsigned int UTCsecs;   //secs since 1970//
                   unsigned int UTCms;    //current ms //
                   unsigned int timeZone;
               } OsaUtcTimeTValue;

  \returns     CmsRetId
*/
CmsRetId appGetSystemTimeUtcSync(OsaUtcTimeTValue *time)
{
    extern OsaUtcTimeTValue *OsaSystemTimeReadUtc(void);
    OsaUtcTimeTValue *temp = OsaSystemTimeReadUtc();

    if (temp == NULL)
    {
        return CMS_FAIL;
    }
    else
    {
        *time = *temp;
        return CMS_RET_SUCC;
    }
}

/**
  \fn          CmsRetId appSetSystemTimeUtcSync(UINT32 time1, UINT32 time2)
  \brief       Set system time, the time must since 2000.1.1, and the format is
               timer1     ---UINT16 year--UINT8 mon--UINT8 day
               timer2     ---UINT8 hour--UINT8 mins--UINT8 sec--INT8 tz
  \returns     CmsRetId
*/
CmsRetId appSetSystemTimeUtcSync(UINT32 time1, UINT32 time2, INT32 timeZone)
{
    extern  INT32 OsaTimerSync(UINT32 srcType, UINT32 cmd, UINT32 Timer1, UINT32 Timer2, INT32 timeZone);
    UINT32 ret = OsaTimerSync(APP_TIME_SRC, SET_LOCAL_TIME, time1, time2, timeZone);

    if (ret != 0)
    {
        return CMS_FAIL;
    }
    else
    {
        if (mwGetTimeSyncFlag()==FALSE)
        {
            mwSetTimeSyncFlag(TRUE);
        }
        return CMS_RET_SUCC;
    }
}

/**
  \fn          CmsRetId appGetAPNSettingSync(UINT8 cid, UINT8 *apn)
  \brief       Get APN setting
  \param[in]   cid     Parameter to indicate which cid's info is getting(0~10)
  \param[out]  *apn    Pointer to stored result apn string (max length: PS_APN_MAX_SIZE )
  \returns     CmsRetId
*/
CmsRetId appGetAPNSettingSync(UINT8 cid, UINT8 *apn)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct cgcontrdpApiMsg msg = {0};
    msg.sem = &sem;
    msg.index = cid;
    msg.APN= apn;

    OsaCheck((sem != NULL)&&(apn != NULL), sem, apn, 0);

    appCallbackWithNoBlock((app_callback_fn)psGetCGCONTRDPParamSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;

}

/**
  \fn          UINT8 appGetCELevelSync(void)
  \brief       Get NB coverage class level
  \returns     UINT8 CELEVEL with range{0,5}
*/
UINT8 appGetCELevelSync(void)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct celevelApiMsg msg = {0};
    msg.sem = &sem;

    OsaCheck((sem != NULL), sem, 0, 0);

    appCallbackWithNoBlock((app_callback_fn)mmGetCoverEnhancStatuSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    if ((msg.act == 1) || (msg.act == 3))
    {
        return msg.celevel;
    }
    else if (msg.act == 2)
    {
        return msg.cc;
    }
    else
    {
        return ret;
    }

}

/**
  \fn          CmsRetId appGetSignalInfoSync(UINT8 *csq, INT8 *snr, INT8 *rsrp)
  \brief       Get NB signal information
  \param[out]   *csq Pointer to signal info csq
                * CSQ mapping with RSSI
                *<rssi>: integer type
                * 0        -113 dBm or less
                * 1        -111 dBm
                * 2...30   -109... -53 dBm
                * 31       -51 dBm or greater
                * 99       not known or not detectable
  \param[out]   *snr Pointer to signal info snr(value in dB, value range: -30 ~ 30);
  \param[out]   *rsrp Pointer to signal info rsrp(value range: -17 ~ 97, 127);
                * 1> AS NB extended the RSRP value in: TS 36.133-v14.5.0, Table 9.1.4-1
                *   -17 rsrp < -156 dBm
                *   -16 -156 dBm <= rsrp < -155 dBm
                *    ...
                *   -3 -143 dBm <= rsrp < -142 dBm
                *   -2 -142 dBm <= rsrp < -141 dBm
                *   -1 -141 dBm <= rsrp < -140 dBm
                *   0 rsrp < -140 dBm
                *   1 -140 dBm <= rsrp < -139 dBm
                *   2 -139 dBm <= rsrp < -138 dBm
                *   ...
                *   95 -46 dBm <= rsrp < -45 dBm
                *   96 -45 dBm <= rsrp < -44 dBm
                *   97 -44 dBm <= rsrp
                * 2> If not valid, set to 127
  \returns     CmsRetId
*/
CmsRetId appGetSignalInfoSync(UINT8 *csq, INT8 *snr, INT8 *rsrp)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct signalApiMsg msg = {0};
    msg.sem = &sem;
    msg.csq = csq;
    msg.snr = snr;
    msg.rsrp = rsrp;

    OsaCheck((sem != NULL), sem, 0, 0);
    appCallbackWithNoBlock((app_callback_fn)mmGetSignalInfoSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CHAR appGetNBVersionInfo(void)
  \brief       Get SDK software version info
  \returns     CHAR
*/
CHAR* appGetNBVersionInfo(void)
{
    return (CHAR*)getVersionInfo();
}

#if 0
/**
  \fn          CmsRetId appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *result )
  \brief       Send request to tcpip to get netinfo.
  \param[in]   cid     Context ID(value range:1~15) which is related with PDN
  \param[out]  result  Pointer to stored result with stucture NmAtiSyncRet
  \returns     CmsRetId
*/
CmsRetId appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *result)
{
    CmsRetId ret = CMS_RET_SUCC;
    NmAtiGetNetInfoReq req = {0};
    req.cid = cid;
    if (NetmgrAtiSyncReq(NM_ATI_SYNC_GET_NET_INFO_REQ, (void *)&req, result) != 0)
        ret = CMS_FAIL;
    return ret;
}
psGetNetInfoSynCallback

#endif

/**
  \fn          CmsRetId appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *pNmNetInfo)
  \brief       Send request to tcpip to get netinfo.
  \param[in]   cid          Context ID(value range:0-10) which is related with PDN, if need to get default PDP info, set to 0
  \param[out]  pNmNetInfo   Pointer to stored result with stucture NmAtiSyncRet
  \returns     CmsRetId
*/
CmsRetId appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *pNmNetInfo)
{
    OsaDebugBegin(cid < 11 && pNmNetInfo != PNULL, cid, pNmNetInfo, 0);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    /*
     * Here, let call "psGetNetInfoSynCallback" which should be processed in CMS task. to solve:
     * When wake up from deep sleep, before netmgr recovery done, netmgr API: NetmgrAtiSyncReq() maybe return not right info.
     * so here, let CMS task call "NetmgrAtiSyncReq()" after netmger recovery done
    */
    return cmsSynApiCall(psGetNetInfoSynCallback, 4, &cid, sizeof(NmAtiSyncRet), pNmNetInfo);
}


/**
  \fn          void drvSetPSToWakeup(void)
  \brief       Send request to wake up PS, only called in IDLE task
  \returns     void
*/
void drvSetPSToWakeup(void)
{
    appCallbackWithNoBlock((app_callback_fn)cmsWakeupPs, PNULL);

    return;
}

/**
  \fn          CmsRetId appSetCFUN(UINT8 fun)
  \brief       Send cfun request to NB
  \param[in]   fun, refer "CmiFuncValueEnum" CMI_DEV_MIN_FUNC(0)/CMI_DEV_FULL_FUNC(1)/CMI_DEV_TURN_OFF_RF_FUNC(4)
  \returns     CmsRetId
*/
CmsRetId appSetCFUN(UINT8 fun)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct cfunApiMsg msg = {0};
    msg.sem = &sem;
    msg.fun = fun;

    OsaCheck((sem != NULL), sem, 0, 0);

    appCallbackWithNoBlock((app_callback_fn)devSetFuncSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;

}

/**
  \fn          CmsRetId appGetCFUN(UINT8 *pOutCfun)
  \brief       Get current CFUN state
  \param[out]  *pOutCfun      //refer "CmiFuncValueEnum" CMI_DEV_MIN_FUNC(0)/CMI_DEV_FULL_FUNC(1)/CMI_DEV_TURN_OFF_RF_FUNC(4)
  \returns     CmsRetId
*/
CmsRetId appGetCFUN(UINT8 *pOutCfun)
{
    CmsRetId    cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData     psCmiReqData;   //20 bytes
    CmiDevGetCfunReq    cmiReq; //4 bytes
    CmiDevGetCfunCnf    cmiCnf; //4 bytes

    /*check the input*/
    OsaCheck(pOutCfun != NULL, pOutCfun, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetCfunCnf));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetCfunReq));

    psCmiReqData.sgId       = CAC_DEV;
    psCmiReqData.reqPrimId  = CMI_DEV_GET_CFUN_REQ;
    psCmiReqData.cnfPrimId  = CMI_DEV_GET_CFUN_CNF;
    psCmiReqData.reqParamLen    = sizeof(cmiReq);
    psCmiReqData.pReqParam  = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen  = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf    = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        *pOutCfun   = cmiCnf.func;
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }

        *pOutCfun = 0;
    }

    return cmsRet;
}



/**
  \fn          CmsRetId appSetBootCFUNMode(UINT8 mode)
  \brief       Set CFUN mode when boot
  \param[in]   mode, refer "CmiFuncValueEnum" CMI_DEV_MIN_FUNC(0)/CMI_DEV_FULL_FUNC(1)/CMI_DEV_TURN_OFF_RF_FUNC(4)
  \returns     CmsRetId
*/
CmsRetId appSetBootCFUNMode(UINT8 mode)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct extCfgApiMsg msg = {0};
    msg.sem = &sem;
    msg.powerCfun = mode;

    OsaCheck((sem != NULL), sem, 0, 0);

    appCallbackWithNoBlock((app_callback_fn)devSetExtCfgSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;

}

/**
  \fn          UINT8 appGetBootCFUNMode(UINT8 *mode)
  \brief       Get CFUN mode when boot
  \returns     refer "CmiFuncValueEnum" CMI_DEV_MIN_FUNC(0)/CMI_DEV_FULL_FUNC(1)/CMI_DEV_TURN_OFF_RF_FUNC(4)
*/
UINT8 appGetBootCFUNMode(void)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct extCfgApiMsg msg = {0};
    msg.sem = &sem;

    appCallbackWithNoBlock((app_callback_fn)devGetExtCfgSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }

    osSemaphoreDelete(sem);
    if (ret == CMS_RET_SUCC)
    {
        return msg.powerCfun;
    }
    else
    {
        return 0xFF;
    }

}

/**
  \fn          appSetSimLogicalChannelOpenSync
  \brief       Send cmi request to open SIM logical channel
  \param[out]  *sessionID: Pointer to a new logical channel number returned by SIM
  \param[in]   *dfName: Pointer to DFname(max length "CMI_SIM_MAX_AID_LEN(16 bytes)") selected on the new logical channel
  \returns     CmsRetId
*/
CmsRetId appSetSimLogicalChannelOpenSync(UINT8 *sessionID, UINT8 *dfName)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct cchoApiMsg msg = {0};

    OsaCheck((sessionID != NULL)&&(sem != NULL) && (dfName != PNULL), sessionID, sem, dfName);

    msg.sem = &sem;
    msg.sessionID = sessionID;
    msg.dfName = dfName;
    msg.result = 0;

    appCallbackWithNoBlock((app_callback_fn)simSetCchoSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    return ret;
}

/**
  \fn          appSetSimLogicalChannelCloseSync
  \brief       Send cmi request to close SIM logical channel
  \param[in]   sessionID: the logical channel number to be closed
  \returns     CmsRetId
*/
CmsRetId appSetSimLogicalChannelCloseSync(UINT8 sessionID)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct cchcApiMsg msg = {0};

    OsaCheck((sem != NULL), sem, 0, 0);

    msg.sem = &sem;
    msg.sessionID = sessionID;
    msg.result = 0;

    appCallbackWithNoBlock((app_callback_fn)simSetCchcSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    return ret;
}

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
                                                UINT8 *response, UINT16 *respLen)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct cglaApiMsg msg = {0};

    OsaCheck((sem != NULL) && (command != NULL) && (cmdLen != 0), sem, command, cmdLen);
    OsaCheck((response != NULL) && (respLen != PNULL), response, respLen, 0);

    msg.sem = &sem;
    msg.sessionID = sessionID;
    msg.command = command;
    msg.cmdLen = cmdLen;
    msg.response = response;
    msg.respLen = respLen;
    msg.result = 0;

    appCallbackWithNoBlock((app_callback_fn)simSetCglaSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    return ret;
}

/**
  \fn          appSetRestrictedSimAccessSync
  \brief       Send cmi request to get generic SIM access
  \param[in]   *pCmdParam: Pointer to command(with max length: CMI_SIM_MAX_CMD_APDU_LEN(256 bytes)) parameters
  \param[out]  *pRspParam: Pointer to response(with max length: CMI_SIM_MAX_CMD_DATA_LEN(264 bytes)) parameters
  \returns     CmsRetId
*/
CmsRetId appSetRestrictedSimAccessSync(CrsmCmdParam *pCmdParam, CrsmRspParam *pRspParam)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct crsmApiMsg msg = {0};

    OsaCheck((sem != NULL) && (pCmdParam != NULL) && (pRspParam != 0), sem, pCmdParam, pRspParam);

    msg.sem = &sem;
    msg.cmdParam = pCmdParam;
    msg.responseParam = pRspParam;
    msg.result = 0;

    appCallbackWithNoBlock((app_callback_fn)simSetCrsmSync, (void *)&msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);

    return ret;
}

/**
  \fn          CmsRetId appSetBandModeSync(UINT8 networkMode, UINT8 bandNum, UINT8 *orderBand)
  \brief       Send cmi request to set band mode parameter
  \param[in]   UINT8 networkMode with enum CMIDEVNWMODE, current only CMI_DEV_NB_MODE(0)
  \param[in]   UINT8 bandNum, max value is CMI_DEV_SUPPORT_MAX_BAND_NUM(32)
  \param[in]   *orderBand point to orderBand(max length CMI_DEV_SUPPORT_MAX_BAND_NUM(32) bytes)
  \returns     CmsRetId
*/
CmsRetId appSetBandModeSync(UINT8 networkMode, UINT8 bandNum, UINT8 *orderBand)
{
    CmsRetId ret = CMS_RET_SUCC;
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, NULL);
    struct ecbandApiMsg msg;

    memset(&msg,0,sizeof(msg));

    msg.sem = &sem;
    msg.networkMode = networkMode;
    msg.bandNum = bandNum;
    msg.orderBand = orderBand;

    OsaCheck((sem != NULL)&&(networkMode == 0), sem, networkMode, 0);

    appCallbackWithNoBlock((app_callback_fn)devSetCiotBandModeReqSync, &msg);

    if ( osSemaphoreAcquire( sem,  portMAX_DELAY) == osOK )
    {
        ret = psSyncProcErrCode(msg.result);
    }
    osSemaphoreDelete(sem);
    return ret;
}

/**
  \fn          CmsRetId appGetBandModeSync(INT8 *networkMode, UINT8 *bandNum, UINT8 *orderBand)
  \brief       Send cmi request to get band mode parameter
  \param[out]  *networkMode point to stored result with enum CMIDEVNWMODE, current only CMI_DEV_NB_MODE(0)
  \param[out]  *bandNum, point to the stored result with max value is CMI_DEV_SUPPORT_MAX_BAND_NUM(32)
  \param[out]  *orderBand point to the stored result orderBand(max length CMI_DEV_SUPPORT_MAX_BAND_NUM(32) bytes)
  \returns     CmsRetId
*/
CmsRetId appGetBandModeSync(UINT8 *networkMode, UINT8 *bandNum, UINT8 *orderBand)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;   //20 bytes
    CmiDevGetCiotBandReq    cmiReq;
    CmiDevGetCiotBandCnf    cmiCnf;

    /*check the input*/
    OsaCheck((networkMode != NULL) && (bandNum != NULL) && (orderBand != NULL),
             networkMode, bandNum, orderBand);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetCiotBandReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetCiotBandCnf));

    psCmiReqData.sgId       = CAC_DEV;
    psCmiReqData.reqPrimId  = CMI_DEV_GET_CIOT_BAND_REQ;
    psCmiReqData.cnfPrimId  = CMI_DEV_GET_CIOT_BAND_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam  = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if ((cmsRet == CMS_RET_SUCC) && (psCmiReqData.cnfRc == CME_SUCC))
    {
        *networkMode = cmiCnf.nwMode;
        *bandNum = cmiCnf.bandNum;
        memcpy(orderBand, cmiCnf.orderedBand, CMI_DEV_SUPPORT_MAX_BAND_NUM);
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }

        *bandNum = 0;
    }

    return cmsRet;
}

/**
  \fn          CmsRetId appGetSupportedBandModeSync(INT8 *networkMode, UINT8 *bandNum, UINT8 *orderBand)
  \brief       Send cmi request to get supported band mode parameter
  \param[out]  *networkMode point to stored result with enum CMIDEVNWMODE, current only CMI_DEV_NB_MODE(0)
  \param[out]  *bandNum, point to the stored result with max value is CMI_DEV_SUPPORT_MAX_BAND_NUM(32)
  \param[out]  *orderBand point to the stored result orderBand(max length CMI_DEV_SUPPORT_MAX_BAND_NUM(32) bytes)
  \returns     CmsRetId
*/
CmsRetId appGetSupportedBandModeSync(INT8 *networkMode, UINT8 *bandNum, UINT8 *orderBand)
{
    CmsRetId                    cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData             psCmiReqData;   //20 bytes
    CmiDevGetCiotBandCapaReq    cmiReq;
    CmiDevGetCiotBandCapaCnf    cmiCnf;

    /*check the input*/
    OsaCheck((networkMode != NULL) && (bandNum != NULL) && (orderBand != NULL),
             networkMode, bandNum, orderBand);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetCiotBandCapaReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetCiotBandCapaCnf));

    psCmiReqData.sgId       = CAC_DEV;
    psCmiReqData.reqPrimId  = CMI_DEV_GET_CIOT_BAND_CAPA_REQ;
    psCmiReqData.cnfPrimId  = CMI_DEV_GET_CIOT_BAND_CAPA_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam  = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen  = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf    = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if ((cmsRet == CMS_RET_SUCC) && (psCmiReqData.cnfRc == CME_SUCC))
    {
        *networkMode = cmiCnf.nwMode;
        *bandNum = cmiCnf.bandNum;
        memcpy(orderBand, cmiCnf.supportBand, CMI_DEV_SUPPORT_MAX_BAND_NUM);
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }

        *bandNum = 0;
    }

    return cmsRet;
}


/**
  \fn          CmsRetId appGetImeiNumSync(CHAR* imei)
  \brief       Send cmi request to get IMEI
  \param[out]  CHAR* imei point to the stored result
  \returns     CmsRetId
*/
CmsRetId appGetImeiNumSync(CHAR* imei)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |       16byte       |                  | */
    // Get it from flash first then use default
    CmsRetId ret = CMS_RET_SUCC;
    UINT8 imeiBuf[18] = {0};
    UINT8 imeiFlag = 1;

    nvram_read(NV9, imeiBuf, 15, 0);

    for (INT32 i=0; i<15; i++)
    {
        // Check if is num
        if((imeiBuf[i] < 0x30) || (imeiBuf[i] > 0x39))
        {
            imeiFlag = 0x0;
            break;
        }
    }

    if (imeiFlag)
    {
        memcpy(imei, imeiBuf, 16);
    }
    else
    {
        memcpy(imei, "866818039921444", 16);        //"866818030021444"
    }

    return ret;
}

/**
  \fn          BOOL appSetImeiNumSync(CHAR* imei)
  \brief       Send cmi request to set IMEI
  \param[in]  CHAR* imei point to IMEI
  \returns     true---success
*/
BOOL appSetImeiNumSync(CHAR* imei)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |       16byte       |                  | */

    UINT8 tmpBuf[NV9_DATA_LEN] = {0};
    UINT8 imeiFlag = 1;
    BOOL ret = false;

    memcpy(&tmpBuf[0],imei,16);

    for (int i=0; i<15; i++)
    {
        if ((tmpBuf[i]<0x30) || (tmpBuf[i]>0x39))
        {
            imeiFlag = 0x0;
            break;
        }
    }
    if (imeiFlag)
    {
        nvram_read(NV9, &tmpBuf[NV9_DATA_SN_OFFSET], (NV9_DATA_LEN-NV9_DATA_IMEI_LEN), NV9_DATA_SN_OFFSET); // save sn, imei lock, sn lcokcopy
        nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
        nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
        //sync to default reliable region
        nvram_sav2fac_at(SAVE_OTHER);
        ret = true;
    }

    return ret;
}

/**
  \fn          BOOL appGetImeiLockSync(CHAR* imeiLock)
  \brief       Send cmi request to get IMEI lock
  \param[out]  CHAR* imeiLock point to the stored result
  \returns     true---success
*/
BOOL appGetImeiLockSync(CHAR* imeiLock)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |       16byte       |                  | */

    UINT8 tmpBuf[NV9_DATA_LEN] = {0};

    nvram_read(NV9, &tmpBuf[NV9_DATA_IMEI_OFFSET], NV9_DATA_LEN, NV9_DATA_IMEI_OFFSET); // save imei, imei lock, sn lcokcopy

    memcpy(imeiLock, &tmpBuf[NV9_DATA_IMEI_LOCK_OFFSET], NV9_DATA_IMEI_LOCK_LEN);

    return true;
}

/**
  \fn          BOOL appSetImeiLockSync(CHAR* imeiLock)
  \brief       Send cmi request to set IMEI lock
  \param[in]   CHAR* imeiLock point to imeilock
  \returns     true---success
*/
BOOL appSetImeiLockSync(CHAR* imeiLock)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |       16byte       |                  | */

    UINT8 tmpBuf[NV9_DATA_LEN] = {0};

    nvram_read(NV9, &tmpBuf[NV9_DATA_IMEI_OFFSET], NV9_DATA_LEN, NV9_DATA_IMEI_OFFSET); // save imei, imei lock, sn lcokcopy

    memcpy(&tmpBuf[NV9_DATA_IMEI_LOCK_OFFSET], imeiLock, NV9_DATA_IMEI_LOCK_LEN);
    nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
    nvram_write(NV9, tmpBuf, NV9_DATA_LEN);

    //sync to default reliable region
    nvram_sav2fac_at(SAVE_OTHER);
    return true;
}

/**
  \fn          BOOL appGetSNNumSync(CHAR* sn)
  \brief       Send cmi request to get sn num
  \param[out]  CHAR* sn point to the stored result
  \returns     true---success
*/
BOOL appGetSNNumSync(CHAR* sn)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |       16byte       |                  | */
    UINT8 snBuf[32] = {0};
    UINT8 i;
    UINT8 len;

    nvram_read(NV9, snBuf, 32, 32);
    len = (strlen((char *)snBuf) < 32) ? strlen((char *)snBuf) : 32;

    for (i = 0; i < len; i++)
    {
        if ((snBuf[i] < 0x21) || (snBuf[i] > 0x7e))            // it's a visable text
        {
            break;
        }
    }

    if (i <= (len-1))
    {
        return false;
    }

    memcpy(sn,snBuf,len);

    return true;
}

/**
  \fn          BOOL appSetSNNumSync(CHAR* sn, UINT8 len)
  \brief       Send cmi request to set sn num
  \param[in]   CHAR* sn point to sn num
  \param[in]   len SN num length with max length 32 bytes
  \returns     true---success
*/
BOOL appSetSNNumSync(CHAR* sn, UINT8 len)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |       16byte       |                  | */
    UINT8 tmpBuf[NV9_DATA_LEN] = {0};
    UINT8 snFlag = 1;
    BOOL ret = false;
    UINT8 i;

    for (i=0;i<len;i++)
    {
        if ((sn[i]<0x21) || (sn[i]>0x7e))            // it's a visable text
        {
            snFlag = 0;
            break;
        }
    }

    if (snFlag)
    {
        nvram_read(NV9, tmpBuf, NV9_DATA_LEN, NV9_DATA_IMEI_OFFSET);        // save imei copy

        memcpy(&tmpBuf[NV9_DATA_SN_OFFSET],sn,NV9_DATA_SN_LEN);
        nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
        nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
        //sync to default reliable region
        nvram_sav2fac_at(SAVE_OTHER);
        ret = true;
    }

    return ret;
}

/**
  \fn          BOOL appGetSNLockSync(CHAR* snLock)
  \brief       Send cmi request to get sn lock flag
  \param[out]  CHAR* snLock point to stored result sn lock flag
  \returns     true---success
*/
BOOL appGetSNLockSync(CHAR* snLock)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |      16byte        |                  | */

    UINT8 tmpBuf[NV9_DATA_LEN] = {0};

    nvram_read(NV9, &tmpBuf[NV9_DATA_IMEI_OFFSET], NV9_DATA_LEN, NV9_DATA_IMEI_OFFSET); // save imei, imei lock, sn lcokcopy

    memcpy(snLock, &tmpBuf[NV9_DATA_SN_LOCK_OFFSET], NV9_DATA_SN_LOCK_LEN);

    return true;
}

/**
  \fn          BOOL appSetSNLockSync(CHAR* snLock)
  \brief       Send cmi request to set sn lock flag
  \param[in]   CHAR* snLock point to sn lock flag
  \returns     true---success
*/
BOOL appSetSNLockSync(CHAR* snLock)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |      16byte        |                  | */

    UINT8 tmpBuf[NV9_DATA_LEN] = {0};

    nvram_read(NV9, &tmpBuf[NV9_DATA_IMEI_OFFSET], NV9_DATA_LEN, NV9_DATA_IMEI_OFFSET); // save imei, imei lock, sn lcokcopy

    memcpy(&tmpBuf[NV9_DATA_SN_LOCK_OFFSET], snLock, NV9_DATA_SN_LOCK_LEN);
    nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
    nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
    //sync to default reliable region
    nvram_sav2fac_at(SAVE_OTHER);

    return true;
}

/**
  \fn          BOOL appSetNV9LockCleanSync(void)
  \brief       Send cmi request to do NV9 lock clean
  \param[out]
  \returns     true---success
*/
BOOL appSetNV9LockCleanSync(void)
{
    /*----imei----|--------sn-------|----imei lock flag----|----sn lock flag----|------------------*/
    /*   32byte   |      32byte     |         16byte       |      16byte        |                  | */

    UINT8 tmpBuf[NV9_DATA_LEN] = {0};

    nvram_read(NV9, &tmpBuf[NV9_DATA_IMEI_OFFSET], NV9_DATA_LEN, NV9_DATA_IMEI_OFFSET); // save imei, imei lock, sn lcokcopy

    memset(&tmpBuf[NV9_DATA_IMEI_LOCK_OFFSET], 0, (NV9_DATA_IMEI_LOCK_LEN+NV9_DATA_SN_LOCK_LEN));
    nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
    nvram_write(NV9, tmpBuf, NV9_DATA_LEN);
    //sync to default reliable region
    nvram_sav2fac_at(SAVE_OTHER);

    return true;
}

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
CmsRetId appGetECBCInfoSync(NeighCellReqType reqType, UINT8 reqMaxCellNum, UINT16 maxMeasTimeSec, BasicCellListInfo *pBcListInfo)
{
    CmsRetId    cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData     psCmiReqData;   //20 bytes
    CmiDevGetBasicCellListInfoReq   cmiReq; //4 bytes
    CmiDevGetBasicCellListInfoCnf   cmiCnf; //124 bytes

    /*check the input*/
    OsaDebugBegin(reqType <= GET_NEIGHBER_CELL_ID_INFO &&
                  reqMaxCellNum > 0 && reqMaxCellNum <= PS_LIB_BASIC_CELL_REQ_MAX_CELL_NUM &&
                  maxMeasTimeSec >= 4 && maxMeasTimeSec <= 300 &&
                  pBcListInfo != PNULL,
                  reqType, reqMaxCellNum, maxMeasTimeSec);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetBasicCellListInfoCnf));

    cmiReq.mode             = reqType;
    cmiReq.needSave         = FALSE;
    cmiReq.maxTimeoutS      = maxMeasTimeSec;
    cmiReq.reqMaxCellNum    = reqMaxCellNum;        //1 serving cell + 4 neighber cells
    cmiReq.rptMode          = CMI_DEV_BCINFO_SYN_RPT;
    cmiReq.rsvd             = 0;

    psCmiReqData.sgId       = CAC_DEV;
    psCmiReqData.reqPrimId  = CMI_DEV_GET_BASIC_CELL_LIST_INFO_REQ;
    psCmiReqData.cnfPrimId  = CMI_DEV_GET_BASIC_CELL_LIST_INFO_CNF;
    psCmiReqData.reqParamLen    = sizeof(cmiReq);
    psCmiReqData.pReqParam  = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen  = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf    = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC &&
        psCmiReqData.cnfRc == CME_SUCC)
    {
        /* typedef CmiDevGetBasicCellListInfoCnf BasicCellListInfo */
        memcpy(pBcListInfo, &cmiCnf, sizeof(CmiDevGetBasicCellListInfoCnf));
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }

        pBcListInfo->sCellPresent = FALSE;
        pBcListInfo->nCellNum = 0;
    }

    return cmsRet;
}


/**
  \fn          appSetCiotFreqSync
  \brief       Send cmi request to set prefer EARFCN list, lock or unlock cell
  \param[in]   CiotSetFreqParams *CiotSetFreqParams, the pointer to the CiotSetFreqParams
  \returns     CmsRetId
  \NTOE:       Set EARFCN must be restricted to execute in power off or air plane state.
*/
CmsRetId appSetCiotFreqSync(CiotSetFreqParams *pCiotFreqParams)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiDevSetCiotFreqReq    cmiReq;
    CmiDevSetCiotFreqCnf    cmiCnf;

    /*check the input*/
    OsaCheck((pCiotFreqParams != NULL), pCiotFreqParams, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevSetCiotFreqReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevSetCiotFreqCnf));

    switch (pCiotFreqParams->mode)
    {
        case CMI_DEV_UNLOCK_FREQ_INFO:
        {
            cmiReq.mode = pCiotFreqParams->mode;
            break;
        }

        case CMI_DEV_SET_PREFER_FREQ:
        {
            OsaDebugBegin(pCiotFreqParams->arfcnNum != 0, pCiotFreqParams->arfcnNum, 0, 0);
            return CMS_FAIL;
            OsaDebugEnd();

            cmiReq.mode = pCiotFreqParams->mode;
            cmiReq.arfcnNum = pCiotFreqParams->arfcnNum < CMI_DEV_SUPPORT_MAX_FREQ_NUM ? pCiotFreqParams->arfcnNum : CMI_DEV_SUPPORT_MAX_FREQ_NUM;

            memcpy(cmiReq.arfcnList, pCiotFreqParams->arfcnList, sizeof(INT32)*(cmiReq.arfcnNum));

            break;
        }

        case CMI_DEV_LOCK_CELL:
        {
            OsaDebugBegin(pCiotFreqParams->arfcnNum == 1, pCiotFreqParams->arfcnNum, 0, 0);
            return CMS_FAIL;
            OsaDebugEnd();

            cmiReq.mode = pCiotFreqParams->mode;
            cmiReq.arfcnNum = 1;
            cmiReq.arfcnList[0] = (UINT32)pCiotFreqParams->arfcnList[0];

            if (pCiotFreqParams->phyCellId <= 503)
            {
                cmiReq.cellPresent = TRUE;
                cmiReq.phyCellId = pCiotFreqParams->phyCellId;
            }

            break;
        }

        case CMI_DEV_CLEAR_PREFER_FREQ_LIST:
        {
            cmiReq.mode = pCiotFreqParams->mode;
            break;
        }

        default:
            OsaDebugBegin(FALSE, pCiotFreqParams->mode, pCiotFreqParams->arfcnNum, pCiotFreqParams->phyCellId);
            OsaDebugEnd();
            return CMS_FAIL;
    }

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_SET_CIOT_FREQ_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_SET_CIOT_FREQ_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC ||
        psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}


/**
  \fn          CmsRetId appGetPSMModeSync(uint8_t *pMode)
  \brief       Send cmi request to get current psm mode
  \param[out]  *pMode     Pointer to store the result of psm mode
                1: in psm state
                0: not in psm state(in cfun=0 state *pMode = 0)
  \returns     CmsRetId
*/
CmsRetId appGetPSMModeSync(uint8_t *pMode)
{
    CmsRetId    cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData     psCmiReqData;   //20 bytes
    CmiMmGetPsmModeReq   cmiReq; //4 bytes
    CmiMmGetPsmModeCnf   cmiCnf; //72 bytes

    /*check the input*/
    OsaCheck((pMode != NULL), pMode, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiMmGetPsmModeReq));
    memset(&cmiCnf, 0x00, sizeof(CmiMmGetPsmModeCnf));

    psCmiReqData.sgId       = CAC_MM;
    psCmiReqData.reqPrimId  = CMI_MM_GET_PSM_MODE_REQ;
    psCmiReqData.cnfPrimId  = CMI_MM_GET_PSM_MODE_CNF;
    psCmiReqData.reqParamLen    = sizeof(cmiReq);
    psCmiReqData.pReqParam  = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen  = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf    = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC &&
        psCmiReqData.cnfRc == CME_SUCC)
    {
        *pMode = cmiCnf.psmMode;
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();
        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
        *pMode = 0xff;
    }
    return cmsRet;
}


/**
  \fn          appGetCiotFreqSync
  \brief       Send cmi request to get the current EARFCN setting.
  \param[out]  CiotGetFreqParams *pCiotFreqParams, the pointer to the CiotGetFreqParams
  \returns     CmsRetId
*/
CmsRetId appGetCiotFreqSync(CiotGetFreqParams *pCiotFreqParams)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiDevGetCiotFreqReq    cmiReq;
    CmiDevGetCiotFreqCnf    cmiCnf;
    UINT32                  tmpIdx = 0;

    /*check the input*/
    OsaCheck((pCiotFreqParams != NULL), pCiotFreqParams, 0, 0);
    memset(pCiotFreqParams, 0x00, sizeof(CiotGetFreqParams));

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetCiotFreqReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetCiotFreqCnf));

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_GET_CIOT_FREQ_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_GET_CIOT_FREQ_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        pCiotFreqParams->mode = cmiCnf.mode;

        if (cmiCnf.mode == CMI_DEV_PREFER_FREQ_INFO)
        {
            pCiotFreqParams->arfcnNum = cmiCnf.arfcnNum;

            for (tmpIdx = 0; tmpIdx < cmiCnf.arfcnNum && tmpIdx < CMI_DEV_SUPPORT_MAX_FREQ_NUM; tmpIdx++)
            {
                pCiotFreqParams->arfcnList[tmpIdx] = cmiCnf.arfcnList[tmpIdx];
            }
        }
        else if (cmiCnf.mode == CMI_DEV_CELL_LOCK_INFO)
        {
            if (cmiCnf.cellPresent)
            {
                pCiotFreqParams->cellPresent = TRUE;
                pCiotFreqParams->phyCellId = cmiCnf.phyCellId;
            }

            pCiotFreqParams->lockedArfcn = cmiCnf.lockedArfcn;
        }
        else if (cmiCnf.mode == CMI_DEV_PREFER_FREQ_CELL_LOCK)
        {
            pCiotFreqParams->arfcnNum = cmiCnf.arfcnNum;

            for (tmpIdx = 0; tmpIdx < cmiCnf.arfcnNum && tmpIdx < CMI_DEV_SUPPORT_MAX_FREQ_NUM; tmpIdx++)
            {
                pCiotFreqParams->arfcnList[tmpIdx] = cmiCnf.arfcnList[tmpIdx];
            }

            if (cmiCnf.cellPresent)
            {
                pCiotFreqParams->cellPresent = TRUE;
                pCiotFreqParams->phyCellId = cmiCnf.phyCellId;
            }

            pCiotFreqParams->lockedArfcn = cmiCnf.lockedArfcn;
        }
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}

/**
  \fn          appGetUeExtStatusInfoSync
  \brief       Send cmi request to get the extended status infomation of UE.
  \param[in]   UeExtStatusType statusType, the request type of status information
  \param[out]  UeExtStatusInfo *pStatusInfo, the pointer to the status information returned.
  \returns     CmsRetId
*/
CmsRetId appGetUeExtStatusInfoSync(UeExtStatusType statusType, UeExtStatusInfo *pStatusInfo)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiDevGetExtStatusReq   cmiReq;
    CmiDevGetExtStatusCnf   cmiCnf;

    /*check the input*/
    OsaDebugBegin((statusType <= UE_EXT_STATUS_CCM && pStatusInfo != PNULL), statusType, pStatusInfo, 0);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    memset(pStatusInfo, 0x00, sizeof(UeExtStatusInfo));

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetExtStatusReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetExtStatusCnf));
    cmiReq.getStatusType    = statusType;   //UeExtStatusType = CmiGetExtStatusEnum

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_GET_EXT_STATUS_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_GET_EXT_STATUS_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        if (cmiCnf.getStatusType != statusType)
        {
            OsaDebugBegin(FALSE, cmiCnf.getStatusType, statusType, 0);
            OsaDebugEnd();
            cmsRet = CMS_FAIL;
        }
        else
        {
            /* typedef CmiDevGetExtStatusCnf   UeExtStatusInfo */
            memcpy(pStatusInfo, &cmiCnf, sizeof(CmiDevGetExtStatusCnf));
            #if 0
            pStatusInfo->statusType = cmiCnf.getStatusType;
            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_PHY)
            {
                memcpy(&pStatusInfo->phyStatus, &cmiCnf.phyStatus, sizeof(PhyStatusInfo));
            }

            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_L2)
            {
                memcpy(&pStatusInfo->l2Status, &cmiCnf.l2Status, sizeof(L2StatusInfo));
            }

            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_ERRC)
            {
                memcpy(&pStatusInfo->errcStatus, &cmiCnf.rrcStatus, sizeof(ErrcStatusInfo));
            }

            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_EMM)
            {
                pStatusInfo->emmStatus.emmState = cmiCnf.nasStatus.emmState;
                pStatusInfo->emmStatus.emmMode = cmiCnf.nasStatus.emmMode;
                pStatusInfo->emmStatus.ptwMs = cmiCnf.nasStatus.ptwMs;
                pStatusInfo->emmStatus.eDRXPeriodMs = cmiCnf.nasStatus.eDRXPeriodMs;
                pStatusInfo->emmStatus.psmExT3412TimerS = cmiCnf.nasStatus.psmExT3412TimerS;
                pStatusInfo->emmStatus.T3324TimerS = cmiCnf.nasStatus.T3324TimerS;
                pStatusInfo->emmStatus.T3346RemainTimeS = cmiCnf.nasStatus.T3346RemainTimeS;
            }

            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_PLMN)
            {
                pStatusInfo->plmnStatus.plmnState = cmiCnf.nasStatus.plmnState;
                pStatusInfo->plmnStatus.plmnType = cmiCnf.nasStatus.plmnType;
                pStatusInfo->plmnStatus.selectPlmn.mcc = cmiCnf.nasStatus.selectPlmn.mcc;
                pStatusInfo->plmnStatus.selectPlmn.mncWithAddInfo = cmiCnf.nasStatus.selectPlmn.mncWithAddInfo;
            }

            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_ESM)
            {
                pStatusInfo->esmStatus.actBearerNum = cmiCnf.nasStatus.actBearerNum;
                memcpy(pStatusInfo->esmStatus.apnStr, cmiCnf.nasStatus.apnStr, APN_LEN);
                memcpy(&pStatusInfo->esmStatus.ipv4Addr, &cmiCnf.nasStatus.ipv4Addr, sizeof(IpAddr));
                memcpy(&pStatusInfo->esmStatus.ipv6Addr, &cmiCnf.nasStatus.ipv6Addr, sizeof(IpAddr));
            }

            if (pStatusInfo->statusType == UE_EXT_STATUS_ALL || pStatusInfo->statusType == UE_EXT_STATUS_CCM)
            {
                memcpy(&pStatusInfo->ccmStatus, &cmiCnf.ccmStatus, sizeof(CcmStatusInfo));
            }
            #endif
        }
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}


/**
  \fn          appSetAttachBearerSync
  \brief       Send cmi request to set attach bearer info, such as ip type and APN
  \param[in]   pAttachBearerParams      INPUT, request attach bearer parameter
  \returns     CmsRetId
  \NOTE:       This attach bearer setting take effect in next attach procedure, refer AT: AT+ECATTBEARER
*/
CmsRetId appSetAttachBearerSync(SetAttachBearerParams *pAttachBearerParams)
{
    CmsRetId                        cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData                 psCmiReqData;
    CmiPsSetAttachedBearerCtxReq    cmiReq;
    CmiPsSetAttachedBearerCtxCnf    cmiCnf;

    /* check the input */
    OsaCheck((pAttachBearerParams != NULL), pAttachBearerParams, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiPsSetAttachedBearerCtxReq));
    memset(&cmiCnf, 0x00, sizeof(CmiPsSetAttachedBearerCtxCnf));

    memcpy(&cmiReq, (CmiPsSetAttachedBearerCtxReq *)pAttachBearerParams, sizeof(CmiPsSetAttachedBearerCtxReq));

    psCmiReqData.sgId        = CAC_PS;
    psCmiReqData.reqPrimId   = CMI_PS_SET_ATTACHED_BEARER_CTX_REQ;
    psCmiReqData.cnfPrimId   = CMI_PS_SET_ATTACHED_BEARER_CTX_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC || psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}

/**
  \fn          appGetAttachBearerSettingSync
  \brief       Send cmi request to get attach bearer setting
  \param[out]  pAttachBearerSettingParams   OUTPUT, return current attach bearer setting info
  \returns     CmsRetId
  \NOTE:       Return attach bearer setting, refer AT: AT+ECATTBEARER?
*/
CmsRetId appGetAttachBearerSettingSync(GetAttachBearerSetting *pAttachBearerSettingParams)
{
    /*
     * NOTE:
     * 1. This only return info set by: appSetAttachBearerSync()/AT+ECATTBEARER,
     *     not the current activated bearer info.
     * Example:
     * a) Set iptype: IPV4V6, APN: "CMIOTA", via API: appSetAttachBearerSync()/AT+ECATTBEARER,
     * b) After attached procedure, network assign/activet the bearer/PDP with: Iptype: IPV4. and APN: "CMIOTB"
     * Then:
     * a) This API will return: iptype: IPV4V6, APN: "CMIOTA"
     * b) If want to get activated bearer info, need to call: appGetAPNSettingSync(UINT8 cid, UINT8 *apn), with cid set to 0.
    */
    CmsRetId                        cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData                 psCmiReqData;
    CmiPsGetAttachedBearerCtxReq    cmiReq;
    //CmiPsGetAttachedBearerCtxCnf    cmiCnf;

    /* check the input */
    OsaCheck((pAttachBearerSettingParams != NULL), pAttachBearerSettingParams, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiPsSetAttachedBearerCtxReq));
    memset(pAttachBearerSettingParams, 0x00, sizeof(GetAttachBearerSetting));

    psCmiReqData.sgId        = CAC_PS;
    psCmiReqData.reqPrimId   = CMI_PS_GET_ATTACHED_BEARER_CTX_REQ;
    psCmiReqData.cnfPrimId   = CMI_PS_GET_ATTACHED_BEARER_CTX_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(GetAttachBearerSetting);
    //typedef CmiPsGetAttachedBearerCtxCnf    GetAttachBearerSetting
    psCmiReqData.pCnfBuf   = pAttachBearerSettingParams;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC || psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }

        memset(pAttachBearerSettingParams, 0x00, sizeof(GetAttachBearerSetting));
    }

    return cmsRet;
}


/**
  \fn          appSetEccfgSync
  \brief       Send cmi request to set the UE extended configuration
  \param[in]   pSetExtCfgParams     the pointer to set the extended status infomation of UE
  \returns     CmsRetId
  \NOTE:       refer: AT+ECCFG
*/
CmsRetId appSetEccfgSync(SetExtCfgParams *pSetExtCfgParams)
{
    CmsRetId            cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData     psCmiReqData;
    CmiDevSetExtCfgReq  cmiReq;
    CmiDevSetExtCfgCnf  cmiCnf;

    /* check the input */
    OsaCheck((pSetExtCfgParams != NULL), pSetExtCfgParams, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevSetExtCfgReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevSetExtCfgCnf));

    memcpy(&cmiReq, (CmiDevSetExtCfgReq *)pSetExtCfgParams, sizeof(CmiDevSetExtCfgReq));

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_SET_EXT_CFG_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_SET_EXT_CFG_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC || psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}

/**
  \fn           appGetEccfgSettingSync
  \brief        Send cmi request to get the extended status infomation of UE.
  \param[out]   pExtCfgSetting   the pointer to the extended status infomation of UE returned.
  \returns      CmsRetId
  \NOTE:        refer: AT+ECCFG
*/
CmsRetId appGetEccfgSettingSync(GetExtCfgSetting *pExtCfgSetting)
{
    CmsRetId            cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData     psCmiReqData;
    CmiDevGetExtCfgReq  cmiReq;
    CmiDevGetExtCfgCnf  cmiCnf;

   /* check the input */
    OsaCheck((pExtCfgSetting != NULL), pExtCfgSetting, 0, 0);

    memset(pExtCfgSetting, 0x00, sizeof(CmiDevGetExtCfgCnf));

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetExtCfgReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetExtCfgCnf));

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_GET_EXT_CFG_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_GET_EXT_CFG_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        memcpy(pExtCfgSetting, &cmiCnf, sizeof(GetExtCfgSetting));
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}


/**
  \fn           appGetCsconStateSync
  \brief        Get RRC connection state: idle (0) or connected (1)
  \param[out]   pCsconState      RRC connection state: idle (0) / connected (1)
  \returns      CmsRetId
  \NOTE:        refer: AT+CSCON?
*/
CmsRetId appGetCsconStateSync(UINT8 *pCsconState)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiPsGetConnStatusReq   cmiReq;
    CmiPsGetConnStatusCnf   cmiCnf;

    OsaDebugBegin(pCsconState != PNULL, pCsconState, 0, 0);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    *pCsconState = 0;

    memset(&cmiReq, 0x00, sizeof(CmiPsGetConnStatusReq));
    memset(&cmiCnf, 0x00, sizeof(CmiPsGetConnStatusCnf));

    psCmiReqData.sgId        = CAC_PS;
    psCmiReqData.reqPrimId   = CMI_PS_GET_CONN_STATUS_REQ;
    psCmiReqData.cnfPrimId   = CMI_PS_GET_CONN_STATUS_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        *pCsconState = cmiCnf.connMode;     //0 - CMI_Ps_CONN_IDLE_MODE; 1 - CMI_Ps_CONNECTED_MODE
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}


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
CmsRetId appNBRAIReleaseConnection(void)
{
    UINT8   rrcConnState = 0;
    CmsRetId            cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData     psCmiReqData;
    CmiPsSendOriDataViaCpReq    cmiReq;
    CmiPsSendOriDataViaCpCnf    cmiCnf;

    cmsRet = appGetCsconStateSync(&rrcConnState);
    if (cmsRet != CMS_RET_SUCC)
    {
        return cmsRet;
    }

    if (rrcConnState == CMI_Ps_CONN_IDLE_MODE)  //already in idle state
    {
        return CMS_RET_SUCC;
    }

    /* CMI REQ, simulate a UL packet (useless) with the RAI: 1*/
    memset(&cmiReq, 0, sizeof(cmiReq));
    cmiReq.cid      = 0;    // 0 - acted as default bearer
    cmiReq.dataLen  = 1;
    cmiReq.cpData   = (UINT8 *)OsaAllocMemory(1);   //must allocated in heap, as PS will call OsaFreeMemeory() to free it
    if (cmiReq.cpData == PNULL)
    {
        OsaDebugBegin(FALSE, cmiReq.cpData, 0, 0);
        return CMS_NO_MEM;
        OsaDebugEnd();
    }
    cmiReq.cpData[0]= 0x00;
    cmiReq.rai      = CMI_PS_RAI_NO_UL_DL_FOLLOWED;

    /* input param here */
    psCmiReqData.sgId        = CAC_PS;
    psCmiReqData.reqPrimId   = CMI_PS_SEND_CP_DATA_REQ;
    psCmiReqData.cnfPrimId   = CMI_PS_SEND_CP_DATA_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC || psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}

/**
  \fn          appSetEcPsSlpCfgSync
  \brief       Send cmi request to set the protocol stack sleep2/hibernate configuration
  \param[in]   SetPsSlpCfgParams     the pointer to set the set parameters
  \returns     CmsRetId
  \NOTE:       refer: AT+ECPSSLPCFG =
*/
CmsRetId appSetEcPsSlpCfgSync(SetPsSlpCfgParams *pSetPsSlpCfgParams)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiDevSetPsSleepCfgReq  cmiReq;
    CmiDevSetPsSleepCfgCnf  cmiCnf;

    /* check the input */
    OsaCheck((pSetPsSlpCfgParams != NULL), pSetPsSlpCfgParams, 0, 0);

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevSetPsSleepCfgReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevSetPsSleepCfgCnf));

    memcpy(&cmiReq, (CmiDevSetPsSleepCfgReq *)pSetPsSlpCfgParams, sizeof(CmiDevSetPsSleepCfgReq));

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_SET_PSSLP_CFG_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_SET_PSSLP_CFG_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet != CMS_RET_SUCC || psCmiReqData.cnfRc != CME_SUCC)
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}

/**
  \fn           appGetEcPsSlpCfgSync
  \brief        Send cmi request to get the protocol stack sleep2/hibernate config.
  \param[out]   pExtCfgSetting   the pointer to the parameters returned.
  \returns      CmsRetId
  \NOTE:        refer: AT+ECPSSLPCFG?
*/
CmsRetId appGetEcPsSlpCfgSync(GetPsSlpCfgParams *pPsSlpCfgSetting)
{
    CmsRetId                cmsRet = CMS_RET_SUCC;
    AppPsCmiReqData         psCmiReqData;
    CmiDevGetPsSleepCfgReq  cmiReq;
    CmiDevGetPsSleepCfgCnf  cmiCnf;

   /* check the input */
    OsaCheck((pPsSlpCfgSetting != NULL), pPsSlpCfgSetting, 0, 0);

    memset(pPsSlpCfgSetting, 0x00, sizeof(GetPsSlpCfgParams));

    memset(&psCmiReqData, 0x00, sizeof(AppPsCmiReqData));
    memset(&cmiReq, 0x00, sizeof(CmiDevGetPsSleepCfgReq));
    memset(&cmiCnf, 0x00, sizeof(CmiDevGetPsSleepCfgCnf));

    psCmiReqData.sgId        = CAC_DEV;
    psCmiReqData.reqPrimId   = CMI_DEV_GET_PSSLP_CFG_REQ;
    psCmiReqData.cnfPrimId   = CMI_DEV_GET_PSSLP_CFG_CNF;
    psCmiReqData.reqParamLen = sizeof(cmiReq);
    psCmiReqData.pReqParam   = &cmiReq;

    /* output here */
    psCmiReqData.cnfBufLen = sizeof(cmiCnf);
    psCmiReqData.pCnfBuf   = &cmiCnf;

    cmsRet = appPsCmiReq(&psCmiReqData, CMS_MAX_DELAY_MS);

    if (cmsRet == CMS_RET_SUCC && psCmiReqData.cnfRc == CME_SUCC)
    {
        memcpy(pPsSlpCfgSetting, &cmiCnf, sizeof(GetPsSlpCfgParams));
    }
    else
    {
        OsaDebugBegin(FALSE, cmsRet, psCmiReqData.cnfRc, 0);
        OsaDebugEnd();

        if (cmsRet == CMS_RET_SUCC)
        {
            cmsRet = psSyncProcErrCode(psCmiReqData.cnfRc);
        }
    }

    return cmsRet;
}



