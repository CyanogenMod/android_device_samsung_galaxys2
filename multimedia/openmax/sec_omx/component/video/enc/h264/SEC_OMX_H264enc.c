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
 * @file        SEC_OMX_H264enc.c
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
#include "SEC_OMX_Venc.h"
#include "SEC_OSAL_ETC.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Thread.h"
#include "SEC_OSAL_Android.h"
#include "library_register.h"
#include "SEC_OMX_H264enc.h"
#include "SsbSipMfcApi.h"
#include "csc.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_H264_ENC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"


/* H.264 Encoder Supported Levels & profiles */
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


OMX_U32 OMXAVCProfileToProfileIDC(OMX_VIDEO_AVCPROFILETYPE profile)
{
    OMX_U32 ret = 0; //default OMX_VIDEO_AVCProfileMain

    if (profile == OMX_VIDEO_AVCProfileMain)
        ret = 0;
    else if (profile == OMX_VIDEO_AVCProfileHigh)
        ret = 1;
    else if (profile == OMX_VIDEO_AVCProfileBaseline)
        ret = 2;

    return ret;
}

OMX_U32 OMXAVCLevelToLevelIDC(OMX_VIDEO_AVCLEVELTYPE level)
{
    OMX_U32 ret = 40; //default OMX_VIDEO_AVCLevel4

    if (level == OMX_VIDEO_AVCLevel1)
        ret = 10;
    else if (level == OMX_VIDEO_AVCLevel1b)
        ret = 9;
    else if (level == OMX_VIDEO_AVCLevel11)
        ret = 11;
    else if (level == OMX_VIDEO_AVCLevel12)
        ret = 12;
    else if (level == OMX_VIDEO_AVCLevel13)
        ret = 13;
    else if (level == OMX_VIDEO_AVCLevel2)
        ret = 20;
    else if (level == OMX_VIDEO_AVCLevel21)
        ret = 21;
    else if (level == OMX_VIDEO_AVCLevel22)
        ret = 22;
    else if (level == OMX_VIDEO_AVCLevel3)
        ret = 30;
    else if (level == OMX_VIDEO_AVCLevel31)
        ret = 31;
    else if (level == OMX_VIDEO_AVCLevel32)
        ret = 32;
    else if (level == OMX_VIDEO_AVCLevel4)
        ret = 40;

    return ret;
}

OMX_U8 *FindDelimiter(OMX_U8 *pBuffer, OMX_U32 size)
{
    OMX_U32 i;

    for (i = 0; i < size - 3; i++) {
        if ((pBuffer[i] == 0x00)   &&
            (pBuffer[i + 1] == 0x00) &&
            (pBuffer[i + 2] == 0x00) &&
            (pBuffer[i + 3] == 0x01))
            return (pBuffer + i);
    }

    return NULL;
}

