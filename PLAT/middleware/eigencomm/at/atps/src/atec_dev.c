/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_dev.c
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
#include "atec_dev_cnf_ind.h"
#include "nvram.h"
#include "ps_lib_api.h"

char* IMEI = "351718070014156";

#define ECCFG_MAX_SET_PARAMS_NUM    16 //limit input parameter number <= 16

#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId devCFUN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devCFUN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 funcVal, resetVal;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CFUN=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CFUN: (0,1,4),(0)");
            break;
        }

        case AT_READ_REQ:            /* AT+CFUN?  */
        {
            ret = devGetFunc(atHandle);
            break;
        }

        case AT_SET_REQ:              /* AT+CFUN= */
        {
            if (atGetNumValue(pParamList, 0, &funcVal,
                              ATC_CFUN_0_FUN_VAL_MIN, ATC_CFUN_0_FUN_VAL_MAX, ATC_CFUN_0_FUN_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (atGetNumValue(pParamList, 1, &resetVal,
                                  ATC_CFUN_1_RST_VAL_MIN, ATC_CFUN_1_RST_VAL_MAX, ATC_CFUN_1_RST_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    ret = devSetFunc(atHandle, funcVal, resetVal);
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+CFUN */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId devECBAND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECBAND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32   nwMode = CMI_DEV_NB_MODE;
    INT32   band = 0;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   getValueRet = AT_PARA_ERR;
    UINT8   i = 0;
    CmiDevSetCiotBandReq    cmiSetBandReq = {0};  //20 bytes

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECBAND=? */
        {
            ret = devGetSupportedCiotBandModeReq(atHandle);
            break;
        }

        case AT_READ_REQ:         /* AT+ECBAND? */
        {
            ret = devGetCiotBandModeReq(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+ECBAND= */
        {
            memset(&cmiSetBandReq, 0x00, sizeof(CmiDevSetCiotBandReq));

            if (atGetNumValue(pParamList, 0, &nwMode,
                              ATC_ECBAND_0_NW_MODE_VAL_MIN, ATC_ECBAND_0_NW_MODE_VAL_MAX, ATC_ECBAND_0_NW_MODE_VAL_DEFAULT) == AT_PARA_OK)
            {
                cmiSetBandReq.nwMode = nwMode;

                for (i = 1; i < pAtCmdReq->paramRealNum; i++)
                {
                    getValueRet = atGetNumValue(pParamList, i, &band,
                                                ATC_ECBAND_1_BAND_VAL_MIN, ATC_ECBAND_1_BAND_VAL_MAX, ATC_ECBAND_1_BAND_VAL_DEFAULT);

                    if (getValueRet == AT_PARA_OK)
                    {
                        if (cmiSetBandReq.bandNum >= CMI_DEV_SUPPORT_MAX_BAND_NUM)
                        {
                            OsaDebugBegin(FALSE, cmiSetBandReq.bandNum, CMI_DEV_SUPPORT_MAX_BAND_NUM, 0);
                            OsaDebugEnd();

                            cmiSetBandReq.bandNum++;    //invalid AT CMD
                            break;
                        }
                        cmiSetBandReq.orderedBand[cmiSetBandReq.bandNum] = band;
                        cmiSetBandReq.bandNum++;
                    }
                    else if (getValueRet == AT_PARA_ERR)
                    {
                        OsaDebugBegin(FALSE, getValueRet, band, i);
                        OsaDebugEnd();

                        cmiSetBandReq.bandNum = CMI_DEV_SUPPORT_MAX_BAND_NUM+1; //invalid
                    }
                    else
                    {
                        break;  //done
                    }
                }

                if (cmiSetBandReq.bandNum > 0 && cmiSetBandReq.bandNum <= CMI_DEV_SUPPORT_MAX_BAND_NUM)
                {
                    cmiSetBandReq.bRefAT = FALSE;
                    ret = devSetCiotBandModeReq(atHandle, &cmiSetBandReq);
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+ECBAND */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECFREQ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECFREQ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32   mode = 0, cellId = 0;
    INT32   freqList[CMI_DEV_SUPPORT_MAX_FREQ_NUM] = {0};
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   paramNumb = pAtCmdReq->paramRealNum;
    UINT8   tmpIdx = 0;

    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+ECFREQ? */
        {
            ret = devGetCiotFreqReq(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+ECFREQ= */
        {
            if (atGetNumValue(pParamList, 0, &mode,
                              ATC_ECFREQ_0_NW_MODE_VAL_MIN, ATC_ECFREQ_0_NW_MODE_VAL_MAX, ATC_ECFREQ_0_NW_MODE_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (mode == CMI_DEV_UNLOCK_FREQ_INFO && paramNumb == 1)
                {
                    ret = devSetCiotFreqReq(atHandle, mode, PNULL, 0, -1, FALSE);
                }
                else if (mode == CMI_DEV_SET_PREFER_FREQ && paramNumb > 1)
                {
                    for (tmpIdx = 1; tmpIdx < paramNumb && tmpIdx < CMI_DEV_SUPPORT_MAX_FREQ_NUM+1; tmpIdx++)
                    {
                        if (atGetNumValue(pParamList, tmpIdx, &freqList[tmpIdx-1],
                                          ATC_ECFREQ_1_EARFCN_VAL_MIN, ATC_ECFREQ_1_EARFCN_VAL_MAX, ATC_ECFREQ_1_EARFCN_VAL_DEFAULT) != AT_PARA_OK)
                        {
                            break;
                        }
                    }

                    if (tmpIdx > 1)
                    {
                        ret = devSetCiotFreqReq(atHandle, mode, freqList, tmpIdx-1, -1, FALSE);
                    }

                }
                else if (CMI_DEV_LOCK_CELL == mode && (paramNumb == 2 || paramNumb == 3))
                {
                    if (atGetNumValue(pParamList, 1, &freqList[0],
                                      ATC_ECFREQ_1_EARFCN_VAL_MIN, ATC_ECFREQ_1_EARFCN_VAL_MAX, ATC_ECFREQ_1_EARFCN_VAL_DEFAULT) == AT_PARA_OK)
                    {
                        if (pParamList[2].bDefault)
                        {
                            ret = devSetCiotFreqReq(atHandle, mode, freqList, 1, -1, FALSE);
                        }
                        else if(atGetNumValue(pParamList, 2, &cellId,
                                              ATC_ECFREQ_2_PHYCELL_VAL_MIN, ATC_ECFREQ_2_PHYCELL_VAL_MAX, ATC_ECFREQ_2_PHYCELL_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            ret = devSetCiotFreqReq(atHandle, mode, freqList, 1, cellId, FALSE);
                        }
                    }
                }
                else if (mode == CMI_DEV_CLEAR_PREFER_FREQ_LIST && paramNumb == 1)
                {
                    ret = devSetCiotFreqReq(atHandle, mode, PNULL, 0, -1, FALSE);
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_TEST_REQ:
        {
            /*+ECFREQ: (list of support <mode>s)*/
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECFREQ: (0,1,2,3)");
            break;
        }

        case AT_EXEC_REQ:
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

#if 0
/**
  \fn          CmsRetId devCGSN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devCGSN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR    rspBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    rspBufTemp[8] = {0};
    CHAR    imeiBuf[18] = {0};
    CHAR    snBuf[32] = {0};
    INT32   snt;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CGSN=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CGSN: (0,1,2,3)");
            break;
        }

        case AT_SET_REQ:            /* AT+CGSN=<snt> */
        {
            if (atGetNumValue(pParamList, 0, &snt, 0, 3, 1) == AT_PARA_OK)
            {
                ret = CMS_RET_SUCC;

                if (snt == 1)
                {
                    /*
                     * returns the IMEI (International Mobile station Equipment Identity)
                     */
                    memset(imeiBuf, 0, sizeof(imeiBuf));
                    appGetImeiNumSync(imeiBuf);
                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s\"", imeiBuf);
                }
                else if (snt == 0)
                {
                    /*
                     * returns <sn>
                     */
                    memset(snBuf, 0, sizeof(snBuf));

                    if (!appGetSNNumSync(snBuf))
                    {
                        /*
                         * <sn>: one or more lines of information text determined by the MT manufacturer. Typically,
                         *  the text will consist of a single line containing the IMEI number of the MT, but manufacturers
                         *  may choose to provide more information if desired.
                         * If SN not set, return IMEI
                         */
                        memset(snBuf, 0, sizeof(snBuf));
                        appGetImeiNumSync(snBuf);
                    }

                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s\"", snBuf);
                }
                else if (snt == 2)
                {
                    /*
                     * returns the IMEISV (International Mobile station Equipment Identity and Software Version number)
                     */
                    memset(imeiBuf, 0, sizeof(imeiBuf));
                    appGetImeiNumSync(imeiBuf);

                    snprintf(rspBufTemp, 8, "%s", SDK_MAJOR_VERSION);

                    /* get 2 bytes SN */
                    rspBufTemp[0] = rspBufTemp[1];
                    rspBufTemp[1] = rspBufTemp[2];
                    rspBufTemp[2] = 0;

                    imeiBuf[14] = 0;      //delete CD

                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s%s\"", imeiBuf, rspBufTemp);
                }
                else if (snt == 3)
                {
                    /*
                     * returns the SVN (Software Version Number)
                    */
                    snprintf(rspBufTemp, 8, "%s", SDK_MAJOR_VERSION);

                    /* get 2 bytes SN */
                    rspBufTemp[0] = rspBufTemp[1];
                    rspBufTemp[1] = rspBufTemp[2];
                    rspBufTemp[2] = 0;
                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s\"", rspBufTemp);
                }
                else
                {
                    //should not arrive here
                    OsaDebugBegin(FALSE, snt, 0, 0);
                    OsaDebugEnd();

                    ret = CMS_FAIL;
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, rspBuf);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:            /* AT+CGSN */
        {
            /*
             * To get <sn> which returns IMEI of the MT
             * AT+CGSN
             * 490154203237518
             * OK
            */
           ret = CMS_RET_SUCC;
           memset(snBuf, 0, sizeof(snBuf));
           if (!appGetSNNumSync(snBuf))
           {
               /*
                * <sn>: one or more lines of information text determined by the MT manufacturer. Typically,
                *  the text will consist of a single line containing the IMEI number of the MT, but manufacturers
                *  may choose to provide more information if desired.
                * If SN not set, return IMEI
                */
               memset(snBuf, 0, sizeof(snBuf));
               appGetImeiNumSync(snBuf);
           }
           snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "%s", snBuf);

           ret = atcReply(atHandle, AT_RC_OK, 0, rspBuf);
           break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}
#endif
/**
  \fn          CmsRetId devECCGSN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECCGSN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   imeiBuf[18] = {0};
    UINT8   snBuf[32] = {0};
    CHAR    RspBuf[32] = {0};
    UINT16  snLen = 0;
    UINT16  imeiLen = 0;
    INT16   imeiFlag = 1;
    INT16   snFlag = 1;

    switch (operaType)
    {
        case AT_SET_REQ:            /* AT+ECCGSN= */
        {
            UINT16  paraLen = 0;
            UINT8   paraIdx = 0;

            UINT8 paraStr[ATC_ECCGSN_MAX_PARM_STR_LEN] = {0};

            memset(paraStr, 0x00, ATC_ECCGSN_MAX_PARM_STR_LEN);
            if(atGetStrValue(pParamList, paraIdx, paraStr,
                                  ATC_ECCGSN_MAX_PARM_STR_LEN, &paraLen, ATC_ECCGSN_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCGSN_1, P_WARNING, 0, "AT+ECCGSN, invalid input string parameter");
            }

            // next parameters
            paraIdx++;
            if(strncasecmp((const CHAR*)paraStr, "IMEI", strlen("IMEI")) == 0)
            {
                if(strlen((const CHAR*)paraStr) == strlen("IMEI"))
                {
                    if(appGetImeiLockSync((CHAR *)imeiBuf))  /* check if imei is locked*/
                    {
                        if(strncasecmp((const CHAR*)imeiBuf, ATC_ECCGSNLOCK_0_IMEI_STR_DEFAULT, strlen(ATC_ECCGSNLOCK_0_IMEI_STR_DEFAULT)) == 0)
                        {
                            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, "+ECCGSN: IMEI LOCKED");
                            imeiFlag = 0;
                            ret = CMS_FAIL;
                        }
                    }

                    if(imeiFlag == 1)
                    {
                        memset(imeiBuf, 0, 18);
                        if (atGetStrValue(pParamList, paraIdx, imeiBuf, ATC_CGSN_0_MAX_PARM_STR_LEN, &imeiLen, ATC_CGSN_0_MAX_PARM_STR_DEFAULT) != AT_PARA_ERR)
                        {
                            ret = CMS_FAIL;
                            if(appSetImeiNumSync((CHAR *)imeiBuf))
                                ret = CMS_RET_SUCC;
                        }
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }
            else if(strncasecmp((const CHAR*)paraStr, "SN", strlen("SN")) == 0)
            {
                if(strlen((const CHAR*)paraStr) == strlen("SN"))
                {
                    if(appGetSNLockSync((CHAR *)snBuf))  /* check if sn is locked*/
                    {
                        if(strncasecmp((const CHAR*)snBuf, ATC_ECCGSNLOCK_0_SN_STR_DEFAULT, strlen(ATC_ECCGSNLOCK_0_SN_STR_DEFAULT)) == 0)
                        {
                            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, "+ECCGSN: SN LOCKED");
                            snFlag = 0;
                            ret = CMS_FAIL;
                        }
                    }

                    if(snFlag == 1)
                    {
                        memset(snBuf, 0, 32);
                        if(atGetStrValue(pParamList, paraIdx, snBuf, ATC_CGSN_1_MAX_PARM_STR_LEN, &snLen, ATC_CGSN_1_MAX_PARM_STR_DEFAULT) != AT_PARA_ERR)
                        {
                            ret = CMS_FAIL;
                            if(appSetSNNumSync((CHAR *)snBuf,snLen))
                                ret = CMS_RET_SUCC;
                        }
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }
            else
            {
                ret = CMS_FAIL;

            }

            if(ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_TEST_REQ:
        {
            sprintf((char*)RspBuf, "+ECCGSN: (\"IMEI\",\"SN\"),(data)\r\n");

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);
            break;
        }
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECCGSN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECCGSNLOCK(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8 imeiBuf[18] = {0};
    UINT8 snBuf[32] = {0};
    CHAR RspBuf[64] = {0};
    CHAR RspBufTemp[32] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECCGSNLOCK=? */
        {
            sprintf((char*)RspBuf, "+ECCGSNLOCK: (\"IMEI\",\"SN\")\r\n");

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);
            break;
        }

        case AT_READ_REQ:            /* AT+ECCGSNLOCK? */
        {
            if(appGetImeiLockSync((CHAR *)imeiBuf))
            {
                if(strncasecmp((const CHAR*)imeiBuf, ATC_ECCGSNLOCK_0_IMEI_STR_DEFAULT, strlen(ATC_ECCGSNLOCK_0_IMEI_STR_DEFAULT)) == 0)
                {
                    sprintf((char*)RspBuf, "+ECCGSNLOCK: IMEI LOCKED, ");
                }
                else
                {
                    sprintf((char*)RspBuf, "+ECCGSNLOCK: IMEI NOT LOCKED, ");
                }
            }
            else
            {
                sprintf((char*)RspBuf, "+ECCGSNLOCK: GET IMEI LOCK ERR, ");
            }

            if(appGetSNLockSync((CHAR *)snBuf))
            {
                if(strncasecmp((const CHAR*)snBuf, ATC_ECCGSNLOCK_0_SN_STR_DEFAULT, strlen(ATC_ECCGSNLOCK_0_SN_STR_DEFAULT)) == 0)
                {
                    sprintf((char*)RspBufTemp, "SN LOCKED\r\n");
                    strcat((CHAR*)RspBuf, RspBufTemp);
                }
                else
                {
                    sprintf((char*)RspBufTemp, "SN NOT LOCKED\r\n");
                    strcat((CHAR*)RspBuf, RspBufTemp);
                }
            }
            else
            {
                sprintf((char*)RspBufTemp, "GET SN LOCK ERR\r\n");
                strcat((CHAR*)RspBuf, RspBufTemp);
            }

            ret = atcReply(atHandle, AT_RC_OK, 0, RspBuf);
            break;
        }

        case AT_SET_REQ:            /* AT+ECCGSNLOCK= */
        {
            UINT16  paraLen = 0;
            UINT8   paraIdx = 0;

            UINT8 paraStr[ATC_ECCGSN_MAX_PARM_STR_LEN] = {0};

            memset(paraStr, 0x00, ATC_ECCGSN_MAX_PARM_STR_LEN);
            if (atGetStrValue(pParamList, paraIdx, paraStr,
                              ATC_ECCGSN_MAX_PARM_STR_LEN, &paraLen, ATC_ECCGSN_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCGSNLOCK_1, P_WARNING, 0, "AT+ECCGSN, invalid input string parameter");
            }

            // next parameters
            paraIdx++;
            if(strncasecmp((const CHAR*)paraStr, "IMEI", strlen("IMEI")) == 0)
            {
                if(strlen((const CHAR*)paraStr) == strlen("IMEI"))
                {
                    if(appSetImeiLockSync((CHAR *)ATC_ECCGSNLOCK_0_IMEI_STR_DEFAULT))
                    {
                        ret = CMS_RET_SUCC;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }
            else if(strncasecmp((const CHAR*)paraStr, "SN", strlen("SN")) == 0)
            {
                if(strlen((const CHAR*)paraStr) == strlen("SN"))
                {
                    if(appSetSNLockSync((CHAR *)ATC_ECCGSNLOCK_0_SN_STR_DEFAULT))
                    {
                        ret = CMS_RET_SUCC;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }
            #if 0
            else if(strncasecmp((const CHAR*)paraStr, "CLEAN", strlen("CLEAN")) == 0)
            {
                if(strlen((const CHAR*)paraStr) == strlen("CLEAN"))
                {
                    if(appSetNV9LockCleanSync())
                    {
                        ret = CMS_RET_SUCC;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }
            #endif
            else
            {
                ret = CMS_FAIL;

            }

            if(ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId devECCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   paramNumb = pAtCmdReq->paramRealNum;
    BOOL    cmdValid = TRUE;
    UINT8   paraStr[ATC_ECCFG_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 32;
    UINT16  paraLen = 0;
    INT32   paraValue = -1;
    CmiDevSetExtCfgReq setExtCfgReq;
    UINT8   paraIdx = 0;

    memset(&setExtCfgReq, 0x00, sizeof(CmiDevSetExtCfgReq));

    setExtCfgReq.bRefAT = FALSE;

    switch (operaType)
    {
        case AT_READ_REQ:            /* AT+ECCFG?  */
        {
            ret = devGetExtCfg(atHandle);
            break;
        }

        case AT_SET_REQ :
        {
            /*
             * AT+ECCFG="AUTOAPN",0/1,"SUPPORTSMS",0/1,"USIMPOWERSAVE",0/1,
             *         "SUPPORTUPRAI",0/1,"DataInactTimer",0/>=40,"RelaxMonitorDeltaP",[0-17],
             *         "Rohc",0/1,"EPCO",0/1
             */
            if ((paramNumb > 0) && (paramNumb <= ECCFG_MAX_SET_PARAMS_NUM) && (paramNumb & 0x01) == 0)
            {
                for (paraIdx = 0; paraIdx < paramNumb; paraIdx++)
                {
                    memset(paraStr, 0x00, ATC_ECCFG_0_MAX_PARM_STR_LEN);

                    if (atGetStrValue(pParamList, paraIdx, paraStr,
                                       ATC_ECCFG_0_MAX_PARM_STR_LEN, &paraLen, ATC_ECCFG_0_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_1, P_WARNING, 0, "AT+ECCFG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }

                    // next parameters
                    paraIdx++;
                    #if 0
                    if (strncasecmp((const CHAR*)paraStr, "GcfTest", strlen("GcfTest")) == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_GCFTEST_VAL_MIN, ATC_ECCFG_1_GCFTEST_VAL_MAX, ATC_ECCFG_1_GCFTEST_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.gcfTestPresent = TRUE;
                            setExtCfgReq.bGcfTest = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_2, P_WARNING, 0, "AT+ECCFG, invalid input 'GcfTest' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    #endif
                    if (strcasecmp((const CHAR*)paraStr, "AutoApn") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_AUTOAPN_VAL_MIN, ATC_ECCFG_1_AUTOAPN_VAL_MAX, ATC_ECCFG_1_AUTOAPN_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.autoApnPresent = TRUE;
                            setExtCfgReq.bAutoApn = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_3, P_WARNING, 0, "AT+ECCFG, invalid input 'AutoApn' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "SupportSms") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_SUPPORTSMS_VAL_MIN, ATC_ECCFG_1_SUPPORTSMS_VAL_MAX, ATC_ECCFG_1_SUPPORTSMS_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.supportSmsPresent = TRUE;
                            setExtCfgReq.supportSms = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_4, P_WARNING, 0, "AT+ECCFG, invalid input 'SupportSms' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "TauForSms") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_TAUFORSMS_VAL_MIN, ATC_ECCFG_1_TAUFORSMS_VAL_MAX, ATC_ECCFG_1_TAUFORSMS_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.tauForSmsPresent = TRUE;
                            setExtCfgReq.tauForSms = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_5, P_WARNING, 0, "AT+ECCFG, invalid input 'TauForSms' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "T3324MaxValueS") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_T3324_VAL_MIN, ATC_ECCFG_1_T3324_VAL_MAX, ATC_ECCFG_1_T3324_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.t3324MaxValuePresent = TRUE;
                            setExtCfgReq.t3324MaxValueS = (UINT32)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_t3324MaxValue, P_WARNING, 0, "AT+ECCFG, invalid input 'T3324MaxValueS' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "BarValueS") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_BAR_VAL_MIN, ATC_ECCFG_1_BAR_VAL_MAX, ATC_ECCFG_1_BAR_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.barValuePresent = TRUE;
                            setExtCfgReq.barValueS = (UINT32)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_barValue, P_WARNING, 0, "AT+ECCFG, invalid input 'BarValueS' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "UsimPowerSave") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_SIMTEST_VAL_MIN, ATC_ECCFG_1_SIMTEST_VAL_MAX, ATC_ECCFG_1_SIMTEST_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            /*
                            *"UsimPowerSave":0/1
                            * 0: internal USIM power save disabled
                            * 1: internal USIM power save enabled
                            */
                            setExtCfgReq.enableSimPsmPresent = TRUE;
                            if (paraValue == 1)
                            {
                                setExtCfgReq.enableSimPsm = TRUE;
                            }
                            else
                            {
                                setExtCfgReq.enableSimPsm = FALSE;
                            }
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_6, P_WARNING, 0, "AT+ECCFG, invalid input 'UsimPowerSave' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "UsimSimulator") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_USIMSIMULATOR_VAL_MIN, ATC_ECCFG_1_USIMSIMULATOR_VAL_MAX, ATC_ECCFG_1_USIMSIMULATOR_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.usimSimuPresent = TRUE;
                            setExtCfgReq.bUsimSimulator = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_7, P_WARNING, 0, "AT+ECCFG, invalid input 'UsimSimulator' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "SimBip") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_SIMBIP_VAL_MIN, ATC_ECCFG_1_SIMBIP_VAL_MAX, ATC_ECCFG_1_SIMBIP_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.enableBipPresent = TRUE;
                            setExtCfgReq.enableBip = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_SimBip, P_WARNING, 0, "AT+ECCFG, invalid input 'SimBip' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "PowerCfun") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_POWERCFUN_VAL_MIN, ATC_ECCFG_1_POWERCFUN_VAL_MAX, ATC_ECCFG_1_POWERCFUN_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            if (paraValue == 0 || paraValue == 1 || paraValue == 4)
                            {
                                setExtCfgReq.powerCfunPresent = TRUE;
                                setExtCfgReq.powerCfun = (UINT8)paraValue;
                            }
                            else
                            {
                                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_PowerCfun_1, P_WARNING, 1,
                                            "AT+ECCFG, invalid input 'PowerCfun' value: %d", paraValue);
                                cmdValid = FALSE;
                                break;
                            }
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_PowerCfun_2, P_WARNING, 0, "AT+ECCFG, invalid input 'UsimSimulator' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "PsPowerOnMaxDelay") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_PSPOWERONMAXDEALY_VAL_MIN, ATC_ECCFG_1_PSPOWERONMAXDEALY_VAL_MAX, ATC_ECCFG_1_PSPOWERONMAXDEALY_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.psPowerOnMaxDelayPresent = TRUE;
                            setExtCfgReq.psPowerOnMaxDelay = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_psPowerOnMaxDelay, P_WARNING, 0, "AT+ECCFG, invalid input 'PsPowerOnMaxDelay' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
#ifdef  FEATURE_REF_AT_ENABLE
                    else if (strcasecmp((const CHAR*)paraStr, "Cfun1PowerOnPs") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_CFUN1POWERONPS_VAL_MIN, ATC_ECCFG_1_CFUN1POWERONPS_VAL_MAX, ATC_ECCFG_1_CFUN1POWERONPS_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.cfun1PowerOnPsPresent = TRUE;
                            setExtCfgReq.cfun1PowerOnPs = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_cfun1PowerOnPs, P_WARNING, 0, "AT+ECCFG, invalid input 'PsPowerOnMaxDelay' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
#endif
                    else if (strcasecmp((const CHAR*)paraStr, "MultiCarrier") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_MULTICARRIER_VAL_MIN, ATC_ECCFG_1_MULTICARRIER_VAL_MAX, ATC_ECCFG_1_MULTICARRIER_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.multiCarrierPresent = TRUE;
                            setExtCfgReq.multiCarrier = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_8, P_WARNING, 0, "AT+ECCFG, invalid input 'MultiCarrier' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "MultiTone") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_MULTITONE_VAL_MIN, ATC_ECCFG_1_MULTITONE_VAL_MAX, ATC_ECCFG_1_MULTITONE_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.multiTonePresent = TRUE;
                            setExtCfgReq.multiTone = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_9, P_WARNING, 0, "AT+ECCFG, invalid input 'MultiTone' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "SupportUpRai") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_SUPPORTUPRAI_VAL_MIN, ATC_ECCFG_1_SUPPORTUPRAI_VAL_MAX, ATC_ECCFG_1_SUPPORTUPRAI_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.upRaiPresent = TRUE;
                            setExtCfgReq.supportUpRai = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_10, P_WARNING, 0, "AT+ECCFG, invalid input 'SupportUpRai' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "DataInactTimer") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_DIT_VAL_MIN, ATC_ECCFG_1_DIT_VAL_MAX, ATC_ECCFG_1_DIT_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.dataInactTimerPresent  = TRUE;
                            setExtCfgReq.dataInactTimerS        = (UINT8)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_11, P_WARNING, 0, "AT+ECCFG, invalid input 'DataInactTimer' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "RelaxMonitorDeltaP") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_RMD_VAL_MIN, ATC_ECCFG_1_RMD_VAL_MAX, ATC_ECCFG_1_RMD_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.relaxMonitorPresent    = TRUE;
                            setExtCfgReq.relaxMonitorDeltaP     = (UINT8)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_12, P_WARNING, 0, "AT+ECCFG, invalid input 'RelaxMonitorDeltaP' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "PlmnSearchPowerLevel") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_POWERLEVEL_VAL_MIN, ATC_ECCFG_1_POWERLEVEL_VAL_MAX, ATC_ECCFG_1_POWERLEVEL_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.powerLevelPresent      = TRUE;
                            setExtCfgReq.plmnSearchPowerLevel   = (UINT8)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_13, P_WARNING, 0, "AT+ECCFG, invalid input 'PlmnSearchPowerLevel' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "RelVersion") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_REL_VERSION_VAL_MIN, ATC_ECCFG_1_REL_VERSION_VAL_MAX, ATC_ECCFG_1_REL_VERSION_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.relVersionPresent  = TRUE;
                            setExtCfgReq.relVersion         = (UINT8)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_14, P_WARNING, 0, "AT+ECCFG, invalid input 'RelVersion' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "Rohc") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_ROHC_VAL_MIN, ATC_ECCFG_1_ROHC_VAL_MAX, ATC_ECCFG_1_ROHC_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.rohcPresent    = TRUE;
                            setExtCfgReq.bRohc          = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_15, P_WARNING, 0, "AT+ECCFG, invalid input 'Rohc' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "Epco") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_EPCO_VAL_MIN, ATC_ECCFG_1_EPCO_VAL_MAX, ATC_ECCFG_1_EPCO_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.epcoPresent    = TRUE;
                            setExtCfgReq.enableEpco     = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_16, P_WARNING, 0, "AT+ECCFG, invalid input 'Epco' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "Ipv6RsForTestSim") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_IPV6RSFORTESTSIM_VAL_MIN, ATC_ECCFG_1_IPV6RSFORTESTSIM_VAL_MAX, ATC_ECCFG_1_IPV6RSFORTESTSIM_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.ipv6RsForTestSimPresent    = TRUE;
                            setExtCfgReq.bIpv6RsForTestSim          = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_Ipv6RsForTestSim_17, P_WARNING, 0, "AT+ECCFG, invalid input 'Ipv6RsForTestSim' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "Ipv6RsDelay") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_IPV6RSDELAY_VAL_MIN, ATC_ECCFG_1_IPV6RSDELAY_VAL_MAX, ATC_ECCFG_1_IPV6RSDELAY_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.ipv6GetPrefixTimePresent    = TRUE;
                            setExtCfgReq.ipv6GetPrefixTime           = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_Ipv6RsDealy_17, P_WARNING, 0, "AT+ECCFG, invalid input 'Ipv6RsDelay' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "DisableNCellMeas") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_DISABLENCELLMEAS_VAL_MIN, ATC_ECCFG_1_DISABLENCELLMEAS_VAL_MAX, ATC_ECCFG_1_DISABLENCELLMEAS_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.disableNCellMeasPresent    = TRUE;
                            setExtCfgReq.disableNCellMeas           = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_DisableNCellMeas_17, P_WARNING, 0, "AT+ECCFG, invalid input 'DisableNCellMeas' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "NbCategory") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_NBCATEGORY_VAL_MIN, ATC_ECCFG_1_NBCATEGORY_VAL_MAX, ATC_ECCFG_1_NBCATEGORY_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.nbCategoryPresent    = TRUE;
                            setExtCfgReq.nbCategory           = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_NbCategory_17, P_WARNING, 0, "AT+ECCFG, invalid input 'NbCategory' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "PsSoftReset") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_PSSOFTRESET_VAL_MIN, ATC_ECCFG_1_PSSOFTRESET_VAL_MAX, ATC_ECCFG_1_PSSOFTRESET_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.psSoftResetPresent = TRUE;
                            setExtCfgReq.bEnablePsSoftReset = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_PsSoftReset, P_WARNING, 0, "AT+ECCFG, invalid input 'PsSoftReset' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "DisableSib14Check") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECCFG_1_DISABLESIB14CHECK_VAL_MIN, ATC_ECCFG_1_DISABLESIB14CHECK_VAL_MAX, ATC_ECCFG_1_DISABLESIB14CHECK_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setExtCfgReq.disableSib14CheckPresent = TRUE;
                            setExtCfgReq.disableSib14Check = (BOOL)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_DisableSib14Check, P_WARNING, 0, "AT+ECCFG, invalid input 'DisableSib14Check' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strlen((const CHAR*)paraStr) > 0)   // other string
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_18, P_WARNING, 0, "AT+ECCFG, invalid input parameters");
                        cmdValid = FALSE;
                        break;
                    }
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECCFG_19, P_WARNING, 1, "AT+ECCFG, invalid parameters number: %d", paramNumb);
                cmdValid = FALSE;
            }

            if (cmdValid)
            {
                ret = devSetExtCfg(atHandle, &setExtCfgReq);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_TEST_REQ:
        {
            /*
             * AT+ECCFG=? test command
             * +ECCFG: ("AutoApn","TauForSms","")
             *\"GcfTest\",%d,\"AutoApn\",%d,\"SimTest\",%d,\"Rohc\",%d,\"Ipv6RsForTestSim\",%d,\"SupportSms\",%d,\"TauForSms\",%d,\"PlmnSearchPowerLevel\",%d,\"Epco\",%d,\"MultiCarrier\",%d,\"MultiTone\",%d,\"SupportUpRai\",%d,\"DataInactTimer\",%d,\"RelaxMonitorDeltaP\",%d,\"DisableNCellMeas\",%d",\"NbCategory\",%d",\"RelVersion\",%d"
            */
#ifdef  FEATURE_REF_AT_ENABLE
            ret = atcReply(atHandle, AT_RC_OK, 0,
                             (CHAR*)"(\"AutoApn\",\"PsSoftReset\",\"UsimPowerSave\",\"UsimSimulator\",\"SimBip\",\"Rohc\",\"Ipv6RsForTestSim\",\"Ipv6RsDelay\",\"PowerCfun\",\"PsPowerOnMaxDelay\",\"Cfun1PowerOnPs\",\"SupportSms\",\"TauForSms\",\"PlmnSearchPowerLevel\",\"Epco\",\"T3324MaxValueS\",\"BarValueS\",\"MultiCarrier\",\"MultiTone\",\"SupportUpRai\",\"DataInactTimer\",\"RelaxMonitorDeltaP\",\"DisableNCellMeas\",\"NbCategory\",\"RelVersion\",\"DisableSib14Check\")");
#else
            ret = atcReply(atHandle, AT_RC_OK, 0,
                             (CHAR*)"(\"AutoApn\",\"PsSoftReset\",\"UsimPowerSave\",\"UsimSimulator\",\"SimBip\",\"Rohc\",\"Ipv6RsForTestSim\",\"Ipv6RsDelay\",\"PowerCfun\",\"PsPowerOnMaxDelay\",\"SupportSms\",\"TauForSms\",\"PlmnSearchPowerLevel\",\"Epco\",\"T3324MaxValueS\",\"BarValueS\",\"MultiCarrier\",\"MultiTone\",\"SupportUpRai\",\"DataInactTimer\",\"RelaxMonitorDeltaP\",\"DisableNCellMeas\",\"NbCategory\",\"RelVersion\",\"DisableSib14Check\")");
#endif
            break;
        }

        case AT_EXEC_REQ:
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECRMFPLMN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECRMFPLMN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 val;

    switch (operaType)
    {
        case AT_SET_REQ:              /* AT+ECRMFPLMN= */
        {
            if (atGetNumValue(pParamList, 0, &val,
                              ATC_ECRMFPLMN_0_VAL_MIN, ATC_ECRMFPLMN_0_VAL_MAX, ATC_ECRMFPLMN_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                ret = devRmFPLMN(atHandle, val);
            }

            if(ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_TEST_REQ:
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECRMFPLMN: (0,1,2)");
            break;
        }

        case AT_EXEC_REQ:
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCMOLR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devCMOLR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 rc = AT_PARA_ERR;
    INT32 enable,method,horAccSet,horAcc,verReq,verAccSet,verAcc,velReq;
    INT32 reqMode,timeOut,interval,shapeReq,plane;
    UINT8* NMEAReq;
    UINT8* thirdPartyAddr;;
    UINT16 NMEAReqLen,thirdPartyAddrLen;
    CHAR tmpAtRspBuf[200] = {0};
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CMOLR=? */
        {
            sprintf((char*)tmpAtRspBuf, "+CMOLR: (0-3),(0-6),(0-1),(0-127),(0-1),(0-1),(0-127),(0-4),(0-1),(1-65535),(1-65535),(1,2,4,8,16,32,64),(0-1),,");
            ret = atcReply(atHandle, AT_RC_OK, 0, (char*)tmpAtRspBuf);
            break;
        }
        case AT_READ_REQ:           /* AT+CMOLR?  */
        {
            ret = devGetCMOLR(atHandle);
            break;
        }
        case AT_SET_REQ:      /* AT+CMOLR= */
        {
            if ((rc = atGetNumValue(pParamList, 0, &enable, ATC_CMOLR_0_ENABLE_VAL_MIN, ATC_CMOLR_0_ENABLE_VAL_MAX, ATC_CMOLR_0_ENABLE_VAL_DEFAULT)) != AT_PARA_ERR)
            {
                if (rc != AT_PARA_DEFAULT)
                {
                    enable = enable | 0x10000000;
                }
                if ((rc = atGetNumValue(pParamList, 1, &method, ATC_CMOLR_1_METHOD_VAL_MIN, ATC_CMOLR_1_METHOD_VAL_MAX, ATC_CMOLR_1_METHOD_VAL_DEFAULT)) != AT_PARA_ERR)
                {
                    if (rc != AT_PARA_DEFAULT)
                    {
                        method = method | 0x10000000;
                    }
                    if ((rc = atGetNumValue(pParamList, 2, &horAccSet, ATC_CMOLR_2_HORACCSET_VAL_MIN, ATC_CMOLR_2_HORACCSET_VAL_MAX, ATC_CMOLR_2_HORACCSET_VAL_DEFAULT)) != AT_PARA_ERR)
                    {
                        if (rc != AT_PARA_DEFAULT)
                        {
                            horAccSet = horAccSet | 0x10000000;
                        }
                        if ((rc = atGetNumValue(pParamList, 3, &horAcc, ATC_CMOLR_3_HORACC_VAL_MIN, ATC_CMOLR_3_HORACC_VAL_MAX, ATC_CMOLR_3_HORACC_VAL_DEFAULT)) != AT_PARA_ERR)
                        {
                            if ((rc = atGetNumValue(pParamList, 4, &verReq, ATC_CMOLR_4_VERREQ_VAL_MIN, ATC_CMOLR_4_VERREQ_VAL_MAX, ATC_CMOLR_4_VERREQ_VAL_DEFAULT)) != AT_PARA_ERR)
                            {
                                if (rc != AT_PARA_DEFAULT)
                                {
                                    verReq = verReq | 0x10000000;
                                }

                                if ((rc = atGetNumValue(pParamList, 5, &verAccSet, ATC_CMOLR_5_VERACCSET_VAL_MIN, ATC_CMOLR_5_VERACCSET_VAL_MAX, ATC_CMOLR_5_VERACCSET_VAL_DEFAULT)) != AT_PARA_ERR)
                                {
                                    if(rc != AT_PARA_DEFAULT)
                                    {
                                        verAccSet = verAccSet | 0x10000000;
                                    }

                                    if ((rc = atGetNumValue(pParamList, 6, &verAcc, ATC_CMOLR_6_VERACC_VAL_MIN, ATC_CMOLR_6_VERACC_VAL_MAX, ATC_CMOLR_6_VERACC_VAL_DEFAULT)) != AT_PARA_ERR)
                                    {
                                        if ((rc = atGetNumValue(pParamList, 7, &velReq, ATC_CMOLR_7_VELREQ_VAL_MIN, ATC_CMOLR_7_VELREQ_VAL_MAX, ATC_CMOLR_7_VELREQ_VAL_DEFAULT)) != AT_PARA_ERR)
                                        {
                                            if (rc != AT_PARA_DEFAULT)
                                            {
                                                velReq = velReq | 0x10000000;
                                            }
                                            if ((rc = atGetNumValue(pParamList, 8, &reqMode, ATC_CMOLR_8_REQMODE_VAL_MIN, ATC_CMOLR_8_REQMODE_VAL_MAX, ATC_CMOLR_8_REQMODE_VAL_DEFAULT)) != AT_PARA_ERR)
                                            {
                                                if (rc != AT_PARA_DEFAULT)
                                                {
                                                    reqMode = reqMode | 0x10000000;
                                                }

                                                if ((rc = atGetNumValue(pParamList, 9, &timeOut, ATC_CMOLR_9_TIMEOUT_VAL_MIN, ATC_CMOLR_9_TIMEOUT_VAL_MAX, ATC_CMOLR_9_TIMEOUT_VAL_DEFAULT)) != AT_PARA_ERR)
                                                {
                                                    if ((rc = atGetNumValue(pParamList, 10, &interval, ATC_CMOLR_10_INTERVAL_VAL_MIN, ATC_CMOLR_10_INTERVAL_VAL_MAX, ATC_CMOLR_10_INTERVAL_VAL_DEFAULT)) != AT_PARA_ERR)
                                                    {
                                                        if ((rc = atGetNumValue(pParamList, 11, &shapeReq, ATC_CMOLR_11_SHAPEREQ_VAL_MIN, ATC_CMOLR_11_SHAPEREQ_VAL_MAX, ATC_CMOLR_11_SHAPEREQ_VAL_DEFAULT)) != AT_PARA_ERR)
                                                        {
                                                            if (rc != AT_PARA_DEFAULT)
                                                            {
                                                                shapeReq = shapeReq | 0x10000000;
                                                            }

                                                            if ((rc = atGetNumValue(pParamList, 12, &plane, ATC_CMOLR_12_PLANE_VAL_MIN, ATC_CMOLR_12_PLANE_VAL_MAX, ATC_CMOLR_12_PLANE_VAL_DEFAULT)) != AT_PARA_ERR)
                                                            {
                                                                if (rc != AT_PARA_DEFAULT)
                                                                {
                                                                    plane = plane | 0x10000000;
                                                                }

                                                                NMEAReq = OsaAllocMemory(ATC_CMOLR_13_NMEAREQ_STR_MAX_LEN);
                                                                thirdPartyAddr = OsaAllocMemory(ATC_CMOLR_14_THIRDPARTYADDR_STR_MAX_LEN);
                                                                OsaCheck(NMEAReq != PNULL && thirdPartyAddr != PNULL, ATC_CMOLR_13_NMEAREQ_STR_MAX_LEN, ATC_CMOLR_14_THIRDPARTYADDR_STR_MAX_LEN, 0);
                                                                memset(NMEAReq, 0, ATC_CMOLR_13_NMEAREQ_STR_MAX_LEN);
                                                                memset(thirdPartyAddr, 0, ATC_CMOLR_14_THIRDPARTYADDR_STR_MAX_LEN);

                                                                if (atGetStrValue(pParamList, 13, NMEAReq, ATC_CMOLR_13_NMEAREQ_STR_MAX_LEN, &NMEAReqLen, NULL) != AT_PARA_ERR)
                                                                {
                                                                    if (atGetStrValue(pParamList, 14, thirdPartyAddr, ATC_CMOLR_14_THIRDPARTYADDR_STR_MAX_LEN, &thirdPartyAddrLen, NULL) != AT_PARA_ERR)
                                                                    {
                                                                        ret = devSetCMOLR(atHandle, enable, method, horAccSet, horAcc, verReq, verAccSet, verAcc, velReq, reqMode, timeOut, interval, shapeReq, plane,
                                                                                          NMEAReq, thirdPartyAddr, pAtCmdReq->paramRealNum);

                                                                    }
                                                                }
                                                                free(NMEAReq);
                                                                free(thirdPartyAddr);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:        /* AT+CMOLR */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devCMTLR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devCMTLR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 subscribe;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CMTLR=? */
        {
            ret = devGetCMTLRCapa(atHandle);
            break;
        }
        case AT_READ_REQ:           /* AT+CMTLR?  */
        {
            //sprintf(tmpAtRspBuf, "+CMTLR: <subscribe>");
            //ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)tmpAtRspBuf);
            ret = devGetCMTLR(atHandle);
            break;
        }
        case AT_SET_REQ:      /* AT+CMTLR= */
        {
            if (atGetNumValue(pParamList, 0, &subscribe, ATC_CMTLR_0_VAL_MIN, ATC_CMTLR_0_VAL_MAX, ATC_CMTLR_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = devSetCMTLR(atHandle, subscribe);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:        /* AT+CMTLR */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devCMTLRA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devCMTLRA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 allow;
    INT32 handleId;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CMTLRA=? */
        {
            ret = devGetCMTLRACapa(atHandle);
            break;
        }
        case AT_READ_REQ:           /* AT+CMTLRA?  */
        {
            ret = devGetCMTLRA(atHandle);
            break;
        }
        case AT_SET_REQ:      /* AT+CMTLRA= */
        {
            if (atGetNumValue(pParamList, 0, &allow, ATC_CMTLRA_0_VAL_MIN, ATC_CMTLRA_0_VAL_MAX, ATC_CMTLRA_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if (atGetNumValue(pParamList, 1, &handleId, ATC_CMTLRA_1_VAL_MIN, ATC_CMTLRA_1_VAL_MAX, ATC_CMTLRA_1_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    ret = devSetCMTLRA(atHandle, allow, handleId);
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:        /* AT+CMTLRA */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId devECSTATUS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 paramNumb = pAtCmdReq->paramRealNum;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   paraStr[ATC_ECSTATUS_0_MAX_PARM_STR_LEN] = {0};
    UINT16   paraLen = 0;
    BOOL    cmdValid = TRUE;
    UINT8   module = CMI_DEV_GET_ECSTATUS;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+ECSTATUS=? */
        {
            /*OK*/
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECSTATUS: (PHY,L2,RRC,EMM,PLMN,ESM,CCM)");
            break;
        }

        case AT_EXEC_REQ:        /* AT+ECSTATUS */
        {
            devGetESTATUS(atHandle, CMI_DEV_GET_ECSTATUS);
            ret = CMS_RET_SUCC;
            break;
        }

        case AT_SET_REQ:           /* AT+ECSTATUS= */
        {
            if (paramNumb == 1)
            {
                memset(paraStr, 0x00, ATC_ECSTATUS_0_MAX_PARM_STR_LEN);
                if (atGetStrValue(pParamList, 0, paraStr,
                                   ATC_ECSTATUS_0_MAX_PARM_STR_LEN, &paraLen, ATC_ECSTATUS_0_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECSTATUS_1, P_WARNING, 0, "AT+ECSTATUS, invalid input string parameter");
                    cmdValid = FALSE;
                    break;
                }

                if (strcasecmp((const CHAR*)paraStr, "PHY") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_PHY;
                }
                else if (strcasecmp((const CHAR*)paraStr, "L2") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_L2;
                }
                else if (strcasecmp((const CHAR*)paraStr, "RRC") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_RRC;
                }
                else if (strcasecmp((const CHAR*)paraStr, "EMM") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_EMM;
                }
                else if (strcasecmp((const CHAR*)paraStr, "PLMN") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_PLMN;
                }
                else if (strcasecmp((const CHAR*)paraStr, "ESM") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_ESM;
                }
                else if (strcasecmp((const CHAR*)paraStr, "CCM") == 0)
                {
                    module = CMI_DEV_GET_ECSTATUS_CCM;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECSTATUS_2, P_WARNING, 0, "AT+ECSTATUS, invalid input string parameter");
                    cmdValid = FALSE;
                    break;
                }

                if (cmdValid)
                {
                    ret = devGetESTATUS(atHandle, module);
                }

                if (ret != CMS_RET_SUCC)
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
                break;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECSTATUS_3, P_WARNING, 1, "AT+ECSTATUS, invalid parameters number: %d", paramNumb);
            }
            break;
        }

        case AT_READ_REQ:           /* AT+ECSTATUS? */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECSTATIS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECSTATIS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32       periodS = 0;

    switch (operaType)
    {
        case AT_SET_REQ:           /* AT+ECSTATIS= */
            if (atGetNumValue(pParamList, 0, &periodS,
                              ATC_ESTATIS_0_VAL_MIN, ATC_ESTATIS_0_VAL_MAX, ATC_ESTATIS_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                ret = devSetESTATIS(atHandle, periodS);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;

        case AT_TEST_REQ:           /* AT+ECSTATIS=?, response: OK */
        {
            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, "+ECSTATIS: (0,5-600)");
            break;
        }

        case AT_READ_REQ:           /* AT+ECSTATIS?  */
        case AT_EXEC_REQ:           /* AT+ECSTATIS */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECBCINFO(const AtCmdInputContext *pAtCmdReq)
  \brief       AT CMD: AT+ECBCINFO handling
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECBCINFO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    CmiDevGetBasicCellListInfoReq   bcInfoReq;
    INT32       mode = 0, maxTimeOutS = 0, saveForLater = 0, reqCellNum = 0, rptMode = 0;

    /* set the default value */
    bcInfoReq.mode      = CMI_DEV_GET_BASIC_CELL_MEAS;
    bcInfoReq.needSave  = FALSE;
    bcInfoReq.maxTimeoutS   = ATC_ECBCINFO_1_VAL_DEFAULT;   //8s
    bcInfoReq.reqMaxCellNum = 5;    //1 serving cell + 4 neighber cells
    bcInfoReq.rptMode       = CMI_DEV_BCINFO_SYN_RPT;
    bcInfoReq.rsvd          = 0;

    switch (operaType)
    {
        case AT_EXEC_REQ:       /*AT+ECBCINFO*/
            ret = devGetECBCINFO(atHandle, &bcInfoReq);
            break;

        case AT_TEST_REQ:       /*AT+ECBCINFO=?*/
            ret = atcReply(atHandle, AT_RC_OK, 0, "+ECBCINFO: (0,1,2),(4-300),(0,1),(1-5),(0,1)");
            break;

        case AT_SET_REQ:        /* AT+ECBCINFO[=<mode>[,<timeoutS>[,<save_for_later>[,<max_cell_number>[,<report_mode>]]]]] */
        {
            ret = CMS_RET_SUCC;

            /* <mode>, if set type, <mode> is must */
            if (atGetNumValue(pParamList, 0, &mode,
                              ATC_ECBCINFO_0_VAL_MIN, ATC_ECBCINFO_0_VAL_MAX, ATC_ECBCINFO_0_VAL_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECBCINFO_warning_1, P_WARNING, 0,
                            "AT+ECBCINFO, invalid <mode> set");
                ret = CMS_FAIL;
            }

            /* <timeoutS> */
            if (ret == CMS_RET_SUCC &&
                atGetNumValue(pParamList, 1, &maxTimeOutS,
                              ATC_ECBCINFO_1_VAL_MIN, ATC_ECBCINFO_1_VAL_MAX, ATC_ECBCINFO_1_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECBCINFO_warning_2, P_WARNING, 0,
                            "AT+ECBCINFO, invalid <timeoutS> set");
                ret = CMS_FAIL;
            }

            /* <save_for_later> */
            if (ret == CMS_RET_SUCC &&
                atGetNumValue(pParamList, 2, &saveForLater,
                              ATC_ECBCINFO_2_VAL_MIN, ATC_ECBCINFO_2_VAL_MAX, ATC_ECBCINFO_2_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECBCINFO_warning_3, P_WARNING, 0,
                            "AT+ECBCINFO, invalid <save_for_later> set");
                ret = CMS_FAIL;
            }

            /* <max_cell_number> */
            if (ret == CMS_RET_SUCC &&
                atGetNumValue(pParamList, 3, &reqCellNum,
                              ATC_ECBCINFO_3_VAL_MIN, ATC_ECBCINFO_3_VAL_MAX, ATC_ECBCINFO_3_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECBCINFO_warning_4, P_WARNING, 0,
                            "AT+ECBCINFO, invalid <max_cell_number> set");
                ret = CMS_FAIL;
            }

            /* <report_mode> */
            if (ret == CMS_RET_SUCC &&
                atGetNumValue(pParamList, 4, &rptMode,
                              ATC_ECBCINFO_4_VAL_MIN, ATC_ECBCINFO_4_VAL_MAX, ATC_ECBCINFO_4_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECBCINFO_warning_5, P_WARNING, 0,
                            "AT+ECBCINFO, invalid <report_mode> set");
                ret = CMS_FAIL;
            }

            if (ret == CMS_RET_SUCC)
            {
                bcInfoReq.mode      = (UINT8)mode;
                bcInfoReq.needSave  = (BOOL)(saveForLater);
                bcInfoReq.maxTimeoutS   = (UINT16)maxTimeOutS;
                bcInfoReq.reqMaxCellNum = (UINT8)reqCellNum;
                bcInfoReq.rptMode       = (UINT8)rptMode;
                bcInfoReq.rsvd          = 0;

                ret = devGetECBCINFO(atHandle, &bcInfoReq);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, PNULL);
            }

            break;
        }

        case AT_READ_REQ:           /* AT+ECBCINFO?  */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECPSTEST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECPSTEST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    INT32       enablePsTest;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+ECPSTEST=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECPSTEST: (0,1)");
            break;
        }
        case AT_READ_REQ:   /* AT+ECPSTEST? */
        {
            ret = devGetECPSTEST(atHandle);
            break;
        }
        case AT_SET_REQ:    /* AT+ECPSTEST= */
        {
            if (atGetNumValue(pParamList, 0, &enablePsTest, ATC_ECPSTEST_VAL_MIN, ATC_ECPSTEST_VAL_MAX, ATC_ECPSTEST_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = devSetECPSTEST(atHandle, enablePsTest);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, devECPSTEST_1, P_WARNING, 0, "ATCMD, invalid ATCMD: AT+ECPSTEST");
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }
        case AT_EXEC_REQ:   /* AT+ECPSTEST */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECPOWERCLASS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECPOWERCLASS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    INT32       band;
    INT32       powerClass;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    BOOL        validAT = TRUE;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+ECPOWERCLASS=? */
        {
            ret = devGetSupportedBandPowerClassReq(atHandle);
            break;
        }
        case AT_READ_REQ:   /* AT+ECPOWERCLASS? */
        {
            ret = devGetECPOWERCLASS(atHandle);
            break;
        }
        case AT_SET_REQ:    /* AT+ECPOWERCLASS= */
        {
            CmiDevSetPowerClassReq    setPowerClassReq;

            /* band */
            if (atGetNumValue(pParamList, 0, &band, ATC_ECPOWERCLASS_1_VAL_MIN, ATC_ECPOWERCLASS_1_VAL_MAX, ATC_ECPOWERCLASS_1_VAL_DEFAULT) != AT_PARA_ERR)
            {
                setPowerClassReq.band = (UINT8)band;
            }
            else
            {
                validAT = FALSE;
            }

            /* power class */
            if (validAT)
            {
                if (atGetNumValue(pParamList, 1, &powerClass, ATC_ECPOWERCLASS_2_VAL_MIN, ATC_ECPOWERCLASS_2_VAL_MAX, ATC_ECPOWERCLASS_2_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    setPowerClassReq.powerClass = (UINT8)powerClass;
                }
                else
                {
                    validAT = FALSE;
                }
            }

            if (validAT)
            {
                ret = devSetECPOWERCLASS(atHandle, &setPowerClassReq);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, devECPOWERCLASS_1, P_WARNING, 0, "ATCMD, invalid ATCMD: AT+ECPOWERCLASS");
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }
        case AT_EXEC_REQ:   /* AT+ECPOWERCLASS */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECEVENTSTATIS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECEVENTSTATIS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    INT32       mode;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    BOOL        validAT = TRUE;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECEVENTSTATIS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECEVENTSTATIS: (0,1,2)");
            break;
        }
        case AT_READ_REQ:   /* AT+ECEVENTSTATIS? */
        {
            ret = devGetEVENTSTATIS(atHandle);
            break;
        }
        case AT_SET_REQ:    /* AT+ECEVENTSTATIS= */
        {
            CmiDevSetEventStatisReq    setEventStatisReq;
            memset(&setEventStatisReq, 0, sizeof(CmiDevSetEventStatisReq));
            /* mode */
            if (atGetNumValue(pParamList, 0, &mode, ATC_ECEVENTSTATIS_1_VAL_MIN, ATC_ECEVENTSTATIS_1_VAL_MAX, ATC_ECEVENTSTATIS_1_VAL_DEFAULT) != AT_PARA_ERR)
            {
                setEventStatisReq.mode = (UINT8)mode;
            }
            else
            {
                validAT = FALSE;
            }
            if (validAT)
            {
                ret = devSetEVENTSTATIS(atHandle, &setEventStatisReq);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, devECEVENTSTATIS_1, P_WARNING, 0, "ATCMD, invalid ATCMD: AT+ECEVENTSTATIS");
            }

            break;
        }
        case AT_EXEC_REQ:   /* AT+ECEVENTSTATIS */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId devECNBR14(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECNBR14(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_EXEC_REQ:           /* AT+ECNBR14 */
        {
            ret = devGetECNBR14(atHandle);
            break;
        }

        case AT_TEST_REQ:           /* AT+ECNBR14=?, response: OK */
        {
            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, PNULL);
            break;
        }

        case AT_READ_REQ:           /* AT+ECNBR14?  */
        case AT_SET_REQ:            /* AT+ECNBR14= */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId devECPSSLPCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  devECPSSLPCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   paramNumb = pAtCmdReq->paramRealNum;
    BOOL    cmdValid = TRUE;
    UINT8   paraStr[ATC_ECPSSLPCFG_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 32;
    UINT16  paraLen = 0;
    INT32   paraValue = -1;
    UINT8   paraIdx = 0;
    CmiDevSetPsSleepCfgReq  setPsSlpCfgReq;

    memset(&setPsSlpCfgReq, 0x00, sizeof(CmiDevSetPsSleepCfgReq));

    switch (operaType)
    {
        case AT_READ_REQ:            /* AT+ECPSSLPCFG? */
        {
            ret = devGetPSSLPCfg(atHandle);
            break;
        }

        case AT_SET_REQ :
        {
            if ((paramNumb > 0) && (paramNumb & 0x01) == 0)
            {
                for (paraIdx = 0; paraIdx < paramNumb; paraIdx++)
                {
                    memset(paraStr, 0x00, ATC_ECPSSLPCFG_0_MAX_PARM_STR_LEN);

                    if (atGetStrValue(pParamList, paraIdx, paraStr,
                                      ATC_ECPSSLPCFG_0_MAX_PARM_STR_LEN, &paraLen, ATC_ECPSSLPCFG_0_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPSSLPCFG_1, P_WARNING, 0, "AT+ECPSSLPCFG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }

                    //next parameters
                    paraIdx++;

                    if (strcasecmp((const CHAR*)paraStr, "minT3324") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPSSLPCFG_1_VAL_MIN, ATC_ECPSSLPCFG_1_VAL_MAX, ATC_ECPSSLPCFG_1_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setPsSlpCfgReq.t3324Present = TRUE;
                            setPsSlpCfgReq.minT3324ValueS = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPSSLPCFG_2, P_WARNING, 0, "AT+ECPSSLPCFG, invalid input 'minT3324' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "minT3412") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPSSLPCFG_2_VAL_MIN, ATC_ECPSSLPCFG_2_VAL_MAX, ATC_ECPSSLPCFG_2_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setPsSlpCfgReq.t3412Present = TRUE;
                            setPsSlpCfgReq.minT3412ValueS = (UINT32)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPSSLPCFG_3, P_WARNING, 0, "AT+ECPSSLPCFG, invalid input 'minT3412' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strcasecmp((const CHAR*)paraStr, "minEDRXPeriod") == 0)
                    {
                        if (atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPSSLPCFG_3_VAL_MIN, ATC_ECPSSLPCFG_3_VAL_MAX, ATC_ECPSSLPCFG_3_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            setPsSlpCfgReq.edrxPresent = TRUE;
                            setPsSlpCfgReq.minEdrxPeriodS = (UINT16)paraValue;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPSSLPCFG_4, P_WARNING, 0, "AT+ECPSSLPCFG, invalid input 'minEDRXPeriod' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else if (strlen((const CHAR*)paraStr) > 0)   // other string
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPSSLPCFG_5, P_WARNING, 0, "AT+ECPSSLPCFG, invalid input parameters");
                        cmdValid = FALSE;
                        break;
                    }
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPSSLPCFG_6, P_WARNING, 1, "AT+ECPSSLPCFG, invalid parameters number: %d", paramNumb);
                cmdValid = FALSE;
            }

            if (cmdValid)
            {
                ret = devSetPSSLPCfg(atHandle, &setPsSlpCfgReq);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_TEST_REQ:
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+ECPSSLPCFG: (0-65535),(0-4294967295),(0-65535)");
            break;
        }

        case AT_EXEC_REQ:
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

#ifdef CUSTOMER_DAHUA
CmsRetId  pdevHXSTATUS(const AtCmdInputContext *pAtCmdReq)
{
	CmsRetId	ret = CMS_FAIL;
	UINT32	atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
	UINT32	operaType = (UINT32)pAtCmdReq->operaType;

	switch (operaType)
	{
		case AT_EXEC_REQ:		/* AT+HXSTATUS */
		{
			// devGetESTATUS(atHandle, CMI_DEV_GET_HX_STATUS_CNF);
			devGetESTATUS(atHandle, CMI_DEV_GET_NUESTATS);
			ret = CMS_RET_SUCC;
			break;
		}

		case AT_TEST_REQ:		  /* AT+HXSTATUS=? */
		{
			/* NUESTATS: (RADIO,CELL,BLER,THP,APPSEM,ALL) */
			ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
			break;
		}

		case AT_READ_REQ:		   /* AT+HXSTATUS? */
		default:
		{
			devGetESTATUS(atHandle, CMI_DEV_GET_NUESTATS);
			ret = CMS_RET_SUCC;
			break;
		}
	}

    return ret;
}
#endif

