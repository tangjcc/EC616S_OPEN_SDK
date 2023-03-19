/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_plat.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atc_decoder.h"
#include "ctype.h"
#include "cmicomm.h"
#include "atec_general.h"


#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position----Below for internal use
/**
  \fn           void gcEnableAllURC(void)
  \brief        Enable all URC
  \returns      void
*/
static void gcEnableAllURC(UINT8 atChanId)
{
    BOOL    cregCfgChanged = FALSE, ceregCfgChanged = FALSE, edrxCfgChanged = FALSE, ciotCfgChanged = FALSE, csconCfgChanged = FALSE;
    BOOL    ctzrCfgChanged = FALSE, eccesqCfgChanged = FALSE, cgerpCfgChanged = FALSE, ecpsmrCfgChanged = FALSE, ptwEdrxCfgChanged = FALSE;
    BOOL    ecpinCfgChanged = FALSE, ecpaddrCfgChanged = FALSE, ecpcfunCfgChanged = FALSE;

    /*CREG URC*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CREG_RPT_CFG, 3, &cregCfgChanged);    //CMI_MM_CREG_LOC_REJ_INFO

    /*CEREG URC*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG, 5, &ceregCfgChanged);  //CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE

    /*EDRX URC, +CEDRXP: <AcT-type>[,<Requested_eDRX_value>[,<NW_provided_eDRX_value>[,<Paging_time_window>]]]*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG, 1, &edrxCfgChanged);

    /*+CCIOTOPTI: <supported_Network_opt>*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG, 1, &ciotCfgChanged);

    /*+CSCON: <mode>*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG, 1, &csconCfgChanged);  //ATC_CSCON_RPT_MODE

    /*+CTZEU: <tz>,<dst>,[<utime>]*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG, 3, &ctzrCfgChanged);   //ATC_CTZR_RPT_UTC_TIME

    /*+ECCESQ: */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG, 2, &eccesqCfgChanged);    //ATC_ECCESQ_RPT_ECCESQ

    /*+CGEV:*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG, 1, &cgerpCfgChanged);    //ATC_CGEREP_FWD_CGEV

    /*+ECPSMR*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_PSM_RPT_CFG, 1, &ecpsmrCfgChanged);

    /*PTWEDRX URC, +ECPTWEDRXP: <AcT-type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>[,<NW_provided_eDRX_value>[,<Paging_time_window>]]]]*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, 1, &ptwEdrxCfgChanged);

    /*ECPIN URC, +ECPIN:<code> */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG, 1, &ecpinCfgChanged);

    /*ECPADDR URC, +ECPADDR:<code> */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECPADDR_RPT_CFG, 1, &ecpaddrCfgChanged);

    /*ECPCFUN URC, +ECPCFUN:<code> */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG, 1, &ecpcfunCfgChanged);


    if (cregCfgChanged || ceregCfgChanged || edrxCfgChanged || ciotCfgChanged ||
        csconCfgChanged || ctzrCfgChanged || eccesqCfgChanged || cgerpCfgChanged ||
        ecpsmrCfgChanged || ptwEdrxCfgChanged || ecpinCfgChanged || ecpaddrCfgChanged ||
        ecpcfunCfgChanged)
    {
        mwSaveAtChanConfig();
    }

    return;
}


/**
  \fn           void gcEnableAllURC(void)
  \brief        Enable all URC
  \returns      void
*/
static void gcDisableAllURC(UINT8 atChanId)
{
    BOOL    cregCfgChanged = FALSE, ceregCfgChanged = FALSE, edrxCfgChanged = FALSE, ciotCfgChanged = FALSE, csconCfgChanged = FALSE;
    BOOL    ctzrCfgChanged = FALSE, eccesqCfgChanged = FALSE, cgerpCfgChanged = FALSE, ecpsmrCfgChanged = FALSE, ptwEdrxCfgChanged = FALSE;
    BOOL    ecpinCfgChanged = FALSE, ecpaddrCfgChanged = FALSE, ecpcfunCfgChanged = FALSE;

    /*CREG URC*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CREG_RPT_CFG, 0, &cregCfgChanged);

    /*CEREG URC*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG, 0, &ceregCfgChanged);

    /*EDRX URC, +CEDRXP: <AcT-type>[,<Requested_eDRX_value>[,<NW_provided_eDRX_value>[,<Paging_time_window>]]]*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG, 0, &edrxCfgChanged);

    /*+CCIOTOPTI: <supported_Network_opt>*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG, 0, &ciotCfgChanged);

    /*+CSCON: <mode>*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG, 0, &csconCfgChanged);

    /*+CTZEU: <tz>,<dst>,[<utime>]*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG, 0, &ctzrCfgChanged);

    /*+ECCESQ: */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG, 0, &eccesqCfgChanged);

    /*+CGEV:*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG, 0, &cgerpCfgChanged);

    /*+ECPSMR*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_PSM_RPT_CFG, 0, &ecpsmrCfgChanged);

    /*PTWEDRX URC, +ECPTWEDRXP: <AcT-type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>[,<NW_provided_eDRX_value>[,<Paging_time_window>]]]]*/
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, 0, &ptwEdrxCfgChanged);

    /*ECPIN URC, +ECPIN:<code> */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG, 0, &ecpinCfgChanged);

    /*ECPADDR URC, +ECPADDR:<code> */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECPADDR_RPT_CFG, 0, &ecpaddrCfgChanged);

    /*ECPCFUN URC, +ECPCFUN:<code> */
    mwSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG, 0, &ecpcfunCfgChanged);

    if (cregCfgChanged || ceregCfgChanged || edrxCfgChanged || ciotCfgChanged ||
        csconCfgChanged || ctzrCfgChanged || eccesqCfgChanged || cgerpCfgChanged ||
        ecpsmrCfgChanged || ptwEdrxCfgChanged || ecpinCfgChanged || ecpaddrCfgChanged ||
        ecpcfunCfgChanged)
    {
        mwSaveAtChanConfig();
    }

    return;
}



