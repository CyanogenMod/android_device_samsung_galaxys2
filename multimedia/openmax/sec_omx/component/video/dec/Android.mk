LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SEC_OMX_Vdec.c

LOCAL_MODULE := libSEC_OMX_Vdec
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/osal \
	$(SEC_OMX_TOP)/core \
	$(SEC_OMX_COMPONENT)/common \
	$(SEC_OMX_COMPONENT)/video/dec

ifeq ($(BOARD_USE_ANB), true)
LOCAL_STATIC_LIBRARIES := libsecosal
LOCAL_CFLAGS += -DUSE_ANB
endif

include $(BUILD_STATIC_LIBRARY)
