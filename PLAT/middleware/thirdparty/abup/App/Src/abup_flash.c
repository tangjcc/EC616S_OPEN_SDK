/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    abup_flash.c
 * Description:  abup fota entry source file
 * History:      Rev1.0   2019-08-12
 *
 ****************************************************************************/
 #include <string.h>
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#include "flash_ec616s_rt.h"
#endif
#include "debug_log.h"
#include "plat_config.h"
#include "debug_trace.h"
#include "lfs.h"
#include "abup_flash.h"
#include "abup_typedef.h"

#define ABUP_BLOCK_SIZE     (32*1024)

static uint8_t abup_update_result = 2;
#define ABUP_BREAK_LEN 			5
#define ABUP_DELTA_ID_LEN 	 	10
#define ABUP_DOWNLOAD_BLOCK_BASE	0			//245
#define ABUP_DOWNLOAD_BLOCK_LEN  30    	//max buff  = (Adups_get_block_size * DOWNLOAD_BLOCK_LEN * 8)/1024 
#define ABUP_DELTA_ID_BASE		ABUP_DOWNLOAD_BLOCK_BASE+ABUP_DOWNLOAD_BLOCK_LEN			//265

enum param_type { 
	ABUP_PARAM_DOWNLOAD_BLOCK,
	ABUP_PARAM_DELTA_ID,
};

enum operation_type {
	ABUP_OPERATION_READ = 1,    
	ABUP_OPERATION_WRITE
};

/*
FLASH_FOTA_REGION_START   :  fota区域的起始地址
FLASH_FOTA_REGION_END  ： fota区域的长度
fota区域包含：存放差分包区域、存放差分包delta id区域（4K，倒数第二块block）、存放备份数据区域（4K，倒数第一块block）
断点续传功能是建立在我们私有服务器上调试的，其他服务器不知道能否使用
*/

//升级完成后会往备份区域写入标志符，此函数是获取升级结果
//1:升级成功;  99 : 升级失败 ; 0: 未进行升级
void abup_check_upgrade_result(void)
{
	uint8_t backup_data[4] = {0};

	abup_read_backup(0, backup_data, 4);
	if (strncmp((char *)backup_data, "OK", 2) == 0)
	{
		abup_update_result=1;
	}
	else if (strncmp((char *)backup_data, "NO", 2) == 0)
	{
		abup_update_result=99;
	}
	else
	{
		abup_update_result=0;
	}
}

//需要先调用abup_check_upgrade_result ，此函数的返回值才有作用
uint8_t abup_get_upgrade_result(void)
{
	return abup_update_result;
}

//清除断点续传标志符
int32_t Adups_destroy_break(void)
{
    uint32_t retValue;

	retValue =abup_erase_backup();
	return retValue;
}

//清除升级结果标志符
void Adups_clear_update_result(void)
{
	abup_erase_backup();
}

//fota区域的起始地址
uint32_t abup_get_delta_base(void)
{
	return FLASH_FOTA_REGION_START;
}

//fota放差分包区域的长度
uint32_t abup_get_delta_len(void)
{
	return (FLASH_FOTA_REGION_END - FLASH_FOTA_REGION_START - abup_get_block_size());
}

//flash block size
uint32_t abup_get_block_size(void)
{
	return ABUP_BLOCK_SIZE;
}

//fota备份区域的起始地址
uint32_t abup_get_backup_base(void)
{
	return (FLASH_FOTA_REGION_END - abup_get_block_size());
}

//存放差分包date区域的起始地址
//主要存放delta_id 和断点续传当前下载进度的标志符
uint32_t abup_get_data_base(void)
{
	return (FLASH_FOTA_REGION_END - 2*abup_get_block_size());
}

//fota放差分包区域的flash块数
adups_uint32 Adups_get_delta_block_num(void)
{
	return (uint32_t)(abup_get_delta_len()/abup_get_block_size());
}

