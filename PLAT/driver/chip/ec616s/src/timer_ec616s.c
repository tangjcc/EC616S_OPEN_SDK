/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    timer_ec616s.c
 * Description:  EC616S timer driver source file
 * History:      Rev1.0   2018-07-18
 *
 ****************************************************************************/

#include "timer_ec616s.h"
#include "clock_ec616s.h"
#include "slpman_ec616s.h"
#include "debug_log.h"

#define EIGEN_TIMER(n)             ((TIMER_TypeDef *) (TIMER0_BASE_ADDR + 0x10000*n))

#ifdef PM_FEATURE_ENABLE

/** \brief Internal used data structure */
typedef struct _timer_database
{
    bool                            isInited;            /**< Whether timer has been initialized */
    struct
    {
        uint32_t TCTLR;                                  /**< Timer Control Register */
        uint32_t TIVR;                                   /**< Timer Init Value Register */
        uint32_t TMR[3];                                 /**< Timer Match N Register */
    } backup_registers;                                  /**< Backup registers for low power restore */
} timer_database_t;

static bool   g_isTimerDriverInited = false;

static timer_database_t g_timerDataBase[TIMER_INSTANCE_NUM];

#endif

static const clock_ID_t g_timerClocks[TIMER_INSTANCE_NUM*2] = {GPR_TIMER0APBClk, GPR_TIMER0FuncClk,
                                                         GPR_TIMER1APBClk, GPR_TIMER1FuncClk,
                                                         GPR_TIMER2APBClk, GPR_TIMER2FuncClk,
                                                         GPR_TIMER3APBClk, GPR_TIMER3FuncClk,
                                                         GPR_TIMER4APBClk, GPR_TIMER4FuncClk,
                                                         GPR_TIMER5APBClk, GPR_TIMER5FuncClk
                                                        };

#ifdef PM_FEATURE_ENABLE

/**
  \brief Bitmap of TIMER working status,
         when all TIMER instances are not working, we can vote to enter to low power state.
 */
static uint32_t g_timerWorkingStatus;

/**
  \fn        static void TIMER_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
  \brief     Perform necessary preparations before sleep.
             After recovering from SLPMAN_SLEEP1_STATE, TIMER hareware is repowered, we backup
             some registers here first so that we can restore user's configurations after exit.
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
static void TIMER_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            for(i = 0; i < TIMER_INSTANCE_NUM; i++)
            {
                if(g_timerDataBase[i].isInited == true)
                {
                    g_timerDataBase[i].backup_registers.TCTLR = EIGEN_TIMER(i)->TCTLR;
                    g_timerDataBase[i].backup_registers.TIVR = EIGEN_TIMER(i)->TIVR;
                    g_timerDataBase[i].backup_registers.TMR[0] = EIGEN_TIMER(i)->TMR[0];
                    g_timerDataBase[i].backup_registers.TMR[1] = EIGEN_TIMER(i)->TMR[1];
                    g_timerDataBase[i].backup_registers.TMR[2] = EIGEN_TIMER(i)->TMR[2];
                }
            }
            break;
        default:
            break;
    }

}

/**
  \fn        static void TIMER_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
  \brief     Restore after exit from sleep.
             After recovering from SLPMAN_SLEEP1_STATE, TIMER hareware is repowered, we restore user's configurations
             by aidding of the stored registers.

  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state

 */
static void TIMER_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            for(i = 0; i < TIMER_INSTANCE_NUM; i++)
            {
                if(g_timerDataBase[i].isInited == true)
                {
                     GPR_ClockEnable(g_timerClocks[i*2]);
                     GPR_ClockEnable(g_timerClocks[i*2+1]);

                     EIGEN_TIMER(i)->TCTLR = g_timerDataBase[i].backup_registers.TCTLR;
                     EIGEN_TIMER(i)->TIVR = g_timerDataBase[i].backup_registers.TIVR;
                     EIGEN_TIMER(i)->TMR[0] = g_timerDataBase[i].backup_registers.TMR[0];
                     EIGEN_TIMER(i)->TMR[1] = g_timerDataBase[i].backup_registers.TMR[1];
                     EIGEN_TIMER(i)->TMR[2] = g_timerDataBase[i].backup_registers.TMR[2];
                }
            }
            break;

        default:
            break;
    }
}
#endif

