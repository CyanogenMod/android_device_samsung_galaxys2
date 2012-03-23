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

#ifndef _SEC_FORMAT_H_
#define _SEC_FORMAT_H_

/* enum related to pixel format */

enum {
    HAL_PIXEL_FORMAT_YCbCr_422_P         = 0x100,
    HAL_PIXEL_FORMAT_YCbCr_420_P         = 0x101,
    HAL_PIXEL_FORMAT_YCbCr_420_I         = 0x102,
    HAL_PIXEL_FORMAT_CbYCrY_422_I        = 0x103,
    HAL_PIXEL_FORMAT_CbYCrY_420_I        = 0x104,
    HAL_PIXEL_FORMAT_YCbCr_420_SP        = 0x105,
    HAL_PIXEL_FORMAT_YCrCb_422_SP        = 0x106,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED  = 0x107,
    HAL_PIXEL_FORMAT_ARGB888             = 0x108,
    // support custom format for zero copy
    HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP = 0x110,
    HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP = 0x111,
    HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED = 0x112,
    HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_SP = 0x113,
    HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_SP = 0x114,
    HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I = 0x115,
    HAL_PIXEL_FORMAT_CUSTOM_YCrCb_422_I = 0x116,
    HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I = 0x117,
    HAL_PIXEL_FORMAT_CUSTOM_CrYCbY_422_I = 0x118,
    HAL_PIXEL_FORMAT_CUSTOM_CbYCr_422_I  = 0x11B,
    HAL_PIXEL_FORMAT_CUSTOM_MAX
};

#endif