#define __DEFINE_AT_REQ_FUNCTION_LIST__
/**
  \fn          CmsRetId gcAT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcAT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);

    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

    return ret;
}

/**
  \fn          CmsRetId gcATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32  echoValue = 0xff;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    //CHAR RspBuf[32] = {0};

    switch (operaType)
    {
        #if 0
        case AT_READ_REQ:       /* ATE? */
        {
            sprintf((char*)RspBuf, "+ECHO: %d", pAtChanEty->cfg.echoFlag);
            ret = atcReply(atHandle, AT_RC_OK, 0, RspBuf);
            break;
        }
        #endif

        case AT_SET_REQ:        /* ATE */
        {
            if ((atGetNumValue(pParamList, 0, &echoValue, ATC_E_0_VAL_MIN, ATC_E_0_VAL_MAX, ATC_E_0_VAL_DEFAULT)) == AT_PARA_OK)
            {
                ret = atcSetConfigValue(pAtCmdReq->chanId, ATC_CFG_ECHO_PARAM, (UINT32)echoValue);
                if (ret == CMS_RET_SUCC)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECHO_CFG, (UINT32)echoValue);
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
  \fn          CmsRetId gcATQ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcATQ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32  suppressValue = 0xff;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        #if 0
        case AT_READ_REQ:         /* ATQ? */
        {
            sprintf((char*)RspBuf, "+SUPPRESS: %d", pAtChanEty->cfg.suppressValue);
            ret = atcReply(atHandle, AT_RC_OK, 0, RspBuf);
            break;
        }
        #endif

        case AT_SET_REQ:         /* ATQ */
        {
            if ((atGetNumValue(pParamList, 0, &suppressValue, ATC_Q_0_VAL_MIN, ATC_E_0_VAL_MAX, ATC_E_0_VAL_DEFAULT)) == AT_PARA_OK)
            {
                ret = atcSetConfigValue(pAtCmdReq->chanId, ATC_CFG_SUPPRESS_PARAM, (UINT32)suppressValue);
                if (ret == CMS_RET_SUCC)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_SUPPRESS_CFG, (UINT32)suppressValue);
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
  \fn          CmsRetId gcATW(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcATW(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    //CHAR        RspBuf[32] = {0};

    switch (operaType)
    {
        #if 0
        case AT_READ_REQ:         /* AT&W? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, RspBuf);
            break;
        }

        case AT_EXEC_REQ:         /* AT&W */
        {
            if(ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            }
            break;
        }
        #endif

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId gcCSCS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcCSCS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CSCS=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CSCS:\"IRA\",\"UCS2\"");
            break;
        }

        case AT_READ_REQ:         /* AT+CSCS? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CSCS:\"IRA\",\"UCS2\"");
            break;
        }

        case AT_SET_REQ:          /* AT+CSCS= */
        {

            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
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
  \fn          CmsRetId gcCMUX(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcCMUX(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CMUX=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CMUX: ");
            break;
        }

        case AT_READ_REQ:         /* AT+CMUX? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CMUX: ");
            break;
        }

        case AT_SET_REQ:          /* AT+CMUX= */
        {

            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
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
  \fn          CmsRetId gcCMEE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcCMEE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   cmeeValue = 0xff;
    CHAR    respStr[ATEC_IND_RESP_128_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CMEE=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CMEE: (0-2)");
            break;
        }

        case AT_READ_REQ:         /* AT+CMEE? */
        {
            snprintf(respStr,
                     ATEC_IND_RESP_128_STR_LEN,
                     "+CMEE: %d",
                     mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CMEE_CFG));

            ret = atcReply(atHandle, AT_RC_OK, 0, respStr);
            break;
        }

        case AT_SET_REQ:          /* AT+CMEE= */
        {
            if (atGetNumValue(pParamList, 0, &cmeeValue, ATC_CMEE_0_VAL_MIN, ATC_CMEE_0_VAL_MAX, ATC_CMEE_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                if (cmeeValue >= ATC_CMEE_DISABLE_ERROR_CODE &&
                    cmeeValue <= ATC_CMEE_VERBOSE_ERROR_CODE)
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CMEE_CFG, cmeeValue);

                    ret = CMS_RET_SUCC;
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
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
  \fn          CmsRetId gcECURC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId gcECURC(const AtCmdInputContext *pAtCmdReq)
{
    /*
     * internal using, to close/open URC (unsolicited result code) report
     * AT+ECURC=<urcStr>,<value>
     * <urcStr> value:
     *  "all" - configure all URC
     *  "creg" - configure +CREG URC
     *  "cereg" - configure +CEREG URC
     *  "cedrxp" - configure +CEDRXP URC
     *  "cciotopti" - configure +CCIOTOPTI URC
     *  "cscon" - configure +CSCON URC
     *  "eccesq" - configure +ECCESQ URC
     *  "cgev" - configure +CGEV URC
     *  "ecpsmr" - configure +ECPSMR URC
     *  "ptwedrx" - configure +PTWEDRX URC
     *  "ecptwedrxp" - configure +ECPTWEDRXP URC
     *  "ecpin" - configure +ECPIN URC
     *  "ECPADDR" - configure +ECPADDR URC
     *  "ECPCFUN" - configure +ECPCFUN URC
     * <value> value:
     *  0 - disable URC report
     *  1 - enable URC report
    */

    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32       nValue = 0xff;
    UINT8       paraStr[ATC_ECURC_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 16;
    UINT16      paraLen = 0;
    CHAR        *rspBuf = PNULL;
    UINT8       cregRptMode = 0, ceregRptMode = 0, edrxRptMode = 0, ciotRptMode = 0, csconRptMode = 0, ctzrRptMode = 0;
    UINT8       eccesqRptMode = 0, cgerpRptMode = 0, ecpsmrRptMode = 0, ptwEdrxRptMode = 0, ecpinRptMode = 0;
    UINT8       ecpaddrRptMode = 0, ecpcfunRptMode = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:           /* AT+ECURC=? */
        {
            ret = atcReply(atHandle,
                           AT_RC_OK,
                           0,
                           (CHAR *)"\r\n+ECURC: \"ALL\":(0-1),\"CREG\":(0-1),\"CEREG\":(0-1),\"CEDRXP\":(0-1),"
                           "\"CCIOTOPTI\":(0-1),\"CSCON\":(0-1),\"CTZEU\":(0-1),\"ECCESQ\":(0-1),\"CGEV\":(0-1),"
                           "\"ECPSMR\":(0-1),\"ECPTWEDRXP\":(0-1),\"ECPIN\":(0-1),\"ECPADDR\":(0-1),\"ECPCFUN\":(0-1)\r\n");
            break;
        }

        case AT_SET_REQ:            /* AT+ECURC= */
        {
            /* AT+ECURC=<n>/<typeStr>,<n> */
            /*
             * first param is MIX type (could: dec/string)
            */
            if (pAtCmdReq->pParamList[0].type != AT_STR_VAL)
            {
                if (atGetNumValue(pParamList, 0, &nValue,
                                  ATC_ECURC_1_VAL_MIN, ATC_ECURC_1_VAL_MAX, ATC_ECURC_1_VAL_DEFAULT) == AT_PARA_OK)
                {
                    if (nValue == 0)
                    {
                        gcDisableAllURC(pAtCmdReq->chanId);
                        ret = CMS_RET_SUCC;
                    }
                    else if (nValue == 1)   /*enable*/
                    {
                        gcEnableAllURC(pAtCmdReq->chanId);
                        ret = CMS_RET_SUCC;
                    }
                }
            }
            else //string type
            {
                if (atGetStrValue(pParamList, 0, paraStr,
                                  ATC_ECURC_0_MAX_PARM_STR_LEN, &paraLen, ATC_ECURC_0_MAX_PARM_STR_DEFAULT) == AT_PARA_OK)
                {
                    if (atGetNumValue(pParamList, 1, &nValue,
                                      ATC_ECURC_1_VAL_MIN, ATC_ECURC_1_VAL_MAX, ATC_ECURC_1_VAL_DEFAULT) == AT_PARA_OK)
                    {
                        ret = CMS_RET_SUCC;
                        if (strcasecmp((const CHAR*)paraStr, "all") == 0)
                        {
                            if (nValue == 0)
                            {
                                gcDisableAllURC(pAtCmdReq->chanId);
                            }
                            else if (nValue == 1)   /*enable*/
                            {
                                gcEnableAllURC(pAtCmdReq->chanId);
                            }
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CREG") == 0)
                        {
                            if (nValue == 0)   /* disable */
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CREG_RPT_CFG, 0);
                            }
                            else if (nValue == 1)   /*enable*/
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CREG_RPT_CFG, 3);    //CMI_MM_CREG_LOC_REJ_INFO
                            }
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CEREG") == 0)
                        {
                            if (nValue == 0)   /* disable */
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG, 0);  //CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE
                            }
                            else if (nValue == 1)   /*enable*/
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG, 5);  //CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE
                            }
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CEDRXP") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG, nValue);
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CCIOTOPTI") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG, nValue);
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CSCON") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG, nValue);  //ATC_CSCON_RPT_MODE
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CTZEU") == 0)
                        {
                            if (nValue == 0)   /* disable */
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG, 0);   //ATC_CTZR_RPT_UTC_TIME
                            }
                            else if (nValue == 1)   /*enable*/
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG, 3);   //ATC_CTZR_RPT_UTC_TIME
                            }
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "ECCESQ") == 0)
                        {
                            if (nValue == 0)   /* disable */
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG, 0);    //ATC_ECCESQ_RPT_ECCESQ
                            }
                            else if (nValue == 1)   /*enable*/
                            {
                                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG, 2);    //ATC_ECCESQ_RPT_ECCESQ
                            }
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "CGEV") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG, nValue);    //ATC_CGEREP_FWD_CGEV
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "ECPSMR") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PSM_RPT_CFG, nValue);
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "ECPTWEDRXP") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG, nValue);
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "ECPIN") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG, nValue);
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "ECPADDR") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPADDR_RPT_CFG, nValue);
                        }
                        else if (strcasecmp((const CHAR*)paraStr, "ECPCFUN") == 0)
                        {
                            mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG, nValue);
                        }
                        else
                        {
                            ECOMM_STRING(UNILOG_ATCMD_EXEC, gcECURC_warning_1, P_WARNING,
                                         "AT CMD, ECURC, invalid param URC: %s", paraStr);
                            ret = CMS_INVALID_PARAM;
                        }
                    }
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

        case AT_READ_REQ:    /* AT+ECURC? */
        {

            rspBuf = (CHAR *)OsaAllocZeroMemory(ATEC_IND_RESP_256_STR_LEN);
            OsaDebugBegin(rspBuf != PNULL, ATEC_IND_RESP_256_STR_LEN, 0, 0);
            return ret;
            OsaDebugEnd();

            cregRptMode     = (mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CREG_RPT_CFG) == 3)?1:0;
            ceregRptMode    = (mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG) == 5)?1:0;
            edrxRptMode     = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_EDRX_RPT_CFG);
            ciotRptMode     = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG);
            csconRptMode    = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG);
            ctzrRptMode     = (mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG) == 3)?1:0;
            eccesqRptMode   = (mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECCESQ_RPT_CFG) == 2)?1:0;
            cgerpRptMode    = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG);
            ecpsmrRptMode   = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PSM_RPT_CFG);
            ptwEdrxRptMode  = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG);
            ecpinRptMode    = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG);
            ecpaddrRptMode  = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPADDR_RPT_CFG);
            ecpcfunRptMode  = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG);

            snprintf(rspBuf, ATEC_IND_RESP_256_STR_LEN,
                     "\r\n+ECURC: \"CREG\":%d,\"CEREG\":%d,\"CEDRXP\":%d,\"CCIOTOPTI\":%d,"
                     "\"CSCON\":%d,\"CTZEU\":%d,\"ECCESQ\":%d,\"CGEV\":%d,"
                     "\"ECPSMR\":%d,\"ECPTWEDRXP\":%d,\"ECPIN\":%d,\"ECPADDR\":%d,"
                     "\"ECPCFUN\":%d\r\n",
                     cregRptMode, ceregRptMode, edrxRptMode, ciotRptMode,
                     csconRptMode, ctzrRptMode, eccesqRptMode, cgerpRptMode,
                     ecpsmrRptMode, ptwEdrxRptMode, ecpinRptMode, ecpaddrRptMode,
                     ecpcfunRptMode);

            ret = atcReply(atHandle, AT_RC_OK, 0, rspBuf);
            if (rspBuf != PNULL)
            {
                OsaFreeMemory(&rspBuf);
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

