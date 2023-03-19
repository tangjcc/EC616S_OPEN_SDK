/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    mw_config.c
 * Description:  middleware configuration operation file
 * History:      Rev1.0   2019-04-10
 *
 ****************************************************************************/
#include "mw_config.h"
#include "debug_log.h"
#include "ps_sms_if.h"

/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__

/*
 * whether "midwareconfig.nvm" already read
*/
BOOL    mwNvmRead = FALSE;

/*
 * Middleware NVM configuration variable;
*/
MidWareNvmConfig    mwNvmConfig;


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
  \fn           void mwSetDefaultNvmConfig(void)
  \brief        set default value of "mwNvmConfig"
  \param[in]    void
  \returns      void
*/
static void mwSetDefaultNvmConfig(void)
{
    UINT32  tmpIdx = 0;

    memset(&mwNvmConfig, 0x00, sizeof(MidWareNvmConfig));

    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        mwNvmConfig.atChanConfig[tmpIdx].chanId         = tmpIdx+1;    //index 0 - chanId 1
        mwNvmConfig.atChanConfig[tmpIdx].echoValue      = 0;
        mwNvmConfig.atChanConfig[tmpIdx].suppressionValue = 0;
        mwNvmConfig.atChanConfig[tmpIdx].cregRptMode    = 0;    //CMI_MM_DISABLE_CREG;
        mwNvmConfig.atChanConfig[tmpIdx].ceregRptMode   = 0;    //CMI_PS_DISABLE_CEREG;
        mwNvmConfig.atChanConfig[tmpIdx].needEdrxRpt    = 0;    //need to report EDRX value, when EDRX value changes
        mwNvmConfig.atChanConfig[tmpIdx].needCiotOptRpt = 0;    //CMI_MM_CIOT_OPT_DISABLE_REPORT
        mwNvmConfig.atChanConfig[tmpIdx].smsSendMode    = 1;    //PSIL_SMS_FORMAT_TXT_MODE
        mwNvmConfig.atChanConfig[tmpIdx].csconRptMode   = 0;    //ATC_DISABLE_CSCON_RPT
        mwNvmConfig.atChanConfig[tmpIdx].cmeeMode       = 1;    //ATC_CMEE_NUM_ERROR_CODE
        mwNvmConfig.atChanConfig[tmpIdx].timeZoneRptMode = 0;   //ATC_CTZR_DISABLE_RPT
        mwNvmConfig.atChanConfig[tmpIdx].timeZoneUpdMode = 1;   //ATC_CTZU_ENABLE_UPDATE_TZ_VIA_NITZ
        mwNvmConfig.atChanConfig[tmpIdx].eccesqRptMode  = 0;    //ATC_ECCESQ_DISABLE_RPT
        mwNvmConfig.atChanConfig[tmpIdx].cgerepMode     = 0;    //ATC_CGEREP_DISCARD_OLD_CGEV
        mwNvmConfig.atChanConfig[tmpIdx].ecpsmRptMode   = 0;    //ATC_ECPSMR_DISABLE
        mwNvmConfig.atChanConfig[tmpIdx].ecemmtimeRptMode = 0;  //+ECEMMTIME
        mwNvmConfig.atChanConfig[tmpIdx].ecPtwEdrxRpt = 0;      //AT+ECPTWEDRXS
        mwNvmConfig.atChanConfig[tmpIdx].ecpinRptMode = 0;      //+ECPIN
        mwNvmConfig.atChanConfig[tmpIdx].ecpaddrRptMode = 0;    //+ECPADDR
        mwNvmConfig.atChanConfig[tmpIdx].ecpcfunRptMode = 0;    //+ECPCFUN

        mwNvmConfig.atChanConfig[tmpIdx].textSmsParam.fo = PSIL_MSG_FO_DEFAULT;
        mwNvmConfig.atChanConfig[tmpIdx].textSmsParam.vp = PSIL_MSG_VP_DEFAULT;
        mwNvmConfig.atChanConfig[tmpIdx].textSmsParam.pid = PSIL_MSG_PID_DEFAULT;
        mwNvmConfig.atChanConfig[tmpIdx].textSmsParam.dcs = PSIL_MSG_CODING_DEFAULT;
    }


    mwNvmConfig.autoRegConfig.autoRegEnableFlag = 1; /*disable register func */
    memcpy(mwNvmConfig.autoRegConfig.model, "EGN-YX EC616", strlen("EGN-YX EC616"));    /*default model, it is used by smart core  */
    memcpy(mwNvmConfig.autoRegConfig.swver, "EC616_SW_V1.1", strlen("EC616_SW_V1.1"));    /*default swver, it is used by smart core  */
    memcpy(mwNvmConfig.autoRegConfig.autoRegAck, "+REGACK", strlen("+REGACK"));    /*default swver, it is used by smart core  */

    mwNvmConfig.autoRegCuccConfig.autoRegEnableFlag = 1; /*disable register func */
    mwNvmConfig.autoRegCuccConfig.autoRegFlag = 1; /*no register */
    mwNvmConfig.autoRegCuccConfig.autoRegLastRegSuccTime = 0; /* clean time */
    mwNvmConfig.autoRegCuccConfig.autoRegPastTime = 0; /* clean time */
    mwNvmConfig.autoRegCuccConfig.autoRegRang = (10*1000);

    mwNvmConfig.autoRegCtccConfig.autoRegEnableFlag = 1; /*disable register func */
    mwNvmConfig.autoRegCtccConfig.autoRegFlag = 1; /*no register */

    #if 0
    /*default IPv4 DNS set to: 114.114.114.114 & 114.114.115.115 */
    mwNvmConfig.defaultDnsCfg.ipv4Dns[0][0]  = 114;
    mwNvmConfig.defaultDnsCfg.ipv4Dns[0][1]  = 114;
    mwNvmConfig.defaultDnsCfg.ipv4Dns[0][2]  = 114;
    mwNvmConfig.defaultDnsCfg.ipv4Dns[0][3]  = 114;

    mwNvmConfig.defaultDnsCfg.ipv4Dns[1][0]  = 114;
    mwNvmConfig.defaultDnsCfg.ipv4Dns[1][1]  = 114;
    mwNvmConfig.defaultDnsCfg.ipv4Dns[1][2]  = 115;
    mwNvmConfig.defaultDnsCfg.ipv4Dns[1][3]  = 115;

    /*default IPv4 DNS set to: 240C::6666 & 240C::6644 */
    mwNvmConfig.defaultDnsCfg.ipv6Dns[0][0]  = 0x24;
    mwNvmConfig.defaultDnsCfg.ipv6Dns[0][1]  = 0x0C;
    mwNvmConfig.defaultDnsCfg.ipv6Dns[0][14] = 0x66;
    mwNvmConfig.defaultDnsCfg.ipv6Dns[0][15] = 0x66;

    mwNvmConfig.defaultDnsCfg.ipv6Dns[1][0]  = 0x24;
    mwNvmConfig.defaultDnsCfg.ipv6Dns[1][1]  = 0x0C;
    mwNvmConfig.defaultDnsCfg.ipv6Dns[1][14] = 0x66;
    mwNvmConfig.defaultDnsCfg.ipv6Dns[1][15] = 0x44;
    #endif

    mwNvmConfig.dmConfig.mode = 0;
    mwNvmConfig.dmConfig.test = 1;
    #ifdef  FEATURE_REF_AT_ENABLE
    mwNvmConfig.dmConfig.lifeTime = 86400;
    #else
    mwNvmConfig.dmConfig.lifeTime = 900;
    #endif
    strcpy(mwNvmConfig.dmConfig.appKey, "M100000364");
    strcpy(mwNvmConfig.dmConfig.secret, "j5MOg7I6456971aQN7z6Bl36Xk5wYA5Q");

    mwNvmConfig.timeSyncTriggered = 0;
    mwNvmConfig.abupFotaEnableFlag = 0;

    //ec soc public setting
    mwNvmConfig.ecSocPubCfg.mode = 1;
    mwNvmConfig.ecSocPubCfg.publicDlBufferToalSize = 2048;
    mwNvmConfig.ecSocPubCfg.publicDlPkgNumMax = 8;

    mwNvmConfig.timeSecs.UTCsecs    = 0;
    mwNvmConfig.timeSecs.CTtimer    = 0;
    mwNvmConfig.timeSecs.timeZone   = 0;

    mwNvmRead = TRUE;

    return;
}

/**
  \fn           void mwAdjustNvmConfigVerion(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
  \brief        Get the NVM config from old version, set into latest version
  \param[in]    oldVer      old version id
  \param[in]    oldCtx      old version config value
  \param[in]    oldCtxSize  old version config structure size
  \param[out]   pMwNvmCfg   current/latest config value
  \returns      BOOL
*/

