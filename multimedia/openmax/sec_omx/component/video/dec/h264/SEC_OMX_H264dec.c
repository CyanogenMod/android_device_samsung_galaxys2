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
 * @file        SEC_OMX_H264dec.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
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
#include "library_register.h"
#include "SEC_OMX_H264dec.h"
#include "SsbSipMfcApi.h"

#ifdef USE_ANB
#include "SEC_OSAL_Android.h"
#endif

/* To use CSC_METHOD_PREFER_HW or CSC_METHOD_HW in SEC OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_H264_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

#define H264_DEC_NUM_OF_EXTRA_BUFFERS 7

#ifdef S3D_SUPPORT
#define ADD_SPS_PPS_I_FRAME
#else
//#define ADD_SPS_PPS_I_FRAME
#endif
//#define FULL_FRAME_SEARCH

/* H.264 Decoder Supported Levels & profiles */
SEC_OMX_VIDEO_PROFILELEVEL supportedAVCProfileLevels[] ={
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},

    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},

    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4}};


static int Check_H264_Frame(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    OMX_U32  preFourByte       = (OMX_U32)-1;
    int      accessUnitSize    = 0;
    int      frameTypeBoundary = 0;
    int      nextNaluSize      = 0;
    int      naluStart         = 0;

    if (bPreviousFrameEOF == OMX_TRUE)
        naluStart = 0;
    else
        naluStart = 1;

    while (1) {
        int inputOneByte = 0;

        if (accessUnitSize == (int)buffSize)
            goto EXIT;

        inputOneByte = *(pInputStream++);
        accessUnitSize += 1;

        if (preFourByte == 0x00000001 || (preFourByte << 8) == 0x00000100) {
            int naluType = inputOneByte & 0x1F;

            SEC_OSAL_Log(SEC_LOG_TRACE, "NaluType : %d", naluType);
            if (naluStart == 0) {
#ifdef ADD_SPS_PPS_I_FRAME
                if (naluType == 1 || naluType == 5)
#else
                if (naluType == 1 || naluType == 5 || naluType == 7 || naluType == 8)
#endif
                    naluStart = 1;
            } else {
#ifdef OLD_DETECT
                frameTypeBoundary = (8 - naluType) & (naluType - 10); //AUD(9)
#else
                if (naluType == 9)
                    frameTypeBoundary = -2;
#endif
                if (naluType == 1 || naluType == 5) {
                    if (accessUnitSize == (int)buffSize) {
                        accessUnitSize--;
                        goto EXIT;
                    }
                    inputOneByte = *pInputStream++;
                    accessUnitSize += 1;

                    if (inputOneByte >= 0x80)
                        frameTypeBoundary = -1;
                }
                if (frameTypeBoundary < 0) {
                    break;
                }
            }

        }
        preFourByte = (preFourByte << 8) + inputOneByte;
    }

    *pbEndOfFrame = OMX_TRUE;
    nextNaluSize = -5;
    if (frameTypeBoundary == -1)
        nextNaluSize = -6;
    if (preFourByte != 0x00000001)
        nextNaluSize++;
    return (accessUnitSize + nextNaluSize);

EXIT:
    *pbEndOfFrame = OMX_FALSE;

    return accessUnitSize;
}

