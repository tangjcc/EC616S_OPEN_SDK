/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_sms.c
*
*  Description: Process SMS related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "cmisms.h"
#include "atec_sms.h"
#include "ps_sms_if.h"
#include "atec_sms_cnf_ind.h"
#include "mw_config.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "ecpm_ec616_external.h"
#elif defined CHIP_EC616S
#include "ecpm_ec616s_external.h"
#endif


/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

/******************************************************************************
 * SMS sending info.
 *  Can't send SMS by several AT channel at same time.
******************************************************************************/
static PsilSmsSendInfo *pSmsSendInfo = PNULL;

#define _DEFINE_STATIC_FUNCTION_LIST_

/**
  \fn           BOOL smsGetInputAddrInfo(AtParamValueCP pParamList, INT32 index, CmiSmsAddressInfo *pSmsAddr)
  \brief        decode the "<addr>[,<toaddr>]" info from AT CMD input parameters
  \param[in]    pParamList      AT CMD parameter list
  \param[in]    index           from index
  \param[out]   pSmsAddr        SMS address info
  \returns      BOOL
*/
static BOOL smsGetInputAddrInfo(AtParamValueCP pParamList, INT32 index, CmiSmsAddressInfo *pSmsAddr)
{
    UINT16  strIdx = 0, addrLen = 0;
    INT32   addrTypeOct = 0;
    UINT8   addrStr[ATC_ADDR_STR_MAX_LEN];

    /*
     * +CMGS=<da>[,<toda>]
     *
    */

    OsaDebugBegin(pSmsAddr != PNULL && index < AT_CMD_PARAM_MAX_NUM, pSmsAddr, index, 0);
    return FALSE;
    OsaDebugEnd();

    memset(pSmsAddr, 0x00, sizeof(CmiSmsAddressInfo));

    /* <addr>, is must */
    if ((atGetStrValue(pParamList, index, addrStr, ATC_ADDR_STR_MAX_LEN, &addrLen, ATC_ADDR_STR_DEFAULT) == AT_PARA_OK))
    {
        if (addrLen > 0)
        {
            /*
             * currently AT cmd server expects the address in 'IRA digits string' only
            */

            pSmsAddr->addrType.numPlan = CMI_SMS_NUMPLAN_E164_E163; /*by default*/

            if (addrStr[strIdx] == '+' )
            {
                pSmsAddr->addrType.numType = CMI_SMS_NUMTYPE_INTERNATIONAL; //don't need to read the next "<toaddr>"
                strIdx++;
            }
            else
            {
                pSmsAddr->addrType.numType = CMI_SMS_NUMTYPE_UNKNOWN;

                /* <toaddr> */
                if (atGetNumValue(pParamList, index+1, &addrTypeOct,
                                  ATC_ADDR_TYPE_OCT_MIN, ATC_ADDR_TYPE_OCT_MAX, ATC_ADDR_TYPE_OCT_DEFAULT) != AT_PARA_ERR)
                {
                    switch (addrTypeOct)
                    {
                        case PSIL_SMS_TOA_OCT_NUMBER_INTERNATIONAL:
                            pSmsAddr->addrType.numType = CMI_SMS_NUMTYPE_INTERNATIONAL;;
                            break;
                        case PSIL_SMS_TOA_OCT_NUMBER_NATIONAL:
                            pSmsAddr->addrType.numType = CMI_SMS_NUMTYPE_NATIONAL;
                            break;
                        case PSIL_SMS_TOA_OCT_NUMBER_NET_SPECIFIC:
                            pSmsAddr->addrType.numType = CMI_SMS_NUMTYPE_NETWORK;
                            break;
                        default:
                            pSmsAddr->addrType.numType = CMI_SMS_NUMTYPE_UNKNOWN;
                            break;
                    }
                }
            }

            memcpy(pSmsAddr->digits, addrStr + strIdx, addrLen - strIdx);
            pSmsAddr->len = addrLen - strIdx;

            return TRUE;
        }
    }

    OsaDebugBegin(FALSE, addrLen, index, 0);
    OsaDebugEnd();

    return FALSE;
}


