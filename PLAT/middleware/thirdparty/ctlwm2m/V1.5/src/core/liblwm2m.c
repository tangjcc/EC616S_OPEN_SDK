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
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Toby Jaffey - Please refer to git log
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ctiot_lwm2m_sdk.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "rng_ec616.h"
#elif defined CHIP_EC616S
#include "rng_ec616s.h"
#endif


lwm2m_context_t * ct_lwm2m_init(void * userData)
{
    lwm2m_context_t * contextP;
    uint8_t Rand[24];
    uint32_t a=0;
    uint32_t b=0;
    uint8_t i=0;
    RngGenRandom(Rand);
    for(i=0; i<24; i++){
        a += Rand[i];
        b ^= Rand[i];
    }
    contextP = (lwm2m_context_t *)ct_lwm2m_malloc(sizeof(lwm2m_context_t));
    if (NULL != contextP)
    {
        memset(contextP, 0, sizeof(lwm2m_context_t));
        contextP->userData = userData;
        contextP->nextMID = (a<<8) + b;
    }
    return contextP;
}

#ifdef LWM2M_CLIENT_MODE

void ct_lwm2m_deregister(lwm2m_context_t * context)
{
    lwm2m_server_t * server = context->serverList;

    LOG("Entering");
    while (NULL != server)
    {
        ct_registration_deregister(context, server);
        server = server->next;
    }
}

static void prv_deleteServer(lwm2m_server_t * serverP, void *userData)
{
    // TODO parse transaction and observation to remove the ones related to this server
    if (serverP->sessionH != NULL)
    {
         ct_lwm2m_close_connection(serverP->sessionH, userData);
         serverP->sessionH = NULL;
    }
    if (NULL != serverP->location)
    {
        ct_lwm2m_free(serverP->location);
    }
    free_block1_buffer(serverP->block1Data);
    ct_lwm2m_free(serverP);
}

void prv_deleteServerList(lwm2m_context_t * context)
{
    while (NULL != context->serverList)
    {
        lwm2m_server_t * server;
        server = context->serverList;
        context->serverList = server->next;
        prv_deleteServer(server, context->userData);
    }
}

static void prv_deleteBootstrapServer(lwm2m_server_t * serverP, void *userData)
{
    // TODO should we free location as in prv_deleteServer ?
    // TODO should we parse transaction and observation to remove the ones related to this server ?
    if (serverP->sessionH != NULL)
    {
         ct_lwm2m_close_connection(serverP->sessionH, userData);
    }
    free_block1_buffer(serverP->block1Data);
    ct_lwm2m_free(serverP);
}

void prv_deleteBootstrapServerList(lwm2m_context_t * context)
{
    while (NULL != context->bootstrapServerList)
    {
        lwm2m_server_t * server;
        server = context->bootstrapServerList;
        context->bootstrapServerList = server->next;
        prv_deleteBootstrapServer(server, context->userData);
    }
}

void prv_deleteObservedList(lwm2m_context_t * contextP)
{
    osMutexAcquire(contextP->observe_mutex, osWaitForever);

    while (NULL != contextP->observedList)
    {
        lwm2m_observed_t * targetP;
        lwm2m_watcher_t * watcherP;

        targetP = contextP->observedList;
        contextP->observedList = contextP->observedList->next;

        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            if (watcherP->parameters != NULL) ct_lwm2m_free(watcherP->parameters);
        }
        LWM2M_LIST_FREE(targetP->watcherList);

        ct_lwm2m_free(targetP);
    }
    osMutexRelease(contextP->observe_mutex);
}
#endif

void ct_prv_deleteTransactionList(lwm2m_context_t * context)
{
    while (NULL != context->transactionList)
    {
        lwm2m_transaction_t * transaction;

        transaction = context->transactionList;
        context->transactionList = context->transactionList->next;
        ctiot_transaction_free(transaction);
    }
}

void ct_lwm2m_close(lwm2m_context_t * contextP)
{
#ifdef LWM2M_CLIENT_MODE

    LOG("Entering");
    //ct_lwm2m_deregister(contextP);
    
    prv_deleteServerList(contextP);
    prv_deleteBootstrapServerList(contextP);
    prv_deleteObservedList(contextP);
    osMutexDelete(contextP->observe_mutex);
    ct_lwm2m_free(contextP->endpointName);
    if (contextP->msisdn != NULL)
    {
        ct_lwm2m_free(contextP->msisdn);
    }
    if (contextP->altPath != NULL)
    {
        ct_lwm2m_free(contextP->altPath);
    }

#endif

    ct_prv_deleteTransactionList(contextP);
    osMutexDelete(contextP->transactMutex);
    ct_lwm2m_free(contextP);
}

