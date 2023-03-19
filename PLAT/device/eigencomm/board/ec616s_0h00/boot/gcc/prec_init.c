#include <string.h>
#include "ec616s.h"
#include "cache_ec616s.h"



extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Limit;
extern UINT32 Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Base;
extern UINT32 Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Limit;
extern UINT32 Image$$LOAD_DRAM_MCU$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_MCU$$ZI$$Limit;

extern UINT32 Image$$LOAD_IRAM2$$Base;
extern UINT32 Load$$LOAD_IRAM2$$Base;
extern UINT32 Image$$LOAD_IRAM2$$Limit;

extern UINT32 Image$$LOAD_IRAM_MCUVECTOR$$Base;
extern UINT32 Load$$LOAD_IRAM_MCUVECTOR$$Base;
extern UINT32 Image$$LOAD_IRAM_MCUVECTOR$$Limit;

extern UINT32 Image$$LOAD_DRAM_SHARED$$Base;
extern UINT32 Load$$LOAD_DRAM_SHARED$$Base;
extern UINT32 Image$$LOAD_DRAM_SHARED$$Limit;

extern UINT32 Image$$LOAD_DRAM_MCU$$Base;
extern UINT32 Load$$LOAD_DRAM_MCU$$Base;
extern UINT32 Image$$LOAD_DRAM_MCU$$Limit;

extern UINT32 Image$$LOAD_IRAM1$$Base;
extern UINT32 Load$$LOAD_IRAM1$$Base;
extern UINT32 Image$$LOAD_IRAM1$$Limit;

extern UINT32 Image$$LOAD_IRAM_MCU$$Base;
extern UINT32 Load$$LOAD_IRAM_MCU$$Base;
extern UINT32 Image$$LOAD_IRAM_MCU$$Limit;

extern UINT32 Image$$LOAD_DRAM_BSP$$Base;
extern UINT32 Load$$LOAD_DRAM_BSP$$Base;
extern UINT32 Image$$LOAD_DRAM_BSP$$Limit;
extern UINT32 Image$$LOAD_DRAM_BSP$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_BSP$$ZI$$Limit;


void __fast_memset(UINT32 *dst, UINT32 value, UINT32 length)
{
    asm volatile(
    "push {r4-r9}\n\t"
    "cmp r2, #0\n\t"
    "beq memset_return\n\t"
    "mov r4, r1\n\t"
    "mov r5, r1\n\t"
    "mov r6, r1\n\t"
    "mov r7, r1\n\t"
    "and r8, r2,#0xf\n\t"
    "mov r9, r8\n\t"
    "cmp r8, #0\n\t"
    "beq clr_16Byte\n\t"
    "clr_4Byte:\n\t"
    "stmia r0!,{r4}\n\t"
    "subs r8,r8,#4\n\t"
    "cmp r8,#0\n\t"
    "bne clr_4Byte\n\t"
    "sub r2,r2,r9\n\t"
    "clr_16Byte:\n\t"
    "stmia r0!,{r4,r5,r6,r7}\n\t"
    "subs r2,r2,#16\n\t"
    "cmp r2,#0\n\t"
    "bne clr_16Byte\n\t"
    "memset_return:\n\t"
    "pop {r4-r9}\n\t"
    "bx lr\n\t"
    );
}



void SetZIDataToZero(void)
{
#if 1

    UINT32 *start_addr;
    UINT32 *end_addr  ;
    UINT32 length;
    start_addr = &(Image$$LOAD_DRAM_SHARED$$ZI$$Base) ;
    end_addr   = &(Image$$LOAD_DRAM_SHARED$$ZI$$Limit);
    length = (UINT32)end_addr - (UINT32)start_addr;
    __fast_memset((UINT32 *)start_addr, 0, length);

    start_addr = &(Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Base);
    end_addr   = &(Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Limit);
    length = (UINT32)end_addr - (UINT32)start_addr;
    __fast_memset((UINT32 *)start_addr, 0, length);

    start_addr = &(Image$$LOAD_DRAM_MCU$$ZI$$Base) ;
    end_addr   = &(Image$$LOAD_DRAM_MCU$$ZI$$Limit);
    length = (UINT32)end_addr - (UINT32)start_addr;
    __fast_memset((UINT32 *)start_addr, 0, length);

#endif

}




