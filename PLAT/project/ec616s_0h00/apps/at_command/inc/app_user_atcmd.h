#ifndef APP_USER_ATCMD_H
#define APP_USER_ATCMD_H

typedef unsigned char 	uint8_t;
typedef unsigned short 	uint16_t;
typedef unsigned int 	uint32_t;
typedef signed char 	int8_t;

#define ATUART_DEBUG 	0

#define CUSTOMER_DAHUA

#define MCU_EC616 	0
#define MCU_EC616S	1
#define USE_MCU		MCU_EC616S

#define MODULAR_SLM100 	0
#define MODULAR_SLM130 	1
#define MODULAR_SLM160 	2
#define MODULAR_SLM190 	3
#define USE_MODULAR		MODULAR_SLM190

#if (USE_MODULAR == MODULAR_SLM100)
#define USE_PIN_GROUP_OUT_NUMBER	8
#define USE_PIN_GROUP_IN_NUMBER		6

#define PIN_GROUP0_PORT_OUT		0
#define PIN_GROUP0_PORT_IN		0
#define PIN_GROUP0_NUMBER_OUT	9
#define PIN_GROUP0_NUMBER_IN	8

#define PIN_GROUP1_PORT_OUT		0
#define PIN_GROUP1_PORT_IN		0
#define PIN_GROUP1_NUMBER_OUT	6
#define PIN_GROUP1_NUMBER_IN	5

#define PIN_GROUP2_PORT_OUT		0
#define PIN_GROUP2_PORT_IN		0
#define PIN_GROUP2_NUMBER_OUT	4
#define PIN_GROUP2_NUMBER_IN	3

#define PIN_GROUP3_PORT_OUT		0
#define PIN_GROUP3_PORT_IN		0
#define PIN_GROUP3_NUMBER_OUT	2
#define PIN_GROUP3_NUMBER_IN	7

#define PIN_GROUP4_PORT_OUT		0
#define PIN_GROUP4_PORT_IN		0
#define PIN_GROUP4_NUMBER_OUT	1
#define PIN_GROUP4_NUMBER_IN	13

#define PIN_GROUP5_PORT_OUT		0
#define PIN_GROUP5_PORT_IN		0
#define PIN_GROUP5_NUMBER_OUT	12
#define PIN_GROUP5_NUMBER_IN	11

#define PIN_GROUP6_PORT_OUT		0
#define PIN_GROUP6_NUMBER_OUT	10

#define PIN_GROUP7_PORT_OUT		0
#define PIN_GROUP7_NUMBER_OUT	0

#elif (USE_MODULAR == MODULAR_SLM130)
#define USE_PIN_GROUP_OUT_NUMBER	6
#define USE_PIN_GROUP_IN_NUMBER		5

#define PIN_GROUP0_PORT_OUT		0
#define PIN_GROUP0_PORT_IN		0
#define PIN_GROUP0_NUMBER_OUT	4
#define PIN_GROUP0_NUMBER_IN	5

#define PIN_GROUP1_PORT_OUT		0
#define PIN_GROUP1_PORT_IN		0
#define PIN_GROUP1_NUMBER_OUT	3
#define PIN_GROUP1_NUMBER_IN	2

#define PIN_GROUP2_PORT_OUT		1
#define PIN_GROUP2_PORT_IN		0
#define PIN_GROUP2_NUMBER_OUT	2
#define PIN_GROUP2_NUMBER_IN	14

#define PIN_GROUP3_PORT_OUT		0
#define PIN_GROUP3_PORT_IN		0
#define PIN_GROUP3_NUMBER_OUT	11
#define PIN_GROUP3_NUMBER_IN	15

#define PIN_GROUP4_PORT_OUT		1
#define PIN_GROUP4_PORT_IN		1
#define PIN_GROUP4_NUMBER_OUT	3
#define PIN_GROUP4_NUMBER_IN	0

#define PIN_GROUP5_PORT_OUT		0
#define PIN_GROUP5_NUMBER_OUT	1

#elif (USE_MODULAR == MODULAR_SLM160)
#define USE_PIN_GROUP_OUT_NUMBER	8
#define USE_PIN_GROUP_IN_NUMBER		6

