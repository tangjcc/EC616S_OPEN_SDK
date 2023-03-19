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
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Manuel Sangoi - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin - Please refer to git log
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ctiot_lwm2m_sdk.h"
#include "ctiot_lwm2m_internals.h"

#define MAX_LOCATION_LENGTH 10      // strlen("/rd/65534") + 1

//extern CTIOT_AEP_INIT initInfo;

#ifdef LWM2M_CLIENT_MODE
/*
static int apn_prv_getRegistrationQueryLength(lwm2m_context_t * contextP,
                                          lwm2m_server_t * server)
{
    int index;
    int res;
    char buffer[21];

    index = strlen(QUERY_STARTER initInfo->version QUERY_DELIMITER QUERY_NAME);
    index += strlen(contextP->endpointName);
    index += strlen(initInfo->apnName);

    if (NULL != contextP->msisdn)
    {
        index += strlen(QUERY_DELIMITER QUERY_SMS);
        index += strlen(contextP->msisdn);
    }

    switch (server->binding)
    {
    case BINDING_U:
        index += strlen("&b=U");
        break;
    case BINDING_UQ:
        index += strlen("&b=UQ");
        break;
    case BINDING_S:
        index += strlen("&b=S");
        break;
    case BINDING_SQ:
        index += strlen("&b=SQ");
        break;
    case BINDING_US:
        index += strlen("&b=US");
        break;
    case BINDING_UQS:
        index += strlen("&b=UQS");
        break;
    default:
        return 0;
    }

    if (0 != server->lifetime)
    {
        index += strlen(QUERY_DELIMITER QUERY_LIFETIME);
        res = utils_intToText(server->lifetime, buffer, sizeof(buffer));
        if (res == 0) return 0;
        index += res;
    }

    return index + 1;
}
*/
int prv_getRegistrationQueryLength(lwm2m_context_t * contextP,
                                          lwm2m_server_t * server)
{
    int index;
    int res;
    uint8_t buffer[21];

    index = strlen(QUERY_STARTER QUERY_VERSION_FULL QUERY_DELIMITER QUERY_NAME);
    index += strlen(contextP->endpointName);

    if (NULL != contextP->msisdn)
    {
        index += strlen(QUERY_DELIMITER QUERY_SMS);
        index += strlen(contextP->msisdn);
    }

    switch (server->binding)
    {
    case BINDING_U:
        index += strlen("&b=U");
        break;
    case BINDING_UQ:
        index += strlen("&b=UQ");
        break;
    case BINDING_S:
        index += strlen("&b=S");
        break;
    case BINDING_SQ:
        index += strlen("&b=SQ");
        break;
    case BINDING_US:
        index += strlen("&b=US");
        break;
    case BINDING_UQS:
        index += strlen("&b=UQS");
        break;
    default:
        return 0;
    }

    if (0 != server->lifetime)
    {
        index += strlen(QUERY_DELIMITER QUERY_LIFETIME);
        res = utils_intToText(server->lifetime, buffer, sizeof(buffer));
        if (res == 0) return 0;
        index += res;
    }

    return index + 1;
}
/*
static int apn_prv_getRegistrationQuery(lwm2m_context_t * contextP,
                                    lwm2m_server_t * server,
                                    char * buffer,
                                    size_t length)
{
    int index;
    int res;

    index = utils_stringCopy(buffer, length, QUERY_STARTER initInfo->version QUERY_DELIMITER QUERY_NAME);
    if (index < 0) return 0;
    index += utils_stringCopy(buffer + index, length - index, initInfo->apnName);
    if (index < 0) return 0;
    res = utils_stringCopy(buffer + index, length - index, contextP->endpointName);
    if (res < 0) return 0;
    index += res;

    if (NULL != contextP->msisdn)
    {
        res = utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_SMS);
        if (res < 0) return 0;
        index += res;
        res = utils_stringCopy(buffer + index, length - index, contextP->msisdn);
        if (res < 0) return 0;
        index += res;
    }

    switch (server->binding)
    {
    case BINDING_U:
        res = utils_stringCopy(buffer + index, length - index, "&b=U");
        break;
    case BINDING_UQ:
        res = utils_stringCopy(buffer + index, length - index, "&b=UQ");
        break;
    case BINDING_S:
        res = utils_stringCopy(buffer + index, length - index, "&b=S");
        break;
    case BINDING_SQ:
        res = utils_stringCopy(buffer + index, length - index, "&b=SQ");
        break;
    case BINDING_US:
        res = utils_stringCopy(buffer + index, length - index, "&b=US");
        break;
    case BINDING_UQS:
        res = utils_stringCopy(buffer + index, length - index, "&b=UQS");
        break;
    default:
        res = -1;
    }
    if (res < 0) return 0;
    index += res;

    if (0 != server->lifetime)
    {
        res = utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_LIFETIME);
        if (res < 0) return 0;
        index += res;
        res = utils_intToText(server->lifetime, buffer + index, length - index);
        if (res == 0) return 0;
        index += res;
    }

    if(index < (int)length)
    {
        buffer[index++] = '\0';
    }
    else
    {
        return 0;
    }

    return index;
}
*/
int prv_getRegistrationQuery(lwm2m_context_t * contextP,
                                    lwm2m_server_t * server,
                                    char * buffer,
                                    size_t length)
{
    int index;
    int res;

    index = utils_stringCopy(buffer, length, QUERY_STARTER QUERY_VERSION_FULL QUERY_DELIMITER QUERY_NAME);
    if (index < 0) return 0;
    res = utils_stringCopy(buffer + index, length - index, contextP->endpointName);
    if (res < 0) return 0;
    index += res;

    if (NULL != contextP->msisdn)
    {
        res = utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_SMS);
        if (res < 0) return 0;
        index += res;
        res = utils_stringCopy(buffer + index, length - index, contextP->msisdn);
        if (res < 0) return 0;
        index += res;
    }

    switch (server->binding)
    {
    case BINDING_U:
        res = utils_stringCopy(buffer + index, length - index, "&b=U");
        break;
    case BINDING_UQ:
        res = utils_stringCopy(buffer + index, length - index, "&b=UQ");
        break;
    case BINDING_S:
        res = utils_stringCopy(buffer + index, length - index, "&b=S");
        break;
    case BINDING_SQ:
        res = utils_stringCopy(buffer + index, length - index, "&b=SQ");
        break;
    case BINDING_US:
        res = utils_stringCopy(buffer + index, length - index, "&b=US");
        break;
    case BINDING_UQS:
        res = utils_stringCopy(buffer + index, length - index, "&b=UQS");
        break;
    default:
        res = -1;
    }
    if (res < 0) return 0;
    index += res;

    if (0 != server->lifetime)
    {
        res = utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_LIFETIME);
        if (res < 0) return 0;
        index += res;
        res = utils_intToText(server->lifetime, (uint8_t*)(buffer + index), length - index);
        if (res == 0) return 0;
        index += res;
    }

    if(index < (int)length)
    {
        buffer[index++] = '\0';
    }
    else
    {
        return 0;
    }

    return index;
}




