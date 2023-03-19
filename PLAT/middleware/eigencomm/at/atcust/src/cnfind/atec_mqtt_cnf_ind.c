/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_mqtt_cnf_ind.c
*
*  Description: Process MQTT related AT commands RESP/URC
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "atec_mqtt_cnf_ind.h"
#include "ec_mqtt_api.h"
#include "atec_mqtt.h"
#include "at_mqtt_task.h"
#include "atc_reply.h"

extern int mqtt_client_status_flag;

/******************************************************************************
 * mqttCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList mqttCmsCnfHdrList[] =
{

    {APPL_MQTT_PRIM_ID_END, PNULL}  /* this should be the last */
};

/******************************************************************************
 * mqttCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList mqttCmsIndHdrList[] =
{
    {APPL_MQTT_OPEN_IND,                mqttOpenInd},
    {APPL_MQTT_CLOSE_IND,               mqttCloseInd},
    {APPL_MQTT_CONN_IND,                mqttConnInd},
    {APPL_MQTT_DISC_IND,                mqttDiscInd},
    {APPL_MQTT_SUB_IND,                 mqttSubInd},
    {APPL_MQTT_UNSUB_IND,               mqttUnSubInd},
    {APPL_MQTT_PUB_IND,                 mqttPubInd},
    {APPL_MQTT_STAT_IND,                mqttStatInd},
    {APPL_MQTT_RECV_IND,                mqttRecvInd},

    {APPL_MQTT_PRIM_ID_END, PNULL}  /* this should be the last */
};

#define _DEFINE_AT_CNF_FUNCTION_LIST_

#define _DEFINE_AT_IND_FUNCTION_LIST_
/******************************************************************************
 * atMmCOPSSetAutoConf
 * Description: Process CMI cnf msg of AT+COPS=0  set auto plmn
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId mqttOpenInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTOPEN: %d,%d",(CHAR)mqttPara->tcp_id,(CHAR *)mqttPara->ret);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttCloseInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTCLOSE: %d,%d",(CHAR)mqttPara->tcp_id,mqttPara->ret);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttConnInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTCONN: %d,%d,%d",(CHAR)mqttPara->tcp_id,mqttPara->ret, mqttPara->conn_ret_code);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttDiscInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTDISC: %d,%d",(CHAR)mqttPara->tcp_id, mqttPara->ret);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttSubInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTSUB: %d,%d,%d,%d",(CHAR)mqttPara->tcp_id, mqttPara->msg_id, mqttPara->ret, mqttPara->sub_ret_value);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttUnSubInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTUNS: %d,%d,%d",(CHAR)mqttPara->tcp_id, mqttPara->msg_id, mqttPara->ret);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttPubInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTPUB: %d,%d,%d",(CHAR)mqttPara->tcp_id, mqttPara->msg_id, mqttPara->ret);
    atcURC(chanId, RespBuf);
    return ret;
}

CmsRetId mqttStatInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RespBuf[32] = {0};
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    sprintf((char *)RespBuf, "+ECMTSTAT: %d,%d",(CHAR)mqttPara->tcp_id, mqttPara->ret);

    ret = atcURC(chanId, (CHAR *)RespBuf);

    return ret;
}

CmsRetId mqttRecvInd(UINT16 reqhandle, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *RespBuf = NULL;
    CHAR *RespBufTemp = NULL;
    int i=0;
    mqtt_message *mqttPara = (mqtt_message*)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandle);

    {
        RespBuf = malloc(mqttPara->mqtt_topic_len+mqttPara->mqtt_payload_len+32);
        RespBufTemp = RespBuf;
        memset(RespBuf, 0, (mqttPara->mqtt_topic_len+mqttPara->mqtt_payload_len+32));

        sprintf((char *)RespBuf, "+ECMTRECV: %d,%d,\"",(CHAR)mqttPara->tcp_id, mqttPara->msg_id);

        RespBuf = RespBuf+strlen(RespBuf);

        if(mqttPara->mqtt_topic != NULL)
        {
            memcpy(RespBuf, mqttPara->mqtt_topic, mqttPara->mqtt_topic_len);
            strcat(RespBuf, "\",");
            RespBuf = RespBuf+strlen(mqttPara->mqtt_topic)+2;
        }
        else
        {
            strcat(RespBuf, ",");
            RespBuf = RespBuf+1;
        }

        if(mqttPara->mqtt_payload != NULL)
        {
            if(mqttPara->mqtt_payload[0] != 0)
            {
                memcpy(RespBuf, mqttPara->mqtt_payload, mqttPara->mqtt_payload_len);
            }
            else
            {
                for(i=0; i<mqttPara->mqtt_payload_len; i++)
                {
                    if(mqttPara->mqtt_payload[i] != 0)
                    {
                        break;
                    }
                }
                memcpy(RespBuf, (mqttPara->mqtt_payload+i), (mqttPara->mqtt_payload_len-i));
            }
        }

        ret = atcURC(chanId, (CHAR *)RespBufTemp);

        if(mqttPara->mqtt_topic != NULL)
        {
            memset(mqttPara->mqtt_topic, 0, strlen(mqttPara->mqtt_topic));
            free(mqttPara->mqtt_topic);
            mqttPara->mqtt_topic = NULL;
        }

        if(mqttPara->mqtt_payload != NULL)
        {
            memset(mqttPara->mqtt_payload, 0, strlen(mqttPara->mqtt_payload));
            free(mqttPara->mqtt_payload);
            mqttPara->mqtt_payload = NULL;
        }
        free(RespBufTemp);
    }

    return ret;
}


#define _DEFINE_AT_CNF_IND_INTERFACE_

void atApplMqttProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    //return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (mqttCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (mqttCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = mqttCmsCnfHdrList[hdrIndex].applCnfHdr;
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

void atApplMqttProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    //return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);

    while (mqttCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (mqttCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = mqttCmsIndHdrList[hdrIndex].applIndHdr;
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