//断点续传使用到的
//获取data 区域的数据，读取断点之前下载到哪个block了
//未下载完，先判断是有否有Adups_get_break ，继续下载时调用
int32_t Adups_get_udp_block_num(void)
{
	uint32_t ret = 0;
	uint8_t buff[ABUP_DOWNLOAD_BLOCK_LEN] = {0};
	uint32_t i = 0;
	int32_t num = 0;
	
	memset(buff, 0xFF, ABUP_DOWNLOAD_BLOCK_LEN);
	ret =  Adups_operation_param(ABUP_PARAM_DOWNLOAD_BLOCK, ABUP_OPERATION_READ, buff);
	if(ret == 0)
	{
		while(buff[i] == 0)
		{
			i++;
			num+=8;
		}
		
		switch(buff[i])
		{
			case 0x80:	num +=7; break;
			case 0xC0:	num +=6; break;
			case 0xE0:	num +=5; break;
			case 0xF0:	num +=4; break;
			case 0xF8:	num +=3; break;
			case 0xFC:	num +=2; break;
			case 0xFE:	num +=1; break; 
			case 0xFF:	num +=0; break; 
			default:				
				ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_1, P_INFO, 0, "Adups_get_udp_block_num-->lash op failed");
				num = -1;
				break;
		}
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_2, P_INFO, 2, "Adups_get_udp_block_num-->num:%d,delta_block_num:%d",num,Adups_get_delta_block_num());
		if(num > 0)
		{
			if(num > Adups_get_delta_block_num())
			{
				num = -1;
			}
		}
		return num;

		
	}
	else
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_3, P_INFO, 1, "Adups_get_udp_block_num-->flash op failed");
	}
	return ret;
	
}

/*断点续传使用到的,往date区域写断点续传的相关数据
buff :用来保存当前下载了多少个block,1bit代表1个block,1个字节就是8个并把当前写到第几块保存到data 区域里
num:当前下载到fota哪一块block
*/
int32_t Adups_set_udp_block_num(uint32_t num)
{
	uint32_t ret = 0;
	uint8_t buff[ABUP_DOWNLOAD_BLOCK_LEN] = {0};
	uint32_t i = 0;
	
	if(num > ABUP_DOWNLOAD_BLOCK_LEN * 8)
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_4, P_INFO, 0, "Adups_set_udp_block_num-->too many udp block num");
		return -1;
	}
	
	memset(buff, 0xFF, ABUP_DOWNLOAD_BLOCK_LEN);
	while(num >= 8)
	{
		buff[i] = 0;
		num-=8;
		i++;
	}
	
	switch(num)
	{
		case 0x00:	buff[i] =0xFF; break;
		case 0x01:	buff[i] =0xFE; break;
		case 0x02:	buff[i] =0xFC; break;
		case 0x03:	buff[i] =0xF8; break;
		case 0x04:	buff[i] =0xF0; break;
		case 0x05:	buff[i] =0xE0; break;
		case 0x06:	buff[i] =0xC0; break;
		case 0x07:	buff[i] =0x80; break;	
	}
	ret =  Adups_operation_param(ABUP_PARAM_DOWNLOAD_BLOCK, ABUP_OPERATION_WRITE, buff);
	if(ret == 0)
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_5, P_INFO, 0, "Adups_set_udp_block_num-->flash op succeed ");
	}
	else
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_6, P_INFO, 0, "Adups_set_udp_block_num-->flash op failed");
	}
	return ret;
}

//断点续传和升级结果上报使用到的
//读取data 区域的数据
int32_t abup_read_data_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
	uint32_t address = abup_get_data_base() + addr;
	
    retValue = BSP_QSPI_Read_Safe((uint8_t *)data_ptr, address, len);
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_7, P_INFO, 3, "abup_read_data_backup-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//断点续传和升级结果上报使用到的
//往data 区域写入数据
int32_t abup_write_data_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
	uint32_t address = abup_get_data_base() + addr;
	
    retValue = BSP_QSPI_Write_Safe((uint8_t *)data_ptr, address, len);

	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_8, P_INFO, 3, "abup_write_data_backup-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//断点续传使用到的
