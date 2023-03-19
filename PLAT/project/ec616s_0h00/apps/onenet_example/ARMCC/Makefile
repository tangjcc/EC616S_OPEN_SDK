
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = onenet-demo-flash
TOP               := ../../../../../..

BUILD_AT                    = n
BUILD_AT_DEBUG              = n
THIRDPARTY_CISONENET_ENABLE = y
THIRDPARTY_WAKAAMA_ENABLE   = n
THIRDPARTY_ERCOAP_ENABLE    = y
THIRDPARTY_TINYDTLS_ENABLE  = y
THIRDPARTY_MBEDTLS_ENABLE   = y

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/onenet_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/onenet_example/src/cisAsynEntry.o \
		     PLAT/project/$(TARGET)/apps/onenet_example/src/bsp_custom.o


include $(TOP)/PLAT/tools/scripts/Makefile.rules
