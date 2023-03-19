/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    clock_ec616s.c
 * Description:  EC616S clock driver source file
 * History:      Rev1.0   2019-02-20
 *
 ****************************************************************************/

#include "clock_ec616s.h"
#include <assert.h>
#include <string.h>

#define INVALID_CLK_ID             (GPR_InvalidClk)
#define CLK_TREE_ELEMENT_NUMBER    (GPR_InvalidClk)

/** \brief support clock driver full feature or not */
#define ENABLE_CLK_TREE_PARENT

/** \brief definition of clk tree element for clock management */
typedef struct _clk_tree_element
{
    clock_ID_t  parentId;     /**< id of this clock's parent clk */
    uint8_t     enableCount;  /**< counter of each clk has been enabled */
} clk_tree_element_t;

/** \brief clock tree Array */
static clk_tree_element_t g_clkTreeArray[CLK_TREE_ELEMENT_NUMBER];

void CLOCK_DriverInit(void)
{
    uint32_t mask;

    mask = SaveAndSetIRQMask();

    for(uint32_t i = 0; i < CLK_TREE_ELEMENT_NUMBER; i++)
    {
        g_clkTreeArray[i].parentId = INVALID_CLK_ID;
        g_clkTreeArray[i].enableCount = 0;
    }

    g_clkTreeArray[GPR_204MClk].parentId = INVALID_CLK_ID;
    g_clkTreeArray[GPR_204MClk].enableCount = 1;

    g_clkTreeArray[GPR_102MClk].parentId = GPR_204MClk;
    g_clkTreeArray[GPR_102MClk].enableCount = 1;

    g_clkTreeArray[GPR_51MClk].parentId = GPR_102MClk;
    g_clkTreeArray[GPR_51MClk].enableCount = 1;

    g_clkTreeArray[GPR_AONFuncClk].parentId = INVALID_CLK_ID;
    g_clkTreeArray[GPR_AONFuncClk].enableCount = 1;

    g_clkTreeArray[GPR_GPIO0APBClk].parentId = GPR_GPIOAPBClk;
    g_clkTreeArray[GPR_GPIO1APBClk].parentId = GPR_GPIOAPBClk;
    g_clkTreeArray[GPR_GPIOAPBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_I2C0FuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_I2C1FuncClk].parentId = GPR_CLKMFClk;

    g_clkTreeArray[GPR_I2C0APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_I2C1APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_PADAPBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_RFFEFuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_RFFEAPBClk].parentId = GPR_DMAAPBClk;

    g_clkTreeArray[GPR_SPI0FuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_SPI1FuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_SPI0APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_SPI1APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_UART0FuncClk].parentId = GPR_CLKMFClk;

    g_clkTreeArray[GPR_UART1FuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_UART2FuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_UART0APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_UART1APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_UART2APBClk].parentId = GPR_DMAAPBClk;

    g_clkTreeArray[GPR_TIMER0FuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_TIMER1FuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_TIMER2FuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_TIMER3FuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_TIMER4FuncClk].parentId = GPR_32KClk;

    g_clkTreeArray[GPR_TIMER5FuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_SystickFuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_TIMER0APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_TIMER1APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_TIMER2APBClk].parentId = GPR_DMAAPBClk;

    g_clkTreeArray[GPR_TIMER3APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_TIMER4APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_TIMER5APBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_WDGFuncClk].parentId = GPR_32KClk;
    g_clkTreeArray[GPR_WDGAPBClk].parentId = GPR_DMAAPBClk;

    g_clkTreeArray[GPR_USIMFuncClk].parentId = GPR_CLKMFClk;
    g_clkTreeArray[GPR_USIMAPBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_EFUSEAPBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_TRNGAPBClk].parentId = GPR_DMAAPBClk;

    g_clkTreeArray[GPR_CoreClk].parentId = GPR_204MClk;
    g_clkTreeArray[GPR_CoreClk].enableCount = 1;

    g_clkTreeArray[GPR_32KClk].parentId = INVALID_CLK_ID;
    g_clkTreeArray[GPR_32KClk].enableCount = 1;

    g_clkTreeArray[GPR_CLKMFClk].parentId = INVALID_CLK_ID;
    g_clkTreeArray[GPR_CLKMFClk].enableCount = 1;

    g_clkTreeArray[GPR_DMAAHBClk].parentId = INVALID_CLK_ID;
    g_clkTreeArray[GPR_DMAAHBClk].enableCount = 1;

    g_clkTreeArray[GPR_DMAAPBClk].parentId = GPR_DMAAHBClk;
    g_clkTreeArray[GPR_DMAAPBClk].enableCount = 1;

    g_clkTreeArray[GPR_GPRAPBClk].parentId = GPR_DMAAPBClk;
    g_clkTreeArray[GPR_GPRAPBClk].enableCount = 1;

    g_clkTreeArray[GPR_CIPHERRMIClk].parentId = INVALID_CLK_ID;

    g_clkTreeArray[GPR_CIPHERAHBClk].parentId = INVALID_CLK_ID;

    g_clkTreeArray[GPR_MAINFABAHBClk].parentId = INVALID_CLK_ID;

    g_clkTreeArray[GPR_SMBAHBClk].parentId = INVALID_CLK_ID;

    RestoreIRQMask(mask);
}