#ifdef LWM2M_CLIENT_MODE
static int prv_refreshServerList(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
    lwm2m_server_t * nextP;

    // Remove all servers marked as dirty
    targetP = contextP->bootstrapServerList;
    contextP->bootstrapServerList = NULL;
    while (targetP != NULL)
    {
        nextP = targetP->next;
        targetP->next = NULL;
        if (!targetP->dirty)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_refreshServerList_0, P_INFO, 0, "targetP not dirty add it to contextP->bootstrapServerList");
            targetP->status = STATE_DEREGISTERED;
            contextP->bootstrapServerList = (lwm2m_server_t *)LWM2M_LIST_ADD(contextP->bootstrapServerList, targetP);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_refreshServerList_1, P_INFO, 0, "targetP dirty deleteserver");
            prv_deleteServer(targetP, contextP->userData);
        }
        targetP = nextP;
    }
    targetP = contextP->serverList;
    contextP->serverList = NULL;
    while (targetP != NULL)
    {
        nextP = targetP->next;
        targetP->next = NULL;
        if (!targetP->dirty)
        {
            // TODO: Should we revert the status to STATE_DEREGISTERED ?
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_refreshServerList_2, P_INFO, 0, "targetP not dirty add it to contextP->serverList");
            contextP->serverList = (lwm2m_server_t *)LWM2M_LIST_ADD(contextP->serverList, targetP);
        }
        else
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_refreshServerList_3, P_INFO, 0, "targetP dirty deleteserver");
            prv_deleteServer(targetP, contextP->userData);
        }
        targetP = nextP;
    }

    return ct_object_getServers(contextP, false);
}

int ct_lwm2m_configure(lwm2m_context_t * contextP,
                    const char * endpointName,
                    const char * msisdn,
                    const char * altPath,
                    uint16_t numObject,
                    lwm2m_object_t * objectList[])
{
    int i;
    uint8_t found;

    LOG_ARG("endpointName: \"%s\", msisdn: \"%s\", altPath: \"%s\", numObject: %d", endpointName, msisdn, altPath, numObject);
    // This API can be called only once for now
    if (contextP->endpointName != NULL || contextP->objectList != NULL) return COAP_400_BAD_REQUEST;
    if (endpointName == NULL) return COAP_400_BAD_REQUEST;

    if (numObject < 2/*3*/) return COAP_400_BAD_REQUEST;
    // Check that mandatory objects are present
    found = 0;
    for (i = 0 ; i < numObject ; i++)
    {
        if (objectList[i]->objID == LWM2M_SECURITY_OBJECT_ID) found |= 0x01;
        if (objectList[i]->objID == LWM2M_SERVER_OBJECT_ID) found |= 0x02;
        //if (objectList[i]->objID == LWM2M_DEVICE_OBJECT_ID) found |= 0x04;
    }
    if (found != 0x03/*0x07*/) return COAP_400_BAD_REQUEST;
    if (altPath != NULL)
    {
        if (0 == utils_isAltPathValid(altPath))
        {
            return COAP_400_BAD_REQUEST;
        }
        if (altPath[1] == 0)
        {
            altPath = NULL;
        }
    }
           
    contextP->endpointName = ct_lwm2m_strdup(endpointName);
    if (contextP->endpointName == NULL)
    {
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if (msisdn != NULL)
    {
        contextP->msisdn = ct_lwm2m_strdup(msisdn);
        if (contextP->msisdn == NULL)
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

    if (altPath != NULL)
    {
        contextP->altPath = ct_lwm2m_strdup(altPath);
        if (contextP->altPath == NULL)
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

    for (i = 0; i < numObject; i++)
    {
        objectList[i]->next = NULL;
        contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_ADD(contextP->objectList, objectList[i]);
    }

    return COAP_NO_ERROR;
}

int ct_lwm2m_add_object(lwm2m_context_t * contextP,
                     lwm2m_object_t * objectP)
{
    lwm2m_object_t * targetP;

    LOG_ARG("ID: %d", objectP->objID);
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, objectP->objID);
    if (targetP != NULL) return COAP_406_NOT_ACCEPTABLE;
    objectP->next = NULL;

    contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_ADD(contextP->objectList, objectP);

    if (contextP->state == STATE_READY)
    {
        return ct_lwm2m_update_registration(contextP, 0, true);
    }

    return COAP_NO_ERROR;
}

int ct_lwm2m_remove_object(lwm2m_context_t * contextP,
                        uint16_t id)
{
    lwm2m_object_t * targetP;

    LOG_ARG("ID: %d", id);
    contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_RM(contextP->objectList, id, &targetP);

    if (targetP == NULL) return COAP_404_NOT_FOUND;

    if (contextP->state == STATE_READY)
    {
        return ct_lwm2m_update_registration(contextP, 0, true);
    }

    return 0;
}
int lwm2m_remove_objects(lwm2m_context_t* contextP,uint16_t* ids,uint8_t removeCount)
{
    lwm2m_object_t * targetP;
    int i=0;
    while(i<removeCount)
    {
        contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_RM(contextP->objectList, ids[i], &targetP);
        i++;
    }
    if (contextP->state == STATE_READY)
    {
        return ct_lwm2m_update_registration(contextP, 0, true);
    }

    return 0;
}


#endif


int ct_lwm2m_step(lwm2m_context_t * contextP,
               time_t * timeoutP)
{
    time_t tv_sec;
    int result;

    LOG_ARG("timeoutP: %" PRId64, *timeoutP);
    tv_sec = ct_lwm2m_gettime();
#ifdef LWM2M_CLIENT_MODE
    LOG_ARG("State: %s", STR_STATE(contextP->state));
    // state can also be modified in bootstrap_handleCommand().
    if(contextP==NULL)
    {
        lwm2m_printf("ct_lwm2m_step:contextP is NULL\n");
    }
next_step:
    switch (contextP->state)
    {
        case STATE_INITIAL:
        {
            if (0 != prv_refreshServerList(contextP)) return COAP_503_SERVICE_UNAVAILABLE;

            if (contextP->serverList != NULL)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_step_1, P_INFO, 0, "STATE_INITIAL -> STATE_REGISTER_REQUIRED");
                contextP->state = STATE_REGISTER_REQUIRED;
                contextP->tv_startreg=ct_lwm2m_gettime();
            }
            else
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_step_1_1, P_INFO, 1, "STATE_INITIAL -> STATE_BOOTSTRAP_REQUIRED bsServer shortID=%d",contextP->bootstrapServerList->shortID);
                // Bootstrapping
                contextP->state = STATE_BOOTSTRAP_REQUIRED;
            }
        }
        goto next_step;

        case STATE_BOOTSTRAP_REQUIRED:
