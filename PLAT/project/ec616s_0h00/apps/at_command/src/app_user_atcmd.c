#include "app.h"

#include "string.h"
#include "bsp.h"
#include "bsp_custom.h"
#include "ostask.h"
#include "debug_log.h"
#include "plat_config.h"
#include "FreeRTOS.h"
#include "at_def.h"
#include "at_api.h"
#include "mw_config.h"
#include "hal_uart_atcmd.h"

#include "ps_lib_api.h"

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
#include "autoreg_common.h"
#endif

#if (USE_MCU == MCU_EC616S)
#include "slpman_ec616s.h"
#include "timer_ec616s.h"
#include "adc_ec616s.h"
#else 
#include "slpman_ec616.h"
#include "timer_ec616.h"
#include "adc_ec616.h"
#endif

#include "osasys.h"
#include "hal_adc.h"

/** \brief TEST location */
#define TEST_GPIO_INSTANCE        (0)
#define TEST_GPIO_PIN             (12)
#define TEST_PAD_INDEX            (11+TEST_GPIO_PIN)
#define TEST_PAD_ALT_FUNC         (PAD_MuxAlt0)

/** \brief INPUT location */
#define INPUT_GPIO_INSTANCE        (0)
#define INPUT_GPIO_PIN             (13)
#define INPUT_PAD_INDEX            (11+INPUT_GPIO_PIN)
#define INPUT_PAD_ALT_FUNC         (PAD_MuxAlt0)

#define AT24C02_ADDR	(0xA0>>1)

#define RetCheck(cond, v1)                  \
do{                                         \
    if (cond == ARM_DRIVER_OK)              \
    {                                       \
        user_printf("%s OK\n", (v1));            \
    } else {                                \
        user_printf("%s Failed !!!\n", (v1));    \
    }                                       \
}while(FALSE)

/** \brief PWM1 initial duty cycle percent */
#define PWM1_INIT_DUTY_CYCLE_PERCENT      (25)
/** \brief TIMER/PWM Alt function select */
#define PWM1_OUT_ALT_SEL                  (PAD_MuxAlt5)
/** \brief TIMER/PWM out pin */
#define PWM1_OUT_PIN                      (0)
/** \brief PWM1 clock id for clock configuration */
#define PWM1_CLOCK_ID                     (GPR_TIMER1FuncClk)
/** \brief PWM1 clock source select */
#define PWM1_CLOCK_SOURCE                 (GPR_TIMER1ClkSel_26M)
/** \brief TIMER instance for PWM1 output */
#define PWM1_INSTANCE                     (1)

/** \brief PWM1 initial duty cycle percent */
#define PWM4_INIT_DUTY_CYCLE_PERCENT      (75)
/** \brief TIMER/PWM Alt function select */
#define PWM4_OUT_ALT_SEL                  (PAD_MuxAlt5)
/** \brief TIMER/PWM out pin */
#define PWM4_OUT_PIN                      (9)
/** \brief PWM1 clock id for clock configuration */
#define PWM4_CLOCK_ID                     (GPR_TIMER4FuncClk)
/** \brief PWM1 clock source select */
#define PWM4_CLOCK_SOURCE                 (GPR_TIMER4ClkSel_26M)
/** \brief TIMER instance for PWM1 output */
#define PWM4_INSTANCE                     (4)

/** \brief PWM1 initial duty cycle percent */
#define PWM5_INIT_DUTY_CYCLE_PERCENT      (25)
/** \brief TIMER/PWM Alt function select */
#define PWM5_OUT_ALT_SEL                  (PAD_MuxAlt5)
/** \brief TIMER/PWM out pin */
#define PWM5_OUT_PIN                      (10)
/** \brief PWM1 clock id for clock configuration */
#define PWM5_CLOCK_ID                     (GPR_TIMER5FuncClk)
/** \brief PWM1 clock source select */
#define PWM5_CLOCK_SOURCE                 (GPR_TIMER5ClkSel_26M)
/** \brief TIMER instance for PWM1 output */
#define PWM5_INSTANCE                     (5)

/** \brief TIMER instance for PWM duty cycle control */
#define TIMER0_INSTANCE                   (0)
/** \brief TIMER clock id for clock configuration */
#define TIMER0_CLOCK_ID                   (GPR_TIMER0FuncClk)
/** \brief TIMER clock source select */
#define TIMER0_CLOCK_SOURCE               (GPR_TIMER0ClkSel_32K)
/** \brief TIMER instance IRQ number */
#define TIMER0_INSTANCE_IRQ               (PXIC_Timer0_IRQn)

/** \brief SPI data width */
#define TRANFER_DATA_WIDTH_8_BITS        (8)
#define TRANFER_DATA_WIDTH_16_BITS       (16)
#define TRANSFER_DATA_WIDTH              TRANFER_DATA_WIDTH_8_BITS

osThreadId_t gpio_test_task_handle;
const osThreadAttr_t gpioTestTask_attributes = {
  .name = "gpioTestTask",
  .priority = (osPriority_t) osPriorityNormal,//osPriorityBelowNormal2,
  .stack_size = 2048///128 * 4
};

osThreadId_t user_atcmd_task_handle;
const osThreadAttr_t userAtCmdTask_attributes = {
  .name = "userAtCmdTask",
  .priority = (osPriority_t) osPriorityNormal1,
  .stack_size = 2048///128 * 4
};

/* Definitions for userAtCmdSemHandle */
osSemaphoreId_t userAtCmdSemHandle;
const osSemaphoreAttr_t userAtCmdSem_attributes = {
  .name = "userAtCmdSem"
};

uint32_t at_cmd;
uint8_t  b_gpio_input;
uint8_t  at_param[64];
uint8_t  at_param_length;
uint8_t	 b_test_init = 0;
uint8_t  gpio_port = 0;
uint8_t  gpio_number = 0;
uint8_t  b_adc_print = 0;
uint8_t  b_gpio_toggle = 0;

static volatile uint32_t conversionDone = 0;
static volatile uint32_t aio1ChannelResult = 0;
static volatile uint32_t aio2ChannelResult = 0;
static volatile uint32_t aio3ChannelResult = 0;
static volatile uint32_t aio4ChannelResult = 0;
static volatile uint32_t *padc_result;

uint8_t adc_channel;

extern ARM_DRIVER_I2C Driver_I2C0;
static ARM_DRIVER_I2C   *i2cDrvInstance = &CREATE_SYMBOL(Driver_I2C, 0);

/** \brief driver instance declare */
extern ARM_DRIVER_SPI Driver_SPI0;
static ARM_DRIVER_SPI   *spiMasterDrv = &CREATE_SYMBOL(Driver_SPI, 0);
//static ARM_DRIVER_SPI   *spiMasterDrv = &Driver_SPI0;
//static ARM_DRIVER_SPI   *spi0 = &Driver_SPI0;

static __ALIGNED(4) uint8_t master_buffer_out[32];
static __ALIGNED(4) uint8_t master_buffer_in[32];

extern ARM_DRIVER_USART Driver_LPUSART1;

uint8_t b_timer0_int = 0;
uint8_t b_enable_debug = ATUART_DEBUG;
uint8_t b_reset_fail = 0;
uint8_t flash_buff[24];
uint8_t userAtCmdSlpHandler = 0xff;
uint8_t atcmd_name;

#if (USE_MODULAR == MODULAR_SLM100)
uint8_t gpio_array[USE_PIN_GROUP_OUT_NUMBER][32] = 
{
	"PIN11(GPIO9),PIN12(GPIO8),",
	"PIN13(GPIO6),PIN14(GPIO5),",
	"PIN15(GPIO4),PIN16(GPIO3),",
	"PIN18(GPIO2),PIN19(GPIO7),",
	"PIN30(GPIO1),PIN31(GPIO13),",
	"PIN32(GPIO12),PIN33(GPIO11),",
	"PIN34(GPIO10),PIN1(ADC0),",
	"PIN35(SWCLK),PIN36(SWDIO),",
	//"PIN20(GPIO0) ,PIN22(RST),"
};
uint8_t	gpio_test_port_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_PORT_OUT,
	PIN_GROUP1_PORT_OUT,
	PIN_GROUP2_PORT_OUT,
	PIN_GROUP3_PORT_OUT,
	PIN_GROUP4_PORT_OUT,
	PIN_GROUP5_PORT_OUT,
	PIN_GROUP6_PORT_OUT,
	PIN_GROUP7_PORT_OUT,
};
uint8_t	gpio_test_port_in[USE_PIN_GROUP_IN_NUMBER] 	= 
{
	PIN_GROUP0_PORT_IN,
	PIN_GROUP1_PORT_IN,
	PIN_GROUP2_PORT_IN,
	PIN_GROUP3_PORT_IN,
	PIN_GROUP4_PORT_IN,	
	PIN_GROUP5_PORT_IN,	
};
uint8_t	gpio_test_number_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_OUT,
	PIN_GROUP1_NUMBER_OUT,
	PIN_GROUP2_NUMBER_OUT,
	PIN_GROUP3_NUMBER_OUT,
	PIN_GROUP4_NUMBER_OUT,
	PIN_GROUP5_NUMBER_OUT,
	PIN_GROUP6_NUMBER_OUT,
	PIN_GROUP7_NUMBER_OUT,
};
uint8_t	gpio_test_number_in[USE_PIN_GROUP_IN_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_IN,
	PIN_GROUP1_NUMBER_IN,
	PIN_GROUP2_NUMBER_IN,
	PIN_GROUP3_NUMBER_IN,
	PIN_GROUP4_NUMBER_IN,
	PIN_GROUP5_NUMBER_IN,
};

