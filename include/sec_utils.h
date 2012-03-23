/*
 * Copyright@ Samsung Electronics Co. LTD
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

#ifndef __SAMSUNG_SYSLSI_SEC_COMMON_H__
#define __SAMSUNG_SYSLSI_SEC_COMMON_H__

//---------------------------------------------------------//
// Include
//---------------------------------------------------------//

#include <hardware/hardware.h>
#include "sec_format.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/videodev2.h>
#include "videodev2_samsung.h"

#ifdef __cplusplus
}
#endif

//---------------------------------------------------------//
// Common structure                                        //
//---------------------------------------------------------//
struct ADDRS {
    unsigned int addr_y;
    unsigned int addr_cbcr;
    unsigned int buf_idx;
    unsigned int reserved;
};

//---------------------------------------------------------//
// Common function                                         //
//---------------------------------------------------------//
inline int HAL_PIXEL_FORMAT_2_V4L2_PIX(int HAL_PIXEL_FORMAT)
{
    int V4L2_PIX = -1;

    switch (HAL_PIXEL_FORMAT) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
        V4L2_PIX = V4L2_PIX_FMT_RGB32;
        break;

    case HAL_PIXEL_FORMAT_RGB_888:
        V4L2_PIX = V4L2_PIX_FMT_RGB24;
        break;

    case HAL_PIXEL_FORMAT_RGB_565:
        V4L2_PIX = V4L2_PIX_FMT_RGB565;
        break;

    case HAL_PIXEL_FORMAT_BGRA_8888:
        V4L2_PIX = V4L2_PIX_FMT_RGB32;
        break;

    case HAL_PIXEL_FORMAT_RGBA_5551:
        V4L2_PIX = V4L2_PIX_FMT_RGB555X;
        break;

    case HAL_PIXEL_FORMAT_RGBA_4444:
        V4L2_PIX = V4L2_PIX_FMT_RGB444;
        break;

    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        V4L2_PIX = V4L2_PIX_FMT_YUV420;
        break;

    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
        V4L2_PIX = V4L2_PIX_FMT_NV61;
        break;

    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
        V4L2_PIX = V4L2_PIX_FMT_NV12;
        break;

    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
        V4L2_PIX = V4L2_PIX_FMT_YUYV;
        break;

    case HAL_PIXEL_FORMAT_YCbCr_422_P:
        V4L2_PIX = V4L2_PIX_FMT_YUV422P;
        break;

    case HAL_PIXEL_FORMAT_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
        V4L2_PIX = V4L2_PIX_FMT_UYVY;
        break;

    case HAL_PIXEL_FORMAT_YCrCb_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
        V4L2_PIX = V4L2_PIX_FMT_NV16;
        break;

    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
        V4L2_PIX = V4L2_PIX_FMT_NV21;
        break;

    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
        V4L2_PIX = V4L2_PIX_FMT_NV12T;
        break;

   case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
        V4L2_PIX = V4L2_PIX_FMT_YVYU;
        break;

   case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
        V4L2_PIX = V4L2_PIX_FMT_VYUY;
        break;

    default:
        LOGE("%s::unmatched HAL_PIXEL_FORMAT color_space(0x%x)\n",
                __func__, HAL_PIXEL_FORMAT);
        break;
    }

    return V4L2_PIX;
}

inline int V4L2_PIX_2_HAL_PIXEL_FORMAT(int V4L2_PIX)
{
    int HAL_PIXEL_FORMAT = -1;

    switch (V4L2_PIX) {
    case V4L2_PIX_FMT_RGB32:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_RGBA_8888;
        break;

    case V4L2_PIX_FMT_RGB24:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_RGB_888;
        break;

    case V4L2_PIX_FMT_RGB565:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_RGB_565;
        break;

    case V4L2_PIX_FMT_BGR32:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_BGRA_8888;
        break;

    case V4L2_PIX_FMT_RGB555X:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_RGBA_5551;
        break;

    case V4L2_PIX_FMT_RGB444:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_RGBA_4444;
        break;

    case V4L2_PIX_FMT_YUV420:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;

    case V4L2_PIX_FMT_NV16:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP;
        break;

    case V4L2_PIX_FMT_NV12:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP;
        break;

    case V4L2_PIX_FMT_YUYV:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I;
        break;

    case V4L2_PIX_FMT_YUV422P:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_YCbCr_422_P;
        break;

    case V4L2_PIX_FMT_UYVY:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I;
        break;

    case V4L2_PIX_FMT_NV21:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP;
        break;

    case V4L2_PIX_FMT_NV12T:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED;
        break;

    case V4L2_PIX_FMT_NV61:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP;
        break;

    case V4L2_PIX_FMT_YVYU:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I;
        break;

    case V4L2_PIX_FMT_VYUY:
        HAL_PIXEL_FORMAT = HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I;
        break;

    default:
        LOGE("%s::unmatched V4L2_PIX color_space(%d)\n",
                __func__, V4L2_PIX);
        break;
    }

    return HAL_PIXEL_FORMAT;
}

#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)

#define GET_32BPP_FRAME_SIZE(w, h)  (((w) * (h)) << 2)
#define GET_24BPP_FRAME_SIZE(w, h)  (((w) * (h)) * 3)
#define GET_16BPP_FRAME_SIZE(w, h)  (((w) * (h)) << 1)

inline unsigned int FRAME_SIZE(int HAL_PIXEL_FORMAT, int w, int h)
{
    unsigned int frame_size = 0;
    unsigned int size       = 0;

    switch (HAL_PIXEL_FORMAT) {
    // 16bpp
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RGBA_5551:
    case HAL_PIXEL_FORMAT_RGBA_4444:
        frame_size = GET_16BPP_FRAME_SIZE(w, h);
        break;

    // 24bpp
    case HAL_PIXEL_FORMAT_RGB_888:
        frame_size = GET_24BPP_FRAME_SIZE(w, h);
        break;

    // 32bpp
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
        frame_size = GET_32BPP_FRAME_SIZE(w, h);
        break;

    // 12bpp
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_YCbCr_420_I:
    case HAL_PIXEL_FORMAT_CbYCrY_420_I:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
        size = w * h;
        frame_size = size + ((size >> 2) << 1);
        break;

    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
        frame_size =   ALIGN_TO_8KB(ALIGN_TO_128B(w) * ALIGN_TO_32B(h))
                     + ALIGN_TO_8KB(ALIGN_TO_128B(w) * ALIGN_TO_32B(h >> 1));
        break;

    // 16bpp
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_YCbCr_422_P:
    case HAL_PIXEL_FORMAT_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_YCrCb_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP:
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
    case HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I:
        frame_size = GET_16BPP_FRAME_SIZE(w, h);
        break;

    default:
        LOGD("%s::no matching source colorformat(0x%x), w(%d), h(%d) fail\n",
                __func__, HAL_PIXEL_FORMAT, w, h);
        break;
    }

    return frame_size;
}

#endif //__SAMSUNG_SYSLSI_SEC_COMMON_H__