#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId smsCSMS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCSMS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 service;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CSMS=? */
        {
            ret = smsQuerySmsService(atHandle);
            break;
        }

        case AT_READ_REQ:         /* AT+CSMS? */
        {
            ret = smsGetSmsService(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+CSMS= */
        {
            if(atGetNumValue(pParamList, 0, (INT32 *)&service, ATC_CSMS_0_SERVICE_VAL_MIN, ATC_CSMS_0_SERVICE_VAL_MAX, ATC_CSMS_0_SERVICE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = smsSetSmsService(atHandle, service);

                if(ret == CMS_RET_SUCC)
                {
                    //CSMS cnf will call atcReply
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                }
                break;
            }
        }

        default:         /* AT+CSMS */
        {
            atcReply(atHandle, AT_RC_CME_ERROR, CMS_SMS_OPERATION_NOT_ALLOWED, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId smsCSCA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCSCA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   addressStr[ATC_CSCA_0_ADDR_STR_MAX_LEN] = {0};
    UINT16  addrStrLen;
    UINT32  type;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CSCA=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_READ_REQ:         /* AT+CSCA? */
        {
            ret = smsGetSrvCenterAddr(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+CSCA=<sca>[,<tosca>] */
        {
            if (atGetStrValue(pParamList, 0, addressStr, ATC_CSCA_0_ADDR_STR_MAX_LEN,
                              &addrStrLen, ATC_CSCA_0_ADDR_STR_DEFAULT) == AT_PARA_OK)
            {
                if (addrStrLen > 0)
                {
                    if (atGetNumValue(pParamList, 1, (INT32 *)&type,
                                      ATC_CSCA_1_TYPE_VAL_MIN, ATC_CSCA_1_TYPE_VAL_MAX, ATC_CSCA_1_TYPE_VAL_DEFAULT) != AT_PARA_ERR)
                    {
                        ret = smsSetSrvCenterAddr(atHandle, addressStr, addrStrLen, type);
                    }
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            }
            break;
        }

        default:         /* AT+CSCA */
        {
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId smsCMGF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCMGF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    PsilSmsFormatMode gSmsFormatMode = PSIL_SMS_FORMAT_TXT_MODE;
    CHAR    rspStr[ATEC_IND_RESP_32_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:               /* AT+CMGF=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CMGF: (0-1)");
            break;
        }

        case AT_READ_REQ:               /* AT+CMGF?  */
        {
            gSmsFormatMode = (PsilSmsFormatMode)mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG);

            snprintf(rspStr, ATEC_IND_RESP_32_STR_LEN, "+CMGF: %d", gSmsFormatMode);
            ret = atcReply(atHandle, AT_RC_OK, 0, rspStr);
            break;
        }

        case AT_SET_REQ:              /* AT+CMGF=<mode> */
        {
            INT32 mode;

            if (atGetNumValue(pParamList, 0, (INT32 *)&mode,
                              ATC_CMGF_0_MODE_VAL_MIN, ATC_CMGF_0_MODE_VAL_MAX, ATC_CMGF_0_MODE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                gSmsFormatMode = (PsilSmsFormatMode)mode;

                if (gSmsFormatMode == PSIL_SMS_FORMAT_PDU_MODE)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG, PSIL_SMS_FORMAT_PDU_MODE);
                }
                else
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG, PSIL_SMS_FORMAT_TXT_MODE);
                }

                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                /* invalid param */
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            }

            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            break;
        }
    }


    return ret;
}

/**
  \fn           CmsRetId smsCMGSCMGCCancel(void)
  \brief        cancel sending SMS command trigger by AT+CMGC
  \param[in]    void
  \returns      void
*/
void smsCMGSCMGCCancel(void)
{
    UINT8   chanId = 0;

    OsaDebugBegin(pSmsSendInfo != PNULL, pSmsSendInfo, 0, 0);
    return;
    OsaDebugEnd();

    /* AT channel state should change back to COMMAND state */
    chanId = AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander);

    atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);

    /*
     * need send CMS ERROR
    */
    atcReply(pSmsSendInfo->reqHander,
             AT_RC_CMS_ERROR,
             CMS_SMS_UNKNOWN_ERROR,
             PNULL);

    // free the pSmsSendInfo
    OsaFreeMemory(&pSmsSendInfo);

    return;
}


/**
  \fn          CmsRetId smsCMGS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCMGS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP      pParamList = pAtCmdReq->pParamList;
    PsilSmsFormatMode   gSmsFormatMode = PSIL_SMS_FORMAT_PDU_MODE;
    INT32       pduLen = 0;
    SmsErrorResultCode  smsRetCode = CMS_SMS_SUCC;
    CmiSmsAddressInfo   destAddrInfo;

    switch (operaType)
    {
        case AT_TEST_REQ:               /* AT+CMGS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:              /* AT+CMGS= */
        {
            gSmsFormatMode = (PsilSmsFormatMode)mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG);

            /*
             * For PDU mode:
             * AT+CMGS=<length><CR>
             *          PDU is given<ctrl-Z/ESC>
             * For TXT mode
             * AT+CMGS=<da>[,<toda>]<CR>
             *         text is entered<ctrl-Z/ESC>
            */

            if (gSmsFormatMode == PSIL_SMS_FORMAT_PDU_MODE)
            {
                /* <length> */
                if ((pParamList[0].type != AT_DEC_VAL) ||
                    (atGetNumValue(pParamList, 0, &pduLen,
                                   ATC_CMGS_0_PDU_LENGTH_MIN, ATC_CMGS_0_PDU_LENGTH_MAX, ATC_CMGS_0_PDU_LENGTH_DEFAULT) != AT_PARA_OK))
                {
                    OsaDebugBegin(FALSE, pParamList[0].type, pduLen, 0);
                    OsaDebugEnd();

                    smsRetCode = CMS_SMS_INVALID_PDU_MODE_PARAMETER;
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    /* check whether any other SMS is ongoing */
                    if (pSmsSendInfo == PNULL)
                    {
                        pSmsSendInfo = (PsilSmsSendInfo *)OsaAllocZeroMemory(sizeof(PsilSmsSendInfo));

                        OsaDebugBegin(pSmsSendInfo != PNULL, sizeof(PsilSmsSendInfo), 0, 0);
                        smsRetCode = CMS_SMS_MEMORY_FAILURE;
                        OsaDebugEnd();
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, smsCMGS_pdu_warn_1, P_WARNING, 1,
                                    "AT CMD, +CMGS, another SMS is ongoing, chanId: %d", AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander));
                        smsRetCode = CMS_SMS_ME_FAILURE;
                    }
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    pSmsSendInfo->reqHander = (UINT16)atHandle;
                    pSmsSendInfo->reqPduLen = pduLen;
                }
            }
            else
            {
                /*
                 * For Text mode,
                 * +CMGS=<da>[,<toda>]<CR>...
                */
                if (smsGetInputAddrInfo(pParamList, 0, &destAddrInfo) != TRUE)
                {
                    smsRetCode = CMS_SMS_INVALID_TEXT_MODE_PARAMETER;
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    /* check whether any other SMS is ongoing */
                    if (pSmsSendInfo == PNULL)
                    {
                        pSmsSendInfo = (PsilSmsSendInfo *)OsaAllocZeroMemory(sizeof(PsilSmsSendInfo));

                        OsaDebugBegin(pSmsSendInfo != PNULL, sizeof(PsilSmsSendInfo), 0, 0);
                        smsRetCode = CMS_SMS_MEMORY_FAILURE;
                        OsaDebugEnd();
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, smsCMGS_text_warn_1, P_WARNING, 1,
                                    "AT CMD, +CMGS, another SMS is ongoing, chanId: %d", AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander));
                        smsRetCode = CMS_SMS_ME_FAILURE;
                    }
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    pSmsSendInfo->reqHander = (UINT16)atHandle;
                    memcpy(&(pSmsSendInfo->daInfo), &destAddrInfo, sizeof(CmiSmsAddressInfo));
                }
            }

            if (smsRetCode == CMS_SMS_SUCC)
            {
                /*
                 * The TA shall send a four character sequence <CR><LF><greater_than><space> (IRA 13, 10, 62, 32)
                 *  after command line is terminated with <CR>; after that text can be entered from TE to ME/TA.
                 * "\r\n> "
                */
                // change the channel state
                ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SMS_CMGS_CMGC_DATA_STATE);

                if (ret == CMS_RET_SUCC)
                {
                    ret = atcReply(atHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
                }

                if (ret == CMS_RET_SUCC)
                {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
                }
                else
                {
                    OsaDebugBegin(FALSE, ret, 0, 0);
                    OsaDebugEnd();

                    OsaFreeMemory(&pSmsSendInfo);
                    atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, smsRetCode, NULL);
            }

            break;

        }   //End Set

        default:
        {
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_ALLOWED, NULL);
            break;
        }
    }


    return ret;
}

