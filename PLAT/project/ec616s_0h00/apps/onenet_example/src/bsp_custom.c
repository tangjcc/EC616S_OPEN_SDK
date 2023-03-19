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

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "clock_ec616.h"
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "clock_ec616s.h"
#include "slpman_ec616s.h"
#endif

#include "cisAsynEntry.h"

#include "bsp_custom.h"
#include "plat_config.h"
#include "debug_log.h"
#include "os_exception.h"

#if (WDT_FEATURE_ENABLE==1)
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "wdt_ec616.h"
#elif defined CHIP_EC616S
#include "wdt_ec616s.h"
#endif

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
    #if defined CHIP_EC616 || defined CHIP_EC616_Z0
    const padWakeupSettings_t cfg =
    {
        false, true,             // group0 posedge, negedge
        false, true,             // group1 posedge, negedge
        false, false,             // group2 posedge, negedge
    };
    #elif defined CHIP_EC616S
    const padWakeupSettings_t cfg =
    {
        false, true,             // group0 posedge, negedge
    };
    #endif

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

#endif


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

	slpManGetPMUSettings();				// get pmu setting from flash, we can overwrite after that

    ////////// wakeup pad 0 for button wakeup ///////////////
    PMU_WakeupPadInit();
    #if defined CHIP_EC616 || defined CHIP_EC616_Z0
    NVIC_EnableIRQ(PadWakeup0_IRQn);
    NVIC_EnableIRQ(PadWakeup3_IRQn);
    #elif defined CHIP_EC616S
    NVIC_EnableIRQ(PadWakeup0_IRQn);
    #endif

#if (WDT_FEATURE_ENABLE == 1)
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_START_WDT))
    {
        BSP_WdtInit();
        WDT_Start();
    }
#endif
	
	CustomInitIsCalled = true;

    slpManDeepSlpTimerRegisterExpCb(onenetDeepSleepCb);
}






#if defined CHIP_EC616 || defined CHIP_EC616_Z0
void Pad0_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup0_IRQn)==false)
        return;

    // add custom code below //

}

void Pad3_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup3_IRQn)==false)
        return;

    // add custom code below //

    ECOMM_TRACE(UNILOG_PLA_DRIVER, Pad3_WakeupIntHandler_2, P_VALUE, 1, "wakeup pad3_0x%x", slpManGetWakeupPinValue());
}
#elif defined CHIP_EC616S

void Pad0_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup0_IRQn)==false)
        return;
    ECOMM_TRACE(UNILOG_PLA_DRIVER, Pad0_WakeupIntHandler_1, P_VALUE, 1, "wakeup pad0_0x%x", slpManGetWakeupPinValue());

    // add custom code below //

}

#endif


