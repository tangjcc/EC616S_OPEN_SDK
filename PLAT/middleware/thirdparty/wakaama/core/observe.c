/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
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

#include "internals.h"
#include <stdio.h>
#include "debug_log.h"


lwm2mObserveBack gLwm2mObserve[MAX_LWM2M_OBSERVE_NUM];//EC add begin

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
        observedP = (lwm2m_observed_t *)lwm2m_malloc(sizeof(lwm2m_observed_t));
        if (observedP == NULL) return NULL;
        allocatedObserver = true;
        memset(observedP, 0, sizeof(lwm2m_observed_t));
        memcpy(&(observedP->uri), uriP, sizeof(lwm2m_uri_t));
        observedP->next = contextP->observedList;
        contextP->observedList = observedP;
    }

    watcherP = prv_findWatcher(observedP, serverP);
    if (watcherP == NULL)
    {
        watcherP = (lwm2m_watcher_t *)lwm2m_malloc(sizeof(lwm2m_watcher_t));
        if (watcherP == NULL)
        {
            if (allocatedObserver == true)
            {
                lwm2m_free(observedP);
            }
            return NULL;
        }
        memset(watcherP, 0, sizeof(lwm2m_watcher_t));
        watcherP->active = false;
        watcherP->server = serverP;
        watcherP->next = observedP->watcherList;
        observedP->watcherList = watcherP;
    }

    return watcherP;
}

//EC add begin
void lwm2m_observe_update_uri(lwm2m_context_t * contextP,
                                        lwm2m_uri_t * uriP,
                                        lwm2m_watcher_t *watcherP)
{
    int i;

    LOG_ARG("update_data URI:flag %d,/%d/%d/%d\n", uriP->flag, uriP->objectId, uriP->instanceId, uriP->resourceId);
    for(i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
        //first need to check save before
        if(gLwm2mObserve[i].is_used == 1 &&
           gLwm2mObserve[i].flag == uriP->flag &&
           gLwm2mObserve[i].objectId == uriP->objectId &&
           gLwm2mObserve[i].instanceId == uriP->instanceId &&
           gLwm2mObserve[i].resourceId == uriP->resourceId) {
            lwm2mObserveBack *backup = &gLwm2mObserve[i];
            backup->lastTime = watcherP->lastTime;
            backup->lastMid = watcherP->lastMid;
            backup->lastValue.asFloat = watcherP->lastValue.asFloat;
            backup->counter = watcherP->counter;
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_update_uri_1, P_INFO, 4, "uri: flag %d, /%d/%d/%d", backup->flag, backup->objectId, backup->instanceId, backup->resourceId);
            //ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_update_uri_2, P_INFO, 1, "secObjInstID: %d", backup->secObjInstID);
            //ECOMM_STRING(UNILOG_LWM2M, lwm2m_observe_update_uri_3, P_INFO, "watcher token: %s", backup->token);
            //ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_update_uri_4, P_INFO, 4, "watcher lt: %d, lm: %d, lv: %f, couter %d\n", backup->lastTime, backup->lastMid, backup->lastValue.asFloat, backup->counter);
            break;
        }
    }
}

void lwm2m_observe_copy_uri(lwm2mObserveBack *backup,
                                    lwm2m_context_t * contextP,
                                    lwm2m_uri_t * uriP,
                                    lwm2m_server_t * serverP)
{
    lwm2m_watcher_t *watcherP;
    backup->flag = uriP->flag;
    backup->objectId = uriP->objectId;
    backup->instanceId = uriP->instanceId;
    backup->resourceId = uriP->resourceId;
    backup->secObjInstID = serverP->secObjInstID;
    watcherP = prv_getWatcher(contextP, uriP, serverP);

    backup->tokenLen =  watcherP->tokenLen;
    memcpy(backup->token, watcherP->token, watcherP->tokenLen);
    backup->lastTime = watcherP->lastTime;
    backup->lastMid = watcherP->lastMid;
    backup->lastValue.asFloat = watcherP->lastValue.asFloat;
    backup->counter = watcherP->counter;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_copy_uri_1, P_INFO, 4, "uri: flag %d, /%d/%d/%d", backup->flag, backup->objectId, backup->instanceId, backup->resourceId);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_copy_uri_2, P_INFO, 1, "secObjInstID: %d", backup->secObjInstID);
    ECOMM_STRING(UNILOG_LWM2M, lwm2m_observe_copy_uri_3, P_INFO, "watcher token: %s", backup->token);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_copy_uri_4, P_INFO, 4, "watcher lastTime: %d, lastMid: %d, lv: %f, couter: %d", backup->lastTime, backup->lastMid, backup->lastValue.asFloat, backup->counter);
}

