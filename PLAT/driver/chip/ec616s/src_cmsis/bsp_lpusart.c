/*
 * Copyright (c) 2013-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bsp_lpusart.h"
#include "slpman_ec616s.h"
#include "debug_log.h"

#define LPUSART_DRIVER_DEBUG  1

//#pragma push
//#pragma O0

#define ARM_LPUSART_DRV_VERSION    ARM_DRIVER_VERSION_MAJOR_MINOR(2, 0)  /* driver version */

#if (!RTE_UART1)
#error "Cooperating UART not enabled in RTE_Device.h!"
#endif

#define CO_USART_DMA_BURST_SIZE  (8)
#define LPUSART_RX_TRIG_LVL      (8)

#ifdef PM_FEATURE_ENABLE
/** \brief Internal used data structure */
typedef struct _lpusart_database
{
    bool                            isInited;            /**< Whether usart has been initialized */
    struct
    {
        uint32_t DLL;                                    /**< Divisor Latch Low */
        uint32_t DLH;                                    /**< Divisor Latch High */
        uint32_t IER;                                    /**< Interrupt Enable Register */
        uint32_t FCR;                                    /**< FIFO Control Register */
        uint32_t LCR;                                    /**< Line Control Register */
        uint32_t MCR;                                    /**< Modem Control Register */
        uint32_t MFCR;                                   /**< Main Function Control Register */
        uint32_t EFCR;                                   /**< Extended Function Control Register */
    } co_usart_registers;                                /**< Backup registers for low power restore */
    struct
    {
        uint32_t TCR;                                    /**< Timeout Control Register,                 offset: 0x4 */
        uint32_t FCSR;                                   /**< FIFO Control and Status Register,         offset: 0xC */
        uint32_t IER;                                    /**< Interrupt Enable Register,                offset: 0x10 */
    } core_registers;
    bool     autoBaudRateDone;                           /**< Flag indication whether auto baud dection is done */
} lpusart_database_t;

static lpusart_database_t g_lpusartDataBase = {0};

#endif

#ifdef PM_FEATURE_ENABLE
/**
  \brief Bitmap of LPUSART working status */
static uint32_t g_lpusartWorkingStatus = 0;

/**
  \fn        static void LPUSART_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
  \brief     Perform necessary preparations before sleep.
             After recovering from SLPMAN_SLEEP1_STATE, LPUSART hareware is repowered, we backup
             some registers here first so that we can restore user's configurations after exit.
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
static void LPUSART_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
{
    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            if(g_lpusartDataBase.isInited == true)
            {
                g_lpusartDataBase.co_usart_registers.IER = USART_1->IER;
                g_lpusartDataBase.co_usart_registers.LCR = USART_1->LCR;
                g_lpusartDataBase.co_usart_registers.MCR = USART_1->MCR;
                g_lpusartDataBase.co_usart_registers.MFCR = USART_1->MFCR;
                g_lpusartDataBase.co_usart_registers.EFCR = USART_1->EFCR;

                g_lpusartDataBase.core_registers.TCR = LPUSART_CORE->TCR;
                g_lpusartDataBase.core_registers.FCSR = LPUSART_CORE->FCSR;
                g_lpusartDataBase.core_registers.IER = LPUSART_CORE->IER;
            }
            break;
        default:
            break;
    }

}

/**
  \fn        static void LPUSART_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
  \brief     Restore after exit from sleep.
             After recovering from SLPMAN_SLEEP1_STATE, LPUSART hareware is repowered, we restore user's configurations
             by aidding of the stored registers.

  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state

 */
static void LPUSART_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
{
    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            // no need to restore if failing to sleep
            if(pmuGetSleepedFlag() == false)
            {
                break;
            }

            if(g_lpusartDataBase.isInited == true)
            {
                GPR_ClockEnable(GPR_UART1APBClk);
                GPR_ClockEnable(GPR_UART1FuncClk);

                USART_1->LCR |= USART_LCR_ACCESS_DIVISOR_LATCH_Msk;
                USART_1->DLL = g_lpusartDataBase.co_usart_registers.DLL;
                USART_1->DLH = g_lpusartDataBase.co_usart_registers.DLH;

                USART_1->LCR &= ~USART_LCR_ACCESS_DIVISOR_LATCH_Msk;
                USART_1->IER = g_lpusartDataBase.co_usart_registers.IER;
                USART_1->LCR = g_lpusartDataBase.co_usart_registers.LCR;
                USART_1->MCR = g_lpusartDataBase.co_usart_registers.MCR;
                USART_1->EFCR = g_lpusartDataBase.co_usart_registers.EFCR;
                USART_1->FCR = g_lpusartDataBase.co_usart_registers.FCR;
                USART_1->MFCR = g_lpusartDataBase.co_usart_registers.MFCR;

                LPUSART_CORE->DCR = LPUSARTCORE_DCR_RX_REQ_EN_Msk;
                LPUSART_CORE->TCR = (LPUSARTCORE_TCR_TOCNT_SWTRG_Msk | g_lpusartDataBase.core_registers.TCR);
                LPUSART_CORE->IER = g_lpusartDataBase.core_registers.IER;
            }

            break;

        default:
            break;
    }

}

#define  LOCK_SLEEP(tx, rx)     do                                                                        \
                                          {                                                               \
                                              g_lpusartWorkingStatus |= (rx);                             \
                                              g_lpusartWorkingStatus |= (tx << 1);                        \
                                              slpManDrvVoteSleep(SLP_VOTE_LPUSART, SLP_ACTIVE_STATE);     \
                                          }                                                               \
                                          while(0)

#define  CHECK_TO_UNLOCK_SLEEP(tx, rx)      do                                                              \
                                                      {                                                     \
                                                          g_lpusartWorkingStatus &= ~(rx);                  \
                                                          g_lpusartWorkingStatus &= ~(tx << 1);             \
                                                          if(g_lpusartWorkingStatus == 0)                   \
                                                          {                                                 \
                                                              NVIC_ClearPendingIRQ(LpuartWakeup_IRQn);                    \
                                                              slpManDrvVoteSleep(SLP_VOTE_LPUSART, SLP_SLP1_STATE);       \
                                                          }                                                               \
                                                      }                                                                   \
                                                      while(0)
#endif

#define  LPUSART_AON_REGISTER_WRITE(register, value)      do                                                      \
                                                          {                                                       \
                                                              (register) = (value);                               \
                                                              while((register) != (value));                       \
                                                          }                                                       \
                                                          while(0)

// declearation for DMA API
extern void DMA_StopChannelNoWait(uint32_t channel);
extern uint32_t DMA_SetDescriptorTransferLen(uint32_t dcmd, uint32_t len);
extern void DMA_ChannelLoadDescriptorAndRun(uint32_t channel, void* descriptorAddress);
extern uint32_t DMA_ChannelGetCurrentTargetAddress(uint32_t channel, bool sync);

// Driver Version
static const ARM_DRIVER_VERSION DriverVersion = {
    ARM_USART_API_VERSION,
    ARM_LPUSART_DRV_VERSION
};

// Driver Capabilities
static const ARM_USART_CAPABILITIES DriverCapabilities = {
    1, /* supports UART (Asynchronous) mode */
    0, /* supports Synchronous Master mode */
    0, /* supports Synchronous Slave mode */
    0, /* supports UART Single-wire mode */
    0, /* supports UART IrDA mode */
    0, /* supports UART Smart Card mode */
    0, /* Smart Card Clock generator available */
    0, /* RTS Flow Control available */
    0, /* CTS Flow Control available */
    0, /* Transmit completed event: \ref ARM_USART_EVENT_TX_COMPLETE */
    0, /* Signal receive character timeout event: \ref ARM_USART_EVENT_RX_TIMEOUT */
    0, /* RTS Line: 0=not available, 1=available */
    0, /* CTS Line: 0=not available, 1=available */
    0, /* DTR Line: 0=not available, 1=available */
    0, /* DSR Line: 0=not available, 1=available */
    0, /* DCD Line: 0=not available, 1=available */
    0, /* RI Line: 0=not available, 1=available */
    0, /* Signal CTS change event: \ref ARM_USART_EVENT_CTS */
    0, /* Signal DSR change event: \ref ARM_USART_EVENT_DSR */
    0, /* Signal DCD change event: \ref ARM_USART_EVENT_DCD */
    0  /* Signal RI change event: \ref ARM_USART_EVENT_RI */
};

void LPUSART1_IRQHandler(void);
void CO_USART1_IRQHandler(void);

#if (RTE_UART1)

static LPUSART_INFO    LPUSART1_Info  = { 0 };
static const PIN LPUSART1_pin_tx  = {RTE_UART1_TX_BIT,   RTE_UART1_TX_FUNC};
static const PIN LPUSART1_pin_rx  = {RTE_UART1_RX_BIT,   RTE_UART1_RX_FUNC};

#if (RTE_UART1_TX_IO_MODE == DMA_MODE)

void LPUSART1_DmaTxEvent(uint32_t event);
static LPUSART_TX_DMA LPUSART1_DMA_Tx = {
                                    -1,
                                    RTE_UART1_DMA_TX_REQID,
                                    LPUSART1_DmaTxEvent
                                 };
#endif

#if (RTE_UART1_RX_IO_MODE == DMA_MODE)

void CO_USART1_DmaRxEvent(uint32_t event);

static dma_descriptor_t __ALIGNED(16) CO_USART1_DMA_Rx_Descriptor[2];

