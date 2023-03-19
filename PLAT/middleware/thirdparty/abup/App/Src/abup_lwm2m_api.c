/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH, Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/
/*
 *  event_handler.c
 *
 *  Callback for value changes.
 *
 *  Created on: 21.01.2015
 *  Author: Achim Kraus
 *  Copyright (c) 2014 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */
#include "liblwm2m.h"
#include "abup_lwm2mclient.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "abup_system_api.h"
#include "abup_typedef.h"
#include "abup_lwm2m_api.h"
#include "abup_task.h"
#include "abup_firmware.h"

extern abup_firmware_data_t* abup_contextp;
extern void abup_creat_main_download_timer(void);
extern void abup_notify_status(adups_uint32 a_status, adups_uint32 a_percentage);
extern void opencpu_fota_event_cb(adups_uint8 event,adups_int8 state);
extern uint8_t adups_process_state;

#ifdef ABUP_WITH_TINYDTLS
void *abup_lwm2m_connect_server(uint16_t secObjInstID, void *userData)
{
	abup_client_data_t *dataP;
	lwm2m_list_t *instance;
	dtls_connection_t *newConnP = NULL;
	dataP = (abup_client_data_t *)userData;
	lwm2m_object_t   *securityObj = dataP->securityObjP;
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_10, P_INFO, 0, "abup_lwm2m_connect_server-->");
	instance = LWM2M_LIST_FIND(dataP->securityObjP->instanceList, secObjInstID);
	if (instance == NULL)
		return NULL;

	abup_main_download_timer_stop();
	newConnP = connection_create(dataP->connList, dataP->sock, securityObj, instance->id, dataP->lwm2mH, dataP->addressFamily);
	abup_creat_main_download_timer();
	if (newConnP == NULL)
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_9, P_INFO, 0, "Connection creation failed.\n");
		if(adups_process_state==0){
			//opencpu_fota_event_cb(0,5);
		}
		abup_main_download_timer_callback(abup_contextp->xmaindownloadTimer);
		return NULL;
	}
	else{
		abup_notify_status(ABUP_STATUS_CHECK_VERSION, 0);
	}

	dataP->connList = newConnP;
	return (void *)newConnP;
}
#else

char *abup_get_server_uri(lwm2m_object_t *objectP, uint16_t secObjInstID)
{
	abup_security_instance_t *targetP = (abup_security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_8, P_INFO, 0, "abup_get_server_uri-->");
	if (NULL != targetP)
	{
		return lwm2m_strdup(targetP->uri);
	}

	return NULL;
}

void *abup_lwm2m_connect_server(uint16_t secObjInstID, void *userData)
{
	abup_client_data_t *dataP;
	char *uri;
	char *host;
	char *port;
	connection_t *newConnP = NULL;
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_7, P_INFO, 0, "abup_lwm2m_connect_server-->");
	dataP = (abup_client_data_t *)userData;

	uri = abup_get_server_uri(dataP->securityObjP, secObjInstID);

	if (uri == NULL)
		return NULL;

	// parse uri in the form "coaps://[host]:[port]"
	if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
	{
		host = uri + strlen("coaps://");
	}
	else if (0 == strncmp(uri, "coap://",  strlen("coap://")))
	{
		host = uri + strlen("coap://");
	}
	else
	{
		goto exit;
	}
	port = strrchr(host, ':');
	if (port == NULL)
		goto exit;
	// remove brackets
	if (host[0] == '[')
	{
		host++;
		if (*(port - 1) == ']')
		{
			*(port - 1) = 0;
		}
		else
			goto exit;
	}
	// split strings
	*port = 0;
	port++;
	
	ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_6, P_INFO, "abup_lwm2m_connect_server-->host: %s",(uint8_t *)host);
	ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_11, P_INFO, "abup_lwm2m_connect_server-->port: %s",(uint8_t *)port);
	
	abup_main_download_timer_stop();
	newConnP = connection_create(dataP->connList,dataP->sock,host,port,dataP->addressFamily);
	
	abup_creat_main_download_timer();
	if (newConnP == NULL)
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_5, P_INFO, 0, "Connection creation failed.\r\n");
		if(abup_get_process_state()==0){
			//opencpu_fota_event_cb(0,5);
		}
		abup_main_download_timer_callback(abup_contextp->xmaindownloadTimer);
	}
	else
	{
		dataP->connList = newConnP;
		abup_notify_status(ABUP_STATUS_CHECK_VERSION, 0);
	}

exit:
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_4, P_INFO, 0, "258\r\n ");
	lwm2m_free(uri);
	return (void *)newConnP;
}
#endif

void abup_lwm2m_close_connection(void *sessionH, void *userData)
{
	abup_client_data_t *app_data;
#ifdef ABUP_WITH_TINYDTLS
	dtls_connection_t *targetP;
#else
	connection_t *targetP;
#endif

	app_data = (abup_client_data_t *)userData;
#ifdef ABUP_WITH_TINYDTLS
	targetP = (dtls_connection_t *)sessionH;
#else
	targetP = (connection_t *)sessionH;
#endif
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_3, P_INFO, 0, "abup_lwm2m_close_connection-->");

	if (targetP == app_data->connList)
	{
		app_data->connList = targetP->next;
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_2, P_INFO, 0, "284\r\n ");
		lwm2m_free(targetP);
	}
	else
	{
	#ifdef ABUP_WITH_TINYDTLS
		dtls_connection_t *parentP;
	#else
		connection_t *parentP;
	#endif

		parentP = app_data->connList;
		while (parentP != NULL && parentP->next != targetP)
		{
			parentP = parentP->next;
		}
		if (parentP != NULL)
		{
			parentP->next = targetP->next;
			ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_LWM2M_API_1, P_INFO, 0, "303\r\n ");
			lwm2m_free(targetP);
		}
	}
}

