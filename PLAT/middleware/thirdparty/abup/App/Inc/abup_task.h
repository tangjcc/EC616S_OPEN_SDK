#ifndef  ABUP_TASK_H
#define  ABUP_TASK_H

#include "abup_typedef.h"
#include "abup_task.h"
#include "abup_flash.h"
#include "abup_device.h"
#include "portmacro.h"
#include "queue.h"

#define ABUP_FOTA_TASK_STACKSIZE        	(1024 * 4)
#define ABUP_FOTA_TASK_TIMERSIZE        	(1024 * 2)
#define ABUP_FOTA_EVENT_QUEUE_SIZE    (10)
#define MAX_MONTIOR_COUNT 1

#define ABUP_CURR_OP_RU	0
#define ABUP_CURR_OP_CV	1
#define ABUP_TASK_MSGID_UPDATE			100
#define ABUP_TASK_MSGID_UPDATE_RESULT	101
#define ABUP_TASK_MSGID_FLASH			102
#define ABUP_TASK_MSGID_TIMER	103

#define ABUP_STATUS_START 500
#define ABUP_STATUS_CHECK_VERSION 501
#define ABUP_STATUS_DOWNLOADING 502
#define ABUP_STATUS_FAIL 503

void abupfotaInit(void);
void abup_init_task(void);
ADUPS_BOOL adups_deactive_pdp(void);
void abup_montior_end(void);
void abup_montior_start(void);
void abup_montior_timer(TimerHandle_t xTimer);
adups_uint8 abup_get_curr_op(void);
int abup_lwm2mclient2(void);
uint8_t WaitNetStatActive(void);
uint8_t CheckNetStatActive(void);
void abup_set_curr_op(adups_uint8 op);

typedef struct _ABUTMutex
{
    QueueHandle_t sem;
}ABUPMutex_t;

extern void abup_trans_mutex_init(void);    
void abup_main_init_flash_queue(void);
void abup_main_download_timer_queue(void);
void abup_timer_task_init(void);

#endif