void H264PrintParams(SSBSIP_MFC_ENC_H264_PARAM *h264Arg)
{
    SEC_OSAL_Log(SEC_LOG_TRACE, "SourceWidth             : %d\n", h264Arg->SourceWidth);
    SEC_OSAL_Log(SEC_LOG_TRACE, "SourceHeight            : %d\n", h264Arg->SourceHeight);
    SEC_OSAL_Log(SEC_LOG_TRACE, "ProfileIDC              : %d\n", h264Arg->ProfileIDC);
    SEC_OSAL_Log(SEC_LOG_TRACE, "LevelIDC                : %d\n", h264Arg->LevelIDC);
    SEC_OSAL_Log(SEC_LOG_TRACE, "IDRPeriod               : %d\n", h264Arg->IDRPeriod);
    SEC_OSAL_Log(SEC_LOG_TRACE, "NumberReferenceFrames   : %d\n", h264Arg->NumberReferenceFrames);
    SEC_OSAL_Log(SEC_LOG_TRACE, "NumberRefForPframes     : %d\n", h264Arg->NumberRefForPframes);
    SEC_OSAL_Log(SEC_LOG_TRACE, "SliceMode               : %d\n", h264Arg->SliceMode);
    SEC_OSAL_Log(SEC_LOG_TRACE, "SliceArgument           : %d\n", h264Arg->SliceArgument);
#ifdef USE_SLICE_OUTPUT_MODE
    SEC_OSAL_Log(SEC_LOG_TRACE, "OutputMode              : %d\n", h264Arg->OutputMode);
#endif
    SEC_OSAL_Log(SEC_LOG_TRACE, "NumberBFrames           : %d\n", h264Arg->NumberBFrames);
    SEC_OSAL_Log(SEC_LOG_TRACE, "LoopFilterDisable       : %d\n", h264Arg->LoopFilterDisable);
    SEC_OSAL_Log(SEC_LOG_TRACE, "LoopFilterAlphaC0Offset : %d\n", h264Arg->LoopFilterAlphaC0Offset);
    SEC_OSAL_Log(SEC_LOG_TRACE, "LoopFilterBetaOffset    : %d\n", h264Arg->LoopFilterBetaOffset);
    SEC_OSAL_Log(SEC_LOG_TRACE, "SymbolMode              : %d\n", h264Arg->SymbolMode);
    SEC_OSAL_Log(SEC_LOG_TRACE, "PictureInterlace        : %d\n", h264Arg->PictureInterlace);
    SEC_OSAL_Log(SEC_LOG_TRACE, "Transform8x8Mode        : %d\n", h264Arg->Transform8x8Mode);
    SEC_OSAL_Log(SEC_LOG_TRACE, "RandomIntraMBRefresh    : %d\n", h264Arg->RandomIntraMBRefresh);
    SEC_OSAL_Log(SEC_LOG_TRACE, "PadControlOn            : %d\n", h264Arg->PadControlOn);
    SEC_OSAL_Log(SEC_LOG_TRACE, "LumaPadVal              : %d\n", h264Arg->LumaPadVal);
    SEC_OSAL_Log(SEC_LOG_TRACE, "CbPadVal                : %d\n", h264Arg->CbPadVal);
    SEC_OSAL_Log(SEC_LOG_TRACE, "CrPadVal                : %d\n", h264Arg->CrPadVal);
    SEC_OSAL_Log(SEC_LOG_TRACE, "EnableFRMRateControl    : %d\n", h264Arg->EnableFRMRateControl);
    SEC_OSAL_Log(SEC_LOG_TRACE, "EnableMBRateControl     : %d\n", h264Arg->EnableMBRateControl);
    SEC_OSAL_Log(SEC_LOG_TRACE, "FrameRate               : %d\n", h264Arg->FrameRate);
    SEC_OSAL_Log(SEC_LOG_TRACE, "Bitrate                 : %d\n", h264Arg->Bitrate);
    SEC_OSAL_Log(SEC_LOG_TRACE, "FrameQp                 : %d\n", h264Arg->FrameQp);
    SEC_OSAL_Log(SEC_LOG_TRACE, "QSCodeMax               : %d\n", h264Arg->QSCodeMax);
    SEC_OSAL_Log(SEC_LOG_TRACE, "QSCodeMin               : %d\n", h264Arg->QSCodeMin);
    SEC_OSAL_Log(SEC_LOG_TRACE, "CBRPeriodRf             : %d\n", h264Arg->CBRPeriodRf);
    SEC_OSAL_Log(SEC_LOG_TRACE, "DarkDisable             : %d\n", h264Arg->DarkDisable);
    SEC_OSAL_Log(SEC_LOG_TRACE, "SmoothDisable           : %d\n", h264Arg->SmoothDisable);
    SEC_OSAL_Log(SEC_LOG_TRACE, "StaticDisable           : %d\n", h264Arg->StaticDisable);
    SEC_OSAL_Log(SEC_LOG_TRACE, "ActivityDisable         : %d\n", h264Arg->ActivityDisable);
    SEC_OSAL_Log(SEC_LOG_TRACE, "FrameMap                : %d\n", h264Arg->FrameMap);
}

void Set_H264Enc_Param(SSBSIP_MFC_ENC_H264_PARAM *pH264Arg, SEC_OMX_BASECOMPONENT *pSECComponent)
{
    SEC_OMX_BASEPORT          *pSECInputPort = NULL;
    SEC_OMX_BASEPORT          *pSECOutputPort = NULL;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    SEC_H264ENC_HANDLE        *pH264Enc = NULL;

    pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];

    pH264Arg->codecType    = H264_ENC;
    pH264Arg->SourceWidth  = pSECOutputPort->portDefinition.format.video.nFrameWidth;
    pH264Arg->SourceHeight = pSECOutputPort->portDefinition.format.video.nFrameHeight;
    pH264Arg->IDRPeriod    = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pH264Arg->SliceMode    = 0;
#ifdef USE_SLICE_OUTPUT_MODE
    pH264Arg->OutputMode   = FRAME;
