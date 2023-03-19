
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
TOP               := ../../../../../..

BINNAME           = driver_example

BUILD_AT                  = n
BUILD_AT_DEBUG            = n

DRIVER_EFUSE_ENABLE         = y
DRIVER_CIPHER_ENABLE        = y
DRIVER_RNG_ENABLE           = n

THIRDPARTY_CISONENET_ENABLE = n
THIRDPARTY_CJSON_ENABLE     = n
THIRDPARTY_RTT_ENABLE       = n

MIDDLEWARE_DEBUG_TRACE_ENABLE = n
MIDDLEWARE_RUNNV_ENABLE       = n

DRIVER_MCUMODE_ENABLE         = y
DRIVER_ECMAIN_ENABLE          = n

ifeq ($(TOOLCHAIN),ARMCC)
PRECINIT_FILE_PATH = PLAT/project/$(TARGET)/apps/driver_example/ARMCC
SYSTEM_FILE_PATH   = PLAT/project/$(TARGET)/apps/driver_example/ARMCC
STARTUP_FILE_PATH  = PLAT/project/$(TARGET)/apps/driver_example/ARMCC
LINK_FILE_PATH     = PLAT/project/$(TARGET)/apps/driver_example/ARMCC
endif

ifeq ($(TOOLCHAIN),GCC)
PRECINIT_FILE_PATH = PLAT/project/$(TARGET)/apps/driver_example/GCC
SYSTEM_FILE_PATH   = PLAT/project/$(TARGET)/apps/driver_example/GCC
STARTUP_FILE_PATH  = PLAT/project/$(TARGET)/apps/driver_example/GCC
LINK_FILE_PATH     = PLAT/project/$(TARGET)/apps/driver_example/GCC
endif

obj-y             += PLAT/project/$(TARGET)/apps/driver_example/src/usart_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/dma_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/timer_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/wdt_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/gpio_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/spi_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/i2c_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/adc_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/bsp_custom.o \
					 PLAT/project/$(TARGET)/apps/driver_example/src/tls_demo.o
					 

include $(TOP)/PLAT/tools/scripts/Makefile.rules

CFLAGS_INC += -I ../inc
