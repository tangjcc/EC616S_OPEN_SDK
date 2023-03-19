/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    timer_demo.c
 * Description:  EC616S TIMER driver example entry source file
 * History:      Rev1.0   2018-07-17
 *
 ****************************************************************************/
#include <stdio.h>
#include "timer_ec616s.h"
#include "bsp.h"

/*
   This project demos PWM feature of TIMER, in this example, we'll use 2 instance of TIMER,
   one is used to output pwm, and its frequency is configure as 1KHz while duty cycle
   is controlled by another TIMER, the duty cycle is updated in interval of 5 from 0 to 100 periodically,
   the period is achieved with aid of another instance of timer whose interrupt is triggered every 500ms.
 */

/** \brief TIMER instance for PWM output */
#define PWM_INSTANCE                     (3)

/** \brief PWM clock id for clock configuration */
#define PWM_CLOCK_ID                     (GPR_TIMER3FuncClk)

/** \brief PWM clock source select */
#define PWM_CLOCK_SOURCE                 (GPR_TIMER3ClkSel_26M)

/** \brief PWM initial duty cycle percent */
#define PWM_INIT_DUTY_CYCLE_PERCENT      (50)

/** \brief TIMER/PWM out pin */
#define PWM_OUT_PIN                      (16)

/** \brief TIMER/PWM Alt function select */
#define PWM_OUT_ALT_SEL                  (PAD_MuxAlt5)

/** \brief TIMER instance for PWM duty cycle control */
#define TIMER_INSTANCE                   (1)

/** \brief TIMER clock id for clock configuration */
#define TIMER_CLOCK_ID                   (GPR_TIMER1FuncClk)

/** \brief TIMER clock source select */
#define TIMER_CLOCK_SOURCE               (GPR_TIMER1ClkSel_32K)

/** \brief TIMER instance IRQ number */
#define TIMER_INSTANCE_IRQ               (PXIC_Timer1_IRQn)

volatile static int update = 0;

/**
  \fn          void Timer_ISR()
  \brief       Timer1 interrupt service routine
  \return
*/
void Timer_ISR()
{
    if (TIMER_GetInterruptFlags(TIMER_INSTANCE) & TIMER_Match0InterruptFlag)
    {
        update = 1;
        TIMER_ClearInterruptFlags(TIMER_INSTANCE, TIMER_Match0InterruptFlag);
    }
}

void TIMER_ExampleEntry(void)
{
    unsigned int dutyCycle = PWM_INIT_DUTY_CYCLE_PERCENT;

    printf("TIMER Example Start\r\n");

    // PWM out pin config
    pad_config_t config;

    PAD_GetDefaultConfig(&config);
    config.mux = PWM_OUT_ALT_SEL;
    PAD_SetPinConfig(PWM_OUT_PIN, &config);

    // Config PWM clock, source from 26MHz and divide by 1
    CLOCK_SetClockSrc(PWM_CLOCK_ID, PWM_CLOCK_SOURCE);
    CLOCK_SetClockDiv(PWM_CLOCK_ID, 1);

    TIMER_DriverInit();

    timer_pwm_config_t pwmConfig;
    pwmConfig.pwmFreq_HZ = 1000;
    pwmConfig.srcClock_HZ = GPR_GetClockFreq(PWM_CLOCK_ID);  // 26MHz
    pwmConfig.dutyCyclePercent = PWM_INIT_DUTY_CYCLE_PERCENT;

    TIMER_SetupPwm(PWM_INSTANCE, &pwmConfig);

    // TIMER config
    // Config TIMER clock, source from 32.768KHz and divide by 1
    CLOCK_SetClockSrc(TIMER_CLOCK_ID, TIMER_CLOCK_SOURCE);
    CLOCK_SetClockDiv(TIMER_CLOCK_ID, 1);

    // Config TIMER period as 500ms, match0 value is 16384 = 0x4000
    timer_config_t timerConfig;
    TIMER_GetDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_ReloadOnMatch0;
    timerConfig.match0 = 0x4000;

    TIMER_Init(TIMER_INSTANCE, &timerConfig);

    TIMER_InterruptConfig(TIMER_INSTANCE, TIMER_Match0Select, TIMER_InterruptLevel);
    TIMER_InterruptConfig(TIMER_INSTANCE, TIMER_Match1Select, TIMER_InterruptDisabled);
    TIMER_InterruptConfig(TIMER_INSTANCE, TIMER_Match2Select, TIMER_InterruptDisabled);

    // Enable TIMER IRQ
    XIC_SetVector(TIMER_INSTANCE_IRQ, Timer_ISR);
    XIC_EnableIRQ(TIMER_INSTANCE_IRQ);

    TIMER_Start(PWM_INSTANCE);
    TIMER_Start(TIMER_INSTANCE);
    while(1)
    {
        if(update == 1)
        {
            update = 0;
            if (dutyCycle > 100)
                dutyCycle = 0;
            dutyCycle += 5;

            TIMER_UpdatePwmDutyCycle(PWM_INSTANCE, dutyCycle);
        }
    }
}

