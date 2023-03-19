
############################################################################
# SDK directory path
############################################################################

CTIOT_NB_SDK_DIR  := $(TOP)/PLAT/middleware/thirdparty/ctlwm2m/V1.5/src

############################################################################
# define micro
############################################################################
include $(TOP)/PLAT/middleware/thirdparty/ctlwm2m/V1.5/config_ctiot.mk

CFLAGS += -DPLATFORM_MCU_ECOM -DFEATURE_CTWINGV1_5_ENABLE -DLWM2M_BOOTSTRAP

ifeq ($(LWM2M_CLIENT_MODE),yes)
	CFLAGS += -DLWM2M_CLIENT_MODE
endif

ifeq ($(LWM2M_SUPPORT_JSON),yes)
	CFLAGS += -DLWM2M_SUPPORT_JSON
endif

ifeq ($(LWM2M_BIG_ENDIAN),yes)
	CFLAGS += -DLWM2M_BIG_ENDIAN
else
	CFLAGS += -DLWM2M_LITTLE_ENDIAN
endif

ifeq ($(NB_SDK),yes)
	CFLAGS += -DNB_SDK
endif

ifeq ($(LWM2M_WITH_LOGS),yes)
	CFLAGS += -DLWM2M_WITH_LOGS 
endif

ifeq ($WITH_CTIOT_LOGS),yes)
	CFLAGS += -DWITH_CTIOT_LOGS
endif

ifeq ($(WITH_MBEDTLS),yes)
	CFLAGS += -DWITH_MBEDTLS
	CFLAGS += -DFEATURE_CT_DTLS_ENABLE
endif

ifeq ($(WITH_AUTO_RESP),yes)
	CFLAGS += -DAUTO_RESP_OBSERVE_OBJ3
endif

ifeq ($(USE_SM9),yes)
	CFLAGS += -DWITH_SM9
endif

ifeq ($(USE_FOTA),yes)
	CFLAGS += -DWITH_FOTA -DCTIOT_ABUP_FOTA_ENABLE
endif

ifeq ($(USE_FOTA_RESUME),yes)
	CFLAGS += -DWITH_FOTA_RESUME
endif

############################################################################
# include path
############################################################################
CFLAGS_INC += -I $(CTIOT_NB_SDK_DIR)/core                           \
              -I $(CTIOT_NB_SDK_DIR)/ctiot_client/common            \
			        -I $(CTIOT_NB_SDK_DIR)/ctiot_client/object19		    \
			        -I $(CTIOT_NB_SDK_DIR)/ctiot_client       		    \
			        -I $(CTIOT_NB_SDK_DIR)                        	  

############################################################################
# src path
############################################################################
CTLWM2M_SRC_DIRS += $(CTIOT_NB_SDK_DIR)/core                            \
                    $(CTIOT_NB_SDK_DIR)/ctiot_client                    \
					$(CTIOT_NB_SDK_DIR)/ctiot_client/common             \
                    $(CTIOT_NB_SDK_DIR)/ctiot_client/object19           \
					$(CTIOT_NB_SDK_DIR)/ctiot_client/objects			\
					$(CTIOT_NB_SDK_DIR)/port

ifeq ($(USE_FOTA),yes)
	CFLAGS_INC +=-I $(CTIOT_NB_SDK_DIR)/ctiot_client/fota
	CTLWM2M_SRC_DIRS += $(CTIOT_NB_SDK_DIR)/ctiot_client/fota 
endif

CTLWM2M_EXCLUDE_FILES := $(CTIOT_NB_SDK_DIR)/ctiot_client/ctiot_aep_coap.c

CTLWM2M_CSRC = $(foreach dir, $(CTLWM2M_SRC_DIRS), $(wildcard $(dir)/*.c))
CTLWM2M_CFILES = $(filter-out $(CTLWM2M_EXCLUDE_FILES), $(CTLWM2M_CSRC))
CTLWM2M_COBJSTEMP := $(patsubst %.c, %.o, $(CTLWM2M_CFILES))
CTLWM2M_COBJSTEMP :=$(subst $(CTIOT_NB_SDK_DIR),PLAT/middleware/thirdparty/ctlwm2m/V1.5/src,$(CTLWM2M_COBJSTEMP))
CTLWM2M_COBJS := $(addprefix $(BUILDDIR)/, $(CTLWM2M_COBJSTEMP))


-include $(CTLWM2M_COBJS:.o=.d)


ifeq ($(TOOLCHAIN),GCC)

lib-y += libctlwm2m.a

$(BUILDDIR)/lib/libctlwm2m.a: $(CTLWM2M_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

lib-y += libctlwm2m.lib

$(BUILDDIR)/lib/libctlwm2m.lib: $(CTLWM2M_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