/**
  \fn          CmsRetId smsCMGC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCMGC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP      pParamList = pAtCmdReq->pParamList;
    PsilSmsFormatMode   gSmsFormatMode = PSIL_SMS_FORMAT_PDU_MODE;
    INT32       pduLen = 0;
    SmsErrorResultCode  smsRetCode = CMS_SMS_SUCC;
    CmiSmsAddressInfo   destAddrInfo;

    switch (operaType)
    {
        case AT_TEST_REQ:               /* AT+CMGC=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_SET_REQ:              /* AT+CMGC= */
        {
            gSmsFormatMode = (PsilSmsFormatMode)mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG);

            /*
             * For PDU mode:
             * AT+CMGC=<length><CR>
             *          PDU is given<ctrl-Z/ESC>
             * For TXT mode
             * +CMGC=<da>[,<toda>]<CR>...
             * AT++CMGC=<fo>,<ct>[,<pid>[,<mn>[,<da>[,<toda>]]]]<CR>
            */

            if (gSmsFormatMode == PSIL_SMS_FORMAT_PDU_MODE)
            {
                /* <length> */
                if ((pParamList[0].type != AT_DEC_VAL) ||
                    (atGetNumValue(pParamList, 0, &pduLen,
                                   ATC_CMGS_0_PDU_LENGTH_MIN, ATC_CMGS_0_PDU_LENGTH_MAX, ATC_CMGS_0_PDU_LENGTH_DEFAULT) != AT_PARA_OK))
                {
                    OsaDebugBegin(FALSE, pParamList[0].type, pduLen, 0);
                    OsaDebugEnd();

                    smsRetCode = CMS_SMS_INVALID_PDU_MODE_PARAMETER;
                }


                if (smsRetCode == CMS_SMS_SUCC)
                {
                    /* check whether any other SMS is ongoing */
                    if (pSmsSendInfo == PNULL)
                    {
                        pSmsSendInfo = (PsilSmsSendInfo *)OsaAllocZeroMemory(sizeof(PsilSmsSendInfo));

                        OsaDebugBegin(pSmsSendInfo != PNULL, sizeof(PsilSmsSendInfo), 0, 0);
                        smsRetCode = CMS_SMS_MEMORY_FAILURE;
                        OsaDebugEnd();
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, smsCMGC_pdu_warn_1, P_WARNING, 1,
                                    "AT CMD, +CMGC, another SMS is ongoing, chanId: %d", AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander));
                        smsRetCode = CMS_SMS_ME_FAILURE;
                    }
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    pSmsSendInfo->reqHander = (UINT16)atHandle;
                    pSmsSendInfo->reqPduLen = pduLen;
                    pSmsSendInfo->bCommand = TRUE;
                }
            }
            else
            {
                /*
                 * For Text mode,
                 * AT++CMGC=<fo>,<ct>[,<pid>[,<mn>[,<da>[,<toda>]]]]<CR>
                 * +CMGC=<da>[,<toda>]<CR>...
                */
                if (smsGetInputAddrInfo(pParamList, 0, &destAddrInfo) != TRUE)
                {
                    smsRetCode = CMS_SMS_INVALID_TEXT_MODE_PARAMETER;
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    /* check whether any other SMS is ongoing */
                    if (pSmsSendInfo == PNULL)
                    {
                        pSmsSendInfo = (PsilSmsSendInfo *)OsaAllocZeroMemory(sizeof(PsilSmsSendInfo));

                        OsaDebugBegin(pSmsSendInfo != PNULL, sizeof(PsilSmsSendInfo), 0, 0);
                        smsRetCode = CMS_SMS_MEMORY_FAILURE;
                        OsaDebugEnd();
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, smsCMGC_text_warn_1, P_WARNING, 1,
                                    "AT CMD, +CMGC, another SMS is ongoing, chanId: %d", AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander));
                        smsRetCode = CMS_SMS_ME_FAILURE;
                    }
                }

                if (smsRetCode == CMS_SMS_SUCC)
                {
                    pSmsSendInfo->reqHander = (UINT16)atHandle;
                    memcpy(&(pSmsSendInfo->daInfo), &destAddrInfo, sizeof(CmiSmsAddressInfo));
                    pSmsSendInfo->bCommand = TRUE;
                }
            }

            if (smsRetCode == CMS_SMS_SUCC)
            {
                /*
                 * The TA shall send a four character sequence <CR><LF><greater_than><space> (IRA 13, 10, 62, 32)
                 *  after command line is terminated with <CR>; after that text can be entered from TE to ME/TA.
                 * "\r\n> "
                */
                // change the channel state
                ret = atcChangeChannelState(pAtCmdReq->chanId, ATC_SMS_CMGS_CMGC_DATA_STATE);

                if (ret == CMS_RET_SUCC)
                {
                    ret = atcReply(atHandle, AT_RC_RAW_INFO_CONTINUE, 0, "\r\n> ");
                }

                if (ret == CMS_RET_SUCC)
                {
                    /*
                     * - not suitable, -TBD
                    */
                    //pmuPlatIntVoteToSleep1State(PMU_SLEEP_ATCMD_MOD, FALSE);
                }
                else
                {
                    OsaDebugBegin(FALSE, ret, 0, 0);
                    OsaDebugEnd();

                    OsaFreeMemory(&pSmsSendInfo);
                    atcChangeChannelState(pAtCmdReq->chanId, ATC_ONLINE_CMD_STATE);
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, smsRetCode, NULL);
            }

            break;

        }   //End Set

        default:
        {
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_ALLOWED, NULL);
            break;
        }
    }


    return ret;
}

