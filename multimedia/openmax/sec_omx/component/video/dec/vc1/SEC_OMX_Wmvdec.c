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
 * @file    SEC_OMX_Wmvdec.c
 * @brief
 * @author    HyeYeon Chung (hyeon.chung@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.8.16 : Create
 *   2010.8.20 : Support WMV3 (Vc-1 Simple/Main Profile)
 *   2010.8.21 : Support WMvC1 (Vc-1 Advanced Profile)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEC_OMX_Macros.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Vdec.h"
#include "SEC_OSAL_ETC.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Thread.h"
#include "SEC_OSAL_Memory.h"
#include "library_register.h"
#include "SEC_OMX_Wmvdec.h"
#include "SsbSipMfcApi.h"
#include "SEC_OSAL_Event.h"

#ifdef USE_ANB
#include "SEC_OSAL_Android.h"
#endif

/* To use CSC_METHOD_PREFER_HW or CSC_METHOD_HW in SEC OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_WMV_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

#define WMV_DEC_NUM_OF_EXTRA_BUFFERS 7

//#define FULL_FRAME_SEARCH

/* ASF parser does not send start code on OpenCORE */
#define WO_START_CODE

static OMX_HANDLETYPE ghMFCHandle = NULL;
static WMV_FORMAT gWvmFormat = WMV_FORMAT_UNKNOWN;

const OMX_U32 wmv3 = 0x33564d57;
const OMX_U32 wvc1 = 0x31435657;
const OMX_U32 wmva = 0x41564d57;

static int Check_Wmv_Frame(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    OMX_U32  compressionID;
    OMX_BOOL bFrameStart;
    OMX_U32  len, readStream;
    OMX_U32  startCode;

    SEC_OSAL_Log(SEC_LOG_TRACE, "buffSize = %d", buffSize);

    len = 0;
    bFrameStart = OMX_FALSE;

    if (flag & OMX_BUFFERFLAG_CODECCONFIG) {
        BitmapInfoHhr *pBitmapInfoHeader;
        pBitmapInfoHeader = (BitmapInfoHhr *)pInputStream;

        compressionID = pBitmapInfoHeader->BiCompression;
        if (compressionID == wmv3) {
            SEC_OSAL_Log(SEC_LOG_TRACE, "WMV_FORMAT_WMV3");
            gWvmFormat  = WMV_FORMAT_WMV3;

            *pbEndOfFrame = OMX_TRUE;
            return buffSize;
        }
        else if ((compressionID == wvc1) || (compressionID == wmva)) {
            SEC_OSAL_Log(SEC_LOG_TRACE, "WMV_FORMAT_VC1");
            gWvmFormat  = WMV_FORMAT_VC1;

#ifdef WO_START_CODE
/* ASF parser does not send start code on OpenCORE */
            *pbEndOfFrame = OMX_TRUE;
            return buffSize;
#endif
        }
    }

    if (gWvmFormat == WMV_FORMAT_WMV3) {
        *pbEndOfFrame = OMX_TRUE;
        return buffSize;
    }

#ifdef WO_START_CODE
/* ASF parser does not send start code on OpenCORE */
    if (gWvmFormat == WMV_FORMAT_VC1) {
        *pbEndOfFrame = OMX_TRUE;
        return buffSize;
    }
#else
 /* TODO : for comformanc test based on common buffer scheme w/o parser */

    if (bPreviousFrameEOF == OMX_FALSE)
        bFrameStart = OMX_TRUE;

    startCode = 0xFFFFFFFF;
    if (bFrameStart == OMX_FALSE) {
        /* find Frame start code */
        while(startCode != 0x10D) {
            readStream = *(pInputStream + len);
            startCode = (startCode << 8) | readStream;
            len++;
            if (len > buffSize)
                goto EXIT;
        }
    }

    /* find next Frame start code */
    startCode = 0xFFFFFFFF;
    while ((startCode != 0x10D)) {
        readStream = *(pInputStream + len);
        startCode = (startCode << 8) | readStream;
        len++;
        if (len > buffSize)
            goto EXIT;
    }

    *pbEndOfFrame = OMX_TRUE;

    SEC_OSAL_Log(SEC_LOG_TRACE, "1. Check_Wmv_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 4, buffSize);

    return len - 4;
#endif

EXIT :
    *pbEndOfFrame = OMX_FALSE;

    SEC_OSAL_Log(SEC_LOG_TRACE, "2. Check_Wmv_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 1, buffSize);

    return --len;
}

OMX_BOOL Check_Stream_PrefixCode(OMX_U8 *pInputStream, OMX_U32 streamSize, WMV_FORMAT wmvFormat)
{
    switch (wmvFormat) {
    case WMV_FORMAT_WMV3:
        if (streamSize > 0)
            return OMX_TRUE;
        else
            return OMX_FALSE;
        break;
    case WMV_FORMAT_VC1:

#ifdef WO_START_CODE
        /* ASF parser does not send start code on OpenCORE */
        if (streamSize > 3)
            return OMX_TRUE;
        else
            return OMX_FALSE;
        break;
#else
        /* TODO : for comformanc test based on common buffer scheme w/o parser */
        if (streamSize < 3) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: streamSize is too small (%d)", __FUNCTION__, streamSize);
            return OMX_FALSE;
        } else if ((pInputStream[0] == 0x00) &&
                   (pInputStream[1] == 0x00) &&
                   (pInputStream[2] == 0x01)) {
            return OMX_TRUE;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: Cannot find prefix", __FUNCTION__);
            return OMX_FALSE;
        }
#endif
        break;

    default:
        SEC_OSAL_Log(SEC_LOG_WARNING, "%s: undefined wmvFormat (%d)", __FUNCTION__, wmvFormat);
        return OMX_FALSE;
        break;
    }
}

OMX_BOOL Make_Stream_MetaData(OMX_U8 *pInputStream, OMX_U32 *pStreamSize, WMV_FORMAT wmvFormat)
{
    OMX_U32 width, height;
    OMX_U8  *pCurrBuf = pInputStream;
    OMX_U32 currPos = 0;

    FunctionIn();

    /* Sequence Layer Data Structure */
    OMX_U8 const_C5[4] = {0x00, 0x00, 0x00, 0xc5};
    OMX_U8 const_04[4] = {0x04, 0x00, 0x00, 0x00};
    OMX_U8 const_0C[4] = {0x0C, 0x00, 0x00, 0x00};
    OMX_U8 struct_B_1[4] = {0xB3, 0x19, 0x00, 0x00};
    OMX_U8 struct_B_2[4] = {0x44, 0x62, 0x05, 0x00};
    OMX_U8 struct_B_3[4] = {0x0F, 0x00, 0x00, 0x00};
    OMX_U8 struct_C[4] = {0x30, 0x00, 0x00, 0x00};

    switch (wmvFormat) {
    case WMV_FORMAT_WMV3:
        if (*pStreamSize >= BITMAPINFOHEADER_SIZE) {
            BitmapInfoHhr *pBitmapInfoHeader;
            pBitmapInfoHeader = (BitmapInfoHhr *)pInputStream;

            width = pBitmapInfoHeader->BiWidth;
            height = pBitmapInfoHeader->BiHeight;
            if (*pStreamSize > BITMAPINFOHEADER_SIZE)
                SEC_OSAL_Memcpy(struct_C, pInputStream+BITMAPINFOHEADER_SIZE, 4);

            SEC_OSAL_Memcpy(pCurrBuf + currPos, const_C5, 4);
            currPos +=4;

            SEC_OSAL_Memcpy(pCurrBuf + currPos, const_04, 4);
            currPos +=4;

            SEC_OSAL_Memcpy(pCurrBuf + currPos, struct_C, 4);
            currPos +=4;

            /* struct_A : VERT_SIZE */
            pCurrBuf[currPos] =  height & 0xFF;
            pCurrBuf[currPos+1] = (height>>8) & 0xFF;
            pCurrBuf[currPos+2] = (height>>16) & 0xFF;
            pCurrBuf[currPos+3] = (height>>24) & 0xFF;
            currPos +=4;

            /* struct_A : HORIZ_SIZE */
            pCurrBuf[currPos] =  width & 0xFF;
            pCurrBuf[currPos+1] = (width>>8) & 0xFF;
            pCurrBuf[currPos+2] = (width>>16) & 0xFF;
            pCurrBuf[currPos+3] = (width>>24) & 0xFF;
            currPos +=4;

            SEC_OSAL_Memcpy(pCurrBuf + currPos,const_0C, 4);
            currPos +=4;

            SEC_OSAL_Memcpy(pCurrBuf + currPos, struct_B_1, 4);
            currPos +=4;

            SEC_OSAL_Memcpy(pCurrBuf + currPos, struct_B_2, 4);
            currPos +=4;

            SEC_OSAL_Memcpy(pCurrBuf + currPos, struct_B_3, 4);
            currPos +=4;

            *pStreamSize = currPos;
            return OMX_TRUE;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: *pStreamSize is too small to contain metadata(%d)", __FUNCTION__, *pStreamSize);
            return OMX_FALSE;
        }
        break;
    case WMV_FORMAT_VC1:
        if (*pStreamSize >= BITMAPINFOHEADER_ASFBINDING_SIZE) {
            SEC_OSAL_Memcpy(pCurrBuf, pInputStream + BITMAPINFOHEADER_ASFBINDING_SIZE, *pStreamSize - BITMAPINFOHEADER_ASFBINDING_SIZE);
            *pStreamSize -= BITMAPINFOHEADER_ASFBINDING_SIZE;
            return OMX_TRUE;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: *pStreamSize is too small to contain metadata(%d)", __FUNCTION__, *pStreamSize);
            return OMX_FALSE;
        }
        break;
    default:
        SEC_OSAL_Log(SEC_LOG_WARNING, "%s: It is not necessary to make bitstream metadata for wmvFormat (%d)", __FUNCTION__, wmvFormat);
        return OMX_FALSE;
        break;
    }
}

#ifdef WO_START_CODE
OMX_BOOL Make_Stream_StartCode(OMX_U8 *pInputStream, OMX_U32 streamSize, WMV_FORMAT wmvFormat)
{
    OMX_U8  frameStartCode[4] = {0x00, 0x00, 0x01, 0x0d};
    OMX_U32 i;

    switch (wmvFormat) {
    case WMV_FORMAT_WMV3:
        return OMX_TRUE;
        break;

    case WMV_FORMAT_VC1:
        /* Should find better way to shift data */
        SEC_OSAL_Memmove(pInputStream+4, pInputStream, streamSize);
        SEC_OSAL_Memcpy(pInputStream, frameStartCode, 4);

        return OMX_TRUE;
        break;

    default:
        SEC_OSAL_Log(SEC_LOG_WARNING, "%s: undefined wmvFormat (%d)", __FUNCTION__, wmvFormat);
        return OMX_FALSE;
        break;
    }
}
#endif

OMX_ERRORTYPE SEC_MFC_WmvDec_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamVideoWmv:
    {
        OMX_VIDEO_PARAM_WMVTYPE *pDstWmvParam = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_WMVTYPE *pSrcWmvParam = NULL;
        SEC_WMV_HANDLE          *pWmvDec = NULL;
        ret = SEC_OMX_Check_SizeVersion(pDstWmvParam, sizeof(OMX_VIDEO_PARAM_WMVTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstWmvParam->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
        }

        pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcWmvParam = &pWmvDec->WmvComponent[pDstWmvParam->nPortIndex];

        SEC_OSAL_Memcpy(pDstWmvParam, pSrcWmvParam, sizeof(OMX_VIDEO_PARAM_WMVTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;
        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
            if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_WMV_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        SEC_WMV_HANDLE                      *pWmvDec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pWmvDec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = SEC_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_WmvDec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamVideoWmv:
    {
        OMX_VIDEO_PARAM_WMVTYPE *pDstWmvParam = NULL;
        OMX_VIDEO_PARAM_WMVTYPE *pSrcWmvParam = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;
        SEC_WMV_HANDLE          *pWmvDec = NULL;
        ret = SEC_OMX_Check_SizeVersion(pSrcWmvParam, sizeof(OMX_VIDEO_PARAM_WMVTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcWmvParam->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstWmvParam = &pWmvDec->WmvComponent[pSrcWmvParam->nPortIndex];

        SEC_OSAL_Memcpy(pDstWmvParam, pSrcWmvParam, sizeof(OMX_VIDEO_PARAM_WMVTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_WMV_DEC_ROLE)) {
            pSECComponent->pSECPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                       portIndex = pPortDefinition->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort;
        OMX_U32 width, height, size;
        OMX_U32 realWidth, realHeight;

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = SEC_OMX_Check_SizeVersion(pPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];

        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            if (pSECPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }
        if (pPortDefinition->nBufferCountActual < pSECPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        SEC_OSAL_Memcpy(&pSECPort->portDefinition, pPortDefinition, pPortDefinition->nSize);

        realWidth = pSECPort->portDefinition.format.video.nFrameWidth;
        realHeight = pSECPort->portDefinition.format.video.nFrameHeight;
        width = ((realWidth + 15) & (~15));
        height = ((realHeight + 15) & (~15));
        size = (width * height * 3) / 2;
        pSECPort->portDefinition.format.video.nStride = width;
        pSECPort->portDefinition.format.video.nSliceHeight = height;
        pSECPort->portDefinition.nBufferSize = (size > pSECPort->portDefinition.nBufferSize) ? size : pSECPort->portDefinition.nBufferSize;

        if (portIndex == INPUT_PORT_INDEX) {
            SEC_OMX_BASEPORT *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
            pSECOutputPort->portDefinition.format.video.nFrameWidth = pSECPort->portDefinition.format.video.nFrameWidth;
            pSECOutputPort->portDefinition.format.video.nFrameHeight = pSECPort->portDefinition.format.video.nFrameHeight;
            pSECOutputPort->portDefinition.format.video.nStride = width;
            pSECOutputPort->portDefinition.format.video.nSliceHeight = height;

            switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                pSECOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
                break;
            case OMX_SEC_COLOR_FormatNV12Tiled:
                pSECOutputPort->portDefinition.nBufferSize =
                    ALIGN_TO_8KB(ALIGN_TO_128B(realWidth) * ALIGN_TO_32B(realHeight)) \
                  + ALIGN_TO_8KB(ALIGN_TO_128B(realWidth) * ALIGN_TO_32B(realHeight/2));
                break;
            default:
                SEC_OSAL_Log(SEC_LOG_ERROR, "Color format is not support!! use default YUV size!!");
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
        SEC_WMV_HANDLE                      *pWmvDec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pWmvDec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = SEC_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_WmvDec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = SEC_OMX_VideoDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_WmvDec_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = SEC_OMX_VideoDecodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_WmvDec_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_ENABLE_THUMBNAIL) == 0) {
        SEC_WMV_HANDLE *pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

        *pIndexType = OMX_IndexVendorThumbnailMode;

        ret = OMX_ErrorNone;
    } else {
        ret = SEC_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_WmvDec_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_WMV_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorUnsupportedIndex;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_DecodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_WMV_HANDLE        *pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while (pVideoDec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
        SEC_OSAL_SemaphoreWait(pVideoDec->NBDecThread.hDecFrameStart);

        if (pVideoDec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
#ifdef CONFIG_MFC_FPS
            SEC_OSAL_PerfStart(PERF_ID_DEC);
#endif
            pWmvDec->hMFCWmvHandle.returnCodec = SsbSipMfcDecExe(pWmvDec->hMFCWmvHandle.hMFCHandle, pVideoDec->NBDecThread.oneFrameSize);
#ifdef CONFIG_MFC_FPS
            SEC_OSAL_PerfStop(PERF_ID_DEC);
#endif
            SEC_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameEnd);
        }
    }

EXIT:
    SEC_OSAL_ThreadExit(NULL);
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE SEC_MFC_WmvDec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT      *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_WMV_HANDLE        *pWmvDec = NULL;
    OMX_HANDLETYPE         hMFCHandle = NULL;
    OMX_PTR                pStreamBuffer = NULL;
    OMX_PTR                pStreamPhyBuffer = NULL;
    CSC_METHOD csc_method = CSC_METHOD_SW;

    FunctionIn();

#ifdef CONFIG_MFC_FPS
    SEC_OSAL_PerfInit(PERF_ID_DEC);
    SEC_OSAL_PerfInit(PERF_ID_CSC);
#endif

    pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    pWmvDec->hMFCWmvHandle.bConfiguredMFC = OMX_FALSE;
    pSECComponent->bUseFlagEOF = OMX_FALSE;
    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    /* MFC(Multi Format Codec) decoder and CMM(Codec Memory Management) driver open */
    if (pSECOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress) {
        hMFCHandle = (OMX_PTR)SsbSipMfcDecOpen();
    } else {
        SSBIP_MFC_BUFFER_TYPE buf_type = CACHE;
        hMFCHandle = (OMX_PTR)SsbSipMfcDecOpenExt(&buf_type);
    }

    if (hMFCHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    ghMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle = hMFCHandle;

    /* Allocate decoder's input buffer */
    /* Get first input buffer */
    pStreamBuffer = SsbSipMfcDecGetInBuf(hMFCHandle, &pStreamPhyBuffer, DEFAULT_MFC_INPUT_BUFFER_SIZE / 2);
    if (pStreamBuffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoDec->MFCDecInputBuffer[0].VirAddr = pStreamBuffer;
    pVideoDec->MFCDecInputBuffer[0].PhyAddr = pStreamPhyBuffer;
    pVideoDec->MFCDecInputBuffer[0].bufferSize = DEFAULT_MFC_INPUT_BUFFER_SIZE / 2;
    pVideoDec->MFCDecInputBuffer[0].dataSize = 0;

#ifdef NONBLOCK_MODE_PROCESS
    /* Get second input buffer */
    pStreamBuffer = NULL;
    pStreamBuffer = SsbSipMfcDecGetInBuf(hMFCHandle, &pStreamPhyBuffer, DEFAULT_MFC_INPUT_BUFFER_SIZE / 2);
    if (pStreamBuffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoDec->MFCDecInputBuffer[1].VirAddr = pStreamBuffer;
    pVideoDec->MFCDecInputBuffer[1].PhyAddr = pStreamPhyBuffer;
    pVideoDec->MFCDecInputBuffer[1].bufferSize = DEFAULT_MFC_INPUT_BUFFER_SIZE / 2;
    pVideoDec->MFCDecInputBuffer[1].dataSize = 0;
    pVideoDec->indexInputBuffer = 0;

    pVideoDec->bFirstFrame = OMX_TRUE;

    pVideoDec->NBDecThread.bExitDecodeThread = OMX_FALSE;
    pVideoDec->NBDecThread.bDecoderRun = OMX_FALSE;
    pVideoDec->NBDecThread.oneFrameSize = 0;
    SEC_OSAL_SemaphoreCreate(&(pVideoDec->NBDecThread.hDecFrameStart));
    SEC_OSAL_SemaphoreCreate(&(pVideoDec->NBDecThread.hDecFrameEnd));
    if (OMX_ErrorNone == SEC_OSAL_ThreadCreate(&pVideoDec->NBDecThread.hNBDecodeThread,
                                                SEC_MFC_DecodeThread,
                                                pOMXComponent)) {
        pWmvDec->hMFCWmvHandle.returnCodec = MFC_RET_OK;
    }
#endif

    pWmvDec->hMFCWmvHandle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[0].VirAddr;
    pWmvDec->hMFCWmvHandle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[0].PhyAddr;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[0].VirAddr;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[0].bufferSize;

    SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pWmvDec->hMFCWmvHandle.indexTimestamp = 0;
    pWmvDec->hMFCWmvHandle.outputIndexTimestamp = 0;
    pSECComponent->getAllDelayBuffer = OMX_FALSE;

#ifdef USE_ANB
#if defined(USE_CSC_FIMC) || defined(USE_CSC_GSCALER)
    if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
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
OMX_ERRORTYPE SEC_MFC_WmvDec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_WMV_HANDLE        *pWmvDec = NULL;
    OMX_HANDLETYPE         hMFCHandle = NULL;

    FunctionIn();

#ifdef CONFIG_MFC_FPS
    SEC_OSAL_PerfPrint("[DEC]",  PERF_ID_DEC);
    SEC_OSAL_PerfPrint("[CSC]",  PERF_ID_CSC);
#endif

    pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;

    pWmvDec->hMFCWmvHandle.pMFCStreamBuffer    = NULL;
    pWmvDec->hMFCWmvHandle.pMFCStreamPhyBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

#ifdef NONBLOCK_MODE_PROCESS
        if (pVideoDec->NBDecThread.hNBDecodeThread != NULL) {
            pVideoDec->NBDecThread.bExitDecodeThread = OMX_TRUE;
            SEC_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameStart);
            SEC_OSAL_ThreadTerminate(pVideoDec->NBDecThread.hNBDecodeThread);
            pVideoDec->NBDecThread.hNBDecodeThread = NULL;
        }

        if(pVideoDec->NBDecThread.hDecFrameEnd != NULL) {
            SEC_OSAL_SemaphoreTerminate(pVideoDec->NBDecThread.hDecFrameEnd);
            pVideoDec->NBDecThread.hDecFrameEnd = NULL;
        }

        if(pVideoDec->NBDecThread.hDecFrameStart != NULL) {
            SEC_OSAL_SemaphoreTerminate(pVideoDec->NBDecThread.hDecFrameStart);
            pVideoDec->NBDecThread.hDecFrameStart = NULL;
        }
#endif

    if (hMFCHandle != NULL) {
        SsbSipMfcDecClose(hMFCHandle);
        pWmvDec->hMFCWmvHandle.hMFCHandle = NULL;
    }

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_Wmv_Decode_Nonblock(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_WMV_HANDLE             *pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     configValue = 0;
    OMX_BOOL                    bMetaData = OMX_FALSE;
    OMX_BOOL                    bStartCode = OMX_FALSE;
    int                         bufWidth = 0;
    int                         bufHeight = 0;
    OMX_U32                     FrameBufferYSize = 0;
    OMX_U32                     FrameBufferUVSize = 0;
    OMX_BOOL                    outputDataValid = OMX_FALSE;

    FunctionIn();

    if (pWmvDec->hMFCWmvHandle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE MFCCodecType;

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_WMV3)
            MFCCodecType = VC1RCV_DEC;
        else if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_VC1)
            MFCCodecType = VC1_DEC;
        else
            MFCCodecType = UNKNOWN_TYPE;

        SEC_OSAL_Log(SEC_LOG_TRACE, "codec type = %d", MFCCodecType);

        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            configValue = 0;    // the number that you want to delay
            SsbSipMfcDecSetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &configValue);
        } else {
            configValue = WMV_DEC_NUM_OF_EXTRA_BUFFERS;
            SsbSipMfcDecSetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &configValue);
        }

        bMetaData = Make_Stream_MetaData(pInputData->dataBuffer, &oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
        if (bMetaData == OMX_FALSE) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "Fail to Make Stream MetaData");
            ret = OMX_ErrorMFCInit;
            goto EXIT;
        }

        SsbSipMfcDecSetInBuf(pWmvDec->hMFCWmvHandle.hMFCHandle,
                             pWmvDec->hMFCWmvHandle.pMFCStreamPhyBuffer,
                             pWmvDec->hMFCWmvHandle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pWmvDec->hMFCWmvHandle.returnCodec = SsbSipMfcDecInit(pWmvDec->hMFCWmvHandle.hMFCHandle, MFCCodecType, oneFrameSize);
        if (pWmvDec->hMFCWmvHandle.returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;

            if (SsbSipMfcDecGetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol) != MFC_RET_OK) {
                ret = OMX_ErrorMFCInit;
                SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetConfig failed");
                goto EXIT;
            }

            SEC_OSAL_Log(SEC_LOG_TRACE, "## nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                pSECInputPort->portDefinition.format.video.nFrameWidth, pSECInputPort->portDefinition.format.video.nFrameHeight,
                pSECInputPort->portDefinition.format.video.nStride, pSECInputPort->portDefinition.format.video.nSliceHeight);

            /** Update Frame Size **/
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
               (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_OSAL_Log(SEC_LOG_TRACE, "nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                    pSECInputPort->portDefinition.format.video.nFrameWidth, pSECInputPort->portDefinition.format.video.nFrameHeight,
                    pSECInputPort->portDefinition.format.video.nStride, pSECInputPort->portDefinition.format.video.nSliceHeight);

                SEC_UpdateFrameSize(pOMXComponent);

                /* Send Port Settings changed call back */
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, // The command was completed
                       OMX_DirOutput, // This is the port index
                       0,
                       NULL);
            }

            pWmvDec->hMFCWmvHandle.bConfiguredMFC = OMX_TRUE;

            if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_WMV3) {
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
            } else {

#ifdef WO_START_CODE
/* ASF parser does not send start code on OpenCORE */
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
#else
/* TODO : for comformanc test based on common buffer scheme w/o parser */
                pOutputData->dataLen = 0;
                ret = OMX_ErrorInputDataDecodeYet;
#endif
            }
            goto EXIT;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecInit failed");
            ret = OMX_ErrorMFCInit;    /* OMX_ErrorUndefined */
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pSECComponent->timeStamp[pWmvDec->hMFCWmvHandle.indexTimestamp] = pInputData->timeStamp;
    pSECComponent->nFlags[pWmvDec->hMFCWmvHandle.indexTimestamp] = pInputData->nFlags;

    if ((pWmvDec->hMFCWmvHandle.returnCodec == MFC_RET_OK) && (pVideoDec->bFirstFrame == OMX_FALSE)) {
        SSBSIP_MFC_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc decode done */
        if (pVideoDec->NBDecThread.bDecoderRun == OMX_TRUE) {
            SEC_OSAL_SemaphoreWait(pVideoDec->NBDecThread.hDecFrameEnd);
            pVideoDec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        SEC_OSAL_SleepMillisec(0);
        status = SsbSipMfcDecGetOutBuf(pWmvDec->hMFCWmvHandle.hMFCHandle, &outputInfo);
            bufWidth = (outputInfo.img_width + 15) & (~15);
            bufHeight = (outputInfo.img_height + 15) & (~15);
            FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
            FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

            if ((SsbSipMfcDecGetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
                (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
            } else {
                /* For timestamp correction. if mfc support frametype detect */
                SEC_OSAL_Log(SEC_LOG_TRACE, "disp_pic_frame_type: %d", outputInfo.disp_pic_frame_type);
#ifdef NEED_TIMESTAMP_REORDER
                if (outputInfo.disp_pic_frame_type == MFC_FRAME_TYPE_I_FRAME) {
                    pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
                    pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
                    pWmvDec->hMFCWmvHandle.outputIndexTimestamp = indexTimestamp;
                } else {
                    pOutputData->timeStamp = pSECComponent->timeStamp[pWmvDec->hMFCWmvHandle.outputIndexTimestamp];
                    pOutputData->nFlags = pSECComponent->nFlags[pWmvDec->hMFCWmvHandle.outputIndexTimestamp];
                }
#else
                pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
#endif
            }

            if ((status == MFC_GETOUTBUF_DISPLAY_DECODING) ||
                (status == MFC_GETOUTBUF_DISPLAY_ONLY)) {
                outputDataValid = OMX_TRUE;
                pWmvDec->hMFCWmvHandle.outputIndexTimestamp++;
                pWmvDec->hMFCWmvHandle.outputIndexTimestamp %= MAX_TIMESTAMP;
            }
            if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS)
                outputDataValid = OMX_FALSE;

            if ((status == MFC_GETOUTBUF_DISPLAY_ONLY) ||
                (pSECComponent->getAllDelayBuffer == OMX_TRUE))
                ret = OMX_ErrorInputDataDecodeYet;

            if (status == MFC_GETOUTBUF_DECODING_ONLY) {
                if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
                    ((pSECComponent->bSaveFlagEOS == OMX_TRUE) || (pSECComponent->getAllDelayBuffer == OMX_TRUE))) {
                    pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    pSECComponent->getAllDelayBuffer = OMX_TRUE;
                    ret = OMX_ErrorInputDataDecodeYet;
                } else {
                    ret = OMX_ErrorNone;
                }
                outputDataValid = OMX_FALSE;
            }

#ifdef FULL_FRAME_SEARCH
            if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
                (pSECComponent->bSaveFlagEOS == OMX_TRUE)) {
                pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pSECComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else
#endif
            if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
                pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
                pSECComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
                pSECComponent->getAllDelayBuffer = OMX_FALSE;
                ret = OMX_ErrorNone;
            }
    } else {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        if ((pSECComponent->bSaveFlagEOS == OMX_TRUE) ||
            (pSECComponent->getAllDelayBuffer == OMX_TRUE) ||
            (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
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

#ifdef WO_START_CODE
    if (pInputData->allocSize < oneFrameSize+4) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "Can't attach startcode due to lack of buffer space");
        ret = OMX_ErrorMFCInit;
        goto EXIT;
    }

    bStartCode = Make_Stream_StartCode(pInputData->dataBuffer, oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
    if (bStartCode == OMX_FALSE) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "Fail to Make Stream Start Code");
        ret = OMX_ErrorMFCInit;
        goto EXIT;
    }
#endif

    if (ret == OMX_ErrorInputDataDecodeYet) {
        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
        pVideoDec->indexInputBuffer++;
        pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pWmvDec->hMFCWmvHandle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pWmvDec->hMFCWmvHandle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].bufferSize;
        oneFrameSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize;
        //pInputData->dataLen = oneFrameSize;
        //pInputData->remainDataLen = oneFrameSize;
    }

    SEC_OSAL_Log(SEC_LOG_TRACE, "SsbSipMfcDecExe oneFrameSize = %d", oneFrameSize);

    if ((Check_Stream_PrefixCode(pInputData->dataBuffer, oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        if ((ret != OMX_ErrorInputDataDecodeYet) || (pSECComponent->getAllDelayBuffer == OMX_TRUE)) {
            SsbSipMfcDecSetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pWmvDec->hMFCWmvHandle.indexTimestamp));
            pWmvDec->hMFCWmvHandle.indexTimestamp++;
            pWmvDec->hMFCWmvHandle.indexTimestamp %= MAX_TIMESTAMP;
        }

        SsbSipMfcDecSetInBuf(pWmvDec->hMFCWmvHandle.hMFCHandle,
                             pWmvDec->hMFCWmvHandle.pMFCStreamPhyBuffer,
                             pWmvDec->hMFCWmvHandle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
#ifdef WO_START_CODE
        pVideoDec->NBDecThread.oneFrameSize = oneFrameSize + 4; /* Frame Start Code */
#else
        pVideoDec->NBDecThread.oneFrameSize = oneFrameSize;
#endif
        /* mfc decode start */
        SEC_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameStart);
        pVideoDec->NBDecThread.bDecoderRun = OMX_TRUE;
        pWmvDec->hMFCWmvHandle.returnCodec = MFC_RET_OK;

        SEC_OSAL_SleepMillisec(0);

        pVideoDec->indexInputBuffer++;
        pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pWmvDec->hMFCWmvHandle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pWmvDec->hMFCWmvHandle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].bufferSize;

        if ((pVideoDec->bFirstFrame == OMX_TRUE) &&
            (pSECComponent->bSaveFlagEOS == OMX_TRUE) &&
            (outputDataValid == OMX_FALSE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }

        pVideoDec->bFirstFrame = OMX_FALSE;
    } else {
        if (pSECComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE)
            pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
    }

    /** Fill Output Buffer **/
    if (outputDataValid == OMX_TRUE) {
        SEC_OMX_BASEPORT *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
        SEC_OMX_BASEPORT *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
        void *pOutputBuf = (void *)pOutputData->dataBuffer;
        void *pSrcBuf[3] = {NULL, };
        void *pYUVBuf[3] = {NULL, };
        unsigned int csc_src_color_format, csc_dst_color_format;
        CSC_METHOD csc_method = CSC_METHOD_SW;
        unsigned int cacheable = 1;

        int frameSize = bufWidth * bufHeight;
        int width = outputInfo.img_width;
        int height = outputInfo.img_height;
        int imageSize = outputInfo.img_width * outputInfo.img_height;

        pSrcBuf[0] = outputInfo.YVirAddr;
        pSrcBuf[1] = outputInfo.CVirAddr;

        pYUVBuf[0]  = (unsigned char *)pOutputBuf;
        pYUVBuf[1]  = (unsigned char *)pOutputBuf + imageSize;
        pYUVBuf[2]  = (unsigned char *)pOutputBuf + imageSize + imageSize / 4;
        pOutputData->dataLen = (imageSize * 3) / 2;

#ifdef USE_ANB
        if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
            OMX_U32 stride;
            SEC_OSAL_LockANB(pOutputData->dataBuffer, width, height, pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat, &stride, pYUVBuf);
            width = stride;
            pOutputData->dataLen = sizeof(void *);
        }
#endif
        if ((pVideoDec->bThumbnailMode == OMX_FALSE) &&
            (pSECOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress)) {
            /* if use Post copy address structure */
            SEC_OSAL_Memcpy(pYUVBuf[0], &(outputInfo.YPhyAddr), sizeof(outputInfo.YPhyAddr));
            SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 1), &(outputInfo.CPhyAddr), sizeof(outputInfo.CPhyAddr));
            SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 2), &(outputInfo.YVirAddr), sizeof(outputInfo.YVirAddr));
            SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 3), &(outputInfo.CVirAddr), sizeof(outputInfo.CVirAddr));
            pOutputData->dataLen = (outputInfo.img_width * outputInfo.img_height * 3) / 2;
        } else {
            SEC_OSAL_Log(SEC_LOG_TRACE, "YUV420 out for ThumbnailMode");
#ifdef CONFIG_MFC_FPS
            SEC_OSAL_PerfStart(PERF_ID_CSC);
#endif
            switch (pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat) {
            case OMX_SEC_COLOR_FormatNV12Tiled:
                SEC_OSAL_Memcpy(pOutputBuf, outputInfo.YVirAddr, FrameBufferYSize);
                SEC_OSAL_Memcpy((unsigned char *)pOutputBuf + FrameBufferYSize, outputInfo.CVirAddr, FrameBufferUVSize);
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
            if ((pSECOutputPort->bIsANBEnabled == OMX_TRUE) && (csc_method == CSC_METHOD_HW)) {
                SEC_OSAL_GetPhysANB(pOutputData->dataBuffer, pYUVBuf);
                pSrcBuf[0] = outputInfo.YPhyAddr;
                pSrcBuf[1] = outputInfo.CPhyAddr;
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
            SEC_OSAL_PerfStop(PERF_ID_CSC);
#endif
        }
#ifdef USE_ANB
        if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
            SEC_OSAL_UnlockANB(pOutputData->dataBuffer);
        }
#endif
    } else {
        pOutputData->dataLen = 0;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_Wmv_Decode_Block(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_WMV_HANDLE             *pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     configValue = 0;
    OMX_S32                     returnCodec = 0;
    OMX_BOOL                    bMetaData = OMX_FALSE;
    OMX_BOOL                    bStartCode = OMX_FALSE;
    int                         bufWidth = 0;
    int                         bufHeight = 0;
    OMX_U32                     FrameBufferYSize;
    OMX_U32                     FrameBufferUVSize;

    FunctionIn();

    if (pWmvDec->hMFCWmvHandle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE MFCCodecType;

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_WMV3)
            MFCCodecType = VC1RCV_DEC;
        else if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_VC1)
            MFCCodecType = VC1_DEC;
        else
            MFCCodecType = UNKNOWN_TYPE;

        SEC_OSAL_Log(SEC_LOG_TRACE, "codec type = %d", MFCCodecType);

        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            configValue = 0;    // the number that you want to delay
            SsbSipMfcDecSetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &configValue);
        } else {
            configValue = WMV_DEC_NUM_OF_EXTRA_BUFFERS;
            SsbSipMfcDecSetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &configValue);
        }

        bMetaData = Make_Stream_MetaData(pInputData->dataBuffer, &oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
        if (bMetaData == OMX_FALSE) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "Fail to Make Stream MetaData");
            ret = OMX_ErrorMFCInit;
            goto EXIT;
        }

        returnCodec = SsbSipMfcDecInit(pWmvDec->hMFCWmvHandle.hMFCHandle, MFCCodecType, oneFrameSize);
        if (returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;

            if (SsbSipMfcDecGetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol) != MFC_RET_OK) {
                ret = OMX_ErrorMFCInit;
                SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetConfig failed");
                goto EXIT;
            }

            SEC_OSAL_Log(SEC_LOG_TRACE, "## nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                pSECInputPort->portDefinition.format.video.nFrameWidth, pSECInputPort->portDefinition.format.video.nFrameHeight,
                pSECInputPort->portDefinition.format.video.nStride, pSECInputPort->portDefinition.format.video.nSliceHeight);

            /** Update Frame Size **/
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
               (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_OSAL_Log(SEC_LOG_TRACE, "nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                    pSECInputPort->portDefinition.format.video.nFrameWidth, pSECInputPort->portDefinition.format.video.nFrameHeight,
                    pSECInputPort->portDefinition.format.video.nStride, pSECInputPort->portDefinition.format.video.nSliceHeight);

                SEC_UpdateFrameSize(pOMXComponent);

                /* Send Port Settings changed call back */
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, // The command was completed
                       OMX_DirOutput, // This is the port index
                       0,
                       NULL);
            }

            pWmvDec->hMFCWmvHandle.bConfiguredMFC = OMX_TRUE;

            if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_WMV3) {
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
            } else {

#ifdef WO_START_CODE
/* ASF parser does not send start code on OpenCORE */
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
#else
/* TODO : for comformanc test based on common buffer scheme w/o parser */
                pOutputData->dataLen = 0;
                ret = OMX_ErrorInputDataDecodeYet;
#endif
            }
            goto EXIT;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecInit failed");
            ret = OMX_ErrorMFCInit;    /* OMX_ErrorUndefined */
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

