/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    abup_task.c
 * Description:  abup fota entry source file
 * History:      Rev1.0   2019-08-13
 *
 ****************************************************************************/
#include "osasys.h"
#include "ostask.h"
#include "queue.h"
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
//#include "App.h"
#include "abup_task.h"
#include "abup_config.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

typedef enum 
{
    FOTA_CHECK_STEP_START,
    FOTA_CHECK_STEP_NORMAL        
}FOTA_CHECK_STEP_TYPE;


#define ABUP_FOTA_CHECK_METHOD_MINUTES 1
//#define ABUP_FOTA_CHECK_TEST 1
#define ABUP_FOTA_CHECK_TEST 0

#ifdef ABUP_FOTA_CHECK_METHOD_MINUTES

#if (ABUP_FOTA_CHECK_TEST==1)
#define ABUP_FOTA_CHECK_STEP_START_IN_MINUTES  5 
#define ABUP_FOTA_CHECK_STEP_NORMAL_IN_MINUTES  20 
#else
#define ABUP_FOTA_CHECK_STEP_START_IN_MINUTES  5 
#define ABUP_FOTA_CHECK_STEP_NORMAL_IN_MINUTES  600
#endif

//24*3*20 MINUTES
//#define ABUP_FOTA_MAX_UNRECOVER_FAIL_TIMES 72 
//3*20 MINUTES

#define ABUP_FOTA_MAX_UNRECOVER_FAIL_TIMES 3
//static UINT32 delayCnt = 0;
#else
#define ABUP_FOTA_CHECK_STEP_START_IN_MINUTES  5 
#define ABUP_FOTA_CHECK_STEP_NORMAL_IN_HOURS  1 
#endif

UINT8	adups_Imsi[16] = {0};

static StaticTask_t             abupTask;
static UINT8                    abupTaskStack[ABUP_FOTA_TASK_STACKSIZE];
static StaticTask_t             abupTaskTimer;
static UINT8                    abupTaskStackTimer[ABUP_FOTA_TASK_TIMERSIZE];
static volatile UINT32          Event;
static QueueHandle_t            abup_fota_queue = NULL;

TimerHandle_t montior_timer = NULL;

volatile uint32_t abup_task = 0;
volatile uint32_t abup_timer_task = 0;
volatile uint32_t abup_quit_task = 0;
uint8_t g_abup_active_pdp = 0;
signed int abup_app_id = -1;
adups_uint8 montior_count = 0;
static adups_uint8 abup_curr_op = ABUP_CURR_OP_RU;//ABUP_CURR_OP_RU;

uint8_t adups_process_state = 0;
uint8_t g_adups_wakeup_start = 0;

extern char * abup_client_mid[21];
extern void abup_notify_status_reset(void);
extern int abup_lwm2mclient2(void);
extern ADUPS_BOOL abup_is_inter_exit(void);
extern void abup_notify_status(adups_uint32 a_status, adups_uint32 a_percentage);
extern void abup_inter_exit_reset(void);
extern int abup_g_quit;
extern void abup_main_download_timeout_handler(void);
extern void abup_download_init_flash(void);

adups_uint8 abup_get_curr_op(void)
{
    return abup_curr_op;
}

void abup_set_curr_op(adups_uint8 op)
{
    abup_curr_op = op;
}

static void adupssendQueueMsg(UINT32 msgId, UINT32 xTickstoWait)
{
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (abup_fota_queue)
    {
        if (pdTRUE != xQueueSend(abup_fota_queue, &queueMsg, xTickstoWait))
        {
			ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_1, P_INFO, 0, "xQueueSend error");
			free(queueMsg);
			queueMsg=NULL;
        }
    }
}

void abup_set_quit_task()
{
	if (abup_quit_task == 0)
	{
		abup_notify_status(ABUP_STATUS_FAIL, 0);
		abup_montior_end();
		abup_inter_exit_reset();
		abup_quit_task = 1;
		montior_count = 0;
	}
}