static void prv_handleRegistrationReply(lwm2m_transaction_t * transacP,
                                        void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    lwm2m_server_t * targetP = (lwm2m_server_t *)(transacP->userData);
    ctiot_funcv1_status_t atStatus[1] = {0};
    ctiot_funcv1_context_t* pTmpContext = ctiot_funcv1_get_context();
    int eventCode = 0;
    
    if(packet != NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, prv_handleRegistrationReply_0, P_INFO, 1, "prv_handleRegistrationReply,code=0x%x", packet->code);
    }
    if (targetP->status == STATE_REG_PENDING)
    {
        targetP->registration = ct_lwm2m_gettime();
        if (packet != NULL && packet->code == COAP_201_CREATED)
        {
            targetP->status = STATE_REGISTERED;
            if (NULL != targetP->location)
            {
                ct_lwm2m_free(targetP->location);
            }

            targetP->location = coap_get_multi_option_as_string(packet->location_path);

            ECOMM_STRING(UNILOG_CTLWM2M, prv_handleRegistrationReply_0_1, P_INFO, "prv_handleRegistrationReply,location_path=%s",(uint8_t*)targetP->location);

            char localIP[CTIOT_MAX_IP_LEN]={0};
            extern uint16_t ctiot_funcv1_get_localIP(char* localIP);
            uint16_t rc = ctiot_funcv1_get_localIP(localIP);
            if(rc == CTIOT_NB_SUCCESS)
            {
                memcpy(pTmpContext->localIP,localIP,strlen(localIP));
            }
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_handleRegistrationReply_1, P_INFO, 0, "Registration successful");
            
            if (pTmpContext->bootFlag == BOOT_FOTA_REBOOT)
            {
                #if defined(CTIOT_ABUP_FOTA_ENABLE)
                    extern void ct_abup_check_upgrade_result(void);
                    extern uint8_t ct_abup_get_upgrade_result(void);
                    extern void report_exec_fota_result();
                    ct_abup_check_upgrade_result();
                    ECOMM_TRACE(UNILOG_CTLWM2M, prv_handleRegistrationReply_2, P_INFO, 1, "prv_handleRegistrationReply-->check fota upgrade result:%d",ct_abup_get_upgrade_result());
                    report_exec_fota_result();
                    ctiot_funcv1_set_boot_flag(NULL, BOOT_LOCAL_BOOTUP);
                    slpManFlushUsrNVMem();
                #endif
            }
            else
            {
                ctiot_funcv1_update_bootflag(pTmpContext,BOOT_LOCAL_BOOTUP);
            }
#ifndef  FEATURE_REF_AT_ENABLE
            atStatus->baseInfo=ct_lwm2m_strdup(SR0);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
#else
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL, 0);
#endif
            pTmpContext->loginStatus = UE_LOGINED;
            ct_lwm2m_free(pTmpContext->idAuthStr);
            // Trigger event if opencpu
            ctiot_funcv1_notify_event(MODULE_LWM2M, REG_SUCCESS, NULL, 0);
        }
        else if(packet ==NULL)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_handleRegistrationReply_3, P_INFO, 0, "Registration failed code:packet ==NULL");
            atStatus->baseInfo=ct_lwm2m_strdup(SR1);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
            eventCode = REG_FAILED_TIMEOUT;
        }
        else if(packet->code == 0x83/*COAP_403*/)
        {
            atStatus->baseInfo=ct_lwm2m_strdup(SR2);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
            eventCode = REG_FAILED_AUTHORIZE_FAIL;
        }
        else if(packet->code == 0x80/*COAP_400*/)
        {
            atStatus->baseInfo=ct_lwm2m_strdup(SR4);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
            eventCode = REG_FAILED_ERROR_ENDPOINTNAME;
        }
        else if(packet->code == 0x8C/*COAP_412*/)
        {
            atStatus->baseInfo=ct_lwm2m_strdup(SR3);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
            eventCode = REG_FAILED_PROTOCOL_NOT_SUPPORT;
        }
        else
        {
            atStatus->baseInfo=ct_lwm2m_strdup(SR5);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
            eventCode = REG_FAILED_OTHER;
        }
        if(eventCode != 0)
        {
            targetP->status = STATE_REG_FAILED;
            pTmpContext->loginStatus = UE_LOG_FAILED;
            ctiot_funcv1_notify_event(MODULE_LWM2M, eventCode, NULL, 0);
#ifdef  FEATURE_REF_AT_ENABLE
            c2f_encode_context(pTmpContext);
#endif
        }
    }
}

