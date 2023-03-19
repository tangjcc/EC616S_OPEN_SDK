
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = lwm2m-demo-flash
TOP               := ../../../../../..

BUILD_AT                    = n
BUILD_AT_DEBUG              = n
THIRDPARTY_WAKAAMA_ENABLE   = y
THIRDPARTY_ERCOAP_ENABLE    = y
THIRDPARTY_MBEDTLS_ENABLE   = n
THIRDPARTY_TINYDTLS_ENABLE  = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/lwm2m_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/lwm2m_example/src/bsp_custom.o
include $(TOP)/PLAT/tools/scripts/Makefile.rules
