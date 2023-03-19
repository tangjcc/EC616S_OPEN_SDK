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
 *    Baijie & Longrong, China Mobile - Please refer to git log
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
#include "cis_internals.h"
#include "cis_log.h"

#if CIS_ENABLE_DM
#include "dm_utils/dm_endpoint.h"//EC add path avoid compile error

#endif
#include "debug_log.h"

#define MAX_LOCATION_LENGTH 10      // utils_strlen("/rd/65534") + 1

#if CIS_ENABLE_DM
static BOOL updateEventHasSend = FALSE;
#endif

static int prv_getRegistrationQuery(st_context_t * contextP,
                                    st_server_t * server,
                                    char * buffer,
                                    size_t length)
{
    int index;
    int res;

    index = cis_utils_stringCopy(buffer, length, "?ep=");
    if (index < 0) 
    {
        return 0;
    }
    res = cis_utils_stringCopy(buffer + index, length - index, contextP->endpointName);
    if (res < 0) 
    {
        return 0;
    }
    index += res;

    res = cis_utils_stringCopy(buffer + index, length - index, "&b=U");
    
    /*switch (server->binding)
    {
    case BINDING_U:
        res = cis_utils_stringCopy(buffer + index, length - index, "&b=U");
        break;        
    case BINDING_UQ:
        res = cis_utils_stringCopy(buffer + index, length - index, "&b=UQ");
        break;
    case BINDING_S:
         res = cis_utils_stringCopy(buffer + index, length - index, "&b=S");
         break;
     case BINDING_SQ:
         res = cis_utils_stringCopy(buffer + index, length - index, "&b=SQ");
         break;
     case BINDING_US:
         res = cis_utils_stringCopy(buffer + index, length - index, "&b=US");
         break;
     case BINDING_UQS:
         res = cis_utils_stringCopy(buffer + index, length - index, "&b=UQS");
         break;
    default:
        res = -1;
    }*/

    if (res < 0) 
    {
        return 0;
    }

    return index + res;
}


static void prv_handleRegistrationReply(st_transaction_t * transacP,
                                        void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    st_server_t * targetP = (st_server_t*)(transacP->server);
    st_context_t* context = (st_context_t*)transacP->userData;
    if(targetP == NULL && context == NULL){
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }

    if(targetP == NULL){
        LOGE("ERROR:Registration failed,server is NULL");
        return;
    }

    if (targetP->status == STATE_REG_PENDING)
    {
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }

        if (packet != NULL && packet->code == COAP_201_CREATED)
        {
            
            targetP->status = STATE_REGISTERED;
            if (NULL != targetP->location)
            {
                cis_free(targetP->location);
            }
            targetP->location = coap_get_multi_option_as_string(packet->location_path);
            if(context->isDM){
                context->location = (char *)cissys_malloc(strlen(targetP->location)+1);
                strcpy(context->location, targetP->location);
                ECOMM_STRING(UNILOG_ONENET,prv_handleRegistrationReply_1,P_SIG,"Registration successful save location=%s",(uint8_t*)context->location);
            }
            core_callbackEvent(context,CIS_EVENT_REG_SUCCESS,NULL);
            LOGI("Registration successful");
        }
        else if(packet != NULL)
        {
            ECOMM_STRING(UNILOG_ONENET,prv_handleRegistrationReply_2,P_SIG,0,"Registr failed callbackEvent");
            core_callbackEvent(context,CIS_EVENT_REG_FAILED,NULL);

            targetP->status = STATE_REG_FAILED;
            LOGE("ERROR:Registration failed");
        }else
        {
            core_callbackEvent(context,CIS_EVENT_REG_TIMEOUT,NULL);

            targetP->status = STATE_REG_FAILED;
            LOGE("ERROR:Registration timeout");
        
        }
    }
}

#define PRV_QUERY_BUFFER_LENGTH 200

