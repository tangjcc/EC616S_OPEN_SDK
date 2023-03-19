AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = GCC
BINNAME           = ct-example-flash
TOP := ../../../../../..

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/ct_example/src/app.o
                     PLAT/project/$(TARGET)/apps/ct_example/src/bsp_custom.o

include $(TOP)/PLAT/tools/scripts/Makefile.rules
