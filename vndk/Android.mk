# TODO(b/69526027): DEPRECATE USE OF THIS.
# USE BOARD_VNDK_VERSION:=current instead.

LOCAL_PATH := $(call my-dir)

# b/69526027: This VNDK-SP install routine must be removed. Instead, we must
# build vendor variants of the VNDK-SP modules.

#ifndef BOARD_VNDK_VERSION
# The libs with "vndk: {enabled: true, support_system_process: true}" will be
# added VNDK_SP_LIBRARIES automatically. And the core variants of the VNDK-SP
# libs will be copied to vndk-sp directory.
# However, some of those libs need FWK-ONLY libs, which must be listed here
# manually.
VNDK_SP_LIBRARIES := \
	libbinder \
	libhardware_legacy \
	libjpeg \
	libyuv

vndk_sp_dir := vndk-sp-$(PLATFORM_VNDK_VERSION)

define define-vndk-sp-lib
include $$(CLEAR_VARS)
LOCAL_MODULE := $1.vndk-sp-gen
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PREBUILT_MODULE_FILE := $$(call intermediates-dir-for,SHARED_LIBRARIES,$1,,,,)/$1.so
LOCAL_STRIP_MODULE := false
LOCAL_MULTILIB := first
LOCAL_MODULE_TAGS := optional
LOCAL_INSTALLED_MODULE_STEM := $1.so
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_RELATIVE_PATH := $(vndk_sp_dir)$(if $(filter $1,$(install_in_hw_dir)),/hw)
include $$(BUILD_PREBUILT)
endef

# Add VNDK-SP libs to the list if they are missing
#$(foreach lib,$(VNDK_SAMEPROCESS_LIBRARIES),\
#    $(if $(filter $(lib),$(VNDK_SP_LIBRARIES)),,\
#    $(eval VNDK_SP_LIBRARIES += $(lib))))

# Remove libz from the VNDK-SP list (b/73296261)
#VNDK_SP_LIBRARIES := $(filter-out libz,$(VNDK_SP_LIBRARIES))

$(foreach lib,$(VNDK_SP_LIBRARIES),\
    $(eval $(call define-vndk-sp-lib,$(lib))))

install_in_hw_dir :=

include $(CLEAR_VARS)
LOCAL_MODULE := vndk-sp
LOCAL_MODULE_OWNER := google
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(addsuffix .vndk-sp-gen,$(VNDK_SP_LIBRARIES))
include $(BUILD_PHONY_PACKAGE)
