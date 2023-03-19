/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_dev_cnf_ind.c
*
*  Description: Process dev related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "cmsis_os2.h"
#include "bsp.h"
#include "at_util.h"

//#include "atec_controller.h"

#include "cmidev.h"
#include "atec_dev.h"
#include "ps_dev_if.h"

#include "app.h" // 20210926 add to support dahua qfashion1

 /******************************************************************************
  ******************************************************************************
  * GLOBAL VARIABLES
  ******************************************************************************
 ******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS


 /******************************************************************************
  ******************************************************************************
  * INERNAL/STATIC FUNCTION
  ******************************************************************************
 ******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS


 /******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCFUNSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNGetConf
 * Description: Process CMI cnf msg of AT+CFUN?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCFUNGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[16];
    CmiDevGetCfunCnf *getDevCfunConf = (CmiDevGetCfunCnf *)paras;
    UINT8 func = getDevCfunConf->func;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CFUN: %d", func);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNGetCapaConf
 * Description: Process CMI cnf msg of AT+CFUN=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCFUNGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    CmiDevGetCfunCapaCnf *getDevCfunConf = (CmiDevGetCfunCapaCnf *)paras;
    UINT32 func = getDevCfunConf->funcBitmap;
    UINT32 rst = getDevCfunConf->rstBitmap;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CFUN: 0x%x, 0x%x", func, rst);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devECBANDSetCnf
 * Description: Process CMI cnf msg of AT+ECBAND=[<mode>,<band>]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECBANDSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devECBANDGetCnf
 * Description: Process CMI cnf msg of AT+ECBAND?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECBANDGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0}, tmpStrBuf[16] = {0};
    CmiDevGetCiotBandCnf *pCmiCnf = (CmiDevGetCiotBandCnf *)paras;
    UINT8   tmpIdx = 0;
    UINT16  rspStrLen = 0;

    if (rc == CME_SUCC)
    {
        /*
         * +ECBAND: <mode>,<band1>,<band2>...
        */
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBAND: %d", pCmiCnf->nwMode);


        for (tmpIdx = 0; tmpIdx < pCmiCnf->bandNum && tmpIdx < CMI_DEV_SUPPORT_MAX_BAND_NUM; tmpIdx++)
        {
            snprintf(tmpStrBuf, 16, ",%d", pCmiCnf->orderedBand[tmpIdx]);

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}


