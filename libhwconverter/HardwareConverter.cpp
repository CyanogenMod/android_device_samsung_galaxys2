/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <utils/Log.h>

#include "SEC_OMX_Def.h"
#include "SecFimc.h"
#include "HardwareConverter.h"

HardwareConverter::HardwareConverter()
{
    SecFimc* handle_fimc = new SecFimc();
    mSecFimc = (void *)handle_fimc;

    if (handle_fimc->create(SecFimc::DEV_2, SecFimc::MODE_MULTI_BUF, 1) == false)
        bHWconvert_flag = 0;
    else
        bHWconvert_flag = 1;
}

HardwareConverter::~HardwareConverter()
{
    SecFimc* handle_fimc = (SecFimc*)mSecFimc;
    handle_fimc->destroy();
    delete mSecFimc;
}

bool HardwareConverter::convert(
    void * src_addr,
    void *dst_addr,
    OMX_COLOR_FORMATTYPE src_format,
    int32_t width,
    int32_t height,
    OMX_COLOR_FORMATTYPE dst_format)
{
    SecFimc* handle_fimc = (SecFimc*)mSecFimc;

    int rotate_value = 0;
    unsigned int src_crop_x = 0;
    unsigned int src_crop_y = 0;
    unsigned int src_crop_width = width;
    unsigned int src_crop_height = height;

    unsigned int dst_crop_x = 0;
    unsigned int dst_crop_y = 0;
    unsigned int dst_crop_width = width;
    unsigned int dst_crop_height = height;

    void **src_addr_array = (void **)src_addr;
    void **dst_addr_array = (void **)dst_addr;

    unsigned int src_har_format = OMXtoHarPixelFomrat(src_format);
    unsigned int dst_har_format = OMXtoHarPixelFomrat(dst_format);

    // set post processor configuration
    if (!handle_fimc->setSrcParams(width, height, src_crop_x, src_crop_y,
                                   &src_crop_width, &src_crop_height,
                                   src_har_format)) {
        LOGE("%s:: setSrcParms() failed", __func__);
        return false;
    }

    if (!handle_fimc->setSrcAddr((unsigned int)src_addr_array[0],
                                 (unsigned int)src_addr_array[1],
                                 (unsigned int)src_addr_array[1],
                                 src_har_format)) {
        LOGE("%s:: setSrcPhyAddr() failed", __func__);
        return false;
    }

    if (!handle_fimc->setRotVal(rotate_value)) {
        LOGE("%s:: setRotVal() failed", __func__);
        return false;
    }

    if (!handle_fimc->setDstParams(width, height, dst_crop_x, dst_crop_y,
                                   &dst_crop_width, &dst_crop_height,
                                   dst_har_format)) {
        LOGE("%s:: setDstParams() failed", __func__);
        return false;
    }

    switch (dst_format) {
    case OMX_COLOR_FormatYUV420SemiPlanar:
        if (!handle_fimc->setDstAddr((unsigned int)(dst_addr_array[0]),
                                     (unsigned int)(dst_addr_array[1]),
                                     (unsigned int)(dst_addr_array[1]))) {
            LOGE("%s:: setDstPhyAddr() failed", __func__);
            return false;
        }
        break;
    case OMX_COLOR_FormatYUV420Planar:
    default:
        if (!handle_fimc->setDstAddr((unsigned int)(dst_addr_array[0]),
                                     (unsigned int)(dst_addr_array[1]),
                                     (unsigned int)(dst_addr_array[2]))) {
            LOGE("%s:: setDstPhyAddr() failed", __func__);
            return false;
        }
        break;
    }

    if (!handle_fimc->draw(0, 0)) {
        LOGE("%s:: handleOneShot() failed", __func__);
        return false;
    }

    return true;
}

unsigned int HardwareConverter::OMXtoHarPixelFomrat(OMX_COLOR_FORMATTYPE omx_format)
{
    unsigned int hal_format = 0;

    switch (omx_format) {
    case OMX_COLOR_FormatYUV420Planar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        hal_format = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED;
        break;
    default:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    }
    return hal_format;
}