void lwm2m_observe_save_uri(lwm2m_context_t * contextP,
                                    lwm2m_uri_t * uriP,
                                    lwm2m_server_t * serverP)
{
    int i;
    for(i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
        if(gLwm2mObserve[i].is_used == 1 &&
           gLwm2mObserve[i].flag == uriP->flag &&
           gLwm2mObserve[i].objectId == uriP->objectId &&
           gLwm2mObserve[i].instanceId == uriP->instanceId &&
           gLwm2mObserve[i].resourceId == uriP->resourceId) {
               ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_save_uri_1, P_INFO, 1, "has this uri(objid:%d) update it",uriP->objectId);
               lwm2m_observe_copy_uri(&gLwm2mObserve[i], contextP, uriP, serverP);
               break;
        }
    }
    if(i == MAX_LWM2M_OBSERVE_NUM) {
        for(i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
            //first save. need to check empty item
            if(gLwm2mObserve[i].is_used == 0) {
                //save item.
                ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_save_uri_2, P_INFO, 1, "no this uri(objid:%d) save it",uriP->objectId);
                lwm2m_observe_copy_uri(&gLwm2mObserve[i], contextP, uriP, serverP);
                gLwm2mObserve[i].is_used = 1;
                break;
            }
        }
        if (i == MAX_LWM2M_OBSERVE_NUM) {
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_save_uri_3, P_INFO, 0, "can't save, up to max count");
        }
    }
}

void lwm2m_observe_clean_uri(lwm2m_context_t * contextP, lwm2m_uri_t * uriP)
{
    int i;
    for (i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
        //first need to check if save before
        if (gLwm2mObserve[i].is_used == 1 &&
           gLwm2mObserve[i].flag == uriP->flag &&
           gLwm2mObserve[i].objectId == uriP->objectId &&
           gLwm2mObserve[i].instanceId == uriP->instanceId &&
           gLwm2mObserve[i].resourceId == uriP->resourceId) {
            //save before, just delete
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_clean_uri_1, P_INFO, 1, "has this uri(objid:%d) clean gLwm2mObserve's one uri.\n",uriP->objectId);
            gLwm2mObserve[i].is_used = 0;
            return;
        }
    }
}

void lwm2m_observe_clean_obj(lwm2m_context_t * contextP, uint16_t objId)
{
    int i;
    for (i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
        //first need to check if save before
        if (gLwm2mObserve[i].is_used == 1 &&
           gLwm2mObserve[i].objectId == objId){
            //save before, just delete
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_clean_obj_1, P_INFO, 1, "has this objid:%d clean gLwm2mObserve's one uri.\n",objId);
            gLwm2mObserve[i].is_used = 0;
            return;
        }
    }
}

void lwm2m_observe_clean_data(lwm2m_context_t * contextP)
{
    int i;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_clean_data_1, P_INFO, 0, "if some observe is used. clean it");
    for (i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
        if (gLwm2mObserve[i].is_used == 1)
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_clean_data_2, P_INFO, 2, "gLwm2mObserve[%d] is_used=%d,clean it", i, gLwm2mObserve[i].is_used);
            gLwm2mObserve[i].is_used = 0;
    }
}