#endif
    pH264Arg->RandomIntraMBRefresh = 0;
    pH264Arg->Bitrate      = pSECOutputPort->portDefinition.format.video.nBitrate;
    pH264Arg->QSCodeMax    = 51;
    pH264Arg->QSCodeMin    = 10;
    pH264Arg->PadControlOn = 0;             // 0: disable, 1: enable
    pH264Arg->LumaPadVal   = 0;
    pH264Arg->CbPadVal     = 0;
    pH264Arg->CrPadVal     = 0;

    pH264Arg->ProfileIDC   = OMXAVCProfileToProfileIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile); //0;  //(OMX_VIDEO_AVCProfileMain)
    pH264Arg->LevelIDC     = OMXAVCLevelToLevelIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eLevel);       //40; //(OMX_VIDEO_AVCLevel4)
    pH264Arg->FrameRate    = (pSECInputPort->portDefinition.format.video.xFramerate) >> 16;
    pH264Arg->SliceArgument = 0;          // Slice mb/byte size number
    pH264Arg->NumberBFrames = 0;            // 0 ~ 2
    pH264Arg->NumberReferenceFrames = 1;
    pH264Arg->NumberRefForPframes   = 1;
    pH264Arg->LoopFilterDisable     = 1;    // 1: Loop Filter Disable, 0: Filter Enable
    pH264Arg->LoopFilterAlphaC0Offset = 0;
    pH264Arg->LoopFilterBetaOffset    = 0;
    pH264Arg->SymbolMode       = 0;         // 0: CAVLC, 1: CABAC
    pH264Arg->PictureInterlace = 0;
    pH264Arg->Transform8x8Mode = 0;         // 0: 4x4, 1: allow 8x8
    pH264Arg->DarkDisable     = 1;
    pH264Arg->SmoothDisable   = 1;
    pH264Arg->StaticDisable   = 1;
    pH264Arg->ActivityDisable = 1;

    pH264Arg->FrameQp      = pVideoEnc->quantization.nQpI;
    pH264Arg->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pH264Arg->FrameQp_B    = pVideoEnc->quantization.nQpB;

    SEC_OSAL_Log(SEC_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateVariable:
        SEC_OSAL_Log(SEC_LOG_TRACE, "Video Encode VBR");
        pH264Arg->EnableFRMRateControl = 0;        // 0: Disable, 1: Frame level RC
        pH264Arg->EnableMBRateControl  = 0;        // 0: Disable, 1:MB level RC
        pH264Arg->CBRPeriodRf  = 100;
        break;
    case OMX_Video_ControlRateConstant:
        SEC_OSAL_Log(SEC_LOG_TRACE, "Video Encode CBR");
        pH264Arg->EnableFRMRateControl = 1;        // 0: Disable, 1: Frame level RC
        pH264Arg->EnableMBRateControl  = 1;         // 0: Disable, 1:MB level RC
        pH264Arg->CBRPeriodRf  = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: //Android default
        SEC_OSAL_Log(SEC_LOG_TRACE, "Video Encode VBR");
        pH264Arg->EnableFRMRateControl = 0;
        pH264Arg->EnableMBRateControl  = 0;
        pH264Arg->CBRPeriodRf  = 100;
        break;
    }

    switch ((SEC_OMX_COLOR_FORMATTYPE)pSECInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV420Planar:
#ifdef USE_METADATABUFFERTYPE
    case OMX_COLOR_FormatAndroidOpaque:
#endif
        pH264Arg->FrameMap = NV12_LINEAR;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        pH264Arg->FrameMap = NV12_TILE;
        break;
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21Linear:
        pH264Arg->FrameMap = NV21_LINEAR;
        break;
    default:
        pH264Arg->FrameMap = NV12_TILE;
        break;
    }

    H264PrintParams(pH264Arg);
}

void Change_H264Enc_Param(SSBSIP_MFC_ENC_H264_PARAM *pH264Arg, SEC_OMX_BASECOMPONENT *pSECComponent)
{
    SEC_OMX_BASEPORT          *pSECInputPort = NULL;
    SEC_OMX_BASEPORT          *pSECOutputPort = NULL;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    SEC_H264ENC_HANDLE        *pH264Enc = NULL;

    pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        int set_conf_IntraRefreshVOP = 1;
        SsbSipMfcEncSetConfig(pH264Enc->hMFCH264Handle.hMFCHandle,
                                MFC_ENC_SETCONF_FRAME_TYPE,
                                &set_conf_IntraRefreshVOP);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pH264Arg->IDRPeriod != (int)pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1) {
        int set_conf_IDRPeriod = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
        SsbSipMfcEncSetConfig(pH264Enc->hMFCH264Handle.hMFCHandle,
                                MFC_ENC_SETCONF_I_PERIOD,
                                &set_conf_IDRPeriod);
    }
    if (pH264Arg->Bitrate != (int)pSECOutputPort->portDefinition.format.video.nBitrate) {
        int set_conf_bitrate = pSECOutputPort->portDefinition.format.video.nBitrate;
        SsbSipMfcEncSetConfig(pH264Enc->hMFCH264Handle.hMFCHandle,
                                MFC_ENC_SETCONF_CHANGE_BIT_RATE,
                                &set_conf_bitrate);
    }
    if (pH264Arg->FrameRate != (int)((pSECOutputPort->portDefinition.format.video.xFramerate) >> 16)) {
        int set_conf_framerate = (pSECInputPort->portDefinition.format.video.xFramerate) >> 16;
        SsbSipMfcEncSetConfig(pH264Enc->hMFCH264Handle.hMFCHandle,
                                MFC_ENC_SETCONF_CHANGE_FRAME_RATE,
                                &set_conf_framerate);
    }

    Set_H264Enc_Param(pH264Arg, pSECComponent);
    H264PrintParams(pH264Arg);
}