#ifdef WO_START_CODE
    if (pInputData->allocSize < oneFrameSize+4) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "Can't attach startcode due to lack of buffer space");
        ret = OMX_ErrorMFCInit;
        goto EXIT;
    }

    bStartCode = Make_Stream_StartCode(pInputData->dataBuffer, oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
    if (bStartCode == OMX_FALSE) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "Fail to Make Stream Start Code");
        ret = OMX_ErrorMFCInit;
        goto EXIT;
    }
#endif

    SEC_OSAL_Log(SEC_LOG_TRACE, "SsbSipMfcDecExe oneFrameSize = %d", oneFrameSize);

    if (Check_Stream_PrefixCode(pInputData->dataBuffer, pInputData->dataLen, pWmvDec->hMFCWmvHandle.wmvFormat) == OMX_TRUE) {
        pSECComponent->timeStamp[pWmvDec->hMFCWmvHandle.indexTimestamp] = pInputData->timeStamp;
        pSECComponent->nFlags[pWmvDec->hMFCWmvHandle.indexTimestamp] = pInputData->nFlags;
        SsbSipMfcDecSetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pWmvDec->hMFCWmvHandle.indexTimestamp));

#ifdef WO_START_CODE
        returnCodec = SsbSipMfcDecExe(pWmvDec->hMFCWmvHandle.hMFCHandle, oneFrameSize+4); /* Frame Start Code */