#define PIN_GROUP0_PORT_OUT		0
#define PIN_GROUP0_PORT_IN		0
#define PIN_GROUP0_NUMBER_OUT	9
#define PIN_GROUP0_NUMBER_IN	8

#define PIN_GROUP1_PORT_OUT		0
#define PIN_GROUP1_PORT_IN		0
#define PIN_GROUP1_NUMBER_OUT	3
#define PIN_GROUP1_NUMBER_IN	5

#define PIN_GROUP2_PORT_OUT		0
#define PIN_GROUP2_PORT_IN		0
#define PIN_GROUP2_NUMBER_OUT	4
#define PIN_GROUP2_NUMBER_IN	6

#define PIN_GROUP3_PORT_OUT		0
#define PIN_GROUP3_PORT_IN		0
#define PIN_GROUP3_NUMBER_OUT	11
#define PIN_GROUP3_NUMBER_IN	10

#define PIN_GROUP4_PORT_OUT		0
#define PIN_GROUP4_PORT_IN		0
#define PIN_GROUP4_NUMBER_OUT	0
#define PIN_GROUP4_NUMBER_IN	1

#define PIN_GROUP5_PORT_OUT		0
#define PIN_GROUP5_PORT_IN		0
#define PIN_GROUP5_NUMBER_OUT	13
#define PIN_GROUP5_NUMBER_IN	12

#define PIN_GROUP6_PORT_OUT		0
#define PIN_GROUP6_NUMBER_OUT	2

#define PIN_GROUP7_PORT_OUT		0
#define PIN_GROUP7_NUMBER_OUT	7

#elif (USE_MODULAR == MODULAR_SLM190)
#define USE_PIN_GROUP_OUT_NUMBER	7
#define USE_PIN_GROUP_IN_NUMBER		1

#define PIN_GROUP0_PORT_OUT		0
#define PIN_GROUP0_PORT_IN		0
#define PIN_GROUP0_NUMBER_OUT	1
#define PIN_GROUP0_NUMBER_IN	0

#define PIN_GROUP1_PORT_OUT		0
#define PIN_GROUP1_NUMBER_OUT	13

#define PIN_GROUP2_PORT_OUT		0
#define PIN_GROUP2_NUMBER_OUT	12

#define PIN_GROUP3_PORT_OUT		0
#define PIN_GROUP3_NUMBER_OUT	8

#define PIN_GROUP4_PORT_OUT		0
#define PIN_GROUP4_NUMBER_OUT	9

#define PIN_GROUP5_PORT_OUT		0
#define PIN_GROUP5_NUMBER_OUT	2

#define PIN_GROUP6_PORT_OUT		0
#define PIN_GROUP6_NUMBER_OUT	7

#endif

#define ATCMD_NCONFIG_READ 	1
#define ATCMD_NBAND_TEST	2
#define ATCMD_NBAND_READ	3
#define ATCMD_NCCID_READ	4
#define ATCMD_NUESTATS_SET	5
#define ATCMD_TEST_SET		6
#define ATCMD_CFUN_SET		7

#define USER_NVM_FILE_NAME	"user_cfg.nvm"

#define SPI0_CS_SET()		GPIO_PinWrite(0, 1 << 3, 1 << 3);	
#define SPI0_CS_CLEAR()		GPIO_PinWrite(0, 1 << 3, 0);


#define at_reply(fmt,...)	atUartPrint(fmt,##__VA_ARGS__)

#define user_printf(fmt,...)				\
	do										\
	{										\
		if(is_enable_debug())				\
		{									\
			atUartPrint(fmt,##__VA_ARGS__);	\
		}									\
	}while(0)								\

uint8_t is_enable_debug(void);
void enable_debug_set(uint8_t value);
void gpio_sleep_state_init(void);
void user_disable_lpusart1(void);
void set_flash_data(uint8_t *pdata,uint8_t length,uint8_t offset);
void get_flash_data(uint8_t *pdata,uint8_t length);
void set_atcmd_name(uint8_t n);
uint8_t get_atcmd_name(void);
void userAtCmdSemRelease(uint32_t cmd,void *pdata,uint16_t length);
void app_user_atcmd_init(void);
void file_test_write(void);
uint8_t file_test_read(void);
int8_t user_file_write(void);
void user_file_handle(void);

#endif


