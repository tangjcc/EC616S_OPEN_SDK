/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616S driver example entry source file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

#include "bsp.h"
#include "bsp_custom.h"
#include "app.h"

// Choose which example to run
static example_id_t exampleId = USART_EX;

void main_entry(void)
{
    BSP_CommonInit();

    //printf IO retarget for debug purpose
    //if(exampleId != USART_EX)
    {
        BSP_CustomInit();
    }

    switch(exampleId)
    {
        case USART_EX:
            USART_ExampleEntry();
            break;

        case SPI_EX:
            SPI_ExampleEntry();
            break;

        case I2C_EX:
            I2C_ExampleEntry();
            break;

        case GPIO_EX:
            GPIO_ExampleEntry();
            break;

        case TIMER_EX:
            TIMER_ExampleEntry();
            break;

        case WDT_EX:
            WDT_ExampleEntry();
            break;

        case DMA_EX:
            DMA_ExampleEntry();
            break;

        case ADC_EX:
            ADC_ExampleEntry();
            break;
        case TLS_EX:
            TLS_ExampleEntry();
            break;

        default:
            break;
    }

    while(1);

}