OMX_BOOL Check_H264_StartCode(OMX_U8 *pInputStream, OMX_U32 streamSize)
{
    if (streamSize < 4) {
        return OMX_FALSE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] == 0x00) &&
              (pInputStream[3] != 0x00) &&
              ((pInputStream[3] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] != 0x00) &&
              ((pInputStream[2] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else {
        return OMX_FALSE;
    }
}

OMX_ERRORTYPE SEC_MFC_H264Dec_Alloc_SecureInputBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                                      OMX_IN OMX_U32 nBufferSize,
                                                      OMX_INOUT OMX_PTR *pInputBuffer_physicalAddress)
{
    FunctionIn();

    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_PTR pStreamBuffer    = NULL;
    OMX_PTR pStreamPhyBuffer = NULL;

#ifdef USE_DRM
    pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pH264Dec == NULL) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pStreamBuffer = SsbSipMfcDecAllocInputBuffer(pH264Dec->hMFCH264Handle.hMFCHandle, &pStreamPhyBuffer, nBufferSize);
    if (pStreamBuffer == NULL) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pStreamPhyBuffer;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = nBufferSize;

    *pInputBuffer_physicalAddress = pStreamPhyBuffer;
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_Free_SecureInputBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                                     OMX_INOUT OMX_PTR pInputBuffer_physicalAddress)
{
    FunctionIn();

    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_PTR pStreamPhyBuffer = NULL;

#ifdef USE_DRM
    pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pH264Dec == NULL) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pStreamPhyBuffer = pInputBuffer_physicalAddress;
    SsbSipMfcDecFreeInputBuffer(pH264Dec->hMFCH264Handle.hMFCHandle, pStreamPhyBuffer);
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
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

    switch (nParamIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Dec->AVCComponent[pDstAVCComponent->nPortIndex];

        SEC_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_H264_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        SEC_OMX_VIDEO_PROFILELEVEL *pProfileLevel = NULL;
        OMX_U32 maxProfileLevelNum = 0;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pProfileLevel = supportedAVCProfileLevels;
        maxProfileLevelNum = sizeof(supportedAVCProfileLevels) / sizeof(SEC_OMX_VIDEO_PROFILELEVEL);

        if (pDstProfileLevel->nProfileIndex >= maxProfileLevelNum) {
            ret = OMX_ErrorNoMore;
            goto EXIT;
        }

        pProfileLevel += pDstProfileLevel->nProfileIndex;
        pDstProfileLevel->eProfile = pProfileLevel->profile;
        pDstProfileLevel->eLevel = pProfileLevel->level;
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Dec->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE SEC_MFC_H264Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
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
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstAVCComponent = &pH264Dec->AVCComponent[pSrcAVCComponent->nPortIndex];

        SEC_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
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

        if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_H264_DEC_ROLE)) {
            pSECComponent->pSECPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
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
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

        pDstAVCComponent = &pH264Dec->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pH264Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE SEC_MFC_H264Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
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
    case OMX_IndexConfigCommonOutputCrop:
    {
        SEC_H264DEC_HANDLE  *pH264Dec = NULL;
        OMX_CONFIG_RECTTYPE *pSrcRectType = NULL;
        OMX_CONFIG_RECTTYPE *pDstRectType = NULL;
        pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

        if (pH264Dec->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
            ret = OMX_ErrorNotReady;
            break;
        }

        pDstRectType = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;

        if ((pDstRectType->nPortIndex != INPUT_PORT_INDEX) &&
            (pDstRectType->nPortIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        SEC_OMX_BASEPORT *pSECPort = &pSECComponent->pSECPort[pDstRectType->nPortIndex];

        pSrcRectType = &(pSECPort->cropRectangle);

        pDstRectType->nTop = pSrcRectType->nTop;
        pDstRectType->nLeft = pSrcRectType->nLeft;
        pDstRectType->nHeight = pSrcRectType->nHeight;
        pDstRectType->nWidth = pSrcRectType->nWidth;
    }
        break;
    default:
        ret = SEC_OMX_VideoDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_SetConfig(
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
        ret = SEC_OMX_VideoDecodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_GetExtensionIndex(
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
        SEC_H264DEC_HANDLE *pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

        *pIndexType = OMX_IndexVendorThumbnailMode;

        ret = OMX_ErrorNone;
    } else {
        ret = SEC_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_H264_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
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
    SEC_H264DEC_HANDLE    *pH264Dec = (SEC_H264DEC_HANDLE *)pVideoDec->hCodecHandle;

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
            pH264Dec->hMFCH264Handle.returnCodec = SsbSipMfcDecExe(pH264Dec->hMFCH264Handle.hMFCHandle, pVideoDec->NBDecThread.oneFrameSize);
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
OMX_ERRORTYPE SEC_MFC_H264Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT      *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_PTR pStreamBuffer    = NULL;
    OMX_PTR pStreamPhyBuffer = NULL;
    OMX_PTR hMFCHandle;

#ifdef S3D_SUPPORT
    OMX_S32 setConfVal       = 0;
#endif
    CSC_METHOD csc_method = CSC_METHOD_SW;

#ifdef CONFIG_MFC_FPS
    SEC_OSAL_PerfInit(PERF_ID_DEC);
    SEC_OSAL_PerfInit(PERF_ID_CSC);
#endif

    pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    pH264Dec->hMFCH264Handle.bConfiguredMFC = OMX_FALSE;
    pSECComponent->bUseFlagEOF = OMX_FALSE;
    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    /* MFC(Multi Function Codec) decoder and CMM(Codec Memory Management) driver open */
    if (pVideoDec->bDRMPlayerMode == OMX_FALSE) {
        hMFCHandle = NULL;

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
        pH264Dec->hMFCH264Handle.hMFCHandle = hMFCHandle;
    } else {
        hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle;
        pSECComponent->bUseFlagEOF = OMX_TRUE;
    }

#ifdef S3D_SUPPORT
    /*Enable SEI parsing for checking frame_packing S3D*/
    setConfVal = 1;
    SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_SEI_PARSE, &setConfVal);
#endif

    /* Allocate decoder's input buffer */
    /* Get first input buffer */
    if (pVideoDec->bDRMPlayerMode == OMX_FALSE) {
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
            pH264Dec->hMFCH264Handle.returnCodec = MFC_RET_OK;
        }
#endif

        pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[0].VirAddr;
        pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[0].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[0].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[0].bufferSize;
    }

    SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Dec->hMFCH264Handle.indexTimestamp = 0;
    pH264Dec->hMFCH264Handle.outputIndexTimestamp = 0;

    pSECComponent->getAllDelayBuffer = OMX_FALSE;

#ifdef USE_ANB
#if defined(USE_CSC_FIMC) || defined(USE_CSC_GSCALER)
    if (pSECOutputPort->bIsANBEnabled == OMX_TRUE)
        csc_method = CSC_METHOD_PREFER_HW;
#endif
    if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
        csc_method = CSC_METHOD_HW;
    }
#endif
    pVideoDec->csc_handle = csc_init(&csc_method);
    pVideoDec->csc_set_format = OMX_FALSE;

    SEC_OSAL_Log(SEC_LOG_ERROR, "%s::csc_method=%d", __func__, csc_method);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE SEC_MFC_H264Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_PTR                hMFCHandle = NULL;

    FunctionIn();

#ifdef CONFIG_MFC_FPS
    SEC_OSAL_PerfPrint("[DEC]",  PERF_ID_DEC);
    SEC_OSAL_PerfPrint("[CSC]",  PERF_ID_CSC);
#endif

    pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle;

    pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = NULL;
    pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

    if (pVideoDec->bDRMPlayerMode == OMX_FALSE) {
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
    }

    if (pVideoDec->bDRMPlayerMode == OMX_FALSE) {
        if (hMFCHandle != NULL) {
            SsbSipMfcDecClose(hMFCHandle);
            hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle = NULL;
        }
    }

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264_Decode_Nonblock(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_H264DEC_HANDLE         *pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     setConfVal = 0;
    int                         bufWidth = 0;
    int                         bufHeight = 0;
    OMX_U32                     FrameBufferYSize = 0;
    OMX_U32                     FrameBufferUVSize = 0;
    OMX_BOOL                    outputDataValid = OMX_FALSE;
#ifdef S3D_SUPPORT
    SSBSIP_MFC_FRAME_PACKING    frame_packing;
#endif

    FunctionIn();

    if (pH264Dec->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE eCodecType = H264_DEC;

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        /* Default number in the driver is optimized */
        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            setConfVal = 0;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        } else {
            setConfVal = H264_DEC_NUM_OF_EXTRA_BUFFERS;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &setConfVal);

            setConfVal = 8;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        }

        SsbSipMfcDecSetInBuf(pH264Dec->hMFCH264Handle.hMFCHandle,
                             pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer,
                             pH264Dec->hMFCH264Handle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pH264Dec->hMFCH264Handle.returnCodec = SsbSipMfcDecInit(pH264Dec->hMFCH264Handle.hMFCHandle, eCodecType, oneFrameSize);
        if (pH264Dec->hMFCH264Handle.returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;
            SSBSIP_MFC_CROP_INFORMATION cropInfo;

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol);
            SEC_OSAL_Log(SEC_LOG_TRACE, "set width height information : %d, %d",
                            pSECInputPort->portDefinition.format.video.nFrameWidth,
                            pSECInputPort->portDefinition.format.video.nFrameHeight);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc width height information : %d, %d",
                            imgResol.width, imgResol.height);

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_CROP_INFO, &cropInfo);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc crop_top crop_bottom crop_left crop_right :  %d, %d, %d, %d",
                            cropInfo.crop_top_offset , cropInfo.crop_bottom_offset ,
                            cropInfo.crop_left_offset , cropInfo.crop_right_offset);

            pSECOutputPort->cropRectangle.nTop    = cropInfo.crop_top_offset;
            pSECOutputPort->cropRectangle.nLeft   = cropInfo.crop_left_offset;
            pSECOutputPort->cropRectangle.nWidth  = imgResol.width - cropInfo.crop_left_offset - cropInfo.crop_right_offset;
            pSECOutputPort->cropRectangle.nHeight = imgResol.height - cropInfo.crop_top_offset - cropInfo.crop_bottom_offset;

            pH264Dec->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

            /** Update Frame Size **/
            if ((cropInfo.crop_left_offset != 0) || (cropInfo.crop_right_offset != 0) ||
                (cropInfo.crop_top_offset != 0) || (cropInfo.crop_bottom_offset != 0)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send crop info call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       OMX_IndexConfigCommonOutputCrop,
                       NULL);
            }
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
                (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                SEC_OSAL_Log(SEC_LOG_TRACE, "change width height information : OMX_EventPortSettingsChanged");

                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send Port Settings changed call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       0,
                       NULL);
            }