/******************************************************************************
 * devECBandGetCapaCnf
 * Description: Process CMI cnf msg of AT+ECBAND=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECBandGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0}, tmpStrBuf[16] = {0};
    CmiDevGetCiotBandCapaCnf *getDevCbandConf = (CmiDevGetCiotBandCapaCnf *)paras;
    UINT8   tmpIdx = 0;
    UINT16  rspStrLen = 0;

    if (rc == CME_SUCC)
    {
        /*
         * +ECBAND: (<modes>),(<band>s)
        */
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBAND: (%d)", getDevCbandConf->nwMode);

        for (tmpIdx = 0; tmpIdx < getDevCbandConf->bandNum && tmpIdx < CMI_DEV_SUPPORT_MAX_BAND_NUM; tmpIdx++)
        {
            if (tmpIdx == 0)
            {
                snprintf(tmpStrBuf, 16, ",(%d", getDevCbandConf->supportBand[tmpIdx]);
            }
            else
            {
                snprintf(tmpStrBuf, 16, ",%d", getDevCbandConf->supportBand[tmpIdx]);
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
        }

        if (tmpIdx > 0)
        {
            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
            strncat(pRspStr, ")", ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atDevCiotfreqGetConf
 * Description: Process CMI cnf msg of AT+ECFREQ?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCiotfreqGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStrBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR tempBuf[40] = {0};
    CmiDevGetCiotFreqCnf *pCmiCnf = (CmiDevGetCiotFreqCnf *)paras;
    UINT32  rspStrLen = 0, tmpIdx = 0;

    if (rc == CME_SUCC)
    {
        if (pCmiCnf->mode == CMI_DEV_NO_FREQ_INFO)
        {
            /* Only return OK */
            ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
        }
        else if (pCmiCnf->mode == CMI_DEV_PREFER_FREQ_INFO)
        {
            snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECFREQ: 1");

            for (tmpIdx = 0; tmpIdx < pCmiCnf->arfcnNum && tmpIdx < CMI_DEV_SUPPORT_MAX_FREQ_NUM; tmpIdx++)
            {
                memset(tempBuf, 0, sizeof(tempBuf));
                snprintf(tempBuf, 10, ",%d", pCmiCnf->arfcnList[tmpIdx]);

                rspStrLen = strlen(rspStrBuf);

                if (rspStrLen < ATEC_IND_RESP_128_STR_LEN-1)
                {
                    strncat(rspStrBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
                }
            }
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspStrBuf);
        }
        else if (pCmiCnf->mode == CMI_DEV_CELL_LOCK_INFO)
        {
            if (pCmiCnf->cellPresent)
            {
                snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECFREQ: 2,%d,%d", pCmiCnf->lockedArfcn, pCmiCnf->phyCellId);
            }
            else
            {
                snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECFREQ: 2,%d", pCmiCnf->lockedArfcn);
            }

            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspStrBuf);
        }
        else if (pCmiCnf->mode == CMI_DEV_PREFER_FREQ_CELL_LOCK)
        {
            snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECFREQ: 1");

            for (tmpIdx = 0; tmpIdx < pCmiCnf->arfcnNum && tmpIdx < CMI_DEV_SUPPORT_MAX_FREQ_NUM; tmpIdx++)
            {
                memset(tempBuf, 0, sizeof(tempBuf));
                snprintf(tempBuf, 10, ",%d", pCmiCnf->arfcnList[tmpIdx]);

                rspStrLen = strlen(rspStrBuf);

                if (rspStrLen < ATEC_IND_RESP_128_STR_LEN-1)
                {
                    strncat(rspStrBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
                }
            }

            memset(tempBuf, 0, sizeof(tempBuf));

            if (pCmiCnf->cellPresent)
            {
#ifdef FEATURE_REF_AT_ENABLE
                snprintf(tempBuf, 40, "\r\n\r\n+ECFREQ:2,%d,%d", pCmiCnf->lockedArfcn, pCmiCnf->phyCellId);
#else
                snprintf(tempBuf, 40, "\r\n\r\n+ECFREQ: 2,%d,%d", pCmiCnf->lockedArfcn, pCmiCnf->phyCellId);
#endif
            }
            else
            {
#ifdef FEATURE_REF_AT_ENABLE
                snprintf(tempBuf, 40, "\r\n\r\n+ECFREQ:2,%d", pCmiCnf->lockedArfcn);
#else
                snprintf(tempBuf, 40, "\r\n\r\n+ECFREQ: 2,%d", pCmiCnf->lockedArfcn);
#endif
            }

            rspStrLen = strlen(rspStrBuf);

            if (rspStrLen < ATEC_IND_RESP_128_STR_LEN-1)
            {
                strncat(rspStrBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
            }

            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspStrBuf);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}


/******************************************************************************
 * atDevECRMFPLMNSetConf
 * Description: Process CMI cnf msg of AT+ECFREQ=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCiotfreqSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPowerStateSetConf
 * Description: Process CMI cnf msg of AT+CIOTPOWER=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devPowerStateSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        if(reqHandle != BROADCAST_IND_HANDLER)//when handle=0x0001,it means this confirm do not need to reply ok
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevECCFGSetConf
 * Description: Process CMI cnf msg of AT+ECCFG=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECCFGSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevECCFGGetConf
 * Description: Process CMI cnf msg of AT+ECCFG?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECCFGGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_ECCFG_GET_RSP_STR_LEN   512

    CmsRetId ret = CMS_FAIL;
    char *pRspStrBuf = PNULL;
    CmiDevGetExtCfgCnf *pGetEcfgCnf = (CmiDevGetExtCfgCnf *)paras;

    if (rc == CME_SUCC)
    {
        pRspStrBuf = (char *)OsaAllocZeroMemory(ATEC_ECCFG_GET_RSP_STR_LEN);

        if (pRspStrBuf != PNULL)
        {
            /* CCM, EMM, ERRC */
            /*snprintf((char*)pRspStrBuf, ATEC_ECCFG_GET_RSP_STR_LEN,
                     "+ECCFG: \"PowerCfun\",%d,\"PsPowerOnMaxDelay\",%d,\"AutoApn\",%d,\"UsimPowerSave\",%d,\"Rohc\",%d,\"Ipv6RsForTestSim\",%d,\"SupportSms\",%d,\"TauForSms\",%d,\"T3324MaxValueS\",%ld,\"PlmnSearchPowerLevel\",%d,\"Epco\",%d,\"MultiCarrier\",%d,\"MultiTone\",%d,\"SupportUpRai\",%d,\"DataInactTimer\",%d,\"RelaxMonitorDeltaP\",%d,\"RelVersion\",%d,\"UsimSimulator\",%d",
                     pGetEcfgCnf->powerCfun, pGetEcfgCnf->powerOnMaxDelay, pGetEcfgCnf->bAutoApn, ((pGetEcfgCnf->bSimTest == TRUE)?FALSE:TRUE), pGetEcfgCnf->bRohc, pGetEcfgCnf->bIpv6RsForTestSim,
                     pGetEcfgCnf->supportSms, pGetEcfgCnf->tauForSms, pGetEcfgCnf->t3324MaxValue, pGetEcfgCnf->plmnSearchPowerLevel, pGetEcfgCnf->enableEpco,
                     pGetEcfgCnf->multiCarrier, pGetEcfgCnf->multiTone, pGetEcfgCnf->supportUpRai, pGetEcfgCnf->ueCfgDataInactTimer,
                     pGetEcfgCnf->ueCfgRelaxMonitorDeltaP, pGetEcfgCnf->relVersion, pGetEcfgCnf->bUsimSimulator);*/
#ifdef FEATURE_REF_AT_ENABLE
            snprintf((char*)pRspStrBuf, ATEC_ECCFG_GET_RSP_STR_LEN,
                     "+ECCFG: \"AutoApn\",%d,\"PsSoftReset\",%d,\"UsimPowerSave\",%d,\"UsimSimulator\",%d,\"SimBip\",%d,\"Rohc\",%d,\"Ipv6RsForTestSim\",%d,\"Ipv6RsDelay\",%d,\"PowerCfun\",%d,\"PsPowerOnMaxDelay\",%d,\"Cfun1PowerOnPs\",%d,\"SupportSms\",%d,\"TauForSms\",%d,\"PlmnSearchPowerLevel\",%d,\"Epco\",%d,\"T3324MaxValueS\",%ld,\"BarValueS\",%d,\"MultiCarrier\",%d,\"MultiTone\",%d,\"SupportUpRai\",%d,\"DataInactTimer\",%d,\"RelaxMonitorDeltaP\",%d,\"DisableNCellMeas\",%d,\"NbCategory\",%d,\"RelVersion\",%d,\"disableSib14Check\",%d",
                     pGetEcfgCnf->bAutoApn, pGetEcfgCnf->bEnablePsSoftReset, pGetEcfgCnf->enableSimPsm, pGetEcfgCnf->bUsimSimulator, pGetEcfgCnf->enableBip, pGetEcfgCnf->bRohc, pGetEcfgCnf->bIpv6RsForTestSim, pGetEcfgCnf->ipv6GetPrefixTime, pGetEcfgCnf->powerCfun,
                     pGetEcfgCnf->powerOnMaxDelay, pGetEcfgCnf->cfun1PowerOnPs, pGetEcfgCnf->supportSms, pGetEcfgCnf->tauForSms, pGetEcfgCnf->plmnSearchPowerLevel, pGetEcfgCnf->enableEpco, pGetEcfgCnf->t3324MaxValue, pGetEcfgCnf->barValue,
                     pGetEcfgCnf->multiCarrier, pGetEcfgCnf->multiTone, pGetEcfgCnf->supportUpRai, pGetEcfgCnf->ueCfgDataInactTimer,
                     pGetEcfgCnf->ueCfgRelaxMonitorDeltaP, pGetEcfgCnf->disableNCellMeas, pGetEcfgCnf->nbCategory, pGetEcfgCnf->relVersion, pGetEcfgCnf->disableSib14Check);
#else
            snprintf((char*)pRspStrBuf, ATEC_ECCFG_GET_RSP_STR_LEN,
                     "+ECCFG: \"AutoApn\",%d,\"PsSoftReset\",%d,\"UsimPowerSave\",%d,\"UsimSimulator\",%d,\"SimBip\",%d,\"Rohc\",%d,\"Ipv6RsForTestSim\",%d,\"Ipv6RsDelay\",%d,\"PowerCfun\",%d,\"PsPowerOnMaxDelay\",%d,\"SupportSms\",%d,\"TauForSms\",%d,\"PlmnSearchPowerLevel\",%d,\"Epco\",%d,\"T3324MaxValueS\",%ld,\"BarValueS\",%d,\"MultiCarrier\",%d,\"MultiTone\",%d,\"SupportUpRai\",%d,\"DataInactTimer\",%d,\"RelaxMonitorDeltaP\",%d,\"DisableNCellMeas\",%d,\"NbCategory\",%d,\"RelVersion\",%d,\"disableSib14Check\",%d",
                     pGetEcfgCnf->bAutoApn, pGetEcfgCnf->bEnablePsSoftReset, pGetEcfgCnf->enableSimPsm, pGetEcfgCnf->bUsimSimulator, pGetEcfgCnf->enableBip, pGetEcfgCnf->bRohc, pGetEcfgCnf->bIpv6RsForTestSim, pGetEcfgCnf->ipv6GetPrefixTime, pGetEcfgCnf->powerCfun,
                     pGetEcfgCnf->powerOnMaxDelay, pGetEcfgCnf->supportSms, pGetEcfgCnf->tauForSms, pGetEcfgCnf->plmnSearchPowerLevel, pGetEcfgCnf->enableEpco, pGetEcfgCnf->t3324MaxValue, pGetEcfgCnf->barValue,
                     pGetEcfgCnf->multiCarrier, pGetEcfgCnf->multiTone, pGetEcfgCnf->supportUpRai, pGetEcfgCnf->ueCfgDataInactTimer,
                     pGetEcfgCnf->ueCfgRelaxMonitorDeltaP, pGetEcfgCnf->disableNCellMeas, pGetEcfgCnf->nbCategory, pGetEcfgCnf->relVersion, pGetEcfgCnf->disableSib14Check);
#endif
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pRspStrBuf);

            OsaFreeMemory(&pRspStrBuf);
        }
        else
        {
            OsaDebugBegin(FALSE, 160, 0, 0);
            OsaDebugEnd();
            ret = atcReply(reqHandle, AT_RC_ERROR, CME_MEMORY_FULL, NULL);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atDevECRMFPLMNSetConf
 * Description: Process CMI cnf msg of AT+ECRMFPLMN=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECRMFPLMNSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMOLRSetCnf
 * Description: Process CMI cnf msg of AT+CMTOR=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMOLRSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    //CmiDevSetCmolrCnf *Cnf = (CmiDevSetCmolrCnf *)paras;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMOLRGetCnf
 * Description: Process CMI cnf msg of AT+CMOLR?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMOLRGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_CMOLR_GET_RSP_STR_LEN   200

    CmsRetId ret = CMS_FAIL;
    char    *pRspStrBuf = PNULL;
    CHAR    tmpStrBuf[20] = {0};
    UINT16  rspStrLen = 0;
    CmiDevGetCmolrCnf *pGetCmolrCnf = (CmiDevGetCmolrCnf *)paras;

    if (rc == CME_SUCC)
    {
        pRspStrBuf = (char *)OsaAllocZeroMemory(ATEC_CMOLR_GET_RSP_STR_LEN);

        if (pRspStrBuf != PNULL)
        {
            snprintf((char*)pRspStrBuf, ATEC_CMOLR_GET_RSP_STR_LEN,
                     "+CMOLR: %d,%d,%d",
                     pGetCmolrCnf->enableMode, pGetCmolrCnf->method, pGetCmolrCnf->horAccSet);

            strcat((CHAR*)pRspStrBuf, ",");

            if (pGetCmolrCnf->horAccPresent)
            {
                snprintf(tmpStrBuf, 2, "%d", pGetCmolrCnf->horAcc);

                rspStrLen = strlen(pRspStrBuf);
                OsaCheck(rspStrLen < ATEC_CMOLR_GET_RSP_STR_LEN, rspStrLen, ATEC_CMOLR_GET_RSP_STR_LEN, 0);
                strncat(pRspStrBuf, tmpStrBuf, ATEC_CMOLR_GET_RSP_STR_LEN-rspStrLen-1);
            }

            strcat((CHAR*)pRspStrBuf, ",");

            memset(tmpStrBuf, 0, 20);
            snprintf(tmpStrBuf, 2, "%d", pGetCmolrCnf->verReq);
            OsaCheck(rspStrLen < ATEC_CMOLR_GET_RSP_STR_LEN, rspStrLen, ATEC_CMOLR_GET_RSP_STR_LEN, 0);
            strncat(pRspStrBuf, tmpStrBuf, ATEC_CMOLR_GET_RSP_STR_LEN-rspStrLen-1);

            strcat((CHAR*)pRspStrBuf, ",");

            if (pGetCmolrCnf->verAccSetPresent)
            {
                memset(tmpStrBuf, 0, 20);
                snprintf(tmpStrBuf, 2, "%d", pGetCmolrCnf->verAccSet);

                rspStrLen = strlen(pRspStrBuf);
                OsaCheck(rspStrLen < ATEC_CMOLR_GET_RSP_STR_LEN, rspStrLen, ATEC_CMOLR_GET_RSP_STR_LEN, 0);
                strncat(pRspStrBuf, tmpStrBuf, ATEC_CMOLR_GET_RSP_STR_LEN-rspStrLen-1);
            }

            strcat((CHAR*)pRspStrBuf, ",");

            if (pGetCmolrCnf->verAccPresent)
            {
                memset(tmpStrBuf, 0, 20);
                snprintf(tmpStrBuf, 2, "%d", pGetCmolrCnf->verAcc);

                rspStrLen = strlen(pRspStrBuf);
                OsaCheck(rspStrLen < ATEC_CMOLR_GET_RSP_STR_LEN, rspStrLen, ATEC_CMOLR_GET_RSP_STR_LEN, 0);
                strncat(pRspStrBuf, tmpStrBuf, ATEC_CMOLR_GET_RSP_STR_LEN-rspStrLen-1);
            }

            strcat((CHAR*)pRspStrBuf, ",");
            memset(tmpStrBuf, 0, 20);

            snprintf(tmpStrBuf, 20, "%d,%d,%d,%d,%d,%d",
                     pGetCmolrCnf->velReq,pGetCmolrCnf->reportMode,pGetCmolrCnf->timeout,
                     pGetCmolrCnf->interval,pGetCmolrCnf->shapeRep,pGetCmolrCnf->plane);

            rspStrLen = strlen(pRspStrBuf);
            OsaCheck(rspStrLen < ATEC_CMOLR_GET_RSP_STR_LEN, rspStrLen, ATEC_CMOLR_GET_RSP_STR_LEN, 0);
            strncat(pRspStrBuf, tmpStrBuf, ATEC_CMOLR_GET_RSP_STR_LEN-rspStrLen-1);

            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pRspStrBuf);

            OsaFreeMemory(&pRspStrBuf);
        }
        else
        {
            OsaDebugBegin(FALSE, 200, 0, 0);
            OsaDebugEnd();
            ret = atcReply(reqHandle, AT_RC_ERROR, CME_MEMORY_FULL, NULL);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * devCMOLRGetCapaCnf
 * Description: Process CMI cnf msg of AT+CMOLR=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMOLRGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    //CmiDevGetCmtlrCapaCnf *Cnf = (CmiDevGetCmtlrCapaCnf *)paras;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMTLRSetCnf
 * Description: Process CMI cnf msg of AT+CMTLR=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMTLRSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    //CmiDevSetCmtlrCnf *Cnf = (CmiDevSetCmtlrCnf *)paras;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMTLRGetCnf
 * Description: Process CMI cnf msg of AT+CMTLR?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMTLRGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStrBuf[24] = {0};
    UINT8 subscribe = 0;
    CmiDevGetCmtlrCnf *Cnf = (CmiDevGetCmtlrCnf *)paras;

    if(rc == CME_SUCC)
    {
        subscribe = Cnf->subscribe;
        sprintf((char*)rspStrBuf, "+CMTLR: %d", subscribe);
        ret = atcReply(reqHandle, AT_RC_OK, 0, rspStrBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMTLRGetCapaCnf
 * Description: Process CMI cnf msg of AT+CMTLR=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMTLRGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStrBuf[24] = {0};
    CHAR tempBuf[4] = {0};
    UINT8 i;
    BOOL first=TRUE;
    CmiDevGetCmtlrCapaCnf *Cnf = (CmiDevGetCmtlrCapaCnf *)paras;
    UINT8 bitmap = Cnf->bBitMap;

    if (rc == CME_SUCC)
    {
        /*+CMTLR: (list of supported <subscribe>s)*/
        sprintf(rspStrBuf, "+CMTLR: (");

        for (i = 0; i < 8; i++)
        {
            memset(tempBuf, 0, sizeof(tempBuf));

            if ((bitmap >> i) & 0x01)
            {
                if (first)
                {
                    sprintf(tempBuf, "%d", i);
                    first = FALSE;
                }
                else
                {
                    sprintf(tempBuf, ",%d", i);
                }
            }

            strcat(rspStrBuf, tempBuf);
        }

        strcat(rspStrBuf, ")");

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspStrBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMTLRASetCnf
 * Description: Process CMI cnf msg of AT+CMTLRA=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMTLRASetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    //CmiDevSetCmtlraCnf *Cnf = (CmiDevSetCmtlraCnf *)paras;

    if (rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMTLRAGetCnf
 * Description: Process CMI cnf msg of AT+CMTLRA?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMTLRAGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStrBuf[24] = {0};
    UINT8 allow = 0;
    UINT8 handleId = 0;
    CmiDevGetCmtlraCnf *Cnf = (CmiDevGetCmtlraCnf *)paras;

    if (rc == CME_SUCC)
    {
        allow = Cnf->allowance;
        handleId = Cnf->handleId;
        sprintf((char*)rspStrBuf, "+CMTLRA: %d, %d", allow, handleId);
        ret = atcReply(reqHandle, AT_RC_OK, 0, rspStrBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devCMTLRAGetCapaCnf
 * Description: Process CMI cnf msg of AT+CMTLRA=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devCMTLRAGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStrBuf[24] = {0};
    CHAR tempBuf[4] = {0};
    UINT8 allow = 0;
    CmiDevGetCmtlraCapaCnf *Cnf = (CmiDevGetCmtlraCapaCnf *)paras;

    if (rc == CME_SUCC)
    {
        sprintf(rspStrBuf, "+CMTLRA: (");

        if ((Cnf->bBitMap & 0x1) == 1)
        {
            allow = 0;
        }

        sprintf(tempBuf, "%d", allow);
        strcat(rspStrBuf, tempBuf);
        memset(tempBuf, 0, sizeof(tempBuf));

        if (((Cnf->bBitMap >> 1) & 0x1) == 1)
        {
            allow = 1;
        }
        sprintf(tempBuf, ",%d", allow);
        strcat(rspStrBuf, tempBuf);
        strcat(rspStrBuf, ")");

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspStrBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}


/******************************************************************************
 * devECSTATUSGetCnf
 * Description: Process CMI cnf msg of AT+ESTATUS?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECSTATUSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_DEV_ECSTATUS_CNF_STR_LEN    384
    CmsRetId    ret = CMS_FAIL;
    CHAR        *pCnfStr = PNULL;

    CmiDevGetExtStatusCnf   *pCmiCnf = (CmiDevGetExtStatusCnf *)paras;
    CmiDevPhyStatusInfo     *pPhyInfo = PNULL;
    CmiDevL2StatusInfo      *pL2Info = PNULL;
    CmiDevRrcStatusInfo     *pRrcInfo = PNULL;
    CmiDevNasStatusInfo     *pNasInfo = PNULL;
    CmiDevCcmStatusInfo     *pCcmInfo = PNULL;
    CHAR    *pEmmState = PNULL,
            *pEmmMode = PNULL,
            *pPlmnState = PNULL,
            *pPlmnType = PNULL;

    if (rc != CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
        return ret;
    }

    OsaCheck(pCmiCnf != PNULL, pCmiCnf, 0, 0);
    pCnfStr = (CHAR *)OsaAllocZeroMemory(ATEC_DEV_ECSTATUS_CNF_STR_LEN);

    OsaDebugBegin(pCnfStr != PNULL, ATEC_DEV_ECSTATUS_CNF_STR_LEN, 0, 0);
    return ret;
    OsaDebugEnd();

    /*
     * +ECSTATUS: PHY, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_PHY == pCmiCnf->getStatusType)
    {
        pPhyInfo = &(pCmiCnf->phyStatus);
        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "+ECSTATUS: PHY, DlEarfcn:%d, UlEarfcn:%d, PCI:%d, Band:%d, RSRP:%d, RSRQ:%d, SNR:%d, AnchorFreqOfst:%d, NonAnchorFreqOfst:%d, CeLevel:%d, DlBler:%d/100, UlBler:%d/100, DataInactTimerS:%d, RetxBSRTimerP:%d, TAvalue:%d, TxPower:%d",
                 pPhyInfo->dlEarfcn, pPhyInfo->ulEarfcn, pPhyInfo->phyCellId, pPhyInfo->band,
                 pPhyInfo->sRsrp/100, pPhyInfo->sRsrq/100, pPhyInfo->snr, pPhyInfo->anchorFreqOffset, pPhyInfo->nonAnchorFreqOffset, ((pPhyInfo->ceLevel == 0xFF)? -2 : pPhyInfo->ceLevel),
                 pPhyInfo->dlBler, pPhyInfo->ulBler, pPhyInfo->dataInactTimerS, pPhyInfo->retxBSRTimerP, pPhyInfo->taValue, pPhyInfo->txPower);

        switch (pPhyInfo->nbOperMode)
        {
            case CMI_DEV_EM_NB_INBAND_SAME_PCI_MODE:
                strncat(pCnfStr, ", NBMode:\"InBand Same PCI\"", ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr));
                break;

            case CMI_DEV_EM_NB_INBAND_DIFF_PCI_MODE:
                strncat(pCnfStr, ", NBMode:\"InBand Diff PCI\"", ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr));
                break;

            case CMI_DEV_EM_NB_GUARD_BAND_MODE:
                strncat(pCnfStr, ", NBMode:\"Guard Band\"", ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr));
                break;

            case CMI_DEV_EM_NB_STAND_ALONE_MODE:
                strncat(pCnfStr, ", NBMode:\"Stand alone\"", ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr));
                break;

            default:
                strncat(pCnfStr, ", NBMode:\"Unknown\"", ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr));
                break;
        }
        if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pCnfStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }
    }

    /*
     * +ECSTATUS: L2, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_L2 == pCmiCnf->getStatusType)
    {
        pL2Info = &(pCmiCnf->l2Status);
        memset(pCnfStr, 0x00, ATEC_DEV_ECSTATUS_CNF_STR_LEN);
        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "+ECSTATUS: L2, SrbNum:%d, DrbNum:%d",
                 pL2Info->srbNum, pL2Info->drbNum);

        if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pCnfStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }
    }

    /*
     * +ECSTATUS: ERRC, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_RRC == pCmiCnf->getStatusType)
    {
        pRrcInfo = &(pCmiCnf->rrcStatus);
        memset(pCnfStr, 0x00, ATEC_DEV_ECSTATUS_CNF_STR_LEN);
        switch (pRrcInfo->rrcState)
        {
            case CMI_DEV_EM_RRC_DEACTIVE_STATE:
                snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                         "+ECSTATUS: RRC, State:\"DEACT\"");
                break;

            case CMI_DEV_EM_RRC_CELL_SEARCH_STATE:
                snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                         "+ECSTATUS: RRC, State:\"CELL SEARCH\"");
                break;

            case CMI_DEV_EM_RRC_NORMAL_IDLE_STATE:
                snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                         "+ECSTATUS: RRC, State:\"IDLE\"");
                break;

            case CMI_DEV_EM_RRC_SUSPEND_IDLE_STATE:
                snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                         "+ECSTATUS: RRC, State:\"SUSPEND IDLE\"");
                break;

            case CMI_DEV_EM_RRC_CONNECTED_STATE:
                snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                         "+ECSTATUS: RRC, State:\"CONNECTED\"");
                break;

            default:
                OsaDebugBegin(FALSE, pRrcInfo->rrcState, 0, 0);
                OsaDebugEnd();
                snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                         "+ECSTATUS: RRC, State:\"UNKONWN\"");
                break;
        }

        snprintf(pCnfStr+strlen(pCnfStr), ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr),
                 ", TAC:%d, CellId:%d", pRrcInfo->tac, pRrcInfo->cellId);

        if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pCnfStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }
    }

    /*
     * +ECSTATUS: EMM, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_EMM == pCmiCnf->getStatusType)
    {
        pNasInfo = &(pCmiCnf->nasStatus);
        memset(pCnfStr, 0x00, ATEC_DEV_ECSTATUS_CNF_STR_LEN);

        switch (pNasInfo->emmState)
        {
            case CMI_DEV_EM_EMM_NULL_STATE:
                pEmmState = "NULL";
                break;

            case CMI_DEV_EM_EMM_DEREGISTERED:
                pEmmState = "DEREG";
                break;

            case CMI_DEV_EM_EMM_REGISTERED_INITIATED:
                pEmmState = "REG INIT";
                break;

            case CMI_DEV_EM_EMM_REGISTERED:
                pEmmState = "REG";
                break;

            case CMI_DEV_EM_EMM_DEREGISTERED_INITIATED:
                pEmmState = "DEREG INIT";
                break;

            case CMI_DEV_EM_EMM_TAU_INITIATED:
                pEmmState = "TAU INIT";
                break;

            case CMI_DEV_EM_EMM_SR_INITIATED:
                pEmmState = "SR INIT";
                break;

            default:
                OsaDebugBegin(FALSE, pNasInfo->emmState, pNasInfo->emmMode, 0);
                OsaDebugEnd();
                pEmmState = "UNKNOWN";
                break;
        }

        switch (pNasInfo->emmMode)
        {
            case CMI_DEV_EM_EMM_IDLE_MODE:
                pEmmMode = "IDLE";
                break;

            case CMI_DEV_EM_EMM_PSM_MODE:
                pEmmMode = "PSM";
                break;

            case CMI_DEV_EM_EMM_CONNECTED_MODE:
                pEmmMode = "CONNECTED";
                break;

            default:
                OsaDebugBegin(FALSE, pNasInfo->emmMode, pNasInfo->emmState, 0);
                OsaDebugEnd();
                pEmmMode = "UNKNOWN";
                break;
        }

        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "+ECSTATUS: EMM, EmmState:\"%s\", EmmMode:\"%s\", PTWMs:%d, EDRXPeriodMs:%d, PsmExT3412TimerS:%u, T3324TimerS:%d, T3346RemainTimeS:%d",
                 pEmmState, pEmmMode, pNasInfo->ptwMs, pNasInfo->eDRXPeriodMs,
                 pNasInfo->psmExT3412TimerS, pNasInfo->T3324TimerS, pNasInfo->T3346RemainTimeS);

        if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pCnfStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }
    }

    /*
     * +ESTATUS: PLMN, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_PLMN == pCmiCnf->getStatusType)
    {
        pNasInfo = &(pCmiCnf->nasStatus);
        memset(pCnfStr, 0x00, ATEC_DEV_ECSTATUS_CNF_STR_LEN);
        switch (pNasInfo->plmnState)
        {
            case CMI_DEV_EM_NO_PLMN_SELECTED:
                pPlmnState = "NO PLMN";
                break;
            case CMI_DEV_EM_PLMN_SEARCHING:
                pPlmnState = "SEARCHING";
                break;

            case CMI_DEV_EM_PLMN_SELECTED:
                pPlmnState = "SELECTED";
                break;

            default:
                OsaDebugBegin(FALSE, pNasInfo->plmnState, pNasInfo->emmMode, pNasInfo->emmState);
                OsaDebugEnd();
                pPlmnState = "UNKNOWN";
                break;
        }

        switch (pNasInfo->plmnType)
        {
            case CMI_DEV_EMM_HPLMN:
                pPlmnType = "HPLMN";
                break;

            case CMI_DEV_EMM_EHPLMN:
                pPlmnType = "EHPLMN";
                break;

            case CMI_DEV_EMM_VPLMN:
                pPlmnType = "VPLMN";
                break;

            case CMI_DEV_EMM_UPLMN:
                pPlmnType = "UPLMN";
                break;

            case CMI_DEV_EMM_OPLMN:
                pPlmnType = "OPLMN";
                break;

            case CMI_DEV_EMM_APLMN:     //in no plmn selected, return this type
                pPlmnType = "UNKNOWN";
                break;

            case CMI_DEV_EMM_FPLMN:
                pPlmnType = "FPLMN";
                break;
            default:
                OsaDebugBegin(FALSE, pNasInfo->plmnType, pNasInfo->emmMode, pNasInfo->emmState);
                OsaDebugEnd();
                pPlmnType = "UNKNOWN";
                break;
        }

        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "+ECSTATUS: PLMN, PlmnState:\"%s\", PlmnType:\"%s\", SelectPlmn:\"0x%x,0x%x\"",
                 pPlmnState, pPlmnType, pNasInfo->selectPlmn.mcc, pNasInfo->selectPlmn.mncWithAddInfo);

        if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pCnfStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }
    }

    /*
     * +ECSTATUS: ESM, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_ESM == pCmiCnf->getStatusType)
    {
        pNasInfo = &(pCmiCnf->nasStatus);
        memset(pCnfStr, 0x00, ATEC_DEV_ECSTATUS_CNF_STR_LEN);
        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "+ECSTATUS: ESM, ActBearerNum:%d", pNasInfo->actBearerNum);

        if (strlen((CHAR*)(pNasInfo->apnStr)) > 0)
        {
            snprintf(pCnfStr+strlen(pCnfStr), ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr),
                     ", APN:\"%s\"", (CHAR*)(pNasInfo->apnStr));
        }

        if (pNasInfo->ipv4Addr.addrType == CMI_IPV4_ADDR)
        {
            snprintf(pCnfStr+strlen(pCnfStr), ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr),
                     ", IPv4:\"%d.%d.%d.%d\"",
                     pNasInfo->ipv4Addr.addrData[0], pNasInfo->ipv4Addr.addrData[1],
                     pNasInfo->ipv4Addr.addrData[2], pNasInfo->ipv4Addr.addrData[3]);
        }

        if (pNasInfo->ipv4Addr.addrType == CMI_FULL_IPV6_ADDR)
        {
            snprintf(pCnfStr+strlen(pCnfStr), ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr),
                     ", IPv6:\"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\"",
                     pNasInfo->ipv6Addr.addrData[0], pNasInfo->ipv6Addr.addrData[1],
                     pNasInfo->ipv6Addr.addrData[2], pNasInfo->ipv6Addr.addrData[3],
                     pNasInfo->ipv6Addr.addrData[4], pNasInfo->ipv6Addr.addrData[5],
                     pNasInfo->ipv6Addr.addrData[6], pNasInfo->ipv6Addr.addrData[7],
                     pNasInfo->ipv6Addr.addrData[8], pNasInfo->ipv6Addr.addrData[9],
                     pNasInfo->ipv6Addr.addrData[10], pNasInfo->ipv6Addr.addrData[11],
                     pNasInfo->ipv6Addr.addrData[12], pNasInfo->ipv6Addr.addrData[13],
                     pNasInfo->ipv6Addr.addrData[14], pNasInfo->ipv6Addr.addrData[15]);
        }
        else if (pNasInfo->ipv4Addr.addrType == CMI_IPV6_INTERFACE_ADDR)
        {
            snprintf(pCnfStr+strlen(pCnfStr), ATEC_DEV_ECSTATUS_CNF_STR_LEN-strlen(pCnfStr),
                     ", IPv6Interface:\"%02x%02x:%02x%02x:%02x%02x:%02x%02x\"",
                     pNasInfo->ipv6Addr.addrData[0], pNasInfo->ipv6Addr.addrData[1],
                     pNasInfo->ipv6Addr.addrData[2], pNasInfo->ipv6Addr.addrData[3],
                     pNasInfo->ipv6Addr.addrData[4], pNasInfo->ipv6Addr.addrData[5],
                     pNasInfo->ipv6Addr.addrData[6], pNasInfo->ipv6Addr.addrData[7]);
        }

        if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pCnfStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }
    }

    /*
     * +ECSTATUS: CCM, xxxx, xxxx....
    */
    if (CMI_DEV_GET_ECSTATUS == pCmiCnf->getStatusType ||
        CMI_DEV_GET_ECSTATUS_CCM == pCmiCnf->getStatusType)
    {
        pCcmInfo = &(pCmiCnf->ccmStatus);
        memset(pCnfStr, 0x00, ATEC_DEV_ECSTATUS_CNF_STR_LEN);
        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "+ECSTATUS: CCM, Cfun:%d, IMSI:\"%s\"", pCcmInfo->cfunValue, pCcmInfo->imsi);

        /* last output print, should with right Handler */
        ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
    }

#ifdef CUSTOMER_DAHUA
    if (CMI_DEV_GET_NUESTATS == pCmiCnf->getStatusType)
    {
        pPhyInfo = &(pCmiCnf->phyStatus);

        snprintf(pCnfStr, ATEC_DEV_ECSTATUS_CNF_STR_LEN,
                 "Signal power:%d\r\nTotal power:%d\r\nTx power:%d\r\nTx time:%d\r\nRx time:%d\r\nCell ID:%d\r\nECL:%d\r\nSNR:%d\r\nEARFCN:%d\r\nPCI:%d\r\nRSRQ:%d\r\nOPERATOR MODE:%d\r\nCURRENT BAND:%d\r\n",
                pPhyInfo->sRsrp/10,pPhyInfo->totalPower/10,pPhyInfo->txPower,pPhyInfo->txTime,pPhyInfo->rxTime,pCmiCnf->rrcStatus.cellId,pPhyInfo->ceLevel,pPhyInfo->snr*10,pPhyInfo->dlEarfcn,pPhyInfo->phyCellId,pPhyInfo->sRsrq,pPhyInfo->nbOperMode,pPhyInfo->band);

        if (CMI_DEV_GET_NUESTATS == pCmiCnf->getStatusType)
        {
            /*As another response string followed, here we using */
            ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);
        }

    }
#endif

    /* Free string memory*/
    if (pCnfStr != PNULL)
    {
        OsaFreeMemory(&pCnfStr);
    }

    return ret;
}


/******************************************************************************
 * devECSTATISSetCnf
 * Description: Process CMI cnf msg of AT+ESTATIS=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECSTATISSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devECBCINFOGetCnf
 * Description: Process CMI cnf msg of AT+ECBCINFO
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECBCINFOGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiDevGetBasicCellListInfoCnf   *pCmiCnf = (CmiDevGetBasicCellListInfoCnf *)paras;
    CHAR    rspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    UINT8   idx = 0;

    if (rc != CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
        return ret;
    }

    /*
     * AT+ECBCINFO
     * +ECBCINFOSC: earfcn,pci,rsrp,rsrq,"sc_mcc","sc_mnc","sc_cellid","sc_tac"
     * +ECBCINFONC: earfcn,pci,rsrp,rsrq[,"nc_mcc","nc_mnc","nc_cellid","nc_tac"]
     * ...
     * OK
    */
    if (pCmiCnf->sCellPresent == FALSE)
    {
        /* maybe ASYN mode, remove debug error print */
        //OsaDebugBegin(FALSE, pCmiCnf->sCellPresent, pCmiCnf->nCellNum, 0);
        //OsaDebugEnd();

        ret = atcReply(reqHandle, AT_RC_OK, CME_SUCC, NULL);
        return ret;
    }

    if (CAC_IS_2_DIGIT_MNC(pCmiCnf->sCellInfo.plmn.mncWithAddInfo))
    {
        snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBCINFOSC: %d,%d,%d,%d,\"%03x\",\"%02x\",\"%08X\",\"%04X\"",
                 pCmiCnf->sCellInfo.earfcn, pCmiCnf->sCellInfo.phyCellId,
                 pCmiCnf->sCellInfo.rsrp, pCmiCnf->sCellInfo.rsrq,
                 pCmiCnf->sCellInfo.plmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->sCellInfo.plmn.mncWithAddInfo),
                 pCmiCnf->sCellInfo.cellId,
                 pCmiCnf->sCellInfo.tac);
    }
    else
    {
        snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBCINFOSC: %d,%d,%d,%d,\"%03x\",\"%03x\",\"%08X\",\"%04X\"",
                 pCmiCnf->sCellInfo.earfcn, pCmiCnf->sCellInfo.phyCellId,
                 pCmiCnf->sCellInfo.rsrp, pCmiCnf->sCellInfo.rsrq,
                 pCmiCnf->sCellInfo.plmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->sCellInfo.plmn.mncWithAddInfo),
                 pCmiCnf->sCellInfo.cellId,
                 pCmiCnf->sCellInfo.tac);
    }

    for (idx = 0; idx < pCmiCnf->nCellNum && idx < CMI_DEV_NCELL_INFO_CELL_NUM; idx++)
    {
        /*As following +ECBCINFONC need to output, here using: AT_RC_CONTINUE*/
        ret = atcReply(reqHandle, AT_RC_CONTINUE, CME_SUCC, rspStr);

        memset(rspStr, 0x00, ATEC_IND_RESP_128_STR_LEN);

        if (pCmiCnf->nCellList[idx].cellInfoValid == TRUE)
        {
            /*+ECBCINFONC: earfcn,pci,rsrp,rsrq[,"nc_mcc","nc_mnc","nc_cellid","nc_tac"]*/
            if (CAC_IS_2_DIGIT_MNC(pCmiCnf->nCellList[idx].plmn.mncWithAddInfo))
            {
                snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBCINFONC: %d,%d,%d,%d,\"%03x\",\"%02x\",\"%08X\",\"%04X\"",
                         pCmiCnf->nCellList[idx].earfcn, pCmiCnf->nCellList[idx].phyCellId,
                         pCmiCnf->nCellList[idx].rsrp, pCmiCnf->nCellList[idx].rsrq,
                         pCmiCnf->nCellList[idx].plmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->nCellList[idx].plmn.mncWithAddInfo),
                         pCmiCnf->nCellList[idx].cellId,
                         pCmiCnf->nCellList[idx].tac);
            }
            else
            {
                snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBCINFONC: %d,%d,%d,%d,\"%03x\",\"%03x\",\"%08X\",\"%04X\"",
                         pCmiCnf->nCellList[idx].earfcn, pCmiCnf->nCellList[idx].phyCellId,
                         pCmiCnf->nCellList[idx].rsrp, pCmiCnf->nCellList[idx].rsrq,
                         pCmiCnf->nCellList[idx].plmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->nCellList[idx].plmn.mncWithAddInfo),
                         pCmiCnf->nCellList[idx].cellId,
                         pCmiCnf->nCellList[idx].tac);
            }
        }
        else
        {
            /*+ECBCINFONC: earfcn,pci,rsrp,rsrq*/
            snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+ECBCINFONC: %d,%d,%d,%d",
                     pCmiCnf->nCellList[idx].earfcn, pCmiCnf->nCellList[idx].phyCellId,
                     pCmiCnf->nCellList[idx].rsrp, pCmiCnf->nCellList[idx].rsrq);
        }

    }

    /*last +ECBCINFONC*/
    ret = atcReply(reqHandle, AT_RC_OK, CME_SUCC, rspStr);

    return ret;
}

