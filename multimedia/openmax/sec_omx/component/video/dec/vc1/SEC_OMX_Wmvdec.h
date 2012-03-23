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
 * @file    SEC_OMX_Wmvdec.h
 * @brief
 * @author    HyeYeon Chung (hyeon.chung@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.8.20 : Create
 */

#ifndef SEC_OMX_WMV_DEC_COMPONENT
#define SEC_OMX_WMV_DEC_COMPONENT

#include "SEC_OMX_Def.h"
#include "OMX_Component.h"

#define BITMAPINFOHEADER_SIZE               40
#define BITMAPINFOHEADER_ASFBINDING_SIZE    41
#define COMPRESSION_POS                     16

typedef enum WMV_FORMAT {
    WMV_FORMAT_VC1,
    WMV_FORMAT_WMV3,
    WMV_FORMAT_UNKNOWN
} WMV_FORMAT;

/*
 * This structure is the same as BitmapInfoHhr struct in pv_avifile_typedefs.h file
 */
typedef struct _BitmapInfoHhr
{
    OMX_U32    BiSize;
    OMX_U32    BiWidth;
    OMX_U32    BiHeight;
    OMX_U16    BiPlanes;
    OMX_U16    BiBitCount;
    OMX_U32    BiCompression;
    OMX_U32    BiSizeImage;
    OMX_U32    BiXPelsPerMeter;
    OMX_U32    BiYPelsPerMeter;
    OMX_U32    BiClrUsed;
    OMX_U32    BiClrImportant;
} BitmapInfoHhr;

typedef struct _SEC_MFC_WMV_HANDLE
{
    OMX_HANDLETYPE hMFCHandle;
    OMX_PTR        pMFCStreamBuffer;
    OMX_PTR        pMFCStreamPhyBuffer;
    OMX_U32        indexTimestamp;
    OMX_U32        outputIndexTimestamp;
    OMX_BOOL       bConfiguredMFC;
    WMV_FORMAT     wmvFormat;
    OMX_S32        returnCodec;
} SEC_MFC_WMV_HANDLE;

typedef struct _SEC_WMV_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_WMVTYPE WmvComponent[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* SEC MFC Codec specific */
    SEC_MFC_WMV_HANDLE      hMFCWmvHandle;
} SEC_WMV_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE SEC_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE SEC_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif
