LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := libsa_jni
LOCAL_SRC_FILES := SACtrl.c

LOCAL_SHARED_LIBRARIES :=  libcutils
LOCAL_STATIC_LIBRARIES := libsrpapi

include $(BUILD_SHARED_LIBRARY)