#ifdef ADD_SPS_PPS_I_FRAME
            ret = OMX_ErrorInputDataDecodeYet;
#else
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;

            ret = OMX_ErrorNone;
#endif
            goto EXIT;
        } else {
            ret = OMX_ErrorMFCInit;
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
    pSECComponent->nFlags[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;

    if ((pH264Dec->hMFCH264Handle.returnCodec == MFC_RET_OK) &&
        (pVideoDec->bFirstFrame == OMX_FALSE)) {
        SSBSIP_MFC_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc decode done */
        if (pVideoDec->NBDecThread.bDecoderRun == OMX_TRUE) {
            SEC_OSAL_SemaphoreWait(pVideoDec->NBDecThread.hDecFrameEnd);
            pVideoDec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        SEC_OSAL_SleepMillisec(0);
        status = SsbSipMfcDecGetOutBuf(pH264Dec->hMFCH264Handle.hMFCHandle, &outputInfo);
        bufWidth = (outputInfo.img_width + 15) & (~15);
        bufHeight = (outputInfo.img_height + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

#ifdef S3D_SUPPORT
        /* Check Whether frame packing information is available */
        SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_FRAME_PACKING, &frame_packing);

        if (pVideoDec->bThumbnailMode == OMX_FALSE &&
            frame_packing.available &&
            pH264Dec->hMFCH264Handle.bS3DMode == OMX_FALSE) {

            SEC_OSAL_Log(SEC_LOG_TRACE, "arrangement ID: 0x%08x", frame_packing.arrangement_id);
            SEC_OSAL_Log(SEC_LOG_TRACE, "arrangement_type: %d", frame_packing.arrangement_type);
            SEC_OSAL_Log(SEC_LOG_TRACE, "content_interpretation_type: %d", frame_packing.content_interpretation_type);
            SEC_OSAL_Log(SEC_LOG_TRACE, "current_frame_is_frame0_flag: %d", frame_packing.current_frame_is_frame0_flag);
            SEC_OSAL_Log(SEC_LOG_TRACE, "spatial_flipping_flag: %d", frame_packing.spatial_flipping_flag);
            SEC_OSAL_Log(SEC_LOG_TRACE, "fr0X:%d fr0Y:%d fr0X:%d fr0Y:%d", frame_packing.frame0_grid_pos_x,
                frame_packing.frame0_grid_pos_y, frame_packing.frame1_grid_pos_x, frame_packing.frame1_grid_pos_y);

            /* Change Outport eColorFormat based on Framepacking information*/
            if (frame_packing.arrangement_type == 3) {
                if (frame_packing.content_interpretation_type == 1) {
                    switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
                    case OMX_SEC_COLOR_FormatNV12Tiled:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatNV12Tiled_SBS_LR;
                        break;
                    case OMX_COLOR_FormatYUV420SemiPlanar:
                    case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_LR;
                        break;
                    case OMX_COLOR_FormatYUV420Planar:
                    default:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420Planar_SBS_LR;
                        break;
                    }
                } else if (frame_packing.content_interpretation_type == 2) {
                    switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
                    case OMX_SEC_COLOR_FormatNV12Tiled:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatNV12Tiled_SBS_RL;
                        break;
                    case OMX_COLOR_FormatYUV420SemiPlanar:
                    case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_RL;
                        break;
                    case OMX_COLOR_FormatYUV420Planar:
                    default:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420Planar_SBS_RL;
                        break;
                    }
                }
            } else if (frame_packing.arrangement_type == 4) {
                if (frame_packing.content_interpretation_type == 1) {
                    switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
                    case OMX_SEC_COLOR_FormatNV12Tiled:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatNV12Tiled_TB_LR;
                        break;
                    case OMX_COLOR_FormatYUV420SemiPlanar:
                    case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_LR;
                        break;
                    case OMX_COLOR_FormatYUV420Planar:
                    default:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420Planar_TB_LR;
                        break;
                    }
                } else if (frame_packing.content_interpretation_type == 2) {
                    switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
                    case OMX_SEC_COLOR_FormatNV12Tiled:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatNV12Tiled_TB_RL;
                        break;
                    case OMX_COLOR_FormatYUV420SemiPlanar:
                    case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_RL;
                        break;
                    case OMX_COLOR_FormatYUV420Planar:
                    default:
                        pSECOutputPort->portDefinition.format.video.eColorFormat = OMX_SEC_COLOR_FormatYUV420Planar_TB_RL;
                        break;
                    }
                }
            }

            /** Send Port Settings changed call back - output color format change */
           (*(pSECComponent->pCallbacks->EventHandler))
                  (pOMXComponent,
                   pSECComponent->callbackData,
                   OMX_EventPortSettingsChanged, /* The command was completed */
                   OMX_DirOutput, /* This is the port index */
                   0,
                   NULL);

            if ((pSECOutputPort->cropRectangle.nTop != 0) || (pSECOutputPort->cropRectangle.nLeft != 0)) {
                /** Send crop info call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       OMX_IndexConfigCommonOutputCrop,
                       NULL);
            }
            pH264Dec->hMFCH264Handle.bS3DMode = OMX_TRUE;
            SEC_OSAL_SleepMillisec(0);
            ret = OMX_ErrorInputDataDecodeYet;
            goto EXIT;
        }
#endif

        if ((SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            /* For timestamp correction. if mfc support frametype detect */
            SEC_OSAL_Log(SEC_LOG_TRACE, "disp_pic_frame_type: %d", outputInfo.disp_pic_frame_type);
#ifdef NEED_TIMESTAMP_REORDER
            if ((outputInfo.disp_pic_frame_type == MFC_FRAME_TYPE_I_FRAME) ||
                (pH264Dec->hMFCH264Handle.bFlashPlayerMode == OMX_TRUE)) {
                pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
                pH264Dec->hMFCH264Handle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.outputIndexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[pH264Dec->hMFCH264Handle.outputIndexTimestamp];
            }
#else
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
#endif
            SEC_OSAL_Log(SEC_LOG_TRACE, "timestamp %lld us (%.2f secs)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);
        }

        if ((status == MFC_GETOUTBUF_DISPLAY_DECODING) ||
            (status == MFC_GETOUTBUF_DISPLAY_ONLY)) {
            outputDataValid = OMX_TRUE;
            pH264Dec->hMFCH264Handle.outputIndexTimestamp++;
            pH264Dec->hMFCH264Handle.outputIndexTimestamp %= MAX_TIMESTAMP;
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

    if (ret == OMX_ErrorInputDataDecodeYet) {
        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
        pVideoDec->indexInputBuffer++;
        pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].bufferSize;
        oneFrameSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize;
        //pInputData->dataLen = oneFrameSize;
        //pInputData->remainDataLen = oneFrameSize;
    }

    if ((Check_H264_StartCode(pInputData->dataBuffer, oneFrameSize) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        if ((ret != OMX_ErrorInputDataDecodeYet) || (pSECComponent->getAllDelayBuffer == OMX_TRUE)) {
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pH264Dec->hMFCH264Handle.indexTimestamp));
            pH264Dec->hMFCH264Handle.indexTimestamp++;
            pH264Dec->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;
        }

        SsbSipMfcDecSetInBuf(pH264Dec->hMFCH264Handle.hMFCHandle,
                             pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer,
                             pH264Dec->hMFCH264Handle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
        pVideoDec->NBDecThread.oneFrameSize = oneFrameSize;

        /* mfc decode start */
        SEC_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameStart);
        pVideoDec->NBDecThread.bDecoderRun = OMX_TRUE;
        pH264Dec->hMFCH264Handle.returnCodec = MFC_RET_OK;

        SEC_OSAL_SleepMillisec(0);

        pVideoDec->indexInputBuffer++;
        pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
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
        void *pOutputBuf = (void *)pOutputData->dataBuffer;
        void *pSrcBuf[3] = {NULL, };
        void *pYUVBuf[3] = {NULL, };
        unsigned int csc_src_color_format, csc_dst_color_format;
        CSC_METHOD csc_method = CSC_METHOD_SW;
        unsigned int cacheable = 1;

        int frameSize = bufWidth * bufHeight;
        int actualWidth = outputInfo.img_width;
        int actualHeight = outputInfo.img_height;
        int actualImageSize = actualWidth * actualHeight;

        pSrcBuf[0] = outputInfo.YVirAddr;
        pSrcBuf[1] = outputInfo.CVirAddr;

        pYUVBuf[0]  = (unsigned char *)pOutputBuf;
        pYUVBuf[1]  = (unsigned char *)pOutputBuf + actualImageSize;
        pYUVBuf[2]  = (unsigned char *)pOutputBuf + actualImageSize + actualImageSize / 4;
        pOutputData->dataLen = (actualImageSize * 3) / 2;

#ifdef USE_ANB
        if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
            OMX_U32 stride;
            SEC_OSAL_LockANB(pOutputData->dataBuffer, actualWidth, actualHeight, pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat, &stride, pYUVBuf);
            actualWidth = stride;
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
            pOutputData->dataLen = (actualWidth * actualHeight * 3) / 2;
        } else {
            SEC_OSAL_Log(SEC_LOG_TRACE, "YUV420p out for ThumbnailMode/Flash player mode");
#ifdef CONFIG_MFC_FPS
            SEC_OSAL_PerfStart(PERF_ID_CSC);
#endif
            switch (pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat) {
            case OMX_SEC_COLOR_FormatNV12Tiled:
#ifdef S3D_SUPPORT
            case OMX_SEC_COLOR_FormatNV12Tiled_SBS_LR:
            case OMX_SEC_COLOR_FormatNV12Tiled_SBS_RL:
            case OMX_SEC_COLOR_FormatNV12Tiled_TB_LR:
            case OMX_SEC_COLOR_FormatNV12Tiled_TB_RL:
#endif
                SEC_OSAL_Memcpy(pOutputBuf, outputInfo.YVirAddr, FrameBufferYSize);
                SEC_OSAL_Memcpy((unsigned char *)pOutputBuf + FrameBufferYSize, outputInfo.CVirAddr, FrameBufferUVSize);
                pOutputData->dataLen = FrameBufferYSize + FrameBufferUVSize;
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
#ifdef S3D_SUPPORT
            case OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_LR:
            case OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_RL:
            case OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_LR:
            case OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_RL:
#endif
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_SEC_COLOR_FormatNV12Tiled);
                csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
                break;
            case OMX_COLOR_FormatYUV420Planar:
#ifdef S3D_SUPPORT
            case OMX_SEC_COLOR_FormatYUV420Planar_SBS_LR:
            case OMX_SEC_COLOR_FormatYUV420Planar_SBS_RL:
            case OMX_SEC_COLOR_FormatYUV420Planar_TB_LR:
            case OMX_SEC_COLOR_FormatYUV420Planar_TB_RL:
#endif
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
                    actualWidth,            /* width */
                    actualHeight,           /* height */
                    0,                      /* crop_left */
                    0,                      /* crop_right */
                    actualWidth,            /* crop_width */
                    actualHeight,           /* crop_height */
                    csc_src_color_format,   /* color_format */
                    cacheable);             /* cacheable */
                csc_set_dst_format(
                    pVideoDec->csc_handle,  /* handle */
                    actualWidth,            /* width */
                    actualHeight,           /* height */
                    0,                      /* crop_left */
                    0,                      /* crop_right */
                    actualWidth,            /* crop_width */
                    actualHeight,           /* crop_height */
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
                pVideoDec->csc_handle,  /* handle */
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

OMX_ERRORTYPE SEC_MFC_H264_Decode_Block(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_H264DEC_HANDLE         *pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     setConfVal = 0;
    OMX_S32                     returnCodec = 0;
    int                         bufWidth = 0;
    int                         bufHeight = 0;
    OMX_U32                     FrameBufferYSize;
    OMX_U32                     FrameBufferUVSize;

    FunctionIn();

    if (pH264Dec->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE eCodecType = H264_DEC;

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        /* Default number in the driver is optimized */
        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            setConfVal = 0;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        } else {
            setConfVal = H264_DEC_NUM_OF_EXTRA_BUFFERS;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &setConfVal);

            setConfVal = 8;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        }

        returnCodec = SsbSipMfcDecInit(pH264Dec->hMFCH264Handle.hMFCHandle, eCodecType, oneFrameSize);
        if (returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;
            SSBSIP_MFC_CROP_INFORMATION cropInfo;

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol);
            SEC_OSAL_Log(SEC_LOG_TRACE, "set width height information : %d, %d",
                            pSECInputPort->portDefinition.format.video.nFrameWidth,
                            pSECInputPort->portDefinition.format.video.nFrameHeight);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc width height information : %d, %d",
                            imgResol.width, imgResol.height);

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_CROP_INFO, &cropInfo);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc crop_top crop_bottom crop_left crop_right :  %d, %d, %d, %d",
                            cropInfo.crop_top_offset , cropInfo.crop_bottom_offset ,
                            cropInfo.crop_left_offset , cropInfo.crop_right_offset);

            pSECOutputPort->cropRectangle.nTop    = cropInfo.crop_top_offset;
            pSECOutputPort->cropRectangle.nLeft   = cropInfo.crop_left_offset;
            pSECOutputPort->cropRectangle.nWidth  = imgResol.width - cropInfo.crop_left_offset - cropInfo.crop_right_offset;
            pSECOutputPort->cropRectangle.nHeight = imgResol.height - cropInfo.crop_top_offset - cropInfo.crop_bottom_offset;

            pH264Dec->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

            /** Update Frame Size **/
            if ((cropInfo.crop_left_offset != 0) || (cropInfo.crop_right_offset != 0) ||
                (cropInfo.crop_top_offset != 0) || (cropInfo.crop_bottom_offset != 0)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send crop info call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       OMX_IndexConfigCommonOutputCrop,
                       NULL);
            }
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
                (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                SEC_OSAL_Log(SEC_LOG_TRACE, "change width height information : OMX_EventPortSettingsChanged");
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send Port Settings changed call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       0,
                       NULL);
            }

