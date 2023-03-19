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
#include "hal_uart_atcmd.h"
#include "timer_ec616s.h"
#include "rte_device.h"
#ifdef FEATURE_CISONENET_ENABLE
#include "at_onenet_task.h"
#endif
#ifdef FEATURE_CTWINGV1_5_ENABLE
#include "at_ctlwm2m_task.h"
#endif
#if (WDT_FEATURE_ENABLE==1)
#include "wdt_ec616s.h"
#define WDT_TIMEOUT_VALUE     (20)            // in unit of second, shall be less than 256s
#endif


extern ARM_DRIVER_USART Driver_LPUSART1;

/*
 *  set printf and at command uart port
 */
void SetAtUart()
{
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

    UsartAtCmdHandle = &CREATE_SYMBOL(Driver_LPUSART, 1);

    atCmdInitUartHandler(UsartAtCmdHandle, baudRate, frameFormat);
//    atCmdInitUartHandler(UsartAtCmdHandle, 115200, frameFormat);

}

void GPR_SetUartClk(void)
{
    GPR_ClockDisable(GPR_UART0FuncClk);
    GPR_ClockDisable(GPR_UART1FuncClk);

    GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);
    GPR_SetClockSrc(GPR_UART1FuncClk, GPR_UART1ClkSel_26M);

    GPR_ClockEnable(GPR_UART0FuncClk);
    GPR_ClockEnable(GPR_UART1FuncClk);

    GPR_SWReset(GPR_ResetUART0Func);
    GPR_SWReset(GPR_ResetUART1Func);
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
        if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION) == EXCEP_OPTION_SILENT_RESET)
        {
            ResetReasonWrite(RESET_REASON_WDT);
            EC_SystemReset();
            while(1);
        }
        else
        {
            EC_ASSERT(0, 0, 0, 0);
        }
    }
}


void NetLightInit(void)
{
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

    padConfig.mux = NETLIGHT_PAD_ALT_FUNC;
    PAD_SetPinConfig(NETLIGHT_PAD_INDEX, &padConfig);

    // Config PWM clock, source from 26MHz and divide by 1
    CLOCK_SetClockSrc(GPR_TIMER4FuncClk, GPR_TIMER4ClkSel_26M);
    CLOCK_SetClockDiv(GPR_TIMER4FuncClk, 1);

    TIMER_DriverInit();

    TIMER_Netlight_Enable(NETLIGHT_PWM_INSTANCE);

}



static void ecDeepSlpTimerExpFunc(uint8_t id)
{
    ECOMM_TRACE(UNILOG_PLA_APP, ecDeepSlpTimerExpFunc_1, P_SIG, 1, "EC DeepSleep Timer Expired: TimerID = %u",id);

#if ONENET_AUTO_UPDATE_ENABLE
	// timer id 6 (defined in at_onenet_task.h) is used for auto update when onenet lifetime is expired
	// do not use this timer id if onenet auto update is enabled(the macro is defined in at_onenet_task.h). 
	if(id == CIS_LIFETIMEOUT_ID)
	{
		cis_lifeTimeExpCallback();
		
	}
#endif

#if CTLWM2M_AUTO_UPDATE_ENABLE
	// timer id 5 (defined in at_ctlwm2m_task.h) is used for auto update when ct lifetime is expired
	// do not use this timer id if ct auto update is enabled(the macro is defined in at_ctlwm2m_task.h). 
	if(id == CTLWM2M_LIFETIMEOUT_ID)
	{
		ctlwm2m_lifeTimeExpCallback();
	}
#endif
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

    SetAtUart();

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
        ECOMM_TRACE(UNILOG_PLA_DRIVER, Enter_Active_2, P_VALUE, 1, "EC616 Active FullImg @ SC = %u",PmuSlowCntRead());           // for EPAT to Show EC616 Wakeup
    }

    ResetReasonInit();  // check and print reset reason

    /*------- Perform User APP Hardware Initialization Below -----*/
    /* Note: DO NOT put time-consuming operation here (better in appInit function)
       so that we can trigger LPUART recv as soon as possible     */

    ////////// wakeup pad 0 for button wakeup ///////////////
    PMU_WakeupPadInit();

    NVIC_EnableIRQ(PadWakeup0_IRQn);

    if((slpManGetWakeupSrc() == WAKEUP_FROM_PAD) || (slpManGetLatchPad() != 0) || (slpManGetWakeupSrc() == WAKEUP_FROM_UART))
    {
        slpManStartWaitATTimer();           // start a timer to avoid sleep until slpWaitTime arrived
    }

#if (WDT_FEATURE_ENABLE == 1)
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_START_WDT))
    {
        BSP_WdtInit();
        WDT_Start();
    }
#endif

    NetLightInit();

	slpManDeepSlpTimerRegisterExpCb(ecDeepSlpTimerExpFunc);
	
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

