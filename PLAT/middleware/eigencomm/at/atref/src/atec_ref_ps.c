/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ref_ps.c
*
*  Description: Process protocol stack related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifdef  FEATURE_REF_AT_ENABLE
#include "atec_ref_ps.h"
#include "cmicomm.h"
#include "cmisim.h"
#include "ps_dev_if.h"
#include "ps_mm_if.h"
#include "ps_ps_if.h"
#include "ps_sim_if.h"
#include "ps_sms_if.h"

#include "mm_debug.h"//add for memory leak debug

#ifdef HEAP_MEM_DEBUG
extern size_t memDebugAllocated;
extern size_t memDebugFree;
extern size_t memDebugMaxFree;
extern size_t memDebugNumAllocs;
extern size_t memDebugNumFrees;
#endif

#define _DEFINE_AT_REQ_FUNCTION_LIST_


/**
  \fn          CmsRetId refPsNBAND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNBAND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32    band = 0;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   getValueRet = AT_PARA_ERR;
    UINT8   i = 0;
    CmiDevSetCiotBandReq    cmiSetBandReq = {0};  //20 bytes

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+NBAND=? */
        {
            ret = devGetSupportedCiotBandModeReq(atHandle);
            break;
        }

        case AT_READ_REQ:         /* AT+NBAND? */
        {
            ret = devGetCiotBandModeReq(atHandle);
            break;
        }

        case AT_SET_REQ:         /* AT+NBAND= */
        {
            memset(&cmiSetBandReq, 0x00, sizeof(CmiDevSetCiotBandReq));

            /* Fixed fill CMI_DEV_NB_MODE  */
            cmiSetBandReq.nwMode = CMI_DEV_NB_MODE;

            for (i = 0; i < pAtCmdReq->paramRealNum; i++)
            {
                getValueRet = atGetNumValue(pParamList, i, &band,
                                            ATC_NBAND_0_BAND_VAL_MIN, ATC_NBAND_0_BAND_VAL_MAX, ATC_NBAND_0_BAND_VAL_DEFAULT);

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
                cmiSetBandReq.bRefAT = TRUE;
                ret = devSetCiotBandModeReq(atHandle, &cmiSetBandReq);
            }


            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+NBAND */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId refPsNEARFCN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNEARFCN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32    mode = 0, cellId = 0;
    INT32    freqList[CMI_DEV_SUPPORT_MAX_FREQ_NUM] = {0};
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   paramNumb = pAtCmdReq->paramRealNum;

    switch (operaType)
    {
        case AT_SET_REQ:         /* AT+NEARFCN= */
        {
            if (paramNumb == 2 || paramNumb == 3)
            {
                if (atGetNumValue(pParamList, 0, &mode,
                    ATC_NEARFCN_0_NW_MODE_VAL_MIN, ATC_NEARFCN_0_NW_MODE_VAL_MAX, ATC_NEARFCN_0_NW_MODE_VAL_DEFAULT) == AT_PARA_OK)
                {
                    if (atGetNumValue(pParamList, 1, &freqList[0],
                                      ATC_NEARFCN_1_EARFCN_VAL_MIN, ATC_NEARFCN_1_EARFCN_VAL_MAX, ATC_NEARFCN_1_EARFCN_VAL_DEFAULT) == AT_PARA_OK)
                    {
                        if (pParamList[2].bDefault)
                        {
                            ret = devSetCiotFreqReq(atHandle, CMI_DEV_LOCK_CELL, freqList, 1, -1, TRUE);
                        }
                        else if(atGetNumValue(pParamList, 2, &cellId,
                                              ATC_NEARFCN_2_PHYCELL_VAL_MIN, ATC_NEARFCN_2_PHYCELL_VAL_MAX, ATC_NEARFCN_2_PHYCELL_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            ret = devSetCiotFreqReq(atHandle, CMI_DEV_LOCK_CELL, freqList, 1, cellId, TRUE);
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

        case AT_TEST_REQ:
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
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
  \fn          CmsRetId refPsNCSEARFCN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNCSEARFCN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_EXEC_REQ:         /* AT+NCSEARFCN */

            ret = devSetCiotFreqReq(atHandle, CMI_DEV_CLEAR_PREFER_FREQ_LIST, PNULL, 0, -1, TRUE);

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        default:
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
    }

    return ret;
}


/**
  \fn          CmsRetId refPsNCONFIG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNCONFIG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   paramNumb = pAtCmdReq->paramRealNum;
    BOOL    cmdValid = TRUE;
    UINT8   paraStr[ATC_NCFG_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 16;
    UINT16  paraLen = 0;
    CmiDevSetExtCfgReq setExtCfgReq;
    INT32   paraValue = -1;

    memset(&setExtCfgReq, 0x00, sizeof(CmiDevSetExtCfgReq));

    setExtCfgReq.bRefAT = TRUE;

    switch (operaType)
    {
        case AT_READ_REQ:            /* AT+NCONFIG?  */
        {
            ret = devGetExtCfg(atHandle);
            break;
        }

        case AT_SET_REQ :
        {
            if (2 == paramNumb)
            {
                memset(paraStr, 0x00, ATC_NCFG_0_MAX_PARM_STR_LEN);
                if (atGetStrValue(pParamList, 0, paraStr,
                                   ATC_NCFG_0_MAX_PARM_STR_LEN, &paraLen, ATC_NCFG_0_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_1, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                    cmdValid = FALSE;
                    break;
                }

                if (strcasecmp((const CHAR*)paraStr, "AUTOCONNECT") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_AUTO_CONNECT_STR_LEN_MAX, &paraLen, ATC_NCFG_1_AUTO_CONNECT_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.powerCfunPresent = TRUE;
                            setExtCfgReq.powerCfun = CMI_DEV_FULL_FUNC;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.powerCfunPresent = TRUE;
                            setExtCfgReq.powerCfun = CMI_DEV_MIN_FUNC;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_2, P_WARNING, 0, "AT+NCONFIG, invalid input 'AUTOCONNECT' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_3, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "CELL_RESELECTION") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_CELL_RESEL_STR_LEN_MAX, &paraLen, ATC_NCFG_1_CELL_RESEL_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.enableCellReselPresent = TRUE;
                            setExtCfgReq.enableCellResel = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.enableCellReselPresent = TRUE;
                            setExtCfgReq.enableCellResel = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_4, P_WARNING, 0, "AT+NCONFIG, invalid input 'CELL_RESELECTION' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_5, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "ENABLE_BIP") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_ENABLE_BIP_STR_LEN_MAX, &paraLen, ATC_NCFG_1_ENABLE_BIP_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.enableBipPresent = TRUE;
                            setExtCfgReq.enableBip = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.enableBipPresent = TRUE;
                            setExtCfgReq.enableBip = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_6, P_WARNING, 0, "AT+NCONFIG, invalid input 'ENABLE_BIP' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_7, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "MULTITONE") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_MULTITONE_STR_LEN_MAX, &paraLen, ATC_NCFG_1_MULTITONE_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.multiTonePresent = TRUE;
                            setExtCfgReq.multiTone = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.multiTonePresent = TRUE;
                            setExtCfgReq.multiTone = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_8, P_WARNING, 0, "AT+NCONFIG, invalid input 'MULTITONE' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_9, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "NAS_SIM_POWER_SAVING_ENABLE") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_SIM_PSM_ENABLE_STR_LEN_MAX, &paraLen, ATC_NCFG_1_SIM_PSM_ENABLE_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.enableSimPsmPresent = TRUE;
                            setExtCfgReq.enableSimPsm = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.enableSimPsmPresent = TRUE;
                            setExtCfgReq.enableSimPsm = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_10, P_WARNING, 0, "AT+NCONFIG, invalid input 'NAS_SIM_POWER_SAVING_ENABLE' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_11, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "RELEASE_VERSION") == 0)
                {
                    if (atGetNumValue(pParamList, 1, &paraValue,
                                      ATC_NCFG_1_REL_VERSION_VAL_MIN, ATC_NCFG_1_REL_VERSION_VAL_MAX, ATC_NCFG_1_REL_VERSION_VAL_DEFAULT) == AT_PARA_OK)
                    {
                        setExtCfgReq.relVersionPresent  = TRUE;
                        setExtCfgReq.relVersion         = (UINT8)paraValue;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_12, P_WARNING, 0, "AT+ECCFG, invalid input 'RELEASE_VERSION' value");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "IPV6_GET_PREFIX_TIME") == 0)
                {
                    if (atGetNumValue(pParamList, 1, &paraValue,
                                      ATC_NCFG_1_IPV6_GET_PREFIX_TIME_VAL_MIN, ATC_NCFG_1_IPV6_GET_PREFIX_TIME_VAL_MAX, ATC_NCFG_1_IPV6_GET_PREFIX_TIME_VAL_DEFAULT) == AT_PARA_OK)
                    {
                        setExtCfgReq.ipv6GetPrefixTimePresent  = TRUE;
                        setExtCfgReq.ipv6GetPrefixTime         = (UINT16)paraValue;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_13, P_WARNING, 0, "AT+ECCFG, invalid input 'RELEASE_VERSION' value");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "NB_CATEGORY") == 0)
                {
                    if (atGetNumValue(pParamList, 1, &paraValue,
                                      ATC_NCFG_1_NB_CATEGORY_VAL_MIN, ATC_NCFG_1_NB_CATEGORY_VAL_MAX, ATC_NCFG_1_NB_CATEGORY_VAL_DEFAULT) == AT_PARA_OK)
                    {
                        setExtCfgReq.nbCategoryPresent  = TRUE;
                        setExtCfgReq.nbCategory         = (UINT8)paraValue;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_14, P_WARNING, 0, "AT+ECCFG, invalid input 'NB_CATEGORY' value");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "RAI") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_RAI_STR_LEN_MAX, &paraLen, ATC_NCFG_1_RAI_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.upRaiPresent = TRUE;
                            setExtCfgReq.supportUpRai = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.upRaiPresent = TRUE;
                            setExtCfgReq.supportUpRai = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_15, P_WARNING, 0, "AT+NCONFIG, invalid input 'RAI' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_16, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }

                else if (strcasecmp((const CHAR*)paraStr, "HEAD_COMPRESS") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_HEAD_COMPRESS_STR_LEN_MAX, &paraLen, ATC_NCFG_1_HEAD_COMPRESS_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.rohcPresent = TRUE;
                            setExtCfgReq.bRohc = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.rohcPresent = TRUE;
                            setExtCfgReq.bRohc = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_17, P_WARNING, 0, "AT+NCONFIG, invalid input 'HEAD_COMPRESS' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_18, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "CONNECTION_REESTABLISHMENT") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_CONNECTION_REEST_STR_LEN_MAX, &paraLen, ATC_NCFG_1_CONNECTION_REEST_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.enableConnReEstPresent = TRUE;
                            setExtCfgReq.enableConnReEst = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.enableConnReEstPresent = TRUE;
                            setExtCfgReq.enableConnReEst = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_19, P_WARNING, 0, "AT+NCONFIG, invalid input 'CONNECTION_REESTABLISHMENT' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_20, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "PCO_IE_TYPE") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_PCO_IE_TYPE_STR_LEN_MAX, &paraLen, ATC_NCFG_1_PCO_IE_TYPE_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "EPCO") == 0)
                        {
                            setExtCfgReq.epcoPresent = TRUE;
                            setExtCfgReq.enableEpco = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "PCO") == 0)
                        {
                            setExtCfgReq.epcoPresent = TRUE;
                            setExtCfgReq.enableEpco = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_21, P_WARNING, 0, "AT+NCONFIG, invalid input 'PCO_IE_TYPE' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_22, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "TWO_HARQ") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_TWO_HARQ_STR_LEN_MAX, &paraLen, ATC_NCFG_1_TWO_HARQ_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.supportTwoHarqPresent = TRUE;
                            setExtCfgReq.supportTwoHarq = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.supportTwoHarqPresent = TRUE;
                            setExtCfgReq.supportTwoHarq = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_23, P_WARNING, 0, "AT+NCONFIG, invalid input 'TWO_HARQ' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_24, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "HPPLMN_SEARCH_ENABLE") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_HPPLMN_SEARCH_ENABLE_STR_LEN_MAX, &paraLen, ATC_NCFG_1_HPPLMN_SEARCH_ENABLE_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.enableHPPlmnSearchPresent = TRUE;
                            setExtCfgReq.enableHPPlmnSearch = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.enableHPPlmnSearchPresent = TRUE;
                            setExtCfgReq.enableHPPlmnSearch = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_25, P_WARNING, 0, "AT+NCONFIG, invalid input 'HPPLMN_SEARCH_ENABLE' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_26, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "T3324_T3412_EXT_CHANGE_REPORT") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_T3324_T3412_EXT_CHANGE_RPT_STR_LEN_MAX, &paraLen, ATC_NCFG_1_T3324_T3412_EXT_CHANGE_RPT_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.t3324AndT3412ExtCeregUrcPresent = TRUE;
                            setExtCfgReq.t3324AndT3412ExtCeregUrc = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.t3324AndT3412ExtCeregUrcPresent = TRUE;
                            setExtCfgReq.t3324AndT3412ExtCeregUrc = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_27, P_WARNING, 0, "AT+NCONFIG, invalid input 'T3324_T3412_EXT_CHANGE_REPORT' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_28, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "NON_IP_NO_SMS_ENABLE") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_NON_IP_NO_SMS_ENABLE_STR_LEN_MAX, &paraLen, ATC_NCFG_1_NON_IP_NO_SMS_ENABLE_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.enableNonIpNoSmsPresent = TRUE;
                            setExtCfgReq.enableNonIpNoSms = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.enableNonIpNoSmsPresent = TRUE;
                            setExtCfgReq.enableNonIpNoSms = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_29, P_WARNING, 0, "AT+NCONFIG, invalid input 'NON_IP_NO_SMS_ENABLE' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_30, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strcasecmp((const CHAR*)paraStr, "SUPPORT_SMS") == 0)
                {
                    if (atGetStrValue(pParamList, 1, paraStr,
                                      ATC_NCFG_1_SUPPORT_SMS_STR_LEN_MAX, &paraLen, ATC_NCFG_1_SUPPORT_SMS_STR_DEFAULT) == AT_PARA_OK)
                    {
                        if (strcasecmp((const CHAR*)paraStr, "TRUE") == 0)
                        {
                            setExtCfgReq.supportSmsPresent = TRUE;
                            setExtCfgReq.supportSms = TRUE;
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "FALSE") == 0)
                        {
                            setExtCfgReq.supportSmsPresent = TRUE;
                            setExtCfgReq.supportSms = FALSE;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_31, P_WARNING, 0, "AT+NCONFIG, invalid input 'SUPPORT_SMS' value");
                            cmdValid = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_32, P_WARNING, 0, "AT+NCONFIG, invalid input string parameter");
                        cmdValid = FALSE;
                        break;
                    }
                }
                else if (strlen((const CHAR*)paraStr) > 0)   // other string
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_33, P_WARNING, 0, "AT+ECCFG, invalid input parameters");
                    cmdValid = FALSE;
                    break;
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCONFIG_34, P_WARNING, 1, "AT+NCONFIG, invalid parameters number: %d", paramNumb);
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
             * AT+NCONFIG=? test command
            */
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(AUTOCONNECT,(FALSE,TRUE))\n+NCONFIG:(CELL_RESELECTION,(FALSE,TRUE))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(ENABLE_BIP,(FALSE,TRUE))\n+NCONFIG:(MULTITONE,(FALSE,TRUE))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(NAS_SIM_POWER_SAVING_ENABLE,(FALSE,TRUE))\n+NCONFIG:(RELEASE_VERSION,(13,14))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(IPV6_GET_PREFIX_TIME,(0-65535))\n+NCONFIG:(NB_CATEGORY,(1,2))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(RAI,(FALSE,TRUE))\n+NCONFIG:(HEAD_COMPRESS,(FALSE,TRUE))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(CONNECTION_REESTABLISHMENT,(FALSE,TRUE))\n+NCONFIG:(TWO_HARQ,(FALSE,TRUE))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(PCO_IE_TYPE,(PCO,EPCO))\n+NCONFIG:(T3324_T3412_EXT_CHANGE_REPORT,(FALSE,TRUE))");
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)"+NCONFIG:(NON_IP_NO_SMS_ENABLE,(FALSE,TRUE))\n+NCONFIG:(SUPPORT_SMS,(FALSE,TRUE))");
            ret = atcReply(atHandle, AT_RC_OK,       0, (CHAR *)"+NCONFIG:(HPPLMN_SEARCH_ENABLE,(FALSE,TRUE))");

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
  \fn          CmsRetId refPsNCPCDPR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNCPCDPR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_2_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 paramNumb = pAtCmdReq->paramRealNum;
    CmiDevSetExtCfgReq setExtCfgReq;
    INT32              ipType;
    INT32              readState;

    memset(&setExtCfgReq, 0x00, sizeof(CmiDevSetExtCfgReq));

    setExtCfgReq.bRefAT = TRUE;

    switch (operaType)
    {
        case AT_TEST_REQ:
        {
            /*
                     * AT+NCPCDPR=? test command
                     * +NCPCDPR:(list of supported <parameter>s),(list of supported <state>s)
                    */
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+NCPCDPR: (0,1),(0,1)");
            break;
        }

        case AT_READ_REQ:            /* AT+NCPCDPR?  */
        {
            ret = devGetExtCfg(atHandle);
            break;
        }

        case AT_SET_REQ:
        {
            /*
                     *      AT+NCPCDPR=<parameter>,<state>
                     */
            ret = CMS_RET_SUCC;

            if (2 == paramNumb)
            {
                if (atGetNumValue(pParamList, 0, &ipType,
                                  ATC_NCPCDPR_0_IP_TYPE_VAL_MIN, ATC_NCPCDPR_0_IP_TYPE_VAL_MAX, ATC_NCPCDPR_0_IP_TYPE_VAL_DEFAULT) != AT_PARA_OK)
                {
                    ret = CMS_FAIL;
                }

                if (ret == CMS_RET_SUCC)
                {
                    if (atGetNumValue(pParamList, 1, &readState,
                                      ATC_NCPCDPR_1_READ_STATE_VAL_MIN, ATC_NCPCDPR_1_READ_STATE_VAL_MAX, ATC_NCPCDPR_1_READ_STATE_VAL_DEFAULT) != AT_PARA_ERR)
                    {
                        setExtCfgReq.dnsIpAddrReadCfgPresent = TRUE;
                        setExtCfgReq.dnsIpType = ipType;
                        setExtCfgReq.enableDnsIpAddrRead = (BOOL)readState;
                    }
                    else
                    {
                        ret = CMS_FAIL;
                    }
                }
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNCPCDPR_1, P_WARNING, 1, "AT+NCPCDPR, invalid parameters number: %d", paramNumb);
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = devSetExtCfg(atHandle, &setExtCfgReq);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
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
  \fn          CmsRetId refPsNUESTATS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNUESTATS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 paramNumb = pAtCmdReq->paramRealNum;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   paraStr[ATC_NUESTATS_0_MAX_PARM_STR_LEN] = {0};
    UINT16   paraLen = 0;
    UINT16    cmdValid = 1;
    UINT8   statusType = CMI_DEV_GET_ECSTATUS;
    #ifdef HEAP_MEM_DEBUG
    CHAR atRspBuf[200] = {0};
    #endif
    switch (operaType)
    {
        case AT_EXEC_REQ:        /* AT+NUESTATS */
        {
            devGetESTATUS(atHandle, CMI_DEV_GET_NUESTATS);
            ret = CMS_RET_SUCC;
            break;
        }
        case AT_SET_REQ:           /* AT+NUESTATS= */
        {
            if (paramNumb == 1)
            {
                memset(paraStr, 0x00, ATC_NUESTATS_0_MAX_PARM_STR_LEN);
                if (atGetStrValue(pParamList, 0, paraStr,
                                   ATC_NUESTATS_0_MAX_PARM_STR_LEN, &paraLen, ATC_NUESTATS_0_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNUESTATS_1, P_WARNING, 0, "AT+NUESTATS, invalid input string parameter");
                    cmdValid = 0;
                    break;
                }

                if (strcasecmp((const CHAR*)paraStr, "RADIO") == 0)
                {
                    statusType = CMI_DEV_GET_NUESTATS_RADIO;
                }
                else if (strcasecmp((const CHAR*)paraStr, "CELL") == 0)
                {
                    statusType = CMI_DEV_GET_NUESTATS_CELL;
                }
                else if (strcasecmp((const CHAR*)paraStr, "BLER") == 0)
                {
                    statusType = CMI_DEV_GET_NUESTATS_BLER;
                }
                else if (strcasecmp((const CHAR*)paraStr, "THP") == 0)
                {
                    statusType = CMI_DEV_GET_NUESTATS_THP;
                }
                else if (strcasecmp((const CHAR*)paraStr, "ALL") == 0)
                {
                    statusType = CMI_DEV_GET_NUESTATS_ALL;
                }
                else if (strcasecmp((const CHAR*)paraStr, "APPSMEM") == 0)
                {
                    cmdValid = 2;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNUESTATS_2, P_WARNING, 0, "AT+NUESTATS, invalid input string parameter");
                    cmdValid = 0;
                    break;
                }

                if (cmdValid == 1)
                {
                    ret = devGetESTATUS(atHandle, statusType);
                }
                else if(cmdValid == 2)
                {
                    #ifdef HEAP_MEM_DEBUG
                    sprintf(atRspBuf, "NUESTATS:APPSMEM,Current Allocated:%d\r\n\nNUESTATS:APPSMEM,Total Free:%d\r\n\nNUESTATS:APPSMEM,Max Free:%d\r\n\nNUESTATS:APPSMEM,Num Allocs:%d\r\n\nNUESTATS:APPSMEM,Num Frees:%d\r\n",
                        memDebugAllocated, memDebugFree, memDebugMaxFree, memDebugNumAllocs, memDebugNumFrees);

                    ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)atRspBuf);
                    #else
                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
                    #endif
                    break;
                }

                if (ret != CMS_RET_SUCC)
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
                break;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, refPsNUESTATS_3, P_WARNING, 1, "AT+NUESTATS, invalid parameters number: %d", paramNumb);
            }
            break;
        }
        case AT_TEST_REQ:          /* AT+NUESTATS=? */
        {
            /* NUESTATS: (RADIO,CELL,BLER,THP,APPSEM,ALL) */
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"NUESTATS: (RADIO,CELL,BLER,THP,APPSMEM,ALL)");
            break;
        }

        case AT_READ_REQ:           /* AT+NUESTATS? */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}



