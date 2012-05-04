#
# Copyright (C) 2012 The Android Open-Source Project
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
#

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk
USE_CAMERA_STUB := true
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_LIBSECRIL_STUB := true

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_ARCH_VARIANT_CPU := cortex-a9
ARCH_ARM_HAVE_NEON := true
ARCH_ARM_HAVE_TLS_REGISTER := true
EXYNOS4210_ENHANCEMENTS := true

ifdef EXYNOS4210_ENHANCEMENTS
COMMON_GLOBAL_CFLAGS += -DEXYNOS4210_ENHANCEMENTS
COMMON_GLOBAL_CFLAGS += -DSURFACEFLINGER_FORCE_SCREEN_RELEASE
endif

TARGET_BOARD_PLATFORM := exynos4
TARGET_SOC := exynos4210
TARGET_BOOTLOADER_BOARD_NAME := smdk4210
TARGET_BOARD_INFO_FILE := device/samsung/galaxys2/board-info.txt

TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

TARGET_PROVIDES_INIT := true
TARGET_PROVIDES_INIT_TARGET_RC := true
TARGET_RECOVERY_INITRC := device/samsung/galaxys2/recovery.rc

BOARD_NAND_PAGE_SIZE := 4096 -s 128
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_BASE := 0x40000000
BOARD_KERNEL_CMDLINE := console=ttySAC2,115200 consoleblank=0

# Inline kernel building
TARGET_KERNEL_SOURCE := kernel/samsung/smdk4210
TARGET_KERNEL_CONFIG := cyanogenmod_galaxys2_defconfig

# Filesystem
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2147483648
BOARD_FLASH_BLOCK_SIZE := 4096

# Releasetools
TARGET_RELEASETOOL_OTA_FROM_TARGET_SCRIPT := ./device/samsung/galaxys2/releasetools/galaxys2_ota_from_target_files
TARGET_RELEASETOOL_IMG_FROM_TARGET_SCRIPT := ./device/samsung/galaxys2/releasetools/galaxys2_img_from_target_files

# Graphics
BOARD_EGL_CFG := device/samsung/galaxys2/configs/egl.cfg
USE_OPENGL_RENDERER := true

# HWComposer
BOARD_USES_HWCOMPOSER := true
BOARD_USE_SECTVOUT := true
BOARD_USES_FIMGAPI := true

# OMX
BOARD_HAVE_CODEC_SUPPORT := SAMSUNG_CODEC_SUPPORT
COMMON_GLOBAL_CFLAGS += -DSAMSUNG_CODEC_SUPPORT
BOARD_NONBLOCK_MODE_PROCESS := true
BOARD_USE_STOREMETADATA := true
BOARD_USE_METADATABUFFERTYPE := true
BOARD_USES_MFC_FPS := true

# Audio
BOARD_USE_YAMAHAPLAYER := true
BOARD_USE_SAMSUNG_SEPARATEDSTREAM := true

# RIL
BOARD_MOBILEDATA_INTERFACE_NAME := "pdp0"

# Wifi
BOARD_WLAN_DEVICE                := bcmdhd
BOARD_WLAN_DEVICE_REV            := bcm4330_b1
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_bcmdhd
WIFI_DRIVER_MODULE_PATH          := "/system/lib/modules/dhd.ko"
WIFI_DRIVER_FW_PATH_PARAM        := "/sys/module/dhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA          := "/system/etc/wifi/bcm4330_sta.bin"
WIFI_DRIVER_FW_PATH_AP           := "/system/etc/wifi/bcm4330_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P          := "/system/etc/wifi/bcm4330_p2p.bin"
WIFI_DRIVER_MODULE_NAME          := "dhd"
WIFI_DRIVER_MODULE_ARG           := "firmware_path=/system/etc/wifi/bcm4330_sta.bin nvram_path=/system/etc/wifi/nvram_net.txt"
WIFI_BAND                        := 802_11_ABG

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Vold
BOARD_VOLD_MAX_PARTITIONS := 12
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/platform/s3c-usbgadget/gadget/lun%d/file"

# Recovery
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/samsung/galaxys2/recovery/recovery_keys.c
BOARD_CUSTOM_GRAPHICS := ../../../device/samsung/galaxys2/recovery/graphics.c
BOARD_UMS_LUNFILE := "/sys/class/android_usb/android0/f_mass_storage/lun0/file"
BOARD_USES_MMCUTILS := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_HAS_NO_SELECT_BUTTON := true

TARGET_SPECIFIC_HEADER_PATH := device/samsung/galaxys2/overlay/include

# assert
TARGET_OTA_ASSERT_DEVICE := galaxys2,GT-I9100,GT-I9100M,GT-I9100P,GT-I9100T

# Use the non-open-source parts, if they're present
-include vendor/samsung/galaxys2/BoardConfigVendor.mk

BOARD_CUSTOM_BOOTIMG_MK := device/samsung/galaxys2/shbootimg.mk