static LPUSART_RX_DMA CO_USART1_DMA_Rx = {
                                    -1,
                                    RTE_UART1_DMA_RX_REQID,
                                    CO_USART1_DMA_Rx_Descriptor,
                                    CO_USART1_DmaRxEvent
                                 };

static dma_descriptor_t __ALIGNED(16) LPUSART1_DMA_Rx_Descriptor;

static LPUSART_RX_DMA LPUSART1_DMA_Rx = {
                                    -1,
                                    DMA_RequestLPUARTRX,
                                    &LPUSART1_DMA_Rx_Descriptor,
                                    NULL
                                 };

#endif

#if (RTE_UART1_TX_IO_MODE == IRQ_MODE) || (RTE_UART1_RX_IO_MODE == IRQ_MODE) || (RTE_UART1_RX_IO_MODE == DMA_MODE)
static LPUSART_IRQ CO_USART1_IRQ = {
                                PXIC_Uart1_IRQn,
                                CO_USART1_IRQHandler
                              };

static LPUSART_IRQ LPUSART1_IRQ = {
                                PXIC_Lpuart_IRQn,
                                LPUSART1_IRQHandler
                              };
#endif

static const LPUSART_RESOURCES LPUSART1_Resources = {
    USART_1,
    LPUSART_AON,
    LPUSART_CORE,
    {
      &LPUSART1_pin_tx,
      &LPUSART1_pin_rx,
    },

#if (RTE_UART1_RX_IO_MODE == DMA_MODE)
    &LPUSART1_DMA_Rx,
#else
    NULL,
#endif

#if (RTE_UART1_TX_IO_MODE == DMA_MODE)
    &LPUSART1_DMA_Tx,
#else
    NULL,
#endif

#if (RTE_UART1_RX_IO_MODE == DMA_MODE)
    &CO_USART1_DMA_Rx,
#else
    NULL,
#endif

#if (RTE_UART1_TX_IO_MODE == IRQ_MODE) || (RTE_UART1_RX_IO_MODE == IRQ_MODE) || (RTE_UART1_RX_IO_MODE == DMA_MODE)
    &LPUSART1_IRQ,
    &CO_USART1_IRQ,
#else
    NULL,
    NULL,
#endif
    &LPUSART1_Info
};
#endif


static dma_transfer_config dmaTxConfig = {NULL, NULL,
                                       DMA_FlowControlTarget, DMA_AddressIncrementSource,
                                       DMA_DataWidthOneByte, DMA_Burst16Bytes, 0
                                      };

ARM_DRIVER_VERSION LPUSART_GetVersion(void)
{
    return DriverVersion;
}

ARM_USART_CAPABILITIES LPUSART_GetCapabilities(void)
{
    return DriverCapabilities;
}

/*
 * when uart input clock is 26000000, the supported baudrate is:
 * 4800,9600,14400,19200,
 * 28800,38400,56000,57600,
 * 115200,230400,460800,921600,
 * 1000000,1500000,2000000,3000000
 */
int32_t LPUSART_SetBaudrate(uint32_t baudrate, LPUSART_RESOURCES *lpusart)
{
    uint8_t frac = 0;
    uint32_t uart_clock = 0;
    uint32_t div;
    int32_t i;

    if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
    {
        const static uint16_t g_lpusartSupportedBaudRate[5] = {600, 1200, 2400, 4800, 9600};

        // timeout threshold is 24 bits, sample clock is 26M, threshold/26M = 24/baudrate
        const static uint32_t g_lpusartTimeoutValue[5] = {1040000, 520000, 260000, 130000, 65000};

        const static uint16_t g_lpusartDLRValue[5] = {1748, 874, 437, 218, 109};

        for(i = 0; i <= 4; i++)
        {
            if(baudrate == g_lpusartSupportedBaudRate[i])
            {
                if((lpusart->core_regs->FCSR & (LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk | LPUSARTCORE_FCSR_AON_RX_BUSY_Msk | LPUSARTCORE_FCSR_AON_RXFIFO_EMPTY_Msk)) == (LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk | LPUSARTCORE_FCSR_AON_RXFIFO_EMPTY_Msk))
                {
                    LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR0, LPUSARTAON_CR0_CLK_ENABLE_Msk);
                    LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->DLR, g_lpusartDLRValue[i]);
                }
                lpusart->core_regs->TCR |= LPUSARTCORE_TCR_TOCNT_SWCLR_Msk;
                lpusart->core_regs->TCR = g_lpusartTimeoutValue[i];
                lpusart->core_regs->TCR |= LPUSARTCORE_TCR_TOCNT_SWTRG_Msk;
                break;
            }
        }

        if(i > 5)
        {
            return ARM_DRIVER_ERROR_PARAMETER;
        }

    }

    if(baudrate == 0)
    {
        lpusart->co_usart_regs->ADCR = 0x3;
    }
    else
    {
        uart_clock = GPR_GetClockFreq(GPR_UART1FuncClk);

        /*
         * formula to calculate baudrate, baudrate = clock_in / (prescalar * divisor_value),
         * where prescalar = MFCR_PRESCALE_FACTOR(4,8,16), divisor_value = DLH:DLL.EFCR_FRAC
         */
        for(i = 0; i <= 2; i++)
        {
            div = (1 << i) * uart_clock / baudrate;
            frac = div & 0xf;
            div >>= 4;
            // Integer part of divisor value shall not be zero, otherwise, the result is invalid
            if (div != 0)
                break;
        }

        if(i > 2)
            return ARM_DRIVER_ERROR_PARAMETER;

        // Disable uart first
        lpusart->co_usart_regs->MFCR &= ~USART_MFCR_UART_EN_Msk;

        // Enable latch bit to change divisor
        lpusart->co_usart_regs->LCR |= USART_LCR_ACCESS_DIVISOR_LATCH_Msk;
        lpusart->co_usart_regs->MFCR = ((lpusart->co_usart_regs->MFCR & ~USART_MFCR_PRESCALE_FACTOR_Msk) | EIGEN_VAL2FLD(USART_MFCR_PRESCALE_FACTOR, i));
        lpusart->co_usart_regs->DLL = (div >> 0) & 0xff;
        lpusart->co_usart_regs->DLH = (div >> 8) & 0xff;
        lpusart->co_usart_regs->EFCR = ((lpusart->co_usart_regs->EFCR & ~USART_EFCR_FRAC_DIVISOR_Msk) | (frac << USART_EFCR_FRAC_DIVISOR_Pos));
        // Reset latch bit
        lpusart->co_usart_regs->LCR &= (~USART_LCR_ACCESS_DIVISOR_LATCH_Msk);

#ifdef PM_FEATURE_ENABLE
        // backup setting
        g_lpusartDataBase.co_usart_registers.DLL = (div >> 0) & 0xff;
        g_lpusartDataBase.co_usart_registers.DLH = (div >> 8) & 0xff;
#endif

    }

    lpusart->info->baudrate = baudrate;

    return ARM_DRIVER_OK;
}

uint32_t LPUSART_GetBaudRate(LPUSART_RESOURCES *lpusart)
{
    return lpusart->info->baudrate;
}

/*
 * Check whether rx is ongoing, return true if rx is ongoing at this moment, otherwise false
 */
