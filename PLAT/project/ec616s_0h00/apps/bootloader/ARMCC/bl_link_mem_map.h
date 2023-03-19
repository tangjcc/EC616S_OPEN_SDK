
#ifndef BL_LINK_MEM_MAP_H
#define BL_LINK_MEM_MAP_H
//included in bootloader ec616_0h00_flash.sct for link only


#define FLASH_XIP_ADDR                  0x00800000

#define BOOTLOADER_FLASH_LOAD_ADDR      (FLASH_XIP_ADDR+0x4000)
#define BOOTLOADER_FLASH_LOAD_SIZE      0x00012000


#endif
