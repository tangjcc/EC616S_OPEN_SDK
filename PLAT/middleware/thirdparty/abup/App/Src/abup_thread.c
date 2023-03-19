/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    abup_thread.c
 * Description:  abup fota entry source file
 * History:      Rev1.0   2020-01-19
 *
 ****************************************************************************/
#include "osasys.h"
#include "ostask.h"
#include "task.h"
#include "ps_event_callback.h"
#include "debug_log.h"
#include "debug_trace.h"
#include "cmisim.h"
#include "cmimm.h"
#include "cmips.h"
#include "sockets.h"
#include "psifevent.h"
#include "ps_lib_api.h"
#include "lwip/netdb.h"
#include "abup_task.h"
#include "abup_config.h"


TimerHandle_t ru_timer = NULL;
volatile uint32_t abup_thread_task = 0;

extern uint32_t abup_task;
extern uint32_t abup_quit_task;
extern adups_uint8 abup_curr_op;
extern char * abup_client_mid[21];
extern UINT8	adups_Imsi[16];
extern StaticTask_t             abupTask;
extern UINT8                    abupTaskStack[ABUP_FOTA_TASK_STACKSIZE];
extern void abup_notify_status_reset(void);
extern int abup_lwm2mclient2(void);
extern void abup_notify_status(adups_uint32 a_status, adups_uint32 a_percentage);


void abup_fota_task_thread(void *arg)
{
    CHAR  imeiNum[21] = {0};
	
    //printf("abup_fota_task_thread start!!!!!!!!!\n");
	ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_THREAD_LOG_3, P_INFO, "Firmware version: %s",ABUP_FIRMWARE_VERSION);

    //wait net stat active, if wait timeout, just pass to update flow
    WaitNetStatActive();

    abup_notify_status_reset();
    abup_quit_task = 0;
		
    while(1)
    {
        if (CheckNetStatActive()==0)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;        
        }
        abup_thread_task += 1;
		
        appGetImsiNumSync((char *)adups_Imsi);
        appGetImeiNumSync(imeiNum);
        memcpy(abup_client_mid, imeiNum, 15);

        ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_THREAD_LOG_4, P_INFO, "abup_fota_task_thread-->imsi:%s",adups_Imsi);
        
        if (1)//(abup_active_pdp() == ADUPS_TRUE)
        {
        	abup_lwm2mclient2();
        }

        abup_montior_end();
    	abup_montior_start();					
        ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_THREAD_LOG_5, P_INFO, 0, "FOTA EXIT2!");

        vTaskDelay(1000 / portTICK_RATE_MS);
        if(abup_quit_task == 1 || abup_thread_task >= 30) 
        {
            break;
        }
    }
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_THREAD_LOG_6, P_INFO, 0, "FOTA EXIT!!!");
    vTaskDelete(NULL);
    abup_thread_task = 0;
}

//task进程，只有启动fota才会调用
void abup_thread_init(void)
{
    osThreadAttr_t task_attr;
    memset(&task_attr,0,sizeof(task_attr));
    memset(abupTaskStack, 0xA5,ABUP_FOTA_TASK_STACKSIZE);
    task_attr.name = "abup_fota";
    task_attr.stack_mem = abupTaskStack;
    task_attr.stack_size = ABUP_FOTA_TASK_STACKSIZE;
    task_attr.priority = osPriorityLow;//osPriorityBelowNormal;
    task_attr.cb_mem = &abupTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(abup_fota_task_thread, NULL, &task_attr);
}

//检查版本API，检查到新版本，自动下载完重启升级
//abup_task 大于1，说明是开启自动启动fota,与手动启动互斥
void adups_check_version(void)
{
    if (abup_thread_task == 0 && abup_task==0)
    {
        if (ru_timer)
        {
            xTimerStop(ru_timer, 0);
            xTimerDelete(ru_timer, 0);
            ru_timer = NULL;
        }
		abup_set_curr_op(ABUP_CURR_OP_CV);
        abup_thread_init();
		vTaskDelay(3*1000 / portTICK_RATE_MS);
		
		abup_get_device_info_string(0, ABUP_RES_O_FIRMWARE_VERSION);
		abup_notify_status(ABUP_STATUS_START, 0);
    }
}

//先检查是否存在升级标志符
//存在标志符，进行升级结果上报；不存在，退出
//abup_task 大于1，说明是开启自动启动fota,与手动启动fota互斥
void adups_send_update_result_timer(TimerHandle_t xTimer)
{
    if (abup_thread_task == 0 && abup_task==0)
	{
		abup_check_upgrade_result();
		if (abup_get_upgrade_result() == 0)
		{
			adups_check_version();//测试用，正式版本请去掉，客户自己调用此函数
			return;
		}
		abup_set_curr_op(ABUP_CURR_OP_RU);
		abup_thread_init();
	}
}

//开机 20s后启动升级结果上报
void adups_check_update_result(void)
{
	if(ru_timer==NULL)
	{
		ru_timer = xTimerCreate("ru_timer", 20000 / portTICK_PERIOD_MS, false, NULL, adups_send_update_result_timer);
		xTimerStart(ru_timer, 0);
	}
}

