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
 *    Simon Bernard - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Pascal Rieux - Please refer to git log
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

/*
Contains code snippets which are:

 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

*/

/************************************************************************
 *  Function for communications transactions.
 *
 *  Basic specification: rfc7252
 *
 *  Transaction implements processing of piggybacked and separate response communication
 *  dialogs specified in section 2.2 of the above specification.
 *  The caller registers a callback function, which is called, when either the result is
 *  received or a timeout occurs.
 *
 *  Supported dialogs:
 *  Requests (GET - DELETE):
 *  - CON with mid, without token => regular finished with corresponding ACK.MID
 *  - CON with mid, with token => regular finished with corresponding ACK.MID and response containing
 *                  the token. Supports both versions, with piggybacked ACK and separate ACK/response.
 *                  Though the ACK.MID may be lost in the separate version, a matching response may
 *                  finish the transaction even without the ACK.MID.
 *  - NON without token => no transaction, no result expected!
 *  - NON with token => regular finished with response containing the token.
 *  Responses (COAP_201_CREATED - ?):
 *  - CON with mid => regular finished with corresponding ACK.MID
 */

#include <string.h>
#include <stdint.h>
#include "ct_internals.h"
#include "ctiot_lwm2m_sdk.h"
/*
 * Modulo mask (+1 and +0.5 for rounding) for a random number to get the tick number for the random
 * retransmission time between COAP_RESPONSE_TIMEOUT and COAP_RESPONSE_TIMEOUT*COAP_RESPONSE_RANDOM_FACTOR.
 */

static int prv_checkFinished(lwm2m_transaction_t * transacP,
                             coap_packet_t * receivedMessage)
{
    int len;
    const uint8_t* token;
    coap_packet_t * transactionMessage = transacP->message;

    if (COAP_DELETE < transactionMessage->code)
    {
        // response
        return transacP->ack_received ? 1 : 0;
    }
    if (!IS_OPTION(transactionMessage, COAP_OPTION_TOKEN))
    {
        // request without token
        return transacP->ack_received ? 1 : 0;
    }

    len = coap_get_header_token(receivedMessage, &token);
    if (transactionMessage->token_len == len)
    {
        if (memcmp(transactionMessage->token, token, len)==0) return 1;
    }

    return 0;
}

lwm2m_transaction_t * ctiot_transaction_new(void * sessionH,
                                      coap_method_t method,
                                      char * altPath,
                                      lwm2m_uri_t * uriP,
                                      uint16_t mID,
                                      uint8_t token_len,
                                      uint8_t* token)
{
    lwm2m_transaction_t * transacP;
    int result;

    LOG_ARG("method: %d, altPath: \"%s\", mID: %d, token_len: %d",
            method, altPath, mID, token_len);
    LOG_URI(uriP);
    //printf(uriP);
    //printf("\n");
    // no transactions without peer
    if (NULL == sessionH) return NULL;
    transacP = (lwm2m_transaction_t *)ct_lwm2m_malloc(sizeof(lwm2m_transaction_t));
    if (NULL == transacP) return NULL;
    memset(transacP, 0, sizeof(lwm2m_transaction_t));

    transacP->message = ct_lwm2m_malloc(sizeof(coap_packet_t));
    if (NULL == transacP->message) goto error;

    coap_init_message(transacP->message, COAP_TYPE_CON, method, mID);

    transacP->peerH = sessionH;

    transacP->mID = mID;

    if (altPath != NULL)
    {
        // TODO: Support multi-segment alternative path
        coap_set_header_uri_path_segment(transacP->message, altPath + 1);
    }
    if (NULL != uriP)
    {
        char stringID[LWM2M_STRING_ID_MAX_LEN];

        result = utils_intToText(uriP->objectId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
        if (result == 0) goto error;
        stringID[result] = 0;
        coap_set_header_uri_path_segment(transacP->message, stringID);

        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            result = utils_intToText(uriP->instanceId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
            if (result == 0) goto error;
            stringID[result] = 0;
            coap_set_header_uri_path_segment(transacP->message, stringID);
        }
        else
        {
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                coap_set_header_uri_path_segment(transacP->message, NULL);
            }
        }
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            result = utils_intToText(uriP->resourceId, (uint8_t*)stringID, LWM2M_STRING_ID_MAX_LEN);
            if (result == 0) goto error;
            stringID[result] = 0;
            coap_set_header_uri_path_segment(transacP->message, stringID);
        }
    }
    if (0 < token_len)
    {
        if (NULL != token)
        {
            coap_set_header_token(transacP->message, token, token_len);
        }
        else {
            // generate a token
            uint8_t temp_token[COAP_TOKEN_LEN];
            time_t tv_sec = ct_lwm2m_gettime();

            // initialize first 6 bytes, leave the last 2 random
            temp_token[0] = mID;
            temp_token[1] = mID >> 8;
            temp_token[2] = tv_sec;
            temp_token[3] = tv_sec >> 8;
            temp_token[4] = tv_sec >> 16;
            temp_token[5] = tv_sec >> 24;
            // use just the provided amount of bytes
            coap_set_header_token(transacP->message, temp_token, token_len);
        }
    }

    LOG("Exiting on success");
    return transacP;

