/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    gpio_demo.c
 * Description:  EC616S GPIO driver example entry source file
 * History:      Rev1.0   2018-07-17
 *
 ****************************************************************************/
#include <stdio.h>
#include "pad_ec616s.h"
#include "gpio_ec616s.h"
#include "ic_ec616s.h"

#include "timer_ec616s.h"
#include "clock_ec616s.h"

/*
   This project demotrates the usage of GPIO, in this example, we'll toggle a LED on each button's press
 */

/*
   There's gap between GPIO names in pinmux table given in datasheet and actual paramters used in GPIO API.

   The pinmux table turns out to be like this:

   Pad name            Pad off            AltFunc0          AltFunc1          AltFunc2          ...     AltFunc7
    GPIO0                 11                GPIO0          SPI0_SSn1          UART2_RXD         ...
    GPIO1                 12                GPIO2                             UART2_TXD         ...
    ...                   ...                ...             ...                ...             ...       ...
    GPIO16                21                GPIO16         UART1_RTSn         SPI0_SSn0
    GPIO17                27                GPIO17         I2C1_SCL

   You can see GPIO is indexed sequentially from 0 to max GPIO numbers in pinmux table, however one GPIO instance can only control max 16 GPIOs.

   Suppose the prototype of GPIO API is:
   void GPIO_PinConfig(uint32_t port, uint16_t pin, const gpio_pin_config_t *config);

   The fomula to deduce GPIO API parameters from GPIO index is:

   port = Index / 16
   pin  = Index % 16

   Example:
   To configure GPIO0, index is 0, port = 0 / 16 = 0, pin = 0 % 16 = 0;
   To configure GPIO17, index is 17, port = 17 / 16 = 1, pin = 17 % 16 = 1;
 */

/** \brief Button location */
#define BUTTON_GPIO_INSTANCE     (0)
#define BUTTON_GPIO_PIN          (1)
#define BUTTON_PAD_INDEX         (12)
#define BUTTON_PAD_ALT_FUNC      (PAD_MuxAlt0)

/** \brief LED location */
#define LED_GPIO_INSTANCE        (0)
#define LED_GPIO_PIN             (5)
#define LED_PAD_INDEX            (16)
#define LED_PAD_ALT_FUNC         (PAD_MuxAlt0)

/** \brief TIMER instance for PWM duty cycle control */
#define TIMER0_INSTANCE                   (0)
/** \brief TIMER clock id for clock configuration */
#define TIMER0_CLOCK_ID                   (GPR_TIMER0FuncClk)
/** \brief TIMER clock source select */
#define TIMER0_CLOCK_SOURCE               (GPR_TIMER0ClkSel_32K)
/** \brief TIMER instance IRQ number */
#define TIMER0_INSTANCE_IRQ               (PXIC_Timer0_IRQn)


/** \brief BUTTON press count, LED is toggled on each button's press */
static volatile int gpioInterruptCount = 0;

static uint8_t b_timer0_int = 0;
static uint8_t state = 0;
/**
  \fn          void Timer0_ISR()
  \brief       Timer1 interrupt service routine
  \return
*/
void Timer0_int_handle()
{
    if (TIMER_GetInterruptFlags(TIMER0_INSTANCE) & TIMER_Match0InterruptFlag)
    {
		b_timer0_int = 1;
        TIMER_ClearInterruptFlags(TIMER0_INSTANCE, TIMER_Match0InterruptFlag);
    }
}

void timer0_init(void)
{
	printf("timer0_init\r\n");

    TIMER_DriverInit();

	// TIMER0 config
    // Config TIMER clock, source from 32.768KHz and divide by 1
    CLOCK_SetClockSrc(TIMER0_CLOCK_ID, TIMER0_CLOCK_SOURCE);
    CLOCK_SetClockDiv(TIMER0_CLOCK_ID, 1);

    // Config TIMER0 period as 500ms, match0 value is 16384 = 0x4000
//    timer_config_t timerConfig;
//    TIMER_GetDefaultConfig(&timerConfig);
//    timerConfig.reloadOption = TIMER_ReloadOnMatch0;
//    timerConfig.match0 = 0x4000;

    // Config TIMER0 period as 1000ms, match0 value is 32768 = 0x8000
    timer_config_t timerConfig;
    TIMER_GetDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_ReloadOnMatch0;
    timerConfig.match0 = 0x8000;

    TIMER_Init(TIMER0_INSTANCE, &timerConfig);

    TIMER_InterruptConfig(TIMER0_INSTANCE, TIMER_Match0Select, TIMER_InterruptLevel);
    TIMER_InterruptConfig(TIMER0_INSTANCE, TIMER_Match1Select, TIMER_InterruptDisabled);
    TIMER_InterruptConfig(TIMER0_INSTANCE, TIMER_Match2Select, TIMER_InterruptDisabled);

    // Enable TIMER0 IRQ
    XIC_SetVector(TIMER0_INSTANCE_IRQ, Timer0_int_handle);
    XIC_EnableIRQ(TIMER0_INSTANCE_IRQ);

    TIMER_Start(TIMER0_INSTANCE);
}

