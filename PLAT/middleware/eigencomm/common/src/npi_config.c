/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    npi_config.c
 * Description:  NPI configuration operation file
 * History:      Rev1.0   2020-04-02
 *               Rev1.1   2020-06-29   Add backwards compatibility mechanism
 *
 ****************************************************************************/
#include "npi_config.h"
#include "debug_log.h"
#include "nvram.h"

#define PRODMODE_ENABLE_MAGIC 0x5A504D45     //0x5A"PME"
#define PRODMODE_DISABLE_MAGIC 0xA5504D44    //0xA5"PMD"

#define NV10_CONTENT_VALID_FLAG   0xA55A

/*
 * NV10 layout typedef
*/
typedef struct _Nv10layout
{
    UINT16 validFlag;
    UINT16 version;
    NPIProcessStatus npiStatus;
} Nv10layout_t;

/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__

/*
 * whether "npiconfig.nvm" and NV10 already read
*/
static BOOL    g_npiNvmRead = FALSE;

/*
 * NPI NVM configuration variable;
*/
static NPINvmConfig    g_npiNvmConfig;

static Nv10layout_t g_nv10Layout = {0};

/******************************************************************************
 ******************************************************************************
 * EXTERNAL VARIABLES/FUNCTIONS
 ******************************************************************************
******************************************************************************/
#define __SPEC_EXTERNAL_VARIABLES__ //just for easy to find this position in SS



/******************************************************************************
 ******************************************************************************
 static functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS

/**
  \fn           void npiSetDefaultNvmConfig(void)
  \brief        set default value of "g_npiNvmConfig"
  \param[in]    void
  \returns      void
*/
static void npiSetDefaultNvmConfig(void)
{
    memset(&g_npiNvmConfig, 0x00, sizeof(NPINvmConfig));
    g_npiNvmConfig.prodModeStatus.prodModeEnable = PRODMODE_DISABLE_MAGIC;
    return;
}

/**
  \fn          void npiSaveConfigToNV10(void)
  \brief
  \returns     void
*/
void npiSaveConfigToNV10(void)
{
    /*----------------------------------NV10 LAYOUT version 1--------------------------*/
    /*----valid flag----|--------version-------|-----NPI config-----|------------------*/
    /*       2byte      |         2byte        |       4byte        |                  | */

    g_nv10Layout.validFlag = NV10_CONTENT_VALID_FLAG;
    g_nv10Layout.version = NV10_NVM_VERSION;

    nvram_write(NV10, (uint8_t*)&g_nv10Layout, sizeof(g_nv10Layout));
    nvram_write(NV10, (uint8_t*)&g_nv10Layout, sizeof(g_nv10Layout));
    //sync to default reliable region
    nvram_sav2fac_at(SAVE_OTHER);

}

/**
  \fn          void npiLoadConfigFromNV10(CHAR* npiConfigBuf)
  \brief
  \param[in]   void
  \returns     void
*/
static void npiLoadConfigFromNV10(void)
{
    /*----------------------------------NV10 LAYOUT version 1--------------------------*/
    /*----valid flag----|--------version-------|-----NPI config-----|------------------*/
    /*       2byte      |         2byte        |       4byte        |                  | */

    nvram_read(NV10, (uint8_t*)&g_nv10Layout, sizeof(g_nv10Layout), 0);
}

