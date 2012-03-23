/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        csc.c
 *
 * @brief       color space convertion abstract source
 *
 * @author      Pyoungjae Jung(pjet.jung@samsung.com)
 *
 * @version     1.0.0
 *
 * @history
 *   2012.1.11 : Create
 */
#define LOG_TAG "libcsc"
#include <cutils/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#include "csc.h"
#include "sec_format.h"
#include "sec_utils_v4l2.h"
#include "swconverter.h"

#ifdef EXYNOS_OMX
#include "Exynos_OMX_Def.h"
#else
#include "SEC_OMX_Def.h"
#endif

#ifdef USE_FIMC
#include "hwconverter_wrapper.h"
#endif

#ifdef USE_GSCALER
#include "exynos_gscaler.h"
#endif

#define GSCALER_IMG_ALIGN 16
#define CSC_MAX_PLANES 3
#define ALIGN(x, a)       (((x) + (a) - 1) & ~((a) - 1))

typedef enum _CSC_PLANE {
    CSC_Y_PLANE = 0,
    CSC_RGB_PLANE = 0,
    CSC_U_PLANE = 1,
    CSC_UV_PLANE = 1,
    CSC_V_PLANE = 2
} CSC_PLANE;

typedef enum _CSC_HW_TYPE {
    CSC_HW_TYPE_FIMC = 0,
    CSC_HW_TYPE_GSCALER
} CSC_HW_TYPE;

typedef struct _CSC_FORMAT {
    unsigned int width;
    unsigned int height;
    unsigned int crop_left;
    unsigned int crop_top;
    unsigned int crop_width;
    unsigned int crop_height;
    unsigned int color_format;
    unsigned int cacheable;
} CSC_FORMAT;

typedef struct _CSC_BUFFER {
    unsigned char *planes[CSC_MAX_PLANES];
    int ion_fd;
} CSC_BUFFER;

typedef struct _CSC_HANDLE {
    CSC_FORMAT      dst_format;
    CSC_FORMAT      src_format;
    CSC_BUFFER      dst_buffer;
    CSC_BUFFER      src_buffer;
    CSC_METHOD      csc_method;
    CSC_HW_TYPE     csc_hw_type;
    void           *csc_hw_handle;
} CSC_HANDLE;

OMX_COLOR_FORMATTYPE hal_2_omx_pixel_format(
    unsigned int hal_format)
{
    OMX_COLOR_FORMATTYPE omx_format;
    switch (hal_format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        omx_format = OMX_COLOR_FormatYUV420SemiPlanar;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        omx_format = OMX_SEC_COLOR_FormatNV12Tiled;
        break;
    case HAL_PIXEL_FORMAT_ARGB888:
        omx_format = OMX_COLOR_Format32bitARGB8888;
        break;
    default:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    }
    return omx_format;
}

unsigned int omx_2_hal_pixel_format(
    OMX_COLOR_FORMATTYPE omx_format)
{
    unsigned int hal_format;
    switch (omx_format) {
    case OMX_COLOR_FormatYUV420Planar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case OMX_SEC_COLOR_FormatNV12Tiled:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        break;
    case OMX_COLOR_Format32bitARGB8888:
        hal_format = HAL_PIXEL_FORMAT_ARGB888;
        break;
    default:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    }
    return hal_format;
}

