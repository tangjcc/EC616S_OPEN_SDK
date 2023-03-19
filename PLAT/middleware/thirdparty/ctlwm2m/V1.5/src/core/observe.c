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
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
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


#include <stdio.h>
#include "ct_internals.h"
#include "ctiot_lwm2m_sdk.h"
#include "ctiot_fota.h"

//#include "ctiot_aep_coap.h"
//#include "ctiot_aep_coap_packet.h"

extern void ctiot_funcv1_obj_19_notify_to_mcu( lwm2m_uri_t * uriP, uint32_t count);
extern ctiotFotaManage fotaStateManagement;

#ifdef LWM2M_CLIENT_MODE
static lwm2m_observed_t * prv_findObserved(lwm2m_context_t * contextP,
                                           lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    targetP = contextP->observedList;
    while (targetP != NULL
        && (targetP->uri.objectId != uriP->objectId
         || targetP->uri.flag != uriP->flag
         || (LWM2M_URI_IS_SET_INSTANCE(uriP) && targetP->uri.instanceId != uriP->instanceId)
         || (LWM2M_URI_IS_SET_RESOURCE(uriP) && targetP->uri.resourceId != uriP->resourceId)))
    {
        targetP = targetP->next;
    }

    return targetP;
}

static void prv_unlinkObserved(lwm2m_context_t * contextP,
                               lwm2m_observed_t * observedP)
{
    if (contextP->observedList == observedP)
    {
        contextP->observedList = contextP->observedList->next;
    }
    else
    {
        lwm2m_observed_t * parentP;

        parentP = contextP->observedList;
        while (parentP->next != NULL
            && parentP->next != observedP)
        {
            parentP = parentP->next;
        }
        if (parentP->next != NULL)
        {
            parentP->next = parentP->next->next;
        }
    }
}

static lwm2m_watcher_t * prv_findWatcher(lwm2m_observed_t * observedP,
                                         lwm2m_server_t * serverP)
{
    lwm2m_watcher_t * targetP;

    targetP = observedP->watcherList;
    while (targetP != NULL
        && targetP->server != serverP)
    {
        targetP = targetP->next;
    }

    return targetP;
}

static lwm2m_watcher_t * prv_getWatcher(lwm2m_context_t * contextP,
                                        lwm2m_uri_t * uriP,
                                        lwm2m_server_t * serverP)
{
    lwm2m_observed_t * observedP;
    bool allocatedObserver;
    lwm2m_watcher_t * watcherP;

    allocatedObserver = false;

    observedP = prv_findObserved(contextP, uriP);
    if (observedP == NULL)
    {
        observedP = (lwm2m_observed_t *)ct_lwm2m_malloc(sizeof(lwm2m_observed_t));
        if (observedP == NULL) return NULL;
        allocatedObserver = true;
        memset(observedP, 0, sizeof(lwm2m_observed_t));
        memcpy(&(observedP->uri), uriP, sizeof(lwm2m_uri_t));
        osMutexAcquire(contextP->observe_mutex, osWaitForever);
        observedP->next = contextP->observedList;
        contextP->observedList = observedP;
        osMutexRelease(contextP->observe_mutex);
    }

    watcherP = prv_findWatcher(observedP, serverP);
    if (watcherP == NULL)
    {
        watcherP = (lwm2m_watcher_t *)ct_lwm2m_malloc(sizeof(lwm2m_watcher_t));
        if (watcherP == NULL)
        {
            if (allocatedObserver == true)
            {
                ct_lwm2m_free(observedP);
            }
            return NULL;
        }
        memset(watcherP, 0, sizeof(lwm2m_watcher_t));
        watcherP->active = false;
        watcherP->server = serverP;
        osMutexAcquire(contextP->observe_mutex, osWaitForever);
        watcherP->next = observedP->watcherList;
        observedP->watcherList = watcherP;
        osMutexRelease(contextP->observe_mutex);
    }

    return watcherP;
}

