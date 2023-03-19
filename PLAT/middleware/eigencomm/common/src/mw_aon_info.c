/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    mw_aon_info.c
 * Description:  MiddleWare AON (Always ON) info, this info (memory) could be
 *               recoveried when wake up from HIB/Sleep2, and useless when reboot
 * History:      Rev1.0
 *
 ****************************************************************************/
#include "osasys.h"
#include "mw_aon_info.h"
#include "ps_sms_if.h"


/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__


/*
 * Middleware AON info.
 * Address: PLAT_AON_MID_WARE_MEM_ADDR+4
 * Size:    PMU_AON_MID_WARE_MEM_SIZE
*/
MidWareAonInfo      *pMwAonInfo = PNULL;


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
  \fn           void mwAonSetDefaultConfig(void)
  \brief        set default value of "pMwAonInfo"
  \param[in]    void
  \returns      void
*/
static void mwAonSetDefaultConfig(void)
{
    UINT32  tmpIdx = 0;

    OsaCheck(pMwAonInfo != PNULL, pMwAonInfo, 0, 0);

    memset(pMwAonInfo, 0x00, sizeof(MidWareAonInfo));

    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        pMwAonInfo->atChanConfig[tmpIdx].chanId         = tmpIdx+1;    //index 0 - chanId 1
        pMwAonInfo->atChanConfig[tmpIdx].echoValue      = 1;
        pMwAonInfo->atChanConfig[tmpIdx].suppressionValue = 0;
        pMwAonInfo->atChanConfig[tmpIdx].cregRptMode    = 0;    //CMI_MM_DISABLE_CREG;
        pMwAonInfo->atChanConfig[tmpIdx].ceregRptMode   = 0;    //CMI_PS_DISABLE_CEREG;
        pMwAonInfo->atChanConfig[tmpIdx].needEdrxRpt    = 0;    //need to report EDRX value, when EDRX value changes
        pMwAonInfo->atChanConfig[tmpIdx].needCiotOptRpt = 0;    //CMI_MM_CIOT_OPT_DISABLE_REPORT
        pMwAonInfo->atChanConfig[tmpIdx].smsSendMode    = 1;    //PSIL_SMS_FORMAT_TXT_MODE
        pMwAonInfo->atChanConfig[tmpIdx].csconRptMode   = 0;    //ATC_DISABLE_CSCON_RPT
        pMwAonInfo->atChanConfig[tmpIdx].cmeeMode       = 0;    //ATC_CMEE_NUM_ERROR_CODE
        pMwAonInfo->atChanConfig[tmpIdx].timeZoneRptMode = 0;   //ATC_CTZR_DISABLE_RPT
        pMwAonInfo->atChanConfig[tmpIdx].timeZoneUpdMode = 1;   //ATC_CTZU_ENABLE_UPDATE_TZ_VIA_NITZ
        pMwAonInfo->atChanConfig[tmpIdx].eccesqRptMode  = 0;    //ATC_ECCESQ_DISABLE_RPT
        pMwAonInfo->atChanConfig[tmpIdx].cgerepMode     = 0;    //ATC_CGEREP_DISCARD_OLD_CGEV
        pMwAonInfo->atChanConfig[tmpIdx].ecpsmRptMode   = 0;    //ATC_ECPSMR_DISABLE
        pMwAonInfo->atChanConfig[tmpIdx].ecemmtimeRptMode = 0;  //+ECEMMTIME
        pMwAonInfo->atChanConfig[tmpIdx].ecPtwEdrxRpt = 0;      //AT+ECPTWEDRXS
        pMwAonInfo->atChanConfig[tmpIdx].ecpinRptMode = 0;      //+ECPIN
        pMwAonInfo->atChanConfig[tmpIdx].ecpaddrRptMode = 0;    //+ECPADDR
        pMwAonInfo->atChanConfig[tmpIdx].ecpcfunRptMode = 0;    //+ECPCFUN
        pMwAonInfo->atChanConfig[tmpIdx].ecipinfoRptMode = 0;    //+IPINFO
        pMwAonInfo->atChanConfig[tmpIdx].ecTimeZoneUpdMode = 1;    //+NITZ

        pMwAonInfo->atChanConfig[tmpIdx].textSmsParam.fo = PSIL_MSG_FO_DEFAULT;
        pMwAonInfo->atChanConfig[tmpIdx].textSmsParam.vp = PSIL_MSG_VP_DEFAULT;
        pMwAonInfo->atChanConfig[tmpIdx].textSmsParam.pid = PSIL_MSG_PID_DEFAULT;
        pMwAonInfo->atChanConfig[tmpIdx].textSmsParam.dcs = PSIL_MSG_CODING_DEFAULT;
    }


    //ec soc public setting
    pMwAonInfo->ecSocPubCfg.mode = 1;
    pMwAonInfo->ecSocPubCfg.publicDlBufferToalSize = 2048;
    pMwAonInfo->ecSocPubCfg.publicDlPkgNumMax = 8;

    return;
}


