/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/

#ifdef  FEATURE_REF_AT_ENABLE
#include <stdio.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atec_ref_ps_cnf_ind.h"
#include "cmidev.h"
#include "cmimm.h"
#include "cmips.h"
#include "cmisim.h"
#include "cmisms.h"
#include "atec_ref_ps.h"

#include "mm_debug.h"//add for memory leak debug

/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

#ifdef HEAP_MEM_DEBUG
extern size_t memDebugAllocated;
extern size_t memDebugFree;
extern size_t memDebugMaxFree;
extern size_t memDebugNumAllocs;
extern size_t memDebugNumFrees;
#endif


/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS


/******************************************************************************
 * refPsNBANDSetCnf
 * Description: Process CMI cnf msg of AT+NBAND=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNBANDSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * refPsNBANDGetCnf
 * Description: Process CMI cnf msg of AT+NBAND?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNBANDGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0}, tmpStrBuf[16] = {0};
    CmiDevGetCiotBandCnf *pCmiCnf = (CmiDevGetCiotBandCnf *)paras;
    UINT8   tmpIdx = 0;
    UINT16  rspStrLen = 0;

    if (rc == CME_SUCC)
    {
        /*
         * +NBAND: (<band>s)
        */
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+NBAND: ");

        for (tmpIdx = 0; tmpIdx < pCmiCnf->bandNum && tmpIdx < CMI_DEV_SUPPORT_MAX_BAND_NUM; tmpIdx++)
        {
            if (tmpIdx == 0)
            {
                snprintf(tmpStrBuf, 16, "%d", pCmiCnf->orderedBand[tmpIdx]);
            }
            else
            {
                snprintf(tmpStrBuf, 16, ",%d", pCmiCnf->orderedBand[tmpIdx]);
            }

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
 * refPsNBANDGetCapaCnf
 * Description: Process CMI cnf msg of AT+NBAND=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNBANDGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0}, tmpStrBuf[16] = {0};
    CmiDevGetCiotBandCapaCnf *getDevCbandConf = (CmiDevGetCiotBandCapaCnf *)paras;
    UINT8   tmpIdx = 0;
    UINT16  rspStrLen = 0;

    if (rc == CME_SUCC)
    {
        /*
         * +NBAND: (<band>s)
        */
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+NBAND: ");

        for (tmpIdx = 0; tmpIdx < getDevCbandConf->bandNum && tmpIdx < CMI_DEV_SUPPORT_MAX_BAND_NUM; tmpIdx++)
        {
            if (tmpIdx == 0)
            {
                snprintf(tmpStrBuf, 16, "(%d", getDevCbandConf->supportBand[tmpIdx]);
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
 * refPsNCFGSetCnf
 * Description: Process CMI cnf msg of AT+NCONFIG=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNCFGSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * refPsNCFGGetCnf
 * Description: Process CMI cnf msg of AT+NCONFIG? and AT+NCPCDPR?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNCFGGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiDevGetExtCfgCnf *pGetEcfgCnf = (CmiDevGetExtCfgCnf *)paras;
    CHAR pRspStr[ATEC_IND_RESP_256_STR_LEN] = {0}, tmpStrBuf[60] = {0};
    UINT16      rspStrLen = 0;

    if (rc == CME_SUCC)
    {
        if (AT_GET_HANDLER_SUB_ATID(reqHandle) == CMS_REF_SUB_AT_1_ID)
        {
            /*
             * 1) +NCONFIG: AUTOCONNECT, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (CMI_DEV_FULL_FUNC == pGetEcfgCnf->powerCfun)
            {
                snprintf((char*)tmpStrBuf, 60, "+NCONFIG:AUTOCONNECT,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "+NCONFIG:AUTOCONNECT,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 2) +NCONFIG: CELL_RESELECTION, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableCellResel)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:CELL_RESELECTION,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:CELL_RESELECTION,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 3) +NCONFIG: ENABLE_BIP, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableBip)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:ENABLE_BIP,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:ENABLE_BIP,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 4) +NCONFIG: MULTITONE, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->multiTone)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:MULTITONE,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:MULTITONE,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /* Print once every 4 lines */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);

            memset(pRspStr, 0, ATEC_IND_RESP_256_STR_LEN);

            /*
             * 5) +NCONFIG: NAS_SIM_POWER_SAVING_ENABLE, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableSimPsm)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:NAS_SIM_POWER_SAVING_ENABLE,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:NAS_SIM_POWER_SAVING_ENABLE,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 6) +NCONFIG: RELEASE_VERSION, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:RELEASE_VERSION,%d", pGetEcfgCnf->relVersion);

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);


            /*
             * 7) +NCONFIG: IPV6_GET_PREFIX_TIME, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:IPV6_GET_PREFIX_TIME,%d", pGetEcfgCnf->ipv6GetPrefixTime);

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);


            /*
             * 8) +NCONFIG: NB_CATEGORY, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:NB_CATEGORY,%d", pGetEcfgCnf->nbCategory);

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /* Print once every 4 lines */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);

            memset(pRspStr, 0, ATEC_IND_RESP_256_STR_LEN);

            /*
             * 9) +NCONFIG: RAI, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->supportUpRai)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:RAI,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:RAI,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 10) +NCONFIG: HEAD_COMPRESS, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->bRohc)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:HEAD_COMPRESS,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:HEAD_COMPRESS,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 11) +NCONFIG: CONNECTION_REESTABLISHMENT, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableConnReEst)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:CONNECTION_REESTABLISHMENT,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:CONNECTION_REESTABLISHMENT,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 12) +NCONFIG: TWO_HARQ, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->supportTwoHarq)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:TWO_HARQ,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:TWO_HARQ,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /* Print once every 4 lines */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);

            memset(pRspStr, 0, ATEC_IND_RESP_256_STR_LEN);

            /*
             * 13) +NCONFIG: PCO_IE_TYPE, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableEpco)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:PCO_IE_TYPE,EPCO");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:PCO_IE_TYPE,PCO");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 14) +NCONFIG: T3324_T3412_EXT_CHANGE_REPORT, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->t3324AndT3412ExtCeregUrc)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:T3324_T3412_EXT_CHANGE_REPORT,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:T3324_T3412_EXT_CHANGE_REPORT,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 15) +NCONFIG: NON_IP_NO_SMS_ENABLE, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableNonIpNoSms)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:NON_IP_NO_SMS_ENABLE,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:NON_IP_NO_SMS_ENABLE,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /*
             * 16) +NCONFIG: SUPPORT_SMS, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->supportSms)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:SUPPORT_SMS,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:SUPPORT_SMS,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /* Print once every 4 lines */
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);

            memset(pRspStr, 0, ATEC_IND_RESP_256_STR_LEN);

            /*
             * 17) +NCONFIG: HPPLMN_SEARCH_ENABLE, xxxx,
            */
            memset(tmpStrBuf, 0, 60);

            if (pGetEcfgCnf->enableHPPlmnSearch)
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:HPPLMN_SEARCH_ENABLE,TRUE");
            }
            else
            {
                snprintf((char*)tmpStrBuf, 60, "\n+NCONFIG:HPPLMN_SEARCH_ENABLE,FALSE");
            }

            rspStrLen = strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_256_STR_LEN, rspStrLen, ATEC_IND_RESP_256_STR_LEN, 0);
            strncat(pRspStr, tmpStrBuf, ATEC_IND_RESP_256_STR_LEN-rspStrLen-1);

            /* Print the last line */
            ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
        }
        else if (AT_GET_HANDLER_SUB_ATID(reqHandle) == CMS_REF_SUB_AT_2_ID)
        {
            snprintf((char*)pRspStr, 60, "+NCPCDPR: 0, %d", pGetEcfgCnf->dnsIpv4AddrReadCfg);

            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);

            snprintf((char*)pRspStr, 60, "+NCPCDPR: 1, %d", pGetEcfgCnf->dnsIpv6AddrReadCfg);

            ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);

        }
        else
        {
            OsaDebugBegin(FALSE, AT_GET_HANDLER_SUB_ATID(reqHandle), 0, 0);
            OsaDebugEnd();
        }

    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}



