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
//#include <ctype.h>
//#include <math.h>
//#include <stdarg.h>
//#include <stdio.h>
//#include <stddef.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atec_mm.h"
//#include "atec_controller.h"
#include "cmimm.h"
#include "ps_mm_if.h"
#include "atec_mm_cnf_ind.h"
#include "osasys.h"

/*Global variable to record signal quality, used in +CESQ */
extern CmiMmCesqInd gCesq;       //0 means not known or not detectable

/*Global variable to record coverage enhancement status, used in +CRCES */
extern CmiMmCesqInd gTemp;       //0 means not known or not detectable  //it's temp since cmimm has no define ind


#ifdef FEATURE_REF_AT_ENABLE
extern BOOL bTestSim;
#endif

#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId mmCREG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCREG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 regParam;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+CREG=? */
        {
            ret = mmQueryRegOption(atHandle);
            //CREG cnf will call atcReply
            break;
        }

        case AT_READ_REQ:              /* AT+CREG? */
        {
            ret = mmGetRegStatus(atHandle);
            //CREG cnf will call atcReply
            break;
        }

        case AT_SET_REQ:              /* AT+CREG= */
        {
            if (atGetNumValue(pParamList, 0, &regParam, ATC_CREG_0_VAL_MIN, ATC_CREG_0_VAL_MAX, ATC_CREG_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CREG_RPT_CFG, regParam);
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
  \fn          CmsRetId mmCSQ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCSQ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, ATEC_SIGNAL_QUALITY_CSQ_SUB_AT, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CSQ=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CSQ: (0-31,99),(0-7,99)");
            break;
        }

        case AT_EXEC_REQ:          /* AT+CSQ */
        {
            ret = mmGetCESQ(atHandle);

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_UNKNOWN, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId mmCOPS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCOPS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   modeVal, formatVal, accTchVal;
    UINT8   networkOperator[ATC_COPS_2_OPER_STR_MAX_LEN + CMS_NULL_CHAR_LEN] = {0};
    UINT16  length = 0;

    switch (operaType)
    {
        /* TS 27.007 - command shall return a list of quadruplets, each representing...
         * ... an operator present in the network. The list of operators shall be in order: ...
         * ... home network, networks referenced in SIM/UICC, and other networks. */
        case AT_TEST_REQ:         /* AT+COPS=? */
        {
            ret = mmQuerySuppOperator(atHandle);
            break;
        }

        case AT_READ_REQ:         /* AT+COPS? */
        {
            ret = mmGetCurrOperator(atHandle);
            break;
        }

        case AT_SET_REQ:          /* AT+COPS=[<mode>[,<format>[,<oper>[,<AcT>]]]] */
        {
            /* Get operator mode. */
            if (atGetNumValue(pParamList, 0, &modeVal,
                              ATC_COPS_0_MODE_VAL_MIN, ATC_COPS_0_MODE_VAL_MAX, ATC_COPS_0_MODE_VAL_DEFAULT) == AT_PARA_OK)
            {
                 switch (modeVal)
                 {
                    case CMI_MM_AUTO_REG_MODE:      //0
                        /*
                         * Other field should not be present
                        */
                        if (pAtCmdReq->paramRealNum > 1)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }

                        ret = mmSetAutoPlmnRegister(atHandle);
                        break;

                    case CMI_MM_MANUAL_REG_ONLY:    //1
                    case CMI_MM_MANUAL_THEN_AUTO_REG:   //4

#ifdef FEATURE_REF_AT_ENABLE
                        if ((CMI_MM_MANUAL_THEN_AUTO_REG == modeVal) && (!bTestSim))
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }
#endif

                        /* format, & oper should be present */
                        if (atGetNumValue(pParamList, 1, &formatVal,
                                          ATC_COPS_1_FORMAT_VAL_MIN, ATC_COPS_1_FORMAT_VAL_MAX, ATC_COPS_1_FORMAT_VAL_DEFAULT) != AT_PARA_OK)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }
                        if (formatVal != CMI_MM_PLMN_NUMERIC)
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_PARSER, mmCOPS_1_0, P_WARNING, 3,
                                        "AT CMD, AT+COPS=%d, not support format: %d", modeVal, formatVal);
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                        }

                        /* get <oper> string, must be present */
                        if (atGetStrValue(pParamList, 2, networkOperator,
                                           ATC_COPS_2_OPER_STR_MAX_LEN, &length, ATC_COPS_2_OPER_STR_DEFAULT) != AT_PARA_OK)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }

                        /* get the act value, could set by default */
                        if (atGetNumValue(pParamList, 3, &accTchVal,
                                          ATC_COPS_3_ACT_VAL_MIN, ATC_COPS_3_ACT_VAL_MAX, ATC_COPS_3_ACT_VAL_DEFAULT) == AT_PARA_ERR)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }

                        /* call MM API*/
                        ret = mmSetManualPlmnRegister(atHandle,
                                                      modeVal == CMI_MM_MANUAL_REG_ONLY ? FALSE : TRUE,
                                                      formatVal,
                                                      networkOperator,
                                                      accTchVal);

                        break;

                    case CMI_MM_DEREG_MODE: //2
                        /*
                         * AT+COPS=2
                         * Other field should not be present
                        */
                        if (pAtCmdReq->paramRealNum > 1)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }

                        ret = mmSetDeregister(atHandle);
                        break;

                    case CMI_MM_SET_FORMAT_MODE:    //3
