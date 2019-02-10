LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<POWERS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS := -Wconversion -Wall -Werror -Wno-sign-conversion
LOCAL_CLANG  := true

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libhardware \
	libutils

LOCAL_SRC_FILES := memtrack_rpi3.c
LOCAL_MODULE := memtrack.rpi3

include $(BUILD_SHARED_LIBRARY)
