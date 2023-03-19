
CFLAGS          += -DADUPS_CUSTOM_LWM2M -DTHIRDPARTY_ABUP_FOTA_ENABLE

ABUP_DIR      := $(TOP)/PLAT/middleware/thirdparty/abup


CFLAGS_INC       +=  -I $(ABUP_DIR)/App/Inc

PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/abup_app_private.lib

ABUP_SRC_DIRS += $(ABUP_DIR)/App/Src
                 
ABUP_SRC_PRIVATE_DIRS += $(ABUP_DIR)/App/core
                 
ABUP_EXCLUDE_FILES :=
ABUP_EXCLUDE_PRIVATE_FILES :=

ABUP_CSRC = $(foreach dir, $(ABUP_SRC_DIRS), $(wildcard $(dir)/*.c))
ABUP_CFILES = $(filter-out $(ABUP_EXCLUDE_FILES), $(ABUP_CSRC))
ABUP_COBJSTEMP := $(patsubst %.c, %.o, $(ABUP_CFILES))
ABUP_COBJSTEMP :=$(subst $(ABUP_DIR),PLAT/middleware/thirdparty/abup,$(ABUP_COBJSTEMP))
ABUP_COBJS := $(addprefix $(BUILDDIR)/, $(ABUP_COBJSTEMP))

ABUP_PRIVATE_CSRC = $(foreach dir, $(ABUP_SRC_PRIVATE_DIRS), $(wildcard $(dir)/*.c))
ABUP_PRIVATE_CFILES = $(filter-out $(ABUP_EXCLUDE_PRIVATE_FILES), $(ABUP_PRIVATE_CSRC))
ABUP_PRIVATE_COBJSTEMP := $(patsubst %.c, %.o, $(ABUP_PRIVATE_CFILES))
ABUP_PRIVATE_COBJSTEMP :=$(subst $(ABUP_DIR),PLAT/middleware/thirdparty/abup,$(ABUP_PRIVATE_COBJSTEMP))
ABUP_PRIVATE_COBJS := $(addprefix $(BUILDDIR)/, $(ABUP_PRIVATE_COBJSTEMP))


-include $(ABUP_COBJS:.o=.d)
-include $(ABUP_PRIVATE_COBJS:.o=.d)




ifeq ($(TOOLCHAIN),ARMCC)


lib-y += abup_app.lib

$(BUILDDIR)/lib/abup_app.lib: $(ABUP_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^


#add lib
#lib-y += abup_app_private.lib

$(BUILDDIR)/lib/abup_app_private.lib: $(ABUP_PRIVATE_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