/******************************************************************************
 * refPsNUESTATSGetCnf
 * Description: Process CMI cnf msg of AT+NUESTATS?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNUESTATSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_DEV_NUESTATS_CNF_STR_LEN    600
    CmsRetId    ret = CMS_FAIL;
    CHAR        *pCnfStr = PNULL;

    CmiDevGetExtStatusCnf   *pCmiCnf = (CmiDevGetExtStatusCnf *)paras;
    CHAR                     tmpStrBuf[100] = {0};
    UINT16                   cnfStrLen = 0;
    UINT8                    mapNbOperMode = 0;
    UINT8                    i;

    if (rc != CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
        return ret;
    }

    OsaCheck(pCmiCnf != PNULL, pCmiCnf, 0, 0);
    pCnfStr = (CHAR *)OsaAllocZeroMemory(ATEC_DEV_NUESTATS_CNF_STR_LEN);

    OsaDebugBegin(pCnfStr != PNULL, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0, 0);
    return ret;
    OsaDebugEnd();

    /*0 Unknown mode
          1 Inband different PCI mode
          2 Inband same PCI mode
          3 Guardband mode
          4 Standalone mode */
    if (CMI_DEV_EM_NB_INBAND_DIFF_PCI_MODE == pCmiCnf->phyStatus.nbOperMode)
    {
        mapNbOperMode = 1;
    }
    else if (CMI_DEV_EM_NB_INBAND_SAME_PCI_MODE == pCmiCnf->phyStatus.nbOperMode)
    {
        mapNbOperMode = 2;
    }
    else if (CMI_DEV_EM_NB_GUARD_BAND_MODE == pCmiCnf->phyStatus.nbOperMode)
    {
        mapNbOperMode = 3;
    }
    else if (CMI_DEV_EM_NB_STAND_ALONE_MODE == pCmiCnf->phyStatus.nbOperMode)
    {
        mapNbOperMode = 4;
    }

    if (CMI_DEV_GET_NUESTATS == pCmiCnf->getStatusType)
    {
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)pCnfStr, ATEC_DEV_NUESTATS_CNF_STR_LEN, "Signal power:%d\nTotal power:%d\nTX power:%d\nTX time:%d\nRX time:%d\nCell ID:%d\nECL:%d\nSNR:%d\nEARFCN:%d\nPCI:%d\nRSRQ:%d\nOPERATOR MODE:%d\nCURRENT BAND:%d",
                     (pCmiCnf->phyStatus.sRsrp/10),
                     (pCmiCnf->phyStatus.totalPower/10),
                      pCmiCnf->phyStatus.txPower,
                      pCmiCnf->phyStatus.txTime,
                      pCmiCnf->phyStatus.rxTime,
                      pCmiCnf->rrcStatus.cellId,
                      pCmiCnf->phyStatus.ceLevel,
                      pCmiCnf->phyStatus.snr,
                      pCmiCnf->phyStatus.dlEarfcn,
                      pCmiCnf->phyStatus.phyCellId,
                      pCmiCnf->phyStatus.sRsrq,
                      mapNbOperMode,
                      pCmiCnf->phyStatus.band);
        }
        else
        {
            //It means ue in cfun0 or cfun4 state, print the same default value as Ref AT.
            snprintf((char*)pCnfStr, ATEC_DEV_NUESTATS_CNF_STR_LEN, "Signal power:%d\nTotal power:%d\nTX power:%d\nTX time:%d\nRX time:%d\nCell ID:%u\nECL:%d\nSNR:%d\nEARFCN:%u\nPCI:%d\nRSRQ:%d\nOPERATOR MODE:%d\nCURRENT BAND:%d",
                      -32768,
                      -32768,
                      -32768,
                      0,
                      0,
                      0xFFFFFFFF,
                      0xFF,
                      -32768,
                      0xFFFFFFFF,
                      0xFFFF,
                      -32768,
                      0,
                      0xFF);
          }
    }

    if (CMI_DEV_GET_NUESTATS_RADIO == pCmiCnf->getStatusType ||
        CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
    {
        /* Print the 4 lines below */
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,Signal power:%d\r\n", (pCmiCnf->phyStatus.sRsrp/10));
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,Signal power:%d\r\n", -32768);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,Total power:%d\r\n", (pCmiCnf->phyStatus.totalPower/10));
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,Total power:%d\r\n", -32768);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,TX power:%d\r\n", pCmiCnf->phyStatus.txPower);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,TX power:%d\r\n", -32768);
        }

        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,TX time:%d\r\n", pCmiCnf->phyStatus.txTime);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,TX time:%d\r\n", 0);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);

        /* Print the 4 lines below */
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,RX time:%d\r\n", pCmiCnf->phyStatus.rxTime);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,RX time:%d\r\n", 0);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,Cell ID:%d\r\n", pCmiCnf->rrcStatus.cellId);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,Cell ID:%u\r\n", 0xFFFFFFFF);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,ECL:%d\r\n", pCmiCnf->phyStatus.ceLevel);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,ECL:%d\r\n", 0xFF);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,SNR:%d\r\n", pCmiCnf->phyStatus.snr * 10);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,SNR:%d\r\n", -32768);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);

        /* Print the 5 lines below */
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,EARFCN:%d\r\n", pCmiCnf->phyStatus.dlEarfcn);
        }
        else
        {

            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,EARFCN:%u\r\n", 0xFFFFFFFF);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,PCI:%d\r\n", pCmiCnf->phyStatus.phyCellId);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,PCI:%d\r\n", 0xFFFF);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,RSRQ:%d\r\n", pCmiCnf->phyStatus.sRsrq/10);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,RSRQ:%d\r\n", -32768);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,OPERATOR MODE:%d\r\n", mapNbOperMode);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,OPERATOR MODE:%d\r\n", 0);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        if (pCmiCnf->bPowerOn)
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,CURRENT BAND:%d\r\n", pCmiCnf->phyStatus.band);
        }
        else
        {
            snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:RADIO,CURRENT BAND:%d\r\n", 0xFF);
        }
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        if (CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
        {
            ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);
        }
    }

    if (CMI_DEV_GET_NUESTATS_BLER == pCmiCnf->getStatusType ||
        CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
    {
        /* Print the 5 lines below */
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,RLC UL BLER,%d\r\n", pCmiCnf->l2Status.rlcUlBler);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,RLC DL BLER,%d\r\n", pCmiCnf->l2Status.rlcDlBler);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,MAC UL BLER,%d\r\n", pCmiCnf->phyStatus.ulBler);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,MAC DL BLER,%d\r\n", pCmiCnf->phyStatus.dlBler);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,Total TX bytes,%d\r\n", pCmiCnf->phyStatus.totalTxBytes);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);

        /* Print the 5 lines below */
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,Total RX bytes,%d\r\n", pCmiCnf->phyStatus.totalRxBytes);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,Total TX blocks,%d\r\n", pCmiCnf->phyStatus.totalTxBlocks);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,Total RX blocks,%d\r\n", pCmiCnf->phyStatus.totalRxBlocks);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,Total RTX blocks,%d\r\n", pCmiCnf->phyStatus.retransTxBlocks);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:BLER,Total ACK/NACK RX,%d\r\n", pCmiCnf->phyStatus.retransRxBlocks);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        if (CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
        {
            ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);
        }
    }

    if (CMI_DEV_GET_NUESTATS_THP == pCmiCnf->getStatusType ||
        CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
    {
        /* Print the 4 lines below */
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:THP,RLC UL,%d\r\n", pCmiCnf->l2Statis.rlcUlPduBytes*8/CMI_DEV_NUESTATS_THP_PERIOD_SECOND);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:THP,RLC DL,%d\r\n", pCmiCnf->l2Statis.rlcDlPduBytes*8/CMI_DEV_NUESTATS_THP_PERIOD_SECOND);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:THP,MAC UL,%d\r\n", pCmiCnf->l2Statis.macUlBytes*8/CMI_DEV_NUESTATS_THP_PERIOD_SECOND);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        memset(tmpStrBuf, 0, 100);
        snprintf((char*)tmpStrBuf, 100, "\r\nNUESTATS:THP,MAC DL,%d\r\n", pCmiCnf->l2Statis.macDlBytes*8/CMI_DEV_NUESTATS_THP_PERIOD_SECOND);
        cnfStrLen = strlen(pCnfStr);
        OsaCheck(cnfStrLen < ATEC_DEV_NUESTATS_CNF_STR_LEN, cnfStrLen, ATEC_DEV_NUESTATS_CNF_STR_LEN, 0);
        strncat(pCnfStr, tmpStrBuf, ATEC_DEV_NUESTATS_CNF_STR_LEN-cnfStrLen-1);

        if (CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
        {
            ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);
        }
    }

