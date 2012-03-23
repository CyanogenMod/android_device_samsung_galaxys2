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
 * @file        SEC_OMX_Vdec.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              HyeYeon Chung (hyeon.chung@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SEC_OMX_Macros.h"
#include "SEC_OSAL_Event.h"
#include "SEC_OMX_Vdec.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OSAL_Thread.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Mutex.h"
#include "SEC_OSAL_ETC.h"

#ifdef USE_ANB
#include "SEC_OSAL_Android.h"
#endif

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_VIDEO_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"


inline void SEC_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent)
{
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *secInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT      *secOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];

    if ((secOutputPort->portDefinition.format.video.nFrameWidth !=
            secInputPort->portDefinition.format.video.nFrameWidth) ||
        (secOutputPort->portDefinition.format.video.nFrameHeight !=
            secInputPort->portDefinition.format.video.nFrameHeight)) {
        OMX_U32 width = 0, height = 0;

        secOutputPort->portDefinition.format.video.nFrameWidth =
            secInputPort->portDefinition.format.video.nFrameWidth;
        secOutputPort->portDefinition.format.video.nFrameHeight =
            secInputPort->portDefinition.format.video.nFrameHeight;
        width = secOutputPort->portDefinition.format.video.nStride =
            secInputPort->portDefinition.format.video.nStride;
        height = secOutputPort->portDefinition.format.video.nSliceHeight =
            secInputPort->portDefinition.format.video.nSliceHeight;

        switch(secOutputPort->portDefinition.format.video.eColorFormat) {
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
            if (width && height)
                secOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
            break;
        case OMX_SEC_COLOR_FormatNV12Tiled:
            width = secOutputPort->portDefinition.format.video.nFrameWidth;
            height = secOutputPort->portDefinition.format.video.nFrameHeight;
            if (width && height) {
                secOutputPort->portDefinition.nBufferSize =
                    ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height)) \
                    + ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height/2));
            }
            break;
        default:
            if (width && height)
                secOutputPort->portDefinition.nBufferSize = width * height * 2;
            break;
        }
    }

  return ;
}

