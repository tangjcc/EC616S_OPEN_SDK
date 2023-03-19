#include <string.h>
#include "ec616s.h"
#include "cache_ec616s.h"

__asm void __fast_memset(uint32_t *dst, uint32_t value, uint32_t length)
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
extern uint32_t Load$$LOAD_IRAM1$$Base    ;
extern uint32_t Image$$LOAD_IRAM1$$Base   ;
extern uint32_t Image$$LOAD_IRAM1$$Length ;

extern uint32_t Image$$UNLOAD_NOCACHE$$Base   ;
extern uint32_t Load$$LOAD_DRAM_SHARED$$Base  ;
extern uint32_t Image$$LOAD_DRAM_SHARED$$Base ;
extern uint32_t Image$$LOAD_DRAM_SHARED$$Length   ;
extern uint32_t Image$$LOAD_DRAM_SHARED$$ZI$$Base;
extern uint32_t Image$$LOAD_DRAM_SHARED$$ZI$$Limit;
extern uint32_t Stack_Size;
#endif

void SetZIDataToZero(void)
{
#if defined(__CC_ARM)
    uint32_t *start_addr;
    uint32_t *end_addr  ;
    uint32_t length;
    uint32_t* stack_len = &(Stack_Size);
    start_addr = &(Image$$LOAD_DRAM_SHARED$$ZI$$Base) ;
    end_addr   = &(Image$$LOAD_DRAM_SHARED$$ZI$$Limit);
    length = (uint32_t)end_addr - (uint32_t)start_addr;
    __fast_memset((uint32_t *)start_addr, 0, length-(uint32_t)stack_len);
#endif
}


void CopyRWDataFromBin(void)
{
    uint32_t *src;
    uint32_t *dst;
    uint32_t length;
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

    DisableICache();                    // flush cache when ramcode update
    EnableICache();

    dst    = &(Image$$LOAD_DRAM_SHARED$$Base);
    src    = &(Load$$LOAD_DRAM_SHARED$$Base);
    length = (uint32_t)&(Image$$LOAD_DRAM_SHARED$$Length);
    length /= sizeof(uint32_t);

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


void mpu_deinit()
{
    int i=0;

    if(MPU->TYPE==0)
        return;

    ARM_MPU_Disable();

    for(i=0;i<8;i++)
        ARM_MPU_ClrRegion(i);

}