#ifdef HEAP_MEM_DEBUG
    if (CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
    {
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        snprintf((char*)pCnfStr, ATEC_DEV_NUESTATS_CNF_STR_LEN, "NUESTATS:APPSMEM,Current Allocated:%d\r\n\nNUESTATS:APPSMEM,Total Free:%d\r\n\nNUESTATS:APPSMEM,Max Free:%d\r\n\nNUESTATS:APPSMEM,Num Allocs:%d\r\n\nNUESTATS:APPSMEM,Num Frees:%d\r\n",
            memDebugAllocated, memDebugFree, memDebugMaxFree, memDebugNumAllocs, memDebugNumFrees);

        ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);
    }
#endif

    if (CMI_DEV_GET_NUESTATS_CELL == pCmiCnf->getStatusType ||
        CMI_DEV_GET_NUESTATS_ALL == pCmiCnf->getStatusType)
    {
        memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

        if (pCmiCnf->basicCellListInfo.sCellPresent)
        {
            snprintf((char*)pCnfStr, ATEC_DEV_NUESTATS_CNF_STR_LEN, "\r\nNUESTATS:CELL,%d,%d,1,%d,%d,%d,%d\r\n",
                      pCmiCnf->basicCellListInfo.sCellInfo.earfcn,
                      pCmiCnf->basicCellListInfo.sCellInfo.phyCellId,
                      pCmiCnf->basicCellListInfo.sCellInfo.rsrp * 10,
                      pCmiCnf->basicCellListInfo.sCellInfo.rsrq * 10,
                      pCmiCnf->basicCellListInfo.sCellInfo.rsrp * 10 - pCmiCnf->basicCellListInfo.sCellInfo.rsrq * 10,
                      pCmiCnf->basicCellListInfo.sCellInfo.snr * 10);
        }

        if (pCmiCnf->basicCellListInfo.nCellNum > 0)
        {
            ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);

            for (i = 0; i < pCmiCnf->basicCellListInfo.nCellNum; i++)
            {
                memset(pCnfStr, 0, ATEC_DEV_NUESTATS_CNF_STR_LEN);

                snprintf((char*)pCnfStr, ATEC_DEV_NUESTATS_CNF_STR_LEN, "\r\nNUESTATS:CELL,%d,%d,0,%d,%d,%d,%d\r\n",
                          pCmiCnf->basicCellListInfo.nCellList[i].earfcn,
                          pCmiCnf->basicCellListInfo.nCellList[i].phyCellId,
                          pCmiCnf->basicCellListInfo.nCellList[i].rsrp * 10,
                          pCmiCnf->basicCellListInfo.nCellList[i].rsrq * 10,
                          pCmiCnf->basicCellListInfo.nCellList[i].rsrp * 10 - pCmiCnf->basicCellListInfo.nCellList[i].rsrq * 10,
                          pCmiCnf->basicCellListInfo.sCellInfo.snr * 10);

                if (i != pCmiCnf->basicCellListInfo.nCellNum - 1)
                {
                    ret = atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, pCnfStr);
                }
            }
        }
    }

    ret = atcReply(reqHandle, AT_RC_OK, 0, pCnfStr);

    /* Free string memory*/
    OsaFreeMemory(&pCnfStr);

    return ret;

}


