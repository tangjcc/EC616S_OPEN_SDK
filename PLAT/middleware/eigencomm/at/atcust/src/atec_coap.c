/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_coap.c
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
#include "atec_coap.h"
#include "ec_coap_api.h"
//#include "queue.h"
#include "atec_coap_cnf_ind.h"
#include "at_coap_task.h"
#include "cmicomm.h"

coap_send_data coapSendTemp;
CHAR *coapInputDataPtr = NULL;
INT32 coapDataTotalLength = 0;
UINT8 coapSendRai = 0;

#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId coapCREATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapCREATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    CoapCreateInfo coapInfo = {COAP_CREATE_FROM_AT, 0xff};
    INT32 coapPort = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPCREATE=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+COAPCREATE:<1-65535>");
            break;
        }

        case AT_SET_REQ:              /* AT+COAPCREATE= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &coapPort, COAPCREATE_0_PORT_MIN, COAPCREATE_0_PORT_MAX, COAPCREATE_0_PORT_DEF)) != AT_PARA_ERR)            
            {
                ret = coap_client_create(reqHandle, coapPort, coapInfo);
            }

            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId coapDELETE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId 
*/
CmsRetId  coapDELETE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_EXEC_REQ:              /* AT+COAPDELETE */
        {
            ret = coap_client_delete(reqHandle, 0);

            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId coapADDRES(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapADDRES(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 resLen = 0;
    UINT8 coapResource[COAPADDRES_1_RESOURCE_MAX_LEN] = {0};
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPADDRES=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+COAPADDRES: <1-50>,\"<resource>\"");
            break;
        }

        case AT_SET_REQ:              /* AT+COAPADDRES= */
        {
            if ((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &resLen, COAPADDRES_0_LEN_MIN, COAPADDRES_0_LEN_MAX, COAPADDRES_0_LEN_DEF)) != AT_PARA_ERR)            
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, coapResource, COAPADDRES_1_RESOURCE_MAX_LEN, &len, COAPADDRES_1_RESOURCE_STR_DEF)) != AT_PARA_ERR)
                {
                    ret = coap_client_addres(reqHandle, 0, resLen, (CHAR *)coapResource); 
                }
            }

            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId coapHEAD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapHEAD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 mode = 0;
    INT32 msgId = 0;
    INT32 tokenLen = 0;
    UINT8 coapToken[COAPHEAD_3_TOKEN_MAX_LEN+1] = {0};
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPHEAD=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+COAPHEAD: <mode>[,[<msgid>][,<tkl>,<token>]]");
            break;
        }

        case AT_SET_REQ:              /* AT+COAPHEAD= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &mode, COAPHEAD_0_MODE_MIN, COAPHEAD_0_MODE_MAX, COAPHEAD_0_MODE_DEF)) != AT_PARA_ERR)            
            {
                switch(mode)
                {
                    case COAPHEAD_MODE_1:
                        ret = coap_client_head(reqHandle, 0, mode, 0, 0, NULL); 
                        break;
                        
                    case COAPHEAD_MODE_2:
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &tokenLen, COAPHEAD_1_TKL_MIN, COAPHEAD_1_TKL_MAX, COAPHEAD_1_TKL_DEF)) != AT_PARA_ERR)             
                        {
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, coapToken, COAPHEAD_3_TOKEN_MAX_LEN, &len, COAPHEAD_3_TOKEN_STR_DEF)) != AT_PARA_ERR)
                            {
                                if(tokenLen*2 == strlen((CHAR *)coapToken))
                                {
                                    ret = coap_client_head(reqHandle, 0, mode, msgId, 0, (CHAR *)coapToken); 
                                }
                                else
                                {
                                    ret = AT_PARA_ERR;
                                }
                            }
                        }
                        break;
                        
                    case COAPHEAD_MODE_3:
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, COAPHEAD_1_MSGID_MIN, COAPHEAD_1_MSGID_MAX, COAPHEAD_1_MSGID_DEF)) != AT_PARA_ERR)             
                        {
                            ret = coap_client_head(reqHandle, 0, mode, msgId, 0, NULL); 
                        }
                        break;
                        
                    case COAPHEAD_MODE_4:
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, COAPHEAD_1_MSGID_MIN, COAPHEAD_1_MSGID_MAX, COAPHEAD_1_MSGID_DEF)) != AT_PARA_ERR)             
                        {
                            ret = coap_client_head(reqHandle, 0, mode, msgId, 0, NULL); 
                        }

                        break;
                        
                    case COAPHEAD_MODE_5:
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, COAPHEAD_1_MSGID_MIN, COAPHEAD_1_MSGID_MAX, COAPHEAD_1_MSGID_DEF)) != AT_PARA_ERR)             
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &tokenLen, COAPHEAD_2_TKL_MIN, COAPHEAD_2_TKL_MAX, COAPHEAD_2_TKL_DEF)) != AT_PARA_ERR)            
                            {
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, coapToken, COAPHEAD_3_TOKEN_MAX_LEN, &len, COAPHEAD_3_TOKEN_STR_DEF)) != AT_PARA_ERR)
                                {
                                    if(tokenLen*2 == strlen((CHAR *)coapToken))
                                    {
                                        ret = coap_client_head(reqHandle, 0, mode, msgId, tokenLen, (CHAR *)coapToken); 
                                    }
                                    else
                                    {
                                        ret = AT_PARA_ERR;
                                    }
                                }
                            }
                        }
                        break;
                }
            }

            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId coapOPTION(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapOPTION(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 i = 0;
    INT32 mode = 0;
    INT32 optName = 0;
    UINT8 coapValue[COAPOPTION_2_VALUE_MAX_LEN+1] = {0};
    UINT16 len = 0;
    INT32 optNumb = 0;
    AtParaRet defaultFlag = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPOPTION=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+COAPOPTION: <opt_cnt,<opt_name>,\"<opt_value>\"[,...]");
            break;
        }

        case AT_SET_REQ:              /* AT+COAPOPTION= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &mode, COAPOPTION_0_CNT_MIN, COAPOPTION_0_CNT_MAX, COAPOPTION_0_CNT_DEF)) != AT_PARA_ERR)            
            {
                for(i=0; i<12; i++)
                {
                    defaultFlag = atCheckParaDefaultFlag(pAtCmdReq, i*2+1);
                    if(defaultFlag == AT_PARA_DEFAULT)
                    {
                        break;
                    }
                    optNumb++;
                }
                if(mode != optNumb)
                {
                    ret = AT_PARA_ERR;
                }
                else
                {
                    for(i=0; i<mode; i++)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, i*2+1, &optName, COAPOPTION_1_NAME_MIN, COAPOPTION_1_NAME_MAX, COAPOPTION_1_NAME_DEF)) != AT_PARA_ERR)          
                        {
                            if((ret = atGetStrValue(pAtCmdReq->pParamList, i*2+2, coapValue, COAPOPTION_2_VALUE_MAX_LEN, &len, COAPOPTION_2_VALUE_STR_DEF)) != AT_PARA_ERR)
                            {
                                if(i == (mode-1))
                                {
                                    ret = coap_client_option(reqHandle, 0, optName, (CHAR *)coapValue, 1);
                                }
                                else
                                {
                                    ret = coap_client_option(reqHandle, 0, optName, (CHAR *)coapValue, 0);
                                }
                            }
                        }
                    }
                }
            }

            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId coapSEND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapSEND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 coapIp[256] = {0};
    INT32 coapMethod = 0;
    INT32 coapMsgTypeAndRai = 0;
    INT32 coapMsgType = 0;
    UINT8 coapRai = 0;
    INT32 coapPort;
    INT32 payloadLen = 0;
    UINT8 *coapPayload;
    UINT16 ipLen = 0;
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPSEND=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+COAPSEND: <msgType>,<method/rspcode>,<ipAddr>,<port>,[,<length>,<data>]");
            break;
        }
        
        case AT_SET_REQ:              /* AT+COAPSEND= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &coapMsgTypeAndRai, COAPSEND_0_MSGTYPE_MIN, COAPSEND_0_MSGTYPE_MAX, COAPSEND_0_MSGTYPE_DEF)) != AT_PARA_ERR)            
            {
                coapMsgType = (coapMsgTypeAndRai%10);
                coapRai = (UINT8)(coapMsgTypeAndRai/10);
                coapSendRai = coapRai;
                if((coapMsgType > 3)||(coapRai > 3))
                {
                    rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
                    break;
                }
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &coapMethod, COAPSEND_1_METHOD_MIN, COAPSEND_1_METHOD_MAX, COAPSEND_1_METHOD_DEF) )!= AT_PARA_ERR)            
                {
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, coapIp, COAPSEND_2_IP_MAX_LEN, &ipLen, COAPSEND_2_IP_STR_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &coapPort, COAPSEND_3_PORT_MIN, COAPSEND_3_PORT_MAX, COAPSEND_3_PORT_DEF)) != AT_PARA_ERR)            
                        {
                            if((ret =atGetNumValue(pAtCmdReq->pParamList, 4, &payloadLen, COAPSEND_4_LEN_MIN, COAPSEND_4_LEN_MAX, COAPSEND_4_LEN_DEF)) == AT_PARA_OK)            
                            {
                                atGetStrLength(pAtCmdReq->pParamList, 5, &len);
                                coapPayload = malloc(len+1);
                                memset(coapPayload, 0, (len+1));
                                
                                if((ret = atGetLastMixStrValue(pAtCmdReq->pParamList, 5, coapPayload, COAPSEND_5_DATA_MAX_LEN, &len, COAPSEND_5_DATA_STR_DEF)) != AT_PARA_ERR)
                                {                                
                                    if(len == 1) 
                                    {
                                        if(coapPayload[0] == '"')/* case one "*/
                                        {
                                            if(payloadLen == 0)
                                            {
                                                ret = coap_client_send(reqHandle, 0, coapMsgType, coapMethod, (CHAR *)coapIp, coapPort, 0, (CHAR *)NULL, COAP_SEND_AT, coapRai);
                                                free(coapPayload);
                                            }
                                            else
                                            {
                                                free(coapPayload);
                                                ret = COAP_ERR;
                                            }
                                        }
                                    }
                                    else if(len == 2)
                                    {
                                        if((coapPayload[0] == '"')&&(coapPayload[len-1] == '"'))/* case two ""*/
                                        {
                                            if(payloadLen == 0)
                                            {
                                                ret = coap_client_send(reqHandle, 0, coapMsgType, coapMethod, (CHAR *)coapIp, coapPort, 0, (CHAR *)NULL, COAP_SEND_AT, coapRai);
                                                free(coapPayload);
                                            }
                                            else
                                            {
                                                free(coapPayload);
                                                ret = COAP_ERR;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if((coapPayload[0] == '"')&&(coapPayload[len-1] == '"'))/* such as "xxxxx"   */
                                        {
                                            if(len != payloadLen+2)
                                            {
                                                free(coapPayload);
                                                ret = COAP_ERR;
                                            }
                                        }
  
                                        if(ret != COAP_ERR)
                                        {
                                            ret = coap_client_send(reqHandle, 0, coapMsgType, coapMethod, (CHAR *)coapIp, coapPort, len, (CHAR *)coapPayload, COAP_SEND_AT, coapRai);
                                            free(coapPayload);
                                        }
                                    }
                                                                          
                                }
                                else
                                {
                                    free(coapPayload);
                                }
                            }
                            else
                            {
                                if(ret == AT_PARA_DEFAULT)
                                {
                                    coapSendTemp.chanId = pAtCmdReq->chanId;
                                    coapSendTemp.reqHandle = reqHandle;
                                    coapSendTemp.msg_type = coapMsgType;
                                    coapSendTemp.msg_method = coapMethod;
                                    coapSendTemp.port = coapPort;
                                    if(coapSendTemp.ip_addr != NULL)
                                    {
                                        free(coapSendTemp.ip_addr);
                                        coapSendTemp.ip_addr = NULL;
                                    }
                                    coapSendTemp.ip_addr = malloc(COAPSEND_2_IP_MAX_LEN);
                                    memset(coapSendTemp.ip_addr, 0, COAPSEND_2_IP_MAX_LEN);
                                    memcpy(coapSendTemp.ip_addr, coapIp, ipLen);
                                    atcChangeChannelState(pAtCmdReq->chanId, ATC_COAP_SEND_DATA_STATE);
                                    atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, ATC_SUCC_CODE, "\r\n> ");
                                    ret = COAP_SEND_CONTINUE;
                                }
                                else
                                {
                                    ret = COAP_ERR;
                                }
                            }
                        }
                    }
                }
            }

            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                rc = CMS_RET_SUCC;
            }
            else if(ret == COAP_SEND_CONTINUE)
            {
                rc = CMS_RET_SUCC;
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+COAPSEND */
        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}
