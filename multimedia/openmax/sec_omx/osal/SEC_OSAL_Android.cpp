/*
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
 * @file        SEC_OSAL_Android.cpp
 * @brief
 * @author      Seungbeom Kim (sbcrux.kim@samsung.com)
 * @author      Hyeyeon Chung (hyeon.chung@samsung.com)
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.1.0
 * @history
 *   2011.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>

#include <ui/android_native_buffer.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <media/stagefright/HardwareAPI.h>
#include <hardware/hardware.h>
#include <media/stagefright/MetadataBufferType.h>

#include "gralloc_priv.h"

#include "SEC_OSAL_Semaphore.h"
#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OMX_Macros.h"
#include "SEC_OMX_Vdec.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_OSAL_Android"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE useAndroidNativeBuffer(
    SEC_OMX_BASEPORT      *pSECPort,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32                nPortIndex,
    OMX_PTR                pAppPrivate,
    OMX_U32                nSizeBytes,
    OMX_U8                *pBuffer)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *temp_bufferHeader = NULL;
    unsigned int          i = 0;

    FunctionIn();

    if (pSECPort == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECPort->portState != OMX_StateIdle) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)SEC_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    SEC_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pSECPort->portDefinition.nBufferCountActual; i++) {
        if (pSECPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pSECPort->bufferHeader[i] = temp_bufferHeader;
            pSECPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = pBuffer;
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;

            pSECPort->assignedBufferNum++;
            if (pSECPort->assignedBufferNum == pSECPort->portDefinition.nBufferCountActual) {
                pSECPort->portDefinition.bPopulated = OMX_TRUE;
                /* SEC_OSAL_MutexLock(pSECComponent->compMutex); */
                SEC_OSAL_SemaphorePost(pSECPort->loadedResource);
                /* SEC_OSAL_MutexUnlock(pSECComponent->compMutex); */
            }
            *ppBufferHdr = temp_bufferHeader;
            goto EXIT;
        }
    }

    SEC_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_LockANBHandle(
    OMX_IN OMX_U32 handle,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_PTR *vaddr)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    Rect bounds(width, height);

    SEC_OSAL_Log(SEC_LOG_TRACE, "%s: handle: 0x%x", __func__, handle);

    int usage = 0;

    switch (format) {
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
#ifdef S3D_SUPPORT
    case OMX_SEC_COLOR_FormatNV12Tiled_SBS_LR:
    case OMX_SEC_COLOR_FormatNV12Tiled_SBS_RL:
    case OMX_SEC_COLOR_FormatNV12Tiled_TB_LR:
    case OMX_SEC_COLOR_FormatNV12Tiled_TB_RL:
    case OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_LR:
    case OMX_SEC_COLOR_FormatYUV420SemiPlanar_SBS_RL:
    case OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_LR:
    case OMX_SEC_COLOR_FormatYUV420SemiPlanar_TB_RL:
    case OMX_SEC_COLOR_FormatYUV420Planar_SBS_LR:
    case OMX_SEC_COLOR_FormatYUV420Planar_SBS_RL:
    case OMX_SEC_COLOR_FormatYUV420Planar_TB_LR:
    case OMX_SEC_COLOR_FormatYUV420Planar_TB_RL:
#endif
    case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_YUV_ADDR;
        break;
    default:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    }

    if (mapper.lock(bufferHandle, usage, bounds, vaddr) != 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: mapper.lock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    SEC_OSAL_Log(SEC_LOG_TRACE, "%s: buffer locked: 0x%x", __func__, *vaddr);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_UnlockANBHandle(OMX_IN OMX_U32 handle)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;

    SEC_OSAL_Log(SEC_LOG_TRACE, "%s: handle: 0x%x", __func__, handle);

    if (mapper.unlock(bufferHandle) != 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: mapper.unlock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    SEC_OSAL_Log(SEC_LOG_TRACE, "%s: buffer unlocked: 0x%x", __func__, handle);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_GetPhysANBHandle(
    OMX_IN OMX_U32 handle,
    OMX_OUT OMX_PTR *paddr)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;

    SEC_OSAL_Log(SEC_LOG_TRACE, "%s: handle: 0x%x", __func__, handle);

    if (mapper.getphys(bufferHandle, paddr) != 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: mapper.getphys() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_LockANB(
    OMX_IN OMX_PTR pBuffer,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_U32 *pStride,
    OMX_OUT OMX_PTR *vaddr)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    android_native_buffer_t *pANB = (android_native_buffer_t *) pBuffer;

    ret = SEC_OSAL_LockANBHandle((OMX_U32)pANB->handle, width, height, format, vaddr);
    *pStride = pANB->stride;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_UnlockANB(OMX_IN OMX_PTR pBuffer)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    android_native_buffer_t *pANB = (android_native_buffer_t *) pBuffer;

    ret = SEC_OSAL_UnlockANBHandle((OMX_U32)pANB->handle);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_GetPhysANB(
    OMX_IN OMX_PTR pBuffer,
    OMX_OUT OMX_PTR *paddr)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    android_native_buffer_t *pANB = (android_native_buffer_t *) pBuffer;

    ret = SEC_OSAL_GetPhysANBHandle((OMX_U32)pANB->handle, paddr);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_GetANBParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
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
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        GetAndroidNativeBufferUsageParams *pANBParams = (GetAndroidNativeBufferUsageParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;

        SEC_OSAL_Log(SEC_LOG_TRACE, "%s: OMX_IndexParamGetAndroidNativeBuffer", __func__);

        ret = SEC_OMX_Check_SizeVersion(pANBParams, sizeof(GetAndroidNativeBufferUsageParams));
        if (ret != OMX_ErrorNone) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_OMX_Check_SizeVersion(GetAndroidNativeBufferUsageParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /* NOTE: OMX_IndexParamGetAndroidNativeBuffer returns original 'nUsage' without any
         * modifications since currently not defined what the 'nUsage' is for.
         */
        pANBParams->nUsage |= 0;
    }
        break;

    default:
    {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_SetANBParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;

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
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;

    switch (nIndex) {
    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *pANBParams = (EnableAndroidNativeBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        SEC_OMX_BASEPORT *pSECPort = NULL;

        SEC_OSAL_Log(SEC_LOG_TRACE, "%s: OMX_IndexParamEnableAndroidNativeBuffers", __func__);

        ret = SEC_OMX_Check_SizeVersion(pANBParams, sizeof(EnableAndroidNativeBuffersParams));
        if (ret != OMX_ErrorNone) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_OMX_Check_SizeVersion(EnableAndroidNativeBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];
        if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSECPort->bIsANBEnabled = pANBParams->enable;
    }
        break;

    case OMX_IndexParamUseAndroidNativeBuffer:
    {
        UseAndroidNativeBufferParams *pANBParams = (UseAndroidNativeBufferParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        SEC_OMX_BASEPORT *pSECPort = NULL;
        android_native_buffer_t *pANB;
        OMX_U32 nSizeBytes;

        SEC_OSAL_Log(SEC_LOG_TRACE, "%s: OMX_IndexParamUseAndroidNativeBuffer, portIndex: %d", __func__, portIndex);

        ret = SEC_OMX_Check_SizeVersion(pANBParams, sizeof(UseAndroidNativeBufferParams));
        if (ret != OMX_ErrorNone) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_OMX_Check_SizeVersion(UseAndroidNativeBufferParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];
        if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (pSECPort->portState != OMX_StateIdle) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: Port state should be IDLE", __func__);
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        pANB = pANBParams->nativeBuffer.get();

        /* MALI alignment restriction */
        nSizeBytes = ALIGN(pANB->width, 16) * ALIGN(pANB->height, 16);
        nSizeBytes += ALIGN(pANB->width / 2, 16) * ALIGN(pANB->height / 2, 16) * 2;

        ret = useAndroidNativeBuffer(pSECPort,
                                     pANBParams->bufferHeader,
                                     pANBParams->nPortIndex,
                                     pANBParams->pAppPrivate,
                                     nSizeBytes,
                                     (OMX_U8 *) pANB);
        if (ret != OMX_ErrorNone) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: useAndroidNativeBuffer is failed", __func__);
            goto EXIT;
        }
    }
        break;

    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pANBParams = (StoreMetaDataInBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        SEC_OMX_BASEPORT *pSECPort = NULL;

        SEC_OSAL_Log(SEC_LOG_TRACE, "%s: OMX_IndexParamStoreMetaDataBuffer", __func__);

        ret = SEC_OMX_Check_SizeVersion(pANBParams, sizeof(StoreMetaDataInBuffersParams));
        if (ret != OMX_ErrorNone) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_OMX_Check_SizeVersion(StoreMetaDataInBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];
        if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSECPort->bStoreMetaData = pANBParams->bStoreMetaData;
    }
        break;

    default:
    {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OSAL_GetInfoFromMetaData(OMX_IN SEC_OMX_DATA *pBuffer,
                                           OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    MetadataBufferType type;
    buffer_handle_t    pBufHandle;

    FunctionIn();

/*
 * meta data contains the following data format.
 * payload depends on the MetadataBufferType
 * --------------------------------------------------------------
 * | MetadataBufferType                         |          payload                           |
 * --------------------------------------------------------------
 *
 * If MetadataBufferType is kMetadataBufferTypeCameraSource, then
 * --------------------------------------------------------------
 * | kMetadataBufferTypeCameraSource  | physical addr. of Y |physical addr. of CbCr |
 * --------------------------------------------------------------
 *
 * If MetadataBufferType is kMetadataBufferTypeGrallocSource, then
 * --------------------------------------------------------------
 * | kMetadataBufferTypeGrallocSource    | buffer_handle_t |
 * --------------------------------------------------------------
 */

    /* MetadataBufferType */
    memcpy(&type, (MetadataBufferType *)(pBuffer->dataBuffer), sizeof(type));

    if (type == kMetadataBufferTypeCameraSource) {
        /* physical addr. of Y */
        ppBuf[0] = (OMX_PTR)(pBuffer->dataBuffer + sizeof(type));
        /* physical addr. of CbCr */
        ppBuf[1] = (OMX_PTR)(pBuffer->dataBuffer + sizeof(type) + sizeof(pBuffer->dataBuffer));
    } else if (type == kMetadataBufferTypeGrallocSource) {
        /* buffer_handle_t */
        memcpy(&pBufHandle, pBuffer->dataBuffer + sizeof(type), sizeof(buffer_handle_t));
        ppBuf[0] = (OMX_PTR)pBufHandle;
    }

EXIT:
    FunctionOut();

    return ret;
}

#ifdef __cplusplus
}
#endif