#ifdef ADD_SPS_PPS_I_FRAME
            ret = OMX_ErrorInputDataDecodeYet;
#else
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;

            ret = OMX_ErrorNone;
#endif
            goto EXIT;
        } else {
            ret = OMX_ErrorMFCInit;
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

    if (Check_H264_StartCode(pInputData->dataBuffer, pInputData->dataLen) == OMX_TRUE) {
        pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
        pSECComponent->nFlags[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;
        SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pH264Dec->hMFCH264Handle.indexTimestamp));

        returnCodec = SsbSipMfcDecExe(pH264Dec->hMFCH264Handle.hMFCHandle, oneFrameSize);
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

        status = SsbSipMfcDecGetOutBuf(pH264Dec->hMFCH264Handle.hMFCHandle, &outputInfo);
        bufWidth =    (outputInfo.img_width + 15) & (~15);
        bufHeight =  (outputInfo.img_height + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

        if (status != MFC_GETOUTBUF_DISPLAY_ONLY) {
            pH264Dec->hMFCH264Handle.indexTimestamp++;
            pH264Dec->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;
        }

        if ((SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            /* For timestamp correction. if mfc support frametype detect */
            SEC_OSAL_Log(SEC_LOG_TRACE, "disp_pic_frame_type: %d", outputInfo.disp_pic_frame_type);
#ifdef NEED_TIMESTAMP_REORDER
            if ((outputInfo.disp_pic_frame_type == MFC_FRAME_TYPE_I_FRAME) ||
                (pH264Dec->hMFCH264Handle.bFlashPlayerMode != OMX_FALSE)) {
                pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
                pH264Dec->hMFCH264Handle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.outputIndexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[pH264Dec->hMFCH264Handle.outputIndexTimestamp];
            }
#else
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
#endif
            SEC_OSAL_Log(SEC_LOG_TRACE, "timestamp %lld us (%.2f secs)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);
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
            int actualWidth = outputInfo.img_width;
            int actualHeight = outputInfo.img_height;
            int actualImageSize = actualWidth * actualHeight;

            pSrcBuf[0] = outputInfo.YVirAddr;
            pSrcBuf[1] = outputInfo.CVirAddr;

            pYUVBuf[0]  = (unsigned char *)pOutputBuf;
            pYUVBuf[1]  = (unsigned char *)pOutputBuf + actualImageSize;
            pYUVBuf[2]  = (unsigned char *)pOutputBuf + actualImageSize + actualImageSize / 4;
            pOutputData->dataLen = (actualImageSize * 3) / 2;

#ifdef USE_ANB
            if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
                OMX_U32 stride;
                SEC_OSAL_LockANB(pOutputData->dataBuffer, actualWidth, actualHeight, pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat, &stride, pYUVBuf);
                actualWidth = stride;
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
                pOutputData->dataLen = (actualWidth * actualHeight * 3) / 2;
            } else {
                SEC_OSAL_Log(SEC_LOG_TRACE, "YUV420p out for ThumbnailMode/Flash player mode");
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
                        actualWidth,            /* width */
                        actualHeight,           /* height */
                        0,                      /* crop_left */
                        0,                      /* crop_right */
                        actualWidth,            /* crop_width */
                        actualHeight,           /* crop_height */
                        csc_src_color_format,   /* color_format */
                        cacheable);             /* cacheable */
                    csc_set_dst_format(
                        pVideoDec->csc_handle,  /* handle */
                        actualWidth,            /* width */
                        actualHeight,           /* height */
                        0,                      /* crop_left */
                        0,                      /* crop_right */
                        actualWidth,            /* crop_width */
                        actualHeight,           /* crop_height */
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
                    pVideoDec->csc_handle,  /* handle */
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
            pH264Dec->hMFCH264Handle.outputIndexTimestamp++;
            pH264Dec->hMFCH264Handle.outputIndexTimestamp %= MAX_TIMESTAMP;
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

OMX_ERRORTYPE SEC_MFC_H264_Decode_DRM(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_H264DEC_HANDLE         *pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     setConfVal = 0;
    OMX_S32                     returnCodec = 0;
    int                         bufWidth = 0;
    int                         bufHeight = 0;
    OMX_U32                     FrameBufferYSize;
    OMX_U32                     FrameBufferUVSize;
    // DRM
    OMX_PTR                     pInputPhysBuffer;
    OMX_PTR                     pInputVirtBuffer;

    FunctionIn();

    if (pH264Dec->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE eCodecType = H264_DEC;

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        /* Set the number of extra buffer to prevent tearing */
        /* DRM Play mode is returned only PhysicalAddress */
        setConfVal = H264_DEC_NUM_OF_EXTRA_BUFFERS;
        SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &setConfVal);

        /* Default number in the driver is optimized */
        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            setConfVal = 0;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        } else {
            setConfVal = 8;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        }

        pInputPhysBuffer = pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer;
        pInputVirtBuffer = NULL; /* ??????????? */

        SEC_OSAL_Log(SEC_LOG_ERROR, "pInputPhysBuffer = 0x%x, allocSize = 0x%0x", pInputPhysBuffer, pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        SsbSipMfcDecSetInBuf(pH264Dec->hMFCH264Handle.hMFCHandle,
                             pInputPhysBuffer, pInputVirtBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        returnCodec = SsbSipMfcDecInit(pH264Dec->hMFCH264Handle.hMFCHandle, eCodecType, oneFrameSize);
        if (returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;
            SSBSIP_MFC_CROP_INFORMATION cropInfo;

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol);
            SEC_OSAL_Log(SEC_LOG_TRACE, "set width height information : %d, %d",
                            pSECInputPort->portDefinition.format.video.nFrameWidth,
                            pSECInputPort->portDefinition.format.video.nFrameHeight);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc width height information : %d, %d",
                            imgResol.width, imgResol.height);

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_CROP_INFO, &cropInfo);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc crop_top crop_bottom crop_left crop_right :  %d, %d, %d, %d",
                            cropInfo.crop_top_offset , cropInfo.crop_bottom_offset ,
                            cropInfo.crop_left_offset , cropInfo.crop_right_offset);

            pSECOutputPort->cropRectangle.nTop    = cropInfo.crop_top_offset;
            pSECOutputPort->cropRectangle.nLeft   = cropInfo.crop_left_offset;
            pSECOutputPort->cropRectangle.nWidth  = imgResol.width - cropInfo.crop_left_offset - cropInfo.crop_right_offset;
            pSECOutputPort->cropRectangle.nHeight = imgResol.height - cropInfo.crop_top_offset - cropInfo.crop_bottom_offset;

            pH264Dec->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

            /** Update Frame Size **/
            if ((cropInfo.crop_left_offset != 0) || (cropInfo.crop_right_offset != 0) ||
                (cropInfo.crop_top_offset != 0) || (cropInfo.crop_bottom_offset != 0)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send crop info call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       OMX_IndexConfigCommonOutputCrop,
                       NULL);
            }
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
                (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                SEC_OSAL_Log(SEC_LOG_TRACE, "change width height information : OMX_EventPortSettingsChanged");
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send Port Settings changed call back **/
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       0,
                       NULL);
            }

#ifdef ADD_SPS_PPS_I_FRAME
            ret = OMX_ErrorInputDataDecodeYet;
#else
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;

            ret = OMX_ErrorNone;
#endif
            goto EXIT;
        } else {
            ret = OMX_ErrorMFCInit;
            goto EXIT;
        }
    }

    if ((pInputData->dataBuffer != NULL) && (pInputData->dataLen != 0)) {
        pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
        pSECComponent->nFlags[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;
        SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pH264Dec->hMFCH264Handle.indexTimestamp));
        if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
            pInputPhysBuffer = pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer;
            pInputVirtBuffer = NULL; /* ??????????? */

            SsbSipMfcDecSetInBuf(pH264Dec->hMFCH264Handle.hMFCHandle,
                                 pInputPhysBuffer, pInputVirtBuffer,
                                 pSECComponent->processData[INPUT_PORT_INDEX].allocSize);
        }
        returnCodec = SsbSipMfcDecExe(pH264Dec->hMFCH264Handle.hMFCHandle, oneFrameSize);
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

        status = SsbSipMfcDecGetOutBuf(pH264Dec->hMFCH264Handle.hMFCHandle, &outputInfo);
        bufWidth =    (outputInfo.img_width + 15) & (~15);
        bufHeight =  (outputInfo.img_height + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

        if (status != MFC_GETOUTBUF_DISPLAY_ONLY) {
            pH264Dec->hMFCH264Handle.indexTimestamp++;
            pH264Dec->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;
        }

        if ((SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            /* For timestamp correction. if mfc support frametype detect */
            SEC_OSAL_Log(SEC_LOG_TRACE, "disp_pic_frame_type: %d", outputInfo.disp_pic_frame_type);
#ifdef NEED_TIMESTAMP_REORDER
            if ((outputInfo.disp_pic_frame_type == MFC_FRAME_TYPE_I_FRAME) ||
                (pH264Dec->hMFCH264Handle.bFlashPlayerMode != OMX_FALSE)) {
                pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
                pH264Dec->hMFCH264Handle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.outputIndexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[pH264Dec->hMFCH264Handle.outputIndexTimestamp];
            }
#else
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
#endif
            SEC_OSAL_Log(SEC_LOG_TRACE, "timestamp %lld us (%.2f secs)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);
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
            int actualWidth = outputInfo.img_width;
            int actualHeight = outputInfo.img_height;
            int actualImageSize = actualWidth * actualHeight;

            pSrcBuf[0] = outputInfo.YVirAddr;
            pSrcBuf[1] = outputInfo.CVirAddr;

            pYUVBuf[0]  = (unsigned char *)pOutputBuf;
            pYUVBuf[1]  = (unsigned char *)pOutputBuf + actualImageSize;
            pYUVBuf[2]  = (unsigned char *)pOutputBuf + actualImageSize + actualImageSize / 4;
            pOutputData->dataLen = (actualImageSize * 3) / 2;

#ifdef USE_ANB
            if (pSECOutputPort->bIsANBEnabled == OMX_TRUE) {
                OMX_U32 stride;
                SEC_OSAL_LockANB(pOutputData->dataBuffer, actualWidth, actualHeight, pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat, &stride, pYUVBuf);
                actualWidth = stride;
                pOutputData->dataLen = sizeof(void *);
            }
#endif
            if (pSECOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress) {
                /* if use Post copy address structure */
                SEC_OSAL_Log(SEC_LOG_TRACE, "DRM] physical address data mode");
                SEC_OSAL_Memcpy(pYUVBuf[0], &(outputInfo.YPhyAddr), sizeof(outputInfo.YPhyAddr));
                SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 1), &(outputInfo.CPhyAddr), sizeof(outputInfo.CPhyAddr));
                SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 2), &(outputInfo.YVirAddr), sizeof(outputInfo.YVirAddr));
                SEC_OSAL_Memcpy((unsigned char *)pYUVBuf[0] + (sizeof(void *) * 3), &(outputInfo.CVirAddr), sizeof(outputInfo.CVirAddr));
                pOutputData->dataLen = (actualWidth * actualHeight * 3) / 2;
            } else {
                SEC_OSAL_Log(SEC_LOG_TRACE, "DRM] Real data method");
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
                        actualWidth,            /* width */
                        actualHeight,           /* height */
                        0,                      /* crop_left */
                        0,                      /* crop_right */
                        actualWidth,            /* crop_width */
                        actualHeight,           /* crop_height */
                        csc_src_color_format,   /* color_format */
                        cacheable);             /* cacheable */
                    csc_set_dst_format(
                        pVideoDec->csc_handle,  /* handle */
                        actualWidth,            /* width */
                        actualHeight,           /* height */
                        0,                      /* crop_left */
                        0,                      /* crop_right */
                        actualWidth,            /* crop_width */
                        actualHeight,           /* crop_height */
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
                    pVideoDec->csc_handle,  /* handle */
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
            if (pSECOutputPort->bIsANBEnabled == OMX_TRUE)
                SEC_OSAL_UnlockANB(pOutputData->dataBuffer);
#endif
            pH264Dec->hMFCH264Handle.outputIndexTimestamp++;
            pH264Dec->hMFCH264Handle.outputIndexTimestamp %= MAX_TIMESTAMP;
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
OMX_ERRORTYPE SEC_MFC_H264Dec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT   *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_H264DEC_HANDLE      *pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SEC_OMX_BASEPORT        *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT        *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 endOfFrame = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pSECInputPort)) || (!CHECK_PORT_ENABLED(pSECOutputPort)) ||
        (!CHECK_PORT_POPULATED(pSECInputPort)) || (!CHECK_PORT_POPULATED(pSECOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == SEC_Check_BufferProcess_State(pSECComponent)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pVideoDec->bDRMPlayerMode == OMX_FALSE) {
#ifdef NONBLOCK_MODE_PROCESS
        ret = SEC_MFC_H264_Decode_Nonblock(pOMXComponent, pInputData, pOutputData);
#else
        ret = SEC_MFC_H264_Decode_Block(pOMXComponent, pInputData, pOutputData);
#endif
    } else {
        ret = SEC_MFC_H264_Decode_DRM(pOMXComponent, pInputData, pOutputData);
    }

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
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    SEC_OMX_BASEPORT        *pSECPort = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    SEC_H264DEC_HANDLE      *pH264Dec = NULL;
    OMX_BOOL                 bDRMPlayerMode = OMX_FALSE;
    OMX_BOOL                 bFlashPlayerMode = OMX_FALSE;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_H264_DEC, componentName) == 0) {
        bDRMPlayerMode = OMX_FALSE;
        bFlashPlayerMode = OMX_FALSE;
    } else if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_H264_DRM_DEC, componentName) == 0) {
        bDRMPlayerMode = OMX_TRUE;
        bFlashPlayerMode = OMX_FALSE;
    } else if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_H264_FP_DEC, componentName) == 0) {
        bDRMPlayerMode = OMX_FALSE;
        bFlashPlayerMode = OMX_TRUE;
    } else {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->codecType = HW_VIDEO_DEC_CODEC;

    pSECComponent->componentName = (OMX_STRING)SEC_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pSECComponent->componentName == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Dec = SEC_OSAL_Malloc(sizeof(SEC_H264DEC_HANDLE));
    if (pH264Dec == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pH264Dec, 0, sizeof(SEC_H264DEC_HANDLE));
    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pH264Dec;

    if (bDRMPlayerMode == OMX_TRUE)
        SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_H264_DRM_DEC);
    else if (bFlashPlayerMode == OMX_TRUE)
        SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_H264_FP_DEC);
    else
        SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_H264_DEC);

    pVideoDec->bDRMPlayerMode = bDRMPlayerMode;
    pH264Dec->hMFCH264Handle.bFlashPlayerMode = bFlashPlayerMode;