uint8_t ct_observe_handleRequest(lwm2m_context_t * contextP,
                              lwm2m_uri_t * uriP,
                              lwm2m_server_t * serverP,
                              int size,
                              lwm2m_data_t * dataP,
                              coap_packet_t * message,
                              coap_packet_t * response)
{
    lwm2m_observed_t * observedP;
    lwm2m_watcher_t * watcherP;
    uint32_t count;

    LOG_ARG("Code: %02X, server status: %s", message->code, STR_STATUS(serverP->status));
    LOG_URI(uriP);
    //ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_1, P_INFO, 2, "Code: %d, server status: %d",message->code,serverP->status);
    //ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_2, P_INFO, 3, "urip:%d:%d:%d",uriP->objectId,uriP->instanceId,uriP->resourceId);

    coap_get_header_observe(message, &count);
    //ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_3, P_INFO, 1, "count:%d",count);

    switch (count)
    {
    case 0:
        //printf("LWM2M_URI_IS_SET_RESOURCE(uriP):%d",LWM2M_URI_IS_SET_RESOURCE(uriP));
        ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_4, P_INFO, 0, "want observe");
        if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;
        if (message->token_len == 0) return COAP_400_BAD_REQUEST;

        watcherP = prv_getWatcher(contextP, uriP, serverP);
        if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        watcherP->tokenLen = message->token_len;
        memcpy(watcherP->token, message->token, message->token_len);
        watcherP->active = true;
        watcherP->lastTime = ct_lwm2m_gettime();
        watcherP->lastMid = response->mid;
        if (IS_OPTION(message, COAP_OPTION_ACCEPT))
        {
            watcherP->format = utils_convertMediaType((coap_content_type_t)(message->accept[0]));
        }
        else
        {
            if ((uriP->objectId == 5) && (uriP->instanceId == 0) && (uriP->resourceId == 3))
                watcherP->format = LWM2M_CONTENT_TEXT;
            else
                watcherP->format = LWM2M_CONTENT_TLV;
        }
        //printf("ct_observe_handleRequest:3\r\n");
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            //printf("LWM2M_URI_IS_SET_RESOURCE\r\n");
            //printf("dataP->type=%d\r\n",dataP->type);
            if(uriP->objectId!=19)
            {
                switch (dataP->type)
                {
                case LWM2M_TYPE_INTEGER:
                    if (1 != ct_lwm2m_data_decode_int(dataP, &(watcherP->lastValue.asInteger))) return COAP_500_INTERNAL_SERVER_ERROR;
                    break;
                case LWM2M_TYPE_FLOAT:
                    if (1 != ct_lwm2m_data_decode_float(dataP, &(watcherP->lastValue.asFloat))) return COAP_500_INTERNAL_SERVER_ERROR;
                    break;
                default:
                    break;
                }
            }
        }
        //printf("ct_observe_handleRequest:4\r\n");
        if(uriP->objectId == 19){//maybe the observe has already restore
            ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_4_1, P_INFO, 1, "watcherP->counter=%d",watcherP->counter);
            coap_set_header_observe(response, watcherP->counter);
        }else{
            coap_set_header_observe(response, watcherP->counter++);
        }
        //printf("ct_observe_handleRequest:5\r\n");
        ctiot_funcv1_obj_19_notify_to_mcu(uriP, count);
        return COAP_205_CONTENT;

    case 1:
        // cancellation
        observedP = prv_findObserved(contextP, uriP);
        ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_5, P_INFO, 0, "want cancel observe");
        if (observedP)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_6, P_INFO, 0, "find observedP");
            watcherP = prv_findWatcher(observedP, serverP);
            if (watcherP)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_7, P_INFO, 0, "find watcherP");
                ct_observe_cancel(contextP, watcherP->lastMid, serverP->sessionH);
                ctiot_funcv1_obj_19_notify_to_mcu(uriP, count);
                if (uriP->objectId == 5&& uriP->instanceId == 0 && uriP->resourceId == 3)
                {
                    if(fotaStateManagement.fotaState == FOTA_STATE_IDIL && fotaStateManagement.fotaResult == FOTA_RESULT_SUCCESS)
                    {
                        ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_8, P_INFO, 0, "fota update success show update over");
                        ctiot_funcv1_status_t wakeStatus[1]={0};
                        wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA6);
#ifndef FEATURE_REF_AT_ENABLE
                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
                        ct_lwm2m_free(wakeStatus->baseInfo);
#else       
                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, wakeStatus, 0);
#endif
                        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_OVER, NULL, 0);
                    }
                    ECOMM_TRACE(UNILOG_CTLWM2M, observe_handleRequest_0, P_INFO, 0, "observe cancel /5/0/3");
                }
            }
            
        }
        
        return COAP_205_CONTENT;

    default:
        return COAP_400_BAD_REQUEST;
    }
}

