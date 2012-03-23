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
 * @file        SEC_OMX_Mpeg4enc.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OMX_MPEG4_ENC_COMPONENT
#define SEC_OMX_MPEG4_ENC_COMPONENT

#include "SEC_OMX_Def.h"
#include "OMX_Component.h"
#include "SsbSipMfcApi.h"


typedef enum _CODEC_TYPE
{
    CODEC_TYPE_H263,
    CODEC_TYPE_MPEG4
} CODEC_TYPE;

typedef struct _SEC_MFC_MPEG4ENC_HANDLE
{
    OMX_HANDLETYPE             hMFCHandle;
    SSBSIP_MFC_ENC_MPEG4_PARAM mpeg4MFCParam;
    SSBSIP_MFC_ENC_H263_PARAM  h263MFCParam;
    SSBSIP_MFC_ENC_INPUT_INFO  inputInfo;
    OMX_U32                    indexTimestamp;
    OMX_BOOL                   bConfiguredMFC;
    CODEC_TYPE                 codecType;
    OMX_S32                    returnCodec;
} SEC_MFC_MPEG4ENC_HANDLE;

typedef struct _SEC_MPEG4ENC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_H263TYPE  h263Component[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_MPEG4TYPE mpeg4Component[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* SEC MFC Codec specific */
    SEC_MFC_MPEG4ENC_HANDLE   hMFCMpeg4Handle;
} SEC_MPEG4ENC_HANDLE;


#ifdef __cplusplus
extern "C" {
#endif


OSCL_EXPORT_REF OMX_ERRORTYPE SEC_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE SEC_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif
