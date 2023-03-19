/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    spi_demo.c
 * Description:  EC616S SPI driver example entry source file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/


#include <stdio.h>
#include <string.h>
#include "bsp.h"

/*
   This project demotrates the usage of SPI, in this example, we'll leverage two instances of SPI in one board to communicate with each other,
   SPI work mode must be configured in RTE_Device.h to be dma mode for work properly.
 */

/** \brief SPI data width */
#define TRANFER_DATA_WIDTH_8_BITS        (8)
#define TRANFER_DATA_WIDTH_16_BITS       (16)
#define TRANSFER_DATA_WIDTH              TRANFER_DATA_WIDTH_8_BITS

/** \brief transfer data size in one transaction
    \note size shall be greater than DMA request level(4)
 */
#define TRANSFER_SIZE                    (8)

/** \brief transaction type */
#define MASTER_SEND_SLAVE_RECV           (0)
#define FULL_DULPEX                      (1)

#define TRANSACTION_TYPE                 FULL_DULPEX

#define RetCheck(cond, v1)                  \
do{                                         \
    if (cond == ARM_DRIVER_OK)              \
    {                                       \
        printf("%s OK\n", (v1));            \
    } else {                                \
        printf("%s Failed !!!\n", (v1));    \
    }                                       \
}while(false)

/** \brief driver instance declare */
extern ARM_DRIVER_SPI Driver_SPI0;
extern ARM_DRIVER_SPI Driver_SPI1;

static ARM_DRIVER_SPI   *spiMasterDrv = &CREATE_SYMBOL(Driver_SPI, 0);
static ARM_DRIVER_SPI   *spiSlaveDrv = &CREATE_SYMBOL(Driver_SPI, 1);

static volatile bool isTransferDone;

/** \brief transaction buffer */
#if(TRANSFER_DATA_WIDTH == TRANFER_DATA_WIDTH_8_BITS)
static __ALIGNED(4) uint8_t master_buffer_out[TRANSFER_SIZE];
static __ALIGNED(4) uint8_t master_buffer_in[TRANSFER_SIZE];

static __ALIGNED(4) uint8_t slave_buffer_out[TRANSFER_SIZE];
static __ALIGNED(4) uint8_t slave_buffer_in[TRANSFER_SIZE];

#elif (TRANSFER_DATA_WIDTH == TRANFER_DATA_WIDTH_16_BITS)
static __ALIGNED(4) uint16_t master_buffer_out[TRANSFER_SIZE];
static __ALIGNED(4) uint16_t master_buffer_in[TRANSFER_SIZE];

static __ALIGNED(4) uint16_t slave_buffer_out[TRANSFER_SIZE];
static __ALIGNED(4) uint16_t slave_buffer_in[TRANSFER_SIZE];
#endif

#if (RTE_SPI0_IO_MODE != DMA_MODE) && (RTE_SPI1_IO_MODE != DMA_MODE)
#error "slave can't work in polling mode in this example!"
#endif

static void SPI_SlaveCallback(uint32_t event)
{
    if(event & ARM_SPI_EVENT_TRANSFER_COMPLETE)
    {
        isTransferDone = true;
    }
}

void SPI_ExampleEntry_org(void)
{
    int32_t ret, i;
    uint32_t timeOut_ms = 5000;

    isTransferDone = false;

    // Transfer buffer initialize
    for(i = 0; i < TRANSFER_SIZE; i++)
    {
        master_buffer_out[i] = i+'A';

#if  (TRANSACTION_TYPE == FULL_DULPEX)
        slave_buffer_out[i] = i+'a';
#endif
    }

#if  (TRANSACTION_TYPE == FULL_DULPEX)
    memset(master_buffer_in, 0, sizeof(master_buffer_in));
#endif

    memset(slave_buffer_in, 0, sizeof(slave_buffer_in));

    printf("SPI Example Start\r\n");

    // Initialize master spi
    ret = spiMasterDrv->Initialize(NULL);
    RetCheck(ret, "Initialize master");
    // Power on
    ret = spiMasterDrv->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on master");

    // Initialize slave spi with callback
    ret = spiSlaveDrv->Initialize(SPI_SlaveCallback);
    RetCheck(ret, "Initialize slave");
    // Power on
    ret = spiSlaveDrv->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on slave");

    // Configure slave spi bus
    ret = spiSlaveDrv->Control(ARM_SPI_MODE_SLAVE | ARM_SPI_CPOL1_CPHA0 | ARM_SPI_DATA_BITS(TRANSFER_DATA_WIDTH) |
                                  ARM_SPI_MSB_LSB, 0);

    RetCheck(ret, "Configure slave bus");

    // Configure slave spi bus
    ret = spiMasterDrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA0 | ARM_SPI_DATA_BITS(TRANSFER_DATA_WIDTH) |
                                  ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_UNUSED, 200000U);

    RetCheck(ret, "Configure master bus");

#if (TRANSACTION_TYPE == FULL_DULPEX)

    spiSlaveDrv->Transfer(slave_buffer_out, slave_buffer_in, TRANSFER_SIZE);
    spiMasterDrv->Transfer(master_buffer_out, master_buffer_in, TRANSFER_SIZE);

#elif (TRANSACTION_TYPE == MASTER_SEND_SLAVE_RECV)

    spiSlaveDrv->Receive(slave_buffer_in, TRANSFER_SIZE);
    spiMasterDrv->Send(master_buffer_out, TRANSFER_SIZE);

#endif

    do
    {
        delay_us(1000);

    }
    while((isTransferDone == false) && --timeOut_ms);

    if(timeOut_ms == 0)
    {
        printf("Transfer failed for timeout\n");
    }
    else
    {
        for(i = 0; i < TRANSFER_SIZE; i++)
        {
            if(master_buffer_out[i] != slave_buffer_in[i])
                break;

#if  (TRANSACTION_TYPE == FULL_DULPEX)
            if(master_buffer_in[i] != slave_buffer_out[i])
                break;
#endif
        }

        if(i != TRANSFER_SIZE)
        {
            printf("Transfer failed for data verifying\n");
        }
        else
        {
            printf("Transfer success\n");
        }
    }

    while(1);

}

void SPI_ExampleEntry_bk1(void)
{
    int32_t ret;

    memset(master_buffer_in, 0, sizeof(master_buffer_in));

	master_buffer_out[0] = 0x90;
	master_buffer_out[1] = 0x00;
	master_buffer_out[2] = 0x00;
	master_buffer_out[3] = 0x00;
	master_buffer_out[4] = 0xFF;
	master_buffer_out[5] = 0xFF;

    printf("spi0_test_init\r\n");

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

}

void SPI_ExampleEntry(void)
{
    int32_t ret,i;
    uint32_t timeOut_ms = 5000;

    memset(master_buffer_in, 0, sizeof(master_buffer_in));

	master_buffer_out[0] = 0x11;
	master_buffer_out[1] = 0x22;
	master_buffer_out[2] = 0x33;
	master_buffer_out[3] = 0x44;
	master_buffer_out[4] = 0x55;
	master_buffer_out[5] = 0x66;

    printf("spi0_test_init\r\n");

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

	while(1)
	{
		printf("spi0 Transfer\r\n");
//		spiMasterDrv->Transfer(master_buffer_out, master_buffer_in, TRANSFER_SIZE);
//        delay_us(1000*500);

		for(i=0;i<5;i++)
		{
			spiMasterDrv->Transfer(&master_buffer_out[i],master_buffer_in, 1);
			delay_us(1000);		
		}
        delay_us(1000*500);		
	}
}

