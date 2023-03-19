BL_DIR_R   = PLAT/project/$(TARGET)/apps/bootloader/code
BLBUILDDIR = $(TOP)/PLAT/out/$(TARGET)/$(PROJECT)

CFLAGS_INC += -I $(TOP)/$(BL_DIR_R)/include/core_inc	\
	-I $(TOP)/$(BL_DIR_R)/include	\
	-I $(TOP)/$(BL_DIR_R)/include/common	\
	-I $(TOP)/$(BL_DIR_R)/common/gpp  	\
	-I $(TOP)/$(BL_DIR_R)/common/secure/hash/inc	\
	-I $(TOP)/$(BL_DIR_R)/common/secure/ecc/inc 
	
	
ifeq ($(THIRDPARTY_ABUP_FOTA_BL_ENABLE), y)	
CFLAGS_INC +=	-I $(TOP)/$(BL_DIR_R)/abup_bl/wosun/inc
endif	

#overwrite  SYSTEM_FILE_PATH in device\eigencomm\Makefile.inc 
SYSTEM_FILE_PATH = PLAT/project/$(TARGET)/apps/bootloader/code/main

ifeq ($(TOOLCHAIN),ARMCC)
CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil
STARTUP_FILE_PATH  =PLAT/project/$(TARGET)/apps/bootloader/ARMCC
PRECINIT_FILE_PATH =PLAT/project/$(TARGET)/apps/bootloader/ARMCC
LINK_FILE_PATH  =PLAT/project/$(TARGET)/apps/bootloader/ARMCC
endif

ifeq ($(TOOLCHAIN),GCC)
CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc
STARTUP_FILE_PATH  =PLAT/project/$(TARGET)/apps/bootloader/GCC
PRECINIT_FILE_PATH =PLAT/project/$(TARGET)/apps/bootloader/GCC
LINK_FILE_PATH  =PLAT/project/$(TARGET)/apps/bootloader/GCC
endif

SYSTEM_VERIFY_KEY_FILE_OBJ ?= 

ifeq ($(THIRDPARTY_ABUP_FOTA_BL_ENABLE), y)	
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/abup_wosun.lib
endif
ifeq ($(MIDDLEWARE_FOTA_ENABLE), y)	
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libfota.lib
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/deltapatch.lib
obj-y += PLAT/project/$(TARGET)/apps/bootloader/code/main/fota/fota_entry.o
endif
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libbootloader.lib

obj-y += PLAT/project/$(TARGET)/apps/bootloader/code/main/ec_main.o \
         PLAT/project/$(TARGET)/apps/bootloader/code/main/stub_os.o 
ifeq ($(THIRDPARTY_ABUP_FOTA_BL_ENABLE), y)	
obj-y += PLAT/project/$(TARGET)/apps/bootloader/code/abup_bl/wosun/src/adups_bl_main.o \
         PLAT/project/$(TARGET)/apps/bootloader/code/abup_bl/wosun/src/adups_bl_flash.o 
endif

obj-y += $(SYSTEM_VERIFY_KEY_FILE_OBJ)


