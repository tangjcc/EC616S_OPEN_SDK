#include <cis_if_sys.h>
#include <cis_def.h>
#include <cis_internals.h>
#include <cis_config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cis_api.h>
#include <assert.h>
#include <stdlib.h> 
#include "ps_lib_api.h"
//EC add begin
/*
#include <malloc.h>
#include <time.h>
#include "win_timers.h" 
#include<process.h> 
#include<windows.h>
#include <memory.h>
#include <windows.h>
*/
//EC add 

#if CIS_ENABLE_CMIOT_OTA
	#include "std_object/std_object.h"
#endif

#include <cis_def.h>
#include <cis_api.h>
#include <cis_if_sys.h>
#include <cis_log.h>
#include <cis_list.h>
#include <cis_api.h>
#include <string.h>

#include "FreeRTOS.h"//EC add
#include "task.h"//EC add
#include "semphr.h"//EC add
#include "bsp.h"//EC add
#include "cmsis_os2.h"//EC add
#include "osasys.h"//EC add
#include "dtls_time.h"//EC add
#include "task.h"//EC add
//#include "pslibapi.h"//EC add
#include "ps_lib_api.h"//EC add
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "rng_ec616.h"
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "rng_ec616s.h"
#include "flash_ec616s_rt.h"
#endif
#include "timers.h"
//#include "at_onenet_task.h"//EC add

//define version type like 001.000
#define EC_VERSION_TYPE

//static char cissys_imsi[20] = {0};//EC add
//static char cissys_imei[20] = {0};//EC add
static uint8_t psmMode = 0;//EC add
static uint32_t tauTimeS = 0;//EC add
static uint32_t activeTimeS = 0;//EC add
static uint8_t actType = 0;//EC add
static uint32_t nwEdrxValueMs = 0;//EC add
static uint32_t nwPtwMs = 0;//EC add
static uint8_t  psmRecoverRequire = 0;//EC add
static uint8_t  edrxRecoverRequire = 0;//EC add

static uint8_t coapAckTimeout = COAP_RESPONSE_TIMEOUT;
static uint32_t coapMaxTransmitWaitTime = COAP_MAX_TRANSMIT_WAIT;
static uint32_t coapMinTransmitWaitTime = COAP_MIN_TRANSMIT_WAIT;

#if defined(CIS_ENABLE_UPDATE_EC_ENABLE)
#define FOTA_NO_TEST_THREAD

#define FOTA_DL_4K_FLASH_BLOCK               (1024 << 2)
#define FOTA_DL_32K_FLASH_BLOCK               (1024 *32)

#define FOTA_RESERVED_BASE               (FLASH_FOTA_REGION_START)
#define FOTA_RESERVED_LENGTH             (FLASH_FOTA_REGION_LEN)
#define FOTA_RESERVED_END                (FOTA_RESERVED_BASE+FLASH_FOTA_REGION_LEN)

#endif
#ifdef FOTA_NO_TEST_THREAD
#else
void testThread(void* arg);
static TaskHandle_t tTestTaskhandle = NULL;
#endif

#if CIS_ENABLE_CMIOT_OTA
#define CIS_CMIOT_OTA_WAIT_UPLINK_FINISHED    1000
#define CIS_CMIOT_OTA_START_QUERY_INTERVAL    10000
#define CIS_CMIOT_OTA_START_QUERY_LOOP   	  6
#define CIS_CMIOT_NEWIMSI_STR   	"460040983303979"
extern uint8_t    cissys_changeIMSI(char* buffer,uint32_t len);


void CmiotOtaMonitor(void* arg);	//EC mod
uint8_t std_poweruplog_ota_notify(st_context_t * contextP,cis_oid_t * objectid,uint16_t mid);
uint8_t    cissys_remove_psm(void);
cis_ota_history_state    cissys_quary_ota_finish_state(void);
uint8_t    cissys_change_ota_finish_state(uint8_t flag);
uint8_t    cissys_recover_psm(void);


//extern cis_ota_history_state g_cmiot_otafinishstate_flag;
void*      g_cmiot_ota_context = NULL;//EC add