/**
  \fn          void GPIO_ISR()
  \brief       GPIO interrupt service routine
  \return
*/
void GPIO_ISR()
{
    //Save current irq mask and diable whole port interrupts to get rid of interrupt overflow
    uint16_t portIrqMask = GPIO_SaveAndSetIRQMask(BUTTON_GPIO_INSTANCE);

    if (GPIO_GetInterruptFlags(BUTTON_GPIO_INSTANCE) & (1 << BUTTON_GPIO_PIN))
    {
        gpioInterruptCount++;
        GPIO_ClearInterruptFlags(BUTTON_GPIO_INSTANCE, 1 << BUTTON_GPIO_PIN);
    }

    GPIO_RestoreIRQMask(BUTTON_GPIO_INSTANCE, portIrqMask);
}

void gpio_all_init(void)
{
//	uint8_t i;

    printf("MCU_EC616S gpio_all_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// TEST pin pad config
    padConfig.mux = LED_PAD_ALT_FUNC;
	PAD_SetPinConfig(0+11, &padConfig);	
	PAD_SetPinConfig(1+11, &padConfig);	
	PAD_SetPinConfig(2+11, &padConfig);	
	PAD_SetPinConfig(3+11, &padConfig);	
	PAD_SetPinConfig(4+11, &padConfig);	
	PAD_SetPinConfig(5+11, &padConfig);	
	PAD_SetPinConfig(6+11, &padConfig);	
	PAD_SetPinConfig(7+11, &padConfig);	
	PAD_SetPinConfig(8+11, &padConfig);	
	PAD_SetPinConfig(9+11, &padConfig);
	PAD_SetPinConfig(10+11, &padConfig);	
	PAD_SetPinConfig(11+11, &padConfig);	
	PAD_SetPinConfig(12+11, &padConfig);	
	PAD_SetPinConfig(13+11, &padConfig);	
//	PAD_SetPinConfig(14+11, &padConfig);	
//	PAD_SetPinConfig(15+11, &padConfig);	

    // TEST pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;

	GPIO_PinConfig(0, 0, &config);
	GPIO_PinConfig(0, 1, &config);
	GPIO_PinConfig(0, 2, &config);
	GPIO_PinConfig(0, 3, &config);
	GPIO_PinConfig(0, 4, &config);
	GPIO_PinConfig(0, 5, &config);
	GPIO_PinConfig(0, 6, &config);
	GPIO_PinConfig(0, 7, &config);
	GPIO_PinConfig(0, 8, &config);
	GPIO_PinConfig(0, 9, &config);
	GPIO_PinConfig(0, 10, &config);
	GPIO_PinConfig(0, 11, &config);
	GPIO_PinConfig(0, 12, &config);
	GPIO_PinConfig(0, 13, &config);
//	GPIO_PinConfig(0, 14, &config);
//	GPIO_PinConfig(0, 15, &config);
}

void GPIO_ExampleEntry(void)
{
    printf("GPIO Example Start\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

    padConfig.pullSelect = PAD_PullInternal;
    padConfig.pullUpEnable = PAD_PullUpEnable;
    padConfig.mux = BUTTON_PAD_ALT_FUNC;
    PAD_SetPinConfig(BUTTON_PAD_INDEX, &padConfig);

    padConfig.mux = LED_PAD_ALT_FUNC;
    PAD_SetPinConfig(LED_PAD_INDEX, &padConfig);

    // LED pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;

    GPIO_PinConfig(LED_GPIO_INSTANCE, LED_GPIO_PIN, &config);

    // Button pin config
    config.pinDirection = GPIO_DirectionInput;
    config.misc.interruptConfig = GPIO_InterruptRisingEdge;

    GPIO_PinConfig(BUTTON_GPIO_INSTANCE, BUTTON_GPIO_PIN, &config);

    // Enable IRQ
    XIC_SetVector(PXIC_Gpio_IRQn, GPIO_ISR);
    XIC_EnableIRQ(PXIC_Gpio_IRQn);

	gpio_all_init();
	timer0_init();

    while(1)
    {
//        if (gpioInterruptCount & 0x1)
//        {
//            GPIO_PinWrite(LED_GPIO_INSTANCE, 1 << LED_GPIO_PIN, 1 << LED_GPIO_PIN);
//        }
//        else
//        {
//            GPIO_PinWrite(LED_GPIO_INSTANCE, 1 << LED_GPIO_PIN, 0);
//        }

//		GPIO_PinWrite(0, 1 << 8, 1 << 8);
//		GPIO_PinWrite(0, 1 << 9, 1 << 9);
//		Delay(1);
//		GPIO_PinWrite(0, 1 << 8, 0);
//		GPIO_PinWrite(0, 1 << 9, 0);
// 		Delay(1);       

		if(b_timer0_int)
		{
			b_timer0_int = 0;

			printf("b_timer0_int\r\n");

			if(state)
			{
				state = 0;
				GPIO_PinWrite(0, 1 << 7, 0);
				GPIO_PinWrite(0, 1 << 8, 0);
				GPIO_PinWrite(0, 1 << 9, 0);
			}
			else
			{
				state = 1;
				GPIO_PinWrite(0, 1 << 7, 1 << 7);
				GPIO_PinWrite(0, 1 << 8, 1 << 8);
				GPIO_PinWrite(0, 1 << 9, 1 << 9);
			}
		}

    }
}

