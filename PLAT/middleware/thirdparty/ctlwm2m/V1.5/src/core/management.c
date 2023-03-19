/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/
/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

#include "ct_internals.h"
#include <stdio.h>
#include "objects/lwm2mclient.h"

#include "ct_liblwm2m.h"

#include "ctiot_lwm2m_sdk.h"



void* sessionForQueue;
lwm2m_media_type_t formatForQueue;



#ifdef LWM2M_CLIENT_MODE
static int prv_readAttributes(multi_option_t * query,
                              lwm2m_attributes_t * attrP)
{
    int64_t intValue;
    double floatValue;

    memset(attrP, 0, sizeof(lwm2m_attributes_t));

    while (query != NULL)
    {
        if (ct_lwm2m_strncmp((char *)query->data, ATTR_MIN_PERIOD_STR, ATTR_MIN_PERIOD_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MIN_PERIOD)) return -1;
            if (query->len == ATTR_MIN_PERIOD_LEN) return -1;

            if (1 != utils_textToInt(query->data + ATTR_MIN_PERIOD_LEN, query->len - ATTR_MIN_PERIOD_LEN, &intValue)) return -1;
            if (intValue < 0) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_MIN_PERIOD;
            attrP->minPeriod = intValue;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_MIN_PERIOD_STR, ATTR_MIN_PERIOD_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MIN_PERIOD)) return -1;
            if (query->len != ATTR_MIN_PERIOD_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_MIN_PERIOD;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_MAX_PERIOD_STR, ATTR_MAX_PERIOD_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MAX_PERIOD)) return -1;
            if (query->len == ATTR_MAX_PERIOD_LEN) return -1;

            if (1 != utils_textToInt(query->data + ATTR_MAX_PERIOD_LEN, query->len - ATTR_MAX_PERIOD_LEN, &intValue)) return -1;
            if (intValue < 0) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_MAX_PERIOD;
            attrP->maxPeriod = intValue;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_MAX_PERIOD_STR, ATTR_MAX_PERIOD_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MAX_PERIOD)) return -1;
            if (query->len != ATTR_MAX_PERIOD_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_MAX_PERIOD;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_GREATER_THAN_STR, ATTR_GREATER_THAN_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_GREATER_THAN)) return -1;
            if (query->len == ATTR_GREATER_THAN_LEN) return -1;

            if (1 != utils_textToFloat(query->data + ATTR_GREATER_THAN_LEN, query->len - ATTR_GREATER_THAN_LEN, &floatValue)) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_GREATER_THAN;
            attrP->greaterThan = floatValue;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_GREATER_THAN_STR, ATTR_GREATER_THAN_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_GREATER_THAN)) return -1;
            if (query->len != ATTR_GREATER_THAN_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_GREATER_THAN;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_LESS_THAN_STR, ATTR_LESS_THAN_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_LESS_THAN)) return -1;
            if (query->len == ATTR_LESS_THAN_LEN) return -1;

            if (1 != utils_textToFloat(query->data + ATTR_LESS_THAN_LEN, query->len - ATTR_LESS_THAN_LEN, &floatValue)) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_LESS_THAN;
            attrP->lessThan = floatValue;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_LESS_THAN_STR, ATTR_LESS_THAN_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_LESS_THAN)) return -1;
            if (query->len != ATTR_LESS_THAN_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_LESS_THAN;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_STEP_STR, ATTR_STEP_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_STEP)) return -1;
            if (query->len == ATTR_STEP_LEN) return -1;

            if (1 != utils_textToFloat(query->data + ATTR_STEP_LEN, query->len - ATTR_STEP_LEN, &floatValue)) return -1;
            if (floatValue < 0) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_STEP;
            attrP->step = floatValue;
        }
        else if (ct_lwm2m_strncmp((char *)query->data, ATTR_STEP_STR, ATTR_STEP_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_STEP)) return -1;
            if (query->len != ATTR_STEP_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_STEP;
        }
        else return -1;

        query = query->next;
    }

    return 0;
}

 
char* getAttributesStr(multi_option_t * query)
{
    multi_option_t* ptr = query;
    char* attrStr = ct_lwm2m_malloc(256);
    if(!attrStr)
    {
        return NULL;
    }
    memset(attrStr, 0x00, 256);
    ptr = query;
    uint8_t len = 0;
    while(ptr)
    {
        memcpy(attrStr + len, ptr->data, ptr->len);
        len+= ptr->len;
        ptr = ptr->next;
        if(!ptr)
        {
            attrStr[len] = '\0';
            len += 1;
        }
        else
        {
            attrStr[len] = ';';
            len += 1;
        }
    }
    return attrStr;
}