CmsRetId  coapSENDInputData(UINT8 chanId, UINT8 *pData, INT16 dataLength)
{
    CmsRetId rc = CMS_RET_SUCC;
    INT32 ret = AT_PARA_ERR;
    INT32 inputDataLength = 0;

    if(coapInputDataPtr == NULL)
    {
        coapInputDataPtr = malloc(COAPSEND_5_DATA_MAX_LEN+1);
        memset(coapInputDataPtr, 0, (COAPSEND_5_DATA_MAX_LEN+1));
    }
    inputDataLength = dataLength;
    if((coapDataTotalLength+inputDataLength) > COAPSEND_5_DATA_MAX_LEN)
    {
        rc = CMS_FAIL;
    }
    else
    {
        memcpy(&coapInputDataPtr[coapDataTotalLength], pData, inputDataLength);
        if(coapInputDataPtr[coapDataTotalLength+inputDataLength-1] == AT_ASCI_CTRL_Z) //0x1A
        {
            coapInputDataPtr[coapDataTotalLength+inputDataLength-1] = 0;
            ret = coap_client_send(coapSendTemp.reqHandle, 0, coapSendTemp.msg_type, coapSendTemp.msg_method, coapSendTemp.ip_addr, coapSendTemp.port, coapDataTotalLength+inputDataLength-1, coapInputDataPtr, COAP_SEND_CTRLZ, coapSendRai);
            
            if(ret == COAP_OK)
            {
                rc = atcReply(coapSendTemp.reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(coapSendTemp.reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            }
            atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
            coapDataTotalLength = 0;
            free(coapInputDataPtr);
            coapInputDataPtr = NULL;
        }
        else if(coapInputDataPtr[coapDataTotalLength+inputDataLength-1] == AT_ASCI_ESC) //0x1B
        {
            atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
            coapDataTotalLength = 0;
            coapSENDCancel();
            rc = atcReply(coapSendTemp.reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
        }
        else
        {
            coapDataTotalLength = coapDataTotalLength+dataLength;
            rc = CMS_RET_SUCC;
        }

    }
    return rc;
}

CmsRetId  coapSENDCancel(void)
{
    CmsRetId rc = CMS_RET_SUCC;

    if(coapInputDataPtr != NULL)
    {
        free(coapInputDataPtr);
        coapInputDataPtr = NULL;
    }
    return rc;
}

/**
  \fn          CmsRetId coapDATASTATUS(const AtParaOperaType   operaType,
                                 const AtAttrParaValueP  paramValuePtr,
                                 const size_t            paramNumb,
                                 const CHAR              *infoPtr,
                                 UINT32                  *xidPtr,
                                 void                    *argPtr)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapDATASTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 outValue = 0;
    UINT8 RspBuf[32] = {0};

    switch (operaType)
    {
        case AT_READ_REQ:              /* AT+COAPDATASTATUS? */
        {
            coap_client_status(reqHandle, 0, &outValue);
            sprintf((char*)RspBuf, "+COAPDATASTATUS: %d", outValue);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId coapCFG(const AtParaOperaType   operaType,
                                 const AtAttrParaValueP  paramValuePtr,
                                 const size_t            paramNumb,
                                 const CHAR              *infoPtr,
                                 UINT32                  *xidPtr,
                                 void                    *argPtr)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 cnfValue1;
    INT32 cnfValue2;
    INT32 cnfValue3;
    UINT8 coapMode[COAPCFG_0_MODE_MAX_LEN+1] = {0};    
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPCFG=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }
        
        case AT_READ_REQ:              /* AT+COAPCFG? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        case AT_SET_REQ:              /* AT+COAPCFG= */
        {
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, coapMode, COAPCFG_0_MODE_MAX_LEN, &len, COAPCFG_0_MODE_STR_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &cnfValue1, COAPCFG_1_VALUE_MIN, COAPCFG_1_VALUE_MAX, COAPCFG_1_VALUE_DEF)) != AT_PARA_ERR)            
                {
                    if(strncasecmp((const CHAR*)coapMode, "Showra", strlen("Showra")) == 0)
                    {
                        ret = coap_client_config(reqHandle, 0, COAP_CFG_SHOW, COAP_SHOW_SHOWRA, cnfValue1, 0);
                    }
                    else if(strncasecmp((const CHAR*)coapMode, "Showrspopt", strlen("Showrspopt")) == 0)
                    {
                        ret = coap_client_config(reqHandle, 0, COAP_CFG_SHOW, COAP_SHOW_SHOWRSPOPT, cnfValue1, 0);
                    }
                    else if(strncasecmp((const CHAR*)coapMode, "dtls", strlen("dtls")) == 0) /*dtls---enable/disable     cloud---ali/onenet...    encrypt---sha1/sha256/md5...*/
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &cnfValue2, COAPCFG_2_VALUE_MIN, COAPCFG_2_VALUE_MAX, COAPCFG_2_VALUE_DEF)) != AT_PARA_ERR)           
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &cnfValue3, COAPCFG_3_VALUE_MIN, COAPCFG_3_VALUE_MAX, COAPCFG_3_VALUE_DEF)) != AT_PARA_ERR)
                            {
                                ret = coap_client_config(reqHandle, 0, COAP_CFG_CLOUD, cnfValue1, cnfValue2, cnfValue3);
                            }
                        }
                    }
                    else
                    {

                    }
                }
            }
            
            if(ret == COAP_OK)
            {
                //rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
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
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}