void CmiotOtaMonitor(void* arg)//EC mod
{
	//cis_oid_t test_obj_id = CIS_POWERUPLOG_OBJECT_ID;
	cis_cmiot_ota_state  monitor_ota_state = CMIOT_OTA_STATE_IDIL;
	cissys_assert(g_cmiot_ota_context !=NULL);
    st_context_t* ctx = (st_context_t*)g_cmiot_ota_context;	
	poweruplog_data_t * targetP;
	char * NewImsiStr;
    bool bExitFlag = false;//EC add
    
    ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_0, P_WARNING, 0, "CmiotOtaMonitor task created.");

	//cissys_callback_t *cb=(cissys_callback_t *)lpParam;
	while(bExitFlag == false)
	{	
		//osDelay(5000);
		uint8_t ota_queri_loop;
		bool ota_imsi_change_stat = FALSE;
		monitor_ota_state = ctx->cmiotOtaState ;
		uint8_t len_imsi =0;
		switch(monitor_ota_state)	
		{
			case CMIOT_OTA_STATE_IDIL:
			{
				osDelay(5000);
			}
			break;

			case CMIOT_OTA_STATE_START:
			{		
				targetP = (poweruplog_data_t *)(std_object_get_poweruplog(ctx, 0)); 				
				if (NULL != targetP)
				{				
					LOGD("UE is trigged to OTA start state and waiting the OTA procedule finished. \r\n");
					LOGD("OTA started with current IMSI: %s . \r\n",targetP->IMSI);	
                    ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_1, P_INFO, 0, "UE is trigged to OTA start state and waiting the OTA procedule finished");
                    ECOMM_STRING(UNILOG_ONENET, CmiotOtaMonitor_2, P_INFO, "OTA started with current IMSI: %s", (uint8_t *)targetP->IMSI);
					NewImsiStr= (char*)cis_malloc(NBSYS_IMSI_MAXLENGTH);
					if(NewImsiStr == NULL)
					{
						cis_free(NewImsiStr);
					}	
    				cis_memset(NewImsiStr, 0, NBSYS_IMSI_MAXLENGTH);			

					osDelay(CIS_CMIOT_OTA_WAIT_UPLINK_FINISHED);	
                    ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_3, P_WARNING, 0, "SIM OTA start, unregister from onenet");
					cis_unregister(ctx);	

					cissys_remove_psm();
				
					
					osDelay(CIS_CMIOT_OTA_START_QUERY_INTERVAL);	
					
					for(ota_queri_loop=0;ota_queri_loop<CIS_CMIOT_OTA_START_QUERY_LOOP;ota_queri_loop++)
					{
						osDelay(CIS_CMIOT_OTA_START_QUERY_INTERVAL);
						cis_memset(NewImsiStr, 0, NBSYS_IMSI_MAXLENGTH);						
						len_imsi = cissys_getIMSI(NewImsiStr,NBSYS_IMSI_MAXLENGTH, true);

						if(cis_memcmp(targetP->IMSI,NewImsiStr,len_imsi)!=0)
						{
							ota_imsi_change_stat = TRUE;
							cis_memset(targetP->IMSI, 0, NBSYS_IMSI_MAXLENGTH);
							cis_memcpy(targetP->IMSI,NewImsiStr,len_imsi);					
							break;
						}
						//change the IMSI for test
						if(ota_queri_loop == 3)
						{
							//cis_memcpy(NewImsiStr,CIS_CMIOT_NEWIMSI_STR,NBSYS_IMSI_MAXLENGTH);	
							//cissys_changeIMSI(NewImsiStr,NBSYS_IMSI_MAXLENGTH);
						}
						
					}
					if(ota_imsi_change_stat == TRUE)
					{
						core_callbackEvent(ctx,CIS_EVENT_CMIOT_OTA_SUCCESS,NULL);
						LOGD("OTA success detected. \r\n");
                        ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_4, P_INFO, 0, "OTA success detected.");
	
					}
					else
					{
						core_callbackEvent(ctx,CIS_EVENT_CMIOT_OTA_FAIL,NULL);
						LOGD("OTA fail detected. \r\n");
                        ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_5, P_ERROR, 0, "OTA fail detected.");
					}
					LOGD("OTA finished with current IMSI: %s . \r\n",targetP->IMSI);
					cis_free(NewImsiStr);
				}
				
				ctx->registerEnabled = true;
				LOGD("Reregister to ONENET to update OTA result. \r\n");	
                ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_6, P_WARNING, 0, "Reregister to ONENET to update OTA result.");

			}
			break;		

			case CMIOT_OTA_STATE_SUCCESS:
			case CMIOT_OTA_STATE_FAIL:
			{
				
			}
			break;

			case CMIOT_OTA_STATE_FINISH:
			{
				cis_unregister(g_cmiot_ota_context);
				LOGD("OTA Finish detected.Unresgister from OneNET \r\n");
                ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_7, P_WARNING, 0, "OTA Finish detected.Unresgister from OneNET.");
                bExitFlag = true;
                //return;
			}
			break;

			default:
				osDelay(5000);				
			
		}
        if (g_cmiot_ota_context == PNULL)
        {
            ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_8, P_WARNING, 0, "onenet simota cis_context is NULL, CmiotOtaMonitor will exit");
            bExitFlag = true;
        }
		//std_poweruplog_ota_notify((st_context_t *)g_context,&test_obj_id, cmiot_ota_observe_mid);
	}
    
    ECOMM_TRACE(UNILOG_ONENET, CmiotOtaMonitor_9, P_WARNING, 0, "CmiotOtaMonitor task delete.");
    vTaskDelete(NULL);
}

#endif//CIS_ENABLE_CMIOT_OTA


//static uint32_t startTime = 0;
cis_ret_t cissys_init(void *context, const cis_cfg_sys_t* cfg, cissys_callback_t* event_cb)
{
	struct st_cis_context * ctx = (struct st_cis_context *)context;
	cis_memcpy(&(ctx->g_sysconfig), cfg, sizeof(cis_cfg_sys_t));

#if CIS_ENABLE_CMIOT_OTA
	if (g_cmiot_ota_context !=NULL)
	{
		//hThread = CreateThread(NULL, 0, testCmiotOta, NULL, 0, &dwThread);
		//CloseHandle(hThread);
        xTaskCreate(CmiotOtaMonitor,
                     "CmiotOtaMonitor",
                     2048 / sizeof(portSTACK_TYPE),
                     NULL,
                     osPriorityBelowNormal7,
                     NULL);
	}
#endif//CIS_ENABLE_CMIOT_OTA

#if CIS_ENABLE_UPDATE
	if (event_cb != NULL) {
		ctx->g_fotacallback.onEvent = event_cb->onEvent;
		ctx->g_fotacallback.userData = event_cb->userData;
#if CIS_ENABLE_DM
		if (TRUE != ctx->isDM)
#endif
		{
		    
			//hThread = CreateThread(NULL, 0, testThread, (LPVOID)&(ctx->g_fotacallback), 0, &dwThread);
			//CloseHandle(hThread);
			#ifdef FOTA_NO_TEST_THREAD
            #else
            xTaskCreate(testThread,
                         "testThread",
                         2048 / sizeof(portSTACK_TYPE),
                         (void*)&(ctx->g_fotacallback),
                         osPriorityBelowNormal7,
                         &tTestTaskhandle);

            #endif
		}
	}
#endif
	return 1;
}

//static uint32_t startTime = 0;
cis_ret_t cissys_deinit(void *context)
{
	struct st_cis_context * ctx = (struct st_cis_context *)context;

#if CIS_ENABLE_CMIOT_OTA

#endif//CIS_ENABLE_CMIOT_OTA

#if CIS_ENABLE_UPDATE

#if CIS_ENABLE_DM
	if (TRUE != ctx->isDM)
#endif

	{
        #ifdef FOTA_NO_TEST_THREAD
        #else
	
	    if (tTestTaskhandle)
        {
            vTaskDelete(tTestTaskhandle);     
            tTestTaskhandle = NULL;
        }
        #endif
	}
#endif
	return 1;
}

