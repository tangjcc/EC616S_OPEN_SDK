/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_mm.c
*
*  Description: Process Network related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include "cmsis_os2.h"
#include "at_util.h"

#include "atec_mm.h"
//#include "atec_controller.h"

#include "cmimm.h"
#include "ps_mm_if.h"
#include "mw_config.h"
#include "osasys.h"
#include "atec_mm_cnf_ind.h"
#include "psdial_plmn_cfg.h"


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


/*Global variable to record signal quality, used in +CESQ */
//extern CmiMmCesqInd gCesq;       //0 means not known or not detectable

extern UINT32 atTimerSync(UINT32 srcType, UINT32 cmd, UINT32 Timer1, UINT32 Timer2, UINT32 Timer3, UINT32 Timer4);
extern BOOL pmuBWakeupFromHib(void);
extern BOOL pmuBWakeupFromSleep1(void);
BOOL pmuBWakeupFromSleep2(void);


/******************************************************************************
 * atMmCREGGetConf
 * Description: Process CMI cnf msg of AT+CREG?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCREGGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_CREG_MAX_RESP_STR_LEN  100

    CmsRetId ret = CMS_FAIL;
    UINT8   atChanId = AT_GET_HANDLER_CHAN_ID(reqHandle);
    UINT32  cregRptMode = mwGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CREG_RPT_CFG);
    CHAR    RspBuf[ATEC_CREG_MAX_RESP_STR_LEN], TempBuf[ATEC_CREG_MAX_RESP_STR_LEN];
    CmiMmGetCregCnf *getMmCregConf = (CmiMmGetCregCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if (cregRptMode > CMI_MM_CREG_LOC_REJ_INFO)
    {
        OsaDebugBegin(FALSE, cregRptMode, CMI_MM_CREG_LOC_REJ_INFO, 0);
        OsaDebugEnd();
        cregRptMode = CMI_MM_CREG_LOC_REJ_INFO;
    }

    /*
     * AT+CREG?
     * +CREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]]
     * 1: Location information elements <lac>, <ci> and <AcT>, if available, are returned
     *    only when <n>=2 and MT is registered in the network.
     * 2: The parameters[,<cause_type>,<reject_cause>], if available, are returned
     *    when <n>=3.
    */

    if (rc == CME_SUCC)
    {
        /* +CREG: <n>,<stat> */
        snprintf((char*)RspBuf, ATEC_CREG_MAX_RESP_STR_LEN, "+CREG: %d,%d", cregRptMode, getMmCregConf->state);

        /*,[<lac>],[<ci>],[<AcT>]*/
        if (cregRptMode >= CMI_MM_CREG_LOC_INFO)
        {
            if (getMmCregConf->locPresent)
            {
                memset(TempBuf, 0x00, sizeof(TempBuf));
                snprintf(TempBuf, ATEC_CREG_MAX_RESP_STR_LEN, ",\"%04X\",\"%08X\",%d", getMmCregConf->tac, getMmCregConf->celId, getMmCregConf->act);
                strcat(RspBuf, TempBuf);
            }
        }

        /* [,<cause_type>,<reject_cause>] */
        if (cregRptMode >= CMI_MM_CREG_LOC_REJ_INFO)
        {
            if (getMmCregConf->rejCausePresent)
            {
                memset(TempBuf, 0x00, sizeof(TempBuf));

                /*check whether need to fill empty: [<lac>],[<ci>],[<AcT>]*/
                if (getMmCregConf->locPresent == FALSE)
                {
                    strcat(RspBuf, ",,,");
                }

                snprintf(TempBuf, ATEC_CREG_MAX_RESP_STR_LEN, ",%d,%d", getMmCregConf->causeType, getMmCregConf->rejCause);
                strcat(RspBuf, TempBuf);
            }
        }

        OsaCheck(strlen(RspBuf) < ATEC_CREG_MAX_RESP_STR_LEN, strlen(RspBuf), ATEC_CREG_MAX_RESP_STR_LEN, 0);

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }


    return ret;
}

