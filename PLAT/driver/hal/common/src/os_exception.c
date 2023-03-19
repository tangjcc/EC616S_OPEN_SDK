/**************************************************************************//**
 * @file
 * @brief
 * @version  V1.0.0
 * @date
 ******************************************************************************/
/*
 *
 *
 */
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h"               // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"                     // ARM.FreeRTOS::RTOS:Core


#if defined CHIP_EC616
#include "flash_ec616_rt.h"
#include "slpman_ec616.h"

#if (WDT_FEATURE_ENABLE==1)
#include "wdt_ec616.h"
#endif

#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#include "slpman_ec616s.h"

#if (WDT_FEATURE_ENABLE==1)
#include "wdt_ec616s.h"
#endif

#endif

#include "bsp.h"
#include "os_exception.h"
#include "debug_log.h"
#include "plat_config.h"

#define hardfault_flash_init()  HAL_QSPI_Init()
#define hardfault_flash_earse(a)    BSP_QSPI_Erase_Safe(a, 0x1000)
#define hardfault_flash_write(a,b,c) BSP_QSPI_Write_Safe(a,b,c)
#define hardfault_flash_read(a,b,c) BSP_QSPI_Read_Safe(a,b,c)

#define AT_PORT_UART_INSTANCE     (1)
#define RESET_REASON_MAGIC		  (0xACD20E00)
#define RESET_REASON_MASK		  (0xFFFFFF00)
#define ACTIVE_RESET_MAGIC		  (0x5)
#define ACTIVE_RESET_MASK		  (0xF)

//#define FUNC_CALL_TRACE            // It's not a good practice to enable this feature since the implementation depends on code section layout which varies.

// external function forward declaration
extern void UART_Init(uint32_t instance, uint32_t baudrate, bool enableFlowCtrl);
extern uint32_t UART_Send(uint32_t instance, const uint8_t *data, uint32_t num, uint32_t timeout_us);

#if defined(__CC_ARM)

    #define CA_STACK_SECTION_START(_name_)      _name_##$$Base
    #define CA_STACK_SECTION_END(_name_)        _name_##$$Limit
    #define CA_RAM2_SECTION_START(_name_)       Image$$##_name_##$$Base
    #define CA_RAM2_SECTION_END(_name_)         Image$$##_name_##$$Limit
    #define CA_RAM2_SECTION_ZI_END(_name_)      Image$$##_name_##$$ZI$$Limit
    #define CA_CODE_SECTION_START(_name_)       Image$$##_name_##$$Base
    #define CA_CODE_SECTION_END(_name_)         Image$$##_name_##$$Limit
    extern const uint32_t CA_STACK_SECTION_START(STACK);
    extern const uint32_t CA_STACK_SECTION_END(STACK);
    extern const uint32_t CA_RAM2_SECTION_START(LOAD_DRAM_SHARED);
    extern const uint32_t CA_RAM2_SECTION_END(LOAD_DRAM_SHARED);
    extern const uint32_t CA_RAM2_SECTION_ZI_END(LOAD_DRAM_SHARED);
    extern const uint32_t CA_CODE_SECTION_START(UNLOAD_IROM);
    extern const uint32_t CA_CODE_SECTION_END(UNLOAD_IROM);

    __INLINE __asm void ec_assert_regs(void)
    {
        PRESERVE8
        IMPORT  excep_store
        ldr r12, =excep_store
        add r12, r12, #16
        stmia r12!, {r0-r11}
        add r12, r12, #4
        mov r0, sp
        stmia r12!, {r0}
        add r12, r12, #8
        mrs r0, PSR
        stmia r12!, {r0}

        add r12, r12, #4
        mrs r0, msp
        stmia r12!, {r0}
        mrs r0, psp
        stmia r12!, {r0}
        mrs r0, CONTROL
        stmia r12!, {r0}

        mrs r0, BASEPRI
        stmia r12!, {r0}
        mrs r0, PRIMASK
        stmia r12!, {r0}
        mrs r0, FAULTMASK
        stmia r12!, {r0}

        isb
        bx lr

    }

#elif defined(__ICCARM_)
    #define ICA_STACK_NAME                      "CSTACK"
    #define ICA_CODE_NAME                       ".text"

    #pragma section=ICA_STACK_NAME
    #pragma section=ICA_CODE_NAME
    static uint32_t ec_get_sp(void)
    {
        register uint32_t result;
        __asm("MOV %0, sp" : "=r" (result));
        return(result);
    }
