#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "queue.h"
#include "gps_proc.h"
#include "i2c_proc.h"

void GpsDataRecvProcessing(uint8_t *rawData, uint32_t rawDataLen);

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
uint32_t recvCount = 0;

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

void USART_ExampleEntry(void *arg)
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

    //uint8_t* greetStr = "UART Echo Demo\n";

    //USARTdrv->Send(greetStr, strlen((const char*)greetStr));

    while (1)
    {
		//printf("Waiting...\r\n");
        // In polling mode, receive api will block until required recv len is reached.
        // In interrupt or dma mode, receive api returns immdiately after transation starts.  

#if(RTE_UART_RX_IO_MODE == POLLING_MODE)   
        USARTdrv->Receive(&recBuffer[recvCount], 1);
		//printf("%c", recBuffer[recvCount]);
		recvCount++;
		if (recBuffer[recvCount - 1] == '\n') {
			GpsDataRecvProcessing(recBuffer, recvCount);
			recBuffer[recvCount] = '\0';
			recvCount=0;
		}
        //USARTdrv->Send(recBuffer, 1);
#elif (RTE_UART_RX_IO_MODE == IRQ_MODE) || (RTE_UART_RX_IO_MODE == DMA_MODE)

        USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);
		printf(">>%s\r\n", recBuffer);
		//printf("[0x%02x0x%02x0x%02x0x%02x]\r\n", recBuffer[0], recBuffer[1], recBuffer[2], recBuffer[3]);

        while((isRecvTimeout == false) && (isRecvComplete == false));

		//USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);
        if(isRecvTimeout == true)
        {
            //USARTdrv->Send(recBuffer, USARTdrv->GetRxCount());
            isRecvTimeout = false;
        }
        else
        {
            isRecvComplete = false;
            //USARTdrv->Send(recBuffer, RECV_BUFFER_LEN);
        }
		recvCount = USARTdrv->GetRxCount();
#else
#error "UART RX IO MODE IS NOT SUPPORTED IN THIS EXAMPLE"
#endif
    }
}

/** \brief GPS_EN location */
#define GPS_EN_GPIO_INSTANCE        (0)
#define GPS_EN_GPIO_PIN             (5)
#define GPS_EN_PAD_INDEX            (16)
#define GPS_EN_PAD_ALT_FUNC         (PAD_MuxAlt0)


void gpsEnable(void)
{
	printf("GPS Enable GPIO config\r\n");

    // GPIO function select
    pad_config_t padConfig;
    PAD_GetDefaultConfig(&padConfig);

    padConfig.mux = GPS_EN_PAD_ALT_FUNC;
    PAD_SetPinConfig(GPS_EN_PAD_INDEX, &padConfig);

    // GPS_EN pin config
    gpio_pin_config_t config;
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 0;

    GPIO_PinConfig(GPS_EN_GPIO_INSTANCE, GPS_EN_GPIO_PIN, &config);
}

// app task static stack and control block
#ifdef WITH_DTLS
    #define GPS_TASK_STACK_SIZE    (512)
#else
    #define GPS_TASK_STACK_SIZE    (1024)
#endif

static StaticTask_t             usartTask;
static UINT8                    usartTaskStack[GPS_TASK_STACK_SIZE];

void gpsApiInit(void)
{
    // Apply own right power policy according to design

    osThreadAttr_t task_attr;
    memset(&task_attr,0,sizeof(task_attr));
    memset(usartTaskStack, 0xA5, GPS_TASK_STACK_SIZE);
    task_attr.name = "usart_task";
    task_attr.stack_mem = usartTaskStack;
    task_attr.stack_size = GPS_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;
    task_attr.cb_mem = &usartTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(USART_ExampleEntry, NULL, &task_attr);  //创建连接平台线程
	gpsEnable();
	//I2C_init();
}

extern unsigned char Flag_Calc_GPGGA_OK;
extern unsigned char Flag_Calc_GPRMC_OK;

GPS_Date g_GPS_Date = {0};
GPS_Location g_GPS_Location = {0};

#define GPS_RAW_DATA_MAX_LEN 80
#define MAX(a, b) ((a) > (b) ? (a) : (b))
char g_GPS_raw_data[GPS_RAW_DATA_MAX_LEN] = {0};

void GpsDataRecvProcessing(uint8_t *rawData, uint32_t rawDataLen)
{
	//printf("GpsDataRecvProcessing:[%d]%s\r\n", rawDataLen, rawData);

	if (GPS_IS_GPGGA_DATA(rawData)) {
		rawDataLen = MAX(rawDataLen + 1, GPS_RAW_DATA_MAX_LEN);
		strncpy(g_GPS_raw_data, (char *)rawData, rawDataLen);
		g_GPS_raw_data[rawDataLen - 1] = '\0';
		GPS_GPGGA_CAL(rawData, &g_GPS_Date, &g_GPS_Location);
	}
	if (GPS_IS_GPRMC_DATA(rawData)) {
		GPS_GPRMC_CAL(rawData, &g_GPS_Date);
	}
	//GPS_DateShow(&g_GPS_Date);
	//GPS_LocationShow(&g_GPS_Location);
}

void GpsGetData(GPS_Date **date, GPS_Location **loc)
{
	*date = &g_GPS_Date;
	*loc = &g_GPS_Location;
}

void GpsGetRawData(char *out, int maxLen, int *actLen)
{
	int gpsDataLen = strlen(g_GPS_raw_data);
	if (out == NULL || maxLen < gpsDataLen) {
		return;
	}
	if (gpsDataLen == 0) {
		return;
	}
	strncpy(out, g_GPS_raw_data, gpsDataLen + 1);
	*actLen = gpsDataLen;
}