/*
//======================how to compatible with the old mw nvm config======================//
typedef struct MidWareNvmConfig_old_v00_Tag
{
    MidWareConfig0_old_v0      mwConfig0;     //the old v0 struct
    MidWareConfig1             mwConfig1;     //the current struct
    MidWareConfig2             mwConfig2;     //the current struct
    MidWareConfig3             mwConfig3;     //the current struct
    MidWareConfig4             mwConfig4;     //the current struct
    MidWareConfig5             mwConfig5;     //the current struct
}MidWareNvmConfig_old_v00;

typedef struct MidWareNvmConfig_old_v01_Tag
{
    MidWareConfig0_old_v1      mwConfig0;     //the old v1 struct, there is a old v0 struct
    MidWareConfig1_old_v1      mwConfig1;     //the old v1 struct
    MidWareConfig2             mwConfig2;     //the current struct
    MidWareConfig3             mwConfig3;     //the current struct
    MidWareConfig4             mwConfig4;     //the current struct
    MidWareConfig5             mwConfig5;     //the current struct
}MidWareNvmConfig_old_v01;

typedef struct MidWareNvmConfig_old_v02_Tag
{
    MidWareConfig0             mwConfig0;     //the current struct, there is a old v0 struct, there is a old v1 struct
    MidWareConfig1             mwConfig1;     //the current struct, there is a old v1 struct
    MidWareConfig2_old_v2      mwConfig2;     //the old v2 struct
    MidWareConfig3             mwConfig3;     //the current struct
    MidWareConfig4             mwConfig4;     //the current struct
    MidWareConfig5             mwConfig5;     //the current struct
}MidWareNvmConfig_old_v02;

typedef struct MidWareNvmConfig_old_v03_Tag
{
    MidWareConfig0             mwConfig0;     //the current struct, there is a old v0 struct, there is a old v1 struct
    MidWareConfig1             mwConfig1;     //the current struct, there is a old v1 struct
    MidWareConfig2             mwConfig2;     //the current struct, there is a old v2 struct
    MidWareConfig3_old_v3      mwConfig3;     //the old v3 struct
    MidWareConfig4             mwConfig4;     //the current struct
    MidWareConfig5             mwConfig5;     //the current struct
}MidWareNvmConfig_old_v03;

//======================keep the current struct======================//
typedef struct MidWareNvmConfig_Tag
{
    MidWareConfig0         mwConfig0;     //the current struct, there is a old v0 struct, there is a old v1 struct
    MidWareConfig1         mwConfig1;     //the current struct, there is a old v1 struct
    MidWareConfig2         mwConfig2;     //the current struct, there is a old v2 struct
    MidWareConfig3         mwConfig3;     //the current struct, there is a old v3 struct
    MidWareConfig4         mwConfig4;     //the current struct
    MidWareConfig5         mwConfig5;     //the current struct
}MidWareNvmConfig;
//======================keep the current struct======================//
*/

static BOOL mwAdjustNvmConfigVerion(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, MidWareNvmConfig *pMwNvmCfg)
{
    UINT32      tmpIdx = 0;
    MidWareNvmConfig_old_v00    *pMwNvmCfg_v00 = PNULL;
    MidWareNvmConfig_old_v01    *pMwNvmCfg_v01 = PNULL;
    MidWareNvmConfig_old_v02    *pMwNvmCfg_v02 = PNULL;

    OsaCheck(oldCtx != PNULL && pMwNvmCfg != PNULL, oldVer, oldCtx, pMwNvmCfg);

    if (oldVer == 0)
    {
        pMwNvmCfg_v00 = (MidWareNvmConfig_old_v00 *)oldCtx;

        OsaDebugBegin(sizeof(MidWareNvmConfig_old_v00) == oldCtxSize, sizeof(MidWareNvmConfig_old_v00), oldCtxSize, 0);
        return FALSE;
        OsaDebugEnd();

        memset(pMwNvmCfg, 0x00, sizeof(MidWareNvmConfig));

        memcpy(pMwNvmCfg->atChanConfig,
               pMwNvmCfg_v00->atChanConfig,
               sizeof(pMwNvmCfg_v00->atChanConfig));

        /* V1 add new flag: atChanConfig->ecHibRptMode */
        for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
        {
            pMwNvmCfg->atChanConfig[tmpIdx].ecHibRptMode = 0;
        }

        /* autoRegConfig */
        memcpy(&(pMwNvmCfg->autoRegConfig),
               &(pMwNvmCfg_v00->autoRegConfig),
               sizeof(pMwNvmCfg_v00->autoRegConfig));

        /* autoRegCuccConfig */
        memcpy(&(pMwNvmCfg->autoRegCuccConfig),
               &(pMwNvmCfg_v00->autoRegCuccConfig),
               sizeof(pMwNvmCfg_v00->autoRegCuccConfig));

        /* autoRegCtccConfig */
        memcpy(&(pMwNvmCfg->autoRegCtccConfig),
               &(pMwNvmCfg_v00->autoRegCtccConfig),
               sizeof(pMwNvmCfg_v00->autoRegCtccConfig));

        /* defaultDnsCfg */
        memcpy(&(pMwNvmCfg->defaultDnsCfg),
               &(pMwNvmCfg_v00->defaultDnsCfg),
               sizeof(pMwNvmCfg_v00->defaultDnsCfg));

        /* phyAtCmdContent */
        memcpy(&(pMwNvmCfg->phyAtCmdContent),
               &(pMwNvmCfg_v00->phyAtCmdContent),
               sizeof(pMwNvmCfg_v00->phyAtCmdContent));

        /* timeSyncTriggered */
        pMwNvmCfg->timeSyncTriggered = pMwNvmCfg_v00->mwSleepRetenFlag.timeSyncTriggered;

        /* ecSocPubCfg */
        memcpy(&(pMwNvmCfg->ecSocPubCfg),
               &(pMwNvmCfg_v00->ecSocPubCfg),
               sizeof(pMwNvmCfg_v00->ecSocPubCfg));

        pMwNvmCfg->timeSecs.UTCsecs = 0;
        pMwNvmCfg->timeSecs.CTtimer = 0;
        pMwNvmCfg->timeSecs.timeZone = 0;

        return TRUE;
    }
    else if(oldVer == 1)
    {
        pMwNvmCfg_v01 = (MidWareNvmConfig_old_v01 *)oldCtx;

        OsaDebugBegin(sizeof(MidWareNvmConfig_old_v01) == oldCtxSize, sizeof(MidWareNvmConfig_old_v01), oldCtxSize, 0);
        return FALSE;
        OsaDebugEnd();

        memset(pMwNvmCfg, 0x00, sizeof(MidWareNvmConfig));

        memcpy(pMwNvmCfg->atChanConfig,
               pMwNvmCfg_v01->atChanConfig,
               sizeof(pMwNvmCfg_v01->atChanConfig));

        /* V1 add new flag: atChanConfig->ecHibRptMode */
        for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
        {
            pMwNvmCfg->atChanConfig[tmpIdx].ecHibRptMode = 0;
        }

        /* autoRegConfig */
        memcpy(&(pMwNvmCfg->autoRegConfig),
               &(pMwNvmCfg_v01->autoRegConfig),
               sizeof(pMwNvmCfg_v01->autoRegConfig));

        /* autoRegCuccConfig */
        memcpy(&(pMwNvmCfg->autoRegCuccConfig),
               &(pMwNvmCfg_v01->autoRegCuccConfig),
               sizeof(pMwNvmCfg_v01->autoRegCuccConfig));

        /* autoRegCtccConfig */
        memcpy(&(pMwNvmCfg->autoRegCtccConfig),
               &(pMwNvmCfg_v01->autoRegCtccConfig),
               sizeof(pMwNvmCfg_v01->autoRegCtccConfig));

        /* defaultDnsCfg */
        memcpy(&(pMwNvmCfg->defaultDnsCfg),
               &(pMwNvmCfg_v01->defaultDnsCfg),
               sizeof(pMwNvmCfg_v01->defaultDnsCfg));

        /* dmConfig */
        memcpy(&(pMwNvmCfg->dmConfig.appKey),
               &(pMwNvmCfg_v01->dmConfig.appKey),
               sizeof(pMwNvmCfg_v01->dmConfig.appKey));
        memcpy(&(pMwNvmCfg->dmConfig.secret),
               &(pMwNvmCfg_v01->dmConfig.secret),
               sizeof(pMwNvmCfg_v01->dmConfig.secret));
        pMwNvmCfg->dmConfig.mode     = pMwNvmCfg_v01->dmConfig.mode;
        pMwNvmCfg->dmConfig.test     = pMwNvmCfg_v01->dmConfig.test;
        pMwNvmCfg->dmConfig.lifeTime = pMwNvmCfg_v01->dmConfig.lifeTime;

        /* phyAtCmdContent */
        memcpy(&(pMwNvmCfg->phyAtCmdContent),
               &(pMwNvmCfg_v01->phyAtCmdContent),
               sizeof(pMwNvmCfg_v01->phyAtCmdContent));

        /* timeSyncTriggered */
        pMwNvmCfg->timeSyncTriggered = pMwNvmCfg_v01->timeSyncTriggered;

        /* ecSocPubCfg */
        memcpy(&(pMwNvmCfg->ecSocPubCfg),
               &(pMwNvmCfg_v01->ecSocPubCfg),
               sizeof(pMwNvmCfg_v01->ecSocPubCfg));

        pMwNvmCfg->timeSecs.UTCsecs = 0;
        pMwNvmCfg->timeSecs.CTtimer = 0;
        pMwNvmCfg->timeSecs.timeZone = 0;

        return TRUE;
    }
    else if(oldVer == 2)
    {
        pMwNvmCfg_v02 = (MidWareNvmConfig_old_v02 *)oldCtx;

        OsaDebugBegin(sizeof(MidWareNvmConfig_old_v02) == oldCtxSize, sizeof(MidWareNvmConfig_old_v02), oldCtxSize, 0);
        return FALSE;
        OsaDebugEnd();

        memset(pMwNvmCfg, 0x00, sizeof(MidWareNvmConfig));

        memcpy(pMwNvmCfg->atChanConfig,
               pMwNvmCfg_v02->atChanConfig,
               sizeof(pMwNvmCfg_v02->atChanConfig));

        /* V1 add new flag: atChanConfig->ecHibRptMode */
        for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
        {
            pMwNvmCfg->atChanConfig[tmpIdx].ecHibRptMode = 0;
        }

        /* autoRegConfig */
        memcpy(&(pMwNvmCfg->autoRegConfig),
               &(pMwNvmCfg_v02->autoRegConfig),
               sizeof(pMwNvmCfg_v02->autoRegConfig));

        /* autoRegCuccConfig */
        memcpy(&(pMwNvmCfg->autoRegCuccConfig),
               &(pMwNvmCfg_v02->autoRegCuccConfig),
               sizeof(pMwNvmCfg_v02->autoRegCuccConfig));

        /* autoRegCtccConfig */
        memcpy(&(pMwNvmCfg->autoRegCtccConfig),
               &(pMwNvmCfg_v02->autoRegCtccConfig),
               sizeof(pMwNvmCfg_v02->autoRegCtccConfig));

        /* defaultDnsCfg */
        memcpy(&(pMwNvmCfg->defaultDnsCfg),
               &(pMwNvmCfg_v02->defaultDnsCfg),
               sizeof(pMwNvmCfg_v02->defaultDnsCfg));

        /* dmConfig */
        memcpy(&(pMwNvmCfg->dmConfig.appKey),
               &(pMwNvmCfg_v02->dmConfig.appKey),
               sizeof(pMwNvmCfg_v02->dmConfig.appKey));
        memcpy(&(pMwNvmCfg->dmConfig.secret),
               &(pMwNvmCfg_v02->dmConfig.secret),
               sizeof(pMwNvmCfg_v02->dmConfig.secret));
        pMwNvmCfg->dmConfig.mode     = pMwNvmCfg_v02->dmConfig.mode;
        pMwNvmCfg->dmConfig.test     = pMwNvmCfg_v02->dmConfig.test;
        pMwNvmCfg->dmConfig.lifeTime = pMwNvmCfg_v02->dmConfig.lifeTime;

        /* phyAtCmdContent */
        memcpy(&(pMwNvmCfg->phyAtCmdContent),
               &(pMwNvmCfg_v02->phyAtCmdContent),
               sizeof(pMwNvmCfg_v02->phyAtCmdContent));

        /* timeSyncTriggered */
        pMwNvmCfg->timeSyncTriggered = pMwNvmCfg_v02->timeSyncTriggered;

        /* ecSocPubCfg */
        memcpy(&(pMwNvmCfg->ecSocPubCfg),
               &(pMwNvmCfg_v02->ecSocPubCfg),
               sizeof(pMwNvmCfg_v02->ecSocPubCfg));

        pMwNvmCfg->timeSecs.UTCsecs = 0;
        pMwNvmCfg->timeSecs.CTtimer = 0;
        pMwNvmCfg->timeSecs.timeZone = 0;

        return TRUE;
    }

    OsaDebugBegin(FALSE, oldVer, oldCtxSize, 0);
    OsaDebugEnd();

    return FALSE;
}

