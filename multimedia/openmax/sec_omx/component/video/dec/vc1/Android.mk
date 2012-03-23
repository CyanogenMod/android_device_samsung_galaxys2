LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	SEC_OMX_Wmvdec.c \
	library_register.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX.SEC.WMV.Decoder
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/omx

LOCAL_CFLAGS :=

ifeq ($(BOARD_NONBLOCK_MODE_PROCESS), true)
LOCAL_CFLAGS += -DNONBLOCK_MODE_PROCESS
endif

ifeq ($(BOARD_USE_ANB), true)
LOCAL_CFLAGS += -DUSE_ANB
ifeq ($(BOARD_USE_CSC_FIMC), true)
ifeq ($(BOARD_USE_V4L2_ION), false)
LOCAL_CFLAGS += -DUSE_CSC_FIMC
endif
endif

ifeq ($(BOARD_USE_CSC_GSCALER), true)
LOCAL_CFLAGS += -DUSE_CSC_GSCALER
endif
endif

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libSEC_OMX_Vdec libsecosal libsecbasecomponent \
	libswconverter libsecmfcapi
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui \
	libSEC_OMX_Resourcemanager libcsc

ifeq ($(filter-out exynos4,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SHARED_LIBRARIES += libfimc libhwconverter
endif

ifeq ($(filter-out exynos5,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

#ifeq ($(BOARD_USE_V4L2_ION),true)
#LOCAL_SHARED_LIBRARIES += libion
#endif

ifeq ($(BOARD_USES_MFC_FPS),true)
LOCAL_CFLAGS += -DCONFIG_MFC_FPS
endif

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/osal \
	$(SEC_OMX_TOP)/core \
	$(SEC_OMX_COMPONENT)/common \
	$(SEC_OMX_COMPONENT)/video/dec \
	$(TARGET_OUT_HEADERS)/$(SEC_COPY_HEADERS_TO) \
	$(BOARD_HAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)
