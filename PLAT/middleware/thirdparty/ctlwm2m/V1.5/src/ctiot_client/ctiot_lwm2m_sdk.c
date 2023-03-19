#include "objects/lwm2mclient.h"
#include "ct_liblwm2m.h"
#include "ct_internals.h"
#include "ctiot_aep_msg_queue.h"
#include "ctiot_lwm2m_sdk.h"
#include <errno.h>
#include "ctiot_lwm2m_internals.h"
#include "ctiot_NV_data.h"


#define CHIP_TYPE "EC616\0"
static uint8_t gCtiotCoapAckTimeout = COAP_RESPONSE_TIMEOUT;
static ctlwm2m_event_handler_t ctevent_handler = NULL;
static ctlwm2m_at_handler_t at_handler = NULL;


int resource_value_changed(char* uri)
{
    int result = 0;
    lwm2m_uri_t uriT;
    size_t uri_len;
    ctiot_funcv1_context_t* pContext;

    pContext = ctiot_funcv1_get_context();

    uri_len = strlen(uri);
    result = ctlwm2m_stringToUri(uri,uri_len,&uriT);

    ct_lwm2m_resource_value_changed(pContext->lwm2mContext,&uriT);
    return result;
}


void ctiot_funcv1_update_bootflag(ctiot_funcv1_context_t* pContext,int bootFlag)
{
    pContext->bootFlag = (ctiot_funcv1_boot_flag_e)bootFlag;
    lwm2m_printf("boot_flag:%d\r\n",bootFlag);
}

int prv_extend_query_len(void)
{
    int length =0;
    ctiot_funcv1_context_t* pContext;

    pContext = ctiot_funcv1_get_context();
    if( pContext == NULL )
        return 0;

    length += strlen("&ctapn=");
    length += strlen("&ctm2m=");
    length += strlen("&ctmt=1.0");
    length += strlen("&ctmv=1.0");

    if(pContext->chipInfo->cApn[0] )
    {
        length += strlen(pContext->chipInfo->cApn);
    }
    if(pContext->chipInfo->cFirmwareVersion[0])
    {
        length+= strlen(pContext->chipInfo->cFirmwareVersion);
    }

    return length;
}


char* prv_extend_query(int querylen)
{
    ctiot_funcv1_context_t* pContext;
    char* extend_query;

    extend_query=ct_lwm2m_malloc(querylen+1);
    if(extend_query == NULL)
        return NULL;

    pContext = ctiot_funcv1_get_context();

    memset(extend_query,0,querylen+1);
    sprintf(extend_query,"&ctapn=%s&ctm2m=%s&ctmv=1.0&ctmt=1.0",pContext->chipInfo->cApn,pContext->chipInfo->cFirmwareVersion);
    return extend_query;
}


static ctiot_funcv1_send_format_e pre_get_send_format_type(lwm2m_media_type_t format)
{
    ctiot_funcv1_send_format_e result = DATA_FORMAT_TEXT;
    switch (format)
    {
        case LWM2M_CONTENT_OPAQUE:
        {
            result = DATA_FORMAT_OPAQUE;
            break;
        }
        case LWM2M_CONTENT_TLV:
        {
            result = DATA_FORMAT_TLV;
            break;
        }
        case LWM2M_CONTENT_JSON:
        {
            result = DATA_FORMAT_JSON;
            break;
        }
        case LWM2M_CONTENT_LINK:
        {
            result = DATA_FORMAT_LINK;
            break;
        }
        default:
        {
            break;
        }
    }
    return result;
}




char* prv_at_encode(uint8_t* data,int datalen)
{
    int bufferlen=datalen*2;
    char* buffer=ct_lwm2m_malloc(bufferlen+1);
    if(buffer==NULL)
        return NULL;
    memset(buffer,0,bufferlen+1);
    int i=0;
    while(i<datalen)
    {
        uint8_t tmpValue=data[i]>>4;
        if(tmpValue<10)
        {
            buffer[2*i]=tmpValue+'0';
        }
        else
        {
            buffer[2*i]=tmpValue+'A'-10;
        }
        tmpValue=data[i] & 0x0f;
        if(tmpValue<10)
        {
            buffer[2*i+1]=tmpValue+'0';
        }
        else
        {
            buffer[2*i+1]=tmpValue+'A'-10;
        }
        i++;
    }
    return buffer;
}



