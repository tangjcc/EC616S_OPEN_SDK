#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "gpr_ec616s.h"
#include "wdt_ec616s.h"
#include "bl_bsp.h"

#ifndef T_USART
#define T_USART USART_TypeDef
#endif


#ifndef USART_0
#define USART_0   ((T_USART*)USART0_BASE_ADDR)
#endif

#ifndef USART_1
#define USART_1   ((T_USART*)USART1_BASE_ADDR)
#endif

#define EXCEPT_AUTO_REBOOT_FLAG 1

#ifndef EXCEPT_AUTO_REBOOT_FLAG
#define EXCEPT_AUTO_REBOOT_FLAG 0
#endif

#define USART_LSR_TX_EMPTY         ((uint32_t)0x00000040)

static void execpt_usart_divide_latch(T_USART *usart,unsigned int val)
{
	unsigned int rd_data = 0;
	rd_data = usart->LCR;
	rd_data = (rd_data&0xffffff7f )|(val <<7);
	usart->LCR = rd_data;
}

static void except_usart_divided(T_USART *usart,unsigned int dlh,unsigned int dll)
{
	execpt_usart_divide_latch(usart,1);
	usart->DLH = dlh;
	usart->DLL = dll;
	execpt_usart_divide_latch(usart,0);
}

static void except_usart_init(T_USART *usart)
{

	//uart pimux set up
	//EC616S should pull up, config 510
	*((uint32_t *)0x4408003c)=0x00000010;//RX
	*((uint32_t *)0x44080040)=0x00000010;//TX

	except_usart_divided(usart,0,27);
	//usart->EFCR = 0x700;
	
	usart->IER = 0x0f;
	usart->FCR = 0x51;
	usart->LCR = 0x3;
	usart->MFCR = 0x1;
}

static void except_usart_send(T_USART *usart,uint16_t Data)
{
	usart->THR = (Data & (uint16_t)0x01FF);;
}

static int except_usart_is_txrdy(T_USART *usart)
{
	int bitstatus = 0;
	if ( (usart->LSR & USART_LSR_TX_EMPTY) != 0)
	{
		bitstatus = 1;
	}
	else
	{
		bitstatus = 0;
	}

	return bitstatus;
}

int except_fputc(int ch, FILE *f)
{
    unsigned int cnt  =0;
    except_usart_send(USART_0,(uint8_t) ch);

    while (1)
    {
        if (except_usart_is_txrdy(USART_0) != 0)
        {
            //tx finish
            break;
        }    
        if (cnt > 70000)
        {
            //when timeout just return
            break;
        }      
        cnt++;
    };
    return (ch);
}

void except_trace(char*log,int len)
{
    int i;      
    for(i=0;i<len;i++)
    {
        except_fputc(*((char *)log+i),NULL);
    }
}

static void except_config_wdt(void)
{
    //when rx data received, do not reboot    
    #if (EXCEPT_AUTO_REBOOT_FLAG !=0)
        //config wdt
        BSP_WdtInit();
        WDT_Start();
    #else
        WDT_Stop();
    #endif
}


void exception_init(void)
{
    int i;      

    //reset uart0, 51M, 115200
    GPR_ClockEnable(GPR_UART0APBClk) ;
    GPR_ClockDisable(GPR_UART0FuncClk);
    GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_51M);    
    GPR_ClockEnable(GPR_UART0FuncClk);    
    GPR_SWReset(GPR_ResetUART0Func);

    for(i=0;i<70000;i++)
    {
    }
    except_usart_init(USART_0);
        
    except_config_wdt();
    except_trace("BL Exception called!\r\n", sizeof("BL Exception called!\r\n"));
    
#if (EXCEPT_AUTO_REBOOT_FLAG !=0)
    except_trace("Waiting wdg reset!\r\n", sizeof("Waiting wdg reset!\r\n"));
#endif
}

__asm void CommonFault_Handler(void)
{
        extern exception_init

        PRESERVE8
        push {lr}
        ldr r0, =exception_init 
        blx  r0
        pop {lr}
        bx lr
}

__asm void HardFault_Hook_Handler(void)
{
        PRESERVE8
        push {lr}
        ldr r0, =CommonFault_Handler
        blx r0
        b .
        nop
        nop
}


