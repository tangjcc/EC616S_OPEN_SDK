/****************************************************************************
 *
 * Copy right:   2018 Copyrigths of EigenComm Ltd.
 * File name:    bsp_custom.c
 * Description:
 * History:
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "clock_ec616s.h"
#include "bsp_custom.h"
#include "slpman_ec616s.h"
#include "plat_config.h"
#include "debug_log.h"
#include "os_exception.h"
#include "osasys.h"

#if (WDT_FEATURE_ENABLE==1)
#include "wdt_ec616s.h"

#define WDT_TIMEOUT_VALUE     (20)            // in unit of second, shall be less than 256s
#endif

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

#if (WDT_FEATURE_ENABLE == 1)

/*
 *  WDT Initialize, wdt timeout value is 20s
 *  Parameter:   none
 */
void BSP_WdtInit(void)
{
    // Config WDT clock, source from 32.768KHz and divide by WDT_TIMEOUT_VALUE
    GPR_SetClockSrc(GPR_WDGFuncClk, GPR_WDGClkSel_32K);
    GPR_SetClockDiv(GPR_WDGFuncClk, WDT_TIMEOUT_VALUE);

    wdt_config_t wdtConfig;
    wdtConfig.mode = WDT_InterruptResetMode;
    wdtConfig.timeoutValue = 32768U;
    WDT_Init(&wdtConfig);
}

#endif

void NMI_Handler()
{
    ECOMM_TRACE(UNILOG_PLA_APP, enter_NMI_handler, P_ERROR, 0, "WDT timeout!!! Enter NMI Handler!!!");
    // If we have been in exception handler excecution, we shall resume it.
    if(is_in_excep_handler())
    {
        WDT_Stop();
    }
    else
    {
    	ResetReasonWrite(RESET_REASON_WDT);
		EC_SystemReset();
        while(1);
    }
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
        extern uint32_t PmuSlowCntRead(void);
        ECOMM_TRACE(UNILOG_PLA_DRIVER, Enter_Active_2, P_VALUE, 1, "EC616 Active FullImg @ SC = %u",PmuSlowCntRead());           // for EPAT to Show EC616 Wakeup
    }

    /*------- Perform User APP Hardware Initialization Below -----*/
    /* Note: DO NOT put time-consuming operation here (better in appInit function)
       so that we can trigger LPUART recv as soon as possible     */

    ////////// wakeup pad 0 for button wakeup ///////////////
    PMU_WakeupPadInit();

    NVIC_EnableIRQ(PadWakeup0_IRQn);

#if (WDT_FEATURE_ENABLE == 1)
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_START_WDT))
    {
        BSP_WdtInit();
        WDT_Start();
    }
#endif

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

	ECOMM_TRACE(UNILOG_PLA_DRIVER, Pad0_WakeupIntHandler_1, P_VALUE, 1, "wakeup pad0_0x%x", slpManGetWakeupPinValue());

    // add custom code below //

}

