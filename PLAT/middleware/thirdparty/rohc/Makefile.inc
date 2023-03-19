ifeq ($(BUILD_USE_PREBUILD_LIB), n)
ROHC_DIR            := $(TOP)/PROTOCOL/SRC/tcpip/rohc
else
ROHC_DIR            := $(TOP)/PLAT/middleware/thirdparty/rohc
endif

CFLAGS_INC += -I $(ROHC_DIR)/comm/inc     \
		      -I $(ROHC_DIR)/api/inc

ROHC_SRC_DIRS += $(ROHC_DIR)/comm/src     \
                $(ROHC_DIR)/api/src       \
                $(ROHC_DIR)/uncomp/src

ROHC_EXCLUDE_FILES :=
                

ROHC_CSRC = $(foreach dir, $(ROHC_SRC_DIRS), $(wildcard $(dir)/*.c))
ROHC_CFILES = $(filter-out $(ROHC_EXCLUDE_FILES), $(ROHC_CSRC))
ROHC_COBJSTEMP := $(patsubst %.c, %.o, $(ROHC_CFILES))
ifeq ($(BUILD_USE_PREBUILD_LIB), n)
ROHC_COBJSTEMP :=$(subst $(ROHC_DIR),PROTOCOL/SRC/tcpip/rohc,$(ROHC_COBJSTEMP))
else
ROHC_COBJSTEMP :=$(subst $(ROHC_DIR),PLAT/middleware/thirdparty/rohc,$(ROHC_COBJSTEMP))
endif
ROHC_COBJS := $(addprefix $(BUILDDIR)/, $(ROHC_COBJSTEMP))


-include $(ROHC_COBJS:.o=.d)

CFLAGS +=  -DROHC_CONFIG_FILE=\"rohcconfig.h\"

ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

ifeq ($(BUILD_USE_PREBUILD_LIB), n)
   lib-y += librohc.a
endif

$(BUILDDIR)/lib/librohc.a: $(ROHC_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

ifeq ($(BUILD_USE_PREBUILD_LIB), n)
   lib-y += librohc.lib
endif

$(BUILDDIR)/lib/librohc.lib: $(ROHC_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