static uint8_t ctiot_funcv1_mcu_obj_notify_to_mcu(lwm2m_uri_t * uriP, coap_packet_t* message, ctiot_funcv1_operate_type_e type, lwm2m_media_type_t format);

//---------------------------

uint8_t ct_dm_handleRequest(lwm2m_context_t * contextP,
                         lwm2m_uri_t * uriP,
                         lwm2m_server_t * serverP,
                         coap_packet_t * message,
                         coap_packet_t * response)
{
    uint8_t result;
    lwm2m_media_type_t format;

    LOG_ARG("Code: %02X, server status: %s", message->code, STR_STATUS(serverP->status));
    LOG_URI(uriP);
    ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_0, P_INFO, 3, "objectId=%d instanceID=%d resourceID=%d", uriP->objectId, uriP->instanceId, uriP->resourceId);

    if (IS_OPTION(message, COAP_OPTION_CONTENT_TYPE))
    {
        //ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_00, P_INFO, 1, "ct_dm_handleRequest content_type=%d", message->content_type);
        format = utils_convertMediaType(message->content_type);
    }
    else
    {
        format =LWM2M_CONTENT_TLV;
    }
    //ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_1, P_INFO, 2, "ct_dm_handleRequest format=%d message len=%d", format, message->payload_len);
    if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID)
    {
        return COAP_404_NOT_FOUND;
    }

    if (serverP->status != STATE_REGISTERED
        && serverP->status != STATE_REG_UPDATE_NEEDED
        && serverP->status != STATE_REG_FULL_UPDATE_NEEDED
        && serverP->status != STATE_REG_UPDATE_PENDING)
    {
        return COAP_IGNORE;
    }

    sessionForQueue = serverP->sessionH;
    formatForQueue = format;


    switch (message->code)
    {
    case COAP_GET:
        {
            uint8_t * buffer = NULL;
            size_t length = 0;
            int res;

            if (IS_OPTION(message, COAP_OPTION_OBSERVE))
            {
                //ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_2, P_INFO, 0, "COAP_OPTION_OBSERVE");
                lwm2m_data_t * dataP = NULL;
                int size = 0;
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_OBSERVE, format);
                if(result == COAP_205_CONTENT)
                {
                    //ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_2_1, P_INFO, 1, "uriP->objectId=%d",uriP->objectId);
                    if(uriP->objectId==19)
                    {
                        if(LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)&& uriP->instanceId == 0 && uriP->resourceId == 0)
                        {
                            result = COAP_205_CONTENT;//object_read_reportData(contextP, uriP, &size, &dataP);
                            //lwm2m_printf("ct_dm_handleRequest observe /19/0/0 arrived!\n");
                        }
                        else
                        {
                            result = COAP_401_UNAUTHORIZED;
                            ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_3, P_INFO, 0, "ct_dm_handleRequest /19/0/0 uri error return COAP_401_UNAUTHORIZED!");
                        }
                    }
                    #ifdef AUTO_RESP_OBSERVE_OBJ3
                    else if(uriP->objectId==3)
                    {
                        if(LWM2M_URI_IS_SET_INSTANCE(uriP)&& uriP->instanceId == 0)
                        {
                            result = COAP_205_CONTENT;
                            ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_3_1, P_INFO, 0, "observe /3/0 arrived!!");
                        }
                        else
                        {
                            result = COAP_401_UNAUTHORIZED;
                            ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_3_2, P_INFO, 0, "uri error return COAP_401_UNAUTHORIZED!");
                        }
                    }
                    #endif
                    else
                    {
                        result = ct_object_readData(contextP, uriP, &size, &dataP);
                    }
                
                }
                if (COAP_205_CONTENT == result)
                {
                    result = ct_observe_handleRequest(contextP, uriP, serverP, size, dataP, message, response);

                    if (COAP_205_CONTENT == result)
                    {
                        if (IS_OPTION(message, COAP_OPTION_ACCEPT))
                        {
                            format = utils_convertMediaType((coap_content_type_t)(message->accept[0]));
                        }
                        else
                        {
                            if ((uriP->objectId == 5) && (uriP->instanceId == 0) && (uriP->resourceId == 3))
                                format = LWM2M_CONTENT_TEXT;
                            else
                                format = LWM2M_CONTENT_TLV;
                        }
                        if(uriP->objectId == 19)
                        {
                            buffer = NULL;
                            length = 0;
                            format = LWM2M_CONTENT_OPAQUE;
                        }
                        else
                        {
                            res = ct_lwm2m_data_serialize(uriP, size, dataP, &format, &buffer);
                            if (res < 0)
                            {
                                result = COAP_500_INTERNAL_SERVER_ERROR;
                            }
                            else
                            {
                                length = (size_t)res;
                                
                                ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_4, P_INFO, 3, "Observe Request[/%d/%d/%d]", uriP->objectId, uriP->instanceId, uriP->resourceId);
                                LOG_ARG("Observe Request[/%d/%d/%d]: %.*s\n", uriP->objectId, uriP->instanceId, uriP->resourceId, length, buffer);
                            }
                        }
                    }
                    /*if(!(uriP->objectId == 19 && uriP->instanceId == 0 && uriP->resourceId == 0))
                        ct_lwm2m_data_free(size, dataP);*/
                }
                ct_lwm2m_data_free(size, dataP);
            }
            else if (IS_OPTION(message, COAP_OPTION_ACCEPT)
                  && message->accept_num == 1
                  && message->accept[0] == APPLICATION_LINK_FORMAT)
            {
                format = LWM2M_CONTENT_LINK;
                
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_DISCOVER, format);
                if(result == COAP_205_CONTENT)
                    result = ct_object_discover(contextP, uriP, serverP, &buffer, &length);
            }
            else
            {
                if (IS_OPTION(message, COAP_OPTION_ACCEPT))
                {
                    format = utils_convertMediaType((coap_content_type_t)(message->accept[0]));
                }
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_READ, format);
                if(result == COAP_205_CONTENT)
                    result = ct_object_read(contextP, uriP, &format, &buffer, &length);
            }
            if (COAP_205_CONTENT == result)
            {
                //ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_5, P_INFO, 1, "format receive from platform is %d", format);
                coap_set_header_content_type(response, format);
                coap_set_payload(response, buffer, length);
                // ct_handle_packet will free buffer
            }
            else
            {
                ct_lwm2m_free(buffer);
            }
        }
        break;

    case COAP_POST:
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_CREATE, format);
                if(result == COAP_205_CONTENT)
                    result = ct_object_create(contextP, uriP, format, message->payload, message->payload_len);
                if (result == COAP_201_CREATED)
                {
                    //longest uri is /65535/65535 = 12 + 1 (null) chars
                    char location_path[13] = "";
                    //instanceId expected
                    if ((uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }

                    if (sprintf(location_path, "/%d/%d", uriP->objectId, uriP->instanceId) < 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }
                    coap_set_header_location_path(response, location_path);

                    ct_lwm2m_update_registration(contextP, 0, true);
                }
            }
            else if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_WRITE, format);
                if(result == COAP_205_CONTENT)
                    result = ct_object_write(contextP, uriP, format, message->payload, message->payload_len);
            }
            else
            {
                
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_EXECUTE, format);
                if(result == COAP_205_CONTENT)
                    result = ct_object_execute(contextP, uriP, message->payload, message->payload_len);
                if(result == COAP_204_CHANGED && uriP->objectId == 1 && uriP->instanceId == 0 && uriP->resourceId == 8)
                {
                    ct_lwm2m_update_registration(contextP, 0, false);
                }
            }
        }
        break;

    case COAP_PUT:
        {
            if (IS_OPTION(message, COAP_OPTION_URI_QUERY))
            {
                lwm2m_attributes_t attr = {0};
                if (0 != prv_readAttributes(message->uri_query, &attr))
                {
                    result = COAP_400_BAD_REQUEST;
                }
                else
                {
                    result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_WRITE, LWM2M_CONTENT_LINK);
                    ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_6, P_INFO, 1, "coap put and COAP_OPTION_URI_QUERY result=%d", result);
                    if(result == COAP_205_CONTENT)
                    {
                        result = ct_observe_setParameters(contextP, uriP, serverP, &attr);
                    }
                }
            }
            else if (LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
                
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_WRITE, format);
                
                ECOMM_TRACE(UNILOG_CTLWM2M, dm_handleRequest_7, P_INFO, 1, "coap put and URI is set instance result=%d", result);
                if(result == COAP_205_CONTENT)
                {
                    result = ct_object_write(contextP, uriP, format, message->payload, message->payload_len);
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
          }

        break;

    case COAP_DELETE:
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP) || LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                result = COAP_400_BAD_REQUEST;
            }
            else
            {
                
                result = ctiot_funcv1_mcu_obj_notify_to_mcu(uriP, message, OPERATE_TYPE_DELETE, format);
                if(result == COAP_205_CONTENT)
                {
                //----------------------
                    result = ct_object_delete(contextP, uriP);
                }
                if (result == COAP_202_DELETED)
                {
                    ct_lwm2m_update_registration(contextP, 0, true);
                }
            }
        }
        break;

    default:
        result = COAP_400_BAD_REQUEST;
        break;
    }
    return result;
}

