/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
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
 * @file        ExynosVideoDecoder.c
 * @brief
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.0.0
 * @history
 *   2012.01.15: Initial Version
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <sys/poll.h>

#include "ExynosVideoApi.h"
#include "ExynosVideoDec.h"
#include "exynos_v4l2.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosVideoDecoder"
#include <utils/Log.h>

/*
 * [Common] __CodingType_To_V4L2PixelFormat
 */
static unsigned int __CodingType_To_V4L2PixelFormat(ExynosVideoCodingType codingType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_H264;

    switch (codingType) {
    case VIDEO_CODING_AVC:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    case VIDEO_CODING_MPEG4:
        pixelformat = V4L2_PIX_FMT_MPEG4;
        break;
    case VIDEO_CODING_VP8:
        pixelformat = V4L2_PIX_FMT_VP8;
        break;
    case VIDEO_CODING_H263:
        pixelformat = V4L2_PIX_FMT_H263;
        break;
    case VIDEO_CODING_VC1:
        pixelformat = V4L2_PIX_FMT_VC1;
        break;
    case VIDEO_CODING_VC1_RCV:
        pixelformat = V4L2_PIX_FMT_VC1_RCV;
        break;
    case VIDEO_CODING_MPEG2:
        pixelformat = V4L2_PIX_FMT_MPEG12;
        break;
    default:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    }

    return pixelformat;
}

/*
 * [Common] __ColorFormatType_To_V4L2PixelFormat
 */
static unsigned int __ColorFormatType_To_V4L2PixelFormat(ExynosVideoColorFormatType colorFormatType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_NV12M;

    switch (colorFormatType) {
    case VIDEO_COLORFORMAT_NV12_TILED:
        pixelformat = V4L2_PIX_FMT_NV12MT_16X16;
        break;
    case VIDEO_COLORFORMAT_NV21:
        pixelformat = V4L2_PIX_FMT_NV21M;
        break;
    case VIDEO_COLORFORMAT_NV12:
    default:
        pixelformat = V4L2_PIX_FMT_NV12M;
        break;
    }

    return pixelformat;
}

/*
 * [Decoder OPS] Init
 */