ctiot_funcv1_chip_info* ctiot_funcv1_get_chip_instance()
{
    static ctiot_funcv1_chip_info instance= {0};
    if(instance.cChipType==NULL)
    {
        instance.cChipType=ct_lwm2m_strdup(CHIP_TYPE);
    }
    return &instance;
}
inline char* ctiot_funcv1_port_str(uint16_t x)
{
    static char port_str[6]={0};
    memset(port_str,0,6);
    sprintf(port_str,"%u",x);
    return port_str;
}
ctiot_funcv1_context_t* ctiot_funcv1_get_context()
{
    static ctiot_funcv1_context_t context_instance={0};
    context_instance.chipInfo=ctiot_funcv1_get_chip_instance();
    return &context_instance;
}
static uint16_t ctiot_funcv1_validate_uristr(char* uriStr, lwm2m_uri_t * uriP)
{
    char* tempStr = ct_lwm2m_malloc(strlen(uriStr));

    uint16_t ret = CTIOT_NB_SUCCESS;
    if(uriStr[0] != '<' || uriStr[strlen(uriStr) - 1] != '>'
            || strlen(uriStr) > 20)// </65535/65535/65535>
    {
        ret = CTIOT_EE_URI_ERROR;
        goto exit;
    }
    memset(tempStr, 0x00, strlen(uriStr));
    strncpy(tempStr, &uriStr[1], strlen(uriStr) - 2);
    if(ctlwm2m_stringToUri(tempStr, strlen(tempStr), uriP) == 0)
    {
        ret = CTIOT_EE_URI_ERROR;
        goto exit;
    }
    else
    {
        if(!LWM2M_URI_IS_SET_INSTANCE(uriP) || uriP->objectId == 0 || uriP->objectId == 4
            || uriP->objectId == 5 || uriP->objectId == 19)
        {
            ret = CTIOT_EE_URI_ERROR;
            goto exit;
        }
    }
exit:
    if(tempStr != NULL)
    {
        ct_lwm2m_free(tempStr);
    }
    return ret;
}

int ctiot_funcv1_location_path_validation(char *location)
{
   char * path= "/rd/" ;
   
   lwm2m_printf("ctiot_location_path_validation,location=%s\r\n", location );
   ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_location_path_validation_0, P_INFO, "ctiot_location_path_validation,location=%s", (const uint8_t *)location);

   if(location != NULL && memcmp(location, path ,4 )== 0 && strlen (location)> 4)
     return CTIOT_NB_SUCCESS ;
   else 
     return CTIOT_EB_OTHER ;   
}


