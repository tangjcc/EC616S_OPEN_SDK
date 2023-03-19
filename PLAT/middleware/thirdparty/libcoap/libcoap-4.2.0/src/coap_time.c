/* coap_time.c -- Clock Handling
 *
 * Copyright (C) 2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include <coap_freertos/coap_config.h>

#ifdef HAVE_TIME_H
//#include <time.h>
#ifdef HAVE_SYS_TIME_H
//#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>  /* _POSIX_TIMERS */
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#include <stdint.h>
#endif
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include <coap2/libcoap.h>
#include <coap2/coap_time.h>

static coap_tick_t coap_clock_offset = 0;

#if _POSIX_TIMERS && !defined(__APPLE__)
  /* _POSIX_TIMERS is > 0 when clock_gettime() is available */

  /* Use real-time clock for correct timestamps in coap_log(). */
#define COAP_CLOCK CLOCK_REALTIME
#endif

#ifdef HAVE_WINSOCK2_H
static int
gettimeofday(struct timeval *tp, TIME_ZONE_INFORMATION *tzp) {
  (void)tzp;
  static const uint64_t s_tUnixEpoch = 116444736000000000Ui64;

  FILETIME file_time;
  ULARGE_INTEGER time;
  uint64_t tUsSinceUnicEpoch;

  GetSystemTimeAsFileTime( &file_time );
  time.LowPart = file_time.dwLowDateTime;
  time.HighPart = file_time.dwHighDateTime;
  tUsSinceUnicEpoch = ( time.QuadPart - s_tUnixEpoch ) / 10;

  tp->tv_sec = (long)(tUsSinceUnicEpoch / 1000000);
  tp->tv_usec = (long)(tUsSinceUnicEpoch % 1000000);
  return 0;
}
#endif

void
coap_clock_init(void) {
#ifdef COAP_CLOCK
  struct timespec tv;
  clock_gettime(COAP_CLOCK, &tv);
  coap_clock_offset = tv.tv_sec;
#else /* _POSIX_TIMERS */
  UINT32 current_ms = 0;
  current_ms = osKernelGetTickCount()/portTICK_PERIOD_MS;
  coap_clock_offset = current_ms/1000;
#endif /* not _POSIX_TIMERS */


}

/* creates a Qx.frac from fval */
#define Q(frac,fval) ((coap_tick_t)(((1 << (frac)) * (fval))))

/* number of frac bits for sub-seconds */
#define FRAC 10

/* rounds val up and right shifts by frac positions */
#define SHR_FP(val,frac) (((val) + (1 << ((frac) - 1))) >> (frac))

void
coap_ticks(coap_tick_t *t) {

#ifdef COAP_CLOCK
  coap_tick_t tmp  = 0; /*ec modify*/
  struct timespec tv;
  clock_gettime(COAP_CLOCK, &tv);
  /* Possible errors are (see clock_gettime(2)):
   *  EFAULT tp points outside the accessible address space.
   *  EINVAL The clk_id specified is not supported on this system.
   * Both cases should not be possible here.
   */

  tmp = SHR_FP(tv.tv_nsec * Q(FRAC, (COAP_TICKS_PER_SECOND/1000000000.0)), FRAC);
  
  *t = tmp + (tv.tv_sec - coap_clock_offset) * COAP_TICKS_PER_SECOND;
#else /* _POSIX_TIMERS */
  UINT32 current_ms = 0;
  current_ms = osKernelGetTickCount()/portTICK_PERIOD_MS;

  /* Finally, convert temporary FP representation to multiple of
   * COAP_TICKS_PER_SECOND */
  *t = current_ms;
#endif /* not _POSIX_TIMERS */
}

coap_time_t
coap_ticks_to_rt(coap_tick_t t) {
  return coap_clock_offset + (t / COAP_TICKS_PER_SECOND);
}

uint64_t coap_ticks_to_rt_us(coap_tick_t t) {
  return (uint64_t)coap_clock_offset * 1000000 + (uint64_t)t * 1000000 / COAP_TICKS_PER_SECOND;
}

coap_tick_t coap_ticks_from_rt_us(uint64_t t) {
  return (coap_tick_t)((t - (uint64_t)coap_clock_offset * 1000000) * COAP_TICKS_PER_SECOND / 1000000);
}

#undef Q
#undef FRAC
#undef SHR_FP

#else /* HAVE_TIME_H */

/* make compilers happy that do not like empty modules */
COAP_STATIC_INLINE void dummy(void)
{
}

#endif /* not HAVE_TIME_H */

