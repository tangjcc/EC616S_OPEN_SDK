/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ref_plat.c
*
*  Description: Process plat/device related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifdef  FEATURE_REF_AT_ENABLE
#include "atec_ref_plat.h"
#include "cmicomm.h"
#include "debug_trace.h"
#include "plat_config.h"
#include "os_exception.h"
#include "mw_config.h"
#include "atc_reply.h"
#include "ec_dm_api.h"
#include "ps_lib_api.h"

#if defined(FEATURE_CMCC_DM_ENABLE)
#include "cmcc_dm.h"
#include "at_onenet_task.h"
#endif

struct logLevel_t logLevels[] = {
  { P_DEBUG,   "DEBUG" },
  { P_INFO,    "INFO" },
  { P_VALUE,   "VALUE" },
  { P_SIG,     "SIG" },
  { P_WARNING, "WARNING" },
  { P_ERROR,   "ERROR" },
};

extern uint8_t* atCmdGetBaudRateString(void);
extern bool atCmdIsBaudRateValid(uint32_t baudRate);
extern void atCmdUartChangeConfig(UINT8 atChannel, UINT32 newBaudRate, UINT32 newFrameFormat, CHAR saveFlag);

#define _DEFINE_AT_REQ_FUNCTION_LIST_

/**
  \fn          CmsRetId refNRB(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for trigger system reset
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refNRB(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

	extern void atCmdUartResetChip(UINT32 delay_ms);

    switch (operaType)
    {
        case AT_EXEC_REQ:           /* AT+NRB */
        {
			atUartPrint("\r\nREBOOTING\r\n");
            //ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
			//atUartPrint("\r\n");
            //read_hardfault_flash();
//            EC_SystemReset();
			atCmdUartResetChip(0);
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
  \fn          CmsRetId refNLOGLEVEL(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to config platform related setting
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refNLOGLEVEL(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_RET_SUCC;
    INT32 ret = AT_PARA_OK;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR RspBuf[ATC_NLOGLEVEL_MAX_LEN] = {0};
    UINT8 core[ATC_NLOGLEVEL_CORE_MAX_STR_LEN] = {0};
    UINT8 level[ATC_NLOGLEVEL_LEVEL_MAX_STR_LEN] = {0};
    UINT16  coreLen = 0;
    UINT16  levelLen = 0;

    plat_config_fs_t* fsPlatConfigPtr = NULL;
    plat_config_raw_flash_t* rawFlashPlatConfigPtr = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+NLOGLEVEL=? */
        {
            rc = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+NLOGLEVEL:(APPLICATION),(DEBUG,INFO,VALUE,SIG,WARNING,ERROR)\r\n");
            break;
        }
        case AT_READ_REQ:            /* AT+NLOGLEVEL?  */
        {
            fsPlatConfigPtr = BSP_GetFsPlatConfig();
            rawFlashPlatConfigPtr = BSP_GetRawFlashPlatConfig();

            if(fsPlatConfigPtr && rawFlashPlatConfigPtr)
            {
                snprintf((char*)RspBuf, ATC_NLOGLEVEL_MAX_LEN, "+NLOGLEVEL:APPLICATION,%s\r\n", logLevels[rawFlashPlatConfigPtr->logLevel].name);

                rc = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            }

            break;
        }

        case AT_SET_REQ:              /* AT+NLOGLEVEL= */
        {
            if((ret = atGetStrValue(pParamList, 0, core, ATC_NLOGLEVEL_CORE_MAX_STR_LEN, &coreLen, ATC_NLOGLEVEL_CORE_STR_DEFAULT)) != AT_PARA_ERR)
            {
                if(strncasecmp((const CHAR*)core, "APPLICATION", strlen("APPLICATION")) == 0)
                {
                    if((ret = atGetStrValue(pParamList, 1, level, ATC_NLOGLEVEL_LEVEL_MAX_STR_LEN, &levelLen, ATC_NLOGLEVEL_LEVEL_STR_DEFAULT)) != AT_PARA_ERR)
                    {
                        if(strncasecmp((const CHAR*)level, "DEBUG", strlen("DEBUG")) == 0)
                        {
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, P_DEBUG);
                            uniLogDebugLevelSet((debugTraceLevelType)P_DEBUG);
                        }
                        else if(strncasecmp((const CHAR*)level, "INFO", strlen("INFO")) == 0)
                        {
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, P_INFO);
                            uniLogDebugLevelSet((debugTraceLevelType)P_INFO);
                        }
                        else if(strncasecmp((const CHAR*)level, "VALUE", strlen("VALUE")) == 0)
                        {
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, P_VALUE);
                            uniLogDebugLevelSet((debugTraceLevelType)P_VALUE);
                        }
                        else if(strncasecmp((const CHAR*)level, "SIG", strlen("SIG")) == 0)
                        {
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, P_SIG);
                            uniLogDebugLevelSet((debugTraceLevelType)P_SIG);
                        }
                        else if(strncasecmp((const CHAR*)level, "WARNING", strlen("WARNING")) == 0)
                        {
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, P_WARNING);
                            uniLogDebugLevelSet((debugTraceLevelType)P_WARNING);
                        }
                        else if(strncasecmp((const CHAR*)level, "ERROR", strlen("ERROR")) == 0)
                        {
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, P_ERROR);
                            uniLogDebugLevelSet((debugTraceLevelType)P_ERROR);
                        }
                        else
                        {
                            ret = AT_PARA_ERR;
                        }
                    }
                }
            }

            if(ret == AT_PARA_OK)
            {
                BSP_SavePlatConfigToRawFlash();
                rc = atcReply( atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                rc = atcReply( atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply( atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId refNITZ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to config platform related setting
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refNITZ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_RET_SUCC;
    INT32 ret = AT_PARA_OK;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    //UINT8       atChanId    = AT_GET_HANDLER_CHAN_ID(atHandle);
    INT32       modeVal = 0xff;
    CHAR        respStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    UINT8       atChanId    = AT_GET_HANDLER_CHAN_ID(atHandle);
    BOOL    aonChanged = FALSE;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+NITZ=? */
        {
            rc = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+NITZ:(0,1)");
            break;
        }

        case AT_READ_REQ:           /* AT+NITZ?  */
        {          
            snprintf(respStr, ATEC_IND_RESP_128_STR_LEN, "+NITZ:%d", mwAonGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG));

            rc = atcReply(atHandle, AT_RC_OK, 0, respStr);
            break;
        }

        case AT_SET_REQ:             /* AT+NITZ=  */
        {
            if ((ret = atGetNumValue(pParamList, 0, &modeVal, ATC_NITZ_0_VAL_MIN, ATC_NITZ_0_VAL_MAX, ATC_NITZ_0_VAL_DEFAULT)) == AT_PARA_OK)
            {
                mwAonSetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG, modeVal, &aonChanged);            
            }

            if (ret == AT_PARA_OK)
            {
                rc = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                rc = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        default:
        {
            rc = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}


/**
  \fn          CmsRetId refQLEDMODE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd QLEDMODE, Netlight
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId refQLEDMODE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR RspBuf[16] = {0};

    INT32 ledModeEn = ATC_QLED_MODE_VAL_DEFAULT;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+QLEDMODE=? */
        {
            // Add supported baudrate
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+QLEDMODE: (0,1)");
            break;
        }

        case AT_READ_REQ:            /* AT+QLEDMODE?  */
        {
            // read from middleware nv

            ledModeEn = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECLED_MODE_CFG);

            snprintf((char*)RspBuf, 16, "+QLEDMODE: %d", ledModeEn);

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            break;
        }

        case AT_SET_REQ:              /* AT+QLEDMODE= */
        {
            if(atGetNumValue(pParamList, 0, &ledModeEn,
                             ATC_QLED_MODE_VAL_MIN, ATC_QLED_MODE_VAL_MAX, ATC_QLED_MODE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                extern void TIMER_Netlight_PWM(uint8_t mode);
                if(ledModeEn == ATC_QLED_MODE_VAL_MIN)  
                    TIMER_Netlight_PWM(0);
                
                // write to middleware nv
                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECLED_MODE_CFG, ledModeEn);
                
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+QLEDMODE */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

CmsRetId refNATSPEED(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_RET_SUCC;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    BOOL cmdValid = TRUE;

    INT32 baudRate, timeout, store, syncMode, stopBits, parity, xonxoff;

    atPortFrameFormat_t newFrameFormat = {.wholeValue = 0};

    CHAR RspBuf[ATEC_IND_RESP_128_STR_LEN] = {0};

    plat_config_fs_t* fsPlatConfigPtr = BSP_GetFsPlatConfig();

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+NATSPEED=? */
        {
            snprintf((char*)RspBuf, ATEC_IND_RESP_128_STR_LEN, "+NATSPEED:(%s),(0-30),(0,1),(0-3),(1-2),(0-2),(0,1)", (CHAR *)atCmdGetBaudRateString());
            rc = atcReply(atHandle, AT_RC_OK, 0, RspBuf);
            break;
        }
        case AT_READ_REQ:            /* AT+NATSPEED?  */
        {
            if(fsPlatConfigPtr)
            {
                snprintf((char*)RspBuf, ATEC_IND_RESP_128_STR_LEN, "+NATSPEED:%d,0,%d,%d,0\r\n",
                    (fsPlatConfigPtr->atPortBaudRate & 0x80000000UL) ? 0 : (fsPlatConfigPtr->atPortBaudRate & 0x7FFFFFFFUL),
                    (fsPlatConfigPtr->atPortFrameFormat.config.stopBits == 2) ? 2 : 1,
                    fsPlatConfigPtr->atPortFrameFormat.config.parity);

                rc = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            }

            break;
        }

        case AT_SET_REQ:              /* AT+NATSPEED= */
        {
            if(atGetNumValue(pParamList, 0, &baudRate, ATC_NATSPEED_BAUDRATE_MIN, ATC_NATSPEED_BAUDRATE_MAX, ATC_NATSPEED_BAUDRATE_DEFAULT) != AT_PARA_ERR)
            {
                if(atCmdIsBaudRateValid(baudRate) == false)
                {
                    cmdValid = FALSE;
                }

            }

            if(cmdValid == TRUE)
            {
                if(atGetNumValue(pParamList, 1, &timeout, ATC_NATSPEED_TIMEOUT_MIN, ATC_NATSPEED_TIMEOUT_MAX, ATC_NATSPEED_TIMEOUT_DEFAULT) != AT_PARA_ERR)
                {
                }
                else
                {
                    cmdValid = FALSE;
                }
            }


            if(cmdValid == TRUE)
            {
                if(atGetNumValue(pParamList, 2, &store, ATC_NATSPEED_STORE_MIN, ATC_NATSPEED_STORE_MAX, ATC_NATSPEED_STORE_DEFAULT) != AT_PARA_ERR)
                {
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

            if(cmdValid == TRUE)
            {
                if(atGetNumValue(pParamList, 3, &syncMode, ATC_NATSPEED_SYNCMODE_MIN, ATC_NATSPEED_SYNCMODE_MAX, ATC_NATSPEED_SYNCMODE_DEFAULT) != AT_PARA_ERR)
                {
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

            if(cmdValid == TRUE)
            {
                if(atGetNumValue(pParamList, 4, &stopBits, ATC_NATSPEED_STOPBITS_MIN, ATC_NATSPEED_STOPBITS_MAX, ATC_NATSPEED_STOPBITS_DEFAULT) != AT_PARA_ERR)
                {
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

            if(cmdValid == TRUE)
            {
                if(atGetNumValue(pParamList, 5, &parity, ATC_NATSPEED_PARITY_MIN, ATC_NATSPEED_PARITY_MAX, ATC_NATSPEED_PARITY_DEFAULT) != AT_PARA_ERR)
                {
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

            if(cmdValid == TRUE)
            {
                if(atGetNumValue(pParamList, 6, &xonxoff, ATC_NATSPEED_XONXOFF_MIN, ATC_NATSPEED_XONXOFF_MAX, ATC_NATSPEED_XONXOFF_DEFAULT) != AT_PARA_ERR)
                {
                }
                else
                {
                    cmdValid = FALSE;
                }
            }

            if(cmdValid == TRUE)
            {
                rc = atcReply( atHandle, AT_RC_OK, 0, NULL);

                newFrameFormat.config.stopBits = stopBits;
                newFrameFormat.config.parity = parity;

                atCmdUartChangeConfig(pAtCmdReq->chanId, baudRate, newFrameFormat.wholeValue, 1);
            }
            else
            {
                rc = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply( atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

#if defined(FEATURE_CMCC_DM_ENABLE)

/**
  \fn          CmsRetId refECDMPCONFIG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function.check dm register status
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  refECDMPCONFIG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *rspBuf = NULL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 length = 0;
    UINT16 lengthPort = 0;
    UINT16 lengthPeriod = 0;
    UINT16 lengthIf = 0;
    INT32 type;
    UINT32 DM_mode;
    UINT8 dm_mode[ECDMPCONFIG_1_MODE_STR_LEN] = {0};
    UINT8 *server_port = NULL;
    UINT32 update_Period;
    UINT8 update_period[ECDMPCONFIG_3_PERIOD_STR_LEN] = {0};
    UINT32 if_Type;
    UINT8 if_type[ECDMPCONFIG_3_IF_STR_LEN] = {0};
    UINT32 query_Opt;
    UINT8 query_opt[ECDMPCONFIG_1_QUERY_STR_LEN] = {0};
    UINT32 erase_Opt;
    UINT8 erase_opt[ECDMPCONFIG_1_ERASE_STR_LEN] = {0};
    UINT8 *server_IP = NULL;
    UINT8 *APP_key = NULL;
    UINT8 *passWord = NULL;
    UINT8 *tml_IMEI = NULL;
    UINT8 *PLMN1 = NULL;
    UINT8 *PLMN2 = NULL;
    UINT8 *PLMN3 = NULL;
    MidWareDmConfig *config = NULL;
    INT32 validParam = 0;
    CHAR rspBufTemp[16] = {0};
    INT8 stat = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+DMPCONFIG=? */
            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+DMPCONFIG: <type>[,[<parameter1>][,[<parameter2>][,[<parameter3>[,<parameter4>]]]]");
            break;
            
        case AT_READ_REQ:            /* AT+DMPCONFIG?  */
            rspBuf = malloc(ECDMPCONFIG_RSPBUFF_LEN);
            memset(rspBuf, 0, ECDMPCONFIG_RSPBUFF_LEN);

            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            
            mwGetAllDMConfig(config);
            if((config->serverIP[0] == 0)&&(config->appKey[0] == 0))
            {
                sprintf((char*)rspBuf, "+DMPCONFIG: 0,%d",config->mode);
            }
            else if((config->serverIP[0] != 0)&&(config->appKey[0] != 0))
            {
                sprintf((char*)rspBuf, "+DMPCONFIG: 0,%d\r\n+DMPCONFIG: 1,%s,%s,%d\r\n+DMPCONFIG: 2,%s,%s,%d,%s",config->mode,config->serverIP,config->serverPort,config->lifeTime
                ,config->appKey,config->secret,config->ifType,config->tmlIMEI);
            }
            else
            {
                if((config->serverIP[0] != 0)&&(config->appKey[0] == 0))
                {
                    sprintf((char*)rspBuf, "+DMPCONFIG: 0,%d\r\n+DMPCONFIG: 1,%s,%s,%d\r\n",config->mode,config->serverIP,config->serverPort,config->lifeTime);
                }
                else if((config->serverIP[0] == 0)&&(config->appKey[0] != 0))
                {
                    sprintf((char*)rspBuf, "+DMPCONFIG: 0,%d\r\n+DMPCONFIG: 2,%s,%s,%d,%s",config->mode,config->appKey,config->secret,config->ifType,config->tmlIMEI);
                }
            }

            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)rspBuf);
            free(rspBuf);
            free(config);
            break;
    
        case AT_SET_REQ:             /* AT+DMPCONFIG   */
        {
            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            mwGetAllDMConfig(config);
            
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, ECDMPCONFIG_0_TYPE_MIN, ECDMPCONFIG_0_TYPE_MAX, ECDMPCONFIG_0_TYPE_DEF)) == AT_PARA_OK)
            {
                switch(type)
                {
                    case 0:             /* AT+DMPCONFIG=0,mode   */
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, dm_mode, ECDMPCONFIG_1_MODE_STR_LEN, &length , (CHAR *)ECDMPCONFIG_1_MODE_STR_DEF)) != AT_PARA_ERR)
                        {
                            cmsDecStrToUInt(&DM_mode, (const UINT8 *)dm_mode, length);
                            if(DM_mode <2)
                            {
                                config->mode = DM_mode;
                                mwSetAndSaveAllDMConfig(config);
                                validParam = 1;
                            }
                        }
                        break;
                    }
                        
                    case 1:             /* AT+DMPCONFIG=1,ip,port,period   */
                    {
                        server_IP = malloc(ECDMPCONFIG_1_IP_STR_LEN+1);
                        memset(server_IP, 0, (ECDMPCONFIG_1_IP_STR_LEN+1));
                        server_port = malloc(ECDMPCONFIG_2_PORT_STR_LEN+1);
                        memset(server_port, 0, (ECDMPCONFIG_2_PORT_STR_LEN+1));
                        
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, server_IP, ECDMPCONFIG_1_IP_STR_LEN, &length , (CHAR *)ECDMPCONFIG_1_IP_STR_DEF)) != AT_PARA_ERR)
                        {
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)server_port, ECDMPCONFIG_2_PORT_STR_LEN, &lengthPort , (CHAR *)ECDMPCONFIG_2_PORT_STR_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, (UINT8 *)update_period, ECDMPCONFIG_3_PERIOD_STR_LEN, &lengthPeriod , (CHAR *)ECDMPCONFIG_3_PERIOD_STR_DEF)) != AT_PARA_ERR)
                                {
                                    if(server_IP != NULL)
                                    {
                                        memset(config->serverIP, 0, strlen(config->serverIP));
                                        memcpy(config->serverIP, server_IP, length);
                                    }
                                    if(server_port != NULL)
                                    {
                                        memset(config->serverPort, 0, strlen(config->serverPort));
                                        memcpy(config->serverPort, server_port, lengthPort);
                                    }
                                    cmsDecStrToUInt(&update_Period, (const UINT8 *)update_period, lengthPeriod);
                                    config->lifeTime = update_Period;
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                }
                            }
                        }
                        free(server_IP);
                        free(server_port);
                        break;
                    }
                        
                    case 2:             /* AT+DMPCONFIG=2,key,pwd,imei   */
                    {
                        APP_key = malloc(ECDMPCONFIG_1_KEY_STR_LEN+1);
                        memset(APP_key, 0, (ECDMPCONFIG_1_KEY_STR_LEN+1));
                        passWord = malloc(ECDMPCONFIG_2_PASSWD_STR_LEN+1);
                        memset(passWord, 0, (ECDMPCONFIG_2_PASSWD_STR_LEN+1));
                        tml_IMEI = malloc(ECDMPCONFIG_4_IMEI_STR_LEN+1);
                        memset(tml_IMEI, 0, (ECDMPCONFIG_4_IMEI_STR_LEN+1));
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, APP_key, ECDMPCONFIG_1_KEY_STR_LEN, &length , (CHAR *)ECDMPCONFIG_1_KEY_STR_DEF)) == AT_PARA_OK)
                        {
                            memset(config->appKey, 0, strlen(config->appKey));
                            memcpy(config->appKey, APP_key, length);
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, passWord, ECDMPCONFIG_2_PASSWD_STR_LEN, &length , (CHAR *)ECDMPCONFIG_2_PASSWD_STR_DEF)) == AT_PARA_OK)
                            {
                                memset(config->secret, 0, strlen(config->secret));
                                memcpy(config->secret, passWord, length);
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, if_type, ECDMPCONFIG_3_IF_STR_LEN, &lengthIf , (CHAR *)ECDMPCONFIG_3_IF_STR_DEF)) == AT_PARA_OK)
                                {
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 4, tml_IMEI, ECDMPCONFIG_4_IMEI_STR_LEN, &length , (CHAR *)ECDMPCONFIG_4_IMEI_STR_DEF)) == AT_PARA_OK)
                                    {
                                        memset(config->tmlIMEI, 0, strlen(config->tmlIMEI));
                                        memcpy(config->tmlIMEI, tml_IMEI, length);
                                        cmsDecStrToUInt(&if_Type, (const UINT8 *)if_type, lengthIf);
                                        config->ifType = if_Type;
                                        mwSetAndSaveAllDMConfig(config);
                                        validParam = 1;
                                    }
                                }
                            }
                        }
                        free(APP_key);
                        free(passWord);
                        free(tml_IMEI);
                        break;
                    }

                    case 3:             /* AT+DMPCONFIG=3,plmn,plmn,plmn   */
                    {
                        PLMN1 = malloc(ECDMPCONFIG_1_PLMM1_STR_LEN+1);
                        memset(PLMN1, 0, (ECDMPCONFIG_1_PLMM1_STR_LEN+1));
                        PLMN2 = malloc(ECDMPCONFIG_2_PLMM2_STR_LEN+1);
                        memset(PLMN2, 0, (ECDMPCONFIG_2_PLMM2_STR_LEN+1));
                        PLMN3 = malloc(ECDMPCONFIG_3_PLMM3_STR_LEN+1);
                        memset(PLMN3, 0, (ECDMPCONFIG_3_PLMM3_STR_LEN+1));
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, PLMN1, ECDMPCONFIG_1_PLMM1_STR_LEN, &length , (CHAR *)ECDMPCONFIG_1_PLMM1_STR_DEF)) == AT_PARA_OK)
                        {
                            memset(config->PLMN1, 0, strlen(config->PLMN1));
                            memcpy(config->PLMN1, PLMN1, length);
                            validParam = 1;
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, PLMN2, ECDMPCONFIG_2_PLMM2_STR_LEN, &length , (CHAR *)ECDMPCONFIG_2_PLMM2_STR_DEF)) == AT_PARA_OK)
                            {
                                memset(config->PLMN2, 0, strlen(config->PLMN2));
                                memcpy(config->PLMN2, PLMN2, length);
                                validParam = 1;
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, PLMN3, ECDMPCONFIG_3_PLMM3_STR_LEN, &length , (CHAR *)ECDMPCONFIG_3_PLMM3_STR_DEF)) == AT_PARA_OK)
                                {
                                    memset(config->PLMN3, 0, strlen(config->PLMN3));
                                    memcpy(config->PLMN3, PLMN3, length);
                                    validParam = 1;
                                }
                            }
                        }
                        if(validParam == 1)
                        {
                            mwSetAndSaveAllDMConfig(config);
                        }
                        free(PLMN1);
                        free(PLMN2);
                        free(PLMN3);
                        break;
                    }
                        
                    case 4:             /* AT+DMPCONFIG=4,(0-3)   */
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, query_opt, ECDMPCONFIG_1_QUERY_STR_LEN, &length , (CHAR *)ECDMPCONFIG_1_QUERY_STR_DEF)) == AT_PARA_OK)
                        {
                            cmsDecStrToUInt(&query_Opt, (const UINT8 *)query_opt, length);
                            mwGetAllDMConfig(config);
                            rspBuf = malloc(ECDMPCONFIG_RSPBUFF_LEN);
                            memset(rspBuf, 0, ECDMPCONFIG_RSPBUFF_LEN);
                            switch(query_Opt)
                            {
                                case 0:
                                    sprintf((char*)rspBuf, "%d,%d",query_Opt, config->mode);
                                    validParam = 2;
                                    break;
                                    
                                case 1:
                                    sprintf((char*)rspBuf, "%d,%s,%s,%d",query_Opt, config->serverIP, config->serverPort, config->lifeTime);
                                    validParam = 2;
                                    break;

                                case 2:
                                    sprintf((char*)rspBuf, "%d,%s,%04s***,%d,%s",query_Opt, config->appKey, config->secret, config->ifType, config->tmlIMEI);
                                    validParam = 2;
                                    break;

                                case 3:
                                    if(config->PLMN1[0]!=0)
                                    {
                                        sprintf((char*)rspBuf, "%d,%s",query_Opt, config->PLMN1);
                                    }
                                    if(config->PLMN2[0]!=0)
                                    {
                                        sprintf((char*)rspBufTemp, ",%s",config->PLMN2);
                                        strcat(rspBuf, rspBufTemp);
                                    }   
                                    if(config->PLMN3[0]!=0)
                                    {
                                        memset(rspBufTemp, 0, 16);
                                        sprintf((char*)rspBufTemp, ",%s",config->PLMN3);
                                        strcat(rspBuf, rspBufTemp);
                                    }
                                    validParam = 2;
                                    break;
                            }
                        }
                        break;
                    }
                        
                    case 5:             /* AT+DMPCONFIG=5,(0-4)   */
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, erase_opt, ECDMPCONFIG_1_ERASE_STR_LEN, &length , (CHAR *)ECDMPCONFIG_1_ERASE_STR_DEF)) == AT_PARA_OK)
                        {
                            cmsDecStrToUInt(&erase_Opt, (const UINT8 *)erase_opt, length);
                            
                            mwGetAllDMConfig(config);
                            switch(erase_Opt)
                            {
                                case 0:
                                    config->mode = 0;
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;
                                    
                                case 1:
                                    memset(config->serverIP, 0, MID_WARE_IPV6_ADDR_LEN);
                                    memset(config->serverPort, 0, MID_WARE_DM_PORT_LEN);
                                    config->lifeTime = 0;
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;

                                case 2:
                                    memset(config->appKey, 0, MID_WARE_DM_APPKEY_LEN);
                                    memset(config->secret, 0, MID_WARE_DM_SECRET_LEN);
                                    config->ifType= 0;
                                    memset(config->tmlIMEI, 0, MID_WARE_DM_TML_IMEI_LEN);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;

                                case 3:
                                    memset(config->PLMN1, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN2, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN3, 0, MID_WARE_DM_PLMN_LEN);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;

                                case 4:
                                    config->mode = 0;
                                    memset(config->serverIP, 0, MID_WARE_IPV6_ADDR_LEN);
                                    memset(config->serverPort, 0, MID_WARE_DM_PORT_LEN);
                                    config->lifeTime = 0;
                                    memset(config->appKey, 0, MID_WARE_DM_APPKEY_LEN);
                                    memset(config->secret, 0, MID_WARE_DM_SECRET_LEN);
                                    config->ifType= 0;
                                    memset(config->tmlIMEI, 0, MID_WARE_DM_TML_IMEI_LEN);
                                    memset(config->PLMN1, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN2, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN3, 0, MID_WARE_DM_PLMN_LEN);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;                             
                                    break;
                            }                           
                        }
                        break;
                    }

                    case 6:             /* AT+DMPCONFIG=6   */
                    {       
                        rspBuf = malloc(16);
                        memset(rspBuf, 0, 16);
                        stat = cmccDmReadRegstat();
                        sprintf((char*)rspBuf, "6,%d",stat);
                        validParam = 2;
                        break;
                    }
                }
            }
            if(validParam == 1)
            {
                ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else if(validParam == 2)
            {
                ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
                free(rspBuf);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_DM_ERROR, (DM_NOT_SUPPORT), NULL);
            }
            free(config);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_DM_ERROR, (DM_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId refECDMPCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function.check dm register status
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  refECDMPCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *rspBuf = NULL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 length = 0;
    UINT16 lengthPort = 0;
    UINT16 lengthPeriod = 0;
    UINT16 lengthIf = 0;
    INT32 type;
    UINT32 DM_mode;
    UINT8 dm_mode[ECDMPCFG_1_MODE_STR_LEN] = {0};
    UINT8 *server_port = NULL;
    UINT32 update_Period;
    UINT8 update_period[ECDMPCFG_3_PERIOD_STR_LEN] = {0};
    UINT32 if_Type;
    UINT8 if_type[ECDMPCFG_3_IF_STR_LEN] = {0};
    UINT32 query_Opt;
    UINT8 query_opt[ECDMPCFG_1_QUERY_STR_LEN] = {0};
    UINT32 erase_Opt;
    UINT8 erase_opt[ECDMPCFG_1_ERASE_STR_LEN] = {0};
    UINT8 *server_IP = NULL;
    UINT8 *APP_key = NULL;
    UINT8 *passWord = NULL;
    UINT8 *tml_IMEI = NULL;
    UINT8 *PLMN1 = NULL;
    UINT8 *PLMN2 = NULL;
    UINT8 *PLMN3 = NULL;
    MidWareDmConfig *config = NULL;
    INT32 validParam = 0;
    CHAR rspBufTemp[16] = {0};
    INT8 stat = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+DMPCFG=? */
            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+DMPCFG: <type>[,[<parameter1>][,[<parameter2>][,[<parameter3>[,<parameter4>]]]]");
            break;
            
        case AT_READ_REQ:            /* AT+DMPCFG?  */
            rspBuf = malloc(ECDMPCFG_RSPBUFF_LEN);
            memset(rspBuf, 0, ECDMPCFG_RSPBUFF_LEN);

            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            
            mwGetAllDMConfig(config);
            if((config->serverIP[0] == 0)&&(config->appKey[0] == 0))
            {
                sprintf((char*)rspBuf, "+DMPCFG: 0,%d",config->mode);
            }
            else if((config->serverIP[0] != 0)&&(config->appKey[0] != 0))
            {
                sprintf((char*)rspBuf, "+DMPCFG: 0,%d\r\n+DMPCFG: 1,%s,%s,%d\r\n+DMPCFG: 2,%s,%s,%d,%s",config->mode,config->serverIP,config->serverPort,config->lifeTime
                ,config->appKey,config->secret,config->ifType,config->tmlIMEI);
            }
            else
            {
                if((config->serverIP[0] != 0)&&(config->appKey[0] == 0))
                {
                    sprintf((char*)rspBuf, "+DMPCFG: 0,%d\r\n+DMPCFG: 1,%s,%s,%d\r\n",config->mode,config->serverIP,config->serverPort,config->lifeTime);
                }
                else if((config->serverIP[0] == 0)&&(config->appKey[0] != 0))
                {
                    sprintf((char*)rspBuf, "+DMPCFG: 0,%d\r\n+DMPCFG: 2,%s,%s,%d,%s",config->mode,config->appKey,config->secret,config->ifType,config->tmlIMEI);
                }
            }

            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)rspBuf);
            free(rspBuf);
            free(config);
            break;
    
        case AT_SET_REQ:             /* AT+DMPCFG   */
        {
            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            mwGetAllDMConfig(config);
            
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, ECDMPCFG_0_TYPE_MIN, ECDMPCFG_0_TYPE_MAX, ECDMPCFG_0_TYPE_DEF)) == AT_PARA_OK)
            {
                switch(type)
                {
                    case 0:             /* AT+DMPCFG=0,mode   */
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, dm_mode, ECDMPCFG_1_MODE_STR_LEN, &length , (CHAR *)ECDMPCFG_1_MODE_STR_DEF)) != AT_PARA_ERR)
                        {
                            cmsDecStrToUInt(&DM_mode, (const UINT8 *)dm_mode, length);
                            config->mode = DM_mode;
                            mwSetAndSaveAllDMConfig(config);
                            validParam = 1;
                        }
                        break;
                    }
                        
                    case 1:             /* AT+DMPCFG=1,ip,port,period   */
                    {
                        server_IP = malloc(ECDMPCFG_1_IP_STR_LEN+1);
                        memset(server_IP, 0, (ECDMPCFG_1_IP_STR_LEN+1));
                        server_port = malloc(ECDMPCFG_2_PORT_STR_LEN+1);
                        memset(server_port, 0, (ECDMPCFG_2_PORT_STR_LEN+1));
                        
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, server_IP, ECDMPCFG_1_IP_STR_LEN, &length , (CHAR *)ECDMPCFG_1_IP_STR_DEF)) != AT_PARA_ERR)
                        {
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)server_port, ECDMPCFG_2_PORT_STR_LEN, &lengthPort , (CHAR *)ECDMPCFG_2_PORT_STR_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, (UINT8 *)update_period, ECDMPCFG_3_PERIOD_STR_LEN, &lengthPeriod , (CHAR *)ECDMPCFG_3_PERIOD_STR_DEF)) != AT_PARA_ERR)
                                {
                                    if(server_IP != NULL)
                                    {
                                        memset(config->serverIP, 0, strlen(config->serverIP));
                                        memcpy(config->serverIP, server_IP, length);
                                    }
                                    if(server_port != NULL)
                                    {
                                        memset(config->serverPort, 0, strlen(config->serverPort));
                                        memcpy(config->serverPort, server_port, lengthPort);
                                    }
                                    cmsDecStrToUInt(&update_Period, (const UINT8 *)update_period, lengthPeriod);
                                    config->lifeTime = update_Period;
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                }
                            }
                        }
                        free(server_IP);
                        free(server_port);
                        break;
                    }
                        
                    case 2:             /* AT+DMPCFG=2,key,pwd,imei   */
                    {
                        APP_key = malloc(ECDMPCFG_1_KEY_STR_LEN+1);
                        memset(APP_key, 0, (ECDMPCFG_1_KEY_STR_LEN+1));
                        passWord = malloc(ECDMPCFG_2_PASSWD_STR_LEN+1);
                        memset(passWord, 0, (ECDMPCFG_2_PASSWD_STR_LEN+1));
                        tml_IMEI = malloc(ECDMPCFG_4_IMEI_STR_LEN+1);
                        memset(tml_IMEI, 0, (ECDMPCFG_4_IMEI_STR_LEN+1));
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, APP_key, ECDMPCFG_1_KEY_STR_LEN, &length , (CHAR *)ECDMPCFG_1_KEY_STR_DEF)) == AT_PARA_OK)
                        {
                            memset(config->appKey, 0, strlen(config->appKey));
                            memcpy(config->appKey, APP_key, length);
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, passWord, ECDMPCFG_2_PASSWD_STR_LEN, &length , (CHAR *)ECDMPCFG_2_PASSWD_STR_DEF)) == AT_PARA_OK)
                            {
                                memset(config->secret, 0, strlen(config->secret));
                                memcpy(config->secret, passWord, length);
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, if_type, ECDMPCFG_3_IF_STR_LEN, &lengthIf , (CHAR *)ECDMPCFG_3_IF_STR_DEF)) == AT_PARA_OK)
                                {
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 4, tml_IMEI, ECDMPCFG_4_IMEI_STR_LEN, &length , (CHAR *)ECDMPCFG_4_IMEI_STR_DEF)) == AT_PARA_OK)
                                    {
                                        memset(config->tmlIMEI, 0, strlen(config->tmlIMEI));
                                        memcpy(config->tmlIMEI, tml_IMEI, length);
                                        cmsDecStrToUInt(&if_Type, (const UINT8 *)if_type, lengthIf);
                                        config->ifType = if_Type;
                                        mwSetAndSaveAllDMConfig(config);
                                        validParam = 1;
                                    }
                                }
                            }
                        }
                        free(APP_key);
                        free(passWord);
                        free(tml_IMEI);
                        break;
                    }

                    case 3:             /* AT+DMPCFG=3,plmn,plmn,plmn   */
                    {
                        PLMN1 = malloc(ECDMPCFG_1_PLMM1_STR_LEN+1);
                        memset(PLMN1, 0, (ECDMPCFG_1_PLMM1_STR_LEN+1));
                        PLMN2 = malloc(ECDMPCFG_2_PLMM2_STR_LEN+1);
                        memset(PLMN2, 0, (ECDMPCFG_2_PLMM2_STR_LEN+1));
                        PLMN3 = malloc(ECDMPCFG_3_PLMM3_STR_LEN+1);
                        memset(PLMN3, 0, (ECDMPCFG_3_PLMM3_STR_LEN+1));
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, PLMN1, ECDMPCFG_1_PLMM1_STR_LEN, &length , (CHAR *)ECDMPCFG_1_PLMM1_STR_DEF)) == AT_PARA_OK)
                        {
                            memset(config->PLMN1, 0, strlen(config->PLMN1));
                            memcpy(config->PLMN1, PLMN1, length);
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, PLMN2, ECDMPCFG_2_PLMM2_STR_LEN, &length , (CHAR *)ECDMPCFG_2_PLMM2_STR_DEF)) == AT_PARA_OK)
                            {
                                memset(config->PLMN2, 0, strlen(config->PLMN2));
                                memcpy(config->PLMN2, PLMN2, length);
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, PLMN3, ECDMPCFG_3_PLMM3_STR_LEN, &length , (CHAR *)ECDMPCFG_3_PLMM3_STR_DEF)) == AT_PARA_OK)
                                {
                                    memset(config->PLMN3, 0, strlen(config->PLMN3));
                                    memcpy(config->PLMN3, PLMN3, length);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                }
                            }
                        }
                        free(PLMN1);
                        free(PLMN2);
                        free(PLMN3);
                        break;
                    }
                        
                    case 4:             /* AT+DMPCFG=4,(0-3)   */
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, query_opt, ECDMPCFG_1_QUERY_STR_LEN, &length , (CHAR *)ECDMPCFG_1_QUERY_STR_DEF)) == AT_PARA_OK)
                        {
                            cmsDecStrToUInt(&query_Opt, (const UINT8 *)query_opt, length);
                            mwGetAllDMConfig(config);
                            rspBuf = malloc(ECDMPCFG_RSPBUFF_LEN);
                            memset(rspBuf, 0, ECDMPCFG_RSPBUFF_LEN);
                            switch(query_Opt)
                            {
                                case 0:
                                    sprintf((char*)rspBuf, "%d,%d",query_Opt, config->mode);
                                    validParam = 2;
                                    break;
                                    
                                case 1:
                                    sprintf((char*)rspBuf, "%d,%s,%s,%d",query_Opt, config->serverIP, config->serverPort, config->lifeTime);
                                    validParam = 2;
                                    break;

                                case 2:
                                    sprintf((char*)rspBuf, "%d,%s,%04s***,%d,%s",query_Opt, config->appKey, config->secret, config->ifType, config->tmlIMEI);
                                    validParam = 2;
                                    break;

                                case 3:
                                    if(config->PLMN1[0]!=0)
                                    {
                                        sprintf((char*)rspBuf, "%d,%s",query_Opt, config->PLMN1);
                                    }
                                    if(config->PLMN2[0]!=0)
                                    {
                                        sprintf((char*)rspBufTemp, ",%s",config->PLMN2);
                                        strcat(rspBuf, rspBufTemp);
                                    }   
                                    if(config->PLMN3[0]!=0)
                                    {
                                        memset(rspBufTemp, 0, 16);
                                        sprintf((char*)rspBufTemp, ",%s",config->PLMN3);
                                        strcat(rspBuf, rspBufTemp);
                                    }
                                    validParam = 2;
                                    break;
                            }
                        }
                        break;
                    }
                        
                    case 5:             /* AT+DMPCFG=5,(0-4)   */
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, erase_opt, ECDMPCFG_1_ERASE_STR_LEN, &length , (CHAR *)ECDMPCFG_1_ERASE_STR_DEF)) == AT_PARA_OK)
                        {
                            cmsDecStrToUInt(&erase_Opt, (const UINT8 *)erase_opt, length);
                            
                            mwGetAllDMConfig(config);
                            switch(erase_Opt)
                            {
                                case 0:
                                    config->mode = 0;
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;
                                    
                                case 1:
                                    memset(config->serverIP, 0, MID_WARE_IPV6_ADDR_LEN);
                                    memset(config->serverPort, 0, MID_WARE_DM_PORT_LEN);
                                    config->lifeTime = 0;
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;

                                case 2:
                                    memset(config->appKey, 0, MID_WARE_DM_APPKEY_LEN);
                                    memset(config->secret, 0, MID_WARE_DM_SECRET_LEN);
                                    config->ifType= 0;
                                    memset(config->tmlIMEI, 0, MID_WARE_DM_TML_IMEI_LEN);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;

                                case 3:
                                    memset(config->PLMN1, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN2, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN3, 0, MID_WARE_DM_PLMN_LEN);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                    break;

                                case 4:
                                    config->mode = 0;
                                    memset(config->serverIP, 0, MID_WARE_IPV6_ADDR_LEN);
                                    memset(config->serverPort, 0, MID_WARE_DM_PORT_LEN);
                                    config->lifeTime = 0;
                                    memset(config->appKey, 0, MID_WARE_DM_APPKEY_LEN);
                                    memset(config->secret, 0, MID_WARE_DM_SECRET_LEN);
                                    config->ifType= 0;
                                    memset(config->tmlIMEI, 0, MID_WARE_DM_TML_IMEI_LEN);
                                    memset(config->PLMN1, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN2, 0, MID_WARE_DM_PLMN_LEN);
                                    memset(config->PLMN3, 0, MID_WARE_DM_PLMN_LEN);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;                             
                                    break;
                            }                           
                        }
                        break;
                    }

                    case 6:             /* AT+DMPCFG=6   */
                    {
                        rspBuf = malloc(16);
                        memset(rspBuf, 0, 16);
                        stat = cmccDmReadRegstat();
                        sprintf((char*)rspBuf, "6,%d",stat);
                        validParam = 2;
                        break;
                    }
                }
            }
            if(validParam == 1)
            {
                ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else if(validParam == 2)
            {
                ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
                free(rspBuf);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_DM_ERROR, (DM_NOT_SUPPORT), NULL);
            }
            free(config);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_DM_ERROR, (DM_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId refECDMPCFGEX(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function.check dm register status
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  refECDMPCFGEX(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *rspBuf = NULL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 length = 0;
    INT32 type;
    INT32 act;
    //UINT8 *app_info = NULL;
    //UINT8 *MAC = NULL;
    //UINT8 *ROM = NULL;
    //UINT8 *RAM = NULL;
    //UINT8 *CPU = NULL;
    //UINT8 *osSysVer = NULL;
    //UINT8 *swVer = NULL;
    //UINT8 *swName = NULL;
    UINT8 *VoLTE = NULL;
    UINT8 *net_type = NULL;
    UINT8 *account = NULL;
    UINT8 *phoneNum = NULL;
    //UINT8 *location = NULL; 
    MidWareDmConfig *config = NULL;
    INT32 validParam = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+DMPCFGEX=? */
            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+DMPCFGEX: <type>,<act>[,[<parameter1>][,[<parameter2>][,[<parameter3>[,<parameter4>]]]]");
            break;
    
        case AT_SET_REQ:             /* AT+DMPCFGEX   */
        {
            config = malloc(sizeof(MidWareDmConfig));
            memset(config, 0, sizeof(MidWareDmConfig));
            mwGetAllDMConfig(config);
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, ECDMPCFGEX_0_TYPE_MIN, ECDMPCFGEX_0_TYPE_MAX, ECDMPCFGEX_0_TYPE_DEF)) == AT_PARA_OK)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &act, ECDMPCFGEX_1_ACT_MIN, ECDMPCFGEX_1_ACT_MAX, ECDMPCFGEX_1_ACT_DEF)) == AT_PARA_OK)
                {
                    if(act == 1)/*read info*/
                    {
                        rspBuf = malloc(ECDMPCFGEX_RSPBUFF_LEN+18);
                        memset(rspBuf, 0, ECDMPCFGEX_RSPBUFF_LEN+18);
                        switch(type)
                        {   
                            case 0:      /* AT+DMPCFGEX=0,1   */
                                sprintf((char*)rspBuf, "+DMPCFGEX: %d,NULL",type);
                                break;

                            case 1:      /* AT+DMPCFGEX=1,1   */
                                sprintf((char*)rspBuf, "+DMPCFGEX: %d,NULL",type);
                                break;
                                
                            case 2:      /* AT+DMPCFGEX=2,1   */
                                //sprintf((char*)rspBuf, "+ECDMPCFGEX: %d,%s,%s,%s,%s",type, config->ROM, config->RAM, config->CPU, config->osSysVer);
                                sprintf((char*)rspBuf, "+DMPCFGEX: %d,%s,%s,%s,%s",type, "4M", "256K", "Eigencomm", "freeRtos");
                                break;
                                
                            case 3:      /* AT+DMPCFGEX=3,1   */
                                //sprintf((char*)rspBuf, "+ECDMPCFGEX: %d,%s,%s",type, config->swVer, config->swName);
                                sprintf((char*)rspBuf, "+DMPCFGEX: %d,%s,%s",type, "V1.0", "EC");
                                break;
                                
                            case 4:      /* AT+DMPCFGEX=4,1   */
                                //sprintf((char*)rspBuf, "+ECDMPCFGEX: %d,%s,%s,%s,%s",type, config->VoLTE, config->netType, config->account, config->phoneNum);
                                sprintf((char*)rspBuf, "+DMPCFGEX: %d,%s,%s,%s,%s",type, "off", "NB-IOT", config->account, config->phoneNum);
                                break;

                            case 5:      /* AT+DMPCFGEX=5,1   */
                                sprintf((char*)rspBuf, "+DMPCFGEX: %d,NULL",type);
                                break;
                        }
                        ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)rspBuf);
                        free(rspBuf);
                        free(config);
                        break;
                    }
                    else if(act == 2)/*clean info*/
                    {
                        switch(type)
                        {   
                            case 0:      /* AT+DMPCFGEX=0,2   */
                                //memset(config->appInfo, 0, MID_WARE_DM_APP_INFO_LEN);
                                break;

                            case 1:      /* AT+DMPCFGEX=1,2   */
                                //memset(config->MAC, 0, MID_WARE_DM_MAC_LEN);
                                break;
                                
                            case 2:      /* AT+DMPCFGEX=2,2   */
                                //memset(config->ROM, 0, MID_WARE_DM_ROM_LEN);
                                //memset(config->RAM, 0, MID_WARE_DM_RAM_LEN);
                                //memset(config->CPU, 0, MID_WARE_DM_CPU_LEN);
                                //memset(config->osSysVer, 0, MID_WARE_DM_OSSYSVER_LEN);
                                //validParam = 1;
                                break;
                                
                            case 3:      /* AT+DMPCFGEX=3,2   */
                                //memset(config->swVer, 0, MID_WARE_DM_SWVER_LEN);
                                //memset(config->swName, 0, MID_WARE_DM_SWNAME_LEN);
                                //validParam = 1;
                                break;
                                
                            case 4:      /* AT+DMPCFGEX=4,2   */
                                //memset(config->VoLTE, 0, MID_WARE_DM_VOLTE_LEN);
                                //memset(config->netType, 0, MID_WARE_DM_NET_TYPE_LEN);
                                memset(config->account, 0, MID_WARE_DM_ACCOUNT_LEN);
                                memset(config->phoneNum, 0, MID_WARE_DM_PHONE_NUM_LEN);
                                validParam = 1;
                                break;

                            case 5:      /* AT+DMPCFGEX=5,2   */
                                //memset(config->location, 0, MID_WARE_DM_LOCATION_LEN);
                                break;
                        }
                        if(validParam == 0)
                        {
                            ret = atcReply(atHandle, AT_RC_ERROR, DM_NOT_SUPPORT, NULL);
                        }
                        else
                        {
                            mwSetAndSaveAllDMConfig(config);
                            ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                        }
                        free(config);
                        break;
                    }
                    else/*act == 0, set info*/
                    {
                        switch(type)
                        {
                            case 0:
                            {
                                /*
                                app_info = malloc(ECDMPCFGEX_2_APP_STR_LEN+1);
                                memset(app_info, 0, (ECDMPCFGEX_2_APP_STR_LEN+1));
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, app_info, ECDMPCFGEX_2_APP_STR_LEN, &length , (CHAR *)ECDMPCFGEX_2_APP_STR_DEF)) == AT_PARA_OK)
                                {
                                    memcpy(config->appInfo, app_info, length);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                }
                                free(app_info);
                                */
                                break;
                            }
                                
                            case 1:
                            {
                                /*
                                MAC = malloc(ECDMPCFGEX_2_MAC_STR_LEN+1);
                                memset(MAC, 0, (ECDMPCFGEX_2_MAC_STR_LEN+1));
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, MAC, ECDMPCFGEX_2_MAC_STR_LEN, &length , (CHAR *)ECDMPCFGEX_2_MAC_STR_DEF)) == AT_PARA_OK)
                                {
                                    memcpy(config->MAC, MAC, length);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                }
                                free(MAC);
                                */
                                break;
                            }
                                
                            case 2:
                            {
                                /*
                                ROM = malloc(ECDMPCFGEX_2_ROM_STR_LEN+1);
                                memset(ROM, 0, (ECDMPCFGEX_2_ROM_STR_LEN+1));
                                RAM = malloc(ECDMPCFGEX_3_RAM_STR_LEN+1);
                                memset(RAM, 0, (ECDMPCFGEX_3_RAM_STR_LEN+1));
                                CPU = malloc(ECDMPCFGEX_4_CPU_STR_LEN+1);
                                memset(CPU, 0, (ECDMPCFGEX_4_CPU_STR_LEN+1));
                                osSysVer = malloc(ECDMPCFGEX_5_OSSYSVER_STR_LEN+1);
                                memset(osSysVer, 0, (ECDMPCFGEX_5_OSSYSVER_STR_LEN+1));
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, ROM, ECDMPCFGEX_2_ROM_STR_LEN, &length , (CHAR *)ECDMPCFGEX_2_ROM_STR_DEF)) == AT_PARA_OK)
                                {
                                    memcpy(config->ROM, ROM, length);
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, RAM, ECDMPCFGEX_3_RAM_STR_LEN, &length , (CHAR *)ECDMPCFGEX_3_RAM_STR_DEF)) == AT_PARA_OK)
                                    {
                                        memcpy(config->RAM, RAM, length);
                                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 5, CPU, ECDMPCFGEX_4_CPU_STR_LEN, &length , (CHAR *)ECDMPCFGEX_4_CPU_STR_DEF)) == AT_PARA_OK)
                                        {
                                            memcpy(config->CPU, CPU, length);
                                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 5, osSysVer, ECDMPCFGEX_5_OSSYSVER_STR_LEN, &length , (CHAR *)ECDMPCFGEX_5_OSSYSVER_STR_DEF)) == AT_PARA_OK)
                                            {
                                                memcpy(config->osSysVer, osSysVer, length);
                                                mwSetAndSaveAllDMConfig(config);
                                                validParam = 1;
                                            }
                                        }
                                    }
                                }
                                free(ROM);
                                free(RAM);
                                free(CPU);
                                free(osSysVer);
                                */
                                break;
                            }
                        
                            case 3:
                            {
                                /*
                                swVer = malloc(ECDMPCFGEX_2_SWVER_STR_LEN+1);
                                memset(swVer, 0, (ECDMPCFGEX_2_SWVER_STR_LEN+1));
                                swName = malloc(ECDMPCFGEX_3_SWNAME_STR_LEN+1);
                                memset(swName, 0, (ECDMPCFGEX_3_SWNAME_STR_LEN+1));
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, swVer, ECDMPCFGEX_2_SWVER_STR_LEN, &length , (CHAR *)ECDMPCFGEX_2_SWVER_STR_DEF)) == AT_PARA_OK)
                                {
                                    memcpy(config->swVer, swVer, length);
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, swName, ECDMPCFGEX_3_SWNAME_STR_LEN, &length , (CHAR *)ECDMPCFGEX_3_SWNAME_STR_DEF)) == AT_PARA_OK)
                                    {
                                        memcpy(config->swName, swName, length);
                                        mwSetAndSaveAllDMConfig(config);
                                        validParam = 1;
                                    }
                                }
                                free(swVer);
                                free(swName);
                                */
                                break;
                            }
                                
                            case 4:
                            {
                                VoLTE = malloc(ECDMPCFGEX_2_VOLTE_STR_LEN+1);
                                memset(VoLTE, 0, (ECDMPCFGEX_2_VOLTE_STR_LEN+1));
                                net_type = malloc(ECDMPCFGEX_3_NET_STR_LEN+1);
                                memset(net_type, 0, (ECDMPCFGEX_3_NET_STR_LEN+1));
                                account = malloc(ECDMPCFGEX_4_ACCOUNT_STR_LEN+1);
                                memset(account, 0, (ECDMPCFGEX_4_ACCOUNT_STR_LEN+1));
                                phoneNum = malloc(ECDMPCFGEX_5_PHONENUM_STR_LEN+1);
                                memset(phoneNum, 0, (ECDMPCFGEX_5_PHONENUM_STR_LEN+1));
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, VoLTE, ECDMPCFGEX_2_VOLTE_STR_LEN, &length , (CHAR *)ECDMPCFGEX_2_VOLTE_STR_DEF)) == AT_PARA_OK)
                                {
                                    //memcpy(config->VoLTE, VoLTE, length);
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, net_type, ECDMPCFGEX_3_NET_STR_LEN, &length , (CHAR *)ECDMPCFGEX_3_NET_STR_DEF)) == AT_PARA_OK)
                                    {
                                        //memcpy(config->netType, net_type, length);
                                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 4, account, ECDMPCFGEX_4_ACCOUNT_STR_LEN, &length , (CHAR *)ECDMPCFGEX_4_ACCOUNT_STR_DEF)) == AT_PARA_OK)
                                        {
                                            memset(config->account, 0, strlen(config->account));
                                            memcpy(config->account, account, length);
                                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 5, phoneNum, ECDMPCFGEX_5_PHONENUM_STR_LEN, &length , (CHAR *)ECDMPCFGEX_5_PHONENUM_STR_DEF)) == AT_PARA_OK)
                                            {
                                                memset(config->phoneNum, 0, strlen(config->phoneNum));
                                                memcpy(config->phoneNum, phoneNum, length);
                                                mwSetAndSaveAllDMConfig(config);
                                                validParam = 1;
                                            }
                                        }
                                    }
                                }
                                free(VoLTE);
                                free(net_type);
                                free(account);
                                free(phoneNum);
                                break;
                            }
                                
                            case 5:
                            {
                                /*
                                location = malloc(ECDMPCFGEX_2_LOCATION_STR_LEN+1);
                                memset(location, 0, (ECDMPCFGEX_2_LOCATION_STR_LEN+1));
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, location, ECDMPCFGEX_2_LOCATION_STR_LEN, &length , (CHAR *)ECDMPCFGEX_2_LOCATION_STR_DEF)) == AT_PARA_OK)
                                {
                                    memcpy(config->location, location, length);
                                    mwSetAndSaveAllDMConfig(config);
                                    validParam = 1;
                                }
                                free(location);
                                */
                                break;
                            }
                        }
                    }
                }
            }
            if(validParam == 1)
            {
                ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_DM_ERROR, (DM_NOT_SUPPORT), NULL);
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

#endif


#endif

