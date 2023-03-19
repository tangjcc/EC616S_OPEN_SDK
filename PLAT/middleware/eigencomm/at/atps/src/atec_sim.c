/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_sim.c
*
*  Description: Process SIM related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "cmisim.h"
#include "atec_sim.h"
#include "ps_sim_if.h"
#include "atec_sim_cnf_ind.h"


#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId simCRSM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCRSM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 command = 0, fileId = 0, p1 = 0, p2 = 0, p3 = 0;
    UINT16 dataLen = 0, pathLen = 0;
    UINT8 *data = PNULL;
    UINT8 pathId[ATC_CRSM_6_PATH_STR_MAX_LEN+1] = {0};/*+1 for "\0"*/

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CRSM=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:         /* AT+CRSM= */
        {
            /* AT+CRSM=<command>[,<fileid>[,<P1>,<P2>,<P3>[,<data>[,<pathid>]]]] */
            ret = CMS_RET_SUCC;

            if((atGetNumValue(pParamList, 0, &command, ATC_CRSM_0_CMD_VAL_MIN, ATC_CRSM_0_CMD_VAL_MAX, 0) != AT_PARA_OK))
            {
                ret = CMS_FAIL;
            }

            if ((ret == CMS_RET_SUCC) &&
                (atGetNumValue(pParamList, 1, &fileId, ATC_CRSM_1_FILEID_VAL_MIN, ATC_CRSM_1_FILEID_VAL_MAX, ATC_CRSM_1_FILEID_VAL_DEFAULT) == AT_PARA_ERR))
            {
                ret = CMS_FAIL;
            }

            if ((ret == CMS_RET_SUCC) &&
                (atGetNumValue( pParamList, 2, &p1, ATC_CRSM_2_P1_VAL_MIN, ATC_CRSM_2_P1_VAL_MAX, ATC_CRSM_2_P1_VAL_DEFAULT) == AT_PARA_ERR))
            {
                ret = CMS_FAIL;
            }

            if ((ret == CMS_RET_SUCC) &&
                (atGetNumValue( pParamList, 3, &p2, ATC_CRSM_3_P2_VAL_MIN, ATC_CRSM_3_P2_VAL_MAX, ATC_CRSM_3_P2_VAL_DEFAULT) == AT_PARA_ERR))
            {
                ret = CMS_FAIL;
            }

            if ((ret == CMS_RET_SUCC) &&
                (atGetNumValue( pParamList, 4, &p3, ATC_CRSM_4_P3_VAL_MIN, ATC_CRSM_4_P3_VAL_MAX, ATC_CRSM_4_P3_VAL_DEFAULT) == AT_PARA_ERR))
            {
                ret = CMS_FAIL;
            }

            if ((ret == CMS_RET_SUCC) && (pAtCmdReq->paramRealNum > 5))
            {
                data = (UINT8 *)OsaAllocZeroMemory(ATC_CRSM_5_DATA_STR_MAX_LEN + 1);/*+1 for "\0"*/
                OsaCheck(data != PNULL, 0, 0, 0);
                if (atGetStrValue(pParamList, 5, data, ATC_CRSM_5_DATA_STR_MAX_LEN + 1, &dataLen, ATC_CRSM_5_DATA_STR_DEFAULT) == AT_PARA_ERR)
                {
                    ret = CMS_FAIL;
                }
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, simCRSM_0, P_INFO, 1, "<data> legnth:%d", dataLen);
                if ((ret == CMS_RET_SUCC) &&
                    (atGetStrValue(pParamList, 6, pathId, ATC_CRSM_6_PATH_STR_MAX_LEN + 1, &pathLen, ATC_CRSM_6_PATH_STR_DEFAULT) == AT_PARA_ERR))
                {
                    ret = CMS_FAIL;
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = simSetCRSM(atHandle, command, fileId, p1, p2, p3, (char *)data, (char *)pathId);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            if (data != PNULL)
            {
                OsaFreeMemory(&data);
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
  \fn          CmsRetId simCSIM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCSIM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 cmdLen = 0;
    UINT16 strLen = 0;
    UINT8 *cmdStr = PNULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CSIM=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:         /* AT+CSIM= */
        {
            if(atGetNumValue(pParamList, 0, &cmdLen, ATC_CSIM_0_VAL_MIN, ATC_CSIM_0_VAL_MAX, ATC_CSIM_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if((cmdLen % 2) == 0)
                {
                    cmdStr = (UINT8 *)OsaAllocZeroMemory(ATC_CSIM_1_CMD_STR_MAX_LEN + 1);/*+1 for "\0"*/
                    OsaCheck(cmdStr != PNULL, 0, 0, 0);
                    if(atGetStrValue(pParamList, 1, cmdStr, ATC_CSIM_1_CMD_STR_MAX_LEN + 1, &strLen, ATC_CSIM_1_CMD_STR_DEFAULT) != AT_PARA_ERR)
                    {
                        if(strLen == cmdLen)
                        {
                            ret = simSetCSIM(atHandle, strLen, cmdStr);
                        }
                    }
                    if (cmdStr != PNULL)
                    {
                        OsaFreeMemory(&cmdStr);
                    }
                }
            }

            if(ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:         /* AT+CSIM, AT+CSIM? */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId simCIMI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCIMI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CIMI=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CIMI */
        {
            ret = simGetImsi(atHandle);
            break;
        }

        default:         /* AT+CIMI?, AT+CIMI= */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId simCPIN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCPIN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:               /* AT+CPIN=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_READ_REQ:          /* AT+CPIN? */
        {
            ret = simGetPinState(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+CPIN= */
        {
            UINT8 pinStr[ATC_CPIN_0_PIN_STR_MAX_LEN + 1] = {0};    /*+ 1 for '\0'*/
            UINT8 newPinStr[ATC_CPIN_1_NEWPIN_STR_MAX_LEN + 1] = {0}; /*+ 1 for '\0'*/
            UINT16 pinLen,  newPinLen;
            /* retrieve the first password */
            if(atGetStrValue(pParamList, 0, &pinStr[0], ATC_CPIN_0_PIN_STR_MAX_LEN + 1, &pinLen, ATC_CPIN_0_PIN_STR_DEFAULT) != AT_PARA_ERR)
            {
                /* Check entered string.... */
                if ((pinLen >= ATC_CPIN_0_PIN_STR_MIN_LEN) && (atBeNumericString(pinStr) == TRUE))
                {
                    /* Extract the new password. */
                    if(atGetStrValue(pParamList, 1, &newPinStr[0], ATC_CPIN_1_NEWPIN_STR_MAX_LEN + 1, &newPinLen, ATC_CPIN_1_NEWPIN_STR_DEFAULT) != AT_PARA_ERR)
                    {
                        if(newPinLen == 0)
                        {
                            //no second parameter <newpin>
                            ret = simEnterPin(atHandle, pinStr, NULL);
                        }
                        else if((newPinLen >= ATC_CPIN_1_NEWPIN_STR_MIN_LEN) &&
                                (atBeNumericString(newPinStr) == TRUE) &&
                                (pinLen == ATC_CPIN_0_PIN_STR_MAX_LEN))
                        {
                            //<newpin> used, means <pin> is PUK and pinlen should be 8
                            ret = simEnterPin(atHandle, pinStr, newPinStr);
                        }
                    }
                }
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simCPWD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCPWD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 facStr[ATC_CPWD_0_FAC_STR_MAX_LEN + 1] = {0};/*+'\0'*/
    UINT16 facStrLen = 0;
    UINT8 oldPinStr[ATC_CPWD_1_OLDPIN_STR_MAX_LEN + 1] = {0}; /*+'\0'*/
    UINT16 oldPinStrLen = 0;
    UINT8 newPinStr[ATC_CPWD_2_NEWPIN_STR_MAX_LEN + 1] = {0}; /*+'\0'*/
    UINT16 newPinStrLen = 0;

    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+CPWD=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CPWD: (\"SC\",8)");
            break;
        }

        case AT_SET_REQ:             /* AT+CPWD=  */
        {
            if(atGetStrValue(pParamList, 0, facStr, ATC_CPWD_0_FAC_STR_MAX_LEN + 1, &facStrLen, ATC_CPWD_0_FAC_STR_DEFAULT) != AT_PARA_ERR)
            {
                if (strcasecmp((const CHAR*)facStr, "SC") == 0)
                {
                    if(atGetStrValue(pParamList, 1, oldPinStr, ATC_CPWD_1_OLDPIN_STR_MAX_LEN + 1, &oldPinStrLen, ATC_CPWD_1_OLDPIN_STR_DEFAULT) != AT_PARA_ERR)
                    {
                        if ((oldPinStrLen >= ATC_CPWD_1_OLDPIN_STR_MIN_LEN) && (atBeNumericString(oldPinStr) == TRUE))
                        {
                            if(atGetStrValue(pParamList, 2, newPinStr, ATC_CPWD_2_NEWPIN_STR_MAX_LEN + 1, &newPinStrLen, ATC_CPWD_2_NEWPIN_STR_DEFAULT) != AT_PARA_ERR)
                            {
                                if ((newPinStrLen >= ATC_CPWD_2_NEWPIN_STR_MIN_LEN) && (atBeNumericString(newPinStr) == TRUE))
                                {
                                    ret = simSetCPWD(atHandle, CMI_SIM_PIN_1, CMI_SIM_PIN_CHANGE, oldPinStr, newPinStr);
                                }
                            }
                        }
                    }
                }
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simECICCID(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simECICCID(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECICCID=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+ECICCID */
        //case AT_READ_REQ:            /* AT+ECICCID?*/
        {
            ret = simGetIccid(atHandle);
            break;
        }

        default:         /* AT+ECICCID?, AT+ECICCID= */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId simCLCK(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCLCK(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8 fac[ATC_CLCK_0_FAC_STR_MAX_LEN + 1] = {0};/*+'\0'*/
    UINT16 facLen = 0xff;
    UINT8 facVal=0;
    INT32 mode = 0;
    UINT8 pinCode[ATC_CLCK_2_PIN_STR_MAX_LEN + 1] = {0};/*+'\0'*/
    UINT16 pinCodeLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CLCK=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (char *)"+CLCK: (\"SC\")");
            break;
        }

        case AT_SET_REQ:         /* AT+CLCK= */
        {
            if(atGetStrValue(pParamList, 0, fac, ATC_CLCK_0_FAC_STR_MAX_LEN + 1, &facLen, ATC_CLCK_0_FAC_STR_DEFAULT) != AT_PARA_ERR)
            {
                if (strcasecmp((const CHAR*)fac, "SC") == 0)
                {
                    facVal = CMI_SIM_FACILITY_SIM;
                    if(atGetNumValue(pParamList, 1, &mode, ATC_CLCK_1_MODE_VAL_MIN, ATC_CLCK_1_MODE_VAL_MAX, ATC_CLCK_1_MODE_VAL_DEFAULT) != AT_PARA_ERR)
                    {
                        if(atGetStrValue(pParamList, 2, pinCode, ATC_CLCK_2_PIN_STR_MAX_LEN + 1, &pinCodeLen, ATC_CLCK_2_PIN_STR_DEFAULT) != AT_PARA_ERR)
                        {
                            if (mode != 2)
                            {
                                if ((pinCodeLen >= ATC_CLCK_2_PIN_STR_MIN_LEN) && (atBeNumericString(pinCode) == TRUE))
                                {
                                    ret = simSetClck(atHandle, mode, facVal, pinCode);
                                }
                            }
                            else
                            {
                                ret = simSetClck(atHandle, mode, facVal, pinCode);
                            }
                        }
                    }
                }
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simCPINR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCPINR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   paraStr[ATC_CPINR_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 16;
    UINT16  paraLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CPINR=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, PNULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CPINR */
        {
            ret = simSetCpinr(atHandle, CMI_SIM_PIN_ALL);
            break;
        }

        case AT_SET_REQ:         /* AT+CPINR= */
        {
            if (atGetStrValue(pParamList, 0, paraStr,
                               ATC_CPINR_0_MAX_PARM_STR_LEN, &paraLen, ATC_CPINR_0_MAX_PARM_STR_DEFAULT) == AT_PARA_OK)
            {
                if (strcasecmp((const CHAR*)paraStr, "SIM PIN") == 0)
                {
                    ret = simSetCpinr(atHandle, CMI_SIM_PIN_1);
                }
                else if (strcasecmp((const CHAR*)paraStr, "SIM PUK") == 0)
                {
                    ret = simSetCpinr(atHandle, CMI_SIM_PUK_1);
                }

            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simCCHO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCCHO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   dfName[ATC_CCHO_0_STR_MAX_LEN + 1] = {0}; /*+"\0"*/
    UINT16  dfNameLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CCHO=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:         /* AT+CCHO= */
        {
            if(atGetStrValue(pParamList, 0, dfName, ATC_CCHO_0_STR_MAX_LEN+1, &dfNameLen, ATC_CCHO_0_STR_DEFAULT) != AT_PARA_ERR)
            {
                ret = simSetCcho(atHandle, dfName);
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simCCHC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCCHC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 sessionId = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CCHC=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:         /* AT+CCHC= */
        {
            if(atGetNumValue(pParamList, 0, &sessionId, ATC_CCHC_0_VAL_MIN, ATC_CCHC_0_VAL_MAX, ATC_CCHC_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = simSetCchc(atHandle, (UINT8)sessionId);
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simCGLA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCGLA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 sessionId = 0;
    INT32 length = 0;
    UINT8 *command = NULL;
    UINT16 commandLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CGLA=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:         /* AT+CGLA= */
        {
            if(atGetNumValue(pParamList, 0, &sessionId, ATC_CGLA_0_VAL_MIN, ATC_CGLA_0_VAL_MAX, ATC_CGLA_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(atGetNumValue(pParamList, 1, &length, ATC_CGLA_1_VAL_MIN, ATC_CGLA_1_VAL_MAX, ATC_CGLA_1_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    if((length % 2) == 0)
                    {
                        command = (UINT8 *)OsaAllocZeroMemory(ATC_CGLA_2_STR_MAX_LEN + 1);/*+1 for "\0"*/
                        OsaCheck(command != PNULL, 0, 0, 0);
                        if(atGetStrValue(pParamList, 2, command, ATC_CGLA_2_STR_MAX_LEN + 1, &commandLen, ATC_CGLA_2_STR_DEFAULT) != AT_PARA_ERR)
                        {
                            if (length == commandLen)
                            {
                                ret = simSetCgla(atHandle, (UINT8)sessionId, commandLen, command);
                            }
                        }

                        if (command != PNULL)
                        {
                            OsaFreeMemory(&command);
                        }
                    }
                }
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simECSIMSLEEP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simECSIMSLEEP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 mode = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSIMSLEEP=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (char *)"+ECSIMSLEEP: (0,1)");
            break;
        }

        case AT_READ_REQ:          /* AT+ECSIMSLEEP? */
        {
            ret = simGetSimSleepState(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSIMSLEEP= */
        {
            if(atGetNumValue(pParamList, 0, &mode, ATC_ECSIMSLEEP_0_VAL_MIN, ATC_ECSIMSLEEP_0_VAL_MAX, ATC_ECSIMSLEEP_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = simSetSimSleep(atHandle, (UINT8)mode);
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simECSWC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simECSWC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 mode = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSWC=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (char *)"+ECSWC: (0,1,2,3)");
            break;
        }

        case AT_READ_REQ:          /* AT+ECSWC? */
        {
            ret = simGetSWCSetting(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSWC=<mode> */
        {
            if(atGetNumValue(pParamList, 0, &mode, ATC_ECSWC_0_VAL_MIN, ATC_ECSWC_0_VAL_MAX, ATC_ECSWC_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = simSetECSWC(atHandle, (UINT8)mode);
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simECSIMPD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simECSIMPD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 mode = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSIMPD=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (char *)"+ECSIMPD: (0,1)");
            break;
        }

        case AT_READ_REQ:          /* AT+ECSIMPD? */
        {
            ret = simGetECSIMPDSetting(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+ECSIMPD=<mode> */
        {
            if(atGetNumValue(pParamList, 0, &mode, ATC_ECSIMPD_0_VAL_MIN, ATC_ECSIMPD_0_VAL_MAX, ATC_ECSIMPD_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = simSetECSIMPD(atHandle, (UINT8)mode);
            }

            if(ret != CMS_RET_SUCC)
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
  \fn          CmsRetId simCNUM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simCNUM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CNUM=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CNUM */
        {
            ret = simGetSubscriberNumber(atHandle);
            break;
        }

        default:         /* AT+CNUM?, AT+CNUM= */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId simECUSATP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  simECUSATP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 cmdLen = 0;
    UINT16 strLen = 0;
    UINT8 profileStr[ATC_ECUSATP_1_CMD_STR_MAX_LEN + 1] = {0}; /*+"\0"*/

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECUSATP=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_READ_REQ:          /* AT+ECUSATP? */
        {
            ret = simGetTerminalProfile(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+ECUSATP= */
        {
            if(atGetNumValue(pParamList, 0, &cmdLen, ATC_ECUSATP_0_VAL_MIN, ATC_ECUSATP_0_VAL_MAX, ATC_ECUSATP_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if((cmdLen % 2) == 0)
                {
                    if(atGetStrValue(pParamList, 1, profileStr, ATC_ECUSATP_1_CMD_STR_MAX_LEN + 1, &strLen, ATC_ECUSATP_1_CMD_STR_DEFAULT) != AT_PARA_ERR)
                    {
                        if(strLen == cmdLen)
                        {
                            ret = simSetTerminalProfile(atHandle, strLen, profileStr);
                        }
                    }
                }
            }

            if(ret != CMS_RET_SUCC)
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




