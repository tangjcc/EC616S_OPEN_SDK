 /*******************************************************************************/
/*
 * abup_lwm2mclient.h
 *
 *  General functions of lwm2m test client.
 *
 *  Created on: 22.08.2019
 *  Author: Achim Kraus
 *  Copyright (c) 2019 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */


#ifndef LWM2MCLIENT_H_
#define LWM2MCLIENT_H_


#include "liblwm2m.h"
#include "abup_system_api.h"
#ifdef ABUP_WITH_TINYDTLS
#include "shared/dtlsconnection.h"
#else
#include "shared/connection.h"
#endif

typedef struct
{
    lwm2m_object_t * securityObjP;
    lwm2m_object_t * serverObject;
    int sock;
#ifdef ABUP_WITH_TINYDTLS
    dtls_connection_t * connList;
    lwm2m_context_t * lwm2mH;
#else
    connection_t * connList;
#endif
    int addressFamily;
} abup_client_data_t;


extern int abup_greboot;
void abup_handle_value_changed(lwm2m_context_t* lwm2mH, lwm2m_uri_t* uri, const char * value, size_t valueLength);

#endif /* LWM2MCLIENT_H_ */

