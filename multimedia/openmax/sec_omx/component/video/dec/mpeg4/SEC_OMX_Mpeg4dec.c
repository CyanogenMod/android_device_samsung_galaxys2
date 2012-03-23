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
 * @file      SEC_OMX_Mpeg4dec.c
 * @brief
 * @author    Yunji Kim (yunji.kim@samsung.com)
 * @version   1.1.0
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
#include "SEC_OMX_Mpeg4dec.h"
#include "SsbSipMfcApi.h"

#ifdef USE_ANB
#include "SEC_OSAL_Android.h"
#endif

/* To use CSC_METHOD_PREFER_HW or CSC_METHOD_HW in SEC OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_MPEG4_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

#define MPEG4_DEC_NUM_OF_EXTRA_BUFFERS 7

//#define FULL_FRAME_SEARCH

/* MPEG4 Decoder Supported Levels & profiles */
SEC_OMX_VIDEO_PROFILELEVEL supportedMPEG4ProfileLevels[] ={
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5}};

/* H.263 Decoder Supported Levels & profiles */
SEC_OMX_VIDEO_PROFILELEVEL supportedH263ProfileLevels[] = {
    /* Baseline (Profile 0) */
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
    /* Profile 1 */
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level70},
    /* Profile 2 */
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level70},
    /* Profile 3, restricted up to SD resolution */
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level70}};

static OMX_HANDLETYPE ghMFCHandle = NULL;
static OMX_BOOL gbFIMV1 = OMX_FALSE;