uint32_t cissys_gettime()
{
    uint32_t time = 0;
    time = osKernelGetTickCount()/portTICK_PERIOD_MS;
    return time;
}

void    cissys_sleepms(uint32_t ms)
{
    osDelay(ms / portTICK_PERIOD_MS);
}


clock_time_t cissys_tick(void)
{
    UINT32 current_ms = 0;
    current_ms = osKernelGetTickCount()/portTICK_PERIOD_MS;
    return (clock_time_t)current_ms;
}


void    cissys_logwrite(uint8_t* buffer,uint32_t length)
{
    if (UsartPrintHandle != NULL)
    {
        UsartPrintHandle->Send((CHAR *)buffer, length);
    }
}

void*     cissys_malloc(size_t length)
{
    return pvPortMalloc(length);
}

void    cissys_free(void* buffer)
{
    vPortFree(buffer);
}


void* cissys_memset( void* s, int c, size_t n)
{
	return memset( s, c, n);
}

void* cissys_memcpy(void* dst, const void* src, size_t n)
{
	return memcpy( dst, src, n);
}

void* cissys_memmove(void* dst, const void* src, size_t n)
{
	return memmove( dst, src, n);
}

int cissys_memcmp( const void* s1, const void* s2, size_t n)
{
	return memcmp( s1, s2, n);
}

void    cissys_fault(uint16_t id)
{
    LOGE("fall in cissys_fault.id=%d\r\n",id);
    configASSERT(0);
}


//void cissys_assert(bool flag)
//{
//    configASSERT(flag);
//}


uint32_t cissys_rand()
{
    uint32_t random=0;
    uint32_t a=0;
    uint32_t b=0;
    uint8_t i=0;
    uint8_t Rand[24];
    RngGenRandom(Rand);
    for(i=0; i<24; i++){
        a += Rand[i];
        b ^= Rand[i];
    }
    random = (a<<8) + b;
    //ECOMM_TRACE(UNILOG_ONENET, cissys_rand_1, P_SIG, 1, "cissys_rand:%d",random);
    return random;
}

// Dynamic change COAP_RESPONSE_TIMEOUT/COAP_MAX_TRANSMIT_WAIT/COAP_MIN_TRANSMIT_WAIT according to signal ce level
void cissys_setCoapAckTimeout(uint8_t celevel)
{
    switch (celevel)
    {
        case 1://Coverage Enhancement level 0
        {
            coapAckTimeout = 2;
            break;
        }
        case 2: //Coverage Enhancement level 1
        {
            coapAckTimeout = 6;
            break;
        }
        case 3: //Coverage Enhancement level 2
        {
            coapAckTimeout = 10;
            break;
        }
        default:
        {
            coapAckTimeout = 4;
            break;
        }
    }
    return;
}

void cissys_setCoapAckTimeoutDirect(uint8_t timeout)
{
	coapAckTimeout = timeout;
}

uint8_t cissys_getCoapAckTimeout(void)
{
    return coapAckTimeout;
}

void cissys_setCoapMaxTransmitWaitTime()
{
    coapMaxTransmitWaitTime = ((coapAckTimeout * ( (1 << (COAP_MAX_RETRANSMIT + 1) ) - 1) * COAP_ACK_RANDOM_FACTOR));
    ECOMM_TRACE(UNILOG_ONENET, cissys_setCoapMaxTransmitWaitTime, P_INFO, 2, "coapAckTimeout = %d, set coapMaxTransmitWaitTime=%d ", coapAckTimeout,coapMaxTransmitWaitTime);
    return;
}

uint32_t cissys_getCoapMaxTransmitWaitTime()
{
    return coapMaxTransmitWaitTime;
}

void cissys_setCoapMinTransmitWaitTime()
{
    coapMinTransmitWaitTime = (coapAckTimeout * ( (1 << (COAP_MAX_RETRANSMIT + 1) ) - 1));
    return;
}

uint32_t cissys_getCoapMinTransmitWaitTime()
{
    return coapMinTransmitWaitTime;
}


uint8_t cissys_getIMEI(char* buffer,uint32_t maxlen)
{
    uint8_t len = 0;
    char cissys_imei[20] = {0};

    appGetImeiNumSync(cissys_imei);
    len = strlen(cissys_imei);

    if(maxlen < len)return 0;
    memcpy(buffer, cissys_imei, len);

    return len;
}
uint8_t cissys_getIMSI(char* buffer,uint32_t maxlen, bool usingReal)
{
    uint8_t len = 0;
    char *str = "imsiwb1";
    char cissys_imsi[20] = {0};

    if (usingReal)
    {
        // not mix to use with AT mode. just for opencpu or called in non psproxytask context
        appGetImsiNumSync(cissys_imsi);
		ECOMM_STRING(UNILOG_ONENET, cissys_getIMSI_0, P_INFO, "Get real IMSI: %s ", (uint8_t *)cissys_imsi);
        len = strlen(cissys_imsi);
        if(maxlen < len)
            return 0;
        memcpy(buffer, cissys_imsi, len);
    }
    else
    {
        len = strlen(str);
        memcpy(buffer, str, len);
    }
    return len;
}

#if CIS_ENABLE_CMIOT_OTA

uint8_t    cissys_changeIMSI(char* buffer,uint32_t len)
{
    FILE* f = fopen("imsi","wb");
    if(f != NULL)
    {
        fwrite(buffer,1,len,f);
        fclose(f);
        return true;
    }
    return false;
}


