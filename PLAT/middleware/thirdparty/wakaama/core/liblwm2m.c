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

#include "internals.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include "debug_log.h"

//#include "rng_ec616.h"


extern int32_t RngGenRandom(uint8_t Rand[24]);
uint32_t lwm2m_ecRand()
{
    uint32_t random=0;
    uint32_t a=0;
    uint32_t b=0;
    uint8_t i=0;
    uint8_t Rand[24];
    RngGenRandom(Rand);
    for(i=0; i<24; i++){
        a += Rand[i];
        b ^= Rand[i];
    }
    random = (a<<8) + b;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_ecRand_1, P_INFO, 1, "lwm2m_ecRand:%x",random);
    return random;
}


lwm2m_context_t * lwm2m_init(void * userData)
{
    lwm2m_context_t * contextP = NULL;

    LOG("Entering");
    contextP = (lwm2m_context_t *)lwm2m_malloc(sizeof(lwm2m_context_t));
    if (NULL != contextP)
    {
        memset(contextP, 0, sizeof(lwm2m_context_t));
        contextP->userData = userData;
        srand((int)lwm2m_gettime());
        contextP->nextMID = lwm2m_ecRand();//rand();
    }

    return contextP;
}

#ifdef LWM2M_CLIENT_MODE
void lwm2m_deregister(lwm2m_context_t * context)
{
    lwm2m_server_t * server = context->serverList;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_deregister_1, P_INFO, 1, "server:%x",server);

    LOG("Entering");
    while (NULL != server)
    {
        registration_deregister(context, server);
        server = server->next;
    }
}

static void prv_deleteServer(lwm2m_server_t * serverP, void *userData)
{
    // TODO parse transaction and observation to remove the ones related to this server
    /*EC comment out
    if (serverP->sessionH != NULL)
    {
         lwm2m_close_connection(serverP->sessionH, userData);
    }
    */
    if (NULL != serverP->location)
    {
        lwm2m_free(serverP->location);
    }
    free_block1_buffer(serverP->block1Data);
    lwm2m_free(serverP);
}

static void prv_deleteServerList(lwm2m_context_t * context)
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
    /*EC comment out
    if (serverP->sessionH != NULL)
    {
         lwm2m_close_connection(serverP->sessionH, userData);
    }
    */
    free_block1_buffer(serverP->block1Data);
    lwm2m_free(serverP);
}

static void prv_deleteBootstrapServerList(lwm2m_context_t * context)
{
    while (NULL != context->bootstrapServerList)
    {
        lwm2m_server_t * server;
        server = context->bootstrapServerList;
        context->bootstrapServerList = server->next;
        prv_deleteBootstrapServer(server, context->userData);
    }
}

static void prv_deleteObservedList(lwm2m_context_t * contextP)
{
    while (NULL != contextP->observedList)
    {
        lwm2m_observed_t * targetP;
        lwm2m_watcher_t * watcherP;

        targetP = contextP->observedList;
        contextP->observedList = contextP->observedList->next;

        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            if (watcherP->parameters != NULL) lwm2m_free(watcherP->parameters);
        }
        LWM2M_LIST_FREE(targetP->watcherList);

        lwm2m_free(targetP);
    }
}
#endif

void prv_deleteTransactionList(lwm2m_context_t * context)
{
    while (NULL != context->transactionList)
    {
        lwm2m_transaction_t * transaction;

        transaction = context->transactionList;
        context->transactionList = context->transactionList->next;
        lwm2m_transaction_free(transaction);
    }
}

void lwm2m_close(lwm2m_context_t * contextP)
{
#ifdef LWM2M_CLIENT_MODE

    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_close_1, P_INFO, 0, "enter");
    LOG("Entering");
    //lwm2m_deregister(contextP); EC modify call it ahead
    prv_deleteServerList(contextP);
    prv_deleteBootstrapServerList(contextP);
    prv_deleteObservedList(contextP);
    lwm2m_free(contextP->endpointName);
    if (contextP->msisdn != NULL)
    {
        lwm2m_free(contextP->msisdn);
    }
    if (contextP->altPath != NULL)
    {
        lwm2m_free(contextP->altPath);
    }

#endif

#ifdef LWM2M_SERVER_MODE
    while (NULL != contextP->clientList)
    {
        lwm2m_client_t * clientP;

        clientP = contextP->clientList;
        contextP->clientList = contextP->clientList->next;

        registration_freeClient(clientP);
    }
#endif

    prv_deleteTransactionList(contextP);
    lwm2m_free(contextP);
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
            targetP->status = STATE_DEREGISTERED;
            contextP->bootstrapServerList = (lwm2m_server_t *)LWM2M_LIST_ADD(contextP->bootstrapServerList, targetP);
        }
        else
        {
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
            contextP->serverList = (lwm2m_server_t *)LWM2M_LIST_ADD(contextP->serverList, targetP);
        }
        else
        {
            prv_deleteServer(targetP, contextP->userData);
        }
        targetP = nextP;
    }

    return object_getServers(contextP, false);
}

