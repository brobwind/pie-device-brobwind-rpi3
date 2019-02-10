LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dhcpserver.cpp \
	main.cpp \
	../common/message.cpp \
	../common/socket.cpp \
	../common/utils.cpp \


LOCAL_CPPFLAGS += -Werror
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_MODULE_TAGS := debug
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := dhcpserver

LOCAL_MODULE_CLASS := EXECUTABLES

include $(BUILD_EXECUTABLE)

