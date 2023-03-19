/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of Abup.
 * File name:    abup_timer.c
 * Description:  abup fota entry source file
 * History:      Rev1.0   2019-08-13
 *
 ****************************************************************************/
#include "debug_log.h"
#include "debug_trace.h"
#include <string.h>
#include "abup_timer.h"

TimerHandle_t abup_creat_timer( const char * const pcTimerName, const uint32_t xTimerPeriodInTicks, const UBaseType_t uxAutoReload, void * const pvTimerID, TimerCallbackFunction_t pxCallbackFunction )
{
        ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_1, P_INFO, 0, "abup_creat_timer");

	if (pcTimerName == NULL || strlen(pcTimerName) == 0)
	{
		return NULL;
	}
	if (xTimerPeriodInTicks == 0)
	{
		return NULL;
	}
	if (pxCallbackFunction == NULL)
	{
		return NULL;
	}
	return xTimerCreate(pcTimerName,xTimerPeriodInTicks/ portTICK_PERIOD_MS,uxAutoReload,pvTimerID,pxCallbackFunction);
}

void abup_start_timer(TimerHandle_t timer_id)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_2, P_INFO, 1, "abup_start_timer id:0x%d", timer_id);

    if(timer_id)
        xTimerStart(timer_id,0);
}

void abup_stop_timer(TimerHandle_t timer_id)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_3, P_INFO, 1, "abup_stop_timer id:0x%d", timer_id);

    if(timer_id)
        xTimerStop(timer_id, 0);
}

void abup_delete_timer(TimerHandle_t timer_id)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_4, P_INFO, 1, "abup_delete_timer id:0x%d", timer_id);

    if(timer_id)
        xTimerDelete(timer_id, 0);
}

void abup_reset_timer(TimerHandle_t timer_id,adups_uint32 delay)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_5, P_INFO, 1, "abup_reset_timer id:0x%d", timer_id);

    if(timer_id)
        xTimerReset(timer_id,delay/ portTICK_PERIOD_MS);
}


