AVAILABLE_TARGETS = ec616_0h00 ec616z0_0h00
TOOLCHAIN         = GCC
BINNAME           = mqtt-demo-flash
TOP := ../../../../../..

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/mqtt_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/mqtt_example/src/bsp_custom.o
include $(TOP)/PLAT/tools/scripts/Makefile.rules
