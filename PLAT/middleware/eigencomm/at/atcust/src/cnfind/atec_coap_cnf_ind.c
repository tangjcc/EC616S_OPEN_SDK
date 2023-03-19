/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_coap_cnf_ind_api.c
*
*  Description: COAP related AT CMD RESP/URC codes
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "atec_coap_cnf_ind.h"
#include "at_coap_task.h"
#include "atec_coap.h"
#include "ec_coap_api.h"
#include "cmicomm.h"

//QueueHandle_t coap_msg_handle;
//extern int coap_check_task_status();
int autoRegSuccFlag = 0;

/******************************************************************************
 * coapCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList coapCmsCnfHdrList[] =
{
    {APPL_COAP_CREATE_CNF,              coapCreateCnf},
    {APPL_COAP_DELETE_CNF,              coapDeleteCnf},
    {APPL_COAP_ADDRES_CNF,              coapAddresCnf},
    {APPL_COAP_HEAD_CNF,                coapHeadCnf},
    {APPL_COAP_OPTION_CNF,              coapOptionCnf},
    {APPL_COAP_SEND_CNF,                coapSendCnf},
    {APPL_COAP_CNF_CNF,                 coapConfigCnf},

    {APPL_COAP_PRIM_ID_END, PNULL}  /* this should be the last */
};

/******************************************************************************
 * coapCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList coapCmsIndHdrList[] =
{
    {APPL_COAP_RECV_IND,                coapRecvInd},

    {APPL_COAP_PRIM_ID_END, PNULL}  /* this should be the last */
};

#define _DEFINE_AT_CNF_FUNCTION_LIST_
CmsRetId coapCreateCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId coapDeleteCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId coapAddresCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else if(rc == APPL_RET_NETWORK_FAIL)
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId coapHeadCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else if(rc == APPL_RET_NETWORK_FAIL)
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId coapOptionCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else if(rc == APPL_RET_NETWORK_FAIL)
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId coapSendCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else if(rc == APPL_RET_NETWORK_FAIL)
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

