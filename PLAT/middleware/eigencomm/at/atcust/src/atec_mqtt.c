/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_dev.c
*
*  Description: Process cloud related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "cmimm.h"
#include "at_util.h"
#include "at_mqtt_task.h"
#include "atec_mqtt.h"
#include "atec_mqtt_cnf_ind.h"
#include "ec_mqtt_api.h"
//extern ATCAtpCtrl atcmd_ctrl[1];

mqtt_pub_data mqttPubTempData;
CHAR *mqttInputDataPtr = NULL;
INT32 mqttDataTotalLength = 0;

#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId mqttCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 mqttCfg[MQTTCFG_CFG_MAX_LEN+1] = {0};    
    INT32 tcpId;
    INT32 echomode;
    INT32 sendFormat;
    INT32 recvFormat;
    INT32 keepAliveTime;
    INT32 pktTimeout;
    INT32 retryTimeout;
    INT32 timeoutNotice;
    INT32 cleanSession;
    INT32 willFlag;
    INT32 willOptionQos;
    INT32 willOptionRetained;
    UINT8 willOptionTopic[MQTTCFG_5_WILLTOPIC_MAX_LEN+1] = {0};
    UINT8 willOptionMessage[MQTTCFG_6_WILLMSG_MAX_LEN+1] = {0};
    INT32 version;
    CHAR *aliProductKey = NULL;  
    CHAR *aliDeviceName = NULL;  
    CHAR *aliDeviceSecret = NULL;  
    CHAR *aliProductName = NULL;       
    CHAR *aliProductSecret = NULL;    
    CHAR *aliAuthType = NULL;    
    CHAR *aliSignMethod = NULL;    
    CHAR *aliAuthMode = NULL;       
    CHAR *aliSecureMode = NULL;       
    CHAR *instanceId = NULL;       
    INT32 aliDynRegUsed = 0;    
    UINT8 sslType[MQTTCFG_1_SSL_MAX_LEN+1] = {0};
    INT32 cloudType;
    INT32 payloadType;    
    mqtt_cfg_data cfgData;
    UINT16 length = 0;
    mqtt_context *mqttNewContext = NULL;
    CHAR atRspBuf[160] = {0};
    CHAR *atRspBufTemp = NULL;
    CHAR *eccKey = NULL;
    CHAR *caKey = NULL;
    CHAR *pskKey = NULL;
    CHAR *pskIdentity = NULL;
    CHAR *caKeyTemp = NULL;
    CHAR *hostName = NULL;

    memset(&cfgData, 0, sizeof(cfgData));

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECMTCFG=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTCFG:\"keepalive\",(0),(0-3600),\r\n+ECMTCFG:\"session\",(0),(0,1)\r\n+ECMTCFG:\"timeout\",(0),(1-60),(1-10),(0,1)\r\n+ECMTCFG:\"will\",(0),(0,1),(0-2),(0,1),\"will_topic\",\"will_msg\"\r\n+ECMTCFG:\"version\",(0),(3,4)\r\n+ECMTCFG:\"aliauth\",(0),\"productkey\",\"devicename\",\"devicesecret\"\r\n+ECMTCFG:\"cloud\",(0-255),(0-255)");
            break;
        }

        case AT_SET_REQ:              /* AT+ECMTCFG= */
        {           
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, mqttCfg, MQTTCFG_CFG_MAX_LEN, &length , (CHAR *)MQTTCFG_CFG_STR_DEF)) != AT_PARA_ERR)
            {
                if(strcmp((CHAR*)mqttCfg, "echomode") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &echomode, MQTTCFG_2_ECHO_MIN, MQTTCFG_2_ECHO_MAX, MQTTCFG_2_ECHO_DEF)) == AT_PARA_OK)
                        {
                            cfgData.decParam1 = echomode;
                            ret = mqtt_client_config(MQTT_CONFIG_ECHOMODE, tcpId, &cfgData);
                        }
                        if(ret == AT_PARA_DEFAULT)
                        {
                            mqttNewContext = mqttFindContext(tcpId);
                            if(mqttNewContext != NULL)
                            {
                                sprintf(atRspBuf, "+ECMTCFG: \"echomode\",%d",mqttNewContext->echomode);
                                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                            }
                            else
                            {
                                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            }
                            break;
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "dataformat") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &sendFormat, MQTTCFG_2_TXFORMAT_MIN, MQTTCFG_2_TXFORMAT_MAX, MQTTCFG_2_TXFORMAT_DEF)) == AT_PARA_OK)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &recvFormat, MQTTCFG_3_RXFORMAT_MIN, MQTTCFG_3_RXFORMAT_MAX, MQTTCFG_3_RXFORMAT_DEF)) != AT_PARA_ERR)
                            {
                                cfgData.decParam1 = sendFormat;
                                cfgData.decParam2 = recvFormat;
                                ret = mqtt_client_config(MQTT_CONFIG_DATAFORMAT, tcpId, &cfgData);
                            }
                        }
                        else
                        {
                            if(ret == AT_PARA_DEFAULT)
                            {
                                mqttNewContext = mqttFindContext(tcpId);
                                if(mqttNewContext != NULL)
                                {
                                    sprintf(atRspBuf, "+ECMTCFG: \"dataformat\",%d,%d",mqttNewContext->send_data_format,mqttNewContext->recv_data_format);
                                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                                }
                                else
                                {
                                    rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                }
                                break;
                            }
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "keepalive") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &keepAliveTime, MQTTCFG_2_KEEPALIVE_MIN, MQTTCFG_2_KEEPALIVE_MAX, MQTTCFG_2_KEEPALIVE_DEF)) == AT_PARA_OK)
                        {
                            cfgData.decParam1 = keepAliveTime;
                            ret = mqtt_client_config(MQTT_CONFIG_KEEPALIVE, tcpId, &cfgData);
                        }
                        if(ret == AT_PARA_DEFAULT)
                        {
                            mqttNewContext = mqttFindContext(tcpId);
                            if(mqttNewContext != NULL)
                            {
                                sprintf(atRspBuf, "+ECMTCFG: \"keepalive\",%d",mqttNewContext->keepalive);
                                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                            }
                            else
                            {
                                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            }
                            break;
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "session") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &cleanSession, MQTTCFG_2_SESSION_MIN, MQTTCFG_2_SESSION_MAX, MQTTCFG_2_SESSION_DEF)) == AT_PARA_OK)
                        {
                            cfgData.decParam1 = cleanSession;
                            ret = mqtt_client_config(MQTT_CONFIG_SEESION, tcpId, &cfgData);
                        }
                        if(ret == AT_PARA_DEFAULT)
                        {
                            mqttNewContext = mqttFindContext(tcpId);
                            if(mqttNewContext != NULL)
                            {
                                sprintf(atRspBuf, "+ECMTCFG: \"session\",%d",mqttNewContext->session);
                                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                            }
                            else
                            {
                                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            }
                            break;
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "timeout") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &pktTimeout, MQTTCFG_2_PKT_MIN, MQTTCFG_2_PKT_MAX, MQTTCFG_2_PKT_DEF)) == AT_PARA_OK)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &retryTimeout, MQTTCFG_3_RETRY_MIN, MQTTCFG_3_RETRY_MAX, MQTTCFG_3_RETRY_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &timeoutNotice, MQTTCFG_4_NOTICE_MIN, MQTTCFG_4_NOTICE_MAX, MQTTCFG_4_NOTICE_DEF)) != AT_PARA_ERR)
                                {
                                    cfgData.decParam1 = pktTimeout;
                                    cfgData.decParam2 = retryTimeout;
                                    cfgData.decParam3 = timeoutNotice;
                                    ret = mqtt_client_config(MQTT_CONFIG_TIMEOUT, tcpId, &cfgData);
                                }
                            }
                        }
                        else
                        {
                            if(ret == AT_PARA_DEFAULT)
                            {
                                mqttNewContext = mqttFindContext(tcpId);
                                if(mqttNewContext != NULL)
                                {
                                    sprintf(atRspBuf, "+ECMTCFG: \"timeout\",%d,%d,%d",mqttNewContext->pkt_timeout,mqttNewContext->retry_time,mqttNewContext->timeout_notice);
                                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                                }
                                else
                                {
                                    rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                }
                                break;
                            }
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "will") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &willFlag, MQTTCFG_2_WILLFLAG_MIN, MQTTCFG_2_WILLFLAG_MAX, MQTTCFG_2_WILLFLAG_DEF)) == AT_PARA_OK)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &willOptionQos, MQTTCFG_3_WILLQOS_MIN, MQTTCFG_3_WILLQOS_MAX, MQTTCFG_3_WILLQOS_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &willOptionRetained, MQTTCFG_4_WILLQRETAIN_MIN, MQTTCFG_4_WILLQRETAIN_MAX, MQTTCFG_4_WILLQRETAIN_DEF)) != AT_PARA_ERR)
                                {
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 5, willOptionTopic, MQTTCFG_5_WILLTOPIC_MAX_LEN, &length , (CHAR *)MQTTCFG_5_WILLTOPIC_STR_DEF)) != AT_PARA_ERR)
                                    {
                                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 6, willOptionMessage, MQTTCFG_6_WILLMSG_MAX_LEN, &length , (CHAR *)MQTTCFG_6_WILLMSG_STR_DEF)) != AT_PARA_ERR)
                                        {
                                            cfgData.decParam1 = willFlag;
                                            cfgData.decParam2 = willOptionQos;
                                            cfgData.decParam3 = willOptionRetained;
                                            cfgData.strParam1 = (CHAR *)willOptionTopic;
                                            cfgData.strParam2 = (CHAR *)willOptionMessage;
                                            ret = mqtt_client_config(MQTT_CONFIG_WILL, tcpId, &cfgData);
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if(ret == AT_PARA_DEFAULT)
                            {
                                mqttNewContext = mqttFindContext(tcpId);
                                if(mqttNewContext != NULL)
                                {
                                    if(mqttNewContext->mqtt_connect_data.will.topicName.cstring == NULL)
                                    {
                                        sprintf(atRspBuf, "+ECMTCFG: \"will\",%d,%d,%d,,",mqttNewContext->mqtt_connect_data.willFlag,mqttNewContext->mqtt_connect_data.will.qos,
                                            mqttNewContext->mqtt_connect_data.will.retained);
                                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                                    }
                                    else
                                    {
                                        length = strlen(mqttNewContext->mqtt_connect_data.will.topicName.cstring) + strlen(mqttNewContext->mqtt_connect_data.will.message.cstring);
                                        atRspBufTemp = malloc(length+40);
                                        memset(atRspBufTemp, 0, (length+40));
                                        sprintf(atRspBufTemp, "+ECMTCFG: \"will\",%d,%d,%d,%s,%s",mqttNewContext->mqtt_connect_data.willFlag,mqttNewContext->mqtt_connect_data.will.qos,
                                            mqttNewContext->mqtt_connect_data.will.retained,mqttNewContext->mqtt_connect_data.will.topicName.cstring,mqttNewContext->mqtt_connect_data.will.message.cstring);
                                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBufTemp);
                                        free(atRspBufTemp);
                                    }
                                }
                                else
                                {
                                    rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                }
                                break;
                            }
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "version") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &version, MQTTCFG_2_VERSION_MIN, MQTTCFG_2_VERSION_MAX, MQTTCFG_2_VERSION_DEF)) == AT_PARA_OK)
                        {
                            cfgData.decParam1 = version;
                            ret = mqtt_client_config(MQTT_CONFIG_VERSION, tcpId, &cfgData);
                        }
                        if(ret == AT_PARA_DEFAULT)
                        {
                            mqttNewContext = mqttFindContext(tcpId);
                            if(mqttNewContext != NULL)
                            {
                                sprintf(atRspBuf, "+ECMTCFG: \"version\",%d",mqttNewContext->version);
                                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                            }
                            else
                            {
                                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                            }
                            break;
                        }
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "aliauth") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 2)) == AT_PARA_OK)
                        {
                            aliProductKey = malloc(MQTTCFG_2_PRODKEY_MAX_LEN+1);
                            memset(aliProductKey, 0, (MQTTCFG_2_PRODKEY_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)aliProductKey, MQTTCFG_2_PRODKEY_MAX_LEN, &length , (CHAR *)MQTTCFG_2_PRODKEY_STR_DEF);
                        }
                        else
                        {
                            if(ret == AT_PARA_DEFAULT)
                            {
                                mqttNewContext = mqttFindContext(tcpId);
                                if(mqttNewContext != NULL)
                                {
                                    if(mqttNewContext->aliAuth.device_name == NULL)
                                    {
                                        sprintf(atRspBuf, "+ECMTCFG: \"aliauth\",,,");
                                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                                    }
                                    else
                                    {
                                        sprintf(atRspBuf, "+ECMTCFG: \"aliauth\",%s,%s,%s",mqttNewContext->aliAuth.device_name,mqttNewContext->aliAuth.device_secret,mqttNewContext->aliAuth.product_key);
                                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                                    }
                                }
                                else
                                {
                                    rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                }
                                break;
                            }
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 3)) == AT_PARA_OK)
                        {
                            aliDeviceName = malloc(MQTTCFG_3_DEVICENAME_MAX_LEN+1);
                            memset(aliDeviceName, 0, (MQTTCFG_3_DEVICENAME_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 3, (UINT8 *)aliDeviceName, MQTTCFG_3_DEVICENAME_MAX_LEN, &length , (CHAR *)MQTTCFG_3_DEVICENAME_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 4)) == AT_PARA_OK)
                        {
                            aliDeviceSecret = malloc(MQTTCFG_4_DEVICESECRET_MAX_LEN+1);
                            memset(aliDeviceSecret, 0, (MQTTCFG_4_DEVICESECRET_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 4, (UINT8 *)aliDeviceSecret, MQTTCFG_4_DEVICESECRET_MAX_LEN, &length , (CHAR *)MQTTCFG_4_DEVICESECRET_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 5)) == AT_PARA_OK)
                        {
                            aliProductName = malloc(MQTTCFG_5_PRODNAME_MAX_LEN+1);
                            memset(aliProductName, 0, (MQTTCFG_5_PRODNAME_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 5, (UINT8 *)aliProductName, MQTTCFG_5_PRODNAME_MAX_LEN, &length , (CHAR *)MQTTCFG_5_PRODNAME_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 6)) == AT_PARA_OK)
                        {
                            aliProductSecret = malloc(MQTTCFG_6_PRODSECRET_MAX_LEN+1);
                            memset(aliProductSecret, 0, (MQTTCFG_6_PRODSECRET_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 6, (UINT8 *)aliProductSecret, MQTTCFG_6_PRODSECRET_MAX_LEN, &length , (CHAR *)MQTTCFG_6_PRODSECRET_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 7)) == AT_PARA_OK)       /*register, regnwl*/ 
                        {
                            aliAuthType = malloc(MQTTCFG_7_AUTHTYPE_MAX_LEN+1);
                            memset(aliAuthType, 0, (MQTTCFG_7_AUTHTYPE_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 7, (UINT8 *)aliAuthType, MQTTCFG_7_AUTHTYPE_MAX_LEN, &length , (CHAR *)MQTTCFG_7_AUTHTYPE_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 8)) == AT_PARA_OK)       /*hmac_sha1, hmac_sha256, hmac_md5*/  
                        {
                            aliSignMethod = malloc(MQTTCFG_8_SIGNMETHOD_MAX_LEN+1);
                            memset(aliSignMethod, 0, (MQTTCFG_8_SIGNMETHOD_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 8, (UINT8 *)aliSignMethod, MQTTCFG_8_SIGNMETHOD_MAX_LEN, &length , (CHAR *)MQTTCFG_8_SIGNMETHOD_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 9)) == AT_PARA_OK)       /*tls-psk, tls-ca*/
                        {
                            aliAuthMode = malloc(MQTTCFG_9_AUTHMODE_MAX_LEN+1);
                            memset(aliAuthMode, 0, (MQTTCFG_9_AUTHMODE_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 9, (UINT8 *)aliAuthMode, MQTTCFG_9_AUTHMODE_MAX_LEN, &length , (CHAR *)MQTTCFG_9_AUTHMODE_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 10)) == AT_PARA_OK)       /*secure mode, -2  2*/
                        {
                            aliSecureMode = malloc(MQTTCFG_10_SECUREMODE_MAX_LEN+1);
                            memset(aliSecureMode, 0, (MQTTCFG_10_SECUREMODE_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 10, (UINT8 *)aliSecureMode, MQTTCFG_10_SECUREMODE_MAX_LEN, &length , (CHAR *)MQTTCFG_10_SECUREMODE_STR_DEF);
                        }
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 11)) == AT_PARA_OK)       /*instanceId,  */
                        {
                            instanceId = malloc(MQTTCFG_11_INSTANCEID_MAX_LEN+1);
                            memset(instanceId, 0, (MQTTCFG_11_INSTANCEID_MAX_LEN+1));
                            atGetStrValue(pAtCmdReq->pParamList, 11, (UINT8 *)instanceId, MQTTCFG_11_INSTANCEID_MAX_LEN, &length , (CHAR *)MQTTCFG_11_INSTANCEID_STR_DEF);
                        }
                        
                        if((ret = atCheckParaDefaultFlag(pAtCmdReq, 12)) == AT_PARA_OK)
                        {
                            atGetNumValue(pAtCmdReq->pParamList, 12, &aliDynRegUsed, MQTTCFG_12_DYNREGUSED_MIN, MQTTCFG_12_DYNREGUSED_MAX, MQTTCFG_12_DYNREGUSED_DEF);
                        }

                        cfgData.strParam1 = (CHAR *)aliProductKey;
                        cfgData.strParam2 = (CHAR *)aliDeviceName;
                        cfgData.strParam3 = (CHAR *)aliDeviceSecret;
                        cfgData.strParam4 = (CHAR *)aliProductName;
                        cfgData.strParam5 = (CHAR *)aliProductSecret;
                        cfgData.strParam6 = (CHAR *)aliAuthType;
                        cfgData.strParam7 = (CHAR *)aliSignMethod;
                        cfgData.strParam8 = (CHAR *)aliAuthMode;
                        cfgData.strParam9 = (CHAR *)aliSecureMode;
                        cfgData.strParam10 = (CHAR *)instanceId;
                        cfgData.decParam1 = aliDynRegUsed;
                        
                        ret = mqtt_client_config(MQTT_CONFIG_ALIAUTH, tcpId, &cfgData);                        
                    }
                }
                else if(strcmp((CHAR*)mqttCfg, "cloud") == 0)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tcpId, MQTTCFG_TCP_ID_MIN, MQTTCFG_TCP_ID_MAX, MQTTCFG_TCP_ID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &cloudType, MQTTCFG_2_CLOUD_MIN, MQTTCFG_2_CLOUD_MAX, MQTTCFG_2_CLOUD_DEF)) == AT_PARA_OK)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &payloadType, MQTTCFG_3_PAYLOADTYPE_MIN, MQTTCFG_3_PAYLOADTYPE_MAX, MQTTCFG_3_PAYLOADTYPE_DEF)) == AT_PARA_OK)
                            {
                                cfgData.decParam1 = cloudType;
                                cfgData.decParam2 = payloadType;
                                ret = mqtt_client_config(MQTT_CONFIG_CLOUD, tcpId, &cfgData);
                            }
                        }
                        else
                        {
                            if(ret == AT_PARA_DEFAULT)
                            {
                                mqttNewContext = mqttFindContext(tcpId);
                                if(mqttNewContext != NULL)
                                {
                                    sprintf(atRspBuf, "+ECMTCFG: \"cloud\",%d,%d",mqttNewContext->cloud_type,mqttNewContext->payloadType);
                                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)atRspBuf);
                                }
                                else
                                {
                                    rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                }
                                break;
                            }
                        }
                    }
                }  
                else if(strcmp((CHAR*)mqttCfg, "ssl") == 0)
                {
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, sslType, MQTTCFG_1_SSL_MAX_LEN, &length , (CHAR *)MQTTCFG_1_SSL_STR_DEF)) == AT_PARA_OK)
                    {
                        if(strcmp((CHAR*)sslType, "psk") == 0)
                        {
                            length = 0;
                            atGetStrLength(pAtCmdReq->pParamList, 2, &length);
                            if(length > 0)
                            {
                                pskKey = malloc(length+1);
                                memset(pskKey, 0, (length+1));
                            }
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)pskKey, MQTTCFG_2_PSK_MAX_LEN, &length , (CHAR *)MQTTCFG_2_PSK_STR_DEF)) == AT_PARA_OK)
                            {
                                length = 0;
                                atGetStrLength(pAtCmdReq->pParamList, 3, &length);
                                if(length > 0)
                                {
                                    pskIdentity = malloc(length+1);
                                    memset(pskIdentity, 0, (length+1));
                                }
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, (UINT8 *)pskIdentity, MQTTCFG_3_PSKID_MAX_LEN, &length , (CHAR *)MQTTCFG_3_PSKID_STR_DEF)) == AT_PARA_OK)
                                {
                                    cfgData.decParam1 = MQTT_SSL_PSK;
                                    cfgData.strParam1 = pskKey;
                                    cfgData.strParam2 = pskIdentity;
                                    ret = mqtt_client_config(MQTT_CONFIG_SSL, 0, &cfgData);
                                }
                            }
                        }
                        else if(strcmp((CHAR*)sslType, "ecc") == 0)
                        {
                            length = 0;
                            atGetStrLength(pAtCmdReq->pParamList, 2, &length);
                            if(length > 0)
                            {
                                eccKey = malloc(length+1);
                                memset(eccKey, 0, (length+1));
                            }
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)eccKey, MQTTCFG_2_ECC_MAX_LEN, &length , (CHAR *)MQTTCFG_2_ECC_STR_DEF)) == AT_PARA_OK)
                            {
                                cfgData.decParam1 = MQTT_SSL_ECC;
                                cfgData.strParam1 = eccKey;
                                ret = mqtt_client_config(MQTT_CONFIG_SSL, 0, &cfgData);
                            }
                        }
                        else if(strcmp((CHAR*)sslType, "ca") == 0)
                        {
                            length = 0;
                            atGetStrLength(pAtCmdReq->pParamList, 2, &length);
                            if(length > 0)
                            {
                                caKey = malloc(length+1);
                                memset(caKey, 0, (length+1));
                            }
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, (UINT8 *)caKey, MQTTCFG_2_CA_MAX_LEN, &length , (CHAR *)MQTTCFG_2_CA_STR_DEF)) == AT_PARA_OK)
                            {
                                int count = 0;
                                int hh = 0;
                                cfgData.decParam1 = MQTT_SSL_CA;
                                //cfgData.strParam1 = caKey;
                                if(length >= MQTT_TLS_CA_SUB_SEQ_LEN)
                                {
                                    count = (length/MQTT_TLS_CA_SUB_SEQ_LEN);
                                }
                                
                                cfgData.strParam1 = malloc(length+count*6+80);
                                memset(cfgData.strParam1, 0, (length+count*6+80));
                                
                                sprintf((char*)cfgData.strParam1, "-----BEGIN CERTIFICATE-----\r\n");
                                
                                for(hh=0; hh<count; hh++)
                                {
                                    caKeyTemp = caKey+MQTT_TLS_CA_SUB_SEQ_LEN*hh;

                                    snprintf((char*)atRspBuf, MQTT_TLS_CA_SUB_SEQ_LEN+1, "%s", caKeyTemp);
                                    
                                    strcat(cfgData.strParam1, atRspBuf);
                                    memset(atRspBuf, 0, 128);
                                    strcat(cfgData.strParam1, "\r\n");
                                }
                                if((length%MQTT_TLS_CA_SUB_SEQ_LEN) > 0)
                                {
                                    memset(atRspBuf, 0, 128);
                                    caKeyTemp = caKey+MQTT_TLS_CA_SUB_SEQ_LEN*hh;
                                    length = (length%MQTT_TLS_CA_SUB_SEQ_LEN);
                                    snprintf((char*)atRspBuf, length+1, "%s", caKeyTemp);
                                    strcat(cfgData.strParam1, atRspBuf);
                                    strcat(cfgData.strParam1, "\r\n");
                                }
                                memset(atRspBuf, 0, 128);
                                sprintf((char*)atRspBuf, "-----END CERTIFICATE-----\r\n");
                                strcat(cfgData.strParam1, atRspBuf);

                                length = 0;
                                atGetStrLength(pAtCmdReq->pParamList, 3, &length);
                                if(length > 0)
                                {
                                    hostName = malloc(length+1);
                                    memset(hostName, 0, (length+1));
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, (UINT8 *)hostName, MQTTCFG_3_NAME_MAX_LEN, &length , (CHAR *)MQTTCFG_3_NAME_STR_DEF)) == AT_PARA_OK)
                                    {
                                        cfgData.strParam2 = hostName;                                   
                                    }
                                }

                                ret = mqtt_client_config(MQTT_CONFIG_SSL, 0, &cfgData);
                            }
                        }
                    }
                }               
                else
                {
                    ret = MQTT_PARAM_ERR;
                }
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttOPEN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttOPEN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 mqttHost[MQTTOPEN_1_HOST_MAX_LEN+1] = {0};
    INT32 mqttPort;
    INT32 tcpId;
    UINT16 length = 0;
    CHAR   rspBuf[128] = {0};
    mqtt_context *mqttNewContext = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECMTOPEN=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTOPEN: (0),<host_name>,<port>");
            break;
        }
        case AT_READ_REQ:            /* AT+ECMTOPEN? */
        {
            mqttNewContext = mqttFindContext(0);
            if(mqttNewContext != NULL)
            {
                if(mqttNewContext->mqtt_uri != NULL)
                {
                    sprintf((char*)rspBuf, "+ECMTOPEN:0,\"%s\",%d",mqttNewContext->mqtt_uri, mqttNewContext->port);
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)rspBuf);
                }
                else
                {
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        case AT_SET_REQ:              /* AT+ECMTOPEN= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTOPEN_TCP_ID_MIN, MQTTOPEN_TCP_ID_MAX, MQTTOPEN_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, mqttHost, MQTTOPEN_1_HOST_MAX_LEN, &length , (CHAR *)MQTTOPEN_1_HOST_STR_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &mqttPort, MQTTOPEN_2_PORT_MIN, MQTTOPEN_2_PORT_MAX, MQTTOPEN_2_PORT_DEF)) != AT_PARA_ERR)
                    {
                        ret = mqtt_client_open(reqHandle, tcpId, (CHAR *)mqttHost, mqttPort);
                    }
                }
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttCLOSE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttCLOSE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 tcpId;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECMTCLOSE=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTCLOSE: (0)");
            break;
        }

        case AT_SET_REQ:              /* AT+MQTTCREATE= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTOPEN_TCP_ID_MIN, MQTTOPEN_TCP_ID_MAX, MQTTOPEN_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                ret = mqtt_client_close(reqHandle, tcpId);
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttCONN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttCONN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 mqttClient[MQTTCONN_1_CLIENTID_MAX_LEN+1] = {0};
    UINT8 mqttUsername[MQTTCONN_2_USERNAME_MAX_LEN+1] = {0};
    UINT8 mqttPassword[MQTTCONN_3_PWD_MAX_LEN+1] = {0};
    INT32 tcpId;
    UINT16 length = 0;
    CHAR   rspBuf[32] = {0};
    mqtt_context *mqttNewContext = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECMTCONN=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTCONN: (0),\"<clientID>\"[,\"<username>\"[,,\"<password>\"]]");
            break;
        }

        case AT_READ_REQ:         /* AT+ECMTCONN? */
        {
            mqttNewContext = mqttFindContext(0);
            if(mqttNewContext != NULL)
            {
                if((mqttNewContext->is_connected == MQTT_CONN_DISCONNECTED)||(mqttNewContext->is_connected == MQTT_CONN_CLOSED))
                {
                    sprintf((char*)rspBuf, "+ECMTCONN:0,4");
                }
                else if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    sprintf((char*)rspBuf, "+ECMTCONN:0,3");
                }
                else if(mqttNewContext->is_connected == MQTT_CONN_IS_CONNECTING)
                {
                    sprintf((char*)rspBuf, "+ECMTCONN:0,2");
                }
                else
                {
                    sprintf((char*)rspBuf, "+ECMTCONN:0,1");
                }
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)rspBuf);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        case AT_SET_REQ:         /* AT+ECMTCONN= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTCONN_TCP_ID_MIN, MQTTCONN_TCP_ID_MAX, MQTTCONN_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, mqttClient, MQTTCONN_1_CLIENTID_MAX_LEN, &length , (CHAR *)MQTTCONN_1_CLIENTID_STR_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, mqttUsername, MQTTCONN_2_USERNAME_MAX_LEN, &length , (CHAR *)MQTTCONN_2_USERNAME_STR_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, mqttPassword, MQTTCONN_3_PWD_MAX_LEN, &length , (CHAR *)MQTTCONN_3_PWD_STR_DEF)) != AT_PARA_ERR)
                        {
                            ret = mqtt_client_connect(reqHandle, tcpId, (mqttClient[0]!=0?(CHAR *)mqttClient:NULL), (mqttUsername[0]!=0?(CHAR *)mqttUsername:NULL), (mqttPassword[0]!=0?(CHAR *)mqttPassword:NULL));
                        }
                    }
                }
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttDISC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttDISC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 tcpId;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECMTDISC=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTDISC: (0)");
            break;
        }

        case AT_SET_REQ:         /* AT+ECMTDISC= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTDISC_TCP_ID_MIN, MQTTDISC_TCP_ID_MAX, MQTTDISC_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                ret = mqtt_client_disconnect(reqHandle, tcpId);
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttSUB(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttSUB(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 mqttTopic0[MQTTSUB_2_TOPIC_MAX_LEN+1] = {0};
    INT32 tcpId;
    INT32 msgId;
    INT32 qos;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECMTSUB=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTSUB: (0),<msgID>,\"<topic>\",<qos>");
            break;
        }

        case AT_SET_REQ:         /* AT+ECMTSUB= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTSUB_TCP_ID_MIN, MQTTSUB_TCP_ID_MAX, MQTTSUB_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, MQTTSUB_1_MSGID_MIN, MQTTSUB_1_MSGID_MAX, MQTTSUB_1_MSGID_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, mqttTopic0, MQTTSUB_2_TOPIC_MAX_LEN, &length , (CHAR *)MQTTSUB_2_TOPIC_STR_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &qos, MQTTSUB_3_QOS_MIN, MQTTSUB_3_QOS_MAX, MQTTSUB_3_QOS_DEF)) != AT_PARA_ERR)
                        {
                            ret = mqtt_client_sub(reqHandle, tcpId, msgId, (CHAR *)mqttTopic0, qos);
                        }
                    }
                }
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttUNS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttUNS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 mqttTopic0[MQTTUNSUB_2_TOPIC_MAX_LEN+1] = {0};
    INT32 tcpId;
    INT32 msgId;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECMTUNS=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTUNS: (0),<msgID>,\"<topic>\"");
            break;
        }

        case AT_SET_REQ:         /* AT+ECMTUNS= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTUNSUB_TCP_ID_MIN, MQTTUNSUB_TCP_ID_MAX, MQTTUNSUB_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, MQTTUNSUB_1_MSGID_MIN, MQTTUNSUB_1_MSGID_MAX, MQTTUNSUB_1_MSGID_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, mqttTopic0, MQTTUNSUB_2_TOPIC_MAX_LEN, &length , (CHAR *)MQTTUNSUB_2_TOPIC_STR_DEF)) != AT_PARA_ERR)
                    {
                        ret = mqtt_client_unsub(reqHandle, tcpId, msgId, (CHAR *)mqttTopic0);
                    }
                }
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId mqttPUB(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  mqttPUB(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 mqttTopic[MQTTPUB_4_TOPIC_MAX_LEN+1] = {0};
    INT32 tcpId;
    INT32 msgId;
    INT32 qos;
    INT32 retainOrRai;
    INT32 defaultFlag = 0;
    UINT8 *mqttMsg = NULL;
    UINT8 topicLen = 0;
    UINT16 length = 0;
    INT32 cloud;
    INT32 msgType;
    INT32 rai;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECMTPUB=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECMTPUB: (0),<msgID>,<qos>,<retain>,\"<topic>\"[,\"<payload>\"]");
            break;
        }

        case AT_SET_REQ:            /* AT+ECMTPUB= */
        {            
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &tcpId, MQTTPUB_TCP_ID_MIN, MQTTPUB_TCP_ID_MAX, MQTTPUB_TCP_ID_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, MQTTPUB_1_MSGID_MIN, MQTTPUB_1_MSGID_MAX, MQTTPUB_1_MSGID_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &qos, MQTTPUB_2_QOS_MIN, MQTTPUB_2_QOS_MAX, MQTTPUB_2_QOS_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &retainOrRai, MQTTPUB_3_RETRAINED_MIN, MQTTPUB_3_RETRAINED_MAX, MQTTPUB_3_RETRAINED_DEF)) != AT_PARA_ERR)
                        {
                            if(((retainOrRai%10) > 1)||((retainOrRai/10) > 3))
                            {
                                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                break;
                            }
                                
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 4, mqttTopic, MQTTPUB_4_TOPIC_MAX_LEN, &length , (CHAR *)MQTTPUB_4_TOPIC_STR_DEF)) != AT_PARA_ERR)
                            {
                                defaultFlag = atCheckParaDefaultFlag(pAtCmdReq, 5);
                                if(defaultFlag == AT_PARA_DEFAULT) /*default is NULL*/
                                {
                                    //atcmd_ctrl[0].bDataEntryModeOn = AT_DATA_MODE_MQTT;
                                    mqttPubTempData.reqHandle = reqHandle;
                                    mqttPubTempData.tcpId = tcpId;
                                    mqttPubTempData.msgId = msgId;
                                    mqttPubTempData.qos = qos;
                                    mqttPubTempData.retained = (retainOrRai%10);
                                    mqttPubTempData.rai = (retainOrRai/10);
                                    topicLen = strlen((CHAR *)mqttTopic);
                                    mqttPubTempData.mqttTopic = malloc(topicLen+1);
                                    memset(mqttPubTempData.mqttTopic, 0, (topicLen+1));
                                    memcpy(mqttPubTempData.mqttTopic, mqttTopic, topicLen);
                                    atcChangeChannelState(pAtCmdReq->chanId, ATC_MQTT_PUB_DATA_STATE);
                                    
                                    ret = MQTT_CONTINUE;//mqtt_client_pub(reqHandle, tcpId, msgId, qos, retain, MQTT_PUB_CTRLZ, mqttTopic, PNULL);
                                    atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, ATC_SUCC_CODE, "\r\n> ");
                                }
                                else
                                {
                                    atGetStrLength(pAtCmdReq->pParamList, 5, &length);
                                    mqttMsg = malloc(length+4);
                                    memset(mqttMsg, 0, (length+4));                                    
                                    if((ret = atGetLastMixStrValue(pAtCmdReq->pParamList, 5, mqttMsg, MQTTPUB_5_MSG_MAX_LEN, &length , (CHAR *)MQTTPUB_5_MSG_STR_DEF)) != AT_PARA_ERR)
                                    {
                                        rai = (retainOrRai/10);
                                        if(length > (MQTTPUB_5_MSG_MAX_LEN-3))
                                        {
                                            if((mqttMsg[0] != '"')&&(mqttMsg[length-1] != '"'))
                                            {
                                                free(mqttMsg);
                                                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                                                break;
                                            }
                                        }

                                        ret = mqtt_client_pub(reqHandle, tcpId, msgId, qos, (retainOrRai%10), MQTT_PUB_AT, (CHAR *)mqttTopic, length, (CHAR *)mqttMsg, cloud, msgType, rai);
                                    }
                                    free(mqttMsg);
                                }
                            }
                        }
                    }
                }
            }

            if(MQTT_OK == ret)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else if(MQTT_CONTINUE == ret)
            {
                rc = CMS_RET_SUCC;
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            break;
        }
    }

    return rc;
}

