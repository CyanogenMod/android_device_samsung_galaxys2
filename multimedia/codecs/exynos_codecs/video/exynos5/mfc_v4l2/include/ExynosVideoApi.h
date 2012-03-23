/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Common header file for Codec driver
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

#ifndef _EXYNOS_VIDEO_API_H_
#define _EXYNOS_VIDEO_API_H_

/* Fixed */
#define VIDEO_BUFFER_MAX_PLANES 3

typedef enum _ExynosVideoBoolType {
    VIDEO_FALSE = 0,
    VIDEO_TRUE  = 1,
} ExynosVideoBoolType;

typedef enum _ExynosVideoErrorType {
    VIDEO_ERROR_NONE      = 1,
    VIDEO_ERROR_BADPARAM  = -1,
    VIDEO_ERROR_OPENFAIL  = -2,
    VIDEO_ERROR_NOMEM     = -3,
    VIDEO_ERROR_APIFAIL   = -4,
    VIDEO_ERROR_MAPFAIL   = -5,
    VIDEO_ERROR_NOBUFFERS = -6,
    VIDEO_ERROR_POLL      = -7,
} ExynosVideoErrorType;

typedef enum _ExynosVideoCodingType {
    VIDEO_CODING_UNKNOWN = 0,
    VIDEO_CODING_MPEG2,
    VIDEO_CODING_H263,
    VIDEO_CODING_MPEG4,
    VIDEO_CODING_VC1,
    VIDEO_CODING_VC1_RCV,
    VIDEO_CODING_AVC,
    VIDEO_CODING_MVC,
    VIDEO_CODING_VP8,
    VIDEO_CODING_RESERVED,
} ExynosVideoCodingType;

typedef enum _ExynosVideoColorFormatType {
    VIDEO_COLORFORMAT_UNKNOWN = 0,
    VIDEO_COLORFORMAT_NV12,
    VIDEO_COLORFORMAT_NV21,
    VIDEO_COLORFORMAT_NV12_TILED,
    VIDEO_COLORFORMAT_RESERVED,
} ExynosVideoColorFormatType;

typedef enum _ExynosVideoFrameType {
    VIDEO_FRAME_NOT_CODED = 0,
    VIDEO_FRAME_I,
    VIDEO_FRAME_P,
    VIDEO_FRAME_B,
    VIDEO_FRAME_SKIPPED,
    VIDEO_FRAME_OTHERS,
} ExynosVideoFrameType;

typedef enum _ExynosVideoFrameStatusType {
    VIDEO_FRAME_STATUS_UNKNOWN = 0,
    VIDEO_FRAME_STATUS_DECODING_ONLY,
    VIDEO_FRAME_STATUS_DISPLAY_DECODING,
    VIDEO_FRAME_STATUS_DISPLAY_ONLY,
    VIDEO_FRAME_STATUS_CHANGE_RESOL,
} ExynosVideoFrameStatusType;

typedef struct _ExynosVideoRect {
    unsigned int nTop;
    unsigned int nLeft;
    unsigned int nWidth;
    unsigned int nHeight;
} ExynosVideoRect;

typedef struct _ExynosVideoGeometry {
    unsigned int               nFrameWidth;
    unsigned int               nFrameHeight;
    unsigned int               nSizeImage;
    ExynosVideoRect            cropRect;
    ExynosVideoCodingType      eCompressionFormat;
    ExynosVideoColorFormatType eColorFormat;
} ExynosVideoGeometry;

typedef struct _ExynosVideoPlane {
    unsigned char *addr;
    unsigned int   allocSize;
    unsigned int   dataSize;
} ExynosVideoPlane;

typedef struct _ExynosVideoBuffer {
    ExynosVideoPlane            planes[VIDEO_BUFFER_MAX_PLANES];
    ExynosVideoGeometry        *pGeometry;
    ExynosVideoFrameStatusType  displayStatus;
    ExynosVideoFrameType        frameType;
    ExynosVideoBoolType         bQueued;
    void                       *pPrivate;
} ExynosVideoBuffer;

typedef struct _ExynosVideoDecOps {
    unsigned int nSize;

    void                 *(*Init)(void);
    ExynosVideoErrorType  (*Finalize)(void *pHandle);

    /* Add new ops at the end of structure, no order change */
    ExynosVideoErrorType  (*Set_FrameTag)(void *pHandle, int frameTag);
    int                   (*Get_FrameTag)(void *pHandle);
    int                   (*Get_ActualBufferCount)(void *pHandle);
    ExynosVideoErrorType  (*Enable_Cacheable)(void *pHandle);
    ExynosVideoErrorType  (*Set_DisplayDelay)(void *pHandle, int delay);
    ExynosVideoErrorType  (*Enable_PackedPB)(void *pHandle);
    ExynosVideoErrorType  (*Enable_LoopFilter)(void *pHandle);
    ExynosVideoErrorType  (*Enable_SliceMode)(void *pHandle);
} ExynosVideoDecOps;

typedef struct _ExynosVideoDecBufferOps {
    unsigned int nSize;

    /* Add new ops at the end of structure, no order change */
    ExynosVideoErrorType  (*Set_Shareable)(void *pHandle);
    ExynosVideoErrorType  (*Get_BufferInfo)(void *pHandle, int nIndex, ExynosVideoBuffer *pBuffer);
    ExynosVideoErrorType  (*Set_Geometry)(void *pHandle, ExynosVideoGeometry *bufferConf);
    ExynosVideoErrorType  (*Get_Geometry)(void *pHandle, ExynosVideoGeometry *bufferConf);
    ExynosVideoErrorType  (*Setup)(void *pHandle, unsigned int nBufferCount);
    ExynosVideoErrorType  (*Run)(void *pHandle);
    ExynosVideoErrorType  (*Stop)(void *pHandle);
    ExynosVideoErrorType  (*Enqueue)(void *pHandle, unsigned char *pBuffer[], unsigned int dataSize[], int nPlanes, void *pPrivate);
    ExynosVideoErrorType  (*Enqueue_All)(void *pHandle);
    ExynosVideoBuffer    *(*Dequeue)(void *pHandle);
} ExynosVideoDecBufferOps;

int Exynos_Video_Register_Decoder(
    ExynosVideoDecOps       *pDecOps,
    ExynosVideoDecBufferOps *pInbufOps,
    ExynosVideoDecBufferOps *pOutbufOps);

#endif /* _EXYNOS_VIDEO_API_H_ */
