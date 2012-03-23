LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(filter-out exynos4,$(TARGET_BOARD_PLATFORM)),)
include   $(LOCAL_PATH)/exynos4/Android.mk
endif

ifeq ($(filter-out exynos5,$(TARGET_BOARD_PLATFORM)),)
include   $(LOCAL_PATH)/exynos5/Android.mk
endif