OMX_ERRORTYPE SEC_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U32                i = 0;

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

    pSECPort = &pSECComponent->pSECPort[nPortIndex];
    if (nPortIndex >= pSECComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
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
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    SEC_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    OMX_U32                i = 0;

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
    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;

    pSECPort = &pSECComponent->pSECPort[nPortIndex];
    if (nPortIndex >= pSECComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
/*
    if (pSECPort->portState != OMX_StateIdle ) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
*/
    if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pVideoDec->bDRMPlayerMode == OMX_TRUE) && (nPortIndex == INPUT_PORT_INDEX)) {
        ret = pSECComponent->sec_allocSecureInputBuffer(hComponent, sizeof(OMX_U8) * nSizeBytes, &temp_buffer);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    } else {
        temp_buffer = SEC_OSAL_Malloc(sizeof(OMX_U8) * nSizeBytes);
        if (temp_buffer == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)SEC_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        if ((pVideoDec->bDRMPlayerMode == OMX_TRUE) && (nPortIndex == INPUT_PORT_INDEX))
            pSECComponent->sec_freeSecureInputBuffer(hComponent, temp_buffer);
        else
            SEC_OSAL_Free(temp_buffer);

        temp_buffer = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    SEC_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pSECPort->portDefinition.nBufferCountActual; i++) {
        if (pSECPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pSECPort->bufferHeader[i] = temp_bufferHeader;
            pSECPort->bufferStateAllocate[i] = (BUFFER_STATE_ALLOCATED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = temp_buffer;
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
            *ppBuffer = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    SEC_OSAL_Free(temp_bufferHeader);
    if ((pVideoDec->bDRMPlayerMode == OMX_TRUE) && (nPortIndex == INPUT_PORT_INDEX))
        pSECComponent->sec_freeSecureInputBuffer(hComponent, temp_buffer);
    else
        SEC_OSAL_Free(temp_buffer);

    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    OMX_U32                i = 0;

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
    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    pSECPort = &pSECComponent->pSECPort[nPortIndex];

    if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pSECPort->portState != OMX_StateLoaded) && (pSECPort->portState != OMX_StateInvalid)) {
        (*(pSECComponent->pCallbacks->EventHandler)) (pOMXComponent,
                        pSECComponent->callbackData,
                        (OMX_U32)OMX_EventError,
                        (OMX_U32)OMX_ErrorPortUnpopulated,
                        nPortIndex, NULL);
    }

    for (i = 0; i < pSECPort->portDefinition.nBufferCountActual; i++) {
        if (((pSECPort->bufferStateAllocate[i] | BUFFER_STATE_FREE) != 0) && (pSECPort->bufferHeader[i] != NULL)) {
            if (pSECPort->bufferHeader[i]->pBuffer == pBufferHdr->pBuffer) {
                if (pSECPort->bufferStateAllocate[i] & BUFFER_STATE_ALLOCATED) {
                    if ((pVideoDec->bDRMPlayerMode == OMX_TRUE) && (nPortIndex == INPUT_PORT_INDEX))
                        pSECComponent->sec_freeSecureInputBuffer(hComponent, pSECPort->bufferHeader[i]->pBuffer);
                    else
                        SEC_OSAL_Free(pSECPort->bufferHeader[i]->pBuffer);
                    pSECPort->bufferHeader[i]->pBuffer = NULL;
                    pBufferHdr->pBuffer = NULL;
                } else if (pSECPort->bufferStateAllocate[i] & BUFFER_STATE_ASSIGNED) {
                    ; /* None*/
                }
                pSECPort->assignedBufferNum--;
                if (pSECPort->bufferStateAllocate[i] & HEADER_STATE_ALLOCATED) {
                    SEC_OSAL_Free(pSECPort->bufferHeader[i]);
                    pSECPort->bufferHeader[i] = NULL;
                    pBufferHdr = NULL;
                }
                pSECPort->bufferStateAllocate[i] = BUFFER_STATE_FREE;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pSECPort->assignedBufferNum == 0) {
            SEC_OSAL_Log(SEC_LOG_TRACE, "pSECPort->unloadedResource signal set");
            /* SEC_OSAL_MutexLock(pSECComponent->compMutex); */
            SEC_OSAL_SemaphorePost(pSECPort->unloadedResource);
            /* SEC_OSAL_MutexUnlock(pSECComponent->compMutex); */
            pSECPort->portDefinition.bPopulated = OMX_FALSE;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_AllocateTunnelBuffer(SEC_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT             *pSECPort = NULL;
    OMX_BUFFERHEADERTYPE         *temp_bufferHeader = NULL;
    OMX_U8                       *temp_buffer = NULL;
    OMX_U32                       bufferSize = 0;
    OMX_PARAM_PORTDEFINITIONTYPE  portDefinition;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE SEC_OMX_FreeTunnelBuffer(SEC_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT* pSECPort = NULL;
    OMX_BUFFERHEADERTYPE* temp_bufferHeader = NULL;
    OMX_U8 *temp_buffer = NULL;
    OMX_U32 bufferSize = 0;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentTunnelRequest(
    OMX_IN OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32        nPort,
    OMX_IN OMX_HANDLETYPE hTunneledComp,
    OMX_IN OMX_U32        nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_BOOL SEC_Check_BufferProcess_State(SEC_OMX_BASECOMPONENT *pSECComponent)
{
    if ((pSECComponent->currentState == OMX_StateExecuting) &&
        (pSECComponent->pSECPort[INPUT_PORT_INDEX].portState == OMX_StateIdle) &&
        (pSECComponent->pSECPort[OUTPUT_PORT_INDEX].portState == OMX_StateIdle) &&
        (pSECComponent->transientState != SEC_OMX_TransStateExecutingToIdle) &&
        (pSECComponent->transientState != SEC_OMX_TransStateIdleToExecuting)) {
        return OMX_TRUE;
    } else {
        return OMX_FALSE;
    }
}

static OMX_ERRORTYPE SEC_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *secOMXInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT      *secOMXOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_OMX_DATABUFFER    *dataBuffer = &pSECComponent->secDataBuffer[INPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader;

    FunctionIn();

    if (bufferHeader != NULL) {
        if (secOMXInputPort->markType.hMarkTargetComponent != NULL ) {
            bufferHeader->hMarkTargetComponent      = secOMXInputPort->markType.hMarkTargetComponent;
            bufferHeader->pMarkData                 = secOMXInputPort->markType.pMarkData;
            secOMXInputPort->markType.hMarkTargetComponent = NULL;
            secOMXInputPort->markType.pMarkData = NULL;
        }

        if (bufferHeader->hMarkTargetComponent != NULL) {
            if (bufferHeader->hMarkTargetComponent == pOMXComponent) {
                pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                                pSECComponent->callbackData,
                                OMX_EventMark,
                                0, 0, bufferHeader->pMarkData);
            } else {
                pSECComponent->propagateMarkType.hMarkTargetComponent = bufferHeader->hMarkTargetComponent;
                pSECComponent->propagateMarkType.pMarkData = bufferHeader->pMarkData;
            }
        }

        if (CHECK_PORT_TUNNELED(secOMXInputPort)) {
            OMX_FillThisBuffer(secOMXInputPort->tunneledComponent, bufferHeader);
        } else {
            bufferHeader->nFilledLen = 0;
            pSECComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pSECComponent->callbackData, bufferHeader);
        }
    }

    if ((pSECComponent->currentState == OMX_StatePause) &&
        ((!CHECK_PORT_BEING_FLUSHED(secOMXInputPort) && !CHECK_PORT_BEING_FLUSHED(secOMXOutputPort)))) {
        SEC_OSAL_SignalWait(pSECComponent->pauseEvent, DEF_MAX_WAIT_TIME);
        SEC_OSAL_SignalReset(pSECComponent->pauseEvent);
    }

    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_InputBufferGetQueue(SEC_OMX_BASECOMPONENT *pSECComponent)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT   *pSECPort = NULL;
    SEC_OMX_DATABUFFER *dataBuffer = NULL;
    SEC_OMX_MESSAGE*    message = NULL;
    SEC_OMX_DATABUFFER *inputUseBuffer = &pSECComponent->secDataBuffer[INPUT_PORT_INDEX];

    FunctionIn();

    pSECPort= &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    dataBuffer = &pSECComponent->secDataBuffer[INPUT_PORT_INDEX];

    if (pSECComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        SEC_OSAL_SemaphoreWait(pSECPort->bufferSemID);
        SEC_OSAL_MutexLock(inputUseBuffer->bufferMutex);
        if (dataBuffer->dataValid != OMX_TRUE) {
            message = (SEC_OMX_MESSAGE *)SEC_OSAL_Dequeue(&pSECPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                SEC_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                goto EXIT;
            }

            dataBuffer->bufferHeader = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            dataBuffer->allocSize = dataBuffer->bufferHeader->nAllocLen;
            dataBuffer->dataLen = dataBuffer->bufferHeader->nFilledLen;
            dataBuffer->remainDataLen = dataBuffer->dataLen;
            dataBuffer->usedDataLen = 0;
            dataBuffer->dataValid = OMX_TRUE;
            dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags;
            dataBuffer->timeStamp = dataBuffer->bufferHeader->nTimeStamp;

            SEC_OSAL_Free(message);

            if (dataBuffer->allocSize <= dataBuffer->dataLen)
                SEC_OSAL_Log(SEC_LOG_WARNING, "Input Buffer Full, Check input buffer size! allocSize:%d, dataLen:%d", dataBuffer->allocSize, dataBuffer->dataLen);
        }
        SEC_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SEC_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *secOMXInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT      *secOMXOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_OMX_DATABUFFER    *dataBuffer = &pSECComponent->secDataBuffer[OUTPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader;

    FunctionIn();

    if (bufferHeader != NULL) {
        bufferHeader->nFilledLen = dataBuffer->remainDataLen;
        bufferHeader->nOffset    = 0;
        bufferHeader->nFlags     = dataBuffer->nFlags;
        bufferHeader->nTimeStamp = dataBuffer->timeStamp;

        if (pSECComponent->propagateMarkType.hMarkTargetComponent != NULL) {
            bufferHeader->hMarkTargetComponent = pSECComponent->propagateMarkType.hMarkTargetComponent;
            bufferHeader->pMarkData = pSECComponent->propagateMarkType.pMarkData;
            pSECComponent->propagateMarkType.hMarkTargetComponent = NULL;
            pSECComponent->propagateMarkType.pMarkData = NULL;
        }

        if (bufferHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventBufferFlag,
                            OUTPUT_PORT_INDEX,
                            bufferHeader->nFlags, NULL);
        }

        if (CHECK_PORT_TUNNELED(secOMXOutputPort)) {
            OMX_EmptyThisBuffer(secOMXOutputPort->tunneledComponent, bufferHeader);
        } else {
            pSECComponent->pCallbacks->FillBufferDone(pOMXComponent, pSECComponent->callbackData, bufferHeader);
        }
    }

    if ((pSECComponent->currentState == OMX_StatePause) &&
        ((!CHECK_PORT_BEING_FLUSHED(secOMXInputPort) && !CHECK_PORT_BEING_FLUSHED(secOMXOutputPort)))) {
        SEC_OSAL_SignalWait(pSECComponent->pauseEvent, DEF_MAX_WAIT_TIME);
        SEC_OSAL_SignalReset(pSECComponent->pauseEvent);
    }

    /* reset dataBuffer */
    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OutputBufferGetQueue(SEC_OMX_BASECOMPONENT *pSECComponent)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT   *pSECPort = NULL;
    SEC_OMX_DATABUFFER *dataBuffer = NULL;
    SEC_OMX_MESSAGE    *message = NULL;
    SEC_OMX_DATABUFFER *outputUseBuffer = &pSECComponent->secDataBuffer[OUTPUT_PORT_INDEX];

    FunctionIn();

    pSECPort= &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    dataBuffer = &pSECComponent->secDataBuffer[OUTPUT_PORT_INDEX];

    if (pSECComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        SEC_OSAL_SemaphoreWait(pSECPort->bufferSemID);
        SEC_OSAL_MutexLock(outputUseBuffer->bufferMutex);
        if (dataBuffer->dataValid != OMX_TRUE) {
            message = (SEC_OMX_MESSAGE *)SEC_OSAL_Dequeue(&pSECPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                goto EXIT;
            }

            dataBuffer->bufferHeader = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            dataBuffer->allocSize = dataBuffer->bufferHeader->nAllocLen;
            dataBuffer->dataLen = 0; //dataBuffer->bufferHeader->nFilledLen;
            dataBuffer->remainDataLen = dataBuffer->dataLen;
            dataBuffer->usedDataLen = 0; //dataBuffer->bufferHeader->nOffset;
            dataBuffer->dataValid =OMX_TRUE;
            /* dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags; */
            /* dataBuffer->nTimeStamp = dataBuffer->bufferHeader->nTimeStamp; */
            pSECComponent->processData[OUTPUT_PORT_INDEX].dataBuffer = dataBuffer->bufferHeader->pBuffer;
            pSECComponent->processData[OUTPUT_PORT_INDEX].allocSize = dataBuffer->bufferHeader->nAllocLen;

            SEC_OSAL_Free(message);
        }
        SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;

}

static OMX_ERRORTYPE SEC_BufferReset(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    /* SEC_OMX_BASEPORT      *pSECPort = &pSECComponent->pSECPort[portIndex]; */
    SEC_OMX_DATABUFFER    *dataBuffer = &pSECComponent->secDataBuffer[portIndex];
    /* OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader; */

    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

    return ret;
}

static OMX_ERRORTYPE SEC_DataReset(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    /* SEC_OMX_BASEPORT      *pSECPort = &pSECComponent->pSECPort[portIndex]; */
    /* SEC_OMX_DATABUFFER    *dataBuffer = &pSECComponent->secDataBuffer[portIndex]; */
    /* OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader; */
    SEC_OMX_DATA          *processData = &pSECComponent->processData[portIndex];

    processData->dataLen       = 0;
    processData->remainDataLen = 0;
    processData->usedDataLen   = 0;
    processData->nFlags        = 0;
    processData->timeStamp     = 0;

    return ret;
}

OMX_BOOL SEC_Preprocessor_InputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL               ret = OMX_FALSE;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_DATABUFFER    *inputUseBuffer = &pSECComponent->secDataBuffer[INPUT_PORT_INDEX];
    SEC_OMX_DATA          *inputData = &pSECComponent->processData[INPUT_PORT_INDEX];
    OMX_U32                copySize = 0;
    OMX_BYTE               checkInputStream = NULL;
    OMX_U32                checkInputStreamLen = 0;
    OMX_U32                checkedSize = 0;
    OMX_BOOL               flagEOF = OMX_FALSE;
    OMX_BOOL               previousFrameEOF = OMX_FALSE;

    FunctionIn();

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        checkInputStream = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
        checkInputStreamLen = inputUseBuffer->remainDataLen;

        if (inputData->dataLen == 0) {
            previousFrameEOF = OMX_TRUE;
        } else {
            previousFrameEOF = OMX_FALSE;
        }
        if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
            flagEOF = OMX_TRUE;
            checkedSize = checkInputStreamLen;
        } else if ((pSECComponent->bUseFlagEOF == OMX_TRUE) &&
           !(inputUseBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
            flagEOF = OMX_TRUE;
            checkedSize = checkInputStreamLen;
        } else {
            pSECComponent->bUseFlagEOF = OMX_FALSE;
            checkedSize = pSECComponent->sec_checkInputFrame(checkInputStream, checkInputStreamLen, inputUseBuffer->nFlags, previousFrameEOF, &flagEOF);
        }

        if (flagEOF == OMX_TRUE) {
            copySize = checkedSize;
            SEC_OSAL_Log(SEC_LOG_TRACE, "sec_checkInputFrame : OMX_TRUE");
        } else {
            copySize = checkInputStreamLen;
            SEC_OSAL_Log(SEC_LOG_TRACE, "sec_checkInputFrame : OMX_FALSE");
        }

        if (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            pSECComponent->bSaveFlagEOS = OMX_TRUE;

        if ((((inputData->allocSize) - (inputData->dataLen)) >= copySize) || (pVideoDec->bDRMPlayerMode == OMX_TRUE)) {
            if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
                inputData->dataBuffer = checkInputStream;
            } else {
                if (copySize > 0)
                    SEC_OSAL_Memcpy(inputData->dataBuffer + inputData->dataLen, checkInputStream, copySize);
            }

            inputUseBuffer->dataLen -= copySize;
            inputUseBuffer->remainDataLen -= copySize;
            inputUseBuffer->usedDataLen += copySize;

            inputData->dataLen += copySize;
            inputData->remainDataLen += copySize;

            if (previousFrameEOF == OMX_TRUE) {
                inputData->timeStamp = inputUseBuffer->timeStamp;
                inputData->nFlags = inputUseBuffer->nFlags;
            }

            if (pSECComponent->bUseFlagEOF == OMX_TRUE) {
                if (pSECComponent->bSaveFlagEOS == OMX_TRUE) {
                    inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    flagEOF = OMX_TRUE;
                    pSECComponent->bSaveFlagEOS = OMX_FALSE;
                } else {
                    inputData->nFlags = (inputData->nFlags & (~OMX_BUFFERFLAG_EOS));
                }
            } else {
                if ((checkedSize == checkInputStreamLen) && (pSECComponent->bSaveFlagEOS == OMX_TRUE)) {
                    if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) &&
                        ((inputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG) ||
                        (inputData->dataLen == 0))) {
                    inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    flagEOF = OMX_TRUE;
                    pSECComponent->bSaveFlagEOS = OMX_FALSE;
                    } else if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) &&
                               (!(inputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) &&
                               (inputData->dataLen != 0)) {
                        inputData->nFlags = (inputData->nFlags & (~OMX_BUFFERFLAG_EOS));
                        flagEOF = OMX_TRUE;
                        pSECComponent->bSaveFlagEOS = OMX_TRUE;
                    }
                } else {
                    inputData->nFlags = (inputUseBuffer->nFlags & (~OMX_BUFFERFLAG_EOS));
                }
            }

            if(((inputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
               (inputData->dataLen <= 0) && (flagEOF == OMX_TRUE)) {
                inputData->dataLen = inputData->previousDataLen;
                inputData->remainDataLen = inputData->previousDataLen;
            }
        } else {
            /*????????????????????????????????? Error ?????????????????????????????????*/
            SEC_DataReset(pOMXComponent, INPUT_PORT_INDEX);
            flagEOF = OMX_FALSE;
        }

        if (inputUseBuffer->remainDataLen == 0) {
            if (pVideoDec->bDRMPlayerMode != OMX_TRUE)
                SEC_InputBufferReturn(pOMXComponent);
        } else {
            inputUseBuffer->dataValid = OMX_TRUE;
        }
    }

    if (flagEOF == OMX_TRUE) {
        if (pSECComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            pSECComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_TRUE;
            pSECComponent->checkTimeStamp.startTimeStamp = inputData->timeStamp;
            pSECComponent->checkTimeStamp.nStartFlags = inputData->nFlags;
            pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
            SEC_OSAL_Log(SEC_LOG_TRACE, "first frame timestamp after seeking %lld us (%.2f secs)",
                inputData->timeStamp, inputData->timeStamp / 1E6);
        }

        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }

    FunctionOut();

    return ret;
}

OMX_BOOL SEC_Postprocess_OutputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL               ret = OMX_FALSE;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *secOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_OMX_DATABUFFER    *outputUseBuffer = &pSECComponent->secDataBuffer[OUTPUT_PORT_INDEX];
    SEC_OMX_DATA          *outputData = &pSECComponent->processData[OUTPUT_PORT_INDEX];
    OMX_U32                copySize = 0;

    FunctionIn();

    if (outputUseBuffer->dataValid == OMX_TRUE) {
        if (pSECComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE) {
            if ((pSECComponent->checkTimeStamp.startTimeStamp == outputData->timeStamp) &&
                (pSECComponent->checkTimeStamp.nStartFlags == outputData->nFlags)){
                pSECComponent->checkTimeStamp.startTimeStamp = -19761123;
                pSECComponent->checkTimeStamp.nStartFlags = 0x0;
                pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
                pSECComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            } else {
                SEC_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);
                ret = OMX_TRUE;
                goto EXIT;
            }
        } else if (pSECComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            SEC_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);
            ret = OMX_TRUE;
            goto EXIT;
        }

        if (outputData->remainDataLen <= (outputUseBuffer->allocSize - outputUseBuffer->dataLen)) {
            copySize = outputData->remainDataLen;

            outputUseBuffer->dataLen += copySize;
            outputUseBuffer->remainDataLen += copySize;
            outputUseBuffer->nFlags = outputData->nFlags;
            outputUseBuffer->timeStamp = outputData->timeStamp;

            ret = OMX_TRUE;

            /* reset outputData */
            SEC_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);

            if ((outputUseBuffer->remainDataLen > 0) ||
                (outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS))
                SEC_OutputBufferReturn(pOMXComponent);
        } else {
            SEC_OSAL_Log(SEC_LOG_ERROR, "output buffer is smaller than decoded data size Out Length");

            ret = OMX_FALSE;

            /* reset outputData */
            SEC_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);
        }
    } else {
        ret = OMX_FALSE;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_BufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_BASEPORT      *secInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT      *secOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    SEC_OMX_DATABUFFER    *inputUseBuffer = &pSECComponent->secDataBuffer[INPUT_PORT_INDEX];
    SEC_OMX_DATABUFFER    *outputUseBuffer = &pSECComponent->secDataBuffer[OUTPUT_PORT_INDEX];
    SEC_OMX_DATA          *inputData = &pSECComponent->processData[INPUT_PORT_INDEX];
    SEC_OMX_DATA          *outputData = &pSECComponent->processData[OUTPUT_PORT_INDEX];
    OMX_U32                copySize = 0;

    pSECComponent->remainOutputData = OMX_FALSE;
    pSECComponent->reInputData = OMX_FALSE;

    FunctionIn();

    while (!pSECComponent->bExitBufferProcessThread) {
        SEC_OSAL_SleepMillisec(0);

        if (((pSECComponent->currentState == OMX_StatePause) ||
            (pSECComponent->currentState == OMX_StateIdle) ||
            (pSECComponent->transientState == SEC_OMX_TransStateLoadedToIdle) ||
            (pSECComponent->transientState == SEC_OMX_TransStateExecutingToIdle)) &&
            (pSECComponent->transientState != SEC_OMX_TransStateIdleToLoaded)&&
            ((!CHECK_PORT_BEING_FLUSHED(secInputPort) && !CHECK_PORT_BEING_FLUSHED(secOutputPort)))) {
            SEC_OSAL_SignalWait(pSECComponent->pauseEvent, DEF_MAX_WAIT_TIME);
            SEC_OSAL_SignalReset(pSECComponent->pauseEvent);
        }

        while ((SEC_Check_BufferProcess_State(pSECComponent)) && (!pSECComponent->bExitBufferProcessThread)) {
            SEC_OSAL_SleepMillisec(0);

            SEC_OSAL_MutexLock(outputUseBuffer->bufferMutex);
            if ((outputUseBuffer->dataValid != OMX_TRUE) &&
                (!CHECK_PORT_BEING_FLUSHED(secOutputPort))) {
                SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                ret = SEC_OutputBufferGetQueue(pSECComponent);
                if ((ret == OMX_ErrorUndefined) ||
                    (secInputPort->portState != OMX_StateIdle) ||
                    (secOutputPort->portState != OMX_StateIdle)) {
                    break;
                }
            } else {
                SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
            }

            if (pSECComponent->remainOutputData == OMX_FALSE) {
                if (pSECComponent->reInputData == OMX_FALSE) {
                    SEC_OSAL_MutexLock(inputUseBuffer->bufferMutex);
                    if ((SEC_Preprocessor_InputData(pOMXComponent) == OMX_FALSE) &&
                        (!CHECK_PORT_BEING_FLUSHED(secInputPort))) {
                            SEC_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                            ret = SEC_InputBufferGetQueue(pSECComponent);
                            break;
                    }

                    SEC_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                }

                SEC_OSAL_MutexLock(inputUseBuffer->bufferMutex);
                SEC_OSAL_MutexLock(outputUseBuffer->bufferMutex);
                ret = pSECComponent->sec_mfc_bufferProcess(pOMXComponent, inputData, outputData);

                if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
                    if ((inputUseBuffer->remainDataLen == 0) &&
                        (ret != OMX_ErrorInputDataDecodeYet))
                        SEC_InputBufferReturn(pOMXComponent);
                    else
                        inputUseBuffer->dataValid = OMX_TRUE;
                }
                SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                SEC_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);

                if (ret == OMX_ErrorInputDataDecodeYet)
                    pSECComponent->reInputData = OMX_TRUE;
                else
                    pSECComponent->reInputData = OMX_FALSE;
            }

            SEC_OSAL_MutexLock(outputUseBuffer->bufferMutex);

            if (SEC_Postprocess_OutputData(pOMXComponent) == OMX_FALSE)
                pSECComponent->remainOutputData = OMX_TRUE;
            else
                pSECComponent->remainOutputData = OMX_FALSE;

            SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoDecodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;

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

    switch (nParamIndex) {
    case OMX_IndexParamVideoInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = SEC_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        portParam->nPorts           = pSECComponent->portParam.nPorts;
        portParam->nStartPortNumber = pSECComponent->portParam.nStartPortNumber;
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        SEC_OMX_BASEPORT               *pSECPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0; /* supportFormatNum = N-1 */

        ret = SEC_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pSECComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }


        if (portIndex == INPUT_PORT_INDEX) {
            supportFormatNum = INPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
            portDefinition = &pSECPort->portDefinition;

            portFormat->eCompressionFormat = portDefinition->format.video.eCompressionFormat;
            portFormat->eColorFormat       = portDefinition->format.video.eColorFormat;
            portFormat->xFramerate           = portDefinition->format.video.xFramerate;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            supportFormatNum = OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
            portDefinition = &pSECPort->portDefinition;

            switch (index) {
            case supportFormat_0:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatYUV420Planar;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_1:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatYUV420SemiPlanar;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_2:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12TPhysicalAddress;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
           case supportFormat_3:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12Tiled;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            }
        }
        ret = OMX_ErrorNone;
    }
        break;
