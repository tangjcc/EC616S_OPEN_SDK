/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ec_ctlwm2m_api.c
 * Description:  API interface implementation source file for ctlwm2m request
 * History:      Rev1.0   2018-10-30
 *
 ****************************************************************************/
#include "er-coap-13.h"
#include "ec_ctlwm2m_api.h"
#include "ctiot_NV_data.h"
#include "netmgr.h"
#include "ct_internals.h"
#include "objects/lwm2mclient.h"
#include "ct_liblwm2m.h"
#ifdef WITH_MBEDTLS
#include "port/dtlsconnection.h"
#else
#include "port/connection.h"
#endif
#ifdef MAX_OBJECT_COUNT
#undef MAX_OBJECT_COUNT
#endif

#ifdef WITH_FOTA
#define MAX_OBJECT_COUNT 7
#else
#define MAX_OBJECT_COUNT 6
#endif

#ifdef WITH_MBEDTLS
    static uint8_t* dtls_buffer = NULL;
#endif

object_instance_array_t objInsArray[MAX_MCU_OBJECT] = {0};

lwm2m_object_t* ctiotObjArray[MAX_OBJECT_COUNT+MAX_MCU_OBJECT];
uint8_t ctiotSlpHandler       = 0xff;
//static uint64_t lastReadTime=0;
uint16_t gRunningSendThread=0,gRunningRecvThread=0;
thread_handle_t nRtid,nStid,vTtid;
osMutexId_t startThreadMutex = NULL;

/***************************************************************
************************Static functions************************
****************************************************************/
static uint16_t prv_free_resources(ctiot_funcv1_context_t* pContext)
{
    if(pContext==NULL || pContext->lwm2mContext==NULL)
        return 1;
    lwm2m_object_t* object=pContext->lwm2mContext->objectList;
    while(object != NULL)
    {
        lwm2m_object_t* pTmpObject=object;
        object=object->next;
        if(pTmpObject->objID == LWM2M_SECURITY_OBJECT_ID)
        {
            ct_clean_security_object(pTmpObject);
            ct_lwm2m_free(pTmpObject);
        }
        else if(pTmpObject->objID == LWM2M_SERVER_OBJECT_ID)
        {
            ct_clean_server_object(pTmpObject);
            ct_lwm2m_free(pTmpObject);
        }
        else if(pTmpObject->objID == LWM2M_DEVICE_OBJECT_ID)
        {
            ct_free_object_device(pTmpObject);
        }
        #ifdef WITH_FOTA
        else if(pTmpObject->objID == LWM2M_FIRMWARE_UPDATE_OBJECT_ID)
        {
            lwm2m_printf("prv_free_resources:ct_free_object_firmware\r\n");
            ct_free_object_firmware(pTmpObject);
        }
        #endif
        else if(pTmpObject->objID == LWM2M_CONN_MONITOR_OBJECT_ID)
        {
            ct_free_object_conn_m(pTmpObject);
        }
        else if(pTmpObject->objID == LWM2M_CONN_STATS_OBJECT_ID)
        {
            ct_free_object_conn_s(pTmpObject);
        }
        else if(pTmpObject->objID == DATA_REPORT_OBJECT)
        {
            free_data_report_object(pTmpObject);
        }
        else
        {
            LWM2M_LIST_FREE(pTmpObject->instanceList);
            if(pTmpObject->userData != NULL)
            {
                ct_lwm2m_free(pTmpObject->userData);
            }
            ct_lwm2m_free(pTmpObject);
        }


    }
    return 0;
}

static uint16_t prv_free_objects(uint8_t count)
{
    while(count--)
    {
        lwm2m_object_t* pTmpObject= ctiotObjArray[count];

        if( pTmpObject == NULL )
            continue;

        if(pTmpObject->objID == LWM2M_SECURITY_OBJECT_ID)
        {
            ct_clean_security_object(pTmpObject);
            ct_lwm2m_free(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        else if(pTmpObject->objID == LWM2M_SERVER_OBJECT_ID)
        {
            ct_clean_server_object(pTmpObject);
            ct_lwm2m_free(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        else if(pTmpObject->objID == LWM2M_DEVICE_OBJECT_ID)
        {
            ct_free_object_device(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        #ifdef WITH_FOTA
        else if(pTmpObject->objID == LWM2M_FIRMWARE_UPDATE_OBJECT_ID)
        {
            lwm2m_printf("prv_free_resources:ct_free_object_firmware\r\n");
            ct_free_object_firmware(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        #endif
        else if(pTmpObject->objID == LWM2M_CONN_MONITOR_OBJECT_ID)
        {
            ct_free_object_conn_m(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        else if(pTmpObject->objID == LWM2M_CONN_STATS_OBJECT_ID)
        {
            ct_free_object_conn_s(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        else if(pTmpObject->objID == DATA_REPORT_OBJECT)
        {
            free_data_report_object(pTmpObject);
            ctiotObjArray[count] = NULL;
        }
        else
        {
            LWM2M_LIST_FREE(pTmpObject->instanceList);
            if(pTmpObject->userData != NULL)
            {
                ct_lwm2m_free(pTmpObject->userData);
            }
            ct_lwm2m_free(pTmpObject);
            ctiotObjArray[count] = NULL;
        }

    }

    return 0;
}

#ifndef  FEATURE_REF_AT_ENABLE
static int32_t prv_get_ep(ctiot_funcv1_context_t* pContext, char *ep_name)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_get_ep_0, P_ERROR, 0, "enter");
    if( ep_name == NULL )
    {
        return -1;
    }

    if(pContext->idAuthMode == AUTHMODE_IMEI)
    {
        //sprintf(ep_name,"urn:imei:%s",pContext->chipInfo->cImei);
        sprintf(ep_name,"%s",pContext->chipInfo->cImei);
    }
    else if(pContext->idAuthMode == AUTHMODE_IMEI_IMSI)
    {
        if(strlen(pContext->chipInfo->cImsi)>0)
        {
            sprintf(ep_name,"urn:imei-imsi:%s-%s",pContext->chipInfo->cImei,pContext->chipInfo->cImsi);
        }
        else
        {
            return -1;
        }
    }
    else if((pContext->idAuthMode == AUTHMODE_SIMID_MCU || pContext->idAuthMode == AUTHMODE_SIMID_MODULE) && pContext->idAuthStr != NULL )
    {
        sprintf(ep_name,"urn:imei+simid:%s+%s",pContext->chipInfo->cImei,pContext->idAuthStr);
    }
    else if((pContext->idAuthMode == AUTHMODE_SIM9_MCU || pContext->idAuthMode == AUTHMODE_SIM9_MODULE) && pContext->idAuthStr != NULL )
    {
        sprintf(ep_name,"urn:imei+sm9:%s+%s",pContext->chipInfo->cImei,pContext->idAuthStr);
    }
    else
    {
       ECOMM_TRACE(UNILOG_CTLWM2M, prv_get_ep_2, P_ERROR, 0, "error 968");
       return -1;
    }
    ECOMM_STRING(UNILOG_CTLWM2M, prv_get_ep_3, P_ERROR,  "ep_name:%s",(uint8_t*)ep_name);

    return 0;
}
#endif

static bool prv_recover_connect(char* serverIp,uint16_t serverPort,ctiot_funcv1_client_data_t * dataP)
{
    lwm2m_server_t * server;
    char localIP[CTIOT_MAX_IP_LEN]={0};
    ctiot_funcv1_context_t* pContext=ctiot_funcv1_get_context();
    if(pContext->lwm2mContext == NULL)
    {
        return FALSE;
    }
    server = pContext->lwm2mContext->serverList;
    pContext->lwm2mContext->userData=(void *)&pContext->clientInfo;
    pContext->lwm2mContext->state=STATE_READY;
    uint16_t rc=ctiot_funcv1_get_localIP(localIP);
    
    while(server!=NULL)
    {
        if (server->sessionH == NULL)
        {
            server->sessionH = ct_lwm2m_connect_server(server->secObjInstID, pContext->lwm2mContext->userData);
        }
        if(server->sessionH==NULL)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_recover_connect_1, P_INFO, 0, "connect to server failed.");
            return FALSE;
        }
        
        server->status=STATE_REGISTERED;
        if(rc==CTIOT_NB_SUCCESS && strcmp(localIP,pContext->localIP)!=0)
        {
            memset(pContext->localIP,0,CTIOT_MAX_IP_LEN);
            strcpy(pContext->localIP,localIP);
        }
        server=server->next;
    }

    return TRUE;
}

static bool prv_check_register_params(ctiot_funcv1_context_t* pContext)
{
    //bool result=false;
#ifndef  FEATURE_REF_AT_ENABLE
    if(pContext->serverIP==NULL || strlen(pContext->serverIP)<3) 
    {
        goto exit;
    }
#else
    if((pContext->serverIP==NULL || strlen(pContext->serverIP)<3) && (pContext->bsServerIP==NULL || strlen(pContext->bsServerIP)<3))
    {
        goto exit;
    }
#endif
    if(pContext->lifetime < 300)
    {
        goto exit;
    }
    if(pContext ->securityMode == PRE_TLS_PSK_WITH_AES_128_CCM_8 || pContext ->securityMode == PRE_TLS_PSK_WITH_AES_128_CBC_SHA256)
    {
        if(pContext->pskid==NULL || strlen(pContext->pskid)==0 || pContext->psk==NULL || pContext->pskLen==0)
        {
            goto exit;
        }
    }
    return true;
exit:
    return false;
}

static bool prv_check_location(ctiot_funcv1_context_t* pContext)
{
    lwm2m_server_t* pServer=pContext->lwm2mContext->serverList;
    while(pServer!=NULL)
    {
        if(ctiot_funcv1_location_path_validation(pServer->location)!=CTIOT_NB_SUCCESS)
            return false;
        pServer = pServer->next;
    }
    return true;
}

static uint16_t prv_check_cominfo(ctiot_funcv1_context_t* pContext,char** firmWareInfo)
{
    ctiot_funcv1_chip_info_ptr pChipInfo=pContext->chipInfo;
    memcpy(firmWareInfo,pChipInfo->cFirmwareVersion,strlen(pChipInfo->cFirmwareVersion));
    return QUERY_STATUS_FIRMWARE_INDEX;
}

bool prv_match_uri(lwm2m_uri_t uriC,lwm2m_uri_t targetUri)
{
    bool result = false;
    if (!LWM2M_URI_IS_SET_INSTANCE(&targetUri))
    {
        return false;
    }
    if(LWM2M_URI_IS_SET_RESOURCE(&targetUri))
    {
        if(uriC.objectId == targetUri.objectId && uriC.instanceId == targetUri.instanceId && uriC.resourceId == targetUri.resourceId)
        {
            result = true;
        }
    }
    else
    {
        if(uriC.objectId == targetUri.objectId && uriC.instanceId == targetUri.instanceId )
        {
            result = true;
        }
    }
    return result;
}

static uint16_t prv_check_transchannel(ctiot_funcv1_context_t* pContext)
{
    uint16_t result=QUERY_STATUS_NO_TRANSCHANNEL;
    lwm2m_context_t* lwm2mContxt=NULL;
    char* uristr="/19/0/0";
    lwm2m_observed_t * targetP;
    lwm2m_uri_t uriC;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(pContext!=NULL)
    {
        lwm2mContxt=pContext->lwm2mContext;
    }
    if(pContext==NULL || lwm2mContxt==NULL)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }
    if(pContext->loginStatus==UE_NOT_LOGINED  || pContext->loginStatus == UE_NO_IP || pContext->loginStatus == UE_LOG_OUT)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }
    if(ctlwm2m_stringToUri(uristr,strlen(uristr),&uriC)==0)
    {
        goto exit;
    }
    if(lwm2mContxt!=NULL)
    {
        for (targetP = lwm2mContxt->observedList ; targetP != NULL ; targetP = targetP->next)
        {
            if(prv_match_uri(uriC,targetP->uri))
            {
                result=QUERY_STATUS_TRANSCHANNEL_ESTABLISHED;
                break;
            }
        }
    }
exit:
    return result;
}


static uint16_t prv_check_connect_status(ctiot_funcv1_context_t* pContext,uint64_t* extend_value)
{
    uint16_t result=QUERY_STATUS_CONNECTION_UNAVAILABLE;
    ctiot_funcv1_chip_info_ptr pChipInfo=NULL;
    lwm2m_context_t* lwm2mContxt=NULL;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(pContext!=NULL)
    {
        lwm2mContxt=pContext->lwm2mContext;
    }
    if(pContext==NULL || lwm2mContxt==NULL)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }
    if(pContext->loginStatus==UE_NOT_LOGINED || pContext->loginStatus == UE_NO_IP || pContext->loginStatus == UE_LOG_OUT)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }
    *extend_value=0;
    
    pChipInfo=pContext->chipInfo;
    if(pChipInfo == NULL)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }
    
    switch(pChipInfo->cState)
    {
        case NETWORK_UNCONNECTED:
        {
            result = QUERY_STATUS_CONNECTION_UNAVAILABLE_TEMPORARY;
            break;
        }
        case NETWORK_CONNECTED:
        {
            result =QUERY_STATUS_CONNECTION_AVAILABLE;//连接正常
            *extend_value=pChipInfo->cSignalLevel;
            break;
        }
        case NETWORK_UNAVAILABLE:
        {
            result = QUERY_STATUS_CONNECTION_UNAVAILABLE;
            break;
        }
        case NETWORK_JAM:
        {
            result = QUERY_STATUS_NETWORK_TRAFFIC;
            time_t curTime=ct_lwm2m_gettime();
            *extend_value=curTime - pChipInfo->cTrafficTime;
            break;
        }
        case NETWORK_RECOVERING:
        {
            result = QUERY_STATUS_ENGINE_EXCEPTION_RECOVERING;
            break;
        }
        case NETWORK_MANUAL_RESTART:
        {
            result =QUERY_STATUS_ENGINE_EXCEPTION_REBOOT;
            break;
        }
        default:break;
    }
exit:
    return result;
}


static uint16_t prv_check_msg(ctiot_funcv1_context_t* pContext,uint16_t msgid)
{
    uint16_t result=QUERY_STATUS_MSGID_NOT_EXIST;
    lwm2m_context_t* lwm2mContxt = NULL;
    ctiot_funcv1_updata_list * upList = NULL;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(pContext!=NULL)
    {
        lwm2mContxt=pContext->lwm2mContext;
        upList=(ctiot_funcv1_updata_list *)pContext->updataList;
    }
    if(pContext==NULL || lwm2mContxt==NULL)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }
    if(pContext->loginStatus==UE_NOT_LOGINED  || pContext->loginStatus == UE_NO_IP  || pContext->loginStatus == UE_LOG_OUT)
    {
        result=QUERY_STATUS_SESSION_NOT_EXIST;
        goto exit;
    }

    while(upList!=NULL)
    {
        if(upList->msgid == msgid)
        {
            switch(upList->status)
            {
                case SEND_DATA_CACHEING:
                {
                    result= QUERY_STATUS_MSG_IN_CACHE;
                    goto exit;
                }
                case SEND_DATA_SENDOUT:
                {
                    result=QUERY_STATUS_MSG_SENDOUT;
                    goto exit;
                }
                case SEND_DATA_DELIVERED:
                {
                    result= QUERY_STATUS_MSG_DELIVERED;
                    goto exit;
                }
                case SEND_DATA_TIMEOUT:
                {
                    result= QUERY_STATUS_PLATFORM_TIMEOUT;
                    goto exit;
                }
                case SEND_DATA_RST:
                {
                    result= QUERY_STATUS_PLATFORM_RST;
                    goto exit;
                }
                default:break;
            }
        }
        upList=upList->next;
    }
exit:
    return result;
}

static uint16_t prv_cmdrsp_set_responsecode(uint16_t responseCode, uint16_t *pCTrespCode)
{
    switch(responseCode)
    {
        case 201: *pCTrespCode = COAP_201_CREATED;break;
        case 202: *pCTrespCode = COAP_202_DELETED;break;
        case 204: *pCTrespCode = COAP_204_CHANGED;break;
        case 205: *pCTrespCode = COAP_205_CONTENT;break;
        case 231: *pCTrespCode = COAP_231_CONTINUE;break;
        case 400: *pCTrespCode = COAP_400_BAD_REQUEST;break;
        case 401: *pCTrespCode = COAP_401_UNAUTHORIZED;break;
        case 402: *pCTrespCode = COAP_402_BAD_OPTION;break;
        case 403: *pCTrespCode = COAP_403_FORBIDDEN;break;
        case 404: *pCTrespCode = COAP_404_NOT_FOUND;break;
        case 405: *pCTrespCode = COAP_405_METHOD_NOT_ALLOWED;break;
        case 406: *pCTrespCode = COAP_406_NOT_ACCEPTABLE;break;
        case 408: *pCTrespCode = COAP_408_REQ_ENTITY_INCOMPLETE;break;
        case 412: *pCTrespCode = COAP_412_PRECONDITION_FAILED;break;
        case 413: *pCTrespCode = COAP_413_ENTITY_TOO_LARGE;break;
        case 500: *pCTrespCode = COAP_500_INTERNAL_SERVER_ERROR;break;
        case 501: *pCTrespCode = COAP_501_NOT_IMPLEMENTED;break;
        case 503: *pCTrespCode = COAP_503_SERVICE_UNAVAILABLE;break;
        default: return CTIOT_EB_PARAMETER_VALUE_ERROR;
    }
    return CTIOT_NB_SUCCESS;
}

static lwm2m_media_type_t prv_get_media_type(ctiot_funcv1_send_format_e sendFormat)
{
    lwm2m_media_type_t result=LWM2M_CONTENT_TEXT;
    switch (sendFormat)
    {
        case DATA_FORMAT_OPAQUE:
        {
            result = LWM2M_CONTENT_OPAQUE;
            break;
        }
        case DATA_FORMAT_TLV:
        {
            result = LWM2M_CONTENT_TLV;
            break;
        }
        case DATA_FORMAT_JSON:
        {
            result = LWM2M_CONTENT_JSON;
            break;
        }
        case DATA_FORMAT_LINK:
        {
            result = LWM2M_CONTENT_LINK;
            break;
        }
        default:
        {
            break;
        }
    }
    return result;
}

static void prv_update_sendOption(coap_packet_t* messageP,ctiot_funcv1_updata_list* dataP)
{
    if(dataP->mode==SENDMODE_CON_REL)
    {
        messageP->sendOption=SEND_OPTION_RAI_DL_FOLLOWED;
    }
    else if(dataP->mode==SENDMODE_NON_REL)
    {
        messageP->sendOption=SEND_OPTION_RAI_NO_UL_DL_FOLLOWED;
    }
    else
    {
        messageP->sendOption=SEND_OPTION_NORMAL;
    }
}

//ec add begin
static void prv_cfg_sendOption(coap_packet_t* messageP, ctiot_funcv1_send_mode_e mode)
{
    if(mode==SENDMODE_CON_REL)
    {
        messageP->sendOption=SEND_OPTION_RAI_DL_FOLLOWED;
    }
    else if(mode==SENDMODE_NON_REL)
    {
        messageP->sendOption=SEND_OPTION_RAI_NO_UL_DL_FOLLOWED;
    }
    else
    {
        messageP->sendOption=SEND_OPTION_NORMAL;
    }
}
//ec add end

static void prv_update_payload(coap_packet_t* messageP,lwm2m_uri_t* uriP,ctiot_funcv1_updata_list* dataP,lwm2m_media_type_t* mediaType,lwm2m_data_t* dataB)
{
    int size = 1;
    //lwm2m_data_t dataB;
    uint8_t * buffer = NULL;

    switch (dataP->sendFormat)
    {
        case DATA_FORMAT_TLV:
        {
            coap_set_payload(messageP, buffer, (size_t)dataP->updata->u.asBuffer.length);
            break;
        }
        case DATA_FORMAT_OPAQUE:
        {
            ct_lwm2m_data_encode_opaque(dataP->updata->u.asBuffer.buffer,dataP->updata->u.asBuffer.length,dataB);
            int len=ct_lwm2m_data_serialize(uriP,size,dataB,mediaType,&buffer);
            if(len>0)
            {
                coap_set_payload(messageP, buffer, (size_t)len);
                //coap_set_payload(messageP, dataP->updata->u.asBuffer.buffer, (size_t)dataP->updata->u.asBuffer.length);
            }
            break;
        }
        case DATA_FORMAT_JSON:
        {
            coap_set_payload(messageP, dataP->updata->u.asBuffer.buffer, (size_t)dataP->updata->u.asBuffer.length);
            break;
        }
        case DATA_FORMAT_TEXT:
        {
            coap_set_payload(messageP, dataP->updata->u.asBuffer.buffer, (size_t)dataP->updata->u.asBuffer.length);
            break;
        }
        case DATA_FORMAT_LINK:
        {
            coap_set_payload(messageP, dataP->updata->u.asBuffer.buffer, (size_t)dataP->updata->u.asBuffer.length);
            break;
        }
        default:break;
    }
}

bool prv_check_repetitive_msgid(ctiot_funcv1_context_t* pContext,uint16_t msgId)
{
    if(ctiot_funcv1_coap_queue_find(pContext->updataList,msgId)!=NULL)
        return true;
    return false;
}

void prv_clear_downlist(ctiot_funcv1_context_t* pContext)
{
    if(pContext==NULL)
        return;
    if(pContext->downdataList==NULL)
        return;
    ctiot_funcv1_downdata_list* pTmp=(ctiot_funcv1_downdata_list*)ctiot_funcv1_coap_queue_get(pContext->downdataList);
    while(pTmp!=NULL)
    {
        ct_lwm2m_free(pTmp->uri);
        ct_lwm2m_free(pTmp);
        pTmp=(ctiot_funcv1_downdata_list*)ctiot_funcv1_coap_queue_get(pContext->downdataList);
    }
}

void prv_clear_uplist(ctiot_funcv1_context_t* pContext)
{
    if(pContext==NULL)
        return;
    if(pContext->updataList==NULL)
        return;
    ctiot_funcv1_updata_list* pTmp=(ctiot_funcv1_updata_list*)ctiot_funcv1_coap_queue_get(pContext->updataList);
    while(pTmp!=NULL)
    {
        ctiot_funcv1_data_list* pTmpData=pTmp->updata;
        while(pTmpData!=NULL)
        {
            ctiot_funcv1_data_list* toDelete=pTmpData;
            pTmpData=pTmpData->next;
            if(toDelete->dataType==DATA_TYPE_STRING || toDelete->dataType==DATA_TYPE_OPAQUE)
            {
                ct_lwm2m_free(toDelete->u.asBuffer.buffer);
            }
            ct_lwm2m_free(toDelete);

        }
        ct_lwm2m_free(pTmp->uri);
        ct_lwm2m_free(pTmp);
        pTmp=(ctiot_funcv1_updata_list*)ctiot_funcv1_coap_queue_get(pContext->updataList);
    }
}

//#ifdef FEATURE_REF_AT_ENABLE
void prv_clear_recvdatalist(ctiot_funcv1_context_t* pContext)
{
    if(pContext==NULL)
        return;
    if(pContext->recvdataList==NULL)
        return;
    ctiot_funcv1_recvdata_list* pTmp=(ctiot_funcv1_recvdata_list*)ctiot_funcv1_coap_queue_get(pContext->recvdataList);
    while(pTmp!=NULL)
    {
        ct_lwm2m_free(pTmp->recvdata);
        ct_lwm2m_free(pTmp);
        pTmp=(ctiot_funcv1_recvdata_list*)ctiot_funcv1_coap_queue_get(pContext->recvdataList);
    }
}
//#endif

uint8_t* prv_at_decode(char* data,int datalen)
{
    int bufferlen= datalen/2;
    uint8_t* buffer=ct_lwm2m_malloc(bufferlen+1);
    if(buffer!=NULL)
    {
        int i=0;
        char tmpdata;
        uint8_t tmpValue=0;
        memset(buffer, 0, bufferlen+1);
        while(i<datalen)
        {
            tmpdata=data[i];
            if(tmpdata<='9' && tmpdata>='0')
            {
                tmpValue=tmpdata-'0';
            }
            else if(tmpdata<='F' && tmpdata>='A')
            {
                tmpValue=tmpdata-'A'+10;
            }
            else if(tmpdata<='f' && tmpdata>='a')
            {
                tmpValue=tmpdata-'a'+10;
            }
            if(i%2==0)
            {
                buffer[i/2] = tmpValue << 4;
            }
            else
            {
                buffer[i/2] |= tmpValue;
            }
            i++;
            tmpdata=data[i];
            if(tmpdata<='9' && tmpdata>='0')
            {
                tmpValue=tmpdata-'0';
            }
            else if(tmpdata<='F' && tmpdata>='A')
            {
                tmpValue=tmpdata-'A'+10;
            }
            else if(tmpdata<='f' && tmpdata>='a')
            {
                tmpValue=tmpdata-'a'+10;
            }
            if(i%2==0)
            {
                buffer[i/2] = tmpValue << 4;
            }
            else
            {
                buffer[i/2] |= tmpValue;
            }
            i++;
        }
    }
    return buffer;
}

static void prv_send_message(ctiot_funcv1_context_t* pContext,coap_packet_t* message,void* sessionH,bool isObject19)
{
    int msgResult=ct_message_send(pContext->lwm2mContext, message, sessionH);
#ifndef  FEATURE_REF_AT_ENABLE
    ctiot_funcv1_status_t atStatus[1]={0};
    if(msgResult == COAP_NO_ERROR)
    {
        //发送成功
        if(message->type != COAP_TYPE_ACK)
        {
            if(isObject19)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SS2);
            }
            else
                atStatus->baseInfo=ct_lwm2m_strdup(SN2);
        }
        else
        {
            atStatus->baseInfo=ct_lwm2m_strdup(SS2);
        }
        atStatus->extraInfo = &message->mid;
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);
    }
    else
    {
        atStatus->baseInfo=ct_lwm2m_strdup(SL2);
        atStatus->extraInfo = &message->mid;
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);
    }