/**
  \fn          CmsRetId smsCMGSCMGCInputData(UINT8 *pInputData, UINT16 length)
  \brief       when at channel change to: ATC_SMS_CMGS_DATA_STATE, all data sent via this
  \            AT channel should pass to SMS by calling this API
  \param[in]   pInput       input data
  \param[in]   length       input data length
  \returns     CmsRetId
  \            if not return: "CMS_RET_SUCC", AT channel should return to command state
*/
CmsRetId smsCMGSCMGCInputData(UINT8 chanId, UINT8 *pInput, UINT16 length)
{
    CmsRetId cmsRet = CMS_RET_SUCC;
    UINT32   smsFormat = PSIL_SMS_FORMAT_PDU_MODE;

    /* check input */
    OsaCheck(pInput != PNULL && length > 0, pInput, length, 0);

    if (pSmsSendInfo == PNULL)
    {
        OsaDebugBegin(FALSE, pSmsSendInfo, pInput, length);
        OsaDebugEnd();

        return CMS_FAIL;
    }

    // check the channId
    OsaCheck(AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander) == chanId,
             pSmsSendInfo->reqHander, chanId, 0);

    smsFormat = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG);

    /*
     * check the input length
    */
    if (smsFormat == PSIL_SMS_FORMAT_PDU_MODE)
    {
        if (pSmsSendInfo->inputOffset + length > PSIL_SMS_HEX_PDU_STR_MAX_SIZE)
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, smsCMGSInputData_warn_1, P_WARNING, 3,
                        "AT CMD, all input SMS PDU length: %d+%d, extended MAX value: %d",
                        pSmsSendInfo->inputOffset, length, PSIL_SMS_HEX_PDU_STR_MAX_SIZE);

            smsCMGSCMGCCancel();

            return CMS_FAIL;
        }
    }
    else    //TXT format
    {
        if (pSmsSendInfo->inputOffset + length > PSIL_SMS_MAX_TXT_SIZE)
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, smsCMGSInputData_warn_2, P_WARNING, 3,
                        "AT CMD, all input SMS text length: %d+%d, extended MAX value: %d",
                        pSmsSendInfo->inputOffset, length, PSIL_SMS_MAX_TXT_SIZE);

            smsCMGSCMGCCancel();

            return CMS_FAIL;
        }
    }

    /*
     * copy the data into buffer
    */
    memcpy(pSmsSendInfo->input+pSmsSendInfo->inputOffset,
           pInput,
           length);
    pSmsSendInfo->inputOffset += length;

    //check CTRL-Z or ESC
    if (pSmsSendInfo->input[pSmsSendInfo->inputOffset-1] == AT_ASCI_CTRL_Z)   //CTRL-Z IRA26
    {
        cmsRet = smsSendSms(smsFormat, pSmsSendInfo);

        /*done, need change the AT channel state to command state*/
        atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);

        /* if failed, need to send the CMS ERROR */
        if (cmsRet != CMS_RET_SUCC)
        {
            if (cmsRet == CMS_INVALID_PARAM)
            {
                if (smsFormat == PSIL_SMS_FORMAT_TXT_MODE)
                {
                    cmsRet = atcReply(pSmsSendInfo->reqHander,
                                      AT_RC_CMS_ERROR,
                                      CMS_SMS_INVALID_TEXT_MODE_PARAMETER,
                                      PNULL);
                }
                else
                {
                    cmsRet = atcReply(pSmsSendInfo->reqHander,
                                      AT_RC_CMS_ERROR,
                                      CMS_SMS_INVALID_PDU_MODE_PARAMETER,
                                      PNULL);
                }
            }
            else if (cmsRet == CMS_NO_MEM)
            {
                cmsRet = atcReply(pSmsSendInfo->reqHander,
                                  AT_RC_CMS_ERROR,
                                  CMS_SMS_MEM_FULL,
                                  PNULL);
            }
            else
            {
                cmsRet = atcReply(pSmsSendInfo->reqHander,
                                  AT_RC_CMS_ERROR,
                                  CMS_SMS_UNKNOWN_ERROR,
                                  PNULL);
            }
        }
        // free the pSmsSendInfo
        OsaFreeMemory(&pSmsSendInfo);
    }
    else if (pSmsSendInfo->input[pSmsSendInfo->inputOffset-1] == AT_ASCI_ESC)  //CTRL-Z IRA27
    {
        smsCMGSCMGCCancel();
    }

    return cmsRet;
}