#ifdef FEATURE_REF_AT_ENABLE
                        if (!bTestSim)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }
#endif
                        /*
                         * AT+COPS=3,(0-2)
                        */
                        if (pAtCmdReq->paramRealNum != 2)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }
                        if (atGetNumValue(pParamList, 1, &formatVal,
                                          ATC_COPS_1_FORMAT_VAL_MIN, ATC_COPS_1_FORMAT_VAL_MAX, ATC_COPS_1_FORMAT_VAL_DEFAULT) != AT_PARA_OK)
                        {
                            return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        }
                        ret = mmSetOperFormat(atHandle, formatVal);
                        break;
                    default:
                        return atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                        //break;
                 }
            }

            if (ret != CMS_RET_SUCC)
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
  \fn          CmsRetId mmCESQ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCESQ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, ATEC_SIGNAL_QUALITY_CESQ_SUB_AT, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;


    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CESQ=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CESQ: (99),(99),(255),(255),(0-34,255),(0-97,255)");
            break;
        }

        case AT_EXEC_REQ:          /* AT+CESQ */
        {
            ret = mmGetCESQ(atHandle);
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
  \fn          CmsRetId mmCPSMS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCPSMS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   modeVal = 0;
    UINT8   prau[ATC_CPSMS_1_PRAU_STR_MAX_LEN+1] = {0};     /*+"\0"*/
    UINT8   gprs[ATC_CPSMS_2_GPRS_STR_MAX_LEN+1] = {0};     /*+"\0"*/
    UINT8   ptau[ATC_CPSMS_3_PTAU_STR_MAX_LEN+1] = {0};     /*+"\0"*/
    UINT8   actTm[ATC_CPSMS_4_ACT_STR_MAX_LEN+1] = {0};     /*+"\0"*/
    UINT16  length = 0;

    switch (operaType)
    {
        /* TS 27.007 - command shall return a list of quadruplets, each representing...
         * ... an operator present in the network. The list of operators shall be in order: ...
         * ... home network, networks referenced in SIM/UICC, and other networks. */
        case AT_TEST_REQ:         /* AT+CPSMS=? */
        {
#ifdef  FEATURE_REF_AT_ENABLE
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CPSMS: (0,1,2),,,(00000000-11111111),(00000000-11111111)");
#else
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CPSMS: (0-2),,,(\"00000000\"-\"11111111\"),(\"00000000\"-\"11111111\")");
#endif
            break;
        }

        case AT_READ_REQ:         /* AT+CPSMS? */
        {
            ret = mmGetCpsms(atHandle);
            break;
        }

        /* TS 27.007 - Set command forces an attempt to select and register the GSM/UMTS network operator.
         * The selected mode affects to all further network registration (e.g. after <mode>=2, MT shall be
         * unregistered until <mode>=0 or 1 is selected). */
        case AT_SET_REQ:
        {
            ret = CMS_RET_SUCC;

            /*+CPSMS=[<mode>[,<Requested_Periodic-RAU>[,<Requested_GPRSREADYtimer>[,<Requested_PeriodicTAU>[,<Requested_ActiveTime>]]]]]*/
            if (atGetNumValue(pParamList, 0, &modeVal,
                              ATC_CPSMS_0_MODE_VAL_MIN, ATC_CPSMS_0_MODE_VAL_MAX, ATC_CPSMS_0_MODE_VAL_DEFAULT) != AT_PARA_OK)
            {
                ret = CMS_FAIL;
            }

            /* get PeriodicRau. should be default */
            if (ret == CMS_RET_SUCC &&
                atGetStrValue(pParamList, 1, prau,
                               ATC_CPSMS_1_PRAU_STR_MAX_LEN+1, &length, ATC_CPSMS_1_PRAU_STR_DEFAULT) != AT_PARA_DEFAULT)
            {
                ret = CMS_FAIL;
            }

            /* Get Gprs ready timer. should be default */
            if (ret == CMS_RET_SUCC &&
                atGetStrValue(pParamList, 2, gprs,
                               ATC_CPSMS_2_GPRS_STR_MAX_LEN+1, &length, ATC_CPSMS_2_GPRS_STR_DEFAULT) != AT_PARA_DEFAULT)
            {
                ret = CMS_FAIL;
            }

            /* Requested_PeriodicTAU */
            if (ret == CMS_RET_SUCC)
            {
                if (atGetStrValue(pParamList, 3, ptau,
                                   ATC_CPSMS_3_PTAU_STR_MAX_LEN+1, &length, ATC_CPSMS_3_PTAU_STR_DEFAULT) != AT_PARA_ERR)
                {
                    if (ptau[0] != 0 && strlen((CHAR *)ptau) != ATC_CPSMS_3_PTAU_STR_MAX_LEN)
                    {
                        ret = CMS_FAIL;
                    }
                    else if (FALSE == atCheckBitFormat(ptau))
                    {
                        ret = CMS_FAIL;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }

            /* Requested_ActiveTime */
            if (ret == CMS_RET_SUCC)
            {
                if (atGetStrValue(pParamList, 4, actTm,
                                   ATC_CPSMS_4_ACT_STR_MAX_LEN+1, &length, ATC_CPSMS_4_ACT_STR_DEFAULT) != AT_PARA_ERR)
                {
                    if (actTm[0] != 0 && strlen((CHAR *)actTm) != ATC_CPSMS_4_ACT_STR_MAX_LEN)
                    {
                        ret = CMS_FAIL;
                    }
                    else if (FALSE == atCheckBitFormat(actTm))
                    {
                        ret = CMS_FAIL;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = mmSetCpsms(atHandle, modeVal, prau, gprs, ptau, actTm);
            }

            if (ret != CMS_RET_SUCC)
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
  \fn          CmsRetId mmCEDRXS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCEDRXS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   modeVal;
    INT32   actType;
    UINT8   edrxVal[ATC_CEDRXS_2_STR_MAX_LEN + 1] = { 0 };  /*+"\0"*/
    UINT16  length;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CEDRXS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CEDRXS: (0,1,2,3),(5),(\"0000\"-\"1111\")");
            break;
        }

        case AT_READ_REQ:         /* AT+CEDRXS? */
        {
            ret = mmGetEdrx(atHandle);
            //CEDRXS cnf will call atcReply
            break;
        }

        case AT_SET_REQ:          /* AT+CEDRXS= [<mode>[,<AcT-type>[,<Requested_eDRX_value>]]] */
        {
            /* Get mode. */
            if(atGetNumValue(pParamList, 0, &modeVal, ATC_CEDRXS_0_MODE_VAL_MIN, ATC_CEDRXS_0_MODE_VAL_MAX, ATC_CEDRXS_0_MODE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                /* get actType. */
                if (atGetNumValue(pParamList, 1, &actType, ATC_CEDRXS_1_VAL_MIN, ATC_CEDRXS_1_VAL_MAX, ATC_CEDRXS_1_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    /* Get edrxVal. */
                    if(atGetStrValue(pParamList, 2, edrxVal, ATC_CEDRXS_2_STR_MAX_LEN + 1, &length, ATC_CEDRXS_2_STR_DEFAULT) != AT_PARA_ERR)
                    {
                        if (TRUE == atCheckBitFormat(edrxVal))
                        {
                            if(length > 0)
                            {
                                ret = mmSetEdrx(atHandle, (UINT8)modeVal, (UINT8)actType, edrxVal);
                            }
                            else
                            {
                                ret = mmSetEdrx(atHandle, (UINT8)modeVal, (UINT8)actType, NULL);
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
  \fn          CmsRetId mmCEDRXRDP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCEDRXRDP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CEDRXRDP=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:          /* AT+CEDRXRDP */
        {
            ret = mmReadEdrxDyn(atHandle);
            //CEDRXRDP cnf will call atcReply
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
  \fn          CmsRetId mmCCIOTOPT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCCIOTOPT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    BOOL    validAT = TRUE;
    INT32   modeVal = 0, typeVal = 0, preOptVal = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CCIOTOPT=? */
        {
            ret = mmGetCiotoptCapa(atHandle);
            //CCIOTOPT cnf will call atcReply
            break;
        }

        case AT_READ_REQ:         /* AT+CCIOTOPT? */
        {
            ret = mmGetCiotoptCfg(atHandle);
            //CCIOTOPT cnf will call atcReply
            break;
        }

        case AT_SET_REQ:          /* AT+CCIOTOPT= [<mode>[,<type>[,<preferred_ue_opt>]]] */
        {
            CmiMmSetCiotOptCfgReq ciotOptCfgReq;

            /* mode */
            if (pParamList[0].bDefault)
            {
                ciotOptCfgReq.reportMode = CMI_MM_CIOT_OPT_RPT_MODE_NOT_PRESENT;
            }
            else if (atGetNumValue(pParamList, 0, &modeVal, ATC_CCIOTOPT_0_VAL_MIN, ATC_CCIOTOPT_0_VAL_MAX, ATC_CCIOTOPT_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if (modeVal == CMI_MM_CIOT_OPT_DISABLE_REPORT ||
                    modeVal == CMI_MM_CIOT_OPT_ENABLE_REPORT ||
                    modeVal == CMI_MM_CIOT_OPT_DISABLE_REPORT_RESET_CFG)
                {
                    ciotOptCfgReq.reportMode = modeVal;
                }
                else
                {
                    validAT = FALSE;
                }
            }
            else
            {
                validAT = FALSE;
            }

            /* type */
            if (validAT)
            {
                if (pParamList[1].bDefault)
                {
                    ciotOptCfgReq.ueSuptOptPresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 1, &typeVal, ATC_CCIOTOPT_1_VAL_MIN, ATC_CCIOTOPT_1_VAL_MAX, ATC_CCIOTOPT_1_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    if(typeVal == 2)
                    {
                        validAT = FALSE;
                    }
                    else
                    {
                        ciotOptCfgReq.ueSuptOptPresent = TRUE;
                        ciotOptCfgReq.ueSuptOptType    = typeVal;
                    }
                }
                else
                {
                    validAT = FALSE;
                }
            }

            /* preferred_ue_opt */
            if (validAT)
            {
                if (pParamList[2].bDefault)
                {
                    ciotOptCfgReq.uePreferOptPresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 2, &preOptVal, ATC_CCIOTOPT_2_VAL_MIN, ATC_CCIOTOPT_2_VAL_MAX, ATC_CCIOTOPT_2_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    ciotOptCfgReq.uePreferOptPresent = TRUE;
                    ciotOptCfgReq.uePreferOpt        = preOptVal;
                }
                else
                {
                    validAT = FALSE;
                }
            }

            if (validAT)
            {
                ret = mmSetCiotoptCfg(atHandle, &ciotOptCfgReq);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, mmCCIOTOPT_1, P_WARNING, 0, "ATCMD, invalid ATCMD: AT+CCIOTOPT");
            }

            if (ret != CMS_RET_SUCC)
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
  \fn          CmsRetId mmCRCES(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCRCES(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CRCES=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, PNULL);
            break;
        }

        case AT_EXEC_REQ:          /* AT+CRCES */
        {
            ret = mmGetCoverEnhancStatu(atHandle);
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
  \fn          CmsRetId  mmCCLK(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCCLK(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR atRspBuf[32] = {0};
    CHAR utcTime[ATC_CCLK_0_STR_MAX_LEN] = {0};
    UINT16  length = 0;
    OsaUtcTimeTValue *utcTimePtr = NULL;
    UINT32 Timer1 = 0;
    UINT32 Timer2 = 0;
    CHAR *timePtr = NULL;
    CHAR *timeTempPtr = NULL;
    UINT32 len = 0;
    UINT32 yy = 0;
    UINT32 MM = 0;
    UINT32 dd = 0;
    UINT32 hh = 0;
    UINT32 mm = 0;
    UINT32 ss = 0;
    INT32 zz = 0;
    INT32 zzFlag = 0xff;
    UINT32 i = 0;
    #ifdef FEATURE_REF_AT_ENABLE
    UINT8  atChanId = AT_GET_HANDLER_CHAN_ID(atHandle);
    UINT32 NITZ_Flag = 0;
    #endif
    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+CCLK=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_READ_REQ:             /* AT+CCLK?  */
        {
            utcTimePtr = OsaSystemTimeReadUtc();
            if(utcTimePtr != NULL)
            {
                if((utcTimePtr->timeZone) >= 0)
                {
                    sprintf(atRspBuf, "+CCLK: \"%04d/%02d/%02d,%02d:%02d:%02d+%02d\"", utcTimePtr->UTCtimer1>>16&0xffff, (utcTimePtr->UTCtimer1>>8)&0xff, utcTimePtr->UTCtimer1&0xff,
                            (utcTimePtr->UTCtimer2>>24)&0xff, (utcTimePtr->UTCtimer2>>16)&0xff, (utcTimePtr->UTCtimer2>>8)&0xff, (utcTimePtr->timeZone));
                }
                else
                {
                    sprintf(atRspBuf, "+CCLK: \"%04d/%02d/%02d,%02d:%02d:%02d%02d\"", utcTimePtr->UTCtimer1>>16&0xffff, (utcTimePtr->UTCtimer1>>8)&0xff, utcTimePtr->UTCtimer1&0xff,
                            (utcTimePtr->UTCtimer2>>24)&0xff, (utcTimePtr->UTCtimer2>>16)&0xff, (utcTimePtr->UTCtimer2>>8)&0xff, (utcTimePtr->timeZone));
                }
                ret = atcReply(atHandle, AT_RC_OK, 0, atRspBuf);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }

            break;
        }

        case AT_SET_REQ:             /* AT+CCLK=  */
        {
            #ifdef FEATURE_REF_AT_ENABLE
            NITZ_Flag = mwAonGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG);
            if(NITZ_Flag == 1)  /* add for ref nitz cmd*/
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                break;
            }
            #endif
            if(atGetStrValue(pParamList, 0, (UINT8 *)utcTime, ATC_CCLK_0_STR_MAX_LEN, &length, ATC_CCLK_0_STR_DEFAULT) != AT_PARA_ERR)
            {
                if(length != 0)
                {
                    timePtr = utcTime;
                    timeTempPtr = strchr(timePtr, '/');
                    len = timeTempPtr - timePtr;
                    if(len > 4)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            yy = (10 * yy) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }
                    if((yy < 2000)||(yy > 2099))  /*smaller than 2000 year, or larger than 2099 year*/
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }
                    timePtr = timeTempPtr + 1;
                    timeTempPtr = strchr(timePtr, '/');
                    len = timeTempPtr - timePtr;
                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            MM = (10 * MM) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }
                    if(MM == 0 || MM > 12)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                    timePtr = timeTempPtr + 1;
                    timeTempPtr = strchr(timePtr, ',');
                    len = timeTempPtr - timePtr;
                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            dd = (10 * dd) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }

                    /*case :20XX/2/30 is invalid*/
                    if((MM == 2 && dd >= 30) || dd == 0 || dd > 31)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }
                    /*case :2011/2/29 is invalid*/
                    if((yy%4 != 0) && (MM == 2) && (dd >= 29))
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                    timePtr = timeTempPtr + 1;
                    timeTempPtr = strchr(timePtr, ':');
                    len = timeTempPtr - timePtr;
                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            hh = (10 * hh) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }
                    if(hh > 23)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                    timePtr = timeTempPtr + 1;
                    if(*timePtr == ' ')
                    {
                        timePtr++;
                    }
                    timeTempPtr = strchr(timePtr, ':');
                    len = timeTempPtr - timePtr;
                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            mm = (10 * mm) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }
                    if(mm > 59)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                    timePtr = timeTempPtr + 1;
                    if(strchr(timePtr, '+') != PNULL)
                    {
                        timeTempPtr = strchr(timePtr, '+');
                        zzFlag = 1;
                    }
                    else if(strchr(timePtr, '-') != PNULL)
                    {
                        timeTempPtr = strchr(timePtr, '-');
                        zzFlag = 0;
                    }
                    else
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                    }

                    len = timeTempPtr - timePtr;

                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            ss = (10 * ss) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }
                    if(ss > 59)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                    timePtr = timeTempPtr + 1;
                    len = strlen(utcTime) - (timePtr - utcTime);

                    for(i=0; i<len; i++)
                    {
                        if ((timePtr[i] >= '0') && (timePtr[i] <= '9'))
                        {
                            zz = (10 * zz) + (timePtr[i] - '0');
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            return ret;
                        }
                    }
                    if(zz > 96)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                        return ret;
                    }

                }
                Timer1 = ((yy<<16)&0xfff0000) | ((MM<<8)&0xff00) | ((dd)&0xff);
                Timer2 = ((hh<<24)&0xff000000) | ((mm<<16)&0xff0000) | ((ss<<8)&0xff00);
                if(zzFlag == 0)
                {
                    zz = 0 - zz;
                }
                ret = OsaTimerSync(0, SET_LOCAL_TIME, Timer1, Timer2, zz);
            }
            if(ret == CMS_RET_SUCC)
            {
                //OsaTimerSync has set the flag
                //if(mwGetTimeSyncFlag()==FALSE)
                //{
                //    mwSetTimeSyncFlag(TRUE);
                //}
                ret = atcReply(atHandle, AT_RC_OK, 0, atRspBuf);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
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
  \fn          CmsRetId mmCTZR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCTZR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8       atChanId    = AT_GET_HANDLER_CHAN_ID(atHandle);
    INT32       modeVal = 0xff;
    CHAR        respStr[ATEC_IND_RESP_128_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+CTZR=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CTZR: (0,1,2,3)");
            break;
        }

        case AT_READ_REQ:             /* AT+CTZR?  */
        {
            snprintf(respStr,
                     ATEC_IND_RESP_128_STR_LEN,
                     "+CTZR: %d",
                     mwGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG));

            ret = atcReply(atHandle, AT_RC_OK, 0, respStr);
            break;
        }

        case AT_SET_REQ:             /* AT+CTZR=  */
        {
            if (atGetNumValue(pParamList, 0, &modeVal, ATC_CTZR_0_VAL_MIN, ATC_CTZR_0_VAL_MAX, ATC_CTZR_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (modeVal == ATC_CTZR_DISABLE_RPT ||
                    modeVal == ATC_CTZR_RPT_TZ ||
                    modeVal == ATC_CTZR_RPT_LOCAL_TIME ||
                    modeVal == ATC_CTZR_RPT_UTC_TIME)
                {
                    mwSetAndSaveAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG, modeVal);
                    ret = CMS_RET_SUCC;
                }
            }

            if (ret == CMS_RET_SUCC)
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
  \fn          CmsRetId mmCTZU(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmCTZU(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8       atChanId    = AT_GET_HANDLER_CHAN_ID(atHandle);
    INT32       modeVal = 0xff;
    CHAR        respStr[ATEC_IND_RESP_128_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+CTZU=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CTZU: (0-1)");
            break;
        }

        case AT_READ_REQ:           /* AT+CTZU?  */
        {
            snprintf(respStr,
                     ATEC_IND_RESP_128_STR_LEN,
                     "+CTZU: %d",
                     mwGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_TIME_ZONE_UPD_CFG));

            ret = atcReply(atHandle, AT_RC_OK, 0, respStr);
            break;
        }

        case AT_SET_REQ:             /* AT+CTZU=  */
        {
            if (atGetNumValue(pParamList, 0, &modeVal, ATC_CTZU_0_VAL_MIN, ATC_CTZU_0_VAL_MAX, ATC_CTZU_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (modeVal >= ATC_CTZU_DISABLE_UPDATE_TZ_VIA_NITZ &&
                    modeVal <= ATC_CTZU_ENABLE_UPDATE_TZ_VIA_NITZ)
                {
                    mwSetAndSaveAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_TIME_ZONE_UPD_CFG, modeVal);
                    ret = CMS_RET_SUCC;
                }
            }

            if (ret == CMS_RET_SUCC)
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
  \fn          CmsRetId  mmECPLMNS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmECPLMNS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+ECPLMNS=? */
        {
            /*OK*/
            ret = atcReply(atHandle, AT_RC_OK, 0, PNULL);
            break;
        }


        case AT_READ_REQ:             /* AT+ECPLMNS?  */
        {
            ret = mmGetEcplmns(atHandle);
            break;
        }

        case AT_EXEC_REQ:             /* AT+ECPLMNS  */
        {
            ret = mmExecEcplmns(atHandle);
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
  \fn          CmsRetId  mmECCESQS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmECCESQS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   rptMode = 0;
    CHAR    respStr[ATEC_IND_RESP_128_STR_LEN] = {0};

    /*
     * AT+ECCESQS=<n>
     * <n>, report level
     * 0: disable URC (unsolicited result code)
     * 1: report, +CESQ: <rxlev>,<ber>,<rscp>,<ecno>,<rsrq>,<rsrp>, as 27.007
     * 2: report, +ECCESQ: RSRP,<rsrp>,RSRQ,<rsrq>,SNR,<snr>
    */
    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+ECCESQS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECCESQS: (0-2)");
            break;
        }

        case AT_READ_REQ:             /* AT+ECCESQ?  */
        {
            snprintf(respStr, ATEC_IND_RESP_128_STR_LEN, "+ECCESQS: %d",
                     mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG));

            ret = atcReply(atHandle, AT_RC_OK, 0, respStr);
            break;
        }

        case AT_SET_REQ:             /* AT+ECCESQS=  */
        {
            if (atGetNumValue(pParamList, 0, &rptMode,
                              ATC_ECCESQ_0_VAL_MIN, ATC_ECCESQ_0_VAL_MAX, ATC_ECCESQ_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (rptMode >= ATC_ECCESQ_DISABLE_RPT && rptMode <= ATC_ECCESQ_RPT_ECCESQ)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG, rptMode);
                    ret = CMS_RET_SUCC;
                }
            }

            if (ret == CMS_RET_SUCC)
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



#ifndef FEATURE_REF_AT_ENABLE

/**
  \fn          CmsRetId mmECPSMR(const AtCmdInputContext *pAtCmdReq)
  \brief       wakeup indication
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmECPSMR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR    respStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    INT32 rptMode;


    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECPSMR=? */
        {
            sprintf((char*)respStr, "+ECPSMR: (0,1)\r\n");

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)respStr);
            break;
        }
        case AT_READ_REQ:             /* AT+ECPSMR?  */
        {
            ret = mmGetECPSMR( atHandle);
            break;

        }

        case AT_SET_REQ:             /* AT+ECPSMR=  */
        {
            if (atGetNumValue(pParamList, 0, &rptMode,
                              ATC_ECPSMR_0_VAL_MIN, ATC_ECPSMR_0_VAL_MAX, ATC_ECPSMR_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (rptMode >= ATC_ECPSMR_DISABLE && rptMode <= ATC_ECPSMR_ENABLE)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PSM_RPT_CFG, rptMode);
                    ret = CMS_RET_SUCC;
                }
            }

            if (ret == CMS_RET_SUCC)
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
#endif

/**
  \fn          CmsRetId mmECPTWEDRXS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmECPTWEDRXS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 modeVal;
    INT32 actType;
    UINT8 ptw[ATC_ECPTWEDRXS_2_STR_MAX_LEN + 1] = { 0 };  /*+"\0"*/
    UINT8 edrxVal[ATC_ECPTWEDRXS_3_STR_MAX_LEN + 1] = { 0 };  /*+"\0"*/
    UINT16 length;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECPTWEDRXS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+ECPTWEDRXS: (0,1,2,3),(5),(\"0000\"-\"1111\"),(\"0000\"-\"1111\")");
            break;
        }

        case AT_READ_REQ:         /* AT+ECPTWEDRXS? */
        {
            ret = mmGetPtwEdrx(atHandle);
            //ECPTWEDRXS cnf will call atcReply
            break;
        }

        case AT_SET_REQ:          /* AT+ECPTWEDRXS=[<mode>[,<AcT-type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>]]]] */
        {
            ret = CMS_RET_SUCC;

            /* Get mode. */
            if (atGetNumValue(pParamList, 0, &modeVal,
                              ATC_ECPTWEDRXS_0_MODE_VAL_MIN, ATC_ECPTWEDRXS_0_MODE_VAL_MAX, ATC_ECPTWEDRXS_0_MODE_VAL_DEFAULT) != AT_PARA_OK)
            {
                ret = CMS_FAIL;
            }

            /* Get actType. */
            if (atGetNumValue(pParamList, 1, &actType,
                              ATC_ECPTWEDRXS_1_VAL_MIN, ATC_ECPTWEDRXS_1_VAL_MAX, ATC_ECPTWEDRXS_1_VAL_DEFAULT) != AT_PARA_OK)
            {
                ret = CMS_FAIL;
            }

            /* Get ptw. */
            if (ret == CMS_RET_SUCC)
            {
                if (atGetStrValue(pParamList, 2, ptw,
                                   ATC_ECPTWEDRXS_2_STR_MAX_LEN + 1, &length, ATC_ECPTWEDRXS_2_STR_DEFAULT) != AT_PARA_ERR)
                {
                    if (ptw[0] != 0 && strlen((CHAR *)ptw) != ATC_ECPTWEDRXS_2_STR_MAX_LEN)
                    {
                        ret = CMS_FAIL;
                    }
                    else if (FALSE == atCheckBitFormat(ptw))
                    {
                        ret = CMS_FAIL;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }

            /* Get edrxValue. */
            if (ret == CMS_RET_SUCC)
            {
                if (atGetStrValue(pParamList, 3, edrxVal,
                                   ATC_ECPTWEDRXS_3_STR_MAX_LEN + 1, &length, ATC_ECPTWEDRXS_3_STR_DEFAULT) != AT_PARA_ERR)
                {
                    if (edrxVal[0] != 0 && strlen((CHAR *)edrxVal) != ATC_ECPTWEDRXS_3_STR_MAX_LEN)
                    {
                        ret = CMS_FAIL;
                    }
                    else if (FALSE == atCheckBitFormat(edrxVal))
                    {
                        ret = CMS_FAIL;
                    }
                }
                else
                {
                    ret = CMS_FAIL;
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = mmSetPtwEdrx(atHandle, (UINT8)modeVal, (UINT8)actType, ptw, edrxVal);

                if(2 == modeVal)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, ATC_ECPTWEDRXS_ENABLE);
                }
                else
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, ATC_ECPTWEDRXS_DISABLE);
                }
            }

            if (ret != CMS_RET_SUCC)
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
  \fn          CmsRetId mmECEMMTIME(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mmECEMMTIME(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR    respStr[ATEC_IND_RESP_64_STR_LEN] = {0};
    INT32   rptMode;


    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECEMMTIME=? */
        {
            snprintf((char*)respStr, ATEC_IND_RESP_64_STR_LEN, "+ECEMMTIME: (0,7)\r\n");

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)respStr);
            break;
        }
        case AT_READ_REQ:             /* AT+ECEMMTIME?  */
        {
            ret = mmGetECEMMTIME(atHandle);
            break;

        }

        case AT_SET_REQ:             /* AT+ECEMMTIME=  */
        {
            if (atGetNumValue(pParamList, 0, &rptMode,
                              ATC_ECEMMTIME_0_VAL_MIN, ATC_ECEMMTIME_0_VAL_MAX, ATC_ECEMMTIME_VAL_DEFAULT) == AT_PARA_OK)
            {
                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_EMM_TIME_RPT_CFG, rptMode);
                ret = CMS_RET_SUCC;
            }

            if (ret == CMS_RET_SUCC)
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




