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
 * @file        Exynos_OMX_Mpeg4enc.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Venc.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Android.h"
#include "library_register.h"
#include "Exynos_OMX_Mpeg4enc.h"
#include "SsbSipMfcApi.h"
#include "color_space_convertor.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_MPEG4_ENC"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


/* MPEG4 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedMPEG4ProfileLevels[] ={
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

/* H.263 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedH263ProfileLevels[] = {
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70}};

OMX_U32 OMXMpeg4ProfileToMFCProfile(OMX_VIDEO_MPEG4PROFILETYPE profile)
{
    OMX_U32 ret;

    switch (profile) {
    case OMX_VIDEO_MPEG4ProfileSimple:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4ProfileAdvancedSimple:
        ret = 1;
        break;
    default:
        ret = 0;
    };

    return ret;
}
OMX_U32 OMXMpeg4LevelToMFCLevel(OMX_VIDEO_MPEG4LEVELTYPE level)
{
    OMX_U32 ret;

    switch (level) {
    case OMX_VIDEO_MPEG4Level0:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4Level0b:
        ret = 9;
        break;
    case OMX_VIDEO_MPEG4Level1:
        ret = 1;
        break;
    case OMX_VIDEO_MPEG4Level2:
        ret = 2;
        break;
    case OMX_VIDEO_MPEG4Level3:
        ret = 3;
        break;
    case OMX_VIDEO_MPEG4Level4:
    case OMX_VIDEO_MPEG4Level4a:
        ret = 4;
        break;
    case OMX_VIDEO_MPEG4Level5:
        ret = 5;
        break;
    default:
        ret = 0;
    };

    return ret;
}

void Mpeg4PrintParams(SSBSIP_MFC_ENC_MPEG4_PARAM *pMpeg4Param)
{
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d\n", pMpeg4Param->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d\n", pMpeg4Param->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d\n", pMpeg4Param->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d\n", pMpeg4Param->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d\n", pMpeg4Param->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d\n", pMpeg4Param->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d\n", pMpeg4Param->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d\n", pMpeg4Param->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d\n", pMpeg4Param->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax               : %d\n", pMpeg4Param->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d\n", pMpeg4Param->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d\n", pMpeg4Param->CBRPeriodRf);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d\n", pMpeg4Param->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d\n", pMpeg4Param->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d\n", pMpeg4Param->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d\n", pMpeg4Param->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d\n", pMpeg4Param->FrameMap);

    /* MPEG4 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ProfileIDC              : %d\n", pMpeg4Param->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LevelIDC                : %d\n", pMpeg4Param->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_B               : %d\n", pMpeg4Param->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "TimeIncreamentRes       : %d\n", pMpeg4Param->TimeIncreamentRes);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "VopTimeIncreament       : %d\n", pMpeg4Param->VopTimeIncreament);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceArgument           : %d\n", pMpeg4Param->SliceArgument);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberBFrames           : %d\n", pMpeg4Param->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DisableQpelME           : %d\n", pMpeg4Param->DisableQpelME);
}

void H263PrintParams(SSBSIP_MFC_ENC_H263_PARAM *pH263Param)
{
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d\n", pH263Param->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d\n", pH263Param->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d\n", pH263Param->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d\n", pH263Param->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d\n", pH263Param->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d\n", pH263Param->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d\n", pH263Param->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d\n", pH263Param->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d\n", pH263Param->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax               : %d\n", pH263Param->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d\n", pH263Param->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d\n", pH263Param->CBRPeriodRf);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d\n", pH263Param->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d\n", pH263Param->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d\n", pH263Param->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d\n", pH263Param->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d\n", pH263Param->FrameMap);

    /* H.263 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameRate               : %d\n", pH263Param->FrameRate);
}

void Set_Mpeg4Enc_Param(SSBSIP_MFC_ENC_MPEG4_PARAM *pMpeg4Param, EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pMpeg4Param->codecType            = MPEG4_ENC;
    pMpeg4Param->SourceWidth          = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pMpeg4Param->SourceHeight         = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pMpeg4Param->IDRPeriod            = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pMpeg4Param->SliceMode            = 0;
    pMpeg4Param->RandomIntraMBRefresh = 0;
    pMpeg4Param->Bitrate              = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pMpeg4Param->QSCodeMax            = 30;
    pMpeg4Param->QSCodeMin            = 10;
    pMpeg4Param->PadControlOn         = 0;    /* 0: Use boundary pixel, 1: Use the below setting value */
    pMpeg4Param->LumaPadVal           = 0;
    pMpeg4Param->CbPadVal             = 0;
    pMpeg4Param->CrPadVal             = 0;

    pMpeg4Param->ProfileIDC           = OMXMpeg4ProfileToMFCProfile(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eProfile);
    pMpeg4Param->LevelIDC             = OMXMpeg4LevelToMFCLevel(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eLevel);
    pMpeg4Param->TimeIncreamentRes    = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
    pMpeg4Param->VopTimeIncreament    = 1;
    pMpeg4Param->SliceArgument        = 0;    /* MB number or byte number */
    pMpeg4Param->NumberBFrames        = 0;    /* 0(not used) ~ 2 */
    pMpeg4Param->DisableQpelME        = 1;

    pMpeg4Param->FrameQp              = pVideoEnc->quantization.nQpI;
    pMpeg4Param->FrameQp_P            = pVideoEnc->quantization.nQpP;
    pMpeg4Param->FrameQp_B            = pVideoEnc->quantization.nQpB;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateVariable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pMpeg4Param->EnableFRMRateControl = 0;        // 0: Disable, 1: Frame level RC
        pMpeg4Param->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pMpeg4Param->EnableFRMRateControl = 1;        // 0: Disable, 1: Frame level RC
        pMpeg4Param->CBRPeriodRf          = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: //Android default
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pMpeg4Param->EnableFRMRateControl = 0;
        pMpeg4Param->CBRPeriodRf          = 100;
        break;
    }

    switch ((EXYNOS_OMX_COLOR_FORMATTYPE)pExynosInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV420Planar:
#ifdef METADATABUFFERTYPE
    case OMX_COLOR_FormatAndroidOpaque:
#endif
        pMpeg4Param->FrameMap = NV12_LINEAR;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        pMpeg4Param->FrameMap = NV12_TILE;
        break;
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21Linear:
        pMpeg4Param->FrameMap = NV21_LINEAR;
        break;
    default:
        pMpeg4Param->FrameMap = NV12_TILE;
        break;
    }

    Mpeg4PrintParams(pMpeg4Param);
}