void ctiot_funcv1_obj_19_notify_to_mcu( lwm2m_uri_t * uriP, uint32_t count)

{
    if(uriP->objectId==19 && LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)&& uriP->instanceId == 0 && uriP->resourceId == 0)
    {
#ifndef FEATURE_REF_AT_ENABLE
        ctiot_funcv1_status_t pTmpStatus;
        memset(&pTmpStatus,0,sizeof(ctiot_funcv1_status_t));
#endif
        switch(count)
        {
        case 0:
#ifndef FEATURE_REF_AT_ENABLE
            pTmpStatus.baseInfo = ct_lwm2m_strdup(SO1);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, &pTmpStatus, 0);
            ct_lwm2m_free(pTmpStatus.baseInfo);
#else
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL, 3);
#endif
            ctiot_funcv1_notify_event(MODULE_LWM2M, OBSERVE_SUBSCRIBE, NULL, 0);
        break;
   
       case 1:
#ifndef FEATURE_REF_AT_ENABLE
            pTmpStatus.baseInfo = ct_lwm2m_strdup(SO2);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, &pTmpStatus, 0);
            ct_lwm2m_free(pTmpStatus.baseInfo);
#else
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL, 9);
#endif
            ctiot_funcv1_notify_event(MODULE_LWM2M, OBSERVE_UNSUBSCRIBE, NULL, 0);
        break;
   
        }

    }
    