/**
  \fn           void mwSaveNvmConfig(void)
  \brief        Save middle ware NVM config file
  \param[in]    void
  \returns      void
*/
static void mwSaveNvmConfig(void)
{
    OSAFILE fp = PNULL;
    UINT32  writeCount = 0;
    MidWareNvmHeader    mwNvmHdr;   //4 bytes

    /*
     * open the NVM file
    */
    fp = OsaFopen(MID_WARE_NVM_FILE_NAME, "wb");   //read & write
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwSaveNvmConfig_1, P_ERROR, 0,
                    "MW NVM, can't open/create NVM: 'midwareconfig.nvm', save NVM failed");

        return;
    }

    /*
     * write the header
    */
    mwNvmHdr.fileBodySize   = sizeof(mwNvmConfig);
    mwNvmHdr.version        = MID_WARE_NVM_FILE_LATEST_VERSION;
    mwNvmHdr.checkSum       = OsaCalcCrcValue((UINT8 *)&mwNvmConfig, sizeof(mwNvmConfig));

    writeCount = OsaFwrite(&mwNvmHdr, sizeof(MidWareNvmHeader), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwSaveNvmConfig_2, P_ERROR, 0,
                   "MW NVM: 'midwareconfig.nvm', write the file header failed");

        OsaFclose(fp);
        return;
    }

    /*
     * write the file body
    */
    writeCount = OsaFwrite(&mwNvmConfig, sizeof(mwNvmConfig), 1, fp);
    if (writeCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwSaveNvmConfig_3, P_ERROR, 0,
                   "MW NVM: 'midwareconfig.nvm', write the file body failed");
    }

    OsaFclose(fp);
    return;
}