error:
    LOG("Exiting on failure");
    ct_lwm2m_free(transacP);
    return NULL;
}

void ctiot_transaction_free(lwm2m_transaction_t * transacP)
{
    if (transacP->message)
    {
        coap_free_header(transacP->message);
        coap_free_payload(transacP->message);
        ct_lwm2m_free(transacP->message);
    }
    if (transacP->buffer_len > 0 && transacP->buffer) ct_lwm2m_free(transacP->buffer);
    ct_lwm2m_free(transacP);
}

void ctiot_transaction_remove(lwm2m_context_t * contextP,
                        lwm2m_transaction_t * transacP)
{
    lwm2m_transaction_t * tempT;
    tempT = (lwm2m_transaction_t *) LWM2M_LIST_FIND(contextP->transactionList, transacP->mID);
    if(tempT != NULL){
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_remove_1, P_INFO, 0, "found the transac remove it");
        contextP->transactionList = (lwm2m_transaction_t *) LWM2M_LIST_RM(contextP->transactionList, transacP->mID, NULL);
        ctiot_transaction_free(transacP);
    }else{
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_remove_2, P_INFO, 0, "didn't find the transac it alrady be remove!");
    }
}

bool ctiot_transaction_handleResponse(lwm2m_context_t * contextP,
                                 void * fromSessionH,
                                 coap_packet_t * message,
                                 coap_packet_t * response)
{
    bool found = false;
    bool reset = false;
    lwm2m_transaction_t * transacP;

    LOG("Entering");
    transacP = contextP->transactionList;

    while (NULL != transacP)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_handleResponse_0, P_INFO, 1, "this transacP->mID=%d",transacP->mID);
        osMutexAcquire(contextP->transactMutex, osWaitForever);
        if (ct_lwm2m_session_is_equal(fromSessionH, transacP->peerH, contextP->userData) == true)
        {
            if (!transacP->ack_received)
            {

                if ((COAP_TYPE_ACK == message->type) || (COAP_TYPE_RST == message->type))
                {
                    if (transacP->mID == message->mid)
                    {
                        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_handleResponse_0_1, P_INFO, 0, "mid right find it!");
                        found = true;
                        transacP->ack_received = true;
                        reset = COAP_TYPE_RST == message->type;
                    }
                }
            }

            if (reset || prv_checkFinished(transacP, message))
            {
                // HACK: If a message is sent from the monitor callback,
                // it will arrive before the registration ACK.
                // So we resend transaction that were denied for authentication reason.
                if (!reset)
                {
                    if (COAP_TYPE_CON == message->type && NULL != response)
                    {
                        coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
                        ct_message_send(contextP, response, fromSessionH);
                    }
                
                    //if ((COAP_401_UNAUTHORIZED == message->code) && (COAP_MAX_RETRANSMIT > transacP->retrans_counter))
                    //{
                        //transacP->ack_received = false;
                        //transacP->retrans_time += ctiot_getCoapAckTimeout();
                        //return true;
                    //}
                }       
                if (transacP->callback != NULL)
                {
                    transacP->callback(transacP, message);
                }
                else
                {
                    lwm2m_printf("transac->callback is null\n");
                }
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_handleResponse_1, P_INFO, 1, "recv ACK just call callback let ctiot_transaction_step remove the transacP mID(%d)",transacP->mID);
                //ctiot_transaction_remove(contextP, transacP);
                osMutexRelease(contextP->transactMutex);
                return true;
            }
            // if we found our guy, exit
            if (found)
            {
                transacP->retrans_time = ct_lwm2m_gettime();
                if (transacP->response_timeout)
                {
                    transacP->retrans_time += transacP->response_timeout;
                }
                else
                {
                    transacP->retrans_time += ctiot_getCoapAckTimeout() * transacP->retrans_counter;
                }
                osMutexRelease(contextP->transactMutex);
                return true;
            }
        }
        osMutexRelease(contextP->transactMutex);

        transacP = transacP->next;
    }
    return false;
}

