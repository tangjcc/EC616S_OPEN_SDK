/****************************************************************************
 *
 * Copy right:   2018 Copyrigths of EigenComm Ltd.
 * File name:    bsp_custom.c
 * Description:
 * History:
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include "clock_ec616s.h"
#include "bsp_custom.h"
#include "slpman_ec616s.h"
#include "plat_config.h"
#include "debug_log.h"

void GPR_SetUartClk(void)
{
    GPR_ClockDisable(GPR_UART0FuncClk);

    GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);

    GPR_ClockEnable(GPR_UART0FuncClk);

    GPR_SWReset(GPR_ResetUART0Func);
}


static void PMU_WakeupPadInit(void)
{
    const padWakeupSettings_t cfg =
    {
        false, true             // group0 posedge, negedge

    };

    slpManSetWakeupPad(cfg);

}

static void UserTimerExpFunc(uint8_t id)
{
    ECOMM_TRACE(UNILOG_PLA_APP, UserTimerExpFunc_1, P_SIG, 1, "User DeepSleep Timer Expired: TimerID = %u",id);

}

/*
 *  custom board related init
 *  Parameter:   none
 *  note: this function shall be called in OS task context for dependency of reading configure file
 *        which is implemented based on file system
 *  This function called by EC SDK automatically, no need to call by user
 */
void BSP_CustomInit(void)
{
	static bool CustomInitIsCalled = false;

	if(CustomInitIsCalled) return;

    extern void mpu_init(void);
    mpu_init();

    GPR_SetUartClk();


    plat_config_raw_flash_t* rawFlashPlatConfig;

    BSP_LoadPlatConfigFromRawFlash();
    rawFlashPlatConfig = BSP_GetRawFlashPlatConfig();

    if(rawFlashPlatConfig && (rawFlashPlatConfig->logControl != 0 ))
    {
        SetUnilogUart(PORT_USART_0, rawFlashPlatConfig->uartBaudRate, true);
        uniLogInitStart(UART_0_FOR_UNILOG);
        ECOMM_STRING(UNILOG_PLA_STRING, build_info, P_SIG, "%s", getBuildInfo());
    }

    /*------- Perform User APP Hardware Initialization Below -----*/
    /* Note: DO NOT put time-consuming operation here (better in appInit function)
       so that we can trigger LPUART recv as soon as possible     */

    ////////// wakeup pad 0 for button wakeup ///////////////
    PMU_WakeupPadInit();

    NVIC_EnableIRQ(PadWakeup0_IRQn);
	
	slpManDeepSlpTimerRegisterExpCb(UserTimerExpFunc);			   // always register a user deepsleep timer callback
	
	CustomInitIsCalled = true;
}

/**
  \fn        void NVIC_WakeupIntHandler(void)
  \brief      NVIC wakeup interrupt handler
  \param     void
 */


void Pad0_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup0_IRQn)==false)
        return;

    // add custom code below //

}

