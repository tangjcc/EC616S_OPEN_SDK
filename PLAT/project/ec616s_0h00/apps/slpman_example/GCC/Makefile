
AVAILABLE_TARGETS = ec616_0h00
TOOLCHAIN         = GCC
BINNAME           = slpman-demo-flash
TOP               := ../../../../../..


THIRDPARTY_CISONENET_ENABLE = y
THIRDPARTY_ERCOAP_ENABLE = y


CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/slpman_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/slpman_example/src/bsp_custom.o
                     

#CONFIG_PROJ_APP_SECURITY_BOOT  = y

include $(TOP)/PLAT/project/$(TARGET)/apps/$(PROJECT)/gcc/MakeProj.rules
include $(TOP)/PLAT/tools/scripts/Makefile.rules

ifneq ($(BUILD_AT),y)
$(error This example needs to modify "BUILD_AT" to "y" in device\eigencomm\board\$(TARGET)\ec616_0h00.mk)
endif

