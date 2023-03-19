COAP_VERSION    ?= 4.2.0

GCCLIB_PATH := ../../../../../../../PLAT_MISC/misc/gcc

LIBCOAP_DIR      := $(TOP)/PLAT/middleware/thirdparty/libcoap
CFLAGS_INC    +=  -I $(TOP)/PLAT/middleware/thirdparty/tinydtls \
                  -I $(TOP)/PLAT/middleware/thirdparty/tinydtls/include \
                  -I $(TOP)/PLAT/middleware/thirdparty/tinydtls/aes \
                  -I $(TOP)/PLAT/middleware/thirdparty/tinydtls/ecc \
                  -I $(TOP)/PLAT/middleware/thirdparty/tinydtls/sha2 \
                  -I $(LIBCOAP_DIR)/coap_adapter/coap_freertos \
                  -I $(LIBCOAP_DIR)/libcoap-$(COAP_VERSION) \
                  -I $(LIBCOAP_DIR)/libcoap-$(COAP_VERSION)/include \
                  -I $(LIBCOAP_DIR)/libcoap-$(COAP_VERSION)/include/coap2 \
                  -I $(LIBCOAP_DIR)/coap_adapter

LIBCOAP_SRC_DIRS += $(LIBCOAP_DIR)/libcoap-$(COAP_VERSION)/src \
                    $(LIBCOAP_DIR)/coap_adapter

CFLAGS += -DFEATURE_LIBCOAP_ENABLE	-DNO_DECLTYPE
	
		
LIBCOAP_EXCLUDE_FILES :=

LIBCOAP_CSRC = $(foreach dir, $(LIBCOAP_SRC_DIRS), $(wildcard $(dir)/*.c))
LIBCOAP_CFILES = $(filter-out $(LIBCOAP_EXCLUDE_FILES), $(LIBCOAP_CSRC))
LIBCOAP_COBJSTEMP := $(patsubst %.c, %.o, $(LIBCOAP_CFILES))
LIBCOAP_COBJSTEMP :=$(subst $(LIBCOAP_DIR),PLAT/middleware/thirdparty/libcoap,$(LIBCOAP_COBJSTEMP))
LIBCOAP_COBJS := $(addprefix $(BUILDDIR)/, $(LIBCOAP_COBJSTEMP))


-include $(LIBCOAP_COBJS:.o=.d)

CFLAGS += -DWITH_POSIX

ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc
CFLAGS_INC += -I $(GCCLIB_PATH)/arm-none-eabi/include

lib-y += liblibcoap.a

$(BUILDDIR)/lib/liblibcoap.a: $(LIBCOAP_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

lib-y += liblibcoap.lib

$(BUILDDIR)/lib/liblibcoap.lib: $(LIBCOAP_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
