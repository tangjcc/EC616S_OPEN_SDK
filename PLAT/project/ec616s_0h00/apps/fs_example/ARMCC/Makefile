AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = filesystem-example
TOP               := ../../../../../..

BUILD_AT                  = n
BUILD_AT_DEBUG            = n
DRIVER_RNG_ENABLE         = n

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/fs_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/fs_example/src/fs_example.o \
		     PLAT/project/$(TARGET)/apps/fs_example/src/bsp_custom.o


include $(TOP)/PLAT/tools/scripts/Makefile.rules
