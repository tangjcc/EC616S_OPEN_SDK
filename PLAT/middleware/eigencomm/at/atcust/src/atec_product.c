/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_product.c
*
*  Description: Process production related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "atec_product.h"
#include "atc_decoder.h"
#include "at_util.h"
#include "cmicomm.h"
#include "nvram.h"
#include "hal_uart.h"
#include "plat_config.h"
#include "npi_config.h"
#include "ec_misc_api.h"

UINT32 EcSleepFlag = ECSLEEP_HANDLE_NOT_CREATED;
extern INT32 RfAtTestCmd(UINT8* dataIn, UINT16 length, UINT8* dataOut, UINT16* lenOut);
extern INT32 RfAtNstCmd(UINT8* dataIn, UINT16 length, UINT8* dataOut, UINT16* lenOut);
extern void slpManProductionTest(uint8_t mode);
extern bool atCmdIsBaudRateValid(uint32_t baudRate);
extern uint8_t* atCmdGetBaudRateString(void);
extern void atCmdUartChangeConfig(UINT8 atChannel, UINT32 newBaudRate, UINT32 newFrameFormat, CHAR saveFlag);

#define _DEFINE_AT_REQ_FUNCTION_LIST_

/**
  \fn          CmsRetId pdevECRFTEST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for RF cal
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECRFTEST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 *rfBufInTemp = NULL;
    UINT8 *rfBufIn = NULL;
    UINT8 *rfBufOut = NULL;
    UINT16 lenIn = 0;
    UINT16 lenOut = 0;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:               /* AT+ECRFTEST=? */
        case AT_READ_REQ:                /* AT+ECRFTEST?  */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECRFTEST\r\n");
            break;
        }

        case AT_SET_REQ:              /* AT+ECRFTEST= */
        {
            rfBufInTemp = malloc(ATC_ECRFTEST_0_STR_MAX_LEN);
            rfBufOut = malloc(ATC_ECRFTEST_0_STR_MAX_LEN);
            if(rfBufInTemp == NULL)
            {
                ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
                return ret;
            }
            if(rfBufOut == NULL)
            {
                free(rfBufInTemp);
                ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
                return ret;
            }
            memset(rfBufInTemp, 0, ATC_ECRFTEST_0_STR_MAX_LEN);
            memset(rfBufOut, 0, ATC_ECRFTEST_0_STR_MAX_LEN);
            if(atGetStrValue(pParamList, 0, rfBufInTemp, ATC_ECRFTEST_0_STR_MAX_LEN, &lenIn, NULL) != AT_PARA_ERR)
            {
                rfBufIn = malloc(lenIn);
                if(rfBufIn == NULL)
                {
                    free(rfBufInTemp);
                    free(rfBufOut);
                    ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
                    return ret;
                }
                memset(rfBufIn, 0, lenIn);
                cmsHexStrToHex(rfBufIn, lenIn, (CHAR *)rfBufInTemp, lenIn);

                RfAtTestCmd(rfBufIn, lenIn/2, rfBufOut, &lenOut);

                if(lenOut == 0)
                {
                    ret = atcReply(atHandle, AT_RC_ERROR, 0, NULL);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rfBufOut);
                }

                free(rfBufInTemp);
                free(rfBufIn);
                free(rfBufOut);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                free(rfBufInTemp);
                free(rfBufOut);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+ECRFTEST */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId pdevECRFNST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for RF cal
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECRFNST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 rc = 0xff;
    UINT8 *rfBufInTemp = NULL;
    UINT8 *rfBufIn = NULL;
    UINT8 *rfBufOut = NULL;
    UINT16  lenIn = 0;
    UINT16  lenOut = 0;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECRFNST=? */
        case AT_READ_REQ:            /* AT+ECRFNST?  */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECRFNST\r\n");
            break;
        }

        case AT_SET_REQ:              /* AT+ECRFNST= */
        {
            rfBufInTemp = malloc(ATC_ECRFNST_0_STR_MAX_LEN);
            rfBufOut = malloc(ATC_ECRFNST_0_STR_MAX_LEN);
            if(rfBufInTemp == NULL)
            {
                ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
                return ret;
            }
            if(rfBufOut == NULL)
            {
                free(rfBufInTemp);
                ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
                return ret;
            }
            memset(rfBufInTemp, 0, ATC_ECRFNST_0_STR_MAX_LEN);
            memset(rfBufOut, 0, ATC_ECRFNST_0_STR_MAX_LEN);

            if(atGetStrValue(pParamList, 0, rfBufInTemp, ATC_ECRFNST_0_STR_MAX_LEN, &lenIn, NULL) != AT_PARA_ERR)
            {
                rfBufIn = malloc(lenIn);
                if(rfBufIn == NULL)
                {
                    free(rfBufInTemp);
                    free(rfBufOut);
                    ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
                    return ret;
                }
                memset(rfBufIn, 0, lenIn);
                //atStrToHex(rfBufIn, rfBufInTemp, lenIn/2);
                cmsHexStrToHex(rfBufIn, lenIn, (CHAR *)rfBufInTemp, lenIn);
                rc = RfAtNstCmd(rfBufIn, lenIn/2, rfBufOut, &lenOut);

                if(rc == 0)
                {
                    if(lenOut == 0)
                    {
                        ret = atcReply(atHandle, AT_RC_ERROR, 0, NULL);
                    }
                    else
                    {
                        ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rfBufOut);
                    }
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_ERROR, 0, NULL);
                }

                free(rfBufInTemp);
                free(rfBufIn);
                free(rfBufOut);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                free(rfBufInTemp);
                free(rfBufOut);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+ECRFNST */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId ccECSLEEP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for prduction line sleep feature power consumption test
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccECSLEEP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[64] = {0};
    INT32 outValue = 0;

    extern void atCmdUartSwitchSleepMode(UINT8 mode);

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSLEEP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECSLEEP: <state>");
            break;
        }

        case AT_SET_REQ:          /* AT+ECSLEEP */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &outValue, CC_ECSLEEP_VALUE_MIN, CC_ECSLEEP_VALUE_MAX, CC_ECSLEEP_VALUE_DEF)) != AT_PARA_ERR)
            {
                switch(outValue)
                {
                    case ECSLEEP_HIB2:
                        sprintf(RspBuf, "+ECSLEEP: HIB2");
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, "OK");
                        atCmdUartSwitchSleepMode(3);
                        break;

                    case ECSLEEP_HIB1:
                        sprintf(RspBuf, "+ECSLEEP: HIB1");
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, "OK");
                        atCmdUartSwitchSleepMode(2);
                        break;

                    case ECSLEEP_SLP2:
                        sprintf(RspBuf, "+ECSLEEP: SLEEP2");
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, "OK");
                        atCmdUartSwitchSleepMode(1);
                        break;

                    case ECSLEEP_SLP1:
                        sprintf(RspBuf, "+ECSLEEP: SLEEP1");
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);
                        ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, "OK");
                        atCmdUartSwitchSleepMode(0);
                        break;

                    default:
                        ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        break;
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}