/******************************************************************************
 * atMmCREGGetCapaConf
 * Description: Process CMI cnf msg of AT+CREG=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCREGGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    rspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    tmpStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    BOOL    bFirst = TRUE;
    CmiMmGetCregCapaCnf *getMmCregCapaConf = (CmiMmGetCregCapaCnf *)paras;
    UINT8 bitmap = getMmCregCapaConf->bBitMap, tmpIdx = 0;

    if (rc == CME_SUCC)
    {
        snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+CREG: (");

        /* bit 0 set to 1, means suppurt n = 0 */
        for (tmpIdx = 0; tmpIdx < 8; tmpIdx++)
        {
            if ((bitmap >> tmpIdx) & 0x01)
            {
                if (bFirst)
                {
                    bFirst = FALSE;
                    snprintf(tmpStr, ATEC_IND_RESP_128_STR_LEN, "%d", tmpIdx);
                }
                else
                {
                    snprintf(tmpStr, ATEC_IND_RESP_128_STR_LEN, ",%d", tmpIdx);
                }

                /*strcat*/
                strcat(rspStr, tmpStr);
            }
        }

        strcat(rspStr, ")");

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atMmCOPSSetAutoConf
 * Description: Process CMI cnf msg of AT+COPS=0  set auto plmn
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCOPSSetAutoCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atMmCOPSManuSeleConf
 * Description: Process CMI cnf msg of AT+COPS=1/4
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCOPSManuSeleCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atMmCOPSDereConf
 * Description: Process CMI cnf msg of AT+COPS=2
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCOPSDereCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atMmCOPSSetIdConf
 * Description: Process CMI cnf msg of AT+COPS=3
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCOPSSetIdCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atMmCOPSGetOperConf
 * Description: Process CMI cnf msg of AT+COPS?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCOPSGetOperCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    rspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    operStr[CMI_MM_STR_PLMN_MAX_LENGTH] = {0};
    CmiMmGetCurOperInfoCnf  *pCmiCnf = (CmiMmGetCurOperInfoCnf *)paras;
    const DialPlmnCfgInfo   *pPlmnCfg = PNULL;

    if (rc == CME_SUCC)
    {
        /*
         *+COPS: <mode>[,<format>,<oper>[,<AcT>]]
        */
        if (pCmiCnf->uniPlmnPresent)
        {
            switch(pCmiCnf->plmnFormat)
            {
                case CMI_MM_PLMN_LONG_ALPH:
                case CMI_MM_PLMN_SHORT_ALPH:
                {
                    strncpy(operStr, (CHAR *)pCmiCnf->uniPlmn.strPlmn.strPlmn, CMI_MM_STR_PLMN_MAX_LENGTH-1);
                    break;
                }
                case CMI_MM_PLMN_NUMERIC:
                    if (CAC_IS_2_DIGIT_MNC(pCmiCnf->uniPlmn.numPlmn.mncWithAddInfo))
                    {
                        snprintf(operStr, CMI_MM_STR_PLMN_MAX_LENGTH, "%03x%02x",
                                 pCmiCnf->uniPlmn.numPlmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->uniPlmn.numPlmn.mncWithAddInfo));
                    }
                    else
                    {
                        snprintf(operStr, CMI_MM_STR_PLMN_MAX_LENGTH, "%03x%03x",
                                 pCmiCnf->uniPlmn.numPlmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->uniPlmn.numPlmn.mncWithAddInfo));
                    }
                    break;
                default:
                    OsaDebugBegin(FALSE, pCmiCnf->plmnFormat, pCmiCnf->numPlmn.mcc, pCmiCnf->numPlmn.mncWithAddInfo);
                    OsaDebugEnd();
                    break;
            }

            /* +COPS: <mode>[,<format>,<oper>[,<AcT>]] */
            snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+COPS: %d,%d,\"%s\",%d",
                     pCmiCnf->mode, pCmiCnf->plmnFormat, operStr, pCmiCnf->act);
        }
        else if (pCmiCnf->numPlmn.mcc != 0) //valid PLMN
        {
            /*
             * if user get longPLMN/shortPLMN name, but PS don't have such info;
             * In such case:
             * 1> "plmnFormat" set to the right value;
             * 2> "uniPlmnPresent" set to FALSE;
             * then, if "numPlmn" is valid, ATCMDSRV could find the PLMN name from local table according to this "numPlmn"
            */
            pPlmnCfg = psDialGetPlmnCfgByPlmn(pCmiCnf->numPlmn.mcc, pCmiCnf->numPlmn.mncWithAddInfo);
            if (pPlmnCfg != PNULL)
            {
                switch(pCmiCnf->plmnFormat)
                {
                    case CMI_MM_PLMN_LONG_ALPH:
                        strncpy(operStr, (CHAR *)pPlmnCfg->pLongName, CMI_MM_STR_PLMN_MAX_LENGTH-1);
                        break;

                    case CMI_MM_PLMN_SHORT_ALPH:
                        strncpy(operStr, (CHAR *)pPlmnCfg->pShortName, CMI_MM_STR_PLMN_MAX_LENGTH-1);
                        break;

                    case CMI_MM_PLMN_NUMERIC:
                        /* In fact, can't enter here */
                        OsaDebugBegin(FALSE, pCmiCnf->plmnFormat, pCmiCnf->uniPlmnPresent, 0);
                        OsaDebugEnd();
                        if (CAC_IS_2_DIGIT_MNC(pCmiCnf->numPlmn.mncWithAddInfo))
                        {
                            snprintf(operStr, CMI_MM_STR_PLMN_MAX_LENGTH, "%03x%02x",
                                     pCmiCnf->numPlmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->numPlmn.mncWithAddInfo));
                        }
                        else
                        {
                            snprintf(operStr, CMI_MM_STR_PLMN_MAX_LENGTH, "%03x%03x",
                                     pCmiCnf->numPlmn.mcc, CAC_GET_PURE_MNC(pCmiCnf->numPlmn.mncWithAddInfo));
                        }
                        break;
                    default:
                        OsaDebugBegin(FALSE, pCmiCnf->plmnFormat, pCmiCnf->numPlmn.mcc, pCmiCnf->numPlmn.mncWithAddInfo);
                        OsaDebugEnd();
                        break;
                }
            }

            /* +COPS: <mode>[,<format>,<oper>[,<AcT>]] */
            if (operStr[0] != 0)
            {
                snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+COPS: %d,%d,\"%s\",%d",
                         pCmiCnf->mode, pCmiCnf->plmnFormat, operStr, pCmiCnf->act);
            }
            else
            {
                snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+COPS: %d,,,%d",
                         pCmiCnf->mode, pCmiCnf->act);
            }
        }
        else //no valid PLMN
        {
            /* +COPS: <mode>[,<format>,<oper>[,<AcT>]] */
            snprintf(rspStr, ATEC_IND_RESP_128_STR_LEN, "+COPS: %d", pCmiCnf->mode);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atMmCOPSManuSerConf
 * Description: Process CMI cnf msg of AT+COPS=?   query supported operator
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCOPSManuSerCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    *pRspStr = PNULL;
    CHAR    pRspStrTemp[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    operNumPlmnStr[CMI_MM_STR_PLMN_MAX_LENGTH] = {0};
    UINT16  rspStrLen = 0;
    UINT8   idx = 0;
    CmiMmManualPlmnSearchCnf    *pCmiCnf = (CmiMmManualPlmnSearchCnf *)paras;
#ifndef FEATURE_REF_AT_ENABLE
    const DialPlmnCfgInfo       *pPlmnCfg = PNULL;
#endif

    //memset(RspBufTemp, 0, sizeof(RspBufTemp));

    if (rc == CME_SUCC)
    {
        /*
         * +COPS: [list of supported (<stat>,long alphanumeric <oper>,short
         *         alphanumeric <oper>,numeric <oper>[,<AcT>])s][,,(list of
         *         supported <mode>s),(list of supported <format>s)]
        */
        if (pCmiCnf->plmnNum > 0)
        {
            rspStrLen   = pCmiCnf->plmnNum * 64;
            pRspStr     = (CHAR*)OsaAllocZeroMemory(rspStrLen);

            if (pRspStr == NULL)
            {
                return atcReply(reqHandle, AT_RC_CME_ERROR, CME_MEMORY_FAILURE, NULL);
            }

            for (idx = 0; idx < pCmiCnf->plmnNum; idx++)
            {
                if (CAC_IS_2_DIGIT_MNC(pCmiCnf->plmnList[idx].mncWithAddInfo))
                {
                    snprintf(operNumPlmnStr, CMI_MM_STR_PLMN_MAX_LENGTH, "%03x%02x",
                             pCmiCnf->plmnList[idx].mcc, CAC_GET_PURE_MNC(pCmiCnf->plmnList[idx].mncWithAddInfo));
                }
                else
                {
                    snprintf(operNumPlmnStr, CMI_MM_STR_PLMN_MAX_LENGTH, "%03x%03x",
                             pCmiCnf->plmnList[idx].mcc, CAC_GET_PURE_MNC(pCmiCnf->plmnList[idx].mncWithAddInfo));
                }

#ifdef FEATURE_REF_AT_ENABLE
                /* For Ref AT: not dispaly long/short <oper>, <AcT> */
                if (strlen(pRspStr) == 0)
                {
                    snprintf(pRspStr, rspStrLen, "+COPS: (%d,,,\"%s\")",
                             pCmiCnf->plmnList[idx].plmnState,
                             operNumPlmnStr);
                }
                else
                {
                    memset(pRspStrTemp, 0x00, ATEC_IND_RESP_128_STR_LEN);

                    snprintf(pRspStrTemp, ATEC_IND_RESP_128_STR_LEN, ",(%d,,,\"%s\")",
                             pCmiCnf->plmnList[idx].plmnState,
                             operNumPlmnStr);

                    if (strlen(pRspStr) + strlen(pRspStrTemp) < rspStrLen)
                    {
                        strcat(pRspStr, pRspStrTemp);
                    }
                }
#else
                if (strlen((CHAR *)(pCmiCnf->plmnList[idx].longPlmn)) == 0 ||
                    strlen((CHAR *)(pCmiCnf->plmnList[idx].shortPlmn)) == 0)
                {
                    pPlmnCfg = psDialGetPlmnCfgByPlmn(pCmiCnf->plmnList[idx].mcc, pCmiCnf->plmnList[idx].mncWithAddInfo);
                }
                else
                {
                    pPlmnCfg = PNULL;
                }

                if (strlen(pRspStr) == 0)
                {
                    snprintf(pRspStr, rspStrLen, "+COPS: (%d,\"%s\",\"%s\",\"%s\",%d)",
                             pCmiCnf->plmnList[idx].plmnState,
                             strlen((CHAR *)(pCmiCnf->plmnList[idx].longPlmn)) == 0 ?
                             (pPlmnCfg != PNULL ? (CHAR *)(pPlmnCfg->pLongName) : "\0") : (CHAR *)(pCmiCnf->plmnList[idx].longPlmn),
                             strlen((CHAR *)(pCmiCnf->plmnList[idx].shortPlmn)) == 0 ?
                             (pPlmnCfg != PNULL ? (CHAR *)(pPlmnCfg->pShortName) : "\0") : (CHAR *)(pCmiCnf->plmnList[idx].shortPlmn),
                             operNumPlmnStr,
                             pCmiCnf->plmnList[idx].act);
                }
                else
                {
                    memset(pRspStrTemp, 0x00, ATEC_IND_RESP_128_STR_LEN);

                    snprintf(pRspStrTemp, ATEC_IND_RESP_128_STR_LEN, ",(%d,\"%s\",\"%s\",\"%s\",%d)",
                             pCmiCnf->plmnList[idx].plmnState,
                             strlen((CHAR *)(pCmiCnf->plmnList[idx].longPlmn)) == 0 ?
                             (pPlmnCfg != PNULL ? (CHAR *)(pPlmnCfg->pLongName) : "\0") : (CHAR *)(pCmiCnf->plmnList[idx].longPlmn),
                             strlen((CHAR *)(pCmiCnf->plmnList[idx].shortPlmn)) == 0 ?
                             (pPlmnCfg != PNULL ? (CHAR *)(pPlmnCfg->pShortName) : "\0") : (CHAR *)(pCmiCnf->plmnList[idx].shortPlmn),
                             operNumPlmnStr,
                             pCmiCnf->plmnList[idx].act);

                    if (strlen(pRspStr) + strlen(pRspStrTemp) < rspStrLen)
                    {
                        strcat(pRspStr, pRspStrTemp);
                    }
                }
#endif
            }

#ifdef FEATURE_REF_AT_ENABLE
            if (strlen(pRspStr) + strlen(",,(0-2),(2)") < rspStrLen)
            {
                strcat(pRspStr, ",,(0-2),(2)");
            }
#else
            /* [,,(list of supported <mode>s),(list of supported <format>s)] */
            if (strlen(pRspStr) + strlen(",,(0-4),(0-2)") < rspStrLen)
            {
                strcat(pRspStr, ",,(0-4),(0-2)");
            }
#endif
            ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);

            OsaFreeMemory(&pRspStr);
        }
        else    //no PLMN found
        {
#ifdef FEATURE_REF_AT_ENABLE
            ret = atcReply(reqHandle, AT_RC_OK, 0, "+COPS: ,,(0-2),(2)");
#else
            ret = atcReply(reqHandle, AT_RC_OK, 0, "+COPS: ,,(0-4),(0-2)");
#endif
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * mmCSQGetCnf
 * Description: Process CMI cnf msg of AT+CSQ
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCSQGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiMmGetCesqCnf *pCmiCnf = (CmiMmGetCesqCnf *)paras;
    CHAR    atRspBuf[40] = {0};
    UINT16  rssi = 0;
    UINT16  ber = 0;


    if (rc == CME_SUCC)
    {
        rssi = mmGetCsqRssiFromCesq(pCmiCnf->rsrp, pCmiCnf->rsrq);

        /*
         * 0...7 as RXQUAL values in the table in 3GPP TS 45.008 [20] subclause 8.2.4
         * 99 not known or not detectable
        */
        if (rssi != 99 &&
            pCmiCnf->dlBer <= 7)
        {
            ber = pCmiCnf->dlBer;
        }
        else
        {
            ber = 99;
        }

        snprintf(atRspBuf, 40, "+CSQ: %d,%d", rssi, ber);
        ret = atcReply(reqHandle, AT_RC_OK, 0, atRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }


    return ret;
}

/******************************************************************************
 * mmCESQGetCnf
 * Description: Process CMI cnf msg of AT+CESQ
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCESQGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiMmGetCesqCnf *pCmiCnf = (CmiMmGetCesqCnf *)paras;
    CHAR atRspBuf[40] = {0};
    UINT8   pRsrp = 0, pRsrq = 0;   //printable

    if (rc == CME_SUCC)
    {
        //gCesq.rsrp = pCmiCnf->rsrp;
        //gCesq.rsrq = pCmiCnf->rsrq;

        /*
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
         *
         * !!!! BUT AT CMD NOT EXTENDED !!!! STILL !!!!!
         *  0    rsrp < -140 dBm
         *  1   -140 dBm â‰?rsrp < -139 dBm
         *  2   -139 dBm â‰?rsrp < -138 dBm
         *  ...
         *  95  -46 dBm â‰?rsrp < -45 dBm
         *  96  -45 dBm â‰?rsrp < -44 dBm
         *  97  -44 dBm â‰?rsrp
         *  255 not known or not detectable
         *
        */
        if (pCmiCnf->rsrp <= 0)
        {
            pRsrp = 0;
        }
        else if (pCmiCnf->rsrp <= 97)
        {
            pRsrp = pCmiCnf->rsrp;
        }
        else
        {
            pRsrp = 255;
        }

        /*
         * 1> AS NB extended the RSRP value in TS 36.133-v14.5.0, Table 9.1.7-1/Table 9.1.24-1
         *   -30 rsrq < -34 dB
         *   -29 -34 dB <= rsrq < -33.5 dB
         *   ...
         *   -2 -20.5 dB <= rsrq < -20 dB
         *   -1 -20 dB <= rsrq < -19.5 dB
         *   0 rsrq < -19.5 dB
         *   1 -19.5 dB <= rsrq < -19 dB
         *   2 -19 dB <= rsrq < -18.5 dB
         *   ...
         *   32 -4 dB <= rsrq < -3.5 dB
         *   33 -3.5 dB <= rsrq < -3 dB
         *   34 -3 dB <= rsrq
         *   35 -3 dB <= rsrq < -2.5 dB
         *   36 -2.5 dB <= rsrq < -2
         *   ...
         *   45 2 dB <= rsrq < 2.5 dB
         *   46 2.5 dB <= rsrq
         * 2> If not valid, set to 127
         *
         * !!!!! BUT AT CMD NOT EXTENDED !!!! STILL !!!!!
         *  0   rsrq < -19.5 dB
         *  1   -19.5 dB â‰?rsrq < -19 dB
         *  2   -19 dB â‰?rsrq < -18.5 dB
         *  ...
         *  32  -4 dB â‰?rsrq < -3.5 dB
         *  33  -3.5 dB â‰?rsrq < -3 dB
         *  34  -3 dB â‰?rsrq
         *  255 not known or not detectable
        */
        if (pCmiCnf->rsrq <= 0)
        {
            pRsrq = 0;
        }
        else if (pCmiCnf->rsrq < 34)
        {
            pRsrq = pCmiCnf->rsrq;
        }
        else if (pCmiCnf->rsrq <= 46)
        {
            pRsrq = 34;
        }
        else
        {
            pRsrq = 255;
        }

        sprintf(atRspBuf, "+CESQ: 99,99,255,255,%d,%d", pRsrq, pRsrp);
        ret = atcReply(reqHandle, AT_RC_OK, 0, atRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/**
  \fn          CmsRetId mmCESQcnf(UINT16 reqHandle, UINT16 rc, void *paras)
  \brief       at cmd cnf function
  \param[in]   primId      hh
  \param[in]   reqHandle   hh
  \param[in]   rc          hh
  \param[in]   *paras      hh
  \returns     CmsRetId
*/
CmsRetId mmCESQcnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (AT_GET_HANDLER_SUB_ATID(reqHandle) == ATEC_SIGNAL_QUALITY_CESQ_SUB_AT)
    {
        ret = mmCESQGetCnf(reqHandle, rc, paras);
    }
    else if (AT_GET_HANDLER_SUB_ATID(reqHandle) == ATEC_SIGNAL_QUALITY_CSQ_SUB_AT)
    {
        ret = mmCSQGetCnf(reqHandle, rc, paras);
    }
    else
    {
        OsaDebugBegin(FALSE, AT_GET_HANDLER_SUB_ATID(reqHandle), CMI_MM_GET_EXTENDED_SIGNAL_QUALITY_CNF, 0);
        OsaDebugEnd();

        ret = mmCESQGetCnf(reqHandle, rc, paras);
    }

    return ret;
}


/******************************************************************************
 * mmECPSMRCnf
 * Description: Process CMI cnf msg of AT+ECPSMR=   get current psm mode
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/

CmsRetId mmECPSMRCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiMmGetPsmModeCnf *Conf = (CmiMmGetPsmModeCnf *)paras;
    CHAR atRspBuf[40] = {0};
    UINT8   n;
    UINT8   chanId = AT_GET_HANDLER_CHAN_ID(reqHandle);

    if(rc == CME_SUCC)
    {
        n = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_PSM_RPT_CFG);
        sprintf(atRspBuf, "+ECPSMR: %d,%d", n, Conf->psmMode);
        ret = atcReply(reqHandle, AT_RC_OK, 0, atRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;

}



/******************************************************************************
 * atMmCPSMSSetConf
 * Description: Process CMI cnf msg of AT+CPSMS=   set psm param
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCPSMSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atMmCPSMSGetConf
 * Description: Process CMI cnf msg of AT+CPSMS?   get psm param
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCPSMSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 RspBuf[CPSMS_BUF_LEN+1] = {0};
    UINT8 tempbuf[32] = {0};
    UINT8 bitmap=0;
    CmiMmGetRequestedPsmParmCnf *Conf = (CmiMmGetRequestedPsmParmCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        bitmap = Conf->reqBitmap;
        sprintf((CHAR*)RspBuf, "+CPSMS: %d", Conf->mode);
        if(bitmap & 0x01 == 0x01)
        {
            atByteToBitString(tempbuf,Conf->reqPeriodicRau,8);
#ifdef  FEATURE_REF_AT_ENABLE
            strcat((CHAR*)RspBuf, ",");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
#else
            strcat((CHAR*)RspBuf, ",\"");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
            strcat((CHAR*)RspBuf, "\"");
#endif
        }
        else
        {
            strcat((CHAR*)RspBuf, ",");
        }
        if((bitmap & 0x02) == 0x02)
        {
            atByteToBitString(tempbuf,Conf->reqGprsReadyTimer,8);
#ifdef  FEATURE_REF_AT_ENABLE
            strcat((CHAR*)RspBuf, ",");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
#else
            strcat((CHAR*)RspBuf, ",\"");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
            strcat((CHAR*)RspBuf, "\"");
#endif
        }
        else
        {
            strcat((CHAR*)RspBuf, ",");
        }
        if((bitmap & 0x04) == 0x04)
        {
            atByteToBitString(tempbuf,Conf->reqPeriodicTau,8);
#ifdef  FEATURE_REF_AT_ENABLE
            strcat((CHAR*)RspBuf, ",");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
#else
            strcat((CHAR*)RspBuf, ",\"");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
            strcat((CHAR*)RspBuf, "\"");
#endif
        }
        else
        {
            strcat((CHAR*)RspBuf, ",");
        }
        if((bitmap & 0x08) == 0x08)
        {
            atByteToBitString(tempbuf,Conf->reqActiveTime,8);
#ifdef  FEATURE_REF_AT_ENABLE
            strcat((CHAR*)RspBuf, ",");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
#else
            strcat((CHAR*)RspBuf, ",\"");
            strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
            strcat((CHAR*)RspBuf, "\"");
#endif
        } else{
            strcat((CHAR*)RspBuf, ",");
        }

        for(int i=CPSMS_BUF_LEN;i>0;i--)
        {
            if(RspBuf[i] == ',')
            {
                RspBuf[i] = 0;
            }
            else if(RspBuf[i] != 0)
            {
                break;
            }
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atMmEDRXSetConf
 * Description: Process CMI cnf msg of AT+CEDRXS=   set eDRX param
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmEDRXSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId            ret = CMS_FAIL;
    UINT8               atChanId = AT_GET_HANDLER_CHAN_ID(reqHandle);
    CmiMmSetEdrxParmCnf *pCmiSetEdrxParmCnf = (CmiMmSetEdrxParmCnf *)paras;

    if (rc == CME_SUCC)
    {
        if (pCmiSetEdrxParmCnf->edrxMode == CMI_MM_ENABLE_EDRX_AND_ENABLE_IND)
        {
            mwSetAndSaveAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG, 1);  //need to report +CEDRXP indication
        }
        else
        {
            mwSetAndSaveAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG, 0);  //disable report +CEDRXP indication
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atMmEDRXGetConf
 * Description: Process CMI cnf msg of AT+CEDRXS?   get eDRX param
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmEDRXGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_EDRX_GET_CNF_STR_LEN   64
    CmsRetId    ret = CMS_FAIL;
    CHAR        rspBuf[ATEC_EDRX_GET_CNF_STR_LEN], tempbuf[ATEC_EDRX_GET_CNF_STR_LEN];
    CmiMmGetRequestedEdrxParmCnf *pEdrxConf = (CmiMmGetRequestedEdrxParmCnf *)paras;

    if (rc == CME_SUCC)
    {
        memset(rspBuf, 0, sizeof(rspBuf));

        /*
         * From 27.007
         * AT+CEDRXS?, return:  +CEDRXS: <AcT-type>,<Requested_eDRX_value>
         */

        snprintf(rspBuf, ATEC_EDRX_GET_CNF_STR_LEN, "+CEDRXS: %d", pEdrxConf->actType);

        memset(tempbuf, 0, sizeof(tempbuf));

        /* <Requested_eDRX_value>: string type; half a byte in a 4 bit format */
        atByteToBitString((UINT8 *)tempbuf, pEdrxConf->reqEdrxValue, 4);
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


/******************************************************************************
 * mmECPTWEDRXSetCnf
 * Description: Process CMI cnf msg of AT+ECPTWEDRXS=   set PTW/eDRX param
 * input: UINT16 reqHandle;   //req handler
 *        UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmECPTWEDRXSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId    ret = CMS_FAIL;
    //UINT8               atChanId = PS_GET_CHANNEL_ID(reqHandle);
    //CmiMmSetEdrxParmCnf *pCmiSetEdrxParmCnf = (CmiMmSetEdrxParmCnf *)paras;

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
 * mmECPTWEDRXGetCnf
 * Description: Process CMI cnf msg of AT+ECPTWEDRXS?   get PTW/eDRX param
 * input: UINT16 reqHandle;   //req handler
 *        UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmECPTWEDRXGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_PTW_EDRX_GET_CNF_STR_LEN   64
    CmsRetId    ret = CMS_FAIL;
    CHAR        rspBuf[ATEC_PTW_EDRX_GET_CNF_STR_LEN], tempbuf[ATEC_PTW_EDRX_GET_CNF_STR_LEN];
    CmiMmGetRequestedEdrxParmCnf *pPtwEdrxConf = (CmiMmGetRequestedEdrxParmCnf *)paras;

    if (rc == CME_SUCC)
    {
        memset(rspBuf, 0, sizeof(rspBuf));

        /* AT+ECPTWEDRXS?, return:  +ECPTWEDRXS: <AcT-type>,<Requested_Paging_time_window>,<Requested_eDRX_value>  */

        snprintf(rspBuf, ATEC_PTW_EDRX_GET_CNF_STR_LEN, "+ECPTWEDRXS: %d", pPtwEdrxConf->actType);

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

/******************************************************************************
 * mmGetECEMMTIMECnf
 * Description: Process CMI cnf msg of AT+ECEMMTIME=   get current Emm Time State
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmGetECEMMTIMECnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_EMM_TIME_CET_RESP_STR_LEN   32
    CmsRetId ret = CMS_FAIL;
    CHAR     rspBuf[ATEC_EMM_TIME_CET_RESP_STR_LEN];
    CmiMmGetEmmTimeStateCnf *pGetEmmTimeStateCnf = (CmiMmGetEmmTimeStateCnf *)paras;
    UINT8    i;

    /*
         * AT+ECEMMTIME?
         *  +ECEMMTIME :<time-id>,<state>[,<remainTimeValue>]
        */
    if (rc == CME_SUCC)
    {
        for (i = 0; i < CMI_MM_EMM_TIME_NUM; i++)
        {
            memset(rspBuf, 0, sizeof(rspBuf));

            if (CMI_MM_EMM_TIMER_START == pGetEmmTimeStateCnf->timerState[i])
            {
                snprintf(rspBuf, ATEC_EMM_TIME_CET_RESP_STR_LEN, "+EMMTIME: %d,%d,%d",
                         pGetEmmTimeStateCnf->emmTimerId[i], pGetEmmTimeStateCnf->timerState[i], pGetEmmTimeStateCnf->remainTimeValue[i]);
            }
            else
            {
                snprintf(rspBuf, ATEC_EMM_TIME_CET_RESP_STR_LEN, "+EMMTIME: %d,%d",
                         pGetEmmTimeStateCnf->emmTimerId[i], pGetEmmTimeStateCnf->timerState[i]);
            }

            if (i != CMI_MM_EMM_TIME_NUM -1)
            {
                /*As another response string followed, here we using */
                ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, rspBuf);
            }
            else
            {
                /* last output print, should with right Handler */
                ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
            }
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}



/******************************************************************************
 * atMmCIOTOPTSetConf
 * Description: Process CMI cnf msg of AT+CCIOTOPT=   set current settings for supported and preferred CIoT EPS
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCIOTOPTSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId    ret = CMS_FAIL;
    UINT8       atChanId = AT_GET_HANDLER_CHAN_ID(reqHandle);
    CmiMmSetCiotOptCfgCnf *pCmiSetCiotOptCfgCnf = (CmiMmSetCiotOptCfgCnf *)paras;

    if (rc == CME_SUCC)
    {
        OsaCheck(pCmiSetCiotOptCfgCnf != PNULL, rc, reqHandle, 0);

        if (pCmiSetCiotOptCfgCnf->reportMode <= CMI_MM_CIOT_OPT_DISABLE_REPORT_RESET_CFG)
        {
            mwSetAndSaveAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG, pCmiSetCiotOptCfgCnf->reportMode);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atMmCIOTOPTGetConf
 * Description: Process CMI cnf msg of AT+CCIOTOPT?   get current settings for supported and preferred CIoT EPS
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCIOTOPTGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_CIOTOPT_CET_RESP_STR_LEN   32
    CmsRetId ret = CMS_FAIL;
    CHAR    rspBuf[ATEC_CIOTOPT_CET_RESP_STR_LEN];
    UINT8   atChanId    = AT_GET_HANDLER_CHAN_ID(reqHandle);
    UINT32  ciotRptMode = mwGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG);
    CmiMmGetCiotOptCfgCnf *pGetCiotOptConf = (CmiMmGetCiotOptCfgCnf *)paras;

    memset(rspBuf, 0, sizeof(rspBuf));

    /*
     * AT+CCIOTOPT?
     *  +CCIOTOPT :<n>,<supported_UE_opt>,<preferred_UE_opt>
    */
    if (rc == CME_SUCC)
    {
        snprintf(rspBuf, ATEC_CIOTOPT_CET_RESP_STR_LEN, "+CCIOTOPT: %d,%d,%d",
                 ciotRptMode, pGetCiotOptConf->ueSuptOptType, pGetCiotOptConf->uePreferOpt);

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atMmCIOTOPTGetCAPAConf
 * Description: Process CMI cnf msg of AT+CCIOTOPT=?   get supported settings
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCIOTOPTGetCAPACnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[64] = {0};
    CHAR RspBufTemp[12] = {0};
    CmiMmGetCiotOptCapaCnf *Conf = (CmiMmGetCiotOptCapaCnf *)paras;
    UINT8 bitCount = 0;

    if (rc == CME_SUCC)
    {
        sprintf(RspBuf, "+CCIOTOPT: (");

        for(int i=0; i<4; i++)
        {
            if((Conf->reportModeBitMap&(0x1<<i)) == (0x1<<i))
            {
                bitCount = i;
            }
        }

        for(int i=0; i<4; i++)
        {
            if((Conf->reportModeBitMap&(0x1<<i)) == (0x1<<i))
            {
                if(i < bitCount)
                {
                    sprintf(RspBufTemp, "%d,", i);
                    strcat((CHAR*)RspBuf, (const CHAR*)RspBufTemp);
                }
                else
                {
                    sprintf(RspBufTemp, "%d),(", i);
                    strcat((CHAR*)RspBuf, (const CHAR*)RspBufTemp);
                }
            }
        }

        for(int i=0; i<4; i++)
        {
            if((Conf->utOptBitmap&(0x1<<i)) == (0x1<<i))
            {
                bitCount = i;
            }
        }

        for(int i=0; i<4; i++)
        {
            if((Conf->utOptBitmap&(0x1<<i)) == (0x1<<i))
            {
                if(i < bitCount)
                {
                    sprintf(RspBufTemp, "%d,", i);
                    strcat((CHAR*)RspBuf, (const CHAR*)RspBufTemp);
                }
                else
                {
                    sprintf(RspBufTemp, "%d),(", i);
                    strcat((CHAR*)RspBuf, (const CHAR*)RspBufTemp);
                }
            }
        }

        for(int i=0; i<4; i++)
        {
            if((Conf->preferOptBitmap&(0x1<<i)) == (0x1<<i))
            {
                bitCount = i;
            }
        }

        for(int i=0; i<4; i++)
        {
            if((Conf->preferOptBitmap&(0x1<<i)) == (0x1<<i))
            {
                if(i < bitCount)
                {
                    sprintf(RspBufTemp, "%d,", i);
                    strcat((CHAR*)RspBuf, (const CHAR*)RspBufTemp);
                }
                else
                {
                    sprintf(RspBufTemp, "%d)", i);
                    strcat((CHAR*)RspBuf, (const CHAR*)RspBufTemp);
                }
            }
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * mmCRCESSetCnf
 * Description: Process CMI cnf msg of AT+CRCES?   get supported settings
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCRCESSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    CmiMmGetCEStatusCnf *Conf = (CmiMmGetCEStatusCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        sprintf(RspBuf, "+CRCES: %d,%d,%d", Conf->act, Conf->ceLevel, Conf->ccLevel);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atMmCEDRXRDPReadConf
 * Description: Process CMI cnf msg of AT+CEDRXRDP   read dynamic eDRX param
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmCEDRXRDPReadCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 RspBuf[64],tempbuf[32];
    CmiMmReadEdrxDynParmCnf *Conf = (CmiMmReadEdrxDynParmCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));
    memset(tempbuf, 0, sizeof(tempbuf));

    if(rc == CME_SUCC)
    {
        sprintf((CHAR*)RspBuf, "+CEDRXRDP: %d", Conf->actType);
        if((Conf->reqEdrxPresent) || (Conf->nwEdrxPresent))
        {
            if(Conf->reqEdrxPresent)
            {
                strcat((CHAR*)RspBuf, ",\"");
                atByteToBitString(tempbuf,Conf->reqEdrxValue,4);
                strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
                strcat((CHAR*)RspBuf, "\"");
            }
            else
            {
                strcat((CHAR*)RspBuf, "\"\"");
            }

            if(Conf->nwEdrxPresent)
            {
                strcat((CHAR*)RspBuf, ",\"");
                atByteToBitString(tempbuf,Conf->nwEdrxValue,4);
                strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
                strcat((CHAR*)RspBuf, "\"");

                strcat((CHAR*)RspBuf, ",\"");
                atByteToBitString(tempbuf,Conf->nwPtw,4);
                strcat((CHAR*)RspBuf, (const CHAR*)tempbuf);
                strcat((CHAR*)RspBuf, "\"");
            }
            /*
            else{
                strcat((CHAR*)RspBuf, "\"\",");
            }
            */
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * mmGetECPlMNSCnf
 * Description: Process CMI cnf msg of AT
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmGetECPlMNSCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiMmGetPlmnSelectStateCnf *Conf = (CmiMmGetPlmnSelectStateCnf *)paras;
    CHAR RspBuf[64];

    if (rc == CME_SUCC)
    {
        if (Conf->oosTimerPresent)
        {
            sprintf((CHAR*)RspBuf, "+ECPLMNS: %d, %d", Conf->plmnSelectState, Conf->oosTimerS);
        }
        else
        {
            sprintf((CHAR*)RspBuf, "+ECPLMNS: %d", Conf->plmnSelectState);
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * mmExecECPlMNSCnf
 * Description: Process CMI cnf msg of AT
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mmExecECPlMNSCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    //CmiMmStartOosPlmnSelectCnf *Conf = (CmiMmGetPlmnSelectStateCnf *)paras;

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





/*Global variable to record signal quality, used in +CESQ */
//extern CmiMmCesqInd gCesq;       //0 means not known or not detectable

/****************************************************************************************
*  Process CI ind msg of +CESQ
*
****************************************************************************************/
CmsRetId mmExSiQuInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiMmCesqInd *pCmiInd = (CmiMmCesqInd*)paras;
    INT16   sRsrp = CMI_MM_NOT_DETECT_RSRP, sRsrq = CMI_MM_NOT_DETECT_RSRQ;
    UINT32  rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG);
    UINT8   pRsrp = 255, pRsrq = 255;

    memset(indBuf, 0, sizeof(indBuf));
    //memcpy(&gCesq, pCmiInd, sizeof(gCesq));

    if (rptMode == ATC_ECCESQ_RPT_CESQ)
    {
        /*+CESQ: <rxlev>,<ber>,<rscp>,<ecno>,<rsrq>,<rsrp>*/
        /*
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
         *
         * !!!! BUT AT CMD NOT EXTENDED !!!! STILL !!!!!
         *  0    rsrp < -140 dBm
         *  1   -140 dBm â‰?rsrp < -139 dBm
         *  2   -139 dBm â‰?rsrp < -138 dBm
         *  ...
         *  95  -46 dBm â‰?rsrp < -45 dBm
         *  96  -45 dBm â‰?rsrp < -44 dBm
         *  97  -44 dBm â‰?rsrp
         *  255 not known or not detectable
         *
        */
        if (pCmiInd->rsrp <= 0)
        {
            pRsrp = 0;
        }
        else if (pCmiInd->rsrp <= 97)
        {
            pRsrp = pCmiInd->rsrp;
        }
        else
        {
            pRsrp = 255;
        }

        /*
         * 1> AS NB extended the RSRP value in TS 36.133-v14.5.0, Table 9.1.7-1/Table 9.1.24-1
         *   -30 rsrq < -34 dB
         *   -29 -34 dB <= rsrq < -33.5 dB
         *   ...
         *   -2 -20.5 dB <= rsrq < -20 dB
         *   -1 -20 dB <= rsrq < -19.5 dB
         *   0 rsrq < -19.5 dB
         *   1 -19.5 dB <= rsrq < -19 dB
         *   2 -19 dB <= rsrq < -18.5 dB
         *   ...
         *   32 -4 dB <= rsrq < -3.5 dB
         *   33 -3.5 dB <= rsrq < -3 dB
         *   34 -3 dB <= rsrq
         *   35 -3 dB <= rsrq < -2.5 dB
         *   36 -2.5 dB <= rsrq < -2
         *   ...
         *   45 2 dB <= rsrq < 2.5 dB
         *   46 2.5 dB <= rsrq
         * 2> If not valid, set to 127
         *
         * !!!!! BUT AT CMD NOT EXTENDED !!!! STILL !!!!!
         *  0   rsrq < -19.5 dB
         *  1   -19.5 dB â‰?rsrq < -19 dB
         *  2   -19 dB â‰?rsrq < -18.5 dB
         *  ...
         *  32  -4 dB â‰?rsrq < -3.5 dB
         *  33  -3.5 dB â‰?rsrq < -3 dB
         *  34  -3 dB â‰?rsrq
         *  255 not known or not detectable
        */
        if (pCmiInd->rsrq <= 0)
        {
            pRsrq = 0;
        }
        else if (pCmiInd->rsrq < 34)
        {
            pRsrq = pCmiInd->rsrq;
        }
        else if (pCmiInd->rsrq <= 46)
        {
            pRsrq = 34;
        }
        else
        {
            pRsrq = 255;
        }

        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+CESQ: 99,99,255,255,%d,%d", pRsrq, pRsrp);

        ret = atcURC(chanId, indBuf);
    }
    else if (rptMode == ATC_ECCESQ_RPT_ECCESQ)
    {
        /*
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
        */
        if (pCmiInd->rsrp <= 97 && pCmiInd->rsrp >= -17)
        {
            sRsrp = (INT16)(pCmiInd->rsrp) - 141;

            /*
             * 1> AS NB extended the RSRP value in TS 36.133-v14.5.0, Table 9.1.7-1/Table 9.1.24-1
             *   -30 rsrq < -34 dB
             *   -29 -34 dB <= rsrq < -33.5 dB
             *   ...
             *   -2 -20.5 dB <= rsrq < -20 dB
             *   -1 -20 dB <= rsrq < -19.5 dB
             *   0 rsrq < -19.5 dB
             *   1 -19.5 dB <= rsrq < -19 dB
             *   2 -19 dB <= rsrq < -18.5 dB
             *   ...
             *   32 -4 dB <= rsrq < -3.5 dB
             *   33 -3.5 dB <= rsrq < -3 dB
             *   34 -3 dB <= rsrq
             *   35 -3 dB <= rsrq < -2.5 dB
             *   36 -2.5 dB <= rsrq < -2
             *   ...
             *   45 2 dB <= rsrq < 2.5 dB
             *   46 2.5 dB <= rsrq
             * 2> If not valid, set to 127
            */
            if (pCmiInd->rsrq > -30)
            {
                if (pCmiInd->rsrq <= 34)
                {
                    sRsrq = ((INT16)(pCmiInd->rsrq) - 40)/2;
                }
                else if (pCmiInd->rsrq <= 46)
                {
                    sRsrq = ((INT16)(pCmiInd->rsrq) - 41)/2;
                }
            }
        }

        if (sRsrp != CMI_MM_NOT_DETECT_RSRP && sRsrq != CMI_MM_NOT_DETECT_RSRQ)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECCESQ: RSRP,%d,RSRQ,%d,SNR,%d",
                     sRsrp, sRsrq, pCmiInd->snr);

            ret = atcURC(chanId, indBuf);
        }
    }

    return ret;
}





/****************************************************************************************
*  Process CI ind msg of +ECPSMR
*
****************************************************************************************/
CmsRetId mmExPSMRInd(UINT32 chanId, void *paras)
{
#ifdef  FEATURE_REF_AT_ENABLE
    /* if define FEATURE_REF_AT_ENABLE, only report NPSMR. otherwise, only report ECPSMR */
    return CMS_RET_SUCC;
#else
    CmsRetId ret = CMS_RET_SUCC;


    CHAR    indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};

    UINT32  rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_PSM_RPT_CFG);
    CmiMmPsmChangeInd *pCmiPsmChangeInd = (CmiMmPsmChangeInd *)paras;

    if(ATC_ECPSMR_ENABLE == rptMode)
    {
        memset(indBuf, 0, sizeof(indBuf));
        /*
         * +ECPSMR: <indicates the power mode of MT>
        */
        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECPSMR: %d", pCmiPsmChangeInd->psmMode);
        ret = atcURC(chanId, indBuf);
    }

    return ret;

#endif
}


/****************************************************************************************
*  Process CI ind msg of +ECEMMTIME
*
****************************************************************************************/
CmsRetId mmExEMMTIMEInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    indBuf[ATEC_IND_RESP_32_STR_LEN] = {0};

    UINT32  rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_EMM_TIME_RPT_CFG);
    CmiMmEmmTimerStateInd *pCmiEmmTimeChangeInd = (CmiMmEmmTimerStateInd *)paras;
    BOOL   enabelT3346Rpt = FALSE;
    BOOL   enabelT3448Rpt = FALSE;
    BOOL   enabelT3412Rpt = FALSE;

    if (((rptMode >> ATC_ECEMMTIME_T3346_BIT_OFFSET) & 0x01) == 0x01)
    {
        enabelT3346Rpt = TRUE;
    }

    if (((rptMode >> ATC_ECEMMTIME_T3448_BIT_OFFSET) & 0x01) == 0x01)
    {
        enabelT3448Rpt = TRUE;
    }

    if (((rptMode >> ATC_ECEMMTIME_T3412_BIT_OFFSET) & 0x01) == 0x01)
    {
        enabelT3412Rpt = TRUE;
    }

    if (((CMI_MM_EMM_T3346 == pCmiEmmTimeChangeInd->emmTimer) && enabelT3346Rpt) ||
        ((CMI_MM_EMM_T3448 == pCmiEmmTimeChangeInd->emmTimer) && enabelT3448Rpt) ||
        ((CMI_MM_EMM_TAU_TIMER == pCmiEmmTimeChangeInd->emmTimer) && enabelT3412Rpt))
    {
        memset(indBuf, 0, sizeof(indBuf));

        if ((CMI_MM_EMM_TIMER_START == pCmiEmmTimeChangeInd->timerState) &&
            (pCmiEmmTimeChangeInd->tValuePst))
        {
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+ECEMMTIME: %d,%d,%d", pCmiEmmTimeChangeInd->emmTimer, pCmiEmmTimeChangeInd->timerState, pCmiEmmTimeChangeInd->timerValueS);
            //ret = atcReply(IND_REQ_HANDLER, AT_RC_OK, 0, indBuf);
            ret = atcURC(chanId, indBuf);
        }
        else
        {
            snprintf(indBuf, ATEC_IND_RESP_32_STR_LEN, "+ECEMMTIME: %d,%d", pCmiEmmTimeChangeInd->emmTimer, pCmiEmmTimeChangeInd->timerState);
            //ret = atcReply(IND_REQ_HANDLER, AT_RC_OK, 0, indBuf);
            ret = atcURC(chanId, indBuf);
        }
    }

    return ret;
}


/****************************************************************************************
*  Process CI ind msg of +CREG
*
****************************************************************************************/
CmsRetId mmExCREGInd(UINT32 chanId, void *paras)
{
#define ATEC_CREG_IND_STR_LEN   100
#define ATEC_CREG_IND_TMP_LEN   30

    CmsRetId    ret = CMS_FAIL;
    UINT32      cregRptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CREG_RPT_CFG);
    CHAR        indBuf[ATEC_CREG_IND_STR_LEN], tmpBuf[ATEC_CREG_IND_TMP_LEN];
    CmiMmCregInd *pCregInd  = (CmiMmCregInd*)paras;
    BOOL        locPrint    = FALSE;

    /*
     * 1: +CREG: <stat>
     * 2: +CREG: <stat>[,[<lac>],[<ci>],[<AcT>]]
     * 3: +CREG: <stat>[,[<lac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]]
    */
    OsaCheck(pCregInd != PNULL, pCregInd, 0, 0);

    if (cregRptMode == CMI_MM_DISABLE_CREG)   //CmiMmCregModeEnum
    {
        return CMS_RET_SUCC;  //nothing need to report
    }

    if (cregRptMode > CMI_MM_CREG_LOC_REJ_INFO ||
        pCregInd->changedType == CMI_CREG_NONE_CHANGED ||
        pCregInd->changedType > CMI_CREG_REJECT_INFO_CHANGED)
    {
        OsaDebugBegin(FALSE, cregRptMode, pCregInd->changedType, 0);
        OsaDebugEnd();
        return CMS_RET_SUCC;
    }

    if ((cregRptMode == CMI_MM_ENABLE_CREG && pCregInd->changedType >= CMI_CREG_LOC_INFO_CHANGED) ||
        (cregRptMode == CMI_MM_CREG_LOC_INFO && pCregInd->changedType >= CMI_CREG_REJECT_INFO_CHANGED))
    {
        ECOMM_TRACE(UNILOG_ATCMD_EXEC, mmExCREGInd_1, P_VALUE, 2,
                    "ATEC, CREG report mode: %d, but REG info changed type: %d, don't need to report CREG",
                    cregRptMode, pCregInd->changedType);
        return CMS_RET_SUCC;
    }

    memset(indBuf, 0, sizeof(indBuf));

    /*
     * 1: +CREG: <stat>
     */
    snprintf(indBuf, ATEC_CREG_IND_STR_LEN, "+CREG: %d", pCregInd->state);

    /*
     * 2: +CREG: <stat>[,[<lac>],[<ci>],[<AcT>]]
    */
    if (cregRptMode >= CMI_MM_CREG_LOC_INFO)
    {
        if (pCregInd->locPresent)
        {
            memset(tmpBuf, 0, sizeof(tmpBuf));
            snprintf(tmpBuf, ATEC_CREG_IND_TMP_LEN, ",\"%04X\",\"%08X\",%d",
                     pCregInd->tac, pCregInd->celId, pCregInd->act);

            strcat(indBuf, tmpBuf);
            locPrint = TRUE;
        }
    }

    /*
     * 3: +CREG: <stat>[,[<lac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]]
    */
    if (cregRptMode == CMI_MM_CREG_LOC_REJ_INFO &&
        pCregInd->rejCausePresent)
    {
        if (locPrint == FALSE)  //need to fill empty: [<lac>],[<ci>],[<AcT>]
        {
            strcat(indBuf, ",,,");
            locPrint = TRUE;
        }

        memset(tmpBuf, 0, sizeof(tmpBuf));
        snprintf(tmpBuf, ATEC_CREG_IND_TMP_LEN, ",%d,%d",
                 pCregInd->causeType, pCregInd->rejCause);

        strcat(indBuf, tmpBuf);
    }

    OsaCheck(strlen(indBuf) < ATEC_CREG_IND_STR_LEN, strlen(indBuf), ATEC_CREG_IND_STR_LEN, 0);

    ret = atcURC(chanId, indBuf);

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +CCIOTOPTI
*
****************************************************************************************/
CmsRetId mmCiotOptInd(UINT32 chanId, void *paras)
{
#define ATEC_CIOTOPT_IND_STR_LEN    32

    CmsRetId    ret = CMS_RET_SUCC;
    CHAR        indBuf[ATEC_CIOTOPT_IND_STR_LEN];
    UINT32      ciotOptRptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG);
    CmiMmNwSupportedCiotOptInd *pCiotOptIndInd = (CmiMmNwSupportedCiotOptInd*)paras;

    if (ciotOptRptMode == CMI_MM_CIOT_OPT_ENABLE_REPORT)
    {
        memset(indBuf, 0, sizeof(indBuf));

        /*
         * +CCIOTOPTI: <supported_Network_opt>
        */
        snprintf(indBuf, ATEC_CIOTOPT_IND_STR_LEN, "+CCIOTOPTI: %d", pCiotOptIndInd->nwCiotOptType);
        ret = atcURC(chanId, indBuf);
    }

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +CRCES
*
****************************************************************************************/
CmsRetId mmCRCESInd(UINT32 chanId, void *paras)//it's temp since cmimm has no define ind
{
    CmsRetId ret = CMS_FAIL;
    CHAR RespBuf[32];
    CmiMmCesqInd *Ind = (CmiMmCesqInd*)paras;

    memset(RespBuf, 0, sizeof(RespBuf));
    sprintf((char *)RespBuf, "+CRCES: %d,%d,%d\r\n", Ind->rsrq, Ind->rsrp, Ind->dlBer);
    ret = atcURC(chanId, (char *)RespBuf );

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +NITZ
*
****************************************************************************************/
CmsRetId mmNITZInd(UINT32 chanId, void *paras)
{
//#define ATEC_IND_RESP_128_STR_LEN    128
#define ATEC_MM_NITZ_IND_TEMP_LEN   64

    CmsRetId ret = CMS_FAIL;
    CHAR    indBuf[ATEC_IND_RESP_128_STR_LEN], tempBuf[ATEC_MM_NITZ_IND_TEMP_LEN];
    CmiMmNITZInd *pCmiInd = (CmiMmNITZInd*)paras;
    BOOL    tzPrint = FALSE, dstPrint = FALSE;
    //UINT8   timeZone = pCmiInd->utcInfo.tz;
    UINT8   ctzeRptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG);
    OsaUtcTimeTValue localTime;

    /*
     * Update local time
     * In fact whether need to update, controlled by AT CMD: AT+CTZU=<onoff>
     *  <onoff>: integer type value indicating
     *  0 Disable automatic time zone update via NITZ.
     *  1 Enable automatic time zone update via NITZ.
     *
     * - TBD
    */
    /*  // this code is moved to mmIndToEvent()
    if (pCmiInd->utcInfoPst)
    {
        if (OsaTimerSync(0,
                         SET_LOCAL_TIME,
                         (pCmiInd->utcInfo.year<<16)|(pCmiInd->utcInfo.mon<<8)|(pCmiInd->utcInfo.day),
                         (pCmiInd->utcInfo.hour<<24)|(pCmiInd->utcInfo.mins<<16)|(pCmiInd->utcInfo.sec<<8)|timeZone,
                         0,
                         0) == CMS_FAIL)
        {
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_synctime2, P_INFO, 0, "sync NITZ time fail");
        }
        else
        {
            mwSetTimeSyncFlag(1);//set to 1 when NITZ triggered
        }
    }
    */

    /*
     * +CTZR=[<reporting>], set the report mode
     *  0 disable time zone change event reporting.
     *  1 Enable time zone change event reporting by unsolicited result code +CTZV: <tz>.
     *  2 Enable extended time zone and local time reporting by unsolicited result code
     *    +CTZE: <tz>,<dst>,[<time>].
     *  3 Enable extended time zone and universal time reporting by unsolicited result code
     *    +CTZEU: <tz>,<dst>,[<utime>].
    */
    OsaDebugBegin(ctzeRptMode <= ATC_CTZR_RPT_UTC_TIME, ctzeRptMode, ATC_CTZR_RPT_UTC_TIME, 0);
    ctzeRptMode = ATC_CTZR_DISABLE_RPT;
    OsaDebugEnd();

    if (ctzeRptMode == ATC_CTZR_DISABLE_RPT)
    {
        // don't report CT info
        return CMS_RET_SUCC;
    }

    memset(indBuf, 0, sizeof(indBuf));

    /*
     * TZ
     *
     * <tz>: string type value representing the sum of the local time zone (difference between the local time and GMT
     *   expressed in quarters of an hour) plus daylight saving time. The format is "Â±zz", expressed as a fixed width, two
     *   digit integer with the range -48 ... +56. To maintain a fixed width, numbers in the range -9 ... +9 are expressed
     *   with a leading zero, e.g. "-09", "+00" and "+09"
     */
    if (pCmiInd->localTZPst || pCmiInd->utcInfoPst)
    {
        if (pCmiInd->localTZPst && pCmiInd->utcInfoPst)
        {
            OsaDebugBegin(pCmiInd->localTimeZone == pCmiInd->utcInfo.tz, pCmiInd->localTimeZone, pCmiInd->utcInfo.tz, 0);
            OsaDebugEnd();
        }

        memset(tempBuf, 0x00, sizeof(tempBuf));
        if (pCmiInd->localTZPst)
        {
            //timeZone = pCmiInd->localTimeZone;
            if(pCmiInd->localTimeZone >=0)
            {
                snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, "\"%+02d\"", pCmiInd->localTimeZone);
            }
            else
            {
                snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, "\"%02d\"", pCmiInd->localTimeZone);
            }
        }
        else
        {
            //timeZone = pCmiInd->utcInfo.tz;
            if(pCmiInd->utcInfo.tz >= 0)
            {
                snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, "\"%+02d\"", pCmiInd->utcInfo.tz);
            }
            else
            {
                snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, "\"%02d\"", pCmiInd->utcInfo.tz);
            }
        }

        tzPrint = TRUE;
    }

    if (ctzeRptMode == ATC_CTZR_RPT_TZ)
    {
        if (tzPrint)
        {
            /* +CTZV: <tz> */
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+CTZV: ");
            strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));

            ret = atcURC(chanId, indBuf);
            return ret;
        }
        else
        {
            // no TZ info, don't need to print out
            return CMS_RET_SUCC;
        }
    }
    else if (ctzeRptMode == ATC_CTZR_RPT_LOCAL_TIME)
    {
        /* +CTZE: <tz>,<dst>,[<time>] */
        if (tzPrint)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+CTZE: ");
            strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
        }
    }
    else    //ATC_CTZR_RPT_UTC_TIME
    {
        /* +CTZEU: <tz>,<dst>,[<utime>] */
        if (tzPrint)
        {
            snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+CTZEU: ");
            strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
        }
    }

    /*
     * dst
    */
    if (pCmiInd->dstPst)
    {
        if (tzPrint == FALSE)
        {
            strcat(indBuf, ",");
            tzPrint = TRUE;
        }

        memset(tempBuf, 0x00, sizeof(tempBuf));
        snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, ",%d", pCmiInd->dst);

        strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
        dstPrint = TRUE;
    }

    /*
     * <time>/<uttime>
    */
    if (pCmiInd->utcInfoPst)
    {
        /*  +CTZEU: <tz>,<dst>,[<utime>] */
        if (ctzeRptMode == ATC_CTZR_RPT_UTC_TIME)
        {
            if (tzPrint == FALSE)
            {
                strcat(indBuf, ",");
                tzPrint = TRUE;
            }

            if (dstPrint == FALSE)
            {
                strcat(indBuf, ",");
                dstPrint = TRUE;
            }

            memset(tempBuf, 0x00, sizeof(tempBuf));

            snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, ",\"%04d/%02d/%02d,%02d:%02d:%02d\"",
                     pCmiInd->utcInfo.year, pCmiInd->utcInfo.mon, pCmiInd->utcInfo.day,
                     pCmiInd->utcInfo.hour, pCmiInd->utcInfo.mins, pCmiInd->utcInfo.sec);

            if (ATEC_IND_RESP_128_STR_LEN > (strlen(indBuf)+strlen(tempBuf)))
            {
                strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
            }
            else if (ATEC_IND_RESP_128_STR_LEN > strlen(indBuf))
            {
                OsaDebugBegin(FALSE, ATEC_IND_RESP_128_STR_LEN, strlen(indBuf), strlen(tempBuf));
                OsaDebugEnd();
                strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
            }
            else    //(ATEC_IND_RESP_128_STR_LEN <= strlen(indBuf))
            {
                OsaCheck(FALSE, ATEC_IND_RESP_128_STR_LEN, strlen(indBuf), strlen(tempBuf));
            }
        }
        else if (ctzeRptMode == ATC_CTZR_RPT_LOCAL_TIME)
        {
            /*  +CTZEU: <tz>,<dst>,[<utime>] */
            if (tzPrint == FALSE)
            {
                strcat(indBuf, ",");
                tzPrint = TRUE;
            }

            if (dstPrint == FALSE)
            {
                strcat(indBuf, ",");
                dstPrint = TRUE;
            }

            memset(&localTime, 0, sizeof(OsaUtcTimeTValue));
            OsaTimerUtcToLocalTime(((pCmiInd->utcInfo.year<<16)|(pCmiInd->utcInfo.mon<<8)|(pCmiInd->utcInfo.day)),
            ((pCmiInd->utcInfo.hour<<24)|(pCmiInd->utcInfo.mins<<16)|(pCmiInd->utcInfo.sec<<8)),
            (((INT32)pCmiInd->utcInfo.tz)&0xff), pCmiInd->dst, &localTime);

            memset(tempBuf, 0x00, sizeof(tempBuf));

            snprintf(tempBuf, ATEC_MM_NITZ_IND_TEMP_LEN, ",\"%04d/%02d/%02d,%02d:%02d:%02d\"",
                     localTime.UTCtimer1>>16, (localTime.UTCtimer1>>8)&0xff, (localTime.UTCtimer1)&0xff,
                     (localTime.UTCtimer2>>24)&0xff, (localTime.UTCtimer2>>16)&0xff, (localTime.UTCtimer2>>8)&0xff);

            if (ATEC_IND_RESP_128_STR_LEN > (strlen(indBuf)+strlen(tempBuf)))
            {
                strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
            }
            else if (ATEC_IND_RESP_128_STR_LEN > strlen(indBuf))
            {
                OsaDebugBegin(FALSE, ATEC_IND_RESP_128_STR_LEN, strlen(indBuf), strlen(tempBuf));
                OsaDebugEnd();
                strncat(indBuf, tempBuf, ATEC_IND_RESP_128_STR_LEN - strlen(indBuf));
            }
            else    //(ATEC_IND_RESP_128_STR_LEN <= strlen(indBuf))
            {
                OsaCheck(FALSE, ATEC_IND_RESP_128_STR_LEN, strlen(indBuf), strlen(tempBuf));
            }

        }

    }

    if (strlen(indBuf) > 0)
    {
        ret = atcURC(chanId, (char *)indBuf);
        return ret;
    }

    return CMS_RET_SUCC;
}