/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/**
  \fn           void mwAonInfoInit(void)
  \brief        Init
  \param[in]    void
  \returns      void
*/
void mwAonInfoInit(void)
{
    UINT32  *pStartMagic = (UINT32 *)(PLAT_AON_MID_WARE_MEM_ADDR);
    UINT32  *pEndMagic   = (UINT32 *)(PLAT_AON_MID_WARE_MEM_ADDR + PMU_AON_MID_WARE_MEM_SIZE - 4);

    pMwAonInfo = (MidWareAonInfo *)(PLAT_AON_MID_WARE_MEM_ADDR + 4);

    OsaCheck(sizeof(MidWareAonInfo)+8 <= PMU_AON_MID_WARE_MEM_SIZE, sizeof(MidWareAonInfo), 0, 0);

    //register hib/sleep2 enter hib callback function
    pmuPreDeepSlpCbRegister(PMU_DEEPSLP_CMS_MOD, cmsSockMgrEnterHibCallback, NULL);

    if (pmuBWakeupFromHib() || pmuBWakeupFromSleep2())
    {
        OsaCheck(*pStartMagic == 0xA3B5C7D9 && *pEndMagic == 0xA3B5C7D9, *pStartMagic, *pEndMagic, pStartMagic);
    }
    else
    {
        // first power on, need set some ddefault value
        *pStartMagic = 0xA3B5C7D9;
        *pEndMagic   = 0xA3B5C7D9;

        mwAonSetDefaultConfig();

        pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
    }

    return;
}

/**
  \fn           void mwAonSaveConfig(void)
  \brief        Aon memory changes, need to remained
  \param[in]    void
  \returns      void
*/
void mwAonSaveConfig(void)
{
    pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
    return;
}

/**
  \fn           UINT32 mwAonGetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum)
  \brief        Get/return one AT channel config value
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item value need to return
  \returns      config value
*/
UINT32 mwAonGetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum)
{
    MidWareAtChanConfig *pAtChanCfg = PNULL;
    UINT32  tmpIdx = 0;

    /*
     * find right config
    */
    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        if (pMwAonInfo->atChanConfig[tmpIdx].chanId == chanId)
        {
            pAtChanCfg = &(pMwAonInfo->atChanConfig[tmpIdx]);
            break;
        }
    }

    OsaDebugBegin(pAtChanCfg != PNULL, chanId, pMwAonInfo->atChanConfig[0].chanId, cfgEnum);
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
        case MID_WARE_AT_CHAN_IPINFO_RPT_MODE_CFG:
            return pAtChanCfg->ecipinfoRptMode;
        case MID_WARE_AT_CHAN_NITZ_TIME_ZONE_CFG:
            return pAtChanCfg->ecTimeZoneUpdMode;
            
        default:
            break;
    }

    OsaDebugBegin(FALSE, chanId, cfgEnum, 0);
    OsaDebugEnd();

    return 0xEFFFFFFF;      //most case, it's a invalid value
}


/**
  \fn           void mwAonGetDefaultAonDnsCfg((MidWareDefaultAonDnsCfg *mwDefaultAonDnsCfg))
  \brief        Get default AON dns cfg
  \param[in]    mwDefaultAonDnsCfg  the point of default AON dns cfg
  \returns     BOOL
*/