#endif
    if(isObject19)
        ctiot_funcv1_enable_sleepmode();
}


static void prv_sendcon_notify(lwm2m_transaction_t* transacP, void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    ctiot_funcv1_status_t atStatus[1]={0};
    ctiot_funcv1_context_t* pTmpContext=ctiot_funcv1_get_context();
    //coap_free_payload(transacP->message);
    if(packet == NULL)
    {
        atStatus->baseInfo=ct_lwm2m_strdup(SN4);//数据发送状态，发送超时
        atStatus->extraInfo=(void *)&transacP->mID;
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);
        ctiot_funcv1_enable_sleepmode();

        return;
    }
    if(packet->type==COAP_TYPE_ACK)
    {
        atStatus->extraInfo=(void *)&transacP->mID;
        atStatus->baseInfo=ct_lwm2m_strdup(SN3);//数据发送状态，已送达
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);

    }
    else if(packet->type==COAP_TYPE_RST)
    {
        //待添加处理过程
        atStatus->extraInfo=(void *)&transacP->mID;
        atStatus->baseInfo=ct_lwm2m_strdup(SN5);//数据发送状态，平台RST响应
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);
    }
    ctiot_funcv1_enable_sleepmode();
    
    ctiot_funcv1_list_t* pTmpDownList=NULL;
    ctiot_funcv1_context_t* pContext=ctiot_funcv1_get_context();
    ctiot_funcv1_coap_queue_remove(pContext->downdataList,packet->mid,&pTmpDownList);

    if(pTmpDownList!=NULL)
    {
        ctiot_funcv1_downdata_list* pDownList=(ctiot_funcv1_downdata_list *)pTmpDownList;
        if(pDownList->uri)
        {
            ct_lwm2m_free(pDownList->uri);
        }
        ct_lwm2m_free(pDownList);
    }
}
static void prv_sendcon(lwm2m_transaction_t* transacP, void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    ctiot_funcv1_status_t atStatus[1]={0};
    ctiot_funcv1_context_t* pTmpContext=ctiot_funcv1_get_context();
    //coap_free_payload(transacP->message);
    if(packet == NULL)
    {
#ifndef  FEATURE_REF_AT_ENABLE
        atStatus->baseInfo=ct_lwm2m_strdup(SS4);//数据发送状态，发送超时
        atStatus->extraInfo=(void *)&transacP->mID;
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);
#else
        pTmpContext->conDataStatus = SEND_TIMEOUT;
#endif
        ctiot_funcv1_notify_event(MODULE_LWM2M, SEND_FAILED, NULL, 0);
        return;
    }
    if(packet->type==COAP_TYPE_ACK)
    {
#ifndef  FEATURE_REF_AT_ENABLE
        atStatus->extraInfo=(void *)&transacP->mID;
        atStatus->baseInfo=ct_lwm2m_strdup(SS3);//数据发送状态，已送达
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
        ct_lwm2m_free(atStatus->baseInfo);
#else
        ECOMM_TRACE(UNILOG_CTLWM2M, prv_sendcon_1, P_INFO, 1, "con msg has recv ack seqnum=%d",transacP->seqnum);
        pTmpContext->conDataStatus = SEND_SUC;
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_NSMI, (void *)&transacP->seqnum, NULL);
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_NSMI, (void *)&transacP->seqnum, 1);//last param set 1 need show +qlwuldatastauts:4
#endif
        ctiot_funcv1_notify_event(MODULE_LWM2M, SEND_CON_DONE, NULL, 0);
    }
    else if(packet->type==COAP_TYPE_RST)
    {
        //待添加处理过程
        atStatus->extraInfo=(void *)&transacP->mID;
        atStatus->baseInfo=ct_lwm2m_strdup(SS6);//数据发送状态，传送通道失败
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
#ifdef  FEATURE_REF_AT_ENABLE
        pTmpContext->conDataStatus = GET_RST;
#endif
        ct_lwm2m_free(atStatus->baseInfo);
        ctiot_funcv1_notify_event(MODULE_LWM2M, SEND_FAILED, NULL, 0);
    }
    ctiot_funcv1_list_t* pTmpDownList=NULL;
    ctiot_funcv1_context_t* pContext=ctiot_funcv1_get_context();
    ctiot_funcv1_coap_queue_remove(pContext->downdataList,packet->mid,&pTmpDownList);

    if(pTmpDownList!=NULL)
    {
        ctiot_funcv1_downdata_list* pDownList=(ctiot_funcv1_downdata_list *)pTmpDownList;
        if(pDownList->uri)
        {
            ct_lwm2m_free(pDownList->uri);
        }
        ct_lwm2m_free(pDownList);
    }
}

