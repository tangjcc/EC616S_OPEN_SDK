/**************************************************************************//**
 * @file     system_ec616s.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM3 Device Series
 * @version  V5.00
 * @date     07. September 2016
 ******************************************************************************/
/*
 * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
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

#include "ec616s.h"
#include "mpu_armv7.h"
#include "driver_common.h"
#include "cache_ec616s.h"
#include "mem_map.h"
#include "slpman_ec616s.h"
#include "bl_bsp.h"

extern void CopyPre1RamtoImage(void);
extern void CopyPre2RamtoImage(void);

extern void CopyRWDataFromBin(void);
extern void SetZIDataToZero(void);
extern void CopyDataRamtoImage(void);
extern void Transfer_Control(uint32_t p_base_addr);
extern void SystemXIPFastBoot(void);

#define SYS_ADDRESS  APP_FLASH_LOAD_ADDR
#define BOOT_TIME_TEST 0

/*
 *  This function is called in flash erase or write apis to prevent 
 *  unexpected access the bootloader image read only area.
 *  Be careful if you want change this function.
 *  Parameter:      Addr: flash erase or write addr
 */
uint8_t BSP_QSPI_Check_ROAddr(uint32_t Addr)
{
    if(Addr <(BOOTLOADER_FLASH_LOAD_ADDR+BOOTLOADER_FLASH_LOAD_SIZE-FLASH_XIP_ADDR))
    {
        return 1;
    }
    return 0;
}

/*
 *  This function is called in flash erase or write apis to prevent 
 *  unexpected access the image read only area.
 *  Be careful if you want change this function.
 *  Parameter:      Addr: flash erase or write addr
 *  Parameter:      Addr: flash erase or write size
 
 */
uint8_t BSP_QSPI_Check_ROSpace(uint32_t Addr, uint32_t Size)
{
    if(BSP_QSPI_Check_ROAddr(Addr))
    {
        return 1;
    }
    if (BSP_QSPI_Check_ROAddr(Addr+Size - 1))
    {
        return 1;
    }
    return 0;
}

/*----------------------------------------------------------------------------
  Externals
 *----------------------------------------------------------------------------*/
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
#if defined(__CC_ARM)
   extern uint32_t __Vectors;
#elif defined(__GNUC__)
   extern uint32_t __isr_vector;
#endif
#endif

uint32_t gBlBootFlag=0;//define a global var for debug.0 is normal power on
#define  CORE_CLOCK_REGISER_ADDRESS    (0x44050018)
uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

/*----------------------------------------------------------------------------
  System Core Clock update function
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void)
{
    switch((*((uint32_t *)CORE_CLOCK_REGISER_ADDRESS)) & 0x3)
    {
        case 0:
            SystemCoreClock = 204800000U;
            break;
        case 1:
            SystemCoreClock = 102400000U;
            break;
        case 2:
            SystemCoreClock = 26000000U;
            break;
        case 3:
            SystemCoreClock = 32768U;
            break;
    }
}


#if BOOT_TIME_TEST
UINT32 PmuSlowCntRead_Flash(void)
{
    *(volatile uint32_t *)(0x44000054) = 3;

    while(0x3 == *(volatile uint32_t *)(0x44000054));

    return (*(uint32_t *)(0x470401B8));
}
#endif



/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit (void)
{
    EnableICache();

    bl_clk_check();

    CopyPre1RamtoImage();

#if BOOT_TIME_TEST
    *((uint32_t *)0x2004) = PmuSlowCntRead_Flash();
#endif

    BSP_QSPI_Set_Clk_68M();

    CopyPre2RamtoImage();
#if BOOT_TIME_TEST
    *((uint32_t *)0x2008) = PmuSlowCntRead_Flash();
#endif
    gBlBootFlag = (*(uint32_t *)0x44000078)&0x7;
    if (gBlBootFlag != 0)
    {
        if (SystemWakeUpBootInit()==TRUE)
        {
            SystemXIPFastBoot();
        }
    }
    
    CopyDataRamtoImage();
    
    /*move the RW data in the image to its excution region*/
    CopyRWDataFromBin();

    /*append the ZI region. TODO: maybe ZI data need not to be 0,
    random value maybe aslo fine for application, if so we could
    remove this func call, since it takes a lot of time*/
    SetZIDataToZero();
    
    SystemNormalBootInit();

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
#if defined(__CC_ARM)
  SCB->VTOR = (uint32_t) &__Vectors;
#elif defined (__GNUC__)
  SCB->VTOR = (uint32_t) &__isr_vector;
#endif
#endif

    SystemCoreClockUpdate();
}

