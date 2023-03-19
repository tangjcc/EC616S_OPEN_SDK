/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    dma_demo.c
 * Description:  EC616S DMA driver example entry source file
 * History:      Rev1.0   2018-08-15
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "dma_ec616s.h"

/*
   This project demotrates the usage of DMA, in this example, we'll employ no-descriptor-fetch mode, also called as one-shot mode,
   to perform memory to memory transfer.
 */

/** \brief DMA total transfer size in unit of byte */
#define DMA_TRANFER_SIZE           (32)

/** \brief DMA transfer source and destination data */
static int8_t sourceData[DMA_TRANFER_SIZE];
static int8_t destData[DMA_TRANFER_SIZE];

/** \brief DMA transfer channel */
static int32_t transferChannel;

/**
  \brief Flag to indicate whether DMA transfer is finished,
         data checking is conducted to see if error happens
         during transfering after transaction is over
  */
static volatile bool isDMATransferDone = false;

/**
  \fn          void DMA_EventHandler(uint32_t event)
  \brief       DAM event handler, will be registered and called upon tranfer ends
  \return
*/
static void DMA_EventHandler(uint32_t event)
{
    switch(event)
    {
        case DMA_EVENT_END:
            isDMATransferDone = true;
            DMA_StopChannel(transferChannel, false);
            break;
        case DMA_EVENT_ERROR:
        default:
            break;
    }
}

/**
  \fn          void DMA_M2MTransferExample(void)
  \brief       DMA M2M transfer demo entry function
  \return
*/
void DMA_M2MTransferExample(void)
{

    int32_t i;
    int32_t dmaRet;

    // Initialize
    for (i = 0; i < DMA_TRANFER_SIZE; i++)
    {
        sourceData[i] = i;
        destData[i] = 0;
    }

    DMA_Init();

    // open a free channel
    dmaRet = DMA_OpenChannel();

    if (ARM_DMA_ERROR_CHANNEL_ALLOC == dmaRet)
    {
        printf("DMA channel open error!\n");
        while(1);
    }

    transferChannel = dmaRet;

    dma_transfer_config dmaTxConfig = {(void*)sourceData, (void*)destData,
                                       DMA_FlowControlNone, DMA_AddressIncrementBoth,
                                       DMA_DataWidthNoUse,
                                       DMA_Burst64Bytes, DMA_TRANFER_SIZE
                                      };
    // channel setup
    DMA_ChannelSetRequestSource(transferChannel, DMA_MemoryToMemory);

    DMA_TransferSetup(transferChannel, &dmaTxConfig);

    DMA_ChannelRigisterCallback(transferChannel, DMA_EventHandler);

    DMA_EnableChannelInterrupts(transferChannel, DMA_EndInterruptEnable);

    // start
    DMA_StartChannel(transferChannel);

    while(1)
    {
        if (isDMATransferDone == true)
        {
            for (i = 0; i < DMA_TRANFER_SIZE; i++)
            {
                if (sourceData[i] != destData[i])
                    break;
            }

            if(i == DMA_TRANFER_SIZE)
                printf("DMA M2M tranfer done and no error happens\n");
            else
                printf("DMA M2M tranfer done, error happens in tranfer");

            DMA_CloseChannel(transferChannel);
            return;
        }
    }
}

void DMA_ExampleEntry(void)
{
    printf("DMA Example Start\r\n");

    DMA_M2MTransferExample();

    printf("DMA Example End\r\n");

    while(1);
}