void Change_Mpeg4Enc_Param(SSBSIP_MFC_ENC_MPEG4_PARAM *pMpeg4Param, EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        int set_conf_IntraRefreshVOP = 1;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_FRAME_TYPE,
                                &set_conf_IntraRefreshVOP);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }

    if (pMpeg4Param->IDRPeriod != (int)pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1) {
        int set_conf_IDRPeriod = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_I_PERIOD,
                                &set_conf_IDRPeriod);
    }
    if (pMpeg4Param->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        int set_conf_bitrate = pExynosOutputPort->portDefinition.format.video.nBitrate;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_CHANGE_BIT_RATE,
                                &set_conf_bitrate);
    }
    if (pMpeg4Param->TimeIncreamentRes != (int)((pExynosOutputPort->portDefinition.format.video.xFramerate) >> 16)) {
        int set_conf_framerate = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_CHANGE_FRAME_RATE,
                                &set_conf_framerate);
    }

    Set_Mpeg4Enc_Param(pMpeg4Param, pExynosComponent);
    Mpeg4PrintParams(pMpeg4Param);
}

void Set_H263Enc_Param(SSBSIP_MFC_ENC_H263_PARAM *pH263Param, EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pH263Param->codecType            = H263_ENC;
    pH263Param->SourceWidth          = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pH263Param->SourceHeight         = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pH263Param->IDRPeriod            = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pH263Param->SliceMode            = 0;
    pH263Param->RandomIntraMBRefresh = 0;
    pH263Param->Bitrate              = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pH263Param->QSCodeMax            = 30;
    pH263Param->QSCodeMin            = 10;
    pH263Param->PadControlOn         = 0;    /* 0: Use boundary pixel, 1: Use the below setting value */
    pH263Param->LumaPadVal           = 0;
    pH263Param->CbPadVal             = 0;
    pH263Param->CrPadVal             = 0;

    pH263Param->FrameRate            = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;

    pH263Param->FrameQp              = pVideoEnc->quantization.nQpI;
    pH263Param->FrameQp_P            = pVideoEnc->quantization.nQpP;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateVariable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pH263Param->EnableFRMRateControl = 0;        // 0: Disable, 1: Frame level RC
        pH263Param->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pH263Param->EnableFRMRateControl = 1;        // 0: Disable, 1: Frame level RC
        pH263Param->CBRPeriodRf          = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: //Android default
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pH263Param->EnableFRMRateControl = 0;
        pH263Param->CBRPeriodRf          = 100;
        break;
    }

    switch ((EXYNOS_OMX_COLOR_FORMATTYPE)pExynosInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
    case OMX_COLOR_FormatYUV420SemiPlanar:
        pH263Param->FrameMap = NV12_LINEAR;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        pH263Param->FrameMap = NV12_TILE;
        break;
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21Linear:
        pH263Param->FrameMap = NV21_LINEAR;
        break;
    default:
        pH263Param->FrameMap = NV12_TILE;
        break;
    }

    H263PrintParams(pH263Param);
}