void TIMER_DriverInit(void)
{

#ifdef PM_FEATURE_ENABLE
    uint32_t i;

    if(g_isTimerDriverInited == false)
    {
        for(i = 0; i < TIMER_INSTANCE_NUM; i++)
        {
            g_timerDataBase[i].isInited = false;
        }

        g_timerWorkingStatus = 0;
        slpManRegisterPredefinedBackupCb(SLP_CALLBACK_TIMER_MODULE, TIMER_EnterLowPowerStatePrepare, NULL, SLPMAN_SLEEP1_STATE);
        slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_TIMER_MODULE, TIMER_ExitLowPowerStateRestore, NULL, SLPMAN_SLEEP1_STATE);

        g_isTimerDriverInited = true;
    }
#endif

}

void TIMER_GetDefaultConfig(timer_config_t *config)
{
    ASSERT(config);

    config->clockSource = TIMER_InternalClock;
    config->reloadOption = TIMER_ReloadOnMatch1;
    config->initValue = 0;
    config->match0 = 0x10000 >> 1U;
    config->match1 = 0x10000;
    config->match2 = 0xFFFFFFFFU;
}

void TIMER_Init(uint32_t instance, const timer_config_t *config)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);
    ASSERT(config);

    CLOCK_ClockEnable(g_timerClocks[instance*2]);
    CLOCK_ClockEnable(g_timerClocks[instance*2+1]);

    /* Stop timer counter */
    EIGEN_TIMER(instance)->TCCR = 0;

    EIGEN_TIMER(instance)->TCTLR = EIGEN_VAL2FLD(TIMER_TCTLR_MODE, config->clockSource) | \
                                   EIGEN_VAL2FLD(TIMER_TCTLR_MCS, config->reloadOption);
    EIGEN_TIMER(instance)->TIVR = config->initValue;
    EIGEN_TIMER(instance)->TMR[0] = config->match0;
    EIGEN_TIMER(instance)->TMR[1] = config->match1;
    EIGEN_TIMER(instance)->TMR[2] = config->match2;

#ifdef PM_FEATURE_ENABLE
    g_timerDataBase[instance].isInited = true;
#endif
}

void TIMER_DeInit(uint32_t instance)
{
    CLOCK_ClockDisable(g_timerClocks[instance*2]);
    CLOCK_ClockDisable(g_timerClocks[instance*2+1]);

#ifdef PM_FEATURE_ENABLE
    g_timerDataBase[instance].isInited = false;
#endif
}
void TIMER_SetMatch(uint32_t instance, timer_match_select_t matchNum, uint32_t matchValue)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    EIGEN_TIMER(instance)->TMR[matchNum] = matchValue;
}

void TIMER_SetInitValue(uint32_t instance, uint32_t initValue)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    EIGEN_TIMER(instance)->TIVR = initValue;
}

void TIMER_SetReloadOption(uint32_t instance, timer_reload_option_t option)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    EIGEN_TIMER(instance)->TCTLR = (EIGEN_TIMER(instance)->TCTLR &~ TIMER_TCTLR_MCS_Msk) | EIGEN_VAL2FLD(TIMER_TCTLR_MCS, option);
}

void TIMER_Start(uint32_t instance)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

#ifdef PM_FEATURE_ENABLE
    g_timerWorkingStatus |= (1U << instance);
    slpManDrvVoteSleep(SLP_VOTE_TIMER, SLP_ACTIVE_STATE);
#endif

    EIGEN_TIMER(instance)->TCCR = 1U;
}

void TIMER_Stop(uint32_t instance)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    EIGEN_TIMER(instance)->TCCR = 0;

#ifdef PM_FEATURE_ENABLE
    g_timerWorkingStatus &= ~(1U << instance);
    if (g_timerWorkingStatus == 0)
        slpManDrvVoteSleep(SLP_VOTE_TIMER, SLP_SLP1_STATE);
#endif

}

