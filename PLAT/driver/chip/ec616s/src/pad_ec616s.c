 /****************************************************************************
  *
  * Copy right:   2017-, Copyrigths of EigenComm Ltd.
  * File name:    pad_ec616s.c
  * Description:  EC616S pad driver source file
  * History:      Rev1.0   2018-11-14
  *
  ****************************************************************************/

#include "pad_ec616s.h"
#include "clock_ec616s.h"
#include "slpman_ec616s.h"

#ifdef PM_FEATURE_ENABLE

static uint32_t g_padBackupRegs[PAD_PIN_MAX_NUM] = {0};

/**
  \fn        void PAD_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
  \brief     Backup pad configurations before sleep.
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
void PAD_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            for(i = 0; i < PAD_PIN_MAX_NUM; i++)
            {
                g_padBackupRegs[i] = PAD->PCR[i];
            }
            break;
        default:
            break;
    }

}

/**
 \fn        void PAD_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
 \brief     Restore PAD configurations after exit from sleep.
 \param[in] pdata pointer to user data, not used now
 \param[in] state low power state
 */
void PAD_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            GPR_ClockEnable(GPR_PADAPBClk);

            for(i = 0; i < PAD_PIN_MAX_NUM; i++)
            {
                PAD->PCR[i] = g_padBackupRegs[i];
            }
            break;

        default:
            break;
    }

}
#endif

void PAD_DriverInit(void)
{
    // enable clock
    CLOCK_ClockEnable(GPR_PADAPBClk);

#ifdef PM_FEATURE_ENABLE
    slpManRegisterPredefinedBackupCb(SLP_CALLBACK_PAD_MODULE, PAD_EnterLowPowerStatePrepare, NULL, SLPMAN_SLEEP1_STATE);
    slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_PAD_MODULE, PAD_ExitLowPowerStateRestore, NULL, SLPMAN_SLEEP1_STATE);
#endif
}

void PAD_DriverDeInit(void)
{
    // disable clock
    CLOCK_ClockDisable(GPR_PADAPBClk);

#ifdef PM_FEATURE_ENABLE
    slpManUnregisterPredefinedBackupCb(SLP_CALLBACK_PAD_MODULE);
    slpManUnregisterPredefinedRestoreCb(SLP_CALLBACK_PAD_MODULE);
#endif
}


void PAD_GetDefaultConfig(pad_config_t *config)
{
    ASSERT(config);

    config->mux = PAD_MuxAlt0;
    config->inputBufferEnable = PAD_InputBufferDisable;
    config->pullSelect = PAD_PullAuto;
    config->pullUpEnable = PAD_PullUpDisable;
    config->pullDownEnable = PAD_PullDownDisable;
}

void PAD_SetPinConfig(uint32_t pin, const pad_config_t *config)
{
    ASSERT(config);
    ASSERT(pin < PAD_PIN_MAX_NUM);

    PAD->PCR[pin] = *((const uint32_t *)config);
}

void PAD_SetPinMux(uint32_t pin, pad_mux_t mux)
{
    ASSERT(pin < PAD_PIN_MAX_NUM);

    PAD->PCR[pin] = (PAD->PCR[pin] & ~PAD_PCR_MUX_Msk) | EIGEN_VAL2FLD(PAD_PCR_MUX, mux);
}

void PAD_EnablePinInputBuffer(uint32_t pin, bool enable)
{
    ASSERT(pin < PAD_PIN_MAX_NUM);

    if (enable == true)
    {
        PAD->PCR[pin] |= PAD_PCR_INPUT_BUFFER_ENABLE_Msk;
    }
    else
    {
        PAD->PCR[pin] &= ~PAD_PCR_INPUT_BUFFER_ENABLE_Msk;
    }
}

void PAD_SetPinPullConfig(uint32_t pin, pad_pull_config_t config)
{
    ASSERT(pin < PAD_PIN_MAX_NUM);

    switch(config)
    {
        case PAD_InternalPullUp:
            PAD->PCR[pin] = (PAD->PCR[pin] & ~PAD_PCR_PULL_DOWN_ENABLE_Msk) | PAD_PCR_PULL_UP_ENABLE_Msk | PAD_PCR_PULL_SELECT_Msk;
            break;
        case PAD_InternalPullDown:
            PAD->PCR[pin] = (PAD->PCR[pin] & ~PAD_PCR_PULL_UP_ENABLE_Msk) | PAD_PCR_PULL_DOWN_ENABLE_Msk | PAD_PCR_PULL_SELECT_Msk;
            break;
        case PAD_AutoPull:
            PAD->PCR[pin] = (PAD->PCR[pin] & ~PAD_PCR_PULL_SELECT_Msk);
            break;
        default:
            break;
    }
}

