LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	SEC_OMX_Basecomponent.c \
	SEC_OMX_Baseport.c

LOCAL_MODULE := libsecbasecomponent

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := libsecosal
LOCAL_SHARED_LIBRARIES := libcutils libutils

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/osal

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	SEC_OMX_Resourcemanager.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libSEC_OMX_Resourcemanager

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := libsecosal
LOCAL_SHARED_LIBRARIES := libcutils libutils

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/osal

include $(BUILD_SHARED_LIBRARY)