/****************************************************************************************
 * URC:
 * +ECBCINFOSC: earfcn,pci,rsrp,rsrq,"sc_mcc","sc_mnc","sc_cellid","sc_tac"
 * +ECBCINFONC: earfcn,pci,rsrp,rsrq[,"nc_mcc","nc_mnc","nc_cellid","nc_tac"]
 * ...
****************************************************************************************/
CmsRetId devECBCINFOInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiDevGetBasicCellListInfoInd   *pCmiInd = (CmiDevGetBasicCellListInfoInd *)paras;
    CHAR    indStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    UINT8   idx = 0;

    //ret = atcURC(chanId, pIndBuf);

    if (pCmiInd->sCellPresent == FALSE)
    {
        OsaDebugBegin(FALSE, pCmiInd->sCellPresent, pCmiInd->nCellNum, 0);
        OsaDebugEnd();

        return ret;
    }

    if (CAC_IS_2_DIGIT_MNC(pCmiInd->sCellInfo.plmn.mncWithAddInfo))
    {
        snprintf(indStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+ECBCINFOSC: %d,%d,%d,%d,\"%03x\",\"%02x\",\"%08X\",\"%04X\"\r\n",
                 pCmiInd->sCellInfo.earfcn, pCmiInd->sCellInfo.phyCellId,
                 pCmiInd->sCellInfo.rsrp, pCmiInd->sCellInfo.rsrq,
                 pCmiInd->sCellInfo.plmn.mcc, CAC_GET_PURE_MNC(pCmiInd->sCellInfo.plmn.mncWithAddInfo),
                 pCmiInd->sCellInfo.cellId,
                 pCmiInd->sCellInfo.tac);
    }
    else
    {
        snprintf(indStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+ECBCINFOSC: %d,%d,%d,%d,\"%03x\",\"%03x\",\"%08X\",\"%04X\"\r\n",
                 pCmiInd->sCellInfo.earfcn, pCmiInd->sCellInfo.phyCellId,
                 pCmiInd->sCellInfo.rsrp, pCmiInd->sCellInfo.rsrq,
                 pCmiInd->sCellInfo.plmn.mcc, CAC_GET_PURE_MNC(pCmiInd->sCellInfo.plmn.mncWithAddInfo),
                 pCmiInd->sCellInfo.cellId,
                 pCmiInd->sCellInfo.tac);
    }

    ret = atcURC(chanId, indStr);

    for (idx = 0; idx < pCmiInd->nCellNum && idx < CMI_DEV_NCELL_INFO_CELL_NUM; idx++)
    {
        memset(indStr, 0x00, ATEC_IND_RESP_128_STR_LEN);

        if (pCmiInd->nCellList[idx].cellInfoValid == TRUE)
        {
            /*+ECBCINFONC: earfcn,pci,rsrp,rsrq[,"nc_mcc","nc_mnc","nc_cellid","nc_tac"]*/
            if (CAC_IS_2_DIGIT_MNC(pCmiInd->nCellList[idx].plmn.mncWithAddInfo))
            {
                snprintf(indStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+ECBCINFONC: %d,%d,%d,%d,\"%03x\",\"%02x\",\"%08X\",\"%04X\"\r\n",
                         pCmiInd->nCellList[idx].earfcn, pCmiInd->nCellList[idx].phyCellId,
                         pCmiInd->nCellList[idx].rsrp, pCmiInd->nCellList[idx].rsrq,
                         pCmiInd->nCellList[idx].plmn.mcc, CAC_GET_PURE_MNC(pCmiInd->nCellList[idx].plmn.mncWithAddInfo),
                         pCmiInd->nCellList[idx].cellId,
                         pCmiInd->nCellList[idx].tac);
            }
            else
            {
                snprintf(indStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+ECBCINFONC: %d,%d,%d,%d,\"%03x\",\"%03x\",\"%08X\",\"%04X\"\r\n",
                         pCmiInd->nCellList[idx].earfcn, pCmiInd->nCellList[idx].phyCellId,
                         pCmiInd->nCellList[idx].rsrp, pCmiInd->nCellList[idx].rsrq,
                         pCmiInd->nCellList[idx].plmn.mcc, CAC_GET_PURE_MNC(pCmiInd->nCellList[idx].plmn.mncWithAddInfo),
                         pCmiInd->nCellList[idx].cellId,
                         pCmiInd->nCellList[idx].tac);
            }
        }
        else
        {
            /*+ECBCINFONC: earfcn,pci,rsrp,rsrq*/
            snprintf(indStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+ECBCINFONC: %d,%d,%d,%d\r\n",
                     pCmiInd->nCellList[idx].earfcn, pCmiInd->nCellList[idx].phyCellId,
                     pCmiInd->nCellList[idx].rsrp, pCmiInd->nCellList[idx].rsrq);
        }

        ret = atcURC(chanId, indStr);
    }

    return ret;
}