/**
  \fn           void mwSetAtChanConfigItemValueStatic(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set one AT channel config item value, but not save to flash, need caller call mwSaveAtChanConfig() to write to flash
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \param[out]   bChanged    return value, whether NVM changed, if return TRUE, caller need to write to flash
  \returns      void
*/
static void mwSetAtChanConfigItemValueStatic(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value, BOOL *bChanged)
{
    MidWareAtChanConfig *pAtChanCfg = PNULL;
    UINT32  tmpIdx = 0;

    OsaCheck(bChanged != PNULL, bChanged, cfgEnum, value);

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    /*
     * find right config
    */
    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        if (mwNvmConfig.atChanConfig[tmpIdx].chanId == chanId)
        {
            pAtChanCfg = &(mwNvmConfig.atChanConfig[tmpIdx]);
            break;
        }
    }

    OsaDebugBegin(pAtChanCfg != PNULL, chanId, mwNvmConfig.atChanConfig[0].chanId, cfgEnum);
    return;
    OsaDebugEnd();

    *bChanged = FALSE;

    switch (cfgEnum)
    {
        case MID_WARE_AT_CHAN_ECHO_CFG:
            if (value != pAtChanCfg->echoValue)
            {
                if (value <= 1)  // 1 bit
                {
                    pAtChanCfg->echoValue = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->echoValue);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_SUPPRESS_CFG:
            if (value != pAtChanCfg->suppressionValue)
            {
                if (value <= 1) //1 bit
                {
                    pAtChanCfg->suppressionValue = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->suppressionValue);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_CREG_RPT_CFG:
            if (value != pAtChanCfg->cregRptMode)
            {
                if (value <= 3) //CmiMmCregModeEnum
                {
                    pAtChanCfg->cregRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->cregRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_CEREG_RPT_CFG:
            if (value != pAtChanCfg->ceregRptMode)
            {
                if (value <= 5) //CmiPsCeregModeEnum
                {
                    pAtChanCfg->ceregRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ceregRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_EDRX_RPT_CFG:
            if (value != pAtChanCfg->needEdrxRpt)
            {
                if (value <= 1) //1 bit
                {
                    pAtChanCfg->needEdrxRpt = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->needEdrxRpt);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG:
            if (value != pAtChanCfg->needCiotOptRpt)
            {
                if (value <= 7) //3 bit
                {
                    pAtChanCfg->needCiotOptRpt = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->needCiotOptRpt);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_CSCON_RPT_CFG:
            if (value != pAtChanCfg->csconRptMode)
            {
                if (value <= 3)
                {
                    pAtChanCfg->csconRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->needCiotOptRpt);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_SMS_MODE_CFG:
            if (value != pAtChanCfg->smsSendMode)
            {
                if (value <= 1)  // 1 bit
                {
                    pAtChanCfg->smsSendMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->smsSendMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG:
            if (value != pAtChanCfg->timeZoneRptMode)
            {
                if (value <= 3)  // 2 bit
                {
                    pAtChanCfg->timeZoneRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->timeZoneRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_TIME_ZONE_UPD_CFG:
            if (value != pAtChanCfg->timeZoneUpdMode)
            {
                if (value <= 1)  // 1 bit
                {
                    pAtChanCfg->timeZoneUpdMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->timeZoneUpdMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_CMEE_CFG:
            if (value != pAtChanCfg->cmeeMode)
            {
                if (value <= 2)  //
                {
                    pAtChanCfg->cmeeMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->cmeeMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_ECCESQ_RPT_CFG:
            if (value != pAtChanCfg->eccesqRptMode)
            {
                if (value <= 2)  //
                {
                    pAtChanCfg->eccesqRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->eccesqRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_CGEREP_MODE_CFG:
            if (value != pAtChanCfg->cgerepMode)
            {
                if (value <= 1)  //
                {
                    pAtChanCfg->cgerepMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->cgerepMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_PSM_RPT_CFG:
            if (value != pAtChanCfg->ecpsmRptMode)
            {
                if (value <= 1)  //
                {
                    pAtChanCfg->ecpsmRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecpsmRptMode);
                    OsaDebugEnd();
                }
            }
            break;
        case MID_WARE_AT_CHAN_EMM_TIME_RPT_CFG:
            if (value != pAtChanCfg->ecemmtimeRptMode)
            {
                if (value <= 7)  // 3 bit
                {
                    pAtChanCfg->ecemmtimeRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecemmtimeRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG:
            if (value != pAtChanCfg->ecPtwEdrxRpt)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecPtwEdrxRpt = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecPtwEdrxRpt);
                    OsaDebugEnd();
                }
            }
            break;


        case MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG:
            if (value != pAtChanCfg->ecpinRptMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecpinRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecpinRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_ECPADDR_RPT_CFG:
            if (value != pAtChanCfg->ecpaddrRptMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecpaddrRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecpaddrRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG:
            if (value != pAtChanCfg->ecpcfunRptMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecpcfunRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecpcfunRptMode);
                    OsaDebugEnd();
                }
            }
            break;


        case MID_WARE_AT_CHAN_ECLED_MODE_CFG:
            if (value != pAtChanCfg->ecledMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecledMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecledMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG:
            if (value != pAtChanCfg->ecHibRptMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecHibRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecHibRptMode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG:
            if (value != pAtChanCfg->ecTimeZoneUpdMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecTimeZoneUpdMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecTimeZoneUpdMode);
                    OsaDebugEnd();
                }
            }
            break;

        default:
            OsaDebugBegin(FALSE, cfgEnum, value, chanId);
            OsaDebugEnd();
            break;
    }

    return;
}

/**
  \fn           void mwSetAndSaveAtChanConfigItemValueStatic(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set one AT channel config item value, and save into NVM (flash)
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \returns      void
*/
static void mwSetAndSaveAtChanConfigItemValueStatic(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
{
    BOOL    nvmChanged = FALSE;

    mwSetAtChanConfigItemValueStatic(chanId, cfgEnum, value, &nvmChanged);

    if (nvmChanged)
    {
        mwSaveNvmConfig();
    }

    return;
}

/**
  \fn           UINT32 mwGetAtChanConfigItemValueStatic(UINT8 chanId, MidWareATChanCfgEnum cfgEnum)
  \brief        Get/return one AT channel config value
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item value need to return
  \returns      config value
*/
static UINT32 mwGetAtChanConfigItemValueStatic(UINT8 chanId, MidWareATChanCfgEnum cfgEnum)
{
    MidWareAtChanConfig *pAtChanCfg = PNULL;
    UINT32  tmpIdx = 0;

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    /*
     * find right config
    */
    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        if (mwNvmConfig.atChanConfig[tmpIdx].chanId == chanId)
        {
            pAtChanCfg = &(mwNvmConfig.atChanConfig[tmpIdx]);
            break;
        }
    }


    OsaDebugBegin(pAtChanCfg != PNULL, chanId, mwNvmConfig.atChanConfig[0].chanId, cfgEnum);
    return 0xEFFFFFFF;      //most case, it's a invalid value
    OsaDebugEnd();

    switch(cfgEnum)
    {
        case MID_WARE_AT_CHAN_ECHO_CFG:
            return pAtChanCfg->echoValue;

        case MID_WARE_AT_CHAN_SUPPRESS_CFG:
            return pAtChanCfg->suppressionValue;

        case MID_WARE_AT_CHAN_CREG_RPT_CFG:
            return pAtChanCfg->cregRptMode;

        case MID_WARE_AT_CHAN_CEREG_RPT_CFG:
            return pAtChanCfg->ceregRptMode;

        case MID_WARE_AT_CHAN_EDRX_RPT_CFG:
            return pAtChanCfg->needEdrxRpt;

        case MID_WARE_AT_CHAN_CIOT_OPT_RPT_CFG:
            return pAtChanCfg->needCiotOptRpt;

        case MID_WARE_AT_CHAN_CSCON_RPT_CFG:
            return pAtChanCfg->csconRptMode;

        case MID_WARE_AT_CHAN_SMS_MODE_CFG:
            return pAtChanCfg->smsSendMode;

        case MID_WARE_AT_CHAN_TIME_ZONE_REP_CFG:
            return pAtChanCfg->timeZoneRptMode;

        case MID_WARE_AT_CHAN_TIME_ZONE_UPD_CFG:
            return pAtChanCfg->timeZoneUpdMode;

        case MID_WARE_AT_CHAN_CMEE_CFG:
            return pAtChanCfg->cmeeMode;

        case MID_WARE_AT_CHAN_ECCESQ_RPT_CFG:
            return pAtChanCfg->eccesqRptMode;

        case MID_WARE_AT_CHAN_CGEREP_MODE_CFG:
            return pAtChanCfg->cgerepMode;

        case MID_WARE_AT_CHAN_PSM_RPT_CFG:
            return pAtChanCfg->ecpsmRptMode;

        case MID_WARE_AT_CHAN_EMM_TIME_RPT_CFG:
            return pAtChanCfg->ecemmtimeRptMode;

        case MID_WARE_AT_CHAN_PTW_EDRX_RPT_CFG:
            return pAtChanCfg->ecPtwEdrxRpt;

        case MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG:
            return pAtChanCfg->ecpinRptMode;

        case MID_WARE_AT_CHAN_ECPADDR_RPT_CFG:
            return pAtChanCfg->ecpaddrRptMode;

        case MID_WARE_AT_CHAN_ECPCFUN_RPT_CFG:
            return pAtChanCfg->ecpcfunRptMode;

        case MID_WARE_AT_CHAN_ECLED_MODE_CFG:
            return pAtChanCfg->ecledMode;

        case MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG:
            return pAtChanCfg->ecHibRptMode;

        case MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG:
            return pAtChanCfg->ecTimeZoneUpdMode;

        default:
            break;
    }

    OsaDebugBegin(FALSE, chanId, cfgEnum, 0);
    OsaDebugEnd();

    return 0xEFFFFFFF;      //most case, it's a invalid value
}


/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/**
  \fn           void mwLoadNvmConfig(void)
  \brief        load/read middle ware NVM file, if not exist, set & write the default value
  \param[in]    void
  \returns      void
*/
void mwLoadNvmConfig(void)
{
    OSAFILE fp = PNULL;
    UINT32  readCount = 0;
    MidWareNvmHeader    mwNvmHdr;   //4 bytes
    UINT8   crcCheck = 0;
    BOOL    needAdjust = FALSE, adjustOK = FALSE;
    void    *pReadNvmConfig = (void *)&mwNvmConfig;

    if (mwNvmRead)
    {
        return;
    }

    /*
     * open the NVM file
    */
    fp = OsaFopen(MID_WARE_NVM_FILE_NAME, "rb");   //read only
    if (fp == PNULL)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_1, P_ERROR, 0,
                    "MW NVM, can't open NVM: 'midwareconfig.nvm', use the defult value");

        mwSetDefaultNvmConfig();
        mwSaveNvmConfig();
        return;
    }

    /*
     * read the file header
    */
    readCount = OsaFread(&mwNvmHdr, sizeof(MidWareNvmHeader), 1, fp);
    if (readCount != 1)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_header_e_1, P_ERROR, 1,
                    "MW NVM: 'midwareconfig.nvm', can't read header, return: %d, use the defult value", readCount);

        OsaFclose(fp);
        mwSetDefaultNvmConfig();
        mwSaveNvmConfig();

        return;
    }

    if (mwNvmHdr.version != MID_WARE_NVM_FILE_LATEST_VERSION)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_ver_w_1, P_ERROR, 2,
                    "MW NVM: 'midwareconfig.nvm', version: %d not latest: %d, try to adapt",
                    mwNvmHdr.version, MID_WARE_NVM_FILE_LATEST_VERSION);

        if (mwNvmHdr.fileBodySize > 1024)   //in fact this NVM size should limited in 1KB
        {
            ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_ver_e_1, P_ERROR, 2,
                        "MW NVM: 'midwareconfig.nvm', version: %d, NVM body size: %d > 1024, not right, use the default value",
                        mwNvmHdr.version, mwNvmHdr.fileBodySize);

            OsaFclose(fp);
            mwSetDefaultNvmConfig();
            mwSaveNvmConfig();

            return;
        }

        /*
         * As need to do adaption, can't read the old NVM into "mwNvmConfig", here we allocate a new buffer to store it
        */
        pReadNvmConfig  = OsaAllocZeroMemory(mwNvmHdr.fileBodySize);

        if (pReadNvmConfig == PNULL)
        {
            ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_buff_e_1, P_ERROR, 2,
                        "MW NVM: 'midwareconfig.nvm', version: %d, can not allo memsize: %d, use the default value",
                        mwNvmHdr.version, mwNvmHdr.fileBodySize);

            OsaFclose(fp);
            mwSetDefaultNvmConfig();
            mwSaveNvmConfig();

            return;
        }

        needAdjust = TRUE;
    }
    else if (mwNvmHdr.fileBodySize != sizeof(mwNvmConfig))  //file version is the same, but NVM file size not right
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_2, P_ERROR, 2,
                    "MW NVM: 'midwareconfig.nvm', file body size not right: (%u/%u), use the defult value",
                    mwNvmHdr.fileBodySize, sizeof(mwNvmConfig));

        OsaFclose(fp);

        mwSetDefaultNvmConfig();
        mwSaveNvmConfig();

        return;
    }

    /*
     * read the file body
    */
    readCount = OsaFread(pReadNvmConfig, mwNvmHdr.fileBodySize, 1, fp);
    crcCheck = OsaCalcCrcValue((UINT8 *)pReadNvmConfig, mwNvmHdr.fileBodySize);

    if (readCount != 1 ||
        crcCheck != mwNvmHdr.checkSum)
    {
        ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwLoadNvmConfig_3, P_ERROR, 2,
                    "MW NVM: 'midwareconfig.nvm', can't read body, or body not right, (%u/%u), use the defult value",
                    crcCheck, mwNvmHdr.checkSum);

        OsaFclose(fp);
        mwSetDefaultNvmConfig();
        mwSaveNvmConfig();

        if (needAdjust)
        {
            OsaFreeMemory(&pReadNvmConfig);
        }
        return;
    }

    if (needAdjust)
    {
        OsaCheck(pReadNvmConfig != &mwNvmConfig, pReadNvmConfig, mwNvmHdr.version, 0);

        adjustOK = mwAdjustNvmConfigVerion(mwNvmHdr.version, pReadNvmConfig, mwNvmHdr.fileBodySize, &mwNvmConfig);

        if (adjustOK != TRUE)   //adjust fail, use default value
        {
            mwSetDefaultNvmConfig();
        }

        OsaFclose(fp);
        mwSaveNvmConfig();

        /* free memory */
        OsaFreeMemory(&pReadNvmConfig);
    }
    else// no error and no needAdjust
    {
        OsaFclose(fp);
    }

    mwNvmRead = TRUE;

    return;
}


/**
  \fn           UINT32 mwGetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum)
  \brief        Get/return one AT channel config value
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item value need to return
  \returns      config value
*/
UINT32 mwGetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum)
{
    #ifdef  FEATURE_REF_AT_ENABLE
    if (cfgEnum == MID_WARE_AT_CHAN_ECHO_CFG ||
        cfgEnum == MID_WARE_AT_CHAN_SUPPRESS_CFG)   //REF ATE also stored in NVM, and not support ATQ
    {
        return mwGetAtChanConfigItemValueStatic(chanId, cfgEnum);
    }
    else
    {
        return mwAonGetAtChanConfigItemValue(chanId, cfgEnum);
    }
    #else
    return mwGetAtChanConfigItemValueStatic(chanId, cfgEnum);
    #endif
}


/**
  \fn           void mwSetAndSaveAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set one AT channel config item value, and save into NVM (flash) / Aon
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \returns      void
*/
void mwSetAndSaveAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
{
    #ifdef  FEATURE_REF_AT_ENABLE
    if (cfgEnum == MID_WARE_AT_CHAN_ECHO_CFG ||
        cfgEnum == MID_WARE_AT_CHAN_SUPPRESS_CFG)   //REF ATE also stored in NVM, and not support ATQ
    {
        mwSetAndSaveAtChanConfigItemValueStatic(chanId, cfgEnum, value);
    }
    else
    {
        mwAonSetAndSaveAtChanConfigItemValue(chanId, cfgEnum, value);
    }
    #else
    mwSetAndSaveAtChanConfigItemValueStatic(chanId, cfgEnum, value);
    #endif
}

/**
  \fn           void mwSetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set one AT channel config item value, but not save to flash/Aon, need caller call mwSaveNvmConfig() to write to flash/Aon
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \param[out]   bChanged    return value, whether NVM changed, if return TRUE, caller need to write to flash
  \returns      void
*/
void mwSetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value, BOOL *bChanged)
{
    #ifdef  FEATURE_REF_AT_ENABLE
    mwAonSetAtChanConfigItemValue(chanId, cfgEnum, value, bChanged);
    #else
    mwSetAtChanConfigItemValueStatic(chanId, cfgEnum, value, bChanged);
    #endif

    return;
}

/**
  \fn           void mwSaveAtChanConfig(void)
  \brief        Save middle ware NVM/AON config, if some AT channel config changes
  \param[in]    void
  \returns      void
*/
void mwSaveAtChanConfig(void)
{
    #ifdef  FEATURE_REF_AT_ENABLE
    // REF AT channel config saved in AON
    mwAonSaveConfig();
    #else
    mwSaveNvmConfig();
    #endif
}


/**
  \fn           BOOL mwGetCsmpConfig(UINT8 chanId, MidWareCSMPParam *config)
  \brief        Get/return all csmp config value
  \param[in]    chanId      channel ID
  \param[out]   config      CSMP parameters
  \returns      BOOL
*/
BOOL mwGetCsmpConfig(UINT8 chanId, MidWareCSMPParam *config)
{
    MidWareAtChanConfig *pAtChanCfg = PNULL;
    UINT32  tmpIdx = 0;

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    /*
     * find right config
    */
    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        if (mwNvmConfig.atChanConfig[tmpIdx].chanId == chanId)
        {
            pAtChanCfg = &(mwNvmConfig.atChanConfig[tmpIdx]);
            break;
        }
    }

    OsaDebugBegin(pAtChanCfg != PNULL, chanId, mwNvmConfig.atChanConfig[0].chanId, 0);
    return FALSE;
    OsaDebugEnd();

    memcpy(config, &(pAtChanCfg->textSmsParam), sizeof(MidWareCSMPParam));

    return TRUE;
}


/**
  \fn           mwSetAndSaveCsmpConfig(UINT8 chanId, MidWareSetCSMPParam *pSetCsmp)
  \brief        Set/save all CSMP config value
  \param[in]    chanId      channel ID
  \param[out]   pSetCsmp    CSMP parameters
*/
void mwSetAndSaveCsmpConfig(UINT8 chanId, MidWareSetCSMPParam *pSetCsmp)
{
    BOOL    nvmChanged = FALSE;
    MidWareAtChanConfig *pAtChanCfg = PNULL;
    UINT32  tmpIdx = 0;

    OsaDebugBegin(pSetCsmp->foPresent == TRUE || pSetCsmp->vpPresent == TRUE ||
                  pSetCsmp->pidPresent == TRUE || pSetCsmp->dcsPresent == TRUE,
                  chanId, pSetCsmp->csmpParam.fo, pSetCsmp->csmpParam.dcs);
    return;
    OsaDebugEnd();

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    /*
     * find right config
    */
    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        if (mwNvmConfig.atChanConfig[tmpIdx].chanId == chanId)
        {
            pAtChanCfg = &(mwNvmConfig.atChanConfig[tmpIdx]);
            break;
        }
    }

    OsaDebugBegin(pAtChanCfg != PNULL, chanId, mwNvmConfig.atChanConfig[0].chanId, 0);
    return;
    OsaDebugEnd();

    if (pSetCsmp->foPresent &&
        pSetCsmp->csmpParam.fo != pAtChanCfg->textSmsParam.fo)
    {
        pAtChanCfg->textSmsParam.fo = pSetCsmp->csmpParam.fo;
        nvmChanged = TRUE;
    }

    if (pSetCsmp->vpPresent &&
        pSetCsmp->csmpParam.vp != pAtChanCfg->textSmsParam.vp)
    {
        pAtChanCfg->textSmsParam.vp = pSetCsmp->csmpParam.vp;
        nvmChanged = TRUE;
    }

    if (pSetCsmp->pidPresent &&
        pSetCsmp->csmpParam.pid != pAtChanCfg->textSmsParam.pid)
    {
        pAtChanCfg->textSmsParam.pid = pSetCsmp->csmpParam.pid;
        nvmChanged = TRUE;
    }

    if (pSetCsmp->dcsPresent &&
        pSetCsmp->csmpParam.dcs != pAtChanCfg->textSmsParam.dcs)
    {
        pAtChanCfg->textSmsParam.dcs = pSetCsmp->csmpParam.dcs;
        nvmChanged = TRUE;
    }

    if (nvmChanged)
    {
        mwSaveNvmConfig();
    }

    return;
}

/**
  \fn           void mwGetAllDMConfig(MidWareDmConfig* config)
  \brief        Get/return all DM config value
  \param[out]   dm config
*/
void mwGetAllDMConfig(MidWareDmConfig* config)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    config->mode = mwNvmConfig.dmConfig.mode;

    config->test = mwNvmConfig.dmConfig.test;

    strcpy(config->serverIP, mwNvmConfig.dmConfig.serverIP);
    strcpy(config->serverPort, mwNvmConfig.dmConfig.serverPort);
    config->lifeTime = mwNvmConfig.dmConfig.lifeTime;

    strcpy(config->appKey, mwNvmConfig.dmConfig.appKey);
    strcpy(config->secret, mwNvmConfig.dmConfig.secret);
    config->ifType = mwNvmConfig.dmConfig.ifType;
    strcpy(config->tmlIMEI, mwNvmConfig.dmConfig.tmlIMEI);

    strcpy(config->PLMN1, mwNvmConfig.dmConfig.PLMN1);
    strcpy(config->PLMN2, mwNvmConfig.dmConfig.PLMN2);
    strcpy(config->PLMN3, mwNvmConfig.dmConfig.PLMN3);

    config->queryOpt = mwNvmConfig.dmConfig.queryOpt;
    config->eraseOpt = mwNvmConfig.dmConfig.eraseOpt;
    config->DMstate = mwNvmConfig.dmConfig.DMstate;

    //strcpy(config->appInfo, mwNvmConfig.dmConfig.appInfo);
    //strcpy(config->MAC, mwNvmConfig.dmConfig.MAC);
    //strcpy(config->ROM, mwNvmConfig.dmConfig.ROM);
    //strcpy(config->RAM, mwNvmConfig.dmConfig.RAM);
    //strcpy(config->CPU, mwNvmConfig.dmConfig.CPU);
    //strcpy(config->osSysVer, mwNvmConfig.dmConfig.osSysVer);
    //strcpy(config->swVer, mwNvmConfig.dmConfig.swVer);
    //strcpy(config->swName, mwNvmConfig.dmConfig.swName);
    //strcpy(config->VoLTE, mwNvmConfig.dmConfig.VoLTE);
    //strcpy(config->netType, mwNvmConfig.dmConfig.netType);
    strcpy(config->account, mwNvmConfig.dmConfig.account);
    strcpy(config->phoneNum, mwNvmConfig.dmConfig.phoneNum);
    //strcpy(config->location, mwNvmConfig.dmConfig.location);
}

/**
  \fn           UINT8 mwGetDMConfigItemValue(MidWareDMCfgEnum cfgEnum)
  \brief        Get/return DM config value
  \param[in]    cfgEnum     which item value need to return
  \returns      config value
UINT16* mwGetDMConfigItemValue(MidWareDMCfgEnum cfgEnum)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    switch(cfgEnum)
    {
        case MID_WARE_AT_DM_MODE_CFG:
            return (UINT16*)&mwNvmConfig.dmConfig.mode;

        case MID_WARE_AT_DM_LIFETIME_CFG:
            return (UINT16*)&mwNvmConfig.dmConfig.lifeTime;

        case MID_WARE_AT_DM_APPKEY_CFG:
            return (UINT8*)mwNvmConfig.dmConfig.appKey;

        case MID_WARE_AT_DM_SECRET_CFG:
            return (UINT8*)mwNvmConfig.dmConfig.secret;

        case MID_WARE_AT_DM_TEST_CFG:
            return &mwNvmConfig.dmConfig.test;

        default:
            break;
    }

    return 0;
}
*/

#if 0
/**
  \fn           void mwSetDMConfigItemValue(MidWareDMCfgEnum cfgEnum, UINT8* value, BOOL *bChanged)
  \brief        Set DM config item value, but not save to flash, need caller call mwSaveNvmConfig() to write to flash
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \param[out]   bChanged    return value, whether NVM changed, if return TRUE, caller need to write to flash
  \returns      void
*/
void mwSetDMConfigItemValue(MidWareDMCfgEnum cfgEnum, UINT8* value, BOOL *bChanged)
{

    OsaCheck(bChanged != PNULL, bChanged, cfgEnum, *value);

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }


    *bChanged = FALSE;

    switch (cfgEnum)
    {
        case MID_WARE_AT_DM_MODE_CFG:
            if (*value != mwNvmConfig.dmConfig.mode)
            {
                if (*value <= 1)  // 1 bit
                {
                    mwNvmConfig.dmConfig.mode = *value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, *value, mwNvmConfig.dmConfig.mode);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_DM_LIFETIME_CFG:
            if (*value != mwNvmConfig.dmConfig.lifeTime)
            {
                if (*value <= 0xFFFFFFFF) //4 byte
                {
                    mwNvmConfig.dmConfig.lifeTime = *value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, *value, mwNvmConfig.dmConfig.lifeTime);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_DM_APPKEY_CFG:
            if (strcmp((CHAR*)value, mwNvmConfig.dmConfig.appKey) != NULL)
            {
                if (strlen((CHAR*)value) < 11) //max 10 byte
                {
                    strcpy((CHAR*)mwNvmConfig.dmConfig.appKey,(CHAR*)value);
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, *value, mwNvmConfig.dmConfig.appKey);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_DM_SECRET_CFG:
            if (strcmp((CHAR*)value, mwNvmConfig.dmConfig.secret) != NULL)
            {
                if (strlen((CHAR*)value) < 33) //max 32 byte
                {
                    strcpy((CHAR*)mwNvmConfig.dmConfig.secret,(CHAR*)value);
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, *value, mwNvmConfig.dmConfig.secret);
                    OsaDebugEnd();
                }
            }
            break;

        case MID_WARE_AT_DM_TEST_CFG:
            if (*value != mwNvmConfig.dmConfig.test)
            {
                if (*value <= 1) //1 bit
                {
                    mwNvmConfig.dmConfig.test = *value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, *value, mwNvmConfig.dmConfig.test);
                    OsaDebugEnd();
                }
            }
            break;

        default:
            break;
    }

    return;
}

/**
  \fn           void mwSetAndSaveDMConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set DM config item value, and save into NVM (flash)
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \returns      void
*/
void mwSetAndSaveDMConfigItemValue(MidWareDMCfgEnum cfgEnum, UINT8* value)
{
    BOOL    nvmChanged = FALSE;

    mwSetDMConfigItemValue(cfgEnum, value, &nvmChanged);

    if (nvmChanged)
    {
        mwSaveNvmConfig();
    }

    return;
}
#endif
/**
  \fn           void mwSetAllDMConfig(MidWareDmConfig* config, BOOL *bChanged)
  \brief        Set DM config item value, but not save to flash, need caller call mwSaveNvmConfig() to write to flash
  \param[in]    value       item value
  \param[out]   bChanged    return value, whether NVM changed, if return TRUE, caller need to write to flash
  \returns      void
*/
void mwSetAllDMConfig(MidWareDmConfig* config, BOOL *bChanged)
{

    OsaCheck(bChanged != PNULL, bChanged, config->mode, config->lifeTime);

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }


    *bChanged = FALSE;

    if (config->mode != mwNvmConfig.dmConfig.mode)
    {
        if (config->mode <= 1)  // 1 bit
        {
            mwNvmConfig.dmConfig.mode = config->mode;
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->mode, mwNvmConfig.dmConfig.mode, 0);
            OsaDebugEnd();
        }
    }
    if (config->test!= mwNvmConfig.dmConfig.test)
    {
        if (config->test <= 1) //1 bit
        {
            mwNvmConfig.dmConfig.test = config->test;
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->test, mwNvmConfig.dmConfig.test, 0);
            OsaDebugEnd();
        }
    }

    if (strcmp(config->serverIP, mwNvmConfig.dmConfig.serverIP) != NULL)
    {
        if (strlen(config->serverIP) < MID_WARE_IPV6_ADDR_LEN)
        {
            memset(mwNvmConfig.dmConfig.serverIP, 0, MID_WARE_IPV6_ADDR_LEN);
            strcpy(mwNvmConfig.dmConfig.serverIP, (CHAR*)config->serverIP);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->serverIP, mwNvmConfig.dmConfig.serverIP, 0);
            OsaDebugEnd();
        }
    }

    if (strcmp(config->serverPort, mwNvmConfig.dmConfig.serverPort) != NULL)
    {
        if (strlen(config->serverPort) < MID_WARE_DM_PORT_LEN)
        {
            memset(mwNvmConfig.dmConfig.serverPort, 0, MID_WARE_DM_PORT_LEN);
            strcpy(mwNvmConfig.dmConfig.serverPort,(CHAR*)config->serverPort);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->serverPort, mwNvmConfig.dmConfig.serverPort, 0);
            OsaDebugEnd();
        }
    }

    if (config->lifeTime != mwNvmConfig.dmConfig.lifeTime)
    {
        mwNvmConfig.dmConfig.lifeTime = config->lifeTime;
        *bChanged = TRUE;
    }

    if (strcmp(config->appKey, mwNvmConfig.dmConfig.appKey) != NULL)
    {
        if (strlen(config->appKey) < MID_WARE_DM_APPKEY_LEN)
        {
            memset(mwNvmConfig.dmConfig.appKey, 0, MID_WARE_DM_APPKEY_LEN);
            strcpy(mwNvmConfig.dmConfig.appKey,(CHAR*)config->appKey);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->appKey, mwNvmConfig.dmConfig.appKey, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->secret, mwNvmConfig.dmConfig.secret) != NULL)
    {
        if (strlen(config->secret) < MID_WARE_DM_SECRET_LEN)
        {
            memset(mwNvmConfig.dmConfig.secret, 0, MID_WARE_DM_SECRET_LEN);
            strcpy(mwNvmConfig.dmConfig.secret, config->secret);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->secret, mwNvmConfig.dmConfig.secret, 0);
            OsaDebugEnd();
        }
    }
    if (config->ifType != mwNvmConfig.dmConfig.ifType)
    {
        mwNvmConfig.dmConfig.ifType = config->ifType;
        *bChanged = TRUE;
    }
    if (strcmp(config->tmlIMEI, mwNvmConfig.dmConfig.tmlIMEI) != NULL)
    {
        if (strlen(config->tmlIMEI) < MID_WARE_DM_TML_IMEI_LEN)
        {
            memset(mwNvmConfig.dmConfig.tmlIMEI, 0, MID_WARE_DM_TML_IMEI_LEN);
            strcpy(mwNvmConfig.dmConfig.tmlIMEI, config->tmlIMEI);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->tmlIMEI, mwNvmConfig.dmConfig.tmlIMEI, 0);
            OsaDebugEnd();
        }
    }

    if (strcmp(config->PLMN1, mwNvmConfig.dmConfig.PLMN1) != NULL)
    {
        if (strlen(config->PLMN1) < MID_WARE_DM_PLMN_LEN)
        {
            memset(mwNvmConfig.dmConfig.PLMN1, 0, MID_WARE_DM_PLMN_LEN);
            strcpy(mwNvmConfig.dmConfig.PLMN1, config->PLMN1);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->PLMN1, mwNvmConfig.dmConfig.PLMN1, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->PLMN2, mwNvmConfig.dmConfig.PLMN2) != NULL)
    {
        if (strlen(config->PLMN2) < MID_WARE_DM_PLMN_LEN)
        {
            memset(mwNvmConfig.dmConfig.PLMN2, 0, MID_WARE_DM_PLMN_LEN);
            strcpy(mwNvmConfig.dmConfig.PLMN2, config->PLMN2);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->PLMN2, mwNvmConfig.dmConfig.PLMN2, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->PLMN3, mwNvmConfig.dmConfig.PLMN3) != NULL)
    {
        if (strlen(config->PLMN3) < MID_WARE_DM_PLMN_LEN)
        {
            memset(mwNvmConfig.dmConfig.PLMN3, 0, MID_WARE_DM_PLMN_LEN);
            strcpy(mwNvmConfig.dmConfig.PLMN3, config->PLMN3);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->PLMN3, mwNvmConfig.dmConfig.PLMN3, 0);
            OsaDebugEnd();
        }
    }

    if (config->queryOpt != mwNvmConfig.dmConfig.queryOpt)
    {
        mwNvmConfig.dmConfig.queryOpt = config->queryOpt;
        *bChanged = TRUE;
    }
    if (config->eraseOpt != mwNvmConfig.dmConfig.eraseOpt)
    {
        mwNvmConfig.dmConfig.eraseOpt = config->eraseOpt;
        *bChanged = TRUE;
    }
    if (config->DMstate != mwNvmConfig.dmConfig.DMstate)
    {
        mwNvmConfig.dmConfig.DMstate = config->DMstate;
        *bChanged = TRUE;
    }
    /*
    if (strcmp(config->appInfo, mwNvmConfig.dmConfig.appInfo) != NULL)
    {
        if (strlen(config->appInfo) < MID_WARE_DM_APP_INFO_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.appInfo, config->appInfo);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->appInfo, mwNvmConfig.dmConfig.appInfo, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->MAC, mwNvmConfig.dmConfig.MAC) != NULL)
    {
        if (strlen(config->MAC) < MID_WARE_DM_MAC_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.appInfo, config->MAC);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->MAC, mwNvmConfig.dmConfig.MAC, 0);
            OsaDebugEnd();
        }
    }

    if (strcmp(config->ROM, mwNvmConfig.dmConfig.ROM) != NULL)
    {
        if (strlen(config->ROM) < MID_WARE_DM_ROM_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.ROM, config->ROM);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->ROM, mwNvmConfig.dmConfig.ROM, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->RAM, mwNvmConfig.dmConfig.RAM) != NULL)
    {
        if (strlen(config->RAM) < MID_WARE_DM_RAM_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.RAM, config->RAM);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->RAM, mwNvmConfig.dmConfig.RAM, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->CPU, mwNvmConfig.dmConfig.CPU) != NULL)
    {
        if (strlen(config->CPU) < MID_WARE_DM_CPU_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.CPU, config->CPU);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->CPU, mwNvmConfig.dmConfig.CPU, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->osSysVer, mwNvmConfig.dmConfig.osSysVer) != NULL)
    {
        if (strlen(config->osSysVer) < MID_WARE_DM_OSSYSVER_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.osSysVer, config->osSysVer);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->osSysVer, mwNvmConfig.dmConfig.osSysVer, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->swVer, mwNvmConfig.dmConfig.swVer) != NULL)
    {
        if (strlen(config->swVer) < MID_WARE_DM_SWVER_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.swVer, config->swVer);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->swVer, mwNvmConfig.dmConfig.swVer, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->swName, mwNvmConfig.dmConfig.swName) != NULL)
    {
        if (strlen(config->swName) < MID_WARE_DM_SWNAME_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.swName, config->swName);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->swName, mwNvmConfig.dmConfig.swName, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->VoLTE, mwNvmConfig.dmConfig.VoLTE) != NULL)
    {
        if (strlen(config->VoLTE) < MID_WARE_DM_VOLTE_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.VoLTE, config->VoLTE);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->VoLTE, mwNvmConfig.dmConfig.VoLTE, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->netType, mwNvmConfig.dmConfig.netType) != NULL)
    {
        if (strlen(config->netType) < MID_WARE_DM_NET_TYPE_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.netType, config->netType);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->netType, mwNvmConfig.dmConfig.netType, 0);
            OsaDebugEnd();
        }
    }
    */
    if (strcmp(config->account, mwNvmConfig.dmConfig.account) != NULL)
    {
        if (strlen(config->account) < MID_WARE_DM_ACCOUNT_LEN)
        {
            memset(mwNvmConfig.dmConfig.account, 0, MID_WARE_DM_ACCOUNT_LEN);
            strcpy(mwNvmConfig.dmConfig.account, config->account);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->account, mwNvmConfig.dmConfig.account, 0);
            OsaDebugEnd();
        }
    }
    if (strcmp(config->phoneNum, mwNvmConfig.dmConfig.phoneNum) != NULL)
    {
        if (strlen(config->phoneNum) < MID_WARE_DM_PHONE_NUM_LEN)
        {
            memset(mwNvmConfig.dmConfig.phoneNum, 0, MID_WARE_DM_PHONE_NUM_LEN);
            strcpy(mwNvmConfig.dmConfig.phoneNum, config->phoneNum);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->phoneNum, mwNvmConfig.dmConfig.phoneNum, 0);
            OsaDebugEnd();
        }
    }
    /*
    if (strcmp(config->location, mwNvmConfig.dmConfig.location) != NULL)
    {
        if (strlen(config->location) < MID_WARE_DM_LOCATION_LEN)
        {
            strcpy(mwNvmConfig.dmConfig.location, config->location);
            *bChanged = TRUE;
        }
        else
        {
            OsaDebugBegin(FALSE, config->location, mwNvmConfig.dmConfig.location, 0);
            OsaDebugEnd();
        }
    }
    */
    return;
}

