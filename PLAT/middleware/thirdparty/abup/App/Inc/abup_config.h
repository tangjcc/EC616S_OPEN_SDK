#ifndef _ADUPS_CONFIG_H
#define _ADUPS_CONFIG_H

#include "abup_typedef.h"
#include "mem_map.h"
#include "abup_fw_ver.h"
#ifdef __cplusplus
extern "C" {
#endif

//download
#define ABUP_DOWNLOAD_MAX_RETRY		3
#define ABUP_MAIN_DOWNLOAD_TIMER	40000
#define ABUP_UDP_MAX_BLOCK_SIZE		512
#define ABUP_MAX_PACKET_SIZE 		1400
//device





#define ABUP_MANUFACTURER      		"nb_oem"   			//OEM
#define ABUP_MODEL_NUMBER      		"101"			//…Ë±∏–Õ∫≈
#define ABUP_SERIAL_NUMBER     		"345000123"

#define ABUP_PRODUCT_ID 			"1571402421"
#define ABUP_PRODUCT_SEC 			"696ccc2beabe47dc99cf730e0ae94ffc"
#define ABUP_DEVICE_TYPE 			"bracelet"
#define ABUP_PLATFORM 				"MT2625"

#define ABUP_SDK_VERSION 			"IOT4.0_R40232"
#define ABUP_APK_VERSION 			"ADUPS_V4.0"

#ifndef ABUP_FIRMWARE_VERSION
#define ABUP_FIRMWARE_VERSION  		"V1.0"
#endif

#define ABUP_HW_VERSION      		"HW01"
#define ABUP_SW_VERSION    	 		"SW01"

#define ABUP_NETWORK_TYPE			"NB"
#define ABUP_MEMORY_TOTAL			FLASH_FOTA_REGION_END - FLASH_FOTA_REGION_START

//connection
#define ABUP_SOC_LIFE_TIME			2000
#define ABUP_SOC_LOCAL_PORT			56830
#define ABUP_SERVER_HOST			"iotlwm2m.abupdate.com:5683"  //"103.40.232.190:5683"  //"iot-lwm2m-test.abupdate.com:5683"	  //103.40.232.187	//172.17.2.254	 172.17.1.215

extern adups_int32 abup_get_main_download_ticks(void);
extern adups_int32 abup_get_download_max_retry(void);
extern adups_int32 abup_get_udp_block_size(void);
extern adups_uint8* abup_get_local_port(void);
extern adups_uint8* abup_get_server_host_and_port(void);
extern adups_uint8* abup_get_device_info_string(adups_uint8 instance,adups_uint16 res_id);
extern adups_int64 abup_get_device_info_int(adups_uint8 instance,adups_uint16 res_id);
extern adups_uint8 *abup_get_manufacturer(void);
extern adups_uint8 *abup_get_firmware_version(void);

//#define ABUP_UPDATE_SUC_REBOOT_TEST 
#ifdef __cplusplus
}
#endif

#endif

