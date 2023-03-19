
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = pslibapi-demo-flash
TOP               := ../../../../../..

BUILD_AT                    = n
BUILD_AT_DEBUG              = n

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/pslibapi_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/pslibapi_example/src/bsp_custom.o


include $(TOP)/PLAT/tools/scripts/Makefile.rules

#enable WDT
CFLAGS += -DWDT_FEATURE_ENABLE=1