adups_int32 Adups_operation_param(adups_uint8 type, adups_uint8 op, adups_uint8 * value)
{
	adups_int32 ret = 0;
	adups_uint32 len = 0;
	adups_uint32 addr = 0;
	
	switch(type)
	{ 
		case ABUP_PARAM_DOWNLOAD_BLOCK:
			len = ABUP_DOWNLOAD_BLOCK_LEN;
			addr = ABUP_DOWNLOAD_BLOCK_BASE;
			break;
		case ABUP_PARAM_DELTA_ID:
			len = ABUP_DELTA_ID_LEN;
			addr = ABUP_DELTA_ID_BASE;
			break;
	}

	if(op == ABUP_OPERATION_READ)
	{
		ret = abup_read_data_backup(addr, value, len);
	}
	else
	{
		ret = abup_write_data_backup(addr, value, len);
	}
	return ret;
}

//升级结果上报使用到的
//读取date区域的delta_id
int32_t Adups_get_delta_id(void)
{
	adups_int32 ret = 0;
	adups_int32 len = 0;
	adups_uint8 buff[ABUP_DELTA_ID_LEN] = {0};
	adups_int8 i;
	memset(buff, 0, ABUP_DELTA_ID_LEN);
	ret = Adups_operation_param(ABUP_PARAM_DELTA_ID, ABUP_OPERATION_READ, buff);

	if(ret == 0)
	{
		for(i = 0; i < ABUP_DELTA_ID_LEN; i++)
		{
			if(buff[i] > 9)
			{
				return 0;
			}
			len = len*10 + buff[i];
		}
		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_9, P_INFO, 1, "Adups_get_delta_id-->delta_id:%d",len);
		return len;
	}	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_10, P_INFO, 0, "Adups_get_delta_id-->flash op failed");
	return ret;
}

//升级结果上报使用到的
//往date区域写入的delta_id
//delta_id :服务器下发的，每个差分包对应一个delta_id，升级结果上报会用到
int32_t Adups_set_delta_id(uint32_t delta_id)
{
	adups_int32 ret = 0;
	adups_int8 i = 0;
	adups_uint8 buff[ABUP_DELTA_ID_LEN] = {0};
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_11, P_INFO, 1, "Adups_set_delta_id-->delta_id:%d",delta_id);
	memset(buff, 0, ABUP_DELTA_ID_LEN);
	for(i = ABUP_DELTA_ID_LEN-1; i >= 0; i--)
	{
		buff[i] = delta_id%10;
		delta_id = delta_id/10;
	}

	ret = Adups_operation_param(ABUP_PARAM_DELTA_ID, ABUP_OPERATION_WRITE, buff);
	if(ret == 0)
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_12, P_INFO, 10, "Adups_set_delta_id-->flash op succeed delta_id=%d%d%d%d%d%d%d%d%d%d",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],buff[6],buff[7],buff[8],buff[9]);
	}
	else
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_13, P_INFO, 0, "Adups_set_delta_id-->flash op failed");
	}
	return ret;
}

//判断是否有断点续传
//下载之前往备份区域写入ADUPS标识符,差分包下载完校验OK后删除ADUPS标识符
ADUPS_BOOL Adups_get_break(void)
{
	adups_int32 ret = 0;
	adups_uint8 buff[ABUP_BREAK_LEN] = {0};

	ret = abup_read_backup(0, buff,ABUP_BREAK_LEN);
	if(ret == 0)
	{
		if(buff[0] == 'A' && buff[1] == 'D' && buff[2] == 'U' && buff[3] == 'P' && buff[4] == 'S')
		{			
			ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_14, P_INFO, 0, "Adups_get_break-->has break point");
			return ADUPS_TRUE;
		}		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_15, P_INFO, 0, "Adups_get_break-->has no break point");
	}
	else
	{		
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_16, P_INFO, 0, "Adups_get_break-->flash op failed");
	}
	return ADUPS_FALSE;
}

//往备份区域写入ADUPS标识符
void Adups_set_break(void)
{
	uint8_t buff[ABUP_BREAK_LEN] = {0};
	
	buff[0]='A'; buff[1]='D'; buff[2]='U'; buff[3]='P';buff[4]='S';
	abup_write_backup(0,buff,ABUP_BREAK_LEN);
} 

