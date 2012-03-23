/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * @file    hwconverter_wrapper.h
 *
 * @brief   hwconverter_wrapper abstract libhwconverter and support c functions
 *
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 *
 * @version 1.0
 *
 * @history
 *   2012.02.01 : Create
 */

#ifndef HWCONVERTER_WRAPPER_H

#define HWCONVERTER_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Video.h>

/*--------------------------------------------------------------------------------*/
/* Structure and Type                                                             */
/*--------------------------------------------------------------------------------*/
typedef enum {
    HWCONVERTER_RET_OK                 = 0,
    HWCONVERTER_RET_FAIL               = -1
} HWCONVERTER_ERROR_CODE;

/*--------------------------------------------------------------------------------*/
/* CSC FIMC APIs                                                                  */
/*--------------------------------------------------------------------------------*/
/*
 * create hwconverter handle
 *
 * @return
 *   hwconverter handle
 */
void *csc_fimc_open();

/*
 * destroy hwconverter handle
 *
 * @param handle
 *   hwconverter handle[in]
 *
 * @return
 *   error code
 */
HWCONVERTER_ERROR_CODE csc_fimc_close(
    void *handle);

/*
 * convert color space nv12t to omxformat
 *
 * @param handle
 *   hwconverter handle[in]
 *
 * @param dst_addr
 *   y,u,v address of dst_addr[out]
 *
 * @param src_addr
 *   y,uv address of src_addr.Format is nv12t[in]
 *
 * @param width
 *   width of dst image[in]
 *
 * @param height
 *   height of dst image[in]
 *
 * @param omxformat
 *   omxformat of dst image[in]
 *
 * @return
 *   error code
 */
HWCONVERTER_ERROR_CODE csc_fimc_convert_nv12t(
    void *handle,
    void **dst_addr,
    void **src_addr,
    unsigned int width,
    unsigned int height,
    OMX_COLOR_FORMATTYPE omxformat);

#ifdef __cplusplus
}
#endif

#endif
