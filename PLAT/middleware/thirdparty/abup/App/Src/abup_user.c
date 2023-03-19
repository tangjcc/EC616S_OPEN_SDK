/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    abup_user.c
 * Description:  abup fota entry source file
 * History:      Rev1.0   2019-08-13
 *
 ****************************************************************************/
 #include <string.h>
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "debug_log.h"
#include "debug_trace.h"
#include "abup_typedef.h"
#include "abup_config.h"
#include "abup_task.h"
#include "bsp.h"


static adups_uint32 abup_status = 0;
static adups_uint32 abup_percentage = 0;
static ADUPS_BOOL abup_inter_exit = ADUPS_FALSE;
extern void abup_quit(void);
extern void abup_set_quit_task(void);
extern adups_uint8 abup_get_curr_op(void);
#ifdef ABUP_UPDATE_SUC_REBOOT_TEST
uint32_t AbupUpdateResultResetFlag = 0;
void AbupUpdateResultResetSet(uint32_t Reset)
{
	AbupUpdateResultResetFlag = Reset;
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER1_RESET_SET, 1, P_INFO,  "AbupUpdateResultFlag:%d", AbupUpdateResultResetFlag);
}

void AbupUpdateChkRebootTest(void)
{
	ECOMM_TRACE(UNILOG_ABUP_APP, AbupUpdateChkRebootTest, 1, P_INFO, "AbupUpdateResultFlag:%d", AbupUpdateResultResetFlag);
        if ((AbupUpdateResultResetFlag == 1) ||
            (AbupUpdateResultResetFlag == 2))
        {
            __disable_irq();
            NVIC_SystemReset();
        }
}
#endif

void abup_notify_fun(char *err_str)
{
	ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_USER1_LOG_1, P_INFO, "abup_notify_fun:%s",(uint8_t *)err_str);

	if (!abup_inter_exit)
	{
		if (strcmp(err_str, "ABUP_DOWNLOAD_INIT timerout failed") != 0)
		{
			abup_inter_exit = ADUPS_TRUE;
		}
	}
}

ADUPS_BOOL abup_is_inter_exit(void)
{
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER1_LOG_2, P_INFO, 1, "abup_is_inter_exit:%d", abup_inter_exit);
	return abup_inter_exit;
}

void abup_inter_exit_reset(void)
{
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER1_LOG_3, P_INFO, 0, "abup_inter_exit_reset");
	abup_inter_exit = ADUPS_FALSE;
}

void abup_notify_exit(void)
{
	abup_quit();
	abup_set_quit_task();
}

void abup_notify_status_reset(void)
{
	abup_status = 0;
	abup_percentage = 0;
}

void abup_notify_status(adups_uint32 a_status, adups_uint32 a_percentage)
{
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER1_LOG_4, P_INFO, 5, "abup_notify_status-->%d,%d,curr_op:%d,status:%d,download_percent:%d",a_status,a_percentage,abup_get_curr_op(),abup_status,abup_percentage);

	if (abup_get_curr_op() != ABUP_CURR_OP_CV)
	{
		return;
	}
	if (a_percentage > 100)
	{
		return;
	}
	if (abup_status == ABUP_STATUS_FAIL && a_status == ABUP_STATUS_FAIL)
	{
		return;
	}
	if (a_status == ABUP_STATUS_DOWNLOADING)
	{
		if (a_percentage < abup_percentage)
		{
			a_percentage = abup_percentage;
		}
	}
	abup_status = a_status;
	abup_percentage = a_percentage;
}

void opencpu_fota_event_cb(adups_uint8 event,adups_int8 state)
{

    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER1_LOG_5, P_INFO, 2, "opencpu_fota_event_cb-->event:%d,state:%d",event,state);
	vTaskDelay(1000 / portTICK_RATE_MS);

    switch(event)
    {
        case 0://检测流程
            switch(state)
            {
                case 0://无升级包
                    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_6, P_INFO, 0, "NO packet");
                    break;

                case 1://有升级包可下载
                    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_7, P_INFO, 0, "find new packet,ready to download");
                    break;

                //case 2://有升级包且已下载
                //ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_8, P_INFO, 0,"find new packet,and has already been downloaded\n");
                //break;

                case 3: //平台拒绝下载，同一个IMEI每天最大访问次数100
                    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_9, P_INFO, 0,"download rejected");
                    break;

                case 4: //检测超时

                    break;
                default:
                	break;
            }
        break;

        case 1://下载流程
            switch(state)
            {
            	case 0://开始下载
            	    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_10, P_INFO, 0,"JMS Fota start download......");
            	    break;

            	case 1://解析下载域名超时

            	    break;

            	case 2://下载超时
            	    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_11, P_INFO, 0,"download pause");
            	    break;

            	case 3://下载成功
            	    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_12, P_INFO, 0,"download success");
            	    break;

            	case 4: //下载完后MD5校验失败，并且丢弃当前进度，调用abup_send_get_new_version_msg()会重新查询升级包并下载
            	    ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_13, P_INFO, 0,"download failed");
            	    break;

            	default:
                break;
            }
    	break;

    case 2://升级结果流程
        switch(state)
        {
        case 1://升级结果上报时网络异常退出
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_16, P_INFO, 0,"OTA update network error");
            break;
        case 2://升级结果上报时网络正常，上报完成
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_17, P_INFO, 0,"OTA update network OK");

            if (UsartAtCmdHandle != NULL)
            {
                UsartAtCmdHandle->SendPolling((CHAR *)"\r\nAB OTA RPT DONE\r\n", strlen("\r\nAB OTA RPT DONE\r\n"));
            }

            break;
        case 3://重启后升级成功
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_14, P_INFO, 0,"OTA update success");

            if (UsartAtCmdHandle != NULL)
            {
                UsartAtCmdHandle->SendPolling((CHAR *)"\r\nAB OTA SUCESS\r\n", strlen("\r\nAB OTA SUCESS\r\n"));
            }
            #ifdef ABUP_UPDATE_SUC_REBOOT_TEST
            AbupUpdateResultResetSet(1);
            #endif
            break;

        case 4: //重启后升级失败
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_USER_LOG_15, P_INFO, 0,"OTA update failed");

            if (UsartAtCmdHandle != NULL)
            {
                UsartAtCmdHandle->SendPolling((CHAR *)"\r\nAB OTA FAIL\r\n", strlen("\r\nAB OTA FAIL\r\n"));
            }


            #ifdef ABUP_UPDATE_SUC_REBOOT_TEST
            AbupUpdateResultResetSet(2);
            #endif
            break;

        default:
            break;
        }
    	break;

    	default:
    		break;
    }
}
