/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    wdt_ec616s.c
 * Description:  EC616S wdt driver source file
 * History:      Rev1.0   2018-07-18
 *
 ****************************************************************************/

#include "wdt_ec616s.h"
#include "clock_ec616s.h"
#include "slpman_ec616s.h"

#ifdef PM_FEATURE_ENABLE

/** \brief Internal used data structure */
typedef struct _wdt_database
{
    bool                   isInited;            /**< whether wdt has been initialized */
    bool                   wdtStartStatus;      /**< wdt enable status, used for low power restore */
    wdt_config_t           wdtConfig;           /**< wdt configuration, used for low power restore */
} wdt_database_t;

static wdt_database_t g_wdtDataBase = {0};

/**
  \fn        static void WDT_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
  \brief     Perform necessary preparations before sleep
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
static void WDT_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
{
    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            if(g_wdtDataBase.isInited == true)
            {
                g_wdtDataBase.wdtStartStatus = WDT_GetStartStatus();
                //disable clock
                CLOCK_ClockDisable(GPR_WDGAPBClk);
                CLOCK_ClockDisable(GPR_WDGFuncClk);
            }

            break;
        default:
            break;
    }

}

/**
  \fn        static void WDT_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
  \brief     Restore after exit from sleep
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state

 */
static void WDT_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
{
    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            if(g_wdtDataBase.isInited == true)
            {
                WDT_Init(&g_wdtDataBase.wdtConfig);

                if(g_wdtDataBase.wdtStartStatus)
                    WDT_Start();
            }

            break;

        default:
            break;
    }
}
#endif


void WDT_Unlock(void)
{
    WDT->LOCK = 0xABABU;
}

void WDT_Kick(void)
{
    uint32_t mask = SaveAndSetIRQMask();

    WDT_Unlock();
    WDT->CCR = WDT_CCR_CNT_CLR_Msk;

    RestoreIRQMask(mask);
}

void WDT_GetDefaultConfig(wdt_config_t *config)
{
    ASSERT(config);

    config->mode = WDT_InterruptOnlyMode;
    config->timeoutValue = 0xFFFFU;
}

void WDT_Init(const wdt_config_t *config)
{
    ASSERT(config);

#ifdef PM_FEATURE_ENABLE

    slpManRegisterPredefinedBackupCb(SLP_CALLBACK_WDT_MODULE, WDT_EnterLowPowerStatePrepare, NULL, SLPMAN_SLEEP1_STATE);
    slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_WDT_MODULE, WDT_ExitLowPowerStateRestore, NULL, SLPMAN_SLEEP1_STATE);

    g_wdtDataBase.isInited = true;
    g_wdtDataBase.wdtConfig = *config;

#endif

    //enable clock
    CLOCK_ClockEnable(GPR_WDGAPBClk);
    CLOCK_ClockEnable(GPR_WDGFuncClk);

    WDT_Unlock();
    WDT->CTRL = (WDT->CTRL &~ WDT_CTRL_MODE_Msk) | EIGEN_VAL2FLD(WDT_CTRL_MODE, config->mode);
    WDT_Unlock();
    WDT->TOVR = config->timeoutValue;
}

void WDT_DeInit(void)
{
#ifdef PM_FEATURE_ENABLE

    slpManUnregisterPredefinedBackupCb(SLP_CALLBACK_WDT_MODULE);
    slpManUnregisterPredefinedRestoreCb(SLP_CALLBACK_WDT_MODULE);

    g_wdtDataBase.isInited = false;
#endif

    //disable clock
    CLOCK_ClockDisable(GPR_WDGAPBClk);
    CLOCK_ClockDisable(GPR_WDGFuncClk);
}

void WDT_Start(void)
{
    uint32_t mask = SaveAndSetIRQMask();

    WDT_Unlock();
    WDT->CTRL |= WDT_CTRL_ENABLE_Msk;

    RestoreIRQMask(mask);
}

void WDT_Stop(void)
{
    uint32_t mask = SaveAndSetIRQMask();

    WDT_Unlock();
    WDT->CTRL &= ~WDT_CTRL_ENABLE_Msk;

    RestoreIRQMask(mask);

}

uint32_t WDT_GetInterruptFlags(void)
{
    return WDT->STAT & WDT_STAT_ISTAT_Msk;
}

void WDT_ClearInterruptFlags(uint32_t mask)
{
    uint32_t msk = SaveAndSetIRQMask();

    WDT_Unlock();
    WDT->ICR = WDT_ICR_ICLR_Msk;

    RestoreIRQMask(msk);
}

wdt_mode_t WDT_GetMode(void)
{
    return (wdt_mode_t)EIGEN_FLD2VAL(WDT_CTRL_MODE, WDT->CTRL);
}

bool WDT_GetStartStatus(void)
{
    return (bool)EIGEN_FLD2VAL(WDT_CTRL_ENABLE, WDT->CTRL);
}