/**
  \fn          CmsRetId smsCNMA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCNMA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId cmsRet = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    PsilSmsFormatMode gSmsFormatMode = PSIL_SMS_FORMAT_PDU_MODE;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+CNMA=? */
        {
            cmsRet = atcReply(atHandle, AT_RC_OK, 0, "+CNMA: 0,1");
            break;
        }

        case AT_SET_REQ:              /* AT+CNMA= */
        {
            INT32 reply;

            /* for PDU mode,  AT+CNMA=<n> */
            if(gSmsFormatMode == PSIL_SMS_FORMAT_PDU_MODE)
            {
                if(atGetNumValue(pParamList, 0, &reply, ATC_CNMA_0_REPLY_VAL_MIN, ATC_CNMA_0_REPLY_VAL_MAX, ATC_CNMA_0_REPLY_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    cmsRet = smsNewMsgAck(atHandle, reply);
                }
            }

            break;
        }

        case AT_EXEC_REQ:              /* AT+CNMA */
        {
            gSmsFormatMode = (PsilSmsFormatMode)mwGetAtChanConfigItemValue(CMS_CHAN_DEFAULT, MID_WARE_AT_CHAN_SMS_MODE_CFG);
            /* for text mode,  AT+CNMA */
            if(gSmsFormatMode == PSIL_SMS_FORMAT_TXT_MODE)
            {
                cmsRet = smsNewMsgAck(atHandle, 0);
            }

            break;
        }

        default:
        {
            cmsRet = atcReply(atHandle, AT_RC_CME_ERROR, CMS_SMS_OPERATION_NOT_ALLOWED, NULL);
            break;
        }
    }

    return cmsRet;
}