bool LPUSART_IsRxActive(void)
{
#ifdef PM_FEATURE_ENABLE
    if(LPUSART1_Info.flags & LPUSART_FLAG_POWER_LOW)
    {
        return !((LPUSART_CORE->FCSR & (LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk | LPUSARTCORE_FCSR_AON_RX_BUSY_Msk | LPUSARTCORE_FCSR_AON_RXFIFO_EMPTY_Msk)) == (LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk | LPUSARTCORE_FCSR_AON_RXFIFO_EMPTY_Msk));
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

void LPUSART_ClearStopFlag(void)
{
#ifdef PM_FEATURE_ENABLE
    if(LPUSART1_Info.flags & LPUSART_FLAG_POWER_LOW)
    {
        LPUSART_AON->SCR = LPUSARTAON_SCR_STOP_SW_CLR_Msk;
        while(LPUSART_CORE->FCSR & LPUSARTCORE_FCSR_AON_STOP_FLAG_Msk);
        LPUSART_AON->SCR = 0;
    }
#endif
}

void LPUSART_SetStopFlag(void)
{
#ifdef PM_FEATURE_ENABLE
    if(LPUSART1_Info.flags & LPUSART_FLAG_POWER_LOW)
    {
        LPUSART_AON->SCR = LPUSARTAON_SCR_STOP_SW_SET_Msk;
        while((LPUSART_CORE->FCSR & LPUSARTCORE_FCSR_AON_STOP_FLAG_Msk) == 0);
        LPUSART_AON->SCR = 0;
    }
#endif
}

static void LPUSART_DMARxConfig(LPUSART_RESOURCES *lpusart, bool isLpuart)
{
    dma_transfer_config dmaConfig;
    dma_extra_config extraConfig;

    dmaConfig.addressIncrement              = DMA_AddressIncrementTarget;
    dmaConfig.dataWidth                     = DMA_DataWidthOneByte;
    dmaConfig.flowControl                   = DMA_FlowControlSource;
    dmaConfig.targetAddress                 = NULL;

    if(isLpuart == true)
    {
        dmaConfig.burstSize                     = DMA_Burst64Bytes;
        dmaConfig.sourceAddress                 = (void*)&(lpusart->core_regs->RBR);
        dmaConfig.targetAddress                 = NULL;
        dmaConfig.totalLength                   = LPUSART_RX_TRIG_LVL - 1;
        extraConfig.stopDecriptorFetch          = true;
        extraConfig.enableStartInterrupt        = false;
        extraConfig.enableEndInterrupt          = false;
        extraConfig.nextDesriptorAddress        = lpusart->dma_rx->descriptor;

        DMA_BuildDescriptor(lpusart->dma_rx->descriptor, &dmaConfig, &extraConfig);

        DMA_ResetChannel(lpusart->dma_rx->channel);

    }
    else
    {
        dmaConfig.burstSize                     = DMA_Burst8Bytes;
        dmaConfig.sourceAddress                 = (void*)&(lpusart->co_usart_regs->RBR);
        dmaConfig.totalLength                   = CO_USART_DMA_BURST_SIZE;

        extraConfig.stopDecriptorFetch          = false;
        extraConfig.enableStartInterrupt        = false;
        extraConfig.enableEndInterrupt          = true;
        extraConfig.nextDesriptorAddress        = &lpusart->co_usart_dma_rx->descriptor[1];

        DMA_BuildDescriptor(&lpusart->co_usart_dma_rx->descriptor[0], &dmaConfig, &extraConfig);

        extraConfig.stopDecriptorFetch          = true;
        extraConfig.nextDesriptorAddress        = &lpusart->co_usart_dma_rx->descriptor[0];

        DMA_BuildDescriptor(&lpusart->co_usart_dma_rx->descriptor[1], &dmaConfig, &extraConfig);

        DMA_ResetChannel(lpusart->co_usart_dma_rx->channel);

    }
}

PLAT_CODE_IN_RAM static void CO_USART_DmaUpdateRxConfig(LPUSART_RESOURCES *lpusart, uint32_t targetAddress, uint32_t num)
{
    lpusart->co_usart_dma_rx->descriptor[0].TAR = targetAddress;
    lpusart->co_usart_dma_rx->descriptor[1].TAR = lpusart->co_usart_dma_rx->descriptor[0].TAR + CO_USART_DMA_BURST_SIZE;
    lpusart->co_usart_dma_rx->descriptor[1].CMDR = DMA_SetDescriptorTransferLen(lpusart->co_usart_dma_rx->descriptor[1].CMDR, num - CO_USART_DMA_BURST_SIZE);
}


int32_t LPUSART_Initialize(ARM_USART_SignalEvent_t cb_event, LPUSART_RESOURCES *lpusart)
{
    int32_t returnCode;

    if (lpusart->info->flags & LPUSART_FLAG_INITIALIZED)
        return ARM_DRIVER_OK;

    // Pin initialize
    pad_config_t config;
    PAD_GetDefaultConfig(&config);

    config.mux = lpusart->pins.pin_tx->funcNum;
    PAD_SetPinConfig(lpusart->pins.pin_tx->pinNum, &config);

    config.pullSelect = PAD_PullInternal;
    config.pullUpEnable = PAD_PullUpEnable;
    config.pullDownEnable = PAD_PullDownDisable;
    config.mux = lpusart->pins.pin_rx->funcNum;
    PAD_SetPinConfig(lpusart->pins.pin_rx->pinNum, &config);

#ifdef PM_FEATURE_ENABLE
    g_lpusartDataBase.isInited = true;
#endif

    // Initialize LPUSART run-time resources
    lpusart->info->cb_event = cb_event;
    memset(&(lpusart->info->rx_status), 0, sizeof(LPUSART_STATUS));

    lpusart->info->xfer.send_active           = 0;
    lpusart->info->xfer.tx_def_val            = 0;

    if(lpusart->co_usart_dma_tx)
    {
        DMA_Init();

        returnCode = DMA_OpenChannel();

        if (returnCode == ARM_DMA_ERROR_CHANNEL_ALLOC)
            return ARM_DRIVER_ERROR;
        else
            lpusart->co_usart_dma_tx->channel = returnCode;

        DMA_ChannelSetRequestSource(lpusart->co_usart_dma_tx->channel, (dma_request_source_t)lpusart->co_usart_dma_tx->request);
        DMA_ChannelRigisterCallback(lpusart->co_usart_dma_tx->channel, lpusart->co_usart_dma_tx->callback);
    }

    if(lpusart->co_usart_dma_rx)
    {
        DMA_Init();

        returnCode = DMA_OpenChannel();

        if (returnCode == ARM_DMA_ERROR_CHANNEL_ALLOC)
            return ARM_DRIVER_ERROR;
        else
            lpusart->co_usart_dma_rx->channel = returnCode;

        DMA_ChannelSetRequestSource(lpusart->co_usart_dma_rx->channel, (dma_request_source_t)lpusart->co_usart_dma_rx->request);
        LPUSART_DMARxConfig(lpusart, false);
        DMA_ChannelRigisterCallback(lpusart->co_usart_dma_rx->channel, lpusart->co_usart_dma_rx->callback);
    }

    if(lpusart->dma_rx)
    {
        DMA_Init();

        returnCode = DMA_OpenChannel();

        if (returnCode == ARM_DMA_ERROR_CHANNEL_ALLOC)
            return ARM_DRIVER_ERROR;
        else
            lpusart->dma_rx->channel = returnCode;

        DMA_ChannelSetRequestSource(lpusart->dma_rx->channel, (dma_request_source_t)lpusart->dma_rx->request);
        LPUSART_DMARxConfig(lpusart, true);
    }

    lpusart->info->flags = LPUSART_FLAG_INITIALIZED;  // LPUSART is initialized

#ifdef PM_FEATURE_ENABLE
    g_lpusartWorkingStatus = 0;
    slpManRegisterPredefinedBackupCb(SLP_CALLBACK_LPUSART_MODULE, LPUSART_EnterLowPowerStatePrepare, NULL, SLPMAN_SLEEP1_STATE);
    slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_LPUSART_MODULE, LPUSART_ExitLowPowerStateRestore, NULL, SLPMAN_SLEEP1_STATE);
#endif
    return ARM_DRIVER_OK;
}

int32_t LPUSART_Uninitialize(LPUSART_RESOURCES *lpusart)
{
    lpusart->info->flags = 0;
    lpusart->info->cb_event = NULL;

    if(lpusart->co_usart_dma_tx)
    {
        DMA_CloseChannel(lpusart->co_usart_dma_tx->channel);
    }

    if(lpusart->co_usart_dma_rx)
    {
        DMA_CloseChannel(lpusart->co_usart_dma_rx->channel);
    }

    if(lpusart->dma_rx)
    {
        DMA_CloseChannel(lpusart->dma_rx->channel);
    }

#ifdef PM_FEATURE_ENABLE

    g_lpusartDataBase.isInited = false;
    g_lpusartWorkingStatus = 0;
    slpManUnregisterPredefinedBackupCb(SLP_CALLBACK_LPUSART_MODULE);
    slpManUnregisterPredefinedRestoreCb(SLP_CALLBACK_LPUSART_MODULE);
#endif

    return ARM_DRIVER_OK;
}

int32_t LPUSART_PowerControl(ARM_POWER_STATE state,LPUSART_RESOURCES *lpusart)
{
    uint32_t val = 0;

    switch (state)
    {
        case ARM_POWER_OFF:

            // Reset LPUSART
            GPR_SWReset(GPR_ResetUART1APB);
            GPR_SWReset(GPR_ResetUART1Func);

            if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
            {
                GPR_SWReset(GPR_ResetLPUARTAONFunc);

                LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR1, 0);

            }

            LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR0, 0);

            // DMA disable
            if(lpusart->co_usart_dma_tx)
                DMA_StopChannel(lpusart->co_usart_dma_tx->channel, false);

            if(lpusart->co_usart_dma_rx)
                DMA_StopChannel(lpusart->co_usart_dma_rx->channel, false);

            if(lpusart->dma_rx)
                DMA_StopChannel(lpusart->dma_rx->channel, false);

            // Disable clock
            GPR_ClockDisable(GPR_UART1APBClk);
            GPR_ClockDisable(GPR_UART1FuncClk);

            // Clear driver variables
            memset(&(lpusart->info->rx_status), 0, sizeof(LPUSART_STATUS));
            lpusart->info->mode             = 0;
            lpusart->info->xfer.send_active = 0;

            // Disable LPUSART IRQ
            if(lpusart->irq)
            {
                XIC_ClearPendingIRQ(lpusart->irq->irq_num);
                XIC_DisableIRQ(lpusart->irq->irq_num);
            }

            if(lpusart->co_usart_irq)
            {
                XIC_ClearPendingIRQ(lpusart->co_usart_irq->irq_num);
                XIC_DisableIRQ(lpusart->co_usart_irq->irq_num);
            }

            lpusart->info->flags &= ~(LPUSART_FLAG_POWER_MSK | LPUSART_FLAG_CONFIGURED);

            break;

        case ARM_POWER_LOW:
            if((lpusart->info->flags & LPUSART_FLAG_INITIALIZED) == 0)
            {
                return ARM_DRIVER_ERROR;
            }

            if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
            {
                return ARM_DRIVER_OK;
            }

            // Enable lpusart clock
            GPR_ClockEnable(GPR_AONFuncClk);
            GPR_ClockEnable(GPR_UART1APBClk);
            GPR_ClockEnable(GPR_UART1FuncClk);

            // Disable interrupts
            lpusart->co_usart_regs->IER = 0;
            lpusart->core_regs->IER = 0;

            // Clear driver variables
            memset(&(lpusart->info->rx_status), 0, sizeof(LPUSART_STATUS));
            lpusart->info->mode             = 0;
            lpusart->info->xfer.send_active = 0;

            // Configure FIFO Control register
            val = USART_FCR_FIFO_EN_Msk | USART_FCR_RESET_TX_FIFO_Msk;

            lpusart->co_usart_regs->FCR = val;

#ifdef PM_FEATURE_ENABLE
            g_lpusartDataBase.co_usart_registers.FCR = val;
#endif

            val = EIGEN_VAL2FLD(LPUSARTCORE_FCSR_RXFIFO_THRLD, LPUSART_RX_TRIG_LVL);
            //val |= LPUSARTCORE_FCSR_FLUSH_RXFIFO_Msk;
            lpusart->core_regs->FCSR = val;

            if(lpusart->co_usart_dma_tx)
            {
                lpusart->co_usart_regs->MFCR |= USART_MFCR_DMA_EN_Msk;
            }

            if(lpusart->dma_rx)
            {
                lpusart->core_regs->DCR = LPUSARTCORE_DCR_RX_REQ_EN_Msk;
            }

            if(lpusart->irq)
            {
                XIC_SetVector(lpusart->irq->irq_num, lpusart->irq->cb_irq);
                XIC_EnableIRQ(lpusart->irq->irq_num);
                XIC_SuppressOvfIRQ(lpusart->irq->irq_num);
            }
            if(lpusart->co_usart_irq)
            {
                XIC_SetVector(lpusart->co_usart_irq->irq_num, lpusart->co_usart_irq->cb_irq);
                XIC_EnableIRQ(lpusart->co_usart_irq->irq_num);
                XIC_SuppressOvfIRQ(lpusart->co_usart_irq->irq_num);
            }

            lpusart->info->flags |= LPUSART_FLAG_POWER_LOW;  // LPUSART is powered on

            break;

        case ARM_POWER_FULL:
            if((lpusart->info->flags & LPUSART_FLAG_INITIALIZED) == 0)
            {
                return ARM_DRIVER_ERROR;
            }
            if(lpusart->info->flags & LPUSART_FLAG_POWER_FULL)
            {
                return ARM_DRIVER_OK;
            }

            // Enable cooperating usart clock
            GPR_ClockEnable(GPR_AONFuncClk);
            GPR_ClockEnable(GPR_UART1APBClk);
            GPR_ClockEnable(GPR_UART1FuncClk);

            GPR_SWReset(GPR_ResetUART1APB);
            GPR_SWReset(GPR_ResetUART1Func);

            // Clear driver variables
            memset(&(lpusart->info->rx_status), 0, sizeof(LPUSART_STATUS));
            lpusart->info->mode             = 0;
            lpusart->info->xfer.send_active = 0;

            // Configure FIFO Control register
            val = USART_FCR_FIFO_EN_Msk | USART_FCR_RESET_RX_FIFO_Msk | USART_FCR_RESET_TX_FIFO_Msk;

            // rxfifo trigger level set as 16 bytes
            val |= (2U << USART_FCR_RX_FIFO_AVAIL_TRIG_LEVEL_Pos);

            lpusart->co_usart_regs->FCR = val;

#ifdef PM_FEATURE_ENABLE
            g_lpusartDataBase.co_usart_registers.FCR = val;
#endif

            if(lpusart->co_usart_dma_tx || lpusart->co_usart_dma_rx)
            {
                lpusart->co_usart_regs->MFCR |= USART_MFCR_DMA_EN_Msk;
            }

            if(lpusart->co_usart_irq)
            {
                XIC_SetVector(lpusart->co_usart_irq->irq_num, lpusart->co_usart_irq->cb_irq);
                XIC_EnableIRQ(lpusart->co_usart_irq->irq_num);
                XIC_SuppressOvfIRQ(lpusart->co_usart_irq->irq_num);
            }
            lpusart->info->flags |= LPUSART_FLAG_POWER_FULL;  // LPUSART is powered on

            // Enable wakeup feature only
            LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR0, LPUSARTAON_CR0_RX_ENABLE_Msk);

            break;

        default:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
    return ARM_DRIVER_OK;
}

