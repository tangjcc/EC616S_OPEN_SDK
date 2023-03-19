#include <string.h>
#include "commontypedef.h"
#include "ec616s.h"
#include "cache_ec616s.h"
#include "mpu_armv7.h"
#include "mem_map.h"


typedef struct
{
    UINT32 *base_addr;
    INT32 offset;
    UINT32 size;
    UINT8 access_permission;
    UINT8 cacheable;
    UINT8 excute_disabled;
}mpu_setting_t;



__asm void __fast_memset(UINT32 *dst, UINT32 value, UINT32 length)
{
    push {r4-r9}
    cmp r2, #0
    beq memset_return
    mov r4, r1
    mov r5, r1
    mov r6, r1
    mov r7, r1
    and r8, r2,#0xf
    mov r9, r8
    cmp r8, #0
    beq clr_16Byte
clr_4Byte
    stmia r0!,{r4}
    subs r8,r8,#4
    cmp r8,#0
    bne clr_4Byte
    sub r2,r2,r9
clr_16Byte
    stmia r0!,{r4,r5,r6,r7}
    subs r2,r2,#16
    cmp r2,#0
    bne clr_16Byte
memset_return
    pop {r4-r9}
    bx lr
}


#if defined(__CC_ARM)
extern UINT32 Load$$LOAD_IRAM1$$Base    ;
extern UINT32 Image$$LOAD_IRAM1$$Base   ;
extern UINT32 Image$$LOAD_IRAM1$$Length ;

extern UINT32 Load$$LOAD_IRAM2$$Base    ;
extern UINT32 Image$$LOAD_IRAM2$$Base   ;
extern UINT32 Image$$LOAD_IRAM2$$Length ;

extern UINT32 Image$$UNLOAD_NOCACHE$$Base   ;

extern UINT32 Load$$LOAD_DRAM_SHARED$$Base  ;
extern UINT32 Image$$LOAD_DRAM_SHARED$$Base ;
extern UINT32 Image$$LOAD_DRAM_SHARED$$Length   ;
extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Limit;

extern UINT32 Load$$LOAD_IRAM_MCUVECTOR$$Base;
extern UINT32 Image$$LOAD_IRAM_MCUVECTOR$$Base;
extern UINT32 Image$$LOAD_IRAM_MCUVECTOR$$Length;


extern UINT32 Load$$LOAD_IRAM_MCU$$Base;
extern UINT32 Image$$LOAD_IRAM_MCU$$Base;
extern UINT32 Image$$LOAD_IRAM_MCU$$Length;

extern UINT32 Load$$LOAD_DRAM_MCU$$Base;
extern UINT32 Image$$LOAD_DRAM_MCU$$Base;
extern UINT32 Image$$LOAD_DRAM_MCU$$Length;

extern UINT32 Image$$LOAD_DRAM_MCU$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_MCU$$ZI$$Limit;


extern UINT32 Load$$UNLOAD_DRAM_PSPHYRET$$Base;
extern UINT32 Image$$UNLOAD_DRAM_PSPHYRET$$Base;
extern UINT32 Image$$UNLOAD_DRAM_PSPHYRET$$Length;

extern UINT32 Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Base;
extern UINT32 Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Limit;

extern UINT32 Load$$LOAD_DRAM_BSP$$Base ;
extern UINT32 Image$$LOAD_DRAM_BSP$$Base    ;
extern UINT32 Image$$LOAD_DRAM_BSP$$Length  ;
extern UINT32 Image$$LOAD_DRAM_BSP$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_BSP$$ZI$$Limit;
extern UINT32 Image$$UNLOAD_IROM$$Base;
extern UINT32 Image$$UNLOAD_DRAM_USRNV$$Limit;



extern UINT32 Stack_Size;
#endif

const mpu_setting_t mpu_region[] =
{
    // base_addr                        offset  size                        access          cache   excute
    {&Image$$UNLOAD_IROM$$Base,         0,      ARM_MPU_REGION_SIZE_4MB,    ARM_MPU_AP_RO,  1,      0},

    {&Image$$UNLOAD_NOCACHE$$Base,      0,      ARM_MPU_REGION_SIZE_128B,   ARM_MPU_AP_RO,  0,      0},
    {&Image$$LOAD_IRAM1$$Base,          0,      ARM_MPU_REGION_SIZE_16KB,   ARM_MPU_AP_RO,  1,      0},
    {&Image$$LOAD_IRAM1$$Base,          0x4000, ARM_MPU_REGION_SIZE_8KB,   ARM_MPU_AP_RO,  1,      0},

	// iram mpu protect only 0x4000-0x9000 on defalut, user can extend mpu area if needed

    {&Image$$LOAD_IRAM_MCUVECTOR$$Base, 0,      ARM_MPU_REGION_SIZE_2KB,    ARM_MPU_AP_RO,  1,      0},
    {&Image$$UNLOAD_DRAM_USRNV$$Limit, -32,     ARM_MPU_REGION_SIZE_32B,    ARM_MPU_AP_RO,  1,      0},

};


void SetZIDataToZero(void)
{
#if defined(__CC_ARM)
    UINT32 *start_addr;
    UINT32 *end_addr  ;
    UINT32 length;
    UINT32* stack_len = &(Stack_Size);
    start_addr = &(Image$$LOAD_DRAM_SHARED$$ZI$$Base) ;
    end_addr   = &(Image$$LOAD_DRAM_SHARED$$ZI$$Limit);
    length = (UINT32)end_addr - (UINT32)start_addr;
    __fast_memset((UINT32 *)start_addr, 0, length-(UINT32)stack_len);

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

    UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)

    dst    = &(Image$$LOAD_IRAM2$$Base);
    src    = &(Load$$LOAD_IRAM2$$Base);
    length = (UINT32)&(Image$$LOAD_IRAM2$$Length);
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
    length = (UINT32)&(Image$$LOAD_IRAM_MCUVECTOR$$Length);
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
    length = (UINT32)&(Image$$LOAD_DRAM_SHARED$$Length);
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
    length = (UINT32)&(Image$$LOAD_DRAM_MCU$$Length);
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
    UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)
    dst    = &(Image$$LOAD_IRAM1$$Base);
    src    = &(Load$$LOAD_IRAM1$$Base);
    length = (unsigned int)&(Image$$LOAD_IRAM1$$Length);
    length /= sizeof(unsigned int);

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
    length = (UINT32)&(Image$$LOAD_IRAM_MCU$$Length);
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
    UINT32 *start_addr;
        UINT32 *end_addr  ;
         UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)
    dst    = &(Image$$LOAD_DRAM_BSP$$Base);
    src    = &(Load$$LOAD_DRAM_BSP$$Base);
    length = (unsigned int)&(Image$$LOAD_DRAM_BSP$$Length);
    length /= sizeof(unsigned int);

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
}


void mpu_deinit()
{
    int i=0;

    if(MPU->TYPE==0)
        return;

    ARM_MPU_Disable();

    for(i=0;i<8;i++)
        ARM_MPU_ClrRegion(i);

}




