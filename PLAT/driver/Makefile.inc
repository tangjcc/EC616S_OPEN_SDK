libdriver-y :=
libdriverprivate-y :=


include $(TOP)/PLAT/driver/board/$(TARGET)/Makefile.inc
include $(TOP)/PLAT/driver/chip/$(CHIP)/Makefile.inc
include $(TOP)/PLAT/driver/hal/Makefile.inc

ifeq ($(TOOLCHAIN),GCC)

DRIVER_OBJS := $(addprefix $(BUILDDIR)/,$(libdriver-y))

-include $(DRIVER_OBJS:.o=.d)

ifneq ($(DRIVER_OBJS),)
lib-y += libdriver.a
endif


$(BUILDDIR)/lib/libdriver.a: $(DRIVER_OBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^



DRIVER_PRIVATE_OBJS := $(addprefix $(BUILDDIR)/,$(libdriverprivate-y))
-include $(DRIVER_PRIVATE_OBJS:.o=.d)
ifeq ($(BUILD_USE_PREBUILD_LIB),n)
ifneq ($(DRIVER_PRIVATE_OBJS),)
  lib-y += libdriver_private.a
endif
endif
$(BUILDDIR)/lib/libdriver_private.a: $(DRIVER_PRIVATE_OBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^


endif

ifeq ($(TOOLCHAIN),ARMCC)

DRIVER_OBJS := $(addprefix $(BUILDDIR)/,$(libdriver-y))
-include $(DRIVER_OBJS:.o=.d)

ifneq ($(DRIVER_OBJS),)
lib-y += libdriver.lib
endif
$(BUILDDIR)/lib/libdriver.lib: $(DRIVER_OBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^


DRIVER_PRIVATE_OBJS := $(addprefix $(BUILDDIR)/,$(libdriverprivate-y))
-include $(DRIVER_PRIVATE_OBJS:.o=.d)
ifeq ($(BUILD_USE_PREBUILD_LIB),n)
ifneq ($(DRIVER_PRIVATE_OBJS),)
  lib-y += libdriver_private.lib
endif
endif
$(BUILDDIR)/lib/libdriver_private.lib: $(DRIVER_PRIVATE_OBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^


endif