void Change_H263Enc_Param(SSBSIP_MFC_ENC_H263_PARAM *pH263Param, EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        int set_conf_IntraRefreshVOP = 1;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_FRAME_TYPE,
                                &set_conf_IntraRefreshVOP);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pH263Param->IDRPeriod != (int)pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1) {
        int set_conf_IDRPeriod = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_I_PERIOD,
                                &set_conf_IDRPeriod);
    }
    if (pH263Param->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        int set_conf_bitrate = pExynosOutputPort->portDefinition.format.video.nBitrate;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_CHANGE_BIT_RATE,
                                &set_conf_bitrate);
    }
    if (pH263Param->FrameRate != (int)((pExynosOutputPort->portDefinition.format.video.xFramerate) >> 16)) {
        int set_conf_framerate = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        SsbSipMfcEncSetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                                MFC_ENC_SETCONF_CHANGE_FRAME_RATE,
                                &set_conf_framerate);
    }

    Set_H263Enc_Param(pH263Param, pExynosComponent);
    H263PrintParams(pH263Param);
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Param = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Param = NULL;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcMpeg4Param = &pMpeg4Enc->mpeg4Component[pDstMpeg4Param->nPortIndex];

        Exynos_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE  *pDstH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_H263TYPE  *pSrcH263Param = NULL;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcH263Param = &pMpeg4Enc->h263Component[pDstH263Param->nPortIndex];

        Exynos_OSAL_Memcpy(pDstH263Param, pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_S32 codecType;
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4)
            Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
        else
            Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        EXYNOS_OMX_VIDEO_PROFILELEVEL    *pProfileLevel = NULL;
        OMX_U32                           maxProfileLevelNum = 0;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pProfileLevel = supportedMPEG4ProfileLevels;
            maxProfileLevelNum = sizeof(supportedMPEG4ProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);
        } else {
            pProfileLevel = supportedH263ProfileLevels;
            maxProfileLevelNum = sizeof(supportedH263ProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);
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
        EXYNOS_MPEG4ENC_HANDLE           *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Enc->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pSrcMpeg4Param = &pMpeg4Enc->mpeg4Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcMpeg4Param->eProfile;
            pDstProfileLevel->eLevel = pSrcMpeg4Param->eLevel;
        } else {
            pSrcH263Param = &pMpeg4Enc->h263Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcH263Param->eProfile;
            pDstProfileLevel->eLevel = pSrcH263Param->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Param = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstMpeg4Param = &pMpeg4Enc->mpeg4Component[pSrcMpeg4Param->nPortIndex];

        Exynos_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *pDstH263Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE *pSrcH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstH263Param = &pMpeg4Enc->h263Component[pSrcH263Param->nPortIndex];

        Exynos_OSAL_Memcpy(pDstH263Param, pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
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

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            //((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_MPEG4;
        } else if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            //((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_H263;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pDstH263Param = NULL;
        EXYNOS_MPEG4ENC_HANDLE           *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Enc->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstMpeg4Param = &pMpeg4Enc->mpeg4Component[pSrcProfileLevel->nPortIndex];
            pDstMpeg4Param->eProfile = pSrcProfileLevel->eProfile;
            pDstMpeg4Param->eLevel = pSrcProfileLevel->eLevel;
        } else {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstH263Param = &pMpeg4Enc->h263Component[pSrcProfileLevel->nPortIndex];
            pDstH263Param->eProfile = pSrcProfileLevel->eProfile;
            pDstH263Param->eLevel = pSrcProfileLevel->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_MPEG4ENC_HANDLE                 *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);

    switch (nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        EXYNOS_OMX_VIDEOENC_COMPONENT *pVEncBase = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames = nPFrames;
        else
            pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone)
        pVideoEnc->configChange = OMX_TRUE;

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_CONFIG_VIDEO_INTRAPERIOD) == 0) {
        *pIndexType = OMX_IndexConfigVideoIntraPeriod;
        ret = OMX_ErrorNone;
    } else {
        ret = Exynos_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    OMX_S32                     codecType;

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

    codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
    if (codecType == CODEC_TYPE_MPEG4)
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
    else
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_EncodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while (pVideoEnc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
        Exynos_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameStart);

        if (pVideoEnc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
            pMpeg4Enc->hMFCMpeg4Handle.returnCodec = SsbSipMfcEncExe(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle);
            Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameEnd);
        }
    }

