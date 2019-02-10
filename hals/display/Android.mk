LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := gralloc.drm.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_SYMLINKS := gralloc.rpi3.so

LOCAL_SRC_FILES := \
	gralloc.drm.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := hwcomposer.drm.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_SYMLINKS := hwcomposer.rpi3.so

LOCAL_SRC_FILES := \
	hwcomposer.drm.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := gallium_dri.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := dri
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_SYMLINKS := vc4_dri.so

LOCAL_REQUIRED_MODULES := \
	libexpat.vendor

LOCAL_SRC_FILES := \
	gallium_dri.so

LOCAL_POST_INSTALL_CMD := \
	$(ACP) $(TARGET_OUT)/lib/vndk-28/libexpat.so $(TARGET_OUT_VENDOR)/lib/libexpat.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := libglapi.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
	libglapi.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := libGLES_mesa.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := $(suffix $(LOCAL_SRC_FILES))
LOCAL_MODULE_RELATIVE_PATH := egl
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
	libGLES_mesa.so

LOCAL_REQUIRED_MODULES := \
	gallium_dri.so \
	libglapi.so

LOCAL_REQUIRED_MODULES += \
	libdrm.vendor \

include $(BUILD_PREBUILT)

# ----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := gralloc.drm
LOCAL_REQUIRED_MODULES := $(LOCAL_MODULE).so
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := hwcomposer.drm
LOCAL_REQUIRED_MODULES := $(LOCAL_MODULE).so
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := libGLES_mesa
LOCAL_REQUIRED_MODULES := $(LOCAL_MODULE).so
include $(BUILD_PHONY_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
