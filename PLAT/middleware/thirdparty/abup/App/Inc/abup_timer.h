#ifndef _ADUPS_TIMER_H_
#define _ADUPS_TIMER_H_

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "abup_typedef.h"
#include "abup_stdlib.h"
#include "FreeRTOS.h"
#include "timers.h"


TimerHandle_t abup_creat_timer( const char * const pcTimerName, const uint32_t xTimerPeriodInTicks, const UBaseType_t uxAutoReload, void * const pvTimerID, TimerCallbackFunction_t pxCallbackFunction );
void abup_start_timer(TimerHandle_t timer_id);
void abup_stop_timer(TimerHandle_t timer_id);
void abup_delete_timer(TimerHandle_t timer_id);
void abup_reset_timer(TimerHandle_t timer_id,adups_uint32 delay);

#endif

