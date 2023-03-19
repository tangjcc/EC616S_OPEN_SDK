/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_dm.c
*
*  Description: Process device service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "atc_decoder.h"
#include "at_util.h"
#include "ctype.h"
#include "cmicomm.h"
#include "atec_dm.h"
#include "cms_comm.h"
#include "task.h"
#include "queue.h"
#include "mw_config.h"
#include "atc_reply.h"
#include "ec_dm_api.h"
#if defined(FEATURE_CMCC_DM_ENABLE)
#include "cmcc_dm.h"
#endif
extern void mwSetAndSaveAllDMConfig(MidWareDmConfig* config);

/**
  \fn          CmsRetId dmAUTOREG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId dmAUTOREGCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 autoRegEnableFlag = 0xff;
    INT32 autoRegFlag = 0xff;
    INT32 autoRegRang = 0;
    INT32 autoRegLastTime = 0;
    CHAR RspBuf[AUTOREGCFG_STR_LEN] = {0};
    UINT8 paraStr[AUTOREGCFG_0_STR_LEN] = {0};
    MidWareDmConfig *config = NULL;
    CHAR *model = NULL;
    CHAR *swver = NULL;
    CHAR *regAck = NULL;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+AUTOREGCFG=? */
        case AT_READ_REQ:            /* AT+AUTOREGCFG?  */
        {
            autoRegEnableFlag = mwGetEcAutoRegEnableFlag();
            if((autoRegEnableFlag == 1)||(autoRegEnableFlag == 0))
            {
                sprintf((char*)RspBuf, "+AUTOREGCFG: the switch of auto reg is closed");
            }
            else if(autoRegEnableFlag == 2)
            {
                sprintf((char*)RspBuf, "+AUTOREGCFG: the switch of auto reg is opened");
            }
            else
            {
                sprintf((char*)RspBuf, "+AUTOREGCFG: the switch of auto reg is unknown status");
            }
            atcReply(atHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);

            memset(RspBuf, 0, AUTOREGCFG_STR_LEN);
            autoRegEnableFlag = mwGetCuccAutoRegEnableFlag();
            autoRegFlag = mwGetCuccAutoRegFlag();
            autoRegRang = mwGetCuccAutoRegRang();
            autoRegLastTime = mwGetCuccAutoRegLastRegTime();
            if(autoRegRang > 0)
            {
                autoRegRang = autoRegRang/60;
            }

            if((autoRegEnableFlag == 1)||(autoRegEnableFlag == 0))
            {
                if((autoRegFlag == 1)||(autoRegFlag == 0)) /* no register */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CUCC,disable,non register,%d,%d",autoRegRang,autoRegLastTime);
                }
                else if(autoRegFlag == 2)
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CUCC,disable,have register,%d,%d",autoRegRang,autoRegLastTime);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, DM_GET_VAL_ERROR, NULL);
                    break;
                }
            }
            else if(autoRegEnableFlag == 2)
            {
                if((autoRegFlag == 1)||(autoRegFlag == 0)) /* no register */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CUCC,enable,non register,%d,%d",autoRegRang,autoRegLastTime);
                }
                else if(autoRegFlag == 2)
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CUCC,enable,have register,%d,%d",autoRegRang,autoRegLastTime);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, DM_GET_VAL_ERROR, NULL);
                    break;
                }
            }
            atcReply(atHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);

            memset(RspBuf, 0, AUTOREGCFG_STR_LEN);
            autoRegEnableFlag = mwGetCtccAutoRegEnableFlag();
            autoRegFlag = mwGetCtccAutoRegFlag();
            if((autoRegEnableFlag == 1)||(autoRegEnableFlag == 0))
            {
                if((autoRegFlag == 1)||(autoRegFlag == 0)) /* no register */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CTCC,disable,non register");
                }
                else if(autoRegFlag == 2)
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CTCC,disable,have register");
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, DM_GET_VAL_ERROR, NULL);
                    break;
                }
            }
            else if(autoRegEnableFlag == 2)
            {
                if((autoRegFlag == 1)||(autoRegFlag == 0)) /* no register */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CTCC,enable,non register");
                }
                else if(autoRegFlag == 2)
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CTCC,enable,have register");
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, DM_GET_VAL_ERROR, NULL);
                    break;
                }
            }

            atcReply(atHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);

            memset(RspBuf, 0, AUTOREGCFG_STR_LEN);
            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            
            mwGetAllDMConfig(config);
            autoRegEnableFlag = config->mode;
            autoRegFlag = config->test;
            if(autoRegEnableFlag == 0)
            {
                if(autoRegFlag == 0) /* commercial platform */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CMCC,disable,%d,%s,%s,commercial platform", config->lifeTime/60, config->appKey, config->secret);
                }
                else if(autoRegFlag == 1) /* test platform */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CMCC,disable,%d,%s,%s,test platform", config->lifeTime/60, config->appKey, config->secret);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, DM_GET_VAL_ERROR, NULL);
                    free(config);
                    break;
                }
            }
            else if(autoRegEnableFlag == 1)
            {
                if(autoRegFlag == 0) /* commercial platform */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CMCC,enable,%d,%s,%s,commercial platform", config->lifeTime/60, config->appKey, config->secret);
                }
                else if(autoRegFlag == 1) /* test platform */
                {
                    sprintf((char*)RspBuf, "+AUTOREGCFG: CMCC,enable,%d,%s,%s,test platform", config->lifeTime/60, config->appKey, config->secret);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, DM_GET_VAL_ERROR, NULL);
                    free(config);
                    break;
                }
            }
            atcReply(atHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, RspBuf);

            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            free(config);
            break;
        }

        case AT_SET_REQ:              /* AT+AUTOREGCFG= */
        {
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, paraStr, AUTOREGCFG_0_STR_LEN, &length , (CHAR *)AUTOREGCFG_0_STR_DEF)) != AT_PARA_ERR)
            {
                if(strncasecmp((const CHAR*)paraStr, "alldisable", strlen("alldisable")) == 0)
                {
                    mwSetEcAutoRegEnableFlag(1);
                }
                else if(strncasecmp((const CHAR*)paraStr, "allenable", strlen("allenable")) == 0)
                {
                    mwSetEcAutoRegEnableFlag(2);
                }
                else if(strncasecmp((const CHAR*)paraStr, "custinfo", strlen("custinfo")) == 0)
                {
                    model = (CHAR *)mwGetEcAutoRegModel();
                    swver = (CHAR *)mwGetEcAutoRegSwver();
                    regAck = (CHAR *)mwGetEcAutoRegAckPrint();
                    sprintf((char*)RspBuf, "+AUTOREGCFG: %s, %s, %s",model,swver,regAck);
                    ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
                    break;
                }
                else if(strncasecmp((const CHAR*)paraStr, "model", strlen("model")) == 0)
                {
                    memset(paraStr, 0, AUTOREGCFG_0_STR_LEN);
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, paraStr, AUTOREGCFG_1_STR_LEN, &length , (CHAR *)AUTOREGCFG_1_STR_DEF)) != AT_PARA_ERR)
                    {
                        if(paraStr != NULL)
                        {
                            mwSetEcAutoRegModel((UINT8 *)paraStr);
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                            break;
                        }
                    }
                }
                else if(strncasecmp((const CHAR*)paraStr, "swver", strlen("swver")) == 0)
                {
                    memset(paraStr, 0, AUTOREGCFG_0_STR_LEN);
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, paraStr, AUTOREGCFG_1_STR_LEN, &length , (CHAR *)AUTOREGCFG_1_STR_DEF)) != AT_PARA_ERR)
                    {
                        if(paraStr != NULL)
                        {
                            mwSetEcAutoRegSwver((UINT8 *)paraStr);
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                            break;
                        }
                    }
                }
                else if(strncasecmp((const CHAR*)paraStr, "ack", strlen("ack")) == 0)
                {
                    memset(paraStr, 0, AUTOREGCFG_0_STR_LEN);
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, paraStr, AUTOREGCFG_1_STR_LEN, &length , (CHAR *)AUTOREGCFG_1_STR_DEF)) != AT_PARA_ERR)
                    {
                        if(paraStr != NULL)
                        {
                            mwSetEcAutoRegAckPrint((UINT8 *)paraStr);
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                            break;
                        }
                    }
                }
                else if(strncasecmp((const CHAR*)paraStr, "cucc", strlen("cucc")) == 0)
                {
                    memset(paraStr, 0, AUTOREGCFG_0_STR_LEN);
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, paraStr, AUTOREGCFG_1_STR_LEN, &length , (CHAR *)AUTOREGCFG_1_STR_DEF)) != AT_PARA_ERR)
                    {
                        if(strncasecmp((const CHAR*)paraStr, "disable", strlen("disable")) == 0)
                        {
                            autoRegEnableFlag = 1;
                            mwSetCuccAutoRegEnableFlag(autoRegEnableFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "enable", strlen("enable")) == 0)
                        {
                            autoRegEnableFlag = 2;
                            mwSetCuccAutoRegEnableFlag(autoRegEnableFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "cleanFlag", strlen("cleanFlag")) == 0)
                        {
                            autoRegFlag = 1;
                            mwSetCuccAutoRegFlag(autoRegFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "setFlag", strlen("setFlag")) == 0)
                        {
                            autoRegFlag = 2;
                            mwSetCuccAutoRegFlag(autoRegFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "setRang", strlen("setRang")) == 0)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &autoRegRang, AUTOREGCFG_2_MIN, AUTOREGCFG_2_MAX, AUTOREGCFG_2_DEF)) != AT_PARA_ERR)
                            {
                                mwSetCuccAutoRegRang(autoRegRang*60); /*autoRegRang is min, so it transt to secs */
                            }
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "setLastRegTime", strlen("setLastRegTime")) == 0)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &autoRegLastTime, AUTOREGCFG_2_MIN, AUTOREGCFG_2_MAX, AUTOREGCFG_2_DEF)) != AT_PARA_ERR)
                            {
                                mwSetCuccAutoRegLastRegTime(autoRegLastTime);
                            }
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                            break;
                        }
                    }
                }
                else if(strncasecmp((const CHAR*)paraStr, "ctcc", strlen("ctcc")) == 0)
                {
                    memset(paraStr, 0, AUTOREGCFG_0_STR_LEN);
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, paraStr, AUTOREGCFG_1_STR_LEN, &length , (CHAR *)AUTOREGCFG_1_STR_DEF)) != AT_PARA_ERR)
                    {
                        if(strncasecmp((const CHAR*)paraStr, "disable", strlen("disable")) == 0)
                        {
                            autoRegEnableFlag = 1;
                            mwSetCtccAutoRegEnableFlag(autoRegEnableFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "enable", strlen("enable")) == 0)
                        {
                            autoRegEnableFlag = 2;
                            mwSetCtccAutoRegEnableFlag(autoRegEnableFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "cleanFlag", strlen("cleanFlag")) == 0)
                        {
                            autoRegFlag = 1;
                            mwSetCtccAutoRegFlag(autoRegFlag);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "setFlag", strlen("setFlag")) == 0)
                        {
                            autoRegFlag = 2;
                            mwSetCtccAutoRegFlag(autoRegFlag);
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                            break;
                        }
                    }
                }
                else if(strncasecmp((const CHAR*)paraStr, "cmcc", strlen("cmcc")) == 0)
                {
                    config = malloc(sizeof(MidWareDmConfig));
                    memset(config, 0, sizeof(MidWareDmConfig));
                    
                    mwGetAllDMConfig(config);
                    memset(paraStr, 0, AUTOREGCFG_0_STR_LEN);
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, paraStr, AUTOREGCFG_1_STR_LEN, &length , (CHAR *)AUTOREGCFG_1_STR_DEF)) != AT_PARA_ERR)
                    {
                        if(strncasecmp((const CHAR*)paraStr, "disable", strlen("disable")) == 0)
                        {
                            config->mode = 0;
                            mwSetAndSaveAllDMConfig(config);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "enable", strlen("enable")) == 0)
                        {
                            config->mode = 1;
                            mwSetAndSaveAllDMConfig(config);
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "lifeTime", strlen("lifeTime")) == 0)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &autoRegRang, AUTOREGCFG_2_LIFETIME_MIN, AUTOREGCFG_2_LIFETIME_MAX, AUTOREGCFG_2_LIFETIME_DEF)) != AT_PARA_ERR)
                            {
                                config->lifeTime = autoRegRang*60;
                                mwSetAndSaveAllDMConfig(config);
                            }
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "appKey", strlen("appKey")) == 0)
                        {
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)config->appKey, AUTOREGCFG_2_APPKEY_STR_LEN, &length , (CHAR *)AUTOREGCFG_2_APPKEY_STR_DEF)) != AT_PARA_ERR)
                            {
                                mwSetAndSaveAllDMConfig(config);
                            }
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "secret", strlen("secret")) == 0)
                        {
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)config->secret, AUTOREGCFG_2_SECRET_STR_LEN, &length , (CHAR *)AUTOREGCFG_2_SECRET_STR_DEF)) != AT_PARA_ERR)
                            {
                                mwSetAndSaveAllDMConfig(config);
                            }
                        }
                        else if(strncasecmp((const CHAR*)paraStr, "platform", strlen("platform")) == 0)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &autoRegRang, AUTOREGCFG_2_TEST_MIN, AUTOREGCFG_2_TEST_MAX, AUTOREGCFG_2_TEST_DEF)) != AT_PARA_ERR)
                            {
                                config->test = autoRegRang;
                                mwSetAndSaveAllDMConfig(config);
                            }
                        }
                        else
                        {
                            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                            free(config);
                            break;
                        }
                    }
                    free(config);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                    break;
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
                break;
            }
            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId dmDMCONFIG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  dmDMCONFIG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 mode, lifeTime, test;
    MidWareDmConfig *config = NULL;
    CHAR rspBuf[DMCONFIG_CET_RESP_STR_LEN] = {0};
    BOOL validParam = FALSE;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+DMCONFIG? */
        {
            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            mwGetAllDMConfig(config);
            snprintf(rspBuf, DMCONFIG_CET_RESP_STR_LEN, "+DMCONFIG: %d,%d,%s,%s,%d",
                     config->mode, config->lifeTime/60, config->appKey, config->secret, config->test);

            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            free(config);
            break;
        }

        case AT_SET_REQ:         /* AT+DMCONFIG= */
        {
            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            do{
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &mode, DMCONFIG_0_MIN, DMCONFIG_0_MAX, DMCONFIG_0_DEF)) == AT_PARA_ERR)
                {
                    break;
                }

                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &lifeTime, DMCONFIG_1_MIN, DMCONFIG_1_MAX, DMCONFIG_1_DEF)) == AT_PARA_ERR)
                {
                    break;
                }

                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)config->appKey, DMCONFIG_2_STR_LEN, &length , (CHAR *)DMCONFIG_2_STR_DEF)) == AT_PARA_ERR)
                {
                    break;
                }

                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, (UINT8 *)config->secret, DMCONFIG_3_STR_LEN, &length , (CHAR *)DMCONFIG_3_STR_DEF)) == AT_PARA_ERR)
                {
                    break;
                }

                if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &test, DMCONFIG_4_MIN, DMCONFIG_4_MAX, DMCONFIG_4_DEF)) == AT_PARA_ERR)
                {
                    break;
                }
                validParam = TRUE;
            }while(0);

            if(validParam)
            {
                config->mode = mode;
                config->lifeTime= lifeTime*60; //minute to second
                config->test = test;
                mwSetAndSaveAllDMConfig(config);
                ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_PARAM_ERROR), NULL);
            }
            free(config);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

#if defined(FEATURE_CMCC_DM_ENABLE)

/**
  \fn          CmsRetId dmREGSTAT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function.check dm register status
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  dmREGSTAT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT8 stat;
    CHAR rspBuf[16] = {0};
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_EXEC_REQ:         /* AT+DMREGSTAT */
        {
            stat = cmccDmReadRegstat();
            if(stat < 0){
                stat = DM_NO_REG_STATE;
            }
            
            snprintf(rspBuf, 16, "+DMREGSTAT: %d",stat);

            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_DM_ERROR, ( DM_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}
#endif