static void *MFC_Decoder_Init(void)
{
    ExynosVideoDecContext *pCtx     = NULL;
    int                    needCaps = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING);

    pCtx = (ExynosVideoDecContext *)malloc(sizeof(*pCtx));
    if (pCtx == NULL) {
        LOGE("%s: Failed to allocate decoder context buffer", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    memset(pCtx, 0, sizeof(*pCtx));

    pCtx->hDec = exynos_v4l2_open_devname(VIDEO_DECODER_NAME, O_RDWR | O_NONBLOCK, 0);
    if (pCtx->hDec < 0) {
        LOGE("%s: Failed to open decoder device", __func__);
        goto EXIT_OPEN_FAIL;
    }

    if (!exynos_v4l2_querycap(pCtx->hDec, needCaps)) {
        LOGE("%s: Failed to querycap", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }

    return (void *)pCtx;

EXIT_QUERYCAP_FAIL:
    close(pCtx->hDec);

EXIT_OPEN_FAIL:
    free(pCtx);
    pCtx = NULL;

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Decoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Decoder_Finalize(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    ExynosVideoPlane      *pVideoPlane;

    int i, j;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nInbufs; i++) {
            for (j = 0; j < VIDEO_DECODER_INBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pInbuf[i].pGeometry = NULL;
                pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nOutbufs; i++) {
            for (j = 0; j < VIDEO_DECODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pOutbuf[i].pGeometry = NULL;
                pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->pInbuf != NULL)
        free(pCtx->pInbuf);

    if (pCtx->pOutbuf != NULL)
        free(pCtx->pOutbuf);

    if (pCtx->hDec > 0)
        close(pCtx->hDec);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Decoder_Set_FrameTag(
    void *pHandle,
    int   frameTag)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CODEC_FRAME_TAG, frameTag)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Tag
 */
static int MFC_Decoder_Get_FrameTag(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int frameTag = -1;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_CODEC_FRAME_TAG, &frameTag);

EXIT:
    return frameTag;
}

/*
 * [Decoder OPS] Get Buffer Count
 */
static int MFC_Decoder_Get_ActualBufferCount(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int bufferCount = -1;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_CODEC_REQ_NUM_BUFS, &bufferCount);

EXIT:
    return bufferCount;
}

/*
 * [Decoder OPS] Set Cacheable
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CACHEABLE, 1)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Display Delay
 */
static ExynosVideoErrorType MFC_Decoder_Set_DisplayDelay(
    void *pHandle,
    int   delay)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CODEC_DISPLAY_DELAY, delay)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Packed PB
 */
static ExynosVideoErrorType MFC_Decoder_Enable_PackedPB(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CODEC_PACKED_PB, 1)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Loop Filter
 */
static ExynosVideoErrorType MFC_Decoder_Enable_LoopFilter(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CODEC_LOOP_FILTER_MPEG4_ENABLE, 1)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Slice Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SliceMode(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CODEC_SLICE_INTERFACE, 1)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Shareable Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Shareable_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareInbuf = 1;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Shareable Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Shareable_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareOutbuf = 1;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Buffer Info (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Get_BufferInfo_Inbuf(
    void              *pHandle,
    int                nIndex,
    ExynosVideoBuffer *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    memcpy(pBuffer, &pCtx->pInbuf[nIndex], sizeof(*pBuffer));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Buffer Info (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_BufferInfo_Outbuf(
    void              *pHandle,
    int                nIndex,
    ExynosVideoBuffer *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    memcpy(pBuffer, &pCtx->pOutbuf[nIndex], sizeof(*pBuffer));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        LOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __CodingType_To_V4L2PixelFormat(bufferConf->eCompressionFormat);
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = bufferConf->nSizeImage;

    if (exynos_v4l2_s_fmt(pCtx->hDec, &fmt)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->inbufGeometry, bufferConf, sizeof(pCtx->inbufGeometry));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        LOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __ColorFormatType_To_V4L2PixelFormat(bufferConf->eColorFormat);

    if (exynos_v4l2_s_fmt(pCtx->hDec, &fmt)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->outbufGeometry, bufferConf, sizeof(pCtx->outbufGeometry));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;
    struct v4l2_crop   crop;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        LOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));
    memset(&crop, 0, sizeof(crop));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_fmt(pCtx->hDec, &fmt)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_crop(pCtx->hDec, &crop)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nFrameWidth = fmt.fmt.pix_mp.width;
    bufferConf->nFrameHeight = fmt.fmt.pix_mp.height;

    bufferConf->cropRect.nTop = crop.c.top;
    bufferConf->cropRect.nLeft = crop.c.left;
    bufferConf->cropRect.nWidth = crop.c.width;
    bufferConf->cropRect.nHeight = crop.c.height;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Inbuf(
    void         *pHandle,
    unsigned int  nBufferCount)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    ExynosVideoPlane      *pVideoPlane;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    struct v4l2_plane          planes[VIDEO_DECODER_INBUF_PLANES];
    int i;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        LOGE("%s: Buffer count must be greater than 0", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = V4L2_MEMORY_USERPTR;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    pCtx->pInbuf = malloc(sizeof(*pCtx->pInbuf) * pCtx->nInbufs);
    if (pCtx->pInbuf == NULL) {
        LOGE("Failed to allocate input buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = VIDEO_DECODER_INBUF_PLANES;

        for (i = 0; i < pCtx->nInbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hDec, &buf)) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            pVideoPlane = &pCtx->pInbuf[i].planes[0];

            pVideoPlane->addr = mmap(NULL,
                    buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                    MAP_SHARED, pCtx->hDec, buf.m.planes[0].m.mem_offset);

            if (pVideoPlane->addr == MAP_FAILED) {
                ret = VIDEO_ERROR_MAPFAIL;
                goto EXIT;
            }

            pVideoPlane->allocSize = buf.m.planes[0].length;
            pVideoPlane->dataSize = 0;

            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_TRUE;
        }
    }

    return ret;

EXIT:
    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nInbufs; i++) {
            pVideoPlane = &pCtx->pInbuf[i].planes[0];
            if (pVideoPlane->addr == MAP_FAILED) {
                pVideoPlane->addr = NULL;
                break;
            }

            munmap(pVideoPlane->addr, pVideoPlane->allocSize);
            pVideoPlane->allocSize = 0;
            pVideoPlane->dataSize = 0;

            pCtx->pInbuf[i].pGeometry = NULL;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
        }
    }

    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Outbuf(
    void         *pHandle,
    unsigned int  nBufferCount)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    ExynosVideoPlane      *pVideoPlane;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    struct v4l2_plane          planes[VIDEO_DECODER_OUTBUF_PLANES];
    int i, j;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        LOGE("%s: Buffer count must be greater than 0", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = V4L2_MEMORY_USERPTR;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req)) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nOutbufs = req.count;

    pCtx->pOutbuf = malloc(sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);
    if (pCtx->pOutbuf == NULL) {
        LOGE("Failed to allocate output buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = VIDEO_DECODER_OUTBUF_PLANES;

        for (i = 0; i < pCtx->nOutbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hDec, &buf)) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < VIDEO_DECODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hDec, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
            pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
        }
    }

    return ret;

