AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = GCC
BINNAME           = filesystem-example
TOP := ../../../../../..

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/fs_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/fs_example/src/fs_example.o \
		     PLAT/project/$(TARGET)/apps/fs_example/src/bsp_custom.o

include $(TOP)/PLAT/tools/scripts/Makefile.rules
