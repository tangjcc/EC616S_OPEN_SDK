
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = GCC
BINNAME           = multi_at-demo-flash
TOP               := ../../../../../..


THIRDPARTY_CTWING1_5_ENABLE   = y
THIRDPARTY_ERCOAP_ENABLE   = y
THIRDPARTY_CISONENET_ENABLE = y
THIRDPARTY_LIBCOAP_ENABLE  = y
THIRDPARTY_MQTT_ENABLE     = y
THIRDPARTY_HTTPC_ENABLE    = y
THIRDPARTY_MBEDTLS_ENABLE  = y
THIRDPARTY_TINYDTLS_ENABLE = y
THIRDPARTY_CJSON_ENABLE = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/multi_at_command/src/app.o \
                     PLAT/project/$(TARGET)/apps/multi_at_command/src/bsp_custom.o                     

#CONFIG_PROJ_APP_SECURITY_BOOT  = y

include $(TOP)/PLAT/project/$(TARGET)/apps/$(PROJECT)/GCC/MakeProj.rules
include $(TOP)/PLAT/tools/scripts/Makefile.rules

#enable WDT
CFLAGS += -DWDT_FEATURE_ENABLE=1

ifneq ($(BUILD_AT),y)
$(error This example needs to modify "BUILD_AT" to "y" in device\eigencomm\board\$(TARGET)\ec616s_0h00.mk)
endif

