/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    adc_ec616s.c
 * Description:  EC616S adc driver source file
 * History:      Rev1.0   2019-03-18
 *               Rev1.1   2020-07-10    Adjust channel bitmap and support polling conversion feature
 *
 ****************************************************************************/

#include "adc_ec616s.h"
#include "ic_ec616s.h"
#include "slpman_ec616s.h"

#define LDO_ADC_CTRL_REG_ADDR           (0x44000020)
#define LDO_AIO_CTRL_REG_ADDR           (0x44000024)

#define RFDIG_CLOCK_ENABLE_REG_ADDR     (0x44050010)

#define ADC_Enable()                    do                                                               \
                                        {                                                                \
                                            *(uint32_t*)(RFDIG_CLOCK_ENABLE_REG_ADDR) |= ( 1 << 10);     \
                                            *(uint32_t*)(LDO_ADC_CTRL_REG_ADDR) = 1;                     \
                                            while((*(uint32_t*)(LDO_ADC_CTRL_REG_ADDR)) == 0);           \
                                            *(uint32_t*)(LDO_AIO_CTRL_REG_ADDR) = 1;                     \
                                            while((*(uint32_t*)(LDO_AIO_CTRL_REG_ADDR)) == 0);           \
                                        } while(0)

#define ADC_Disable()                   do                                                               \
                                        {                                                                \
                                            *(uint32_t*)(LDO_ADC_CTRL_REG_ADDR) = 0;                     \
                                            while((*(uint32_t*)(LDO_ADC_CTRL_REG_ADDR)) == 1);           \
                                            *(uint32_t*)(LDO_AIO_CTRL_REG_ADDR) = 0;                     \
                                            while((*(uint32_t*)(LDO_AIO_CTRL_REG_ADDR)) == 1);           \
                                        } while(0)

#define ADC_CHANNEL_NUMBER              (6)

#ifdef PM_FEATURE_ENABLE
#define  LOCK_SLEEP()                   do                                                                  \
                                        {                                                                   \
                                            slpManDrvVoteSleep(SLP_VOTE_ADC, SLP_ACTIVE_STATE);             \
                                        }                                                                   \
                                        while(0)

#define  UNLOCK_SLEEP()                 do                                                                  \
                                        {                                                                   \
                                             slpManDrvVoteSleep(SLP_VOTE_ADC, SLP_SLP1_STATE);              \
                                        }                                                                   \
                                        while(0)
#endif

#define ADC_BASE_CHANNEL_ENABLE_BIT_POSITION  (ADC_AIOCFG_THM_EN_Pos)

/**
    \brief size of ADC request queue, value shall be integer power of 2 for fast calcualtion of MOD.
    \note The valid number of requests is (this_macro_value - 1)
 */
#define ADC_REQUEST_QUEUE_SIZE          (8)
#define ADC_REQUEST_QUEUE_SIZE_MASK     (ADC_REQUEST_QUEUE_SIZE - 1)

/**
   \brief ADC conversion request queue typedef
 */