EXIT:
    FunctionOut();
    Exynos_OSAL_ThreadExit(NULL);

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;
    OMX_HANDLETYPE                 hMFCHandle = NULL;
    OMX_S32                        returnCodec = 0;

    FunctionIn();

    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_FALSE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;

    /* MFC(Multi Format Codec) encoder and CMM(Codec Memory Management) driver open */
    switch (pExynosInputPort->portDefinition.format.video.eColorFormat) {
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
    pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle = hMFCHandle;

    /* set MFC ENC VIDEO PARAM and initialize MFC encoder instance */
    if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
        Set_Mpeg4Enc_Param(&(pMpeg4Enc->hMFCMpeg4Handle.mpeg4MFCParam), pExynosComponent);
        returnCodec = SsbSipMfcEncInit(hMFCHandle, &(pMpeg4Enc->hMFCMpeg4Handle.mpeg4MFCParam));
    } else {
        Set_H263Enc_Param(&(pMpeg4Enc->hMFCMpeg4Handle.h263MFCParam), pExynosComponent);
        returnCodec = SsbSipMfcEncInit(hMFCHandle, &(pMpeg4Enc->hMFCMpeg4Handle.h263MFCParam));
    }
    if (returnCodec != MFC_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* Allocate encoder's input buffer */
    returnCodec = SsbSipMfcEncGetInBuf(hMFCHandle, &(pMpeg4Enc->hMFCMpeg4Handle.inputInfo));
    if (returnCodec != MFC_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoEnc->MFCEncInputBuffer[0].YPhyAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YPhyAddr;
    pVideoEnc->MFCEncInputBuffer[0].CPhyAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CPhyAddr;
    pVideoEnc->MFCEncInputBuffer[0].YVirAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YVirAddr;
    pVideoEnc->MFCEncInputBuffer[0].CVirAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CVirAddr;
    pVideoEnc->MFCEncInputBuffer[0].YBufferSize = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YSize;
    pVideoEnc->MFCEncInputBuffer[0].CBufferSize = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CSize;
    pVideoEnc->MFCEncInputBuffer[0].YDataSize = 0;
    pVideoEnc->MFCEncInputBuffer[0].CDataSize = 0;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YVirAddr : 0x%x", pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YVirAddr);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CVirAddr : 0x%x", pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CVirAddr);

    returnCodec = SsbSipMfcEncGetInBuf(hMFCHandle, &(pMpeg4Enc->hMFCMpeg4Handle.inputInfo));
    if (returnCodec != MFC_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoEnc->MFCEncInputBuffer[1].YPhyAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YPhyAddr;
    pVideoEnc->MFCEncInputBuffer[1].CPhyAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CPhyAddr;
    pVideoEnc->MFCEncInputBuffer[1].YVirAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YVirAddr;
    pVideoEnc->MFCEncInputBuffer[1].CVirAddr = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CVirAddr;
    pVideoEnc->MFCEncInputBuffer[1].YBufferSize = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YSize;
    pVideoEnc->MFCEncInputBuffer[1].CBufferSize = pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CSize;
    pVideoEnc->MFCEncInputBuffer[1].YDataSize = 0;
    pVideoEnc->MFCEncInputBuffer[1].CDataSize = 0;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YVirAddr : 0x%x", pMpeg4Enc->hMFCMpeg4Handle.inputInfo.YVirAddr);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CVirAddr : 0x%x", pMpeg4Enc->hMFCMpeg4Handle.inputInfo.CVirAddr);

    pVideoEnc->indexInputBuffer = 0;

    pVideoEnc->bFirstFrame = OMX_TRUE;

#ifdef NONBLOCK_MODE_PROCESS
    pVideoEnc->NBEncThread.bExitEncodeThread = OMX_FALSE;
    pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
    Exynos_OSAL_SemaphoreCreate(&(pVideoEnc->NBEncThread.hEncFrameStart));
    Exynos_OSAL_SemaphoreCreate(&(pVideoEnc->NBEncThread.hEncFrameEnd));
    if (OMX_ErrorNone == Exynos_OSAL_ThreadCreate(&pVideoEnc->NBEncThread.hNBEncodeThread,
                                                Exynos_MFC_EncodeThread,
                                                pOMXComponent)) {
        pMpeg4Enc->hMFCMpeg4Handle.returnCodec = MFC_RET_OK;
    }
#endif
    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp = 0;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;
    OMX_HANDLETYPE                 hMFCHandle = NULL;

    FunctionIn();

    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
#ifdef NONBLOCK_MODE_PROCESS
    if (pVideoEnc->NBEncThread.hNBEncodeThread != NULL) {
        pVideoEnc->NBEncThread.bExitEncodeThread = OMX_TRUE;
        Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
        Exynos_OSAL_ThreadTerminate(pVideoEnc->NBEncThread.hNBEncodeThread);
        pVideoEnc->NBEncThread.hNBEncodeThread = NULL;
    }

    if(pVideoEnc->NBEncThread.hEncFrameEnd != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameEnd);
        pVideoEnc->NBEncThread.hEncFrameEnd = NULL;
    }

    if(pVideoEnc->NBEncThread.hEncFrameStart != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameStart);
        pVideoEnc->NBEncThread.hEncFrameStart = NULL;
    }
