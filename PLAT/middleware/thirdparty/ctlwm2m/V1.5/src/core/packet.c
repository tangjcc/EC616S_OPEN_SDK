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
 *    Fabien Fleutot - Please refer to git log
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


#include "ct_internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "objects/lwm2mclient.h"

#include "ct_liblwm2m.h"

#include "ctiot_lwm2m_sdk.h"


static void ct_handle_reset(lwm2m_context_t * contextP,
                         void * fromSessionH,
                         coap_packet_t * message)
{
#ifdef LWM2M_CLIENT_MODE
    LOG("Entering");
    ct_observe_cancel(contextP, message->mid, fromSessionH);
#endif
}

static uint8_t ct_handle_request(lwm2m_context_t * contextP,
                              void * fromSessionH,
                              coap_packet_t * message,
                              coap_packet_t * response)
{
    lwm2m_uri_t * uriP;
    uint8_t result = COAP_IGNORE;

    LOG("Entering");

#ifdef LWM2M_CLIENT_MODE
    uriP = ctlwm2m_uri_decode(contextP->altPath, message->uri_path);
#else
    uriP = ctlwm2m_uri_decode(NULL, message->uri_path);
#endif
    if (uriP == NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_request_0_0, P_INFO, 0, "uriP == NULL");
    }
    else                                                                            
    {                                                                               
        //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_request_0_1, P_INFO, 2, "uriP->objectId=%d,uriP->flag=0x%x",uriP->objectId,uriP->flag);
        //if (LWM2M_URI_IS_SET_INSTANCE(uriP)) ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_request_0_2, P_INFO, 1, "uriP->instanceId=%d",uriP->instanceId);
        //if (LWM2M_URI_IS_SET_RESOURCE(uriP)) ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_request_0_3, P_INFO, 1, "uriP->resourceId=%d",uriP->resourceId);
    }                                                                               

    if (uriP == NULL) return COAP_400_BAD_REQUEST;

    switch(uriP->flag & LWM2M_URI_MASK_TYPE)
    {
#ifdef LWM2M_CLIENT_MODE
    case LWM2M_URI_FLAG_DM:
    {
        lwm2m_server_t * serverP;

        serverP = utils_findServer(contextP, fromSessionH);
        if (serverP != NULL)
        {
            //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_request_1, P_INFO, 0, "LWM2M_URI_FLAG_DM");
            result = ct_dm_handleRequest(contextP, uriP, serverP, message, response);
        }
#ifdef LWM2M_BOOTSTRAP
        else
        {
            serverP = utils_findBootstrapServer(contextP, fromSessionH);
            if (serverP != NULL)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_request_2, P_INFO, 0, "call ct_bootstrap_handleCommand");
                result = ct_bootstrap_handleCommand(contextP, uriP, serverP, message, response);
            }
        }
#endif
    }
    break;

#ifdef LWM2M_BOOTSTRAP
    case LWM2M_URI_FLAG_DELETE_ALL:
        if (COAP_DELETE != message->code)
        {
            result = COAP_400_BAD_REQUEST;
        }
        else
        {
            //result = ct_bootstrap_handleDeleteAll(contextP, fromSessionH);
            result = COAP_202_DELETED;// not delete objects added in ctiot_funcv1_session_init
        }
        break;

    case LWM2M_URI_FLAG_BOOTSTRAP:
        if (message->code == COAP_POST)
        {
            result = ct_bootstrap_handleFinish(contextP, fromSessionH);
        }
        break;
#endif
#endif

#ifdef LWM2M_BOOTSTRAP_SERVER_MODE
    case LWM2M_URI_FLAG_BOOTSTRAP:
        result = ct_bootstrap_handleRequest(contextP, uriP, fromSessionH, message, response);
        break;
#endif
    default:
        result = COAP_IGNORE;
        break;
    }

    coap_set_status_code(response, result);

    if (COAP_IGNORE < result && result < COAP_400_BAD_REQUEST)
    {
        result = COAP_NO_ERROR;
    }

    ct_lwm2m_free(uriP);
    return result;
}