/******************************************************************************
 * refPsNPSMRGetCnf
 * Description: Process CMI cnf msg of AT+NPSMR=   get current psm mode
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNPSMRGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiMmGetPsmModeCnf *Conf = (CmiMmGetPsmModeCnf *)paras;
    CHAR atRspBuf[40] = {0};
    UINT8   n;
    UINT8   chanId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if (rc == CME_SUCC)
    {
        n = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG);

        /* The read command returns "+NPSMR:<n>" when <n> is 0,
           and return "+NPSMR:<n>,<mode>" when <n> is 1. */
        if (ATC_NPSMR_DISABLE == n)
        {
            sprintf(atRspBuf, "+NPSMR: 0");
        }
        else
        {
            sprintf(atRspBuf, "+NPSMR: 1,%d", Conf->psmMode);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, atRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;

}

/******************************************************************************
 * refPsNPTWEDRXSetCnf
 * Description: Process CMI cnf msg of AT+NPTWEDRXS=   set PTW/eDRX param
 * input: UINT16 reqHandle;   //req handler
 *        UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNPTWEDRXSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId    ret = CMS_FAIL;

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
 * refPsNPTWEDRXGetCnf
 * Description: Process CMI cnf msg of AT+NPTWEDRXS?   get PTW/eDRX param
 * input: UINT16 reqHandle;   //req handler
 *        UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNPTWEDRXGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_NPTW_EDRX_GET_CNF_STR_LEN   64
    CmsRetId    ret = CMS_FAIL;
    CHAR        rspBuf[ATEC_NPTW_EDRX_GET_CNF_STR_LEN], tempbuf[ATEC_NPTW_EDRX_GET_CNF_STR_LEN];
    CmiMmGetRequestedEdrxParmCnf *pPtwEdrxConf = (CmiMmGetRequestedEdrxParmCnf *)paras;

    if (rc == CME_SUCC)
    {
        memset(rspBuf, 0, sizeof(rspBuf));

        /* AT+NPTWEDRXS?, return:  +NPTWEDRXS: <AcT-type>,<Requested_Paging_time_window>,<Requested_eDRX_value>  */

        snprintf(rspBuf, ATEC_NPTW_EDRX_GET_CNF_STR_LEN, "+NPTWEDRXS: %d", pPtwEdrxConf->actType);

        memset(tempbuf, 0, sizeof(tempbuf));

        /* <Requested_Paging_time_window>: string type; half a byte in a 4 bit format */
        atByteToBitString((UINT8 *)tempbuf, pPtwEdrxConf->reqPtwValue, 4);
        strcat(rspBuf, ",\"");
        strcat(rspBuf, tempbuf);
        strcat(rspBuf, "\"");

        memset(tempbuf, 0, sizeof(tempbuf));

        /* <Requested_eDRX_value>: string type; half a byte in a 4 bit format */
        atByteToBitString((UINT8 *)tempbuf, pPtwEdrxConf->reqEdrxValue, 4);
        strcat(rspBuf, ",\"");
        strcat(rspBuf, tempbuf);
        strcat(rspBuf, "\"");

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of +NPSMR
*
****************************************************************************************/
CmsRetId refPsNPSMRInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    indBuf[ATEC_IND_RESP_32_STR_LEN] = {0};
    UINT32  rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG);
    CmiMmPsmChangeInd *pCmiPsmChangeInd = (CmiMmPsmChangeInd *)paras;

    if (ATC_NPSMR_ENABLE == rptMode)
    {
        memset(indBuf, 0, sizeof(indBuf));
        /*
             * +NPSMR: <indicates the power mode of MT>
            */
        snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPSMR:%d", pCmiPsmChangeInd->psmMode);

        ret = atcURC(chanId, indBuf);
    }

    return ret;
}