/* source is RGB888 */
static CSC_ERRORCODE conv_sw_src_argb888(
    CSC_HANDLE *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;

    switch (handle->dst_format.color_format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        csc_ARGB8888_to_YUV420P(
            (unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
            (unsigned char *)handle->dst_buffer.planes[CSC_U_PLANE],
            (unsigned char *)handle->dst_buffer.planes[CSC_V_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_RGB_PLANE],
            handle->src_format.width,
            handle->src_format.height);
        ret = CSC_ErrorNone;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        csc_ARGB8888_to_YUV420SP(
            (unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
            (unsigned char *)handle->dst_buffer.planes[CSC_UV_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_RGB_PLANE],
            handle->src_format.width,
            handle->src_format.height);
        ret = CSC_ErrorNone;
        break;
    default:
        ret = CSC_ErrorUnsupportFormat;
        break;
    }

    return ret;
}

/* source is NV12T */
static CSC_ERRORCODE conv_sw_src_nv12t(
    CSC_HANDLE *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;

    switch (handle->dst_format.color_format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        csc_tiled_to_linear_y_neon(
            (unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_Y_PLANE],
            handle->src_format.width,
            handle->src_format.height);
        csc_tiled_to_linear_uv_deinterleave_neon(
            (unsigned char *)handle->dst_buffer.planes[CSC_U_PLANE],
            (unsigned char *)handle->dst_buffer.planes[CSC_V_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_UV_PLANE],
            handle->src_format.width,
            handle->src_format.height / 2);
        ret = CSC_ErrorNone;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        csc_tiled_to_linear_y_neon(
            (unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_Y_PLANE],
            handle->src_format.width,
            handle->src_format.height);
        csc_tiled_to_linear_uv_neon(
            (unsigned char *)handle->dst_buffer.planes[CSC_UV_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_UV_PLANE],
            handle->src_format.width,
            handle->src_format.height / 2);
        ret = CSC_ErrorNone;
        break;
    default:
        ret = CSC_ErrorUnsupportFormat;
        break;
    }

    return ret;
}

/* source is YUV420P */
static CSC_ERRORCODE conv_sw_src_yuv420p(
    CSC_HANDLE *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;

    switch (handle->dst_format.color_format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_P:  /* bypass */
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_Y_PLANE],
               handle->src_format.width * handle->src_format.height);
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_U_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_U_PLANE],
               (handle->src_format.width * handle->src_format.height) >> 2);
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_V_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_V_PLANE],
               (handle->src_format.width * handle->src_format.height) >> 2);
        ret = CSC_ErrorNone;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_Y_PLANE],
               handle->src_format.width * handle->src_format.height);
        csc_interleave_memcpy_neon(
            (unsigned char *)handle->dst_buffer.planes[CSC_UV_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_U_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_V_PLANE],
            (handle->src_format.width * handle->src_format.height) >> 2);
        ret = CSC_ErrorNone;
        break;
    default:
        ret = CSC_ErrorUnsupportFormat;
        break;
    }

    return ret;
}

/* source is YUV420SP */
static CSC_ERRORCODE conv_sw_src_yuv420sp(
    CSC_HANDLE *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;

    switch (handle->dst_format.color_format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_Y_PLANE],
               handle->src_format.width * handle->src_format.height);
        csc_deinterleave_memcpy(
            (unsigned char *)handle->dst_buffer.planes[CSC_U_PLANE],
            (unsigned char *)handle->dst_buffer.planes[CSC_V_PLANE],
            (unsigned char *)handle->src_buffer.planes[CSC_UV_PLANE],
            handle->src_format.width * handle->src_format.height >> 1);
        ret = CSC_ErrorNone;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP: /* bypass */
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_Y_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_Y_PLANE],
               handle->src_format.width * handle->src_format.height);
        memcpy((unsigned char *)handle->dst_buffer.planes[CSC_UV_PLANE],
               (unsigned char *)handle->src_buffer.planes[CSC_UV_PLANE],
               handle->src_format.width * handle->src_format.height >> 1);
        ret = CSC_ErrorNone;
        break;
    default:
        ret = CSC_ErrorUnsupportFormat;
        break;
    }

    return ret;
}

static CSC_ERRORCODE conv_sw(
    CSC_HANDLE *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;

    switch (handle->src_format.color_format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        ret = conv_sw_src_nv12t(handle);
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        ret = conv_sw_src_yuv420p(handle);
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        ret = conv_sw_src_yuv420sp(handle);
        break;
    case HAL_PIXEL_FORMAT_ARGB888:
        ret = conv_sw_src_argb888(handle);
        break;
    default:
        ret = CSC_ErrorUnsupportFormat;
        break;
    }

    return ret;
}