/* This function is an adaptation of function coap_receive() from Erbium's er-coap-13-engine.c.
 * Erbium is Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 */
void ct_handle_packet(lwm2m_context_t * contextP,
                         uint8_t * buffer,
                         int length,
                         void * fromSessionH)
{
    uint8_t coap_error_code = COAP_NO_ERROR;
    static coap_packet_t message[1];
    static coap_packet_t response[1];

    //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_0_1, P_INFO, 0, "Entering");
    //printf("Entering ct_handle_packet length=%d\n",length);
    if(buffer==NULL)
    {
        //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_0_2, P_INFO, 0, "NO Content");
        return;
    }
    if(contextP == NULL)
    {
        //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_0_3, P_INFO, 0, "contextP == NULL");
        return;
    }
    if(fromSessionH == NULL)
    {
        //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_0_4, P_INFO, 0, "fromSessionH == NULL");
        return;
    }
    coap_error_code = coap_parse_message(message, buffer, (uint16_t)length);

    if (coap_error_code == COAP_NO_ERROR)
    {
        lwm2m_printf("Parsed: ver %u, type %u, tkl %u, code %u.%.2u, mid %u, Content type: %d",
                message->version, message->type, message->token_len, message->code >> 5, message->code & 0x1F, message->mid, message->content_type);
        lwm2m_printf("Payload: %d %s", message->payload_len, message->payload);
        ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_1, P_INFO, 4, "message->code=%d.0%d, message->type=%d, message->msgid=%d", message->code >> 5, message->code & 0x1F, message->type, message->mid);

        if (message->code >= COAP_GET && message->code <= COAP_DELETE)
        {
            if(contextP->lastRecvMID != message->mid){
                contextP->lastRecvMID = message->mid;
            }else{
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_1_1, P_INFO, 1, "last recvived msgid=%d. get same msgid packet abandon it",contextP->lastRecvMID);
                return;
            }
            uint32_t block_num = 0;
            uint16_t block_size = REST_MAX_CHUNK_SIZE;
            uint32_t block_offset = 0;
            int64_t new_offset = 0;

            /* prepare response */
            if (message->type == COAP_TYPE_CON)
            {
                /* Reliable CON requests are answered with an ACK. */
                coap_init_message(response, COAP_TYPE_ACK, COAP_205_CONTENT, message->mid);
            }
            else
            {
                /* Unreliable NON requests are answered with a NON as well. */
                coap_init_message(response, COAP_TYPE_NON, COAP_205_CONTENT, contextP->nextMID++);
            }

            //write response with ack,add by xwb
            /*
            if(message->code == COAP_PUT)
            {
                coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
                coap_error_code = ct_message_send(contextP, response, fromSessionH);
                coap_init_message(response, COAP_TYPE_CON, 0, message->mid);
            }
            */
            /* mirror token */
            if (message->token_len)
            {
                coap_set_header_token(response, message->token, message->token_len);
            }

            /* get offset for blockwise transfers */
            if (coap_get_header_block2(message, &block_num, NULL, &block_size, &block_offset))
            {
                LOG_ARG("Blockwise: block request %u (%u/%u) @ %u bytes", block_num, block_size, REST_MAX_CHUNK_SIZE, block_offset);
                block_size = MIN(block_size, REST_MAX_CHUNK_SIZE);
                new_offset = block_offset;
            }

            /* handle block1 option */
            if (IS_OPTION(message, COAP_OPTION_BLOCK1))
            {
#ifdef LWM2M_CLIENT_MODE
                // get server
                lwm2m_server_t * serverP;
                serverP = utils_findServer(contextP, fromSessionH);
#ifdef LWM2M_BOOTSTRAP
                if (serverP == NULL)
                {
                    serverP = utils_findBootstrapServer(contextP, fromSessionH);
                }
#endif
                if (serverP == NULL)
                {
                    coap_error_code = COAP_500_INTERNAL_SERVER_ERROR;
                }
                else
                {
                    uint32_t block1_num;
                    uint8_t  block1_more;
                    uint16_t block1_size;
                    uint8_t * complete_buffer = NULL;
                    size_t complete_buffer_size;

                    // parse block1 header
                    coap_get_header_block1(message, &block1_num, &block1_more, &block1_size, NULL);
                    LOG_ARG("Blockwise: block1 request NUM %u (SZX %u/ SZX Max%u) MORE %u", block1_num, block1_size, REST_MAX_CHUNK_SIZE, block1_more);

                    // handle block 1
                    coap_error_code = coap_block1_handler(&serverP->block1Data, message->mid, message->payload, message->payload_len, block1_size, block1_num, block1_more, &complete_buffer, &complete_buffer_size);

                    // if payload is complete, replace it in the coap message.
                    if (coap_error_code == COAP_NO_ERROR)
                    {
                        message->payload = complete_buffer;
                        message->payload_len = complete_buffer_size;
                    }
                    else if (coap_error_code == COAP_231_CONTINUE)
                    {
                        block1_size = MIN(block1_size, REST_MAX_CHUNK_SIZE);
                        coap_set_header_block1(response,block1_num, block1_more,block1_size);
                    }
                }
#else
                coap_error_code = COAP_501_NOT_IMPLEMENTED;
#endif
            }
            if (coap_error_code == COAP_NO_ERROR)
            {
                coap_error_code = ct_handle_request(contextP, fromSessionH, message, response);
                //ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_2, P_INFO, 1, "ct_handle_request return 0x%x",coap_error_code);
                //log_print_plain_text("coap_error_code is %d\n",coap_error_code);
            }
            if (coap_error_code==COAP_NO_ERROR)
            {
                /* Save original payload pointer for later freeing. Payload in response may be updated. */
                uint8_t *payload = response->payload;
                if ( IS_OPTION(message, COAP_OPTION_BLOCK2) )
                {
                    /* unchanged new_offset indicates that resource is unaware of blockwise transfer */
                    if (new_offset==block_offset)
                    {
                        LOG_ARG("Blockwise: unaware resource with payload length %u/%u", response->payload_len, block_size);
                        if (block_offset >= response->payload_len)
                        {
                            LOG("handle_incoming_data(): block_offset >= response->payload_len");


                            response->code = COAP_402_BAD_OPTION;
                            coap_set_payload(response, "BlockOutOfScope", 15); /* a const char str[] and sizeof(str) produces larger code size */
                        }
                        else
                        {
                            coap_set_header_block2(response, block_num, response->payload_len - block_offset > block_size, block_size);
                            coap_set_payload(response, response->payload+block_offset, MIN(response->payload_len - block_offset, block_size));
                        } /* if (valid offset) */
                    }
                    else
                    {
                        /* resource provides chunk-wise data */
                        LOG_ARG("Blockwise: blockwise resource, new offset %d", (int) new_offset);
                        coap_set_header_block2(response, block_num, new_offset!=-1 || response->payload_len > block_size, block_size);
                        if (response->payload_len > block_size) coap_set_payload(response, response->payload, block_size);
                    } /* if (resource aware of blockwise) */
                }
                else if (new_offset!=0)
                {
                    LOG_ARG("Blockwise: no block option for blockwise resource, using block size %u", REST_MAX_CHUNK_SIZE);

                    coap_set_header_block2(response, 0, new_offset!=-1, REST_MAX_CHUNK_SIZE);
                    coap_set_payload(response, response->payload, MIN(response->payload_len, REST_MAX_CHUNK_SIZE));
                } /* if (blockwise request) */
                
                ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_3, P_INFO, 0, "send the reponse to peer");
                coap_error_code = ct_message_send(contextP, response, fromSessionH);
                ct_lwm2m_free(payload);
                response->payload = NULL;
                response->payload_len = 0;
            }
            else if (coap_error_code != COAP_IGNORE)
            {
                if (1 == coap_set_status_code(response, coap_error_code))
                {
                    coap_error_code = ct_message_send(contextP, response, fromSessionH);
                }
            }
        }
        else
        {
            /* Responses */
           
            ECOMM_TRACE(UNILOG_CTLWM2M, ct_handle_packet_4, P_INFO, 1, "handle response packet:message->type:%d", message->type);
            //lwm2m_uri_t * uriP;
            //uriP = ctlwm2m_uri_decode(contextP->altPath, message->uri_path);
            switch (message->type)
            {
            case COAP_TYPE_NON:
            case COAP_TYPE_CON:
                {
                    if(message->code != 66)//DELETED 2_02 no need handle it 
                    {
                        bool done = ctiot_transaction_handleResponse(contextP, fromSessionH, message, response);
    
                        if (!done && message->type == COAP_TYPE_CON )
                        {
                            coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
                            coap_error_code = ct_message_send(contextP, response, fromSessionH);
                        }
                    }
                }
                break;

            case COAP_TYPE_RST:
                /* Cancel possible subscriptions. */
                ctiot_funcv1_notify_event(MODULE_LWM2M, RECV_RST_CMD, NULL, 0);
                ct_handle_reset(contextP, fromSessionH, message);
                if(ctiot_transaction_handleResponse(contextP, fromSessionH, message, NULL) == false)
                {
                    ctiot_funcv1_status_t pTmpStatus[1] = {0};
                    pTmpStatus->baseInfo = ct_lwm2m_strdup(SS5);
                    pTmpStatus->extraInfo = &message->mid;
                    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, pTmpStatus, 0);
                    ct_lwm2m_free(pTmpStatus->baseInfo);
                }
                break;

            case COAP_TYPE_ACK:
                //log_print_plain_text("handle packet:COAP_TYPE_ACK\n");
                ctiot_transaction_handleResponse(contextP, fromSessionH, message, NULL);
                break;

            default:
                break;
            }
        } /* Request or Response */
        coap_free_header(message);
    } /* if (parsed correctly) */
    else
    {
        LOG_ARG("Message parsing failed %u.%2u", coap_error_code >> 5, coap_error_code & 0x1F);
    }
   
    if (coap_error_code != COAP_NO_ERROR && coap_error_code != COAP_IGNORE)
    {
        LOG_ARG("ERROR %u: %s", coap_error_code, coap_error_message);

        /* Set to sendable error code. */
        if (coap_error_code >= 192)
        {
            coap_error_code = COAP_500_INTERNAL_SERVER_ERROR;
        }
        /* Reuse input buffer for error message. */
        coap_init_message(message, COAP_TYPE_ACK, coap_error_code, message->mid);
        coap_set_payload(message, coap_error_message, strlen(coap_error_message));
        ct_message_send(contextP, message, fromSessionH);
    }
}


uint8_t ct_message_send(lwm2m_context_t * contextP,
                     coap_packet_t * message,
                     void * sessionH)
{
    uint8_t result = COAP_500_INTERNAL_SERVER_ERROR;
    uint8_t * pktBuffer;
    size_t pktBufferLen = 0;
    size_t allocLen;

    LOG("Entering");
    allocLen = coap_serialize_get_size(message);
    if (allocLen == 0) return COAP_500_INTERNAL_SERVER_ERROR;

    pktBuffer = (uint8_t *)ct_lwm2m_malloc(allocLen);
    if (pktBuffer != NULL)
    {
        pktBufferLen = coap_serialize_message(message, pktBuffer);
        if (0 != pktBufferLen)
        {
            //ECOMM_TRACE(UNILOG_CTLWM2M, ct_message_send_1, P_INFO, 0, "call ct_lwm2m_buffer_send");
            result = ct_lwm2m_buffer_send(sessionH, pktBuffer, pktBufferLen, contextP->userData,message->sendOption);
        }
        ct_lwm2m_free(pktBuffer);
    }

    return result;
}

