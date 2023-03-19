
MQTT_DIR      := $(TOP)/PLAT/middleware/thirdparty/mqtt
CFLAGS_INC    += -I $(TOP)/PLAT/os/freertos/inc               \
                  -I $(TOP)/PLAT/os/freertos/CMSIS/inc        \
                  -I $(TOP)/PLAT/middleware/thirdparty/cjson  \
                  -I $(MQTT_DIR)/MQTTPacket/src               \
                  -I $(MQTT_DIR)/MQTTClient-C/src/FreeRTOS    \
                  -I $(MQTT_DIR)/MQTTClient-C/src             \
                  -I $(MQTT_DIR)   
              
CFLAGS += -DFEATURE_MQTT_ENABLE

MQTT_SRC_DIRS += $(MQTT_DIR)/MQTTPacket/src            \
                 $(MQTT_DIR)/MQTTClient-C/src/FreeRTOS \
                 $(MQTT_DIR)/MQTTClient-C/src          \
                 $(MQTT_DIR)

MQTT_EXCLUDE_FILES :=

MQTT_CSRC = $(foreach dir, $(MQTT_SRC_DIRS), $(wildcard $(dir)/*.c))
MQTT_CFILES = $(filter-out $(MQTT_EXCLUDE_FILES), $(MQTT_CSRC))
MQTT_COBJSTEMP := $(patsubst %.c, %.o, $(MQTT_CFILES))
MQTT_COBJSTEMP :=$(subst $(MQTT_DIR),PLAT/middleware/thirdparty/mqtt,$(MQTT_COBJSTEMP))
MQTT_COBJS := $(addprefix $(BUILDDIR)/, $(MQTT_COBJSTEMP))


-include $(MQTT_COBJS:.o=.d)


ifeq ($(TOOLCHAIN),GCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/gcc

lib-y += libmqtt.a

$(BUILDDIR)/lib/libmqtt.a: $(MQTT_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) -cr $@ $^

endif

ifeq ($(TOOLCHAIN),ARMCC)

CFLAGS_INC += -I $(TOP)/PLAT/os/freertos/portable/keil

lib-y += libmqtt.lib

$(BUILDDIR)/lib/libmqtt.lib: $(MQTT_COBJS)
	@mkdir -p $(dir $@)
	$(ECHO) AR $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
