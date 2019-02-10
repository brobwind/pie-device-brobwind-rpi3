# Copyright (C) 2013 The Android Open Source Project
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

# common stuffs across rpi3

# Host modules
PRODUCT_PACKAGES += \

# Device modules
PRODUCT_PACKAGES += \
    bootctl \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libsync \
    libhwc2on1adapter \
    audio.a2dp.default \
    audio.primary.rpi3 \
    camera.rpi3 \
    camera.rpi3.jpeg \
    gatekeeper.rpi3 \
    android.hardware.audio@2.0-service \
    android.hardware.wifi@1.0-service \
    android.hardware.graphics.composer@2.1-impl \
    android.hardware.graphics.composer@2.1-service \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.mapper@2.0-impl \
    sh_vendor \
    vintf \
    toybox_vendor \
    CarrierConfig \
    audio.r_submix.default \
    local_time.default \
    SdkSetup

PRODUCT_PACKAGES += \
    DeskClock \
    Launcher3Go \

PRODUCT_PACKAGES += \
    android.hardware.audio@2.0-impl \
    android.hardware.audio.effect@2.0-impl \

PRODUCT_PACKAGES += \
    android.hardware.audio@4.0-impl \
    android.hardware.audio.effect@4.0-impl \

#PRODUCT_PACKAGES += \
    android.hardware.broadcastradio@1.1-service \
    android.hardware.broadcastradio@1.0-impl \
    android.hardware.soundtrigger@2.0-impl

PRODUCT_PACKAGES += \
    android.hardware.keymaster@3.0-service \
    android.hardware.keymaster@3.0-impl

PRODUCT_PACKAGES += \
    android.hardware.keymaster@4.0-strongbox-service

PRODUCT_PACKAGES += \
    gps.rpi3 \
    android.hardware.gnss@1.0-service \
    android.hardware.gnss@1.0-impl

# Boot control
PRODUCT_PACKAGES += \
    android.hardware.boot@1.0-service \
    android.hardware.boot@1.0-impl

# Sensor HAL
TARGET_USES_NANOHUB_SENSORHAL := true
NANOHUB_SENSORHAL_LID_STATE_ENABLED := true
NANOHUB_SENSORHAL_NAME_OVERRIDE := sensors.rpi3
NANOHUB_SENSORHAL_SENSORLIST := $(LOCAL_PATH)/hals/sensorhal/sensorlist_neonkey.cpp
NANOHUB_SENSORHAL_DIRECT_REPORT_ENABLED := true
NANOHUB_SENSORHAL_DYNAMIC_SENSOR_EXT_ENABLED := true

PRODUCT_PACKAGES += \
    sensors.rpi3 \
    android.hardware.sensors@1.0-service \
    android.hardware.sensors@1.0-impl

PRODUCT_PACKAGES += \
    android.hardware.drm@1.0-service \
    android.hardware.drm@1.0-impl

PRODUCT_PACKAGES += \
    android.hardware.power@1.0-service \
    android.hardware.power@1.0-impl

PRODUCT_PACKAGES += \
    camera.device@1.0-impl \
    android.hardware.camera.provider@2.4-service \
    android.hardware.camera.provider@2.4-impl

PRODUCT_PACKAGES += \
    android.hardware.gatekeeper@1.0-service \
    android.hardware.gatekeeper@1.0-impl

# Memtrack
PRODUCT_PACKAGES += \
    memtrack.rpi3 \
    android.hardware.memtrack@1.0-service \
    android.hardware.memtrack@1.0-impl

# Light
PRODUCT_PACKAGES += \
    lights.rpi3 \
    android.hardware.light@2.0-service \
    android.hardware.light@2.0-impl

# need this for gles libraries to load properly
# after moving to /vendor/lib/
PRODUCT_PACKAGES += \
    vndk-sp

# Bluetooth
PRODUCT_PACKAGES += \
	btuart \
	android.hardware.bluetooth@1.0-service.rpi3 \

# Audio
PRODUCT_COPY_FILES += \
    device/brobwind/rpi3/hals/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    device/brobwind/rpi3/hals/audio/primary_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/primary_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml

# USB
PRODUCT_PACKAGES += \
    android.hardware.usb@1.0-service

# Wifi
PRODUCT_PACKAGES += \
    dhcpclient \
    dhcpserver \
    ip \
    iw \
    wpa_supplicant \

PRODUCT_PACKAGES += \
    android.hardware.wifi@1.0-service

PRODUCT_COPY_FILES += \
    device/brobwind/rpi3/hals/wifi/wpa_supplicant.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant.conf \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml \

PRODUCT_COPY_FILES += \
    device/brobwind/rpi3/init.rpi3.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.rpi3.rc \
    device/brobwind/rpi3/fstab.rpi3:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.rpi3 \
    device/brobwind/rpi3/fstab_swap.rpi3:$(TARGET_COPY_OUT_VENDOR)/etc/fstab_swap.rpi3 \
    device/brobwind/rpi3/ueventd.rpi3.rc:$(TARGET_COPY_OUT_VENDOR)/ueventd.rc \

PRODUCT_COPY_FILES += \
    device/brobwind/rpi3/hals/bootctrl/bootctrl.default.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/bootctrl.default.so \
    device/brobwind/rpi3/hals/bootctrl/bootctrl.bcb.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/bootctrl.bcb.so \

PRODUCT_COPY_FILES += \
    device/brobwind/rpi3/media_codecs.xml:system/etc/media_codecs.xml \
    device/brobwind/rpi3/media_profiles.xml:system/etc/media_profiles.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \

PRODUCT_COPY_FILES += \
    device/brobwind/rpi3/init.rpi3-net.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.rpi3-net.sh

PRODUCT_FULL_TREBLE_OVERRIDE := false

#watchdog tiggers reboot because location service is not
#responding, disble it for now.
#still keep it on internal master as it is still working
#once it is fixed in aosp, remove this block of comment.
#PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
#config.disable_location=true

# Enable Perfetto traced
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    persist.traced.enable=1

# enable Google-specific location features,
# like NetworkLocationProvider and LocationCollector
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.com.google.locationfeatures=1

# disable setupwizard
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.setupwizard.mode=DISABLED

PRODUCT_PACKAGES += \
	libdrm \
	libGLES_mesa \
	gralloc.drm \
	hwcomposer.drm \