#ifdef USE_ANB
    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        ret = SEC_OSAL_GetANBParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }
        break;
#endif
    default:
    {
        ret = SEC_OMX_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}
OMX_ERRORTYPE SEC_OMX_VideoDecodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;

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
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        SEC_OMX_BASEPORT               *pSECPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0;

        ret = SEC_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pSECComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pSECPort = &pSECComponent->pSECPort[portIndex];
            portDefinition = &pSECPort->portDefinition;

            portDefinition->format.video.eColorFormat       = portFormat->eColorFormat;
            portDefinition->format.video.eCompressionFormat = portFormat->eCompressionFormat;
            portDefinition->format.video.xFramerate         = portFormat->xFramerate;
        }
    }
        break;
#ifdef USE_ANB
    case OMX_IndexParamEnableAndroidBuffers:
    case OMX_IndexParamUseAndroidNativeBuffer:
    {
        ret = SEC_OSAL_SetANBParameter(hComponent, nIndex, ComponentParameterStructure);
    }
        break;
#endif
    default:
    {
        ret = SEC_OMX_SetParameter(hComponent, nIndex, ComponentParameterStructure);
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoDecodeGetConfig(
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
    default:
        ret = SEC_OMX_GetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoDecodeSetConfig(
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
    case OMX_IndexVendorThumbnailMode:
    {
        SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
        pVideoDec->bThumbnailMode = *((OMX_BOOL *)pComponentConfigStructure);

        ret = OMX_ErrorNone;
    }
        break;
    default:
        ret = SEC_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoDecodeGetExtensionIndex(
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

#ifdef USE_ANB
    if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_ENABLE_ANB) == 0)
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
    else if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_GET_ANB) == 0)
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
    else if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_USE_ANB) == 0)
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamUseAndroidNativeBuffer;
    else
        ret = SEC_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