/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/**
  \fn           void npiLoadNvmConfig(void)
  \brief        load/read NPI NVM file, if not exist, set & write the default value
  \param[in]    void
  \returns      void
*/
void npiLoadNvmConfig(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    NPINvmHeader    npiNvmHdr;   //4 bytes
    UINT8   crcCheck = 0;

    if(g_npiNvmRead)
    {
        return;
    }

    npiLoadConfigFromNV10();

    /*
     * open NVM file
    */
    fp = OsaFopen(NPI_NVM_FILE_NAME, "rb");   //read only
    if(fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiLoadNvmConfig_1, P_ERROR, 0,
                    "Can't open NVM: 'npiconfig.nvm', use the defult value");

        npiSetDefaultNvmConfig();
        npiSaveNvmConfig();

        g_npiNvmRead = TRUE;

        return;
    }

    /*
     * read file header
    */
    readCount = OsaFread(&npiNvmHdr, sizeof(NPINvmHeader), 1, fp);
    if(readCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiLoadNvmConfig_2, P_ERROR, 0,
                    "NPI NVM: 'npiconfig.nvm', can't read header, use the defult value");

        OsaFclose(fp);

        npiSetDefaultNvmConfig();
        npiSaveNvmConfig();

        g_npiNvmRead = TRUE;

        return;
    }

    /*
     * read file body, check validation and handle compatiblity issue
     */
    if(npiNvmHdr.version != NPI_NVM_FILE_VERSION)
    {
        if(npiNvmHdr.version == 0)
        {
            NPINvmConfig_v0 v0Config;

            if(npiNvmHdr.fileBodySize != sizeof(NPINvmConfig_v0))
            {
                ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiLoadNvmConfig_3, P_ERROR, 3,
                            "'npiconfig.nvm' version:%d file body size not right: (%u/%u), use the defult value",
                            npiNvmHdr.version, npiNvmHdr.fileBodySize, sizeof(NPINvmConfig_v0));

                OsaFclose(fp);

                npiSetDefaultNvmConfig();
                npiSaveNvmConfig();
            }
            else
            {
                readCount = OsaFread(&v0Config, sizeof(v0Config), 1, fp);
                crcCheck = OsaCalcCrcValue((UINT8 *)&v0Config, sizeof(v0Config));

                OsaFclose(fp);

                if (readCount != 1 || crcCheck != npiNvmHdr.checkSum)
                {
                    ECOMM_TRACE(UNILOG_PLA_DRIVER, npiLoadNvmConfig_4, P_ERROR, 3,
                                "Can't read 'npiconfig.nvm' version:%d file body, or body not right, (%u/%u), use the defult value",
                                npiNvmHdr.version, crcCheck, npiNvmHdr.checkSum);

                    npiSetDefaultNvmConfig();
                    npiSaveNvmConfig();
                }
                else
                {
                    g_npiNvmConfig.prodModeStatus = v0Config.prodModeStatus;
                    npiSaveNvmConfig();

                    if(g_nv10Layout.validFlag != NV10_CONTENT_VALID_FLAG)
                    {
                        g_nv10Layout.npiStatus = v0Config.npiStatus;
                        npiSaveConfigToNV10();
                    }
                }
            }

        }
        // handle future version below
        else if(0)
        {

        }
        else
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, npiLoadNvmConfig_5, P_ERROR, 1,
                        "'npiconfig.nvm' version:%d not right, use the defult value", npiNvmHdr.version);

            OsaFclose(fp);

            npiSetDefaultNvmConfig();
            npiSaveNvmConfig();
        }
    }
    else
    {
        if(npiNvmHdr.fileBodySize != sizeof(g_npiNvmConfig))
        {
            ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiLoadNvmConfig_3, P_ERROR, 3,
                        "'npiconfig.nvm' version:%d file body size not right: (%u/%u), use the defult value",
                        npiNvmHdr.version, npiNvmHdr.fileBodySize, sizeof(g_npiNvmConfig));

            OsaFclose(fp);

            npiSetDefaultNvmConfig();
            npiSaveNvmConfig();
        }
        else
        {
            readCount = OsaFread(&g_npiNvmConfig, sizeof(g_npiNvmConfig), 1, fp);
            crcCheck = OsaCalcCrcValue((UINT8 *)&g_npiNvmConfig, sizeof(g_npiNvmConfig));

            OsaFclose(fp);

            if (readCount != 1 || crcCheck != npiNvmHdr.checkSum)
            {
                ECOMM_TRACE(UNILOG_PLA_DRIVER, npiLoadNvmConfig_4, P_ERROR, 3,
                            "Can't read 'npiconfig.nvm' version:%d file body, or body not right, (%u/%u), use the defult value",
                            npiNvmHdr.version, crcCheck, npiNvmHdr.checkSum);

                npiSetDefaultNvmConfig();
                npiSaveNvmConfig();
            }
        }

    }

    g_npiNvmRead = TRUE;

    return;
}


/**
  \fn           void npiSaveNvmConfig(void)
  \brief        Save NPI NVM config file
  \param[in]    void
  \returns      void
*/
void npiSaveNvmConfig(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    NPINvmHeader    npiNvmHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(NPI_NVM_FILE_NAME, "wb");   //read & write
    if(fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiSaveNvmConfig_1, P_ERROR, 0,
                    "NPI NVM, can't open/create NVM: 'npiconfig.nvm', save NVM failed");

        return;
    }

    /*
     * write the header
    */
    npiNvmHdr.fileBodySize   = sizeof(g_npiNvmConfig);
    npiNvmHdr.version        = NPI_NVM_FILE_VERSION;
    npiNvmHdr.checkSum       = OsaCalcCrcValue((UINT8 *)&g_npiNvmConfig, sizeof(g_npiNvmConfig));

    writeCount = OsaFwrite(&npiNvmHdr, sizeof(NPINvmHeader), 1, fp);
    if(writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiSaveNvmConfig_2, P_ERROR, 0,
                   "NPI NVM: 'npiconfig.nvm', write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&g_npiNvmConfig, sizeof(g_npiNvmConfig), 1, fp);
    if(writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, npiSaveNvmConfig_3, P_ERROR, 0,
                   "NPI NVM: 'npiconfig.nvm', write the file body failed");
    }

    OsaFclose(fp);
    return;
}