/**
  \fn           void mwSetAndSaveAllDMConfig(MidWareDmConfig* config)
  \brief        Set/save all DM config value
  \param[in]    config   value
*/
void mwSetAndSaveAllDMConfig(MidWareDmConfig* config)
{
    BOOL    nvmChanged = FALSE;

    mwSetAllDMConfig(config, &nvmChanged);

    if (nvmChanged)
    {
        mwSaveNvmConfig();
    }

    return;
}

/**
  \fn           void mwGetCmccAutoRegEnableFlag()
  \brief
  \param[in]    config   value
*/
UINT8 mwGetCmccAutoRegEnableFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.dmConfig.mode;
}

/**
  \fn           void mwSetAndSaveDefaultDnsConfig(MidWareDefaultDnsCfg* pDnsCfg)
  \brief        Set/save all default DNS config
  \param[in]    pDnsCfg   value
*/
void mwSetAndSaveDefaultDnsConfig(MidWareDefaultDnsCfg *pDnsCfg)
{
    OsaCheck(pDnsCfg != PNULL, pDnsCfg, 0, 0);

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    /* check whether changed */
    if (memcmp(&(mwNvmConfig.defaultDnsCfg), pDnsCfg, sizeof(MidWareDefaultDnsCfg)) != 0)
    {
        memcpy(&(mwNvmConfig.defaultDnsCfg), pDnsCfg, sizeof(MidWareDefaultDnsCfg));
        mwSaveNvmConfig();
    }

    return;
}

