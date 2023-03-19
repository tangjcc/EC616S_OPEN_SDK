

SOFTSIM_DIR     := $(TOP)/PLAT/middleware/thirdparty/softsim


CFLAGS_INC       +=  -I $(SOFTSIM_DIR)/ecsoftsimapt/Inc \
					 -I $(SOFTSIM_DIR)/ecsoftsimapt/src/HalAes/Inc   \
					 -I $(SOFTSIM_DIR)/ecsoftsimapt/wrapper 
					 
SOFTSIM_SRC_DIRS += $(SOFTSIM_DIR)/ecsoftsimapt/src \
                    $(SOFTSIM_DIR)/ecsoftsimapt/src/HalAes/src   \
					$(SOFTSIM_DIR)/ecsoftsimapt/wrapper 


##########################softsim vendor feature area############################

#ifeq ($(THIRDPARTY_SOFTSIM_CT_ENABLE),y)		
CFLAGS_INC   += -I $(SOFTSIM_DIR)/vendor/ctcc/Inc 

PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libesim_cos.lib \
                $(LIBDIR)/PLAT/lib/keil/libesim_ta.lib


#endif


##########################softsim vendor feature area ############################

                 
SOFTSIM_EXCLUDE_FILES :=

SOFTSIM_CSRC = $(foreach dir, $(SOFTSIM_SRC_DIRS), $(wildcard $(dir)/*.c))
SOFTSIM_CFILES = $(filter-out $(SOFTSIM_EXCLUDE_FILES), $(SOFTSIM_CSRC))
SOFTSIM_COBJSTEMP := $(patsubst %.c, %.o, $(SOFTSIM_CFILES))
SOFTSIM_COBJSTEMP :=$(subst $(SOFTSIM_DIR),PLAT/middleware/thirdparty/softsim,$(SOFTSIM_COBJSTEMP))
SOFTSIM_COBJS := $(addprefix $(BUILDDIR)/, $(SOFTSIM_COBJSTEMP))


-include $(SOFTSIM_COBJS:.o=.d)



ifeq ($(TOOLCHAIN),ARMCC)


lib-y += libsoftsim_apt.lib

$(BUILDDIR)/lib/libsoftsim_apt.lib: $(SOFTSIM_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

	
endif
