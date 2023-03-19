
AVAILABLE_TARGETS = ec616s_0h00
TOOLCHAIN         = ARMCC
BINNAME           = app-demo-flash
TOP               := ../../../../../..


THIRDPARTY_CTWING1_5_ENABLE   = y
THIRDPARTY_WAKAAMA_ENABLE   = y
THIRDPARTY_ERCOAP_ENABLE   = y
THIRDPARTY_CISONENET_ENABLE = y
THIRDPARTY_LIBCOAP_NO_DTLS_ENABLE  = y
# THIRDPARTY_LIBCOAP_DTLS_ENABLE  = y
THIRDPARTY_MQTT_ENABLE     = y
############################################## 
# if mqtt tls is enable, other protocols such as ctwing/wakaama/cisonenet/http should be disabled. 
# becase the memory is not large enough to support so many protocols
# THIRDPARTY_MQTT_TLS_ENABLE     = y
##############################################
THIRDPARTY_HTTPC_ENABLE    = y
THIRDPARTY_MBEDTLS_ENABLE  = y
THIRDPARTY_TINYDTLS_ENABLE = y
THIRDPARTY_CJSON_ENABLE = n
THIRDPARTY_CTCC_DM_ENABLE = y
THIRDPARTY_CUCC_DM_ENABLE = y
THIRDPARTY_CMCC_DM_ENABLE = y
THIRDPARTY_SOFTSIM_ENABLE = y
THIRDPARTY_SOFTSIM_CT_ENABLE = n
# MIDDLEWARE_FEATURE_PRODMODE_ENABLE = y
AT_EXAMPLE_ENABLE         = n
AT_ADC_ENABLE             = y
# FLASH_MONITOR_ENABLE = y
##############################################
#Select the OEM of FOTA scheme
#definitions of FEATURE_FOTA_SCHEME_OEM:
# 0 - ABUP
# 1 - ECOM
# 2 - OTHERS
FEATURE_FOTA_SCHEME_OEM = 2

ifeq ($(FEATURE_FOTA_SCHEME_OEM), 0)
#ABUP
THIRDPARTY_ABUP_FOTA_ENABLE  = y
MIDDLEWARE_FOTA_ENABLE       = n
else ifeq ($(FEATURE_FOTA_SCHEME_OEM), 1)
#ECOM
THIRDPARTY_ABUP_FOTA_ENABLE  = n
MIDDLEWARE_FOTA_ENABLE       = y
else
#OTHERS
THIRDPARTY_ABUP_FOTA_ENABLE  = n
MIDDLEWARE_FOTA_ENABLE       = n
endif

CFLAGS_INC        +=  -I ../inc
obj-y             += PLAT/project/$(TARGET)/apps/at_command/src/app.o \
                     PLAT/project/$(TARGET)/apps/at_command/src/bsp_custom.o \
                     PLAT/project/$(TARGET)/apps/at_command/src/hal_uart_atcmd.o \
                     PLAT/project/$(TARGET)/apps/at_command/src/app_user_atcmd.o \
                     PLAT/project/$(TARGET)/apps/at_command/src/app_uart2_handle.o

#CONFIG_PROJ_APP_SECURITY_BOOT  = y

include $(TOP)/PLAT/project/$(TARGET)/apps/$(PROJECT)/armcc/MakeProj.rules
include $(TOP)/PLAT/tools/scripts/Makefile.rules

ifeq ($(FLASH_MONITOR_ENABLE), y)
#enable wdt
CFLAGS += -DWDT_FEATURE_ENABLE=1  -DFS_FILE_OPERATION_STATISTIC
else
CFLAGS += -DWDT_FEATURE_ENABLE=1
endif

ifeq ($(THIRDPARTY_SOFTSIM_ENABLE), y)
#enable device softsim feature
CFLAGS += -DSOFTSIM_FEATURE_ENABLE
endif

ifeq ($(THIRDPARTY_SOFTSIM_CT_ENABLE), y)
#enable softsim vendor feature enable
CFLAGS += -DSOFTSIM_CT_ENABLE
endif

ifneq ($(BUILD_AT),y)
$(error This example needs to modify "BUILD_AT" to "y" in device\eigencomm\board\$(TARGET)\ec616s_0h00.mk)
endif

