#include "string.h"
#include "bsp.h"
#include "bsp_custom.h"
#include "ostask.h"
#include "debug_log.h"
#include "plat_config.h"
#include "app.h"
#include "FreeRTOS.h"
#include "at_def.h"
#include "at_api.h"
#include "mw_config.h"
#include "slpman_ec616s.h"
#include "hal_uart_atcmd.h"

#include "ps_lib_api.h"

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
#include "autoreg_common.h"
#endif

#include "timer_ec616s.h"
#include "adc_ec616s.h"
#include "osasys.h"
#include "hal_adc.h"
#include "app_user_atcmd.h"

#define RTE_UART_RX_IO_MODE     RTE_UART2_RX_IO_MODE

/** \brief usart receive buffer length in unit of byte */
#define RECV_BUFFER_LEN     (256)

extern ARM_DRIVER_USART Driver_USART2;
static ARM_DRIVER_USART *USARTdrv = &Driver_USART2;

osThreadId_t uart2_handle_task_handle;
const osThreadAttr_t uart2HandleTask_attributes = {
  .name = "userAtCmdTask",
  .priority = (osPriority_t) osPriorityBelowNormal3,
  .stack_size = 1024///128 * 4
};

/* Definitions for uart2HandleSemHandle */
osSemaphoreId_t uart2HandleSemHandle;
const osSemaphoreAttr_t uart2HandleSem_attributes = {
  .name = "uart2HandleSem"
};

/** \brief usart receive buffer */
uint8_t recBuffer[RECV_BUFFER_LEN];
/** \brief receive timeout flag */
volatile bool isRecvTimeout = false;
/** \brief receive complete flag */
volatile bool isRecvComplete = false;

void uart2_set_rxbuff(uint8_t *prxbuff,uint32_t rx_length)
{
	USARTdrv->Receive(prxbuff, rx_length);
}

void uart2_tx(uint8_t *ptxbuff,uint32_t tx_length)
{
	USARTdrv->Send(ptxbuff, tx_length);
}

uint8_t uart2_get_flag_timeout(void)
{
	return 	isRecvTimeout;
}

void uart2_clear_flag_timeout(void)
{
	isRecvTimeout = 0;
}

uint8_t uart2_get_flag_complete(void)
{
	return 	isRecvComplete;
}

void uart2_clear_flag_complete(void)
{
	isRecvComplete = 0;
}

/**
  \fn          void USART_callback(uint32_t event)
  \brief       usart event handler
  \return
*/
void USART2_callback(uint32_t event)
{
    if(event & ARM_USART_EVENT_RX_TIMEOUT)
    {
        isRecvTimeout = true;
//		osSemaphoreRelease(uart2HandleSemHandle);
    }
    if(event & ARM_USART_EVENT_RECEIVE_COMPLETE)
    {
        isRecvComplete = true;
//		osSemaphoreRelease(uart2HandleSemHandle);
    }
}

void uart2_init(void)
{
    /*Initialize the USART driver */
    USARTdrv->Initialize(USART2_callback);

    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);

    /*Configure the USART to 115200 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 115200);

    uint8_t* greetStr = "uart2_init\r\n";

    USARTdrv->Send(greetStr, strlen((const char*)greetStr));    
}


void uart2_handle_task(void * argument)
{
	uint8_t cnt_1s = 0;

	while(1)
	{
//		cnt_1s++;
//		if(cnt_1s >= 100)
//		{
//			cnt_1s = 0;
//			atUartPrint("uart2_handle_task\r\n");
//		}
//		osDelay(10);

		USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);
		osSemaphoreAcquire(uart2HandleSemHandle,osWaitForever);

		atUartPrint("uart2_handle get sem\r\n");
		
		if(isRecvTimeout == true)
        {
            USARTdrv->Send(recBuffer, USARTdrv->GetRxCount());
            isRecvTimeout = false;
        }
        else if(isRecvComplete == true)
        {
            isRecvComplete = false;
            USARTdrv->Send(recBuffer, RECV_BUFFER_LEN);
        }
	}
}

void app_uart2_handle_init(void)
{
	uart2_handle_task_handle	= osThreadNew(uart2_handle_task, NULL, &uart2HandleTask_attributes);
 	uart2HandleSemHandle 		= osSemaphoreNew(1, 0, &uart2HandleSem_attributes);

//	gpio_test_task_handle 	= osThreadNew(gpio_test_task, NULL, &gpioTestTask_attributes);

	uart2_init();

	user_printf("app_uart2_handle_init\r\n");
}