int ctiot_transaction_send(lwm2m_context_t * contextP,
                     lwm2m_transaction_t * transacP)
{
    bool maxRetriesReached = false;
    LOG("Entering");
    if (transacP->buffer == NULL)
    {
        transacP->buffer_len = coap_serialize_get_size(transacP->message);
        if (transacP->buffer_len == 0)
        {
           ctiot_transaction_remove(contextP, transacP);
           return COAP_500_INTERNAL_SERVER_ERROR;
        }

        transacP->buffer = (uint8_t*)ct_lwm2m_malloc(transacP->buffer_len);
        if (transacP->buffer == NULL)
        {
           ctiot_transaction_remove(contextP, transacP);
           return COAP_500_INTERNAL_SERVER_ERROR;
        }

        transacP->buffer_len = coap_serialize_message(transacP->message, transacP->buffer);
        if (transacP->buffer_len == 0)
        {
            ct_lwm2m_free(transacP->buffer);
            transacP->buffer = NULL;
            ctiot_transaction_remove(contextP, transacP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

    if (!transacP->ack_received)
    {
        long unsigned timeout;

        if (0 == transacP->retrans_counter)
        {
            transacP->retrans_time = ct_lwm2m_gettime() + ctiot_getCoapAckTimeout() + 10;//+1 to wait more longer 
            transacP->retrans_counter = 1;
            timeout = 0;
        }
        else
        {
            timeout = ctiot_getCoapAckTimeout() << (transacP->retrans_counter - 1) + 1;
        }

        if (COAP_MAX_RETRANSMIT + 1 >= transacP->retrans_counter)
        {
            int res;
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_send_1, P_INFO, 2, "%d times send the package(%d)",transacP->retrans_counter, transacP->mID);
            res = ct_lwm2m_buffer_send(transacP->peerH, transacP->buffer, transacP->buffer_len, contextP->userData,((coap_packet_t *)(transacP->message))->sendOption);
#ifdef WITH_MBEDTLS
            if(res == MBEDTLS_FAILED)
            {
                maxRetriesReached = true;
                ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_send_2, P_INFO, 0, "this send handshake failure, send failure abandon it");
            }
#endif
            transacP->retrans_time += timeout;
            transacP->retrans_counter += 1;
        }
        else
        {
            maxRetriesReached = true;
            #if 0
            switch(transacP->trans_business)
            {
                case 1:
                    setCoapGlobalStatus(UB_STATUS,CTIOT_COAP_UB_TIMEOUT);
                    break;
                case 2:
                    setCoapGlobalStatus(SEND_STATUS,CTIOT_COAP_SEND_TIMEOUT);
                    msg_state(transacP->mID,MSG_SEND_TIMEOUT);
                    break;
                case 3:
                    setCoapGlobalStatus(RECVRSP_STATUS,CTIOT_COAP_RECVRSP_TIMEOUT);
                    msg_state(transacP->mID,MSG_SEND_TIMEOUT);
                    break;
                case 4:
                    setCoapGlobalStatus(DEREG_STATUS,CTIOT_COAP_RECVRSP_TIMEOUT);
                    break;
                default:
                    log_print_plain_text("transacP->trans_business is %d,timeout\n",transacP->trans_business);
                    break;
            }
            #endif
        }
    }

    if ( maxRetriesReached )
    {
        if (transacP->callback)
        {
            transacP->callback(transacP, NULL);
        }
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_send_3, P_INFO, 0, "ctiot_transaction_remove because maxRetriesReached");
        ctiot_transaction_remove(contextP, transacP);
        return -1;
    }
    return 0;
}

void ctiot_transaction_step(lwm2m_context_t * contextP,
                      time_t currentTime,
                      time_t * timeoutP)
{
    lwm2m_transaction_t * transacP;

    LOG("Entering");
    transacP = contextP->transactionList;
    while (transacP != NULL)
    {
        // ctiot_transaction_send() may remove transaction from the linked list
        osMutexAcquire(contextP->transactMutex, osWaitForever);
        lwm2m_transaction_t * nextP = transacP->next;
        int removed = 0;
        if ( transacP->ack_received )
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_transaction_step_0, P_INFO, 1, "remove this transaction mID(%d) because ack_received,", transacP->mID);
            ctiot_transaction_remove(contextP, transacP);
        }
        else
        {
            if (transacP->retrans_time <= currentTime)
            {
                removed = ctiot_transaction_send(contextP, transacP);
            }
    
            if (0 == removed)
            {
                time_t interval;
    
                if (transacP->retrans_time > currentTime)
                {
                    interval = transacP->retrans_time - currentTime;
                }
                else
                {
                    interval = 1;
                }
    
                if (*timeoutP > interval)
                {
                    *timeoutP = interval;
                }
            }
            else
            {
                *timeoutP = 1;
            }
        }
        transacP = nextP;
        osMutexRelease(contextP->transactMutex);
    }
}

//ec add begin
void cfg_sendOption(void * packet, uint8_t mode)
{
    coap_packet_t* messageP = (coap_packet_t*)packet;
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
      