//读取备份区域数据
int32_t abup_read_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
	uint32_t address = abup_get_backup_base() + addr;
	
    retValue = BSP_QSPI_Read_Safe((uint8_t *)data_ptr, address, len);
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_17, P_INFO, 3, "abup_read_backup-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//往备份区域写入数据
int32_t abup_write_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
	uint32_t address = abup_get_backup_base() + addr;
	
    retValue = BSP_QSPI_Write_Safe((uint8_t *)data_ptr, address, len);

	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_18, P_INFO, 3, "abup_write_backup-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//读取fota 差分包区域的数据
int32_t abup_read_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
	uint32_t address = abup_get_delta_base() + addr;

    retValue = BSP_QSPI_Read_Safe((uint8_t *)data_ptr, address, len);
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_19, P_INFO, 3, "abup_read_flash-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//fota 差分包区域写入数据
int32_t abup_write_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
	uint32_t address = abup_get_delta_base() + addr;

    retValue = BSP_QSPI_Write_Safe((uint8_t *)data_ptr, address, len);
	
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_20, P_INFO, 3, "abup_write_flash-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//fota erase 32k area by sector erase
uint8_t abup_erase_32k_by_sector(uint32_t SectorAddress)
{
    uint8_t RetValue;
     uint32_t OffsetAddress;
    uint32_t RemainLen;
    uint32_t CurrEraseSize;

    if ((SectorAddress&0xfff) !=0)
    {
         return QSPI_ERROR;
    }

    OffsetAddress = SectorAddress;
    RemainLen = ABUP_BLOCK_SIZE;

    while(RemainLen > 0)
    {
        CurrEraseSize = (RemainLen >= 0x1000) ? 0x1000 : RemainLen;
        RemainLen -= CurrEraseSize;

        RetValue = BSP_QSPI_Erase_Safe(OffsetAddress, CurrEraseSize);
        if (RetValue!=QSPI_OK)
        {
             return QSPI_ERROR;
        }

        OffsetAddress += CurrEraseSize;
    }
     return QSPI_OK;
}


//格式化差分包区域
int32_t abup_erase_delata(void)
{
    uint8_t retValue;
	uint32_t address = abup_get_delta_base();
	uint32_t len=abup_get_delta_len();
    uint32_t real_erase_size = len;
    uint32_t real_address = address;

	while(real_erase_size > 0)
	{
		retValue = abup_erase_32k_by_sector(real_address);
		if (retValue != QSPI_OK)
		{                
            ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_21, P_INFO, 2, "abup_erase_delata error:real_address:%x,ret:%d",real_address,retValue);
			return LFS_ERR_IO;		
		}
		
		real_address += ABUP_BLOCK_SIZE;
		real_erase_size -= ABUP_BLOCK_SIZE;
        
        ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_ERASE_22, P_INFO, 2, "abup_erase_delata-->real_address;%x,ret:%d",real_address,retValue);
        vTaskDelay(300 / portTICK_RATE_MS);
   }    
        return LFS_ERR_OK;
}

//断点续传使用到的
//差分包下载一部分时:数据是按块存储的，不满一块，下次继续下载时，会删除当前块的数据
int32_t Adups_erase_block(uint32_t block)
{
    uint8_t retValue;
	uint32_t block_size = abup_get_block_size();
    uint32_t address = abup_get_delta_base()+block* block_size;
				
	retValue = abup_erase_32k_by_sector(address);
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_23, P_INFO, 2, "Adups_erase_block-->address;%x,ret:%d",address,retValue);
	return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//格式化备份区域
int32_t abup_erase_backup(void)
{
    uint8_t retValue;
	uint32_t address = abup_get_backup_base();

	retValue = abup_erase_32k_by_sector(address );
	ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_ERASE_25, P_INFO, 2, "abup_erase_backup-->address;%x,ret:%d",address,retValue);
	return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

//格式化备份区域和差分包区域
//下载差分包前调用
int32_t abup_init_flash(void)
{
	uint16_t ret1,ret2;
	
	ret1 = abup_erase_delata();
	ret2 = abup_erase_backup();
	if(ret1 == 0 && ret2 == 0)
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_27, P_INFO, 0, "abup_init_flash-->sucess");
		return 0;
	}
	else
	{
		ECOMM_TRACE(UNILOG_ABUP_APP, UNILOG_ABUP_FLASH1_LOG_28, P_INFO, 0, "abup_init_flash-->fail");
		return -1;
	}
}