uint8_t    cissys_getEID(char* buffer,uint32_t maxlen)
{
    NBStatus_t result = NBStatusSuccess;
    CrsmCmdParam cmdParam;
    CrsmRspParam rspParam;
    uint8_t len = 0;

    memset(&cmdParam, 0, sizeof(CrsmCmdParam));
    memset(&rspParam, 0, sizeof(CrsmRspParam));

    //READ EFeid: AT+CRSM=176,12034,0,0 
    cmdParam.command = 0xB0;//INS: READ BINARY
    cmdParam.fileId = 0x2F02;//eid file Id
    cmdParam.p1 = 0;
    cmdParam.p2 = 0;
    
    result = appSetRestrictedSimAccessSync(&cmdParam, &rspParam);

    if (result == NBStatusSuccess)
    {
        if (rspParam.respLen > maxlen)
        {
            rspParam.respLen = maxlen;
        }

        //return normal status words: 9000
        if ((rspParam.sw1 == 0x90) && (rspParam.sw2 == 0x00))
        {
            len = rspParam.respLen;
            memcpy(buffer, rspParam.responseData, rspParam.respLen);
            ECOMM_STRING(UNILOG_ONENET, cissys_getEID_0, P_INFO, "OTA get EID: %s ", rspParam.responseData)
        }
    }

    return len;
}	

uint8_t    cissys_remove_psm(void)
{
    NBStatus_t result = NBStatusSuccess;
        
    result = appGetPSMSettingSync((UINT8 *)&psmMode, (UINT32 *)&tauTimeS, (UINT32 *)&activeTimeS);
    if (result == NBStatusSuccess) 
    {
        if (psmMode == 1)
        {
            result = appSetPSMSettingSync(0, (UINT32)tauTimeS, (UINT32)activeTimeS);
            
            if (result == NBStatusSuccess)
            {
                psmRecoverRequire = 1;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ONENET, cissys_remove_psm_0, P_WARNING, 1, "disable PSM mode failed: %d.", result);
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ONENET, cissys_remove_psm_1, P_INFO, 0, "PSM mode was already disabled");
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_ONENET, cissys_remove_psm_2, P_WARNING, 1, "GET PSM mode failed: %d.", result);
    }

    if (psmRecoverRequire == 1)
    {
        result = appGetEDRXSettingSync((UINT8 *)&actType, (UINT32 *)&nwEdrxValueMs, (UINT32 *)&nwPtwMs);
        if (result == NBStatusSuccess)
        {
            result = appSetEDRXSettingSync(0, (UINT8)actType, 0);
            if (result == NBStatusSuccess)
            {
                edrxRecoverRequire = 1;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ONENET, cissys_remove_psm_3, P_WARNING, 1, "disable edex failed: %d.", result);
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ONENET, cissys_remove_psm_4, P_WARNING, 1, "get edex failed: %d.", result);
        }
    }

    return 1;
}	

uint8_t cissys_recover_psm(void)
{
    NBStatus_t result = NBStatusSuccess;

    if (psmRecoverRequire == 0)
        return 1;
    
    result = appSetPSMSettingSync(1, (UINT32)tauTimeS, (UINT32)activeTimeS);
    if (result == NBStatusSuccess) 
    {
        psmRecoverRequire = 0;
        tauTimeS = 0;
        activeTimeS= 0;
        
        ECOMM_TRACE(UNILOG_ONENET, cissys_recover_psm_0, P_INFO, 0, "Recover PSM ok.");
    }
    else
    {
        ECOMM_TRACE(UNILOG_ONENET, cissys_recover_psm_1, P_WARNING, 1, "enable PSM mode failed: %d.", result);
    }

    if (edrxRecoverRequire == 0)
        return 1;
    
    result = appSetEDRXSettingSync(1, (UINT8)actType, (UINT32)nwEdrxValueMs);    
    if (result == NBStatusSuccess)
    {
        edrxRecoverRequire = 0;
        nwEdrxValueMs = 0;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ONENET, cissys_recover_psm_2, P_WARNING, 1, "enable edrx failed: %d.", result);
    }

    return 1;
}	

cis_ota_history_state    cissys_quary_ota_finish_state(void)
{
    //return g_cmiot_otafinishstate_flag;
    return OTA_HISTORY_STATE_FINISHED;//(cis_ota_history_state)gOnenetSimOtaCtx.otaFinishState;
}	
uint8_t    cissys_change_ota_finish_state(cis_ota_history_state flag)
{
	//g_cmiot_otafinishstate_flag = flag;
    return 1;
}	
#endif

void cissys_lockcreate(void** mutex)
{
    SemaphoreHandle_t mutex_handle;
    mutex_handle = xSemaphoreCreateMutex();
    (*mutex) = mutex_handle;
}

void cissys_lockdestory(void* mutex)
{
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
}

cis_ret_t   cissys_lock(void* mutex,uint32_t ms)
{
    //assert(mutex != NULL);
    xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY);
    return CIS_RET_OK;
}

void cissys_unlock(void* mutex)
{
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}

bool cissys_save(uint8_t* buffer,uint32_t length)
{
#if 0
    FILE* f = NULL;
    f = fopen("d:\\cis_serialize.bin","wb");
    if(f != NULL)
    {
        fwrite(buffer,1,length,f);
        fclose(f);
        return true;
    }
    return false;
#endif
    return true;
}

bool cissys_load(uint8_t* buffer,uint32_t length)
{
#if 0
    uint32_t readlen;
    FILE* f = fopen("d:\\cis_serialize.bin","rb");
    if(f != NULL)
    {
        while(length)
        {
            readlen = fread(buffer,1,length,f);
            if(readlen == 0)
            {
                break;
            }
            length -=readlen;
        }
        if(length == 0)
        {
            return true;
        }
    }
    return false;
#endif
    return true;
}


char VERSION[16]="V000.000.000";

#define VERSION_MAGIC_LOC 0
#define VERSION_DATA_LOC 32
uint32_t    cissys_getFwVersion(uint8_t** version)
{
	int length = strlen(VERSION)+1;

	cis_memcpy(*version,VERSION,length);

	#ifdef CIS_DEBUG_MOD_ONENET
	ECOMM_STRING(UNILOG_ONENET, cissys_getFwVersion_1, P_INFO, "CIS VERSION: %s", (uint8_t*)VERSION);
	#endif
    
	return length;
}

#define DEFAULT_CELL_ID (95)
#define DEFAULT_RADIO_SIGNAL_STRENGTH (99)