uint32_t TIMER_GetCount(uint32_t instance)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    return EIGEN_TIMER(instance)->ATCR;
}

int32_t TIMER_SetupPwm(uint32_t instance, const timer_pwm_config_t *config)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);
    ASSERT(config);

    uint32_t period;

    CLOCK_ClockEnable(g_timerClocks[instance*2]);
    CLOCK_ClockEnable(g_timerClocks[instance*2+1]);

    if(config->srcClock_HZ == 0 || config->pwmFreq_HZ == 0 || config->srcClock_HZ <= config->pwmFreq_HZ)
        return ARM_DRIVER_ERROR_PARAMETER;

    period = config->srcClock_HZ / config->pwmFreq_HZ;

    /* Set PWM period */
    EIGEN_TIMER(instance)->TMR[1] = period - 1;

    /* Set duty cycle */
    if(config->dutyCyclePercent == 0)
    {
        EIGEN_TIMER(instance)->TMR[0] = period;      // if TMR[0] > TMR[1], PWM output keeps low
    }
    else if(config->dutyCyclePercent == 100)
    {
        EIGEN_TIMER(instance)->TMR[0] = period - 1;  // let TMR[0] == TMR[1]
    }
    else
    {
        /*
            Note the higher pwm frequency is, the lower reslution of dutyCycle we'll get.
            Below calculation may overflow for specific dutyCyclePercent and in such case, TMR[0] > TMR[1], PWM output keeps low
        */
        EIGEN_TIMER(instance)->TMR[0] = ((uint64_t)period) * (100U - config->dutyCyclePercent) / 100U - 1;
    }
    EIGEN_TIMER(instance)->TIVR = 0;

    /* Enable PWM out */
    EIGEN_TIMER(instance)->TCTLR = (EIGEN_TIMER(instance)->TCTLR & ~ TIMER_TCTLR_MCS_Msk) | \
                                   EIGEN_VAL2FLD(TIMER_TCTLR_MCS, 2U) | TIMER_TCTLR_PWMOUT_Msk;

#ifdef PM_FEATURE_ENABLE
    g_timerDataBase[instance].isInited = true;
#endif

    return ARM_DRIVER_OK;

}

void TIMER_UpdatePwmDutyCycle(uint32_t instance, uint32_t dutyCyclePercent)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    if(dutyCyclePercent == 0)
    {
        EIGEN_TIMER(instance)->TMR[0] = EIGEN_TIMER(instance)->TMR[1] + 1;      // if TMR[0] > TMR[1], PWM output keeps low
    }
    else if(dutyCyclePercent >= 100)
    {
        EIGEN_TIMER(instance)->TMR[0] = EIGEN_TIMER(instance)->TMR[1];          // let TMR[0] == TMR[1]
    }
    else
    {
        EIGEN_TIMER(instance)->TMR[0] = ((uint64_t)EIGEN_TIMER(instance)->TMR[1] + 1) * (100U - dutyCyclePercent) / 100U - 1;
    }

}

void TIMER_InterruptConfig(uint32_t instance, timer_match_select_t match, timer_interrupt_config_t config)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    uint32_t enableMask, typeMask, registerValue;

    enableMask = 1U << (match + TIMER_TCTLR_IE_0_Pos);
    typeMask = 1U << (match + TIMER_TCTLR_IT_0_Pos);
    registerValue = EIGEN_TIMER(instance)->TCTLR;

    switch(config)
    {
        case TIMER_InterruptDisabled:

            registerValue &= ~enableMask;
            EIGEN_TIMER(instance)->TCTLR = registerValue;
            break;

        case TIMER_InterruptLevel:

            registerValue |= enableMask;
            registerValue &= ~typeMask;
            EIGEN_TIMER(instance)->TCTLR = registerValue;
            break;

        case TIMER_InterruptPulse:

            registerValue |= enableMask;
            registerValue |= typeMask;
            EIGEN_TIMER(instance)->TCTLR = registerValue;
            break;

        default:
            break;
    }
}