#else
    ret = SEC_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = SEC_OMX_BaseComponent_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = SEC_OMX_Port_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OMX_BaseComponent_Destructor(pOMXComponent);
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pVideoDec = SEC_OSAL_Malloc(sizeof(SEC_OMX_VIDEODEC_COMPONENT));
    if (pVideoDec == NULL) {
        SEC_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    SEC_OSAL_Memset(pVideoDec, 0, sizeof(SEC_OMX_VIDEODEC_COMPONENT));
    pSECComponent->hComponentHandle = (OMX_HANDLETYPE)pVideoDec;

    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    /* Input port */
    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECPort->portDefinition.nBufferCountActual = MAX_VIDEO_INPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferCountMin = MAX_VIDEO_INPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferSize = 0;
    pSECPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pSECPort->portDefinition.format.video.cMIMEType = SEC_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "raw/video");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pSECPort->portDefinition.format.video.nFrameWidth = 0;
    pSECPort->portDefinition.format.video.nFrameHeight= 0;
    pSECPort->portDefinition.format.video.nStride = 0;
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.format.video.nBitrate = 64000;
    pSECPort->portDefinition.format.video.xFramerate = (15 << 16);
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pSECPort->portDefinition.format.video.pNativeWindow = NULL;

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.nBufferCountActual = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferCountMin = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pSECPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pSECPort->portDefinition.format.video.cMIMEType = SEC_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "raw/video");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pSECPort->portDefinition.format.video.nFrameWidth = 0;
    pSECPort->portDefinition.format.video.nFrameHeight= 0;
    pSECPort->portDefinition.format.video.nStride = 0;
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.format.video.nBitrate = 64000;
    pSECPort->portDefinition.format.video.xFramerate = (15 << 16);
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pSECPort->portDefinition.format.video.pNativeWindow = NULL;

    pOMXComponent->UseBuffer              = &SEC_OMX_UseBuffer;
    pOMXComponent->AllocateBuffer         = &SEC_OMX_AllocateBuffer;
    pOMXComponent->FreeBuffer             = &SEC_OMX_FreeBuffer;
    pOMXComponent->ComponentTunnelRequest = &SEC_OMX_ComponentTunnelRequest;

    pSECComponent->sec_AllocateTunnelBuffer = &SEC_OMX_AllocateTunnelBuffer;
    pSECComponent->sec_FreeTunnelBuffer     = &SEC_OMX_FreeTunnelBuffer;
    pSECComponent->sec_BufferProcess        = &SEC_OMX_BufferProcess;
    pSECComponent->sec_BufferReset          = &SEC_BufferReset;
    pSECComponent->sec_InputBufferReturn    = &SEC_InputBufferReturn;
    pSECComponent->sec_OutputBufferReturn   = &SEC_OutputBufferReturn;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    SEC_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    int                    i = 0;

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

    pVideoDec = (SEC_OMX_VIDEODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OSAL_Free(pVideoDec);
    pSECComponent->hComponentHandle = pVideoDec = NULL;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        pSECPort = &pSECComponent->pSECPort[i];
        SEC_OSAL_Free(pSECPort->portDefinition.format.video.cMIMEType);
        pSECPort->portDefinition.format.video.cMIMEType = NULL;
    }

    ret = SEC_OMX_Port_Destructor(pOMXComponent);

    ret = SEC_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}