#ifdef LWM2M_BOOTSTRAP
            if (contextP->bootstrapServerList != NULL)
            {
                ct_bootstrap_start(contextP);
                contextP->state = STATE_BOOTSTRAPPING;
                ct_bootstrap_step(contextP, tv_sec, timeoutP);
                break;
            }
            else
#endif
           {
                return COAP_503_SERVICE_UNAVAILABLE;
           }

#ifdef LWM2M_BOOTSTRAP
    case STATE_BOOTSTRAPPING:
        switch (ct_bootstrap_getStatus(contextP))
        {
        case STATE_BS_FINISHED:
            contextP->state = STATE_INITIAL;
            goto next_step;
            //break;

        case STATE_BS_FAILED:
            return COAP_503_SERVICE_UNAVAILABLE;

        default:
            // keep on waiting
            ct_bootstrap_step(contextP, tv_sec, timeoutP);
            break;
        }
        break;
#endif
        case STATE_REGISTER_REQUIRED:
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_step_2, P_INFO, 0, "STATE_REGISTER_REQUIRED -> registration_start");
        
            result = ct_registration_start(contextP);
            if (COAP_NO_ERROR != result){
                ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_step_2_1, P_INFO, 1, "ct_registration_start failed return %d",result);
                return result;
            }
            contextP->state = STATE_REGISTERING;
            break;
        }
        case STATE_REGISTERING:
        {
            switch (ct_registration_getStatus(contextP))
            {
                case STATE_REGISTERED:
                contextP->state = STATE_READY;
                break;

                case STATE_REG_FAILED:
                // TODO avoid infinite loop by checking the bootstrap info is different
                ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_step_3, P_INFO, 0, "register failed return error");
                return REGISTER_FAILED;

                case STATE_REG_PENDING:
                default:
                // keep on waiting
                break;
            }
        }
        break;

        case STATE_READY:
            if (ct_registration_getStatus(contextP) == STATE_REG_FAILED)
            {
                // TODO avoid infinite loop by checking the bootstrap info is different
                ECOMM_TRACE(UNILOG_CTLWM2M, lwm2m_step_4, P_INFO, 0, "when ready reg failed return error");
                return REGISTER_FAILED;
            }
            break;

        default:
            // do nothing
            break;
    }
    ct_observe_step(contextP, tv_sec, timeoutP);
#endif
    ct_registration_step(contextP, tv_sec, timeoutP);
    ctiot_transaction_step(contextP, tv_sec, timeoutP);
    LOG_ARG("Final timeoutP: %" PRId64, *timeoutP);
#ifdef LWM2M_CLIENT_MODE
    LOG_ARG("Final state: %s", STR_STATE(contextP->state));
#endif
    return 0;
}
