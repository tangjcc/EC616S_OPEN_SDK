ifeq ($(BUILD_OS),y)
include $(TOP)/PLAT/os/freertos/Makefile.inc
ifeq ($(BUILD_USE_PREBUILD_LIB),n)
include $(TOP)/PLAT/os/osa/Makefile.inc
endif
endif