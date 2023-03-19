AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = GCC
BINNAME           = onenet-demo-flash
TOP := ../../../../../..

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/onenet_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/onenet_example/src/cisAsynEntry.o \
		     PLAT/project/$(TARGET)/apps/onenet_example/src/bsp_custom.o

include $(TOP)/PLAT/tools/scripts/Makefile.rules

#enable WDT
CFLAGS += -DWDT_FEATURE_ENABLE=1