static void prv_stop_send_thread()
{
    gRunningSendThread = 2;
}
static void prv_stop_recv_thread()
{
    gRunningRecvThread = 2;
}

static int prv_coap_yield(ctiot_funcv1_context_t* pContext)
{
    int result=CTIOT_NB_SUCCESS;
    uint64_t curTime = ct_lwm2m_gettime();
    if(pContext == NULL)
    {
        return -1;
    }

    struct timeval tv={5,0};
#ifdef WITH_MBEDTLS
    mbedtls_connection_t * connP = pContext->clientInfo.connList;
    if(connP != NULL && connP->dtlsSession != 0)
    {
        memset(dtls_buffer, 0x00, CTIOT_MAX_PACKET_SIZE);
        int result = ct_connection_handle_packet(dtls_buffer,CTIOT_MAX_PACKET_SIZE,connP);
        if(result < 0){
            //ECOMM_TRACE(UNILOG_CTLWM2M, prv_coap_yield_0, P_INFO, 1, "handle packet return -0x%x",-result);
        }
    }
    else
#endif
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(pContext->clientInfo.sock, &readfds);
        int hasdata=select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
        if(hasdata < 0)
        {
            if (errno != EINTR)
            {
                result = -2; //system error
                goto exit;
            }
        }
        else if (hasdata > 0)
        {
            //lastReadTime=ct_lwm2m_gettime();
            static uint8_t buffer[CTIOT_MAX_PACKET_SIZE];
            memset(buffer, 0x00, CTIOT_MAX_PACKET_SIZE);
            int numBytes;
            pContext->recv_thread_status.thread_status=THREAD_STATUS_BUSY;
            //ctiot_funcv1_sleep_vote(SYSTEM_STATUS_BUSY);
            if (FD_ISSET(pContext->clientInfo.sock, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;
                addrLen = sizeof(addr);
                numBytes = recvfrom(pContext->clientInfo.sock, buffer, CTIOT_MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);
    
    
                if (0 > numBytes)
                {
                    lwm2m_printf("Error in recvfrom(): %d %s\r\n", errno, strerror(errno));
                }
                else if (0 < numBytes)
                {
                    char s[INET_ADDRSTRLEN];
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_coap_yield_1, P_INFO, 1, "recv %d bytes from peer",numBytes);
                    if(numBytes<3)
                    {
                            return 0;
                    }
    
    #ifdef WITH_MBEDTLS
                    mbedtls_connection_t * connP;
    #else
                    connection_t * connP;
    #endif
                    if(AF_INET == addr.ss_family){
                        struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
                    } else if (AF_INET6 == addr.ss_family) {
                        struct sockaddr_in6 *saddr = (struct sockaddr_in6*)&addr;
                        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
                    }
    
                    connP = ct_connection_find(pContext->clientInfo.connList, &addr, addrLen);
                    if (connP != NULL)
                    {
                        /*
                        * Let liblwm2m respond to the query depending on the context
                        */
                        ct_handle_packet(pContext->lwm2mContext, buffer, numBytes, connP);
                    }
                }
            }
        }
    }
exit:
    return result;
}

static void ctiot_clear_all()
{
    ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();

    //pContext->loginStatus = UE_NOT_LOGINED;

    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_clear_all_1, P_INFO, 0, "exit recv thread");

    //1. exit recv thread
    prv_stop_recv_thread();
    while(pContext->recv_thread_status.thread_started != false)
        usleep(CTIOT_DEREG_WAIT_TIME);
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_clear_all_2, P_INFO, 0, "delete various lists");
    prv_deleteServerList(pContext->lwm2mContext);
    prv_deleteBootstrapServerList(pContext->lwm2mContext);
    prv_deleteObservedList(pContext->lwm2mContext);
    ct_lwm2m_free(pContext->lwm2mContext->endpointName);
    
    osMutexDelete(pContext->lwm2mContext->observe_mutex);
    if (pContext->lwm2mContext->msisdn != NULL)
    {
        ct_lwm2m_free(pContext->lwm2mContext->msisdn);
    }
    if (pContext->lwm2mContext->altPath != NULL)
    {
        ct_lwm2m_free(pContext->lwm2mContext->altPath);
    }
    ct_prv_deleteTransactionList(pContext->lwm2mContext);
    osMutexDelete(pContext->lwm2mContext->transactMutex);
    //2. release all objects
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_clear_all_3, P_INFO, 0, "release all objects");
    prv_free_resources(pContext);
    ct_lwm2m_free(pContext->lwm2mContext);
    
    pContext->lwm2mContext = NULL;

    //3. 清空uplist和downlist 待添加
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_clear_all_4, P_INFO, 0, "free message queue list");
    prv_clear_uplist(pContext);
    prv_clear_downlist(pContext);
    ctiot_funcv1_coap_queue_free(&(pContext->updataList));
    ctiot_funcv1_coap_queue_free(&(pContext->downdataList));
//#ifdef  FEATURE_REF_AT_ENABLE
   prv_clear_recvdatalist(pContext);
   ctiot_funcv1_coap_queue_free(&(pContext->recvdataList));
//#endif
    //4. release socket and connlist
    close(pContext->clientInfo.sock);
    pContext->clientInfo.sock=0;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_clear_all_5, P_INFO, 1, "connection free 0x%x",pContext->clientInfo.connList);
    ct_connection_free(pContext->clientInfo.connList);
    pContext->clientInfo.connList     = NULL;
    pContext->clientInfo.securityObjP = NULL;
    pContext->clientInfo.serverObject = NULL;

    //5. clear bootflag
    ctiot_funcv1_update_bootflag(pContext,0);

    //6. clear cache
    c2f_encode_context(pContext);

    ctiot_funcv1_enable_sleepmode();
}


__WEAK void ct_send_loop_callback(ctiot_funcv1_context_t* pContext){}

static void prv_send_thread(void* arg)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_0, P_INFO, 0, "enter");
    //static uint16_t tokenvalue=0;
    ctiot_funcv1_context_t* pContext=(ctiot_funcv1_context_t*)arg;
    if(pContext==NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }
    pContext->send_thread_status.thread_started=true;
    while(1)
    {
        if(gRunningSendThread==0)
        {
            usleep(CTIOT_THREAD_TIMEOUT);
            continue;
        }
        else if(gRunningSendThread==2)
        {

            break;
        }
        else
        {
            ctiot_funcv1_updata_list* pTmp=(ctiot_funcv1_updata_list*)ctiot_funcv1_coap_queue_get(pContext->updataList);
            if(pTmp!=NULL)
            {
                //发送当前节点数据
                lwm2m_data_t dataP;
                memset(&dataP,0,sizeof(dataP));

                if(pTmp->mode==SENDMODE_CON || pTmp->mode==SENDMODE_CON_REL)
                {
                    lwm2m_printf("\r\nsending con packet\r\n");
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_1, P_INFO, 1, "sending con packet, mid=%d", pTmp->msgid);
                    pContext->send_thread_status.thread_status=THREAD_STATUS_BUSY;
                    //CON方式发送
                    lwm2m_uri_t uriC;
                    if(ctlwm2m_stringToUri((const char *)(pTmp->uri),strlen((const char *)(pTmp->uri)),&uriC)!=0)
                    {
                        bool isObject19=false;
                        bool found=false;
                        if(uriC.objectId==19)
                            isObject19=true;
                        lwm2m_transaction_t * transaction;
                        transaction=(lwm2m_transaction_t *)ct_lwm2m_malloc(sizeof(lwm2m_transaction_t));
                        memset(transaction, 0, sizeof(lwm2m_transaction_t));
                        transaction->message = ct_lwm2m_malloc(sizeof(coap_packet_t));
                        coap_init_message(transaction->message, COAP_TYPE_CON, pTmp->responseCode, pTmp->msgid);
                        lwm2m_media_type_t mediaType=prv_get_media_type(pTmp->sendFormat);
                        coap_set_header_content_type(transaction->message,mediaType);
                        prv_update_sendOption(transaction->message,pTmp);

                        lwm2m_observed_t * targetP;
                        prv_set_uri_option(transaction->message, &uriC);
                        for (targetP = pContext->lwm2mContext->observedList ; targetP != NULL ; targetP = targetP->next)
                        {
                            if(!prv_match_uri(uriC,targetP->uri))
                                continue;
                            lwm2m_watcher_t * watcherP;
                            for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
                            {
                                if (watcherP->active == true)
                                {
                                    watcherP->lastTime=ct_lwm2m_gettime();
                                    watcherP->lastMid=pTmp->msgid;
                                    transaction->mID=pTmp->msgid;
                                    transaction->seqnum=pTmp->seqnum;
                                    transaction->peerH=watcherP->server->sessionH;
                                    transaction->userData=(void*)watcherP->server;
                                    coap_set_header_token(transaction->message, watcherP->token, watcherP->tokenLen);
                                    //coap_set_header_observe(transaction->message, watcherP->counter++);
                                    found=true;
                                    break;
                                }
                            }
                            if(found)
                                break;
                        }
                        if(!found)
                        {
                            if(uriC.objectId!=5 && uriC.objectId!=19 && uriC.objectId!=1 && uriC.objectId!=4 && uriC.objectId!=0)
                            {
                                transaction->mID=pTmp->msgid;
                                coap_set_header_token(transaction->message,pTmp->token, pTmp->tokenLen);
                                if(pContext->lwm2mContext->serverList!=NULL)
                                {
                                    transaction->peerH=pContext->lwm2mContext->serverList->sessionH;
                                    transaction->userData=pContext->lwm2mContext->serverList;
                                }
                                else
                                {
                                    transaction->peerH=NULL;
                                    transaction->userData=NULL;
                                }
                                //coap_set_header_observe(transaction->message, ++tokenvalue);
                                if(transaction->peerH!=NULL)
                                {
                                    found = true;
                                }
                            }
                        }
                        if(!found)
                        {
                            ctiot_funcv1_status_t atStatus[1]={0};
                            if(isObject19)
                                atStatus->baseInfo=ct_lwm2m_strdup(SS6);
                            else
                                atStatus->baseInfo=ct_lwm2m_strdup(SN6);
                            atStatus->extraInfo=&pTmp->msgid;
                            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS,AT_TO_MCU_STATUS,atStatus,0);
                            ctiot_funcv1_notify_event(MODULE_LWM2M, SEND_FAILED, NULL, 0);
                            ct_lwm2m_free(atStatus->baseInfo);
#ifdef  FEATURE_REF_AT_ENABLE
                            pContext->conDataStatus = SENT_FAIL;//add for *16*
#endif
                        }
                        else
                        {
                            prv_update_payload(transaction->message, &uriC, pTmp, &mediaType,&dataP);
                            if(isObject19)
                                transaction->callback = (lwm2m_transaction_callback_t)prv_sendcon;
                            else
                                transaction->callback = (lwm2m_transaction_callback_t)prv_sendcon_notify;
                            pContext->lwm2mContext->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(pContext->lwm2mContext->transactionList, transaction);
#ifndef  FEATURE_REF_AT_ENABLE
                            ctiot_funcv1_status_t atStatus[1]={0};
                            if(isObject19)
                                atStatus->baseInfo=ct_lwm2m_strdup(SS2);
                            else
                                atStatus->baseInfo=ct_lwm2m_strdup(SN2);
                            atStatus->extraInfo=&pTmp->msgid;
                            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS,AT_TO_MCU_STATUS,atStatus,0);
                            ct_lwm2m_free(atStatus->baseInfo);
#else                               
                            pContext->conDataStatus = SENT_WAIT_RESP;//add for *16*
#endif
                        }
                    }
                }
                else if(pTmp->mode==SENDMODE_NON || pTmp->mode==SENDMODE_NON_REL)
                {
                    pContext->send_thread_status.thread_status=THREAD_STATUS_BUSY;
                    lwm2m_printf("\r\nsending non packet\r\n");
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_2, P_INFO, 0, "sending non packet");
                    coap_packet_t message[1];
                    lwm2m_uri_t uriC;
                    #ifdef WITH_MBEDTLS
                    mbedtls_connection_t* sessionH;
                    #else
                    connection_t* sessionH;
                    #endif

                    bool found=false;
                    if(ctlwm2m_stringToUri((const char *)(pTmp->uri),strlen((const char *)(pTmp->uri)),&uriC)!=0)
                    {
                        bool isObject19=false;
                        if(uriC.objectId==19)
                            isObject19=true;
                        //lwm2m_printf("\r\pTmp->responseCode=%d\r\n",pTmp->responseCode);
                        coap_init_message(message, COAP_TYPE_NON, pTmp->responseCode, pTmp->msgid);
                        lwm2m_media_type_t mediaType=prv_get_media_type(pTmp->sendFormat);
                        //lwm2m_printf("\r\mediaType=%d\r\n",mediaType);

                        coap_set_header_content_type(message,mediaType);

                        prv_update_sendOption(message,pTmp);
                        prv_set_uri_option(message, &uriC);

                        lwm2m_observed_t * targetP;
                        for (targetP = pContext->lwm2mContext->observedList ; targetP != NULL ; targetP = targetP->next)
                        {
                            if(!prv_match_uri(uriC,targetP->uri))
                                continue;
                            lwm2m_watcher_t * watcherP;
                            for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
                            {
                                //lwm2m_printf("\r\nwatcherP = targetP->watcherList\r\n");
                                if (watcherP->active == true)
                                {
                                    //lwm2m_printf("\r\nwatcherP->active == true\r\n");
                                    watcherP->lastTime=ct_lwm2m_gettime();
                                    watcherP->lastMid=pTmp->msgid;
                                    sessionH=watcherP->server->sessionH;
                                    //coap_set_header_observe(message, watcherP->counter++);
                                    coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
                                    found=true;
                                    break;
                                }

                            }
                            if(found) break;
                        }
                        if(!found)
                        {
                            if(uriC.objectId!=5 && uriC.objectId!=19 && uriC.objectId!=1 && uriC.objectId!=4 && uriC.objectId!=0)
                            {
                                coap_set_header_token(message,pTmp->token, pTmp->tokenLen);
                                if(pContext->lwm2mContext->serverList!=NULL)
                                    sessionH=pContext->lwm2mContext->serverList->sessionH;
                                else
                                    sessionH=NULL;
                                //coap_set_header_observe(message, ++tokenvalue);
                                if(sessionH!=NULL)
                                {
                                    found = true;
                                }
                            }
                        }
                        if(!found)
                        {
                            ctiot_funcv1_status_t atStatus[1];
                            if(isObject19)
                                atStatus->baseInfo=ct_lwm2m_strdup(SS6);
                            else
                                atStatus->baseInfo=ct_lwm2m_strdup(SN6);
                            atStatus->extraInfo=&pTmp->msgid;
                            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS,AT_TO_MCU_STATUS,atStatus,0);
                            ct_lwm2m_free(atStatus->baseInfo);
                            ctiot_funcv1_notify_event(MODULE_LWM2M, SEND_FAILED, NULL, 0);
                        }
                        else
                        {
                            prv_update_payload(message, &uriC, pTmp, &mediaType,&dataP);
                            prv_send_message(pContext,message,sessionH,isObject19);
#ifdef FEATURE_REF_AT_ENABLE
                            ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_2_1, P_INFO, 0, "notify AT_TO_MCU_NSMI");
                            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_NSMI, (void *)&(pTmp->seqnum), NULL);