#elif defined(__GNUC__)
    __attribute__((always_inline)) static inline uint32_t ec_get_sp(void)
    {
        register uint32_t ret;
        __asm volatile ("MOV %0, sp\n" : "=r" (ret));
        return(ret);
    }

    void * __current_pc(void)
    {
        return __builtin_return_address(0);
    }

    void ec_assert_regs(void)
    {
        asm volatile(
                "LDR R12,=excep_store\n\t"
                "add r12, r12, #16\n\t"
                "stmia r12!, {r0-r11}\n\t"
                "add r12, r12, #4\n\t"
                "mov r0, sp\n\t"
                "stmia r12!, {r0}\n\t"
                "add r12, r12, #8\n\t"
                "mrs r0, PSR\n\t"
                "stmia r12!, {r0}\n\t"

                "add r12, r12, #4\n\t"
                "mrs r0, msp\n\t"
                "stmia r12!, {r0}\n\t"
                "mrs r0, psp\n\t"
                "stmia r12!, {r0}\n\t"
                "mrs r0, CONTROL\n\t"
                "stmia r12!, {r0}\n\t"

                "mrs r0, BASEPRI\n\t"
                "stmia r12!, {r0}\n\t"
                "mrs r0, PRIMASK\n\t"
                "stmia r12!, {r0}\n\t"
                "mrs r0, FAULTMASK\n\t"
                "stmia r12!, {r0}\n\t"

                "isb\n\t"
                "bx lr\n\t"
                );

    }
#else
    //#error "compiler is error"
#endif

ExcepConfigOp excep_config_option = EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP;
uint8_t dump_to_at_port = 0;
ec_exception_store excep_store;

#ifdef FUNC_CALL_TRACE
uint32_t code_start_addr         = 0;
uint32_t code_end_addr           = 0;
#endif

uint32_t ec_stack_start_addr     = 0;
uint32_t ec_stack_end_addr       = 0;

char ec_assert_buff[200];
static uint32_t fs_assert_count = 0;
static bool is_fs_assert_flag = false;
static uint32_t resetReasonBuffer;
extern uint32_t *vTaskStackEndAddr(void);
extern uint32_t EcDumpTopFlow(void);


void ResetReasonWrite(ResetReason_e reason)
{
	uint32_t *pReason = (uint32_t *)EC_RESET_REASON_ADDR;

	*pReason = ((reason & 0xFF) | RESET_REASON_MAGIC);


	ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonWrite_0, P_VALUE, 1, "Reset Write = 0x%x",(*pReason));
}

static void ResetReasonSwActiveReset(void)
{
	uint32_t *pReason = (uint32_t *)EC_RESET_REASON_ADDR;

	*pReason = (*pReason | ACTIVE_RESET_MAGIC);


	ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonSwActiveReset_0, P_ERROR, 1, "SW Reset Value = 0x%x",(*pReason));
}


void ResetReasonInit(void)
{
	uint32_t *pReason = (uint32_t *)EC_RESET_REASON_ADDR;
	uint32_t result;

	ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonCheck_0, P_VALUE, 1, "Reset Value = 0x%x",(*pReason));

	if(slpManGetLastSlpState() != SLP_ACTIVE_STATE)
	{
		resetReasonBuffer = RESET_REASON_MAGIC | RESET_REASON_CLEAR;
		ResetReasonWrite(RESET_REASON_CLEAR);

		return;
	}
	resetReasonBuffer = *pReason;

	if(((*pReason) & RESET_REASON_MASK) == RESET_REASON_MAGIC)
	{

		result = (*pReason) & (~RESET_REASON_MASK);

		if((result & ACTIVE_RESET_MASK) == ACTIVE_RESET_MAGIC)
		{
        	ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonCheck_1, P_WARNING, 0, "Reset: Software Active Reset");
		}

		result &= (~ACTIVE_RESET_MASK);

		switch(result)
		{
			case RESET_REASON_HARDFAULT:
				ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonCheck_2, P_WARNING, 0, "Reset Trigger by HardFault");
				break;

			case RESET_REASON_ASSERT:
				ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonCheck_3, P_WARNING, 0, "Reset Trigger by Assert");
				break;

			case RESET_REASON_WDT:
				ECOMM_TRACE(UNILOG_EXCEP_PRINT, ResetReasonCheck_4, P_WARNING, 0, "Reset Trigger by Watch Dog");
				break;

			default:
				break;

		}

	}

	ResetReasonWrite(RESET_REASON_CLEAR);

}


LastResetState_e appGetLastResetReason(void)
{
	LastResetState_e ret = LAST_RESET_MAXNUM;
	uint32_t result;
	slpManSlpState_t slpState = slpManGetLastSlpState();

	if((slpState == SLP_HIB_STATE) || (slpState == SLP_SLP2_STATE))
	{
		ret = LAST_RESET_NORMAL;
	}
	else if((resetReasonBuffer & RESET_REASON_MASK) == RESET_REASON_MAGIC)
	{
		result = resetReasonBuffer & (~RESET_REASON_MASK);
		result &= (~ACTIVE_RESET_MASK);

		if(result == RESET_REASON_HARDFAULT)
			ret = LAST_RESET_HARDFAULT;
		else if(result == RESET_REASON_ASSERT)
			ret = LAST_RESET_ASSERT;
		else if(result == RESET_REASON_WDT)
			ret = LAST_RESET_WDT;
        else if((resetReasonBuffer & ACTIVE_RESET_MASK) == ACTIVE_RESET_MAGIC)
            ret = LAST_RESET_SWRESET;
		else if(result == RESET_REASON_CLEAR)
			ret = LAST_RESET_NORMAL;
		else
			ret = LAST_RESET_UNKNOWN;
	}
	else
		ret = LAST_RESET_POR;
	return ret;
}



