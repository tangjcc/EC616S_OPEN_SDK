#ifndef _CTIOT_FOTA_H
#define _CTIOT_FOTA_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "ctiot_fota.h"
#include "ctiot_parseuri.h"
#include "er-coap-13.h"
#ifdef WITH_MBEDTLS
#include "../../src/port/dtlsconnection.h"
#else
#include "../../src/port/connection.h"
#endif
#include "ct_liblwm2m.h"
#include "ctiot_lwm2m_sdk.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#endif


#include "lfs.h"
#include "timers.h"
#include "mem_map.h"

//#define FOTA_SERVER_IP "139.196.187.107"
#define FOTA_SERVER_IP "192.168.157.212"

#define FOTA_SERVER_PORT 5683
#define FOTA_PATH1 "file"
#define START_NUM  0
#define START_MORE 0
#define BLOCK_SIZE 512
#define MAX_DOWNLOAD_TIMES 5

//#define FOTA_PATH1 ".well-know"
//#define FOTA_PATH2 "core"

typedef enum
{
	FOTA_STATE_IDIL,
	FOTA_STATE_DOWNLOADING,
	FOTA_STATE_DOWNLOADED,
	FOTA_STATE_UPDATING,
}ctiotFotaState;
typedef struct
{
	char* package;
	char* packageUri;
	int packageLength;
	int packageUriLength;
}firmwareWritePara;

typedef enum
{
	FOTA_RESULT_INIT,                //0 init
	FOTA_RESULT_SUCCESS,             //1 success
	FOTA_RESULT_NOFREE,              //2 no space
	FOTA_RESULT_OVERFLOW,            //3 downloading overflow
	FOTA_RESULT_DISCONNECT,          //4 downloading disconnect
	FOTA_RESULT_CHECKFAIL,           //5 validate fail
	FOTA_RESULT_NOSUPPORT,           //6 unsupport package
	FOTA_RESULT_URIINVALID,          //7 invalid uri
	FOTA_RESULT_UPDATEFAIL,          //8 update fail
	FOTA_RESULT_PROTOCOLFAIL        //9 unsupport protocol
}ctiotFotaResult;

typedef struct
{
	ctiotFotaState fotaState;
	ctiotFotaResult fotaResult;
}ctiotFotaManage;

int fota_start(char* url);
int fota_resume(void);

void ctiot_funcv1_fota_state_changed(void);

#if defined(CTIOT_ABUP_FOTA_ENABLE)
int8_t ct_abup_erase_delata(void);
uint32_t ct_abup_erase_backup(void);
int32_t ct_abup_init_flash(void);
//uint32_t ct_abup_read_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len);
uint32_t ct_abup_write_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len);
uint32_t ct_abup_read_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len);
void ct_abup_check_upgrade_result(void);
uint8_t ct_abup_get_upgrade_result(void);
void ct_abup_system_reboot(void);
#endif
void notify_fotastate_con(lwm2m_transaction_t* transacP, void * message);

#endif

