
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = slpman-demo-flash
TOP               := ../../../../../..

BUILD_AT                  = n
BUILD_AT_DEBUG            = n
THIRDPARTY_ERCOAP_ENABLE   = y
THIRDPARTY_LIBCOAP_ENABLE  = y
THIRDPARTY_MQTT_ENABLE     = y
THIRDPARTY_HTTPC_ENABLE    = y
THIRDPARTY_MBEDTLS_ENABLE  = y
THIRDPARTY_TINYDTLS_ENABLE = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/slpman_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/slpman_example/src/bsp_custom.o                     

#CONFIG_PROJ_APP_SECURITY_BOOT  = y

include $(TOP)/PLAT/project/$(TARGET)/apps/$(PROJECT)/armcc/MakeProj.rules
include $(TOP)/PLAT/tools/scripts/Makefile.rules
