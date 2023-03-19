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
#include "hal_uart_multi_atcmd.h"

#if (WDT_FEATURE_ENABLE==1)
#include "wdt_ec616s.h"

#define WDT_TIMEOUT_VALUE     (20)            // in unit of second, shall be less than 256s
#endif

extern ARM_DRIVER_USART Driver_USART1;
extern ARM_DRIVER_USART Driver_USART2;

ARM_DRIVER_USART *UsartAtCmd2Handle = NULL;

/*
 *  set at command uart port
 *  AT baud rate is controlled by configure file setting, AT2 baud rate is fixed as 115200
 */
void SetMultiAtUart(void)
{
    UsartAtCmdHandle = &CREATE_SYMBOL(Driver_USART, 1);

    UsartAtCmd2Handle = &CREATE_SYMBOL(Driver_USART, 2);

    uint32_t baudRate = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE);
    uint32_t frameFormat = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_FRAME_FORMAT);

    // check if auto baud rate flag is set
    if(baudRate & 0x80000000UL)
    {
        // re-enable auto baud detection on POR, otherwise keep setting if wakeup from other low power modes
        if(slpManGetLastSlpState() == SLP_ACTIVE_STATE)
        {
            // Shall overwrite previous detection value for corner case:
            // previous detection e.g 9600 is recorded, when power on again, the baud rate is going to change to 115200, however device enter
            // lower poer mode before auto detection is done, after wakeup, it'll mess up, so we need to keep this state sync by saving setting
            if(baudRate & 0x7FFFFFFFUL)
            {
                BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE, 0x80000000UL);
                BSP_SavePlatConfigToFs();
            }

            baudRate = 0; // auto baud rate

        }
        else
        {
            baudRate &= 0x7FFFFFFFUL;
        }
    }
    else
    {
        baudRate &= 0x7FFFFFFFUL;
    }

    atCmdInitUartHandler(UsartAtCmdHandle, baudRate, frameFormat);
    atCmd2InitUartHandler(UsartAtCmd2Handle, 115200);
}

void GPR_SetUartClk(void)
{
    GPR_ClockDisable(GPR_UART0FuncClk);
    GPR_ClockDisable(GPR_UART1FuncClk);
    GPR_ClockDisable(GPR_UART2FuncClk);

    GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);
    GPR_SetClockSrc(GPR_UART1FuncClk, GPR_UART1ClkSel_26M);
    GPR_SetClockSrc(GPR_UART2FuncClk, GPR_UART1ClkSel_26M);

    GPR_ClockEnable(GPR_UART0FuncClk);
    GPR_ClockEnable(GPR_UART1FuncClk);
    GPR_ClockEnable(GPR_UART2FuncClk);

    GPR_SWReset(GPR_ResetUART0Func);
    GPR_SWReset(GPR_ResetUART1Func);
    GPR_SWReset(GPR_ResetUART2Func);
}

#if LOW_POWER_AT_TEST
bool slpEn = true;
void UsrToggleSlpEn(void)
{
    uint32_t mask = SaveAndSetIRQMask();
    if(slpEn)
        slpEn = false;
    else
        slpEn = true;
    RestoreIRQMask(mask);

    ECOMM_TRACE(UNILOG_PLA_APP, UsrSetSlpEn_1, P_VALUE, 1, "UsrToggleSlpEn-%u",slpEn);
}

slpManSlpState_t CheckUsrdefSlpStatus(void)
{
    slpManSlpState_t status = SLP_HIB_STATE;

    uint32_t mask = SaveAndSetIRQMask();
#if NEW_WAKEUP_METHOD
    if(slpEn == false)
#else
    if(((slpManGetWakeupPinValue() & (0x1<<1)) == 0) || ((slpManGetWakeupPinValue() & (0x1<<3)) == 0))         // pad1 or pad3 value is low
#endif
        status = SLP_IDLE_STATE;
    RestoreIRQMask(mask);
    return status;
}

#endif

static void PMU_WakeupPadInit(void)
{
    const padWakeupSettings_t cfg =
    {
/*#if NEW_WAKEUP_METHOD
        true, false,             // group0 posedge, negedge
#else
        false,true,             // group0 posedge, negedge
#endif
        false, true,             // group1 posedge, negedge
    */    false, false,             // group2 posedge, negedge
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

#if LOW_POWER_AT_TEST
    slpManRegisterUsrSlpDepthCb(CheckUsrdefSlpStatus);
#endif

    SetMultiAtUart();

    plat_config_raw_flash_t* rawFlashPlatConfig;
    plat_config_fs_t* fsPlatConfig;

    BSP_LoadPlatConfigFromRawFlash();

    rawFlashPlatConfig = BSP_GetRawFlashPlatConfig();
    fsPlatConfig = BSP_GetFsPlatConfig();

    if(rawFlashPlatConfig && (rawFlashPlatConfig->logControl != 0 ))
    {
        SetUnilogUart(PORT_USART_0, rawFlashPlatConfig->uartBaudRate, true);
        uniLogInitStart(UART_0_FOR_UNILOG);
        ECOMM_STRING(UNILOG_PLA_STRING, build_info, P_SIG, "%s", getBuildInfo());

        if(UsartAtCmdHandle)
        {
            ECOMM_TRACE(UNILOG_PLA_DRIVER, at_port_config, P_SIG, 4, "AT port config: baudRate:%u, dataBits: %u, parity:%u(0->NONE, 1->ODD, 2->EVEN), stopBits:%u",
                                                                        UsartAtCmdHandle->GetBaudRate(),
                                                                        (fsPlatConfig->atPortFrameFormat.config.dataBits == 0) ? 8: 7,
                                                                        fsPlatConfig->atPortFrameFormat.config.parity,
                                                                        (fsPlatConfig->atPortFrameFormat.config.stopBits == 2) ? 2 : 1);
        }

        extern uint32_t PmuSlowCntRead(void);
        ECOMM_TRACE(UNILOG_PLA_DRIVER, Enter_Active_2S, P_VALUE, 1, "EC616S Active FullImg @ SC = %u",PmuSlowCntRead());           // for EPAT to Show EC616 Wakeup
    }

    ////////// wakeup pad 0 for button wakeup ///////////////
    PMU_WakeupPadInit();
    NVIC_EnableIRQ(PadWakeup0_IRQn);
    //NVIC_EnableIRQ(PadWakeup3_IRQn);

#if LOW_POWER_AT_TEST
    ////////// wakeup pad 1 for wakeup by mcu ///////////////
    ////////// negetive edge to wakup, rise high until safe to sleep /////
    NVIC_EnableIRQ(PadWakeup1_IRQn);
#endif
    slpManGetPMUSettings();             // get pmu setting from flash, we can overwrite after that

    slpManStartWaitATTimer();           // start a timer to avoid sleep until slpWaitTime arrived

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

    // add custom code below //

}

/*void Pad1_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup1_IRQn)==false)
        return;

    // add custom code below //
}

void Pad2_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup2_IRQn)==false)
        return;

    // add custom code below //

}

void Pad3_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup3_IRQn)==false)
        return;

    // add custom code below //


}

void Pad4_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup4_IRQn)==false)
        return;

    // add custom code below //
}

void Pad5_WakeupIntHandler(void)
{
    if(slpManExtIntPreProcess(PadWakeup5_IRQn)==false)
        return;

    // add custom code below //

}
*/

