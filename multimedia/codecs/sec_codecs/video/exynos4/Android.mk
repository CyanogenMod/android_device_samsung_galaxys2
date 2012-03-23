LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_VIDEO_PATH :=$(LOCAL_PATH)

ifeq ($(BOARD_USE_V4L2), true)
include    $(LOCAL_VIDEO_PATH)/mfc_v4l2/Android.mk
else
include    $(LOCAL_VIDEO_PATH)/mfc/Android.mk
endif