int lwm2m_configure(lwm2m_context_t * contextP,
                    const char * endpointName,
                    const char * msisdn,
                    const char * altPath,
                    uint16_t numObject,
                    lwm2m_object_t * objectList[])
{
    int i;
    uint8_t found;

    LOG_ARG("endpointName: \"%s\", msisdn: \"%s\", altPath: \"%s\", numObject: %d", endpointName, (msisdn==NULL)?"":msisdn, (altPath==NULL)?"":altPath, numObject);
    // This API can be called only once for now
    if (contextP->endpointName != NULL || contextP->objectList != NULL) return COAP_400_BAD_REQUEST;

    if (endpointName == NULL) return COAP_400_BAD_REQUEST;
    if (numObject < 3) return COAP_400_BAD_REQUEST;
    // Check that mandatory objects are present
    found = 0;
    for (i = 0 ; i < numObject ; i++)
    {
        if (objectList[i]->objID == LWM2M_SECURITY_OBJECT_ID) found |= 0x01;
        if (objectList[i]->objID == LWM2M_SERVER_OBJECT_ID) found |= 0x02;
        if (objectList[i]->objID == LWM2M_DEVICE_OBJECT_ID) found |= 0x04;
    }
    if (found != 0x07) return COAP_400_BAD_REQUEST;
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
    contextP->endpointName = lwm2m_strdup(endpointName);
    if (contextP->endpointName == NULL)
    {
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if (msisdn != NULL)
    {
        contextP->msisdn = lwm2m_strdup(msisdn);
        if (contextP->msisdn == NULL)
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

    if (altPath != NULL)
    {
        contextP->altPath = lwm2m_strdup(altPath);
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
    //EC add begin
    if(contextP->state != STATE_READY) {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2m_configure_0, P_INFO, 1, "contextP->state:%d",contextP->state);
        lwm2m_observe_clean_data(contextP);
        return COAP_NO_ERROR;
    }else{
        ECOMM_TRACE(UNILOG_LWM2M, lwm2m_configure_1, P_INFO, 0, "contextP->state==STATE_READY");
        object_getServers(contextP,false);
        
        lwm2m_server_t * targetP;
        targetP = contextP->serverList;
        while (targetP != NULL)
        {
            targetP->status = STATE_REGISTERED;
            targetP->location = contextP->location;
            ECOMM_STRING(UNILOG_LWM2M, lwm2m_configure_2, P_INFO,"restore the targetP->location(%s)",(uint8_t*)targetP->location);
            targetP = targetP->next;
        }
        lwm2m_observe_load_data(contextP);
        //EC add end
        if (contextP->connectServerCallback) {
            contextP->serverList->sessionH = contextP->connectServerCallback(contextP->serverList->secObjInstID, contextP->userData);
        }
    }
    return COAP_NO_ERROR;
}

int lwm2m_add_object(lwm2m_context_t * contextP,
                     lwm2m_object_t * objectP)
{
    lwm2m_object_t * targetP;

    LOG_ARG("ID: %d", objectP->objID);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_add_object, P_INFO, 1, "add objid:%d, if stateready update reg",objectP->objID);
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, objectP->objID);
    if (targetP != NULL) return COAP_406_NOT_ACCEPTABLE;
    objectP->next = NULL;

    contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_ADD(contextP->objectList, objectP);

    if (contextP->state == STATE_READY)
    {
        return lwm2m_update_registration(contextP, 0, true);
    }

    return COAP_NO_ERROR;
}

//EC add begin
int lwm2m_add_object_ec(lwm2m_context_t * contextP,
                     lwm2m_object_t * objectP)
{
    lwm2m_object_t * targetP;

    LOG_ARG("ID: %d", objectP->objID);
    ECOMM_TRACE(UNILOG_LWM2M, lwm2m_add_object_ec, P_INFO, 1, "just add objid:%d, not update reg",objectP->objID);
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, objectP->objID);
    if (targetP != NULL) return COAP_406_NOT_ACCEPTABLE;
    objectP->next = NULL;

    contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_ADD(contextP->objectList, objectP);

    return COAP_NO_ERROR;
}
//EC add end

int lwm2m_remove_object(lwm2m_context_t * contextP,
                        uint16_t id)
{
    lwm2m_object_t * targetP;

    LOG_ARG("ID: %d", id);
    contextP->objectList = (lwm2m_object_t *)LWM2M_LIST_RM(contextP->objectList, id, &targetP);

    if (targetP == NULL) return COAP_404_NOT_FOUND;

    if (contextP->state == STATE_READY)
    {
        contextP->updateMode = 0;
        return lwm2m_update_registration(contextP, 0, true);
    }

    return 0;
}