void EC_SystemReset(void)
{
	ResetReasonSwActiveReset();

	__NVIC_SystemReset();
}



#ifdef FUNC_CALL_TRACE
uint32_t check_excep_func_call(uint32_t *buf, uint32_t func_call_depth, uint32_t *curr_stack)
{
    uint32_t addr_temp = 0;
    uint32_t *sp_temp = curr_stack;
    uint32_t *task_stack_end_addr = NULL;
    uint32_t depth = 0;
    uint32_t i = 0;

    buf[depth] = excep_store.excep_regs.stack_frame.pc;
    depth++;
    addr_temp = excep_store.excep_regs.stack_frame.lr;
    if(addr_temp&EC_SP_PSP_FLAG)
    {
        task_stack_end_addr = vTaskStackEndAddr();
    }
    else
    {
        task_stack_end_addr = (uint32_t *)(ec_stack_end_addr);
    }
    if((code_start_addr <= addr_temp)&&(addr_temp <= code_end_addr))
    {
        buf[depth] = addr_temp;
        depth++;

        for(;sp_temp < task_stack_end_addr; sp_temp++)
        {
            addr_temp = *((uint32_t *)sp_temp);
            if((code_start_addr <= addr_temp)&&(addr_temp <= code_end_addr))
            {
                buf[depth] = addr_temp;
                depth++;
                if(depth >= func_call_depth)
                {
                    break;
                }
            }
        }
    }

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, check_excep_func_call_1, P_ERROR, 0, "try to parse exception call stack by address compare!");

    for(i=0; i< func_call_depth; i++)
    {
        if(buf[i] != 0)
        {

            ECOMM_TRACE(UNILOG_PLA_STRING, check_excep_func_call_2, P_ERROR, 1,"maybe function address @ 0x%x", buf[i]);
        }
    }

    return depth;
}
#endif

void check_fault_type(void)
{

    if(excep_store.excep_regs.hfsr.bits.VECTBL)
    {
      ECOMM_TRACE(UNILOG_EXCEP_PRINT, hardfault_1, P_ERROR, 0, "hardfault: casued by vector fetch error!");
    }
    else if(excep_store.excep_regs.hfsr.bits.DEBUGEVT)
    {

      ECOMM_TRACE(UNILOG_EXCEP_PRINT, hardfault_2, P_ERROR, 0, "hardfault: casued by debug event!");

    }
    // this hardfault was caused by memory management fault, bus fault or usage fault
    else if(excep_store.excep_regs.hfsr.bits.FORCED)
    {
        //memory manage fault
        if(excep_store.excep_regs.mfsr.value)
        {

            if(excep_store.excep_regs.mfsr.bits.IACCVIOL)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, mmfault_1, P_ERROR, 0, "mem fault: instruction access violatio");
            }
            if(excep_store.excep_regs.mfsr.bits.DACCVIOL)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, mmfault_2, P_ERROR, 0, "mem fault: data access violation");
            }
            if(excep_store.excep_regs.mfsr.bits.MUNSTKERR)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, mmfault_3, P_ERROR, 0, "mem fault: unstacking error");
            }
            if(excep_store.excep_regs.mfsr.bits.MSTKERR)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, mmfault_4, P_ERROR, 0, "mem fault: stacking error");
            }

            //check MMARVALID to make sure MMFAR is still valid
            if(excep_store.excep_regs.mfsr.bits.MMARVALID)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, mmfault_5, P_ERROR, 1, "memory manage fault address @ 0x%x",excep_store.excep_regs.mmfar);
            }
        }
        //bus fault
        if(excep_store.excep_regs.bfsr.value)
        {

            if(excep_store.excep_regs.bfsr.bits.IBUSERR)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, busfault_1, P_ERROR, 0, "bus fault: instrunction acess error");
            }
            if(excep_store.excep_regs.bfsr.bits.PRECISEER)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, busfault_2, P_ERROR, 0, "bus fault: precise data acess error");
            }
            if(excep_store.excep_regs.bfsr.bits.IMPREISEER)
            {
                  ECOMM_TRACE(UNILOG_EXCEP_PRINT, busfault_3, P_ERROR, 0, "bus fault: imprecise data acess error");
            }
            if(excep_store.excep_regs.bfsr.bits.UNSTKERR)
            {
                  ECOMM_TRACE(UNILOG_EXCEP_PRINT, busfault_4, P_ERROR, 0, "bus fault: unstacking error");
            }
            if(excep_store.excep_regs.bfsr.bits.STKERR)
            {
                  ECOMM_TRACE(UNILOG_EXCEP_PRINT, busfault_5, P_ERROR, 0, "bus fault: stacking error");
            }

            //check BFARVALID to make sure BFAR is still valid
            if(excep_store.excep_regs.bfsr.bits.BFARVALID)
            {
                 ECOMM_TRACE(UNILOG_EXCEP_PRINT, busfault_6, P_ERROR, 1, "bus fault address @ 0x%x",excep_store.excep_regs.bfar);
            }

        }
        //usage fault
        if(excep_store.excep_regs.ufsr.value)
        {
            if(excep_store.excep_regs.ufsr.bits.UNDEFINSTR)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, usgaefault_1, P_ERROR, 0, "usage fault: try to execute undefined instruction");
            }
            if(excep_store.excep_regs.ufsr.bits.INVSTATE)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, usgaefault_2, P_ERROR, 0, "usage fault: try to switch to wrong state(ARM)");
            }
            if(excep_store.excep_regs.ufsr.bits.INVPC)
            {
               ECOMM_TRACE(UNILOG_EXCEP_PRINT, usgaefault_3, P_ERROR, 0, "usage fault: execute EXC_RETURN error");
            }
            if(excep_store.excep_regs.ufsr.bits.NOCP)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, usgaefault_4, P_ERROR, 0, "usage fault: try to execute coprocessor instruction");
            }
            if(excep_store.excep_regs.ufsr.bits.UNALIGNED)
            {
               ECOMM_TRACE(UNILOG_EXCEP_PRINT, usgaefault_5, P_ERROR, 0, "usage fault: unaligned access");
            }
            if(excep_store.excep_regs.ufsr.bits.DIVBYZERO)
            {
                ECOMM_TRACE(UNILOG_EXCEP_PRINT, usgaefault_6, P_ERROR, 0, "usage fault: divide by zero");
            }

        }

    }//end of FORCED
}

