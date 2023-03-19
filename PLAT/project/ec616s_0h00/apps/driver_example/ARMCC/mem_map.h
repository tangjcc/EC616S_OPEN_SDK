
#ifndef MEM_MAP_H
#define MEM_MAP_H



#define FLASH_XIP_ADDR                  0x00800000


#define APP_FLASH_LOAD_ADDR             (FLASH_XIP_ADDR+0x20000)
#define APP_FLASH_LOAD_SIZE             0x200000

#define BOOTLOADER_FLASH_LOAD_ADDR      (FLASH_XIP_ADDR+0x4000)
#define BOOTLOADER_FLASH_LOAD_SIZE      0x00012000

#define FLASH_MEM_BACKUP_ADDR           (FLASH_XIP_ADDR+0x36A000)
#define FLASH_MEM_BACKUP_SIZE           0x4000

#define HEAP_MEM_MAX_START_ADDR             0x32000///0x33000

/////////////////////////////////////////////////


#endif