#else
        returnCodec = SsbSipMfcDecExe(pWmvDec->hMFCWmvHandle.hMFCHandle, oneFrameSize);
#endif
    } else {
        if (pSECComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE)
            pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;

        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;
        returnCodec = MFC_RET_OK;
        goto EXIT;
    }

    if (returnCodec == MFC_RET_OK) {
        SSBSIP_MFC_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        status = SsbSipMfcDecGetOutBuf(pWmvDec->hMFCWmvHandle.hMFCHandle, &outputInfo);
        bufWidth = (outputInfo.img_width + 15) & (~15);
        bufHeight = (outputInfo.img_height + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

        if (status != MFC_GETOUTBUF_DISPLAY_ONLY) {
            pWmvDec->hMFCWmvHandle.indexTimestamp++;
            pWmvDec->hMFCWmvHandle.indexTimestamp %= MAX_TIMESTAMP;
        }

        if (SsbSipMfcDecGetConfig(pWmvDec->hMFCWmvHandle.hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            /* For timestamp correction. if mfc support frametype detect */
            SEC_OSAL_Log(SEC_LOG_TRACE, "disp_pic_frame_type: %d", outputInfo.disp_pic_frame_type);
#ifdef NEED_TIMESTAMP_REORDER
            if (outputInfo.disp_pic_frame_type == MFC_FRAME_TYPE_I_FRAME) {
                pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
                pWmvDec->hMFCWmvHandle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pSECComponent->timeStamp[pWmvDec->hMFCWmvHandle.outputIndexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[pWmvDec->hMFCWmvHandle.outputIndexTimestamp];
            }
#else
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
#endif
        }

        if ((status == MFC_GETOUTBUF_DISPLAY_DECODING) ||
            (status == MFC_GETOUTBUF_DISPLAY_ONLY)) {
            /** Fill Output Buffer **/
            void *pOutputBuf = (void *)pOutputData->dataBuffer;
            void *pSrcBuf[3] = {NULL, };
            void *pYUVBuf[3] = {NULL, };
            unsigned int csc_src_color_format, csc_dst_color_format;
            CSC_METHOD csc_method = CSC_METHOD_SW;
            unsigned int cacheable = 1;

            int frameSize = bufWidth * bufHeight;
            int width = outputInfo.img_width;
            int height = outputInfo.img_height;
            int imageSize = outputInfo.img_width * outputInfo.img_height;

            pSrcBuf[0] = outputInfo.YVirAddr;
            pSrcBuf[1] = outputInfo.CVirAddr;

            pYUVBuf[0]  = (unsigned char *)pOutputBuf;
            pYUVBuf[1]  = (unsigned char *)pOutputBuf + imageSize;
            pYUVBuf[2]  = (unsigned char *)pOutputBuf + imageSize + imageSize / 4;
            pOutputData->dataLen = (imageSize * 3) / 2;

#ifdef USE_ANB
            if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
                OMX_U32 stride;
                SEC_OSAL_LockANB(pOutputData->dataBuffer, width, height, pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat, &stride, pYUVBuf);
                width = stride;
                pOutputData->dataLen = sizeof(void *);
            }
#endif
            if ((pVideoDec->bThumbnailMode == OMX_FALSE) &&
                (pSECOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress)) {
                SEC_OSAL_Memcpy(pYUVBuf[0], &(outputInfo.YPhyAddr), sizeof(outputInfo.YPhyAddr));
                SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 1), &(outputInfo.CPhyAddr), sizeof(outputInfo.CPhyAddr));
                SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 2), &(outputInfo.YVirAddr), sizeof(outputInfo.YVirAddr));
                SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 3), &(outputInfo.CVirAddr), sizeof(outputInfo.CVirAddr));
                pOutputData->dataLen = (outputInfo.img_width * outputInfo.img_height * 3) / 2;
            } else {
                SEC_OSAL_Log(SEC_LOG_TRACE, "YUV420 out for ThumbnailMode");
#ifdef CONFIG_MFC_FPS
                SEC_OSAL_PerfStart(PERF_ID_CSC);
#endif
                switch (pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat) {
                case OMX_SEC_COLOR_FormatNV12Tiled:
                    SEC_OSAL_Memcpy(pOutputBuf, outputInfo.YVirAddr, FrameBufferYSize);
                    SEC_OSAL_Memcpy((unsigned char *)pOutputBuf + FrameBufferYSize, outputInfo.CVirAddr, FrameBufferUVSize);
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
                if ((pSECOutputPort->bIsANBEnabled == OMX_TRUE) && (csc_method == CSC_METHOD_HW)) {
                    SEC_OSAL_GetPhysANB(pOutputData->dataBuffer, pYUVBuf);
                    pSrcBuf[0] = outputInfo.YPhyAddr;
                    pSrcBuf[1] = outputInfo.CPhyAddr;
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
                SEC_OSAL_PerfStop(PERF_ID_CSC);
#endif
            }

#ifdef USE_ANB
            if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
                SEC_OSAL_UnlockANB(pOutputData->dataBuffer);
            }
#endif
            pWmvDec->hMFCWmvHandle.outputIndexTimestamp++;
            pWmvDec->hMFCWmvHandle.outputIndexTimestamp %= MAX_TIMESTAMP;
        }
        if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS)
            pOutputData->dataLen = 0;

        if ((status == MFC_GETOUTBUF_DISPLAY_ONLY) ||
            (pSECComponent->getAllDelayBuffer == OMX_TRUE))
            ret = OMX_ErrorInputDataDecodeYet;

        if (status == MFC_GETOUTBUF_DECODING_ONLY) {
            if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
                ((pSECComponent->bSaveFlagEOS == OMX_TRUE) || (pSECComponent->getAllDelayBuffer == OMX_TRUE))) {
                pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pSECComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else {
                ret = OMX_ErrorNone;
            }
            goto EXIT;
        }