uint16_t ctiot_funcv1_parse_uristr(char* uriStr,object_instance_array_t objInsArray[], uint32_t arraySize)
{
    char* tempUri = ct_lwm2m_malloc(strlen(uriStr) + 1);
    uint16_t ret = CTIOT_NB_SUCCESS;

    object_instance_str_t objInst;
    memset(objInsArray, 0x00, arraySize);
    if(tempUri == NULL)
    {
        ret = CTIOT_EB_OTHER;
        goto exit;
    }
    strcpy(tempUri, uriStr);
    if( t_strtok((char *)tempUri, ",", objInst.args, &objInst.argc) == 0 )
    {
        ret = CTIOT_EE_URI_ERROR;
        goto exit;
    }
    for(int i = 0; i < objInst.argc; i++)
    {
        lwm2m_uri_t uriP;
        memset(&uriP, 0x00, sizeof(lwm2m_uri_t));
        lwm2m_printf("uriStr:%s\n", objInst.args[i]);
        ret = ctiot_funcv1_validate_uristr(objInst.args[i], &uriP);
        if(ret != 0)
        {
            goto exit;
        }
        int ii = 0;
        for(ii = 0; ii < MAX_MCU_OBJECT; ii ++)
        {
            if(objInsArray[ii].objId != 0 && objInsArray[ii].objId != uriP.objectId)
            {
                continue;
            }
            objInsArray[ii].objId = uriP.objectId;
            int count = objInsArray[ii].intsanceCount;
            if(count == 0)
            {
                objInsArray[ii].intanceId[objInsArray[ii].intsanceCount++] = uriP.instanceId;
            }
            else if(count < MAX_INSTANCE_IN_ONE_OBJECT)
            {
                int j = 0;
                for(j = 0; j < objInsArray[ii].intsanceCount; j ++)
                {
                    if(objInsArray[ii].intanceId[j] == uriP.instanceId)
                    {
                        break;
                    }
                }
                if(j >= objInsArray[ii].intsanceCount)
                {
                    objInsArray[ii].intanceId[objInsArray[ii].intsanceCount++] = uriP.instanceId;
                }
            }
            break;
        }
    }
exit:
    if(tempUri != NULL)
    {
        ct_lwm2m_free(tempUri);
    }
    return ret;
}

void ctiot_setCoapAckTimeout(ctiot_funcv1_signal_level_e celevel)
{
    switch (celevel)
    {
        case SIGNAL_LEVEL_0://Coverage Enhancement level 0
        {
            gCtiotCoapAckTimeout = 2;
            break;
        }
        case SIGNAL_LEVEL_1: //Coverage Enhancement level 1
        {
            gCtiotCoapAckTimeout = 6;
            break;
        }
        case SIGNAL_LEVEL_2: //Coverage Enhancement level 2
        {
            gCtiotCoapAckTimeout = 10;
            break;
        }
        default:
        {
            gCtiotCoapAckTimeout = 4;
            break;
        }
    }
    return;
}
uint8_t ctiot_getCoapAckTimeout(void)
{
    return gCtiotCoapAckTimeout;
}

uint32_t ctiot_getExchangeLifetime(void)
{
    return ((gCtiotCoapAckTimeout * ( (1 << COAP_MAX_RETRANSMIT) - 1) * COAP_ACK_RANDOM_FACTOR) + (2 * COAP_MAX_LATENCY) + gCtiotCoapAckTimeout);
}


uint32_t ctiot_getCoapMaxTransmitWaitTime(void)
{
    return ((gCtiotCoapAckTimeout * ( (1 << (COAP_MAX_RETRANSMIT + 1) ) - 1) * COAP_ACK_RANDOM_FACTOR));
}


uint16_t ctiot_funcv1_set_boot_flag(ctiot_funcv1_context_t* pTContext, uint8_t bootFlag)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(pContext == NULL)
    {
        result=CTIOT_EB_OTHER;
        goto exit;
    }

    if(bootFlag >= BOOT_FLAG_MAX_VALUE)
    {
        result = CTIOT_EB_PARAMETER_VALUE_ERROR;
        goto exit;
    }
    pContext->bootFlag = (ctiot_funcv1_boot_flag_e)bootFlag;

    if(bootFlag == BOOT_FOTA_REBOOT){
        pContext->loginStatus = UE_NOT_LOGINED;
    }

    c2f_encode_context(pContext);
exit:
    return result;

}


void prv_set_uri_option(coap_packet_t* messageP,lwm2m_uri_t* uriP)
{
    int result;
    char stringID1[LWM2M_STRING_ID_MAX_LEN];
    result=utils_intToText(uriP->objectId, (uint8_t*)stringID1, LWM2M_STRING_ID_MAX_LEN);
    stringID1[result] = 0;
    coap_set_header_uri_path_segment(messageP, stringID1);
    if (LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        result = utils_intToText(uriP->instanceId, (uint8_t*)stringID1, LWM2M_STRING_ID_MAX_LEN);
        stringID1[result] = 0;
        coap_set_header_uri_path_segment(messageP, stringID1);
    }
    else
    {
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            coap_set_header_uri_path_segment(messageP, NULL);
        }
    }
    if (LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        result = utils_intToText(uriP->resourceId, (uint8_t*)stringID1, LWM2M_STRING_ID_MAX_LEN);
        stringID1[result] = 0;
        coap_set_header_uri_path_segment(messageP, stringID1);
    }
}