/****************************************************************************************
*  Process CI ind msg CMI_DEV_ERRC_EXIT_DEACT_IND
*
****************************************************************************************/
CmsRetId refPsExitDeactInd(UINT32 chanId)
{
    CmsRetId    ret = CMS_RET_SUCC;
    CHAR        indBuf[ATEC_IND_RESP_32_STR_LEN] = {0};
    UINT32      rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG);

//	uniLogFlushOut(0);
//	ECOMM_TRACE(UNILOG_PLA_APP, refPsExitDeactInd_1, P_WARNING, 2, "psmr-%d,%d", rptMode, slpManAPPGetAONFlag1());

    if ((ATC_NPSMR_ENABLE == rptMode) && (TRUE == slpManAPPGetAONFlag1()))
    {
        memset(indBuf, 0, sizeof(indBuf));
        /* Exit from deactive state,report NPSMR: 0 */
        snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPSMR: 0");

        ret = atcURC(chanId, indBuf);
    }

    slpManAPPSetAONFlag1(FALSE);

    return ret;
}


/****************************************************************************************
*  Process CI ind msg of +NPTWEDRXP
*
****************************************************************************************/
CmsRetId refPsNPTWEDRXPInd(UINT32 chanId, void *paras)
{
    CmsRetId    ret = CMS_RET_SUCC;
    CHAR        tmpStrBuf[20] = {0};
    BOOL        reqEdrxPrint = FALSE, reqPtwPrint = FALSE;
    CHAR        indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};

    UINT32  rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG);

    CmiMmEdrxDynParmInd *pCmiCedrxInd = (CmiMmEdrxDynParmInd *)paras;

    if (ATC_NPTWEDRXS_ENABLE == rptMode)
    {
        memset(indBuf, 0, sizeof(indBuf));
        /* +NPTWEDRXP: <AcT-type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>[,<NW_provided_eDRX_value>[,<Paging_time_window>]]]] */
        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+NPTWEDRXP: %d", pCmiCedrxInd->actType);

        if (pCmiCedrxInd->reqPtwPresent)
        {
            memset(tmpStrBuf, 0, sizeof(tmpStrBuf));
            atByteToBitString((UINT8 *)tmpStrBuf, pCmiCedrxInd->reqPtwValue, 4);
            strcat(indBuf, ",\"");
            strcat(indBuf, tmpStrBuf);
            strcat(indBuf, "\"");

            reqPtwPrint = TRUE;
        }

        if (pCmiCedrxInd->reqEdrxPresent)
        {
            if (reqPtwPrint == FALSE)
            {
                strcat(indBuf, ",");
                reqPtwPrint = TRUE;
            }
            memset(tmpStrBuf, 0, sizeof(tmpStrBuf));
            atByteToBitString((UINT8 *)tmpStrBuf, pCmiCedrxInd->reqEdrxValue, 4);
            strcat(indBuf, ",\"");
            strcat(indBuf, tmpStrBuf);
            strcat(indBuf, "\"");

            reqEdrxPrint = TRUE;
        }

        if (pCmiCedrxInd->nwEdrxPresent)
        {
            if (reqEdrxPrint == FALSE)
            {
                strcat(indBuf, ",");
                reqEdrxPrint = TRUE;
            }
            memset(tmpStrBuf, 0, sizeof(tmpStrBuf));
            atByteToBitString((UINT8 *)tmpStrBuf, pCmiCedrxInd->nwEdrxValue, 4);
            strcat(indBuf, ",\"");
            strcat(indBuf, tmpStrBuf);
            strcat(indBuf, "\"");

            memset(tmpStrBuf, 0, sizeof(tmpStrBuf));
            atByteToBitString((UINT8 *)tmpStrBuf, pCmiCedrxInd->nwPtw, 4);
            strcat(indBuf, ",\"");
            strcat(indBuf, tmpStrBuf);
            strcat(indBuf, "\"");
        }

        OsaCheck(strlen(indBuf) < ATEC_IND_RESP_128_STR_LEN, strlen(indBuf), ATEC_IND_RESP_128_STR_LEN, 0);

        ret = atcURC(chanId, indBuf);
    }

    return ret;
}