void lwm2m_observe_load_data(lwm2m_context_t * contextP)
{
    int i;
    for(i = 0; i < MAX_LWM2M_OBSERVE_NUM; i++) {
        if (gLwm2mObserve[i].is_used == 1) {
            lwm2m_uri_t uriP;
            lwm2m_server_t *serP = NULL;

            uriP.flag = gLwm2mObserve[i].flag;
            uriP.objectId = gLwm2mObserve[i].objectId;
            uriP.instanceId = gLwm2mObserve[i].instanceId;
            uriP.resourceId = gLwm2mObserve[i].resourceId;

            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_load_data_1, P_INFO, 3, "load data retention URI:/%d/%d/%d\n", uriP.objectId, uriP.instanceId, uriP.resourceId);
            serP = contextP->serverList;
            while(serP != NULL) {
                if(serP->secObjInstID == gLwm2mObserve[i].secObjInstID) {
                    break;
                }
                serP = serP->next;
            }
            if(serP != NULL) {
                lwm2m_watcher_t *watcherP = NULL;
                watcherP = prv_getWatcher(contextP, &uriP, serP);
                if (watcherP == NULL) {
                   ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_load_data_2, P_INFO, 0, "watcherP is NULL\n");
                    return;
                }
                ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_load_data_3, P_INFO, 0, "Add watcherP\n");
                watcherP->tokenLen = gLwm2mObserve[i].tokenLen;
                memcpy(watcherP->token, gLwm2mObserve[i].token, gLwm2mObserve[i].tokenLen);
                watcherP->active = true;
                watcherP->update = false;
                watcherP->lastTime = gLwm2mObserve[i].lastTime;
                watcherP->lastMid = gLwm2mObserve[i].lastMid;
                watcherP->lastValue.asFloat = gLwm2mObserve[i].lastValue.asFloat;
                watcherP->counter = gLwm2mObserve[i].counter;
            } else {
                ECOMM_TRACE(UNILOG_LWM2M, lwm2m_observe_load_data_4, P_INFO, 0, "server is NULL\n");
            }
        }
    }
}
//EC add end

uint8_t observe_handleRequest(lwm2m_context_t * contextP,
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
    ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_1, P_INFO, 2, "Code: %d, server status: %d",message->code,serverP->status);
    ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_2, P_INFO, 3, "urip:%d:%d:%d",uriP->objectId,uriP->instanceId,uriP->resourceId);

    coap_get_header_observe(message, &count);
    ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_22, P_INFO, 1, "count:%d",count);

    switch (count)
    {
    case 0:
        if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;
        if (message->token_len == 0) return COAP_400_BAD_REQUEST;

        watcherP = prv_getWatcher(contextP, uriP, serverP);
        if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        watcherP->tokenLen = message->token_len;
        memcpy(watcherP->token, message->token, message->token_len);
        watcherP->active = true;
        watcherP->lastTime = lwm2m_gettime();
        watcherP->lastMid = response->mid;
        if (IS_OPTION(message, COAP_OPTION_ACCEPT))
        {
            watcherP->format = utils_convertMediaType((coap_content_type_t)message->accept[0]);
        }
        else
        {
            watcherP->format = LWM2M_CONTENT_TLV;
        }

        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            switch (dataP->type)
            {
            case LWM2M_TYPE_INTEGER:
                if (1 != lwm2m_data_decode_int(dataP, &(watcherP->lastValue.asInteger))) return COAP_500_INTERNAL_SERVER_ERROR;
                break;
            case LWM2M_TYPE_FLOAT:
                if (1 != lwm2m_data_decode_float(dataP, &(watcherP->lastValue.asFloat))) return COAP_500_INTERNAL_SERVER_ERROR;
                break;
            default:
                break;
            }
        }

        coap_set_header_observe(response, watcherP->counter++);

        // ADD FOR ATCMD EC add begin
        lwm2m_observe_save_uri(contextP, uriP, serverP);
        if (contextP->observeCallback) {
            contextP->observeCallback(0, message->mid, uriP, contextP->observeUserdata);
        }//EC add end
        ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_3, P_INFO, 0, "show observer indication");

        return COAP_205_CONTENT;

    case 1:
        // cancellation
        observedP = prv_findObserved(contextP, uriP);
        ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_4, P_INFO, 0, "want cancel observe");
        if (observedP)
        {
            ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_5, P_INFO, 0, "find observedP");
            watcherP = prv_findWatcher(observedP, serverP);
            if (watcherP)
            {
                ECOMM_TRACE(UNILOG_LWM2M, observe_handleRequest_6, P_INFO, 0, "find watcherP");
                observe_cancel(contextP, watcherP->lastMid, serverP->sessionH);
                // ADD FOR ATCMD EC add begin
                lwm2m_observe_clean_uri(contextP, uriP);
                if(contextP->observeCallback){
                    contextP->observeCallback(1, message->mid, uriP, contextP->observeUserdata);
                }//EC add end
            }
        }
        return COAP_205_CONTENT;

    default:
        return COAP_400_BAD_REQUEST;
    }
}