uint32_t    cissys_getCellId()
{
	uint32_t cellID = DEFAULT_CELL_ID;
	
	CmiDevGetExtStatusCnf   ueExtStatusInfo;
	memset(&ueExtStatusInfo, 0, sizeof(CmiDevGetExtStatusCnf));

	if(appGetUeExtStatusInfoSync(UE_EXT_STATUS_ERRC, &ueExtStatusInfo) == CMS_RET_SUCC)
	{
		cellID = ueExtStatusInfo.rrcStatus.cellId;
	}
	
	ECOMM_TRACE(UNILOG_ONENET, cissys_getCellId_1, P_INFO, 1, "OTA: Cell ID = %d", cellID);
	return cellID;
}

uint32_t    cissys_getRadioSignalStrength()
{
	uint32_t ret = DEFAULT_RADIO_SIGNAL_STRENGTH;
	INT8 snr = 0;
	INT8 rsrp = 0;
	UINT8 csq = 0;
	
	if(appGetSignalInfoSync(&csq, &snr, &rsrp) == CMS_RET_SUCC)
	{
		ret = csq;
	}

	ECOMM_TRACE(UNILOG_ONENET, cissys_getRadioSignalStrength_1, P_INFO, 1, "OTA: Signal Strength = %d", ret);

	return ret;

}



#if CIS_ENABLE_UPDATE

#define DEFAULT_BATTERY_LEVEL (99)
#define DEFAULT_BATTERY_VOLTAGE (3800)
#define DEFAULT_FREE_MEMORY   	((FOTA_RESERVED_LENGTH - FOTA_DL_32K_FLASH_BLOCK)/1024)

#if CIS_ENABLE_UPDATE_MCU
bool isupdatemcu = false;
#endif


int LENGTH = 0;
int RESULT = 0;
int STATE = 0;
int ERASED_LENGTH = 0;

#if 1
int writeCallback = 0;
int validateCallback = 0;
int eraseCallback = 0;
void testThread(void* arg)
{
	cissys_callback_t *cb=(cissys_callback_t *)arg;
	while(1)
	{			
		if(writeCallback == 1) //write callback
		{
			cissys_sleepms(2000);
			//FILE* f = NULL;
			//int i = 0;
			//OutputDebugString("cissys_writeFwBytes\n");

			//osDelay(100);
			//f = fopen("cis_serialize_package.bin","a+b");
			//fseek(f, 0, SEEK_END);
			//if(f != NULL)
			//{
			// fwrite(buffer,1,size,f);
			// fclose(f);
			//return true;
			//}
			writeCallback = 0;
			cb->onEvent(cissys_event_write_success,NULL,cb->userData,NULL);
			//cb->onEvent(cissys_event_write_fail,NULL,cb->userData,NULL);
			
		}
		else if(eraseCallback == 1) //erase callback
		{
			cissys_sleepms(5000);
			eraseCallback = 0;
			//cb->onEvent(cissys_event_fw_erase_fail,0,cb->userData,NULL);
			cb->onEvent(cissys_event_fw_erase_success,0,cb->userData,NULL);
		}
		else if(validateCallback == 1)//validate
		{
			cissys_sleepms(3000);
			validateCallback = 0;
			//cb->onEvent(cissys_event_fw_validate_fail,0,cb->userData,NULL);
			cb->onEvent(cissys_event_fw_validate_success,0,cb->userData,NULL);
		}
		/*else if(nThreadNo == 3)
		{
			osDelay(3000);
			g_syscallback.onEvent(cissys_event_fw_update_success,0,g_syscallback.userData,NULL);
			nThreadNo = -1;
		}*/
		else
		{
		cissys_sleepms(2000);
		}


	}
}

#endif


uint32_t    cissys_getFwBatteryLevel()
{
	return DEFAULT_BATTERY_LEVEL;
}

uint32_t    cissys_getFwBatteryVoltage()
{
	return DEFAULT_BATTERY_VOLTAGE;
}
uint32_t    cissys_getFwAvailableMemory()
{
	return DEFAULT_FREE_MEMORY;
}

int    cissys_getFwSavedBytes()
{
	//FILE * pFile;
	//long size = 0;
	//pFile = fopen ("F:\\cissys_writeFwBytes.bin","rb");
	//if (pFile==NULL)
	//	perror ("Error opening file");
	//else
	//{
	//	fseek (pFile, 0, SEEK_END);
	//	size=ftell (pFile); 
	//	fclose (pFile);
	//}
	//return size;
	return LENGTH;
}

bool    cissys_checkFwValidation(cissys_callback_t *cb)
{
	validateCallback = 1;
    
    #ifdef FOTA_NO_TEST_THREAD
	//cissys_sleepms(1000);
	cb->onEvent(cissys_event_fw_validate_success,0,cb->userData,NULL);
    #endif
    
	return true;
}



bool        cissys_eraseFwFlash (cissys_callback_t *cb)
{
	#ifdef CIS_DEBUG_MOD_ONENET
	ECOMM_TRACE(UNILOG_ONENET, erase_flash_0, P_INFO, 0, "cissys_eraseFwFlash" );
	#endif    
	
	eraseCallback = 1;
    
	//cissys_sleepms(8000);
	//LENGTH=0;
	#ifdef FOTA_NO_TEST_THREAD
        cb->onEvent(cissys_event_fw_erase_success,0,cb->userData,NULL);
    #endif
    
	return true;
}

void cissys_ClearFwBytes(void)
{
	LENGTH = 0;
    #ifdef CIS_ENABLE_UPDATE_EC_ENABLE    
    ERASED_LENGTH = 0;
	
    #ifdef CIS_DEBUG_MOD_ONENET
    ECOMM_TRACE(UNILOG_ONENET, cissys_ClearFwBytes_1, P_INFO, 0, "cissys_ClearFwBytes ");
    #endif
    
    #endif
}

