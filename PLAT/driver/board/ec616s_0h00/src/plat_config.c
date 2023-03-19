/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    plat_config.c
 * Description:  platform configuration source file
 * History:      Rev1.0   2019-01-18
 *               Rev1.1   2019-11-27   Reimplement file operations with OSA APIs(LFS wrapper), not directly using LFS APIs in case of file system replacement
 *               Rev1.2   2020-01-01   Separate plat config into two parts, FS and raw flash
 *               Rev1.3   2020-06-29   Add backwards compatibility mechanism
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "plat_config.h"
#include "osasys.h"
#include "debug_log.h"

/** \brief config file version
 *  \note when the order of struct \ref plat_config_fs_t and \ref plat_config_raw_flash_t field has changed,
 *        for example, from 1,2,3 to 1,3,2, update version to refresh flash,
 *        in other cases(add more fields or remove some fields from struct \ref plat_config_fs_t), it's not a must to update
 *  \details
 *   version update from 0 to 1: Add atPortFrameFormat config item
 */
#define FS_PLAT_CONFIG_FILE_CURRENT_VERSION           (1)

#define RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION    (0)

static plat_config_fs_t g_fsPlatConfig;

plat_config_raw_flash_t g_rawFlashPlatConfig;

/**
  \fn           void BSP_SetDefaultFsPlatConfig(void)
  \brief        set default value of "g_fsPlatConfig"
  \return       void
*/
static void BSP_SetDefaultFsPlatConfig(void)
{
    memset(&g_fsPlatConfig, 0x0, sizeof(g_fsPlatConfig));
    g_fsPlatConfig.atPortBaudRate = 9600;
}

/**
  \fn           void BSP_SetDefaultRawFlashPlatConfig(void)
  \brief        set default value of "g_rawFlashPlatConfig"
  \return       void
*/
static void BSP_SetDefaultRawFlashPlatConfig(void)
{

#ifdef SDK_REL_BUILD
    g_rawFlashPlatConfig.faultAction = 4;//silent anable
    g_rawFlashPlatConfig.startWDT = 1;//start wdt
#else
    g_rawFlashPlatConfig.faultAction = 0;
    g_rawFlashPlatConfig.startWDT = 0;
#endif


    g_rawFlashPlatConfig.dumpToATPort = 0;
    g_rawFlashPlatConfig.logControl = 0x2;


    g_rawFlashPlatConfig.uartBaudRate = 3000000;


    g_rawFlashPlatConfig.logLevel = P_DEBUG;

}