void ct_observe_cancel(lwm2m_context_t * contextP,
                    uint16_t mid,
                    void * fromSessionH)
{
    lwm2m_observed_t * observedP;

    LOG_ARG("mid: %d", mid);

    for (observedP = contextP->observedList;
         observedP != NULL;
         observedP = observedP->next)
    {
        lwm2m_watcher_t * targetP = NULL;

        if (observedP->watcherList->lastMid == mid
         && ct_lwm2m_session_is_equal(observedP->watcherList->server->sessionH, fromSessionH, contextP->userData))
        {
            targetP = observedP->watcherList;
            observedP->watcherList = observedP->watcherList->next;
        }
        else
        {
            lwm2m_watcher_t * parentP;

            parentP = observedP->watcherList;
            while (parentP->next != NULL
                && (parentP->next->lastMid != mid
                 || !ct_lwm2m_session_is_equal(parentP->next->server->sessionH, fromSessionH, contextP->userData)))
            {
                parentP = parentP->next;
            }
            if (parentP->next != NULL)
            {
                targetP = parentP->next;
                parentP->next = parentP->next->next;
            }
        }
        if (targetP != NULL)
        {
            osMutexAcquire(contextP->observe_mutex, osWaitForever);
            #if 0
            if(observedP != NULL )
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, observe_cancel_1, P_INFO, 1, "send 32 msgid=%d",mid);
                ctiot_funcv1_status_t pTmpStatus[1] = {0};
                pTmpStatus->baseInfo = ct_lwm2m_strdup(SS6);
                pTmpStatus->extraInfo = &mid;
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, pTmpStatus, 0);
            }
            #endif 
            if (targetP->parameters != NULL) ct_lwm2m_free(targetP->parameters);
            ct_lwm2m_free(targetP);

            if (observedP->watcherList == NULL)
            {
                prv_unlinkObserved(contextP, observedP);
                ct_lwm2m_free(observedP);
            }
            osMutexRelease(contextP->observe_mutex);

            return;
        }
    }
}

void ct_observe_clear(lwm2m_context_t * contextP,
                   lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * observedP;

    LOG_URI(uriP);

    observedP = contextP->observedList;
    while(observedP != NULL)
    {
        if (observedP->uri.objectId == uriP->objectId
            && (LWM2M_URI_IS_SET_INSTANCE(uriP) == false
                || observedP->uri.instanceId == uriP->instanceId))
        {
            lwm2m_observed_t * nextP;
            lwm2m_watcher_t * watcherP;
            osMutexAcquire(contextP->observe_mutex, osWaitForever);

            nextP = observedP->next;

            for (watcherP = observedP->watcherList; watcherP != NULL; watcherP = watcherP->next)
            {
                if (watcherP->parameters != NULL) ct_lwm2m_free(watcherP->parameters);
            }
            LWM2M_LIST_FREE(observedP->watcherList);

            prv_unlinkObserved(contextP, observedP);
            ct_lwm2m_free(observedP);

            observedP = nextP;
            osMutexRelease(contextP->observe_mutex);
        }
        else
        {
            observedP = observedP->next;
        }
    }
}

