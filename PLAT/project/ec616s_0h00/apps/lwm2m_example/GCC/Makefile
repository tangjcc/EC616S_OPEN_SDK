AVAILABLE_TARGETS = ec616_0h00 ec616z0_0h00
TOOLCHAIN         = GCC
BINNAME           = lwm2m-demo-flash
TOP := ../../../../../..

BUILD_AT                    = n
BUILD_AT_DEBUG              = n
THIRDPARTY_CISONENET_ENABLE = n
THIRDPARTY_WAKAAMA_ENABLE   = y
THIRDPARTY_TINYDTLS_ENABLE  = y
THIRDPARTY_MBEDTLS_ENABLE   = n

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/lwm2m_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/lwm2m_example/src/bsp_custom.o
include $(TOP)/PLAT/tools/scripts/Makefile.rules