/**
  \fn           void mwGetDefaultDnsConfig(MidWareDefaultDnsCfg* pDnsCfg)
  \brief        Get/Read default DNS config info
  \param[out]   pDnsCfg   value
*/
void mwGetDefaultDnsConfig(MidWareDefaultDnsCfg *pDnsCfg)
{
    OsaCheck(pDnsCfg != PNULL, pDnsCfg, 0, 0);

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    memcpy(pDnsCfg, &(mwNvmConfig.defaultDnsCfg), sizeof(MidWareDefaultDnsCfg));

    return;
}


void mwGetPhyAtCmdValue(PhyDebugAtCmdInfo *pPhyDebugAtCmdInfo)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    memcpy(pPhyDebugAtCmdInfo, (PhyDebugAtCmdInfo *)(&(mwNvmConfig.phyAtCmdContent)), sizeof(PhyDebugAtCmdInfo));

    return;

}

void mwSetPhyAtCmdValue(PhyDebugAtCmdInfo *pPhyDebugAtCmdInfo)
{

    memcpy((PhyDebugAtCmdInfo *)(&(mwNvmConfig.phyAtCmdContent)), pPhyDebugAtCmdInfo, sizeof(PhyDebugAtCmdInfo));
    mwSaveNvmConfig();

    return;
}