/******************************************************************************
 * atDevCFUNSetConf
 * Description: Process CMI cnf msg of AT+CFUN=[<fun>[,<rst>]]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId coapConfigCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else if(rc == APPL_RET_NETWORK_FAIL)
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    else
    {
        rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, NULL);
    }
    return ret;
}

#define _DEFINE_AT_IND_FUNCTION_LIST_
CmsRetId coapRecvInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    coap_message *parasCoap = (coap_message *)paras;
    int RspBufLen = 0;
    CHAR *RspBuf = NULL;
    CHAR *buff = NULL;
    int optLen = 0;
    int optBuffLen = 0;
    int payloadLen = 0;
    CHAR *payloadBuff = NULL;
    CHAR *coap_token = NULL;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);
    
    if(parasCoap->recv_optlist_buf != NULL)
    {
        optLen = strlen(parasCoap->recv_optlist_buf);
        if(optLen < COAP_RSP_TEMP_BUFF_LEN)
        {
            buff = malloc(COAP_RSP_TEMP_BUFF_LEN);
            memset(buff, 0, COAP_RSP_TEMP_BUFF_LEN);
            optBuffLen = COAP_RSP_TEMP_BUFF_LEN;
        }
        else
        {
            buff = malloc(optLen+1);
            memset(buff, 0, (optLen+1));
            optBuffLen = (optLen+1);
        }
    }
    else
    {
        buff = malloc(COAP_RSP_TEMP_BUFF_LEN);
        memset(buff, 0, COAP_RSP_TEMP_BUFF_LEN);
        optBuffLen = COAP_RSP_TEMP_BUFF_LEN;
    }

    {
        if(parasCoap->coap_payload != NULL)
        {
            payloadLen = strlen(parasCoap->coap_payload);
        }

        if(((parasCoap->msg_type == 2)||(parasCoap->msg_type == 3))||(parasCoap->msg_method >= 0x40))/* if msg_method > 0x40, it is response code*/
        {
            RspBufLen = payloadLen+optLen+COAP_RSP_BUFF_LEN_OFFSET;
            RspBuf = malloc(RspBufLen);
            memset(RspBuf, 0, RspBufLen);
            if(parasCoap->showra == 1)
            {
                sprintf((char*)RspBuf, "+COAPURC: \"rsp\",%d.%d.%d.%d,%d,%d,%d.%02d,%d", parasCoap->server_ip&0xff, 
                    (parasCoap->server_ip>>8)&0xff, (parasCoap->server_ip>>16)&0xff, (parasCoap->server_ip>>24)&0xff, ntohs(parasCoap->server_port), parasCoap->msg_type, parasCoap->msg_method>>5, parasCoap->msg_method&0x1f, parasCoap->msg_id);
            }
            else
            {
                sprintf((char*)RspBuf, "+COAPURC: \"rsp\",%d,%d.%02d,%d", parasCoap->msg_type, parasCoap->msg_method>>5, parasCoap->msg_method&0x1f, parasCoap->msg_id);
            }

            if(parasCoap->showrspopt == 1)
            {
                optLen = strlen(RspBuf);
                if(parasCoap->token_len != 0)
                {
                    parasCoap->recv_opt_cnt = parasCoap->recv_opt_cnt;
                }
                
                sprintf((char*)&RspBuf[optLen], ",%02d,", parasCoap->recv_opt_cnt);
                if(parasCoap->recv_optlist_buf != NULL)
                {    
                    optLen = strlen(parasCoap->recv_optlist_buf);
                    memset(buff, 0, optBuffLen);
                    memcpy((char*)buff, parasCoap->recv_optlist_buf, optLen);
                    strcat(RspBuf, buff);
                }
            }
            
            if(parasCoap->coap_payload != NULL)
            {
                payloadBuff = malloc(payloadLen+6);
                memset(payloadBuff, 0, (payloadLen+6));
                sprintf((char*)payloadBuff, ",%d,%s", parasCoap->payload_len, parasCoap->coap_payload);
                strcat(RspBuf, payloadBuff);  
            }
        }
        else
        {
            RspBufLen = payloadLen+optLen+COAP_RSP_BUFF_LEN_OFFSET;
            RspBuf = malloc(RspBufLen);
            memset(RspBuf, 0, RspBufLen);

            if(parasCoap->showra == 1)
            {
                sprintf((char*)RspBuf, "+COAPURC: \"req\",%d.%d.%d.%d,%d,%d,%d,%d", parasCoap->server_ip&0xff, 
                    (parasCoap->server_ip>>8)&0xff, (parasCoap->server_ip>>16)&0xff, (parasCoap->server_ip>>24)&0xff, ntohs(parasCoap->server_port), parasCoap->msg_type, parasCoap->msg_method, parasCoap->msg_id);
            }
            else
            {
                sprintf((char*)RspBuf, "+COAPURC: \"req\",%d,%d,%d", parasCoap->msg_type, parasCoap->msg_method, parasCoap->msg_id);
            }   
            
            //for mode
            {
                optLen = strlen(RspBuf);
                if(parasCoap->token_len != 0)
                {
                    if(parasCoap->token_len != 0)
                    {
                        parasCoap->recv_opt_cnt = (parasCoap->recv_opt_cnt<<1) | 0x1;
                    }
                    else
                    {
                        parasCoap->recv_opt_cnt = (parasCoap->recv_opt_cnt<<1);
                    }
                }
                sprintf((char*)&RspBuf[optLen], ",%02d", parasCoap->recv_opt_cnt);
            }

            if(parasCoap->token_len != 0)
            {
                coap_token = malloc(parasCoap->token_len*2+1);
                memset(coap_token, 0, parasCoap->token_len*2+1);
                if(cmsHexToHexStr(coap_token, parasCoap->token_len*2+1, (const UINT8 *)parasCoap->coap_token, parasCoap->token_len) > 0)
                {
                    sprintf((char*)buff, ",%d,%s", parasCoap->token_len, coap_token);
                    strcat(RspBuf, buff);
                }
                free(coap_token);
            }
            
            if(parasCoap->recv_optlist_buf != NULL)
            {    
                optLen = strlen(parasCoap->recv_optlist_buf);
                memset(buff, 0, optBuffLen);
                buff[0] = ',';
                memcpy((char*)&buff[1], parasCoap->recv_optlist_buf, optLen);
                strcat(RspBuf, buff);
            }
            
            if(parasCoap->coap_payload != NULL)
            {
                payloadBuff = malloc(payloadLen+6);
                memset(payloadBuff, 0, (payloadLen+6));
                sprintf((char*)payloadBuff, ",%d,%s", parasCoap->payload_len, parasCoap->coap_payload);
                strcat(RspBuf, payloadBuff);
            }
        }
        if(strstr(RspBuf, "\"resultDesc\":\"Success\"") != NULL)
        {
            autoRegSuccFlag = 1;
            memset(buff, 0, optBuffLen);
            sprintf((char*)buff, "%s", parasCoap->coap_payload);
            ECOMM_STRING(UNILOG_PLA_APP, autoReg580, P_SIG, "autoReg recvie ack is: %s", (UINT8 *)buff);
            memset(buff, 0, optBuffLen);
            sprintf((char*)buff, "%s", mwGetEcAutoRegAckPrint());
            ret = atcURC(AT_CHAN_DEFAULT, (CHAR*)buff);
        }
        else
        {               
            ret = atcURC(chanId, (CHAR*)RspBuf);
        }
                
        if(RspBuf != NULL)
        {
            free(RspBuf);
        }
        
        if(buff != NULL)
        {
            free(buff);
        }

        if(payloadBuff != NULL)
        {
            free(payloadBuff);
        }
    
        if(parasCoap->coap_payload != NULL)
        {
            free(parasCoap->coap_payload);
            parasCoap->coap_payload = NULL;
        }
        if(parasCoap->coap_token != NULL)
        {
            free(parasCoap->coap_token);
            parasCoap->coap_token = NULL;
        }       
        if(parasCoap->recv_optlist_buf != NULL)
        {
            free(parasCoap->recv_optlist_buf);
            parasCoap->recv_optlist_buf = NULL;
        }  
    }
    return ret;
}

#define _DEFINE_AT_CNF_IND_INTERFACE_

void atApplCoapProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (coapCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (coapCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = coapCmsCnfHdrList[hdrIndex].applCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (applCnfHdr != PNULL)
    {
        (*applCnfHdr)(pCmsCnf->header.srcHandler, pCmsCnf->header.rc, pCmsCnf->body);
    }
    else
    {
        OsaDebugBegin(applCnfHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}

void atApplCoapProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);
    ECOMM_TRACE(UNILOG_COAP, coap_msg_0, P_INFO, 1, ".....atApplCoapProcCmsInd..%d..",primId);

    while (coapCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (coapCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = coapCmsIndHdrList[hdrIndex].applIndHdr;
            break;
        }
        hdrIndex++;
    }

    if (applIndHdr != PNULL)
    {
        (*applIndHdr)(pCmsInd->header.reqHandler, pCmsInd->body);
    }
    else
    {
        OsaDebugBegin(applIndHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}



