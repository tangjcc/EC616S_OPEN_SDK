#include <string.h>

#include "plat_config.h"
#include "flash_ec616s_rt.h"
#include "mem_map.h"

#include "adups_bl_main.h"
#include "adups_bl_flash.h"

#if (WDT_FEATURE_ENABLE==1)
#include "wdt_ec616s.h"

#endif

extern uint32_t ImageGeneralRead(uint8_t *pData,uint32_t ReadAddr, uint32_t Size);
extern uint8_t BSP_QSPI_Write(uint8_t *pData,uint32_t ReadAddr, uint32_t Size);
extern uint8_t BSP_QSPI_Erase_Sector(uint32_t BlockAddress);


int32_t adups_bl_read_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
    uint32_t address = adups_bl_flash_delta_base() + addr;

    retValue = ImageGeneralRead((uint8_t *)data_ptr, address, len);

#ifdef ABUP_DEBUG_PRINTF
    adups_bl_debug_print(LOG_DEBUG,"adups_bl_read_flash-->address;%x,len:%x,retValue:%d\r\n",address,len,retValue);
#endif

    return (retValue == QSPI_OK) ? FLASH_STATUS_OK: FLASH_STATUS_ERROR;
}

int32_t adups_bl_write_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
    uint32_t address = addr;//adups_bl_flash_delta_base() + addr;

    if(address>=0x00800000)
    address=address-0x00800000;
    retValue = BSP_QSPI_Write((uint8_t *)data_ptr, address, len);

#ifdef ABUP_DEBUG_PRINTF
    adups_bl_debug_print(LOG_DEBUG,"adups_bl_write_flash-->address;%x,len:%x,ret:%d\r\n",address,len,retValue);
#endif
    return (retValue == QSPI_OK) ? FLASH_STATUS_OK: FLASH_STATUS_ERROR;
}




void abup_init_flash(void)
{
    adups_bl_erase_delata();
    adups_bl_erase_backup_region();
}

adups_uint32 adups_bl_flash_block_size(void)
{
    return ADUPS_BLOCK_SIZE;
}
//wpf add 还原读写的传入的BLOCK  SIZE
adups_uint32 adups_bl_flash_true_block_size(void)
{
    return ADUPS_TRUE_BLOCK_SIZE;
}


ADUPS_BOOL AdupsUseFlash(void)
{
    return ADUPS_TRUE;
}

adups_uint32 adups_bl_flash_backup_base(void)
{
    //return FLASH_FOTA_REGION_START + FLASH_FOTA_REGION_LEN - adups_bl_flash_block_size();
    return FLASH_FOTA_REGION_START + FLASH_FOTA_REGION_LEN - ADUPS_TRUE_BLOCK_SIZE;
}

adups_uint32 adups_bl_flash_delta_base(void)
{
    return FLASH_FOTA_REGION_START;
}

adups_uint32 adups_bl_flash_delta_size(void)
{
    //return FLASH_FOTA_REGION_LEN - ADUPS_BLOCK_SIZE;
    return FLASH_FOTA_REGION_LEN - ADUPS_TRUE_BLOCK_SIZE;

}

adups_uint32 AdupsGetFlashDiskSize(void)
{
    return adups_bl_flash_delta_size();
}

adups_uint32 adups_bl_get_rtos_len(void)
{
    return APP_FLASH_LOAD_SIZE; 
}

adups_uint32 adups_bl_get_rtos_base(void)
{
    return APP_FLASH_LOAD_ADDR; 
}

adups_int32 adups_bl_erase_block(adups_uint32 addr)
{
    adups_int32 marker_addr  = addr;
    uint8_t retValue;

    
    if(marker_addr>=0x00800000)
    marker_addr=marker_addr-0x00800000;

    retValue = BSP_QSPI_Erase_Sector(marker_addr);
    if (retValue != QSPI_OK)
    {
    	adups_bl_debug_print(LOG_DEBUG, "flash erase block error status = %d\r\n", retValue);
    	return FLASH_STATUS_ERROR;
    }

    return FLASH_STATUS_OK;
}

adups_int32 adups_bl_read_block(adups_uint8* dest, adups_uint32 start, adups_uint32 size)
{

    long ret_val = -1; // read error

    adups_uint8* read_buf = dest;
    uint8_t retValue;
    uint32_t address = start;//abup_get_backup_base() + start;

#if (WDT_FEATURE_ENABLE==1)	
    WDT_Kick();
#endif

    retValue = ImageGeneralRead(read_buf, address, size);

#ifdef ABUP_DEBUG_PRINTF
    adups_bl_debug_print(LOG_DEBUG, "read block address:%x,size:%d,retValue:%d\r\n",address,size,retValue);
#endif
    if(retValue == QSPI_OK)
    {
    	ret_val = size;
    }
    else
    {
    	ret_val = FLASH_STATUS_ERROR;
    }

    return ret_val;
}