void CLOCK_DriverDeInit(void)
{
    uint32_t mask;

    mask = SaveAndSetIRQMask();
    memset(g_clkTreeArray, 0, sizeof(g_clkTreeArray));
    RestoreIRQMask(mask);
}

PLAT_CODE_IN_RAM int32_t CLOCK_ClockEnable(clock_ID_t id)
{

#if defined(ENABLE_CLK_TREE_PARENT)
    int32_t ret;
#endif

    uint32_t mask;

    if(id > INVALID_CLK_ID)
        return ARM_DRIVER_ERROR_PARAMETER;
    else if(id == INVALID_CLK_ID)
        return ARM_DRIVER_OK;

    mask = SaveAndSetIRQMask();

    if(g_clkTreeArray[id].enableCount++ == 0)
    {
        GPR_ClockEnable(id);
        RestoreIRQMask(mask);

#if defined(ENABLE_CLK_TREE_PARENT)
        ret = CLOCK_ClockEnable(g_clkTreeArray[id].parentId);

        assert(ret == ARM_DRIVER_OK);
        #if 0
        if(ret != ARM_DRIVER_OK)   /*in fact already enabled, should disable*/
        {
            CLOCK_ClockDisable(id); /*maybe a bug here, as in this API maybe disable parent clock*/
            return ret;
        }
        #endif
#endif
    }
    else
    {
        RestoreIRQMask(mask);
    }

    return ARM_DRIVER_OK;
}

PLAT_CODE_IN_RAM void CLOCK_ClockDisable(clock_ID_t id)
{
    uint32_t mask;

    if(id >= INVALID_CLK_ID)
        return;

    mask = SaveAndSetIRQMask();

    //assert(g_clkTreeArray[id].enableCount > 0);
    if(g_clkTreeArray[id].enableCount == 0)
    {
        RestoreIRQMask(mask);
        return;
    }

    if(!(--g_clkTreeArray[id].enableCount))
    {
        GPR_ClockDisable(id);

        RestoreIRQMask(mask);

#if defined(ENABLE_CLK_TREE_PARENT)
        CLOCK_ClockDisable(g_clkTreeArray[id].parentId);
#endif

    }
    else
    {
        RestoreIRQMask(mask);
    }
}

PLAT_CODE_IN_RAM void CLOCK_ClockReset(clock_ID_t id)
{
    uint32_t    mask;
    uint8_t     count = 0;
    if(id >= INVALID_CLK_ID)
        return;

    mask = SaveAndSetIRQMask();

    count = g_clkTreeArray[id].enableCount;

    g_clkTreeArray[id].enableCount = 0;

    GPR_ClockDisable(id);

    RestoreIRQMask(mask);

    /* here, maybe not right */
#if defined(ENABLE_CLK_TREE_PARENT)
    if(count > 0)
    {
        CLOCK_ClockDisable(g_clkTreeArray[id].parentId);
    }
#endif

    return;
}

int32_t CLOCK_SetClockSrc(clock_ID_t id, clock_select_t select)
{
    int32_t ret, mask;

    if(id >= INVALID_CLK_ID)
        return ARM_DRIVER_ERROR_PARAMETER;

    mask = SaveAndSetIRQMask();

    if(g_clkTreeArray[id].enableCount == 0)
    {
        ret = GPR_SetClockSrc(id, select);
        if(ret == ARM_DRIVER_OK)
        {
            g_clkTreeArray[id].parentId = (clock_ID_t)(GET_PARENTCLOCKID_FROM_CLOCK_SEL_VALUE(select));
        }

        RestoreIRQMask(mask);

        return ret;
    }

    RestoreIRQMask(mask);

    return ARM_DRIVER_ERROR;
}

int32_t CLOCK_SetClockDiv(clock_ID_t id, uint32_t div)
{
    int32_t mask;

    if(id >= INVALID_CLK_ID)
        return ARM_DRIVER_ERROR_PARAMETER;

    mask = SaveAndSetIRQMask();

    if(g_clkTreeArray[id].enableCount == 0)
    {
        RestoreIRQMask(mask);
        return GPR_SetClockDiv(id, div);
    }

    RestoreIRQMask(mask);

    return ARM_DRIVER_ERROR;

}

uint32_t CLOCK_GetClockFreq(clock_ID_t id)
{
    return GPR_GetClockFreq(id);
}