void ctiot_funcv1_register_event_handler(ctlwm2m_event_handler_t callback)
{
    ctevent_handler = callback;
}

void ctiot_funcv1_notify_event(module_type_t type, int code, const char* arg, int arg_len)
{
    if(ctevent_handler != NULL)
        ctevent_handler(type, code, arg, arg_len);
}

void ctiot_funcv1_deregister_event_handler()
{
    ctevent_handler = NULL;
}

void ctiot_funcv1_register_at_handler(ctlwm2m_at_handler_t callback)
{
    at_handler = callback;
}

void ctiot_funcv1_notify_at(uint32_t reqhandle, char* msg)
{
    if(at_handler != NULL)
        at_handler(reqhandle,msg);
}

void ctiot_funcv1_deregister_at_handler()
{
    at_handler = NULL;
}

void ctiot_recvlist_slp_set(bool slp_enable)
{
    static uint8_t recvlist_slp = 0xff;
    uint8_t count;
    slpManSlpState_t state;
    slpManRet_t ret = RET_UNKNOW_FALSE;

    if(recvlist_slp == 0xff)
    {
        ret = slpManApplyPlatVoteHandle("CT_RECV", &recvlist_slp);
    
        OsaCheck(ret == RET_TRUE, ret, recvlist_slp, 0);
    }

    if(slp_enable)          
    {
        if(RET_TRUE == slpManCheckVoteState(recvlist_slp, &state, &count))
        {
            for(;count > 0; count -- )
            {
                slpManPlatVoteEnableSleep(recvlist_slp, SLP_SLP2_STATE);
            }
        }
    }
    else
    {
        if(RET_TRUE == slpManCheckVoteState(recvlist_slp, &state, &count))
        {
            slpManPlatVoteDisableSleep(recvlist_slp, SLP_SLP2_STATE);
        }
    }

}


void ctiot_funcv1_notify_nb_info(CTIOT_NB_ERRORS errInfo,ctiot_funcv1_at_to_mcu_type_e infoType,void* params,uint16_t paramLen )
{

    uint8_t* at_str=NULL;
    uint16_t len = 0;
    ctiot_funcv1_context_t* pContext;
    pContext = ctiot_funcv1_get_context();
    switch(infoType)
    {

        case AT_TO_MCU_RECEIVE:
        {
            char* tmpbuf;
#ifdef  FEATURE_REF_AT_ENABLE
            pContext->recvdataList->recv_msg_num += 1;
            if (pContext->nnmiFlag == 1)
            {
                len = strlen("+NNMI: ") + paramLen*2 + 6;//6 is for len
                tmpbuf = prv_at_encode(params, paramLen);
                if(tmpbuf!=NULL)
                {
                    at_str = ct_lwm2m_malloc(len);
                    memset(at_str,0,len);
                    if(at_str!=NULL)
                    {
                        sprintf((char *)at_str,"+NNMI: %d,%s", paramLen, tmpbuf);
                    }
                    ct_lwm2m_free(tmpbuf);
                }
                //ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_0_0, P_INFO, "pContext->nnmiFlag == 1,show=%s",at_str);
            }
            else if (pContext->nnmiFlag == 2 || pContext->nnmiFlag == 0)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_0_1, P_INFO, 0, "add data to recv list can't sleep now");
                ctiot_recvlist_slp_set(FALSE);//recv data in list can't slp2
                if(pContext->nnmiFlag == 2){
                    at_str = (uint8_t*)ct_lwm2m_strdup("+NNMI");
                }
                // add recv data to list here
                ctiot_funcv1_recvdata_list* recvDataListNode;
                recvDataListNode = ct_lwm2m_malloc(sizeof(ctiot_funcv1_recvdata_list));
                recvDataListNode->recvdata = ct_lwm2m_malloc(paramLen);
                memcpy(recvDataListNode->recvdata,params,paramLen);
                recvDataListNode->recvdataLen = paramLen;
                ctiot_funcv1_recvdata_list *nodePtrDel = NULL;
                uint16_t ret = ctiot_funcv1_coap_queue_add(pContext->recvdataList, (ctiot_funcv1_list_t *)recvDataListNode, &nodePtrDel);
                if(ret != CTIOT_NB_SUCCESS){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_0, P_INFO, 0, "add recvdataList sth wrong drop it");
                    ct_lwm2m_free(recvDataListNode->recvdata);
                    ct_lwm2m_free(recvDataListNode);
                }
                if(nodePtrDel != NULL){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_1, P_INFO, 0, "recvdataList overflow drop the earliest node");
                    ct_lwm2m_free(nodePtrDel->recvdata);
                    ct_lwm2m_free(nodePtrDel);
                }
            }
