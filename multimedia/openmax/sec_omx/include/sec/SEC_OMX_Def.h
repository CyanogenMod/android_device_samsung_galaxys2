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
 * @file    SEC_OMX_Def.h
 * @brief   SEC_OMX specific define
 * @author  SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OMX_DEF
#define SEC_OMX_DEF

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

#define SEC_OMX_INSTALL_PATH "/system/lib/omx/"

typedef enum _SEC_CODEC_TYPE
{
    SW_CODEC,
    HW_VIDEO_DEC_CODEC,
    HW_VIDEO_ENC_CODEC,
    HW_AUDIO_DEC_CODEC,
    HW_AUDIO_ENC_CODEC
} SEC_CODEC_TYPE;

typedef struct _SEC_OMX_PRIORITYMGMTTYPE
{
    OMX_U32 nGroupPriority; /* the value 0 represents the highest priority */
                            /* for a group of components                   */
    OMX_U32 nGroupID;
} SEC_OMX_PRIORITYMGMTTYPE;

typedef enum _SEC_OMX_INDEXTYPE
{
#define SEC_INDEX_PARAM_ENABLE_THUMBNAIL "OMX.SEC.index.ThumbnailMode"
    OMX_IndexVendorThumbnailMode        = 0x7F000001,
#define SEC_INDEX_CONFIG_VIDEO_INTRAPERIOD "OMX.SEC.index.VideoIntraPeriod"
    OMX_IndexConfigVideoIntraPeriod     = 0x7F000002,

    /* for Android Native Window */
#define SEC_INDEX_PARAM_ENABLE_ANB "OMX.google.android.index.enableAndroidNativeBuffers"
    OMX_IndexParamEnableAndroidBuffers    = 0x7F000011,
#define SEC_INDEX_PARAM_GET_ANB "OMX.google.android.index.getAndroidNativeBufferUsage"
    OMX_IndexParamGetAndroidNativeBuffer  = 0x7F000012,
#define SEC_INDEX_PARAM_USE_ANB "OMX.google.android.index.useAndroidNativeBuffer"
    OMX_IndexParamUseAndroidNativeBuffer  = 0x7F000013,
    /* for Android Store Metadata Inbuffer */
#define SEC_INDEX_PARAM_STORE_METADATA_BUFFER "OMX.google.android.index.storeMetaDataInBuffers"
    OMX_IndexParamStoreMetaDataBuffer     = 0x7F000014,

    /* for Android PV OpenCore*/
    OMX_COMPONENT_CAPABILITY_TYPE_INDEX = 0xFF7A347
} SEC_OMX_INDEXTYPE;

typedef enum _SEC_OMX_ERRORTYPE
{
    OMX_ErrorNoEOF = (OMX_S32) 0x90000001,
    OMX_ErrorInputDataDecodeYet = (OMX_S32) 0x90000002,
    OMX_ErrorInputDataEncodeYet = (OMX_S32) 0x90000003,
    OMX_ErrorMFCInit = (OMX_S32) 0x90000004
} SEC_OMX_ERRORTYPE;

typedef enum _SEC_OMX_COMMANDTYPE
{
    SEC_OMX_CommandComponentDeInit = 0x7F000001,
    SEC_OMX_CommandEmptyBuffer,
    SEC_OMX_CommandFillBuffer
} SEC_OMX_COMMANDTYPE;

typedef enum _SEC_OMX_TRANS_STATETYPE {
    SEC_OMX_TransStateInvalid,
    SEC_OMX_TransStateLoadedToIdle,
    SEC_OMX_TransStateIdleToExecuting,
    SEC_OMX_TransStateExecutingToIdle,
    SEC_OMX_TransStateIdleToLoaded,
    SEC_OMX_TransStateMax = 0X7FFFFFFF
} SEC_OMX_TRANS_STATETYPE;

typedef enum _SEC_OMX_COLOR_FORMATTYPE {
    OMX_SEC_COLOR_FormatNV12TPhysicalAddress = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
    OMX_SEC_COLOR_FormatNV12LPhysicalAddress = 0x7F000002,
    OMX_SEC_COLOR_FormatNV12LVirtualAddress = 0x7F000003,
    OMX_SEC_COLOR_FormatNV12Tiled            = 0x7FC00002,  /* 0x7FC00002 */
#ifdef S3D_SUPPORT
    OMX_SEC_COLOR_FormatNV12Tiled_SBS_LR     = 0x7FC00003,  /* 0x7FC00003 */
    OMX_SEC_COLOR_FormatNV12Tiled_SBS_RL     = 0x7FC00004,  /* 0x7FC00004 */
    OMX_SEC_COLOR_FormatNV12Tiled_TB_LR     = 0x7FC00005,  /* 0x7FC00005 */
    OMX_SEC_COLOR_FormatNV12Tiled_TB_RL   = 0x7FC00006,  /* 0x7FC00006 */
    OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_LR     = 0x7FC00007,  /* 0x7FC00007 */
    OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_RL     = 0x7FC00008,  /* 0x7FC00008 */
    OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_LR     = 0x7FC00009,  /* 0x7FC00009 */
    OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_RL   = 0x7FC0000A,  /* 0x7FC0000A */
    OMX_SEC_COLOR_FormatYUV420Planar_SBS_LR     = 0x7FC0000B,  /* 0x7FC0000B */
    OMX_SEC_COLOR_FormatYUV420Planar_SBS_RL     = 0x7FC0000C,  /* 0x7FC0000C */
    OMX_SEC_COLOR_FormatYUV420Planar_TB_LR     = 0x7FC0000D,  /* 0x7FC0000D */
    OMX_SEC_COLOR_FormatYUV420Planar_TB_RL   = 0x7FC0000E,  /* 0x7FC0000E */
#endif
    OMX_SEC_COLOR_FormatNV21LPhysicalAddress = 0x7F000010,
    OMX_SEC_COLOR_FormatNV21Linear           = 0x7F000011,

    /* for Android Native Window */
    OMX_SEC_COLOR_FormatANBYUV420SemiPlanar  = 0x100,
    /* for Android SurfaceMediaSource*/
    OMX_COLOR_FormatAndroidOpaque            = 0x7F000789
}SEC_OMX_COLOR_FORMATTYPE;

typedef enum _SEC_OMX_SUPPORTFORMAT_TYPE
{
    supportFormat_0 = 0x00,
    supportFormat_1,
    supportFormat_2,
    supportFormat_3,
    supportFormat_4,
    supportFormat_5,
    supportFormat_6,
    supportFormat_7,
    supportFormat_8
} SEC_OMX_SUPPORTFORMAT_TYPE;

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

typedef struct _SEC_OMX_VIDEO_PROFILELEVEL
{
    OMX_S32  profile;
    OMX_S32  level;
} SEC_OMX_VIDEO_PROFILELEVEL;

#define OMX_VIDEO_CodingVPX     0x09    /**< Google VPX, formerly known as On2 VP8 */

#ifndef __OMX_EXPORTS
#define __OMX_EXPORTS
#define SEC_EXPORT_REF __attribute__((visibility("default")))
#define SEC_IMPORT_REF __attribute__((visibility("default")))
#endif

#endif
