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
 * @file    Exynos_OMX_Def.h
 * @brief   Exynos_OMX specific define
 * @author  SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef EXYNOS_OMX_DEF
#define EXYNOS_OMX_DEF

#include "OMX_Types.h"
#include "OMX_IVCommon.h"

#define VERSIONMAJOR_NUMBER                1
#define VERSIONMINOR_NUMBER                0
#define REVISION_NUMBER                    0
#define STEP_NUMBER                        0


#define MAX_OMX_COMPONENT_NUM              20
#define MAX_OMX_COMPONENT_ROLE_NUM         10
#define MAX_OMX_COMPONENT_NAME_SIZE        OMX_MAX_STRINGNAME_SIZE
#define MAX_OMX_COMPONENT_ROLE_SIZE        OMX_MAX_STRINGNAME_SIZE
#define MAX_OMX_COMPONENT_LIBNAME_SIZE     OMX_MAX_STRINGNAME_SIZE * 2
#define MAX_OMX_MIMETYPE_SIZE              OMX_MAX_STRINGNAME_SIZE

#define MAX_TIMESTAMP        17
#define MAX_FLAGS            17

#define EXYNOS_OMX_INSTALL_PATH "/system/lib/omx/"

typedef enum _EXYNOS_CODEC_TYPE
{
    SW_CODEC,
    HW_VIDEO_DEC_CODEC,
    HW_VIDEO_ENC_CODEC,
    HW_AUDIO_DEC_CODEC,
    HW_AUDIO_ENC_CODEC
} EXYNOS_CODEC_TYPE;

typedef struct _EXYNOS_OMX_PRIORITYMGMTTYPE
{
    OMX_U32 nGroupPriority; /* the value 0 represents the highest priority */
                            /* for a group of components                   */
    OMX_U32 nGroupID;
} EXYNOS_OMX_PRIORITYMGMTTYPE;

typedef enum _EXYNOS_OMX_INDEXTYPE
{
#define EXYNOS_INDEX_PARAM_ENABLE_THUMBNAIL "OMX.SEC.index.ThumbnailMode"
    OMX_IndexVendorThumbnailMode        = 0x7F000001,
#define EXYNOS_INDEX_CONFIG_VIDEO_INTRAPERIOD "OMX.SEC.index.VideoIntraPeriod"
    OMX_IndexConfigVideoIntraPeriod     = 0x7F000002,

    /* for Android Native Window */
#define EXYNOS_INDEX_PARAM_ENABLE_ANB "OMX.google.android.index.enableAndroidNativeBuffers"
    OMX_IndexParamEnableAndroidBuffers    = 0x7F000011,
#define EXYNOS_INDEX_PARAM_GET_ANB "OMX.google.android.index.getAndroidNativeBufferUsage"
    OMX_IndexParamGetAndroidNativeBuffer  = 0x7F000012,
#define EXYNOS_INDEX_PARAM_USE_ANB "OMX.google.android.index.useAndroidNativeBuffer"
    OMX_IndexParamUseAndroidNativeBuffer  = 0x7F000013,
    /* for Android Store Metadata Inbuffer */
#define EXYNOS_INDEX_PARAM_STORE_METADATA_BUFFER "OMX.google.android.index.storeMetaDataInBuffers"
    OMX_IndexParamStoreMetaDataBuffer     = 0x7F000014,

    /* for Android PV OpenCore*/
    OMX_COMPONENT_CAPABILITY_TYPE_INDEX = 0xFF7A347
} EXYNOS_OMX_INDEXTYPE;

typedef enum _EXYNOS_OMX_ERRORTYPE
{
    OMX_ErrorNoEOF = (OMX_S32) 0x90000001,
    OMX_ErrorInputDataDecodeYet = (OMX_S32) 0x90000002,
    OMX_ErrorInputDataEncodeYet = (OMX_S32) 0x90000003,
    OMX_ErrorMFCInit = (OMX_S32) 0x90000004
} EXYNOS_OMX_ERRORTYPE;

typedef enum _EXYNOS_OMX_COMMANDTYPE
{
    EXYNOS_OMX_CommandComponentDeInit = 0x7F000001,
    EXYNOS_OMX_CommandEmptyBuffer,
    EXYNOS_OMX_CommandFillBuffer
} EXYNOS_OMX_COMMANDTYPE;

typedef enum _EXYNOS_OMX_TRANS_STATETYPE {
    EXYNOS_OMX_TransStateInvalid,
    EXYNOS_OMX_TransStateLoadedToIdle,
    EXYNOS_OMX_TransStateIdleToExecuting,
    EXYNOS_OMX_TransStateExecutingToIdle,
    EXYNOS_OMX_TransStateIdleToLoaded,
    EXYNOS_OMX_TransStateMax = 0X7FFFFFFF
} EXYNOS_OMX_TRANS_STATETYPE;

typedef enum _EXYNOS_OMX_COLOR_FORMATTYPE {
    OMX_SEC_COLOR_FormatNV12TPhysicalAddress = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
    OMX_SEC_COLOR_FormatNV12LPhysicalAddress = 0x7F000002,
    OMX_SEC_COLOR_FormatNV12LVirtualAddress  = 0x7F000003,
    OMX_SEC_COLOR_FormatNV12Tiled            = 0x7FC00002,  /* 0x7FC00002 */
    OMX_SEC_COLOR_FormatNV21LPhysicalAddress = 0x7F000010,
    OMX_SEC_COLOR_FormatNV21Linear           = 0x7F000011,

    /* for Android Native Window */
    OMX_SEC_COLOR_FormatANBYUV420SemiPlanar  = 0x100,
    /* for Android SurfaceMediaSource*/
    OMX_COLOR_FormatAndroidOpaque            = 0x7F000789
}EXYNOS_OMX_COLOR_FORMATTYPE;

typedef enum _EXYNOS_OMX_SUPPORTFORMAT_TYPE
{
    supportFormat_0 = 0x00,
    supportFormat_1,
    supportFormat_2,
    supportFormat_3,
    supportFormat_4,
    supportFormat_5,
    supportFormat_6,
    supportFormat_7
} EXYNOS_OMX_SUPPORTFORMAT_TYPE;

/* for Android PV OpenCore*/
typedef struct _OMXComponentCapabilityFlagsType
{
    /* OMX COMPONENT CAPABILITY RELATED MEMBERS */
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
    OMX_BOOL iOMXComponentSupportsPartialFrames;
    OMX_BOOL iOMXComponentUsesNALStartCodes;
    OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
    OMX_BOOL iOMXComponentUsesFullAVCFrames;
} OMXComponentCapabilityFlagsType;

typedef struct _EXYNOS_OMX_VIDEO_PROFILELEVEL
{
    OMX_S32  profile;
    OMX_S32  level;
} EXYNOS_OMX_VIDEO_PROFILELEVEL;

#define OMX_VIDEO_CodingVPX     0x09    /**< Google VPX, formerly known as On2 VP8 */

#ifndef __OMX_EXPORTS
#define __OMX_EXPORTS
#define EXYNOS_EXPORT_REF __attribute__((visibility("default")))
#define EXYNOS_IMPORT_REF __attribute__((visibility("default")))
#endif

#endif
