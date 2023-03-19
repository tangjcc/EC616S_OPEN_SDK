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

#ifndef PLATFORM_H_
#define PLATFORM_H_
#include "ct_liblwm2m.h"
#include "ctiot_lwm2m_sdk.h"
#ifdef __cplusplus
extern "C"
{
#endif


void platform_notify(uint32_t reqhandle, char* at_str);


#ifdef __cplusplus
}
#endif

#endif //PLATFORM_H_