#endif
                            coap_free_payload(message);
                        }
                    }
                }
                else if(pTmp->mode == SENDMODE_ACK)
                {
                    pContext->send_thread_status.thread_status=THREAD_STATUS_BUSY;
                    lwm2m_printf("\r\nsending ack packet\r\n");
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_3, P_INFO, 0, "sending ack packet");
                    coap_packet_t message[1];
                    lwm2m_uri_t uriC;
                    #ifdef WITH_MBEDTLS
                    mbedtls_connection_t* sessionH;
                    #else
                    connection_t* sessionH;
                    #endif
                    if(ctlwm2m_stringToUri((const char *)(pTmp->uri),strlen((const char *)(pTmp->uri)),&uriC)!=0)
                    {
                        //lwm2m_printf("\r\pTmp->responseCode=%d\r\n",pTmp->responseCode);
                        coap_init_message(message, COAP_TYPE_ACK, pTmp->responseCode, pTmp->msgid);
                        lwm2m_media_type_t mediaType=prv_get_media_type(pTmp->sendFormat);
                        //lwm2m_printf("\r\mediaType=%d\r\n",mediaType);
                        coap_set_header_content_type(message,mediaType);
                        coap_set_header_token(message,pTmp->token,pTmp->tokenLen);
                        if(pTmp->observeMode==0||pTmp->observeMode==1)
                        {
                            coap_set_header_observe(message, pTmp->observeMode);
                        }
                        prv_update_sendOption(message,pTmp);
                        prv_set_uri_option(message,&uriC);
                        if(pTmp->responseCode==COAP_205_CONTENT && pTmp->observeMode==OBSERVE_WITH_DATA_FOLLOW)
                        {
                            prv_update_payload(message, &uriC, pTmp, &mediaType,&dataP);
                        }
                        lwm2m_server_t* server=pContext->lwm2mContext->serverList;
                        sessionH=server->sessionH;
                        if(sessionH!=NULL)
                        {
                            prv_send_message(pContext,message,sessionH,false);
                            coap_free_payload(message);
                        }
                        else
                        {
                            lwm2m_printf("prv_send_message:sessionH=NULL\r\n");
                        }
                        ctiot_funcv1_downdata_list *nodePtr = NULL;
                        ctiot_funcv1_coap_queue_remove(pContext->downdataList, message->mid, &nodePtr);
                        if(nodePtr!=NULL)
                        {
                            if(nodePtr->uri)
                            {
                                ct_lwm2m_free(nodePtr->uri);
                            }
                            ct_lwm2m_free(nodePtr);
                        }
                    }
                }
                if(dataP.value.asBuffer.buffer != NULL){
                    free(dataP.value.asBuffer.buffer);
                }
                while(pTmp->updata!=NULL)
                {
                    if(pTmp->updata->dataType == DATA_TYPE_STRING || pTmp->updata->dataType == DATA_TYPE_OPAQUE)
                    {
                        ct_lwm2m_free(pTmp->updata->u.asBuffer.buffer);
                    }
                    ctiot_funcv1_data_list* pTmpNode=pTmp->updata;
                    pTmp->updata=pTmpNode->next;
                    ct_lwm2m_free(pTmpNode);
                }
                if(pTmp->uri)
                {
                    ct_lwm2m_free(pTmp->uri);
                }
                ct_lwm2m_free(pTmp);
            }
 
            {
                time_t sec = 1;
                
                int result = 0;
                result = ct_lwm2m_step(pContext->lwm2mContext,&sec);
                if(result != 0){
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_3_1, P_INFO, 1, "ct_lwm2m_step meet (0x%x)error clear all",result);
                    if(result == CONNECT_FAILED){
                        ctiot_funcv1_status_t atStatus[1] = {0};
                        atStatus->baseInfo=ct_lwm2m_strdup(SR1);
                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                        ct_lwm2m_free(atStatus->baseInfo);
                        ctiot_funcv1_notify_event(MODULE_LWM2M, REG_FAILED_OTHER, NULL, 0);
                    }
                    ctiot_clear_all();
                    gRunningSendThread = 2;
                    break;
                }
                    
                //----1.5 检查下行缓存队列超时----
                uint64_t now = ct_lwm2m_gettime();
                ctiot_funcv1_downdata_list* nodePtr = (ctiot_funcv1_downdata_list*)pContext->downdataList->head;
                ctiot_funcv1_list_t * next = NULL;
                uint8_t ii = 0;
                //printf("after ct_lwm2m_step()\r\n");
                while (nodePtr != NULL && ii < MAX_DOWNDATA_QUEUE_CHECK_NUM)
                {
                    next = (ctiot_funcv1_list_t* )nodePtr->next;
                    if(nodePtr->recvtime + CTIOT_DOWNDATA_TIMEOUT < now )
                    {
                        ctiot_funcv1_coap_queue_remove(pContext->downdataList, nodePtr->msgid, &nodePtr);
                        //TODO TIMEOUT RESPONSE
                        if(nodePtr!=NULL)
                        {
                            coap_packet_t response[1] = {0};
                            coap_init_message(response, COAP_TYPE_ACK, COAP_NO_ERROR, nodePtr->msgid);
                            coap_set_header_token(response, nodePtr->token, nodePtr->tokenLen);
                            ct_message_send(pContext->lwm2mContext, response, pContext->lwm2mContext->serverList->sessionH);

                            if(nodePtr->uri)
                            {
                                ct_lwm2m_free(nodePtr->uri);
                            }
                            ct_lwm2m_free(nodePtr);
                        }
                    }
                    nodePtr = (ctiot_funcv1_downdata_list*)next;
                    ii ++;
                }

                ct_send_loop_callback(pContext);
                
                //----------------------------
                usleep(CTIOT_THREAD_TIMEOUT);//switch thread

            }
        }
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_send_thread_4, P_INFO, 0, "prv_send_thread exit");
    pContext->send_thread_status.thread_status=THREAD_STATUS_FREE;
    pContext->send_thread_status.thread_started = false;
    thread_exit(NULL);
}

static void prv_recv_thread(void* arg)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_recv_thread_0, P_INFO, 0, "enter");
    ctiot_funcv1_context_t* pContext=(ctiot_funcv1_context_t*)arg;
    if(pContext==NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }
#ifdef WITH_MBEDTLS
    if(dtls_buffer == NULL){
        dtls_buffer = (uint8_t*)malloc(CTIOT_MAX_PACKET_SIZE);
    }
#endif
    while(1)
    {
        if(gRunningRecvThread==0)
        {
            usleep(CTIOT_THREAD_TIMEOUT);
            continue;
        }
        else if(gRunningRecvThread==2)
        {
            break;
        }
        prv_coap_yield(pContext);
        //thread switch
        usleep(CTIOT_THREAD_TIMEOUT);
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_recv_thread_1, P_INFO, 0, "prv_recv_thread exit");
    pContext->recv_thread_status.thread_status=THREAD_STATUS_FREE;
    pContext->recv_thread_status.thread_started=false;
    thread_exit(NULL);
}

static uint16_t prv_start_thread(ctiot_funcv1_context_t* pContext)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext==NULL)
        pContext=ctiot_funcv1_get_context();
    int err = 0;
    osMutexAcquire(startThreadMutex, osWaitForever);
    ECOMM_TRACE(UNILOG_CTLWM2M, prv_start_thread_0, P_INFO, 2, "send(%d) recv(%d)", gRunningSendThread, gRunningRecvThread);
    
    if(gRunningSendThread != 1 && gRunningRecvThread != 1)
    {
        pContext->updataList=ctiot_funcv1_coap_queue_init(CTIOT_MAX_QUEUE_SIZE);
        pContext->downdataList=ctiot_funcv1_coap_queue_init(CTIOT_MAX_QUEUE_SIZE);
//#ifdef  FEATURE_REF_AT_ENABLE
        pContext->recvdataList=ctiot_funcv1_coap_queue_init(CTIOT_MAX_RECV_QUEUE_SIZE);
//#endif
        ECOMM_TRACE(UNILOG_CTLWM2M, prv_start_thread_1, P_INFO, 0, "start these two thread");
    
        err = thread_create(&nRtid, NULL, prv_recv_thread, (void*)pContext, LWM2M_RECV_TASK);
        if (err != 0)
        {
            result=CTIOT_EB_OTHER;
            goto exit;
        }
        err = thread_create(&nStid, NULL, prv_send_thread, (void*)pContext, LWM2M_SEND_TASK);
        if (err != 0)
        {
            gRunningRecvThread=2;
            gRunningSendThread=2;
            result=CTIOT_EB_OTHER;
            goto exit;
        }
        gRunningRecvThread=1;
        gRunningSendThread=1;
    }

    osMutexRelease(startThreadMutex);

exit:
    return result;
}

/***************************************************************
************************Global functions************************
****************************************************************/

void ctiot_funcv1_save_context()
{
    ctiot_funcv1_context_t* pContext;
    pContext=ctiot_funcv1_get_context();
    c2f_encode_context(pContext);
}

uint16_t ctiot_funcv1_set_mod(ctiot_funcv1_context_t* pTContext, char idAuthMode, char natType, char onUQMode, char autoHeartBeat, char wakeupNotify, char protocolMode)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_context_t* pContext=pTContext;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }

    if(idAuthMode!=(char)-1)
    {
        pContext->idAuthMode = (ctiot_funcv1_id_auth_mode_e)idAuthMode;
    }
    if(natType!=(char)-1)
    {
        pContext->natType = (ctiot_funcv1_nat_type_e)natType;
    }
    if(onUQMode!=(char)-1)
    {
        pContext->onUqMode = (ctiot_funcv1_on_uq_mode_e)onUQMode;
    }
    if(autoHeartBeat!=(char)-1)
    {
        pContext->autoHeartBeat = (ctiot_funcv1_auto_heartbeat_e)autoHeartBeat;
    }
    if(wakeupNotify!=(char)-1)
    {
        pContext->wakeupNotify = (ctiot_funcv1_wakeup_notify_e)wakeupNotify;
    }
    if(protocolMode != (char)-1)
    {
        pContext->protocolMode = (ctiot_funcv1_protocol_mode_e)protocolMode;
    }
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_mod(ctiot_funcv1_context_t* pContext,uint8_t* idAuthMode,uint8_t* natType,uint8_t* onUQMode,uint8_t* autoHeartBeat,uint8_t* wakeupNotify, uint8_t* protocolMode)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(idAuthMode!=NULL)
    {
        *idAuthMode=pContext->idAuthMode ;
    }
    if(natType!=NULL)
    {
        *natType=pContext->natType;
    }
    if(autoHeartBeat!=NULL)
    {
        *autoHeartBeat=pContext->autoHeartBeat;
    }
    if(wakeupNotify!=NULL)
    {
        *wakeupNotify=pContext->wakeupNotify;
    }
    if(onUQMode!=NULL)
    {
        *onUQMode=pContext->onUqMode;
    }
    if(protocolMode!=NULL)
    {
        *protocolMode=pContext->protocolMode;
    }
    return result;
}


uint16_t ctiot_funcv1_set_pm(ctiot_funcv1_context_t* pTContext,char* serverIP,uint16_t port, int32_t lifeTime, uint32_t reqhandle)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_context_t* pContext = pTContext;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_set_pm_1, P_INFO, "pContext->pskid=%s",(uint8_t*)pContext->pskid);
    ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_set_pm_2, P_INFO, "pContext->psk=%s",(uint8_t*)pContext->psk);
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_set_pm_3, P_INFO, 1, "pContext->pskLen=%d",pContext->pskLen);

    if(lifeTime != -1)
    {
        pContext->lifetime = lifeTime;
    }
    if(serverIP != NULL)
    {
        if(pContext->serverIP != NULL)
            ct_lwm2m_free(pContext->serverIP);
        pContext->serverIP = ct_lwm2m_strdup(serverIP);
    }
    pContext->port = port;
    pContext->reqhandle = reqhandle;
    
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_pm(ctiot_funcv1_context_t* pContext,char* serverIP,uint16_t* port,uint32_t* lifeTime)
{
    uint16_t result = CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }
    if(port != NULL)
    {
        *port = pContext->port;
    }
    if(serverIP != NULL && (pContext->serverIP != NULL))
    {
        memcpy(serverIP,pContext->serverIP,strlen(pContext->serverIP));
    }
    if(lifeTime != NULL)
    {
        *lifeTime = pContext->lifetime;
    }
    return result;
}

#ifdef  FEATURE_REF_AT_ENABLE

uint16_t ctiot_funcv1_reset_dtls(ctiot_funcv1_context_t* pTContext)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }

    if(pContext->lwm2mContext != NULL)
    {
#ifdef WITH_MBEDTLS
        lwm2m_context_t* lwm2mCxt = pContext->lwm2mContext;
        ctiot_funcv1_client_data_t * dataP = (ctiot_funcv1_client_data_t *)lwm2mCxt->userData;
        if(dataP != NULL){
            if(dataP->handshakeResult == HANDSHAKE_OVER){
                dataP->resetHandshake = TRUE;
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_reset_dtls_1, P_INFO, 0, "has complete handshake,set it init, rehandshake next send");
            }else if(dataP->handshakeResult == HANDSHAKE_ING){
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_reset_dtls_2, P_INFO, 0, "in handshaking, just return OK");
            }
            result = CTIOT_NB_SUCCESS;
        }
#else
        result = CTIOT_NB_SUCCESS;
#endif
    }
    else
    {
        result = CTIOT_EA_OPERATION_FAILED_NOSESSION;
    }

    return result;
}

uint16_t ctiot_funcv1_get_dtls_status(ctiot_funcv1_context_t* pTContext, uint8_t * status)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result = CTIOT_NB_SUCCESS;
    
    *status = 1;//no handshake
    
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }

    if(pContext->lwm2mContext != NULL)
    {
#ifdef WITH_MBEDTLS
        lwm2m_context_t* lwm2mCxt = pContext->lwm2mContext;
        ctiot_funcv1_client_data_t * dataP = (ctiot_funcv1_client_data_t *)lwm2mCxt->userData;
        ECOMM_TRACE(UNILOG_CTLWM2M,  ctiot_funcv1_get_dtls_status_1, P_INFO, 1, "dataP->handshakeResult=%d",dataP->handshakeResult);
        if(dataP != NULL){
            switch(dataP->handshakeResult)
            {
            case HANDSHAKE_INIT:
                *status = 1;//no handshake
                break;
            case HANDSHAKE_OVER:
                *status = 0;//handshake complete
                break;
            case HANDSHAKE_FAIL:
                *status = 3;//handshake fail
                break;
            case HANDSHAKE_ING:
                *status = 2;//handshakeing
                break;
            default:
                *status = 1;//no handshake
                break;
            }
        }
#endif
    }
    else
    {
        result = CTIOT_EA_OPERATION_FAILED_NOSESSION;
    }

    return result;
}

