LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_AUDIO_PATH :=$(LOCAL_PATH)

ifeq ($(BOARD_USE_ALP_AUDIO), true)
  include $(LOCAL_AUDIO_PATH)/srp/alp/Android.mk
else
  ifeq ($(USE_ULP_AUDIO), true)
    include $(LOCAL_AUDIO_PATH)/srp/ulp/Android.mk
    include $(LOCAL_AUDIO_PATH)/srp/libsa_jni/Android.mk
  endif
endif