#else
            // just display EC indicaton recv msg here
            pContext->recvdataList->recv_msg_num += 1;
            if (pContext->nnmiFlag == 1)
            {
                len = strlen("+CTM2MRECV: ")+paramLen*2+6;
                tmpbuf=prv_at_encode(params, paramLen);
                lwm2m_printf("at:%s\r\n",tmpbuf);
                if(tmpbuf!=NULL)
                {
                    at_str=ct_lwm2m_malloc(len);
                    memset(at_str,0,len);
                    if(at_str!=NULL)
                    {
                        sprintf((char *)at_str,"+CTM2MRECV: %s",tmpbuf);
                    }
                    ct_lwm2m_free(tmpbuf);
                }
            }
            else if (pContext->nnmiFlag == 2 || pContext->nnmiFlag == 0)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_2_1, P_INFO, 0, "add data to recv list can't sleep now");
                ctiot_recvlist_slp_set(FALSE);//recv data in list can't slp2
                if(pContext->nnmiFlag == 2){
                    at_str = (uint8_t*)ct_lwm2m_strdup("+CTM2MRECV");
                }
                // add recv data to list here
                ctiot_funcv1_recvdata_list* recvDataListNode;
                recvDataListNode = ct_lwm2m_malloc(sizeof(ctiot_funcv1_recvdata_list));
                recvDataListNode->recvdata = ct_lwm2m_malloc(paramLen);
                memcpy(recvDataListNode->recvdata,params,paramLen);
                recvDataListNode->recvdataLen = paramLen;
                ctiot_funcv1_recvdata_list *nodePtrDel = NULL;
                uint16_t ret = ctiot_funcv1_coap_queue_add(pContext->recvdataList, (ctiot_funcv1_list_t *)recvDataListNode, &nodePtrDel);
                if(ret != CTIOT_NB_SUCCESS){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_2_2, P_INFO, 0, "add recvdataList sth wrong drop it");
                    ct_lwm2m_free(recvDataListNode->recvdata);
                    ct_lwm2m_free(recvDataListNode);
                }
                if(nodePtrDel != NULL){
                    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_2_3, P_INFO, 0, "recvdataList overflow drop the earliest node");
                    ct_lwm2m_free(nodePtrDel->recvdata);
                    ct_lwm2m_free(nodePtrDel);
                }
            }
             