uint32_t   cissys_writeFwBytes(uint32_t size, uint8_t* buffer, cissys_callback_t *cb)
{
    uint32_t tmp_len;
    uint32_t tmp_erase_size;
    uint8_t ret ;
	uint8_t *DldStatPtr = (uint8_t*)(FOTA_RESERVED_END-FOTA_DL_4K_FLASH_BLOCK+FLASH_XIP_ADDR);
    uint8_t DldStatStart[5] = {'A','D', 'U', 'P', 'S'};
	
    #ifdef CIS_ENABLE_UPDATE_EC_ENABLE
    
    #ifdef CIS_DEBUG_MOD_ONENET
    ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_0, P_INFO, 2, "write start, length %d, erased length %d", LENGTH, ERASED_LENGTH);
    #endif

	if (memcmp(&DldStatStart[0], DldStatPtr,sizeof(DldStatStart)))
	{
        ret = BSP_QSPI_Erase_Safe(FOTA_RESERVED_END - FOTA_DL_32K_FLASH_BLOCK, FOTA_DL_4K_FLASH_BLOCK);
        if (ret != 0)
        {
            #ifdef CIS_DEBUG_MOD_ONENET     
            ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_0_0, P_INFO, 0, "erase failed");
            #endif
            
            return 0;
        }	
		
		//mark downloading state
		ret = BSP_QSPI_Write_Safe(&DldStatStart[0], FOTA_RESERVED_END - FOTA_DL_32K_FLASH_BLOCK, sizeof(DldStatStart));
        if (ret != 0)
        {
            #ifdef CIS_DEBUG_MOD_ONENET     
            ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_0_1, P_INFO, 0, "write failed");
            #endif
            
            return 0;
        }	
	}
	
    if (size != 0)
    {
        #ifdef CIS_DEBUG_MOD_ONENET
        ECOMM_DUMP(UNILOG_ONENET, write_fwbytes_0_2, P_INFO, "write data:",16, (uint8_t*)buffer);
        #endif
    }
    
    tmp_len =  LENGTH + size;
    if ((tmp_len < FOTA_RESERVED_LENGTH - FOTA_DL_32K_FLASH_BLOCK) &&
        (tmp_len > ERASED_LENGTH))
    {
        tmp_erase_size = tmp_len - ERASED_LENGTH + FOTA_DL_4K_FLASH_BLOCK -1;
        tmp_erase_size = tmp_erase_size - (tmp_erase_size%FOTA_DL_4K_FLASH_BLOCK);
        ret = BSP_QSPI_Erase_Safe(FOTA_RESERVED_BASE + ERASED_LENGTH, tmp_erase_size);
        if (ret != 0)
        {
            #ifdef CIS_DEBUG_MOD_ONENET     
            ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_1, P_INFO, 1, "erase failed, size %d", size);
            #endif
            
            return 0;
        }
        ERASED_LENGTH += tmp_erase_size;
    }

    ret = BSP_QSPI_Write_Safe(buffer, FOTA_RESERVED_BASE+LENGTH, size);
    if (ret != 0)
    {
        #ifdef CIS_DEBUG_MOD_ONENET                 
        ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_2, P_INFO, 1, "write failed, size %d", size);
        #endif
        
        return 0;
    }        
    
    //LENGTH += size; //will do sync in cissys_event_write_success 
    cb->onEvent(cissys_event_write_success, NULL, cb->userData, (int *)&size);
    
    #else	
    ///writeCallback = 1;    
    cissys_sleepms(2000);
    cb->onEvent(cissys_event_write_success, NULL, cb->userData, (int *)&size);
    #endif    
    
    #ifdef CIS_DEBUG_MOD_ONENET                 
    ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_3, P_INFO, 1, "write success, size %d", size);
    ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_4, P_INFO, 2, "write success, length %d, erased length %d", LENGTH, ERASED_LENGTH);
    #endif
    
    return 1;
}


void cissys_savewritebypes(uint32_t size)
{
	LENGTH += size;
}

bool		cissys_updateFirmware (cissys_callback_t *cb)
{
	//nThreadNo = 3;
	#ifdef CIS_ENABLE_UPDATE_EC_ENABLE
    #ifndef EC_VERSION_TYPE     
        //for test only
       char tmpVERSION[16] = "2.2.1";
    


    int ret = 0;
       char versionFlag[] = "OK";
        ret = BSP_QSPI_Erase_Safe(FOTA_RESERVED_END-FOTA_DL_32K_FLASH_BLOCK, FOTA_DL_4K_FLASH_BLOCK);
    if (ret != 0)
    {
        ECOMM_TRACE(UNILOG_ONENET, cissys_updateFirmware_1, P_INFO, 0, "erase failed");
        return 0;
    }

        ret = BSP_QSPI_Write_Safe((uint8_t *)versionFlag, FOTA_RESERVED_END-FOTA_DL_32K_FLASH_BLOCK + VERSION_MAGIC_LOC, sizeof (versionFlag));
    if (ret != 0)
    {
        #ifdef CIS_DEBUG_MOD_ONENET                             
        ECOMM_TRACE(UNILOG_ONENET, cissys_updateFirmware_2, P_INFO, 1, "write failed, size %d", sizeof (versionFlag));
        #endif
        
        return 0;
    }              
    

        ret = BSP_QSPI_Write_Safe((uint8_t *)tmpVERSION, FOTA_RESERVED_END-FOTA_DL_32K_FLASH_BLOCK+VERSION_DATA_LOC, sizeof (tmpVERSION));
    if (ret != 0)
    {
        #ifdef CIS_DEBUG_MOD_ONENET                                         
        ECOMM_TRACE(UNILOG_ONENET, cissys_updateFirmware_3, P_INFO, 1, "write failed, size %d", sizeof (tmpVERSION));
        #endif
        
        return 0;
    }           
    #ifdef CIS_DEBUG_MOD_ONENET                                                 
    ECOMM_DUMP(UNILOG_ONENET, cissys_updateFirmware_4, P_INFO, "write data:",5, (uint8_t*)tmpVERSION);
    #endif

    cis_memcpy(VERSION, tmpVERSION,sizeof(tmpVERSION));

	#else
    int ret = 0;

	//clear downloading mark, bootloader deal it as full patch
	ret = BSP_QSPI_Erase_Safe(FOTA_RESERVED_END - FOTA_DL_32K_FLASH_BLOCK, FOTA_DL_4K_FLASH_BLOCK);
	if (ret != 0)
	{
		#ifdef CIS_DEBUG_MOD_ONENET 	
		ECOMM_TRACE(UNILOG_ONENET, write_fwbytes_0_0, P_INFO, 0, "erase failed");
		#endif
		
		return 0;
	}	

    #endif        
	
    #ifdef CIS_DEBUG_MOD_ONENET               
    ECOMM_TRACE(UNILOG_ONENET, cissys_updateFirmware_5, P_INFO, 0, "updating");
    #endif

    #endif
    
	return true;
}