static CSC_ERRORCODE conv_hw(
    CSC_HANDLE *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;

    switch (handle->csc_hw_type) {
#ifdef USE_FIMC
    case CSC_HW_TYPE_FIMC:
    {
        void *src_addr[3];
        void *dst_addr[3];
        OMX_COLOR_FORMATTYPE omx_format;
        src_addr[0] = handle->src_buffer.planes[CSC_Y_PLANE];
        src_addr[1] = handle->src_buffer.planes[CSC_UV_PLANE];
        dst_addr[0] = handle->dst_buffer.planes[CSC_Y_PLANE];
        dst_addr[1] = handle->dst_buffer.planes[CSC_U_PLANE];
        dst_addr[2] = handle->dst_buffer.planes[CSC_V_PLANE];
        omx_format = hal_2_omx_pixel_format(handle->dst_format.color_format);
        csc_hwconverter_convert_nv12t(
            handle->csc_hw_handle,
            dst_addr,
            src_addr,
            handle->dst_format.width,
            handle->dst_format.height,
            omx_format);
        break;
    }
#endif
#ifdef USE_GSCALER
    case CSC_HW_TYPE_GSCALER:
        exynos_gsc_convert(handle->csc_hw_handle);
        break;
#endif
    default:
        LOGE("%s:: unsupported csc_hw_type", __func__);
        break;
    }

    return CSC_ErrorNotImplemented;
}

void *csc_init(
    CSC_METHOD *method)
{
    CSC_HANDLE *csc_handle;
    csc_handle = (CSC_HANDLE *)malloc(sizeof(CSC_HANDLE));
    if (csc_handle == NULL)
        return NULL;

    memset(csc_handle, 0, sizeof(CSC_HANDLE));

    csc_handle->csc_method = *method;

    if (csc_handle->csc_method == CSC_METHOD_HW ||
        csc_handle->csc_method == CSC_METHOD_PREFER_HW) {
#ifdef USE_FIMC
        csc_handle->csc_hw_type = CSC_HW_TYPE_FIMC;
#endif
#ifdef USE_GSCALER
        csc_handle->csc_hw_type = CSC_HW_TYPE_GSCALER;
#endif
        switch (csc_handle->csc_hw_type) {
#ifdef USE_FIMC
        case CSC_HW_TYPE_FIMC:
            csc_handle->csc_hw_handle = csc_hwconverter_open();
            LOGD("%s:: CSC_HW_TYPE_FIMC", __func__);
            break;
#endif
#ifdef USE_GSCALER
        case CSC_HW_TYPE_GSCALER:
            csc_handle->csc_hw_handle = exynos_gsc_create();
            LOGD("%s:: CSC_HW_TYPE_GSCALER", __func__);
            break;
#endif
        default:
            LOGE("%s:: unsupported csc_hw_type, csc use sw", __func__);
            csc_handle->csc_hw_handle == NULL;
            break;
        }
    }

    if (csc_handle->csc_method == CSC_METHOD_PREFER_HW) {
        if (csc_handle->csc_hw_handle == NULL) {
            csc_handle->csc_method = CSC_METHOD_SW;
            *method = CSC_METHOD_SW;
        } else {
            csc_handle->csc_method = CSC_METHOD_HW;
            *method = CSC_METHOD_HW;
        }
    }

    if (csc_handle->csc_method == CSC_METHOD_HW) {
        if (csc_handle->csc_hw_handle == NULL) {
            LOGE("%s:: CSC_METHOD_HW can't open HW", __func__);
            free(csc_handle);
            csc_handle = NULL;
        }
    }

    LOGD("%s:: CSC_METHOD=%d", __func__, csc_handle->csc_method);

    return (void *)csc_handle;
}

CSC_ERRORCODE csc_deinit(
    void *handle)
{
    CSC_ERRORCODE ret = CSC_ErrorNone;
    CSC_HANDLE *csc_handle;

    csc_handle = (CSC_HANDLE *)handle;
    if (csc_handle->csc_method == CSC_METHOD_HW) {
        switch (csc_handle->csc_hw_type) {
#ifdef USE_FIMC
        case CSC_HW_TYPE_FIMC:
            csc_hwconverter_close(csc_handle->csc_hw_handle);
            break;
#endif
#ifdef USE_GSCALER
        case CSC_HW_TYPE_GSCALER:
            exynos_gsc_destroy(csc_handle->csc_hw_handle);
            break;
#endif
        default:
            LOGE("%s:: unsupported csc_hw_type", __func__);
            break;
        }
    }

    if (csc_handle != NULL) {
        free(csc_handle);
        ret = CSC_ErrorNone;
    }

    return ret;
}

CSC_ERRORCODE csc_get_method(
    void           *handle,
    CSC_METHOD     *method)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    *method = csc_handle->csc_method;

    return ret;
}