uint8_t ct_observe_setParameters(lwm2m_context_t * contextP,
                              lwm2m_uri_t * uriP,
                              lwm2m_server_t * serverP,
                              lwm2m_attributes_t * attrP)
{
    uint8_t result;
    lwm2m_watcher_t * watcherP;

    LOG_URI(uriP);
    LOG_ARG("toSet: %08X, toClear: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
            attrP->toSet, attrP->toClear, attrP->minPeriod, attrP->maxPeriod, attrP->greaterThan, attrP->lessThan, attrP->step);

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;

    result = ct_object_checkReadable(contextP, uriP, attrP);
    if (COAP_205_CONTENT != result) return result;

    watcherP = prv_getWatcher(contextP, uriP, serverP);
    if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    // Check rule “lt” value + 2*”stp” values < “gt” value
    if ((((attrP->toSet | (watcherP->parameters?watcherP->parameters->toSet:0)) & ~attrP->toClear) & ATTR_FLAG_NUMERIC) == ATTR_FLAG_NUMERIC)
    {
        float gt;
        float lt;
        float stp;

        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_GREATER_THAN))
        {
            gt = attrP->greaterThan;
        }
        else
        {
            gt = watcherP->parameters->greaterThan;
        }
        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_LESS_THAN))
        {
            lt = attrP->lessThan;
        }
        else
        {
            lt = watcherP->parameters->lessThan;
        }
        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_STEP))
        {
            stp = attrP->step;
        }
        else
        {
            stp = watcherP->parameters->step;
        }

        //if (lt + (2 * stp) >= gt) return COAP_400_BAD_REQUEST;
        if (gt + (2 * stp) >= lt) return COAP_400_BAD_REQUEST;
    }

    if (watcherP->parameters == NULL)
    {
        if (attrP->toSet != 0)
        {
            watcherP->parameters = (lwm2m_attributes_t *)ct_lwm2m_malloc(sizeof(lwm2m_attributes_t));
            if (watcherP->parameters == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
            memcpy(watcherP->parameters, attrP, sizeof(lwm2m_attributes_t));
        }
    }
    else
    {
        watcherP->parameters->toSet &= ~attrP->toClear;
        if (attrP->toSet & LWM2M_ATTR_FLAG_MIN_PERIOD)
        {
            watcherP->parameters->minPeriod = attrP->minPeriod;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD)
        {
            watcherP->parameters->maxPeriod = attrP->maxPeriod;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_GREATER_THAN)
        {
            watcherP->parameters->greaterThan = attrP->greaterThan;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_LESS_THAN)
        {
            watcherP->parameters->lessThan = attrP->lessThan;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_STEP)
        {
            watcherP->parameters->step = attrP->step;
        }
    }

    LOG_ARG("Final toSet: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
            watcherP->parameters->toSet, watcherP->parameters->minPeriod, watcherP->parameters->maxPeriod, watcherP->parameters->greaterThan, watcherP->parameters->lessThan, watcherP->parameters->step);

    return COAP_204_CHANGED;
}

lwm2m_observed_t * ct_observe_findByUri(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_URI(uriP);
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if ((!LWM2M_URI_IS_SET_INSTANCE(uriP) && !LWM2M_URI_IS_SET_INSTANCE(&(targetP->uri)))
             || (LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_INSTANCE(&(targetP->uri)) && (uriP->instanceId == targetP->uri.instanceId)))
             {
                 if ((!LWM2M_URI_IS_SET_RESOURCE(uriP) && !LWM2M_URI_IS_SET_RESOURCE(&(targetP->uri)))
                     || (LWM2M_URI_IS_SET_RESOURCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(&(targetP->uri)) && (uriP->resourceId == targetP->uri.resourceId)))
                 {
                     LOG_ARG("Found one with%s observers.", targetP->watcherList ? "" : " no");
                     LOG_URI(&(targetP->uri));
                     return targetP;
                 }
             }
        }
        targetP = targetP->next;
    }

    LOG("Found nothing");
    return NULL;
}

void ct_lwm2m_resource_value_changed(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_URI(uriP);
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
             || (targetP->uri.flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0
             || uriP->instanceId == targetP->uri.instanceId)
            {
                if (!LWM2M_URI_IS_SET_RESOURCE(uriP)
                 || (targetP->uri.flag & LWM2M_URI_FLAG_RESOURCE_ID) == 0
                 || uriP->resourceId == targetP->uri.resourceId)
                {
                    lwm2m_watcher_t * watcherP;

                    LOG("Found an observation");
                    ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_resource_value_changed_0, P_INFO, 3, "Found an observation=%d-%d-%d", targetP->uri.objectId,targetP->uri.instanceId,targetP->uri.resourceId);
                    LOG_URI(&(targetP->uri));

                    for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
                    {
                        if (watcherP->active == true)
                        {
                            LOG("Tagging a watcher");

                            ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_resource_value_changed_1, P_INFO, 0, "Tagging a watcher");
                            watcherP->update = true;
                        }
                    }
                }
            }
        }
        targetP = targetP->next;
    }
}