int32_t LPUSART_Send(const void *data, uint32_t num, LPUSART_RESOURCES *lpusart)
{
    uint32_t mask;
    if ((data == NULL) || (num == 0))
        return ARM_DRIVER_ERROR_PARAMETER;
    if ((lpusart->info->flags & LPUSART_FLAG_CONFIGURED) == 0)
        return ARM_DRIVER_ERROR;

#ifdef PM_FEATURE_ENABLE
    if((lpusart->co_usart_regs->ADCR & USART_ADCR_AUTO_BAUD_INT_EN_Msk) && (g_lpusartDataBase.autoBaudRateDone == false))
    {
        return ARM_DRIVER_OK;
    }
#endif

    mask = SaveAndSetIRQMask();
    if (lpusart->info->xfer.send_active != 0)
    {
        RestoreIRQMask(mask);
        return ARM_DRIVER_ERROR_BUSY;
    }

    lpusart->info->xfer.send_active = 1U;
    RestoreIRQMask(mask);

    // Save transmit buffer info
    lpusart->info->xfer.tx_buf = (uint8_t *)data;
    lpusart->info->xfer.tx_num = num;
    lpusart->info->xfer.tx_cnt = 0;
    // DMA mode
    if(lpusart->co_usart_dma_tx)
    {
        // wait until tx is empty
        while((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0);

#ifdef PM_FEATURE_ENABLE
        mask = SaveAndSetIRQMask();

        LOCK_SLEEP(1, 0);

        RestoreIRQMask(mask);
#endif

        if(num == 1)
        {
            mask = SaveAndSetIRQMask();

            lpusart->co_usart_regs->IER |= USART_IER_TX_DATA_REQ_Msk;

            lpusart->co_usart_regs->THR = lpusart->info->xfer.tx_buf[0];

            RestoreIRQMask(mask);
        }
        else
        {
            dmaTxConfig.sourceAddress = (void*)data;
            dmaTxConfig.targetAddress = (void*)&(lpusart->co_usart_regs->THR);
            dmaTxConfig.totalLength = num - 1;

            // Configure tx DMA and start it
            DMA_TransferSetup(lpusart->co_usart_dma_tx->channel, &dmaTxConfig);
            DMA_EnableChannelInterrupts(lpusart->co_usart_dma_tx->channel, DMA_EndInterruptEnable);
            DMA_StartChannel(lpusart->co_usart_dma_tx->channel);
        }

    }
    else
    {
        while (lpusart->info->xfer.tx_cnt < lpusart->info->xfer.tx_num)
        {
            // wait until tx is empty
            while((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0);
            lpusart->co_usart_regs->THR = lpusart->info->xfer.tx_buf[lpusart->info->xfer.tx_cnt++];
        }
        while((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0);
        mask = SaveAndSetIRQMask();
        lpusart->info->xfer.send_active = 0;
        RestoreIRQMask(mask);
    }

    return ARM_DRIVER_OK;
}

int32_t LPUSART_SendPolling(const void *data, uint32_t num, LPUSART_RESOURCES *lpusart)
{
    uint32_t mask;
    if ((data == NULL) || (num == 0))
        return ARM_DRIVER_ERROR_PARAMETER;
    if ((lpusart->info->flags & LPUSART_FLAG_CONFIGURED) == 0)
        return ARM_DRIVER_ERROR;

#ifdef PM_FEATURE_ENABLE
    if((lpusart->co_usart_regs->ADCR & USART_ADCR_AUTO_BAUD_INT_EN_Msk) && (g_lpusartDataBase.autoBaudRateDone == false))
    {
        return ARM_DRIVER_OK;
    }
#endif

    mask = SaveAndSetIRQMask();
    if (lpusart->info->xfer.send_active != 0)
    {
        RestoreIRQMask(mask);
        return ARM_DRIVER_ERROR_BUSY;
    }

    lpusart->info->xfer.send_active = 1U;
    RestoreIRQMask(mask);

    // Save transmit buffer info
    lpusart->info->xfer.tx_buf = (uint8_t *)data;
    lpusart->info->xfer.tx_num = num;
    lpusart->info->xfer.tx_cnt = 0;

    while (lpusart->info->xfer.tx_cnt < lpusart->info->xfer.tx_num)
    {
        // wait until tx is empty
        while((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0);
        lpusart->co_usart_regs->THR = lpusart->info->xfer.tx_buf[lpusart->info->xfer.tx_cnt++];
    }
    while((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0);
    mask = SaveAndSetIRQMask();
    lpusart->info->xfer.send_active = 0;
    RestoreIRQMask(mask);

    return ARM_DRIVER_OK;
}

int32_t LPUSART_Receive(void *data, uint32_t num, LPUSART_RESOURCES *lpusart)
{
    uint32_t mask;

    if ((data == NULL) || num == 0)
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if ((lpusart->info->flags & LPUSART_FLAG_CONFIGURED) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    // check if receiver is busy
    if (lpusart->info->rx_status.rx_busy == 1U)
    {
        return ARM_DRIVER_ERROR_BUSY;
    }

    lpusart->info->rx_status.rx_busy = 1U;

    // save num of data to be received
    lpusart->info->xfer.rx_num = num;
    lpusart->info->xfer.rx_buf = (uint8_t *)data;
    lpusart->info->xfer.rx_cnt = 0;

    if(lpusart->dma_rx || lpusart->co_usart_dma_rx)
    {
       lpusart->info->rx_status.rx_dma_triggered = 0;
    }

    if(lpusart->info->flags & LPUSART_FLAG_POWER_FULL)
    {
        if(lpusart->co_usart_dma_rx)
        {
            lpusart->co_usart_regs->IER |= USART_IER_RX_TIMEOUT_Msk   | \
                                           USART_IER_RX_LINE_STATUS_Msk;

            // Enable DMA tansfer only if there is enough space for supplied buffer
            if(num >= CO_USART_DMA_BURST_SIZE)
            {
                CO_USART_DmaUpdateRxConfig(lpusart, (uint32_t)data, num);
                DMA_ChannelLoadDescriptorAndRun(lpusart->co_usart_dma_rx->channel, &lpusart->co_usart_dma_rx->descriptor[0]);
            }
        }
        else if(lpusart->co_usart_irq)
        {
            lpusart->co_usart_regs->IER |= USART_IER_RX_TIMEOUT_Msk   | \
                                           USART_IER_RX_DATA_REQ_Msk  | \
                                           USART_IER_RX_LINE_STATUS_Msk;
        }
        else
        {
            while(lpusart->info->xfer.rx_cnt < lpusart->info->xfer.rx_num)
            {
                //wait unitl receive data is ready
                while((lpusart->co_usart_regs->LSR & USART_LSR_RX_DATA_READY_Msk) == 0);
                //read data
                lpusart->info->xfer.rx_buf[lpusart->info->xfer.rx_cnt++] = lpusart->co_usart_regs->RBR;
            }
            lpusart->info->rx_status.rx_busy = 0;
        }
    }

    if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
    {
        if(lpusart->irq)
        {
#if LPUSART_DRIVER_DEBUG
            ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_recv_0, P_DEBUG, 4,"lpuart recv enter, iir: 0x%x, fcsr: 0x%x, tcr: 0x%x, tsr: 0x%x", lpusart->core_regs->IIR, lpusart->core_regs->FCSR, lpusart->core_regs->TCR, lpusart->core_regs->TSR);
#endif
            mask = SaveAndSetIRQMask();

            // refresh timeout counter when wakeup from low power state
            lpusart->core_regs->IIR |= LPUSARTCORE_IIR_CLR_Msk;
            lpusart->core_regs->TCR |= LPUSARTCORE_TCR_TOCNT_SWTRG_Msk;
            lpusart->core_regs->IER = LPUSARTCORE_IER_AON_RX_OVERRUN_Msk   | \
                                      LPUSARTCORE_IER_AON_RX_PARITY_Msk    | \
                                      LPUSARTCORE_IER_AON_RX_FRMERR_Msk    | \
                                      LPUSARTCORE_IER_RX_DATA_AVAIL_Msk    | \
                                      LPUSARTCORE_IER_RX_TIMEOUT_Msk       | \
                                      LPUSARTCORE_IER_RX_OVERRUN_Msk;

            RestoreIRQMask(mask);

#if LPUSART_DRIVER_DEBUG
            ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_recv_1, P_DEBUG, 4,"lpuart recv exit, iir: 0x%x, fcsr: 0x%x, tcr: 0x%x, tsr: 0x%x", lpusart->core_regs->IIR, lpusart->core_regs->FCSR, lpusart->core_regs->TCR, lpusart->core_regs->TSR);
#endif
        }
        else
        {
            while(lpusart->info->xfer.rx_cnt < lpusart->info->xfer.rx_num)
            {
                //wait unitl receive data is ready
                while(lpusart->core_regs->FCSR & LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk);
                //read data
                lpusart->info->xfer.rx_buf[lpusart->info->xfer.rx_cnt++] = lpusart->core_regs->RBR;
            }
            lpusart->info->rx_status.rx_busy = 0;
        }
    }

    return ARM_DRIVER_OK;
}

int32_t LPUSART_Transfer(const void *data_out, void *data_in, uint32_t num,LPUSART_RESOURCES *lpusart)
{
    //maybe used by command transfer
    return ARM_DRIVER_ERROR_UNSUPPORTED;
}

int32_t LPUSART_GetTxCount(LPUSART_RESOURCES *lpusart)
{
    uint32_t cnt;
    if (!(lpusart->info->flags & LPUSART_FLAG_CONFIGURED))
        return 0;
    if(lpusart->co_usart_dma_tx)
        cnt = DMA_ChannelGetCount(lpusart->co_usart_dma_tx->channel);
    else
        cnt = lpusart->info->xfer.tx_cnt;
    return cnt;
}

PLAT_CODE_IN_RAM int32_t LPUSART_GetRxCount(LPUSART_RESOURCES *lpusart)
{
    if (!(lpusart->info->flags & LPUSART_FLAG_CONFIGURED))
        return 0;
    return lpusart->info->xfer.rx_cnt;
}

int32_t LPUSART_Control(uint32_t control, uint32_t arg, LPUSART_RESOURCES *lpusart)
{
    uint32_t mode, val, mfcr;
    uint8_t aon_lcr = 0;
    uint8_t lcr = lpusart->co_usart_regs->LCR;

    if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
    {
        aon_lcr = lpusart->aon_regs->LCR;
    }

    switch (control & ARM_USART_CONTROL_Msk)
    {
        // Control TX
        case ARM_USART_CONTROL_TX:
            return ARM_DRIVER_OK;
        // Control RX
        case ARM_USART_CONTROL_RX:
            return ARM_DRIVER_OK;
        // Control break
        case ARM_USART_CONTROL_BREAK:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        // Abort Send
        case ARM_USART_ABORT_SEND:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        // Abort receive
        case ARM_USART_ABORT_RECEIVE:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        // Abort transfer
        case ARM_USART_ABORT_TRANSFER:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        case ARM_USART_MODE_ASYNCHRONOUS:
            mode = ARM_USART_MODE_ASYNCHRONOUS;
            break;
        // Flush TX fifo
        case ARM_USART_CONTROL_FLUSH_TX:

            if(lpusart->co_usart_regs->MFCR & USART_MFCR_AUTO_FLOW_CTS_EN_Msk)
            {
                while(((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0) && ((lpusart->co_usart_regs->MSR & USART_MSR_CTS_Msk) == USART_MSR_CTS_Msk));
            }
            else
            {
                while((lpusart->co_usart_regs->LSR & USART_LSR_TX_EMPTY_Msk) == 0);
            }
            return ARM_DRIVER_OK;

        case ARM_USART_CONTROL_PURGE_COMM:
            if(lpusart->info->flags & LPUSART_FLAG_POWER_FULL)
            {
                mfcr = lpusart->co_usart_regs->MFCR;
                lpusart->co_usart_regs->MFCR = 0;
                // reconfigure FIFO Control register
                val = USART_FCR_FIFO_EN_Msk | USART_FCR_RESET_RX_FIFO_Msk | USART_FCR_RESET_TX_FIFO_Msk;

                // rxfifo trigger level set as 16 bytes
                val |= (2U << USART_FCR_RX_FIFO_AVAIL_TRIG_LEVEL_Pos);

                lpusart->co_usart_regs->FCR = val;
                lpusart->co_usart_regs->MFCR = mfcr;
            }
            else if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
            {
                val = EIGEN_VAL2FLD(LPUSARTCORE_FCSR_RXFIFO_THRLD, LPUSART_RX_TRIG_LVL);
                val |= LPUSARTCORE_FCSR_FLUSH_RXFIFO_Msk;
                lpusart->core_regs->FCSR = val;
                GPR_SWReset(GPR_ResetLPUARTAONFunc);
                lpusart->core_regs->IIR = LPUSARTCORE_IIR_CLR_Msk;
            }
            return ARM_DRIVER_OK;

        default:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    switch (control & ARM_USART_DATA_BITS_Msk)
    {
        case ARM_USART_DATA_BITS_5:
            lcr &= ~USART_LCR_CHAR_LEN_Msk;
            aon_lcr &= ~LPUSARTAON_LCR_CHAR_LEN_Msk;
            break;
        case ARM_USART_DATA_BITS_6:
            lcr &= ~USART_LCR_CHAR_LEN_Msk;
            lcr |= 1U;
            aon_lcr &= ~LPUSARTAON_LCR_CHAR_LEN_Msk;
            aon_lcr |= 1U;
            break;
        case ARM_USART_DATA_BITS_7:
            lcr &= ~USART_LCR_CHAR_LEN_Msk;
            lcr |= 2U;
            aon_lcr &= ~LPUSARTAON_LCR_CHAR_LEN_Msk;
            aon_lcr |= 2U;
            break;
        case ARM_USART_DATA_BITS_8:
            lcr &= ~USART_LCR_CHAR_LEN_Msk;
            lcr |= 3U;
            aon_lcr &= ~LPUSARTAON_LCR_CHAR_LEN_Msk;
            aon_lcr |= 3U;
            break;
        default:
            return ARM_USART_ERROR_DATA_BITS;
    }

    // LPUSART Parity
    switch (control & ARM_USART_PARITY_Msk)
    {
        case ARM_USART_PARITY_NONE:
            lcr &= ~USART_LCR_PARITY_EN_Msk;
            aon_lcr &= ~LPUSARTAON_LCR_PARITY_EN_Msk;
            break;
        case ARM_USART_PARITY_EVEN:
            lcr |= (USART_LCR_PARITY_EN_Msk | USART_LCR_EVEN_PARITY_Msk);
            aon_lcr |= (LPUSARTAON_LCR_PARITY_EN_Msk | LPUSARTAON_LCR_EVEN_PARITY_Msk);
            break;
        case ARM_USART_PARITY_ODD:
            lcr |= USART_LCR_PARITY_EN_Msk;
            lcr &= ~USART_LCR_EVEN_PARITY_Msk;
            aon_lcr |= LPUSARTAON_LCR_PARITY_EN_Msk;
            aon_lcr &= ~LPUSARTAON_LCR_EVEN_PARITY_Msk;
            break;
        default:
            return (ARM_USART_ERROR_PARITY);
    }

    // LPUSART Stop bits
    switch (control & ARM_USART_STOP_BITS_Msk)
    {
        case ARM_USART_STOP_BITS_1:
            lcr &=~ USART_LCR_STOP_BIT_NUM_Msk;
            break;
        case ARM_USART_STOP_BITS_1_5:
            if ((control & ARM_USART_DATA_BITS_Msk) == ARM_USART_DATA_BITS_5)
            {
                lcr |= USART_LCR_STOP_BIT_NUM_Msk;
                break;
            }
            else
                return ARM_USART_ERROR_STOP_BITS;
        case ARM_USART_STOP_BITS_2:
            lcr |= USART_LCR_STOP_BIT_NUM_Msk;
            break;
        default:
            return ARM_USART_ERROR_STOP_BITS;
    }

    // LPUSART Flow Control
    switch (control & ARM_USART_FLOW_CONTROL_Msk)
    {
        case ARM_USART_FLOW_CONTROL_NONE:
        case ARM_USART_FLOW_CONTROL_RTS:
        case ARM_USART_FLOW_CONTROL_CTS:
            break;
        case ARM_USART_FLOW_CONTROL_RTS_CTS:
            lpusart->co_usart_regs->MFCR |= (USART_MFCR_AUTO_FLOW_CTS_EN_Msk);
            break;
    }
    // LPUSART Baudrate
    if(ARM_DRIVER_OK != LPUSART_SetBaudrate(arg, lpusart))
    {
        return ARM_USART_ERROR_BAUDRATE;
    }

    // Configuration is OK - Mode is valid
    lpusart->info->mode = mode;

    lpusart->co_usart_regs->LCR = lcr;

    if(lpusart->info->flags & LPUSART_FLAG_POWER_LOW)
    {

        if((lpusart->core_regs->FCSR & (LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk | LPUSARTCORE_FCSR_AON_RX_BUSY_Msk | LPUSARTCORE_FCSR_AON_RXFIFO_EMPTY_Msk)) == (LPUSARTCORE_FCSR_RXFIFO_EMPTY_Msk | LPUSARTCORE_FCSR_AON_RXFIFO_EMPTY_Msk))
        {
            LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR0, LPUSARTAON_CR0_RX_ENABLE_Msk | LPUSARTAON_CR0_CLK_ENABLE_Msk);
            LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->LCR, aon_lcr);
            //LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR1, LPUSARTAON_CR1_ENABLE_Msk | LPUSARTAON_CR1_ACG_EN_Msk | LPUSARTAON_CR1_AUTO_ADJ_Msk);
            LPUSART_AON_REGISTER_WRITE(lpusart->aon_regs->CR1, LPUSARTAON_CR1_ENABLE_Msk | LPUSARTAON_CR1_ACG_EN_Msk);
        }
        LPUSART_ClearStopFlag();
    }

    // lpusart enable
    lpusart->co_usart_regs->MFCR |= USART_MFCR_UART_EN_Msk;

    lpusart->info->flags |= LPUSART_FLAG_CONFIGURED;
    return ARM_DRIVER_OK;
}

ARM_USART_STATUS LPUSART_GetStatus(LPUSART_RESOURCES *lpusart)
{
    ARM_USART_STATUS status;

    status.tx_busy          = lpusart->info->xfer.send_active;
    status.rx_busy          = lpusart->info->rx_status.rx_busy;
    status.tx_underflow     = 0;
    status.rx_overflow      = lpusart->info->rx_status.rx_overflow;
    status.rx_break         = lpusart->info->rx_status.rx_break;
    status.rx_framing_error = lpusart->info->rx_status.rx_framing_error;
    status.rx_parity_error  = lpusart->info->rx_status.rx_parity_error;
    status.is_send_block    = (lpusart->co_usart_dma_tx == NULL);
    return status;
}

int32_t LPUSART_SetModemControl(ARM_USART_MODEM_CONTROL control, LPUSART_RESOURCES *lpusart)
{
    if((lpusart->info->flags & LPUSART_FLAG_CONFIGURED) == 0U)
    {
        // USART is not configured
       return ARM_DRIVER_ERROR;
    }

    if(control == ARM_USART_RTS_CLEAR)
    {
        lpusart->co_usart_regs->MCR &= ~USART_MCR_RTS_Msk;
    }
    if(control == ARM_USART_RTS_SET)
    {
        lpusart->co_usart_regs->MCR |= USART_MCR_RTS_Msk;
    }

    if(control == ARM_USART_DTR_CLEAR)
    {
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    if(control == ARM_USART_DTR_SET)
    {
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    return ARM_DRIVER_OK;

}

ARM_USART_MODEM_STATUS LPUSART_GetModemStatus(LPUSART_RESOURCES *lpusart)
{
    ARM_USART_MODEM_STATUS status = {0};

    if(lpusart->info->flags & LPUSART_FLAG_CONFIGURED)
    {
        status.cts = EIGEN_FLD2VAL(USART_MSR_CTS, lpusart->co_usart_regs->MSR);
    }

    return status;
}

void Lpuart_WakeupIntHandler(void)
{
    extern void pmuClearWIC(void);
    // WIC clear
    pmuClearWIC();
    NVIC_DisableIRQ(LpuartWakeup_IRQn);

#if LPUSART_DRIVER_DEBUG
    ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_wakeup_irq, P_SIG, 1, "LPUART->FCSR:0x%x", LPUSART_CORE->FCSR);
#endif
}

PLAT_CODE_IN_RAM void LPUSART_IRQHandler(LPUSART_RESOURCES *lpusart)
{
    uint32_t i, dma_rx_channel;
    uint32_t event = 0;
    uint32_t iir_reg;
    uint32_t current_cnt, total_cnt, left_to_recv, bytes_in_fifo;

#define LPUSARTAON_INT_MASK      (LPUSARTCORE_IIR_AON_RX_OVERRUN_Msk | LPUSARTCORE_IIR_AON_RX_PARITY_Msk | LPUSARTCORE_IIR_AON_RX_FRMERR_Msk)
#define LPUSARTCORE_INT_MASK     (LPUSARTCORE_IIR_RX_DATA_AVAIL_Msk | LPUSARTCORE_IIR_RX_TIMEOUT_Msk | LPUSARTCORE_IIR_RX_OVERRUN_Msk)

    LPUSART_INFO *info = lpusart->info;

    // Check interrupt source
#if LPUSART_DRIVER_DEBUG
    ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_irq_0, P_DEBUG, 4,"Enter lpuart irq, iir: 0x%x, fcsr: 0x%x, tcr: 0x%x, tsr: 0x%x", lpusart->core_regs->IIR, lpusart->core_regs->FCSR, lpusart->core_regs->TCR, lpusart->core_regs->TSR);
#endif

    iir_reg = lpusart->core_regs->IIR;

    if(iir_reg & LPUSARTAON_INT_MASK)
    {
        if(iir_reg & LPUSARTCORE_IIR_AON_RX_OVERRUN_Msk)
        {
            info->rx_status.rx_overflow = 1U;
            info->rx_status.aon_rx_overflow = 1U;
            event |= ARM_USART_EVENT_RX_OVERFLOW;
        }

        if(iir_reg & LPUSARTCORE_IIR_RX_OVERRUN_Msk)
        {
            info->rx_status.rx_overflow = 1U;
            event |= ARM_USART_EVENT_RX_OVERFLOW;
        }

        // Parity error
        if (iir_reg & LPUSARTCORE_IIR_AON_RX_PARITY_Msk)
        {
            info->rx_status.rx_parity_error= 1U;
            event |= ARM_USART_EVENT_RX_PARITY_ERROR;
        }

        // Framing error
        if (iir_reg & LPUSARTCORE_IIR_AON_RX_FRMERR_Msk)
        {
            info->rx_status.rx_framing_error= 1U;
            event |= ARM_USART_EVENT_RX_FRAMING_ERROR;
        }
    }
    else if(iir_reg & LPUSARTCORE_INT_MASK)
    {
        // clear interrupt flags
        lpusart->core_regs->IIR = LPUSARTCORE_IIR_CLR_Msk;

        if(iir_reg & LPUSARTCORE_IIR_RX_OVERRUN_Msk)
        {
            info->rx_status.rx_overflow = 1U;
            event |= ARM_USART_EVENT_RX_OVERFLOW;
        }
        else
        {
            if(iir_reg & LPUSARTCORE_IIR_RX_DATA_AVAIL_Msk)
            {
                // need to retrigger timeout counter if both interrupts have reached or timeout is approaching
                if((lpusart->core_regs->TSR & LPUSARTCORE_TSR_TOCNT_Msk) > ((lpusart->core_regs->TCR & LPUSARTCORE_TCR_TIMEOUT_THRLD_Msk) - 10000))
                {
                    lpusart->core_regs->TCR |= LPUSARTCORE_TCR_TOCNT_SWTRG_Msk;
                }

#ifdef PM_FEATURE_ENABLE
                LOCK_SLEEP(0, 1);
#endif
                info->rx_status.rx_busy = 1U;

                current_cnt = info->xfer.rx_cnt;
                total_cnt = info->xfer.rx_num;

                left_to_recv = total_cnt - current_cnt;

                bytes_in_fifo = EIGEN_FLD2VAL(LPUSARTCORE_FCSR_RXFIFO_NUM, lpusart->core_regs->FCSR);

                // leave at least one byte in fifo to trigger timeout interrupt
                i = bytes_in_fifo - 1;

                if(i == 0)
                    i = 1;

                i = MIN(i, left_to_recv);

                // DMA mode
                if(lpusart->dma_rx)
                {
                    // Let CPU transfer last chunk and all data when trigger level is set to be 1
                    if ((left_to_recv <= (bytes_in_fifo - 1)) || (LPUSART_RX_TRIG_LVL == 1))
                    {
#if LPUSART_DRIVER_DEBUG
                        ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_irq_3, P_DEBUG, 1,"CPU will transfer: %d", i);
#endif
                        while(i--)
                        {
                            info->xfer.rx_buf[current_cnt++] = lpusart->core_regs->RBR;
                        }

                    }
                    else
                    {
                        dma_rx_channel = lpusart->dma_rx->channel;
#if LPUSART_DRIVER_DEBUG
                        ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_irq_2, P_DEBUG, 1,"DMA will transfer: %d", i);
#endif
                        extern uint32_t DMA_SetDescriptorTransferLen(uint32_t dcmd, uint32_t len);

                        lpusart->dma_rx->descriptor->TAR = (uint32_t)(info->xfer.rx_buf + current_cnt);
                        lpusart->dma_rx->descriptor->CMDR = DMA_SetDescriptorTransferLen(lpusart->dma_rx->descriptor->CMDR, i);

                        // load descriptor and start DMA transfer
                        extern void DMA_ChannelLoadDescriptorAndRun(uint32_t channel, void* descriptorAddress);
                        DMA_ChannelLoadDescriptorAndRun(dma_rx_channel, lpusart->dma_rx->descriptor);

                        current_cnt += i;
                        info->rx_status.rx_dma_triggered = 1;
                    }

                }
                // IRQ mode
                else
                {
                    while(i--)
                    {
                        info->xfer.rx_buf[current_cnt++] = lpusart->core_regs->RBR;
                    }
                    // clear interrupt flags
                    lpusart->core_regs->IIR = LPUSARTCORE_IIR_CLR_Msk;

                }

                info->xfer.rx_cnt = current_cnt;

                if(current_cnt == total_cnt)
                {
                    // Clear RX busy flag and set receive transfer complete event
                    event |= ARM_USART_EVENT_RECEIVE_COMPLETE;

                    //Disable interrupt
                    lpusart->core_regs->IER = 0;

                    info->rx_status.rx_busy = 0;
                }

            }
            else if(iir_reg & LPUSARTCORE_IIR_RX_TIMEOUT_Msk)
            {
                bytes_in_fifo = EIGEN_FLD2VAL(LPUSARTCORE_FCSR_RXFIFO_NUM, lpusart->core_regs->FCSR);

                if((bytes_in_fifo > 0) && (lpusart->core_regs->TSR & LPUSARTCORE_TSR_TOCNT_REACH_Msk))
                {
#ifdef PM_FEATURE_ENABLE
                    LOCK_SLEEP(0, 1);
#endif
                    info->rx_status.rx_busy = 1U;

                    info->rx_status.rx_dma_triggered = 0;

                    current_cnt = info->xfer.rx_cnt;

                    total_cnt = info->xfer.rx_num;

                    left_to_recv = total_cnt - current_cnt;

                    i = MIN(bytes_in_fifo, left_to_recv);

#if LPUSART_DRIVER_DEBUG
                    ECOMM_TRACE(UNILOG_PLA_DRIVER, lpuart_irq_4, P_DEBUG, 1,"Time out, CPU will transfer: %d", i);
#endif

                    while(i--)
                    {
                        info->xfer.rx_buf[current_cnt++] = lpusart->core_regs->RBR;
                    }

                    info->xfer.rx_cnt = current_cnt;

                    // Check if required amount of data is received
                    if (current_cnt == total_cnt)
                    {
                        // Clear RX busy flag and set receive transfer complete event
                        event |= ARM_USART_EVENT_RECEIVE_COMPLETE;

                        //Disable interrupt
                        lpusart->core_regs->IER = 0;

                    }
                    else
                    {
                        event |= ARM_USART_EVENT_RX_TIMEOUT;
                    }

                    info->rx_status.rx_busy = 0;
                }

            }
        }
    }

    if ((info->cb_event != NULL) && (event != 0))
    {
        info->cb_event (event);

#ifdef PM_FEATURE_ENABLE
        CHECK_TO_UNLOCK_SLEEP(0, 1);
#endif

    }
}


PLAT_CODE_IN_RAM void CO_USART_IRQHandler(LPUSART_RESOURCES *lpusart)
{
    uint32_t i;
    uint32_t event = 0;
    uint32_t iir_reg, lsr_reg;
    uint32_t current_cnt, total_cnt, left_to_recv, bytes_in_fifo;

#define CO_USART_TFE_INT          (0x2 << USART_IIR_INT_ID_Pos)
#define CO_USART_RDA_INT          (0x4 << USART_IIR_INT_ID_Pos)
#define CO_USART_RLS_INT          (0x6 << USART_IIR_INT_ID_Pos)
#define CO_USART_CTI_INT          (0xC << USART_IIR_INT_ID_Pos)
#define CO_USART_INT_TYPE_MSK     (0xF << USART_IIR_INT_ID_Pos)

    LPUSART_INFO *info = lpusart->info;

    // Check interrupt source
    iir_reg = lpusart->co_usart_regs->IIR;
    if ((iir_reg & USART_IIR_INT_PENDING_Msk) == 0)
    {
        if ((iir_reg & CO_USART_INT_TYPE_MSK) == CO_USART_RLS_INT)
        {
            lsr_reg = lpusart->co_usart_regs->LSR;

            if (lsr_reg & USART_LSR_RX_OVERRUN_ERROR_Msk)
            {
                info->rx_status.rx_overflow = 1U;
                event |= ARM_USART_EVENT_RX_OVERFLOW;
            }
            else if (lsr_reg & USART_LSR_RX_FIFO_ERROR_Msk)
            {
                // Parity error
                if (lsr_reg & USART_LSR_RX_PARITY_ERROR_Msk)
                {
                    info->rx_status.rx_parity_error= 1U;
                    event |= ARM_USART_EVENT_RX_PARITY_ERROR;
                }
                // Break detected
                if (lsr_reg & USART_LSR_RX_BREAK_Msk)
                {
                    info->rx_status.rx_break= 1U;
                    event |= ARM_USART_EVENT_RX_BREAK;
                }
                // Framing error
                if (lsr_reg & USART_LSR_RX_FRAME_ERROR_Msk)
                {
                    info->rx_status.rx_framing_error= 1U;
                    event |= ARM_USART_EVENT_RX_FRAMING_ERROR;
                }
            }

            info->rx_status.rx_busy = 0;

        }
        else if((iir_reg & CO_USART_INT_TYPE_MSK) == CO_USART_RDA_INT)
        {
#ifdef PM_FEATURE_ENABLE
            LOCK_SLEEP(0, 1);
#endif
            info->rx_status.rx_busy = 1U;

            current_cnt = info->xfer.rx_cnt;
            total_cnt = info->xfer.rx_num;

            left_to_recv = total_cnt - current_cnt;

            bytes_in_fifo = lpusart->co_usart_regs->FCNR >> USART_FCNR_RX_FIFO_NUM_Pos;

            // leave at least one byte in fifo to trigger timeout interrupt
            i = bytes_in_fifo - 1;

            if(i == 0)
                i = 1;

            i = MIN(i, left_to_recv);

            while(i--)
            {
                info->xfer.rx_buf[current_cnt++] = lpusart->co_usart_regs->RBR;
            }

            info->xfer.rx_cnt = current_cnt;

            if(current_cnt == total_cnt)
            {
                // Clear RX busy flag and set receive transfer complete event
                event |= ARM_USART_EVENT_RECEIVE_COMPLETE;

                //Disable RDA interrupt
                lpusart->co_usart_regs->IER &= ~(USART_IER_RX_DATA_REQ_Msk | USART_IER_RX_TIMEOUT_Msk | USART_IER_RX_LINE_STATUS_Msk);

                info->rx_status.rx_busy = 0;
            }
        }
        else if((iir_reg & CO_USART_INT_TYPE_MSK) == CO_USART_CTI_INT)
        {
#ifdef PM_FEATURE_ENABLE
            LOCK_SLEEP(0, 1);
#endif
            {
                info->rx_status.rx_busy = 1U;

                current_cnt = info->xfer.rx_cnt;

                if(lpusart->co_usart_dma_rx)
                {
                    if(info->rx_status.rx_dma_triggered)
                    {
                        // Sync with undergoing DMA transfer, wait until DMA burst transfer(8 bytes) done and update current_cnt
                        do
                        {
                            current_cnt = DMA_ChannelGetCurrentTargetAddress(lpusart->co_usart_dma_rx->channel, true) - (uint32_t)info->xfer.rx_buf;
#if LPUSART_DRIVER_DEBUG
                            ECOMM_TRACE(UNILOG_PLA_DRIVER, CO_USART_IRQHandler_1, P_WARNING, 1, "dma transfer done, cnt:%d", current_cnt);
#endif
                        } while(((current_cnt - info->xfer.rx_cnt) & (CO_USART_DMA_BURST_SIZE - 1)) != 0);

                    }
                    /*
                       No matter DMA transfer is started or not(left recv buffer space is not enough),
                       now we can stop DMA saftely for next transfer and handle tailing bytes in FIFO
                    */
                    DMA_StopChannelNoWait(lpusart->co_usart_dma_rx->channel);
                }

                total_cnt = info->xfer.rx_num;

                bytes_in_fifo = lpusart->co_usart_regs->FCNR >> USART_FCNR_RX_FIFO_NUM_Pos;

                left_to_recv = total_cnt - current_cnt;

                i = MIN(bytes_in_fifo, left_to_recv);

                // if still have space to recv
                if(left_to_recv > 0)
                {
                    while(i--)
                    {
                        info->xfer.rx_buf[current_cnt++] = lpusart->co_usart_regs->RBR;
                    }
                }

                info->xfer.rx_cnt = current_cnt;

                // Check if required amount of data is received
                if (current_cnt == total_cnt)
                {
                    // Clear RX busy flag and set receive transfer complete event
                    event |= ARM_USART_EVENT_RECEIVE_COMPLETE;

                    //Disable RDA interrupt
                    lpusart->co_usart_regs->IER &= ~(USART_IER_RX_DATA_REQ_Msk | USART_IER_RX_TIMEOUT_Msk | USART_IER_RX_LINE_STATUS_Msk);

                }
                else
                {
                    event |= ARM_USART_EVENT_RX_TIMEOUT;

                    if(lpusart->co_usart_dma_rx)
                    {
                        // Prepare for next recv
                        left_to_recv = total_cnt - info->xfer.rx_cnt;

                        // shall not start DMA transfer, next recv event would be timeout or overflow
                        if(left_to_recv >= CO_USART_DMA_BURST_SIZE)
                        {
                            info->rx_status.rx_dma_triggered = 0;

                            CO_USART_DmaUpdateRxConfig(lpusart, (uint32_t)info->xfer.rx_buf + info->xfer.rx_cnt, left_to_recv);

                            // load descriptor and start DMA transfer
                            DMA_ChannelLoadDescriptorAndRun(lpusart->co_usart_dma_rx->channel, &lpusart->co_usart_dma_rx->descriptor[0]);
                        }

                    }

                }

                info->rx_status.rx_busy = 0;
            }
        }

        if(((iir_reg & CO_USART_INT_TYPE_MSK) == CO_USART_TFE_INT) || \
            ((lpusart->co_usart_regs->IER & USART_IER_TX_DATA_REQ_Msk) && ((lpusart->co_usart_regs->FCNR & USART_FCNR_TX_FIFO_NUM_Msk) == 0)))
        {
            info->xfer.tx_cnt = info->xfer.tx_num;
            info->xfer.send_active = 0U;
            event |= ARM_USART_EVENT_SEND_COMPLETE;
            lpusart->co_usart_regs->IER &= ~USART_IER_TX_DATA_REQ_Msk;

#ifdef PM_FEATURE_ENABLE
            CHECK_TO_UNLOCK_SLEEP(1, 0);
#endif
        }
    }

    if ((info->cb_event != NULL) && (event != 0))
    {
        info->cb_event (event);

#ifdef PM_FEATURE_ENABLE
        CHECK_TO_UNLOCK_SLEEP(0, 1);
#endif

    }
}

/**
  \fn          void LPUSART_DmaTxEvent(uint32_t event, LPUSART_RESOURCES *usart)
  \brief       LPUSART DMA Tx Event handler.
  \param[in]   event DMA Tx Event
  \param[in]   usart   Pointer to LPUSART resources
*/
void LPUSART_DmaTxEvent(uint32_t event, LPUSART_RESOURCES *lpusart)
{
    switch (event)
    {
        case DMA_EVENT_END:
            // TXFIFO may still have data not sent out
            lpusart->co_usart_regs->IER |= USART_IER_TX_DATA_REQ_Msk;
            lpusart->co_usart_regs->THR = lpusart->info->xfer.tx_buf[lpusart->info->xfer.tx_num-1];

            break;
        case DMA_EVENT_ERROR:
        default:
            break;
    }
}

void CO_USART_DmaRxEvent(uint32_t event, LPUSART_RESOURCES *lpusart)
{

    uint32_t dmaCurrentTargetAddress = DMA_ChannelGetCurrentTargetAddress(lpusart->co_usart_dma_rx->channel, false);

    switch (event)
    {
        case DMA_EVENT_END:

#if LPUSART_DRIVER_DEBUG
            ECOMM_TRACE(UNILOG_PLA_DRIVER, CO_USART_DmaRxEvent_0, P_SIG, 2, "uart dma rx event, fcnr:%x, cnt:%d", lpusart->co_usart_regs->FCNR, dmaCurrentTargetAddress - (uint32_t)lpusart->info->xfer.rx_buf);
#endif

#ifdef PM_FEATURE_ENABLE
            LOCK_SLEEP(0, 1);
#endif
            lpusart->info->rx_status.rx_busy = 1U;
            lpusart->info->rx_status.rx_dma_triggered = 1;

            if(dmaCurrentTargetAddress == ( (uint32_t)lpusart->info->xfer.rx_buf + lpusart->info->xfer.rx_num))
            {
#if LPUSART_DRIVER_DEBUG
                ECOMM_TRACE(UNILOG_PLA_DRIVER, CO_USART_DmaRxEvent_1, P_SIG, 0, "uart dma rx complete");
#endif
                //Disable all recv interrupt
                lpusart->co_usart_regs->IER &= ~(USART_IER_RX_DATA_REQ_Msk | USART_IER_RX_TIMEOUT_Msk | USART_IER_RX_LINE_STATUS_Msk);
                lpusart->info->rx_status.rx_busy = 0;

                if(lpusart->info->cb_event)
                {
                    lpusart->info->cb_event(ARM_USART_EVENT_RECEIVE_COMPLETE);
                }

#ifdef PM_FEATURE_ENABLE
                CHECK_TO_UNLOCK_SLEEP(0, 1);
#endif
            }

            break;
        case DMA_EVENT_ERROR:
        default:
            break;
    }
}
#if (RTE_UART1)
static int32_t                    LPUSART1_Initialize      (ARM_USART_SignalEvent_t cb_event)                    { return LPUSART_Initialize(cb_event, &LPUSART1_Resources); }
static int32_t                    LPUSART1_Uninitialize    (void)                                                { return LPUSART_Uninitialize(&LPUSART1_Resources); }
static int32_t                    LPUSART1_PowerControl    (ARM_POWER_STATE state)                               { return LPUSART_PowerControl(state, &LPUSART1_Resources); }
static int32_t                    LPUSART1_Send            (const void *data, uint32_t num)                      { return LPUSART_Send(data, num, &LPUSART1_Resources); }
static int32_t                    LPUSART1_Receive         (void *data, uint32_t num)                            { return LPUSART_Receive(data, num, &LPUSART1_Resources); }
static int32_t                    LPUSART1_Transfer        (const void *data_out, void *data_in, uint32_t num)   { return LPUSART_Transfer(data_out, data_in, num, &LPUSART1_Resources); }
static int32_t                    LPUSART1_SendPolling     (const void *data, uint32_t num)                      { return LPUSART_SendPolling (data, num, &LPUSART1_Resources); }
static uint32_t                   LPUSART1_GetTxCount      (void)                                                { return LPUSART_GetTxCount(&LPUSART1_Resources); }
PLAT_CODE_IN_RAM static uint32_t  LPUSART1_GetRxCount      (void)                                                { return LPUSART_GetRxCount(&LPUSART1_Resources); }
static uint32_t                   LPUSART1_GetBaudRate      (void)                                                 { return LPUSART_GetBaudRate(&LPUSART1_Resources); }
static int32_t                    LPUSART1_Control         (uint32_t control, uint32_t arg)                      { return LPUSART_Control(control, arg, &LPUSART1_Resources); }
static ARM_USART_STATUS           LPUSART1_GetStatus       (void)                                                { return LPUSART_GetStatus(&LPUSART1_Resources); }
static int32_t                    LPUSART1_SetModemControl (ARM_USART_MODEM_CONTROL control)                     { return LPUSART_SetModemControl(control, &LPUSART1_Resources); }
static ARM_USART_MODEM_STATUS     LPUSART1_GetModemStatus  (void)                                                { return LPUSART_GetModemStatus(&LPUSART1_Resources); }
PLAT_CODE_IN_RAM       void       LPUSART1_IRQHandler      (void)                                                {        LPUSART_IRQHandler(&LPUSART1_Resources); }
PLAT_CODE_IN_RAM       void       CO_USART1_IRQHandler      (void)                                               {        CO_USART_IRQHandler(&LPUSART1_Resources); }

#if (RTE_UART1_TX_IO_MODE == DMA_MODE)
void                              LPUSART1_DmaTxEvent(uint32_t event)                                            { LPUSART_DmaTxEvent(event, &LPUSART1_Resources);}
#endif
#if (RTE_UART1_RX_IO_MODE == DMA_MODE)
void                              CO_USART1_DmaRxEvent(uint32_t event)                                           { CO_USART_DmaRxEvent(event, &LPUSART1_Resources);};
#endif

ARM_DRIVER_USART Driver_LPUSART1 = {
    LPUSART_GetVersion,
    LPUSART_GetCapabilities,
    LPUSART1_Initialize,
    LPUSART1_Uninitialize,
    LPUSART1_PowerControl,
    LPUSART1_Send,
    LPUSART1_Receive,
    LPUSART1_Transfer,
    LPUSART1_GetTxCount,
    LPUSART1_GetRxCount,
    LPUSART1_Control,
    LPUSART1_GetStatus,
    LPUSART1_SetModemControl,
    LPUSART1_GetModemStatus,
    LPUSART1_GetBaudRate,
    LPUSART1_SendPolling
};

#endif

//#pragma pop