#ifdef  FEATURE_REF_AT_ENABLE
    if(uriP->objectId==5 && LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)&& uriP->instanceId == 0 && uriP->resourceId == 3)
    {
        if(count == 0)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_obj_19_notify_to_mcu_1, P_INFO, 0, "FOTA process, platform observe 5/0/3 firmware state ");
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL, 5);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_obj_19_notify_to_mcu_2, P_INFO, 0, "FOTA process, platform cancel observe 5/0/3 firmware state ");
        }
    }
#endif

}

static uint8_t ctiot_funcv1_mcu_obj_notify_to_mcu(lwm2m_uri_t * uriP, coap_packet_t* message, ctiot_funcv1_operate_type_e type, lwm2m_media_type_t format)
{
    uint8_t result = COAP_205_CONTENT;
    if(( uriP->objectId == 3 && (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId == 0)
        && (LWM2M_URI_IS_SET_RESOURCE(uriP) && uriP->resourceId == 3) && type == OPERATE_TYPE_READ)
        || uriP->objectId == 1 || uriP->objectId == 4 || uriP->objectId == 5
        || uriP->objectId == 19 || uriP->objectId == 0
        #ifdef AUTO_RESP_OBSERVE_OBJ3
        ||(uriP->objectId == 3 && (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId == 0))
        #endif
        )
    {
        if( uriP->objectId == 4 ||uriP->objectId == 1)
        {
            if( type == OPERATE_TYPE_OBSERVE
              ||(type == OPERATE_TYPE_WRITE && message->code == COAP_PUT && IS_OPTION(message, COAP_OPTION_URI_QUERY)))
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, obj_notify_to_mcu_0_1, P_INFO, 1, "type=%d return unauthorized", type);
                result = COAP_401_UNAUTHORIZED;
            }
        }
        //ECOMM_TRACE(UNILOG_CTLWM2M, obj_notify_to_mcu_0_2, P_INFO, 0, "this situation retun 205 content");
        return result;
    }


    lwm2m_object_t * targetP;
    ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(pContext->lwm2mContext->objectList, uriP->objectId);
    if(targetP == NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, obj_notify_to_mcu_1, P_INFO, 0, "not find the uriP in objectlist return 404 ");
        return COAP_404_NOT_FOUND;
    }

    uint8_t uriStr[20] = {0};
    uri_depth_t depth;
    ctlwm2m_uri_toString( uriP, uriStr, 20, &depth);
    uriStr[strlen((const char *)uriStr) - 1] = '\0';

    if(ctiot_funcv1_add_downdata_to_queue(pContext, message->mid, message->token,
                message->token_len, uriStr, type, format) == CTIOT_NB_SUCCESS)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, obj_notify_to_mcu_2, P_INFO, 1, "add this messge to downlist. type = %d ", type);
        ctiot_funcv1_object_operation_t objectPtr[1];

        switch(type)
        {
            case OPERATE_TYPE_READ:
                objectPtr->cmdType = CMD_TYPE_READ;
                break;
            case OPERATE_TYPE_DISCOVER:
                objectPtr->cmdType = CMD_TYPE_DISCOVER;
                break;
            case OPERATE_TYPE_OBSERVE:
                objectPtr->cmdType = CMD_TYPE_OBSERVE;
                break;
            case OPERATE_TYPE_WRITE:
                if(message->code == COAP_PUT)
                {
                    if(IS_OPTION(message, COAP_OPTION_URI_QUERY))
                    {
                        objectPtr->cmdType = CMD_TYPE_WRITE_ATTRIBUTE;
                    }
                    else
                    {
                        objectPtr->cmdType = CMD_TYPE_WRITE;
                    }
                }
                else if(message->code == COAP_POST)
                {
                    objectPtr->cmdType = CMD_TYPE_WRITE_PARTIAL;
                }
                break;
            case OPERATE_TYPE_EXECUTE:
                objectPtr->cmdType = CMD_TYPE_EXECUTE;
                break;
            case OPERATE_TYPE_CREATE:
                objectPtr->cmdType = CMD_TYPE_CREATE;
                break;
            case OPERATE_TYPE_DELETE:
                objectPtr->cmdType = CMD_TYPE_DELETE;
                break;
        }

        objectPtr->dataFormat = format;

        if(objectPtr->cmdType == CMD_TYPE_WRITE_ATTRIBUTE)
        {
            objectPtr->data = (uint8_t *)getAttributesStr(message->uri_query);
            objectPtr->dataLen = strlen((const char *)(objectPtr->data));
        }
        else
        {
            objectPtr->data = message->payload;
            objectPtr->dataLen = message->payload_len;
        }

        if(objectPtr->cmdType == CMD_TYPE_OBSERVE)
        {
            uint32_t count = 0;
            coap_get_header_observe(message, &count);
            objectPtr->observe = count;
        }

        memcpy(objectPtr->token, message->token, message->token_len);
        objectPtr->tokenLen = message->token_len;

        objectPtr->uri = uriStr;

        objectPtr->msgId = message->mid;

        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_COMMAND, objectPtr, 0);
        if(objectPtr->cmdType == CMD_TYPE_OBSERVE)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, obj_notify_to_mcu_3, P_INFO, 0, "CMD_TYPE_OBSERVE notify opencpu user");
            ctiot_funcv1_notify_event(MODULE_LWM2M, OBSV_CMD, (const void*)objectPtr, 0);
        }

        if(objectPtr->cmdType == CMD_TYPE_WRITE_ATTRIBUTE)
        {
            ct_lwm2m_free(objectPtr->data);
        }
    }
    result = COAP_IGNORE;
    return result;
}



#endif

