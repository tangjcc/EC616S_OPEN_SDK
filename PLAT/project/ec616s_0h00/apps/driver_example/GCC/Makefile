AVAILABLE_TARGETS = ec616_0h00

TOOLCHAIN = GCC
TOP := ../../../../../..

BINNAME           = driver_example

BUILD_AT                  = n
BUILD_AT_DEBUG            = n

obj-y             += PLAT/project/$(TARGET)/apps/driver_example/src/usart_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/dma_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/timer_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/wdt_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/gpio_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/spi_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/i2c_demo.o \
                     PLAT/project/$(TARGET)/apps/driver_example/src/app.o \
		     PLAT/project/$(TARGET)/apps/driver_example/src/bsp_custom.o

include $(TOP)/PLAT/tools/scripts/Makefile.rules
