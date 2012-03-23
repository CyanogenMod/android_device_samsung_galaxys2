LOCAL_PATH := $(call my-dir)
BOARD_USE_ANB := true

include $(CLEAR_VARS)

SEC_OMX_TOP := $(LOCAL_PATH)

SEC_COPY_HEADERS_TO := libsecmm

SEC_OMX_INC := $(SEC_OMX_TOP)/include/
SEC_OMX_COMPONENT := $(SEC_OMX_TOP)/component

include $(SEC_OMX_TOP)/osal/Android.mk
include $(SEC_OMX_TOP)/core/Android.mk

include $(SEC_OMX_COMPONENT)/common/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/h264/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/mpeg4/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/vc1/Android.mk
include $(SEC_OMX_COMPONENT)/video/enc/Android.mk
include $(SEC_OMX_COMPONENT)/video/enc/h264/Android.mk
include $(SEC_OMX_COMPONENT)/video/enc/mpeg4/Android.mk

ifeq ($(filter-out exynos5,$(TARGET_BOARD_PLATFORM)),)
include $(SEC_OMX_COMPONENT)/video/dec/vp8/Android.mk
endif

ifeq ($(BOARD_USE_ALP_AUDIO), true)
include $(SEC_OMX_COMPONENT)/audio/dec/Android.mk
include $(SEC_OMX_COMPONENT)/audio/dec/mp3/Android.mk
endif