adups_int32 adups_bl_write_block(adups_uint8 *src, adups_uint32 start, adups_uint32 size)
{

    adups_uint32 address = start;
    adups_int32 write_size = size;
    adups_uint8  retValue ;
    adups_uint8 *buffer = src;
    adups_uint32 real_address = start;
    adups_int32  real_write_size = size;

#ifdef ABUP_DEBUG_PRINTF
    adups_bl_debug_print(LOG_DEBUG, "adups_bl_write_block address = %x,size = %d \n\r", address, size);
#endif

#if 1 //erase 4K every time
    while(real_write_size > 0)
    {
    	retValue = adups_bl_erase_block(real_address);
    	if (retValue != QSPI_OK)
    	{
    		adups_bl_debug_print(LOG_DEBUG, "w flash erase block error status = %d\r\n", retValue);
    		return FLASH_STATUS_ERROR;		
    	}
    	
    	real_address += adups_bl_flash_block_size();
    	real_write_size -= adups_bl_flash_block_size();
    	#ifdef ABUP_DEBUG_PRINTF
    	adups_bl_debug_print(LOG_DEBUG, "w flash erase block real_address:%x, status = %d\r\n", real_address,retValue);//debug log
    	#endif
    }

#else
    retValue = adups_bl_erase_block(address);
    if (retValue != QSPI_OK)
    {
    adups_bl_debug_print(LOG_DEBUG, "w flash erase block error status = %d\r\n", retValue);
    return FLASH_STATUS_ERROR;

    }
#endif


    retValue = adups_bl_write_flash(address, (uint8_t *)buffer, write_size);
#ifdef ABUP_DEBUG_PRINTF
    adups_bl_debug_print(LOG_DEBUG, "flash write addr=%x, write_size=%d\n\r",address,write_size);
#endif


    if (retValue != QSPI_OK)
    {
    	adups_bl_debug_print(LOG_DEBUG, "flash write block error status = %d\r\n", retValue);
    	return FLASH_STATUS_ERROR;
    }

    return FLASH_STATUS_OK;
}






adups_int32 adups_bl_erase_delata(void)
{
    uint8_t retValue;
    adups_int32 marker_addr = adups_bl_flash_delta_base();
#if 1 // only init first block 
    retValue = adups_bl_erase_block(marker_addr);
    if (retValue != QSPI_OK)
    {
    	adups_bl_debug_print(LOG_DEBUG, "flash erase delata error status = %d\r\n", retValue);
    	return FLASH_STATUS_ERROR;
    }
#else
    adups_int32 erase_len = adups_bl_flash_delta_size();
    while (erase_len > 0)
    {
    	retValue = adups_bl_erase_block(marker_addr);
    	if (retValue != QSPI_OK)
    	{
    	#ifdef ADUPS_FOTA_ENABLE_DIFF_ERROR
    		adups_bl_debug_print(LOG_DEBUG, "hal_flash_erase status = %d\r\n", flash_status);
    	#endif
    		return FLASH_STATUS_ERROR;
    	}
    	erase_len -= adups_bl_flash_block_size();
    	marker_addr += adups_bl_flash_block_size();
    }
#endif
    return FLASH_STATUS_OK;

}

void adups_bl_erase_backup_region(void)
{

    adups_uint8 retValue;
    adups_int32 marker_addr = adups_bl_flash_backup_base();

    retValue = adups_bl_erase_block(marker_addr);
    if (retValue != QSPI_OK)
    {
    	adups_bl_debug_print(LOG_DEBUG, "flash erase backup error status = %d\r\n", retValue);

    }

}

adups_int32 adups_bl_write_backup_region(adups_uint8 *data_ptr, adups_uint32 len)
{

    adups_int32 result;
    adups_int32 marker_addr = adups_bl_flash_backup_base();

    result = adups_bl_write_block(data_ptr, marker_addr, len);
    return result;
}

adups_int32  adups_bl_read_backup_region(adups_uint8 *data_ptr, adups_uint32 len)
{

    adups_int32 result;
    adups_int32 marker_addr = adups_bl_flash_backup_base();

    result = adups_bl_read_block(data_ptr, marker_addr, len);
    return result;

}



