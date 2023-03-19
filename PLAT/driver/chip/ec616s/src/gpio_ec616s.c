 /****************************************************************************
  *
  * Copy right:   2017-, Copyrigths of EigenComm Ltd.
  * File name:    gpio_ec616s.c
  * Description:  EC616S gpio driver source file
  * History:      Rev1.0   2018-07-12
  *
  ****************************************************************************/

#include "gpio_ec616s.h"
#include "clock_ec616s.h"
#include "slpman_ec616s.h"

#define EIGEN_GPIO(n)             ((GPIO_TypeDef *) (GPIO_BASE_ADDR + 0x1000*n))

/**
  \brief GPIO clock for each instance
 */
static clock_ID_t g_gpioClocks[GPIO_INSTANCE_NUM] = {GPR_GPIO0APBClk, GPR_GPIO1APBClk};

/**
  \brief GPIO stutas flag
         The low significant two bits are bitmap of clock enable flag,
         the most significant bit indicates GPIO driver's initialization status
 */
static uint32_t g_gpioStatus = 0;

#ifdef PM_FEATURE_ENABLE
/**
  \fn        void GPIO_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
  \brief     Backup gpio configurations before sleep.
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
void GPIO_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
{
    return;
}

/**
 \fn        void GPIO_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
 \brief     Restore gpio configurations after exit from sleep.
 \param[in] pdata pointer to user data, not used now
 \param[in] state low power state
 */
void GPIO_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            for(i = 0; i < 2U; i++)
            {
                if(g_gpioStatus & (0x1U << i))
                {
                    GPR_ClockEnable(GPR_GPIOAPBClk);
                    GPR_ClockEnable(g_gpioClocks[i]);
                }
            }
            break;

        default:
            break;
    }

}
#endif

void GPIO_DriverInit(void)
{
    if((g_gpioStatus & 0x80000000U) == 0)
    {

#ifdef PM_FEATURE_ENABLE
        slpManRegisterPredefinedBackupCb(SLP_CALLBACK_GPIO_MODULE, GPIO_EnterLowPowerStatePrepare, NULL, SLPMAN_SLEEP1_STATE);
        slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_GPIO_MODULE, GPIO_ExitLowPowerStateRestore, NULL, SLPMAN_SLEEP1_STATE);
#endif

        g_gpioStatus = 0x80000000U;

    }

}

void GPIO_DriverDeInit(void)
{
    g_gpioStatus = 0;

    // disable clock
    CLOCK_ClockDisable(GPR_GPIO0APBClk);
    CLOCK_ClockDisable(GPR_GPIO1APBClk);

#ifdef PM_FEATURE_ENABLE
    slpManUnregisterPredefinedBackupCb(SLP_CALLBACK_GPIO_MODULE);
    slpManUnregisterPredefinedRestoreCb(SLP_CALLBACK_GPIO_MODULE);
#endif
}

void GPIO_PinConfig(uint32_t port, uint16_t pin, const gpio_pin_config_t *config)
{
    ASSERT(port < GPIO_INSTANCE_NUM);

    uint16_t pinMask = 0x1U << pin;
    GPIO_TypeDef *base = EIGEN_GPIO(port);

    GPIO_DriverInit();

    if((g_gpioStatus & (0x1U << port)) == 0)
    {
        CLOCK_ClockEnable(g_gpioClocks[port]);
        g_gpioStatus |= (0x1U << port);
    }

    switch(config->pinDirection)
    {
        case GPIO_DirectionInput:

            base->OUTENCLR = pinMask;

            GPIO_InterruptConfig(port, pin, config->misc.interruptConfig);

            break;
        case GPIO_DirectionOutput:

            GPIO_PinWrite(port, pinMask, ((uint16_t)config->misc.initOutput) << pin);

            base->OUTENSET = pinMask;

            break;
        default:
            break;
    }
}

void GPIO_InterruptConfig(uint32_t port, uint16_t pin, gpio_interrupt_config_t config)
{
    uint16_t pinMask = 0x1U << pin;
    GPIO_TypeDef *base = EIGEN_GPIO(port);

    switch(config)
    {
        case GPIO_InterruptDisabled:

            base->INTENCLR = pinMask;

            break;
        case GPIO_InterruptLowLevel:

            base->INTPOLCLR = pinMask;
            base->INTTYPECLR = pinMask;
            base->INTENSET = pinMask;

            break;
        case GPIO_InterruptHighLevel:

            base->INTPOLSET = pinMask;
            base->INTTYPECLR = pinMask;
            base->INTENSET = pinMask;

            break;
        case GPIO_InterruptFallingEdge:

            base->INTPOLCLR = pinMask;
            base->INTTYPESET = pinMask;
            base->INTENSET = pinMask;

            break;
        case GPIO_InterruptRisingEdge:

            base->INTPOLSET = pinMask;
            base->INTTYPESET = pinMask;
            base->INTENSET = pinMask;

            break;
        default :
            break;
    }
}

void GPIO_PinWrite(uint32_t port, uint16_t pinMask, uint16_t output)
{
    EIGEN_GPIO(port)->MASKLOWBYTE[pinMask & 0xFFU] = output;
    EIGEN_GPIO(port)->MASKHIGHBYTE[pinMask >> 8U] = output;
}

uint32_t GPIO_PinRead(uint32_t port, uint16_t pin)
{
    return (((EIGEN_GPIO(port)->DATA) >> pin) & 0x01U);
}

uint16_t GPIO_GetInterruptFlags(uint32_t port)
{
    return EIGEN_GPIO(port)->INTSTATUS;
}

void GPIO_ClearInterruptFlags(uint32_t port, uint16_t mask)
{
    EIGEN_GPIO(port)->INTSTATUS = mask;
}

uint16_t GPIO_SaveAndSetIRQMask(uint32_t port)
{
    uint16_t mask = EIGEN_GPIO(port)->INTENSET;
    EIGEN_GPIO(port)->INTENCLR = 0xFFFFU;
    return mask;
}

void GPIO_RestoreIRQMask(uint32_t port, uint16_t mask)
{
    EIGEN_GPIO(port)->INTENSET = mask;
}