CSC_ERRORCODE csc_get_src_format(
    void           *handle,
    unsigned int   *width,
    unsigned int   *height,
    unsigned int   *crop_left,
    unsigned int   *crop_top,
    unsigned int   *crop_width,
    unsigned int   *crop_height,
    unsigned int   *color_format,
    unsigned int   *cacheable)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    *width = csc_handle->src_format.width;
    *height = csc_handle->src_format.height;
    *crop_left = csc_handle->src_format.crop_left;
    *crop_top = csc_handle->src_format.crop_top;
    *crop_width = csc_handle->src_format.crop_width;
    *crop_height = csc_handle->src_format.crop_height;
    *color_format = csc_handle->src_format.color_format;
    *cacheable = csc_handle->src_format.cacheable;

    return ret;
}

CSC_ERRORCODE csc_set_src_format(
    void           *handle,
    unsigned int    width,
    unsigned int    height,
    unsigned int    crop_left,
    unsigned int    crop_top,
    unsigned int    crop_width,
    unsigned int    crop_height,
    unsigned int    color_format,
    unsigned int    cacheable)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    csc_handle->src_format.width = width;
    csc_handle->src_format.height = height;
    csc_handle->src_format.crop_left = crop_left;
    csc_handle->src_format.crop_top = crop_top;
    csc_handle->src_format.crop_width = crop_width;
    csc_handle->src_format.crop_height = crop_height;
    csc_handle->src_format.color_format = color_format;
    csc_handle->src_format.cacheable = cacheable;

    if (csc_handle->csc_method == CSC_METHOD_HW) {
        switch (csc_handle->csc_hw_type) {
        case CSC_HW_TYPE_FIMC:
            break;
#ifdef USE_GSCALER
        case CSC_HW_TYPE_GSCALER:
            exynos_gsc_set_src_format(
                csc_handle->csc_hw_handle,
                ALIGN(csc_handle->src_format.width, GSCALER_IMG_ALIGN),
                ALIGN(csc_handle->src_format.height, GSCALER_IMG_ALIGN),
                csc_handle->src_format.crop_left,
                csc_handle->src_format.crop_top,
                ALIGN(csc_handle->src_format.crop_width, GSCALER_IMG_ALIGN),
                ALIGN(csc_handle->src_format.crop_height, GSCALER_IMG_ALIGN),
                HAL_PIXEL_FORMAT_2_V4L2_PIX(csc_handle->src_format.color_format),
                csc_handle->src_format.cacheable);
            break;
#endif
        default:
            LOGE("%s:: unsupported csc_hw_type", __func__);
            break;
        }
    }

    return ret;
}

CSC_ERRORCODE csc_get_dst_format(
    void           *handle,
    unsigned int   *width,
    unsigned int   *height,
    unsigned int   *crop_left,
    unsigned int   *crop_top,
    unsigned int   *crop_width,
    unsigned int   *crop_height,
    unsigned int   *color_format,
    unsigned int   *cacheable)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    *width = csc_handle->dst_format.width;
    *height = csc_handle->dst_format.height;
    *crop_left = csc_handle->dst_format.crop_left;
    *crop_top = csc_handle->dst_format.crop_top;
    *crop_width = csc_handle->dst_format.crop_width;
    *crop_height = csc_handle->dst_format.crop_height;
    *color_format = csc_handle->dst_format.color_format;
    *cacheable = csc_handle->dst_format.cacheable;

    return ret;
}

