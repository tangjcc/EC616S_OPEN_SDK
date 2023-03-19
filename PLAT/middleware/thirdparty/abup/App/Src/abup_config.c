/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    abup_config.c
 * Description:  abup fota entry source file
 * History:      Rev1.0   2019-08-12
 *
 ****************************************************************************/
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "abup_config.h"
#include "abup_device.h"
#include "abup_hmd5.h"
#include "abup_firmware.h"
#include "abup_typedef.h"

typedef struct abup_fota_project_info
{
	char abup_info_manufacturer[16];
	char abup_info_model[12];
	char abup_info_pro_id[12];
	char abup_info_pro_sec[34];
	char abup_info_device_type[12];
	char abup_info_platform[12];
	char abup_info_version[36];
}abup_FOTA_PROJECT_INFO;

abup_FOTA_PROJECT_INFO s_abup_info = {0};

extern abup_firmware_data_t* abup_contextp;
extern char  abup_client_mid[21];
extern int abup_g_quit;
extern uint32_t abup_get_delta_len(void);

 
//对应后台项目信息里的 OEM
adups_uint8 *abup_get_manufacturer(void)
{
	if(strlen(s_abup_info.abup_info_manufacturer)>0)
 		return (uint8_t *)s_abup_info.abup_info_manufacturer;
	else
		return (uint8_t *)ABUP_MANUFACTURER;
}

//对应后台项目信息里的设备型号
adups_uint8 *abup_get_model_number(void)
{
	 if(strlen(s_abup_info.abup_info_model)>0)
		 return (uint8_t *)s_abup_info.abup_info_model;
	 else
	 	return (uint8_t *)ABUP_MODEL_NUMBER;
}
 
//对应后台项目信息 Product ID	 ,创建项目时自动生成
adups_uint8 *abup_get_product_id(void)
{
	 if(strlen(s_abup_info.abup_info_pro_id)>0)
		 return (uint8_t *)s_abup_info.abup_info_pro_id;
	 else
	 	return (uint8_t *)ABUP_PRODUCT_ID;
}

//对应后台项目信息 Product Secret,创建项目时自动生成
adups_uint8 *abup_get_product_sec(void)
{
	 if(strlen(s_abup_info.abup_info_pro_sec)>0)
		 return (uint8_t *)s_abup_info.abup_info_pro_sec;
	 else
		 return (uint8_t *)ABUP_PRODUCT_SEC;
}
 
//对应后台项目信息 设备类型
adups_uint8 *abup_get_device_type(void)
{
	if(strlen(s_abup_info.abup_info_device_type)>0)
		return (uint8_t *)s_abup_info.abup_info_device_type;
	else
		return (uint8_t *)ABUP_DEVICE_TYPE;
}

//对应后台项目信息 平台
adups_uint8 *abup_get_platform(void)
{
	if(strlen(s_abup_info.abup_info_platform)>0)
		return (uint8_t *)s_abup_info.abup_info_platform;
	else
		return (uint8_t *)ABUP_PLATFORM;
}
 
//检测版本时的版本号，要与abup_info.txt里的 version 一致
adups_uint8 *abup_get_firmware_version(void)
{
	if(strlen(s_abup_info.abup_info_version)>0)
		return (uint8_t *)s_abup_info.abup_info_version;
	else
		return (uint8_t *)ABUP_FIRMWARE_VERSION;
}
 
//设置后台项目信息里的 OEM
void abup_set_manufacturer(adups_uint8 * abup_oem)
{
	strcpy(s_abup_info.abup_info_manufacturer, (char *)abup_oem);
}

//设置后台项目信息里的model
void abup_set_model(adups_uint8 * abup_moedl)
{
	strcpy(s_abup_info.abup_info_model, (char *)abup_moedl);
}

//设置后台项目信息里的pro_id
void abup_set_pro_id(adups_uint8 * abup_pro_id)
{
	strcpy(s_abup_info.abup_info_pro_id, (char *)abup_pro_id);
}

//设置后台项目信息里的pro_sec
void abup_set_pro_sec(adups_uint8 * abup_pro_sec)
{
	strcpy(s_abup_info.abup_info_pro_sec, (char *)abup_pro_sec);
}

//设置后台项目信息里的device_type
void abup_set_device_type(adups_uint8 * abup_device_type)
{
	strcpy(s_abup_info.abup_info_device_type, (char *)abup_device_type);
}

//设置后台项目信息里的platform
void abup_set_platform(adups_uint8 * abup_platform)
{
	strcpy(s_abup_info.abup_info_platform, (char *)abup_platform);
}

//设置版本号
void abup_set_version(adups_uint8 * abup_version)
{
	strcpy(s_abup_info.abup_info_version, (char *)abup_version);
}

adups_int32 abup_get_download_max_retry()
{
	return ABUP_DOWNLOAD_MAX_RETRY;
}

adups_int32 abup_get_main_download_ticks()
{
	return ABUP_MAIN_DOWNLOAD_TIMER;
}