/**
  \fn          CmsRetId smsCSMP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCSMP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId cmsRet = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    MidWareCSMPParam csmpParam = {0};
    MidWareSetCSMPParam setCsmpParam = {0};
    CHAR    rspBuf[ATEC_IND_RESP_64_STR_LEN] = {0};
    AtParaRet   paraRet = AT_PARA_OK;

    switch (operaType)
    {
        case AT_TEST_REQ:    //AT+CSMP=?
        {
            cmsRet = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_READ_REQ:    //AT+CSMP?
        {
            if (mwGetCsmpConfig(pAtCmdReq->chanId, &csmpParam) != TRUE)
            {
                OsaDebugBegin(FALSE, pAtCmdReq->chanId, 0, 0);
                OsaDebugEnd();
                //use the default value
                csmpParam.fo    = PSIL_MSG_FO_DEFAULT;
                csmpParam.vp    = PSIL_MSG_VP_DEFAULT;
                csmpParam.pid   = PSIL_MSG_PID_DEFAULT;
                csmpParam.dcs   = PSIL_MSG_CODING_DEFAULT;
            }
            snprintf(rspBuf, ATEC_IND_RESP_64_STR_LEN, "+CSMP: %d,%d,%d,%d", csmpParam.fo, csmpParam.vp, csmpParam.pid, csmpParam.dcs);
            cmsRet = atcReply(atHandle, AT_RC_OK, 0, rspBuf);
            break;
        }

        case AT_SET_REQ:     //AT+CSMP=
        {
            /*AT+CSMP=[<fo>[,<vp>[,<pid>[,<dcs>]]]]*/
            memset(&setCsmpParam, 0x00, sizeof(MidWareSetCSMPParam));

            paraRet = atGetNumValue(pParamList, 0, (INT32*)&(setCsmpParam.csmpParam.fo),
                                    ATC_CSMP_0_FO_VAL_MIN, ATC_CSMP_0_FO_VAL_MAX, PSIL_MSG_FO_DEFAULT);
            if (paraRet == AT_PARA_OK)
            {
                setCsmpParam.foPresent = TRUE;
            }

            if (paraRet != AT_PARA_ERR)
            {
                paraRet = atGetNumValue(pParamList, 1, (INT32*)&(setCsmpParam.csmpParam.vp),
                                        ATC_CSMP_1_VP_VAL_MIN, ATC_CSMP_1_VP_VAL_MAX, PSIL_MSG_VP_DEFAULT);

                if (paraRet == AT_PARA_OK)
                {
                    setCsmpParam.vpPresent = TRUE;
                }
            }

            if (paraRet != AT_PARA_ERR)
            {
                paraRet = atGetNumValue(pParamList, 2, (INT32*)&(setCsmpParam.csmpParam.pid),
                                        ATC_CSMP_2_PID_VAL_MIN, ATC_CSMP_2_PID_VAL_MAX, PSIL_MSG_PID_DEFAULT);

                if (paraRet == AT_PARA_OK)
                {
                    setCsmpParam.pidPresent = TRUE;
                }
            }

            if (paraRet != AT_PARA_ERR)
            {
                paraRet = atGetNumValue(pParamList, 3, (INT32*)&(setCsmpParam.csmpParam.dcs),
                                        ATC_CSMP_3_DCS_VAL_MIN, ATC_CSMP_3_DCS_VAL_MAX, PSIL_MSG_CODING_DEFAULT);

                if (paraRet == AT_PARA_OK)
                {
                    setCsmpParam.dcsPresent = TRUE;
                }
            }

            if (setCsmpParam.foPresent || setCsmpParam.vpPresent ||
                setCsmpParam.pidPresent || setCsmpParam.dcsPresent)
            {
                mwSetAndSaveCsmpConfig(pAtCmdReq->chanId, &setCsmpParam);

                cmsRet = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                cmsRet = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_INVALID_TEXT_MODE_PARAMETER, NULL);
            }

            break;
        }

        default:
        {
            cmsRet = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_ALLOWED, NULL);
            break;
        }
    }

    return cmsRet;
}