uint16_t ctiot_funcv1_get_bs_server(ctiot_funcv1_context_t* pContext,char* serverIP,uint16_t* port)
{
    uint16_t result = CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }
    if(port != NULL)
    {
        *port = pContext->bsPort;
    }
    if(serverIP != NULL && (pContext->bsServerIP != NULL))
    {
        memcpy(serverIP,pContext->bsServerIP,strlen(pContext->bsServerIP));
    }
    return result;
}

uint16_t ctiot_funcv1_set_bs_server(ctiot_funcv1_context_t* pTContext, char* serverIP, uint16_t port, uint32_t reqHandle)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_context_t* pContext = pTContext;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(serverIP != NULL)
    {
        if(pContext->bsServerIP != NULL)
            ct_lwm2m_free(pContext->bsServerIP);
        pContext->bsServerIP = ct_lwm2m_strdup(serverIP);
    }
    pContext->bsPort = port;
    pContext->reqhandle = reqHandle;

    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_del_serverIP(ctiot_funcv1_context_t* pTContext,char* serverIP,uint16_t port)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_context_t* pContext = pTContext;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(serverIP != NULL && pContext->serverIP)
    {
        if (strcmp(serverIP, pContext->serverIP) == NULL){
            ct_lwm2m_free(pContext->serverIP);
            pContext->serverIP = NULL;
            c2f_clear_serverIP(pContext);
        }
    }
    if(serverIP != NULL && pContext->bsServerIP)
    {
        if (strcmp(serverIP, pContext->bsServerIP) == NULL){
            ct_lwm2m_free(pContext->bsServerIP);
            pContext->bsServerIP = NULL;
            c2f_clear_bsServerIP(pContext);
        }
    }

    return result;
}

uint16_t ctiot_funcv1_get_ep(ctiot_funcv1_context_t* pContext, char* endpoint)
{
    uint16_t result = CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }
    
    if(endpoint != NULL && (pContext->endpoint != NULL))
    {
        strcpy(endpoint,pContext->endpoint);
    }
    return result;
}

uint16_t ctiot_funcv1_set_ep(ctiot_funcv1_context_t* pContext, char* endpoint)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(endpoint != NULL)
    {
        if(pContext->endpoint != NULL)
            ct_lwm2m_free(pContext->endpoint);
        pContext->endpoint = ct_lwm2m_strdup(endpoint);
    }
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_lifetime(ctiot_funcv1_context_t* pContext, uint32_t* lifetime)
{
    uint16_t result = CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }
    
    
    if(lifetime != NULL)
    {
        *lifetime = pContext->lifetime;
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_get_lifetime_1, P_INFO, 2, "get lifeTime=%d, pContext->lifetime=%d",*lifetime,pContext->lifetime);
    return result;
}

uint16_t ctiot_funcv1_set_lifetime(ctiot_funcv1_context_t* pContext, uint32_t lifeTime)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(lifeTime == 0){
        pContext->autoHeartBeat = NO_AUTO_HEARTBEAT;
        pContext->lifetime = 0;
    }else if(lifeTime>0 && lifeTime<900){
        pContext->autoHeartBeat = AUTO_HEARTBEAT;
        pContext->lifetime = 900;
    }else{
        pContext->autoHeartBeat = AUTO_HEARTBEAT;
        pContext->lifetime = lifeTime;
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_set_lifetime_1, P_INFO, 2, "set lifeTime=%d, pContext->lifetime=%d",lifeTime,pContext->lifetime);
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_dlpm(ctiot_funcv1_context_t* pContext, uint32_t* buffered, uint32_t* received, uint32_t* dropped)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(pContext->recvdataList != NULL){
        *buffered = pContext->recvdataList->msg_count;
        *received = pContext->recvdataList->recv_msg_num;
        *dropped = pContext->recvdataList->del_msg_count;
    }else{
        *buffered = 0;
        *received = 0;
        *dropped = 0;
    }
    return result;
}

uint16_t ctiot_funcv1_get_ulpm(ctiot_funcv1_context_t* pContext, uint32_t* pending, uint32_t*sent, uint32_t*error)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(pContext->updataList != NULL){
        *pending = pContext->updataList->msg_count;
        *sent = pContext->updataList->del_msg_count;
    }else{
        *pending = 0;
        *sent = 0;
    }
    *error = 0;
    return result;
}

uint16_t ctiot_funcv1_get_dtlstype(ctiot_funcv1_context_t* pContext, uint8_t* type, uint8_t* natType)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    *type = pContext->dtlsType;
    *natType = pContext->natType;
    
    return result;
}

uint16_t ctiot_funcv1_set_dtlstype(ctiot_funcv1_context_t* pContext, uint8_t type, uint8_t natType)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    pContext->dtlsType = type;
    pContext->natType = natType;
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_set_nsmi_mode(ctiot_funcv1_context_t* pTContext, uint8_t type)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext == NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    // clear when reboot
    pContext->nsmiFlag = type;
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_nsmi_mode(ctiot_funcv1_context_t* pContext, uint8_t* type)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    *type = pContext->nsmiFlag;
    return result;
}

BOOL ctiot_funcv1_get_data_status(ctiot_funcv1_context_t* pTContext,uint8_t* status,uint8_t* seqNum)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }

    *status = (uint8_t)pContext->conDataStatus;
    *seqNum = pContext->seqNum;

    return TRUE;
}

#endif

uint16_t ctiot_funcv1_set_nnmi_mode(ctiot_funcv1_context_t* pTContext, uint8_t type)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext == NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    // clear when reboot
    pContext->nnmiFlag = type;
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_nnmi_mode(ctiot_funcv1_context_t* pContext, uint8_t* type)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    *type = pContext->nnmiFlag;
    return result;
}

uint16_t ctiot_funcv1_get_recvData(ctiot_funcv1_context_t* pTContext, uint16_t* datalen, uint8_t** dataStr)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_recvdata_list* pRecv = NULL;

    if(pContext == NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    
    if(pContext->recvdataList == NULL){
        result=CTIOT_EB_NO_RECV_DATA;
        goto exit;
    }
    
    pRecv = (ctiot_funcv1_recvdata_list*)ctiot_funcv1_coap_queue_get(pContext->recvdataList);
    if(pRecv == NULL)
    {
        result=CTIOT_EB_NO_RECV_DATA;
        goto exit;
    }
    *datalen = pRecv->recvdataLen;
    *dataStr = pRecv->recvdata;
    ct_lwm2m_free(pRecv);
    if(pContext->recvdataList->msg_count == 0){
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_get_recvData_1, P_INFO, 0, "no more recv data can slp2 now");
        ctiot_recvlist_slp_set(TRUE);//no more recv data can slp2
    }
exit:
    return result;
}

uint16_t ctiot_funcv1_session_init(ctiot_funcv1_context_t* pContext)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    char serverUri[CTIOT_MAX_URI_LEN]={0};
    uint16_t i=0;
    bool bBroadcast=true;
    char *ep_name = NULL;
    bool bsMode = false;
    
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_0, P_INFO, 0, "enter" );
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    
    if(pContext->lwm2mContext!=NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_1, P_INFO, 0, "session already initialized return" );
        lwm2m_printf("session already initialized\r\n");
        goto exit;
    }
    
    if(pContext->serverIP != NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_0_1, P_INFO, 1, "use serverIP, pContext->port=%d",pContext->port );
        if(pContext->port == 5684)
            sprintf (serverUri, "coaps://%s:%d", pContext->serverIP, pContext->port);
        else{
            sprintf (serverUri, "coap://%s:%d", pContext->serverIP, pContext->port);
        }
    }
#ifdef  FEATURE_REF_AT_ENABLE
    else if(pContext->bsServerIP != NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_0_2, P_INFO, 0, "use bsServerIP" );
        bsMode = true;
        if(pContext->bsPort == 5684)
            sprintf (serverUri, "coaps://%s:%d", pContext->bsServerIP, pContext->bsPort);
        else{
            sprintf (serverUri, "coap://%s:%d", pContext->bsServerIP, pContext->bsPort);
        }
    }
#endif
    else
    {
        result = CTIOT_EA_PARAM_NOT_INTIALIZED;
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_0_3, P_INFO, 0, "no serverIP and bsServerIP" );
        goto exit;
    }
    pContext->chipInfo->cState=NETWORK_CONNECTED;
    pContext->fotaFlag = 1;

    pContext->clientInfo.addressFamily=AF_INET;
    pContext->clientInfo.sock=0;
    pContext->clientInfo.sock=ct_create_socket(CTIOT_DEFAULT_LOCAL_PORT, pContext->clientInfo.addressFamily);
    pContext->localPort=atoi(CTIOT_DEFAULT_LOCAL_PORT);
    if(pContext->clientInfo.sock < 0)
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("pContext->clientInfo.sock<0\r\n");
        goto exit;
    }

    setsockopt(pContext->clientInfo.sock,SOL_SOCKET, SO_BROADCAST, (const char*)&bBroadcast,sizeof(bool));

#if defined WITH_MBEDTLS
    pContext->clientInfo.natType = pContext->natType;
#endif

#ifndef  FEATURE_REF_AT_ENABLE
    if(pContext->port == 5684)
        ctiotObjArray[i] = ct_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, pContext->pskid, /*initInfo.u.pskInfo.pskStr*/pContext->psk, pContext->pskLen/*initInfo.u.pskInfo.pskLen*/, false);
    else
        ctiotObjArray[i] = ct_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, NULL, NULL, 0, false);
#else
    if(bsMode == true){
        if(pContext->bsPort == 5684)
            ctiotObjArray[i] = ct_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, pContext->pskid, /*initInfo.u.pskInfo.pskStr*/pContext->psk, pContext->pskLen/*initInfo.u.pskInfo.pskLen*/, true);
        else
            ctiotObjArray[i] = ct_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, NULL, NULL, 0, true);
    }else{
        if(pContext->port == 5684)
            ctiotObjArray[i] = ct_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, pContext->pskid, /*initInfo.u.pskInfo.pskStr*/pContext->psk, pContext->pskLen/*initInfo.u.pskInfo.pskLen*/, false);
        else
            ctiotObjArray[i] = ct_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, NULL, NULL, 0, false);
    }
#endif
    //OBJ = 0
    if(ctiotObjArray[i]==NULL)
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("ctiotObjArray[%d]==NULL\r\n",i);
        goto exit;
    }
    pContext->clientInfo.securityObjP=ctiotObjArray[i];
    //OBJ = 1
    if(pContext->onUqMode == DISABLE_UQ_MODE)
        ctiotObjArray[++i] = ct_get_server_object(CTIOT_DEFAULT_SERVER_ID, "U"/*default binding mode*/, pContext->lifetime, false);
    else
        ctiotObjArray[++i] = ct_get_server_object(CTIOT_DEFAULT_SERVER_ID, "UQ"/*default binding mode*/, pContext->lifetime, false);
    
    if (NULL == ctiotObjArray[i])
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("ctiotObjArray[%d]==NULL\r\n",i);
        prv_free_objects(i+1);
        goto exit;
    }
    pContext->clientInfo.serverObject=ctiotObjArray[i];
    #ifdef WITH_FOTA
    //OBJ = 3
    ctiotObjArray[++i] = ct_get_object_device();
    if (NULL == ctiotObjArray[i])
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("ctiotObjArray[%d]==NULL\r\n",i);
        prv_free_objects(i+1);
        goto exit;
    }

    //OBJ = 5
    ctiotObjArray[++i] = ct_get_object_firmware();
    if (NULL == ctiotObjArray[i])
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("ctiotObjArray[%d]==NULL\r\n",i);
        prv_free_objects(i+1);
        goto exit;
    }
    #endif
    //OBJ = 4
    ctiotObjArray[++i] = ct_get_object_conn_m();
    if (NULL == ctiotObjArray[i])
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("ctiotObjArray[%d]==NULL\r\n",i);
        prv_free_objects(i+1);
        goto exit;
    }

    //OBJ = 19
    ctiotObjArray[++i] = get_data_report_object();
    if (NULL == ctiotObjArray[i])
    {
        result = CTIOT_EB_OTHER;
        lwm2m_printf("ctiotObjArray[%d]==NULL\r\n",i);
        prv_free_objects(i+1);
        goto exit;
    }
    #if 0
    //Customer OBJ
    if(pContext->objectInstList != NULL)
    {
        for(int ii = 0; ii < MAX_MCU_OBJECT; ii ++)
        {
            if(objInsArray[ii].objId == 0) break;
            ctiotObjArray[++i] = get_mcu_object(ii,objInsArray);
            if (NULL == ctiotObjArray[i])
            {
                result = CTIOT_EB_OTHER;
                prv_free_objects(i+1);
                goto exit;
            }
        }
    }
    #endif
    pContext->lwm2mContext = ct_lwm2m_init(&(pContext->clientInfo));
#ifdef WITH_MBEDTLS
    pContext->clientInfo.lwm2mH = pContext->lwm2mContext;
#endif
    
    ep_name = (char *)ct_lwm2m_malloc(ENDPOINT_NAME_SIZE+1);
    memset(ep_name,0,ENDPOINT_NAME_SIZE+1);
#ifndef  FEATURE_REF_AT_ENABLE
    result = prv_get_ep(pContext,ep_name);
    if( result != 0 )
    {
        ct_lwm2m_free(ep_name);
        prv_free_objects(i+1);
        ct_lwm2m_free(pContext->lwm2mContext);
        pContext->lwm2mContext=NULL;
        result = CTIOT_ED_MODE_ERROR;

        return result;
    }
#else
    if(pContext->endpoint==0)//hasn't use at+qcfg="LWM2M/EndpointName",xxx set endpointname
    {
        sprintf(ep_name,"%s",pContext->chipInfo->cImei);//use imei as endpointname
        ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_session_init_1_1, P_INFO, "no set endpoint catch imei:ep_name:%s", (uint8_t*)ep_name);
    }
    else
    {
        strcpy(ep_name, pContext->endpoint);
        ECOMM_STRING(UNILOG_CTLWM2M, ctiot_funcv1_session_init_1_2, P_INFO, "has set endpoint ep_name:%s", (uint8_t*)ep_name);
    }
#endif
    
    result = ct_lwm2m_configure(pContext->lwm2mContext, ep_name, NULL, NULL, i+1, ctiotObjArray);
    if(result!= 0)
    {
        prv_free_objects(i+1);
        ct_lwm2m_free(pContext->lwm2mContext);
        result = CTIOT_EB_OTHER;//ct_lwm2m_configure fail
        lwm2m_printf("result!= 0\r\n");
        pContext->lwm2mContext=NULL;
        ct_lwm2m_free(ep_name);
        return result;
    }
    else
    {
        ct_lwm2m_free(ep_name);
    }
    
    if(pContext->lwm2mContext->observe_mutex == NULL)
    {
        pContext->lwm2mContext->observe_mutex = osMutexNew(NULL);
    }
    
    if(pContext->lwm2mContext->transactMutex == NULL)
    {
        pContext->lwm2mContext->transactMutex = osMutexNew(NULL);
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_2, P_INFO, 1, "pContext->bootFlag:%d", pContext->bootFlag);
    if((pContext->regFlag == 1 && pContext->restoreFlag != 1) || pContext->bootFlag == BOOT_FOTA_REBOOT || pContext->ipReadyRereg)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_5, P_INFO, 0, "in auto reg/no ip login status/fota reboot to register platform");
        f2c_encode_context(pContext);
        prv_start_thread(pContext);
        pContext->loginStatus = UE_LOGINING;
    }
    else if(pContext->bootFlag == BOOT_LOCAL_BOOTUP
#ifdef WITH_FOTA_RESUME
        || pContext->bootFlag == BOOT_FOTA_BREAK_RESUME)