ADUPS_BOOL adup_deactive_pdp(void)
{
    if (abup_app_id != -1)
    {
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_4, P_INFO, 0, "adups_deactive_pdp!");
        //tel_conn_mgr_deactivate(abup_app_id);
        abup_app_id = -1;
    }
    return ADUPS_TRUE;
}

void abup_montior_timer(TimerHandle_t xTimer)
{
    //need deactive then re-active?
    //adup_deactive_pdp();
}

void abup_montior_start(void)
{
    if (montior_count >= MAX_MONTIOR_COUNT || abup_is_inter_exit() || abup_quit_task == 1)
    {
        abup_montior_end();
        abup_set_quit_task();
    }
    else
    {
        montior_count++;
        montior_timer = xTimerCreate("montior_timer", 30000 / portTICK_RATE_MS, pdFALSE, NULL, abup_montior_timer);
        xTimerStart(montior_timer, 0);
    }
}

void abup_montior_end(void)
{
    if (montior_timer)
    {
        xTimerStop(montior_timer, 0);
        xTimerDelete(montior_timer, 0);
        montior_timer = NULL;
    }
}

ADUPS_BOOL abup_active_pdp()
{
	//Default boot activation pdp
    return ADUPS_TRUE;
}

ADUPS_BOOL adups_deactive_pdp(void)
{
    if (abup_app_id != -1)
    {		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_6, P_INFO, 0, "adups_deactive_pdp!");
        //tel_conn_mgr_deactivate(adups_app_id);
        abup_app_id = -1;
    }
    return ADUPS_TRUE;
}

uint8_t CheckNetStatActive(void)
{
    NmAtiSyncRet netStatus;
    appGetNetInfoSync(0, &netStatus);
    if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
    {
        return 1;
    }
    return 0;

}
uint8_t WaitNetStatActive(void)
{
    UINT32 start, end;
    
    /*data observe data report*/   
    start = osKernelGetTickCount()/portTICK_PERIOD_MS/1000;
    end=start;
    
    // Check network for 120 seconds
    while(end-start < 120)
    {
        if (CheckNetStatActive())
        {
            return 1;
        }
        osDelay(1000);
        end = osKernelGetTickCount()/portTICK_PERIOD_MS/1000;
    }
    
    ECOMM_TRACE(UNILOG_ABUP_APP, wait_netstat_active_1, P_INFO, 0, "network not ready");
    return 0;
}


//格式化flash的回调发送task消息
void abup_main_init_flash_queue(void)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_18, P_INFO, 1, "abup_main_init_flash_queue msg_id=%d", ABUP_TASK_MSGID_FLASH);
    adupssendQueueMsg(ABUP_TASK_MSGID_FLASH, 500);
}

//网络异常超时等待的回调发送task消息
void abup_main_download_timer_queue(void)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_19, P_INFO, 1, "abup_main_download_timer_queue msg_id=%d", ABUP_TASK_MSGID_TIMER);
    adupssendQueueMsg(ABUP_TASK_MSGID_TIMER, 500);
}

uint32_t AbupFotaCheckStepStart(void)
{
    static FOTA_CHECK_STEP_TYPE step = FOTA_CHECK_STEP_START;
    if (step == FOTA_CHECK_STEP_START)
    {
        step = FOTA_CHECK_STEP_NORMAL;
        return 1;
    }
    else
    {
        return 0;
    }
}

void AbupDeepSleepCb(uint8_t id)
{
    ECOMM_TRACE(UNILOG_ABUP_APP, deepSleepCb_1, P_SIG, 1, "callback ID:%d come, disable slp2", id);
    g_adups_wakeup_start = 1;
}    

void fotaDeepSleepTimerCbRegister(void)
{
    slpManInternalDeepSlpTimerRegisterExpCb(AbupDeepSleepCb,DEEPSLP_TIMER_ID9);
}