/******************************************************************************
 * atSimNCCIDGetConf
 * Description: Process CMI cnf msg of AT+NCCID
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simNCCIDGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32], tempBuf[22];
    CmiSimGetIccIdCnf *Conf = (CmiSimGetIccIdCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));
    memset(RspBuf, 0, sizeof(tempBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+NCCID: ");
        memcpy(tempBuf, Conf->iccidStr.data, CMI_SIM_ICCID_LEN);
        tempBuf[CMI_SIM_ICCID_LEN] = '\0';
        strcat((char *)RspBuf,(char *)tempBuf);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}


/******************************************************************************
 * refPsNEARFCNSetCnf
 * Description: Process CMI cnf msg of AT+NEARFCN=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsNEARFCNSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atSimNPINSetConf
 * Description: Process CMI cnf msg of AT+NPIN
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;  //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simNPINSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 chanId  = AT_GET_HANDLER_CHAN_ID(reqHandle);
    CHAR indBuf[ATEC_IND_RESP_32_STR_LEN] = {0};
    CmiSimOperatePinCnf *pOperatePinCnf = (CmiSimOperatePinCnf *)paras;

    /*
    * response 'OK' for the command excuted
    */
    ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);

    /*
    * process the PIN operation result and send URC
    */
    switch (rc)
    {
        case CME_SUCC:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s", "OK");
            break;

        case CME_SIM_PIN_DISABLED:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s", "ERROR PIN disabled");
            break;

        case CME_SIM_PIN_ALREADY_ENABLED:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s", "PIN already enabled");
            break;

        case CME_SIM_PIN_BLOCKED:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s", "ERROR PIN blocked");
            break;

        case CME_INCORRECT_PASSWORD:
        case CME_SIM_PUK_REQ:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s (%d)", "ERROR wrong PIN",pOperatePinCnf->remianRetries);
            break;

        case CME_SIM_PIN_WRONG_FORMAT:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s", "ERROR wrong format");
            break;

        default:
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+NPIN: %s", "ERROR");
            break;
    }

    ret = atcURC(chanId, indBuf);
    return ret;
}

