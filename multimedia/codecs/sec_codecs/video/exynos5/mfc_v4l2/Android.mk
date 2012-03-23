LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libsecmm
LOCAL_COPY_HEADERS := \
	include/mfc_errno.h \
	include/mfc_interface.h \
	include/SsbSipMfcApi.h

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	dec/src/SsbSipMfcDecAPI.c \
	enc/src/SsbSipMfcEncAPI.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(BOARD_HAL_PATH)/include

LOCAL_MODULE := libsecmfcapi

LOCAL_PRELINK_MODULE := false

ifeq ($(BOARD_USE_S3D_SUPPORT), true)
LOCAL_CFLAGS += -DS3D_SUPPORT
endif

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := liblog

#ifeq ($(BOARD_USE_V4L2_ION),true)
#LOCAL_CFLAGS += -DUSE_ION
#LOCAL_SHARED_LIBRARIES += libion
#endif

include $(BUILD_STATIC_LIBRARY)