#elif (USE_MODULAR == MODULAR_SLM130)
uint8_t gpio_array[USE_PIN_GROUP_OUT_NUMBER-1][32] = 
{
	"PIN38(GPIO4),PIN39(GPIO5),",
	"PIN28(GPIO2),PIN29(GPIO3),",
	"PIN3(GPIO14),PIN2(GPIO18),",
	"PIN4(GPIO11),PIN5(GPIO15),",
	"PIN6(GPIO16),PIN8(GPIO19),",
	//"PIN7(GPIO1) ,PIN15(RST),"
};
uint8_t	gpio_test_port_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_PORT_OUT,
	PIN_GROUP1_PORT_OUT,
	PIN_GROUP2_PORT_OUT,
	PIN_GROUP3_PORT_OUT,
	PIN_GROUP4_PORT_OUT,
	PIN_GROUP5_PORT_OUT,
};
uint8_t	gpio_test_port_in[USE_PIN_GROUP_IN_NUMBER] 	= 
{
	PIN_GROUP0_PORT_IN,
	PIN_GROUP1_PORT_IN,
	PIN_GROUP2_PORT_IN,
	PIN_GROUP3_PORT_IN,
	PIN_GROUP4_PORT_IN,	
};
uint8_t	gpio_test_number_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_OUT,
	PIN_GROUP1_NUMBER_OUT,
	PIN_GROUP2_NUMBER_OUT,
	PIN_GROUP3_NUMBER_OUT,
	PIN_GROUP4_NUMBER_OUT,
	PIN_GROUP5_NUMBER_OUT,
};
uint8_t	gpio_test_number_in[USE_PIN_GROUP_IN_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_IN,
	PIN_GROUP1_NUMBER_IN,
	PIN_GROUP2_NUMBER_IN,
	PIN_GROUP3_NUMBER_IN,
	PIN_GROUP4_NUMBER_IN,
};
#elif (USE_MODULAR == MODULAR_SLM160)
uint8_t gpio_array[USE_PIN_GROUP_OUT_NUMBER][32] = 
{
	"PIN1(GPIO9),PIN2(GPIO8),",
	"PIN3(GPIO3),PIN4(GPIO5),",
	"PIN5(GPIO4),PIN6(GPIO6),",
	"PIN7(GPIO11),PIN8(GPIO10),",
	"PIN34(GPIO0),PIN35(GPIO1),",
	"PIN36(GPIO13),PIN37(GPIO12),",
	"PIN16(GPIO2),PIN38(ADC0),",
	"PIN44(SWDIO),PIN45(SWCLK),",
	//"PIN21(GPIO7) ,PIN17(RST),"
};
uint8_t	gpio_test_port_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_PORT_OUT,
	PIN_GROUP1_PORT_OUT,
	PIN_GROUP2_PORT_OUT,
	PIN_GROUP3_PORT_OUT,
	PIN_GROUP4_PORT_OUT,
	PIN_GROUP5_PORT_OUT,
	PIN_GROUP6_PORT_OUT,
	PIN_GROUP7_PORT_OUT,
};
uint8_t	gpio_test_port_in[USE_PIN_GROUP_IN_NUMBER] 	= 
{
	PIN_GROUP0_PORT_IN,
	PIN_GROUP1_PORT_IN,
	PIN_GROUP2_PORT_IN,
	PIN_GROUP3_PORT_IN,
	PIN_GROUP4_PORT_IN,	
	PIN_GROUP5_PORT_IN,	
};
uint8_t	gpio_test_number_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_OUT,
	PIN_GROUP1_NUMBER_OUT,
	PIN_GROUP2_NUMBER_OUT,
	PIN_GROUP3_NUMBER_OUT,
	PIN_GROUP4_NUMBER_OUT,
	PIN_GROUP5_NUMBER_OUT,
	PIN_GROUP6_NUMBER_OUT,
	PIN_GROUP7_NUMBER_OUT,
};
uint8_t	gpio_test_number_in[USE_PIN_GROUP_IN_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_IN,
	PIN_GROUP1_NUMBER_IN,
	PIN_GROUP2_NUMBER_IN,
	PIN_GROUP3_NUMBER_IN,
	PIN_GROUP4_NUMBER_IN,
	PIN_GROUP5_NUMBER_IN,
};

#elif (USE_MODULAR == MODULAR_SLM190)
uint8_t gpio_array[USE_PIN_GROUP_OUT_NUMBER][32] = 
{
	"PIN3(GPIO1),PIN4(GPIO0),",///25
	"PIN5(GPIO13),PIN7(ADC2),",///25
	"PIN6(GPIO12),PIN8(ADC3),",///25
	"PIN19(GPIO8),PIN21(ADC0),",///26
	"PIN20(GPIO9),PIN22(ADC1),",///26
	"PIN34(GPIO2),PIN2(ADC1),",///25
	"PIN12(SWCLK),PIN13(SWDIO),",///27
	//"PIN18(GPIO7),PIN15(RST),"
};
uint8_t	gpio_test_port_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_PORT_OUT,
	PIN_GROUP1_PORT_OUT,
	PIN_GROUP2_PORT_OUT,
	PIN_GROUP3_PORT_OUT,
	PIN_GROUP4_PORT_OUT,
	PIN_GROUP5_PORT_OUT,
	PIN_GROUP6_PORT_OUT,
};
uint8_t	gpio_test_port_in[USE_PIN_GROUP_IN_NUMBER] 	= 
{
	PIN_GROUP0_PORT_IN,
};
uint8_t	gpio_test_number_out[USE_PIN_GROUP_OUT_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_OUT,
	PIN_GROUP1_NUMBER_OUT,
	PIN_GROUP2_NUMBER_OUT,
	PIN_GROUP3_NUMBER_OUT,
	PIN_GROUP4_NUMBER_OUT,
	PIN_GROUP5_NUMBER_OUT,
	PIN_GROUP6_NUMBER_OUT,
};
uint8_t	gpio_test_number_in[USE_PIN_GROUP_IN_NUMBER]	= 
{
	PIN_GROUP0_NUMBER_IN,
};

#endif

extern ARM_DRIVER_USART Driver_USART0;

