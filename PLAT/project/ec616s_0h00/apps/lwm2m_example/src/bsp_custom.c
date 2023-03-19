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

#if LOW_POWER_AT_TEST
slpManSlpState_t CheckUsrdefSlpStatus(void)
{
    slpManSlpState_t status = SLP_HIB_STATE;
    if((slpManGetWakeupPinValue() & (0x1<<1)) == 0)         // pad1 value is low
        status = SLP_IDLE_STATE;
    else
        status = SLP_HIB_STATE;

    return status;
}

#endif

static void PMU_WakeupPadInit(void)
{
    const padWakeupSettings_t cfg =
    {
        false, true             // group0 posedge, negedge

    };

    slpManSetWakeupPad(cfg);
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

#if LOW_POWER_AT_TEST
    slpManRegisterUsrSlpDepthCb(CheckUsrdefSlpStatus);
#endif

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
    NVIC_EnableIRQ(PadWakeup0_IRQn);

#if LOW_POWER_AT_TEST
    ////////// wakeup pad 1 for wakeup by mcu ///////////////
    ////////// negetive edge to wakup, rise high until safe to sleep /////
    NVIC_EnableIRQ(PadWakeup1_IRQn);
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

    // add custom code below //

}