adups_int32 abup_get_udp_block_size(void)
{
	return ABUP_UDP_MAX_BLOCK_SIZE;
}

adups_int32 abup_get_soc_lifetime(void)
{
	return ABUP_SOC_LIFE_TIME;
}

adups_uint8 *abup_get_local_port(void)
{
	static adups_uint8 port[8];
	memset(port, 0, sizeof(port));
	sprintf((char *)port, "%d", ABUP_SOC_LOCAL_PORT);	
	ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_15, P_INFO, "port=%s",port);
	return port;
}

adups_uint8 *abup_get_server_host_and_port(void)
{
	static adups_uint8 host[50];
	memset(host, 0, sizeof(host));
	sprintf((char *)host, "%s", ABUP_SERVER_HOST);	
	ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_14, P_INFO, "host=%s",host);
	return host;
}

adups_int64 abup_get_device_info_int(adups_uint8 instance, adups_uint16 res_id)
{
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_13, P_INFO, 1, "abup_get_device_info_int->res_id=%d",res_id);
	switch (res_id)
	{
		case ABUP_RES_O_BATTERY_LEVEL:
			return 100;

		case ABUP_RES_O_MEMORY_FREE:
			return ABUP_MEMORY_TOTAL;

		case ABUP_RES_O_CURRENT_TIME:
			return abup_mktime();

		case ABUP_RES_O_BATTERY_STATUS:
			return 0;//0:normal 1:charging 2:charge complete 3: Damaged 4:Low Battery 5:Not Installed 6:  Unknown

		case ABUP_RES_O_MEMORY_TOTAL:
			return ABUP_MEMORY_TOTAL;

		case ABUP_R_TIME_STAMP:
			abup_contextp->utc_time = abup_mktime();
			return abup_contextp->utc_time;
		default:
			return 0;
	}
}

adups_uint8 *abup_get_device_info_string(adups_uint8 instance, adups_uint16 res_id)
{
	adups_uint8 *info;
	adups_uint8 tinfo[50];
	adups_uint32 len = 0;
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_12, P_INFO, 1, "abup_get_device_info_string-->resid=%d",res_id);
	switch (res_id)
	{
		case ABUP_RES_O_MANUFACTURER:
			return abup_get_manufacturer();

		case ABUP_RES_O_MODEL_NUMBER:
			return abup_get_model_number();

		case ABUP_RES_O_SERIAL_NUMBER:
			return ABUP_SERIAL_NUMBER;

		case ABUP_RES_O_FIRMWARE_VERSION:						
			return abup_get_firmware_version();

		case ABUP_RES_O_DEVICE_TYPE:
			return abup_get_device_type();

		case ABUP_RES_O_HARDWARE_VERSION:
			return ABUP_HW_VERSION;

		case ABUP_RES_O_SOFTWARE_VERSION:
			return ABUP_SW_VERSION;

		case ABUP_R_PLATFORM:
			return abup_get_platform();

		case ABUP_R_SIGN:
		{
			char *ptr = adups_hmd5_did_pid_psecret(abup_client_mid, (char *)abup_get_product_id(), (char *)abup_get_product_sec(), abup_contextp->utc_time);
			len = strlen(ptr);
			strcpy((char *)tinfo, ptr);
						
			ECOMM_STRING(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_10, P_INFO, "Firmware IMEI: %s",(uint8_t *)abup_client_mid);
			break;
		}
		case ABUP_R_SDK_VERSION:
			return ABUP_SDK_VERSION;

		case ABUP_R_APK_VERSION:
			return ABUP_APK_VERSION;

		case ABUP_R_NETWORK_TYPE:
			return ABUP_NETWORK_TYPE;

		case ABUP_R_PRODUCT_ID:
			return abup_get_product_id();

		case ABUP_R_DEVICE_ID:
			return abup_contextp->device_id;

		default:
			break;
	}

	if (len > 0)
	{
		info = malloc(len + 1);
		if (info)
		{
			strcpy((char *)info, (char *)tinfo);
			return info;
		}
	}
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_9, P_INFO, 1, "abup_get_device_info_string-->resid:%d malloc error",res_id);
	return NULL;
}


void adups_close_fota(void)
{
	abup_g_quit = 1;
}

ADUPS_BOOL adups_delta_check_md5(adups_uint32 file_size, adups_uint8 *file_md5)
{
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_3, P_INFO, 2, "adups_delta_check_md5-->file_size=%d,file_md5=%d",file_size,strlen((char *)file_md5));
	if (file_size == 0 || file_size > abup_get_delta_len())
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_2, P_INFO, 0, "adups_delta_check_md5-->file size error");
		return ADUPS_FALSE;
	}
	if (strlen((char *)file_md5) == 0 || strlen((char *)file_md5) > 32)
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_CONFIG_1, P_INFO, 0, "adups_delta_check_md5-->file md5 error");
		return ADUPS_FALSE;
	}
	return ADUPS_TRUE;
}


