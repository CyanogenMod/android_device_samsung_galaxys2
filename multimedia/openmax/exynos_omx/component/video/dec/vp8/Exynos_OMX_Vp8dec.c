/*
 *
 * Copyright 2011 Samsung Electronics S.LSI Co. LTD
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
 * @file        Exynos_OMX_Vp8dec.c
 * @brief
 * @author      Satish Kumar Reddy (palli.satish@samsung.com)
 * @version     1.1.0
 * @history
 *   2011.11.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "library_register.h"
#include "Exynos_OMX_Vp8dec.h"
#include "ExynosVideoApi.h"

#ifdef USE_ANB
#include "Exynos_OSAL_Android.h"
#endif

/* To use CSC_METHOD_PREFER_HW or CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VP8_DEC"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

#define VP8_DEC_NUM_OF_EXTRA_BUFFERS 7

//#define FULL_FRAME_SEARCH /* Full frame search not support*/

static int Check_VP8_Frame(
    OMX_U8   *pInputStream,
    int       buffSize,
    OMX_U32   flag,
    OMX_BOOL  bPreviousFrameEOF,
    OMX_BOOL *pbEndOfFrame)
{
    /* Uncompressed data Chunk comprises a common
    (for key frames and interframes) 3-byte frame tag that
    contains four fields
    - 1-bit frame type (0 - key frame, 1 - inter frame)
    - 3-bit version number (0 - 3 are defined as four different
                                      profiles with different decoding complexity)
    - 1-bit show_frame flag ( 0 - current frame not for display,
                                          1 - current frame is for dispaly)
    - 19-bit field - size of the first data partition in bytes

    Key Frames : frame tag followed by 7 bytes of uncompressed
    data
    3-bytes : Start code (byte 0: 0x9d,byte 1: 0x01,byte 2: 0x2a)
    Next 4-bytes: Width & height, Horizontal and vertical scale information
    16 bits      :     (2 bits Horizontal Scale << 14) | Width (14 bits)
    16 bits      :     (2 bits Vertical Scale << 14) | Height (14 bits)
    */
    int width, height;
    int horizSscale, vertScale;

    FunctionIn();

    *pbEndOfFrame = OMX_TRUE;

    /*Check for Key frame*/
    if (!(pInputStream[0] & 0x01)){
        /* Key Frame  Start code*/
        if (pInputStream[3] != 0x9d || pInputStream[4] != 0x01 || pInputStream[5]!=0x2a) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, " VP8 Key Frame Start Code not Found");
            *pbEndOfFrame = OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, " VP8 Found Key Frame Start Code");
        width = (pInputStream[6] | (pInputStream[7] << 8)) & 0x3fff;
        horizSscale = pInputStream[7] >> 6;
        height = (pInputStream[8] | (pInputStream[9] << 8)) & 0x3fff;
        vertScale = pInputStream[9] >> 6;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "width = %d, height = %d, horizSscale = %d, vertScale = %d", width, height, horizSscale, vertScale);
    }

    FunctionOut();
    return buffSize;
}

OMX_BOOL Check_VP8_StartCode(
    OMX_U8     *pInputStream,
    OMX_U32     streamSize)
{
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "streamSize: %d",streamSize);
    if (streamSize < 3) {
        return OMX_FALSE;
    }

    if (!(pInputStream[0] & 0x01)){
        /* Key Frame  Start code*/
        if (pInputStream[3] != 0x9d || pInputStream[4] != 0x01 || pInputStream[5]!=0x2a) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, " VP8 Key Frame Start Code not Found");
            return OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, " VP8 Found Key Frame Start Code");
    }

    return OMX_TRUE;
}