#endif


int lwm2m_step(lwm2m_context_t * contextP,
               time_t * timeoutP)
{
    time_t tv_sec;
    int result;

    LOG_ARG("timeoutP: %" PRId64, *timeoutP);
    tv_sec = lwm2m_gettime();
    //if (tv_sec < 0) return COAP_500_INTERNAL_SERVER_ERROR;

#ifdef LWM2M_CLIENT_MODE
    LOG_ARG("State: %s", STR_STATE(contextP->state));
    char logbuf[32] = {0};
    snprintf(logbuf,32,"%s",STR_STATE(contextP->state));
    ECOMM_STRING(UNILOG_LWM2M, lwm2m_step_0, P_INFO, "lwm2m_step:state:%s", (uint8_t *)logbuf);
    // state can also be modified in bootstrap_handleCommand().

next_step:
    switch (contextP->state)
    {
    case STATE_INITIAL:
        if (0 != prv_refreshServerList(contextP)) return COAP_503_SERVICE_UNAVAILABLE;
        if (contextP->serverList != NULL)
        {
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_step_1, P_INFO, 0, "STATE_INITIAL -> STATE_REGISTER_REQUIRED");
            contextP->state = STATE_REGISTER_REQUIRED;
        }
        else
        {
            // Bootstrapping
            contextP->state = STATE_BOOTSTRAP_REQUIRED;
        }
        goto next_step;
        //break;

    case STATE_BOOTSTRAP_REQUIRED:
#ifdef LWM2M_BOOTSTRAP
        if (contextP->bootstrapServerList != NULL)
        {
            bootstrap_start(contextP);
            contextP->state = STATE_BOOTSTRAPPING;
            lwm2m_bootstrap_step(contextP, tv_sec, timeoutP);
        }
        else
#endif
        {
            return COAP_503_SERVICE_UNAVAILABLE;
        }
#ifdef LWM2M_BOOTSTRAP
        break;
#endif

#ifdef LWM2M_BOOTSTRAP
    case STATE_BOOTSTRAPPING:
        switch (lwm2m_bootstrap_getStatus(contextP))
        {
        case STATE_BS_FINISHED:
            contextP->state = STATE_INITIAL;
            goto next_step;
            //break;

        case STATE_BS_FAILED:
            return COAP_503_SERVICE_UNAVAILABLE;

        default:
            // keep on waiting
            lwm2m_bootstrap_step(contextP, tv_sec, timeoutP);
            break;
        }
        break;
#endif
    case STATE_REGISTER_REQUIRED:
        ECOMM_TRACE(UNILOG_LWM2M, lwm2m_step_2, P_INFO, 0, "STATE_REGISTER_REQUIRED -> registration_start");
        result = registration_start(contextP);
        if (COAP_NO_ERROR != result) return result;
        contextP->state = STATE_REGISTERING;
        break;

    case STATE_REGISTERING:
    {
        switch (registration_getStatus(contextP))
        {
        case STATE_REGISTERED:
            contextP->state = STATE_READY;
            break;

        case STATE_REG_FAILED:
            // TODO avoid infinite loop by checking the bootstrap info is different
            LOG("when registering reg failed re-initial");
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_step_3, P_INFO, 0, "when registering reg failed go initial");
            contextP->state = STATE_INITIAL;//STATE_BOOTSTRAP_REQUIRED; reg failed re-initial EC add
            goto next_step;
            //break;

        case STATE_REG_PENDING:
        default:
            // keep on waiting
            break;
        }
    }
    break;

    case STATE_READY:
        if (registration_getStatus(contextP) == STATE_REG_FAILED)
        {
            // TODO avoid infinite loop by checking the bootstrap info is different
            LOG("when ready reg failed re-initial");
            ECOMM_TRACE(UNILOG_LWM2M, lwm2m_step_4, P_INFO, 0, "reg failed not re-initial. let up level handle it");
            //contextP->state = STATE_INITIAL;//STATE_BOOTSTRAP_REQUIRED;reg failed re-initial  EC add
            //goto next_step;
            //break;
        }
        break;

    default:
        // do nothing
        break;
    }

    observe_step(contextP, tv_sec, timeoutP);
#endif

    registration_step(contextP, tv_sec, timeoutP);
    lwm2m_transaction_step(contextP, tv_sec, timeoutP);

    LOG_ARG("Final timeoutP: %" PRId64, *timeoutP);
#ifdef LWM2M_CLIENT_MODE
    LOG_ARG("Final state: %s", STR_STATE(contextP->state));
#endif
    return 0;
}