// send the registration for the server
static coap_status_t prv_register(st_context_t * contextP)
{
    
    char* query = NULL;
    uint8_t* payload = NULL;  
    int query_length = 0;
    int payload_length = 0;
    st_transaction_t * transaction = NULL;
    coap_status_t result = COAP_NO_ERROR; 

    st_server_t* server = contextP->server;
    query = (char*)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
    if(query == NULL){
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }
    payload = (uint8_t*)cis_malloc(CIS_COFNIG_REG_PAYLOAD_SIZE);
    if(payload == NULL){
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    payload_length = cis_object_getRegisterPayload(contextP, payload, CIS_COFNIG_REG_PAYLOAD_SIZE - 1);
    if (payload_length == 0) 
    {
       result = COAP_500_INTERNAL_SERVER_ERROR;
       goto TAG_END;
    }
    
    query_length = prv_getRegistrationQuery(contextP, contextP->server, query, CIS_COFNIG_REG_QUERY_SIZE);

    if (query_length == 0) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    if (0 != contextP->lifetime)
    {
        int res;

        res = cis_utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_DELIMITER QUERY_LIFETIME);
        if (res < 0) 
        {
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
        res = utils_intCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, contextP->lifetime);
        if (res < 0) 
        {
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
    }

    if (NULL == server->sessionH) return COAP_503_SERVICE_UNAVAILABLE;
    
    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, contextP->nextMid++, 4, NULL);
    if (transaction == NULL) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    coap_set_header_uri_path(transaction->message, "/"URI_REGISTRATION_SEGMENT);
    coap_set_header_uri_query(transaction->message, query);
    coap_set_header_content_type(transaction->message, LWM2M_CONTENT_LINK);
#if CIS_ENABLE_AUTH
#if CIS_ENABLE_DM
    if(contextP->isDM!=TRUE&&contextP->authCode!=NULL)
#else
     if(contextP->authCode!=NULL)
#endif
     coap_set_header_auth_code( transaction->message, contextP->authCode );
#endif
    coap_set_payload(transaction->message, payload, payload_length);
    transaction->callback = prv_handleRegistrationReply;
    transaction->userData = (void *)contextP;
    transaction->server = contextP->server;
    transaction->regTimeoutTarget = utils_gettime_s() + contextP->regTimeout;
    contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);


    server->status = STATE_REG_PENDING;

    if ( ! transaction_send(contextP, transaction)) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }


TAG_END:
    cis_free(query);
    cis_free(payload);
    return result;
}

static void prv_handleRegistrationUpdateReply(st_transaction_t * transacP,
                                              void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    st_server_t * targetP = (st_server_t *)(transacP->server);
    st_context_t* context = (st_context_t*)transacP->userData;
    if(targetP == NULL && context == NULL){
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }
    ECOMM_TRACE(UNILOG_ONENET, prv_handleRegistrationUpdateReply_1, P_INFO, 1, "(%x)update reply come",context);

    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        ECOMM_TRACE(UNILOG_ONENET, prv_handleRegistrationUpdateReply_2, P_INFO, 1, "(%x)targetP->status == STATE_REG_UPDATE_PENDING",context);
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            if(context->fotaNeedUpdate == 2)
            {
                context->fotaNeedUpdate = 0;
                if(cissys_checkLocalUpdateResult())
                {
                    core_callbackEvent(context,CIS_EVENT_FIRMWARE_UPDATE_SUCCESS,NULL);
                }
                else
                {
                    core_callbackEvent(context,CIS_EVENT_FIRMWARE_UPDATE_FAILED,NULL);
                }
            }
            targetP->status = STATE_REGISTERED;
            core_callbackEvent(context,CIS_EVENT_UPDATE_SUCCESS,NULL);
            ECOMM_TRACE(UNILOG_ONENET, prv_handleRegistrationUpdateReply_3, P_INFO, 1, "(%x)Registration update successful",context);
            LOGD("Registration update successful");
        }
        else if(packet != NULL)
        {
            targetP->status = STATE_REG_FAILED;
            core_callbackEvent(context,CIS_EVENT_UPDATE_FAILED,NULL);
            ECOMM_TRACE(UNILOG_ONENET, prv_handleRegistrationUpdateReply_4, P_INFO, 1, "(%x)packet !=null ERROR:Registration update failed",context);
            LOGE("ERROR:Registration update failed");
        }else
        {
            targetP->status = STATE_REG_FAILED;
            core_callbackEvent(context,CIS_EVENT_UPDATE_TIMEOUT,NULL);
            ECOMM_TRACE(UNILOG_ONENET, prv_handleRegistrationUpdateReply_5, P_INFO, 1, "(%x)ERROR:Registration update failed",context);
            LOGE("ERROR:Registration update failed");
        }
    }
}