#else
        )
#endif
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_2_1, P_INFO, 0, "get context and call prv_recover_connect");
        int32_t ret = f2c_encode_context(pContext);
        if(ret == NV_OK && pContext->lwm2mContext->serverList != NULL && prv_recover_connect(pContext->serverIP,pContext->port,&pContext->clientInfo))
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_3_1, P_INFO, 0, "restore the ctlwm2m prv_recover_connect success");
            if(pContext->wakeupNotify == NOTICE_MCU)
            {
                ctiot_funcv1_status_t wakeStatus[1]={0};
                wakeStatus->baseInfo=ct_lwm2m_strdup(SE3);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
                ct_lwm2m_free(wakeStatus->baseInfo);
            }
            prv_start_thread(pContext);
            pContext->loginStatus = UE_LOGINED;
        }
        else
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_session_init_3_2, P_INFO, 0, "restore failed clear all");
            ctiot_funcv1_update_bootflag(pContext,0);
            prv_free_resources(pContext);
            ct_lwm2m_close(pContext->lwm2mContext);
            pContext->lwm2mContext=NULL;
            pContext->loginStatus=UE_NOT_LOGINED;
            if(pContext->wakeupNotify == NOTICE_MCU)
            {
                ctiot_funcv1_status_t wakeStatus[1]={0};
                wakeStatus->baseInfo=ct_lwm2m_strdup(SL1);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
                ct_lwm2m_free(wakeStatus->baseInfo);
            }
            close(pContext->clientInfo.sock);
            pContext->clientInfo.sock=0;
            ct_connection_free(pContext->clientInfo.connList);
            pContext->clientInfo.connList     = NULL;
            pContext->clientInfo.securityObjP = NULL;
            pContext->clientInfo.serverObject = NULL;
            result = CTIOT_EB_OTHER;
        }
    }
exit:
    return result;
}

uint16_t ctiot_funcv1_reg(ctiot_funcv1_context_t* pTContext)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    ctiot_funcv1_chip_info_ptr pChipInfo;
    NmAtiSyncRet netStatus;
    ctiot_funcv1_context_t* pContext=pTContext;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_reg_1, P_INFO, 0, "enter");

    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    
    if(pContext->loginStatus == UE_NO_IP)
    {
        result = CTIOT_EB_NO_IP;
        goto exit;
    }
    
#ifndef  FEATURE_REF_AT_ENABLE
    if(pContext->bootFlag == BOOT_LOCAL_BOOTUP)
    {
        result = CTIOT_EA_ALREADY_LOGIN;
        goto exit;
    }
#else
    if(pContext->bootFlag == BOOT_LOCAL_BOOTUP)
    {
        result = CTIOT_NB_SUCCESS;
        goto exit;
    }
#endif
    pChipInfo=pContext->chipInfo;
    if(pChipInfo==NULL)
    {
        result= CTIOT_EB_OTHER;
        goto exit;
    }
    /*
    appGetNetInfoSync(0, &netStatus);
    if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
    {
       pChipInfo->cState = NETWORK_CONNECTED;
    }
    */
    NmAtiGetNetInfoReq  getNetInfoReq;
    memset(&getNetInfoReq, 0, sizeof(NmAtiGetNetInfoReq));
    getNetInfoReq.cid = 0;
    
    NetmgrAtiSyncReq(NM_ATI_SYNC_GET_NET_INFO_REQ, (void *)&getNetInfoReq, &netStatus);
    if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
    {
       pChipInfo->cState = NETWORK_CONNECTED;
    }


    if(pChipInfo->cState == NETWORK_JAM)
    {
        result = CTIOT_EA_NETWORK_TRAFFIC;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_RECOVERING)
    {
        result = CTIOT_EA_ENGINE_EXCEPTION;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_MANUAL_RESTART)
    {
        result = CTIOT_EA_ENGINE_REBOOT;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNCONNECTED)
    {
        result = CTIOT_EA_CONNECT_USELESS_TEMP;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNAVAILABLE)
    {
        result = CTIOT_EA_CONNECT_FAILED_SIMCARD;//需进一步确认
        goto exit;
    }
    else if(pChipInfo->cState==NETWORK_CONNECTED)
    {
        if(pContext->loginStatus == UE_LOGINED)
        {
            result = CTIOT_EA_ALREADY_LOGIN;
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINING)
        {
            result = CTIOT_EA_LOGIN_PROCESSING;
            goto exit;
        }
        else if(pContext->loginStatus == UE_NOT_LOGINED || pContext->loginStatus == UE_LOG_FAILED || pContext->loginStatus == UE_LOG_OUT)
        {
            if(pContext->idAuthMode != AUTHMODE_IMEI && pContext->idAuthMode != AUTHMODE_IMEI_IMSI && (pContext->idAuthStr == NULL || strlen(pContext->idAuthStr)==0))
            {
                result = CTIOT_EA_OPERATION_NO_AUTHSTR;
                goto exit;
            }
            if(appGetImeiNumSync(pContext->chipInfo->cImei) != NBStatusSuccess)
            {
                result = CTIOT_EB_OTHER;
                goto exit;
            }
            result=ctiot_funcv1_session_init(pContext);
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_reg_2, P_INFO, 1, "ctiot_session_init:result=%d",result);
            if(result!=CTIOT_NB_SUCCESS)
            {
                goto exit;
            }

            if(pContext->lwm2mContext==NULL)
            {
                result=CTIOT_EA_OPERATION_FAILED_NOSESSION;
                goto exit;
            }
            if(prv_check_register_params(pContext)==false)
            {
                result =CTIOT_EA_PARAM_NOT_INTIALIZED;
                goto exit;
            }
            
            prv_start_thread(pContext);

            pContext->lwm2mContext->state = STATE_INITIAL;
            pContext->loginStatus         = UE_LOGINING;
        }
        else
        {
            result = CTIOT_EB_OTHER;
        }
    }
    else
    {
        result = CTIOT_EB_OTHER;
    }
exit:
    return result;
}

//extern void ct_lwm2m_deregister(lwm2m_context_t * context);
//extern void ct_prv_deleteTransactionList(lwm2m_context_t * context);
//extern void prv_deleteServerList(lwm2m_context_t * context);
//extern void prv_deleteBootstrapServerList(lwm2m_context_t * context);
//extern void prv_deleteObservedList(lwm2m_context_t * contextP);

uint16_t ctiot_funcv1_dereg(ctiot_funcv1_context_t* pContext, ps_event_callback_t eventcallback)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext == NULL)
        pContext = ctiot_funcv1_get_context();

    if(pContext->loginStatus == UE_NO_IP)
    {
        result = CTIOT_EB_NO_IP;
        goto exit;
    }
    
    if(pContext->loginStatus != UE_LOGINED)
    {
        result=CTIOT_EA_NOT_LOGIN;
        goto exit;
    }

    
    if(pContext->lwm2mContext==NULL)
    {
        result=CTIOT_EA_OPERATION_FAILED_NOSESSION;
        goto exit;
    }

    pContext->loginStatus = UE_LOG_OUT;

    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_dereg_1, P_INFO, 0, "free all objects");

    //deregisterPSEventCallback(eventcallback);
    //1. exit thread send/recv firstly
    prv_stop_recv_thread();
    prv_stop_send_thread();
    ct_lwm2m_deregister(pContext->lwm2mContext);
    while((pContext->recv_thread_status.thread_started != false) || (pContext->send_thread_status.thread_started != false))
        usleep(CTIOT_DEREG_WAIT_TIME);
    prv_deleteServerList(pContext->lwm2mContext);
    prv_deleteBootstrapServerList(pContext->lwm2mContext);
    prv_deleteObservedList(pContext->lwm2mContext);
    ct_lwm2m_free(pContext->lwm2mContext->endpointName);
    
    osMutexDelete(pContext->lwm2mContext->observe_mutex);
    if (pContext->lwm2mContext->msisdn != NULL)
    {
        ct_lwm2m_free(pContext->lwm2mContext->msisdn);
    }
    if (pContext->lwm2mContext->altPath != NULL)
    {
        ct_lwm2m_free(pContext->lwm2mContext->altPath);
    }
    ct_prv_deleteTransactionList(pContext->lwm2mContext);
    osMutexDelete(pContext->lwm2mContext->transactMutex);
    //2. release all objects
    lwm2m_printf("free all objects\r\n");
    prv_free_resources(pContext);
    ct_lwm2m_free(pContext->lwm2mContext);
    
    pContext->lwm2mContext = NULL;

    //3. 清空uplist和downlist 待添加
    lwm2m_printf("free message queue list\r\n");
    prv_clear_uplist(pContext);
    prv_clear_downlist(pContext);
    ctiot_funcv1_coap_queue_free(&(pContext->updataList));
    ctiot_funcv1_coap_queue_free(&(pContext->downdataList));
//#ifdef  FEATURE_REF_AT_ENABLE
    prv_clear_recvdatalist(pContext);
    ctiot_funcv1_coap_queue_free(&(pContext->recvdataList));
//#endif
    //4. release socket and connlist
    lwm2m_printf("close server socekt and free connlist\r\n");
    close(pContext->clientInfo.sock);
    pContext->clientInfo.sock=0;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_dereg_2, P_INFO, 1, "connection free 0x%x",pContext->clientInfo.connList);
    ct_connection_free(pContext->clientInfo.connList);
    pContext->clientInfo.connList     = NULL;
    pContext->clientInfo.securityObjP = NULL;
    pContext->clientInfo.serverObject = NULL;

    //5. clear bootflag
    ctiot_funcv1_update_bootflag(pContext,0);

    //6. notify MCU
#ifndef  FEATURE_REF_AT_ENABLE
    ctiot_funcv1_status_t pTmpStatus;
    pTmpStatus.baseInfo     = SD0;
    pTmpStatus.extraInfo    = NULL;
    pTmpStatus.extraInfoLen = 0;
    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, &pTmpStatus, 0);
#else
    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL, 1);
#endif

    ctiot_funcv1_notify_event(MODULE_LWM2M, DEREG_SUCCESS, NULL, 0);

    // 7. vote to sleep and update flash
    c2f_encode_context(pContext);
    ctiot_funcv1_sleep_vote(SYSTEM_STATUS_FREE);
exit:
    return result;
}

void ctiot_funcv1_clear_session(BOOL saveToNV)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_clear_session_1, P_INFO, 0, "enter");
    ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();
    prv_stop_recv_thread();
    prv_stop_send_thread();
    while((pContext->recv_thread_status.thread_started != false) || (pContext->send_thread_status.thread_started != false))
        usleep(CTIOT_DEREG_WAIT_TIME);
    pContext->loginStatus = UE_NO_IP;
    prv_deleteServerList(pContext->lwm2mContext);
    prv_deleteBootstrapServerList(pContext->lwm2mContext);
    prv_deleteObservedList(pContext->lwm2mContext);
    ct_lwm2m_free(pContext->lwm2mContext->endpointName);
    
    osMutexDelete(pContext->lwm2mContext->observe_mutex);
    if (pContext->lwm2mContext->msisdn != NULL)
    {
        ct_lwm2m_free(pContext->lwm2mContext->msisdn);
    }
    if (pContext->lwm2mContext->altPath != NULL)
    {
        ct_lwm2m_free(pContext->lwm2mContext->altPath);
    }
    ct_prv_deleteTransactionList(pContext->lwm2mContext);
    osMutexDelete(pContext->lwm2mContext->transactMutex);
    prv_free_resources(pContext);
    ct_lwm2m_free(pContext->lwm2mContext);
    pContext->lwm2mContext = NULL;
    
    prv_clear_uplist(pContext);
    prv_clear_downlist(pContext);
    ctiot_funcv1_coap_queue_free(&(pContext->updataList));
    ctiot_funcv1_coap_queue_free(&(pContext->downdataList));
//#ifdef  FEATURE_REF_AT_ENABLE
    prv_clear_recvdatalist(pContext);
    ctiot_funcv1_coap_queue_free(&(pContext->recvdataList));
//#endif
    
    close(pContext->clientInfo.sock);
    pContext->clientInfo.sock=0;
    ct_connection_free(pContext->clientInfo.connList);
    pContext->clientInfo.connList     = NULL;
    pContext->clientInfo.securityObjP = NULL;
    pContext->clientInfo.serverObject = NULL;
    if(saveToNV){
        c2f_encode_context(pContext);
    }
}

uint16_t ctiot_funcv1_update_reg(ctiot_funcv1_context_t* pTContext,uint16_t*msgId, bool withObjects, lwm2m_transaction_callback_t updateCallback)
{
    ctiot_funcv1_context_t* pContext = pTContext;
    ctiot_funcv1_chip_info_ptr pChipInfo;
    uint16_t result = CTIOT_NB_SUCCESS;
    if(msgId)
    {
        *msgId=0;
    }
    if(pContext == NULL)
    {
        pContext = ctiot_funcv1_get_context();
    }

    if(pContext->loginStatus == UE_NO_IP)
    {
        result = CTIOT_EB_NO_IP;
        goto exit;
    }
    
    pChipInfo = pContext->chipInfo;
    if(pChipInfo == NULL)
    {
        result = CTIOT_EB_OTHER;
        goto exit;
    }
    if(pChipInfo->cState == NETWORK_JAM)
    {
        result = CTIOT_EA_NETWORK_TRAFFIC;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_RECOVERING)
    {
        result = CTIOT_EA_ENGINE_EXCEPTION;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_MANUAL_RESTART)
    {
        result = CTIOT_EA_ENGINE_REBOOT;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNCONNECTED)
    {
        result = CTIOT_EA_CONNECT_USELESS_TEMP;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNAVAILABLE)
    {
        result = CTIOT_EA_CONNECT_FAILED_SIMCARD;//
        goto exit;
    }
    else if(pChipInfo->cState==NETWORK_CONNECTED)
    {
        if(pContext->loginStatus == UE_NOT_LOGINED || pContext->loginStatus == UE_LOG_OUT || pContext->loginStatus == UE_LOG_FAILED)
        {
            result = CTIOT_EA_NOT_LOGIN;
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINING)
        {
            result = CTIOT_EA_LOGIN_PROCESSING;
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINED)
        {
            //ct_lwm2m_update_registration
            if(pContext->lwm2mContext==NULL)
            {
                result = CTIOT_EA_OPERATION_FAILED_NOSESSION;
                goto exit;
            }
            if(!prv_check_location(pContext))
            {
                lwm2m_printf("ctiot_update_reg:prv_check_location:false\r\n");
                result =CTIOT_EA_OPERATION_FAILED_NOSESSION;
                goto exit;
            }
            lwm2m_server_t* pServer=pContext->lwm2mContext->serverList;

            if(pServer == NULL)
            {
                result = CTIOT_EA_OPERATION_FAILED_NOSESSION;
                goto exit;
            }

            while(pServer!=NULL)
            {
                lwm2m_transaction_t * transaction;
                uint8_t * payload = NULL;
                int payload_length;
                transaction = ctiot_transaction_new(pServer->sessionH, COAP_POST, NULL, NULL, pContext->lwm2mContext->nextMID++, 4, NULL);

                if (transaction == NULL)
                {
                    pServer=pServer->next;
                    continue;
                }
                if(msgId) *msgId=transaction->mID;
                coap_set_header_uri_path(transaction->message, pServer->location);
                prv_cfg_sendOption((coap_packet_t*)transaction->message,SENDMODE_CON_REL);
                if(withObjects == true)
                {
                    payload_length = ct_object_getRegisterPayloadBufferLength(pContext->lwm2mContext);
                    if(payload_length == 0)
                    {
                        ctiot_transaction_free(transaction);
                        result = CTIOT_EB_OTHER;
                        continue;
                    }

                    payload = ct_lwm2m_malloc(payload_length);
                    if(!payload)
                    {
                        ctiot_transaction_free(transaction);
                        result = CTIOT_EB_OTHER;
                        continue;
                    }
                    payload_length = ct_object_getRegisterPayload(pContext->lwm2mContext, payload, payload_length);
                    if(payload_length == 0)
                    {
                        ctiot_transaction_free(transaction);
                        ct_lwm2m_free(payload);
                        result = CTIOT_EB_OTHER;
                        continue;
                    }
                    coap_set_payload(transaction->message, payload, payload_length);
                }
                transaction->callback = updateCallback;
                transaction->userData = (void *) pServer;
                transaction->buffer=NULL;
                transaction->buffer_len=0;
                pContext->lwm2mContext->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(pContext->lwm2mContext->transactionList, transaction);

                pServer->status = STATE_REG_UPDATE_PENDING;
                pServer=pServer->next;
            }
        }
        else
        {
            result = CTIOT_EB_OTHER;
        }
    }
    else
    {
        result = CTIOT_EB_OTHER;
    }
exit:
    return result;
}

