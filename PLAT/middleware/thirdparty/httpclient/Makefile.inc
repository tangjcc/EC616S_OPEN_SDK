
HTTPCLIENT_DIR         := $(TOP)/PLAT/middleware/thirdparty/httpclient
CFLAGS_INC       +=  -I $(HTTPCLIENT_DIR) 
              
CFLAGS += -DFEATURE_HTTPC_ENABLE
HTTPCLIENT_SRC_DIRS += $(HTTPCLIENT_DIR) 


HTTPCLIENT_EXCLUDE_FILES :=

HTTPCLIENT_CSRC = $(foreach dir, $(HTTPCLIENT_SRC_DIRS), $(wildcard $(dir)/*.c))
HTTPCLIENT_CFILES = $(filter-out $(HTTPCLIENT_EXCLUDE_FILES), $(HTTPCLIENT_CSRC))
HTTPCLIENT_COBJSTEMP := $(patsubst %.c, %.o, $(HTTPCLIENT_CFILES))
HTTPCLIENT_COBJSTEMP :=$(subst $(HTTPCLIENT_DIR),PLAT/middleware/thirdparty/httpclient,$(HTTPCLIENT_COBJSTEMP))
HTTPCLIENT_COBJS := $(addprefix $(BUILDDIR)/, $(HTTPCLIENT_COBJSTEMP))


-include $(HTTPCLIENT_COBJS:.o=.d)

ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += libhttpclient.a

$(BUILDDIR)/lib/libhttpclient.a: $(HTTPCLIENT_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

lib-y += libhttpclient.lib

$(BUILDDIR)/lib/libhttpclient.lib: $(HTTPCLIENT_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