/******************************************************************************
 * refPsECOWERCLASSSetCnf
 * Description: Process CMI cnf msg of AT+NPOWERCLASS=<band>,<power class>
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsECOWERCLASSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * refPsECPOWERCLASSGetCapaCnf
 * Description: Process CMI cnf msg of AT+NPOWERCLASS=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsECPOWERCLASSGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId    ret = CMS_FAIL;
    UINT8       tmpIdx = 0;
    UINT16      rspStrLen = 0;
    CHAR        tmpStrBuf[30] = {0};
    CHAR       *pResponse = PNULL;
    CmiDevGetPowerClassCapaCnf *getPowerClassConf = (CmiDevGetPowerClassCapaCnf *)paras;
    BOOL        bPowerClass3Support = FALSE;
    BOOL        bPowerClass5Support = FALSE;
    BOOL        bPowerClass6Support = FALSE;

    if (rc == CME_SUCC)
    {
        pResponse = (CHAR *)GosAllocZeroMemory(30 * (getPowerClassConf->numOfBand));
        GosCheck(pResponse != PNULL, pResponse, 0, 0);

        for (tmpIdx = 0; tmpIdx < getPowerClassConf->numOfBand && tmpIdx < CMI_DEV_SUPPORT_MAX_BAND_NUM; tmpIdx++)
        {
            memset(tmpStrBuf, 0, 30);

            if (tmpIdx == 0)
            {
                snprintf(tmpStrBuf, 30, "+NPOWERCLASS:(%d", getPowerClassConf->band[tmpIdx]);
            }
            else
            {
                snprintf(tmpStrBuf, 30, ",%d", getPowerClassConf->band[tmpIdx]);
            }

            rspStrLen = strlen(pResponse);
            OsaCheck(rspStrLen < 30 * (getPowerClassConf->numOfBand), rspStrLen, 30 * (getPowerClassConf->numOfBand), getPowerClassConf->numOfBand);
            strncat(pResponse, tmpStrBuf, strlen(tmpStrBuf));
            memset(tmpStrBuf, 0, 20);

            /*
            #define UE_POWER_CLASS_3_BIT_OFFSET                  (1 << 0)
            #define UE_POWER_CLASS_5_BIT_OFFSET                  (1 << 1)
            #define UE_POWER_CLASS_6_BIT_OFFSET                  (1 << 2)
            */
            if (0x01 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass3Support = TRUE;
            }
            else if (0x02 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass5Support = TRUE;
            }
            else if (0x03 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass3Support = TRUE;
                bPowerClass5Support = TRUE;
            }
            else if (0x04 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass6Support = TRUE;
            }
            else if (0x05 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass3Support = TRUE;
                bPowerClass6Support = TRUE;
            }
            else if (0x06 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass5Support = TRUE;
                bPowerClass6Support = TRUE;
            }
            else if (0x07 == getPowerClassConf->powerClass[tmpIdx] & 0x07)
            {
                bPowerClass3Support = TRUE;
                bPowerClass5Support = TRUE;
                bPowerClass6Support = TRUE;
            }
        }

        if (bPowerClass3Support == TRUE &&
            bPowerClass5Support == TRUE &&
            bPowerClass6Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(3,5,6)");
        }
        else if (bPowerClass3Support == TRUE &&
                 bPowerClass5Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(3,5)");
        }
        else if (bPowerClass3Support == TRUE &&
                 bPowerClass6Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(3,6)");
        }
        else if (bPowerClass5Support == TRUE &&
                 bPowerClass6Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(5,6)");
        }
        else if (bPowerClass3Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(3)");
        }
        else if (bPowerClass3Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(3)");
        }
        else if (bPowerClass5Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(5)");
        }
        else if (bPowerClass6Support == TRUE)
        {
            snprintf(tmpStrBuf, ATEC_IND_RESP_128_STR_LEN, "),(6)");
        }

        rspStrLen = strlen(pResponse);
        OsaCheck(rspStrLen < 30 * (getPowerClassConf->numOfBand), rspStrLen, 30 * (getPowerClassConf->numOfBand), getPowerClassConf->numOfBand);
        strncat(pResponse, tmpStrBuf, strlen(tmpStrBuf));
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pResponse);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}


/******************************************************************************
 * refPsECPOWERCLASSGetCnf
 * Description: Process CMI cnf msg of AT+NPOWERCLASS?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId refPsECPOWERCLASSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
                snprintf(tmpStrBuf, 30, "+NPOWERCLASS:%d,%d", pGetPowerClass->band[tmpIdx], pGetPowerClass->powerClass[tmpIdx]);
            }
            else
            {
                snprintf(tmpStrBuf, 30, "\r\n+NPOWERCLASS:%d,%d", pGetPowerClass->band[tmpIdx], pGetPowerClass->powerClass[tmpIdx]);
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
 * simNUICCSetCnf
 * Description: Process CMI cnf msg of AT+NUICC
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simNUICCSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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

/****************************************************************************************
*  Process CI ind msg for +NUSIM:PIN/PUK REQUIRED
*
****************************************************************************************/
CmsRetId refPsSIMPINInd(UINT32 chanId, void *paras)
{
    CmsRetId    ret = CMS_RET_SUCC;
    CHAR        indBuf[32], tempBuf[32];
    CmiSimUiccPinStateInd *Ind = (CmiSimUiccPinStateInd *)paras;

    memset(indBuf, 0, sizeof(indBuf));
    memset(tempBuf, 0, sizeof(tempBuf));
    sprintf((char *)indBuf, "+NUSIM: ");

    switch(Ind->uiccPinState)
    {
        case CMI_SIM_PIN_STATE_PIN1_REQUIRED:
        case CMI_SIM_PIN_STATE_UNIVERSALPIN_REQUIRED:
            sprintf((char*)tempBuf, "PIN REQUIRED");
            break;

        case CMI_SIM_PIN_STATE_UNBLOCK_PIN1_REQUIRED:
        case CMI_SIM_PIN_STATE_UNBLOCK_UNIVERSALPIN_REQUIRED:
            sprintf((char*)tempBuf, "PUK REQUIRED");
            break;

        case CMI_SIM_PIN_STATE_UNBLOCK_PIN1_BLOCKED:
        case CMI_SIM_PIN_STATE_UNBLOCK_UNIVERSALPIN_BLOCKED:
            sprintf((char*)tempBuf, "PUK BLOCKED");
            break;

        default:
            return ret;
    }
    strcat((char *)indBuf,(char *)tempBuf);
    ret = atcURC(chanId, (char*)indBuf);

    return ret;
}

/******************************************************************************
 ******************************************************************************
 * static func table
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_VARIABLE__ //just for easy to find this position in SS

/******************************************************************************
 * refDevCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
const static CmiCnfFuncMapList refDevCmiCnfHdrList[] =
{
    {CMI_DEV_SET_CIOT_BAND_CNF,                 refPsNBANDSetCnf},
    {CMI_DEV_GET_CIOT_BAND_CNF,                 refPsNBANDGetCnf},
    {CMI_DEV_GET_CIOT_BAND_CAPA_CNF,            refPsNBANDGetCapaCnf},
    {CMI_DEV_SET_EXT_CFG_CNF,                   refPsNCFGSetCnf},
    {CMI_DEV_GET_EXT_CFG_CNF,                   refPsNCFGGetCnf},
    {CMI_DEV_GET_EXT_STATUS_CNF,                refPsNUESTATSGetCnf},
    {CMI_DEV_SET_CIOT_FREQ_CNF,                 refPsNEARFCNSetCnf},
    {CMI_DEV_SET_ECPOWERCLASS_CNF,              refPsECOWERCLASSSetCnf},
    {CMI_DEV_GET_ECPOWERCLASS_CAPA_CNF,         refPsECPOWERCLASSGetCapaCnf},
    {CMI_DEV_GET_ECPOWERCLASS_CNF,              refPsECPOWERCLASSGetCnf},

    //...

    {CMI_DEV_PRIM_BASE, PNULL}   /* this should be the last */
};