// send the registration for a single server
static uint8_t prv_register(lwm2m_context_t * contextP,
                            lwm2m_server_t * server)
{
    char * query = NULL;
    int query_length;
    uint8_t * payload;
    int payload_length;
    lwm2m_transaction_t * transaction;
    
    //lwm2m_printf("prv_register\r\n");
    payload_length = ct_object_getRegisterPayloadBufferLength(contextP);
    if(payload_length == 0) 
        return COAP_500_INTERNAL_SERVER_ERROR;
    
    payload = ct_lwm2m_malloc(payload_length);
    if(!payload) 
        return COAP_500_INTERNAL_SERVER_ERROR;
    
    payload_length = ct_object_getRegisterPayload(contextP, payload, payload_length);
    if(payload_length == 0)
    {
        ct_lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    query_length = prv_getRegistrationQueryLength(contextP, server);
    if(query_length == 0)
    {
        ct_lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();
    if(pContext->protocolMode == PROTOCOL_MODE_ENHANCE)
    {
        int extendlen = prv_extend_query_len();

        query = ct_lwm2m_malloc(query_length+extendlen);
        if(query == NULL)
        {
            ct_lwm2m_free(payload);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        if(prv_getRegistrationQuery(contextP, server, query, query_length) != query_length)
        {
            ct_lwm2m_free(payload);
            ct_lwm2m_free(query);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        
        char *extend_query = prv_extend_query(extendlen);
        //lwm2m_printf("extend_query:%s",extend_query);
        if(extend_query == NULL)
        {
            printf("extend_query == NULL\n");
            ct_lwm2m_free(payload);
            ct_lwm2m_free(query);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        strcat(query, extend_query);
        //sprintf(query,"%s%s",query,extend_query);
        
        ct_lwm2m_free(extend_query);
    }
    else
    {
        query = ct_lwm2m_malloc(query_length);
        if(!query)
        {
            ct_lwm2m_free(payload);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        if(prv_getRegistrationQuery(contextP, server, query, query_length) != query_length)
        {
            ct_lwm2m_free(payload);
            ct_lwm2m_free(query);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        //sprintf(query,"%s",query);
    }
    
    lwm2m_printf("query:%s\n",query);

    if (server->sessionH == NULL)
    {
        server->sessionH = ct_lwm2m_connect_server(server->secObjInstID, contextP->userData);
    }

    if (NULL == server->sessionH)
    {
        ct_lwm2m_free(payload);
        ct_lwm2m_free(query);
        return CONNECT_FAILED;
    }

    transaction = ctiot_transaction_new(server->sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL)
    {
        ct_lwm2m_free(payload);
        ct_lwm2m_free(query);
        return COAP_503_SERVICE_UNAVAILABLE;
    }

    coap_set_header_uri_path(transaction->message, "/"URI_REGISTRATION_SEGMENT);
    coap_set_header_uri_query(transaction->message, query);
    coap_set_header_content_type(transaction->message, LWM2M_CONTENT_LINK);
    coap_set_payload(transaction->message, payload, payload_length);

    transaction->callback = prv_handleRegistrationReply;
    transaction->userData = (void *) server;

    ECOMM_TRACE(UNILOG_CTLWM2M, prv_register_1, P_INFO, 0, "add register request to transactionlist, and send one time first");
    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
    if (ctiot_transaction_send(contextP, transaction) != 0)
    {
        //ct_lwm2m_free(payload);
        //ct_lwm2m_free(query);
        return COAP_503_SERVICE_UNAVAILABLE;
    }
    //ct_lwm2m_free(payload);
    //ct_lwm2m_free(query);
    server->status = STATE_REG_PENDING;

    return COAP_NO_ERROR;
}

void prv_handleRegistrationUpdateReply(lwm2m_transaction_t * transacP,
                                              void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    lwm2m_server_t * targetP = (lwm2m_server_t *)(transacP->userData);
    ctiot_funcv1_status_t atStatus[1]={0};
    ctiot_funcv1_context_t* pTmpContext=ctiot_funcv1_get_context();
    int eventCode = 0;
    
    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        targetP->registration = ct_lwm2m_gettime();
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
#ifndef  FEATURE_REF_AT_ENABLE
            atStatus->baseInfo=ct_lwm2m_strdup(SU0);
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
            ct_lwm2m_free(atStatus->baseInfo);
#else
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_QLWEVTIND, NULL,2);
#endif
            ctiot_funcv1_notify_event(MODULE_LWM2M, UPDATE_SUCCESS, NULL, 0);
        }
        else
        {
            if(packet == NULL)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU2);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                eventCode = UPDATE_FAILED_TIMEOUT;
            }
            if(packet->code == 0x83/*COAP_403*/)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU4);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                eventCode = UPDATE_FAILED_SERVER_FORBIDDEN;
            }
            else if(packet->code == 0x80/*COAP_400*/)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU3);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                eventCode = UPDATE_FAILED_WRONG_PARAM;
            }
            else if(packet->code == 0x84 || packet->code == 0x81/*COAP_404 or COAP_401*/)
            {
                atStatus->baseInfo=ct_lwm2m_strdup(SU1);
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *) atStatus, 0);
                ct_lwm2m_free(atStatus->baseInfo);
                eventCode = UPDATE_FAILED_UE_OFFLINE;
            }
            
            targetP->status = STATE_REG_FAILED;
            pTmpContext->loginStatus = UE_LOG_FAILED;
            ctiot_funcv1_notify_event(MODULE_LWM2M, eventCode, NULL, 0);