void show_stack_frame(void)
{
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_1, P_ERROR, 0, "dump regs start: ");
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_2, P_ERROR, 1, "dump reg: r0: 0x%x !",   excep_store.excep_regs.stack_frame.r0 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_3, P_ERROR, 1, "dump reg: r1: 0x%x !",   excep_store.excep_regs.stack_frame.r1 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_4, P_ERROR, 1, "dump reg: r2: 0x%x !",   excep_store.excep_regs.stack_frame.r2 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_5, P_ERROR, 1, "dump reg: r3: 0x%x !",   excep_store.excep_regs.stack_frame.r3 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_6, P_ERROR, 1, "dump reg: r4: 0x%x !",   excep_store.excep_regs.stack_frame.r4 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_7, P_ERROR, 1, "dump reg: r5: 0x%x !",   excep_store.excep_regs.stack_frame.r5 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_8, P_ERROR, 1, "dump reg: r6: 0x%x !",   excep_store.excep_regs.stack_frame.r6 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_9, P_ERROR, 1, "dump reg: r7: 0x%x !",   excep_store.excep_regs.stack_frame.r7 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_10, P_ERROR, 1, "dump reg: r8: 0x%x !",   excep_store.excep_regs.stack_frame.r8 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_11, P_ERROR, 1, "dump reg: r9: 0x%x !",   excep_store.excep_regs.stack_frame.r9 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_12, P_ERROR, 1, "dump reg: r10: 0x%x !",   excep_store.excep_regs.stack_frame.r10 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_13, P_ERROR, 1, "dump reg: r11: 0x%x !",   excep_store.excep_regs.stack_frame.r11 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_14, P_ERROR, 1, "dump reg: r12: 0x%x !",   excep_store.excep_regs.stack_frame.r12 );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_15, P_ERROR, 1, "dump reg: sp:  0x%x !",   excep_store.excep_regs.stack_frame.sp );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_16, P_ERROR, 1, "dump reg: msp: 0x%x !",   excep_store.excep_regs.stack_frame.msp );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_17, P_ERROR, 1, "dump reg: psp: 0x%x !",   excep_store.excep_regs.stack_frame.psp );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_18, P_ERROR, 1, "dump reg: lr: 0x%x !",   excep_store.excep_regs.stack_frame.lr );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_19, P_ERROR, 1, "dump reg: exception pc: 0x%x !",   excep_store.excep_regs.stack_frame.pc );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_20, P_ERROR, 1, "dump reg: psr: 0x%x !",   excep_store.excep_regs.stack_frame.psr );

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_21, P_ERROR, 1, "dump reg: exc_return: 0x%x !",   excep_store.excep_regs.stack_frame.exc_return);

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_22, P_ERROR, 1, "dump reg BASEPRI: 0x%x !",   excep_store.excep_regs.stack_frame.BASEPRI );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_23, P_ERROR, 1, "dump reg PRIMASK: 0x%x !",   excep_store.excep_regs.stack_frame.PRIMASK );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_24, P_ERROR, 1, "dump reg FAULTMASK: 0x%x !",   excep_store.excep_regs.stack_frame.FAULTMASK );
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_frame_25, P_ERROR, 1, "dump reg CONTROL: 0x%x !",   excep_store.excep_regs.stack_frame.CONTROL );

}


static uint32_t dump_ram_to_flash(void)
{
    uint32_t ret = 0;
    uint32_t block_numb = 0;

    uint32_t ram_addr = EC_RAM_START_ADDR;
    uint8_t TempRam0;

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, dump_ram_to_flash_1, P_ERROR, 0, "start dump ram to flash!");

#if (WDT_FEATURE_ENABLE==1)
    WDT_Kick();
#endif

    // erase
    for(block_numb = 0; block_numb < EC_ASSERT_BLOCK_NUMBS; block_numb++)
    {
        ret = hardfault_flash_earse(EC_EXCEP_FLASH_SECTOR_BASE + (block_numb*EC_EXCEP_SECTOR_SIZE));
    }
    if(ret != 0)
    {
        ret = 0;
    }