BOOL mwAonGetDefaultAonDnsCfg(MidWareDefaultAonDnsCfg *mwDefaultAonDnsCfg)
{

    OsaDebugBegin(pMwAonInfo != PNULL, pMwAonInfo, 0, 0);
    return FALSE;      //most case, it's a invalid value
    OsaDebugEnd();

    memcpy(mwDefaultAonDnsCfg, &pMwAonInfo->mwDefaultAonDns, sizeof(MidWareDefaultAonDnsCfg));

    return TRUE;
}

/**
  \fn           void mwAonSetDefaultAonDnsCfgAndSave((MidWareDefaultAonDnsCfg *mwDefaultAonDnsCfg))
  \brief        Set default AON dns cfg
  \param[in]    mwDefaultAonDnsCfg  default AON dns cfg
  \returns      void
*/
void mwAonSetDefaultAonDnsCfgAndSave(MidWareDefaultAonDnsCfg *mwDefaultAonDnsCfg)
{
    OsaDebugBegin(pMwAonInfo != PNULL && mwDefaultAonDnsCfg != PNULL, pMwAonInfo, mwDefaultAonDnsCfg, 0);
    return ;      //most case, it's a invalid value
    OsaDebugEnd();

    if(memcmp(&pMwAonInfo->mwDefaultAonDns, mwDefaultAonDnsCfg, sizeof(MidWareDefaultAonDnsCfg)) != 0)
    {
        memcpy(&pMwAonInfo->mwDefaultAonDns, mwDefaultAonDnsCfg, sizeof(MidWareDefaultAonDnsCfg));

        pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
    }
}

void mwGetAonEcSocPublicConfig(UINT8 *mode, UINT8 *publicDlPkgNumMax, UINT16 *publicDlBufferToalSize)
{
    OsaDebugBegin(pMwAonInfo != PNULL , pMwAonInfo, 0, 0);
    return ;      //most case, it's a invalid value
    OsaDebugEnd();

    *mode = pMwAonInfo->ecSocPubCfg.mode;
    *publicDlPkgNumMax = pMwAonInfo->ecSocPubCfg.publicDlPkgNumMax;
    *publicDlBufferToalSize = pMwAonInfo->ecSocPubCfg.publicDlBufferToalSize;
}

void mwSetAonEcSocPublicConfig(UINT8 mode, UINT8 publicDlPkgNumMax, UINT16 publicDlBufferToalSize)
{
    BOOL bSave = FALSE;

    OsaDebugBegin(pMwAonInfo != PNULL , pMwAonInfo, 0, 0);
    return ;      //most case, it's a invalid value
    OsaDebugEnd();


    if(pMwAonInfo->ecSocPubCfg.mode != mode)
    {
        pMwAonInfo->ecSocPubCfg.mode = mode;
        bSave = TRUE;
    }

    if(publicDlPkgNumMax != 0xFF && pMwAonInfo->ecSocPubCfg.publicDlPkgNumMax != publicDlPkgNumMax)
    {
        pMwAonInfo->ecSocPubCfg.publicDlPkgNumMax = publicDlPkgNumMax;
        bSave = TRUE;
    }

    if(publicDlBufferToalSize != 0xFFFF && pMwAonInfo->ecSocPubCfg.publicDlBufferToalSize != publicDlBufferToalSize)
    {
        pMwAonInfo->ecSocPubCfg.publicDlBufferToalSize = publicDlBufferToalSize;
        bSave = TRUE;
    }

    if(bSave == TRUE)
    {
        pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
    }

}


