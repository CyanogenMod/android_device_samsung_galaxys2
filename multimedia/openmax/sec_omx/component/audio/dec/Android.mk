LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SEC_OMX_Adec.c

LOCAL_MODULE := libSEC_OMX_Adec
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/osal \
	$(SEC_OMX_TOP)/core \
	$(SEC_OMX_COMPONENT)/common \
	$(SEC_OMX_COMPONENT)/audio/dec \
	$(TARGET_OUT_HEADERS)/$(SEC_COPY_HEADERS_TO)

include $(BUILD_STATIC_LIBRARY)