/**
  \fn          CmsRetId coapALISIGN(const AtParaOperaType   operaType,
                                 const AtAttrParaValueP  paramValuePtr,
                                 const size_t            paramNumb,
                                 const CHAR              *infoPtr,
                                 UINT32                  *xidPtr,
                                 void                    *argPtr)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumbMax
  \param[in]   paramNumbReal
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  coapALISIGN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 deviceID[COAPALISIGN_0_DEVID_MAX_LEN+1] = {0};    
    UINT8 deviceName[COAPALISIGN_1_DEVNAME_MAX_LEN+1] = {0};    
    UINT8 deviceScret[COAPALISIGN_2_DEVSECRET_MAX_LEN+1] = {0};   
    UINT8 productKey[COAPALISIGN_3_PRODUCTKEY_MAX_LEN+1] = {0}; 
    INT32 seq = 0;
    UINT8 signature[COAPALISIGN_MAX_LEN] = {0};
    UINT8 RspBuf[COAPALISIGN_MAX_LEN] = {0};
    UINT16 len = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+COAPALISIGN=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+COAPALISIGN: \"<dev_id>\",\"<dev_name>\",\"<dev_secret>\",\"<product_key>\"");
            break;
        }

        case AT_SET_REQ:              /* AT+COAPALISIGN= */
        {
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, deviceID, COAPALISIGN_0_DEVID_MAX_LEN, &len, COAPALISIGN_0_DEVID_STR_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, deviceName, COAPALISIGN_1_DEVNAME_MAX_LEN, &len, COAPALISIGN_1_DEVNAME_STR_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 2, deviceScret, COAPALISIGN_2_DEVSECRET_MAX_LEN, &len, COAPALISIGN_2_DEVSECRET_STR_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, productKey, COAPALISIGN_3_PRODUCTKEY_MAX_LEN, &len, COAPALISIGN_3_PRODUCTKEY_STR_DEF)) != AT_PARA_ERR)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &seq, COAPALISIGN_4_SEQ__MIN, COAPALISIGN_4_SEQ__MAX, COAPALISIGN_4_SEQ__DEF)) == AT_PARA_DEFAULT)
                            {
                                ret = coap_client_alisign(reqHandle, 0, (CHAR *)deviceID, (CHAR *)deviceName, (CHAR *)deviceScret, (CHAR *)productKey, COAP_ALI_RANDOM_DEF, (CHAR *)signature);
                            }
                            else
                            {
                                ret = coap_client_alisign(reqHandle, 0, (CHAR *)deviceID, (CHAR *)deviceName, (CHAR *)deviceScret, (CHAR *)productKey, seq, (CHAR *)signature);
                            }
                        }
                    }
                }
            }

            if(ret == COAP_OK)
            {
                snprintf((char*)RspBuf, COAPALISIGN_MAX_LEN, "+COAPALISIGN: \"%s\"", (CHAR *)signature);
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)RspBuf);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

#define _DEFINE_AT_CNF_FUNCTION_LIST_

#define _DEFINE_AT_IND_FUNCTION_LIST_
/**
  \fn          CmsRetId coapRECVind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
  \brief       at cmd ind function
  \param[in]   primId      hh
  \param[in]   reqHandle   hh
  \param[in]   rc          hh
  \param[in]   *paras      hh
  \returns     CmsRetId
*/
CmsRetId coapSENDVind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    
    switch(primId)
    {
        case APPL_COAP_SEND_IND:
            ret = coapRecvInd(reqHandle, paras);
            break;
                    
        default:
            break;

    }
    return ret;
}

/**
  \fn          CmsRetId coapRECVind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
  \brief       at cmd ind function
  \param[in]   primId      hh
  \param[in]   reqHandle   hh
  \param[in]   rc          hh
  \param[in]   *paras      hh
  \returns     CmsRetId
*/
CmsRetId coapRECVind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    
    switch(primId)
    {
        case APPL_COAP_RECV_IND:
            ret = coapRecvInd(reqHandle, paras);
            break;
                    
        default:
            break;

    }
    return ret;
}



