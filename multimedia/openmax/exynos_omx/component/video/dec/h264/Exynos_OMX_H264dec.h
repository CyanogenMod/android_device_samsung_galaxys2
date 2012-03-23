/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
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
 * @file    Exynos_OMX_H264dec.h
 * @brief
 * @author    SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef EXYNOS_OMX_H264_DEC_COMPONENT
#define EXYNOS_OMX_H264_DEC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "OMX_Video.h"


#define MAX_H264_FP_VIDEO_INPUTBUFFER_NUM  4
#define MAX_H264_FP_VIDEO_OUTPUTBUFFER_NUM 4

typedef struct _EXYNOS_MFC_H264DEC_HANDLE
{
    OMX_HANDLETYPE hMFCHandle;
    OMX_PTR  pMFCStreamBuffer;
    OMX_PTR  pMFCStreamPhyBuffer;
    OMX_U32  indexTimestamp;
    OMX_U32  outputIndexTimestamp;
    OMX_BOOL bConfiguredMFC;
    OMX_BOOL bFlashPlayerMode;
    OMX_S32  returnCodec;
} EXYNOS_MFC_H264DEC_HANDLE;

typedef struct _EXYNOS_H264DEC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_AVCTYPE AVCComponent[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* SEC MFC Codec specific */
    EXYNOS_MFC_H264DEC_HANDLE hMFCH264Handle;
} EXYNOS_H264DEC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif
