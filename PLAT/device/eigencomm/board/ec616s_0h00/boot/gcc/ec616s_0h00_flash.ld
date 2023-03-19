
/* Entry Point */
ENTRY(Reset_Handler)

/* Specify the memory areas */
MEMORY
{
  LOAD_IRAM1(rwx)      				: ORIGIN = 0x00004000, LENGTH = 20K
  LOAD_IRAM2(rwx)      				: ORIGIN = 0x00009000, LENGTH = 12K
  LOAD_DRAM_SHARED(rwx)				: ORIGIN = 0x0000C000, LENGTH = 160K
  
  LOAD_DRAM_MCURESEVED(rwx)		: ORIGIN = 0x0003B000, LENGTH = 18K
  UNLOAD_DRAM_USRNV(rwx)			: ORIGIN = 0x0003F800, LENGTH = 2K
  LOAD_DRAM_BSP(rwx)					: ORIGIN = 0x00040000, LENGTH = 8K
  UNLOAD_DRAM_FLASHMEM(rwx)		: ORIGIN = 0x00042000, LENGTH = 7K
  UNLOAD_DRAM_SLPMEM(rwx)		  : ORIGIN = 0x00043C00, LENGTH = 1K-32

  LOAD_IRAM_MCUVECTOR(rwx)		: ORIGIN = 0x00000000, LENGTH = 100
  LOAD_IRAM_MCU(rwx)					: ORIGIN = 0x00000064, LENGTH = 4K-100
  LOAD_DRAM_MCU(rwx)					: ORIGIN = 0x00001000, LENGTH = 6K
  UNLOAD_DRAM_PSPHYRET(rwx)		: ORIGIN = 0x00002800, LENGTH = 6K
  
  FLASH_APP (rx)     : ORIGIN = 0x00820000, LENGTH = 2560K		/* Use only the first bank */
}

/* Define output sections */
SECTIONS
{
  . = ORIGIN(FLASH_APP);
  .isr_vector :
  {
    . = ALIGN(8);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(8);
  } >FLASH_APP AT>FLASH_APP
  
  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(8);
    *(EXCLUDE_FILE(*flash_ec616s.o *qspi_ec616s.o) .text*)
    *(.eh_frame)
    
    KEEP (*(.init))
    KEEP (*(.fini))    
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(8);
    
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH_APP AT>FLASH_APP


  /* used by the startup to initialize data */

  Load$$LOAD_IRAM1$$Base = LOADADDR(.ramcode1);
  .ramcode1 ORIGIN(LOAD_IRAM1):
  {
    Image$$LOAD_IRAM1$$Base = .;
    *(.ramCode)
    *libc_nano.a:lib_a-memcpy-stub.o(.text*)
    Image$$LOAD_IRAM1$$Limit = .;
  } >LOAD_IRAM1 AT>FLASH_APP

  Load$$LOAD_IRAM2$$Base = LOADADDR(.ramcode2);
  .ramcode2 ORIGIN(LOAD_IRAM2):
  {
    Image$$LOAD_IRAM2$$Base = .;
    *(.ramCode2)
    Image$$LOAD_IRAM2$$Limit = .;
  } >LOAD_IRAM2 AT>FLASH_APP
	
	
  Load$$LOAD_DRAM_SHARED$$Base = LOADADDR(.load_dram_shared);
  .load_dram_shared ORIGIN(LOAD_DRAM_SHARED):
  {
    Image$$LOAD_DRAM_SHARED$$Base = .;
    *(.RTTmem)
    
    *(EXCLUDE_FILE(*libdriver.a: *libdriver_private.a: *libdriver_private_dep.a:) .data*)
    
    *(COMMON)
    Image$$LOAD_DRAM_SHARED$$Limit = .;

  } >LOAD_DRAM_SHARED AT>FLASH_APP
	
  Load$$LOAD_DRAM_SHARED2$$Base = LOADADDR(.bss_data);
  .bss_data (NOLOAD):
  {
    Image$$LOAD_DRAM_SHARED$$ZI$$Base = .;
    *(EXCLUDE_FILE(*libdriver.a: *libdriver_private.a: *libdriver_private_dep.a:) .bss*)
    Image$$LOAD_DRAM_SHARED$$ZI$$Limit = .;
    __stack_start = .;
    *(.stack)
    __stack_end = .;
  }>LOAD_DRAM_SHARED AT>FLASH_APP


  Load$$LOAD_DRAM_MCURESEVED$$Base = LOADADDR(.load_dram_mcureseved);
  .load_dram_mcureseved ORIGIN(LOAD_DRAM_MCURESEVED):
  {
    Image$$LOAD_DRAM_MCU$$Base = .;
    *(.mcuCode)									
    *(.mcuRWData)
    Image$$LOAD_DRAM_MCU$$Limit = .;
    . = ALIGN(4);
    Image$$LOAD_DRAM_MCU$$ZI$$Base = .;
    *(.mcuZIData)									/* bss */
    Image$$LOAD_DRAM_MCU$$ZI$$Limit = .;
  } >LOAD_DRAM_MCURESEVED AT>FLASH_APP
	
  .unload_dram_usrnv ORIGIN(UNLOAD_DRAM_USRNV)  (NOLOAD):
  {
    *(.usrNvMem)									/* bss */
  } >UNLOAD_DRAM_USRNV
	
  Load$$LOAD_DRAM_BSP$$Base = LOADADDR(.load_dram_bsp);
  .load_dram_bsp ORIGIN(LOAD_DRAM_BSP):
  {
    Image$$LOAD_DRAM_BSP$$Base = .;
    *libdriver.a: (.data*)
    *libdriver_private.a: (.data*)
    *libdriver_private_dep.a: (.data*)
    Image$$LOAD_DRAM_BSP$$Limit = .;
    . = ALIGN(4);
    Image$$LOAD_DRAM_BSP$$ZI$$Base = .;
    *libdriver.a: (.bss*)
    *libdriver_private.a: (.bss*)
    *libdriver_private_dep.a: (.bss*)
    Image$$LOAD_DRAM_BSP$$ZI$$Limit = .;
  } >LOAD_DRAM_BSP AT>FLASH_APP
	
  .unload_dram_flashmem ORIGIN(UNLOAD_DRAM_FLASHMEM)  (NOLOAD):
  {
    *(.flashbackupdata)
  } >UNLOAD_DRAM_FLASHMEM
	
  .unload_dram_slpmem ORIGIN(UNLOAD_DRAM_SLPMEM)  (NOLOAD):
  {
    *(.sleepmem)				/* bss */
    *(.ctTimermem)			/* bss */
    *(.swcnt)						/* bss */
  } >UNLOAD_DRAM_SLPMEM
	
  Load$$LOAD_IRAM_MCUVECTOR$$Base = LOADADDR(.load_iram_mcuvector);
  .load_iram_mcuvector ORIGIN(LOAD_IRAM_MCUVECTOR):
  {
    Image$$LOAD_IRAM_MCUVECTOR$$Base = .;
    *(.mcuVector)
    Image$$LOAD_IRAM_MCUVECTOR$$Limit = .;
  } >LOAD_IRAM_MCUVECTOR AT>FLASH_APP
	
  Load$$LOAD_IRAM_MCU$$Base = LOADADDR(.load_iram_mcu);
  .load_iram_mcu ORIGIN(LOAD_IRAM_MCU):
  {
    Image$$LOAD_IRAM_MCU$$Base = .;
    KEEP(*(.ramBootCode))
    *(.mcuCode)
    *flash_ec616s.o(.text*)
    *qspi_ec616s.o(.text*)
    *(.pre1RamCode)
    *(.pre2RamCode)
    Image$$LOAD_IRAM_MCU$$Limit = .;
  } >LOAD_IRAM_MCU AT>FLASH_APP
	
  Load$$LOAD_DRAM_MCU$$Base = LOADADDR(.load_dram_mcu);
  .load_dram_mcu ORIGIN(LOAD_DRAM_MCU):
  {
    *(.ramBootRWData)
    *(.ramBootZIData)

  } >LOAD_DRAM_MCU AT>FLASH_APP
	
  Load$$UNLOAD_DRAM_PSPHYRET$$Base = LOADADDR(.unload_dram_psphyret);
  .unload_dram_psphyret ORIGIN(UNLOAD_DRAM_PSPHYRET)  (NOLOAD):
  {
    Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Base = .;
    *(.psphyret_6k)				/* bss */
    Image$$UNLOAD_DRAM_PSPHYRET$$ZI$$Limit = .;
  } >UNLOAD_DRAM_PSPHYRET AT>FLASH_APP

}

GROUP(
    libgcc.a
    libc.a
    libm.a
 )
