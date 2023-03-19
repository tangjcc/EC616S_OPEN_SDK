#include "adups_typedef.h"
#include "adups_bl_main.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "adups_bl_flash.h"
#include "peer_type.h"
#include "abup_stdlib.h"
#include "bl_bsp.h"

#define ADUPS_FOTA_ENABLE_DIFF_DEBUG


#define ADUPS_FOTA_ENABLE_DIFF

#ifdef ADUPS_FOTA_ENABLE_DIFF

extern adups_uint32 Image$$LOAD_DRAM_FOTA$$ZI$$Base;  /* Length of LOAD_DRAM_FOTA region */
extern adups_uint32 Image$$LOAD_DRAM_FOTA$$ZI$$Length ;  /* Length of LOAD_DRAM_FOTA region */

#define ADUPS_WORKING_RAM_BUFFER ((adups_uint32)(&Image$$LOAD_DRAM_FOTA$$ZI$$Base))
#define UPI_WORKING_BUFFER_LEN ((adups_uint32)(&Image$$LOAD_DRAM_FOTA$$ZI$$Length))





//for multi upgrade
#define ADUPS_MAX_DELTA_NUM 2
adups_uint8 adups_delta_num = 1;
adups_uint8 adups_curr_delta = 1;
multi_head_info_patch PackageInfo = {0};
static adups_uint32 s_status = 0;

extern int fotapatch_procedure(void);
extern void frame_memory_finalize(void);
extern ADUPS_BOOL adups_init_mem_pool(adups_uint8 *buffer, adups_uint32 len);
extern ADUPS_BOOL adups_IsPackgeFound(void);
extern adups_int32 adups_get_package_info(multi_head_info_patch *info,adups_uint8 curr_delta);
extern void adups_get_package_number(adups_uint8*);

ADUPS_BOOL adups_get_pre_patch_flag(void)
{
    return ADUPS_TRUE;
}

ADUPS_BOOL adups_get_pre_check_flag(void)
{
    return ADUPS_FALSE;
}

adups_uint8 *adups_bl_get_working_buffer(void)
{
    return (adups_uint8 *)ADUPS_WORKING_RAM_BUFFER;
}

adups_uint64 adups_bl_get_working_buffer_len(void)
{
    return UPI_WORKING_BUFFER_LEN;
}

adups_uint32 adups_bl_get_diff_param_size(void)
{
    return ADUPS_DIFF_PARAM_SIZE;
}

adups_uint32 adups_bl_get_curr_write_address(void)
{
    return 0;
}


#define BL_FOTA_UPDATE_INFO_RESERVE_SIZE       (512)
/*
 * RAW write update info
 */

adups_uint32 adups_bl_get_app_base(void)
{
    //adups_bl_debug_print(NULL,"adups_bl_get_app_base %x", PackageInfo.multi_bin_address);
    return PackageInfo.multi_bin_address;
    //return adups_bl_get_rtos_base();
}

adups_uint32 adups_bl_get_delta_base(void)
{
    //adups_bl_debug_print(NULL,"adups_bl_get_delta_base %x", PackageInfo.multi_bin_address);
    return (adups_bl_flash_delta_base()+PackageInfo.multi_bin_offset);
}


void WacthDogRestart()
{
	
}

void adups_wait_sec(adups_int32 sec)
{
#if 0//
    hal_rtc_time_t time;
    adups_int32 second;

    if (sec < 1)
    {
    	return;
    }

    hal_rtc_get_time(&time);
    second = time.rtc_sec;

    while (1)
    {
    	hal_rtc_get_time(&time);
    	if (time.rtc_sec != second)
    	{
    		adups_bl_debug_print(LOG_DEBUG, "adups_wait_sec\r\n");
    		second = time.rtc_sec;
    		sec -= 1;
    		if (sec == 0)
    		{
    			break;
    		}
    	}
    }
#endif
}

ADUPS_BOOL adups_IsPartPackge(void)
{
    adups_uint8 data[5];
    adups_bl_debug_print(LOG_DEBUG, "adups_IsPartPackge\r\n");
    memset(data, 0, sizeof(data));
    adups_bl_read_backup_region(data, sizeof(data));
    adups_bl_debug_print(LOG_DEBUG, "%c%c%c%c%c\r\n", data[0], data[1], data[2], data[3], data[4]);
    if (data[0] == 'A' && data[1] == 'D' && data[2] == 'U' && data[3] == 'P' && data[4] == 'S')
    {
    	adups_bl_debug_print(LOG_DEBUG, "adups_IsPartPackge: YES!\r\n");
    	return ADUPS_TRUE;
    }
    adups_bl_debug_print(LOG_DEBUG, "adups_IsPartPackge: NO!\r\n");
    return ADUPS_FALSE;
}

void adups_bl_notify_status(adups_uint32 a_status, adups_uint32 a_percentage)
{
    adups_bl_debug_print(LOG_DEBUG, "adups_bl_notify_status:%d,%d\r\n", a_status, a_percentage);
    s_status = a_status;

    switch (a_status)
    {
    	case ADUPS_BL_UPDATE_START:
    		//bl_disp_fota_upgrading_face(a_percentage);
    		break;
    	case ADUPS_BL_UPDATE_END_SUC:
    	case ADUPS_BL_UPDATE_END_FAIL:
    	default:
    		break;
    }
}

