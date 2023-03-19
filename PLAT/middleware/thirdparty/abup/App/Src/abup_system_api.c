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
#ifdef ABUP_WITH_TINYDTLS
#include "shared/dtlsconnection.h"
#else
#include "shared/connection.h"
#endif
#include "abup_system_api.h"
#include "abup_typedef.h"
#include "abup_config.h"
#include "abup_stdlib.h"

#ifdef LWM2M_EMBEDDED_MODE
static void abup_prv_value_change(void* context, const char* uriPath, const char * value, size_t valueLength)
{
	lwm2m_uri_t uri;
	if (lwm2m_stringToUri(uriPath, strlen(uriPath), &uri))
	{
		abup_handle_value_changed(context, &uri, value, valueLength);
	}
}

void abup_init_value_change(lwm2m_context_t * lwm2m)
{
	system_setValueChangedHandler(lwm2m, abup_prv_value_change);
}
#else
void abup_init_value_change(lwm2m_context_t * lwm2m)
{
}
#endif

void abup_system_reboot(void)
{	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_REBOOT_API_1, P_INFO, 0, "abup_system_reboot-->");
	__disable_irq();
    
    #if defined CHIP_EC616S
    extern void PmuAonPorEn(void);
    PmuAonPorEn();
    #endif
    
	NVIC_SystemReset();
}


adups_uint8* abup_get_ip_addr(void * addr, adups_uint8 len)
{
	static adups_uint8 ipstr[20] = {0};
	adups_uint8* ptr = NULL;
	adups_memset((char *)ipstr, 0, sizeof(ipstr));
	if(addr)
	{
		ptr = (adups_uint8*)addr;
		sprintf((char*)ipstr, "%d.%d.%d.%d", *(ptr+4),*(ptr+5),*(ptr+6),*(ptr+7));
	}
	else
	{
		sprintf((char*)ipstr, "%d.%d.%d.%d", 0,0,0,0);
	}
	return ipstr;
}

