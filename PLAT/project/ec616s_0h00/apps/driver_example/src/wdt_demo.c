/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    wdt_demo.c
 * Description:  EC616S WDT driver example entry source file
 * History:      Rev1.0   2018-07-23
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "timer_ec616s.h"
#include "wdt_ec616s.h"

/*
   This project demos the usage of WDT, in this example, we'll start to kick WDT for 5 times and then stop,
   the WDT is configured as WDT_InterruptResetMode and we won't clear WDT interrupt flag upon timeout,
   so a system reset shall follow a WDT interrupt, the time tick is achieved through TIMER's interrupt.
 */

/** \brief WDT counter refresh times */
#define WDT_KICK_TIMES      (5)

/** \brief WDT interrupt count */
static volatile int wdtInterruptCount = 0;

/** \brief timer interupt count */
static volatile int timerInterruptCount = 0;

/** \brief Flag to control message print */
static volatile int printDone = 1;

/**
  \fn          void NMI_Handler()
  \brief       wdt interrupt is NMI
  \return
*/
void NMI_Handler()
{
    wdtInterruptCount++;
}

/**
  \fn          void Timer0_ISR()
  \brief       timer0 interrupt service routine
  \return
*/
void Timer0_ISR()
{
    timerInterruptCount++;
    printDone = 0;
    if(timerInterruptCount <= WDT_KICK_TIMES)
        WDT_Kick();
}

void WDT_ExampleEntry(void)
{
    // Config WDT clock, source from 32.768KHz and divide by 5
    CLOCK_SetClockSrc(GPR_WDGFuncClk, GPR_WDGClkSel_32K);
    CLOCK_SetClockDiv(GPR_WDGFuncClk, 5);

    // WDT config, timeout is 5s
    wdt_config_t wdtConfig;
    wdtConfig.mode = WDT_InterruptResetMode;
    wdtConfig.timeoutValue = 0x8000;
    WDT_Init(&wdtConfig);

    // Config TIMER clock, source from 32.768KHz and divide by 1
    CLOCK_SetClockSrc(GPR_TIMER0FuncClk, GPR_TIMER0ClkSel_32K);
    CLOCK_SetClockDiv(GPR_TIMER0FuncClk, 1);

    TIMER_DriverInit();

    // Config timer period as 1s, match0 value is 32768 = 0x8000
    timer_config_t timerConfig;
    TIMER_GetDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_ReloadOnMatch0;
    timerConfig.match0 = 0x8000;

    TIMER_Init(0, &timerConfig);

    // Note interrupt flag won't assert in TIMER_InterruptPulse mode
    TIMER_InterruptConfig(0, TIMER_Match0Select, TIMER_InterruptPulse);
    TIMER_InterruptConfig(0, TIMER_Match1Select, TIMER_InterruptDisabled);
    TIMER_InterruptConfig(0, TIMER_Match2Select, TIMER_InterruptDisabled);

    // Enable IRQ
    XIC_SetVector(PXIC_Timer0_IRQn, Timer0_ISR);
    XIC_EnableIRQ(PXIC_Timer0_IRQn);

    WDT_Start();
    TIMER_Start(0);
    while(1)
    {
        if (printDone == 0)
        {
            printf("tick:%d, wdt interrupt count: %d\n", timerInterruptCount, wdtInterruptCount);
            if (timerInterruptCount < WDT_KICK_TIMES)
                printf("wdt kick: %d\n", timerInterruptCount);
            else if (timerInterruptCount == WDT_KICK_TIMES)
                printf("Last kick, wdt interrupt will occur in 5 ticks\n");
            else if (timerInterruptCount == WDT_KICK_TIMES * 2)
                printf("System will reset in 5 ticks\n");

            printDone = 1;
        }
    }
}