uint16_t ctiot_funcv1_send(ctiot_funcv1_context_t* pTContext,char* data,ctiot_funcv1_send_mode_e sendMode, ctiot_callback_notify notifyCallback, UINT8 seqNum)
{
    ctiot_funcv1_chip_info_ptr pChipInfo;
    ctiot_funcv1_status_t atStatus[1]={0};
    ctiot_funcv1_context_t* pContext=pTContext;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    uint16_t result = CTIOT_NB_SUCCESS;
    int datalen= strlen(data);

    if(sendMode > 3)
    {
        result = CTIOT_EB_PARAMETER_VALUE_ERROR;//3 参数值错误
        goto exit;
    }
    if( pContext==NULL || datalen ==0)
    {
        result= CTIOT_EB_OTHER;  //1 其它错误
        goto exit;
    }
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_send_0, P_INFO, 1, "datalen=%d",datalen);
    if(datalen % 2 != 0)
    {
        result = CTIOT_EB_DATA_LENGTH_NOT_EVEN;//17 Data字段长度不是偶数
        goto exit;
    }
    if(datalen > 2048)
    {
        result = CTIOT_EB_DATA_LENGTH_OVERRRUN;//14 Data字段长度超过上限
        goto exit;
    }
    
    if(pContext->loginStatus == UE_NO_IP)
    {
        result = CTIOT_EB_NO_IP;
        goto exit;
    }
    
    pChipInfo=pContext->chipInfo;

    if(pChipInfo==NULL)
    {
        result= CTIOT_EB_OTHER;
        goto exit;
    }
    if(pChipInfo->cState == NETWORK_JAM)
    {
        result = CTIOT_EA_NETWORK_TRAFFIC; // 951连接不可用，网络拥塞
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_RECOVERING)
    {
        result = CTIOT_EA_ENGINE_EXCEPTION; //950 Engine异常，恢复中
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_MANUAL_RESTART)
    {
        result = CTIOT_EA_ENGINE_REBOOT;   //953 Engine异常，需reboot重启
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNCONNECTED)
    {
        result = CTIOT_EA_CONNECT_USELESS_TEMP;  //957 连接暂时不可用
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNAVAILABLE)
    {
        result = CTIOT_EA_CONNECT_FAILED_SIMCARD;//16 连接不可用，卡原因等
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_CONNECTED)
    {

        if(pContext->loginStatus == UE_NOT_LOGINED || pContext->loginStatus == UE_LOG_OUT || pContext->loginStatus == UE_LOG_FAILED)
        {
            result = CTIOT_EA_NOT_LOGIN;//954 操作失败，未登录
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINING)
        {
            result = CTIOT_EA_LOGIN_PROCESSING; //955 操作失败，登录中
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINED)
        {
            lwm2m_observed_t * targetP;
            lwm2m_uri_t uriC;
            lwm2m_context_t*  lwm2mContext=pContext->lwm2mContext;
            lwm2m_watcher_t * watcherP;
            char * uri="/19/0/0";
            bool found =false;
            ctlwm2m_stringToUri(uri,strlen(uri),&uriC);
            lwm2m_printf("lwm2mContext->observedList %p\r\n",lwm2mContext->observedList);
            for (targetP = lwm2mContext->observedList ; targetP != NULL ; targetP = targetP->next)
            {
                lwm2m_printf("observe uri:/%d/%d/%d\r\n",targetP->uri.objectId,targetP->uri.instanceId,targetP->uri.resourceId);
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_send_1, P_INFO, 3, "observe uri:/%d/%d/%d",targetP->uri.objectId,targetP->uri.instanceId,targetP->uri.resourceId);
                found=prv_match_uri(uriC,targetP->uri);

                //printf("found %d\r\n",found);
                if(found)
                {
                    watcherP=targetP->watcherList;
                    break;
                }
            }
            if(!found)
            {
                result = CTIOT_EC_OBJECT_NOT_OBSERVED; //15 object19传送通道不存在
                goto exit;
            }
               //入队
            //printf("ctiot_coap_queue_add:2\r\n");
            ctiot_funcv1_updata_list *updata_list;
            updata_list = (ctiot_funcv1_updata_list *)ct_lwm2m_malloc(sizeof(ctiot_funcv1_updata_list));
            ctiot_funcv1_data_list *data_list;
            data_list = (ctiot_funcv1_data_list *)ct_lwm2m_malloc(sizeof(ctiot_funcv1_data_list));
            //printf("ctiot_coap_queue_add:3\r\n");
            data_list->next = NULL;
            data_list->dataType  = DATA_TYPE_OPAQUE;
            data_list->u.asBuffer.length = datalen/2;
            data_list->u.asBuffer.buffer = prv_at_decode(data,datalen);
            updata_list->updata = data_list;
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_send_2, P_INFO, 1, "seqNum=%d", seqNum);
            if (seqNum != 0){
                updata_list->msgid = seqNum;
                updata_list->seqnum = seqNum;
            }
            else{
                updata_list->msgid = pContext->lwm2mContext->nextMID++;
                updata_list->seqnum = 0;
            }
            lwm2m_printf("updata_list->msgid:%u\r\n",updata_list->msgid);
            updata_list->responseCode=COAP_205_CONTENT;
            updata_list->sendType=SEND_TYPE_NOTIFY_19;
            updata_list->status=SEND_DATA_CACHEING;
            updata_list->observeMode=OBSERVE_WITH_DATA_FOLLOW;
            updata_list->sendFormat=DATA_FORMAT_OPAQUE;
            memset(updata_list->token, 0x00, 8);

            updata_list->tokenLen=watcherP->tokenLen;
            memcpy(updata_list->token, watcherP->token, watcherP->tokenLen);
            updata_list->mode = sendMode;
            updata_list->uri = (uint8_t *)ct_lwm2m_strdup(uri);
            //printf("ctiot_coap_queue_add:0\r\n");

            ctiot_funcv1_updata_list *nodePtrDel = NULL;
            int isSuccess=ctiot_funcv1_coap_queue_add(pContext->updataList, (ctiot_funcv1_list_t *)updata_list, &nodePtrDel);
            if(isSuccess!=CTIOT_NB_SUCCESS)
            {
                result=CTIOT_EE_QUEUE_OVERRUN;
                ctiot_funcv1_data_list* pTempDataNode=updata_list->updata;
                while (pTempDataNode!=NULL)
                {
                    ctiot_funcv1_data_list* toDelete=pTempDataNode;
                    pTempDataNode=pTempDataNode->next;
                    if(toDelete->dataType == DATA_TYPE_STRING || toDelete->dataType == DATA_TYPE_OPAQUE)
                    {
                        ct_lwm2m_free(toDelete->u.asBuffer.buffer);
                    }
                    ct_lwm2m_free(toDelete);
                }
                if(updata_list->uri!=NULL)
                {
                    ct_lwm2m_free(updata_list->uri);
                }
                ct_lwm2m_free(updata_list);
            }
            if(nodePtrDel != NULL)//list full need delete the earliest node
            {
                ctiot_funcv1_data_list* pTempDataNode=nodePtrDel->updata;
                while (pTempDataNode!=NULL)
                {
                    ctiot_funcv1_data_list* toDelete=pTempDataNode;
                    pTempDataNode=pTempDataNode->next;
                    if(toDelete->dataType == DATA_TYPE_STRING || toDelete->dataType == DATA_TYPE_OPAQUE)
                    {
                        ct_lwm2m_free(toDelete->u.asBuffer.buffer);
                    }
                    ct_lwm2m_free(toDelete);
                }
                if(nodePtrDel->uri!=NULL)
                {
                    ct_lwm2m_free(nodePtrDel->uri);
                }
                ct_lwm2m_free(nodePtrDel);
            }
            
#ifdef  FEATURE_REF_AT_ENABLE
            else
            {
                pContext->seqNum = seqNum;
            }
#endif
            //printf("ctiot_coap_queue_add:1\r\n");
            atStatus->baseInfo=NULL;
            atStatus->extraInfo=(void *)&updata_list->msgid;
        }
        else
        {
          result = CTIOT_EB_OTHER;
        }

    }
    else
    {
      result = CTIOT_EB_OTHER;
    }

exit:
    if(result == CTIOT_NB_SUCCESS)
    {
        if (notifyCallback != NULL){
            notifyCallback(CTIOT_NB_SUCCESS, AT_TO_MCU_SENDSTATUS, (void *) atStatus, 0);
        }
    }
    return result;
}