#endif
            ctiot_funcv1_notify_event(MODULE_COMM, RECV_DATA_MSG, (const void*)at_str, len);
            break;
        }

        case AT_TO_MCU_STATUS:
        {
#ifndef  FEATURE_REF_AT_ENABLE
            ctiot_funcv1_status_t* pTmpStatus=(ctiot_funcv1_status_t*)params;
            uint16_t len=40;
            if(pTmpStatus->extraInfo != NULL)
            {
                if(pTmpStatus->extraInfoLen==0)
                    len += pTmpStatus->extraInfoLen+1;
                else
                    len += 6;//uint16_t msgid 最大值65536
            }
            at_str=ct_lwm2m_malloc(len);
            memset(at_str,0,len);
            if(at_str!=NULL)
            {
                if(pTmpStatus->extraInfo==NULL)
                {
                    sprintf((char *)at_str,"+CTM2M: %s",pTmpStatus->baseInfo);
                }
                else
                {
                    if(pTmpStatus->extraInfoLen!=0)
                    {
                        sprintf((char *)at_str,"+CTM2M: %s,%s",pTmpStatus->baseInfo,(char*)pTmpStatus->extraInfo);
                    }
                    else
                    {
                        uint16_t* tmpvalue=(uint16_t*)(pTmpStatus->extraInfo);
                        sprintf((char *)at_str,"+CTM2M: %s,%u",pTmpStatus->baseInfo,*tmpvalue);
                    }
                }
            }
#endif
            break;
        }
        case AT_TO_MCU_COMMAND:
        {
            ctiot_funcv1_object_operation_t* pTmpCommand = (ctiot_funcv1_object_operation_t*) params;
            uint16_t len = strlen("+CTM2MCMD: ") + pTmpCommand->tokenLen * 2 + strlen((const char*)(pTmpCommand->uri)) + 50;
            char* token = prv_at_encode(pTmpCommand->token, pTmpCommand->tokenLen);
            if(pTmpCommand->cmdType == CMD_TYPE_OBSERVE)
            {
                len += 2;
                at_str=ct_lwm2m_malloc(len);
                memset(at_str,0,len);
                sprintf((char *)at_str,"+CTM2MCMD: %d,%d,%s,%s,%d",pTmpCommand->msgId,pTmpCommand->cmdType,token,pTmpCommand->uri,pTmpCommand->observe);
            }
            else if(pTmpCommand->cmdType == CMD_TYPE_READ || pTmpCommand->cmdType == CMD_TYPE_DISCOVER || pTmpCommand->cmdType == CMD_TYPE_DELETE)
            {

                at_str=ct_lwm2m_malloc(len);
                memset(at_str,0,len);
                sprintf((char *)at_str,"+CTM2MCMD: %d,%d,%s,%s",pTmpCommand->msgId,pTmpCommand->cmdType,token,pTmpCommand->uri);
            }
            else if(pTmpCommand->cmdType == CMD_TYPE_WRITE || pTmpCommand->cmdType == CMD_TYPE_WRITE_PARTIAL
                ||pTmpCommand->cmdType == CMD_TYPE_WRITE_ATTRIBUTE ||  pTmpCommand->cmdType ==  CMD_TYPE_EXECUTE
                ||pTmpCommand->cmdType == CMD_TYPE_CREATE)
            {
                ctiot_funcv1_send_format_e dataFormat = pre_get_send_format_type(pTmpCommand->dataFormat );
                if(pTmpCommand->dataLen == 0)
                {
                    len += 4;
                    at_str = ct_lwm2m_malloc(len);
                    sprintf((char *)at_str,"+CTM2MCMD: %d,%d,%s,%s,,",pTmpCommand->msgId,pTmpCommand->cmdType,token,pTmpCommand->uri);
                }
                else
                {
                    len += pTmpCommand->dataLen * 2 + 4;
                    at_str = ct_lwm2m_malloc(len);
                    memset(at_str, 0, len);
                    if(dataFormat == DATA_FORMAT_TLV || dataFormat == DATA_FORMAT_OPAQUE)
                    {
                        char* payload = prv_at_encode(pTmpCommand->data, pTmpCommand->dataLen);
                        sprintf((char *)at_str,"+CTM2MCMD: %d,%d,%s,%s,%d,%s",pTmpCommand->msgId,pTmpCommand->cmdType,token,pTmpCommand->uri,dataFormat,payload);
                        ct_lwm2m_free(payload);
                    }
                    else
                    {
                        char* payload = ct_lwm2m_malloc(pTmpCommand->dataLen + 1);
                        memset(payload, 0x00, pTmpCommand->dataLen + 1);
                        memcpy(payload, pTmpCommand->data, pTmpCommand->dataLen);
                        sprintf((char *)at_str,"+CTM2MCMD: %d,%d,%s,%s,%d,%s",pTmpCommand->msgId,pTmpCommand->cmdType,token,pTmpCommand->uri,dataFormat,payload);
                        ct_lwm2m_free(payload);
                    }
                }
            }
            ct_lwm2m_free(token);
            break;
        }
        case AT_TO_MCU_QUERY_STATUS:
        {
            uint16_t len=strlen("+CTM2MTGETSTATUS: ")+50;
            ctiot_funcv1_query_status_t* pTmpQueryStatus=(ctiot_funcv1_query_status_t*)params;
            if(pTmpQueryStatus->queryType==STATUS_TYPE_COMINFO)//
            {
                len+=strlen((const char *)(pTmpQueryStatus->u.extraInfo.buffer));
            }
            at_str=ct_lwm2m_malloc(len);
            memset(at_str,0,len);
            if(pTmpQueryStatus->queryType==STATUS_TYPE_COMINFO)
            {
                sprintf((char *)at_str,"+CTM2MTGETSTATUS: %d,%s",pTmpQueryStatus->queryType,pTmpQueryStatus->u.extraInfo.buffer);
            }
            else if(pTmpQueryStatus->queryType==STATUS_TYPE_MSG)
            {
                sprintf((char *)at_str,"+CTM2MTGETSTATUS: %d,%d,%ld",pTmpQueryStatus->queryType,pTmpQueryStatus->queryResult,(int)(pTmpQueryStatus->u.extraInt));
            }
            else if(pTmpQueryStatus->queryType==STATUS_TYPE_CONNECT && (pTmpQueryStatus->queryResult==QUERY_STATUS_CONNECTION_AVAILABLE || pTmpQueryStatus->queryResult==QUERY_STATUS_NETWORK_TRAFFIC))
            {
                sprintf((char *)at_str,"+CTM2MTGETSTATUS: %d,%d,%ld",pTmpQueryStatus->queryType,pTmpQueryStatus->queryResult,(int)(pTmpQueryStatus->u.extraInt));
            }
            else
            {
                sprintf((char *)at_str,"+CTM2MTGETSTATUS: %d,%d",pTmpQueryStatus->queryType,pTmpQueryStatus->queryResult);
            }
            break;
        }
        case AT_TO_MCU_SENDSTATUS:
        {
#ifndef  FEATURE_REF_AT_ENABLE
            ctiot_funcv1_status_t* pTmpStatus=(ctiot_funcv1_status_t*)params;
            uint16_t len=50;
            if(pTmpStatus->extraInfo!=NULL)
            {
                if(pTmpStatus->extraInfoLen==0)
                    len += pTmpStatus->extraInfoLen+1;
                else
                    len += 6;//uint16_t msgid 最大值65536
            }
            at_str=ct_lwm2m_malloc(len);
            memset(at_str,0,len);
            if(at_str!=NULL)
            {
                //printf("baseInfo:%s\r\n",pTmpStatus->baseInfo);
                if(pTmpStatus->extraInfo==NULL)
                {
                    sprintf((char *)at_str,"+CTM2MSEND:");
                }
                else
                {
                    if(pTmpStatus->extraInfoLen!=0)
                    {
                        sprintf((char *)at_str,"+CTM2MSEND: %s",(char*)pTmpStatus->extraInfo);
                    }
                    else
                    {
                        uint16_t* tmpvalue=(uint16_t*)(pTmpStatus->extraInfo);
                        sprintf((char *)at_str,"+CTM2MSEND: %u",*tmpvalue);
                    }
                }
            }
#endif
            break;
        }
        