void observe_cancel(lwm2m_context_t * contextP,
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
         && lwm2m_session_is_equal(observedP->watcherList->server->sessionH, fromSessionH, contextP->userData))
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
                 || !lwm2m_session_is_equal(parentP->next->server->sessionH, fromSessionH, contextP->userData)))
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
            if (targetP->parameters != NULL) lwm2m_free(targetP->parameters);
            lwm2m_free(targetP);
            if (observedP->watcherList == NULL)
            {
                prv_unlinkObserved(contextP, observedP);
                lwm2m_free(observedP);
            }
            return;
        }
    }
}

void observe_clear(lwm2m_context_t * contextP,
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

            nextP = observedP->next;

            for (watcherP = observedP->watcherList; watcherP != NULL; watcherP = watcherP->next)
            {
                if (watcherP->parameters != NULL) lwm2m_free(watcherP->parameters);
            }
            LWM2M_LIST_FREE(observedP->watcherList);

            prv_unlinkObserved(contextP, observedP);
            lwm2m_free(observedP);

            observedP = nextP;
        }
        else
        {
            observedP = observedP->next;
        }
    }
}

uint8_t observe_setParameters(lwm2m_context_t * contextP,
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

    result = object_checkReadable(contextP, uriP, attrP);
    if (COAP_205_CONTENT != result) return result;

    watcherP = prv_getWatcher(contextP, uriP, serverP);
    if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    // Check rule "lt" value + 2*"stp" values < "gt" value
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

        if (lt + (2 * stp) >= gt) return COAP_400_BAD_REQUEST;
    }

    if (watcherP->parameters == NULL)
    {
        if (attrP->toSet != 0)
        {
            watcherP->parameters = (lwm2m_attributes_t *)lwm2m_malloc(sizeof(lwm2m_attributes_t));
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

    // ADD FOR ATCMD EC add begin
    if (contextP->parameterCallback) {
        contextP->parameterCallback(uriP, attrP, contextP->observeUserdata);
    }//EC add end

    return COAP_204_CHANGED;
}

lwm2m_observed_t * observe_findByUri(lwm2m_context_t * contextP,
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

void lwm2m_resource_value_changed(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_URI(uriP);
    targetP = contextP->observedList;
	ECOMM_TRACE(UNILOG_LWM2M, lwm2m_resource_value_changed_0_1, P_INFO, 1, "to find uriP->objid:%d",uriP->objectId);
    while (targetP != NULL)
    {
		ECOMM_TRACE(UNILOG_LWM2M, lwm2m_resource_value_changed_0_2, P_INFO, 1, "targetP->objid:%d",targetP->uri.objectId);
        if (targetP->uri.objectId == uriP->objectId)
        {
			ECOMM_TRACE(UNILOG_LWM2M, lwm2m_resource_value_changed_1, P_INFO, 0, "Found it");
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
                    LOG_URI(&(targetP->uri));
					ECOMM_TRACE(UNILOG_LWM2M, lwm2m_resource_value_changed_2, P_INFO, 0, "Found the uri");

                    for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
                    {
                        if (watcherP->active == true)
                        {
							ECOMM_TRACE(UNILOG_LWM2M, lwm2m_resource_value_changed_3, P_INFO, 0, "watcher is active, Tagging it");
                            LOG("Tagging a watcher");
                            watcherP->update = true;
                        }
                    }
                }
            }
        }
        targetP = targetP->next;
    }
}