/****************************************************************************************
 * URC:
 *  +ECSTATIS:
 *
****************************************************************************************/
CmsRetId devECSTATISInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    *pIndBuf = PNULL;
    CmiDevExtStatisInd  *pStatisInd = (CmiDevExtStatisInd*)paras;
    CmiDevL2StatisInfo  *pL2StatisInfo = &(pStatisInd->l2StatisInfo);
    CmiDevPhyStatisInfo *pPhyStatisInfo = &(pStatisInd->phyStatisInfo);

    OsaDebugBegin(pStatisInd != PNULL && pStatisInd->periodTimeS > 0, pStatisInd, 0, 0);
    return ret;
    OsaDebugEnd();

    pIndBuf = (CHAR *)OsaAllocZeroMemory(ATEC_IND_RESP_256_STR_LEN);

    OsaDebugBegin(pIndBuf != PNULL, ATEC_IND_RESP_256_STR_LEN, 0, 0);
    return ret;
    OsaDebugEnd();

    /*
     * PHY DL
    */
    snprintf(pIndBuf, ATEC_IND_RESP_256_STR_LEN,
             "+ECSTATIS: PHY DL, AvgRSRP: %d, AvgSnr: %d, DlBler: %d%%, PhyDlTpt: %d bps, "
                                 "AvgTBS: %d, AvgItbs: %d, AvgNRep: %d, AvgSbfrmNum: %d, Harq2Ratio: %d%%",
             pPhyStatisInfo->avgRsrp/100, pPhyStatisInfo->avgSnr/100, pPhyStatisInfo->dlBler/100, (pPhyStatisInfo->dlAccTBSize*8)/pStatisInd->periodTimeS,
             pPhyStatisInfo->dlAvgTBSize/100, pPhyStatisInfo->dlAvgITBS/100, pPhyStatisInfo->dlAvgNrep/10, pPhyStatisInfo->dlAvgSbfrmNum/10,
             (pPhyStatisInfo->dlGrantRatio[1] == 0) ? 0 : ((pPhyStatisInfo->dlGrantRatio[1]*100)/(pPhyStatisInfo->dlGrantRatio[0]+pPhyStatisInfo->dlGrantRatio[1])));

    ret = atcURC(chanId, pIndBuf);

    /*
     * PHY UL
    */
    snprintf(pIndBuf, ATEC_IND_RESP_256_STR_LEN,
             "+ECSTATIS: PHY UL, UlBler: %d%%, PhyUlTpt: %d bps, AvgTBS: %d, AvgItbs: %d, "
                                 "AvgNRep: %d, AvgSbfrmNum: %d, Harq2Ratio: %d%%, AvgScNum: %d",
             pPhyStatisInfo->ulBler/100, (pPhyStatisInfo->ulAccTBSize*8)/pStatisInd->periodTimeS, pPhyStatisInfo->ulAvgTBSize/100, pPhyStatisInfo->ulAvgITBS/100,
             pPhyStatisInfo->ulAvgNrep/10, pPhyStatisInfo->ulAvgSbfrmNum/10,
             (pPhyStatisInfo->ulGrantRatio[1] == 0) ? 0 : ((pPhyStatisInfo->ulGrantRatio[1]*100)/(pPhyStatisInfo->ulGrantRatio[0]+pPhyStatisInfo->ulGrantRatio[1])),
             pPhyStatisInfo->ulAvgScNum/100);

    ret = atcURC(chanId, pIndBuf);

    /*
     * +ECSTATIS: MAC, MacUlBytes:%d, MacUlPadBytes:%d, MacDlBytes: %d, MacdlPadBytes: %d, MacDLTpT: %d bps, MacUlTpt: %d bps
    */
    snprintf(pIndBuf, ATEC_IND_RESP_256_STR_LEN,
             "+ECSTATIS: MAC, MacUlBytes:%d, MacUlPadBytes:%d, MacDlBytes: %d, MacDlPadBytes: %d, MacUlTpt: %d bps, MacDlTpt: %d bps",
             pL2StatisInfo->macUlBytes, pL2StatisInfo->macUlPadBytes, pL2StatisInfo->macDlBytes, pL2StatisInfo->macDlPadBytes,
             (pL2StatisInfo->macUlBytes*8)/pStatisInd->periodTimeS, (pL2StatisInfo->macDlBytes*8)/pStatisInd->periodTimeS);

    ret = atcURC(chanId, pIndBuf);

    /*
     * +ECSTATIS: RLC
    */
    snprintf(pIndBuf, ATEC_IND_RESP_256_STR_LEN,
             "+ECSTATIS: RLC, RlcUlPduBytes:%d, RlcUlRetxBytes:%d, RlcDlPduBytes: %d, RlcUlTpt: %d bps, RlcDlTpt: %d bps",
             pL2StatisInfo->rlcUlPduBytes, pL2StatisInfo->rlcUlRetxBytes, pL2StatisInfo->rlcDlPduBytes,
             (pL2StatisInfo->rlcUlPduBytes*8)/pStatisInd->periodTimeS, (pL2StatisInfo->rlcDlPduBytes*8)/pStatisInd->periodTimeS);

    ret = atcURC(chanId, pIndBuf);

    /*
     * +ECSTATIS: PDCP
    */
    //memset(pIndBuf, 0x00, ATEC_IND_RESP_256_STR_LEN);
    snprintf(pIndBuf, ATEC_IND_RESP_256_STR_LEN,
             "+ECSTATIS: PDCP, PdcpUlPduBytes: %d, PdcpDlPduBytes: %d, PdcpULDiscardBytes: %d, PdcpUlTpt: %d bps, PdcpDlTpt: %d bps",
             pL2StatisInfo->pdcpUlPduBytes, pL2StatisInfo->pdcpDlPduBytes, pL2StatisInfo->pdcpULDiscardBytes,
             (pL2StatisInfo->pdcpUlPduBytes*8)/pStatisInd->periodTimeS, (pL2StatisInfo->pdcpDlPduBytes*8)/pStatisInd->periodTimeS);

    ret = atcURC(chanId, pIndBuf);

    OsaFreeMemory(&pIndBuf);

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +CMTLR
*
****************************************************************************************/
CmsRetId devCMTLRInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RespBuf[48] = {0};
    CmiDevCmtlrInd *Ind = (CmiDevCmtlrInd*)paras;

    sprintf((char *)RespBuf, "+CMTLR: %d,%d,%d\r\n", Ind->handleId, Ind->notificationType, Ind->locationType);
    ret = atcURC(chanId, (char *)RespBuf);
    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +ECPCFUN