uint16_t ctiot_funcv1_cmdrsp(ctiot_funcv1_context_t* pTContext,uint16_t msgId,char* token,uint16_t responseCode,char* uriStr,uint8_t observe,uint8_t dataFormat,char* dataS)
{

    ctiot_funcv1_context_t* pContext=pTContext;
    ctiot_funcv1_data_list* dataNode = NULL;
    uint16_t result=CTIOT_NB_SUCCESS;
    lwm2m_uri_t uriP;
    ctiot_funcv1_chip_info_ptr pChipInfo;

    if(observe != OBSERVE_SET && observe != OBSERVE_CANCEL && observe !=OBSERVE_WITH_NO_DATA_FOLLOW && observe != OBSERVE_WITH_DATA_FOLLOW)
    {
        result = CTIOT_EB_PARAMETER_VALUE_ERROR;
        goto exit;
    }
    if(observe == OBSERVE_WITH_DATA_FOLLOW && (dataFormat != DATA_FORMAT_TLV && dataFormat != DATA_FORMAT_OPAQUE && dataFormat != DATA_FORMAT_TEXT && dataFormat != DATA_FORMAT_JSON && dataFormat != DATA_FORMAT_LINK))
    {
        result = CTIOT_EB_PARAMETER_VALUE_ERROR;
        goto exit;
    }
    if(ctlwm2m_stringToUri(uriStr, strlen(uriStr), &uriP) == 0
        ||( uriP.objectId == 0 || uriP.objectId == 4
            || uriP.objectId == 5 || uriP.objectId == 19))
    {
        result = CTIOT_EE_URI_ERROR;
        goto exit;
    }
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    pChipInfo=pContext->chipInfo;
    if(pChipInfo==NULL)
    {
        result= CTIOT_EB_OTHER;
        goto exit;
    }
    if(pChipInfo->cState == NETWORK_JAM)
    {
        result = CTIOT_EA_NETWORK_TRAFFIC;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_RECOVERING)
    {
        result = CTIOT_EA_ENGINE_EXCEPTION;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_MANUAL_RESTART)
    {
        result = CTIOT_EA_ENGINE_REBOOT;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNCONNECTED)
    {
        result = CTIOT_EA_CONNECT_USELESS_TEMP;
        goto exit;
    }
    else if(pChipInfo->cState == NETWORK_UNAVAILABLE)
    {
        result = CTIOT_EA_CONNECT_FAILED_SIMCARD;//需进一步确认
        goto exit;
    }
    else if(pChipInfo->cState==NETWORK_CONNECTED)
    {
        if(pContext->loginStatus == UE_NOT_LOGINED)
        {
            result = CTIOT_EA_NOT_LOGIN;
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINING)
        {
            result = CTIOT_EA_LOGIN_PROCESSING;
            goto exit;
        }
        else if(pContext->loginStatus == UE_LOGINED)
        {
            lwm2m_uri_t uriP[1]={0};
            if(ctlwm2m_stringToUri(uriStr, strlen(uriStr), uriP)==0)
            {
                result=CTIOT_EE_URI_ERROR;
                goto exit;
            }
            if(prv_check_repetitive_msgid(pContext,msgId)==true)
            {
                result = CTIOT_EC_MSG_ID_REPETITIVE;
                goto exit;
            }
            ctiot_funcv1_updata_list* pTempListNode=ct_lwm2m_malloc(sizeof(ctiot_funcv1_updata_list));
            if(pTempListNode==NULL)
            {
                result=CTIOT_EB_OTHER;
                goto exit;
            }
            memset(pTempListNode,0,sizeof(ctiot_funcv1_updata_list));
            pTempListNode->msgid=msgId;
            if(msgId == 0)
            {
                ct_lwm2m_free(pTempListNode);
                result = CTIOT_EB_PARAMETER_VALUE_ERROR;
                goto exit;
            }
            pTempListNode->sendType=SEND_TYPE_OTHER;
            pTempListNode->uri=(uint8_t *)ct_lwm2m_strdup(uriStr);
            if(token!=NULL)
            {
                //解码？
                //int tokenlen=strlen(token);
                pTempListNode->tokenLen=strlen(token)/2;
                uint8_t* tokens= prv_at_decode(token,strlen(token));
                memcpy(pTempListNode->token,tokens,pTempListNode->tokenLen);
                ct_lwm2m_free(tokens);
            }
            result = prv_cmdrsp_set_responsecode(responseCode, &pTempListNode->responseCode);
            if(result != CTIOT_NB_SUCCESS)
            {
                if(pTempListNode->uri!=NULL)
                {
                    ct_lwm2m_free(pTempListNode->uri);
                }
                ct_lwm2m_free(pTempListNode);
                goto exit;
            }
            pTempListNode->observeMode=(ctiot_funcv1_observe_e)observe;
            pTempListNode->mode=SENDMODE_ACK;
            pTempListNode->sendFormat=(ctiot_funcv1_send_format_e)dataFormat;
            pTempListNode->status=SEND_DATA_CACHEING;
            pTempListNode->next=NULL;
            if(observe == OBSERVE_WITH_DATA_FOLLOW)
            {
                dataNode=ct_lwm2m_malloc(sizeof(ctiot_funcv1_data_list));
                if(dataNode==NULL)
                {
                    if(pTempListNode->uri!=NULL)
                    {
                        ct_lwm2m_free(pTempListNode->uri);
                    }
                    ct_lwm2m_free(pTempListNode);
                    result = CTIOT_EB_OTHER;
                    goto exit;
                }
                else
                {
                    memset(dataNode,0,sizeof(ctiot_funcv1_data_list));
                }
                switch(pTempListNode->sendFormat)
                {
                    case DATA_FORMAT_TLV:
                    case DATA_FORMAT_OPAQUE:
                    {
                        if(strlen(dataS)%2!=0)
                        {
                            if(pTempListNode->uri!=NULL)
                            {
                                ct_lwm2m_free(pTempListNode->uri);
                            }
                            ct_lwm2m_free(pTempListNode);
                            ct_lwm2m_free(dataNode);
                            result=CTIOT_EB_DATA_LENGTH_NOT_EVEN;
                            goto exit;
                        }
                        if(strlen(dataS)>2*CTIOT_MAX_PACKET_SIZE)
                        {
                            if(pTempListNode->uri!=NULL)
                            {
                                ct_lwm2m_free(pTempListNode->uri);
                            }
                            ct_lwm2m_free(pTempListNode);
                            ct_lwm2m_free(dataNode);
                            result=CTIOT_EB_DATA_LENGTH_OVERRRUN;
                            goto exit;
                        }
                        dataNode->dataType=DATA_TYPE_OPAQUE;
                        dataNode->u.asBuffer.length=strlen(dataS)/2;
                        dataNode->u.asBuffer.buffer=prv_at_decode(dataS,strlen(dataS));
                        break;
                    }
                    case DATA_FORMAT_TEXT:
                    case DATA_FORMAT_JSON:
                    {
                        if(strlen(dataS)>CTIOT_MAX_PACKET_SIZE)
                        {
                            if(pTempListNode->uri!=NULL)
                            {
                                ct_lwm2m_free(pTempListNode->uri);
                            }
                            ct_lwm2m_free(pTempListNode);
                            ct_lwm2m_free(dataNode);
                            result=CTIOT_EB_DATA_LENGTH_OVERRRUN;
                            goto exit;
                        }
                        dataNode->dataType=DATA_TYPE_STRING;
                        dataNode->u.asBuffer.length=strlen(dataS);
                        dataNode->u.asBuffer.buffer=(uint8_t *)ct_lwm2m_strdup(dataS);
                        break;
                    }
                    case DATA_FORMAT_LINK:
                    {
                        //待补充
                        if(strlen(dataS)>CTIOT_MAX_PACKET_SIZE)
                        {
                            if(pTempListNode->uri!=NULL)
                            {
                                ct_lwm2m_free(pTempListNode->uri);
                            }
                            ct_lwm2m_free(pTempListNode);
                            ct_lwm2m_free(dataNode);
                            result=CTIOT_EB_DATA_LENGTH_OVERRRUN;
                            goto exit;
                        }
                        dataNode->dataType=DATA_TYPE_STRING;
                        dataNode->u.asBuffer.length=strlen(dataS);
                        dataNode->u.asBuffer.buffer=(uint8_t *)ct_lwm2m_strdup(dataS);
                        break;
                    }
                    default:break;
                }
                pTempListNode->updata=dataNode;
            }
            ctiot_funcv1_updata_list *nodePtrDel = NULL;
            int isSuccess=ctiot_funcv1_coap_queue_add(pContext->updataList,(ctiot_funcv1_list_t *)pTempListNode, &nodePtrDel);
            if(isSuccess!=CTIOT_NB_SUCCESS)
            {
                if(dataNode != NULL)
                {
                    if(dataNode->u.asBuffer.buffer != NULL)
                    {
                        ct_lwm2m_free(dataNode->u.asBuffer.buffer);
                    }
                    ct_lwm2m_free(dataNode);
                }
                result=CTIOT_EE_QUEUE_OVERRUN;
                ctiot_funcv1_data_list* pTempDataNode=pTempListNode->updata;
                while (pTempDataNode!=NULL)
                {
                    ctiot_funcv1_data_list* toDelete=pTempDataNode;
                    pTempDataNode=pTempDataNode->next;
                    if(toDelete->dataType == DATA_TYPE_STRING || toDelete->dataType == DATA_TYPE_OPAQUE)
                    {
                        ct_lwm2m_free(toDelete->u.asBuffer.buffer);
                    }
                    ct_lwm2m_free(toDelete);
                }
                if(pTempListNode->uri!=NULL)
                {
                    ct_lwm2m_free(pTempListNode->uri);
                }
                ct_lwm2m_free(pTempListNode);
            }
            if(nodePtrDel != NULL)//list full need delete the earliest node
            {
                ctiot_funcv1_data_list* pTempDataNode=nodePtrDel->updata;
                while (pTempDataNode!=NULL)
                {
                    ctiot_funcv1_data_list* toDelete=pTempDataNode;
                    pTempDataNode=pTempDataNode->next;
                    if(toDelete->dataType == DATA_TYPE_STRING || toDelete->dataType == DATA_TYPE_OPAQUE)
                    {
                        ct_lwm2m_free(toDelete->u.asBuffer.buffer);
                    }
                    ct_lwm2m_free(toDelete);
                }
                if(nodePtrDel->uri!=NULL)
                {
                    ct_lwm2m_free(nodePtrDel->uri);
                }
                ct_lwm2m_free(nodePtrDel);
            }
        }
        else
        {
            result = CTIOT_EB_OTHER;
        }
    }
    else
    {
        result = CTIOT_EB_OTHER;
    }

exit:
    return result;
}

uint16_t ctiot_funcv1_get_status(ctiot_funcv1_context_t* pTContext,uint8_t queryType,uint16_t msgId)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    ctiot_funcv1_query_status_t queryStatus;
    char firmWareInfo[50]={0};
    uint16_t result=QUERY_STATUS_SESSION_NOT_EXIST;
    memset((void *)&queryStatus, 0, sizeof(ctiot_funcv1_query_status_t));
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }

    if(pContext==NULL || pContext->lwm2mContext==NULL)
    {
        goto exit;
    }

    queryStatus.queryType=(ctiot_funcv1_query_status_type_e)queryType;
    switch(queryType)
    {
        case STATUS_TYPE_SESSION://会话状态
        {
            switch(pContext->loginStatus)
            {
                case UE_LOGINED:
                {
                    result=QUERY_STATUS_LOGIN;
                    break;
                }
                case UE_LOGINING:
                {
                    result=QUERY_STATUS_LOGINING;
                    break;
                }
                case UE_NOT_LOGINED:
                {
                    result=QUERY_STATUS_NOT_LOGIN;
                    break;
                }
                case UE_LOG_FAILED:
                {
                    result=QUERY_STATUS_LOGIN_FAILED;
                    break;
                }
                default:break;
            }
            break;
        }
        case STATUS_TYPE_TRANSCHANNEL://会话状态
        {
            result=prv_check_transchannel(pContext);
            break;
        }
        case STATUS_TYPE_MSG://消息状态
        {
            result=prv_check_msg(pContext,msgId);
            queryStatus.u.extraInt=msgId;
            break;
        }
        case STATUS_TYPE_CONNECT://连接状态
        {
            uint64_t extravalue;
            result= prv_check_connect_status(pContext,&extravalue);
            queryStatus.u.extraInt=extravalue;
            break;
        }
        case STATUS_TYPE_COMINFO: //通信信息
        {
            result = prv_check_cominfo(pContext,(char **)&firmWareInfo);
            queryStatus.u.extraInfo.buffer=(uint8_t *)firmWareInfo;
            queryStatus.u.extraInfo.bufferLen=strlen(firmWareInfo);
        }
        default:break;
    }
exit:
    queryStatus.queryResult=(ctiot_funcv1_query_status_e)result;
    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QUERY_STATUS, &queryStatus, 0);
    return result;
}

void ctiot_funcv1_clean_context()
{
    c2f_clean_context(NULL);
}

void ctiot_funcv1_clean_params()
{
    c2f_clean_params(NULL);
}

uint16_t ctiot_funcv1_set_psk(ctiot_funcv1_context_t* pTContext,uint8_t securityMode,uint8_t* pskId,uint8_t* psk)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    uint16_t result=CTIOT_NB_SUCCESS;
    if(securityMode!=1)
    {
        if(pskId==NULL || psk==NULL || strlen((const char *)pskId)==0 || strlen((const char *)psk)==0)
        {
            result=CTIOT_ED_PSK_ERROR;
            goto exit;
        }
        pContext->psk=ct_lwm2m_strdup((const char *)psk);
        pContext->pskid=ct_lwm2m_strdup((const char *)pskId);
        pContext->pskLen=strlen(pContext->psk);
    }
    else
    {
        pContext->psk=NULL;
        pContext->pskid=NULL;
        pContext->pskLen=0;
    }
    pContext->securityMode=(ctiot_funcv1_security_mode_e)securityMode;
    c2f_encode_params(pContext);
exit:
    return result;
}


uint16_t ctiot_funcv1_get_pskid(ctiot_funcv1_context_t* pContext, char* pskid)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(pskid!=NULL)
    {
        strncpy(pskid, pContext->pskid, MAX_PSKID_LEN);
    }
    return result;
}

uint16_t ctiot_funcv1_get_sec_mode(ctiot_funcv1_context_t* pContext,uint8_t* securityMode)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(securityMode!=NULL)
    {
        *securityMode=pContext->securityMode;
    }
    return result;
}

uint16_t ctiot_funcv1_set_idauth_pm(ctiot_funcv1_context_t* pTContext,char* idAuthStr)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }

    if(idAuthStr == NULL || strlen(idAuthStr)<=0)
    {
        result = CTIOT_EB_PARAMETER_VALUE_ERROR;
        goto exit;
    }

    if((pContext->idAuthMode != AUTHMODE_SIMID_MCU && pContext->idAuthMode != AUTHMODE_SIM9_MCU)|| pContext->loginStatus!=UE_NOT_LOGINED)
    {
        result = CTIOT_ED_ONLY_IDAUTHMOD2_PERMITTED;
        goto exit;
    }

    if( pContext->idAuthStr != NULL )
    {
        ct_lwm2m_free(pContext->idAuthStr);

    }
    pContext->idAuthStr=ct_lwm2m_strdup(idAuthStr);
    c2f_encode_params(pContext);

exit:
    return result;
}

void ctiot_funcv1_disable_sleepmode()
{
    slpManPlatVoteDisableSleep(ctiotSlpHandler, SLP_SLP2_STATE);
}
void ctiot_funcv1_enable_sleepmode()
{
    slpManPlatVoteEnableSleep(ctiotSlpHandler, SLP_SLP2_STATE);
}

void ctiot_funcv1_init_sleep_handler()
{
    slpManRet_t result=slpManFindPlatVoteHandle("CTIOT_NB", &ctiotSlpHandler);
    if(result==RET_HANDLE_NOT_FOUND)
        slpManApplyPlatVoteHandle("CTIOT_NB",&ctiotSlpHandler);
}
void ctiot_funcv1_sleep_vote(system_status_e status)
{
    uint8_t counter = 0;
    slpManSlpState_t pstate;

    if(status==SYSTEM_STATUS_FREE)
    {
        if(RET_TRUE == slpManCheckVoteState(ctiotSlpHandler, &pstate, &counter))
        {
            for(;counter > 0; counter -- )
            {
                ctiot_funcv1_enable_sleepmode();
            }
        }
    }
    else
    {
        if(RET_TRUE == slpManCheckVoteState(ctiotSlpHandler, &pstate, &counter))
        {
            ctiot_funcv1_disable_sleepmode();
        }
    }
}

uint16_t ctiot_funcv1_get_localIP(char* localIP)
{
    uint16_t result=CTIOT_EB_OTHER;
    //int len =  sizeof(struct sockaddr_in);
    if(localIP==NULL)
    {
        return result;
    }

    NmAtiSyncRet netInfo;
    NBStatus_t nbstatus=appGetNetInfoSync(0/*pContext->chipInfo.cCellID*/,&netInfo);
    if ( NM_NET_TYPE_IPV4 == netInfo.body.netInfoRet.netifInfo.ipType)
    {
        uint8_t* ips=(uint8_t *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr;
        sprintf(localIP,"%u.%u.%u.%u",ips[0],ips[1],ips[2],ips[3]);
        ECOMM_STRING(UNILOG_PLA_STRING, ctiot_funcv1_get_localIP_0, P_INFO, "local IP:%s", (const uint8_t *)localIP);

        result = CTIOT_NB_SUCCESS;
    }
    return result;
}

uint16_t ctiot_funcv1_wait_cstate(ctiot_funcv1_chip_info* pChipInfo)
{
    uint16_t result=0;//success
    time_t start;
    time_t end;
    NmAtiSyncRet netStatus;
    
    start=ct_lwm2m_gettime();
    end=start;
    while(end-start<120)//wait 120s
    {
        appGetNetInfoSync(0, &netStatus);
        if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
        {
           pChipInfo->cState = NETWORK_CONNECTED;
           goto exit;
        }
        usleep(1000000);
        end=ct_lwm2m_gettime();
    }
    result=1;//timeout
exit:
    return result;
}

uint16_t ctiot_funcv1_set_dfota_mode(ctiot_funcv1_context_t* pTContext, uint8_t mode)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext == NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    // clear when reboot
    pContext->fotaMode = mode;
    return result;
}

uint16_t ctiot_funcv1_set_regswt_mode(ctiot_funcv1_context_t* pTContext, uint8_t type)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint16_t result=CTIOT_NB_SUCCESS;

    if(pContext == NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    // save to flash and will take effect after reboot
    pContext->regFlag = type;
    c2f_encode_params(pContext);
    return result;
}

uint16_t ctiot_funcv1_get_regswt_mode(ctiot_funcv1_context_t* pContext,uint8_t* type)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(type!=NULL)
    {
        *type=pContext->regFlag;
    }
    return result;
}

uint16_t ctiot_funcv1_set_imsi(ctiot_funcv1_context_t* pTContext, char* imsi)
{
    ctiot_funcv1_context_t* pContext=pTContext;
    uint8_t len;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    uint16_t result=CTIOT_NB_SUCCESS;
    len = strlen(imsi);
    if(len > 0 && len < 16)
    {
        for(uint8_t i=0; i<len; i++)
        {
            if((imsi[i]<0x30) || (imsi[i]>0x39))
            {
                result= CTIOT_EB_PARAMETER_VALUE_ERROR;
                goto exit;
            }
        }
        memset(pContext->chipInfo->cImsi, 0, 20);
        memcpy(pContext->chipInfo->cImsi, imsi, len);
    }
    else
    {
        result= CTIOT_EB_PARAMETER_VALUE_ERROR;
        goto exit;
    }
    c2f_encode_params(pContext);
exit:
    return result;
}


uint16_t ctiot_funcv1_get_imsi(ctiot_funcv1_context_t* pContext, char* imsi)
{
    uint16_t result=CTIOT_NB_SUCCESS;
    if(pContext==NULL)
    {
        pContext=ctiot_funcv1_get_context();
    }
    if(imsi!=NULL)
    {
        strncpy(imsi, pContext->chipInfo->cImsi, MAX_IMSI_LEN);//imsi is a mazimum 15 decimal digits
    }
    return result;
}




