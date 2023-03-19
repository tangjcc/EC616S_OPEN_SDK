CFLAGS_INC += -I$(TOP)/PLAT/driver/hal/common/inc \
              -I$(TOP)/PLAT/driver/hal/$(CHIP)/inc \

DRIVER_HAL_ADC_ENABLE ?= n
DRIVER_HAL_UART_ENABLE ?= y

ifeq ($(DRIVER_EFUSE_ENABLE)_$(DRIVER_ADC_ENABLE),y_y)  
    DRIVER_HAL_ADC_ENABLE = y
endif

ifeq ($(DRIVER_HAL_ADC_ENABLE), y)
libdriver-y += PLAT/driver/hal/$(CHIP)/src/hal_adc.o
endif

libdriver-y += PLAT/driver/hal/$(CHIP)/src/hal_misc.o 

ifeq ($(DRIVER_HAL_UART_ENABLE), y)
libdriver-y += PLAT/driver/hal/$(CHIP)/src/hal_uart.o 
endif

libdriver-y += PLAT/driver/hal/common/src/ring_buffer.o \
               PLAT/driver/hal/common/src/os_exception.o
libdriverprivate-$(BUILD_OS) += PLAT/driver/hal/common/src/exception_dump.o