UINT32 mwGetSimCcidValue(UINT8 *ccid, UINT8 len)
{
    UINT32 ret = 0;

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    if(len != sizeof(mwNvmConfig.autoRegConfig.ccid))
    {
        return ret;
    }

    memcpy(ccid, mwNvmConfig.autoRegConfig.ccid, sizeof(mwNvmConfig.autoRegConfig.ccid));
    ret = 1;

    return ret;
}

UINT32 mwSetSimCcidValue(UINT8 *ccid, UINT8 len)
{
    UINT32 ret = 0;

    if(len != sizeof(mwNvmConfig.autoRegConfig.ccid))
    {
        return ret;
    }
    memcpy(mwNvmConfig.autoRegConfig.ccid, ccid, sizeof(mwNvmConfig.autoRegConfig.ccid));
    mwSaveNvmConfig();
    ret = 1;

    return ret;
}

UINT32 mwGetSimEdrxValue(AtCmdPhyEdrxPara *atCmdPhyEdrxPara)
{
    UINT32 ret = 1;

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    memcpy(atCmdPhyEdrxPara, &mwNvmConfig.phyAtCmdContent.atCmdPhyEdrxPara, sizeof(AtCmdPhyEdrxPara));

    return ret;
}

UINT32 mwSetSimEdrxValue(AtCmdPhyEdrxPara *atCmdPhyEdrxPara)
{
    UINT32 ret = 0;

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }
    
    memcpy(&mwNvmConfig.phyAtCmdContent.atCmdPhyEdrxPara, atCmdPhyEdrxPara, sizeof(AtCmdPhyEdrxPara));
    mwSaveNvmConfig();
    ret = 1;

    return ret;
}

UINT8 mwGetEcAutoRegEnableFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegConfig.autoRegEnableFlag;
}

void mwSetEcAutoRegEnableFlag(UINT8 autoReg)
{
    mwNvmConfig.autoRegConfig.autoRegEnableFlag = autoReg;
    mwSaveNvmConfig();
}