CSC_ERRORCODE csc_set_dst_format(
    void           *handle,
    unsigned int    width,
    unsigned int    height,
    unsigned int    crop_left,
    unsigned int    crop_top,
    unsigned int    crop_width,
    unsigned int    crop_height,
    unsigned int    color_format,
    unsigned int    cacheable)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    csc_handle->dst_format.width = width;
    csc_handle->dst_format.height = height;
    csc_handle->dst_format.crop_left = crop_left;
    csc_handle->dst_format.crop_top = crop_top;
    csc_handle->dst_format.crop_width = crop_width;
    csc_handle->dst_format.crop_height = crop_height;
    csc_handle->dst_format.color_format = color_format;
    csc_handle->dst_format.cacheable = cacheable;

    if (csc_handle->csc_method == CSC_METHOD_HW) {
        switch (csc_handle->csc_hw_type) {
        case CSC_HW_TYPE_FIMC:
            break;
#ifdef USE_GSCALER
        case CSC_HW_TYPE_GSCALER:
            exynos_gsc_set_dst_format(
                csc_handle->csc_hw_handle,
                ALIGN(csc_handle->dst_format.width, GSCALER_IMG_ALIGN),
                ALIGN(csc_handle->dst_format.height, GSCALER_IMG_ALIGN),
                csc_handle->dst_format.crop_left,
                csc_handle->dst_format.crop_top,
                ALIGN(csc_handle->dst_format.crop_width, GSCALER_IMG_ALIGN),
                ALIGN(csc_handle->dst_format.crop_height, GSCALER_IMG_ALIGN),
                HAL_PIXEL_FORMAT_2_V4L2_PIX(csc_handle->dst_format.color_format),
                csc_handle->dst_format.cacheable);
            break;
#endif
        default:
            LOGE("%s:: unsupported csc_hw_type", __func__);
            break;
        }
    }

    return ret;
}

CSC_ERRORCODE csc_set_src_buffer(
    void           *handle,
    unsigned char  *y,
    unsigned char  *u,
    unsigned char  *v,
    int             ion_fd)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;
    void *addr[3] = {NULL, };

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    csc_handle->src_buffer.planes[CSC_Y_PLANE] = y;
    csc_handle->src_buffer.planes[CSC_U_PLANE] = u;
    csc_handle->src_buffer.planes[CSC_V_PLANE] = v;

    if (csc_handle->csc_method == CSC_METHOD_HW) {
        addr[0] = csc_handle->src_buffer.planes[CSC_Y_PLANE];
        addr[1] = csc_handle->src_buffer.planes[CSC_U_PLANE];
        addr[2] = csc_handle->src_buffer.planes[CSC_V_PLANE];

        switch (csc_handle->csc_hw_type) {
        case CSC_HW_TYPE_FIMC:
            break;
#ifdef USE_GSCALER
        case CSC_HW_TYPE_GSCALER:
            exynos_gsc_set_src_addr(csc_handle->csc_hw_handle, addr);
            break;
#endif
        default:
            LOGE("%s:: unsupported csc_hw_type", __func__);
            break;
        }
    }

    return ret;
}

CSC_ERRORCODE csc_set_dst_buffer(
    void           *handle,
    unsigned char  *y,
    unsigned char  *u,
    unsigned char  *v,
    int             ion_fd)
{
    CSC_HANDLE *csc_handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;
    void *addr[3] = {NULL, };

    if (handle == NULL)
        return CSC_ErrorNotInit;

    csc_handle = (CSC_HANDLE *)handle;
    csc_handle->dst_buffer.planes[CSC_Y_PLANE] = y;
    csc_handle->dst_buffer.planes[CSC_U_PLANE] = u;
    csc_handle->dst_buffer.planes[CSC_V_PLANE] = v;

    if (csc_handle->csc_method == CSC_METHOD_HW) {
        addr[0] = csc_handle->dst_buffer.planes[CSC_Y_PLANE];
        addr[1] = csc_handle->dst_buffer.planes[CSC_U_PLANE];
        addr[2] = csc_handle->dst_buffer.planes[CSC_V_PLANE];

        switch (csc_handle->csc_hw_type) {
        case CSC_HW_TYPE_FIMC:
            break;
#ifdef USE_GSCALER
        case CSC_HW_TYPE_GSCALER:
            exynos_gsc_set_dst_addr(csc_handle->csc_hw_handle, addr);
            break;
#endif
        default:
            LOGE("%s:: unsupported csc_hw_type", __func__);
            break;
        }
    }

    return ret;
}

CSC_ERRORCODE csc_convert(
    void *handle)
{
    CSC_HANDLE *csc_handle = (CSC_HANDLE *)handle;
    CSC_ERRORCODE ret = CSC_ErrorNone;

    if (csc_handle == NULL)
        return CSC_ErrorNotInit;

    if (csc_handle->csc_method == CSC_METHOD_HW)
        ret = conv_hw(csc_handle);
    else
        ret = conv_sw(csc_handle);

    return ret;
}
