/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
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
 *    Simon Bernard - initial API and implementation
 *    Christian Renz - Please refer to git log
 *
 *******************************************************************************/

#ifndef CT_DTLS_CONNECTION_H_
#define CT_DTLS_CONNECTION_H_

#include <stdio.h>
//#include <unistd.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <netdb.h>
#include "sockets.h"

#include <stdint.h>
#include <stdbool.h>

//#include "mbedtls_sample.h"
#include "../core/ct_liblwm2m.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/timing.h"
#include "mbedtls/debug.h"


#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT      5683
#define LWM2M_DTLS_PORT_STR     "6683"
#define LWM2M_DTLS_PORT          6683
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT      5685

// after 30sec of inactivity we rehandshake
#define DTLS_NAT_TIMEOUT 30


typedef struct _mbedtls_connection_t
{
    struct _mbedtls_connection_t *  next;
    int                     sock;
    struct sockaddr_in6     addr;
    size_t                  addrLen;
    int              dtlsSession;
    lwm2m_object_t * securityObj;
    int securityInstId;
    lwm2m_context_t * lwm2mH;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_timing_delay_context timer;
    time_t lastSend; // last time a data was sent to the server (used for NAT timeouts)
    bool hasHS;
} mbedtls_connection_t;


int ct_create_socket(const char * portStr, int ai_family);

mbedtls_connection_t * ct_connection_find(mbedtls_connection_t * connList, const struct sockaddr_storage * addr, size_t addrLen);
mbedtls_connection_t * connection_new_incoming(mbedtls_connection_t * connList, int sock, const struct sockaddr * addr, size_t addrLen);
mbedtls_connection_t * ct_connection_create(mbedtls_connection_t * connList, int sock, lwm2m_object_t * securityObj, int instanceId, lwm2m_context_t * lwm2mH, int addressFamily, void* userdata);

void ct_connection_free(mbedtls_connection_t * connList);

int ct_connection_handle_packet(uint8_t* buf, int length, mbedtls_connection_t *connP);

// rehandshake a connection, useful when your NAT timed out and your client has a new IP/PORT
int ct_connection_rehandshake(mbedtls_connection_t *connP, bool sendCloseNotify);
//void lwm2m_dtls_destroy(mbedtls_ssl_context *ssl);
#endif