void observe_step(lwm2m_context_t * contextP,
                  time_t currentTime,
                  time_t * timeoutP)
{
    lwm2m_observed_t * targetP;

    LOG("Entering");
    ECOMM_TRACE(UNILOG_LWM2M, observe_step_1, P_INFO, 0, "Entering");
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

        LOG_URI(&(targetP->uri));
        ECOMM_TRACE(UNILOG_LWM2M, observe_step_2, P_INFO, 3, "observed list uri=%d:%d:%d",
            targetP->uri.objectId,targetP->uri.instanceId,targetP->uri.resourceId);
        #if 0//EC add
        if (LWM2M_URI_IS_SET_RESOURCE(&targetP->uri))
        {
            if (COAP_205_CONTENT != object_readData(contextP, &targetP->uri, &size, &dataP)) continue;
            switch (dataP->type)
            {
            case LWM2M_TYPE_INTEGER:
                if (1 != lwm2m_data_decode_int(dataP, &integerValue))
                {
                    lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            case LWM2M_TYPE_FLOAT:
                if (1 != lwm2m_data_decode_float(dataP, &floatValue))
                {
                    lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            default:
                break;
            }
        }
        #endif//EC add
        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            ECOMM_TRACE(UNILOG_LWM2M, observe_step_3, P_INFO, 0, "traversin targetP's watcherList");
            if (watcherP->active == true)
            {
                ECOMM_TRACE(UNILOG_LWM2M, observe_step_4, P_INFO, 0, "watcherP->active == true");
                bool notify = false;

                if (watcherP->update == true)
                {
                    // value changed, should we notify the server ?
                    ECOMM_TRACE(UNILOG_LWM2M, observe_step_4_1, P_INFO, 0, "watcherP->update == true");

                    if (watcherP->parameters == NULL || watcherP->parameters->toSet == 0)
                    {
                        // no conditions
                        notify = true;
                        LOG("Notify with no conditions");
                        LOG_URI(&(targetP->uri));
                        ECOMM_TRACE(UNILOG_LWM2M, observe_step_5, P_INFO, 0, "Notify with no conditions");
                    }

                    //EC add#if 0
                    if (notify == false
                     && watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & ATTR_FLAG_NUMERIC) != 0)
                    {
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_LESS_THAN) != 0)
                        {
                            LOG("Checking lower threshold");
                            ECOMM_TRACE(UNILOG_LWM2M, observe_step_6, P_INFO, 0, "Checking lower threshold");
                            // Did we cross the lower threshold ?
                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue < watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->lessThan)
                                 || (integerValue > watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->lessThan))
                                {
                                    LOG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue < watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->lessThan)
                                 || (floatValue > watcherP->parameters->lessThan
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
                            ECOMM_TRACE(UNILOG_LWM2M, observe_step_7, P_INFO, 0, "Checking on upper crossing");
                            // Did we cross the upper threshold ?
                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue < watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->greaterThan)
                                 || (integerValue > watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->greaterThan))
                                {
                                    LOG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue < watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->greaterThan)
                                 || (floatValue > watcherP->parameters->greaterThan
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
                            ECOMM_TRACE(UNILOG_LWM2M, observe_step_8, P_INFO, 0, "Checking step");

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
                        ECOMM_TRACE(UNILOG_LWM2M, observe_step_9, P_INFO, 0, "Checking minimal period");

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
                    //EC add#endif
                }

                //EC add#if 0
                // Is the Maximum Period reached ?
                if (notify == false
                 && watcherP->parameters != NULL
                 && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    LOG_ARG("Checking maximal period (%d s)", watcherP->parameters->maxPeriod);
                    ECOMM_TRACE(UNILOG_LWM2M, observe_step_a, P_INFO, 0, "Checking maximal period");

                    if (watcherP->lastTime + watcherP->parameters->maxPeriod <= currentTime)
                    {
                        LOG("Notify on maximal period");
                        notify = true;
                    }
                }
                //EC add#endif
                if (notify == true)
                {
                    if (buffer == NULL)
                    {
                        dataP = contextP->notifyDataP;//EC add
                        size = 1;//EC add
                        if (dataP != NULL)
                        {
                            int res;
                            ECOMM_TRACE(UNILOG_LWM2M, observe_step_b, P_INFO, 0, "dataP != NULL, use it serialize payload directly");

                            res = lwm2m_data_serialize(&targetP->uri, size, dataP, &(watcherP->format), &buffer);
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
                            ECOMM_TRACE(UNILOG_LWM2M, observe_step_c, P_INFO, 0, "dataP == NULL, call readFun to get it");
                            if (COAP_205_CONTENT != object_read(contextP, &targetP->uri, &(watcherP->format), &buffer, &length))
                            {
                                ECOMM_TRACE(UNILOG_LWM2M, observe_step_d, P_ERROR, 0, "object_read failed!!!");
                                buffer = NULL;
                                break;
                            }
                        }
                        ECOMM_TRACE(UNILOG_LWM2M, observe_step_e, P_INFO, 0, "object_read success!!!");
                        coap_init_message(message, COAP_TYPE_NON, COAP_205_CONTENT, 0);
                        coap_set_header_content_type(message, watcherP->format);
                        coap_set_payload(message, buffer, length);
                    }
                    watcherP->lastTime = currentTime;
                    watcherP->lastMid = contextP->nextMID++;
                    message->mid = watcherP->lastMid;
                    coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
                    coap_set_header_observe(message, watcherP->counter++);
                    (void)message_send(contextP, message, watcherP->server->sessionH);
                    if (contextP->notifyCallback != NULL) contextP->notifyCallback(LWM2M_NOTIFY_TYPE_SEND_CONFIRM, LWM2M_NOTIFY_CODE_SUCCESS, 0);
                    lwm2m_observe_update_uri(contextP, &(targetP->uri), watcherP);//EC add
                    watcherP->update = false;
                    ECOMM_TRACE(UNILOG_LWM2M, observe_step_f, P_INFO, 0, "send message to server");
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
        if (dataP != NULL){
            lwm2m_data_free(size, dataP);
            contextP->notifyDataP = NULL;
        }
        if (buffer != NULL) lwm2m_free(buffer);
    }
}

