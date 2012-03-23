LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libsecmm
LOCAL_COPY_HEADERS := \
	include/ExynosVideoApi.h

LOCAL_SRC_FILES := \
	dec/src/ExynosVideoDecoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(BOARD_HAL_PATH)/include

LOCAL_MODULE := libExynosVideoApi
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := liblog libexynosv4l2

include $(BUILD_STATIC_LIBRARY)