void BSP_LoadPlatConfigFromFs(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    UINT8   crcCheck = 0;
    config_file_header_t fileHeader;

    /*
     * open NVM file
     */
    fp = OsaFopen("plat_config", "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_LoadPlatConfig_1, P_ERROR, 0,
                    "Can't open 'plat_config' file, use the defult value");

        BSP_SetDefaultFsPlatConfig();
        BSP_SavePlatConfigToFs();

        return;
    }

    /*
     * read file header
     */
    readCount = OsaFread(&fileHeader, sizeof(config_file_header_t), 1, fp);
    if (readCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_LoadPlatConfig_2, P_ERROR, 0,
                    "Can't read 'plat_config' file header, use the defult value");

        OsaFclose(fp);

        BSP_SetDefaultFsPlatConfig();
        BSP_SavePlatConfigToFs();

        return;
    }

    /*
     * read file body, check validation and handle compatiblity issue
     */
    if(fileHeader.version != FS_PLAT_CONFIG_FILE_CURRENT_VERSION)
    {
        if(fileHeader.version == 0)
        {
            plat_config_fs_v0_t v0Config;

            if(fileHeader.fileBodySize != sizeof(plat_config_fs_v0_t))
            {
                ECOMM_TRACE(UNILOG_PLA_MIDWARE, BSP_LoadPlatConfig_3, P_ERROR, 3,
                            "'plat_config' version:%d file body size not right: (%u/%u), use the defult value",
                            fileHeader.version, fileHeader.fileBodySize, sizeof(plat_config_fs_v0_t));

                OsaFclose(fp);

                BSP_SetDefaultFsPlatConfig();
                BSP_SavePlatConfigToFs();
            }
            else
            {
                readCount = OsaFread(&v0Config, sizeof(v0Config), 1, fp);
                crcCheck = OsaCalcCrcValue((UINT8 *)&v0Config, sizeof(v0Config));

                OsaFclose(fp);

                if (readCount != 1 || crcCheck != fileHeader.checkSum)
                {
                    ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_LoadPlatConfig_4, P_ERROR, 3,
                                "Can't read 'plat_config' version:%d file body, or body not right, (%u/%u), use the defult value",
                                fileHeader.version, crcCheck, fileHeader.checkSum);

                    BSP_SetDefaultFsPlatConfig();
                    BSP_SavePlatConfigToFs();
                }
                else
                {
                    g_fsPlatConfig.enablePM = v0Config.enablePM;
                    g_fsPlatConfig.sleepMode = v0Config.sleepMode;
                    g_fsPlatConfig.slpWaitTime = v0Config.slpWaitTime;
                    g_fsPlatConfig.atPortBaudRate = v0Config.atPortBaudRate;
                    BSP_SavePlatConfigToFs();
                }
            }

        }
        // handle future version below
        else if(0)
        {

        }
        else
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_LoadPlatConfig_5, P_ERROR, 1,
                        "'plat_config' version:%d not right, use the defult value", fileHeader.version);

            OsaFclose(fp);

            BSP_SetDefaultFsPlatConfig();
            BSP_SavePlatConfigToFs();
        }
    }
    else
    {
        if(fileHeader.fileBodySize != sizeof(g_fsPlatConfig))
        {
            ECOMM_TRACE(UNILOG_PLA_MIDWARE, BSP_LoadPlatConfig_3, P_ERROR, 3,
                        "'plat_config' version:%d file body size not right: (%u/%u), use the defult value",
                        fileHeader.version, fileHeader.fileBodySize, sizeof(plat_config_fs_v0_t));

            OsaFclose(fp);

            BSP_SetDefaultFsPlatConfig();
            BSP_SavePlatConfigToFs();
        }
        else
        {
            readCount = OsaFread(&g_fsPlatConfig, sizeof(g_fsPlatConfig), 1, fp);
            crcCheck = OsaCalcCrcValue((UINT8 *)&g_fsPlatConfig, sizeof(g_fsPlatConfig));

            OsaFclose(fp);

            if (readCount != 1 || crcCheck != fileHeader.checkSum)
            {
                ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_LoadPlatConfig_4, P_ERROR, 3,
                            "Can't read 'plat_config' version:%d file body, or body not right, (%u/%u), use the defult value",
                            fileHeader.version, crcCheck, fileHeader.checkSum);

                BSP_SetDefaultFsPlatConfig();
                BSP_SavePlatConfigToFs();
            }
        }

    }

    return;
}

void BSP_SavePlatConfigToFs(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    config_file_header_t fileHeader;

    /*
     * open the config file
    */
    fp = OsaFopen("plat_config", "wb");   //write & create
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfig_1, P_ERROR, 0,
                    "Can't open/create 'plat_config' file, save plat_config failed");

        return;
    }

    /*
     * write the header
    */
    fileHeader.fileBodySize   = sizeof(g_fsPlatConfig);
    fileHeader.version        = FS_PLAT_CONFIG_FILE_CURRENT_VERSION;
    fileHeader.checkSum       = OsaCalcCrcValue((UINT8 *)&g_fsPlatConfig, sizeof(g_fsPlatConfig));

    writeCount = OsaFwrite(&fileHeader, sizeof(config_file_header_t), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfig_2, P_ERROR, 0,
                   "Write 'plat_config' file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&g_fsPlatConfig, sizeof(g_fsPlatConfig), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfig_3, P_ERROR, 0,
                   "Write 'plat_config' file body failed");
    }

    OsaFclose(fp);
    return;

}