#endif

#ifdef LWM2M_SERVER_MODE

typedef struct
{
    uint16_t                client;
    lwm2m_uri_t             uri;
    lwm2m_result_callback_t callbackP;
    void *                  userDataP;
    lwm2m_context_t *       contextP;
} cancellation_data_t;

typedef struct
{
    uint16_t                id;
    uint16_t                client;
    lwm2m_uri_t             uri;
    lwm2m_result_callback_t callback;
    void *                  userData;
    lwm2m_context_t *       contextP;
} observation_data_t;



static lwm2m_observation_t * prv_findObservationByURI(lwm2m_client_t * clientP,
                                                      lwm2m_uri_t * uriP)
{
    lwm2m_observation_t * targetP;

    targetP = clientP->observationList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId
         && targetP->uri.flag == uriP->flag
         && targetP->uri.instanceId == uriP->instanceId
         && targetP->uri.resourceId == uriP->resourceId)
        {
            return targetP;
        }

        targetP = targetP->next;
    }

    return targetP;
}

void observe_remove(lwm2m_observation_t * observationP)
{
    LOG("Entering");
    observationP->clientP->observationList = (lwm2m_observation_t *) LWM2M_LIST_RM(observationP->clientP->observationList, observationP->id, NULL);
    lwm2m_free(observationP);
}

static void prv_obsRequestCallback(lwm2m_transaction_t * transacP,
        void * message)
{
    lwm2m_observation_t * observationP = NULL;
    observation_data_t * observationData = (observation_data_t *)transacP->userData;
    coap_packet_t * packet = (coap_packet_t *)message;
    uint8_t code;
    lwm2m_client_t * clientP;
    lwm2m_uri_t * uriP = & observationData->uri;

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)observationData->contextP->clientList, observationData->client);
    if (clientP == NULL)
    {
        observationData->callback(observationData->client,
                &observationData->uri,
                COAP_500_INTERNAL_SERVER_ERROR,  //?
                0, NULL, 0,
                observationData->userData);
        goto end;
    }

    observationP = prv_findObservationByURI(clientP, uriP);

    // Fail it if the latest user intention is cancellation
    if(observationP && observationP->status == STATE_DEREG_PENDING)
    {
        code = COAP_400_BAD_REQUEST;
    }
    else if (message == NULL)
    {
        code = COAP_503_SERVICE_UNAVAILABLE;
    }
    else if (packet->code == COAP_205_CONTENT
            && !IS_OPTION(packet, COAP_OPTION_OBSERVE))
    {
        code = COAP_405_METHOD_NOT_ALLOWED;
    }
    else
    {
        code = packet->code;
    }

    if (code != COAP_205_CONTENT)
    {
        observationData->callback(clientP->internalID,
                &observationData->uri,
                code,
                LWM2M_CONTENT_TEXT, NULL, 0,
                observationData->userData);
    }
    else
    {
        if(observationP == NULL)
        {
            observationP = (lwm2m_observation_t *)lwm2m_malloc(sizeof(*observationP));
            if (observationP == NULL) goto end;
            memset(observationP, 0, sizeof(*observationP));
        }
        else
        {
            observationP->clientP->observationList = (lwm2m_observation_t *) LWM2M_LIST_RM(observationP->clientP->observationList, observationP->id, NULL);

            // give the user chance to free previous observation userData
            // indicator: COAP_202_DELETED and (Length ==0)
            observationP->callback(observationP->clientP->internalID,
                    &observationP->uri,
                    COAP_202_DELETED,
                    0, NULL, 0,
                    observationP->userData);
        }

        observationP->id = observationData->id;
        observationP->clientP = clientP;

        observationP->callback = observationData->callback;
        observationP->userData = observationData->userData;
        observationP->status = STATE_REGISTERED;
        memcpy(&observationP->uri, uriP, sizeof(lwm2m_uri_t));

        observationP->clientP->observationList = (lwm2m_observation_t *)LWM2M_LIST_ADD(observationP->clientP->observationList, observationP);

        observationData->callback(observationData->client,
                &observationData->uri,
                0,
                packet->content_type, packet->payload, packet->payload_len,
                observationData->userData);
    }

