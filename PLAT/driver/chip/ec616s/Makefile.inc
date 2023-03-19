
CFLAGS_INC += -I$(TOP)/PLAT/driver/board/$(TARGET)/inc       \
              -I$(TOP)/PLAT/driver/chip/$(CHIP)/inc          \
              -I$(TOP)/PLAT/driver/chip/$(CHIP)/inc_cmsis    \
              -I$(TOP)/PLAT/os/freertos/CMSIS/inc            \
              -I$(TOP)/PLAT/os/freertos/inc                  \
              -I$(TOP)/PLAT/os/osa/inc


ifeq ($(DRIVER_PMU_ENABLE),y)  
    CFLAGS      += -DPM_FEATURE_ENABLE
endif

ifeq ($(DRIVER_UNILOG_ENABLE),y)  
    CFLAGS      += -DUINILOG_FEATURE_ENABLE
endif
libdriver-$(DRIVER_USART_ENABLE)         += PLAT/driver/chip/$(CHIP)/src_cmsis/bsp_usart.o 

libdriver-$(DRIVER_LPUART_ENABLE)        += PLAT/driver/chip/$(CHIP)/src_cmsis/bsp_lpusart.o											
											
libdriver-$(DRIVER_ADC_ENABLE)           += PLAT/driver/chip/$(CHIP)/src/adc_$(CHIP).o
libdriver-$(DRIVER_SPI_ENABLE)           += PLAT/driver/chip/$(CHIP)/src_cmsis/bsp_spi.o
libdriver-$(DRIVER_I2C_ENABLE)           += PLAT/driver/chip/$(CHIP)/src_cmsis/bsp_i2c.o
libdriver-$(DRIVER_GPIO_ENABLE)          += PLAT/driver/chip/$(CHIP)/src/gpio_$(CHIP).o          \
                                            PLAT/driver/chip/$(CHIP)/src/pad_$(CHIP).o
libdriver-$(DRIVER_TIMER_ENABLE)         += PLAT/driver/chip/$(CHIP)/src/timer_$(CHIP).o
libdriver-$(DRIVER_WDG_ENABLE)           += PLAT/driver/chip/$(CHIP)/src/wdt_$(CHIP).o
libdriver-y                              += PLAT/driver/chip/$(CHIP)/src/clock_$(CHIP).o

libdriverprivate-y                       += PLAT/driver/chip/$(CHIP)/src/uart_$(CHIP).o
libdriverprivate-$(DRIVER_UNILOG_ENABLE) += PLAT/driver/chip/$(CHIP)/src/unilog_$(CHIP).o
libdriverprivate-$(DRIVER_EFUSE_ENABLE)  += PLAT/driver/chip/$(CHIP)/src/efuse_$(CHIP).o
libdriverprivate-$(DRIVER_CIPHER_ENABLE) += PLAT/driver/chip/$(CHIP)/src/l2cflh_$(CHIP).o       \
                                            PLAT/driver/chip/$(CHIP)/src/l2ctls_$(CHIP).o
libdriverprivate-$(DRIVER_RNG_ENABLE)    += PLAT/driver/chip/$(CHIP)/src/rng_$(CHIP).o
libdriverprivate-$(DRIVER_CACHE_ENABLE)  += PLAT/driver/chip/$(CHIP)/src/cache_$(CHIP).o

libdriverprivate-$(DRIVER_FLASHRT_ENABLE)+= PLAT/driver/chip/$(CHIP)/src/flash_$(CHIP)_rt.o    \
                                            PLAT/driver/chip/$(CHIP)/src/batmonos_$(CHIP).o
libdriverprivate-$(DRIVER_FLASH_ENABLE)  += PLAT/driver/chip/$(CHIP)/src/qspi_$(CHIP).o           \
                                            PLAT/driver/chip/$(CHIP)/src/flash_$(CHIP).o          \
											PLAT/driver/chip/$(CHIP)/src/batmonraw_$(CHIP).o


libdriverprivate-$(DRIVER_IC_ENABLE)     += PLAT/driver/chip/$(CHIP)/src/ic_$(CHIP).o
libdriverprivate-$(DRIVER_GPR_ENABLE)    += PLAT/driver/chip/$(CHIP)/src/gpr_$(CHIP).o
libdriverprivate-$(DRIVER_DMA_ENABLE)    += PLAT/driver/chip/$(CHIP)/src/dma_$(CHIP).o
libdriverprivate-$(DRIVER_PMU_ENABLE)       += PLAT/driver/chip/$(CHIP)/src/ecpm_$(CHIP).o          \
                                            PLAT/driver/chip/$(CHIP)/src/pmu_$(CHIP).o            \
                                            PLAT/driver/chip/$(CHIP)/src/swcnt_$(CHIP).o          \
                                            PLAT/driver/chip/$(CHIP)/src/ramboot_$(CHIP).o        \
                                           PLAT/driver/chip/$(CHIP)/src/timetrans_$(CHIP).o      \
                                           PLAT/driver/chip/$(CHIP)/src/hibtimer_$(CHIP).o       \
                                           PLAT/driver/chip/$(CHIP)/src/slpman_$(CHIP).o  