static coap_status_t prv_updateRegistration(st_context_t * contextP,
                                  bool withObjects, uint8_t raiflag)
{
    char* query = NULL;
    uint8_t* payload = NULL;
    int payload_length = 0;
    int query_length = 0;
    st_transaction_t * transaction = NULL;
    coap_status_t result = COAP_NO_ERROR;
    st_server_t* server = contextP->server;

    if (contextP->transactionList != NULL)
    {
        return COAP_NO_ERROR;
    }

    query = (char*)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
    if(query == NULL){
        result = COAP_500_INTERNAL_SERVER_ERROR;
        //goto TAG_END;EC add avoid compile warning
    }
    

    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, contextP->nextMid++, 4, NULL);
    if (transaction == NULL) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        //goto TAG_END;EC add avoid compile warning

    }

    coap_set_header_uri_path(transaction->message, server->location);
#if CIS_ENABLE_DM
    uint8_t *dmQuery=NULL;
    int dm_query_length=0;  
  if(contextP->isDM==TRUE)
  {
            dm_query_length = prv_getDmUpdateQueryLength(contextP, server);
            if(dm_query_length == 0)
            {
                result =COAP_500_INTERNAL_SERVER_ERROR;
                goto TAG_END;
            }
            dmQuery = (uint8_t *)cis_malloc(dm_query_length);
            if(!dmQuery)
            {
                result =COAP_500_INTERNAL_SERVER_ERROR;
                goto TAG_END;
            }
            if(prv_getDmUpdateQuery(contextP, server, dmQuery, dm_query_length) != dm_query_length)
            {
                cis_free(dmQuery);
                result =COAP_500_INTERNAL_SERVER_ERROR;
                goto TAG_END;
            }
            coap_set_header_uri_query(transaction->message, (const char *)dmQuery);
            cis_free(dmQuery);
    
  }
 else
 #endif
 {
#if CIS_ENABLE_AUTH
    if(contextP->authCode!=NULL)
     coap_set_header_auth_code( transaction->message, contextP->authCode );
#endif

    if (0 != contextP->lifetime)
    {
        int res;

        res = cis_utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_LIFETIME);
        if (res < 0) 
        {
            transaction_free(transaction);
            result =COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
        res = utils_intCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, contextP->lifetime);
        if (res < 0) 
        {
            transaction_free(transaction);
            result =COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;

        coap_set_header_uri_query(transaction->message, query);
    }

}

    if (withObjects == TRUE)
    {
        payload = (uint8_t*)cis_malloc(CIS_COFNIG_REG_PAYLOAD_SIZE);
        if(payload == NULL){
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        payload_length = cis_object_getRegisterPayload(contextP, payload, CIS_COFNIG_REG_PAYLOAD_SIZE);
        if (payload_length == 0)
        {
            transaction_free(transaction);
            result =COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        coap_set_payload(transaction->message, payload, payload_length);
    }

    transaction->callback = prv_handleRegistrationUpdateReply;
    transaction->userData = (void *) contextP;
    transaction->server = contextP->server;
    transaction->raiflag = raiflag;
	
    contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);
    server->status = STATE_REG_UPDATE_PENDING;

    if (!transaction_send(contextP, transaction))
    {
        result = COAP_NO_ERROR;
        goto TAG_END;
    }

    
    
TAG_END:
    cis_free(payload);
    cis_free(query);
    return result;
}