end:
    lwm2m_free(observationData);
}


static void prv_obsCancelRequestCallback(lwm2m_transaction_t * transacP,
        void * message)
{
    cancellation_data_t * cancelP = (cancellation_data_t *)transacP->userData;
    coap_packet_t * packet = (coap_packet_t *)message;
    uint8_t code;
    lwm2m_client_t * clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)cancelP->contextP->clientList, cancelP->client);

    if (clientP == NULL)
    {
        cancelP->callbackP(cancelP->client,
                &cancelP->uri,
                COAP_500_INTERNAL_SERVER_ERROR,
                packet->content_type, NULL, 0,
                cancelP->userDataP);
        goto end;
    }

    lwm2m_observation_t * observationP = prv_findObservationByURI(clientP, &cancelP->uri);

    if (message == NULL)
    {
        code = COAP_503_SERVICE_UNAVAILABLE;
    }
    else
    {
        code = packet->code;
    }

    if (code != COAP_205_CONTENT)
    {
        cancelP->callbackP(cancelP->client,
                &cancelP->uri,
                code,
                LWM2M_CONTENT_TEXT, NULL, 0,
                cancelP->userDataP);
    }
    else
    {
        cancelP->callbackP(cancelP->client,
                &cancelP->uri,
                0,
                packet->content_type, packet->payload, packet->payload_len,
                cancelP->userDataP);
    }

    observe_remove(observationP);
end:
    lwm2m_free(cancelP);
}