#ifdef  FEATURE_REF_AT_ENABLE
        case AT_TO_MCU_QLWEVTIND:
        {
            uint8_t type = (uint8_t)paramLen;
            at_str = ct_lwm2m_malloc(16);
            memset(at_str,0,16);
            if(at_str!=NULL)
            {
                sprintf((char *)at_str,"+QLWEVTIND:%d", type);
            }
            break;
        }

        case AT_TO_MCU_NSMI:
        {
            if(paramLen == 1)
            {
                uint8_t* seqnum =(uint8_t*)params;
                at_str = ct_lwm2m_malloc(48);
                memset(at_str,0,48);
                if(at_str!=NULL)
                {
                    if(*seqnum == 0){
                        sprintf((char *)at_str,"+QLWULDATASTATUS:4");
                    }else{
                        sprintf((char *)at_str,"+QLWULDATASTATUS:4,%d", *seqnum);
                    }
                }
                //ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_3_1, P_INFO, "show=%s",at_str);
            }
            else
            {
                if (pContext->nsmiFlag == 1)
                {
                    uint8_t* seqnum =(uint8_t*)params;
                    at_str = ct_lwm2m_malloc(48);
                    memset(at_str,0,48);
                    if(at_str!=NULL)
                    {
                        if(*seqnum == 0){
                            sprintf((char *)at_str,"+NSMI: SENT");
                        }else{
                            sprintf((char *)at_str,"+NSMI: SENT_TO_AIR_INTERFACE,%d",*seqnum);
                        }
                    }
                    //ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_notify_nb_info_3_2, P_INFO, "show=%s",at_str);
                }
            }
            break;
        }
        case AT_TO_MCU_HSSTATUS:
        {
            ctiot_funcv1_status_t* pTmpStatus=(ctiot_funcv1_status_t*)params;
            at_str=(uint8_t*)pTmpStatus->baseInfo;
            break;
        }
 #endif       
        default:break;
    }
    //TODO:添加往设备或端口写at数据代码
    if(at_str != NULL)
    {
        //platform_notify((char *)at_str);
        ctiot_funcv1_notify_at(pContext->reqhandle,(char *)at_str);
        ct_lwm2m_free(at_str);
    }

}


