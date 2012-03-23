LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libsecmm
LOCAL_COPY_HEADERS := \
	include/srp_api.h \
	include/srp_ioctl.h \
	include/srp_error.h

LOCAL_SRC_FILES := \
	src/srp_api.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_MODULE := libsrpapi

LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES :=

include $(BUILD_STATIC_LIBRARY)