#if (WDT_FEATURE_ENABLE==1)
    WDT_Kick();
#endif

    // program
    if (ram_addr==0)
    {
        //ram address 0 not accept by flash write api, copy to local and write
        TempRam0 = *(uint8_t*)(ram_addr);
        ret = hardfault_flash_write(&TempRam0, EC_EXCEP_FLASH_SECTOR_BASE, 1);
        ret = hardfault_flash_write((uint8_t*)(ram_addr+1), EC_EXCEP_FLASH_SECTOR_BASE+1, EC_EXCEP_SECTOR_SIZE-1);
        block_numb = 1;
    }

    for(; block_numb < EC_ASSERT_BLOCK_NUMBS; block_numb++)
    {
        ret = hardfault_flash_write((uint8_t*)(ram_addr + (block_numb*EC_EXCEP_SECTOR_SIZE)), (EC_EXCEP_FLASH_SECTOR_BASE + (block_numb*EC_EXCEP_SECTOR_SIZE)), EC_EXCEP_SECTOR_SIZE);
        if(ret != 0)
        {
            ret=0;
        }
    }

    if(ret != 0)
    {
        ret = 0;
    }

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, dump_ram_to_flash_2, P_ERROR, 0, "Finsh dump ram to flash!");

    return ret;

}


void Ec_HardFault_Handler(uint32_t *stack_sp, uint32_t *stack_psp, uint32_t stack_lr)
{
    *((unsigned int *)EC_EXCEPTION_MAGIC_ADDR) = EC_EXCEP_MAGIC_NUMBER;

    uint32_t *stack = stack_sp;
    uint32_t i = 0, atPortBaudRate;
    char * task_name;

#ifdef FUNC_CALL_TRACE
    uint32_t *stack_func;
#endif

#if (WDT_FEATURE_ENABLE==1)
    WDT_Kick();
#endif

	ResetReasonWrite(RESET_REASON_HARDFAULT);

    excep_config_option = (ExcepConfigOp)BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION);

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, hardfault_0, P_ERROR, 1, "Current fault action:%d",excep_config_option);

    __disable_irq();

    if(is_fs_assert())
    {
        // Erase all FS related region
        BSP_QSPI_Erase_Safe(FLASH_MEM_PLAT_INFO_NONXIP_ADDR, FLASH_MEM_PLAT_INFO_SIZE);

#if (WDT_FEATURE_ENABLE==1)
        WDT_Kick();
#endif
        BSP_QSPI_Erase_Safe(FLASH_FS_REGION_OFFSET, FLASH_FS_REGION_SIZE);

		ECOMM_TRACE(UNILOG_EXCEP_PRINT, hardfault_00, P_ERROR, 0, "!!!File System and Plat config clear!!!");
    }

    if(excep_config_option == EXCEP_OPTION_SILENT_RESET)
    {
        *((unsigned int *)EC_EXCEPTION_MAGIC_ADDR) = ~EC_EXCEP_MAGIC_NUMBER;
		EC_SystemReset();
        while(1);
    }

#if (WDT_FEATURE_ENABLE==1)
    WDT_Stop();
#endif

    dump_to_at_port = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_DUMP);

    //stop hardware unilog
    uniLogStopHwLog();

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, hardfault_enter, P_ERROR, 0, "hard fault triggered!!");

    if(dump_to_at_port == 1)
    {
        atPortBaudRate = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE);
        UART_Init(AT_PORT_UART_INSTANCE, atPortBaudRate & 0x7FFFFFFFUL, false);
        UART_Send(AT_PORT_UART_INSTANCE, "!!!Hard fault!!!\r\n", strlen("!!!Hard fault!!!\r\n"), 10000);
    }

	// for uart debug
//	PAD->PCR[20] = (PAD->PCR[20] & ~PAD_PCR_MUX_Msk) | EIGEN_VAL2FLD(PAD_PCR_MUX, 3);

//	UART_Init(AT_PORT_UART_INSTANCE, 115200, false);
//	UART_Printf(AT_PORT_UART_INSTANCE, "abc-%d",dump_to_at_port);

    if(stack_lr & EC_SP_PSP_FLAG)
    {
        stack = stack_psp;
    }
    excep_store.excep_regs.stack_frame.r0        = stack[excep_r0];
    excep_store.excep_regs.stack_frame.r1        = stack[excep_r1];
    excep_store.excep_regs.stack_frame.r2        = stack[excep_r2];
    excep_store.excep_regs.stack_frame.r3        = stack[excep_r3];
    excep_store.excep_regs.stack_frame.r12       = stack[excep_r12];
    excep_store.excep_regs.stack_frame.lr        = stack[excep_lr];
    excep_store.excep_regs.stack_frame.pc        = stack[excep_pc];
    excep_store.excep_regs.stack_frame.psr.value = stack[excep_psr];

    excep_store.excep_regs.stack_frame.sp = (uint32_t)stack + 0x20;// pop exception auto stored context

    excep_store.excep_regs.sys_ctrl_stat.value = EC_REG_SYS_CTRL_STATE;
    excep_store.excep_regs.afar         = EC_REG_AFSR;
    excep_store.excep_regs.mmfar        = EC_REG_MMAR;
    excep_store.excep_regs.mfsr.value   = EC_REG_MFSR;
    excep_store.excep_regs.bfar         = EC_REG_BFAR;
    excep_store.excep_regs.bfsr.value   = EC_REG_BFSR;
    excep_store.excep_regs.ufsr.value   = EC_REG_UFSR;
    excep_store.excep_regs.hfsr.value   = EC_REG_HFSR;
    excep_store.excep_regs.dfsr.value   = EC_REG_DFSR;

#ifdef FUNC_CALL_TRACE
    stack_func = stack+8;
#endif

    //excep_store.ec_start_flag = 0xF2F0F1F3;  //have set in HardFault_Handler
    excep_store.ec_end_flag   = 0xF0F1F2F9;
    excep_store.ec_hardfault_flag = 0xE2E0E1E3;
    excep_store.ec_exception_count++;
    *((unsigned int *)EC_EXCEP_STORE_RAM_ADDR) = (unsigned int)(&excep_store);

    show_stack_frame();

#if defined(__CC_ARM)
#ifdef FUNC_CALL_TRACE
    code_start_addr    = (uint32_t)&CA_CODE_SECTION_START(UNLOAD_IROM);
    code_end_addr      = (uint32_t)&CA_CODE_SECTION_END(UNLOAD_IROM);
#endif
    ec_stack_start_addr  = (uint32_t)&CA_RAM2_SECTION_START(LOAD_DRAM_SHARED);
    ec_stack_end_addr    = (uint32_t)&CA_RAM2_SECTION_ZI_END(LOAD_DRAM_SHARED);
#elif defined(__ICCARM_)
    code_start_addr = (uint32_t)__section_begin(ICA_CODE_NAME);
    code_end_addr   = (uint32_t)__section_end(ICA_CODE_NAME);
#elif defined(__GNUC__)
    extern UINT32 __stack_start;
    extern UINT32 __stack_end;
    ec_stack_start_addr = (uint32_t)(&__stack_start);
    ec_stack_end_addr   = (uint32_t)(&__stack_end);

#else
    //#error "..compiler is error"
#endif

#ifdef FUNC_CALL_TRACE
    check_excep_func_call(excep_store.func_call_stack, EC_FUNC_CALL_ADDR_DEPTH, stack_func);
#endif

    check_fault_type();

    if(stack_lr & EC_SP_PSP_FLAG)
    {
        task_name = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());

        memcpy(excep_store.curr_task_name, task_name, (EC_EXCEP_TASK_NAME_LEN-1));

    }
    else
    {
        excep_store.curr_task_name[0] = 'i';
        excep_store.curr_task_name[1] = 'n';
        excep_store.curr_task_name[2] = 't';
        excep_store.curr_task_name[3] = 'e';
        excep_store.curr_task_name[4] = 'r';
        excep_store.curr_task_name[5] = 'r';
        excep_store.curr_task_name[6] = 'u';
        excep_store.curr_task_name[7] = 'p';
        excep_store.curr_task_name[8] = 't';
    }

    ECOMM_STRING(UNILOG_PLA_STRING, exception_task, P_ERROR, "hardfault task: %s", excep_store.curr_task_name);

    if(dump_to_at_port == 1)
    {
        UART_Send(AT_PORT_UART_INSTANCE, "hardfault task: ", strlen("hardfault task: "), 10000);
        UART_Send(AT_PORT_UART_INSTANCE, excep_store.curr_task_name, strlen((char*)excep_store.curr_task_name), 10000);
        UART_Send(AT_PORT_UART_INSTANCE, "\r\n", 2, 1000);
    }

    stack = (uint32_t *) excep_store.excep_regs.stack_frame.sp;

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_dump_1, P_ERROR, 1, "dump latest %d words stack start", EC_EXCEP_STACK_DEPTH);

    for(i = 0; i < EC_EXCEP_STACK_DEPTH/4; i++)
    {
        ECOMM_TRACE(UNILOG_EXCEP_PRINT, stack_dump_2, P_ERROR, 4, "dump stack frame: 0x%x  0x%x  0x%x  0x%x", stack[4*i],stack[4*i+1],stack[4*i+2],stack[4*i+3]);
    }

    if((excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP) ||
       (excep_config_option == EXCEP_OPTION_DUMP_FLASH_RESET)     ||
       (excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_RESET))
    {
        dump_ram_to_flash();
    }

    if((excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP) || (excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_RESET))
    {
        if(uniLogIsInitialized())
            EcDumpTopFlow();
    }

    if((excep_config_option == EXCEP_OPTION_DUMP_FLASH_RESET) ||
       (excep_config_option == EXCEP_OPTION_PRINT_RESET)     ||
       (excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_RESET))
    {
        *((unsigned int *)EC_EXCEPTION_MAGIC_ADDR) = ~EC_EXCEP_MAGIC_NUMBER;
		EC_SystemReset();
    }

    while(1);
}




void unilogAssertInfo(const char *func, uint32_t line, const char *file, uint32_t v1, uint32_t v2, uint32_t v3)
{
    if (func != PNULL)
    {
        ECOMM_STRING(UNILOG_PLA_STRING, unilogAssertInfo_1, P_INFO, "ASSERT, FUNC: %s", (UINT8 *)func);
    }

    if (file != PNULL)
    {
        ECOMM_STRING(UNILOG_PLA_STRING, unilogAssertInfo_2, P_INFO, "ASSERT, FILE : %s", (UINT8 *)file);
    }

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, unilogAssertInfo_3, P_INFO, 4, "ASSERT line: %d, val is 0x%x,0x%x,0x%x", line, v1, v2, v3);

    return;
}

void ec_assert_task_inter_info(void)
{
    uint32_t PSP = 0;
    uint32_t MSP = 0;
    uint32_t SP = 0;
    char * task_name;
    char task_name_buf[EC_EXCEP_TASK_NAME_LEN] = {0};

#if defined ( __CC_ARM )
    PSP = __get_PSP();
    MSP = __get_MSP();
    SP = __current_sp();

#elif defined(__GNUC__)
#include "cmsis_gcc.h"

    PSP = __get_PSP();
    MSP = __get_MSP();
    SP = __current_sp();
#endif

    if(PSP == SP)
    {
        task_name = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());
        memcpy(task_name_buf, task_name, (EC_EXCEP_TASK_NAME_LEN-1));
        ECOMM_STRING(UNILOG_PLA_STRING, assert_task_onfo_1, P_ERROR, "Assert occur in task context, task name :%s", (UINT8 *)task_name_buf);
    }
    else if(MSP == SP)
    {
        ECOMM_TRACE(UNILOG_EXCEP_PRINT, assert_task_onfo_2, P_ERROR, 0,"Assert occur in interrupt context");
    }
}

void ec_assert(const char *func, uint32_t line, const char *file, uint32_t v1, uint32_t v2, uint32_t v3)
{
    uint32_t i = 0;
    uint32_t *sp_temp = NULL;
    uint8_t* outputlog = NULL;
    uint32_t atPortBaudRate;

#ifdef FUNC_CALL_TRACE
    uint32_t func_call_flag = 0;
    uint32_t func_call_addr[EC_ASSERT_FUNC_CALL_ADDR_DEPTH] = {0};
    uint32_t addr_temp = 0;
    uint32_t *stack_end_addr = NULL;
#endif

#if (WDT_FEATURE_ENABLE==1)
    WDT_Kick();
#endif

	ResetReasonWrite(RESET_REASON_ASSERT);

    __disable_irq();

    excep_config_option = (ExcepConfigOp)BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION);

    if(excep_config_option == EXCEP_OPTION_SILENT_RESET)
    {
        // no more other operations are allowded for this option
        dump_to_at_port = 0;
    }
    else
    {
        dump_to_at_port = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_DUMP);

        excep_store.ec_start_flag = 0xA2A0A1A3;
        excep_store.ec_assert_flag = 0xB2B0B1B3;
        excep_store.ec_exception_count++;

        is_fs_assert_flag = false;

        if(dump_to_at_port == 1)
        {
            atPortBaudRate = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE);
            UART_Init(AT_PORT_UART_INSTANCE, atPortBaudRate & 0x7FFFFFFFUL, false);
        }
    }

    // Assert triggered by FS
    if((v1 == EC_FS_ASSERT_MAGIC_NUMBER0) && (v2 == EC_FS_ASSERT_MAGIC_NUMBER1) && (v3 == EC_FS_ASSERT_MAGIC_NUMBER2))
    {
        is_fs_assert_flag = true;

#if (WDT_FEATURE_ENABLE==1)
        WDT_Kick();
#endif
        fs_assert_count = BSP_GetFSAssertCount();
        BSP_SetFSAssertCount(++fs_assert_count);

        if(dump_to_at_port == 1)
        {
            if(fs_assert_count && ((fs_assert_count % EC_FS_ASSERT_REFORMAT_THRESHOLD) == 0))
            {
                outputlog = "FS assert count reaches threshold, FS region will be reformated on next power up!!!\r\n";
                UART_Send(AT_PORT_UART_INSTANCE, outputlog, strlen((char*)outputlog), 30000);
            }
        }
    }

    if(excep_config_option == EXCEP_OPTION_SILENT_RESET)
    {
        *((unsigned int *)EC_EXCEPTION_MAGIC_ADDR) = ~EC_EXCEP_MAGIC_NUMBER;

		EC_SystemReset();
        while(1);
    }

#if (WDT_FEATURE_ENABLE==1)
    WDT_Stop();
#endif

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, ec_assert_0, P_ERROR, 1, "Current fault action:%d",excep_config_option);

    //stop hardware unilog
    uniLogStopHwLog();

    snprintf(ec_assert_buff, sizeof(ec_assert_buff),"!!!Assert!!!\r\nFunc:%s\r\nLine:%d\r\nVal:0x%x,0x%x,0x%x\r\n", func, (int)line, v1, v2, v3);

    if(dump_to_at_port == 1)
    {
        UART_Send(AT_PORT_UART_INSTANCE, (uint8_t*)ec_assert_buff, strlen(ec_assert_buff), 30000);
    }

    ec_assert_task_inter_info();

    unilogAssertInfo(func, line, file, v1, v2, v3);