uint32_t BSP_GetPlatConfigItemValue(plat_config_id_t id)
{
    switch(id)
    {
        case PLAT_CONFIG_ITEM_FAULT_ACTION:
            return g_rawFlashPlatConfig.faultAction;

        case PLAT_CONFIG_ITEM_AT_PORT_DUMP:
            return g_rawFlashPlatConfig.dumpToATPort;

        case PLAT_CONFIG_ITEM_START_WDT:
            return g_rawFlashPlatConfig.startWDT;

        case PLAT_CONFIG_ITEM_LOG_CONTROL:
            return g_rawFlashPlatConfig.logControl;

        case PLAT_CONFIG_ITEM_LOG_BAUDRATE:
            return g_rawFlashPlatConfig.uartBaudRate;

        case PLAT_CONFIG_ITEM_LOG_LEVEL:
            return g_rawFlashPlatConfig.logLevel;

        case PLAT_CONFIG_ITEM_ENABLE_PM:
            return g_fsPlatConfig.enablePM;

        case PLAT_CONFIG_ITEM_SLEEP_MODE:
            return g_fsPlatConfig.sleepMode;

        case PLAT_CONFIG_ITEM_WAIT_SLEEP:
            return g_fsPlatConfig.slpWaitTime;

        case PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE:
            return g_fsPlatConfig.atPortBaudRate;

        case PLAT_CONFIG_ITEM_AT_PORT_FRAME_FORMAT:
            return g_fsPlatConfig.atPortFrameFormat.wholeValue;

        default:
            return 0;
    }
}

void BSP_SetPlatConfigItemValue(plat_config_id_t id, uint32_t value)
{
    switch(id)
    {
        case PLAT_CONFIG_ITEM_FAULT_ACTION:
            if(value <= (EXCEP_OPTION_MAX -1))
                g_rawFlashPlatConfig.faultAction = value;
            break;

        case PLAT_CONFIG_ITEM_AT_PORT_DUMP:
            if(value <= 1)
                g_rawFlashPlatConfig.dumpToATPort = value;
            break;

        case PLAT_CONFIG_ITEM_START_WDT:
            if(value <= 1)
                g_rawFlashPlatConfig.startWDT = value;
            break;

        case PLAT_CONFIG_ITEM_LOG_CONTROL:
            if(value <= 2)
                g_rawFlashPlatConfig.logControl = value;
            break;

        case PLAT_CONFIG_ITEM_LOG_BAUDRATE:
            g_rawFlashPlatConfig.uartBaudRate = value;
            break;

        case PLAT_CONFIG_ITEM_LOG_LEVEL:
            if(value <= P_ERROR)
                g_rawFlashPlatConfig.logLevel = (debugTraceLevelType)value;
            break;

        case PLAT_CONFIG_ITEM_ENABLE_PM:
            g_fsPlatConfig.enablePM = value;
            break;

        case PLAT_CONFIG_ITEM_SLEEP_MODE:
            if(value <= 4)
                g_fsPlatConfig.sleepMode = value;
            break;

        case PLAT_CONFIG_ITEM_WAIT_SLEEP:
            g_fsPlatConfig.slpWaitTime = value;
            break;

        case PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE:
            g_fsPlatConfig.atPortBaudRate = value;
            break;

        case PLAT_CONFIG_ITEM_AT_PORT_FRAME_FORMAT:
            g_fsPlatConfig.atPortFrameFormat.wholeValue = value;
            break;

        default:
            break;
    }
    return;

}

plat_config_fs_t* BSP_GetFsPlatConfig(void)
{
    return &g_fsPlatConfig;
}