void cis_SetCFUN(uint8_t fun)
{
	appSetCFUN(fun);
}


bool    cissys_readContext(cis_fw_context_t * context)
{
    #ifdef CIS_ENABLE_UPDATE_EC_ENABLE
    int length;
    char versionSucc[] = "OK";//for test only
    char versionFail[] = "NO";//for test only
    char versionFlag[2] = {0, 0};//read back
    
    #ifndef EC_VERSION_TYPE     
    char tmpVERSION[16];//version read back
    #endif
    int ret = 0;
    int flagExist = 0;

    #ifdef EC_VERSION_TYPE     
    cis_loadVersion();
	length = strlen(VERSION)+1;
    #endif

    do 
    {
        ret = BSP_QSPI_Read_Safe((uint8_t *)(&versionFlag[0]), FOTA_RESERVED_END-FOTA_DL_32K_FLASH_BLOCK + VERSION_MAGIC_LOC, sizeof (versionFlag));
        if (ret != 0)
        {
            #ifdef CIS_DEBUG_MOD_ONENET            
            ECOMM_TRACE(UNILOG_ONENET, cissys_readContext_2, P_INFO, 1, "read failed, size %d", sizeof (versionFlag));
            #endif
            
            RESULT = FOTA_RESULT_UPDATEFAIL;

        }              
        else
        {
            if (memcmp(versionFlag, versionSucc, sizeof(versionFlag)) ==0)
            {
                flagExist = 1;
            	#ifdef CIS_DEBUG_MOD_ONENET
                ECOMM_DUMP(UNILOG_ONENET, cissys_readContext_4, P_INFO, "Flag OK, SDK version:", length, (uint8_t*)VERSION);
				#endif
				
                #ifndef EC_VERSION_TYPE     

                ret = BSP_QSPI_Read_Safe((uint8_t *)(&tmpVERSION[0]), FOTA_RESERVED_END-FOTA_DL_32K_FLASH_BLOCK + VERSION_DATA_LOC, sizeof(tmpVERSION));
                if (ret != 0)
                {
                    #ifdef CIS_DEBUG_MOD_ONENET                                
                    ECOMM_TRACE(UNILOG_ONENET, cissys_readContext_3, P_INFO, 1, "read failed, size %d", sizeof(tmpVERSION));
                    #endif
                    
                    RESULT = FOTA_RESULT_UPDATEFAIL;
                    
                    break;
                }            

                //erase magic data
               //cis_memcpy(UpdateNewVERSION,tmpVERSION,sizeof(UpdateNewVERSION));


                RESULT = FOTA_RESULT_SUCCESS;
                //UpdateVerValid = 1;          	
                cis_memcpy(VERSION, tmpVERSION, length);
                
                #ifdef CIS_DEBUG_MOD_ONENET                                            
                ECOMM_DUMP(UNILOG_ONENET, cissys_readContext_5, P_INFO, "VERSION data:", sizeof(tmpVERSION), (uint8_t*)tmpVERSION);
                #endif             

                #else
                
                RESULT = FOTA_RESULT_SUCCESS;
                
                #ifdef CIS_DEBUG_MOD_ONENET                                                
                ECOMM_DUMP(UNILOG_ONENET, cissys_readContext_5, P_INFO, "VERSION data:", length, (uint8_t*)VERSION);    
                #endif
                
                #endif
    
            }
            else if (memcmp(versionFlag, versionFail, sizeof(versionFlag)) ==0)
            {
                flagExist = 1;
                RESULT = FOTA_RESULT_UPDATEFAIL;
                ECOMM_DUMP(UNILOG_ONENET, cissys_readContext_6, P_INFO, "Flag NO, SDK Version:", length, (uint8_t*)VERSION);    
            }
        }
    }while(0);
    if (flagExist == 1)
    {
        ret = BSP_QSPI_Erase_Safe(FOTA_RESERVED_END-FOTA_DL_32K_FLASH_BLOCK, FOTA_DL_4K_FLASH_BLOCK);
        if (ret != 0)
        {
            #ifdef CIS_DEBUG_MOD_ONENET                                
            ECOMM_TRACE(UNILOG_ONENET, cissys_readContext_7, P_INFO, 0, "erase failed");                
            #endif
            
            RESULT = FOTA_RESULT_UPDATEFAIL;
        }          
    }
    #endif
    context->result = RESULT;
    context->savebytes = LENGTH;
    context->state = STATE;
	/*uint32_t readlen;
	char buffer[10];
    FILE* f = fopen("f:\\cis_setFwUpdateResult.bin","rb");
    if(f != NULL)
    {
        fread(buffer,1,sizeof(buffer),f);
		sscanf(buffer,"%d",&(context->result));
    }
	fclose(f);

	f = fopen("f:\\cis_setFwState.bin","rb");
	if(f != NULL)
	{
		fread(buffer,1,sizeof(buffer),f);
		sscanf(buffer,"%d",&(context->state));
	}
	fclose(f);

	f = fopen("f:\\cis_setFwSavedBytes.bin","rb");
    if(f != NULL)
    {
        fread(buffer,1,sizeof(buffer),f);
		sscanf(buffer,"%d",&(context->savebytes));
    }
	fclose(f);*/
	return 1;
}


