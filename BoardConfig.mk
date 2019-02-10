# config.mk
#
# Product-specific compile-time definitions.
#

# The generic product target doesn't have any hardware-specific pieces.
TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := false
TARGET_ARCH := arm

TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := generic
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
HAVE_HTC_AUDIO_DRIVER := true
BOARD_USES_GENERIC_AUDIO := true
TARGET_BOOTLOADER_BOARD_NAME := rpi3

TARGET_USES_64_BIT_BINDER := true

#SMALLER_FONT_FOOTPRINT := true
#MINIMAL_FONT_FOOTPRINT := true
# Exclude serif fonts for saving system.img size.
EXCLUDE_SERIF_FONTS := true

# Some framework code requires this to enable BT
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/brobwind/rpi3/hals/bluetooth/cfg

# no hardware camera
USE_CAMERA_STUB := true

TARGET_USES_HWC2 := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3

# Build and enable the OpenGL ES View renderer.
USE_OPENGL_RENDERER := true

BOARD_SYSTEMIMAGE_PARTITION_SIZE := $(shell echo $$((650*1024*1024))) # 650MB
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4

BOARD_USERDATAIMAGE_PARTITION_SIZE := $(shell echo $$((128*1024*1024))) # 256MB
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := ext4

TARGET_COPY_OUT_VENDOR := vendor
BOARD_VENDORIMAGE_PARTITION_SIZE := $(shell echo $$((256*1024*1024))) # 256MB
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4

BOARD_FLASH_BLOCK_SIZE := 512
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

DEVICE_MANIFEST_FILE := device/brobwind/rpi3/manifest.xml
DEVICE_MATRIX_FILE   := device/brobwind/rpi3/compatibility_matrix.xml

# Android generic system image always create metadata partition
BOARD_USES_METADATA_PARTITION := true

BOARD_SEPOLICY_DIRS += device/brobwind/rpi3/sepolicy
BOARD_PROPERTY_OVERRIDES_SPLIT_ENABLED := true
BOARD_VNDK_VERSION := current

TARGET_USERIMAGES_USE_EXT4 := true
# Use mke2fs to create ext4 images
TARGET_USES_MKE2FS := true

# Audio policy configuration
USE_XML_AUDIO_POLICY_CONF := 1

# Wifi
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_WLAN_DEVICE                := bcmdhd
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)

# Enable A/B update
TARGET_NO_RECOVERY := true
BOARD_BUILD_SYSTEM_ROOT_IMAGE := true

TARGET_AUX_OS_VARIANT_LIST := neonkey

# Bootimage
BOARD_KERNEL_CMDLINE     := console=ttyS0,115200 buildvariant=eng
BOARD_KERNEL_BASE        := 0x10000000
BOARD_MKBOOTIMG_ARGS     := --kernel_offset 0x080000 --ramdisk_offset 0x01000000
BOARD_KERNEL_PAGESIZE    := 2048

# GPU Acceleration
BOARD_GPU_DRIVERS := vc4
BOARD_USES_DRM_HWCOMPOSER := true
BOARD_USES_MINIGBM := true
BOARD_DRM_HWCOMPOSER_BUFFER_IMPORTER := minigbm