/****************************************************************************************
*  Process CI ind msg of
*
****************************************************************************************/
CmsRetId mmEcplmnsInd(UINT32 chanId, void *paras)
{
    /*
     * disable this URC, as no customer need it now
    */
    #if 0
    CmsRetId ret = CMS_FAIL;
    CmiMmPlmnSelectStateInd *Ind = (CmiMmPlmnSelectStateInd*)paras;
    CHAR RespBuf[32];

    if(Ind->oosTimerPresent)
    {
        sprintf((CHAR*)RespBuf, "+ECPLMNSI: %d, %d", Ind->plmnSelectState, Ind->oosTimerS);
    }
    else
    {
        sprintf((CHAR*)RespBuf, "+ECPLMNSI: %d", Ind->plmnSelectState);
    }

    ret = atcURC(AT_CHAN_DEFAULT, (char *)RespBuf );
    #endif
    return CMS_RET_SUCC;
}

/****************************************************************************************
*  Process CI ind msg of +CEDRXP/+ECPTWEDRXP
*
****************************************************************************************/
CmsRetId mmCEDRXPInd(UINT32 chanId, void *paras)
{
    CmsRetId    ret = CMS_RET_SUCC;
    CHAR        tmpStrBuf[30] = {0};
    BOOL        reqEdrxPrint = FALSE;
#ifndef  FEATURE_REF_AT_ENABLE
    BOOL        reqPtwPrint = FALSE;
#endif
    CHAR        indBuf[ATEC_IND_RESP_128_STR_LEN] = {0};

    UINT32  rptMode1 = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG);
    UINT32  rptMode2 = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG);
    CmiMmEdrxDynParmInd *pCmiCedrxInd = (CmiMmEdrxDynParmInd *)paras;

    if (1 == rptMode1)
    {
        memset(indBuf, 0, sizeof(indBuf));
        /* +CEDRXP: <AcTtype>[,<Requested_eDRX_value>[,<NW-provided_eDRX_value>[,<Paging_time_window>]]] */
        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+CEDRXP: %d", pCmiCedrxInd->actType);

        if (pCmiCedrxInd->reqEdrxPresent)
        {
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

#ifndef  FEATURE_REF_AT_ENABLE
    /* if not define FEATURE_REF_AT_ENABLE, only report ECPTWEDRXP. otherwise, only report NPTWEDRXP */
    if (ATC_ECPTWEDRXS_ENABLE == rptMode2)
    {
        memset(indBuf, 0, sizeof(indBuf));
        /* +ECPTWEDRXP: <AcT-type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>[,<NW_provided_eDRX_value>[,<Paging_time_window>]]]] */
        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+ECPTWEDRXP: %d", pCmiCedrxInd->actType);

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
#endif

    return ret;
}

/******************************************************************************
 ******************************************************************************
 * static func table
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_VARIABLE__ //just for easy to find this position in SS


/******************************************************************************
 * mmCmiCnfqHdrList
 * const table, should store in FLASH memory
******************************************************************************/
const static CmiCnfFuncMapList mmCmiCnfHdrList[] =
{
    {CMI_MM_GET_CREG_CNF, mmCREGGetCnf},
    {CMI_MM_GET_CREG_CAPA_CNF, mmCREGGetCapaCnf},
    {CMI_MM_SET_AUTO_PLMN_CNF, mmCOPSSetAutoCnf},
    {CMI_MM_MANUAL_PLMN_SELECT_CNF, mmCOPSManuSeleCnf},
    {CMI_MM_DEREGISTER_CNF, mmCOPSDereCnf},
    {CMI_MM_SET_OPER_ID_FORMAT_CNF, mmCOPSSetIdCnf},
    {CMI_MM_GET_CURRENT_OPER_INFO_CNF, mmCOPSGetOperCnf},
    {CMI_MM_MANUAL_PLMN_SEARCH_CNF, mmCOPSManuSerCnf},
    {CMI_MM_SET_REQUESTED_PSM_PARM_CNF, mmCPSMSSetCnf},
    {CMI_MM_GET_REQUESTED_PSM_PARM_CNF, mmCPSMSGetCnf},
    {CMI_MM_GET_PSM_CAPA_CNF, PNULL},
    {CMI_MM_SET_REQUESTED_EDRX_PARM_CNF, mmEDRXSetCnf},
    {CMI_MM_GET_REQUESTED_EDRX_PARM_CNF, mmEDRXGetCnf},
    {CMI_MM_GET_EDRX_CAPA_CNF, PNULL},
    {CMI_MM_READ_EDRX_DYN_PARM_CNF, mmCEDRXRDPReadCnf},
    {CMI_MM_SET_CIOT_OPT_CFG_CNF, mmCIOTOPTSetCnf},
    {CMI_MM_GET_CIOT_OPT_CFG_CNF, mmCIOTOPTGetCnf},
    {CMI_MM_GET_CIOT_OPT_CAPA_CNF, mmCIOTOPTGetCAPACnf},
    {CMI_MM_GET_COVERAGE_ENHANCEMENT_STATUS_CNF, mmCRCESSetCnf},
    {CMI_MM_GET_EXTENDED_SIGNAL_QUALITY_CNF, mmCESQcnf},
    {CMI_MM_START_OOS_PLMN_SELECT_CNF, mmExecECPlMNSCnf},
    {CMI_MM_GET_PLMN_SELECT_STATE_CNF, mmGetECPlMNSCnf},
    {CMI_MM_GET_PSM_MODE_CNF, mmECPSMRCnf},
    {CMI_MM_SET_REQUESTED_PTW_EDRX_PARM_CNF, mmECPTWEDRXSetCnf},
    {CMI_MM_GET_REQUESTED_PTW_EDRX_PARM_CNF, mmECPTWEDRXGetCnf},
    {CMI_MM_GET_EMM_TIME_STATE_CNF, mmGetECEMMTIMECnf},

    {CMI_MM_PRIM_BASE, PNULL}   /* this should be the last */
};


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


/******************************************************************************
 * atMmProcCmiCnf
 * Description: Process CMI cnf msg for MM sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atMmProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (mmCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (mmCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = mmCmiCnfHdrList[hdrIndex].cmiCnfHdr;
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
 * atDevProcCmiInd
 * Description: Process CMI Ind msg for dev sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atMmProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32 chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    if (pCmiInd->header.reqHandler != BROADCAST_IND_HANDLER)
    {
        //if not broadcast Ind, add here
    }
    else //broadcast Ind
    {
        for (chanId = 1; chanId < CMS_CHAN_NUM; chanId++)
        {
            if (atcBeURCConfiged(chanId) != TRUE)
            {
                continue;
            }

            switch (primId)
            {
                case CMI_MM_EXTENDED_SIGNAL_QUALITY_IND:
                    mmExSiQuInd(chanId, pCmiInd->body);
                    break;

                case CMI_MM_PSM_CHANGE_IND:
                    mmExPSMRInd(chanId, pCmiInd->body);
                    break;

                case CMI_MM_CREG_IND:
                    mmExCREGInd(chanId, pCmiInd->body);
                    break;

                case CMI_MM_NW_SUPPORTED_CIOT_OPT_IND:
                    mmCiotOptInd(chanId, pCmiInd->body);
                    break;

                //case CMI_MM_COVERAGE_ENHANCEMENT_STATUS_IND:
                //    ret = mmCRCESInd(chanId, pCmiInd->body);
                //    break;

                case CMI_MM_NITZ_IND:
                    mmNITZInd(chanId, pCmiInd->body);
                    break;

                case CMI_MM_PLMN_SELECT_STATE_IND:
                    mmEcplmnsInd(chanId, pCmiInd->body);
                    break;

                case CMI_MM_EMM_TIMER_STATE_IND:
                    mmExEMMTIMEInd(chanId, pCmiInd->body);
                    break;

                case CMI_MM_EDRX_DYN_PARM_IND:
                    mmCEDRXPInd(chanId, pCmiInd->body);
                    break;

                default:
                    break;
            }
    }

    }

}


