
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = mqtt-demo-flash
TOP               := ../../../../../..

BUILD_AT                 = n
BUILD_AT_DEBUG           = n
THIRDPARTY_MQTT_ENABLE   = y
THIRDPARTY_CJSON_ENABLE  = y
THIRDPARTY_MBEDTLS_ENABLE  = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/mqtt_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/mqtt_example/src/bsp_custom.o
include $(TOP)/PLAT/tools/scripts/Makefile.rules