adups_uint32 adups_bl_get_status(void)
{
    return s_status;
}
//abup add 
void AbupUpgradeProgressCallback(int prepatch,int percent)
{
    static adups_uint8 i = 0;
    SelNormalOrURCPrint(1);
    if (prepatch)
    {
        i++;
        if (i==1)
        {
            //abup_bl_main_printf("Progress:%d\r\n",percent);
            abup_bl_main_printf("+QIND: \"FOTA\",\"UPDATING\",%d%%,1,1\r\n",percent);
        }
    }
    else
    {
        //Normal patch
        //abup_bl_main_printf("Progress:%d\r\n",percent);    
        abup_bl_main_printf("+QIND: \"FOTA\",\"UPDATING\",%d%%,1,1\r\n",percent);
    }
    SelNormalOrURCPrint(0);    
}

void AUDPSProcedure(void)
{
    adups_int16 status = ADUPS_FUDIFFNET_ERROR_NONE;
    //adups_uint8 i;
    adups_uint8 ok[2]={"OK"};
    adups_uint8 no[2]={"NO"};


    adups_bl_debug_print(LOG_DEBUG, "AUDPSProcedure 4.0\r\n");

    if (adups_IsPartPackge() == ADUPS_TRUE)
    {
    	return;
    }
    if (!adups_IsPackgeFound())
    {
    	status = ADUPS_FUDIFFNET_ERROR_UPDATE_ERROR_END;
		adups_bl_debug_print(LOG_DEBUG, "adups Packge: not found!\r\n");
    	return;
    }

    adups_get_package_number(&adups_delta_num);
    if ((adups_delta_num<1) || (adups_delta_num>ADUPS_MAX_DELTA_NUM))
    {
    	adups_bl_debug_print(LOG_DEBUG, "adups_get_package_number %d, not valid\n\r", adups_delta_num);
    }
    else
    {
    	adups_bl_debug_print(LOG_DEBUG, "adups_get_package_number %d\n\r", adups_delta_num);
    }

    if (status == ADUPS_FUDIFFNET_ERROR_NONE)
    {
    	adups_init_mem_pool(adups_bl_get_working_buffer(), adups_bl_get_working_buffer_len());

    	//bl_oled_init();
    	while (adups_curr_delta <= adups_delta_num)
    	{
    		memset(&PackageInfo, 0, sizeof(multi_head_info_patch));
    	#ifdef ADUPS_FOTA_ENABLE_DIFF_DEBUG
    		adups_bl_debug_print(LOG_DEBUG, "adups_curr_delta=%d\n\r", adups_curr_delta);
    	#endif
    		status = adups_get_package_info(&PackageInfo, adups_curr_delta);
    		if (status != ADUPS_FUDIFFNET_ERROR_NONE)
    			break;
    	#ifdef ADUPS_FOTA_ENABLE_DIFF_DEBUG
    		adups_bl_debug_print(LOG_DEBUG, "PackageInfo add=%x,m=%d,offset=%d\n\r", PackageInfo.multi_bin_address,
    			PackageInfo.multi_bin_method, PackageInfo.multi_bin_offset);
    	#endif

    		adups_bl_notify_status(ADUPS_BL_UPDATE_START, 0);
    		status = fotapatch_procedure();

    		if (status != ADUPS_FUDIFFNET_ERROR_NONE)
    		{
    		#ifdef ADUPS_FOTA_ENABLE_DIFF_DEBUG
    			adups_bl_debug_print(LOG_DEBUG, "patch fail\r\n");
    		#endif
    			adups_bl_notify_status(ADUPS_BL_UPDATE_END_FAIL, 100);
    			break;
    		}
    		else
    		{
    		#ifdef ADUPS_FOTA_ENABLE_DIFF_DEBUG
    			adups_bl_debug_print(LOG_DEBUG, "patch sucess\r\n");
    		#endif
    			adups_curr_delta++;
    		}
    	}

    	adups_bl_notify_status(ADUPS_BL_UPDATE_END_SUC, 100);

    	frame_memory_finalize();
    }

    if (status == ADUPS_FUDIFFNET_ERROR_NONE)
    {
    	adups_bl_erase_delata();
    	adups_bl_erase_backup_region();
    	adups_bl_write_backup_region(ok, 2);
    	adups_bl_debug_print(NULL, "Update done, reboot now ...\n\r");
    }
    else
    {
    	adups_bl_erase_delata();
    	adups_bl_erase_backup_region();
    	adups_bl_write_backup_region(no, 2);
    	adups_bl_debug_print(NULL, "***Something wrong during update, status=%d\n\r", status);
    }
    
}
#endif

void abup_patch_progress_ext(abup_bool PrePatch,abup_uint16 total,abup_uint16 current)
{    
    static abup_int curr = -1;
    if(curr != current)   
    {       
        curr = current;   
        AbupUpgradeProgressCallback(PrePatch, curr*100/total);
    }
}

#ifdef THIRDPARTY_ABUP_FOTA_BL_ENABLE
void FotaProcedure(void)
{
    AUDPSProcedure();
}
#endif

