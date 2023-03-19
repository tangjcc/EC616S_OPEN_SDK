AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = GCC
BINNAME           = ec-socket-demo-flash
TOP := ../../../../../..

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/ec_socket_example/src/app.o \
                     PLAT/project/$(TARGET)/apps/ec_socket_example/src/bsp_custom.o
include $(TOP)/PLAT/tools/scripts/Makefile.rules