*
****************************************************************************************/
CmsRetId devECPCFUNInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    indStr[ATEC_IND_RESP_64_STR_LEN] = {0};
    CmiDevPowerOnCfunInd    *pCmiInd = (CmiDevPowerOnCfunInd *)paras;

    if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG) != 1)
    {
        return CMS_RET_SUCC;
    }

    snprintf(indStr, ATEC_IND_RESP_64_STR_LEN, "\r\n+ECPCFUN: %d\r\n", pCmiInd->func);

    ret = atcURC(chanId, indStr);

    return ret;
}

/******************************************************************************
 * devECPSTESTSetCnf
 * Description: Process CMI cnf msg of AT+ECPSTEST=[n]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECPSTESTSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devECPSTESTGetCnf
 * Description: Process CMI cnf msg of AT+ECPSTEST?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECPSTESTGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId                ret = CMS_FAIL;
    CHAR                    RspBuf[16];
    CmiDevGetPsTestCnf      *pGetPsTestCnf = (CmiDevGetPsTestCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if (rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+ECPSTEST: %d", pGetPsTestCnf->enablePsTest);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * devECOWERCLASSSetCnf
 * Description: Process CMI cnf msg of AT+ECPOWERCLASS=<band>,<power class>
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECOWERCLASSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devECPOWERCLASSGetCnf
 * Description: Process CMI cnf msg of AT+ECPOWERCLASS?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECPOWERCLASSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId    ret = CMS_FAIL;
    UINT8       tmpIdx = 0;
    UINT16      rspStrLen = 0;
    CHAR        tmpStrBuf[30] = {0};
    CHAR       *pResponse = PNULL;
    CmiDevGetPowerClassCnf  *pGetPowerClass = (CmiDevGetPowerClassCnf *)paras;

    if (rc == CME_SUCC)
    {
        pResponse = (CHAR *)GosAllocZeroMemory(20 * (pGetPowerClass->numOfBand));
        GosCheck(pResponse != PNULL, pResponse, 0, 0);

        for (tmpIdx = 0; tmpIdx < pGetPowerClass->numOfBand; tmpIdx++)
        {
            if (tmpIdx == 0)
            {
                snprintf(tmpStrBuf, 30, "+ECPOWERCLASS: %d,%d", pGetPowerClass->band[tmpIdx], pGetPowerClass->powerClass[tmpIdx]);
            }
            else
            {
                snprintf(tmpStrBuf, 30, "\r\n+ECPOWERCLASS: %d,%d", pGetPowerClass->band[tmpIdx], pGetPowerClass->powerClass[tmpIdx]);
            }

            rspStrLen = strlen(pResponse);
            OsaCheck(rspStrLen < 20 * (pGetPowerClass->numOfBand), rspStrLen, 20 * (pGetPowerClass->numOfBand), pGetPowerClass->numOfBand);
            strncat(pResponse, tmpStrBuf, strlen(tmpStrBuf));
            memset(tmpStrBuf, 0, 30);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pResponse);
        GosFreeMemory(&pResponse);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * devECPOWERCLASSGetCapaCnf
 * Description: Process CMI cnf msg of AT+ECPOWERCLASS=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECPOWERCLASSGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId    ret = CMS_FAIL;
    UINT8       tmpIdx = 0;
    UINT16      rspStrLen = 0;
    CHAR        tmpStrBuf[30] = {0};
    CHAR       *pResponse = PNULL;
    CmiDevGetPowerClassCapaCnf *getPowerClassConf = (CmiDevGetPowerClassCapaCnf *)paras;

    if (rc == CME_SUCC)
    {
        pResponse = (CHAR *)GosAllocZeroMemory(30 * (getPowerClassConf->numOfBand));
        GosCheck(pResponse != PNULL, pResponse, 0, 0);

        for (tmpIdx = 0; tmpIdx < getPowerClassConf->numOfBand && tmpIdx < CMI_DEV_SUPPORT_MAX_BAND_NUM; tmpIdx++)
        {
            memset(tmpStrBuf, 0, 30);

            if (tmpIdx == 0)
            {
                snprintf(tmpStrBuf, 30, "+ECPOWERCLASS: %d", getPowerClassConf->band[tmpIdx]);
            }
            else
            {
                snprintf(tmpStrBuf, 30, "\r\n+ECPOWERCLASS: %d", getPowerClassConf->band[tmpIdx]);
            }
            rspStrLen = strlen(pResponse);
            OsaCheck(rspStrLen < 30 * (getPowerClassConf->numOfBand), rspStrLen, 30 * (getPowerClassConf->numOfBand), getPowerClassConf->numOfBand);
            strncat(pResponse, tmpStrBuf, strlen(tmpStrBuf));
            memset(tmpStrBuf, 0, 30);

            /*
            #define UE_POWER_CLASS_3_BIT_OFFSET                  (1 << 0)
            #define UE_POWER_CLASS_5_BIT_OFFSET                  (1 << 1)
            #define UE_POWER_CLASS_6_BIT_OFFSET                  (1 << 2)
            */
            if (0x01 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(3)");
            }
            else if (0x02 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(5)");
            }
            else if (0x03 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(3,5)");
            }
            else if (0x04 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(6)");
            }
            else if (0x05 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(3,6)");
            }
            else if (0x06 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(5,6)");
            }
            else if (0x07 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                snprintf(tmpStrBuf, 30, ",(3,5,6)");
            }

            rspStrLen = strlen(pResponse);
            OsaCheck(rspStrLen < 30 * (getPowerClassConf->numOfBand), rspStrLen, 30 * (getPowerClassConf->numOfBand), getPowerClassConf->numOfBand);
            strncat(pResponse, tmpStrBuf, strlen(tmpStrBuf));
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pResponse);
        GosFreeMemory(&pResponse);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}