OMX_ERRORTYPE Exynos_MFC_VP8Dec_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pVp8Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_VP8Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE)) {
            pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingVPX;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                       portIndex       = pPortDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort;
        OMX_U32                       width, height, size;
        OMX_U32                       realWidth, realHeight;

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(pPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            if (pExynosPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }
        if (pPortDefinition->nBufferCountActual < pExynosPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        Exynos_OSAL_Memcpy(&pExynosPort->portDefinition, pPortDefinition, pPortDefinition->nSize);

        realWidth = pExynosPort->portDefinition.format.video.nFrameWidth;
        realHeight = pExynosPort->portDefinition.format.video.nFrameHeight;
        width = ((realWidth + 15) & (~15));
        height = ((realHeight + 15) & (~15));
        size = (width * height * 3) / 2;
        pExynosPort->portDefinition.format.video.nStride = width;
        pExynosPort->portDefinition.format.video.nSliceHeight = height;
        pExynosPort->portDefinition.nBufferSize = (size > pExynosPort->portDefinition.nBufferSize) ? size : pExynosPort->portDefinition.nBufferSize;

        if (portIndex == INPUT_PORT_INDEX) {
            EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            pExynosOutputPort->portDefinition.format.video.nFrameWidth = pExynosPort->portDefinition.format.video.nFrameWidth;
            pExynosOutputPort->portDefinition.format.video.nFrameHeight = pExynosPort->portDefinition.format.video.nFrameHeight;
            pExynosOutputPort->portDefinition.format.video.nStride = width;
            pExynosOutputPort->portDefinition.format.video.nSliceHeight = height;

            switch (pExynosOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                pExynosOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
                break;
            case OMX_SEC_COLOR_FormatNV12Tiled:
                pExynosOutputPort->portDefinition.nBufferSize =
                    ALIGN_TO_8KB(ALIGN_TO_128B(realWidth) * ALIGN_TO_32B(realHeight)) \
                  + ALIGN_TO_8KB(ALIGN_TO_128B(realWidth) * ALIGN_TO_32B(realHeight/2));
                break;
            default:
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Color format is not support!! use default YUV size!!");
                ret = OMX_ErrorUnsupportedSetting;
                break;
            }
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pVp8Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_VP8Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_VP8Dec_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoDecodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_VP8Dec_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE  hComponent,
    OMX_IN  OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE   *pIndexType)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_ENABLE_THUMBNAIL) == 0) {
        EXYNOS_VP8DEC_HANDLE *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        *pIndexType = OMX_IndexVendorThumbnailMode;

        ret = OMX_ErrorNone;
    } else {
        ret = Exynos_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_VP8Dec_ComponentRoleEnum(
    OMX_HANDLETYPE hComponent,
    OMX_U8        *cRole,
    OMX_U32        nIndex)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_DecodeThread(
    OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec        = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec          = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    ExynosVideoDecOps             *pDecOps          = pVideoDec->pDecOps;
    ExynosVideoDecBufferOps       *pInbufOps        = pVideoDec->pInbufOps;
    ExynosVideoDecBufferOps       *pOutbufOps       = pVideoDec->pOutbufOps;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while (pVideoDec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
        Exynos_OSAL_SemaphoreWait(pVideoDec->NBDecThread.hDecFrameStart);

        if (pVideoDec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
#ifdef CONFIG_MFC_FPS
            Exynos_OSAL_PerfStart(PERF_ID_DEC);
#endif
            if (pVideoDec->NBDecThread.oneFrameSize > 0) {
                /* queue work for input buffer */
                pInbufOps->Enqueue(pVp8Dec->hMFCVp8Handle.hMFCHandle,
                                  (unsigned char **)&pVp8Dec->hMFCVp8Handle.pMFCStreamBuffer,
                                  (unsigned int *)&pVideoDec->NBDecThread.oneFrameSize,
                                   1, NULL);

                pVideoDec->indexInputBuffer++;
                pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
                pVp8Dec->hMFCVp8Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
                pVp8Dec->hMFCVp8Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
                pExynosComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
                pExynosComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].bufferSize;

                pInbufOps->Dequeue(pVp8Dec->hMFCVp8Handle.hMFCHandle);
                pVideoDec->pOutbuf = pOutbufOps->Dequeue(pVp8Dec->hMFCVp8Handle.hMFCHandle);
            }

            pVp8Dec->hMFCVp8Handle.returnCodec = VIDEO_TRUE;
#ifdef CONFIG_MFC_FPS
            Exynos_OSAL_PerfStop(PERF_ID_DEC);
#endif
            Exynos_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameEnd);
        }
    }

EXIT:
    Exynos_OSAL_ThreadExit(NULL);
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_MFC_VP8Dec_Init(
    OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = NULL;
    OMX_PTR                        hMFCHandle        = NULL;
    OMX_PTR                        pStreamBuffer     = NULL;
    OMX_PTR                        pStreamPhyBuffer  = NULL;
    ExynosVideoDecOps             *pDecOps           = NULL;
    ExynosVideoDecBufferOps       *pInbufOps         = NULL;
    ExynosVideoDecBufferOps       *pOutbufOps        = NULL;
    ExynosVideoBuffer              bufferInfo;
    ExynosVideoGeometry            bufferConf;

    CSC_METHOD csc_method = CSC_METHOD_SW;
    int i = 0;

    FunctionIn();

#ifdef CONFIG_MFC_FPS
    Exynos_OSAL_PerfInit(PERF_ID_DEC);
    Exynos_OSAL_PerfInit(PERF_ID_CSC);
#endif

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pVp8Dec->hMFCVp8Handle.bConfiguredMFC = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_FALSE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;

    /* alloc ops structure */
    pDecOps = (ExynosVideoDecOps *)malloc(sizeof(*pDecOps));
    pInbufOps = (ExynosVideoDecBufferOps *)malloc(sizeof(*pInbufOps));
    pOutbufOps = (ExynosVideoDecBufferOps *)malloc(sizeof(*pOutbufOps));

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate decoder ops buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVideoDec->pDecOps = pDecOps;
    pVideoDec->pInbufOps = pInbufOps;
    pVideoDec->pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pDecOps->nSize = sizeof(*pDecOps);
    pInbufOps->nSize = sizeof(*pInbufOps);
    pOutbufOps->nSize = sizeof(*pOutbufOps);

    Exynos_Video_Register_Decoder(pDecOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for decoder ops */
    if ((pDecOps->Init == NULL) || (pDecOps->Finalize == NULL) ||
        (pDecOps->Get_ActualBufferCount == NULL) || (pDecOps->Set_FrameTag == NULL) ||
        (pDecOps->Get_FrameTag == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* check mandatory functions for buffer ops */
    if ((pInbufOps->Setup == NULL) || (pOutbufOps->Setup == NULL) ||
        (pInbufOps->Run == NULL) || (pOutbufOps->Run == NULL) ||
        (pInbufOps->Stop == NULL) || (pOutbufOps->Stop == NULL) ||
        (pInbufOps->Enqueue == NULL) || (pOutbufOps->Enqueue == NULL) ||
        (pInbufOps->Dequeue == NULL) || (pOutbufOps->Dequeue == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* alloc context, open, querycap */
    pVp8Dec->hMFCVp8Handle.hMFCHandle = pVideoDec->pDecOps->Init();
    if (pVp8Dec->hMFCVp8Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    /* input buffer info: only 2 config values needed */
    bufferConf.nSizeImage = DEFAULT_MFC_INPUT_BUFFER_SIZE / 2;
    bufferConf.eCompressionFormat = VIDEO_CODING_VP8;

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry) {
        if (pInbufOps->Set_Geometry(pVp8Dec->hMFCVp8Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* setup input buffer */
    if (pInbufOps->Setup(pVp8Dec->hMFCVp8Handle.hMFCHandle, MFC_INPUT_BUFFER_NUM_MAX) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* get input buffer info */
    for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
        Exynos_OSAL_Memset(&bufferInfo, 0, sizeof(bufferInfo));

        if (pInbufOps->Get_BufferInfo) {
           if (pInbufOps->Get_BufferInfo(pVp8Dec->hMFCVp8Handle.hMFCHandle, i, &bufferInfo) != VIDEO_ERROR_NONE) {
               Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get input buffer info");
               ret = OMX_ErrorInsufficientResources;
               goto EXIT;
           }
        }

        pVideoDec->MFCDecInputBuffer[i].VirAddr = (void *)bufferInfo.planes[0].addr;
        pVideoDec->MFCDecInputBuffer[i].PhyAddr = NULL;
        pVideoDec->MFCDecInputBuffer[i].bufferSize = bufferInfo.planes[0].allocSize;
        pVideoDec->MFCDecInputBuffer[i].dataSize = 0;
    }

    pVideoDec->indexInputBuffer = 0;
    pVideoDec->bFirstFrame = OMX_TRUE;

    pVideoDec->NBDecThread.bExitDecodeThread = OMX_FALSE;
    pVideoDec->NBDecThread.bDecoderRun = OMX_FALSE;
    pVideoDec->NBDecThread.oneFrameSize = 0;
    Exynos_OSAL_SemaphoreCreate(&(pVideoDec->NBDecThread.hDecFrameStart));
    Exynos_OSAL_SemaphoreCreate(&(pVideoDec->NBDecThread.hDecFrameEnd));
    if (OMX_ErrorNone == Exynos_OSAL_ThreadCreate(&pVideoDec->NBDecThread.hNBDecodeThread,
                                                Exynos_MFC_DecodeThread,
                                                pOMXComponent)) {
        pVp8Dec->hMFCVp8Handle.returnCodec = VIDEO_TRUE;
    }

    pVp8Dec->hMFCVp8Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[0].VirAddr;
    pVp8Dec->hMFCVp8Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[0].PhyAddr;
    pExynosComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[0].VirAddr;
    pExynosComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[0].bufferSize;

    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pVp8Dec->hMFCVp8Handle.indexTimestamp = 0;
    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

#ifdef USE_ANB
#if defined(USE_CSC_FIMC) || defined(USE_CSC_GSCALER)
    if (pExynosOutputPort->bIsANBEnabled == OMX_TRUE) {
        csc_method = CSC_METHOD_PREFER_HW;
    }
#endif
#endif
    pVideoDec->csc_handle = csc_init(&csc_method);
    pVideoDec->csc_set_format = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_MFC_VP8Dec_Terminate(
    OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec        = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec          = NULL;
    OMX_PTR                        hMFCHandle       = NULL;
    ExynosVideoDecOps             *pDecOps          = pVideoDec->pDecOps;
    ExynosVideoDecBufferOps       *pInbufOps        = pVideoDec->pInbufOps;
    ExynosVideoDecBufferOps       *pOutbufOps       = pVideoDec->pOutbufOps;

    FunctionIn();

#ifdef CONFIG_MFC_FPS
    Exynos_OSAL_PerfPrint("[DEC]",  PERF_ID_DEC);
    Exynos_OSAL_PerfPrint("[CSC]",  PERF_ID_CSC);
#endif

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;

    pVp8Dec->hMFCVp8Handle.pMFCStreamBuffer    = NULL;
    pVp8Dec->hMFCVp8Handle.pMFCStreamPhyBuffer = NULL;
    pExynosComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pExynosComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

    if (pVideoDec->NBDecThread.hNBDecodeThread != NULL) {
        pVideoDec->NBDecThread.bExitDecodeThread = OMX_TRUE;
        Exynos_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameStart);
        Exynos_OSAL_ThreadTerminate(pVideoDec->NBDecThread.hNBDecodeThread);
        pVideoDec->NBDecThread.hNBDecodeThread = NULL;
    }

    if(pVideoDec->NBDecThread.hDecFrameEnd != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoDec->NBDecThread.hDecFrameEnd);
        pVideoDec->NBDecThread.hDecFrameEnd = NULL;
    }

    if(pVideoDec->NBDecThread.hDecFrameStart != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoDec->NBDecThread.hDecFrameStart);
        pVideoDec->NBDecThread.hDecFrameStart = NULL;
    }

    if (hMFCHandle != NULL) {
        pInbufOps->Stop(hMFCHandle);
        pOutbufOps->Stop(hMFCHandle);
        pDecOps->Finalize(hMFCHandle);

        free(pInbufOps);
        free(pOutbufOps);
        free(pDecOps);

        pVp8Dec->hMFCVp8Handle.hMFCHandle = NULL;
    }

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_VP8_Decode_Configure(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pInputData,
    EXYNOS_OMX_DATA     *pOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_HANDLETYPE                 hMFCHandle        = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pInputData->dataLen;
    OMX_S32                        setConfVal        = 0;
    ExynosVideoDecOps             *pDecOps           = pVideoDec->pDecOps;
    ExynosVideoDecBufferOps       *pInbufOps         = pVideoDec->pInbufOps;
    ExynosVideoDecBufferOps       *pOutbufOps        = pVideoDec->pOutbufOps;
    ExynosVideoGeometry            bufferConf;

    int i, nOutbufs;

    FunctionIn();

    if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pDecOps->Set_DisplayDelay && (pVideoDec->bThumbnailMode == OMX_TRUE)) {
        setConfVal = 0;
        pDecOps->Set_DisplayDelay(hMFCHandle, setConfVal);
    }

    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    /* set geometry for dest */
    if (pOutbufOps->Set_Geometry) {
        /* only output color format is needed to set */
        bufferConf.eColorFormat = VIDEO_COLORFORMAT_NV12_TILED;
        if (pOutbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* input buffer enqueue for header parsing */
    if (pInbufOps->Enqueue(hMFCHandle,
                          (unsigned char **)&pVp8Dec->hMFCVp8Handle.pMFCStreamBuffer,
                          (unsigned int *)&oneFrameSize, 1, NULL) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enqueue input buffer for header parsing");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* start header parsing */
    if (pInbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run input buffer for header parsing");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    /* get geometry for output */
    if (pOutbufOps->Get_Geometry) {
        if (pOutbufOps->Get_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get geometry for parsed header info");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    if ((pExynosInputPort->portDefinition.format.video.nFrameWidth != bufferConf.nFrameWidth) ||
        (pExynosInputPort->portDefinition.format.video.nFrameHeight != bufferConf.nFrameHeight)) {
        pExynosInputPort->portDefinition.format.video.nFrameWidth = bufferConf.nFrameWidth;
        pExynosInputPort->portDefinition.format.video.nFrameHeight = bufferConf.nFrameHeight;
        pExynosInputPort->portDefinition.format.video.nStride = ((bufferConf.nFrameWidth + 15) & (~15));
        pExynosInputPort->portDefinition.format.video.nSliceHeight = ((bufferConf.nFrameHeight + 15) & (~15));

        Exynos_UpdateFrameSize(pOMXComponent);

        /** Send Port Settings changed call back **/
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent,
             pExynosComponent->callbackData,
             OMX_EventPortSettingsChanged, /* The command was completed */
             OMX_DirOutput, /* This is the port index */
             0,
             NULL);
    }

    /* should be done before prepare output buffer */
    if (pExynosOutputPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV12TPhysicalAddress) {
        if (pVideoDec->pDecOps->Enable_Cacheable) {
            if (pVideoDec->pDecOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }

    /* get dpb count */
    nOutbufs = pDecOps->Get_ActualBufferCount(hMFCHandle);
    if (nOutbufs < 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Invalid DPB count");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pVideoDec->bThumbnailMode == OMX_FALSE)
        nOutbufs += VP8_DEC_NUM_OF_EXTRA_BUFFERS;

    if (pOutbufOps->Setup(hMFCHandle, nOutbufs) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pOutbufOps->Enqueue_All) {
        if (pOutbufOps->Enqueue_All(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to flush all output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pInbufOps->Dequeue(hMFCHandle) == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to wait input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVp8Dec->hMFCVp8Handle.bConfiguredMFC = OMX_TRUE;
    pVp8Dec->hMFCVp8Handle.returnCodec = VIDEO_TRUE;
    pOutputData->timeStamp = pInputData->timeStamp;
    pOutputData->nFlags = pInputData->nFlags;

    ret = OMX_ErrorInputDataDecodeYet;

EXIT:
    FunctionOut();
    return ret;
}
OMX_ERRORTYPE Exynos_MFC_VP8_Decode_Nonblock(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pInputData,
    EXYNOS_OMX_DATA     *pOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_HANDLETYPE                 hMFCHandle        = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    OMX_U32                        oneFrameSize      = pInputData->dataLen;
    OMX_S32                        setConfVal        = 0;
    OMX_U32                        FrameBufferYSize  = 0;
    OMX_U32                        FrameBufferUVSize = 0;
    OMX_BOOL                       outputDataValid   = OMX_FALSE;
    ExynosVideoDecOps             *pDecOps           = pVideoDec->pDecOps;
    ExynosVideoDecBufferOps       *pInbufOps         = pVideoDec->pInbufOps;
    ExynosVideoDecBufferOps       *pOutbufOps        = pVideoDec->pOutbufOps;

    OMX_PTR outputYPhyAddr, outputCPhyAddr, outputYVirAddr, outputCVirAddr;
    int bufWidth, bufHeight, outputImgWidth, outputImgHeight;

    FunctionIn();

    if (pVp8Dec->hMFCVp8Handle.bConfiguredMFC == OMX_FALSE) {
        ret = Exynos_MFC_VP8_Decode_Configure(pOMXComponent, pInputData, pOutputData);
        goto EXIT;
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pExynosComponent->bUseFlagEOF == OMX_FALSE))
        pExynosComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.indexTimestamp] = pInputData->timeStamp;
    pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.indexTimestamp] = pInputData->nFlags;

    if (pVideoDec->bFirstFrame == OMX_FALSE) {
        ExynosVideoFrameStatusType status = VIDEO_FRAME_STATUS_UNKNOWN;
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc decode done */
        if (pVideoDec->NBDecThread.bDecoderRun == OMX_TRUE) {
            Exynos_OSAL_SemaphoreWait(pVideoDec->NBDecThread.hDecFrameEnd);
            pVideoDec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        Exynos_OSAL_SleepMillisec(0);

        if (pVideoDec->pOutbuf != NULL) {
            status = pVideoDec->pOutbuf->displayStatus;
            outputYVirAddr = pVideoDec->pOutbuf->planes[0].addr;
            outputCVirAddr = pVideoDec->pOutbuf->planes[1].addr;
        }

        outputImgWidth = pExynosInputPort->portDefinition.format.video.nFrameWidth;
        outputImgHeight = pExynosInputPort->portDefinition.format.video.nFrameHeight;

        bufWidth = (outputImgWidth + 15) & (~15);
        bufHeight = (outputImgHeight + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputImgWidth) * ALIGN_TO_32B(outputImgHeight));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputImgWidth) * ALIGN_TO_32B(outputImgHeight/2));

        indexTimestamp = pDecOps->Get_FrameTag(hMFCHandle);
        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            /* For timestamp correction. if mfc support frametype detect */
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "disp_pic_frame_type: %d", pVideoDec->pOutbuf->frameType);
#ifdef NEED_TIMESTAMP_REORDER
            if (pVideoDec->pOutbuf->frameType == VIDEO_FRAME_I) {
                pOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
                pOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
                pVp8Dec->hMFCVp8Handle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
                pOutputData->nFlags = pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
            }
#else
            pOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
#endif
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "timestamp %lld us (%.2f secs)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);
        }

        if ((status == VIDEO_FRAME_STATUS_DISPLAY_DECODING) ||
            (status == VIDEO_FRAME_STATUS_DISPLAY_ONLY)) {
            outputDataValid = OMX_TRUE;
            pVp8Dec->hMFCVp8Handle.outputIndexTimestamp++;
            pVp8Dec->hMFCVp8Handle.outputIndexTimestamp %= MAX_TIMESTAMP;
        }
        if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS)
            outputDataValid = OMX_FALSE;

        if ((status == VIDEO_FRAME_STATUS_DISPLAY_ONLY) ||
            (pExynosComponent->getAllDelayBuffer == OMX_TRUE))
            ret = OMX_ErrorInputDataDecodeYet;

        if (status == VIDEO_FRAME_STATUS_DECODING_ONLY) {
            if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
                ((pExynosComponent->bSaveFlagEOS == OMX_TRUE) || (pExynosComponent->getAllDelayBuffer == OMX_TRUE))) {
                pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pExynosComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else {
                ret = OMX_ErrorNone;
            }
            outputDataValid = OMX_FALSE;
        }

#ifdef FULL_FRAME_SEARCH
        if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
            (pExynosComponent->bSaveFlagEOS == OMX_TRUE)) {
            pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pExynosComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else
#endif
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pExynosComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            ret = OMX_ErrorNone;
        }
    } else {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        if ((pExynosComponent->bSaveFlagEOS == OMX_TRUE) ||
            (pExynosComponent->getAllDelayBuffer == OMX_TRUE) ||
            (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
        }

        if ((pVideoDec->bFirstFrame == OMX_TRUE) &&
            ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
            ((pInputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG) != OMX_BUFFERFLAG_CODECCONFIG)) {
            pOutputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
        }

        outputDataValid = OMX_FALSE;

        /* ret = OMX_ErrorUndefined; */
        ret = OMX_ErrorNone;
    }

    if (ret == OMX_ErrorInputDataDecodeYet) {
        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
        pVideoDec->indexInputBuffer++;
        pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pVp8Dec->hMFCVp8Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pVp8Dec->hMFCVp8Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
        pExynosComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pExynosComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].bufferSize;
        oneFrameSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize;
    }

    if ((Check_VP8_StartCode(pInputData->dataBuffer, oneFrameSize) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        if ((ret != OMX_ErrorInputDataDecodeYet) || (pExynosComponent->getAllDelayBuffer == OMX_TRUE)) {
            pDecOps->Set_FrameTag(hMFCHandle, pVp8Dec->hMFCVp8Handle.indexTimestamp);
            pVp8Dec->hMFCVp8Handle.indexTimestamp++;
            pVp8Dec->hMFCVp8Handle.indexTimestamp %= MAX_TIMESTAMP;
        }

        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
        pVideoDec->NBDecThread.oneFrameSize = oneFrameSize;

        /* mfc decode start */
        Exynos_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameStart);
        pVideoDec->NBDecThread.bDecoderRun = OMX_TRUE;
        pVp8Dec->hMFCVp8Handle.returnCodec = VIDEO_TRUE;

        Exynos_OSAL_SleepMillisec(0);

        if ((pVideoDec->bFirstFrame == OMX_TRUE) &&
            (pExynosComponent->bSaveFlagEOS == OMX_TRUE) &&
            (outputDataValid == OMX_FALSE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }

        pVideoDec->bFirstFrame = OMX_FALSE;
    } else {
        if (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE)
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
    }

    /** Fill Output Buffer **/
    if (outputDataValid == OMX_TRUE) {
        void        *pOutputBuf = (void *)pOutputData->dataBuffer;
        void        *pSrcBuf[3] = {NULL, };
        void        *pYUVBuf[3] = {NULL, };
        unsigned int csc_src_color_format, csc_dst_color_format;
        CSC_METHOD   csc_method = CSC_METHOD_SW;
        unsigned int cacheable  = 1;
        int          frameSize  = bufWidth * bufHeight;
        int          width      = outputImgWidth;
        int          height     = outputImgHeight;
        int          imageSize  = outputImgWidth * outputImgHeight;

        pSrcBuf[0] = outputYVirAddr;
        pSrcBuf[1] = outputCVirAddr;

        pYUVBuf[0]  = (unsigned char *)pOutputBuf;
        pYUVBuf[1]  = (unsigned char *)pOutputBuf + imageSize;
        pYUVBuf[2]  = (unsigned char *)pOutputBuf + imageSize + imageSize / 4;
        pOutputData->dataLen = (imageSize * 3) / 2;

#ifdef USE_ANB
        if (pExynosOutputPort->bIsANBEnabled == OMX_TRUE) {
            unsigned int stride;
            Exynos_OSAL_LockANB(pOutputData->dataBuffer, width, height, pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat, &stride, pYUVBuf);
            width = stride;
            pOutputData->dataLen = sizeof(void *);
         }
#endif
        if ((pVideoDec->bThumbnailMode == OMX_FALSE) &&
            (pExynosOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress)) {
            /* if use Post copy address structure */
            Exynos_OSAL_Memcpy(pYUVBuf[0], &(outputYPhyAddr), sizeof(outputYPhyAddr));
            Exynos_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 1), &(outputCPhyAddr), sizeof(outputCPhyAddr));
            Exynos_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 2), &(outputYVirAddr), sizeof(outputYVirAddr));
            Exynos_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 3), &(outputCVirAddr), sizeof(outputCVirAddr));
            pOutputData->dataLen = (width * height * 3) / 2;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "YUV420 SP/P Output mode");
#ifdef CONFIG_MFC_FPS
            Exynos_OSAL_PerfStart(PERF_ID_CSC);
#endif
            switch (pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat) {
            case OMX_SEC_COLOR_FormatNV12Tiled:
                Exynos_OSAL_Memcpy(pOutputBuf, outputYVirAddr, FrameBufferYSize);
                Exynos_OSAL_Memcpy((unsigned char *)pOutputBuf + FrameBufferYSize, outputCVirAddr, FrameBufferUVSize);
                pOutputData->dataLen = FrameBufferYSize + FrameBufferUVSize;
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_SEC_COLOR_FormatNV12Tiled);
                csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
                break;
            case OMX_COLOR_FormatYUV420Planar:
            default:
                csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_SEC_COLOR_FormatNV12Tiled);
                csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420Planar);
                break;
            }

            csc_get_method(pVideoDec->csc_handle, &csc_method);
#ifdef USE_CSC_FIMC
            if ((pExynosOutputPort->bIsANBEnabled == OMX_TRUE) && (csc_method == CSC_METHOD_HW)) {
                Exynos_OSAL_GetPhysANB(pOutputData->dataBuffer, pYUVBuf);
                pSrcBuf[0] = outputYPhyAddr;
                pSrcBuf[1] = outputCPhyAddr;
            }
#endif
            if (pVideoDec->csc_set_format == OMX_FALSE) {
                csc_set_src_format(
                    pVideoDec->csc_handle,  /* handle */
                    width,                  /* width */
                    height,                 /* height */
                    0,                      /* crop_left */
                    0,                      /* crop_right */
                    width,                  /* crop_width */
                    height,                 /* crop_height */
                    csc_src_color_format,   /* color_format */
                    cacheable);             /* cacheable */
                csc_set_dst_format(
                    pVideoDec->csc_handle,  /* handle */
                    width,                  /* width */
                    height,                 /* height */
                    0,                      /* crop_left */
                    0,                      /* crop_right */
                    width,                  /* crop_width */
                    height,                 /* crop_height */
                    csc_dst_color_format,   /* color_format */
                    cacheable);             /* cacheable */
                pVideoDec->csc_set_format = OMX_TRUE;
            }
            csc_set_src_buffer(
                pVideoDec->csc_handle,  /* handle */
                pSrcBuf[0],             /* y addr */
                pSrcBuf[1],             /* u addr or uv addr */
                pSrcBuf[2],             /* v addr or none */
                0);                     /* ion fd */
            csc_set_dst_buffer(
                pVideoDec->csc_handle,
                pYUVBuf[0],             /* y addr */
                pYUVBuf[1],             /* u addr or uv addr */
                pYUVBuf[2],             /* v addr or none */
                0);                     /* ion fd */
            csc_convert(pVideoDec->csc_handle);

#ifdef CONFIG_MFC_FPS
            Exynos_OSAL_PerfStop(PERF_ID_CSC);
#endif
        }
#ifdef USE_ANB
        if (pExynosOutputPort->bIsANBEnabled == OMX_TRUE) {
            Exynos_OSAL_UnlockANB(pOutputData->dataBuffer);
        }
#endif
        /* enqueue all available output buffers */
        pOutbufOps->Enqueue_All(hMFCHandle);
    } else {
        pOutputData->dataLen = 0;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Decode */
OMX_ERRORTYPE Exynos_MFC_VP8Dec_bufferProcess(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pInputData,
    EXYNOS_OMX_DATA     *pOutputData)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_VP8DEC_HANDLE        *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT         *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT         *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                     endOfFrame         = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_ENABLED(pExynosOutputPort)) ||
        (!CHECK_PORT_POPULATED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_MFC_VP8_Decode_Nonblock(pOMXComponent, pInputData, pOutputData);
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataDecodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                    pExynosComponent->callbackData,
                                                    OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->previousDataLen = pInputData->dataLen;
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        pOutputData->usedDataLen = 0;
        pOutputData->remainDataLen = pOutputData->dataLen;
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(
    OMX_HANDLETYPE  hComponent,
    OMX_STRING      componentName)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = NULL;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = NULL;

    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_VP8_DEC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_DEC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pVp8Dec = Exynos_OSAL_Malloc(sizeof(EXYNOS_VP8DEC_HANDLE));
    if (pVp8Dec == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pVp8Dec, 0, sizeof(EXYNOS_VP8DEC_HANDLE));
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pVp8Dec;

    Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_VP8_DEC);

    /* Set componentVersion */
    pExynosComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pExynosComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Android CapabilityFlags */
    pExynosComponent->capabilityFlags.iIsOMXComponentMultiThreaded                   = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc  = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsMovableInputBuffers       = OMX_FALSE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsPartialFrames             = OMX_FALSE;
    pExynosComponent->capabilityFlags.iOMXComponentUsesNALStartCodes                 = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentCanHandleIncompleteFrames         = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentUsesFullAVCFrames                 = OMX_TRUE;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingVPX;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp8");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    pOMXComponent->GetParameter      = &Exynos_MFC_VP8Dec_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_MFC_VP8Dec_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_MFC_VP8Dec_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_MFC_VP8Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_MFC_VP8Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_MFC_VP8Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_mfc_componentInit      = &Exynos_MFC_VP8Dec_Init;
    pExynosComponent->exynos_mfc_componentTerminate = &Exynos_MFC_VP8Dec_Terminate;
    pExynosComponent->exynos_mfc_bufferProcess      = &Exynos_MFC_VP8Dec_bufferProcess;
    pExynosComponent->exynos_checkInputFrame        = &Check_VP8_Frame;

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(
    OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;
    EXYNOS_VP8DEC_HANDLE        *pVp8Dec            = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    if (pVp8Dec != NULL) {
        Exynos_OSAL_Free(pVp8Dec);
        pVp8Dec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