// external API declarations
extern uint8_t BSP_QSPI_Erase_Safe(uint32_t SectorAddress, uint32_t Size);
extern uint8_t BSP_QSPI_Write_Safe(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
extern uint8_t BSP_QSPI_Erase_Sector(uint32_t BlockAddress);
extern uint8_t BSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
extern UINT32 getOSState(void);

void BSP_SavePlatConfigToRawFlash(void)
{
    plat_info_layout_t platInfo;

    // read
    memcpy((uint8_t*)&platInfo, (void*)FLASH_MEM_PLAT_INFO_ADDR, sizeof(plat_info_layout_t));

    // modify start //
    // header part
    platInfo.header.fileBodySize   = sizeof(plat_config_raw_flash_t);
    platInfo.header.version        = RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION;
    platInfo.header.checkSum       = 0;

    // body part
    platInfo.config = g_rawFlashPlatConfig;
    // modify end //

    // write back

    if(1 == getOSState())
    {
        if(BSP_QSPI_Erase_Safe(FLASH_MEM_PLAT_INFO_NONXIP_ADDR, FLASH_MEM_PLAT_INFO_SIZE) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfigToRawFlash_1, P_ERROR, 0, "Erase flash error!!!");
            return;
        }

        if(BSP_QSPI_Write_Safe((uint8_t*)&platInfo, FLASH_MEM_PLAT_INFO_NONXIP_ADDR, sizeof(plat_info_layout_t)) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfigToRawFlash_2, P_ERROR, 0, "Save plat config to raw flash error!!!");
        }
    }
    else
    {
        if(BSP_QSPI_Erase_Sector(FLASH_MEM_PLAT_INFO_NONXIP_ADDR) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfigToRawFlash_3, P_ERROR, 0, "Erase flash error!!!");
            return;
        }

        if(BSP_QSPI_Write((uint8_t*)&platInfo, FLASH_MEM_PLAT_INFO_NONXIP_ADDR, sizeof(plat_info_layout_t)) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SavePlatConfigToRawFlash_4, P_ERROR, 0, "Save plat config to raw flash error!!!");
        }
    }

}

static void BSP_WriteToRawFlash(uint8_t* pBuffer, uint32_t bufferSize)
{
    if(pBuffer == NULL || bufferSize == 0)
    {
        return;
    }

    if(1 == getOSState())
    {
        if(BSP_QSPI_Erase_Safe(FLASH_MEM_PLAT_INFO_NONXIP_ADDR, FLASH_MEM_PLAT_INFO_SIZE) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_WriteToRawFlash_1, P_ERROR, 0, "Erase flash error!!!");
            return;
        }

        if(BSP_QSPI_Write_Safe(pBuffer, FLASH_MEM_PLAT_INFO_NONXIP_ADDR, bufferSize) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_WriteToRawFlash_2, P_ERROR, 0, "Save plat config to raw flash error!!!");
        }
    }
    else
    {
        if(BSP_QSPI_Erase_Sector(FLASH_MEM_PLAT_INFO_NONXIP_ADDR) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_WriteToRawFlash_3, P_ERROR, 0, "Erase flash error!!!");
            return;
        }

        if(BSP_QSPI_Write(pBuffer, FLASH_MEM_PLAT_INFO_NONXIP_ADDR, bufferSize) != 0)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_WriteToRawFlash_4, P_ERROR, 0, "Save plat config to raw flash error!!!");
        }
    }

}

