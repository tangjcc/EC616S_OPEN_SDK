/*******************************************************************************
 *
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Olaf Bergmann (TZI) and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v. 1.0 which accompanies this distribution.
 *
 * The Eclipse Public License is available at http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Olaf Bergmann  - initial API and implementation
 *
 *******************************************************************************/

/**
 * @file dtls_time.c
 * @brief Clock Handling
 */

#include "tinydtls.h"
#include "dtls_time.h"
#include "cmsis_os2.h"//EC add
#include "osasys.h"//EC add

#ifdef WITH_CONTIKI
clock_time_t dtls_clock_offset;

void
dtls_clock_init(void) {
  clock_init();
  dtls_clock_offset = clock_time();
}

void
dtls_ticks(dtls_tick_t *t) {
  *t = clock_time();
}

#else /* WITH_CONTIKI */

time_t dtls_clock_offset;

void
dtls_clock_init(void) {
#ifdef HAVE_TIME_H
  dtls_clock_offset = osKernelGetTickCount()/portTICK_PERIOD_MS;//EC changed
#else
#  ifdef __GNUC__
  /* Issue a warning when using gcc. Other prepropressors do 
   *  not seem to have a similar feature. */ 
#   warning "cannot initialize clock"
#  endif
  dtls_clock_offset = osKernelGetTickCount()/portTICK_PERIOD_MS;
#endif
}

void dtls_ticks(dtls_tick_t *t) {
#if HAVE_SYS_TIME_H
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *t = (tv.tv_sec - dtls_clock_offset) * DTLS_TICKS_PER_SECOND 
    + (tv.tv_usec * DTLS_TICKS_PER_SECOND / 1000000);

#else
//#error "clock not implemented"
  UINT32 current_ms = 0;
  current_ms = osKernelGetTickCount()/portTICK_PERIOD_MS;
  *t = current_ms - dtls_clock_offset;

#endif
}

#endif /* WITH_CONTIKI */