CmsRetId  mqttPUBInputData(UINT8 chanId, UINT8 *pData, INT16 dataLength)
{
    CmsRetId rc = CMS_RET_SUCC;
    INT32 ret = AT_PARA_ERR;
    INT32 inputDataLength = 0;
    CHAR *mqttInputDataHexPtr = NULL;

    if(mqttInputDataPtr == NULL)
    {
        mqttInputDataPtr = malloc(MQTTPUB_5_MSG_MAX_LEN+1);
        memset(mqttInputDataPtr, 0, (MQTTPUB_5_MSG_MAX_LEN+1));
    }
    inputDataLength = dataLength;   /**/
    if((mqttDataTotalLength+inputDataLength) > MQTTPUB_5_MSG_MAX_LEN)
    {
        rc = CMS_FAIL;
    }
    else
    {
        memcpy(&mqttInputDataPtr[mqttDataTotalLength], pData, inputDataLength);
        if(mqttInputDataPtr[mqttDataTotalLength+inputDataLength-1] == AT_ASCI_CTRL_Z) //0x1A
        {
            mqttInputDataPtr[mqttDataTotalLength+inputDataLength-1] = 0;
            ret = mqtt_client_pub(mqttPubTempData.reqHandle, mqttPubTempData.tcpId, mqttPubTempData.msgId, mqttPubTempData.qos, mqttPubTempData.retained, MQTT_PUB_CTRLZ, mqttPubTempData.mqttTopic, (mqttDataTotalLength+inputDataLength-1), (CHAR *)mqttInputDataPtr, MQTTPUB_6_CLOUD_DEF, MQTTPUB_7_RETRAINED_DEF, 0);

            if(MQTT_OK == ret)
            {
                rc = atcReply(mqttPubTempData.reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(mqttPubTempData.reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
            free(mqttPubTempData.mqttTopic);
            mqttPubTempData.mqttTopic = NULL;
            mqttDataTotalLength = 0;
            if(mqttInputDataPtr != NULL)
            {
                free(mqttInputDataPtr);
                mqttInputDataPtr = NULL;
            }
            if(mqttInputDataHexPtr != NULL)
            {
                free(mqttInputDataHexPtr);
                mqttInputDataHexPtr = NULL;
            }
        }
        else if(mqttInputDataPtr[mqttDataTotalLength+inputDataLength-1] == AT_ASCI_ESC) //0x1B
        {
            atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
            mqttPUBCancel();
            mqttDataTotalLength = 0;
            rc = atcReply(mqttPubTempData.reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
        }
        else
        {
            mqttDataTotalLength = mqttDataTotalLength+dataLength;
            rc = CMS_RET_SUCC;
        }
    }

    return rc;
}

CmsRetId  mqttPUBCancel(void)
{
    CmsRetId rc = CMS_RET_SUCC;

    if(mqttInputDataPtr != NULL)
    {
        free(mqttInputDataPtr);
        mqttInputDataPtr = NULL;
    }
    if(mqttInputDataPtr != NULL)
    {
        free(mqttPubTempData.mqttTopic);
        mqttPubTempData.mqttTopic = NULL;
    }
    
    return rc;
}


