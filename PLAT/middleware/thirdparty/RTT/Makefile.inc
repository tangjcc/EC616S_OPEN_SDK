RTT_DIR        := $(TOP)/PLAT/middleware/thirdparty/RTT

CFLAGS_INC     +=  -I $(TOP)/PLAT/middleware/thirdparty/RTT
              
CFLAGS         += -DFEATURE_RTT_ENABLED
RTT_SRC_DIRS += $(RTT_DIR)
RTT_EXCLUDE_FILES :=
RTT_CSRC = $(foreach dir, $(RTT_SRC_DIRS), $(wildcard $(dir)/*.c))
RTT_CFILES = $(filter-out $(RTT_EXCLUDE_FILES), $(RTT_CSRC))
RTT_COBJSTEMP := $(patsubst %.c, %.o, $(RTT_CFILES))
RTT_COBJSTEMP :=$(subst $(RTT_DIR),PLAT/middleware/thirdparty/RTT,$(RTT_COBJSTEMP))
RTT_COBJS := $(addprefix $(BUILDDIR)/, $(RTT_COBJSTEMP))


-include $(RTT_COBJS:.o=.d)


ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += libRTT.a

$(BUILDDIR)/lib/libRTT.a: $(RTT_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil


lib-y += libRTT.lib

$(BUILDDIR)/lib/libRTT.lib: $(RTT_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