bool        cissys_setFwState(uint8_t state)
{
	//FILE* f = NULL;
	//int i = 0;
	//char buffer[10];
	//OutputDebugString("cissys_setFwState\n");
	//sprintf(buffer,"%d",state);
	//f = fopen("f:\\cis_setFwState.bin","wb");
	////fseek(f, 0, SEEK_END);
	//if(f != NULL)
	//{
	//	fwrite(buffer,1,1,f);
	//	fclose(f);
	//}
    #ifdef CIS_ENABLE_UPDATE_EC_ENABLE    
	
	if ((STATE == FOTA_STATE_IDIL) && (STATE == FOTA_STATE_DOWNLOADING))
	{
        ERASED_LENGTH = 0;
	}

    #ifdef CIS_DEBUG_MOD_ONENET                                    
    ECOMM_TRACE(UNILOG_ONENET, cissys_setFwState_1, P_INFO, 2, "state %d,  new state %d", STATE, state);
    #endif
    
    #endif   
    
	STATE=state;
	return true;
}
bool        cissys_setFwUpdateResult(uint8_t result)
{
	//FILE* f = NULL;
	//int i = 0;
	//char buffer[10];
	//OutputDebugString("cissys_setFwUpdateResult\n");
	//sprintf(buffer,"%d",result);
	//f = fopen("f:\\cis_setFwUpdateResult.bin","wb");
	////fseek(f, 0, SEEK_END);
	//if(f != NULL)
	//{
	//	fwrite(buffer,1,1,f);
	//	fclose(f);
	//}
	RESULT=result;
		return true;
}



bool cissys_checkLocalUpdateResult(void)
{
    char versionSucc[] = "OK";//for test only
    char versionFlag[2] = {0, 0};//read back
    bool ret = false;

	if(BSP_QSPI_Read_Safe((uint8_t *)(&versionFlag[0]), FOTA_RESERVED_END - FOTA_DL_32K_FLASH_BLOCK + VERSION_MAGIC_LOC, sizeof (versionFlag)) == 0)
	{
		if (memcmp(versionFlag, versionSucc, sizeof(versionFlag)) ==0)
		{	
			ret = true;
		}
	}
	return ret;
}


#if CIS_ENABLE_UPDATE_MCU

char SOTA_VERSION[16]="0.0.0";
uint32_t MCU_SotaSize=100;


bool cissys_setSwState(bool ismcu)
{
	isupdatemcu = ismcu;
	return true;
}

bool cissys_getSwState(void)
{

    return isupdatemcu;
}

void  cissys_setSotaMemory(uint32_t size)
{
	MCU_SotaSize=size;
}

uint32_t  cissys_getSotaMemory(void)
{
	return MCU_SotaSize;
}

uint32_t    cissys_getSotaVersion(uint8_t** version)
{
	int length = strlen(SOTA_VERSION)+1;
	cis_memcpy(*version,SOTA_VERSION,length);
	return length;
}

void  cissys_setSotaVersion(char* version)
{
	int length = strlen(version)+1;
	cis_memcpy(SOTA_VERSION,version,length);
}

#endif


#ifdef CIS_ENABLE_UPDATE_EC_ENABLE    

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#define SDK_VER_NUM_POS 10
#elif defined CHIP_EC616S
#define SDK_VER_NUM_POS 11
#endif


#ifdef EC_VERSION_TYPE
void cis_loadVersion(void)
{
    int vlength = strlen(SOFTVERSION);
	cis_memset(VERSION, 0, sizeof(VERSION));

	if((strlen(SOFTVERSION)+1) <= sizeof(VERSION))
    {
    	cis_memcpy(VERSION, SOFTVERSION, vlength);
	}
	else
	{
		ECOMM_TRACE(UNILOG_ONENET, cis_loadVersion_1, P_ERROR, 0, "SOFTVERSION string is too long");
		cis_memcpy(VERSION, "V000.000.000", sizeof("V000.000.000"));
	}
}
#endif    

void cis_reboot_timer_handler(TimerHandle_t xTimer)
{
#ifdef CIS_DEBUG_MOD_ONENET       
    ECOMM_TRACE(UNILOG_ONENET, cis_reboot_timer_handler_1, P_INFO, 0, "reboot ...");
#endif

    delay_us(20000);

	cis_SetCFUN(0);

    #if defined CHIP_EC616S
    extern void PmuAonPorEn(void);
    PmuAonPorEn();
    #endif

    NVIC_SystemReset();
}

void cis_reboot_timer(void)
{
	TimerHandle_t reboot_timer = xTimerCreate("reboot_timer", 3000 / portTICK_PERIOD_MS, false, NULL, cis_reboot_timer_handler);
    
#ifdef CIS_DEBUG_MOD_ONENET    
    ECOMM_TRACE(UNILOG_ONENET, cis_reboot_timer_1, P_INFO, 0, "start reboot timer");
#endif
	xTimerStart(reboot_timer, 0);
}
#endif


#endif // CIS_ENABLE_UPDATE

#if CIS_ENABLE_MONITER
/*
int8_t      cissys_getRSRP()
{
	return 0;
}
*/
int8_t      cissys_getSINR()
{
	return 0;
}
int8_t    cissys_getBearer()
{
    return 0;
}

int8_t    cissys_getUtilization()
{
    return 0;
}

int8_t    cissys_getCSQ()
{
    return 0;
}

int8_t    cissys_getECL()
{
    return 0;
}

int8_t    cissys_getSNR()
{
    return 0;
}

int16_t   cissys_getPCI()
{
    return 0;
}

int32_t   cissys_getECGI()
{
    return 0;
}

int32_t   cissys_getEARFCN()
{
    return 0;
}

int16_t   cissys_getPCIFrist()
{
    return 0;
}

int16_t   cissys_getPCISecond()
{
    return 0;
}

int8_t     cissys_getRSRPFrist()
{
    return 0;
}

int8_t     cissys_getRSRPSecond()
{
    return 0;
}

int8_t     cissys_getPLMN()
{
	return 0;
}

int8_t     cissys_getLatitude()
{
  return 0;

}

int8_t     cissys_getLongitude()
{
  return 0;

}

int8_t     cissys_getAltitude()
{
 return 0;

}
#endif // CIS_ENABLE_MONITER