/**
  \fn          CmsRetId ccECSAVEFAC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to save factory recover region
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccECSAVEFAC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[64] = {0};
    UINT8 paraStr[SAVEFAC_0_STR_BUF_LEN];
    UINT32 nv_ret=0;
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSAVEFAC=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+SAVEFAC: <mode>");
            break;
        }

        case AT_SET_REQ:          /* AT+ECSAVEFAC */
        {
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, paraStr, SAVEFAC_0_STR_BUF_LEN, &len , (CHAR *)SAVEFAC_0_STR_DEF)) != AT_PARA_ERR)
            {
                if(strncasecmp((const CHAR*)paraStr, "all", strlen("all")) == 0)
                {
                    //func
                    nv_ret=nvram_sav2fac_at(SAVE_ALL);
                    if(nv_ret==0)
                    {
                        sprintf(RspBuf, "+ECSAVEFAC: OK");
                    }
                    else
                    {
                        sprintf(RspBuf, "+ECSAVEFAC: ERR,%d",nv_ret);
                    }
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
                }
                else if(strncasecmp((const CHAR*)paraStr, "rfregion", strlen("rfregion")) == 0)
                {
                    //func
                    nv_ret=nvram_sav2fac_at(SAVE_CALI);
                    if(nv_ret==0)
                    {
                        sprintf(RspBuf, "+ECSAVEFAC: OK");
                    }
                    else
                    {
                        sprintf(RspBuf, "+ECSAVEFAC: ERR,%d",nv_ret);
                    }
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
                }
                else if(strncasecmp((const CHAR*)paraStr, "other", strlen("other")) == 0)
                {
                    //func
                    nv_ret=nvram_sav2fac_at(SAVE_OTHER);
                    if(nv_ret==0)
                    {
                        sprintf(RspBuf, "+ECSAVEFAC: OK");
                    }
                    else
                    {
                        sprintf(RspBuf, "+ECSAVEFAC: ERR,%d",nv_ret);
                    }
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }

            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId pdevECIPR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd ECIPR
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECIPR(const AtCmdInputContext *pAtCmdReq)
{
    extern uint8_t* atCmdGetECIPRString(void);

    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR rspBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    UINT8 paraStr[ATC_ECIPR_MAX_PARM_STR_LEN] = {0};

    static UINT32 currentBaudRate = 0;

    UINT16  paraLen = 0;

    UINT32 baudRateToSet = 0;

    plat_config_fs_t* fsPlatConfigPtr = BSP_GetFsPlatConfig();

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECIPR=? */
        {
            snprintf((char*)rspBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPR:(%s)", (CHAR *)atCmdGetBaudRateString());
            ret = atcReply(atHandle, AT_RC_OK, 0, rspBuf);
            break;
        }

        case AT_READ_REQ:            /* AT+ECIPR?  */
        {
            if(currentBaudRate == 0)
            {
                if(fsPlatConfigPtr)
                {
                    snprintf((char*)rspBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPR: %d", (fsPlatConfigPtr->atPortBaudRate & 0x80000000UL) ? 0 : (fsPlatConfigPtr->atPortBaudRate & 0x7FFFFFFFUL));
                }
            }
            else
            {
                snprintf((char*)rspBuf, ATEC_IND_RESP_128_STR_LEN, "+ECIPR: %d", currentBaudRate);
            }

            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rspBuf);

            break;
        }

        case AT_SET_REQ:              /* AT+ECIPR= */
        {
            if(atGetStrValue(pParamList, 0, paraStr, ATC_ECIPR_MAX_PARM_STR_LEN, &paraLen, ATC_ECIPR_MAX_PARM_STR_DEFAULT) != AT_PARA_ERR)
            {
                char* end;
                baudRateToSet = strtol((char *)paraStr, &end, 10);

                if(((char *)paraStr == end) || (baudRateToSet == 0) ||atCmdIsBaudRateValid(baudRateToSet) == false)
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                    break;
                }

                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                if(currentBaudRate !=  baudRateToSet)
                {
                    currentBaudRate = baudRateToSet;

                    atCmdUartChangeConfig(pAtCmdReq->chanId, baudRateToSet, fsPlatConfigPtr->atPortFrameFormat.wholeValue, 0);

                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+ECIPR */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId pdevNPICFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for NPI config
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevNPICFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 paramNumb = pAtCmdReq->paramRealNum;

    CHAR RspBuf[128] = {0};
    UINT8 paraStr[ATC_ECNPICFG_MAX_PARM_STR_LEN] = {0};

    UINT16  paraLen = 0;
    INT32   paraValue = 0;
    UINT8   paraIdx = 0;

    UINT8   configValidFlag = 0;
    BOOL    valueChanged = false;
    BOOL    valueChangedTemp = false;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECNPICFG=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECNPICFG:<option>,<setting>\r\n");
            break;
        }
        case AT_READ_REQ:            /* AT+ECNPICFG?  */
        {
            snprintf((char*)RspBuf, 128, "+ECNPICFG: \"rfCaliDone\":%d,\"rfNSTDone\":%d\r\n",
                                          npiGetProcessStatusItemValue(NPI_PROCESS_STATUS_ITEM_RFCALI),
                                          npiGetProcessStatusItemValue(NPI_PROCESS_STATUS_ITEM_RFNST));

            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            break;
        }

        case AT_SET_REQ:              /* AT+ECNPICFG= */
        {

            /*
             * AT+ECPCFG="rfCaliDone",0/1,"rfNSTDone",0/1
             */
            if((paramNumb > 0) && (paramNumb & 0x01) == 0)
            {
                for(paraIdx = 0; paraIdx < paramNumb; paraIdx++)
                {
                    memset(paraStr, 0x00, ATC_ECNPICFG_MAX_PARM_STR_LEN);
                    if(atGetStrValue(pParamList, paraIdx, paraStr,
                                     ATC_ECNPICFG_MAX_PARM_STR_LEN, &paraLen, ATC_ECNPICFG_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, pdevNPICFG_1, P_WARNING, 0, "AT+ECNPICFG, invalid input string parameter");
                        break;
                    }

                    // next parameters
                    paraIdx++;
                    if(strncasecmp((const CHAR*)paraStr, "rfCaliDone", strlen("rfCaliDone")) == 0)
                    {
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                         ATC_ECNPICFG_RFCALI_VAL_MIN, ATC_ECNPICFG_RFCALI_VAL_MAX, ATC_ECNPICFG_RFCALI_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            npiSetProcessStatusItemValue(NPI_PROCESS_STATUS_ITEM_RFCALI, paraValue, &valueChangedTemp);
                            valueChanged |= valueChangedTemp;
                            configValidFlag = 1;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, pdevNPICFG_2, P_WARNING, 0, "AT+ECNPICFG, invalid input 'rfCaliDone' value");
                            configValidFlag = 0;
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "rfNSTDone", strlen("rfNSTDone")) == 0)
                    {
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECNPICFG_RFNST_VAL_MIN, ATC_ECNPICFG_RFNST_VAL_MAX, ATC_ECNPICFG_RFNST_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            npiSetProcessStatusItemValue(NPI_PROCESS_STATUS_ITEM_RFNST, paraValue, &valueChangedTemp);
                            valueChanged |= valueChangedTemp;
                            configValidFlag = 1;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, pdevNPICFG_3, P_WARNING, 0, "AT+ECNPICFG, invalid input 'rfNSTDone' value");
                            configValidFlag = 0;
                            break;
                        }
                    }

                }
            }

            if(configValidFlag == 0)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            else
            {
                if(valueChanged)
                {
                    npiSaveConfigToNV10();
                }
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply( atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }


    return ret;
}

/**
  \fn          CmsRetId pdevECRFSTAT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show whether RF was calibrated
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECRFSTAT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    BOOL validFlag = FALSE;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECRFSTAT=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECRFSTAT:<status>\r\n");
            break;
        }
        case AT_EXEC_REQ:            /* AT+ECRFSTAT */
        {
            validFlag = npiGetProcessStatusItemValue(NPI_PROCESS_STATUS_ITEM_RFCALI);
            if(validFlag == FALSE)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECRFSTAT: not calibrate\r\n");
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECRFSTAT: calibrate done\r\n");
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

#ifdef FEATURE_PRODMODE_ENABLE
CmsRetId pdevECPRODMODE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[64] = {0};
    UINT8 paraStr[ATC_ECPRODMODE_MAX_PARM_STR_LEN];
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECPRODMODE=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECPRODMODE: <status>");
            break;
        }

        case AT_READ_REQ:            /* AT+ECPRODMODE?  */
        {
            snprintf((char*)RspBuf, 64, "+ECPRODMODE: \"prodModeEnable\":%d,\"prodModeEnter\":%d\r\n",
                                          npiGetProdModeEnableStatus(),
                                          npiIsProdModeOngoing());

            ret = atcReply(reqHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            break;
        }

        case AT_SET_REQ:          /* AT+ECPRODMODE= */
        {
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, paraStr, SAVEFAC_0_STR_BUF_LEN, &len , (CHAR *)SAVEFAC_0_STR_DEF)) != AT_PARA_ERR)
            {
                if(strncasecmp((const CHAR*)paraStr, "prodModeEnable", strlen("prodModeEnable")) == 0)
                {
                    npiSetProdModeEnableStatus(true);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
                else if(strncasecmp((const CHAR*)paraStr, "prodModeDisable", strlen("prodModeDisable")) == 0)
                {
                    npiSetProdModeEnableStatus(false);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
                else if(strncasecmp((const CHAR*)paraStr, "prodModeEnter", strlen("prodModeEnter")) == 0)
                {
                    npiSetProdModeEnterFlag();
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
                else if(strncasecmp((const CHAR*)paraStr, "prodModeExit", strlen("prodModeExit")) == 0)
                {
                    npiSetProdModeExitFlag();
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }

            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}
#endif