/******************************************************************************
 * devECEVENTMODESetCnf
 * Description: Process CMI cnf msg of AT+ECEVENTSTATIS=<mode>
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECEVENTMODESetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * devECEVENTSTATUSGetCnf
 * Description: Process CMI cnf msg of AT+ECEVENTSTATIS?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECEVENTSTATUSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    *pIndBuf = PNULL;
    BOOL    eventMode = FALSE;
    CmiDevEmmRrcEventStatisCnf  *pStatisInd = (CmiDevEmmRrcEventStatisCnf*)paras;

    OsaDebugBegin(pStatisInd != PNULL, pStatisInd, 0, 0);
    return ret;
    OsaDebugEnd();

    pIndBuf = (CHAR *)OsaAllocZeroMemory(ATEC_IND_RESP_256_STR_LEN);

    OsaDebugBegin(pIndBuf != PNULL, ATEC_IND_RESP_128_STR_LEN, 0, 0);
    return ret;
    OsaDebugEnd();

    eventMode = pStatisInd->emmEventStatisCnf.eventMode;
    /*
     * Event statis mode
    */
    snprintf(pIndBuf, ATEC_IND_RESP_32_STR_LEN, "+ECEVENTSTATIS: mode: %d", eventMode);
    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pIndBuf);

    /* RRC
     * +ECEVENTSTATIS: RRC, ConEstSucc: %d, ConEstFail: %d */
    snprintf(pIndBuf, ATEC_IND_RESP_64_STR_LEN,
             "+ECEVENTSTATIS: RRC, ConEstSucc:%d, ConEstFail:%d",
             pStatisInd->rrcEventStatisCnf.numRrcConnEstSucc, pStatisInd->rrcEventStatisCnf.numRrcConnEstFail);
    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pIndBuf);

    /* EMM
     * +ECEVENTSTATIS: EMM, AttachSucc: %d, AttachFail: %d, TAUSucc: %d, TAUFail: %d, SRSucc: %d, SRFail: %d, AuthFail: %d, DetachNum: %d */
    snprintf(pIndBuf, ATEC_IND_RESP_256_STR_LEN,
             "+ECEVENTSTATIS: EMM, AttachSucc:%d, AttachFail:%d, TAUSucc:%d, TAUFail:%d, SRSucc:%d, SRFail:%d, AuthFail:%d, DetachNum:%d",
             pStatisInd->emmEventStatisCnf.numAttachSucc, pStatisInd->emmEventStatisCnf.numAttachFail, pStatisInd->emmEventStatisCnf.numTauSucc,
             pStatisInd->emmEventStatisCnf.numTauFail, pStatisInd->emmEventStatisCnf.numSrSucc, pStatisInd->emmEventStatisCnf.numSrFail,
             pStatisInd->emmEventStatisCnf.numAuthFail, pStatisInd->emmEventStatisCnf.numDetach);
    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pIndBuf);

    /* PLMN
     * +ECEVENTSTATIS: PLMN, OOS:%d
    */
    snprintf(pIndBuf, ATEC_IND_RESP_32_STR_LEN, "+ECEVENTSTATIS: PLMN, OOS:%d", pStatisInd->emmEventStatisCnf.numOOS);
    ret = atcReply(reqHandle, AT_RC_OK, 0, pIndBuf);

    OsaFreeMemory(&pIndBuf);
    return ret;
}



