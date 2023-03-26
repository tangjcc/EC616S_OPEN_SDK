#include <stdio.h>
#include <string.h>
#include "bsp.h"

/*
   This project demotrates the usage of USART, in this example, we'll echo user's input character one by one,
   USART work mode can be configured in RTE_Device.h to enable polling/interrupt/dma mode respectively.
 */

/** \brief usart receive buffer length in unit of byte */
#define RECV_BUFFER_LEN     (128)

//#if(RTE_UART0)

//#define RTE_UART_RX_IO_MODE     RTE_UART0_RX_IO_MODE
//extern ARM_DRIVER_USART Driver_USART0;
//static ARM_DRIVER_USART *USARTdrv = &Driver_USART0;

//#elif(RTE_UART1)

//#define RTE_UART_RX_IO_MODE     RTE_UART1_RX_IO_MODE
//extern ARM_DRIVER_USART Driver_LPUSART1;
//static ARM_DRIVER_USART *USARTdrv = &Driver_LPUSART1;


//#elif(RTE_UART2)

//#define RTE_UART_RX_IO_MODE     RTE_UART2_RX_IO_MODE
//extern ARM_DRIVER_USART Driver_USART2;
//static ARM_DRIVER_USART *USARTdrv = &Driver_USART2;

//#endif

#define RTE_UART_RX_IO_MODE     RTE_UART2_RX_IO_MODE
extern ARM_DRIVER_USART Driver_USART2;
static ARM_DRIVER_USART *USARTdrv = &Driver_USART2;


/** \brief usart receive buffer */
uint8_t recBuffer[RECV_BUFFER_LEN];

/** \brief receive timeout flag */
volatile bool isRecvTimeout = false;

/** \brief receive complete flag */
volatile bool isRecvComplete = false;

/**
  \fn          void USART_callback(uint32_t event)
  \brief       usart event handler
  \return
*/
void USART_callback(uint32_t event)
{
    if(event & ARM_USART_EVENT_RX_TIMEOUT)
    {
        isRecvTimeout = true;
    }
    if(event & ARM_USART_EVENT_RECEIVE_COMPLETE)
    {
        isRecvComplete = true;
    }
}

void USART_ExampleEntry_org(void)
{
    /*Initialize the USART driver */
    USARTdrv->Initialize(USART_callback);

    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);

    /*Configure the USART to 115200 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 115200);

    uint8_t* greetStr = "UART Echo Demo\n";

    USARTdrv->Send(greetStr, strlen((const char*)greetStr));

    while (1)
    {

        // In polling mode, receive api will block until required recv len is reached.
        // In interrupt or dma mode, receive api returns immdiately after transation starts.  

#if(RTE_UART_RX_IO_MODE == POLLING_MODE)   
        USARTdrv->Receive(recBuffer, 1);
        USARTdrv->Send(recBuffer, 1);
#elif (RTE_UART_RX_IO_MODE == IRQ_MODE) || (RTE_UART_RX_IO_MODE == DMA_MODE)

        USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);

        while((isRecvTimeout == false) && (isRecvComplete == false));

        if(isRecvTimeout == true)
        {
            USARTdrv->Send(recBuffer, USARTdrv->GetRxCount());
            isRecvTimeout = false;
        }
        else
        {
            isRecvComplete = false;
            USARTdrv->Send(recBuffer, RECV_BUFFER_LEN);
        }
#else
#error "UART RX IO MODE IS NOT SUPPORTED IN THIS EXAMPLE"
#endif
    }
}

void USART_ExampleEntry(void)
{
	printf("USART_ExampleEntry\r\n");
    /*Initialize the USART driver */
    USARTdrv->Initialize(USART_callback);

    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);

    /*Configure the USART to 115200 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 9600); /*GPS 9600; OLD:115200*/

    uint8_t* greetStr = "UART Echo Demo\n";

    USARTdrv->Send(greetStr, strlen((const char*)greetStr));

    while (1)
    {
		printf("Waiting...\r\n");
        // In polling mode, receive api will block until required recv len is reached.
        // In interrupt or dma mode, receive api returns immdiately after transation starts.  

#if(RTE_UART_RX_IO_MODE == POLLING_MODE)   
        USARTdrv->Receive(recBuffer, 1);
        USARTdrv->Send(recBuffer, 1);
#elif (RTE_UART_RX_IO_MODE == IRQ_MODE) || (RTE_UART_RX_IO_MODE == DMA_MODE)

        USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);
		char ss[128] = {0};
		strcpy(ss, recBuffer);
		printf(">>%s\r\n", ss);
		printf("[0x%02x0x%02x0x%02x0x%02x]\r\n", recBuffer[0], recBuffer[1], recBuffer[2], recBuffer[3]);

        while((isRecvTimeout == false) && (isRecvComplete == false));

        if(isRecvTimeout == true)
        {
            USARTdrv->Send(recBuffer, USARTdrv->GetRxCount());
            isRecvTimeout = false;
        }
        else
        {
            isRecvComplete = false;
            USARTdrv->Send(recBuffer, RECV_BUFFER_LEN);
        }
		//memset(recBuffer, 0, sizeof(recBuffer));
#else
#error "UART RX IO MODE IS NOT SUPPORTED IN THIS EXAMPLE"
#endif
    }
}

