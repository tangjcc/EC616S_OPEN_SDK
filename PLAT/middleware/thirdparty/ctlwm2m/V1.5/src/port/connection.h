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
 *    
 *******************************************************************************/

#ifndef CT_CONNECTION_H_
#define CT_CONNECTION_H_


#if defined(PLATFORM_LINUX) 
#include <unistd.h>//read,write,usleep
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>  //posix api (gethostbyaddr,gethostbyname)


#elif defined(PLATFORM_MCU_LITEOS) 
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"

#elif defined(PLATFORM_MCU_ECOM)
#include "netdb.h"

#endif


//#include <sys/stat.h>


#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT      5683
#define LWM2M_DTLS_PORT_STR     "5684"
#define LWM2M_DTLS_PORT          5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT      5685

typedef struct _connection_t
{
    struct _connection_t *  next;
    int                     sock;
    struct sockaddr_in     addr;
    size_t                  addrLen;
} connection_t;

int ct_create_socket(const char * portStr, int ai_family);

connection_t * ct_connection_find(connection_t * connList, struct sockaddr_storage * addr, size_t addrLen);
connection_t * ct_connection_new_incoming(connection_t * connList, int sock, struct sockaddr * addr, size_t addrLen);
connection_t * ct_connection_create(connection_t * connList, int sock, char * host, char * port, int addressFamily);

void ct_connection_free(connection_t * connList);

#endif