/**
  \fn          CmsRetId  smsECSMSSEND(const AtCmdInputContext *pAtCmdReq)
  \brief       proc: AT+ECSMSSEND
  \param[in]   pAtCmdReq        AT CMD request info
  \returns     CmsRetId
*/
CmsRetId  smsECSMSSEND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT16  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT8   operaType = (UINT8)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    SmsErrorResultCode  smsErrCode = CMS_SMS_SUCC;
    INT32   mode = 0;
    UINT8   smsFormatMode = 0;  /* read from NVM, which set in AT+CMGF=<0/1> */

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECSMSSEND=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+ECSMSSEND: (1),(\"da\"),(128,129,145,161,177,193),(160)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECSMSSEND= */
        {
            /*
             * AT+ECSMSSEND=<mode>,<pdu/da>[,<toda>,<text_sms>]
            */
            if (atGetNumValue(pParamList, 0, (INT32 *)&mode,
                              ATC_ECSMSSEND_0_MODE_VAL_MIN, ATC_ECSMSSEND_0_MODE_VAL_MAX, ATC_ECSMSSEND_0_MODE_VAL_DEFAULT) != AT_PARA_OK)
            {
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
                break;
            }
                              
            /* check current input sms-mode whether match with the mode set in AT+CMGF */
            smsFormatMode = (PsilSmsFormatMode)mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SMS_MODE_CFG);
            if (smsFormatMode != mode)
            {
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_ALLOWED, NULL);
                break;
            }
            
            // check whether another SMS is ongoing
            if (pSmsSendInfo != PNULL)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, smsECSMSSEND_warn_1, P_WARNING, 1,
                            "AT CMD, +ECSMSSEND, another SMS is ongoing, chanId: %d", AT_GET_HANDLER_CHAN_ID(pSmsSendInfo->reqHander));

                ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
                break;
            }

            pSmsSendInfo = (PsilSmsSendInfo *)OsaAllocZeroMemory(sizeof(PsilSmsSendInfo));

            OsaDebugBegin(pSmsSendInfo != PNULL, sizeof(PsilSmsSendInfo), 0, 0);
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_MEMORY_FAILURE, NULL);
            break;
            OsaDebugEnd();

            pSmsSendInfo->reqHander = (UINT16)atHandle;

            if (mode == PSIL_SMS_FORMAT_PDU_MODE)
            {
                /* AT+ECSMSSEND=<mode>,<pdu> */
                if (pAtCmdReq->paramRealNum == 2 &&
                    atGetStrValue(pParamList, 1, pSmsSendInfo->input, PSIL_SMS_HEX_PDU_STR_MAX_SIZE,
                                  &(pSmsSendInfo->inputOffset), PNULL) == AT_PARA_OK)
                {
                    ret = smsSendSms(PSIL_SMS_FORMAT_PDU_MODE, pSmsSendInfo);

                    if (ret == CMS_INVALID_PARAM)
                    {
                        smsErrCode = CMS_SMS_INVALID_PDU_MODE_PARAMETER;
                    }
                    else if (ret == CMS_NO_MEM)
                    {
                        smsErrCode = CMS_SMS_MEM_FULL;
                    }
                    else if (ret != CMS_RET_SUCC)
                    {
                        smsErrCode = CMS_SMS_UNKNOWN_ERROR;
                    }
                }
                else
                {
                    smsErrCode = CMS_SMS_INVALID_PDU_MODE_PARAMETER;
                }
            }
            else    //text mode
            {
                /*AT+ECSMSSEND=<mode>,<pdu/da>[,<toda>,<text_sms>]*/
                if (smsGetInputAddrInfo(pParamList, 1, &(pSmsSendInfo->daInfo)) != TRUE)
                {
                    smsErrCode = CMS_SMS_INVALID_TEXT_MODE_PARAMETER;
                }

                if (smsErrCode == CMS_SMS_SUCC)
                {
                    /*<text_sms>*/
                    if (atGetStrValue(pParamList, 3, pSmsSendInfo->input, PSIL_SMS_MAX_TXT_SIZE+1,
                                      &(pSmsSendInfo->inputOffset), PNULL) == AT_PARA_OK)
                    {
                        ret = smsSendSms(PSIL_SMS_FORMAT_TXT_MODE, pSmsSendInfo);

                        if (ret == CMS_INVALID_PARAM)
                        {
                            smsErrCode = CMS_SMS_INVALID_PDU_MODE_PARAMETER;
                        }
                        else if (ret == CMS_NO_MEM)
                        {
                            smsErrCode = CMS_SMS_MEM_FULL;
                        }
                        else if (ret != CMS_RET_SUCC)
                        {
                            smsErrCode = CMS_SMS_UNKNOWN_ERROR;
                        }
                    }
                    else
                    {
                        smsErrCode = CMS_SMS_INVALID_TEXT_MODE_PARAMETER;
                    }
                }
            }

            if (smsErrCode != CMS_SMS_SUCC) //AT CMD FAILED
            {
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, smsErrCode, NULL);
            }

            if (pSmsSendInfo != PNULL)
            {
                OsaFreeMemory(&pSmsSendInfo);
            }

            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId smsCMMS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  smsCMMS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   mode;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CMMS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CMMS: (0-2)");
            break;
        }

        case AT_READ_REQ:         /* AT+CMMS? */
        {
            ret = smsGetMoreMessageState(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+CMMS=<mode> */
        {
            if (atGetNumValue(pParamList, 0, (INT32 *)&mode,
                              ATC_CMMS_0_MODE_VAL_MIN, ATC_CMMS_0_MODE_VAL_MAX, ATC_CMMS_0_MODE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = smsSetMoreMessageMode(atHandle, mode);

                if(ret == CMS_RET_SUCC)
                {
                    //CMMS cnf will call atcReply
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                }
                break;
            }
            else
            {
                /* invalid param */
                ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            }

            break;
        }

        default:         /* AT+CMMS */
        {
            ret = atcReply(atHandle, AT_RC_CMS_ERROR, CMS_SMS_OPERATION_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return ret;
}

