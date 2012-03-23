LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_Mpeg4dec.c \
	library_register.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX.SEC.M4V.Decoder
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

LOCAL_STATIC_LIBRARIES := libExynosOMX_Vdec libExynosOMX_OSAL libExynosOMX_Basecomponent \
	libswconverter libExynosVideoApi
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui \
	libExynosOMX_Resourcemanager libcsc

ifeq ($(filter-out exynos4,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SHARED_LIBRARIES += libfimc libhwconverter
endif

ifeq ($(filter-out exynos5,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

ifeq ($(BOARD_USES_MFC_FPS),true)
LOCAL_CFLAGS += -DCONFIG_MFC_FPS
endif

ifeq ($(BOARD_USE_EXYNOS_OMX), true)
LOCAL_SHARED_LIBRARIES += libexynosv4l2
endif

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	$(TARGET_OUT_HEADERS)/$(EXYNOS_OMX_COPY_HEADERS_TO) \
	$(BOARD_HAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)