uint8_t abup_vote_handle = 0xff;
void adups_fota_task_thread(void *arg)
{
    CHAR  imeiNum[21] = {0};
    uint32_t UpdatePeriodIdx = 0; //hours
    int ret = 0;
    slpManApplyPlatVoteHandle("ABUP", &abup_vote_handle);
    	
    //printf("adups_fota_task_thread start!!!!!!!!!\n");
    ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_01, P_INFO, "Firmware version: %s",ABUP_FIRMWARE_VERSION);

    //wait net stat active, if wait timeout, just pass to update flow
    WaitNetStatActive();

    fotaDeepSleepTimerCbRegister();
    abup_notify_status_reset();
    
    abup_trans_mutex_init();
    abup_quit_task = 0;
	
    while(1)
    {
        if (CheckNetStatActive()==0)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;        
        }
    
        //ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_11, P_INFO, 0, "FOTA TASK V01");
        ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_01, P_INFO, "Firmware version: %s",ABUP_FIRMWARE_VERSION);        
        abup_task += 1;

        //check update info and set curr op 
        abup_check_upgrade_result();
        if(abup_get_upgrade_result() == 0)
        {
            abup_get_device_info_string(0, ABUP_RES_O_FIRMWARE_VERSION);        
            abup_set_curr_op(ABUP_CURR_OP_CV);
        }
        else
        {
            abup_set_curr_op(ABUP_CURR_OP_RU);        
        }
        
        appGetImsiNumSync((char *)adups_Imsi);
        appGetImeiNumSync(imeiNum);
        memcpy(abup_client_mid, imeiNum, 15);

        ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_13, P_INFO, "adups_fota_task_thread-->imsi:%s",adups_Imsi);

        g_abup_active_pdp = 0;

        slpManPlatVoteDisableSleep(abup_vote_handle, SLP_SLP2_STATE);
        if (1)//(abup_active_pdp() == ADUPS_TRUE)
        {
        	ret = abup_lwm2mclient2();
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_14, P_INFO, 1, "abup_lwm2mclient2 ret=%d!", ret);
            
        	//adups_deactive_pdp();
        }
        slpManPlatVoteEnableSleep(abup_vote_handle, SLP_SLP2_STATE);

        //abup_montior_end();
        //abup_montior_start();					
        ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_16, P_INFO, 0, "FOTA EXIT2!");

        vTaskDelay(1000 / portTICK_RATE_MS);
        if(abup_quit_task == 1 || abup_task >= 300) 
        {
        	
            #if (ABUP_FOTA_CHECK_METHOD_MINUTES==1)
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_20, P_INFO, 0, "FOTA DELAY START!");
            
            if (AbupFotaCheckStepStart())
            {
                ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_21, P_INFO, 0, "FOTA CHECK START STEP!");
                for (UpdatePeriodIdx = 0; UpdatePeriodIdx < ABUP_FOTA_CHECK_STEP_START_IN_MINUTES; UpdatePeriodIdx++)
                {
                    vTaskDelay(60*1000 / portTICK_RATE_MS);
                }        		                            
            }
            else
            {
                ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_22, P_INFO, 0, "FOTA CHECK NORMAL STEP!");
                //slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID9, ABUP_FOTA_CHECK_STEP_NORMAL_IN_MINUTES*60*1000);
                
                for (UpdatePeriodIdx = 0; UpdatePeriodIdx < ABUP_FOTA_CHECK_STEP_NORMAL_IN_MINUTES; UpdatePeriodIdx++)
                {
                    vTaskDelay(60*1000 / portTICK_RATE_MS);
                }        		
            }
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_23, P_INFO, 0, "FOTA DELAY END!");
            
    		/*if (abup_quit_task==0)
    		{
                delayCnt++;
                if (delayCnt > ABUP_FOTA_MAX_UNRECOVER_FAIL_TIMES)
                {
    			ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_22, P_INFO, 0, "FOTA failed too many times, SystemReset now!");
    			vTaskDelay(5*1000 / portTICK_RATE_MS);
    			_disable_irq();
    			NVIC_SystemReset();
                }
            }*/
            
            #ifdef ABUP_UPDATE_SUC_REBOOT_TEST
            extern void AbupUpdateChkRebootTest();
            AbupUpdateChkRebootTest();
            #endif
            
            #else
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_25, P_INFO, 0, "FOTA DELAY START!");
            
            if (AbupFotaCheckStepStart())
            {
                ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_26, P_INFO, 0, "FOTA CHECK START STEP!");
                for (UpdatePeriodIdx = 0; UpdatePeriodIdx < ABUP_FOTA_CHECK_STEP_NORMAL_IN_MINUTES; UpdatePeriodIdx++)
                {
                    vTaskDelay(60*1000 / portTICK_RATE_MS);
                }                                           
            }
            else
            {
            
                ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_27, P_INFO, 0, "FOTA CHECK NORMAL STEP!");
                //slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID9, ABUP_FOTA_CHECK_STEP_NORMAL_IN_HOURS*3600*1000);
            
                for (UpdatePeriodIdx = 0; UpdatePeriodIdx < ABUP_FOTA_CHECK_STEP_NORMAL_IN_HOURS; UpdatePeriodIdx++)
                {
                    vTaskDelay(3600*1000 / portTICK_RATE_MS);
                }
                ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_28, P_INFO, 0, "FOTA DELAY END!");
            }
            #endif
            //reset the count before  next update time  
            abup_quit_task = 0;
            abup_task = 0;
             //break;
        }
    }
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TASK_LOG_17, P_INFO, 0, "FOTA EXIT!!!");
    abup_task = 0;
    vTaskDelete(NULL);
}