__WEAK void atCmdURCReportInPMUFlow(bool enable)	// realize in hal_uart_atcmd.c
{


}

/**
  \fn          CmsRetId refPsNPSMR(const AtCmdInputContext *pAtCmdReq)
  \brief       wakeup indication
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNPSMR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR    respStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    INT32   rptMode;


    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+NPSMR=? */
        {
            sprintf((char*)respStr, "+NPSMR: (0,1)\r\n");

            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)respStr);
            break;
        }
        case AT_READ_REQ:             /* AT+NPSMR? */
        {
            UINT8   n;
            CHAR    atRspBuf[40] = {0};

            //ret = mmGetECPSMR(atHandle);

            n = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG);

            /* The read command returns "+NPSMR:<n>" when <n> is 0,
               and return "+NPSMR:<n>,<mode>" when <n> is 1. */
            if (ATC_NPSMR_DISABLE == n)
            {
                sprintf(atRspBuf, "+NPSMR: 0");
            }
            else
            {
                if (TRUE == slpManAPPGetAONFlag1())
                {
                    sprintf(atRspBuf, "+NPSMR: 1,1");
                }
                else
                {
                    sprintf(atRspBuf, "+NPSMR: 1,0");
                }
            }

            ret = atcReply(atHandle, AT_RC_OK, 0, atRspBuf);

            break;

        }
        case AT_SET_REQ:             /* AT+NPSMR= */
        {
            if (atGetNumValue(pParamList, 0, &rptMode,
                              ATC_NPSMR_0_VAL_MIN, ATC_NPSMR_0_VAL_MAX, ATC_NPSMR_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (rptMode >= ATC_NPSMR_DISABLE && rptMode <= ATC_NPSMR_ENABLE)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG, rptMode);
                    slpManAPPSetAONFlag1(FALSE);

                    if (rptMode == ATC_NPSMR_DISABLE)
                    {
                        atCmdURCReportInPMUFlow(false);
                    }
                    else
                    {
                        atCmdURCReportInPMUFlow(true);
                    }
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
  \fn          CmsRetId refPsNPTWEDRXS(const AtCmdReqParaType *AtCmdReqParaPtr)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     RetCodeT
*/
CmsRetId refPsNPTWEDRXS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32   atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 modeVal;
    INT32 actType;
    UINT8 ptw[ATC_NPTWEDRXS_2_STR_MAX_LEN + 1] = { 0 };  /*+"\0"*/
    UINT8 edrxVal[ATC_NPTWEDRXS_3_STR_MAX_LEN + 1] = { 0 };  /*+"\0"*/
    UINT16 length;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+NPTWEDRXS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+NPTWEDRXS: (0,1,2,3),(5),(\"0000\"-\"1111\"),(\"0000\"-\"1111\")");
            break;
        }

        case AT_READ_REQ:         /* AT+NPTWEDRXS? */
        {
            ret = mmGetPtwEdrx(atHandle);
            //ECPTWEDRXS cnf will call ATCRespStr
            break;
        }

        case AT_SET_REQ:          /* AT+NPTWEDRXS=<mode>,<Act-type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>]] */
        {
            ret = CMS_RET_SUCC;

            /* Get mode. */
            if (atGetNumValue(pParamList, 0, &modeVal,
                              ATC_NPTWEDRXS_0_MODE_VAL_MIN, ATC_NPTWEDRXS_0_MODE_VAL_MAX, ATC_NPTWEDRXS_0_MODE_VAL_DEFAULT) != AT_PARA_OK)
            {
                ret = CMS_FAIL;
            }

            /* Get actType. */
            if (atGetNumValue(pParamList, 1, &actType,
                              ATC_NPTWEDRXS_1_VAL_MIN, ATC_NPTWEDRXS_1_VAL_MAX, ATC_NPTWEDRXS_1_VAL_DEFAULT) != AT_PARA_OK)
            {
                ret = CMS_FAIL;
            }

            /* Get ptw. */
            if (ret == CMS_RET_SUCC)
            {
                if (atGetStrValue(pParamList, 2, ptw,
                                   ATC_NPTWEDRXS_2_STR_MAX_LEN + 1, &length, ATC_NPTWEDRXS_2_STR_DEFAULT) != AT_PARA_ERR)
                {
                    if (ptw[0] != 0 && strlen((CHAR *)ptw) != ATC_NPTWEDRXS_2_STR_MAX_LEN)
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
                                   ATC_NPTWEDRXS_3_STR_MAX_LEN + 1, &length, ATC_NPTWEDRXS_3_STR_DEFAULT) != AT_PARA_ERR)
                {
                    if (edrxVal[0] != 0 && strlen((CHAR *)edrxVal) != ATC_NPTWEDRXS_3_STR_MAX_LEN)
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
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, ATC_NPTWEDRXS_ENABLE);
                }
                else
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, ATC_NPTWEDRXS_DISABLE);
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
  \fn          CmsRetId refPsNPOWERCLASS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId refPsNPOWERCLASS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    INT32       band;
    INT32       powerClass;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    BOOL        validAT = TRUE;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+NPOWERCLASS=? */
        {
            ret = devGetSupportedBandPowerClassReq(atHandle);
            break;
        }
        case AT_READ_REQ:   /* AT+NPOWERCLASS? */
        {
            ret = devGetECPOWERCLASS(atHandle);
            break;
        }
        case AT_SET_REQ:    /* AT+NPOWERCLASS= */
        {
            CmiDevSetPowerClassReq    setPowerClassReq;

            /* band */
            if (atGetNumValue(pParamList, 0, &band, ATC_NPOWERCLASS_1_VAL_MIN, ATC_NPOWERCLASS_1_VAL_MAX, ATC_NPOWERCLASS_1_VAL_DEFAULT) != AT_PARA_ERR)
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
                if (atGetNumValue(pParamList, 1, &powerClass, ATC_NPOWERCLASS_2_VAL_MIN, ATC_NPOWERCLASS_2_VAL_MAX, ATC_NPOWERCLASS_2_VAL_DEFAULT) != AT_PARA_ERR)
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
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, refPsNPOWERCLASS_1, P_WARNING, 0, "ATCMD, invalid ATCMD: AT+NPOWERCLASS");
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }
        case AT_EXEC_REQ:   /* AT+NPOWERCLASS */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId refPsNCCID(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   *pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refPsNCCID(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+NCCID=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+NCCID */
        case AT_READ_REQ:         /* AT+NCCID?*/
        {
            ret = simGetIccid(atHandle);
            break;
        }

        default:         /* AT+NCCID= */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId refPsNPIN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   *pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refPsNPIN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 command = 0;
    UINT8 param1Len = 0, param2Len = 0;
    UINT8 parameter1[ATC_NPIN_1_STR_MAX_LEN + 1] = {0};/*+"\0"*/
    UINT8 parameter2[ATC_NPIN_2_STR_MAX_LEN + 1] = {0};/*+"\0"*/

    switch (operaType)
    {
        case AT_SET_REQ:         /* AT+NPIN=<command>,<parameter1>[,<paramter2>] */
        {
            ret = CMS_RET_SUCC;
            // <command>
            if(atGetNumValue(pParamList, 0, &command, ATC_NPIN_0_VAL_MIN, ATC_NPIN_0_VAL_MAX, ATC_NPIN_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                /*
                * change PIN/unblock PIN, paramter2 is mandatory
                */
                if ((command == ATC_NPIN_CHANGE_PIN || command == ATC_NPIN_UNBLOCK_PIN) && (pAtCmdReq->paramRealNum != 3))
                {
                    ret = CMS_FAIL;
                }
            }
            else
            {
                ret = CMS_FAIL;
            }
            // <parameter1>
            if(atGetStrValue(pParamList, 1, parameter1, ATC_NPIN_1_STR_MAX_LEN + 1, (UINT16 *)&param1Len, ATC_NPIN_1_STR_DEFAULT) == AT_PARA_OK)
            {
                //Only (decimal) digits (0-9) shall be used, the number of digit for PIN is 4 - 8,  the number of digit for PUK is always 8
                if ((param1Len < ATC_NPIN_1_STR_MIN_LEN) ||
                    (atBeNumericString(parameter1) == FALSE) ||
                    ((command == ATC_NPIN_UNBLOCK_PIN) &&
                     (param1Len != ATC_NPIN_1_STR_MAX_LEN)))
                {

                    ret = CMS_FAIL;
                }
            }
            else
            {
                ret = CMS_FAIL;
            }
            // <parameter2>
            if ((ret == CMS_RET_SUCC) && (pAtCmdReq->paramRealNum == 3))
            {
                if(atGetStrValue(pParamList, 2, parameter2, ATC_NPIN_2_STR_MAX_LEN + 1, (UINT16 *)&param2Len, ATC_NPIN_2_STR_DEFAULT) == AT_PARA_OK)
                {
                    //Only (decimal) digits (0-9) shall be used, the number of digit for PIN is 4 - 8
                    if ((param2Len < ATC_NPIN_2_STR_MIN_LEN) ||
                        (atBeNumericString(parameter2) == FALSE))
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
                switch (command)
                {
                    case ATC_NPIN_VERIFY_PIN:
                    case ATC_NPIN_ENABLE_PIN:
                    case ATC_NPIN_DISABLE_PIN:
                        ret = simSetPinOperation(atHandle, command, parameter1, PNULL);
                        break;

                    case ATC_NPIN_CHANGE_PIN:
                    case ATC_NPIN_UNBLOCK_PIN:
                        ret = simSetPinOperation(atHandle, command, parameter1, parameter2);
                        break;

                    default:
                        ret = CMS_FAIL;
                        break;
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
  \fn          CmsRetId refPsNUICC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   *pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refPsNUICC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_REF_SUB_AT_1_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR respStr[ATEC_IND_RESP_32_STR_LEN] = {0};
    INT32 mode = 0;
    BOOL sleepMode = FALSE;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+NUICC=? */
        {
            sprintf((char*)respStr, "+NUICC: (0,1)");

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)respStr);
            break;
        }

        case AT_SET_REQ:         /* AT+NUICC=<mode> */
        {
            // <mode>
            if(atGetNumValue(pParamList, 0, &mode, ATC_NUICC_0_VAL_MIN, ATC_NUICC_0_VAL_MAX, ATC_NUICC_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (mode == 0 || mode ==1)
                {
                    sleepMode = (mode == 0) ? TRUE : FALSE;
                    ret = simSetSimSleep(atHandle, sleepMode);
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


#endif