#ifdef FULL_FRAME_SEARCH
        if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
            (pSECComponent->bSaveFlagEOS == OMX_TRUE)) {
            pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pSECComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else
#endif
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pSECComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            ret = OMX_ErrorNone;
        }
    } else {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        if ((pSECComponent->bSaveFlagEOS == OMX_TRUE) ||
            (pSECComponent->getAllDelayBuffer == OMX_TRUE) ||
            (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
        }
        pOutputData->dataLen = 0;

        /* ret = OMX_ErrorUndefined; */
        ret = OMX_ErrorNone;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Decode */
OMX_ERRORTYPE SEC_MFC_WmvDec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_WMV_HANDLE        *pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SEC_OMX_BASEPORT      *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT      *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    OMX_BOOL              bCheckPrefix = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pSECInputPort)) || (!CHECK_PORT_ENABLED(pSECOutputPort)) ||
        (!CHECK_PORT_POPULATED(pSECInputPort)) || (!CHECK_PORT_POPULATED(pSECOutputPort))) {
        goto EXIT;
    }
    if (OMX_FALSE == SEC_Check_BufferProcess_State(pSECComponent)) {
        goto EXIT;
    }

    pWmvDec->hMFCWmvHandle.wmvFormat = gWvmFormat;

#ifdef NONBLOCK_MODE_PROCESS
    ret = SEC_MFC_Wmv_Decode_Nonblock(pOMXComponent, pInputData, pOutputData);