/******************************************************************************
 * devECNBR14GetCnf
 * Description: Process CMI cnf msg of AT+ECNBR14
 * input: UINT16    reqHandle;   //req handler
 *        UINT16    rc;  //
 *        void      *paras;
 * output: CmsRetId
 * Comment:
******************************************************************************/
CmsRetId devECNBR14GetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiDevGetNBRelFeatureCnf *pCmiGetRelCnf = (CmiDevGetNBRelFeatureCnf *)paras;

    if (rc == CME_SUCC)
    {
        //sprintf((char*)RspBuf, "+CFUN: %d", func);
        /*
         * +ECNBR14: UeRelVersion,%d,UeR14UpRai,%d
         * \r\n+ECNBR14: TwoHarq,%d,R14UpRai,%d,NonAnchorNPRACH,%d,NonAnchorPaging,%d,CpReest,%d
        */
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+ECNBR14: UeRelVersion,%d,UeR14UpRai,%d",
                 pCmiGetRelCnf->ueRelVer, pCmiGetRelCnf->ueR14UpRaiCap);
        atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);

        memset(pRspStr, 0x00, ATEC_IND_RESP_128_STR_LEN);
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+ECNBR14: TwoHarq,%d,R14UpRai,%d,NonAnchorNPRACH,%d,NonAnchorPaging,%d,CpReest,%d",
                 pCmiGetRelCnf->r14TwoHarqEnabled, pCmiGetRelCnf->r14UpRaiEnabled, pCmiGetRelCnf->r14NonAnchorNPrach,
                 pCmiGetRelCnf->r14NonAnchorPaging, pCmiGetRelCnf->r14CpReest);

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * devECPSSLPCFGGetCnf
 * Description: Process CMI cnf msg of AT+ECPSSLPCFG?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECPSSLPCFGGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId                ret = CMS_FAIL;
    CHAR                    RspBuf[128];
    CmiDevGetPsSleepCfgCnf  *pGetPsSlpCfgCnf = (CmiDevGetPsSleepCfgCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if (rc == CME_SUCC)
    {
        snprintf((char*)RspBuf, 128,
                 "+ECPSSLPCFG: \"minT3324\",%d,\"minT3412\",%d,\"minEDRXPeriod\",%d",
                 pGetPsSlpCfgCnf->minT3324, pGetPsSlpCfgCnf->minT3412, pGetPsSlpCfgCnf->minEdrxPeriod);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * devECPSSLPCFGSetCnf
 * Description: Process CMI cnf msg of AT+ECPSSLPCFG=...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId devECPSSLPCFGSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}


/******************************************************************************
 ******************************************************************************
 * static func table
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_VARIAVLE__ //just for easy to find this position in SS


/******************************************************************************
  * devCmiCnfHdrList
  * const table, should store in FLASH memory
 ******************************************************************************/
 static const CmiCnfFuncMapList devCmiCnfHdrList[] =
 {
     {CMI_DEV_SET_CFUN_CNF,                     devCFUNSetCnf},
     {CMI_DEV_GET_CFUN_CNF,                     devCFUNGetCnf},
     {CMI_DEV_GET_CFUN_CAPA_CNF,                devCFUNGetCapaCnf},
     {CMI_DEV_SET_CIOT_BAND_CNF,                devECBANDSetCnf},
     {CMI_DEV_GET_CIOT_BAND_CNF,                devECBANDGetCnf},
     {CMI_DEV_GET_CIOT_BAND_CAPA_CNF,           devECBandGetCapaCnf},
     {CMI_DEV_SET_CIOT_FREQ_CNF,                devCiotfreqSetCnf},
     {CMI_DEV_GET_CIOT_FREQ_CNF,                devCiotfreqGetCnf},
     {CMI_DEV_SET_POWER_STATE_CNF,              devPowerStateSetCnf},
     {CMI_DEV_SET_EXT_CFG_CNF,                  devECCFGSetCnf},
     {CMI_DEV_GET_EXT_CFG_CNF,                  devECCFGGetCnf},
     {CMI_DEV_REMOVE_FPLMN_CNF,                 devECRMFPLMNSetCnf},
     {CMI_DEV_SET_CMOLR_CNF,                    devCMOLRSetCnf},
     {CMI_DEV_GET_CMOLR_CNF,                    devCMOLRGetCnf},
     {CMI_DEV_GET_CMOLR_CAPA_CNF,               devCMOLRGetCapaCnf},
     {CMI_DEV_SET_CMTLR_CNF,                    devCMTLRSetCnf},
     {CMI_DEV_GET_CMTLR_CNF,                    devCMTLRGetCnf},
     {CMI_DEV_GET_CMTLR_CAPA_CNF,               devCMTLRGetCapaCnf},
     {CMI_DEV_SET_CMTLRA_CNF,                   devCMTLRASetCnf},
     {CMI_DEV_GET_CMTLRA_CNF,                   devCMTLRAGetCnf},
     {CMI_DEV_GET_CMTLRA_CAPA_CNF,              devCMTLRAGetCapaCnf},
     {CMI_DEV_GET_EXT_STATUS_CNF,               devECSTATUSGetCnf},
     {CMI_DEV_SET_EXT_STATIS_MODE_CNF,          devECSTATISSetCnf},
     {CMI_DEV_GET_BASIC_CELL_LIST_INFO_CNF,     devECBCINFOGetCnf},
     {CMI_DEV_SET_ECPSTEST_CNF,                 devECPSTESTSetCnf},
     {CMI_DEV_GET_ECPSTEST_CNF,                 devECPSTESTGetCnf},
     {CMI_DEV_SET_ECPOWERCLASS_CNF,             devECOWERCLASSSetCnf},
     {CMI_DEV_GET_ECPOWERCLASS_CNF,             devECPOWERCLASSGetCnf},
     {CMI_DEV_GET_ECPOWERCLASS_CAPA_CNF,        devECPOWERCLASSGetCapaCnf},
     {CMI_DEV_GET_ECEVENTSTATIS_STATUS_CNF,     devECEVENTSTATUSGetCnf},
     {CMI_DEV_SET_ECEVENTSTATIS_MODE_CNF,       devECEVENTMODESetCnf},
     {CMI_DEV_GET_NB_REL_FEATURE_CNF,           devECNBR14GetCnf},
     {CMI_DEV_GET_PSSLP_CFG_CNF,                devECPSSLPCFGGetCnf},
     {CMI_DEV_SET_PSSLP_CFG_CNF,                devECPSSLPCFGSetCnf},
     {CMI_DEV_PRIM_BASE,                        PNULL}  /* this should be the last */
 };


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/******************************************************************************
 * atDevProcCmiCnf
 * Description: Process CMI cnf msg for dev sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atDevProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (devCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (devCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = devCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }
    else
    {
        OsaDebugBegin(FALSE, primId, AT_GET_CMI_SG_ID(pCmiCnf->header.cnfId), 0);
        OsaDebugEnd();
    }

    return;
}

/******************************************************************************
 * atDevProcCmiInd
 * Description: Process CMI Ind msg for dev sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atDevProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32   chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    if (pCmiInd->header.reqHandler != BROADCAST_IND_HANDLER)
    {
        chanId = AT_GET_HANDLER_CHAN_ID(pCmiInd->header.reqHandler);

        switch (primId)
        {
            case CMI_DEV_CMTLR_IND:
                devCMTLRInd(chanId, pCmiInd->body);
                break;

            case CMI_DEV_EXT_STATIS_IND:
                devECSTATISInd(chanId, pCmiInd->body);
                break;

            case CMI_DEV_GET_BASIC_CELL_LIST_INFO_IND:
                devECBCINFOInd(chanId, pCmiInd->body);
                break;

            default:
                OsaDebugBegin(FALSE, primId, pCmiInd->header.reqHandler, 0);
                OsaDebugEnd();
                break;
        }
    }
    else //broadcast URC
    {
        for (chanId = 1; chanId < CMS_CHAN_NUM; chanId++)
        {
            if (atcBeURCConfiged(chanId) != TRUE)
            {
                continue;
            }

            switch (primId)
            {
                case CMI_DEV_POWER_ON_CFUN_IND:
                    devECPCFUNInd(chanId, pCmiInd->body);
                    break;

                case CMI_DEV_ERRC_EXIT_DEACT_IND:
                    /* Do nothing */
                    break;

                default:
                    OsaDebugBegin(FALSE, primId, pCmiInd->header.reqHandler, 0);
                    OsaDebugEnd();
                    break;
            }
        }
    }


}



