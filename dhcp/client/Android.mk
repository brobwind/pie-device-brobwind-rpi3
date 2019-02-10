LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dhcpclient.cpp \
	interface.cpp \
	main.cpp \
	router.cpp \
	timer.cpp \
	../common/message.cpp \
	../common/socket.cpp \


LOCAL_CPPFLAGS += -Werror
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_MODULE_TAGS := debug
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := dhcpclient

LOCAL_MODULE_CLASS := EXECUTABLES

include $(BUILD_EXECUTABLE)