OMX_ERRORTYPE SEC_MFC_H264Enc_GetParameter(
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
        SEC_H264ENC_HANDLE      *pH264Enc = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstAVCComponent->nPortIndex];

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

        SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_H264_ENC_ROLE);
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
        SEC_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        SEC_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = SEC_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Enc_SetParameter(
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
        SEC_H264ENC_HANDLE      *pH264Enc = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcAVCComponent->nPortIndex];

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

        if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_H264_ENC_ROLE)) {
            pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        SEC_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        SEC_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = SEC_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Enc_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_H264ENC_HANDLE    *pH264Enc = NULL;

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
    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pAVCIntraPeriod->nIDRPeriod = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
            pAVCIntraPeriod->nPFrames = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames;
        }
    }
        break;
    default:
        ret = SEC_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Enc_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    SEC_H264ENC_HANDLE         *pH264Enc = NULL;

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

    pVideoEnc = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle);
    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        SEC_OMX_VIDEOENC_COMPONENT *pVEncBase = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle);
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            if (pAVCIntraPeriod->nIDRPeriod == (pAVCIntraPeriod->nPFrames + 1))
                pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = pAVCIntraPeriod->nPFrames;
            else {
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
        }
    }
        break;
    default:
        ret = SEC_OMX_VideoEncodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone)
        pVideoEnc->configChange = OMX_TRUE;

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Enc_GetExtensionIndex(
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
    if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_CONFIG_VIDEO_INTRAPERIOD) == 0) {
        *pIndexType = OMX_IndexConfigVideoIntraPeriod;
        ret = OMX_ErrorNone;
    } else {
        ret = SEC_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Enc_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
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
        SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_H264_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_EncodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_H264ENC_HANDLE    *pH264Enc = (SEC_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while (pVideoEnc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
        SEC_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameStart);

        if (pVideoEnc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
            pH264Enc->hMFCH264Handle.returnCodec = SsbSipMfcEncExe(pH264Enc->hMFCH264Handle.hMFCHandle);
            SEC_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameEnd);
        }
    }