// update the registration
int registration_update_registration(st_context_t * contextP,bool withObjects, uint8_t raiflag)
{
    st_server_t * targetP;
    
    LOGD("update_registration state: %s", STR_STATE(contextP->stateStep));

    targetP = contextP->server;
    if(targetP == NULL)
    {
        // no server found
        ECOMM_TRACE(UNILOG_ONENET, registration_update_registration_1, P_INFO, 0, "ERROR: no server found for update registration!");
        return COAP_404_NOT_FOUND;
    }

    if (targetP->status == STATE_REGISTERED)
    {
        targetP->raiflag = raiflag;
        if(withObjects){
            targetP->status = STATE_REG_UPDATE_NEEDED_WITHOBJECTS;
        }else{
            targetP->status = STATE_REG_UPDATE_NEEDED;
        }
        return COAP_NO_ERROR;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ONENET, registration_update_registration_2, P_INFO, 1, "targetP->status=%d not STATE_REGISTERED",targetP->status);
        return COAP_400_BAD_REQUEST;
    }
}

coap_status_t cis_registration_start(st_context_t * contextP)
{
    st_server_t * targetP;
    uint8_t result;

    result = COAP_NO_ERROR;

    targetP = contextP->server;
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        if (targetP->status == STATE_CONNECTED
         || targetP->status == STATE_REG_FAILED)
        {
            result = prv_register(contextP);
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
et_status_t cis_registration_getStatus(st_context_t * contextP)
{
    st_server_t * targetP;
    et_status_t reg_status;

    targetP = contextP->server;
    reg_status = STATE_REG_FAILED;

    if (targetP != NULL)
    {
        switch (targetP->status)
        {
            case STATE_REGISTERED:
            case STATE_REG_UPDATE_NEEDED:
            case STATE_REG_UPDATE_NEEDED_WITHOBJECTS:
            case STATE_REG_UPDATE_PENDING:
                {
                    if (reg_status == STATE_REG_FAILED)
                    {
                        reg_status = STATE_REGISTERED;
                    }
                }
                break;
            case STATE_REG_PENDING:
                {
                   reg_status = STATE_REG_PENDING;
                }
                break;
            case STATE_DEREG_PENDING:
                {
                    reg_status = STATE_DEREG_PENDING;
                }
                break;
            case STATE_REG_FAILED:
            case STATE_UNCREATED:
            default:
                break;
        }
    }

    return reg_status;
}

static void prv_handleDeregistrationReply(st_transaction_t * transacP,
                                          void * message)
{
    st_server_t * targetP;

    targetP = (st_server_t *)(transacP->server);
    if (NULL != targetP)
    {
        if (targetP->status == STATE_DEREG_PENDING)
        {
            targetP->status = STATE_UNCREATED;
        }
    }
}

void cis_registration_deregister(st_context_t * contextP)
{
    st_transaction_t * transaction;
    st_server_t* serverP;

    serverP = contextP->server;
    if(serverP != NULL)
    {

        LOGD("State: %s, serverP->status: %s", STR_STATE(contextP->stateStep), STR_STATUS(serverP->status));

        if (serverP->status == STATE_UNCREATED
         || serverP->status == STATE_CREATED
         || serverP->status == STATE_CONNECT_PENDING
         || serverP->status == STATE_CONNECTED
         || serverP->status == STATE_REG_PENDING
         || serverP->status == STATE_DEREG_PENDING
         || serverP->status == STATE_REG_FAILED
         || serverP->location == NULL)
        {
            return;
        }

        transaction = transaction_new(COAP_TYPE_CON, COAP_DELETE, NULL, NULL, contextP->nextMid++, 4, NULL);
        if (transaction == NULL) 
        {
            return;
        }

        coap_set_header_uri_path(transaction->message, serverP->location);
#if CIS_ENABLE_AUTH
#if CIS_ENABLE_DM
    if(contextP->isDM!=TRUE&&contextP->authCode!=NULL)
#else
     if(contextP->authCode!=NULL)
#endif
        coap_set_header_auth_code( transaction->message, contextP->authCode );
#endif
        transaction->callback = prv_handleDeregistrationReply;
        transaction->userData = (void *) contextP;
        transaction->server = serverP;
        contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);
        if (transaction_send(contextP, transaction) == TRUE)
        {
            serverP->status = STATE_DEREG_PENDING;
        }
    }
}


// update the registration if needed
// check if the registration expired
void cis_registration_step(st_context_t * contextP,
                       cis_time_t currentTime)
{
    st_server_t * targetP = contextP->server;

    if (targetP != NULL)
    {
        switch (targetP->status)
        {
        case STATE_REGISTERED:
            {
                cis_time_t lifetime;
                cis_time_t interval;
                cis_time_t lasttime;
                cis_time_t notifytime;

                lifetime = contextP->lifetime;
                lasttime = targetP->registration;

                if(lifetime > cissys_getCoapMaxTransmitWaitTime() * 2)
                {
                    notifytime = (cis_time_t)cissys_getCoapMaxTransmitWaitTime();
                }
                else if(lifetime > cissys_getCoapMaxTransmitWaitTime())
                {
                    notifytime = cissys_getCoapMinTransmitWaitTime();
                }
                else
                {
                    notifytime = (cis_time_t)(lifetime * (1 - ((float)lifetime / 120)));
                }

                interval = lasttime + lifetime - currentTime;    
#if CIS_ENABLE_DM
                if(contextP->isDM)
                {
                    if(interval <= 1 && updateEventHasSend == FALSE){
                        updateEventHasSend = TRUE;
                        ECOMM_TRACE(UNILOG_ONENET, registration_step_1_3, P_INFO, 3, "EVNET_UPDATE_NEED,currenttime=%d,warningtime=%d,interval=%d",currentTime,contextP->lifetimeWarnningTime,interval);
                        contextP->callback.onEvent(contextP,CIS_EVENT_UPDATE_NEED,(void*)interval);
                        contextP->lifetimeWarnningTime = currentTime;
                        ECOMM_TRACE(UNILOG_ONENET, registration_step_1_4, P_INFO, 1, "and now warningtime=%d",contextP->lifetimeWarnningTime);
                    }else if(contextP->lifetimeWarnningTime == 0){
                        updateEventHasSend = FALSE;
                    }
                    break;
                }
#endif
                if(interval <= 0)
                {
                    //once event
                    if(contextP->lifetimeWarnningTime >= 0)
                    {
                        ECOMM_TRACE(UNILOG_ONENET, registration_step_2, P_INFO, 1, "lifetimeWarnningTime=%d, lifetime timeout occurred.",contextP->lifetimeWarnningTime);
                        LOGE("ERROR:lifetime timeout occurred.");
                        contextP->callback.onEvent(contextP,CIS_EVENT_LIFETIME_TIMEOUT,NULL);
                        contextP->lifetimeWarnningTime = INFINITE_TIMEOUT;
                        core_updatePumpState(contextP,PUMP_STATE_UNREGISTER);
                    }
                }
                else if(interval <= notifytime)
                {
                    //a half of last interval time;
                    if(contextP->lifetimeWarnningTime == 0 ||
                        currentTime - contextP->lifetimeWarnningTime > interval)
                    {
                        ECOMM_TRACE(UNILOG_ONENET, registration_step_3, P_INFO, 2, "EVNET_UPDATE_NEED,update now.warningtime=%d,currenttime=%d",contextP->lifetimeWarnningTime,currentTime);
                        LOGW("WARNNING:lifetime timeout warnning time:%ds",interval);
                        contextP->callback.onEvent(contextP,CIS_EVENT_UPDATE_NEED,(void*)interval);
                        contextP->lifetimeWarnningTime = currentTime;
                    }
                }
            }
            break;
        case STATE_REG_UPDATE_NEEDED:
            ECOMM_TRACE(UNILOG_ONENET, registration_step_4, P_INFO, 0, "set targetP->status == STATE_REG_UPDATE_NEEDED call prv_updateRegistration");
            prv_updateRegistration(contextP, false, targetP->raiflag);
            break;
        case STATE_REG_UPDATE_NEEDED_WITHOBJECTS:
            ECOMM_TRACE(UNILOG_ONENET, registration_step_5, P_INFO, 1, "(%x)set targetP->status == STATE_REG_UPDATE_NEEDED_WITHOBJECTS call prv_updateRegistration",contextP);
            prv_updateRegistration(contextP, true, targetP->raiflag);
            break;
        case STATE_REG_FAILED:
            if (contextP->server->sessionH != NULL)
            {
                cisnet_disconnect((cisnet_t)contextP->server->sessionH);
                cisnet_destroy((cisnet_t)contextP->server->sessionH);
                contextP->server->sessionH = NULL;
            }
            break;

        default:
            break;
        }
    }

}

