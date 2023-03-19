/****************************************************************************
 *
 * Copy right:   2018 Copyrigths of EigenComm Ltd.
 * File name:    bsp_custom.c
 * Description:
 * History:
 *
 ****************************************************************************/
#include "bsp_custom.h"

extern ARM_DRIVER_USART Driver_USART0;
extern ARM_DRIVER_USART Driver_USART1;

/*
 *  set printf uart port
 *  Parameter:      port: for printf
 */
static void SetPrintUart(usart_port_t port)
{
    if(port == PORT_USART_0)
    {
#if (RTE_UART0)
        UsartPrintHandle = &CREATE_SYMBOL(Driver_USART, 0);
        GPR_ClockDisable(GPR_UART0FuncClk);
        GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);
        GPR_ClockEnable(GPR_UART0FuncClk);
        GPR_SWReset(GPR_ResetUART0Func);
#endif
    }
    else if(port == PORT_USART_1)
    {
#if (RTE_UART1)
        UsartPrintHandle = &CREATE_SYMBOL(Driver_USART, 1);
        GPR_ClockDisable(GPR_UART1FuncClk);
        GPR_SetClockSrc(GPR_UART1FuncClk, GPR_UART1ClkSel_26M);
        GPR_ClockEnable(GPR_UART1FuncClk);
        GPR_SWReset(GPR_ResetUART1Func);
#endif
    }

    if(UsartPrintHandle == NULL)
        return;

    UsartPrintHandle->Initialize(NULL);
    UsartPrintHandle->PowerControl(ARM_POWER_FULL);
    UsartPrintHandle->Control(ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 |
                        ARM_USART_PARITY_NONE       | ARM_USART_STOP_BITS_1 |
                        ARM_USART_FLOW_CONTROL_NONE, 115200ul);
}

/*
 *  custom board related init
 *  Parameter:   none
 */
void BSP_CustomInit(void)
{
    SetPrintUart(PORT_USART_1);
}