uint16_t ctiot_funcv1_add_downdata_to_queue(ctiot_funcv1_context_t* pContext, uint16_t msgid, uint8_t* token,uint8_t tokenLen, uint8_t* uri,ctiot_funcv1_operate_type_e type, lwm2m_media_type_t mediaType )
{
    if(pContext == NULL)
    {
        return CTIOT_EB_OTHER;
    }

    ctiot_funcv1_downdata_list* node = (ctiot_funcv1_downdata_list*)ct_lwm2m_malloc(sizeof(ctiot_funcv1_downdata_list));
    if(node == NULL)
    {
        return CTIOT_EB_OTHER;
    }

    if(ctiot_funcv1_coap_queue_find(pContext->downdataList, msgid) != NULL)
    {
        return CTIOT_EC_MSG_ID_REPETITIVE;
    }
    node->msgid = msgid;
    //node->token = (uint8_t *)ct_lwm2m_malloc(tokenLen);
    memcpy(node->token, token, tokenLen);
    node->tokenLen = tokenLen;
    node->uri = (uint8_t *)ct_lwm2m_strdup((const char *)uri);
    node->type = type;
    node->mediaType = mediaType;
    node->recvtime = ct_lwm2m_gettime();
    node->next = NULL;

    uint16_t result;
    ctiot_funcv1_downdata_list *nodePtrDel = NULL;
    result = ctiot_funcv1_coap_queue_add(pContext->downdataList, (ctiot_funcv1_list_t *)node, &nodePtrDel);
    if(nodePtrDel != NULL)
    {
        ct_lwm2m_free(nodePtrDel->uri);
        ct_lwm2m_free(nodePtrDel);
    }
    return result;
}


