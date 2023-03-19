#include <string.h>
#include "ec616s.h"


typedef unsigned long UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;


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
extern UINT32 Load$$LOAD_IRAM_PRE1$$Base	;  
extern UINT32 Image$$LOAD_IRAM_PRE1$$Base	;  
extern UINT32 Image$$LOAD_IRAM_PRE1$$Length	;  
extern UINT32 Load$$LOAD_IRAM_PRE2$$Base	;  
extern UINT32 Image$$LOAD_IRAM_PRE2$$Base	;  
extern UINT32 Image$$LOAD_IRAM_PRE2$$Length	;  


extern UINT32 Load$$LOAD_IRAM$$Base	;
extern UINT32 Image$$LOAD_IRAM$$Base	;
extern UINT32 Image$$LOAD_IRAM$$Length	;
extern UINT32 Load$$LOAD_DRAM_SHARED$$Base	;
extern UINT32 Image$$LOAD_DRAM_SHARED$$Base	;
extern UINT32 Image$$LOAD_DRAM_SHARED$$Length	;
extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Base;
extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Limit;

extern UINT32 Stack_Size;
#endif

//extern UINT32 Image$$DDR_HEAP_ENDMARK$$Base;
//extern UINT32 Image$$DDR_HEAP_GUARD$$Base;
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
#endif
}

void CopyPre1RamtoImage(void)
{

    UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)
    dst    = &(Image$$LOAD_IRAM_PRE1$$Base);
    src    = &(Load$$LOAD_IRAM_PRE1$$Base);
    length = (unsigned int)&(Image$$LOAD_IRAM_PRE1$$Length);
    length /= sizeof(unsigned int);

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

void CopyPre2RamtoImage(void)
{

    UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)
    dst    = &(Image$$LOAD_IRAM_PRE2$$Base);
    src    = &(Load$$LOAD_IRAM_PRE2$$Base);
    length = (unsigned int)&(Image$$LOAD_IRAM_PRE2$$Length);
    length /= sizeof(unsigned int);

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


void CopyDataRamtoImage(void)
{

    UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)
    dst    = &(Image$$LOAD_IRAM$$Base);
    src    = &(Load$$LOAD_IRAM$$Base);
    length = (unsigned int)&(Image$$LOAD_IRAM$$Length);
    length /= sizeof(unsigned int);

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



void CopyRWDataFromBin(void)
{

    UINT32 *src;
    UINT32 *dst;
    UINT32 length;
#if defined(__CC_ARM)
    dst    = &(Image$$LOAD_DRAM_SHARED$$Base);
    src    = &(Load$$LOAD_DRAM_SHARED$$Base);
    length = (unsigned int)&(Image$$LOAD_DRAM_SHARED$$Length);
    length /= sizeof(unsigned int);

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