#if defined(__CC_ARM)

#ifdef FUNC_CALL_TRACE
    code_start_addr    = (uint32_t)&CA_CODE_SECTION_START(UNLOAD_IROM);
    code_end_addr      = (uint32_t)&CA_CODE_SECTION_END(UNLOAD_IROM);
#endif  // FUNC_CALL_TRACE
    ec_stack_start_addr  = (uint32_t)&CA_RAM2_SECTION_START(LOAD_DRAM_SHARED);
    ec_stack_end_addr    = (uint32_t)&CA_RAM2_SECTION_ZI_END(LOAD_DRAM_SHARED);
#elif defined(__ICCARM_)
    code_start_addr = (uint32_t)__section_begin(ICA_CODE_NAME);
    code_end_addr   = (uint32_t)__section_end(ICA_CODE_NAME);
#elif defined(__GNUC__)
    extern UINT32 __stack_start;
    extern UINT32 __stack_end;
    ec_stack_start_addr = (uint32_t)(&__stack_start);
    ec_stack_end_addr   = (uint32_t)(&__stack_end);
#else
    //#error "..compiler is error"
#endif

#ifdef FUNC_CALL_TRACE
    stack_end_addr = (uint32_t *)(ec_stack_end_addr);
    sp_temp = (uint32_t *)excep_store.excep_regs.stack_frame.sp;

    for(;sp_temp < stack_end_addr; sp_temp++)
    {
        addr_temp = *((uint32_t *)sp_temp);
        if((code_start_addr <= addr_temp)&&(addr_temp <= code_end_addr)&&(func_call_flag == 0))
        {
            excep_store.excep_regs.stack_frame.lr = addr_temp;
            func_call_addr[func_call_flag] = addr_temp;
            func_call_flag++;
        }
        else if((code_start_addr <= addr_temp)&&(addr_temp <= code_end_addr))
        {
            func_call_addr[func_call_flag] = addr_temp;
            func_call_flag++;
        }
        if(EC_ASSERT_FUNC_CALL_ADDR_DEPTH == func_call_flag)
        {
            break;
        }
    }
#endif

    excep_store.excep_regs.stack_frame.r12      = 0x0;
    excep_store.excep_regs.stack_frame.pc       = *((unsigned int *)EC_ASSERT_PC_ADDR) - 0xA;
    excep_store.ec_end_flag   = 0xA0A1A2A9;

    *((unsigned int *)EC_EXCEP_STORE_RAM_ADDR) = (unsigned int)(&excep_store);

    show_stack_frame();

#ifdef FUNC_CALL_TRACE
    ECOMM_TRACE(UNILOG_EXCEP_PRINT, ec_assert_1, P_ERROR, 0, "try to parse assert call stack by address compare!");

    for(i=0; i<EC_ASSERT_FUNC_CALL_ADDR_DEPTH; i++)
    {
        ECOMM_TRACE(UNILOG_EXCEP_PRINT, ec_assert_2, P_ERROR, 1,"maybe function address @ 0x%x", func_call_addr[i]);
    }
#endif

    ECOMM_TRACE(UNILOG_EXCEP_PRINT, ec_assert_3, P_ERROR, 1, "dump latest %d words stack start", EC_EXCEP_STACK_DEPTH);

    sp_temp = (uint32_t *)excep_store.excep_regs.stack_frame.sp;

    for(i=0; i<EC_EXCEP_STACK_DEPTH/4; i++)
    {
        ECOMM_TRACE(UNILOG_EXCEP_PRINT, ec_assert_4, P_ERROR, 4, "dump stack frame:0x%x  0x%x  0x%x  0x%x", sp_temp[4*i],sp_temp[4*i+1],sp_temp[4*i+2],sp_temp[4*i+3]);
    }

    if((excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP) ||
       (excep_config_option == EXCEP_OPTION_DUMP_FLASH_RESET)     ||
       (excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_RESET))
    {
        dump_ram_to_flash();
    }

    if((excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP) || (excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_RESET))
    {
        if(uniLogIsInitialized())
            EcDumpTopFlow();
    }

    if((excep_config_option == EXCEP_OPTION_DUMP_FLASH_RESET) ||
       (excep_config_option == EXCEP_OPTION_PRINT_RESET)      ||
       (excep_config_option == EXCEP_OPTION_DUMP_FLASH_EPAT_RESET))
    {
        *((unsigned int *)EC_EXCEPTION_MAGIC_ADDR) = ~EC_EXCEP_MAGIC_NUMBER;
		EC_SystemReset();
    }

    while(1);
}


BOOL is_in_excep_handler(void)
{
    if(*((unsigned int *)EC_EXCEPTION_MAGIC_ADDR)==EC_EXCEP_MAGIC_NUMBER)
        return TRUE;
    else
        return FALSE;

}

BOOL is_fs_assert(void)
{
    return is_fs_assert_flag;
}