UINT8 *mwGetEcAutoRegModel(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegConfig.model;
}

UINT8 mwSetEcAutoRegModel(UINT8 *model)
{
    UINT32 modelLen = 0;

    if(model != NULL)
    {
        memset((CHAR *)mwNvmConfig.autoRegConfig.model, 0, MID_WARE_MODEL_LEN);
        modelLen = strlen((CHAR *)model);
        if(modelLen < MID_WARE_MODEL_LEN)
        {
            memcpy((CHAR *)mwNvmConfig.autoRegConfig.model, (CHAR *)model, modelLen);
        }
        else
        {
            memcpy((CHAR *)mwNvmConfig.autoRegConfig.model, (CHAR *)model, MID_WARE_MODEL_LEN);
        }
        mwSaveNvmConfig();
        return 1;
    }
    return 0;
}

void mwGetEcSocPublicConfig(UINT8 *mode, UINT8 *publicDlPkgNumMax, UINT16 *publicDlBufferToalSize)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    *mode = mwNvmConfig.ecSocPubCfg.mode;
    *publicDlPkgNumMax = mwNvmConfig.ecSocPubCfg.publicDlPkgNumMax;
    *publicDlBufferToalSize = mwNvmConfig.ecSocPubCfg.publicDlBufferToalSize;

    return;
}

UINT8 mwSetEcSocPublicConfig(UINT8 mode, UINT8 publicDlPkgNumMax, UINT16 publicDlBufferToalSize)
{
    BOOL bSave = FALSE;

    if(mwNvmConfig.ecSocPubCfg.mode != mode)
    {
        mwNvmConfig.ecSocPubCfg.mode = mode;
        bSave = TRUE;
    }

    if(publicDlPkgNumMax != 0xFF && mwNvmConfig.ecSocPubCfg.publicDlPkgNumMax != publicDlPkgNumMax)
    {
        mwNvmConfig.ecSocPubCfg.publicDlPkgNumMax = publicDlPkgNumMax;
        bSave = TRUE;
    }

    if(publicDlBufferToalSize != 0xFFFF && mwNvmConfig.ecSocPubCfg.publicDlBufferToalSize != publicDlBufferToalSize)
    {
        mwNvmConfig.ecSocPubCfg.publicDlBufferToalSize = publicDlBufferToalSize;
        bSave = TRUE;
    }

    if(bSave == TRUE)
    {
        mwSaveNvmConfig();
    }

    return 1;
}

UINT8 *mwGetEcAutoRegSwver(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegConfig.swver;
}

UINT8 mwSetEcAutoRegSwver(UINT8 *swver)
{
    UINT32 swverLen = 0;

    if(swver != NULL)
    {
        memset((CHAR *)mwNvmConfig.autoRegConfig.swver, 0, MID_WARE_SWVER_LEN);
        swverLen = strlen((CHAR *)swver);
        if(swverLen < MID_WARE_SWVER_LEN)
        {
            memcpy((CHAR *)mwNvmConfig.autoRegConfig.swver, (CHAR *)swver, swverLen);
        }
        else
        {
            memcpy((CHAR *)mwNvmConfig.autoRegConfig.swver, (CHAR *)swver, MID_WARE_SWVER_LEN);
        }
        mwSaveNvmConfig();
        return 1;
    }
    return 0;
}

UINT8 *mwGetEcAutoRegAckPrint(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegConfig.autoRegAck;
}

UINT8 mwSetEcAutoRegAckPrint(UINT8 *ack)
{
    UINT32 ackLen = 0;

    if(ack != NULL)
    {
        memset((CHAR *)mwNvmConfig.autoRegConfig.autoRegAck, 0, MID_WARE_REGACK_LEN);
        ackLen = strlen((CHAR *)ack);
        if(ackLen < MID_WARE_REGACK_LEN)
        {
            memcpy((CHAR *)mwNvmConfig.autoRegConfig.autoRegAck, (CHAR *)ack, ackLen);
        }
        else
        {
            memcpy((CHAR *)mwNvmConfig.autoRegConfig.autoRegAck, (CHAR *)ack, MID_WARE_REGACK_LEN);
        }
        mwSaveNvmConfig();
        return 1;
    }
    return 0;
}

UINT8 mwGetCtccAutoRegFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCtccConfig.autoRegFlag;
}

void mwSetCtccAutoRegFlag(UINT8 autoReg)
{
    mwNvmConfig.autoRegCtccConfig.autoRegFlag = autoReg;
    mwSaveNvmConfig();
}

UINT8 mwGetCtccAutoRegEnableFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCtccConfig.autoRegEnableFlag;
}

void mwSetCtccAutoRegEnableFlag(UINT8 autoReg)
{
    mwNvmConfig.autoRegCtccConfig.autoRegEnableFlag = autoReg;
    mwSaveNvmConfig();
}

UINT8 mwGetCuccAutoRegFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCuccConfig.autoRegFlag;
}

void mwSetCuccAutoRegFlag(UINT8 autoReg)
{
    mwNvmConfig.autoRegCuccConfig.autoRegFlag = autoReg;
    mwSaveNvmConfig();
}

UINT8 mwGetCuccAutoRegEnableFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCuccConfig.autoRegEnableFlag;
}

void mwSetCuccAutoRegEnableFlag(UINT8 autoReg)
{
    mwNvmConfig.autoRegCuccConfig.autoRegEnableFlag = autoReg;
    mwSaveNvmConfig();
}

UINT32 mwGetCuccAutoRegLastRegTime(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCuccConfig.autoRegLastRegSuccTime;
}

void mwSetCuccAutoRegLastRegTime(UINT32 autoRegTime)
{
    mwNvmConfig.autoRegCuccConfig.autoRegLastRegSuccTime = autoRegTime;
    mwSaveNvmConfig();
}

UINT32 mwGetCuccAutoRegPastTime(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCuccConfig.autoRegPastTime;
}

void mwSetCuccAutoRegPastTime(UINT32 autoRegTime)
{
    mwNvmConfig.autoRegCuccConfig.autoRegPastTime = autoRegTime;
    mwSaveNvmConfig();
}

UINT32 mwGetCuccAutoRegRang()
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    return mwNvmConfig.autoRegCuccConfig.autoRegRang;
}

void mwSetCuccAutoRegRang(UINT32 autoRegRangTime)
{
    mwNvmConfig.autoRegCuccConfig.autoRegRang = autoRegRangTime;
    mwSaveNvmConfig();
}

UINT32 mwGetTimeSyncFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwGetTimeSyncFlag_1, P_VALUE, 1,
                "MW, Get TimeSyncFlag is %d",
                mwNvmConfig.timeSyncTriggered);

    return mwNvmConfig.timeSyncTriggered;
}

void mwSetTimeSyncFlag(UINT32 timeSyncTriggered)
{
    /* check the value range */
    OsaDebugBegin(timeSyncTriggered == 0 || timeSyncTriggered == 1,
                  timeSyncTriggered, 0, 0);
    return;
    OsaDebugEnd();

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    if (mwNvmConfig.timeSyncTriggered != timeSyncTriggered)
    {
        mwNvmConfig.timeSyncTriggered = timeSyncTriggered;
        mwSaveNvmConfig();
    }

    ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwSetTimeSyncFlag_1, P_VALUE, 1,
                "MW, Set TimeSyncFlag to %d", timeSyncTriggered);

    return;
}

OsaTimeTValue mwGetTimeSecs(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwGetTimeSecs_1, P_VALUE, 1,
                "MW, Get TimeSecs is 0x%x", mwNvmConfig.timeSecs);

    return mwNvmConfig.timeSecs;
}

void mwSetTimeSecs(OsaTimeTValue time)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    if ((mwNvmConfig.timeSecs.UTCsecs != time.UTCsecs) ||
        (mwNvmConfig.timeSecs.CTtimer != time.CTtimer) ||
        (mwNvmConfig.timeSecs.timeZone != time.timeZone))
    {
        mwNvmConfig.timeSyncTriggered = 1;   
        mwNvmConfig.timeSecs = time;
        mwSaveNvmConfig();
    }
    else    //time must be changed
    {
        OsaDebugBegin(FALSE, mwNvmConfig.timeSecs.UTCsecs, time.UTCsecs, 0);
        OsaDebugEnd();
    }

    return;
}

UINT32 mwGetAbupFotaEnableFlag(void)
{
    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwSetAbupFotaEnableFlag_0, P_VALUE, 1,
                "MW, Get abupFotaEnableFlag is %d",
                mwNvmConfig.abupFotaEnableFlag);

    return mwNvmConfig.abupFotaEnableFlag;
}

void mwSetAbupFotaEnableFlag(UINT32 abupFotaEnableFlag)
{
    /* check the value range */
    OsaDebugBegin(abupFotaEnableFlag == 0 || abupFotaEnableFlag == 1,
                  abupFotaEnableFlag, 0, 0);
    return;
    OsaDebugEnd();

    if (mwNvmRead == FALSE)
    {
        mwLoadNvmConfig();
    }

    if (mwNvmConfig.abupFotaEnableFlag != abupFotaEnableFlag)
    {
        mwNvmConfig.abupFotaEnableFlag = abupFotaEnableFlag;
        mwSaveNvmConfig();
    }

    ECOMM_TRACE(UNILOG_PLA_MIDWARE, mwSetAbupFotaEnableFlag_1, P_VALUE, 1,
                "MW, Set abupFotaEnableFlag to %d", abupFotaEnableFlag);

    return;
}