#if (USE_MCU == MCU_EC616S)
void gpio_all_init_org(void)
{
//	uint8_t i;

    user_printf("MCU_EC616S gpio_all_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// TEST pin pad config
    padConfig.mux = TEST_PAD_ALT_FUNC;
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

void gpio_all_init(void)
{
	uint8_t i;

    user_printf("MCU_EC616S gpio_all_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// output pin pad config
    padConfig.mux = PAD_MuxAlt0;
	for(i=0;i<USE_PIN_GROUP_OUT_NUMBER;i++)
	{
		PAD_SetPinConfig(gpio_test_number_out[i]+11, &padConfig);		
	}

    // output pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;
	for(i=0;i<USE_PIN_GROUP_OUT_NUMBER;i++)
	{
		GPIO_PinConfig(gpio_test_port_out[i], gpio_test_number_out[i], &config);
	}
	
	// INPUT pin pad config
    padConfig.pullSelect = PAD_PullInternal;
    padConfig.pullUpEnable = PAD_PullUpEnable;
    padConfig.mux = INPUT_PAD_ALT_FUNC;
	for(i=0;i<USE_PIN_GROUP_IN_NUMBER;i++)
	{
		PAD_SetPinConfig(gpio_test_number_in[i]+11, &padConfig);
   		PAD_SetPinPullConfig(gpio_test_number_in[i]+11, PAD_InternalPullUp);
	}

	// INPUT pin config
    config.pinDirection = GPIO_DirectionInput;
    config.misc.interruptConfig = GPIO_InterruptRisingEdge;
	for(i=0;i<USE_PIN_GROUP_IN_NUMBER;i++)
	{
	    GPIO_PinConfig(gpio_test_port_in[i], gpio_test_number_in[i], &config);
	}
}

#else
void gpio_all_init_org(void)
{
//	uint8_t i;

    user_printf("MCU_EC616 gpio_all_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// TEST pin pad config
    padConfig.mux = PAD_MuxAlt0;
	PAD_SetPinConfig(1+11, &padConfig);	
	PAD_SetPinConfig(2+11, &padConfig);	
	PAD_SetPinConfig(3+11, &padConfig);	
	PAD_SetPinConfig(4+11, &padConfig);	
	PAD_SetPinConfig(5+11, &padConfig);	
	PAD_SetPinConfig(7+11, &padConfig);	
	PAD_SetPinConfig(11+11, &padConfig);	
	PAD_SetPinConfig(14+11, &padConfig);	
	PAD_SetPinConfig(15+11, &padConfig);	
	PAD_SetPinConfig(16+11, &padConfig);	
	PAD_SetPinConfig(19+11, &padConfig);	

    // TEST pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;
	GPIO_PinConfig(0, 1, &config);
	GPIO_PinConfig(0, 2, &config);
	GPIO_PinConfig(0, 3, &config);
	GPIO_PinConfig(0, 4, &config);
	GPIO_PinConfig(0, 5, &config);
	GPIO_PinConfig(0, 7, &config);
	GPIO_PinConfig(0, 11, &config);
	GPIO_PinConfig(0, 14, &config);
	GPIO_PinConfig(0, 15, &config);
	GPIO_PinConfig(1, 0, &config);///16
//	GPIO_PinConfig(1, 1, &config);
//	GPIO_PinConfig(1, 2, &config);
	GPIO_PinConfig(1, 3, &config);///19

}

void gpio_all_init(void)
{
//	uint8_t i;

    user_printf("MCU_EC616 gpio_all_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// output pin pad config
    padConfig.mux = PAD_MuxAlt0;
	PAD_SetPinConfig(1+11, &padConfig);//GPIO1
	PAD_SetPinConfig(3+11, &padConfig);//GPIO3
	PAD_SetPinConfig(4+11, &padConfig);//GPIO4
	PAD_SetPinConfig(11+11, &padConfig);//GPIO11

	//swclk swdio output pin pad config
    padConfig.mux = PAD_MuxAlt7;
	PAD_SetPinConfig(9, &padConfig);//GPIO18	
	PAD_SetPinConfig(10, &padConfig);//GPIO19

    // output pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;
	GPIO_PinConfig(0, 1, &config);
	GPIO_PinConfig(0, 3, &config);
	GPIO_PinConfig(0, 4, &config);
	GPIO_PinConfig(0, 11, &config);
	GPIO_PinConfig(1, 2, &config);
	GPIO_PinConfig(1, 3, &config);
	
	// INPUT pin pad config
    padConfig.pullSelect = PAD_PullInternal;
    padConfig.pullUpEnable = PAD_PullUpEnable;
    padConfig.mux = INPUT_PAD_ALT_FUNC;
    PAD_SetPinConfig(2+11, &padConfig);
    PAD_SetPinPullConfig(2+11, PAD_InternalPullUp);
    PAD_SetPinConfig(5+11, &padConfig);
    PAD_SetPinPullConfig(5+11, PAD_InternalPullUp);
    PAD_SetPinConfig(23, &padConfig);//GPIO14
    PAD_SetPinPullConfig(23, PAD_InternalPullUp);
    PAD_SetPinConfig(24, &padConfig);//GPIO15
    PAD_SetPinPullConfig(24, PAD_InternalPullUp);
    PAD_SetPinConfig(21, &padConfig);//GPIO16
    PAD_SetPinPullConfig(21, PAD_InternalPullUp);

	// INPUT pin config
    config.pinDirection = GPIO_DirectionInput;
    config.misc.interruptConfig = GPIO_InterruptRisingEdge;
    GPIO_PinConfig(0, 2, &config);
    GPIO_PinConfig(0, 5, &config);
	GPIO_PinConfig(0, 14, &config);
    GPIO_PinConfig(0, 15, &config);
	GPIO_PinConfig(1, 0, &config);
}
#endif

void gpio_sleep_state_init(void)
{
    user_printf("MCU_EC616S gpio_all_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// TEST pin pad config
    padConfig.mux = TEST_PAD_ALT_FUNC;
	PAD_SetPinConfig(7+11, &padConfig);	

    // TEST pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;

	GPIO_PinConfig(0, 7, &config);

	uint8_t state;
	state = (1<<7);
//	state = ~(1<<7);
	GPIO_PinWrite(0,1<<7,state);

}

void test_wakeup_pin_init(void)
{
	// GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// TEST pin pad config
    padConfig.mux = 0;
    PAD_SetPinConfig(2+11, &padConfig);

    // TEST pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;
    GPIO_PinConfig(0, 2, &config);
}

uint8_t is_enable_debug(void)
{
	return b_enable_debug;
}

void enable_debug_set(uint8_t value)
{
	b_enable_debug = value;
	atUartPrint("b_enable_debug = %d\r\n",b_enable_debug);
}

void mDelay(int ms)
{
    int32_t i=0;
    for(i=0;i<70000;i++)
            ;
}

void set_flash_data(uint8_t *pdata,uint8_t length,uint8_t offset)
{
	uint8_t i;

	for(i=0;i<length;i++)
	{
		flash_buff[offset+i] = pdata[i];		
	}
}

void get_flash_data(uint8_t *pdata,uint8_t length)
{
	uint8_t i;

	for(i=0;i<length;i++)
	{
		pdata[i] = flash_buff[i];		
	}
}

void set_atcmd_name(uint8_t n)
{
	atcmd_name = n;
}

uint8_t get_atcmd_name(void)
{
	return atcmd_name;
}

void user_disable_lpusart1(void)
{	
	Driver_LPUSART1.PowerControl (ARM_POWER_OFF);    // Stop USART0
	Driver_LPUSART1.Uninitialize ();
}

CmsRetId gpio_test_handle(uint32_t gpio_port_output, uint16_t gpio_number_output,uint32_t gpio_port_input, uint16_t gpio_number_input)
{
	uint32_t pin_read;

	GPIO_PinWrite(gpio_port_output, 1 << gpio_number_output, 0);
	osDelay(1);
	pin_read = GPIO_PinRead(gpio_port_input,gpio_number_input);
	user_printf("(2)pin_read = %d\r\n",pin_read);
	if(pin_read == 0)
	{
		user_printf("read low level ok.\r\n");
	}
	else
	{
		user_printf("read low level fail.\r\n");
		return 2;
	}

	GPIO_PinWrite(gpio_port_output, 1 << gpio_number_output, 1 << gpio_number_output);
	osDelay(1);
	pin_read = GPIO_PinRead(gpio_port_input,gpio_number_input);
	user_printf("(1)pin_read = %d\r\n",pin_read);
	if(pin_read == 1)
	{
		user_printf("read high level ok.\r\n");
	}
	else
	{
		user_printf("read high level fail.\r\n");
		return 1;
	}

	return 0;
}

CmsRetId adc_test_handle(adc_channel_t channel,uint8_t group,volatile uint32_t *presult)
{
	GPIO_PinWrite(gpio_test_port_out[group],1<<gpio_test_number_out[group],
					1<<gpio_test_number_out[group]);
	conversionDone = 0;
    ADC_StartConversion(channel, ADC_UserAPP);
	//while((conversionDone&ADC_ChannelAio4) == 0);
	while(conversionDone == 0);
	if(*presult > 1000)
	{
		GPIO_PinWrite(gpio_test_port_out[group],1<<gpio_test_number_out[group],0);
		conversionDone = 0;
		ADC_StartConversion(channel, ADC_UserAPP);
		//while((conversionDone&ADC_ChannelAio4) == 0);
		while(conversionDone == 0);
		if(*presult > 1000)
		{
			return 1;
		}
	}
	else
	{
		return 2;
	}

	return 0;
}

void gpio_test_init(void)
{
    user_printf("gpio_test_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// INPUT pin pad config
    padConfig.pullSelect = PAD_PullInternal;
    padConfig.pullUpEnable = PAD_PullUpEnable;
    padConfig.mux = INPUT_PAD_ALT_FUNC;
    PAD_SetPinConfig(INPUT_PAD_INDEX, &padConfig);
    PAD_SetPinPullConfig(INPUT_PAD_INDEX, PAD_InternalPullUp);

	// TEST pin pad config
    padConfig.mux = TEST_PAD_ALT_FUNC;
    PAD_SetPinConfig(TEST_PAD_INDEX, &padConfig);

    // TEST pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;
    GPIO_PinConfig(TEST_GPIO_INSTANCE, TEST_GPIO_PIN, &config);

    // INPUT pin config
    config.pinDirection = GPIO_DirectionInput;
    config.misc.interruptConfig = GPIO_InterruptRisingEdge;
    GPIO_PinConfig(INPUT_GPIO_INSTANCE, INPUT_GPIO_PIN, &config);

    // Enable IRQ
//    XIC_SetVector(PXIC_Gpio_IRQn, gpio_input_handle);
//    XIC_EnableIRQ(PXIC_Gpio_IRQn);

//    while(1)
//    {
//        if (gpioInterruptCount & 0x1)
//        {
//            GPIO_PinWrite(LED_GPIO_INSTANCE, 1 << LED_GPIO_PIN, 1 << LED_GPIO_PIN);
//        }
//        else
//        {
//            GPIO_PinWrite(LED_GPIO_INSTANCE, 1 << LED_GPIO_PIN, 0);
//        }
//    }
}

void i2c_test_init(void)
{
    int32_t ret;
//    uint16_t regVal, exponent, fraction;

    // Initialize with callback
    ret = i2cDrvInstance->Initialize(NULL);
    RetCheck(ret, "initialize");

    // Power on
    ret = i2cDrvInstance->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on");

    // Configure I2C bus
    ret = i2cDrvInstance->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
    RetCheck(ret, "Config bus");

    ret = i2cDrvInstance->Control(ARM_I2C_BUS_CLEAR, 0);
    RetCheck(ret, "Clear bus");

    // Check device id
//    regVal = OPT3001_ReadReg(OPT3001_REG_DEVICE_ID);

//    printf("Device ID=%x\r\n", regVal);

//    OPT3001_Config(CONFIG_ENABLE_CONTINUOUS);

//    while(1)
//    {
//        // check if sensor data ready
//        while(1)
//        {
//            printf("check sensor data ready\n");
//            regVal = OPT3001_ReadReg(OPT3001_REG_CONFIGURATION);
//            if(regVal & CONFIG_CRF_Msk)
//            {
//                printf("sensor data ready\r\n");
//                break;
//            }
//            delay_us(100000);
//        }
//        // read result out
//        regVal = OPT3001_ReadReg(OPT3001_REG_RESULT);

//        // convert result to LUX
//        fraction = regVal & 0xFFF;
//        exponent = 1 << (regVal >> 12);

//        printf("=============read lux=%d/100 =================\n", fraction * exponent);

//        delay_us(3000000);
//    }

}

void pwm1_test_init(void)
{
//    unsigned int dutyCycle = PWM1_INIT_DUTY_CYCLE_PERCENT;

    user_printf("pwm1_test_init\r\n");

    // PWM1 out pin config
    pad_config_t config;

    PAD_GetDefaultConfig(&config);
    config.pullSelect = PAD_PullInternal;
    config.pullUpEnable = PAD_PullUpEnable;
    config.mux = PWM1_OUT_ALT_SEL;
    PAD_SetPinConfig(PWM1_OUT_PIN, &config);

    // Config PWM1 clock, source from 26MHz and divide by 1
    CLOCK_SetClockSrc(PWM1_CLOCK_ID, PWM1_CLOCK_SOURCE);
    CLOCK_SetClockDiv(PWM1_CLOCK_ID, 1);

    TIMER_DriverInit();

    timer_pwm_config_t pwmConfig;
    pwmConfig.pwmFreq_HZ = 2000;
    pwmConfig.srcClock_HZ = GPR_GetClockFreq(PWM1_CLOCK_ID);  // 26MHz
    pwmConfig.dutyCyclePercent = PWM1_INIT_DUTY_CYCLE_PERCENT;

    TIMER_SetupPwm(PWM1_INSTANCE, &pwmConfig);

    TIMER_Start(PWM1_INSTANCE);
}

void pwm4_test_init(void)
{
//    unsigned int dutyCycle = PWM4_INIT_DUTY_CYCLE_PERCENT;

    user_printf("pwm4_test_init\r\n");

    // PWM1 out pin config
    pad_config_t config;

    PAD_GetDefaultConfig(&config);
    config.pullSelect = PAD_PullInternal;
//    config.pullUpEnable = PAD_PullUpEnable;
    config.pullDownEnable = PAD_PullDownEnable;
    config.mux = PWM4_OUT_ALT_SEL;
    PAD_SetPinConfig(PWM4_OUT_PIN, &config);

    // Config PWM1 clock, source from 26MHz and divide by 1
    CLOCK_SetClockSrc(PWM4_CLOCK_ID, PWM4_CLOCK_SOURCE);
    CLOCK_SetClockDiv(PWM4_CLOCK_ID, 1);

    TIMER_DriverInit();

    timer_pwm_config_t pwmConfig;
    pwmConfig.pwmFreq_HZ = 1000;
    pwmConfig.srcClock_HZ = GPR_GetClockFreq(PWM4_CLOCK_ID);  // 26MHz
    pwmConfig.dutyCyclePercent = PWM4_INIT_DUTY_CYCLE_PERCENT;

    TIMER_SetupPwm(PWM4_INSTANCE, &pwmConfig);

//    TIMER_Start(PWM4_INSTANCE);
}

void pwm5_test_init(void)
{
//    unsigned int dutyCycle = PWM5_INIT_DUTY_CYCLE_PERCENT;

    user_printf("pwm5_test_init\r\n");

    // PWM1 out pin config
    pad_config_t config;

    PAD_GetDefaultConfig(&config);
    config.pullSelect = PAD_PullInternal;
//    config.pullUpEnable = PAD_PullUpEnable;
    config.pullDownEnable = PAD_PullDownEnable;
    config.mux = PWM5_OUT_ALT_SEL;
    PAD_SetPinConfig(PWM5_OUT_PIN, &config);

    // Config PWM1 clock, source from 26MHz and divide by 1
    CLOCK_SetClockSrc(PWM5_CLOCK_ID, PWM5_CLOCK_SOURCE);
    CLOCK_SetClockDiv(PWM5_CLOCK_ID, 1);

    TIMER_DriverInit();

    timer_pwm_config_t pwmConfig;
    pwmConfig.pwmFreq_HZ = 1000;
    pwmConfig.srcClock_HZ = GPR_GetClockFreq(PWM5_CLOCK_ID);  // 26MHz
    pwmConfig.dutyCyclePercent = PWM5_INIT_DUTY_CYCLE_PERCENT;

    TIMER_SetupPwm(PWM5_INSTANCE, &pwmConfig);

//    TIMER_Start(PWM5_INSTANCE);
}

/**
  \fn          void ADC_Aio1ChannelCallback()
  \brief       Aio1 channel callback
  \return
*/
void ADC_Aio1ChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelAio1;
    aio1ChannelResult = result;
}

/**
  \fn          void ADC_Aio2ChannelCallback()
  \brief       Aio2 channel callback
  \return
*/
void ADC_Aio2ChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelAio2;
    aio2ChannelResult = result;
}

/**
  \fn          void ADC_Aio3ChannelCallback()
  \brief       Aio3 channel callback
  \return
*/
void ADC_Aio3ChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelAio3;
    aio3ChannelResult = result;
}

/**
  \fn          void ADC_Aio4ChannelCallback()
  \brief       Aio4 channel callback
  \return
*/
void ADC_Aio4ChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelAio4;
    aio4ChannelResult = result;
}

void adc1_test_init(void)
{
    adc_config_t adcConfig;

    user_printf("adc1_test_init ******\r\n");

    ADC_GetDefaultConfig(&adcConfig);

    adcConfig.channelConfig.aioResDiv = ADC_AioResDivRatio1;
    ADC_ChannelInit(ADC_ChannelAio1, ADC_UserAPP, &adcConfig, ADC_Aio1ChannelCallback);
}

void adc2_test_init(void)
{
    adc_config_t adcConfig;

    user_printf("adc2_test_init ******\r\n");

    ADC_GetDefaultConfig(&adcConfig);

    adcConfig.channelConfig.aioResDiv = ADC_AioResDivRatio1;
    ADC_ChannelInit(ADC_ChannelAio2, ADC_UserAPP, &adcConfig, ADC_Aio2ChannelCallback);
}

void adc3_test_init(void)
{
    adc_config_t adcConfig;

    user_printf("adc3_test_init ******\r\n");

    ADC_GetDefaultConfig(&adcConfig);

    adcConfig.channelConfig.aioResDiv = ADC_AioResDivRatio1;
    ADC_ChannelInit(ADC_ChannelAio3, ADC_UserAPP, &adcConfig, ADC_Aio3ChannelCallback);
}

void adc4_test_init(void)
{
    adc_config_t adcConfig;

    user_printf("adc4_test_init\r\n");

    ADC_GetDefaultConfig(&adcConfig);

    adcConfig.channelConfig.aioResDiv = ADC_AioResDivRatio1;
    ADC_ChannelInit(ADC_ChannelAio4, ADC_UserAPP, &adcConfig, ADC_Aio4ChannelCallback);
}

void spi0_cs_init(void)
{
    user_printf("spi0_cs_init\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

	// TEST pin pad config
    padConfig.mux = TEST_PAD_ALT_FUNC;
//	PAD_SetPinConfig(0+11, &padConfig);	
//	PAD_SetPinConfig(1+11, &padConfig);	
//	PAD_SetPinConfig(2+11, &padConfig);	
	PAD_SetPinConfig(3+11, &padConfig);	
//	PAD_SetPinConfig(4+11, &padConfig);	
//	PAD_SetPinConfig(5+11, &padConfig);	
//	PAD_SetPinConfig(6+11, &padConfig);	
//	PAD_SetPinConfig(7+11, &padConfig);	
//	PAD_SetPinConfig(8+11, &padConfig);	
//	PAD_SetPinConfig(9+11, &padConfig);
//	PAD_SetPinConfig(10+11, &padConfig);	
//	PAD_SetPinConfig(11+11, &padConfig);	
//	PAD_SetPinConfig(12+11, &padConfig);	
//	PAD_SetPinConfig(13+11, &padConfig);	
//	PAD_SetPinConfig(14+11, &padConfig);	
//	PAD_SetPinConfig(15+11, &padConfig);	

    // TEST pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 1;

//	GPIO_PinConfig(0, 0, &config);
//	GPIO_PinConfig(0, 1, &config);
//	GPIO_PinConfig(0, 2, &config);
	GPIO_PinConfig(0, 3, &config);
//	GPIO_PinConfig(0, 4, &config);
//	GPIO_PinConfig(0, 5, &config);
//	GPIO_PinConfig(0, 6, &config);
//	GPIO_PinConfig(0, 7, &config);
//	GPIO_PinConfig(0, 8, &config);
//	GPIO_PinConfig(0, 9, &config);
//	GPIO_PinConfig(0, 10, &config);
//	GPIO_PinConfig(0, 11, &config);
//	GPIO_PinConfig(0, 12, &config);
//	GPIO_PinConfig(0, 13, &config);
//	GPIO_PinConfig(0, 14, &config);
//	GPIO_PinConfig(0, 15, &config);
}

void spi0_test_init_org(void)
{
    int32_t ret;

    memset(master_buffer_in, 0, sizeof(master_buffer_in));

	master_buffer_out[0] = 0x90;
	master_buffer_out[1] = 0x00;
	master_buffer_out[2] = 0x00;
	master_buffer_out[3] = 0x00;
	master_buffer_out[4] = 0xFF;
	master_buffer_out[5] = 0xFF;

    user_printf("spi0_test_init\r\n");

    // Initialize master spi
    ret = spiMasterDrv->Initialize(NULL);
    RetCheck(ret, "Initialize master");
    // Power on
    ret = spiMasterDrv->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on master");

    // Configure master spi bus
    ret = spiMasterDrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_DATA_BITS(TRANSFER_DATA_WIDTH) |
                                  ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_UNUSED, 200000U);

    RetCheck(ret, "Configure master bus");

	spi0_cs_init();

//	GPIO_PinWrite(0, 1 << 3, 0);
//    spiMasterDrv->Transfer(master_buffer_out, master_buffer_in, 8);
//	GPIO_PinWrite(0, 1 << 3, 1 << 3);	
}

void spi0_test_init(void)
{
    int32_t ret;

	spi0_cs_init();

    memset(master_buffer_in, 0, sizeof(master_buffer_in));

	master_buffer_out[0] = 0x11;
	master_buffer_out[1] = 0x22;
	master_buffer_out[2] = 0x33;
	master_buffer_out[3] = 0x44;
	master_buffer_out[4] = 0x55;
	master_buffer_out[5] = 0x66;

    user_printf("spi0_test_init\r\n");

    // Initialize master spi
    ret = spiMasterDrv->Initialize(NULL);
    RetCheck(ret, "Initialize master");
    // Power on
    ret = spiMasterDrv->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on master");

    // Configure master spi bus
    ret = spiMasterDrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_DATA_BITS(TRANSFER_DATA_WIDTH) |
                                  ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_UNUSED, 200000U);

    RetCheck(ret, "Configure master bus");

//    spiMasterDrv->Transfer(master_buffer_out, master_buffer_in, TRANSFER_SIZE);
//    spiMasterDrv->Transfer(master_buffer_out, master_buffer_in, 1);
//	do
//    {
//        delay_us(1000);
//    }
//    while((isTransferDone == false) && --timeOut_ms);

//    if(timeOut_ms == 0)
//    {
//        printf("Transfer failed for timeout\n");
//    }
//    else
//    {
//    	printf("ok\r\n");
//    }
}

void file_test_write(void)
{
	OSAFILE fp = NULL;
	UINT32 write_count;
	char *pwbuff;

	pwbuff = malloc(32);
		
	fp = OsaFopen(USER_NVM_FILE_NAME,"wb+");
//	fp = OsaFopen(USER_NVM_FILE_NAME,"rb+");

	if(fp == NULL)
	{
		user_printf("open file fail!!!\r\n");
		return;
	}
	else
	{
		user_printf("open file success!!!\r\n");
	}

	pwbuff[0] = 0xAA;	
	write_count = OsaFwrite(pwbuff, 1, 12, fp);

	user_printf("write_count = %d\r\n",write_count);

	free(pwbuff);
	OsaFclose(fp);
}

uint8_t file_test_read(void)
{
	OSAFILE fp = NULL;
	UINT32 read_count;
	uint8_t *prbuff;

	prbuff = malloc(32);
		
	fp = OsaFopen(USER_NVM_FILE_NAME,"wb+");
//	fp = OsaFopen(USER_NVM_FILE_NAME,"rb+");

	if(fp == NULL)
	{
		user_printf("open file fail!!!\r\n");
		return 2;
	}
	else
	{
		user_printf("open file success!!!\r\n");
	}
	
	read_count  = OsaFread(prbuff,1,12,fp);

	user_printf("read_count = %d\r\n",read_count);
//	user_printf("prbuff = %s\r\n",prbuff);
	user_printf("prbuff[0] = %x\r\n",prbuff[0]);
	user_printf("prbuff[1] = %x\r\n",prbuff[1]);
	user_printf("prbuff[2] = %x\r\n",prbuff[2]);

	free(prbuff);
	OsaFclose(fp);

	if((prbuff[0] == 0xAA)&&(prbuff[2] == 0xAA))
	{
		return 0;
	}
	else if((prbuff[0] == 0xAA)&&(prbuff[2] != 0xAA))
	{
		return 1;
	}
	else if((prbuff[0] != 0xAA)&&(prbuff[2] == 0xAA))
	{
		return 2;
	}
	else if((prbuff[0] != 0xAA)&&(prbuff[2] != 0xAA))
	{
		return 3;
	} 

	return 0;
}

int8_t user_file_read(void)
{
	OSAFILE fp = NULL;
	UINT32 read_count;
		
	fp = OsaFopen(USER_NVM_FILE_NAME,"wb+");
//	fp = OsaFopen(USER_NVM_FILE_NAME,"rb+");

	if(fp == NULL)
	{
		user_printf("open file fail!!!\r\n");
		return 2;
	}
	else
	{
		user_printf("open file success!!!\r\n");
	}
	
	read_count  = OsaFread(flash_buff,1,12,fp);

	user_printf("read_count = %d\r\n",read_count);
//	user_printf("prbuff = %s\r\n",prbuff);
	user_printf("flash_buff[0] = %x\r\n",flash_buff[0]);
	user_printf("flash_buff[1] = %x\r\n",flash_buff[1]);
	user_printf("flash_buff[2] = %x\r\n",flash_buff[2]);
	user_printf("flash_buff[2] = %x\r\n",flash_buff[3]);
	
	OsaFclose(fp);

	return 0;
}

int8_t user_file_write(void)
{
	OSAFILE fp = NULL;
	UINT32 write_count;
		
	fp = OsaFopen(USER_NVM_FILE_NAME,"wb+");
//	fp = OsaFopen(USER_NVM_FILE_NAME,"rb+");

	if(fp == NULL)
	{
		user_printf("open file fail!!!\r\n");
		return -1;
	}
	else
	{
		user_printf("open file success!!!\r\n");
	}

	write_count = OsaFwrite(flash_buff, 1, 12, fp);

	user_printf("write_count = %d\r\n",write_count);

	OsaFclose(fp);

	return 0;
}

void user_file_handle(void)
{
	uint8_t b_write = 0;

	user_file_read();
	mDelay(20);

	if((flash_buff[1] == 0x01)&&(flash_buff[2] == 0xBB))
	{
		flash_buff[1] = 0x00;
		flash_buff[2] = 0xAA;
		b_write = 1;
	}

#ifdef CUSTOMER_DAHUA
	if(flash_buff[3] == 0x02)
	{
		slpManIOLevelSel(1);
		slpManNormalIOVoltSet(IOVOLT_2_80V);
	}
	else if(flash_buff[3] == 0x01)
	{
		slpManIOLevelSel(0);
        slpManNormalIOVoltSet(IOVOLT_1_80V);
	}
	else 
	{
		flash_buff[3] = 0x02;
		slpManIOLevelSel(1);
		slpManNormalIOVoltSet(IOVOLT_2_80V);
		b_write = 1;
	}
#endif

	if(b_write)
	{
		user_file_write();
	}
}

//void file_test_read_from_int(void)
//{
//	OSAFILE fp = NULL;
//		
//	fp = OsaFopen(USER_NVM_FILE_NAME,"wb+");
//	//fp = OsaFopen(USER_NVM_FILE_NAME,"rb+");

//	if(fp == NULL)
//	{
//		read_count = 0xFF;
//		return;
//	}
//	
//	read_count  = OsaFread(file_read_buff,1,12,fp);

//	OsaFclose(fp);
//}

/**
  \fn          void Timer0_ISR()
  \brief       Timer1 interrupt service routine
  \return
*/
void Timer0_ISR()
{
    if (TIMER_GetInterruptFlags(TIMER0_INSTANCE) & TIMER_Match0InterruptFlag)
    {
//        user_printf("Timer0_ISR\r\n");
		b_timer0_int = 1;
//		file_test_read_from_int();
		userAtCmdSemRelease(0,&b_timer0_int,1);
        TIMER_ClearInterruptFlags(TIMER0_INSTANCE, TIMER_Match0InterruptFlag);
    }
}

void timer0_init(void)
{
	user_printf("timer0_init\r\n");

    TIMER_DriverInit();

	// TIMER0 config
    // Config TIMER clock, source from 32.768KHz and divide by 1
    CLOCK_SetClockSrc(TIMER0_CLOCK_ID, TIMER0_CLOCK_SOURCE);
    CLOCK_SetClockDiv(TIMER0_CLOCK_ID, 1);

    // Config TIMER0 period as 500ms, match0 value is 16384 = 0x4000
    timer_config_t timerConfig;
    TIMER_GetDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_ReloadOnMatch0;
    timerConfig.match0 = 0x4000;

    TIMER_Init(TIMER0_INSTANCE, &timerConfig);

    TIMER_InterruptConfig(TIMER0_INSTANCE, TIMER_Match0Select, TIMER_InterruptLevel);
    TIMER_InterruptConfig(TIMER0_INSTANCE, TIMER_Match1Select, TIMER_InterruptDisabled);
    TIMER_InterruptConfig(TIMER0_INSTANCE, TIMER_Match2Select, TIMER_InterruptDisabled);

    // Enable TIMER0 IRQ
    XIC_SetVector(TIMER0_INSTANCE_IRQ, Timer0_ISR);
    XIC_EnableIRQ(TIMER0_INSTANCE_IRQ);

    TIMER_Start(TIMER0_INSTANCE);
}

void userAtCmdSemRelease(uint32_t cmd,void *pdata,uint16_t length)
{
	osSemaphoreRelease(userAtCmdSemHandle);
	at_cmd = cmd;
	memcpy(at_param,pdata,length);
	at_param_length = length;
}

void number2string(uint8_t *band,uint8_t *band_string,uint8_t number)
{
	uint8_t i;
	uint8_t length;
	uint8_t pos;
		
	length = 0;
	pos = 0;
	
	for(i=0;i<number;i++)
	{
		length = sprintf((char*)&band_string[pos],"%d,",band[i]);	
		pos += length;
	}
	band_string[pos-1] = '\0';
}

void atcmd_NCCID_read(void)
{
	CHAR ccid[30] = {0};

	appGetIccidNumSync(ccid);

//	user_printf("ccid = %s\r\n",ccid);

	user_printf("\r\n+NCCID:%s\r\n\r\nOK\r\n",ccid);
}

void atcmd_NUESTATS_SET(void)
{
//	BasicCellListInfo *pBcListInfo;
	CmiDevGetBasicCellListInfoCnf *pBcListInfo;
	CmsRetId ret;


    //15. get basic cell info
    pBcListInfo = (BasicCellListInfo *)OsaAllocZeroMemory(sizeof(BasicCellListInfo));
    OsaCheck(pBcListInfo != PNULL, sizeof(BasicCellListInfo), 0, 0);
    ret = appGetECBCInfoSync(GET_NEIGHBER_CELL_ID_INFO, 2, 10, pBcListInfo);

	user_printf("\r\npBcListInfo->sCellPresent = %d\r\n",pBcListInfo->sCellPresent);
	user_printf("\r\npBcListInfo->nCellNum = %d\r\n",pBcListInfo->nCellNum);

	if(ret == NBStatusFail)
	{
		user_printf("\r\n\r\nOK\r\n");
	}
	else if(ret == NBStatusSuccess)
	{
		user_printf("\r\nNUESTATS:CELL,%d,%d,%d,%d,%d,%d,%d\r\n",
					pBcListInfo->sCellInfo.earfcn,
					pBcListInfo->sCellInfo.phyCellId,
					1,
					pBcListInfo->sCellInfo.rsrp,
					pBcListInfo->sCellInfo.rsrq,
					99,
					pBcListInfo->sCellInfo.snr);

		user_printf("\r\nOK\r\n");
	}

    OsaFreeMemory(&pBcListInfo);
}

void atcmd_TEST_set(void)
{
	uint8_t i;
	static uint8_t pwm1_state = 0;
	static uint8_t pwm4_state = 0;
	static uint8_t pwm5_state = 0;
	static uint8_t gpio_port_bk = 0;
	static uint8_t gpio_number_bk = 0;
	static uint8_t adc_channel_bk = 0;
	uint8_t  gpio_port_output = 0;
	uint8_t  gpio_number_output = 0;
	uint8_t  gpio_port_input = 0;
	uint8_t  gpio_number_input = 0;
	CmsRetId result;
	CHAR *piccid;
	uint8_t b_gpio_test = 0;
	uint8_t length;
	uint8_t *p_gpio_info;
	uint8_t *puart2_rxbuff;
 	volatile uint32_t *padc_value[5];
	adc_channel_t channel[5];
		
	user_printf("\r\natcmd_TEST_set\r\n");

	if(b_test_init == 0)
	{
		b_test_init = 1;

		Driver_USART0.PowerControl (ARM_POWER_OFF);    // Stop USART0
		Driver_USART0.Uninitialize ();

		//every gpio pin set output
//		gpio_all_init_org();
//		//pwm1_test_init();
//		pwm4_test_init();
//		pwm5_test_init();
//		adc1_test_init();
//		adc2_test_init();
//		adc3_test_init();
//		adc4_test_init();

		gpio_all_init();
#if (USE_MODULAR == MODULAR_SLM100)
		adc4_test_init();
		uart2_init();
#elif (USE_MODULAR == MODULAR_SLM160)
		adc4_test_init();
#elif (USE_MODULAR == MODULAR_SLM190)
		adc1_test_init();
		adc2_test_init();
		adc3_test_init();
		adc4_test_init();
		uart2_init();
#endif
	}

	for(i=0;i<at_param_length;i++)
	{
		user_printf("at_param[%d] = %d\r\n",i,at_param[i]);
	}

	switch(at_param[0])
	{
		case 0:
			user_printf("GPIO TEST\r\n");
			if(at_param[1]<16)
			{
				gpio_port = 0;
				gpio_number = at_param[1];
			}
			else
			{
				gpio_port = 1;
				gpio_number = at_param[1]-16;
			}

			if((gpio_port_bk != gpio_port)||(gpio_number_bk != gpio_number))
			{
				b_gpio_toggle = 1;
			}
			else if((gpio_port_bk == gpio_port)&&(gpio_number_bk == gpio_number))
			{
				if(b_gpio_toggle)
				{
					b_gpio_toggle = 0;
				}
				else
				{
					b_gpio_toggle = 1;				
				}
			}
			
			gpio_port_bk = gpio_port;
			gpio_number_bk = gpio_number;
			
			user_printf("gpio_port = %d\r\n",gpio_port);
			user_printf("gpio_number = %d\r\n",gpio_number);
			break;
		case 1:
			user_printf("PWM TEST\r\n");

			switch(at_param[1])
			{
				case 1:
					if(pwm1_state == 0)
					{	
						pwm1_state = 1;
					    TIMER_Start(PWM1_INSTANCE);
					}
					else
					{
						pwm1_state = 0;
					    TIMER_Stop(PWM1_INSTANCE);
					}
					break;
			
				case 4:
					if(pwm4_state == 0)
					{	
						pwm4_state = 1;
					    TIMER_Start(PWM4_INSTANCE);
					}
					else
					{
						pwm4_state = 0;
					    TIMER_Stop(PWM4_INSTANCE);
					}
					break;
				case 5:
					if(pwm5_state == 0)
					{	
						pwm5_state = 1;
					    TIMER_Start(PWM5_INSTANCE);
					}
					else
					{
						pwm5_state = 0;
					    TIMER_Stop(PWM5_INSTANCE);
					}
					break;
			}
			
			break;
		case 2:
			user_printf("ADC TEST\r\n");
			switch(at_param[1])
			{
				case 1:
					adc_channel = ADC_ChannelAio1;
					padc_result = &aio1ChannelResult;
					break;
				case 2:
					adc_channel = ADC_ChannelAio2;
					padc_result = &aio2ChannelResult;
					break;
				case 3:
					adc_channel = ADC_ChannelAio3;
					padc_result = &aio3ChannelResult;
					break;
				case 4:
					adc_channel = ADC_ChannelAio4;
					padc_result = &aio4ChannelResult;
					break;
			}

			if(adc_channel != adc_channel_bk)
			{
				b_adc_print = 1;
			}
			else
			{
				if(b_adc_print)
				{
					b_adc_print = 0;
				}
				else
				{
					b_adc_print = 1;
				}
			}

			adc_channel_bk = adc_channel;
			break;
		case 3:
			user_printf("SETP TEST\r\n");
			switch(at_param[1])
			{
				case 0:
					piccid = malloc(32);
					
					result = appGetIccidNumSync(piccid);
					user_printf("read iccid result = %d\r\n",result);
					if(result == CMS_RET_SUCC)
					{
						user_printf("read iccid ok.\r\n");
						user_printf("piccid = %s\r\n",piccid);
						at_reply("\r\n+TEST=STEP,%d\r\n\r\nOK\r\n",at_param[1]);
					}
					else
					{
						user_printf("read iccid fail.\r\n");
						at_reply("\r\n+TEST=STEP,%d\r\n\r\nERROR\r\n",at_param[1]);
					}

					free(piccid);
					break;
				case 1:
					user_file_read();
					if(flash_buff[0] == 0xAA)
					{
						at_reply("\r\n+FACTORYTEST=STEP,%d\r\nHAVE CARIBRATION\r\nOK\r\n",at_param[1]);
						return;
					}

					b_gpio_test = 1;
										
					break;
				case 2:
					user_file_read();
					osDelay(20);
					if(flash_buff[2] == 0xAA)
					{
						at_reply("\r\n+FACTORYTEST=STEP,%d\r\nHAVE CARIBRATION\r\nOK\r\n",at_param[1]);
						return;
					}
					
					flash_buff[1] = 0x01;
					flash_buff[2] = 0xBB;
					user_file_write();
					osDelay(500);
										
#if (USE_MODULAR == MODULAR_SLM100)
					GPIO_PinWrite(0, 1 << 0, 0);				
#elif (USE_MODULAR == MODULAR_SLM130)
					GPIO_PinWrite(0, 1 << 1, 0);				
#elif (USE_MODULAR == MODULAR_SLM160)
					GPIO_PinWrite(0, 1 << 7, 0);				
#elif (USE_MODULAR == MODULAR_SLM190)
					GPIO_PinWrite(0, 1 << 7, 0);				
#endif
					
					at_reply("\r\n+FACTORYTEST=STEP,%d\r\n\r\nERROR\r\n",at_param[1]);

					b_reset_fail = 1;
					return;
				case 3:
//					GPIO_PinWrite(0, 1 << 2, 0);
					break;
				case 4:
					flash_buff[0] = 0xBB;
					flash_buff[1] = 0x00;
					flash_buff[2] = 0xBB;
					user_file_write();
					break;
				case 5:
					flash_buff[0] = 0xAA;
					flash_buff[1] = 0x00;
					flash_buff[2] = 0xAA;
					user_file_write();
					break;
				case 6:
					//every gpio pin set output
					gpio_all_init_org();
					//pwm1_test_init();
					pwm4_test_init();
					pwm5_test_init();
					adc1_test_init();
					adc2_test_init();
					adc3_test_init();
					adc4_test_init();
					break;
				case 7:
					break;
				case 8:
					break;
				case 9:
					break;
				case 10:
					break;
				case 11:
					break;
				case 12:
					break;
				case 13:
					break;
				case 14:
					break;
				case 15:
					break;
			}

			if(b_gpio_test)
			{
				p_gpio_info = malloc(300);
				p_gpio_info[0] = 0;

				puart2_rxbuff = malloc(32);

				for(i=0;i<USE_PIN_GROUP_IN_NUMBER;i++)
				{					
					gpio_port_output 	= gpio_test_port_out[i];
					gpio_number_output 	= gpio_test_number_out[i];
					gpio_port_input 	= gpio_test_port_in[i];
					gpio_number_input 	= gpio_test_number_in[i];				
					result = gpio_test_handle(gpio_port_output,
													gpio_number_output,
													gpio_port_input,
													gpio_number_input);

					if(result != 0)
					{
						strcat((char*)p_gpio_info,(const char*)gpio_array[i]);
					}
				}
#if (USE_MODULAR == MODULAR_SLM100)
				padc_value[3] = &aio4ChannelResult;
				result = adc_test_handle(ADC_ChannelAio4,6,padc_value[3]);
				if(result != 0)
				{
					strcat((char*)p_gpio_info,(const char*)gpio_array[6]);
				}

				uart2_clear_flag_timeout();
				uart2_set_rxbuff(puart2_rxbuff,32);
				uart2_tx("OK",2);
				while(uart2_get_flag_timeout() == 0)
				{
					osDelay(10);
					break;
				}
				puart2_rxbuff[2] = 0;
				if(strcmp((const char*)puart2_rxbuff,"OK") != 0)
				{
					strcat((char*)p_gpio_info,(const char*)gpio_array[7]);
				}
#elif (USE_MODULAR == MODULAR_SLM160)
//				GPIO_PinWrite(gpio_test_port_out[6],1<<gpio_test_number_out[6],1<<gpio_test_number_out[6]);
//				conversionDone = 0;
//		        ADC_StartConversion(ADC_ChannelAio4, ADC_UserAPP);
//				//while((conversionDone&ADC_ChannelAio4) == 0);
//				while(conversionDone == 0);
//				if(aio4ChannelResult > 1000)
//				{
//					GPIO_PinWrite(gpio_test_port_out[6],1<<gpio_test_number_out[6],0);
//					conversionDone = 0;
//					ADC_StartConversion(ADC_ChannelAio4, ADC_UserAPP);
//					//while((conversionDone&ADC_ChannelAio4) == 0);
//					while(conversionDone == 0);
//					if(aio4ChannelResult > 1000)
//					{
//						strcat((char*)p_gpio_info,(const char*)gpio_array[6]);
//					}
//				}
//				else
//				{
//					strcat((char*)p_gpio_info,(const char*)gpio_array[6]);
//				}

//				padc_value[3] = &aio4ChannelResult;
//				result = adc_test_handle(ADC_ChannelAio4,6,padc_value[3]);
//				if(result != 0)
//				{
//					strcat((char*)p_gpio_info,(const char*)gpio_array[6]);
//				}

#elif (USE_MODULAR == MODULAR_SLM190)

				padc_value[0] = &aio2ChannelResult;
				padc_value[1] = &aio3ChannelResult;
				padc_value[2] = &aio4ChannelResult;
				padc_value[3] = &aio1ChannelResult;
				padc_value[4] = &aio1ChannelResult;

				channel[0] = ADC_ChannelAio2;
				channel[1] = ADC_ChannelAio3;
				channel[2] = ADC_ChannelAio4;
				channel[3] = ADC_ChannelAio1;
				channel[4] = ADC_ChannelAio1;

				for(i=0;i<5;i++)
				{
					if(i==4)
					{
						GPIO_PinWrite(gpio_test_port_out[i],
								1<<gpio_test_number_out[i],
								1<<gpio_test_number_out[i]);
					}

					result = adc_test_handle(channel[i],1+i,padc_value[i]);
					if(result != 0)
					{
						strcat((char*)p_gpio_info,(const char*)gpio_array[1+i]);
					}
				}
				
				uart2_clear_flag_timeout();
				uart2_set_rxbuff(puart2_rxbuff,32);
				uart2_tx("OK",2);
				while(uart2_get_flag_timeout() == 0)
				{
					osDelay(10);
					break;
				}
				puart2_rxbuff[2] = 0;
				if(strcmp((const char*)puart2_rxbuff,"OK") != 0)
				{
					strcat((char*)p_gpio_info,(const char*)gpio_array[6]);
				}
#endif

				length = strlen((const char*)p_gpio_info);
				
				if(length == 0)
				{
					at_reply("\r\n+FACTORYTEST=STEP,%d\r\n\r\nOK\r\n",at_param[1]);
					flash_buff[0] = 0xAA;
					user_file_write();
				}
				else
				{
					at_reply("\r\n+FACTORYTEST=STEP,%d\r\n%s\r\nERROR\r\n",at_param[1],p_gpio_info);	
				}

				free(p_gpio_info);
				free(puart2_rxbuff);
			}
			else
			{
				at_reply("\r\n+FACTORYTEST=STEP,%d\r\n\r\nOK\r\n",at_param[1]);
			}

			break;
	}
}

/**
  \fn          void GPIO_ISR()
  \brief       GPIO interrupt service routine
  \return
*/
void gpio_input_handle(void)
{
    //Save current irq mask and diable whole port interrupts to get rid of interrupt overflow
    uint16_t portIrqMask = GPIO_SaveAndSetIRQMask(INPUT_GPIO_INSTANCE);

    if (GPIO_GetInterruptFlags(INPUT_GPIO_INSTANCE) & (1 << INPUT_GPIO_PIN))
    {
//        gpioInterruptCount++;
//		user_printf("***input\r\n");
		b_gpio_input = 1;
        GPIO_ClearInterruptFlags(INPUT_GPIO_INSTANCE, 1 << INPUT_GPIO_PIN);
    }

    GPIO_RestoreIRQMask(INPUT_GPIO_INSTANCE, portIrqMask);
}

void at24c02_read_test(uint8_t addr)
{
    uint8_t tempBuffer[16];
	uint8_t a;
    a = addr;

    i2cDrvInstance->MasterTransmit(AT24C02_ADDR, &a, 1, true);

    i2cDrvInstance->MasterReceive(AT24C02_ADDR, tempBuffer, 1, true);

//	user_printf("tempBuffer[0] = %x\r\n",tempBuffer[0]);
}

void user_atcmd_task(void * argument)
{
//	GetExtCfgSetting extCfgSetting;
//	UINT8 bandNum[8];
//	UINT8 orderBand[8];
//	INT8 networkMode[8];
//	UINT8 temp[32];

	//重新定义AT串口波特率
//    extern uint8_t* atCmdGetIPRString(void);
//    plat_config_fs_t* fsPlatConfigPtr = BSP_GetFsPlatConfig();
//    atCmdUartChangeConfig(0, 115200, fsPlatConfigPtr->atPortFrameFormat.wholeValue, 1);

//	移动卡ICCID: 8986 0493 2621 9021 0987

	while(1)
	{
		osSemaphoreAcquire(userAtCmdSemHandle,osWaitForever);
		user_printf("\r\n*****user_atcmd_task()*****\r\n");

		switch(at_cmd)
		{
			case ATCMD_TEST_SET:
				atcmd_TEST_set();
				break;
			
			default:
				break;
		}
	}
}

void gpio_test_task(void * argument)
{
	uint8_t state = 0;
	uint8_t time_cnt = 0;
//	uint32_t pin_read;
	ARM_SPI_STATUS spi_state;
//	uint8_t i;
//	uint8_t *puart2_rxbuff;
//	int8_t result;
	uint8_t cnt_1s = 0;
//    uint8_t counter = 0;
//    slpManSlpState_t pstate;
//	slpManRet_t slp_result;
		
//	i2c_test_init();
//	gpio_test_init();
//	pwm1_test_init();
//	file_test_write();
//	timer0_init();
//	spi0_test_init();
//	uart2_init();

	user_printf("gpio_test_task\r\n");
	
	//	puart2_rxbuff = malloc(32);

	while(1)
	{
		time_cnt++;
		if(time_cnt >= 100)
		{
			time_cnt = 0;

//			user_printf("gpio_test_task\r\n");
//			file_test_read();

			if(b_gpio_toggle)
			{
				if(state == 0)
				{
					state = 1;
					GPIO_PinWrite(gpio_port, 1 << gpio_number, 1 << gpio_number);
				}
				else
				{
					state = 0;
					GPIO_PinWrite(gpio_port, 1 << gpio_number, 0);
				}
			}

			if(b_adc_print)
			{
			  	conversionDone = 0;
		        ADC_StartConversion((adc_channel_t)adc_channel, ADC_UserAPP);
		        // Wait for all channels conversion completes
		        while(conversionDone != adc_channel);
		        user_printf("Conversion result:\r\n");
		        user_printf("Aio%d    : raw register :%d, Voltage:%dmv\r\n",adc_channel, (int)*padc_result, (int)HAL_ADC_CalibrateRawCode(*padc_result));
			}

			if(b_timer0_int)
			{
				b_timer0_int = 0;
//				user_printf("b_timer0_int get\r\n");
			}
			
//			at24c02_read_test(100);

//			if(state == 0)
//			{
//				state = 1;
//				SPI0_CS_SET();
//				GPIO_PinWrite(TEST_GPIO_INSTANCE, 1 << TEST_GPIO_PIN, 1 << TEST_GPIO_PIN);
//			}
//			else
//			{
//				state = 0;
//				SPI0_CS_CLEAR();
//				GPIO_PinWrite(TEST_GPIO_INSTANCE, 1 << TEST_GPIO_PIN, 0);
//			}
//			pin_read = GPIO_PinRead(INPUT_GPIO_INSTANCE,INPUT_GPIO_PIN);
//			user_printf("pin_read = %d\r\n",pin_read);

//			user_printf("master_buffer_in[4] = %x\r\n",master_buffer_in[4]);
//			user_printf("master_buffer_in[5] = %x\r\n",master_buffer_in[5]);
//			user_printf("master_buffer_in[6] = %x\r\n",master_buffer_in[6]);
//			for(i=0;i<5;i++)
//			{
//				spiMasterDrv->Transfer(&master_buffer_out[i],master_buffer_in, 1);
//				delay_us(1000);		
//			}

//			uart2_clear_flag_timeout();
//			uart2_set_rxbuff(puart2_rxbuff,32);
//			uart2_tx("OK",2);
//			while(uart2_get_flag_timeout() == 0)
//			{
//				;
//			}
//			atUartPrint("puart2_rxbuff[0] = %x\r\n",puart2_rxbuff[0]);	
//			atUartPrint("puart2_rxbuff[1] = %x\r\n",puart2_rxbuff[1]);	

			if(b_reset_fail)
			{
				cnt_1s++;
				if(cnt_1s >= 1)
				{
					cnt_1s = 0;

					b_reset_fail = 0;
					flash_buff[1] = 0x22;
					flash_buff[2] = 0x33;
					
					user_file_write();
				}
			}

//			slp_result = slpManCheckVoteState(userAtCmdSlpHandler, &pstate, &counter);
//	       	user_printf("slp_result = %d,pstate = %d,counter = %d\r\n",slp_result,pstate,counter);
//			if(RET_TRUE == slp_result)
//	        {
//	        	atUartPrint("pstate = %d,counter = %d\r\n",pstate,counter);
//	            for(; counter > 0; counter -- )
//	            {
//	                slpManPlatVoteForceEnableSleep(userAtCmdSlpHandler, SLP_HIB_STATE);
//	            }
//	        }
//			
//	        pmuVoteToHIBState(PMU_DEEPSLP_PLAT_MOD, TRUE);

//			slpManPlatVoteDisableSleep(userAtCmdSlpHandler, SLP_SLP2_STATE);
//	        slp_result = slpManPlatVoteEnableSleep(userAtCmdSlpHandler, SLP_HIB_STATE);
//	       	user_printf("(2)slp_result = %d\r\n",slp_result);

//			osThreadSuspend(gpio_test_task_handle);
		}


//	    slpManSetPmuSleepMode(true,SLP_HIB_STATE,false);

//        slpManPlatVoteDisableSleep(userAtCmdSlpHandler, SLP_HIB_STATE);
//        slpManPlatVoteEnableSleep(userAtCmdSlpHandler, SLP_HIB_STATE);

		if(b_gpio_input)
		{
			b_gpio_input = 0;
			user_printf("***b_gpio_input\r\n");
		}
		
		osDelay(10);
	}
}

//static void appBeforeSlp1(void *pdata, slpManLpState state)
//{
//	//atUartPrint("appBeforeSlp1\r\n");
//}

//static void appAfterSlp1(void *pdata, slpManLpState state)
//{
//	//atUartPrint("appAfterSlp1\r\n");
//}

void app_user_atcmd_init(void)
{	
	user_atcmd_task_handle 	= osThreadNew(user_atcmd_task, NULL, &userAtCmdTask_attributes);
 	userAtCmdSemHandle 		= osSemaphoreNew(1, 0, &userAtCmdSem_attributes);

	gpio_test_task_handle 	= osThreadNew(gpio_test_task, NULL, &gpioTestTask_attributes);

//	file_test_read();
//	file_test_write();
	user_printf("app_user_atcmd_init\r\n");

//	slpManRegisterUsrdefinedBackupCb(appBeforeSlp1,NULL,SLPMAN_SLEEP1_STATE);
//    slpManRegisterUsrdefinedRestoreCb(appAfterSlp1,NULL,SLPMAN_SLEEP1_STATE);

//	slpManApplyPlatVoteHandle("USERATCMD",&userAtCmdSlpHandler);

	user_printf("gpio_test_task create.\r\n");

//	timer0_init();

}