/**
  \fn           UINT32 npiGetProcessStatusItemValue(NPIProcessStatusItem cfgEnum)
  \brief        Get/return one NPI process status value
  \param[in]    cfgEnum     which item value need to return
  \returns      config value
*/
UINT32 npiGetProcessStatusItemValue(NPIProcessStatusItem cfgEnum)
{
    if(g_npiNvmRead == FALSE)
    {
        npiLoadNvmConfig();
    }

    switch(cfgEnum)
    {
        case NPI_PROCESS_STATUS_ITEM_RFCALI:
            return (g_nv10Layout.validFlag ==  NV10_CONTENT_VALID_FLAG) ? g_nv10Layout.npiStatus.rfCaliDone : 0;

        case NPI_PROCESS_STATUS_ITEM_RFNST:
            return (g_nv10Layout.validFlag ==  NV10_CONTENT_VALID_FLAG) ? g_nv10Layout.npiStatus.rfNSTDone : 0;

        default:
            break;
    }

    return 0;
}


/**
  \fn           void npiSetAndSaveProcessStatusItemValue(NPIProcessStatusItem cfgEnum, UINT32 value)
  \brief        Set one NPI process status item value, and save into NVM (flash)
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \returns      void
*/
void npiSetAndSaveProcessStatusItemValue(NPIProcessStatusItem cfgEnum, UINT32 value)
{
    BOOL    nvmChanged = FALSE;

    npiSetProcessStatusItemValue(cfgEnum, value, &nvmChanged);

    if(nvmChanged)
    {
        npiSaveConfigToNV10();
    }

    return;
}

/**
  \fn           void npiSetProcessStatusItemValue(NPIProcessStatusItem cfgEnum, UINT32 value, BOOL *bChanged)
  \brief        Set one NPI process status item value, but not save to flash, need caller call npiSaveNvmConfig() to write to flash
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \param[out]   bChanged    return value, whether NVM changed, if return TRUE, caller need to write to flash
  \returns      void
*/
void npiSetProcessStatusItemValue(NPIProcessStatusItem cfgEnum, UINT32 value, BOOL *bChanged)
{
    OsaCheck(bChanged != PNULL, bChanged, cfgEnum, value);

    if(g_npiNvmRead == FALSE)
    {
        npiLoadNvmConfig();
    }

    *bChanged = FALSE;

    switch(cfgEnum)
    {
        case NPI_PROCESS_STATUS_ITEM_RFCALI:

            if(value <= 1)  // 1 bit
            {
                // if first write or value changed
                if((g_nv10Layout.validFlag != NV10_CONTENT_VALID_FLAG) || (value != g_nv10Layout.npiStatus.rfCaliDone))
                {
                    g_nv10Layout.npiStatus.rfCaliDone = value;
                    *bChanged = TRUE;
                }
            }
            else
            {
                OsaDebugBegin(FALSE, cfgEnum, value, g_nv10Layout.npiStatus.rfCaliDone);
                OsaDebugEnd();
            }

            break;

        case NPI_PROCESS_STATUS_ITEM_RFNST:

            if(value <= 1) //1 bit
            {
                if((g_nv10Layout.validFlag != NV10_CONTENT_VALID_FLAG) || (value != g_nv10Layout.npiStatus.rfNSTDone))
                {
                    g_nv10Layout.npiStatus.rfNSTDone = value;
                    *bChanged = TRUE;
                }
            }
            else
            {
                OsaDebugBegin(FALSE, cfgEnum, value, g_nv10Layout.npiStatus.rfNSTDone);
                OsaDebugEnd();
            }

            break;

        default:
            OsaDebugBegin(FALSE, cfgEnum, value, 0);
            OsaDebugEnd();
            break;
    }

    return;
}

/**
  \fn           bool npiGetProdModeEnableStatus(void)
  \brief        Get/return prodModeEnable status value
  \returns      status value
*/
bool npiGetProdModeEnableStatus(void)
{
    if(g_npiNvmRead == FALSE)
    {
        npiLoadNvmConfig();
    }

    if(g_npiNvmConfig.prodModeStatus.prodModeEnable == PRODMODE_ENABLE_MAGIC)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
  \fn           void npiSetProdModeEnableStatus(bool enable)
  \brief        Set prodModeEnable status value
  \param[in]    enable   whether enable prodMode or not
  \returns      void
*/
void npiSetProdModeEnableStatus(bool enable)
{
    if(g_npiNvmRead == FALSE)
    {
        npiLoadNvmConfig();
    }

    if((enable == true) && (g_npiNvmConfig.prodModeStatus.prodModeEnable != PRODMODE_ENABLE_MAGIC))
    {
        g_npiNvmConfig.prodModeStatus.prodModeEnable = PRODMODE_ENABLE_MAGIC;
        npiSaveNvmConfig();
    }
    else if((enable == false) && (g_npiNvmConfig.prodModeStatus.prodModeEnable != PRODMODE_DISABLE_MAGIC))
    {
        g_npiNvmConfig.prodModeStatus.prodModeEnable = PRODMODE_DISABLE_MAGIC;
        npiSaveNvmConfig();
    }
}