#endif

    hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    if (hMFCHandle != NULL) {
        SsbSipMfcEncClose(hMFCHandle);
        hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4_Encode_Nonblock(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData, EXYNOS_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_HANDLETYPE                 hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    SSBSIP_MFC_ENC_INPUT_INFO     *pInputInfo = &(pMpeg4Enc->hMFCMpeg4Handle.inputInfo);
    SSBSIP_MFC_ENC_OUTPUT_INFO     outputInfo;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    MFC_ENC_ADDR_INFO              addrInfo;
    OMX_U32                        oneFrameSize = pInputData->dataLen;

    FunctionIn();

    if (pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC == OMX_FALSE) {
        pMpeg4Enc->hMFCMpeg4Handle.returnCodec = SsbSipMfcEncGetOutBuf(hMFCHandle, &outputInfo);
        if (pMpeg4Enc->hMFCMpeg4Handle.returnCodec != MFC_RET_OK)
        {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncGetOutBuf failed, ret:%d", __FUNCTION__, pMpeg4Enc->hMFCMpeg4Handle.returnCodec);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pOutputData->dataBuffer = outputInfo.StrmVirAddr;
        pOutputData->allocSize = outputInfo.headerSize;
        pOutputData->dataLen = outputInfo.headerSize;
        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC = OMX_TRUE;

        ret = OMX_ErrorInputDataEncodeYet;
        goto EXIT;
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pExynosComponent->bUseFlagEOF == OMX_FALSE))
        pExynosComponent->bUseFlagEOF = OMX_TRUE;

    if (oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
        (pExynosComponent->getAllDelayBuffer == OMX_TRUE)){
        /* Dummy input data for get out encoded last frame */
        pInputInfo->YPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YPhyAddr;
        pInputInfo->CPhyAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CPhyAddr;
        pInputInfo->YVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
        pInputInfo->CVirAddr = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
    } else {
        switch (pExynosPort->portDefinition.format.video.eColorFormat) {
        case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV21LPhysicalAddress: {
#ifndef USE_METADATABUFFERTYPE
            /* USE_FIMC_FRAME_BUFFER */
            Exynos_OSAL_Memcpy(&addrInfo.pAddrY, pInputData->dataBuffer, sizeof(addrInfo.pAddrY));
            Exynos_OSAL_Memcpy(&addrInfo.pAddrC, pInputData->dataBuffer + sizeof(addrInfo.pAddrY), sizeof(addrInfo.pAddrC));
#else
            OMX_PTR ppBuf[3];
            Exynos_OSAL_GetInfoFromMetaData(pInputData, ppBuf);

            Exynos_OSAL_Memcpy(&addrInfo.pAddrY, ppBuf[0], sizeof(addrInfo.pAddrY));
            Exynos_OSAL_Memcpy(&addrInfo.pAddrC, ppBuf[1], sizeof(addrInfo.pAddrC));
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

    pExynosComponent->timeStamp[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
    pExynosComponent->nFlags[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pInputData->nFlags;

    if ((pMpeg4Enc->hMFCMpeg4Handle.returnCodec == MFC_RET_OK) &&
        (pVideoEnc->bFirstFrame == OMX_FALSE)) {
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc encode done */
        if (pVideoEnc->NBEncThread.bEncoderRun != OMX_FALSE) {
            Exynos_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameEnd);
            pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
        }

        Exynos_OSAL_SleepMillisec(0);
        pMpeg4Enc->hMFCMpeg4Handle.returnCodec = SsbSipMfcEncGetOutBuf(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, &outputInfo);
        if ((SsbSipMfcEncGetConfig(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, MFC_ENC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))){
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
        }

        if (pMpeg4Enc->hMFCMpeg4Handle.returnCodec == MFC_RET_OK) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = outputInfo.StrmVirAddr;
            pOutputData->allocSize = outputInfo.dataSize;
            pOutputData->dataLen = outputInfo.dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (outputInfo.frameType == MFC_FRAME_TYPE_I_FRAME)
                pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "MFC Encode OK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

            ret = OMX_ErrorNone;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncGetOutBuf failed, ret:%d", __FUNCTION__, pMpeg4Enc->hMFCMpeg4Handle.returnCodec);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        if (pExynosComponent->getAllDelayBuffer == OMX_TRUE) {
            ret = OMX_ErrorInputDataEncodeYet;
        }
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pExynosComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataEncodeYet;
        }
        if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            pOutputData->dataLen = 0;
            pOutputData->usedDataLen = 0;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX_BUFFERFLAG_EOS!!!");
            ret = OMX_ErrorNone;
        }
    }
    if (pMpeg4Enc->hMFCMpeg4Handle.returnCodec != MFC_RET_OK) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "In %s : SsbSipMfcEncExe Failed!!!\n", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pMpeg4Enc->hMFCMpeg4Handle.returnCodec = SsbSipMfcEncSetInBuf(hMFCHandle, pInputInfo);
    if (pMpeg4Enc->hMFCMpeg4Handle.returnCodec != MFC_RET_OK) {
       Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncSetInBuf failed, ret:%d", __FUNCTION__, pMpeg4Enc->hMFCMpeg4Handle.returnCodec);
       ret = OMX_ErrorUndefined;
       goto EXIT;
    } else {
        pVideoEnc->indexInputBuffer++;
        pVideoEnc->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            Change_Mpeg4Enc_Param(&(pMpeg4Enc->hMFCMpeg4Handle.mpeg4MFCParam), pExynosComponent);
        else
            Change_H263Enc_Param(&(pMpeg4Enc->hMFCMpeg4Handle.h263MFCParam), pExynosComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }

    SsbSipMfcEncSetConfig(hMFCHandle, MFC_ENC_SETCONF_FRAME_TAG, &(pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp));

    /* mfc encode start */
    Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
    pVideoEnc->NBEncThread.bEncoderRun = OMX_TRUE;
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp++;
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;
    pVideoEnc->bFirstFrame = OMX_FALSE;
    Exynos_OSAL_SleepMillisec(0);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_MFC_Mpeg4_Encode_Block(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData, EXYNOS_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_HANDLETYPE                 hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    SSBSIP_MFC_ENC_INPUT_INFO     *pInputInfo = &(pMpeg4Enc->hMFCMpeg4Handle.inputInfo);
    SSBSIP_MFC_ENC_OUTPUT_INFO     outputInfo;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    MFC_ENC_ADDR_INFO              addrInfo;
    OMX_U32                        oneFrameSize = pInputData->dataLen;
    OMX_S32                        returnCodec = 0;

    FunctionIn();

    if (pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC == OMX_FALSE) {
        returnCodec = SsbSipMfcEncGetOutBuf(hMFCHandle, &outputInfo);
        if (returnCodec != MFC_RET_OK)
        {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncGetOutBuf failed, ret:%d", __FUNCTION__, returnCodec);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pOutputData->dataBuffer = outputInfo.StrmVirAddr;
        pOutputData->allocSize = outputInfo.headerSize;
        pOutputData->dataLen = outputInfo.headerSize;
        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC = OMX_TRUE;

        ret = OMX_ErrorInputDataEncodeYet;
        goto EXIT;
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pExynosComponent->bUseFlagEOF == OMX_FALSE))
        pExynosComponent->bUseFlagEOF = OMX_TRUE;

    if (oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    switch (pExynosPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress: {
#ifndef USE_METADATABUFFERTYPE
        /* USE_FIMC_FRAME_BUFFER */
        Exynos_OSAL_Memcpy(&addrInfo.pAddrY, pInputData->dataBuffer, sizeof(addrInfo.pAddrY));
        Exynos_OSAL_Memcpy(&addrInfo.pAddrC, pInputData->dataBuffer + sizeof(addrInfo.pAddrY), sizeof(addrInfo.pAddrC));
#else
        OMX_PTR ppBuf[3];
        Exynos_OSAL_GetInfoFromMetaData(pInputData,ppBuf);

        Exynos_OSAL_Memcpy(&addrInfo.pAddrY, ppBuf[0], sizeof(addrInfo.pAddrY));
        Exynos_OSAL_Memcpy(&addrInfo.pAddrC, ppBuf[1], sizeof(addrInfo.pAddrC));
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

    returnCodec = SsbSipMfcEncSetInBuf(hMFCHandle, pInputInfo);
    if (returnCodec != MFC_RET_OK) {
       Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncSetInBuf failed, ret:%d", __FUNCTION__, returnCodec);
       ret = OMX_ErrorUndefined;
       goto EXIT;
    } else {
        pVideoEnc->indexInputBuffer++;
        pVideoEnc->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            Change_Mpeg4Enc_Param(&(pMpeg4Enc->hMFCMpeg4Handle.mpeg4MFCParam), pExynosComponent);
        else
            Change_H263Enc_Param(&(pMpeg4Enc->hMFCMpeg4Handle.h263MFCParam), pExynosComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }

    pExynosComponent->timeStamp[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
    pExynosComponent->nFlags[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pInputData->nFlags;
    SsbSipMfcEncSetConfig(hMFCHandle, MFC_ENC_SETCONF_FRAME_TAG, &(pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp));

    returnCodec = SsbSipMfcEncExe(hMFCHandle);
    if (returnCodec == MFC_RET_OK) {
        OMX_S32 indexTimestamp = 0;

        pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp++;
        pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;

        returnCodec = SsbSipMfcEncGetOutBuf(hMFCHandle, &outputInfo);

        if ((SsbSipMfcEncGetConfig(hMFCHandle, MFC_ENC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
        }

        if (returnCodec == MFC_RET_OK) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = outputInfo.StrmVirAddr;
            pOutputData->allocSize = outputInfo.dataSize;
            pOutputData->dataLen = outputInfo.dataSize;
            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (outputInfo.frameType == MFC_FRAME_TYPE_I_FRAME)
                pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            ret = OMX_ErrorNone;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncGetOutBuf failed, ret:%d", __FUNCTION__, returnCodec);
            ret = OMX_ErrorUndefined;
        }
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: SsbSipMfcEncExe failed, ret:%d", __FUNCTION__, returnCodec);
        ret = OMX_ErrorUndefined;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Encode */
OMX_ERRORTYPE Exynos_MFC_Mpeg4Enc_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData, EXYNOS_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT        *pInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT        *pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pInputPort)) || (!CHECK_PORT_ENABLED(pOutputPort)) ||
            (!CHECK_PORT_POPULATED(pInputPort)) || (!CHECK_PORT_POPULATED(pOutputPort))) {
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent)) {
        goto EXIT;
    }

#ifdef NONBLOCK_MODE_PROCESS
    ret = Exynos_MFC_Mpeg4_Encode_Nonblock(pOMXComponent, pInputData, pOutputData);
#else
    ret = Exynos_MFC_Mpeg4_Encode_Block(pOMXComponent, pInputData, pOutputData);
#endif
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataEncodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                            pExynosComponent->callbackData,
                                            OMX_EventError, ret, 0, NULL);
        }
    } else {
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

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc = NULL;
    OMX_S32                        codecType = -1;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_MPEG4_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_MPEG4;
    } else if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H263_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_H263;
    } else {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, componentName, ret);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_VideoDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_ENC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg4Enc = Exynos_OSAL_Malloc(sizeof(EXYNOS_MPEG4ENC_HANDLE));
    if (pMpeg4Enc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: EXYNOS_MPEG4ENC_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pMpeg4Enc, 0, sizeof(EXYNOS_MPEG4ENC_HANDLE));
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pMpeg4Enc;
    pMpeg4Enc->hMFCMpeg4Handle.codecType = codecType;

    if (codecType == CODEC_TYPE_MPEG4)
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_MPEG4_ENC);
    else
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H263_ENC);

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
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate= (15 << 16);
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate= (15 << 16);
    if (codecType == CODEC_TYPE_MPEG4) {
        pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/mpeg4");
    } else {
        pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
        Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/h263");
    }
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    if (codecType == CODEC_TYPE_MPEG4) {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->mpeg4Component[i], OMX_VIDEO_PARAM_MPEG4TYPE);
            pMpeg4Enc->mpeg4Component[i].nPortIndex = i;
            pMpeg4Enc->mpeg4Component[i].eProfile   = OMX_VIDEO_MPEG4ProfileSimple;
            pMpeg4Enc->mpeg4Component[i].eLevel     = OMX_VIDEO_MPEG4Level4;

            pMpeg4Enc->mpeg4Component[i].nPFrames = 10;
            pMpeg4Enc->mpeg4Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->mpeg4Component[i].nMaxPacketSize = 256;  /* Default value */
            pMpeg4Enc->mpeg4Component[i].nAllowedPictureTypes =  OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->mpeg4Component[i].bGov = OMX_FALSE;

        }
    } else {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->h263Component[i], OMX_VIDEO_PARAM_H263TYPE);
            pMpeg4Enc->h263Component[i].nPortIndex = i;
            pMpeg4Enc->h263Component[i].eProfile   = OMX_VIDEO_H263ProfileBaseline;
            pMpeg4Enc->h263Component[i].eLevel     = OMX_VIDEO_H263Level45;

            pMpeg4Enc->h263Component[i].nPFrames = 20;
            pMpeg4Enc->h263Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->h263Component[i].bPLUSPTYPEAllowed = OMX_FALSE;
            pMpeg4Enc->h263Component[i].nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->h263Component[i].bForceRoundingTypeToZero = OMX_TRUE;
            pMpeg4Enc->h263Component[i].nPictureHeaderRepetition = 0;
            pMpeg4Enc->h263Component[i].nGOBHeaderInterval = 0;
        }
    }

    pOMXComponent->GetParameter      = &Exynos_MFC_Mpeg4Enc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_MFC_Mpeg4Enc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_MFC_Mpeg4Enc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_MFC_Mpeg4Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_MFC_Mpeg4Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_MFC_Mpeg4Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_mfc_componentInit      = &Exynos_MFC_Mpeg4Enc_Init;
    pExynosComponent->exynos_mfc_componentTerminate = &Exynos_MFC_Mpeg4Enc_Terminate;
    pExynosComponent->exynos_mfc_bufferProcess      = &Exynos_MFC_Mpeg4Enc_bufferProcess;
    pExynosComponent->exynos_checkInputFrame        = NULL;

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    EXYNOS_MPEG4ENC_HANDLE     *pMpeg4Enc = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    if (pMpeg4Enc != NULL) {
        Exynos_OSAL_Free(pMpeg4Enc);
        pMpeg4Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
