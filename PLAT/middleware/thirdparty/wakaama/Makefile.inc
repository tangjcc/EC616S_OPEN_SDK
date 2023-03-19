LWM2M_DIR        := $(TOP)/PLAT/middleware/thirdparty/wakaama
CFLAGS          += -DLWM2M_OLD_CONTENT_FORMAT_SUPPORT -DNO_DECLTYPE -DLWM2M_BOOTSTRAP -DFEATURE_WAKAAMA_ENABLE -DLWM2M_CLIENT_MODE

ifeq ($(LWM2M_BIG_ENDIAN),yes)
	CFLAGS += -DLWM2M_BIG_ENDIAN
else
	CFLAGS += -DLWM2M_LITTLE_ENDIAN
endif

CFLAGS_INC += -I $(LWM2M_DIR)/core                         \
              -I $(LWM2M_DIR)/examples


LWM2M_SRC_DIRS += $(LWM2M_DIR)/core \
                  $(LWM2M_DIR)/examples/client    \
                  $(LWM2M_DIR)/examples/shared

LWM2M_EXCLUDE_FILES :=

LWM2M_CSRC = $(foreach dir, $(LWM2M_SRC_DIRS), $(wildcard $(dir)/*.c))
LWM2M_CFILES = $(filter-out $(LWM2M_EXCLUDE_FILES), $(LWM2M_CSRC))
LWM2M_COBJSTEMP := $(patsubst %.c, %.o, $(LWM2M_CFILES))
LWM2M_COBJSTEMP :=$(subst $(LWM2M_DIR),PLAT/middleware/thirdparty/wakaama,$(LWM2M_COBJSTEMP))
LWM2M_COBJS := $(addprefix $(BUILDDIR)/, $(LWM2M_COBJSTEMP))


-include $(LWM2M_COBJS:.o=.d)


ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += liblwm2m.a

$(BUILDDIR)/lib/liblwm2m.a: $(LWM2M_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

lib-y += liblwm2m.lib

$(BUILDDIR)/lib/liblwm2m.lib: $(LWM2M_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