//timer的回调单独建一个task
void adups_fota_timer_thread(void *arg)
{
	eventCallbackMessage_t *message = NULL;
	
	abup_fota_queue = xQueueCreate(ABUP_FOTA_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
	if (abup_fota_queue == NULL)
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_TASK_LOG_20, P_INFO, 0, "abup_fota_timer_queue create error!");
		return;
	}
	else
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_TASK_LOG_21, P_INFO, 0, "abup_fota_timer_queue create sucess!");
	}
		
	while(1)
	{
		abup_timer_task += 1;
		if(xQueueReceive(abup_fota_queue, &message, portMAX_DELAY)) 
		{
			ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_TIMER_TASK_LOG_12, P_INFO, 1, "adups_fota_timer_thread-->message:%d",message->messageId);
			switch(message->messageId)
			{
				case ABUP_TASK_MSGID_FLASH:
					abup_download_init_flash();
				break;
				
				case ABUP_TASK_MSGID_TIMER:
					abup_main_download_timeout_handler();
				break;

                default:
                    break;
			}
            free(message);
		}	
		
		vTaskDelay(1000 / portTICK_RATE_MS);
		if(abup_quit_task == 1 || abup_timer_task >= 300) 
		{
			 //break;
		}
	}
	
	abup_timer_task = 0;
	vTaskDelete(NULL);
}

void abup_timer_task_init(void)
{
    osThreadAttr_t task_attr;
    memset(&task_attr,0,sizeof(task_attr));
    memset(abupTaskStackTimer, 0xA5,ABUP_FOTA_TASK_TIMERSIZE);
    task_attr.name = "abup_timer";
    task_attr.stack_mem = abupTaskStackTimer;
    task_attr.stack_size = ABUP_FOTA_TASK_TIMERSIZE;
    task_attr.priority = osPriorityLow;//osPriorityBelowNormal;
    task_attr.cb_mem = &abupTaskTimer;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(adups_fota_timer_thread, NULL, &task_attr);
}

void abupfotaInit(void)
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

    osThreadNew(adups_fota_task_thread, NULL, &task_attr);
	
    abup_timer_task_init();
}

void abup_fota_exit(void)
{	
	if (abup_quit_task == 0)
	{
		//abup_montior_end();
		abup_quit_task = 1;
	}
	
	//adups_test_freesize();
}

uint8_t abup_get_process_state(void)
{
	return adups_process_state;
}


//void abup_init_task(void)
//{
//	TimerHandle_t get_init_timer = xTimerCreate("get_init_timer", 10000 / portTICK_PERIOD_MS, false, NULL, abupfotaInit);
//	xTimerStart(get_init_timer, 0);
//}