EXIT:
    FunctionOut();
    SEC_OSAL_ThreadExit(NULL);

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE SEC_MFC_H264Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT     *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT          *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_H264ENC_HANDLE        *pH264Enc = NULL;
    OMX_PTR                    hMFCHandle = NULL;
    OMX_S32                    returnCodec = 0;
    CSC_METHOD csc_method = CSC_METHOD_SW;

    FunctionIn();

    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    pH264Enc->hMFCH264Handle.bConfiguredMFC = OMX_FALSE;
    pSECComponent->bUseFlagEOF = OMX_FALSE;
    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    /* MFC(Multi Function Codec) encoder and CMM(Codec Memory Management) driver open */
    switch (pSECInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
        hMFCHandle = (OMX_PTR)SsbSipMfcEncOpen();
        break;
    default: {
        SSBIP_MFC_BUFFER_TYPE buf_type = CACHE;
        hMFCHandle = (OMX_PTR)SsbSipMfcEncOpenExt(&buf_type);
        break;
    }
    }

    if (hMFCHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Enc->hMFCH264Handle.hMFCHandle = hMFCHandle;

    Set_H264Enc_Param(&(pH264Enc->hMFCH264Handle.mfcVideoAvc), pSECComponent);

    returnCodec = SsbSipMfcEncInit(hMFCHandle, &(pH264Enc->hMFCH264Handle.mfcVideoAvc));
    if (returnCodec != MFC_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* Allocate encoder's input buffer */
    returnCodec = SsbSipMfcEncGetInBuf(hMFCHandle, &(pH264Enc->hMFCH264Handle.inputInfo));
    if (returnCodec != MFC_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoEnc->MFCEncInputBuffer[0].YPhyAddr = pH264Enc->hMFCH264Handle.inputInfo.YPhyAddr;
    pVideoEnc->MFCEncInputBuffer[0].CPhyAddr = pH264Enc->hMFCH264Handle.inputInfo.CPhyAddr;
    pVideoEnc->MFCEncInputBuffer[0].YVirAddr = pH264Enc->hMFCH264Handle.inputInfo.YVirAddr;
    pVideoEnc->MFCEncInputBuffer[0].CVirAddr = pH264Enc->hMFCH264Handle.inputInfo.CVirAddr;
    pVideoEnc->MFCEncInputBuffer[0].YBufferSize = pH264Enc->hMFCH264Handle.inputInfo.YSize;
    pVideoEnc->MFCEncInputBuffer[0].CBufferSize = pH264Enc->hMFCH264Handle.inputInfo.CSize;
    pVideoEnc->MFCEncInputBuffer[0].YDataSize = 0;
    pVideoEnc->MFCEncInputBuffer[0].CDataSize = 0;
    SEC_OSAL_Log(SEC_LOG_TRACE, "pH264Enc->hMFCH264Handle.inputInfo.YVirAddr : 0x%x", pH264Enc->hMFCH264Handle.inputInfo.YVirAddr);
    SEC_OSAL_Log(SEC_LOG_TRACE, "pH264Enc->hMFCH264Handle.inputInfo.CVirAddr : 0x%x", pH264Enc->hMFCH264Handle.inputInfo.CVirAddr);

    returnCodec = SsbSipMfcEncGetInBuf(hMFCHandle, &(pH264Enc->hMFCH264Handle.inputInfo));
    if (returnCodec != MFC_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoEnc->MFCEncInputBuffer[1].YPhyAddr = pH264Enc->hMFCH264Handle.inputInfo.YPhyAddr;
    pVideoEnc->MFCEncInputBuffer[1].CPhyAddr = pH264Enc->hMFCH264Handle.inputInfo.CPhyAddr;
    pVideoEnc->MFCEncInputBuffer[1].YVirAddr = pH264Enc->hMFCH264Handle.inputInfo.YVirAddr;
    pVideoEnc->MFCEncInputBuffer[1].CVirAddr = pH264Enc->hMFCH264Handle.inputInfo.CVirAddr;
    pVideoEnc->MFCEncInputBuffer[1].YBufferSize = pH264Enc->hMFCH264Handle.inputInfo.YSize;
    pVideoEnc->MFCEncInputBuffer[1].CBufferSize = pH264Enc->hMFCH264Handle.inputInfo.CSize;
    pVideoEnc->MFCEncInputBuffer[1].YDataSize = 0;
    pVideoEnc->MFCEncInputBuffer[1].CDataSize = 0;
    SEC_OSAL_Log(SEC_LOG_TRACE, "pH264Enc->hMFCH264Handle.inputInfo.YVirAddr : 0x%x", pH264Enc->hMFCH264Handle.inputInfo.YVirAddr);
    SEC_OSAL_Log(SEC_LOG_TRACE, "pH264Enc->hMFCH264Handle.inputInfo.CVirAddr : 0x%x", pH264Enc->hMFCH264Handle.inputInfo.CVirAddr);

    pVideoEnc->indexInputBuffer = 0;

    pVideoEnc->bFirstFrame = OMX_TRUE;

#ifdef NONBLOCK_MODE_PROCESS
    pVideoEnc->NBEncThread.bExitEncodeThread = OMX_FALSE;
    pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
    SEC_OSAL_SemaphoreCreate(&(pVideoEnc->NBEncThread.hEncFrameStart));
    SEC_OSAL_SemaphoreCreate(&(pVideoEnc->NBEncThread.hEncFrameEnd));
    if (OMX_ErrorNone == SEC_OSAL_ThreadCreate(&pVideoEnc->NBEncThread.hNBEncodeThread,
                                                SEC_MFC_EncodeThread,
                                                pOMXComponent)) {
        pH264Enc->hMFCH264Handle.returnCodec = MFC_RET_OK;
    }
#endif
    SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Enc->hMFCH264Handle.indexTimestamp = 0;

    pVideoEnc->csc_handle = csc_init(&csc_method);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE SEC_MFC_H264Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle);
    SEC_H264ENC_HANDLE    *pH264Enc = NULL;
    OMX_PTR                hMFCHandle = NULL;

    FunctionIn();

    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
#ifdef NONBLOCK_MODE_PROCESS
    if (pVideoEnc->NBEncThread.hNBEncodeThread != NULL) {
        pVideoEnc->NBEncThread.bExitEncodeThread = OMX_TRUE;
        SEC_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
        SEC_OSAL_ThreadTerminate(pVideoEnc->NBEncThread.hNBEncodeThread);
        pVideoEnc->NBEncThread.hNBEncodeThread = NULL;
    }

    if(pVideoEnc->NBEncThread.hEncFrameEnd != NULL) {
        SEC_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameEnd);
        pVideoEnc->NBEncThread.hEncFrameEnd = NULL;
    }

    if(pVideoEnc->NBEncThread.hEncFrameStart != NULL) {
        SEC_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameStart);
        pVideoEnc->NBEncThread.hEncFrameStart = NULL;
    }
#endif

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    if (hMFCHandle != NULL) {
        SsbSipMfcEncClose(hMFCHandle);
        hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle = NULL;
    }

    if (pVideoEnc->csc_handle != NULL) {
        csc_deinit(pVideoEnc->csc_handle);
        pVideoEnc->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264_Encode_Nonblock(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT     *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle);
    SEC_H264ENC_HANDLE        *pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SSBSIP_MFC_ENC_INPUT_INFO *pInputInfo = &pH264Enc->hMFCH264Handle.inputInfo;
    SSBSIP_MFC_ENC_OUTPUT_INFO outputInfo;
    SEC_OMX_BASEPORT          *pSECPort = NULL;
    MFC_ENC_ADDR_INFO          addrInfo;
    OMX_U32                    oneFrameSize = pInputData->dataLen;

    FunctionIn();

    if (pH264Enc->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        pH264Enc->hMFCH264Handle.returnCodec = SsbSipMfcEncGetOutBuf(pH264Enc->hMFCH264Handle.hMFCHandle, &outputInfo);
        if (pH264Enc->hMFCH264Handle.returnCodec != MFC_RET_OK) {
            SEC_OSAL_Log(SEC_LOG_TRACE, "%s - SsbSipMfcEncGetOutBuf Failed\n", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        } else {
            OMX_U8 *p = NULL;
            int iSpsSize = 0;
            int iPpsSize = 0;

            p = FindDelimiter((OMX_U8 *)outputInfo.StrmVirAddr + 4, outputInfo.headerSize - 4);

            iSpsSize = (unsigned int)p - (unsigned int)outputInfo.StrmVirAddr;
            pH264Enc->hMFCH264Handle.headerData.pHeaderSPS = (OMX_PTR)outputInfo.StrmVirAddr;
            pH264Enc->hMFCH264Handle.headerData.SPSLen = iSpsSize;

            iPpsSize = outputInfo.headerSize - iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.pHeaderPPS = (OMX_U8 *)outputInfo.StrmVirAddr + iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.PPSLen = iPpsSize;
        }

        pOutputData->dataBuffer = outputInfo.StrmVirAddr;
        pOutputData->allocSize = outputInfo.headerSize;
        pOutputData->dataLen = outputInfo.headerSize;
        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pH264Enc->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

        ret = OMX_ErrorInputDataEncodeYet;
        goto EXIT;
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;

    if (oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
        (pSECComponent->getAllDelayBuffer == OMX_TRUE)){
        /* Dummy input data for get out encoded last frame */
        pInputInfo->YPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YPhyAddr;
        pInputInfo->CPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CPhyAddr;
        pInputInfo->YVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
        pInputInfo->CVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
    } else {
        switch (pSECPort->portDefinition.format.video.eColorFormat) {
        case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV21LPhysicalAddress: {
#ifndef USE_METADATABUFFERTYPE
            /* USE_FIMC_FRAME_BUFFER */
            SEC_OSAL_Memcpy(&addrInfo.pAddrY, pInputData->dataBuffer, sizeof(addrInfo.pAddrY));
            SEC_OSAL_Memcpy(&addrInfo.pAddrC, pInputData->dataBuffer + sizeof(addrInfo.pAddrY), sizeof(addrInfo.pAddrC));
#else
            OMX_PTR ppBuf[3];
            SEC_OSAL_GetInfoFromMetaData(pInputData, ppBuf);

            SEC_OSAL_Memcpy(&addrInfo.pAddrY, ppBuf[0], sizeof(addrInfo.pAddrY));
            SEC_OSAL_Memcpy(&addrInfo.pAddrC, ppBuf[1], sizeof(addrInfo.pAddrC));
#endif
            pInputInfo->YPhyAddr = addrInfo.pAddrY;
            pInputInfo->CPhyAddr = addrInfo.pAddrC;
            break;
        }
        case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
            addrInfo.pAddrY = *((void **)pInputData->dataBuffer);
            addrInfo.pAddrC = (void *)((char *)addrInfo.pAddrY + pInputInfo->YSize);

            pInputInfo->YPhyAddr = addrInfo.pAddrY;
            pInputInfo->CPhyAddr = addrInfo.pAddrC;
            break;
        default:
            pInputInfo->YPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YPhyAddr;
            pInputInfo->CPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CPhyAddr;
            pInputInfo->YVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
            pInputInfo->CVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
            break;
        }
    }

    pSECComponent->timeStamp[pH264Enc->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
    pSECComponent->nFlags[pH264Enc->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;

    if ((pH264Enc->hMFCH264Handle.returnCodec == MFC_RET_OK) &&
        (pVideoEnc->bFirstFrame == OMX_FALSE)) {
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc encode done */
        if (pVideoEnc->NBEncThread.bEncoderRun != OMX_FALSE) {
            SEC_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameEnd);
            pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
        }

        SEC_OSAL_SleepMillisec(0);
        pH264Enc->hMFCH264Handle.returnCodec = SsbSipMfcEncGetOutBuf(pH264Enc->hMFCH264Handle.hMFCHandle, &outputInfo);
        if ((SsbSipMfcEncGetConfig(pH264Enc->hMFCH264Handle.hMFCHandle, MFC_ENC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))){
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
        }

        if (pH264Enc->hMFCH264Handle.returnCodec == MFC_RET_OK) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = outputInfo.StrmVirAddr;
            pOutputData->allocSize = outputInfo.dataSize;
            pOutputData->dataLen = outputInfo.dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (outputInfo.frameType == MFC_FRAME_TYPE_I_FRAME)
                pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            SEC_OSAL_Log(SEC_LOG_TRACE, "MFC Encode OK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

            ret = OMX_ErrorNone;
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SsbSipMfcEncGetOutBuf failed, ret:%d", __FUNCTION__, pH264Enc->hMFCH264Handle.returnCodec);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        if (pSECComponent->getAllDelayBuffer == OMX_TRUE) {
            ret = OMX_ErrorInputDataEncodeYet;
        }
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pSECComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataEncodeYet;
        }
        if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            pOutputData->dataLen = 0;
            pOutputData->usedDataLen = 0;
            SEC_OSAL_Log(SEC_LOG_TRACE, "OMX_BUFFERFLAG_EOS!!!");
            ret = OMX_ErrorNone;
        }
    }
    if (pH264Enc->hMFCH264Handle.returnCodec != MFC_RET_OK) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "In %s : SsbSipMfcEncExe Failed!!!\n", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.returnCodec = SsbSipMfcEncSetInBuf(pH264Enc->hMFCH264Handle.hMFCHandle, pInputInfo);
    if (pH264Enc->hMFCH264Handle.returnCodec != MFC_RET_OK) {
        SEC_OSAL_Log(SEC_LOG_TRACE, "Error : SsbSipMfcEncSetInBuf() \n");
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        pVideoEnc->indexInputBuffer++;
        pVideoEnc->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        Change_H264Enc_Param(&(pH264Enc->hMFCH264Handle.mfcVideoAvc), pSECComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }

    SsbSipMfcEncSetConfig(pH264Enc->hMFCH264Handle.hMFCHandle, MFC_ENC_SETCONF_FRAME_TAG, &(pH264Enc->hMFCH264Handle.indexTimestamp));

    /* mfc encode start */
    SEC_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
    pVideoEnc->NBEncThread.bEncoderRun = OMX_TRUE;
    pH264Enc->hMFCH264Handle.indexTimestamp++;
    pH264Enc->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;
    pVideoEnc->bFirstFrame = OMX_FALSE;
    SEC_OSAL_SleepMillisec(0);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264_Encode_Block(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT     *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle);
    SEC_H264ENC_HANDLE        *pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SSBSIP_MFC_ENC_INPUT_INFO *pInputInfo = &pH264Enc->hMFCH264Handle.inputInfo;
    SSBSIP_MFC_ENC_OUTPUT_INFO outputInfo;
    SEC_OMX_BASEPORT          *pSECPort = NULL;
    MFC_ENC_ADDR_INFO          addrInfo;
    OMX_U32                    oneFrameSize = pInputData->dataLen;
    OMX_S32                    returnCodec = 0;

    FunctionIn();

    if (pH264Enc->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        returnCodec = SsbSipMfcEncGetOutBuf(pH264Enc->hMFCH264Handle.hMFCHandle, &outputInfo);
        if (returnCodec != MFC_RET_OK)
        {
            SEC_OSAL_Log(SEC_LOG_TRACE, "%s - SsbSipMfcEncGetOutBuf Failed\n", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        } else {
            OMX_U8 *p = NULL;
            int iSpsSize = 0;
            int iPpsSize = 0;

            p = FindDelimiter((OMX_U8 *)outputInfo.StrmVirAddr + 4, outputInfo.headerSize - 4);

            iSpsSize = (unsigned int)p - (unsigned int)outputInfo.StrmVirAddr;
            pH264Enc->hMFCH264Handle.headerData.pHeaderSPS = (OMX_PTR)outputInfo.StrmVirAddr;
            pH264Enc->hMFCH264Handle.headerData.SPSLen = iSpsSize;

            iPpsSize = outputInfo.headerSize - iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.pHeaderPPS = (OMX_U8 *)outputInfo.StrmVirAddr + iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.PPSLen = iPpsSize;
        }

        pOutputData->dataBuffer = outputInfo.StrmVirAddr;
        pOutputData->allocSize = outputInfo.headerSize;
        pOutputData->dataLen = outputInfo.headerSize;
        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pH264Enc->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

        ret = OMX_ErrorInputDataEncodeYet;
        goto EXIT;
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;

    if (oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    switch (pSECPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress: {
#ifndef USE_METADATABUFFERTYPE
        /* USE_FIMC_FRAME_BUFFER */
        SEC_OSAL_Memcpy(&addrInfo.pAddrY, pInputData->dataBuffer, sizeof(addrInfo.pAddrY));
        SEC_OSAL_Memcpy(&addrInfo.pAddrC, pInputData->dataBuffer + sizeof(addrInfo.pAddrY), sizeof(addrInfo.pAddrC));
#else
        OMX_PTR ppBuf[3];
        SEC_OSAL_GetInfoFromMetaData(pInputData, ppBuf);

        SEC_OSAL_Memcpy(&addrInfo.pAddrY, ppBuf[0], sizeof(addrInfo.pAddrY));
        SEC_OSAL_Memcpy(&addrInfo.pAddrC, ppBuf[1], sizeof(addrInfo.pAddrC));
#endif
        pInputInfo->YPhyAddr = addrInfo.pAddrY;
        pInputInfo->CPhyAddr = addrInfo.pAddrC;
        break;
    }
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
        addrInfo.pAddrY = *((void **)pInputData->dataBuffer);
        addrInfo.pAddrC = (void *)((char *)addrInfo.pAddrY + pInputInfo->YSize);

        pInputInfo->YPhyAddr = addrInfo.pAddrY;
        pInputInfo->CPhyAddr = addrInfo.pAddrC;
        break;
    default:
        pInputInfo->YPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YPhyAddr;
        pInputInfo->CPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CPhyAddr;
        pInputInfo->YVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
        pInputInfo->CVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
        break;
    }

    returnCodec = SsbSipMfcEncSetInBuf(pH264Enc->hMFCH264Handle.hMFCHandle, pInputInfo);
    if (returnCodec != MFC_RET_OK) {
        SEC_OSAL_Log(SEC_LOG_TRACE, "Error : SsbSipMfcEncSetInBuf() \n");
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        pVideoEnc->indexInputBuffer++;
        pVideoEnc->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        Change_H264Enc_Param(&(pH264Enc->hMFCH264Handle.mfcVideoAvc), pSECComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }

    pSECComponent->timeStamp[pH264Enc->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
    pSECComponent->nFlags[pH264Enc->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;
    SsbSipMfcEncSetConfig(pH264Enc->hMFCH264Handle.hMFCHandle, MFC_ENC_SETCONF_FRAME_TAG, &(pH264Enc->hMFCH264Handle.indexTimestamp));

    returnCodec = SsbSipMfcEncExe(pH264Enc->hMFCH264Handle.hMFCHandle);
    if (returnCodec == MFC_RET_OK) {
        OMX_S32 indexTimestamp = 0;

        pH264Enc->hMFCH264Handle.indexTimestamp++;
        pH264Enc->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;

        returnCodec = SsbSipMfcEncGetOutBuf(pH264Enc->hMFCH264Handle.hMFCHandle, &outputInfo);
        if ((SsbSipMfcEncGetConfig(pH264Enc->hMFCH264Handle.hMFCHandle, MFC_ENC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))){
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
        }

        if (returnCodec == MFC_RET_OK) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = outputInfo.StrmVirAddr;
            pOutputData->allocSize = outputInfo.dataSize;
            pOutputData->dataLen = outputInfo.dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (outputInfo.frameType == MFC_FRAME_TYPE_I_FRAME)
                pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            SEC_OSAL_Log(SEC_LOG_TRACE, "MFC Encode OK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

            ret = OMX_ErrorNone;
        }
    }

    if (returnCodec != MFC_RET_OK) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "In %s : SsbSipMfcEncExe OR SsbSipMfcEncGetOutBuf Failed!!!\n", __func__);
        ret = OMX_ErrorUndefined;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Encode */
OMX_ERRORTYPE SEC_MFC_H264Enc_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT   *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_H264ENC_HANDLE      *pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    SEC_OMX_BASEPORT        *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT        *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 endOfFrame = OMX_FALSE;
    OMX_BOOL                 flagEOS = OMX_FALSE;

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
    ret = SEC_MFC_H264_Encode_Nonblock(pOMXComponent, pInputData, pOutputData);
#else
    ret = SEC_MFC_H264_Encode_Block(pOMXComponent, pInputData, pOutputData);
#endif
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataEncodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                            pSECComponent->callbackData,
                                            OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        /* pOutputData->usedDataLen = 0; */
        pOutputData->remainDataLen = pOutputData->dataLen - pOutputData->usedDataLen;
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
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    SEC_H264ENC_HANDLE      *pH264Enc = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_H264_ENC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->codecType = HW_VIDEO_ENC_CODEC;

    pSECComponent->componentName = (OMX_STRING)SEC_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pSECComponent->componentName == NULL) {
        SEC_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Enc = SEC_OSAL_Malloc(sizeof(SEC_H264ENC_HANDLE));
    if (pH264Enc == NULL) {
        SEC_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pH264Enc, 0, sizeof(SEC_H264ENC_HANDLE));
    pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pH264Enc;

    SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_H264_ENC);
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
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "raw/video");
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pSECPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pSECPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "video/avc");
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Enc->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Enc->AVCComponent[i].nPortIndex = i;
        pH264Enc->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Enc->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel31;

        pH264Enc->AVCComponent[i].nPFrames = 20;
    }

    pOMXComponent->GetParameter      = &SEC_MFC_H264Enc_GetParameter;
    pOMXComponent->SetParameter      = &SEC_MFC_H264Enc_SetParameter;
    pOMXComponent->GetConfig         = &SEC_MFC_H264Enc_GetConfig;
    pOMXComponent->SetConfig         = &SEC_MFC_H264Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &SEC_MFC_H264Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &SEC_MFC_H264Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &SEC_OMX_ComponentDeinit;

    pSECComponent->sec_mfc_componentInit      = &SEC_MFC_H264Enc_Init;
    pSECComponent->sec_mfc_componentTerminate = &SEC_MFC_H264Enc_Terminate;
    pSECComponent->sec_mfc_bufferProcess      = &SEC_MFC_H264Enc_bufferProcess;
    pSECComponent->sec_checkInputFrame        = NULL;

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
    SEC_H264ENC_HANDLE      *pH264Enc = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;

    pH264Enc = (SEC_H264ENC_HANDLE *)((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pH264Enc != NULL) {
        SEC_OSAL_Free(pH264Enc);
        pH264Enc = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle = NULL;
    }

    ret = SEC_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
