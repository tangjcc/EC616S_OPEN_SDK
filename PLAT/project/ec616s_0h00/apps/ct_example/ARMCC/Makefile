
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = ct-example-flash
TOP               := ../../../../../..

BUILD_AT                    = n
BUILD_AT_DEBUG              = n
THIRDPARTY_CTWING1_5_ENABLE = y
THIRDPARTY_ERCOAP_ENABLE    = y
THIRDPARTY_MBEDTLS_ENABLE   = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/ct_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/ct_example/src/bsp_custom.o \
                     PLAT/project/$(TARGET)/apps/ct_example/src/ct_api.o

include $(TOP)/PLAT/tools/scripts/Makefile.rules