typedef struct _adc_request_queue
{
    uint32_t requestArray[ADC_REQUEST_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
} adc_reqeust_queue_t;

static adc_reqeust_queue_t g_adcRequestqueue;

/**
   \brief variable for keeping track the index of current request bitmap, see below bitmap layout
 */
static volatile uint32_t g_currentRequestIndex;

/** \brief Internal used data structure */
typedef struct _adc_channel_database
{
    struct
    {
        uint32_t AIOCFG;                                           /**< AIO configuration Register */
        uint32_t CFG;                                              /**< ADC configuration Register */
    } config_registers;

    adc_callback_t                  eventCallback;                 /**< Callback function passed in by application */
} adc_channel_database_t;

/**
 ****************************** Bitmap layout *****************************
 *
 *  23 22  21  20  19  18  17  16   15          -               8   7               3           0
 *+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *|  AIO1 |  AIO2 |  AIO3 |  AIO4 |          CH Vbat              |          CH thermal           |
 *+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *                        /        \                  /           \                   /           \
 *                       /          \                /             \                 /             \
 *                      /            \              /               \               /               \
 *                      |  APP |  PLAT|             |APP | PLAT | PHY|              |APP | PLAT | PHY|
 */
#define CHANNEL_ID_TO_INDEX(channel, userID)  ((channel < ADC_ChannelAio4) ? ((channel << 3) + userID) : (((channel - ADC_ChannelAio4) << 1) + (userID - ADC_UserPLAT) + 16 ))

#define ADC_MAX_LOGIC_CHANNELS           (24)

/** \brief Internal used data structure */
typedef struct _adc_database
{
    uint32_t channelConfigValidBitMap;                                           /**< Bitmap of channel configuration valid flag */
    adc_channel_database_t channelDataBase[ADC_MAX_LOGIC_CHANNELS];              /**< Array of channel database, each element represents one configuration */
} adc_database_t;

static adc_database_t g_adcDataBase = {0};
void delay_us(uint32_t us);

static void ADC_RequestQueueInit(void)
{
    g_adcRequestqueue.head = 0;
    g_adcRequestqueue.tail = 0;
    g_currentRequestIndex = ADC_MAX_LOGIC_CHANNELS;
}

// no need to perform atomic access since this function is called only in ISR
static int32_t ADC_RequestQueueRead(uint32_t *request)
{
    if(g_adcRequestqueue.tail  == g_adcRequestqueue.head)
    {
        // queue is empty
        return -1;
    }

    *request = g_adcRequestqueue.requestArray[g_adcRequestqueue.head];
    g_adcRequestqueue.head = (g_adcRequestqueue.head + 1) & ADC_REQUEST_QUEUE_SIZE_MASK;

    return 0;

}

static int32_t ADC_RequestQueueWrite(uint32_t *request)
{
    uint32_t mask;

    mask = SaveAndSetIRQMask();
    if(((g_adcRequestqueue.tail + 1) & ADC_REQUEST_QUEUE_SIZE_MASK) == g_adcRequestqueue.head)
    {
        // queue is full
        RestoreIRQMask(mask);
        return -1;
    }

    g_adcRequestqueue.requestArray[g_adcRequestqueue.tail] = *request;
    g_adcRequestqueue.tail = (g_adcRequestqueue.tail + 1) & ADC_REQUEST_QUEUE_SIZE_MASK;

    RestoreIRQMask(mask);

    return 0;
}

static void ADC_LoadConfigAndStartConversion(uint32_t requestIndex)
{
    uint32_t aioCfg = g_adcDataBase.channelDataBase[requestIndex].config_registers.AIOCFG;

    // clear status first
    ADC->CTRL = 0;

    // special handle for AIO1
    if(aioCfg & ADC_AIOCFG_AIO1_EN_Msk)
    {
        ADC->AIOCFG = aioCfg & (~ADC_AIOCFG_AIO1_EN_Msk);
        ADC->ECFG |= ADC_ECFG_AIO1EN_Msk;
        ADC->CFG = g_adcDataBase.channelDataBase[requestIndex].config_registers.CFG;
    }
    else
    {
        ADC->ECFG &= ~ADC_ECFG_AIO1EN_Msk;
        ADC->AIOCFG = aioCfg;
        ADC->CFG = g_adcDataBase.channelDataBase[requestIndex].config_registers.CFG;
    }

    // start conversion
    ADC->CTRL = ADC_CTRL_EN_Msk;
    delay_us(5);
    ADC->CTRL = (ADC_CTRL_EN_Msk | ADC_CTRL_RSTN_Msk);
}


static void ADC_InterruptHandler(void)
{
    uint32_t requestIndex;

    int32_t ret;

    if(g_adcDataBase.channelDataBase[g_currentRequestIndex].eventCallback != NULL)
    {
        g_adcDataBase.channelDataBase[g_currentRequestIndex].eventCallback(ADC->RESULT);
    }

    ret = ADC_RequestQueueRead(&requestIndex);

    // Start next round conversion
    if(ret == 0)
    {
        // there is pending request
        g_currentRequestIndex = requestIndex;

        ADC_LoadConfigAndStartConversion(g_currentRequestIndex);
    }
    else
    {
        g_currentRequestIndex = ADC_MAX_LOGIC_CHANNELS;

#ifdef PM_FEATURE_ENABLE
        UNLOCK_SLEEP();
#endif
    }
}

void ADC_GetDefaultConfig(adc_config_t *config)
{
    ASSERT(config);

    config->clockSource = ADC_ClockSourceDCXO;
    config->clockDivider = ADC_ClockDiv4;
    config->channelConfig.aioResDiv = ADC_AioResDivRatio1;
}

void ADC_ChannelInit(adc_channel_t channel, adc_user_t userID, const adc_config_t *config, adc_callback_t callback)
{
    uint32_t mask, configValue, requestIndex;

    configValue = 0;

    requestIndex = CHANNEL_ID_TO_INDEX(channel, userID);

    ASSERT(requestIndex < ADC_MAX_LOGIC_CHANNELS);

    mask = SaveAndSetIRQMask();
    // First Initialization
    if(g_adcDataBase.channelConfigValidBitMap == 0)
    {
        // enable ADC
        ADC_Enable();
        XIC_SetVector(PXIC_Adc_IRQn, ADC_InterruptHandler);
        XIC_EnableIRQ(PXIC_Adc_IRQn);
        ADC_RequestQueueInit();
    }
    g_adcDataBase.channelConfigValidBitMap |= ( 1 << requestIndex);
    RestoreIRQMask(mask);

    configValue |= ( EIGEN_VAL2FLD(ADC_CFG_IBIAS_SEL, 1) | \
                     EIGEN_VAL2FLD(ADC_CFG_WAIT_CTRL, 1) | \
                     EIGEN_VAL2FLD(ADC_CFG_OFFSET_CTRL, 4) | \
                     EIGEN_VAL2FLD(ADC_CFG_SAMPLE_AVG, 0) | \
                     EIGEN_VAL2FLD(ADC_CFG_VREF_BS, 1) | \
                     EIGEN_VAL2FLD(ADC_CFG_VREF_SEL, 4) | \
                     EIGEN_VAL2FLD(ADC_CFG_CLKIN_DIV, config->clockDivider) | \
                     EIGEN_VAL2FLD(ADC_CFG_CLK_DRIVE, 4) | \
                     EIGEN_VAL2FLD(ADC_CFG_CLK_SEL, config->clockSource)
                );
    g_adcDataBase.channelDataBase[requestIndex].config_registers.CFG = configValue;

    configValue = 0;

    switch(channel)
    {
        case ADC_ChannelThermal:
             configValue = (1 << (channel + ADC_BASE_CHANNEL_ENABLE_BIT_POSITION)) | EIGEN_VAL2FLD(ADC_AIOCFG_THM_SEL, config->channelConfig.thermalInput);
             break;

        case ADC_ChannelVbat:
             configValue = (1 << (channel + ADC_BASE_CHANNEL_ENABLE_BIT_POSITION)) | EIGEN_VAL2FLD(ADC_AIOCFG_VBATSEN_RDIV, config->channelConfig.vbatResDiv);
             break;

        case ADC_ChannelAio1:
        case ADC_ChannelAio2:
        case ADC_ChannelAio3:
        case ADC_ChannelAio4:

             if(config->channelConfig.aioResDiv < ADC_AioResDivRatio8Over16)
             {
                configValue = (1 << (channel + ADC_BASE_CHANNEL_ENABLE_BIT_POSITION)) | ADC_AIOCFG_RDIV_BYP_Msk | EIGEN_VAL2FLD(ADC_AIOCFG_RDIV, config->channelConfig.aioResDiv);
             }
             else
             {
                configValue = (1 << (channel + ADC_BASE_CHANNEL_ENABLE_BIT_POSITION)) | (EIGEN_VAL2FLD(ADC_AIOCFG_RDIV, (config->channelConfig.aioResDiv - ADC_AioResDivRatio8Over16)));
             }
             break;
        default:
             break;
    }

    g_adcDataBase.channelDataBase[requestIndex].config_registers.AIOCFG = configValue;
    g_adcDataBase.channelDataBase[requestIndex].eventCallback = callback;
}

void ADC_ChannelDeInit(adc_channel_t channel, adc_user_t userID)
{
    uint32_t mask = SaveAndSetIRQMask();

    g_adcDataBase.channelConfigValidBitMap &= ~(1 << CHANNEL_ID_TO_INDEX(channel, userID));

    if(g_adcDataBase.channelConfigValidBitMap == 0)
    {
        // diable ADC
        ADC->CTRL = 0;
        // disable clock
        ADC_Disable();
        XIC_DisableIRQ(PXIC_Adc_IRQn);
    }

    RestoreIRQMask(mask);
}

int32_t ADC_StartConversion(adc_channel_t channel, adc_user_t userID)
{
    uint32_t mask, requestIndex;
    int32_t ret;

    requestIndex = CHANNEL_ID_TO_INDEX(channel, userID);

    ASSERT(requestIndex < ADC_MAX_LOGIC_CHANNELS);

    mask = SaveAndSetIRQMask();

    // validation check
    if(g_adcDataBase.channelConfigValidBitMap & (1 << requestIndex) == 0)
    {
        RestoreIRQMask(mask);
        return -2;
    }

    // A conversion is ongoing, pending the request
    if(g_currentRequestIndex != ADC_MAX_LOGIC_CHANNELS)
    {
        RestoreIRQMask(mask);

        ret = ADC_RequestQueueWrite(&requestIndex);

        if(ret != 0)
        {
            // request queue is full
            return ret;
        }
    }
    else
    {
        g_currentRequestIndex = requestIndex;
        ADC_LoadConfigAndStartConversion(g_currentRequestIndex);
        RestoreIRQMask(mask);

#ifdef PM_FEATURE_ENABLE
        LOCK_SLEEP();
#endif

    }

    return 0;

}

/**
  \fn        int32_t ADC_SamplePolling(adc_channel_t channel, adc_user_t userID, uint32_t timeout_ms)
  \brief     Poll ADC conversion.

  \param[in] channel     ADC physical channel to converse
  \param[in] userID      user ID of specific channel, customer user is assigned with ADC_UserAPP, used to compose logical channel combining with channel parameter
  \param[in] timeout_ms  timeout value in unit of ms
  \return                ADC sample result on success, -1 if timeout, -2 if channel has not been initialized yet
  \note                  This API is used internally, customers shall not use!!!!!
 */
int32_t ADC_SamplePolling(adc_channel_t channel, adc_user_t userID, uint32_t timeout_ms)
{
    uint32_t mask, requestIndex, timeout_100us;
    int32_t ret;

    requestIndex = CHANNEL_ID_TO_INDEX(channel, userID);

    ASSERT(requestIndex < ADC_MAX_LOGIC_CHANNELS);

    mask = SaveAndSetIRQMask();

    // 1. validation check
    if(g_adcDataBase.channelConfigValidBitMap & (1 << requestIndex) == 0)
    {
        RestoreIRQMask(mask);
        return -2;
    }

    RestoreIRQMask(mask);

    timeout_100us = timeout_ms * 10;

    // 2. Check if a conversion is ongoing and wait for all conversions in request queue are complete
    while(timeout_100us)
    {
        mask = SaveAndSetIRQMask();

        if(g_currentRequestIndex == ADC_MAX_LOGIC_CHANNELS)
        {
            // set g_currentRequestIndex as a lock here
            g_currentRequestIndex = requestIndex;
            RestoreIRQMask(mask);
            break;
        }

        RestoreIRQMask(mask);

        delay_us(100);
        timeout_100us--;
    }

    // 3. check timeout expired or not
    if(timeout_100us == 0)
    {
        // return directly here to let the first call of this API to restore queue request service
        return -1;
    }
    else
    {
        // 4. no timeout then diable IRQ and start conversion
        XIC_DisableIRQ(PXIC_Adc_IRQn);
        ADC_LoadConfigAndStartConversion(g_currentRequestIndex);

        // 5.wait for conversion done flag
        while((ADC->STATUS & ADC_STATUS_DATA_VALID_Msk) == 0 && timeout_100us)
        {
            delay_us(100);
            timeout_100us--;
        }
        // 6. check timeout
        if(timeout_100us == 0)
        {
            ret = -1;
        }
        else
        {
            ret = ADC->RESULT;
        }

        // 7. clear status
        ADC->CTRL = 0;
        XIC_ClearPendingIRQ(PXIC_Adc_IRQn);

    }

    mask = SaveAndSetIRQMask();

    // 8. restore IRQ to resume queue request service
    XIC_EnableIRQ(PXIC_Adc_IRQn);

    // 9. Check if there is pending request during polling conversion
    if(ADC_RequestQueueRead(&requestIndex) == 0)
    {
        //resume
        g_currentRequestIndex = requestIndex;
        ADC_LoadConfigAndStartConversion(g_currentRequestIndex);

        RestoreIRQMask(mask);
#ifdef PM_FEATURE_ENABLE
        LOCK_SLEEP();
#endif
    }
    else
    {
        g_currentRequestIndex = ADC_MAX_LOGIC_CHANNELS;
        RestoreIRQMask(mask);
    }
    return ret;

}