int lwm2m_observe(lwm2m_context_t * contextP,
        uint16_t clientID,
        lwm2m_uri_t * uriP,
        lwm2m_result_callback_t callback,
        void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_transaction_t * transactionP;
    observation_data_t * observationData;
    lwm2m_observation_t * observationP;
    uint8_t token[4];

    LOG_ARG("clientID: %d", clientID);
    LOG_URI(uriP);

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    observationP = prv_findObservationByURI(clientP, uriP);

    observationData = (observation_data_t *)lwm2m_malloc(sizeof(observation_data_t));
    if (observationData == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(observationData, 0, sizeof(observation_data_t));

    observationData->id = ++clientP->observationId;

    // observationId can overflow. ensure new ID is not already present
    if(lwm2m_list_find((lwm2m_list_t *)clientP->observationList, observationData->id))
    {
        LOG("Can't get available observation ID. Request failed.\n");
        lwm2m_free(observationData);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    memcpy(&observationData->uri, uriP, sizeof(lwm2m_uri_t));

    // don't hold refer to the clientP
    observationData->client = clientP->internalID;
    observationData->callback = callback;
    observationData->userData = userData;
    observationData->contextP = contextP;

    token[0] = clientP->internalID >> 8;
    token[1] = clientP->internalID & 0xFF;
    token[2] = observationData->id >> 8;
    token[3] = observationData->id & 0xFF;

    transactionP = lwm2m_transaction_new(clientP->sessionH, COAP_GET, clientP->altPath, uriP, contextP->nextMID++, 4, token);
    if (transactionP == NULL)
    {
        lwm2m_free(observationData);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    coap_set_header_observe(transactionP->message, 0);
    if (clientP->supportJSON == true)
    {
        coap_set_header_accept(transactionP->message, LWM2M_CONTENT_JSON);
    }
    else
    {
        coap_set_header_accept(transactionP->message, LWM2M_CONTENT_TLV);
    }

    transactionP->callback = prv_obsRequestCallback;
    transactionP->userData = (void *)observationData;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

    // update the user latest intention
    if(observationP) observationP->status = STATE_REG_PENDING;

    int ret = lwm2m_transaction_send(contextP, transactionP);
    if (ret != 0)
    {
        LOG("lwm2m_transaction_send failed!");
        lwm2m_free(observationData);
    }
    return ret;
}

int lwm2m_observe_cancel(lwm2m_context_t * contextP,
        uint16_t clientID,
        lwm2m_uri_t * uriP,
        lwm2m_result_callback_t callback,
        void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_observation_t * observationP;
    int ret;

    LOG_ARG("clientID: %d", clientID);
    LOG_URI(uriP);

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    observationP = prv_findObservationByURI(clientP, uriP);
    if (observationP == NULL) return COAP_404_NOT_FOUND;

    switch (observationP->status)
    {
    case STATE_REGISTERED:
    {
        lwm2m_transaction_t * transactionP;
        cancellation_data_t * cancelP;
        uint8_t token[4];

        token[0] = clientP->internalID >> 8;
        token[1] = clientP->internalID & 0xFF;
        token[2] = observationP->id >> 8;
        token[3] = observationP->id & 0xFF;

        transactionP = lwm2m_transaction_new(clientP->sessionH, COAP_GET, clientP->altPath, uriP, contextP->nextMID++, 4, token);
        if (transactionP == NULL)
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        cancelP = (cancellation_data_t *)lwm2m_malloc(sizeof(cancellation_data_t));
        if (cancelP == NULL)
        {
            lwm2m_free(transactionP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        coap_set_header_observe(transactionP->message, 1);

        // don't hold refer to the clientP
        cancelP->client = clientP->internalID;
        memcpy(&cancelP->uri, uriP, sizeof(lwm2m_uri_t));
        cancelP->callbackP = callback;
        cancelP->userDataP = userData;
        cancelP->contextP = contextP;

        transactionP->callback = prv_obsCancelRequestCallback;
        transactionP->userData = (void *)cancelP;

        contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

        observationP->status = STATE_DEREG_PENDING;

        int ret = lwm2m_transaction_send(contextP, transactionP);
        if (ret != 0) lwm2m_free(cancelP);
        return ret;
    }

    case STATE_REG_PENDING:
        observationP->status = STATE_DEREG_PENDING;
        ret = COAP_204_CHANGED;
        break;

    default:
        // Should not happen
        ret = COAP_IGNORE;
        break;
    }

    // no other chance to remove the observationP since not sending a transaction
    observe_remove(observationP);

    // need to give a indicator (non-zero) to user for properly freeing the userData
    return ret;
}

bool observe_handleNotify(lwm2m_context_t * contextP,
                           void * fromSessionH,
                           coap_packet_t * message,
                           coap_packet_t * response)
{
    uint8_t * tokenP;
    int token_len;
    uint16_t clientID;
    uint16_t obsID;
    lwm2m_client_t * clientP;
    lwm2m_observation_t * observationP;
    uint32_t count;

    LOG("Entering");
    token_len = coap_get_header_token(message, (const uint8_t **)&tokenP);
    if (token_len != sizeof(uint32_t)) return false;

    if (1 != coap_get_header_observe(message, &count)) return false;

    clientID = (tokenP[0] << 8) | tokenP[1];
    obsID = (tokenP[2] << 8) | tokenP[3];

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return false;

    observationP = (lwm2m_observation_t *)lwm2m_list_find((lwm2m_list_t *)clientP->observationList, obsID);
    if (observationP == NULL)
    {
        coap_init_message(response, COAP_TYPE_RST, 0, message->mid);
        message_send(contextP, response, fromSessionH);
    }
    else
    {
        if (message->type == COAP_TYPE_CON ) {
            coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
            message_send(contextP, response, fromSessionH);
        }
        observationP->callback(clientID,
                               &observationP->uri,
                               (int)count,
                               message->content_type, message->payload, message->payload_len,
                               observationP->userData);
    }
    return true;
}
#endif
