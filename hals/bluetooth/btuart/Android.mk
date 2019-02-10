# Copyright (C) 2016 http://www.brobwind.com
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := BCM43430A1.hcd
LOCAL_MODULE_CLASS := DATA
LOCAL_SRC_FILES := $(LOCAL_MODULE)

LOCAL_MODULE_PATH := \
	$(TARGET_OUT_VENDOR)/etc/firmware/bcm

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := BCM4345C0.hcd
LOCAL_MODULE_CLASS := DATA
LOCAL_SRC_FILES := $(LOCAL_MODULE)

LOCAL_MODULE_PATH := \
	$(TARGET_OUT_VENDOR)/etc/firmware/bcm

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := btuart
LOCAL_PROPRIETARY_MODULE := true

LOCAL_REQUIRED_MODULES := \
	BCM43430A1.hcd \
	BCM4345C0.hcd

LOCAL_SRC_FILES := \
	hciattach_rpi3.c

LOCAL_CFLAGS := \
	-DFIRMWARE_DIR=\"/vendor/etc/firmware/bcm\"

LOCAL_SHARED_LIBRARIES := \
	libcutils liblog

include $(BUILD_EXECUTABLE)