#ifdef S3D_SUPPORT
    pH264Dec->hMFCH264Handle.bS3DMode = OMX_FALSE;
#endif

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
    pSECPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "video/avc");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;
    if (bFlashPlayerMode != OMX_FALSE) {
        pSECPort->portDefinition.nBufferCountActual = MAX_H264_FP_VIDEO_INPUTBUFFER_NUM;
        pSECPort->portDefinition.nBufferCountMin = MAX_H264_FP_VIDEO_INPUTBUFFER_NUM;
    }

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pSECPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pSECPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "raw/video");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;
    if (bFlashPlayerMode != OMX_FALSE) {
        pSECPort->portDefinition.nBufferCountActual = MAX_H264_FP_VIDEO_OUTPUTBUFFER_NUM;
        pSECPort->portDefinition.nBufferCountMin = MAX_H264_FP_VIDEO_OUTPUTBUFFER_NUM;
    }

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Dec->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Dec->AVCComponent[i].nPortIndex = i;
        pH264Dec->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Dec->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel4;
    }

    pOMXComponent->GetParameter      = &SEC_MFC_H264Dec_GetParameter;
    pOMXComponent->SetParameter      = &SEC_MFC_H264Dec_SetParameter;
    pOMXComponent->GetConfig         = &SEC_MFC_H264Dec_GetConfig;
    pOMXComponent->SetConfig         = &SEC_MFC_H264Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &SEC_MFC_H264Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &SEC_MFC_H264Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &SEC_OMX_ComponentDeinit;

    pSECComponent->sec_mfc_componentInit      = &SEC_MFC_H264Dec_Init;
    pSECComponent->sec_mfc_componentTerminate = &SEC_MFC_H264Dec_Terminate;
    pSECComponent->sec_mfc_bufferProcess      = &SEC_MFC_H264Dec_bufferProcess;
    pSECComponent->sec_checkInputFrame        = &Check_H264_Frame;

    pSECComponent->sec_allocSecureInputBuffer = &SEC_MFC_H264Dec_Alloc_SecureInputBuffer;
    pSECComponent->sec_freeSecureInputBuffer  = &SEC_MFC_H264Dec_Free_SecureInputBuffer;

    if (bDRMPlayerMode == OMX_TRUE) {
        OMX_PTR hMFCHandle = NULL;
        SEC_OSAL_Log(SEC_LOG_ERROR, "DRM--SsbSipMfcDecOpen, Line:%d", __LINE__);
        hMFCHandle = (OMX_PTR)SsbSipMfcDecOpen();
        if (hMFCHandle == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        pH264Dec->hMFCH264Handle.hMFCHandle = hMFCHandle;
    }

    pSECComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    SEC_H264DEC_HANDLE      *pH264Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;

    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;

    pH264Dec = (SEC_H264DEC_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pH264Dec != NULL) {
        OMX_PTR hMFCHandle = NULL;
        hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle;
        if ((hMFCHandle != NULL) && (pVideoDec->bDRMPlayerMode == OMX_TRUE)) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "DRM--SsbSipMfcDecClose, Line:%d", __LINE__);
            SsbSipMfcDecClose(hMFCHandle);
            hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle = NULL;
        }
        SEC_OSAL_Free(pH264Dec);
        pH264Dec = ((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle = NULL;
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
