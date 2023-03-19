CIS_VERSION     := NB_SDK2.3.0
CIS_DIR         := $(TOP)/PLAT/middleware/thirdparty/cis_onenet/$(CIS_VERSION)


ifeq ($(CIS_VERSION), NB_SDK2.3.0)
CFLAGS          += -DCIS_EMBED -DHAVE_STRUCT_TIMESPEC -DFEATURE_CISONENET_ENABLE
CFLAGS_INC       +=  -I $(CIS_DIR)/src/ciscore \
                     -I $(CIS_DIR)/src/ciscore/std_object \
                     -I $(CIS_DIR)/src/ciscore/dm_utils   \
                     -I $(CIS_DIR)/src/adapter/freeRTOS

             

CIS_SRC_DIRS += $(CIS_DIR)/src/ciscore \
                 $(CIS_DIR)/src/ciscore/std_object \
                 $(CIS_DIR)/src/ciscore/dm_utils \
                 $(CIS_DIR)/src/adapter/freeRTOS
CIS_CSRC = $(foreach dir, $(CIS_SRC_DIRS), $(wildcard $(dir)/*.c))
CIS_CFILES = $(filter-out $(CIS_EXCLUDE_FILES), $(CIS_CSRC))
CIS_COBJSTEMP := $(patsubst %.c, %.o, $(CIS_CFILES))
CIS_COBJSTEMP :=$(subst $(CIS_DIR),PLAT/middleware/thirdparty/cis_onenet/$(CIS_VERSION),$(CIS_COBJSTEMP))
CIS_COBJS := $(addprefix $(BUILDDIR)/, $(CIS_COBJSTEMP))
endif

-include $(CIS_COBJS:.o=.d)

ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += libcis_onenet.a

$(BUILDDIR)/lib/libcis_onenet.a: $(CIS_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

lib-y += libcis_onenet.lib

$(BUILDDIR)/lib/libcis_onenet.lib: $(CIS_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
