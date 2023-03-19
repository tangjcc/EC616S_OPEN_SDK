#ifndef _ABUP_FIREWARE_H_
#define _ABUP_FIREWARE_H_
#include "FreeRTOS.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
#include "abup_typedef.h"
#include "abup_stdlib.h"
#include "timers.h"
#ifdef ABUP_WITH_TINYDTLS
#include "shared/dtlsconnection.h"
//#include "shared/dlts_connect.h"
#else
#include "shared/connection.h"
#endif


typedef struct
{
    uint8_t state;
    bool supported;
    uint8_t result;
} firmware_data_t;

typedef enum
{
	ABUP_DOWNLOAD_INIT,
	ABUP_DOWNLOAD_RU,
	ABUP_DOWNLOAD_CV,
	ABUP_DOWNLOAD_ING,
	ABUP_DOWNLOAD_END
}ABUP_DOWNLOAD_STATE;


typedef struct
{
	firmware_data_t* 		firmware_data; 
	ABUP_DOWNLOAD_STATE 	state;							//FOTA STATE	
	adups_char 			device_id[50];
	adups_char 			down_host[50];
	adups_uint8 			down_uri[200];
#ifdef ABUP_WITH_TINYDTLS
	dtls_connection_t * 	download_session;
#else
	connection_t * 	download_session;
#endif
	adups_uint8 		 	download_session_retry_num;
	adups_uint32 			file_size;
	adups_uint32 			delta_id;
	adups_uint8 			file_md5[33];
	adups_uint16 			gcurrudpblock;
	TimerHandle_t 			xmaindownloadTimer;
	TimerHandle_t			wakeupTimer;
	TimerHandle_t 			xdownloadTimer;
	adups_uint8 			retry_num; 						// overtime
	adups_uint8 			data_err_retry_num; 			// data error
	adups_uint8 			retry_rucv_num;
	adups_uint8 			send_download_result_retry_num; // download report
	ADUPS_BOOL  			send_download_result;
	adups_uint16			msg_id;
	adups_uint32 			utc_time;
	adups_uint32 			start_time;
	adups_uint32 			end_time;
	adups_uint32 			status;
	adups_uint32 			err_code;
	adups_uint8             xProcessNum;
} abup_firmware_data_t;


void abup_main_download_timer_reset(void);
void abup_main_download_timer_stop(void);
void abup_downloadTimer_callback(TimerHandle_t expiredTimer);
void abup_main_download_timer_callback(TimerHandle_t expiredTimer);
void abup_get_firmware_package(adups_uint32 offset);
void abup_creat_main_download_timer(void);
void abup_main_download_timeout_handler(void);
void abup_download_init_flash(void);

#endif