void BSP_LoadPlatConfigFromRawFlash(void)
{
    plat_info_layout_t platInfo;

    config_file_header_t header;

    /*
     * read file header
     */
    memcpy((uint8_t*)&header, (void*)FLASH_MEM_PLAT_INFO_ADDR, sizeof(header));

    if(header.version != RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION)
    {
        if(header.version == 0)
        {
            plat_config_raw_flash_v0_t v0Config;
            uint32_t fsAssertCount;

            BSP_SetDefaultRawFlashPlatConfig();

            // migrate from old version
            if(header.fileBodySize == sizeof(plat_config_raw_flash_v0_t))
            {
                memcpy((uint8_t*)&v0Config, (void*)(FLASH_MEM_PLAT_INFO_ADDR + sizeof(config_file_header_t)), sizeof(v0Config));
                memcpy((uint8_t*)&fsAssertCount, (void*)(FLASH_MEM_PLAT_INFO_ADDR + sizeof(config_file_header_t) + sizeof(v0Config)), sizeof(fsAssertCount));

                g_rawFlashPlatConfig.faultAction = v0Config.faultAction;
                g_rawFlashPlatConfig.dumpToATPort = v0Config.dumpToATPort;
                g_rawFlashPlatConfig.startWDT = v0Config.startWDT;
                g_rawFlashPlatConfig.logControl = v0Config.logControl;
                g_rawFlashPlatConfig.uartBaudRate = v0Config.uartBaudRate;
                g_rawFlashPlatConfig.logLevel = v0Config.logLevel;

                platInfo.header.fileBodySize   = sizeof(plat_config_raw_flash_t);
                platInfo.header.version        = RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION;
                platInfo.header.checkSum       = 0;

                platInfo.config = g_rawFlashPlatConfig;
                platInfo.fsAssertCount = fsAssertCount;

            }
            // version matches but size is wrong, use default value
            else
            {
                platInfo.header.fileBodySize   = sizeof(plat_config_raw_flash_t);
                platInfo.header.version        = RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION;
                platInfo.header.checkSum       = 0;

                platInfo.config = g_rawFlashPlatConfig;
                platInfo.fsAssertCount = 0;
            }

            BSP_WriteToRawFlash((uint8_t*)&platInfo, sizeof(platInfo));

        }
        else if(0)
        {
            // handle future version
        }
        else
        {
            // version is invalid

            BSP_SetDefaultRawFlashPlatConfig();

            platInfo.header.fileBodySize   = sizeof(plat_config_raw_flash_t);
            platInfo.header.version        = RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION;
            platInfo.header.checkSum       = 0;

            platInfo.config = g_rawFlashPlatConfig;
            platInfo.fsAssertCount = 0;

            BSP_WriteToRawFlash((uint8_t*)&platInfo, sizeof(platInfo));

        }
    }
    else
    {
        if(header.fileBodySize == sizeof(plat_config_raw_flash_t))
        {
            memcpy((uint8_t*)&g_rawFlashPlatConfig, (void*)(FLASH_MEM_PLAT_INFO_ADDR + sizeof(config_file_header_t)), sizeof(g_rawFlashPlatConfig));
        }
        else
        {
            BSP_SetDefaultRawFlashPlatConfig();

            platInfo.header.fileBodySize   = sizeof(plat_config_raw_flash_t);
            platInfo.header.version        = RAW_FLASH_PLAT_CONFIG_FILE_CURRENT_VERSION;
            platInfo.header.checkSum       = 0;

            platInfo.config = g_rawFlashPlatConfig;
            platInfo.fsAssertCount = 0;

            BSP_WriteToRawFlash((uint8_t*)&platInfo, sizeof(platInfo));
        }
    }

}

plat_config_raw_flash_t* BSP_GetRawFlashPlatConfig(void)
{
    return &g_rawFlashPlatConfig;
}


uint32_t BSP_GetFSAssertCount(void)
{
    plat_info_layout_t platInfo;

    // read
    memcpy((uint8_t*)&platInfo, (void*)FLASH_MEM_PLAT_INFO_ADDR, sizeof(plat_info_layout_t));

    return platInfo.fsAssertCount;
}

void BSP_SetFSAssertCount(uint32_t value)
{
    plat_info_layout_t platInfo;

    // read
    memcpy((uint8_t*)&platInfo, (void*)FLASH_MEM_PLAT_INFO_ADDR, sizeof(plat_info_layout_t));

    // modify
    platInfo.fsAssertCount = value;

    // erase
    if(BSP_QSPI_Erase_Safe(FLASH_MEM_PLAT_INFO_NONXIP_ADDR, FLASH_MEM_PLAT_INFO_SIZE) != 0)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SetFSAssertCount_0, P_ERROR, 0, "Erase flash error!!!");
        return;
    }

    // write back
    if(BSP_QSPI_Write_Safe((uint8_t*)&platInfo, FLASH_MEM_PLAT_INFO_NONXIP_ADDR, sizeof(plat_info_layout_t)) != 0)
    {
        ECOMM_TRACE(UNILOG_PLA_DRIVER, BSP_SetFSAssertCount_1, P_ERROR, 0, "Update fsAssertCount value error!!!");
    }

}