EXIT:
    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nOutbufs; i++) {
            for (j = 0; j < VIDEO_DECODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                if (pVideoPlane->addr == MAP_FAILED) {
                    pVideoPlane->addr = NULL;
                    break;
                }

                munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                pVideoPlane->allocSize = 0;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = NULL;
            pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
        }
    }

    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamon(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
        LOGE("%s: Failed to streamon for input buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamon(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
        LOGE("%s: Failed to streamon for output buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Stop (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamoff(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
        LOGE("%s: Failed to streamoff for input buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Stop (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamoff(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
        LOGE("%s: Failed to streamoff for output buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Wait (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Wait_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hDec;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_DECODER_POLL_TIMEOUT);
        if (poll_state > 0) {
            if (poll_events.revents & POLLOUT) {
                break;
            } else {
                LOGE("%s: Poll return error", __func__);
                ret = VIDEO_ERROR_POLL;
                break;
            }
        } else if (poll_state < 0) {
            LOGE("%s: Poll state error", __func__);
            ret = VIDEO_ERROR_POLL;
            break;
        }
    } while (poll_state == 0);

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Find (Input)
 */
static int MFC_Decoder_Find_Inbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        nIndex = -1;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
            if (pCtx->pInbuf[nIndex].bQueued == VIDEO_FALSE)
                break;
        }
    } else {
        for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
            if (pCtx->pInbuf[nIndex].planes[0].addr == pBuffer)
                break;
        }
    }

    if (nIndex == pCtx->nInbufs) {
        nIndex = -1;
        goto EXIT;
    }

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Find (Outnput)
 */
static int MFC_Decoder_Find_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        nIndex = -1;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
            if (pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE)
                break;
        }
    } else {
        for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
            if (pCtx->pOutbuf[nIndex].planes[0].addr == pBuffer)
                break;
        }
    }

    if (nIndex == pCtx->nOutbufs) {
        nIndex = -1;
        goto EXIT;
    }

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Enqueue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Inbuf(
    void          *pHandle,
    unsigned char *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_plane  planes[VIDEO_DECODER_INBUF_PLANES];
    struct v4l2_buffer buf;
    int index, i;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_DECODER_INBUF_PLANES;

    if ((pCtx->bShareInbuf == VIDEO_TRUE) || (pBuffer == NULL))
        index = MFC_Decoder_Find_Inbuf(pCtx, NULL);
    else
        index = MFC_Decoder_Find_Inbuf(pCtx, pBuffer[0]);

    if (index == -1) {
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;

    if (pCtx->bShareInbuf == VIDEO_TRUE) {
        buf.memory = V4L2_MEMORY_USERPTR;
        for (i = 0; i < nPlanes; i++) {
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            buf.m.planes[i].length = VIDEO_DECODER_INBUF_SIZE;
            buf.m.planes[i].bytesused = dataSize[i];
            pCtx->pInbuf[buf.index].planes[i].addr = pBuffer[i];
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
        for (i = 0; i < nPlanes; i++)
            buf.m.planes[i].bytesused = dataSize[i];
    }

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf)) {
        LOGE("%s: Failed to enqueue input buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enqueue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_plane  planes[VIDEO_DECODER_OUTBUF_PLANES];
    struct v4l2_buffer buf;
    int i, index;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_DECODER_OUTBUF_PLANES;

    if ((pCtx->bShareOutbuf == VIDEO_TRUE) || (pBuffer == NULL))
        index = MFC_Decoder_Find_Outbuf(pCtx, NULL);
    else
        index = MFC_Decoder_Find_Outbuf(pCtx, pBuffer[0]);

    if (index == -1) {
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;

    if (pCtx->bShareOutbuf == VIDEO_TRUE) {
        buf.memory = V4L2_MEMORY_USERPTR;
        for (i = 0; i < nPlanes; i++) {
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            buf.m.planes[i].length = dataSize[i];
            buf.m.planes[i].bytesused = dataSize[i];
            pCtx->pOutbuf[buf.index].planes[i].addr = pBuffer[i];
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
    }

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf)) {
        LOGE("%s: Failed to enqueue output buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Dequeue (Input)
 */
static ExynosVideoBuffer *MFC_Decoder_Dequeue_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx    = (ExynosVideoDecContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;

    struct v4l2_buffer buf;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    if (MFC_Decoder_Wait_Inbuf(pCtx) != VIDEO_ERROR_NONE) {
        LOGE("%s: Failed to poll for input buffer", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        buf.memory = V4L2_MEMORY_USERPTR;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) == 0) {
        pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;
        pOutbuf = &pCtx->pInbuf[buf.index];
    } else {
        LOGE("%s: Failed to dequeue input buffer", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

EXIT:
    return pOutbuf;
}

/*
 * [Decoder Buffer OPS] Enqueue All (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_All_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    int i;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nOutbufs; i++)
        MFC_Decoder_Enqueue_Outbuf(pCtx, NULL, NULL, 0, NULL);

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Dequeue (Output)
 */
static ExynosVideoBuffer *MFC_Decoder_Dequeue_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx    = (ExynosVideoDecContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;

    struct v4l2_buffer buf;
    int value;

    if (pCtx == NULL) {
        LOGE("%s: Video context info must be supplied", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        buf.memory = V4L2_MEMORY_USERPTR;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    /* HACK: pOutbufurn -1 means DECODING_ONLY for almost cases */
    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf)) {
        LOGW("%s: Failed to dequeue output buffer or DECODING_ONLY", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    pOutbuf = &pCtx->pOutbuf[buf.index];

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_CODEC_DISPLAY_STATUS, &value);

    switch (value) {
    case 0:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_ONLY;
        break;
    case 1:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_DECODING;
        break;
    case 2:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_ONLY;
        break;
    case 3:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_CHANGE_RESOL;
        break;
    default:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
        break;
    }

    switch (buf.flags & (0x7 << 3)) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pOutbuf->frameType = VIDEO_FRAME_I;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pOutbuf->frameType = VIDEO_FRAME_P;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pOutbuf->frameType = VIDEO_FRAME_B;
        break;
    default:
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    pOutbuf->bQueued = VIDEO_FALSE;

EXIT:
    return pOutbuf;
}

/*
 * [Decoder OPS] Common
 */
static ExynosVideoDecOps defDecOps = {
    .nSize = 0,
    .Init = MFC_Decoder_Init,
    .Finalize = MFC_Decoder_Finalize,
    .Enable_Cacheable = MFC_Decoder_Enable_Cacheable,
    .Set_DisplayDelay = MFC_Decoder_Set_DisplayDelay,
    .Enable_PackedPB = MFC_Decoder_Enable_PackedPB,
    .Enable_LoopFilter = MFC_Decoder_Enable_LoopFilter,
    .Enable_SliceMode = MFC_Decoder_Enable_SliceMode,
    .Get_ActualBufferCount = MFC_Decoder_Get_ActualBufferCount,
    .Set_FrameTag = MFC_Decoder_Set_FrameTag,
    .Get_FrameTag = MFC_Decoder_Get_FrameTag,
};

/*
 * [Decoder Buffer OPS] Input
 */
static ExynosVideoDecBufferOps defInbufOps = {
    .nSize = 0,
    .Set_Shareable = MFC_Decoder_Set_Shareable_Inbuf,
    .Get_BufferInfo = MFC_Decoder_Get_BufferInfo_Inbuf,
    .Set_Geometry = MFC_Decoder_Set_Geometry_Inbuf,
    .Get_Geometry = NULL,
    .Setup = MFC_Decoder_Setup_Inbuf,
    .Run = MFC_Decoder_Run_Inbuf,
    .Stop = MFC_Decoder_Stop_Inbuf,
    .Enqueue = MFC_Decoder_Enqueue_Inbuf,
    .Enqueue_All = NULL,
    .Dequeue = MFC_Decoder_Dequeue_Inbuf,
};

/*
 * [Decoder Buffer OPS] Output
 */
static ExynosVideoDecBufferOps defOutbufOps = {
    .nSize = 0,
    .Set_Shareable = MFC_Decoder_Set_Shareable_Outbuf,
    .Get_BufferInfo = MFC_Decoder_Get_BufferInfo_Outbuf,
    .Set_Geometry = MFC_Decoder_Set_Geometry_Outbuf,
    .Get_Geometry = MFC_Decoder_Get_Geometry_Outbuf,
    .Setup = MFC_Decoder_Setup_Outbuf,
    .Run = MFC_Decoder_Run_Outbuf,
    .Stop = MFC_Decoder_Stop_Outbuf,
    .Enqueue = MFC_Decoder_Enqueue_Outbuf,
    .Enqueue_All = MFC_Decoder_Enqueue_All_Outbuf,
    .Dequeue = MFC_Decoder_Dequeue_Outbuf,
};

int Exynos_Video_Register_Decoder(
    ExynosVideoDecOps       *pDecOps,
    ExynosVideoDecBufferOps *pInbufOps,
    ExynosVideoDecBufferOps *pOutbufOps)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    defDecOps.nSize = sizeof(defDecOps);
    defInbufOps.nSize = sizeof(defInbufOps);
    defOutbufOps.nSize = sizeof(defOutbufOps);

    memcpy((char *)pDecOps + sizeof(pDecOps->nSize), (char *)&defDecOps + sizeof(defDecOps.nSize),
            pDecOps->nSize - sizeof(pDecOps->nSize));

    memcpy((char *)pInbufOps + sizeof(pInbufOps->nSize), (char *)&defInbufOps + sizeof(defInbufOps.nSize),
            pInbufOps->nSize - sizeof(pInbufOps->nSize));

    memcpy((char *)pOutbufOps + sizeof(pOutbufOps->nSize), (char *)&defOutbufOps + sizeof(defOutbufOps.nSize),
            pOutbufOps->nSize - sizeof(pOutbufOps->nSize));

EXIT:
    return ret;
}