void CopyRWDataFromBin(void)
{
#if 1
    UINT32 *src;
    UINT32 *dst;
    UINT32 length;

    dst    = &(Image$$LOAD_IRAM2$$Base);
    src    = &(Load$$LOAD_IRAM2$$Base);
    length = (UINT32)&(Image$$LOAD_IRAM2$$Limit) - (UINT32)&(Image$$LOAD_IRAM2$$Base);
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
            dst[length-1]=src[length-1];
            length--; 
        }
    }

    dst    = &(Image$$LOAD_IRAM_MCUVECTOR$$Base);
    src    = &(Load$$LOAD_IRAM_MCUVECTOR$$Base);
    length = (UINT32)&(Image$$LOAD_IRAM_MCUVECTOR$$Limit) - (UINT32)&(Image$$LOAD_IRAM_MCUVECTOR$$Base);;
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
            dst[length-1]=src[length-1];
            length--; 
        }
    }


    DisableICache();                    // flush cache when ramcode update
    EnableICache();

    dst    = &(Image$$LOAD_DRAM_SHARED$$Base);
    src    = &(Load$$LOAD_DRAM_SHARED$$Base);
    length = (UINT32)&(Image$$LOAD_DRAM_SHARED$$Limit) - (UINT32)&(Image$$LOAD_DRAM_SHARED$$Base);
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
            dst[length-1]=src[length-1];
            length--; 
        }
    }


    dst    = &(Image$$LOAD_DRAM_MCU$$Base);
    src    = &(Load$$LOAD_DRAM_MCU$$Base);
    length = (UINT32)&(Image$$LOAD_DRAM_MCU$$Limit) - (UINT32)&(Image$$LOAD_DRAM_MCU$$Base);
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
            dst[length-1]=src[length-1];
            length--; 
        }
    }

#endif

}



void CopyRamCodeForDeepSleep(void)
{
#if 1

    UINT32 *src;
    UINT32 *dst;
    UINT32 length;

    dst    = &(Image$$LOAD_IRAM1$$Base);
    src    = &(Load$$LOAD_IRAM1$$Base);
    length = (UINT32)&(Image$$LOAD_IRAM1$$Limit) - (UINT32)&(Image$$LOAD_IRAM1$$Base);
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
            dst[length-1]=src[length-1];
            length--; 
        }
    }

    dst    = &(Image$$LOAD_IRAM_MCU$$Base);
    src    = &(Load$$LOAD_IRAM_MCU$$Base);
    length = (UINT32)&(Image$$LOAD_IRAM_MCU$$Limit) - (UINT32)&(Image$$LOAD_IRAM_MCU$$Base);
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
            dst[length-1]=src[length-1];
            length--; 
        }
    }

    DisableICache();        // flush cache when ramcode update
    EnableICache();


#endif

}






void CopyDataRWZIForDeepSleep(void)
{
#if 1

    UINT32 *start_addr;
        UINT32 *end_addr  ;
         UINT32 *src;
    UINT32 *dst;
    UINT32 length;

    dst    = &(Image$$LOAD_DRAM_BSP$$Base);
    src    = &(Load$$LOAD_DRAM_BSP$$Base);
    length = (UINT32)&(Image$$LOAD_DRAM_BSP$$Limit) - (UINT32)&(Image$$LOAD_DRAM_BSP$$Base);
    length /= sizeof(UINT32);

    if(dst != src)
    {
        while(length >0)
        {
          dst[length-1]=src[length-1];
          length--; 
        }
    }

    start_addr = &(Image$$LOAD_DRAM_BSP$$ZI$$Base) ;
    end_addr   = &(Image$$LOAD_DRAM_BSP$$ZI$$Limit);
    length = (UINT32)end_addr - (UINT32)start_addr;
    __fast_memset((UINT32 *)start_addr, 0, length);


#endif

}


void mpu_init(void)
{
#if 0

    int i=0;
    uint8_t region_num = 0;

    if(MPU->TYPE==0)
        return;

    ARM_MPU_Disable();

    for(i=0;i<8;i++)
        ARM_MPU_ClrRegion(i);

    region_num = sizeof(mpu_region)/sizeof(mpu_setting_t);

    for(i=0;i<region_num;i++)
    {
        ARM_MPU_SetRegionEx(i,((uint32_t)mpu_region[i].base_addr)+mpu_region[i].offset,ARM_MPU_RASR(mpu_region[i].excute_disabled,mpu_region[i].access_permission,0,0,mpu_region[i].cacheable,1,0,mpu_region[i].size));
    }

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
#endif
}


void mpu_deinit()
{
#if 0
    int i=0;

    if(MPU->TYPE==0)
        return;

    ARM_MPU_Disable();

    for(i=0;i<8;i++)
        ARM_MPU_ClrRegion(i);
#endif

}



