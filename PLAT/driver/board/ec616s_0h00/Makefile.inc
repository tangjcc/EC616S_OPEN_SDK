CFLAGS_INC += -I$(TOP)/PLAT/driver/board/$(TARGET)/inc

libdriver-y                                   += PLAT/driver/board/$(TARGET)/src/bsp.o
libdriver-$(THIRDPARTY_LITTEFS_ENABLE)        += PLAT/driver/board/$(TARGET)/src/plat_config.o
libdriverprivate-$(DRIVER_ECMAIN_ENABLE)   += PLAT/driver/board/$(TARGET)/src/ec_main.o