/******************************************************************************
 * refMmCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
const static CmiCnfFuncMapList refMmCmiCnfHdrList[] =
{
    {CMI_MM_GET_PSM_MODE_CNF,                   refPsNPSMRGetCnf},
    {CMI_MM_SET_REQUESTED_PTW_EDRX_PARM_CNF,    refPsNPTWEDRXSetCnf},
    {CMI_MM_GET_REQUESTED_PTW_EDRX_PARM_CNF,    refPsNPTWEDRXGetCnf},

    //...

    {CMI_MM_PRIM_BASE, PNULL}   /* this should be the last */
};

/******************************************************************************
 * refPsCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
const static CmiCnfFuncMapList refPsCmiCnfHdrList[] =
{
    //...

    {CMI_PS_PRIM_BASE, PNULL}   /* this should be the last */
};

/******************************************************************************
 * refSimCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
const static CmiCnfFuncMapList refSimCmiCnfHdrList[] =
{
    {CMI_SIM_GET_ICCID_CNF, simNCCIDGetCnf},
    {CMI_SIM_OPERATE_PIN_CNF, simNPINSetCnf},
    {CMI_SIM_SET_SIM_SLEEP_CNF, simNUICCSetCnf},

    {CMI_DEV_PRIM_BASE, PNULL}   /* this should be the last */
};

/******************************************************************************
 * refSmsCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
const static CmiCnfFuncMapList refSmsCmiCnfHdrList[] =
{
    //...

    {CMI_SMS_PRIM_BASE, PNULL}   /* this should be the last */
};


#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/******************************************************************************
 * atRefPsDevProcCmiCnf
 * Description: Process CMI cnf msg for DEV sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atRefDevProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (refDevCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (refDevCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = refDevCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }

    return;
}

/******************************************************************************
 * atRefPsDevProcCmiCnf
 * Description: Process CMI cnf msg for MM sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atRefMmProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (refMmCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (refMmCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = refMmCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }

    return;
}

/******************************************************************************
 * atRefPsDevProcCmiCnf
 * Description: Process CMI cnf msg for PS sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atRefPsProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (refPsCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (refPsCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = refPsCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }

    return;
}

/******************************************************************************
 * atRefPsDevProcCmiCnf
 * Description: Process CMI cnf msg for SIM sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atRefSimProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (refSimCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (refSimCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = refSimCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }

    return;
}

/******************************************************************************
 * atRefPsDevProcCmiCnf
 * Description: Process CMI cnf msg for SMS sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atRefSmsProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (refSmsCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (refSmsCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = refSmsCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }

    return;
}




/******************************************************************************
 * atRefDevProcCmiInd
 * Description: Process CMI Ind msg for DEV sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atRefDevProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32 chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    for (chanId = 1; chanId < CMS_CHAN_NUM; chanId++)
    {
        switch (primId)
        {
            case CMI_DEV_ERRC_EXIT_DEACT_IND:
                refPsExitDeactInd(chanId);
                break;

            default:
                break;
        }

    }


}


/******************************************************************************
 * atRefMmProcCmiInd
 * Description: Process CMI Ind msg for MM sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atRefMmProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32 chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    for (chanId = 1; chanId < CMS_CHAN_NUM; chanId++)
    {
        switch (primId)
        {
            case CMI_MM_PSM_CHANGE_IND:
                //refPsNPSMRInd(chanId, pCmiInd->body);
                break;

            case CMI_MM_EDRX_DYN_PARM_IND:
                refPsNPTWEDRXPInd(chanId, pCmiInd->body);
                break;

            default:
                break;
        }

    }


}

/******************************************************************************
 * atRefPsProcCmiInd
 * Description: Process CMI Ind msg for PS sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atRefPsProcCmiInd(CacCmiInd *pCmiInd)
{
    //UINT16 primId = 0;

    //OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    //return;
    //OsaDebugEnd();

    //TBD

}


/******************************************************************************
 * atRefSimProcCmiInd
 * Description: Process CMI Ind msg for SIM sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atRefSimProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32 chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    for (chanId = 1; chanId < CMS_CHAN_NUM; chanId++)
    {
        if (atcBeURCConfiged(chanId) != TRUE)
        {
            continue;
        }

        switch (primId)
        {
            case CMI_SIM_UICC_PIN_STATE_IND:
                refPsSIMPINInd(chanId, pCmiInd->body);
                break;

            default:
                break;
        }

    }


}



/******************************************************************************
 * atRefSmsProcCmiInd
 * Description: Process CMI Ind msg for SMS sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atRefSmsProcCmiInd(CacCmiInd *pCmiInd)
{
    //UINT16 primId = 0;

    //OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    //return;
    //OsaDebugEnd();

    //TBD

}


#endif

