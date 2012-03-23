LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libsecmm
LOCAL_COPY_HEADERS := \
	csc.h

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	csc.c

ifeq ($(BOARD_USE_EXYNOS_OMX), true)
OMX_NAME := exynos
else
OMX_NAME := sec
endif

LOCAL_C_INCLUDES := \
	$(TOP)/$(BOARD_HMM_PATH)/openmax/$(OMX_NAME)_omx/include/khronos \
	$(TOP)/$(BOARD_HMM_PATH)/openmax/$(OMX_NAME)_omx/include/$(OMX_NAME)

LOCAL_CFLAGS :=

LOCAL_MODULE := libcsc

LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libswconverter
LOCAL_SHARED_LIBRARIES := liblog

ifeq ($(BOARD_USE_SAMSUNG_COLORFORMAT), true)
LOCAL_CFLAGS += -DUSE_SAMSUNG_COLORFORMAT
endif

ifeq ($(TARGET_BOARD_PLATFORM), exynos4)
LOCAL_SRC_FILES += hwconverter_wrapper.cpp
LOCAL_C_INCLUDES += $(TOP)/$(BOARD_HMM_PATH)/utils/csc/exynos4 \
	$(TOP)/$(BOARD_HAL_PATH)/include \
	$(TOP)/$(BOARD_HAL_PATH)/libhwconverter
LOCAL_CFLAGS += -DUSE_FIMC
LOCAL_SHARED_LIBRARIES += libfimc libhwconverter
endif

ifeq ($(TARGET_BOARD_PLATFORM), exynos5)
LOCAL_C_INCLUDES += $(TOP)/$(BOARD_HMM_PATH)/utils/csc/exynos5 \
	$(TOP)/device/samsung/exynos5/include
LOCAL_CFLAGS += -DUSE_GSCALER
LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

ifeq ($(BOARD_USE_V4L2_ION),true)
LOCAL_CFLAGS += -DUSE_ION
LOCAL_SHARED_LIBRARIES += libion
endif

ifeq ($(BOARD_USE_EXYNOS_OMX), true)
LOCAL_CFLAGS += -DEXYNOS_OMX
endif

include $(BUILD_SHARED_LIBRARY)
