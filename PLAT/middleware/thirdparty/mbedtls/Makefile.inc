MBEDTLS_DIR      := $(TOP)/PLAT/middleware/thirdparty/mbedtls
CFLAGS_INC    +=  -I $(MBEDTLS_DIR)/include  \
                  -I $(MBEDTLS_DIR)/include/mbedtls  \
                  -I $(MBEDTLS_DIR)/configs  \
                  -I $(MBEDTLS_DIR)/library/ec61x/inc
 
MBEDTLS_SRC_DIRS += $(MBEDTLS_DIR)/library    \
                    $(MBEDTLS_DIR)/library/ec61x/src


MBEDTLS_CFLAGS ?= -DMBEDTLS_CONFIG_FILE=\"config_ec_ssl_comm.h\"

ifeq ($(MBEDTLS_WITH_CA),y)
	CFLAGS += -DHTTPS_WITH_CA
endif

CFLAGS += $(MBEDTLS_CFLAGS)
CFLAGS += -DFEATURE_MBEDTLS_ENABLE
CFLAGS += -DMQTT_RAI_OPTIMIZE

MBEDTLS_EXCLUDE_FILES :=

MBEDTLS_CSRC = $(foreach dir, $(MBEDTLS_SRC_DIRS), $(wildcard $(dir)/*.c))
MBEDTLS_CFILES = $(filter-out $(MBEDTLS_EXCLUDE_FILES), $(MBEDTLS_CSRC))
MBEDTLS_COBJSTEMP := $(patsubst %.c, %.o, $(MBEDTLS_CFILES))
MBEDTLS_COBJSTEMP :=$(subst $(MBEDTLS_DIR),PLAT/middleware/thirdparty/mbedtls,$(MBEDTLS_COBJSTEMP))
MBEDTLS_COBJS := $(addprefix $(BUILDDIR)/, $(MBEDTLS_COBJSTEMP))


-include $(MBEDTLS_COBJS:.o=.d)


ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += libmbedtls.a

$(BUILDDIR)/lib/libmbedtls.a: $(MBEDTLS_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

lib-y += libmbedtls.lib

$(BUILDDIR)/lib/libmbedtls.lib: $(MBEDTLS_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
