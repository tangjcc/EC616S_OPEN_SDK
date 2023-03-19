/* --------------------------------------------------------------------------
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
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
 *
 *      Name:    Bootmain.c
 *      Purpose: for cm3 bootrom
 *
 *---------------------------------------------------------------------------*/
#include "ec616s.h"
#include <stdio.h>
#include "error.h"
#include "boot.h"
#include "common.h"
#include "image.h"
#include <string.h>
#include "cache_ec616s.h"
#include "mem_map.h"
#include "bl_bsp.h"

#define BOOT_TIME_TEST 0


extern void FotaProcedure(void);
void Transfer_Control(uint32_t p_base_addr);


#define XIP_BOOT 0
#define DOWNLOAD_BOOT 1

DownloadCtrl GDownloadCtrl;
void uDelay(void)
{
	int32_t i=0;
	for(i=0;i<70;i++)
			;
}

void mDelay(int ms)
{
    int32_t i=0;
    for(i=0;i<70000;i++)
            ;
}

void Delay(int sec)
{
    int32_t i=0;
    for(i=0;i<1000;i++)
    {
       mDelay(1);
    }
}


#ifndef CONFIG_PROJ_APP_SECURITY_BOOT

#else
extern unsigned char gVerifyPubKey[64];

uint8_t IsSecuritySupport(void)
{
	return 1;
}

uint8_t *GetVerifyPubKey(void)
{
    return &gVerifyPubKey[0];
}
#endif

void SystemXIPFastBoot(void)
{
    //printf("bootloader try xip boot system start!\n");
    CleanBootRomResouce();

    //flush cache
    DisableICache();
    __DSB();
    __ISB();
    EnableICache();
    Transfer_Control(APP_FLASH_LOAD_ADDR);
    while(1);
}

uint32_t SystemXIPNormalBoot(void)
{
    uint32_t RetValue = NoError;

#if LOG_ON
    printf("bootloader try normal boot system start!\n");
#endif

    //load ih from flash
    RetValue = LoadVerifyImageHead();
    if (RetValue != NoError)
    {
        return RetValue;
    }

    //load image from flash
    RetValue = LoadVerifyImageBody();
    if (RetValue != NoError)
    {
        return RetValue;
    }

    BSP_DeInit();

    DisableICache();
    __DSB();
    __ISB();
    Transfer_Control(APP_FLASH_LOAD_ADDR);
    while(1);

}


__WEAK void FotaProcedure(void){};

/*
 * FUNCTION :BSP_InitHook, called at start of BSP_Init

 */
void BSP_InitHook(void)
{
    BSP_WdtTimeCntCfg(32768);
    /* UART1 default baudrate */
    BSP_URCSetCfg(1, 9600);
}
 
/*
 * FUNCTION :main

 */
int ec_main(void)
{
    uint32_t RetValue = NoError;

    BSP_Init();

    FotaProcedure();
    
    RetValue = SystemXIPNormalBoot();
    if (RetValue != NoError)
    {
        printf("bootloader try normal boot system failed(0x%x)!\n", RetValue);
    }
    while(1);
}

void Transfer_Control(uint32_t p_base_addr)
{
	uint32_t *msp_addr = (uint32_t *)p_base_addr;
    typedef void *jmp_fun(void);
    __set_MSP(*msp_addr);
    jmp_fun *jump = (jmp_fun *)(*(msp_addr+1));
    (*jump)();
}


