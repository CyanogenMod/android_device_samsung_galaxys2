LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	SEC_OMX_Mpeg4enc.c \
	library_register.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX.SEC.M4V.Encoder
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/omx

LOCAL_CFLAGS :=

ifeq ($(BOARD_NONBLOCK_MODE_PROCESS), true)
LOCAL_CFLAGS += -DNONBLOCK_MODE_PROCESS
endif

ifeq ($(BOARD_USE_METADATABUFFERTYPE), true)
LOCAL_CFLAGS += -DUSE_METADATABUFFERTYPE
endif

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libSEC_OMX_Venc libsecosal libsecbasecomponent \
	libswconverter libsecmfcapi
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui \
	libSEC_OMX_Resourcemanager libcsc

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/osal \
	$(SEC_OMX_TOP)/core \
	$(SEC_OMX_COMPONENT)/common \
	$(SEC_OMX_COMPONENT)/video/enc \
    $(TARGET_OUT_HEADERS)/$(SEC_COPY_HEADERS_TO)

include $(BUILD_SHARED_LIBRARY)