void ct_observe_step(lwm2m_context_t * contextP,
                  time_t currentTime,
                  time_t * timeoutP)
{
    lwm2m_observed_t * targetP;

    lwm2m_transaction_t * transaction;
    bool bNotfreeBuf = false;
    
    LOG("Entering");
    for (targetP = contextP->observedList ; targetP != NULL ; targetP = targetP->next)
    {
        lwm2m_watcher_t * watcherP;
        uint8_t * buffer = NULL;
        size_t length = 0;
        lwm2m_data_t * dataP = NULL;
        int size = 0;
        double floatValue = 0;
        int64_t integerValue = 0;
        bool storeValue = false;
        coap_packet_t message[1];
        time_t interval;
        lwm2m_uri_t *uriP = &targetP->uri;

        LOG_URI(&(targetP->uri));
        if (LWM2M_URI_IS_SET_RESOURCE(&targetP->uri))
        {
            if(uriP->objectId == 19 || (uriP->objectId == 3 && !(uriP->instanceId == 0 && uriP->resourceId == 3)))
            {
                continue;
            }
            else if (COAP_205_CONTENT != ct_object_readData(contextP, &targetP->uri, &size, &dataP))
            { 
                continue;
            }
            
            //ECOMM_TRACE(UNILOG_CTLWM2M, observe_step_01, P_INFO, 1, "ct_observe_step dataP type=%d",dataP->type);
            switch (dataP->type)
            {
            case LWM2M_TYPE_INTEGER:
                if (1 != ct_lwm2m_data_decode_int(dataP, &integerValue))
                {
                    ct_lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            case LWM2M_TYPE_FLOAT:
                if (1 != ct_lwm2m_data_decode_float(dataP, &floatValue))
                {
                    ct_lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            default:
                break;
            }
        }
        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            if (watcherP->active == true)
            {
                bool notify = false;
                if (watcherP->update == true)
                {   
                    // value changed, should we notify the server ?

                    if (watcherP->parameters == NULL || watcherP->parameters->toSet == 0)
                    {
                        // no conditions
                        notify = true;
                        LOG("Notify with no conditions");
                        ECOMM_TRACE(UNILOG_CTLWM2M, observe_step_1, P_INFO, 3, "Notify %d/%d/%d", targetP->uri.objectId, targetP->uri.instanceId, targetP->uri.resourceId);
                        LOG_URI(&(targetP->uri));
                    }

                    
                    if (notify == false
                     && watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & ATTR_FLAG_NUMERIC) != 0)
                    {
                        
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_LESS_THAN) != 0)
                        {
                            LOG("Checking lower threshold");
                            // Did we cross the lower threshold ?
                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:                    
                                if ((integerValue <= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->lessThan)
                                 || (integerValue >= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->lessThan))
                                {
                                    LOG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue <= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->lessThan)
                                 || (floatValue >= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat < watcherP->parameters->lessThan))
                                {
                                    LOG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_GREATER_THAN) != 0)
                        {
                            LOG("Checking upper threshold");
                            // Did we cross the upper threshold ?
                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue <= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->greaterThan)
                                 || (integerValue >= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->greaterThan))
                                {
                                    LOG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue <= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->greaterThan)
                                 || (floatValue >= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat < watcherP->parameters->greaterThan))
                                {
                                    LOG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_STEP) != 0)
                        {
                            LOG("Checking step");

                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                            {
                                int64_t diff;

                                diff = integerValue - watcherP->lastValue.asInteger;
                                if ((diff < 0 && (0 - diff) >= watcherP->parameters->step)
                                 || (diff >= 0 && diff >= watcherP->parameters->step))
                                {
                                    LOG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            case LWM2M_TYPE_FLOAT:
                            {
                                double diff;

                                diff = floatValue - watcherP->lastValue.asFloat;
                                if ((diff < 0 && (0 - diff) >= watcherP->parameters->step)
                                 || (diff >= 0 && diff >= watcherP->parameters->step))
                                {
                                    LOG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            default:
                                break;
                            }
                        }
                    }

                    if (watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MIN_PERIOD) != 0)
                    {
                        LOG_ARG("Checking minimal period (%d s)", watcherP->parameters->minPeriod);

                        if (watcherP->lastTime + watcherP->parameters->minPeriod > currentTime)
                        {
                            // Minimum Period did not elapse yet
                            interval = watcherP->lastTime + watcherP->parameters->minPeriod - currentTime;
                            if (*timeoutP > interval) *timeoutP = interval;
                            notify = false;
                        }
                        else
                        {
                            LOG("Notify on minimal period");
                            notify = true;
                        }
                    }
               }

                // Is the Maximum Period reached ?
                if (notify == false
                 && watcherP->parameters != NULL
                 && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    LOG_ARG("Checking maximal period (%d s)", watcherP->parameters->maxPeriod);

                    if (watcherP->lastTime + watcherP->parameters->maxPeriod <= currentTime)
                    {
                        LOG("Notify on maximal period");
                        notify = true;
                    }
                }

                if (notify == true)
                {
                    if (dataP != NULL)
                    {
                        int res;

                        res = ct_lwm2m_data_serialize(&targetP->uri, size, dataP, &(watcherP->format), &buffer);
                        if (res < 0)
                        {
                            break;
                        }
                        else
                        {
                            length = (size_t)res;
                        }

                    }
                    else
                    {
                    
                        ECOMM_TRACE(UNILOG_CTLWM2M, observe_step_2_1, P_INFO, 0, "call object read");
                        if (COAP_205_CONTENT != ct_object_read(contextP, &targetP->uri, &(watcherP->format), &buffer, &length))
                        {
                            buffer = NULL;
                            break;
                        }
                    }
                    if(uriP->objectId == 5 && uriP->instanceId == 0 && uriP->resourceId == 3 && fotaStateManagement.fotaState == FOTA_STATE_DOWNLOADED)
                    {
                        ECOMM_TRACE(UNILOG_CTLWM2M, observe_step_2, P_INFO, 0, "send con msg");
                        transaction=(lwm2m_transaction_t *)ct_lwm2m_malloc(sizeof(lwm2m_transaction_t));
                        memset(transaction, 0, sizeof(lwm2m_transaction_t));
                        transaction->message = ct_lwm2m_malloc(sizeof(coap_packet_t));
                        coap_init_message(transaction->message, COAP_TYPE_CON, COAP_205_CONTENT, 0);
                        coap_set_header_content_type(transaction->message, watcherP->format);
                        coap_set_payload(transaction->message, buffer, length);
                        bNotfreeBuf = true;
                        watcherP->lastTime = currentTime;
                        watcherP->lastMid = contextP->nextMID++;
                        ((coap_packet_t*)transaction->message)->mid = watcherP->lastMid;
                        coap_set_header_token(transaction->message, watcherP->token, watcherP->tokenLen);
                        coap_set_header_observe(transaction->message, watcherP->counter++);
                        ((coap_packet_t*)transaction->message)->sendOption = SEND_OPTION_NORMAL;
                        transaction->mID=watcherP->lastMid;
                        transaction->seqnum=0;
                        transaction->peerH=watcherP->server->sessionH;
                        transaction->userData=(void*)watcherP->server;
                        transaction->peerH=contextP->serverList->sessionH;
                        transaction->userData=contextP->serverList;
                        transaction->callback = (lwm2m_transaction_callback_t)notify_fotastate_con;
                        contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
                        ctiot_transaction_send(contextP, transaction);
                    }
                    else
                    {
                        coap_init_message(message, COAP_TYPE_NON, COAP_205_CONTENT, 0);
                        coap_set_header_content_type(message, watcherP->format);
                        coap_set_payload(message, buffer, length);
                        watcherP->lastTime = currentTime;
                        watcherP->lastMid = contextP->nextMID++;
                        message->mid = watcherP->lastMid;
                        coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
                        coap_set_header_observe(message, watcherP->counter++);
                        ct_message_send(contextP, message, watcherP->server->sessionH);
                    }
                    osMutexAcquire(contextP->observe_mutex, osWaitForever);
                    watcherP->update = false;
                    osMutexRelease(contextP->observe_mutex);
                }

                // Store this value
                if (notify == true && storeValue == true)
                {
                    switch (dataP->type)
                    {
                    case LWM2M_TYPE_INTEGER:
                        watcherP->lastValue.asInteger = integerValue;
                        break;
                    case LWM2M_TYPE_FLOAT:
                        watcherP->lastValue.asFloat = floatValue;
                        break;
                    default:
                        break;
                    }
                }

                if (watcherP->parameters != NULL && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    // update timers
                    interval = watcherP->lastTime + watcherP->parameters->maxPeriod - currentTime;
                    if (*timeoutP > interval) *timeoutP = interval;
                }
            }
        }
        if (dataP != NULL) ct_lwm2m_data_free(size, dataP);
        if (buffer != NULL && !bNotfreeBuf)
        {
            ct_lwm2m_free(buffer);
            buffer = NULL;
        }
    }
}

#endif