#ifdef  FEATURE_REF_AT_ENABLE
            c2f_encode_context(pTmpContext);
#endif
            
        }
    }
}

static int prv_updateRegistration(lwm2m_context_t * contextP,
                                  lwm2m_server_t * server,
                                  bool withObjects)
{
    lwm2m_transaction_t * transaction;
    uint8_t * payload = NULL;
    int payload_length;

    transaction = ctiot_transaction_new(server->sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    coap_set_header_uri_path(transaction->message, server->location);
    cfg_sendOption(transaction->message, SENDMODE_CON_REL);

    if (withObjects == true)
    {
        payload_length = ct_object_getRegisterPayloadBufferLength(contextP);
        if(payload_length == 0)
        {
            ctiot_transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        payload = ct_lwm2m_malloc(payload_length);
        if(!payload)
        {
            ctiot_transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        payload_length = ct_object_getRegisterPayload(contextP, payload, payload_length);
        if(payload_length == 0)
        {
            ctiot_transaction_free(transaction);
            ct_lwm2m_free(payload);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        coap_set_payload(transaction->message, payload, payload_length);
    }

    transaction->callback = prv_handleRegistrationUpdateReply;
    transaction->userData = (void *) server;
    transaction->buffer=NULL;
    transaction->buffer_len=0;
    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    //if (ctiot_transaction_send(contextP, transaction) == 0)
    //{
        server->status = STATE_REG_UPDATE_PENDING;
    //}

    if (withObjects == true)
    {
        //ct_lwm2m_free(payload);
    }
    return COAP_NO_ERROR;
}

// update the registration of a given server
int ct_lwm2m_update_registration(lwm2m_context_t * contextP,
                              uint16_t shortServerID,
                              bool withObjects)
{
    lwm2m_server_t * targetP;
    uint8_t result;

    LOG_ARG("State: %s, shortServerID: %d", STR_STATE(contextP->state), shortServerID);

    result = COAP_NO_ERROR;

    targetP = contextP->serverList;
    if (targetP == NULL)
    {
        if (ct_object_getServers(contextP, false) == -1)
        {
            LOG("No server found");
            return COAP_404_NOT_FOUND;
        }
    }
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        if (shortServerID != 0)
        {
            if (targetP->shortID == shortServerID)
            {
                // found the server, trigger the update transaction
                if (targetP->status == STATE_REGISTERED
                 || targetP->status == STATE_REG_UPDATE_PENDING)
                {
                    if (withObjects == true)
                    {
                        targetP->status = STATE_REG_FULL_UPDATE_NEEDED;
                    }
                    else
                    {
                        targetP->status = STATE_REG_UPDATE_NEEDED;
                    }
                    return COAP_NO_ERROR;
                }
                else if ((targetP->status == STATE_REG_FULL_UPDATE_NEEDED)
                      || (targetP->status == STATE_REG_UPDATE_NEEDED))
                {
                    // if REG (FULL) UPDATE is already set, returns COAP_NO_ERROR
                    if (withObjects == true)
                    {
                        targetP->status = STATE_REG_FULL_UPDATE_NEEDED;
                    }
                    return COAP_NO_ERROR;
                }
                else
                {
                    return COAP_400_BAD_REQUEST;
                }
            }
        }
        else
        {
            if (targetP->status == STATE_REGISTERED
             || targetP->status == STATE_REG_UPDATE_PENDING)
            {
                if (withObjects == true)
                {
                    targetP->status = STATE_REG_FULL_UPDATE_NEEDED;
                }
                else
                {
                    targetP->status = STATE_REG_UPDATE_NEEDED;
                }
            }
        }
        targetP = targetP->next;
    }

    if (shortServerID != 0
     && targetP == NULL)
    {
        // no server found
        result = COAP_404_NOT_FOUND;
    }

    return result;
}
uint8_t ct_registration_start(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
    uint8_t result;

    LOG_ARG("State: %s", STR_STATE(contextP->state));

    result = COAP_NO_ERROR;

    targetP = contextP->serverList;
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        
        if (targetP->status == STATE_DEREGISTERED || targetP->status == STATE_REG_FAILED)
        {
            result = prv_register(contextP, targetP);
        }
        targetP = targetP->next;
    }

    return result;
}


/*
 * Returns STATE_REG_PENDING if at least one registration is still pending
 * Returns STATE_REGISTERED if no registration is pending and there is at least one server the client is registered to
 * Returns STATE_REG_FAILED if all registration failed.
 */
lwm2m_status_t ct_registration_getStatus(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
    lwm2m_status_t reg_status = STATE_REG_FAILED;

    if( contextP == NULL )
        return reg_status;

    LOG_ARG("State: %s", STR_STATE(contextP->state));
    targetP = contextP->serverList;

    while (targetP != NULL)
    {
        LOG_ARG("targetP->status: %s", STR_STATUS(targetP->status));
        switch (targetP->status)
        {
            case STATE_REGISTERED:
            case STATE_REG_UPDATE_NEEDED:
            case STATE_REG_FULL_UPDATE_NEEDED:
            case STATE_REG_UPDATE_PENDING:
                if (reg_status == STATE_REG_FAILED)
                {
                    reg_status = STATE_REGISTERED;
                }
                break;

            case STATE_REG_PENDING:
                reg_status = STATE_REG_PENDING;
                break;

            case STATE_REG_FAILED:
                reg_status = STATE_REG_FAILED;
                break;
            case STATE_DEREG_PENDING:
                reg_status = STATE_DEREG_PENDING;
                break;
            case STATE_DEREGISTERED:
                reg_status = STATE_DEREGISTERED;
                break;
            default:
                break;
        }
        LOG_ARG("reg_status: %s", STR_STATUS(reg_status));

        targetP = targetP->next;
    }

    return reg_status;
}


void ct_registration_deregister(lwm2m_context_t * contextP,
                             lwm2m_server_t * serverP)
{
    coap_packet_t message[1];
    uint8_t       temp_token[COAP_TOKEN_LEN];
    time_t        tv_sec = ct_lwm2m_gettime();
    uint16_t      mID    = contextP->nextMID++;
    
    LOG_ARG("State: %s, serverP->status: %s", STR_STATE(contextP->state), STR_STATUS(serverP->status));

    if (serverP->status == STATE_DEREGISTERED
     || serverP->status == STATE_REG_PENDING
     || serverP->status == STATE_DEREG_PENDING
     || serverP->status == STATE_REG_FAILED
     || serverP->location == NULL)
    {
        return;
    }

    coap_init_message(message, COAP_TYPE_NON, COAP_DELETE, mID);
    coap_set_header_uri_path(message,serverP->location);

    temp_token[0] = mID;
    temp_token[1] = mID >> 8;
    temp_token[2] = tv_sec;
    temp_token[3] = tv_sec >> 8;
    temp_token[4] = tv_sec >> 16;
    temp_token[5] = tv_sec >> 24;
    coap_set_header_token(message, temp_token, 4);
    ct_message_send(contextP, message, serverP->sessionH);
}
#endif

// for each server update the registration if needed
// for each client check if the registration expired
void ct_registration_step(lwm2m_context_t * contextP,
                       time_t currentTime,
                       time_t * timeoutP)
{
#ifdef LWM2M_CLIENT_MODE
    lwm2m_server_t * targetP = contextP->serverList;
    ctiot_funcv1_context_t* pContext=ctiot_funcv1_get_context();
    LOG_ARG("State: %s", STR_STATE(contextP->state));

    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        switch (targetP->status)
        {
        case STATE_REGISTERED:
        {
            if(pContext != NULL && pContext->autoHeartBeat == NO_AUTO_HEARTBEAT)
                break;
            time_t nextUpdate;
            time_t interval;
            
            nextUpdate = targetP->lifetime;
#ifndef FEATURE_REF_AT_ENABLE
            if (ctiot_getCoapMaxTransmitWaitTime() < nextUpdate)
            {
                nextUpdate -= ctiot_getCoapMaxTransmitWaitTime();
            }
            else
            {
                nextUpdate = nextUpdate >> 1;
            }
#else
            nextUpdate = (nextUpdate * 9) / 10;

#endif
            interval = targetP->registration + nextUpdate - currentTime;
            if (0 >= interval)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, registration_step_0, P_INFO, 0, "Updating registration(STATE_REGISTERED)");
                prv_updateRegistration(contextP, targetP, false);
            }
            else if (interval < *timeoutP)
            {
                *timeoutP = interval;
            }
            break;
        }
        case STATE_REG_UPDATE_NEEDED:
            prv_updateRegistration(contextP, targetP, false);
            ECOMM_TRACE(UNILOG_CTLWM2M, registration_step_1, P_INFO, 0, "Updating registration(STATE_REG_UPDATE_NEEDED)");
            break;

        case STATE_REG_FULL_UPDATE_NEEDED:
            ECOMM_TRACE(UNILOG_CTLWM2M, registration_step_2, P_INFO, 0, "Updating registration(STATE_REG_FULL_UPDATE_NEEDED)");
            prv_updateRegistration(contextP, targetP, true);
            break;

        case STATE_REG_FAILED:
            if (targetP->sessionH != NULL)
            {
                ct_lwm2m_close_connection(targetP->sessionH, contextP->userData);
                targetP->sessionH = NULL;
            }
            break;

        default:
            break;
        }
        targetP = targetP->next;
    }

#endif

}

int updateRegistration(lwm2m_context_t * contextP,lwm2m_server_t * server)
{
    return prv_updateRegistration(contextP,server,false);
}