static int Check_Mpeg4_Frame(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    OMX_U32 len;
    int readStream;
    unsigned startCode;
    OMX_BOOL bFrameStart;

    len = 0;
    bFrameStart = OMX_FALSE;

    if (flag & OMX_BUFFERFLAG_CODECCONFIG) {
        if (*pInputStream == 0x03) { /* FIMV1 */
            if (ghMFCHandle != NULL) {
                BitmapInfoHhr *pInfoHeader;
                SSBSIP_MFC_IMG_RESOLUTION imgResolution;

                pInfoHeader = (BitmapInfoHhr *)(pInputStream + 1);
                imgResolution.width = pInfoHeader->BiWidth;
                imgResolution.height = pInfoHeader->BiHeight;
                SsbSipMfcDecSetConfig(ghMFCHandle, MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT, &imgResolution);

                SEC_OSAL_Log(SEC_LOG_TRACE, "width(%d), height(%d)", imgResolution.width, imgResolution.height);
                gbFIMV1 = OMX_TRUE;
                *pbEndOfFrame = OMX_TRUE;
                return buffSize;
            }
        }
    }

    if (gbFIMV1) {
        *pbEndOfFrame = OMX_TRUE;
        return buffSize;
    }

    if (bPreviousFrameEOF == OMX_FALSE)
        bFrameStart = OMX_TRUE;

    startCode = 0xFFFFFFFF;
    if (bFrameStart == OMX_FALSE) {
        /* find VOP start code */
        while(startCode != 0x1B6) {
            readStream = *(pInputStream + len);
            startCode = (startCode << 8) | readStream;
            len++;
            if (len > buffSize)
                goto EXIT;
        }
    }

    /* find next VOP start code */
    startCode = 0xFFFFFFFF;
    while ((startCode != 0x1B6)) {
        readStream = *(pInputStream + len);
        startCode = (startCode << 8) | readStream;
        len++;
        if (len > buffSize)
            goto EXIT;
    }

    *pbEndOfFrame = OMX_TRUE;

    SEC_OSAL_Log(SEC_LOG_TRACE, "1. Check_Mpeg4_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 4, buffSize);

    return len - 4;

EXIT :
    *pbEndOfFrame = OMX_FALSE;

    SEC_OSAL_Log(SEC_LOG_TRACE, "2. Check_Mpeg4_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 1, buffSize);

    return --len;
}

static int Check_H263_Frame(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    OMX_U32 len;
    int readStream;
    unsigned startCode;
    OMX_BOOL bFrameStart = 0;
    unsigned pTypeMask = 0x03;
    unsigned pType = 0;

    len = 0;
    bFrameStart = OMX_FALSE;

    if (bPreviousFrameEOF == OMX_FALSE)
        bFrameStart = OMX_TRUE;

    startCode = 0xFFFFFFFF;
    if (bFrameStart == OMX_FALSE) {
        /* find PSC(Picture Start Code) : 0000 0000 0000 0000 1000 00 */
        while (((startCode << 8 >> 10) != 0x20) || (pType != 0x02)) {
            readStream = *(pInputStream + len);
            startCode = (startCode << 8) | readStream;

            readStream = *(pInputStream + len + 1);
            pType = readStream & pTypeMask;

            len++;
            if (len > buffSize)
                goto EXIT;
        }
    }

    /* find next PSC */
    startCode = 0xFFFFFFFF;
    pType = 0;
    while (((startCode << 8 >> 10) != 0x20) || (pType != 0x02)) {
        readStream = *(pInputStream + len);
        startCode = (startCode << 8) | readStream;

        readStream = *(pInputStream + len + 1);
        pType = readStream & pTypeMask;

        len++;
        if (len > buffSize)
            goto EXIT;
    }

    *pbEndOfFrame = OMX_TRUE;

    SEC_OSAL_Log(SEC_LOG_TRACE, "1. Check_H263_Frame returned EOF = %d, len = %d, iBuffSize = %d", *pbEndOfFrame, len - 3, buffSize);

    return len - 3;

EXIT :

    *pbEndOfFrame = OMX_FALSE;

    SEC_OSAL_Log(SEC_LOG_TRACE, "2. Check_H263_Frame returned EOF = %d, len = %d, iBuffSize = %d", *pbEndOfFrame, len - 1, buffSize);

    return --len;
}

OMX_BOOL Check_Stream_PrefixCode(OMX_U8 *pInputStream, OMX_U32 streamSize, CODEC_TYPE codecType)
{
    switch (codecType) {
    case CODEC_TYPE_MPEG4:
        if (gbFIMV1) {
            return OMX_TRUE;
        } else {
            if (streamSize < 3) {
                return OMX_FALSE;
            } else if ((pInputStream[0] == 0x00) &&
                       (pInputStream[1] == 0x00) &&
                       (pInputStream[2] == 0x01)) {
                return OMX_TRUE;
            } else {
                return OMX_FALSE;
            }
        }
        break;
    case CODEC_TYPE_H263:
        if (streamSize > 0)
            return OMX_TRUE;
        else
            return OMX_FALSE;
    default:
        SEC_OSAL_Log(SEC_LOG_WARNING, "%s: undefined codec type (%d)", __FUNCTION__, codecType);
        return OMX_FALSE;
    }
}

OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
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
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Param = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Param = NULL;
        SEC_MPEG4_HANDLE          *pMpeg4Dec = NULL;
        ret = SEC_OMX_Check_SizeVersion(pDstMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcMpeg4Param = &pMpeg4Dec->mpeg4Component[pDstMpeg4Param->nPortIndex];

        SEC_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE  *pDstH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_H263TYPE  *pSrcH263Param = NULL;
        SEC_MPEG4_HANDLE          *pMpeg4Dec = NULL;
        ret = SEC_OMX_Check_SizeVersion(pDstH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcH263Param = &pMpeg4Dec->h263Component[pDstH263Param->nPortIndex];

        SEC_OSAL_Memcpy(pDstH263Param, pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_S32 codecType;
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        codecType = ((SEC_MPEG4_HANDLE *)(((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4)
            SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_MPEG4_DEC_ROLE);
        else
            SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_H263_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        SEC_OMX_VIDEO_PROFILELEVEL       *pProfileLevel = NULL;
        OMX_U32                           maxProfileLevelNum = 0;
        OMX_S32                           codecType;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        codecType = ((SEC_MPEG4_HANDLE *)(((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pProfileLevel = supportedMPEG4ProfileLevels;
            maxProfileLevelNum = sizeof(supportedMPEG4ProfileLevels) / sizeof(SEC_OMX_VIDEO_PROFILELEVEL);
        } else {
            pProfileLevel = supportedH263ProfileLevels;
            maxProfileLevelNum = sizeof(supportedH263ProfileLevels) / sizeof(SEC_OMX_VIDEO_PROFILELEVEL);
        }

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
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pSrcMpeg4Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pSrcH263Param = NULL;
        SEC_MPEG4_HANDLE                 *pMpeg4Dec = NULL;
        OMX_S32                           codecType;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Dec->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pSrcMpeg4Param = &pMpeg4Dec->mpeg4Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcMpeg4Param->eProfile;
            pDstProfileLevel->eLevel = pSrcMpeg4Param->eLevel;
        } else {
            pSrcH263Param = &pMpeg4Dec->h263Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcH263Param->eProfile;
            pDstProfileLevel->eLevel = pSrcH263Param->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        SEC_MPEG4_HANDLE                    *pMpeg4Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg4Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_SetParameter(
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
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Param = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        SEC_MPEG4_HANDLE          *pMpeg4Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstMpeg4Param = &pMpeg4Dec->mpeg4Component[pSrcMpeg4Param->nPortIndex];

        SEC_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *pDstH263Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE *pSrcH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        SEC_MPEG4_HANDLE         *pMpeg4Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstH263Param = &pMpeg4Dec->h263Component[pSrcH263Param->nPortIndex];

        SEC_OSAL_Memcpy(pDstH263Param, pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
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

        if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_MPEG4_DEC_ROLE)) {
            pSECComponent->pSECPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            //((SEC_MPEG4_HANDLE *)(((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_MPEG4;
        } else if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_H263_DEC_ROLE)) {
            pSECComponent->pSECPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            //((SEC_MPEG4_HANDLE *)(((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_H263;
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
        OMX_VIDEO_PARAM_MPEG4TYPE        *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pDstH263Param = NULL;
        SEC_MPEG4_HANDLE                 *pMpeg4Dec = NULL;
        OMX_S32                           codecType;

        ret = SEC_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Dec->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstMpeg4Param = &pMpeg4Dec->mpeg4Component[pSrcProfileLevel->nPortIndex];
            pDstMpeg4Param->eProfile = pSrcProfileLevel->eProfile;
            pDstMpeg4Param->eLevel = pSrcProfileLevel->eLevel;
        } else {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstH263Param = &pMpeg4Dec->h263Component[pSrcProfileLevel->nPortIndex];
            pDstH263Param->eProfile = pSrcProfileLevel->eProfile;
            pDstH263Param->eLevel = pSrcProfileLevel->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        SEC_MPEG4_HANDLE                    *pMpeg4Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg4Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
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
        ret = SEC_OMX_VideoDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
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

OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_GetExtensionIndex(
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
        SEC_MPEG4_HANDLE *pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

        *pIndexType = OMX_IndexVendorThumbnailMode;

        ret = OMX_ErrorNone;
    } else {
        ret = SEC_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    OMX_S32                  codecType;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex != (MAX_COMPONENT_ROLE_NUM - 1)) {
        ret = OMX_ErrorNoMore;
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

    codecType = ((SEC_MPEG4_HANDLE *)(((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
    if (codecType == CODEC_TYPE_MPEG4)
        SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_MPEG4_DEC_ROLE);
    else
        SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_H263_DEC_ROLE);

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
    SEC_MPEG4_HANDLE      *pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

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
            pMpeg4Dec->hMFCMpeg4Handle.returnCodec = SsbSipMfcDecExe(pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle, pVideoDec->NBDecThread.oneFrameSize);
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
OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT      *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_MPEG4_HANDLE      *pMpeg4Dec = NULL;
    OMX_HANDLETYPE         hMFCHandle = NULL;
    OMX_PTR                pStreamBuffer = NULL;
    OMX_PTR                pStreamPhyBuffer = NULL;
#ifdef CONFIG_MFC_FPS
    SEC_OSAL_PerfInit(PERF_ID_DEC);
    SEC_OSAL_PerfInit(PERF_ID_CSC);
#endif
    CSC_METHOD csc_method = CSC_METHOD_SW;

    FunctionIn();

    pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    pMpeg4Dec->hMFCMpeg4Handle.bConfiguredMFC = OMX_FALSE;
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
    ghMFCHandle = pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle = hMFCHandle;

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
    pVideoDec->MFCDecInputBuffer[1].bufferSize    = DEFAULT_MFC_INPUT_BUFFER_SIZE / 2;
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
        pMpeg4Dec->hMFCMpeg4Handle.returnCodec = MFC_RET_OK;
    }
#endif

    pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[0].VirAddr;
    pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[0].PhyAddr;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[0].VirAddr;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[0].bufferSize;

    SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp = 0;
    pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp = 0;

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
OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_MPEG4_HANDLE      *pMpeg4Dec = NULL;
    OMX_HANDLETYPE         hMFCHandle = NULL;

    FunctionIn();

#ifdef CONFIG_MFC_FPS
    SEC_OSAL_PerfPrint("[DEC]",  PERF_ID_DEC);
    SEC_OSAL_PerfPrint("[CSC]",  PERF_ID_CSC);
#endif

    pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    hMFCHandle = pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle;

    pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamBuffer    = NULL;
    pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamPhyBuffer = NULL;
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
        ghMFCHandle = hMFCHandle = NULL;
        pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle = NULL;
    }

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_Mpeg4_Decode_Nonblock(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_MPEG4_HANDLE           *pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    OMX_HANDLETYPE              hMFCHandle = pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle;
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     configValue;
    int                         bufWidth;
    int                         bufHeight;
    OMX_U32                     FrameBufferYSize = 0;
    OMX_U32                     FrameBufferUVSize = 0;
    OMX_BOOL                    outputDataValid = OMX_FALSE;

    FunctionIn();

    if (pMpeg4Dec->hMFCMpeg4Handle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE MFCCodecType;
        if (pMpeg4Dec->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
            if (gbFIMV1)
                MFCCodecType = FIMV1_DEC;
            else
                MFCCodecType = MPEG4_DEC;
        } else {
            MFCCodecType = H263_DEC;
        }

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        /* Set mpeg4 deblocking filter enable */
        configValue = 1;
        SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_POST_ENABLE, &configValue);

        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            configValue = 0;    // the number that you want to delay
            SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &configValue);
        } else {
            configValue = MPEG4_DEC_NUM_OF_EXTRA_BUFFERS;
            SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &configValue);
        }

        SsbSipMfcDecSetInBuf(pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle,
                             pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamPhyBuffer,
                             pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pMpeg4Dec->hMFCMpeg4Handle.returnCodec = SsbSipMfcDecInit(hMFCHandle, MFCCodecType, oneFrameSize);
        if (pMpeg4Dec->hMFCMpeg4Handle.returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;

            if (SsbSipMfcDecGetConfig(hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol) != MFC_RET_OK) {
                ret = OMX_ErrorMFCInit;
                SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SsbSipMfcDecGetConfig failed", __FUNCTION__);
                goto EXIT;
            }

            /** Update Frame Size **/
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
                (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

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

            SEC_OSAL_Log(SEC_LOG_TRACE, "nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                    pSECInputPort->portDefinition.format.video.nFrameWidth, pSECInputPort->portDefinition.format.video.nFrameHeight,
                    pSECInputPort->portDefinition.format.video.nStride, pSECInputPort->portDefinition.format.video.nSliceHeight);

            pMpeg4Dec->hMFCMpeg4Handle.bConfiguredMFC = OMX_TRUE;
            if (pMpeg4Dec->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
            } else {
                pOutputData->dataLen = 0;
                ret = OMX_ErrorInputDataDecodeYet;
            }
            goto EXIT;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SsbSipMfcDecInit failed", __FUNCTION__);
            ret = OMX_ErrorMFCInit;    /* OMX_ErrorUndefined */
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pSECComponent->timeStamp[pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
    pSECComponent->nFlags[pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp] = pInputData->nFlags;

    if ((pMpeg4Dec->hMFCMpeg4Handle.returnCodec == MFC_RET_OK) &&
        (pVideoDec->bFirstFrame == OMX_FALSE)) {
        SSBSIP_MFC_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc decode done */
        if (pVideoDec->NBDecThread.bDecoderRun == OMX_TRUE) {
            SEC_OSAL_SemaphoreWait(pVideoDec->NBDecThread.hDecFrameEnd);
            pVideoDec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        SEC_OSAL_SleepMillisec(0);
        status = SsbSipMfcDecGetOutBuf(hMFCHandle, &outputInfo);
        bufWidth = (outputInfo.img_width + 15) & (~15);
        bufHeight = (outputInfo.img_height + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

        if ((SsbSipMfcDecGetConfig(hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
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
                pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pSECComponent->timeStamp[pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp];
            }
#else
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
#endif
        }

        if ((status == MFC_GETOUTBUF_DISPLAY_DECODING) ||
            (status == MFC_GETOUTBUF_DISPLAY_ONLY)) {
            outputDataValid = OMX_TRUE;
            pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp++;
            pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp %= MAX_TIMESTAMP;
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
        pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].bufferSize;
        oneFrameSize = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize;
        //pInputData->dataLen = oneFrameSize;
        //pInputData->remainDataLen = oneFrameSize;
    }

    if ((Check_Stream_PrefixCode(pInputData->dataBuffer, oneFrameSize, pMpeg4Dec->hMFCMpeg4Handle.codecType) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        if ((ret != OMX_ErrorInputDataDecodeYet) || (pSECComponent->getAllDelayBuffer == OMX_TRUE)) {
            SsbSipMfcDecSetConfig(pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp));
            pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp++;
            pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;
        }

        SsbSipMfcDecSetInBuf(pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle,
                             pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamPhyBuffer,
                             pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].dataSize = oneFrameSize;
        pVideoDec->NBDecThread.oneFrameSize = oneFrameSize;

        /* mfc decode start */
        SEC_OSAL_SemaphorePost(pVideoDec->NBDecThread.hDecFrameStart);
        pVideoDec->NBDecThread.bDecoderRun = OMX_TRUE;
        pMpeg4Dec->hMFCMpeg4Handle.returnCodec = MFC_RET_OK;

        SEC_OSAL_SleepMillisec(0);

        pVideoDec->indexInputBuffer++;
        pVideoDec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamBuffer    = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].VirAddr;
        pMpeg4Dec->hMFCMpeg4Handle.pMFCStreamPhyBuffer = pVideoDec->MFCDecInputBuffer[pVideoDec->indexInputBuffer].PhyAddr;
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

OMX_ERRORTYPE SEC_MFC_Mpeg4_Decode_Block(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT           *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT           *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_MPEG4_HANDLE           *pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    OMX_HANDLETYPE              hMFCHandle = pMpeg4Dec->hMFCMpeg4Handle.hMFCHandle;
    OMX_U32                     oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO  outputInfo;
    OMX_S32                     configValue;
    OMX_S32                     returnCodec;
    int                         bufWidth;
    int                         bufHeight;
    OMX_U32                     FrameBufferYSize;
    OMX_U32                     FrameBufferUVSize;

    FunctionIn();

    if (pMpeg4Dec->hMFCMpeg4Handle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE MFCCodecType;
        if (pMpeg4Dec->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
            if (gbFIMV1)
                MFCCodecType = FIMV1_DEC;
            else
                MFCCodecType = MPEG4_DEC;
        } else {
            MFCCodecType = H263_DEC;
        }

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        /* Set mpeg4 deblocking filter enable */
        configValue = 1;
        SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_POST_ENABLE, &configValue);

        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            configValue = 0;    // the number that you want to delay
            SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &configValue);
        } else {
            configValue = MPEG4_DEC_NUM_OF_EXTRA_BUFFERS;
            SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &configValue);
        }

        returnCodec = SsbSipMfcDecInit(hMFCHandle, MFCCodecType, oneFrameSize);
        if (returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;

            if (SsbSipMfcDecGetConfig(hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol) != MFC_RET_OK) {
                ret = OMX_ErrorMFCInit;
                SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SsbSipMfcDecGetConfig failed", __FUNCTION__);
                goto EXIT;
            }

            /** Update Frame Size **/
            if ((pSECInputPort->portDefinition.format.video.nFrameWidth != (unsigned int)imgResol.width) ||
               (pSECInputPort->portDefinition.format.video.nFrameHeight != (unsigned int)imgResol.height)) {
                /* change width and height information */
                pSECInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pSECInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pSECInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pSECInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

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

            SEC_OSAL_Log(SEC_LOG_TRACE, "nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                    pSECInputPort->portDefinition.format.video.nFrameWidth,  pSECInputPort->portDefinition.format.video.nFrameHeight,
                    pSECInputPort->portDefinition.format.video.nStride, pSECInputPort->portDefinition.format.video.nSliceHeight);

            pMpeg4Dec->hMFCMpeg4Handle.bConfiguredMFC = OMX_TRUE;
            if (pMpeg4Dec->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
            } else {
                pOutputData->dataLen = 0;
                ret = OMX_ErrorInputDataDecodeYet;
            }
            goto EXIT;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SsbSipMfcDecInit failed", __FUNCTION__);
            ret = OMX_ErrorMFCInit;    /* OMX_ErrorUndefined */
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

    if (Check_Stream_PrefixCode(pInputData->dataBuffer, pInputData->dataLen, pMpeg4Dec->hMFCMpeg4Handle.codecType) == OMX_TRUE) {
        pSECComponent->timeStamp[pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
        pSECComponent->nFlags[pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp] = pInputData->nFlags;
        SsbSipMfcDecSetConfig(hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp));

        returnCodec = SsbSipMfcDecExe(hMFCHandle, oneFrameSize);
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

        status = SsbSipMfcDecGetOutBuf(hMFCHandle, &outputInfo);
        bufWidth =  (outputInfo.img_width + 15) & (~15);
        bufHeight =  (outputInfo.img_height + 15) & (~15);
        FrameBufferYSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height));
        FrameBufferUVSize = ALIGN_TO_8KB(ALIGN_TO_128B(outputInfo.img_width) * ALIGN_TO_32B(outputInfo.img_height/2));

        if (status != MFC_GETOUTBUF_DISPLAY_ONLY) {
            pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp++;
            pMpeg4Dec->hMFCMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;
        }

        if ((SsbSipMfcDecGetConfig(hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
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
                pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp = indexTimestamp;
            } else {
                pOutputData->timeStamp = pSECComponent->timeStamp[pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp];
                pOutputData->nFlags = pSECComponent->nFlags[pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp];
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
            pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp++;
            pMpeg4Dec->hMFCMpeg4Handle.outputIndexTimestamp %= MAX_TIMESTAMP;
        }
        if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS)
            pOutputData->dataLen = 0;

        if ((status == MFC_GETOUTBUF_DISPLAY_ONLY) ||
            (pSECComponent->getAllDelayBuffer == OMX_TRUE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }

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
OMX_ERRORTYPE SEC_MFC_Mpeg4Dec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT   *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_MPEG4_HANDLE        *pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SEC_OMX_BASEPORT        *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT        *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 bCheckPrefix = OMX_FALSE;

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

#ifdef NONBLOCK_MODE_PROCESS
    ret = SEC_MFC_Mpeg4_Decode_Nonblock(pOMXComponent, pInputData, pOutputData);
#else
    ret = SEC_MFC_Mpeg4_Decode_Block(pOMXComponent, pInputData, pOutputData);
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
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    SEC_OMX_BASEPORT        *pSECPort = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    SEC_MPEG4_HANDLE        *pMpeg4Dec = NULL;
    OMX_S32                  codecType = -1;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_MPEG4_DEC, componentName) == 0) {
        codecType = CODEC_TYPE_MPEG4;
    } else if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_H263_DEC, componentName) == 0) {
        codecType = CODEC_TYPE_H263;
    } else {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_OMX_VideoDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->codecType = HW_VIDEO_DEC_CODEC;

    pSECComponent->componentName = (OMX_STRING)SEC_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pSECComponent->componentName == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg4Dec = SEC_OSAL_Malloc(sizeof(SEC_MPEG4_HANDLE));
    if (pMpeg4Dec == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_MPEG4_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    SEC_OSAL_Memset(pMpeg4Dec, 0, sizeof(SEC_MPEG4_HANDLE));
    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pMpeg4Dec;
    pMpeg4Dec->hMFCMpeg4Handle.codecType = codecType;

    if (codecType == CODEC_TYPE_MPEG4)
        SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_MPEG4_DEC);
    else
        SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_H263_DEC);

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
    if (codecType == CODEC_TYPE_MPEG4) {
        pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "video/mpeg4");
    } else {
        pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
        SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "video/h263");
    }
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

    if (codecType == CODEC_TYPE_MPEG4) {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Dec->mpeg4Component[i], OMX_VIDEO_PARAM_MPEG4TYPE);
            pMpeg4Dec->mpeg4Component[i].nPortIndex = i;
            pMpeg4Dec->mpeg4Component[i].eProfile   = OMX_VIDEO_MPEG4ProfileSimple;
            pMpeg4Dec->mpeg4Component[i].eLevel     = OMX_VIDEO_MPEG4Level3;
        }
    } else {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Dec->h263Component[i], OMX_VIDEO_PARAM_H263TYPE);
            pMpeg4Dec->h263Component[i].nPortIndex = i;
            pMpeg4Dec->h263Component[i].eProfile   = OMX_VIDEO_H263ProfileBaseline | OMX_VIDEO_H263ProfileISWV2;
            pMpeg4Dec->h263Component[i].eLevel     = OMX_VIDEO_H263Level45;
        }
    }

    pOMXComponent->GetParameter      = &SEC_MFC_Mpeg4Dec_GetParameter;
    pOMXComponent->SetParameter      = &SEC_MFC_Mpeg4Dec_SetParameter;
    pOMXComponent->GetConfig         = &SEC_MFC_Mpeg4Dec_GetConfig;
    pOMXComponent->SetConfig         = &SEC_MFC_Mpeg4Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &SEC_MFC_Mpeg4Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &SEC_MFC_Mpeg4Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &SEC_OMX_ComponentDeinit;

    pSECComponent->sec_mfc_componentInit      = &SEC_MFC_Mpeg4Dec_Init;
    pSECComponent->sec_mfc_componentTerminate = &SEC_MFC_Mpeg4Dec_Terminate;
    pSECComponent->sec_mfc_bufferProcess      = &SEC_MFC_Mpeg4Dec_bufferProcess;
    if (codecType == CODEC_TYPE_MPEG4)
        pSECComponent->sec_checkInputFrame = &Check_Mpeg4_Frame;
    else
        pSECComponent->sec_checkInputFrame = &Check_H263_Frame;

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
    SEC_MPEG4_HANDLE        *pMpeg4Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;

    pMpeg4Dec = (SEC_MPEG4_HANDLE *)((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pMpeg4Dec != NULL) {
        SEC_OSAL_Free(pMpeg4Dec);
        ((SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle = NULL;
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
