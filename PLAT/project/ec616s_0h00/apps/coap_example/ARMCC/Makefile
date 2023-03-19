
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = coap-demo-flash
TOP               := ../../../../../..

BUILD_AT                  = n
BUILD_AT_DEBUG           = n
THIRDPARTY_LIBCOAP_NO_DTLS_ENABLE   = y
THIRDPARTY_TINYDTLS_ENABLE = y
THIRDPARTY_MBEDTLS_ENABLE  = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/coap_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/coap_example/src/bsp_custom.o
include $(TOP)/PLAT/tools/scripts/Makefile.rules

#enable WDT
CFLAGS += -DWDT_FEATURE_ENABLE=1