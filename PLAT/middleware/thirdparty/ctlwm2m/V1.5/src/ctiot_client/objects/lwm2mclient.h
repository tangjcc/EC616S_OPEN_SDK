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
 * lwm2mclient.h
 *
 *  General functions of lwm2m test client.
 *
 *  Created on: 22.01.2015
 *  Author: Achim Kraus
 *  Copyright (c) 2015 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */

#ifndef LWM2MCLIENT_H_
#define LWM2MCLIENT_H_

#include "ct_liblwm2m.h"
#include "ctiot_lwm2m_sdk.h"

/*
 * object_device.c
 */
lwm2m_object_t * ct_get_object_device(void);
void ct_free_object_device(lwm2m_object_t * objectP);
uint8_t ct_device_change(lwm2m_data_t * dataArray, lwm2m_object_t * objectP);
void ct_display_device_object(lwm2m_object_t * objectP);
/*
 * object_firmware.c
 */

#ifdef WITH_FOTA
lwm2m_object_t * ct_get_object_firmware(void);
void ct_free_object_firmware(lwm2m_object_t * objectP);
void ct_display_firmware_object(lwm2m_object_t * objectP);
#endif
/*
 * object_location.c
 */
lwm2m_object_t * get_object_location(void);
void free_object_location(lwm2m_object_t * object);
void display_location_object(lwm2m_object_t * objectP);

/*
 * object_server.c
 */
lwm2m_object_t * ct_get_server_object(int serverId, const char* binding, int lifetime, bool storing);
void ct_clean_server_object(lwm2m_object_t * object);
void display_server_object(lwm2m_object_t * objectP);
void copy_server_object(lwm2m_object_t * objectDest, lwm2m_object_t * objectSrc);

/*
 * object_connectivity_moni.c
 */
lwm2m_object_t * ct_get_object_conn_m(void);
void ct_free_object_conn_m(lwm2m_object_t * objectP);
uint8_t ct_connectivity_moni_change(lwm2m_data_t * dataArray, lwm2m_object_t * objectP);

/*
 * object_connectivity_stat.c
 */
extern lwm2m_object_t * ct_get_object_conn_s(void);
void ct_free_object_conn_s(lwm2m_object_t * objectP);
extern void ct_conn_s_updateTxStatistic(lwm2m_object_t * objectP, uint16_t txDataByte, bool smsBased);
extern void ct_conn_s_updateRxStatistic(lwm2m_object_t * objectP, uint16_t rxDataByte, bool smsBased);

/*
 * object_access_control.c
 */
lwm2m_object_t* acc_ctrl_create_object(void);
void acl_ctrl_free_object(lwm2m_object_t * objectP);
bool  acc_ctrl_obj_add_inst (lwm2m_object_t* accCtrlObjP, uint16_t instId,
                 uint16_t acObjectId, uint16_t acObjInstId, uint16_t acOwner);
bool  acc_ctrl_oi_add_ac_val(lwm2m_object_t* accCtrlObjP, uint16_t instId,
                 uint16_t aclResId, uint16_t acValue);

/*
 * object_security.c
 */
lwm2m_object_t * ct_get_security_object(int serverId, const char* serverUri, char * bsPskId, char * psk, uint16_t pskLen, bool isBootstrap);
void ct_clean_security_object(lwm2m_object_t * objectP);
char * ct_get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);
void ct_display_security_object(lwm2m_object_t * objectP);
void ct_copy_security_object(lwm2m_object_t * objectDest, lwm2m_object_t * objectSrc);

/*
 *  object_data_report.c
*/
lwm2m_object_t * get_data_report_object(void);
void free_data_report_object(lwm2m_object_t * object);

/*
 *	object_custom.c
 */
 lwm2m_object_t * get_mcu_object(uint32_t serialNum,object_instance_array_t objInsArray[]);
uint8_t find_use_targe(char data, char *targe);
char t_strtok(char *in, char *targe, char **args, uint8_t *argc);

#endif /* LWM2MCLIENT_H_ */
