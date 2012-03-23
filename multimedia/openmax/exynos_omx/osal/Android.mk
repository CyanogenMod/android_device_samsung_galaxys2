LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OSAL_Android.cpp \
	Exynos_OSAL_Event.c \
	Exynos_OSAL_Queue.c \
	Exynos_OSAL_ETC.c \
	Exynos_OSAL_Mutex.c \
	Exynos_OSAL_Thread.c \
	Exynos_OSAL_Memory.c \
	Exynos_OSAL_Semaphore.c \
	Exynos_OSAL_Library.c \
	Exynos_OSAL_Log.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosOMX_OSAL

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := libcutils libutils \
	libui \
	libhardware \
	libandroid_runtime \
	libsurfaceflinger_client \
	libbinder \
	libmedia

LOCAL_C_INCLUDES := \
    $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	$(TARGET_OUT_HEADERS)/$(EXYNOS_OMX_COPY_HEADERS_TO) \
	$(LOCAL_PATH)/../../../../include

include $(BUILD_STATIC_LIBRARY)