/**
  \fn           void mwSetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set one AT channel config item value, but not save to flash, need caller call mwSaveNvmConfig() to write to flash
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \param[out]   bChanged    return value, whether NVM changed, if return TRUE, caller need to write to flash
  \returns      void
*/
void mwAonSetAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value, BOOL *bChanged)
{
    MidWareAtChanConfig *pAtChanCfg = PNULL;
    UINT32  tmpIdx = 0;

    OsaCheck(bChanged != PNULL, bChanged, cfgEnum, value);

    /*
     * find right config
    */
    for (tmpIdx = 0; tmpIdx < MID_WARE_USED_AT_CHAN_NUM; tmpIdx++)
    {
        if (pMwAonInfo->atChanConfig[tmpIdx].chanId == chanId)
        {
            pAtChanCfg = &(pMwAonInfo->atChanConfig[tmpIdx]);
            break;
        }
    }

    OsaDebugBegin(pAtChanCfg != PNULL, chanId, pMwAonInfo->atChanConfig[0].chanId, cfgEnum);
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
        case MID_WARE_AT_CHAN_IPINFO_RPT_MODE_CFG:
            if (value != pAtChanCfg->ecipinfoRptMode)
            {
                if (value <= 1) // 1 bit
                {
                    pAtChanCfg->ecipinfoRptMode = value;
                    *bChanged = TRUE;
                }
                else
                {
                    OsaDebugBegin(FALSE, cfgEnum, value, pAtChanCfg->ecipinfoRptMode);
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
  \fn           void mwAonSetAndSaveAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
  \brief        Set one AT channel config item value, and save into AON
  \param[in]    chanId      AT channel ID
  \param[in]    cfgEnum     which item
  \param[in]    value       item value
  \returns      void
*/
void mwAonSetAndSaveAtChanConfigItemValue(UINT8 chanId, MidWareATChanCfgEnum cfgEnum, UINT32 value)
{
    BOOL    aonChanged = FALSE;

    mwAonSetAtChanConfigItemValue(chanId, cfgEnum, value, &aonChanged);

    if (aonChanged)
    {
        pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
    }

    return;
}


/**
  \fn           MidWareSockAonInfo* mwGetSockAonInfo(void)
  \brief        Get middle ware socket AON info
  \param[in]    void
  \returns      MidWareSockAonInfo *
*/
MidWareSockAonInfo* mwGetSockAonInfo(void)
{
    OsaCheck(pMwAonInfo != PNULL, 0, 0, 0);

    return &(pMwAonInfo->mwSockAonInfo);
}

/**
  \fn           MidWareSockAonInfo* mwGetSockAonInfo(void)
  \brief        Get middle ware socket AON info
  \param[in]    void
  \returns      MidWareSockAonInfo *
*/
void mwSetSockAonInfo(MidWareSockAonInfo *pSockAonInfo)
{
    UINT32  tmpIdx  = 0, structSize = sizeof(MidWareSockAonInfo);
    BOOL    bChanged = FALSE;
    UINT8   *pDest  = (UINT8 *)&(pMwAonInfo->mwSockAonInfo);
    UINT8   *pSrc   = (UINT8 *)pSockAonInfo;

    OsaCheck((pMwAonInfo != PNULL) && (pDest != pSrc), pMwAonInfo, pDest, pSrc);

    if (pSockAonInfo != PNULL)
    {
        if (memcmp(&(pMwAonInfo->mwSockAonInfo), pSockAonInfo, sizeof(MidWareSockAonInfo)) != 0)
        {
            memcpy(&(pMwAonInfo->mwSockAonInfo),
                   pSockAonInfo,
                   sizeof(MidWareSockAonInfo));

            pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
        }
    }
    else    //PNULL
    {
        while (tmpIdx < structSize)
        {
            if (pDest[tmpIdx] != 0)
            {
                bChanged = TRUE;
                pDest[tmpIdx] = 0;
            }

            tmpIdx++;
        }

        if (bChanged)
        {
            pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);
        }
    }

    return;
}

/**
  \fn           MidWareSockAonInfo* mwGetSockAonInfo(void)
  \brief        Get middle ware socket AON info
  \param[in]    void
  \returns      MidWareSockAonInfo *
*/
void mwSockAonInfoChanged(void)
{
    pmuAonCtxChanged(PMU_DEEPSLP_CMS_MOD);

    return;
}


/**
  \fn           void mwAonTaskRecovery(void)
  \brief        Need recovery some middle ware task, after wakeup from HIB/Sleep2
  \param[in]    void
  \returns      void
*/
void mwAonTaskRecovery(void)
{
    if (pmuBWakeupFromHib() || pmuBWakeupFromSleep2())
    {
        //recover at socket hib context
        cmsSockMgrRecoverHibCallback((void*)&(pMwAonInfo->mwSockAonInfo.sockContext));
    }
    else
    {
        OsaDebugBegin(FALSE, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}