#else
    ret = SEC_MFC_Wmv_Decode_Block(pOMXComponent, pInputData, pOutputData);
#endif
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataDecodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                    pSECComponent->callbackData,
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

OSCL_EXPORT_REF OMX_ERRORTYPE SEC_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    SEC_WMV_HANDLE        *pWmvDec = NULL;
    OMX_S32               wmvFormat = WMV_FORMAT_UNKNOWN;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "SEC_OMX_ComponentInit: parameters are null, ret:%X", ret);
        goto EXIT;
    }
    if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_WMV_DEC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SEC_OMX_ComponentInit: SEC_OMX_VideoDecodeComponentInit error, ret: %X", ret);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->codecType = HW_VIDEO_DEC_CODEC;

    pSECComponent->componentName = (OMX_STRING)SEC_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pSECComponent->componentName == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "SEC_OMX_ComponentInit: componentName alloc error, ret: %X", ret);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pWmvDec = SEC_OSAL_Malloc(sizeof(SEC_WMV_HANDLE));
    if (pWmvDec == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "SEC_OMX_ComponentInit: SEC_WMV_HANDLE alloc error, ret: %X", ret);
        goto EXIT;
    }
    SEC_OSAL_Memset(pWmvDec, 0, sizeof(SEC_WMV_HANDLE));
    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pWmvDec;
    pWmvDec->hMFCWmvHandle.wmvFormat = wmvFormat;

    SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_WMV_DEC);

    /* Set componentVersion */
    pSECComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pSECComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pSECComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pSECComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pSECComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pSECComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pSECComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pSECComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Android CapabilityFlags */
    pSECComponent->capabilityFlags.iIsOMXComponentMultiThreaded                   = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc  = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsMovableInputBuffers       = OMX_FALSE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsPartialFrames             = OMX_FALSE;
    pSECComponent->capabilityFlags.iOMXComponentUsesNALStartCodes                 = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentCanHandleIncompleteFrames         = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentUsesFullAVCFrames                 = OMX_TRUE;

    /* Input port */
    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pSECPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pSECPort->portDefinition.format.video.nStride = 0;
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;

    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "video/wmv");

    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pSECPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pSECPort->portDefinition.format.video.nStride = 0;
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "raw/video");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pWmvDec->WmvComponent[i], OMX_VIDEO_PARAM_WMVTYPE);
        pWmvDec->WmvComponent[i].nPortIndex = i;
        pWmvDec->WmvComponent[i].eFormat    = OMX_VIDEO_WMVFormat9;
    }

    pOMXComponent->GetParameter      = &SEC_MFC_WmvDec_GetParameter;
    pOMXComponent->SetParameter      = &SEC_MFC_WmvDec_SetParameter;
    pOMXComponent->GetConfig         = &SEC_MFC_WmvDec_GetConfig;
    pOMXComponent->SetConfig         = &SEC_MFC_WmvDec_SetConfig;
    pOMXComponent->GetExtensionIndex = &SEC_MFC_WmvDec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &SEC_MFC_WmvDec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &SEC_OMX_ComponentDeinit;

    pSECComponent->sec_mfc_componentInit      = &SEC_MFC_WmvDec_Init;
    pSECComponent->sec_mfc_componentTerminate = &SEC_MFC_WmvDec_Terminate;
    pSECComponent->sec_mfc_bufferProcess      = &SEC_MFC_WmvDec_bufferProcess;
    pSECComponent->sec_checkInputFrame = &Check_Wmv_Frame;

    pSECComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_WMV_HANDLE        *pWmvDec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;

    pWmvDec = (SEC_WMV_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pWmvDec != NULL) {
        SEC_OSAL_Free(pWmvDec);
        pWmvDec = ((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle = NULL;
    }

    ret = SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
