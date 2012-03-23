LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/srp_api.c \
	src/srp_api_ctrl.c

LOCAL_MODULE := libsrpapi

LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES :=

LOCAL_COPY_HEADERS := \
	include/srp_api.h \
	include/srp_api_ctrl.h \
	include/srp_ioctl.h

include $(BUILD_STATIC_LIBRARY)
