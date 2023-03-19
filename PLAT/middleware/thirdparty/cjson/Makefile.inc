CJSON_DIR        := $(TOP)/PLAT/middleware/thirdparty/CJSON

CFLAGS_INC     +=  -I $(TOP)/PLAT/middleware/thirdparty/CJSON
              

CJSON_SRC_DIRS += $(CJSON_DIR)
CJSON_EXCLUDE_FILES :=
CJSON_CSRC = $(foreach dir, $(CJSON_SRC_DIRS), $(wildcard $(dir)/*.c))
CJSON_CFILES = $(filter-out $(CJSON_EXCLUDE_FILES), $(CJSON_CSRC))
CJSON_COBJSTEMP := $(patsubst %.c, %.o, $(CJSON_CFILES))
CJSON_COBJSTEMP :=$(subst $(CJSON_DIR),PLAT/middleware/thirdparty/CJSON,$(CJSON_COBJSTEMP))
CJSON_COBJS := $(addprefix $(BUILDDIR)/, $(CJSON_COBJSTEMP))


-include $(CJSON_COBJS:.o=.d)

CFLAGS += -DFEATURE_CJSON_ENABLE

ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += libcjson.a

$(BUILDDIR)/lib/libcjson.a: $(CJSON_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil


lib-y += libcjson.lib

$(BUILDDIR)/lib/libcjson.lib: $(CJSON_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