timer_interrupt_config_t TIMER_GetInterruptConfig(uint32_t instance, timer_match_select_t match)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    uint32_t enableMask, typeMask, registerValue;

    enableMask = 1U << (match + TIMER_TCTLR_IE_0_Pos);
    typeMask = 1U << (match + TIMER_TCTLR_IT_0_Pos);
    registerValue = EIGEN_TIMER(instance)->TCTLR;

    if ((enableMask & registerValue) == 0)
        return TIMER_InterruptDisabled;
    else if ((typeMask & registerValue) == 0)
        return TIMER_InterruptLevel;
    else
        return TIMER_InterruptPulse;
}

uint32_t TIMER_GetInterruptFlags(uint32_t instance)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    return EIGEN_TIMER(instance)->TSR;
}

void TIMER_ClearInterruptFlags(uint32_t instance, uint32_t mask)
{
    ASSERT(instance < TIMER_INSTANCE_NUM);

    EIGEN_TIMER(instance)->TSR = mask;
}

static uint8_t gNetLightInstance = 0xff;
void TIMER_Netlight_Enable(uint8_t instance)        // call by user in bsp_custom.c
{
    gNetLightInstance = instance;
}

/////// Internal use for Netlight Function /////////////////
void TIMER_Netlight_PWM(uint8_t mode)
{
    uint8_t instance = gNetLightInstance;
    static uint8_t curMode;
	uint32_t mask;

	ECOMM_TRACE(UNILOG_PLA_DRIVER, TIMER_Netlight_PWM_1, P_WARNING, 1, "Netlight mode=%u",mode);

	if(instance == 0xff)
		return;

    ASSERT(instance < TIMER_INSTANCE_NUM);

    if(curMode == mode)    // do nothing if mode not change
        return;

    EIGEN_TIMER(instance)->TCCR = 0;

    if(mode == 0)
    {
    	EIGEN_TIMER(instance)->TCTLR = (EIGEN_TIMER(instance)->TCTLR & (~TIMER_TCTLR_PWMOUT_Msk));

		extern void delay_us(uint32_t us);
		delay_us(100);

		GPR_SWReset((sw_reset_ID_t)(GPR_ResetTIMER0Func+instance));

        CLOCK_ClockDisable(g_timerClocks[instance*2]);
        CLOCK_ClockDisable(g_timerClocks[instance*2+1]);

#ifdef PM_FEATURE_ENABLE
		mask = SaveAndSetIRQMask();
		g_timerWorkingStatus &= ~(1U << instance);
		if (g_timerWorkingStatus == 0)
			slpManDrvVoteSleep(SLP_VOTE_TIMER, SLP_SLP1_STATE);
		RestoreIRQMask(mask);
#endif
		curMode = mode;

        return;
    }

    CLOCK_ClockEnable(g_timerClocks[instance*2]);
    CLOCK_ClockEnable(g_timerClocks[instance*2+1]);

#ifdef PM_FEATURE_ENABLE
	mask = SaveAndSetIRQMask();
	g_timerWorkingStatus |= (1U << instance);
	slpManDrvVoteSleep(SLP_VOTE_TIMER, SLP_ACTIVE_STATE);
	RestoreIRQMask(mask);
#endif

    /* Enable PWM out */
    EIGEN_TIMER(instance)->TCTLR = (EIGEN_TIMER(instance)->TCTLR & ~ TIMER_TCTLR_MCS_Msk) | \
                                   EIGEN_VAL2FLD(TIMER_TCTLR_MCS, 2U) | TIMER_TCTLR_PWMOUT_Msk;

    if(mode == 1)       // fast flash:  64ms high and 800ms low
    {
        EIGEN_TIMER(instance)->TMR[0] = 0x13D61FF;

        EIGEN_TIMER(instance)->TMR[1] = 0x156C5FE;

        EIGEN_TIMER(instance)->TCCR = 1;

        curMode = mode;
    }
    else if(mode == 2)  // slow flash:  64ms high and 2000ms low
    {
        EIGEN_TIMER(instance)->TMR[0] = 0x31974FF;

        EIGEN_TIMER(instance)->TMR[1] = 0x332D8FE;

        EIGEN_TIMER(instance)->TCCR = 1;

        curMode = mode;
    }
    else
    {
        ASSERT(0);
    }

}

