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
 * @file        SEC_OMX_Venc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
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
#include "SEC_OMX_Venc.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OSAL_Thread.h"
#include "SEC_OSAL_Mutex.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_ETC.h"
#include "csc.h"

#ifdef USE_STOREMETADATA
#include "SEC_OSAL_Android.h"
#endif

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_VIDEO_ENC"
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

        if (width && height)
            secOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
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

    temp_buffer = SEC_OSAL_Malloc(sizeof(OMX_U8) * nSizeBytes);
    if (temp_buffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)SEC_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
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
    OMX_PARAM_PORTDEFINITIONTYPE portDefinition;

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
        (pSECComponent->transientState != SEC_OMX_TransStateIdleToExecuting))
        return OMX_TRUE;
    else
        return OMX_FALSE;
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
            dataBuffer->usedDataLen = 0; //dataBuffer->bufferHeader->nOffset;
            dataBuffer->dataValid = OMX_TRUE;
            dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags;
            dataBuffer->timeStamp = dataBuffer->bufferHeader->nTimeStamp;
            pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = dataBuffer->bufferHeader->pBuffer;
            pSECComponent->processData[INPUT_PORT_INDEX].allocSize = dataBuffer->bufferHeader->nAllocLen;
            SEC_OSAL_Free(message);
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
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OMX_DATABUFFER    *inputUseBuffer = &pSECComponent->secDataBuffer[INPUT_PORT_INDEX];
    SEC_OMX_DATA          *inputData = &pSECComponent->processData[INPUT_PORT_INDEX];
    OMX_U32                copySize = 0;
    OMX_BYTE               checkInputStream = NULL;
    OMX_U32                checkInputStreamLen = 0;
    OMX_U32                checkedSize = 0;
    OMX_BOOL               flagEOS = OMX_FALSE;
    OMX_BOOL               flagEOF = OMX_FALSE;
    OMX_BOOL               previousFrameEOF = OMX_FALSE;

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        checkInputStream = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
        checkInputStreamLen = inputUseBuffer->remainDataLen;

        if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
            (pSECComponent->bUseFlagEOF == OMX_FALSE)) {
            pSECComponent->bUseFlagEOF = OMX_TRUE;
        }

        if (inputData->dataLen == 0) {
            previousFrameEOF = OMX_TRUE;
        } else {
            previousFrameEOF = OMX_FALSE;
        }
        if (pSECComponent->bUseFlagEOF == OMX_TRUE) {
            flagEOF = OMX_TRUE;
            checkedSize = checkInputStreamLen;
            if (checkedSize == 0) {
                SEC_OMX_BASEPORT *pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
                int width = pSECPort->portDefinition.format.video.nFrameWidth;
                int height = pSECPort->portDefinition.format.video.nFrameHeight;
                inputUseBuffer->remainDataLen = inputUseBuffer->dataLen = (width * height * 3) / 2;
                checkedSize = checkInputStreamLen = inputUseBuffer->remainDataLen;
                inputUseBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
            }
            if (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
                flagEOS = OMX_TRUE;
            }
        } else {
            SEC_OMX_BASEPORT *pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
            int width = pSECPort->portDefinition.format.video.nFrameWidth;
            int height = pSECPort->portDefinition.format.video.nFrameHeight;
            unsigned int oneFrameSize = 0;

            switch (pSECPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
            case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
            case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
            case OMX_SEC_COLOR_FormatNV12Tiled:
            case OMX_SEC_COLOR_FormatNV21Linear:
                oneFrameSize = (width * height * 3) / 2;
                break;
            case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
                oneFrameSize = ALIGN((ALIGN(width, 16) * ALIGN(height, 16)), 2048) + ALIGN((ALIGN(width, 16) * ALIGN(height >> 1, 8)), 2048);
                break;
            default:
                SEC_OSAL_Log(SEC_LOG_ERROR, "SEC_Preprocessor_InputData: eColorFormat is wrong");
                break;
            }

            if (previousFrameEOF == OMX_TRUE) {
                if (checkInputStreamLen >= oneFrameSize) {
                    checkedSize = oneFrameSize;
                    flagEOF = OMX_TRUE;
                } else {
                    flagEOF = OMX_FALSE;
                }
            } else {
                if (checkInputStreamLen >= (oneFrameSize - inputData->dataLen)) {
                    checkedSize = oneFrameSize - inputData->dataLen;
                    flagEOF = OMX_TRUE;
                } else {
                    flagEOF = OMX_FALSE;
                }
            }

            if ((flagEOF == OMX_FALSE) && (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)) {
                flagEOF = OMX_TRUE;
                flagEOS = OMX_TRUE;
            }
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

        if (((inputData->allocSize) - (inputData->dataLen)) >= copySize) {
            SEC_OMX_BASEPORT *pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
            if ((pSECPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV12TPhysicalAddress) &&
                (pSECPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV12LPhysicalAddress) &&
                (pSECPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV12LVirtualAddress) &&
                (pSECPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV21LPhysicalAddress)) {
                if (flagEOF == OMX_TRUE) {
                    OMX_U32 width, height;
                    unsigned int csc_src_color_format, csc_dst_color_format;
                    unsigned int cacheable = 1;
                    unsigned char *pSrcBuf[3] = {NULL, };
                    unsigned char *pDstBuf[3] = {NULL, };
                    OMX_PTR ppBuf[3];

                    width = pSECPort->portDefinition.format.video.nFrameWidth;
                    height = pSECPort->portDefinition.format.video.nFrameHeight;

                    pDstBuf[0] = (unsigned char *)pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
                    pDstBuf[1] = (unsigned char *)pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;

                    SEC_OSAL_Log(SEC_LOG_TRACE, "pVideoEnc->MFCEncInputBuffer[%d].YVirAddr : 0x%x", pVideoEnc->indexInputBuffer, pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr);
                    SEC_OSAL_Log(SEC_LOG_TRACE, "pVideoEnc->MFCEncInputBuffer[%d].CVirAddr : 0x%x", pVideoEnc->indexInputBuffer, pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr);

                    SEC_OSAL_Log(SEC_LOG_TRACE, "width:%d, height:%d, Ysize:%d", width, height, ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height)));
                    SEC_OSAL_Log(SEC_LOG_TRACE, "width:%d, height:%d, Csize:%d", width, height, ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height / 2)));

                    if (pSECPort->bStoreMetaData == OMX_FALSE) {
                        pSrcBuf[0]  = checkInputStream;
                        pSrcBuf[1]  = checkInputStream + (width * height);
                        pSrcBuf[2]  = checkInputStream + (((width * height) * 5) / 4);

                        switch (pSECPort->portDefinition.format.video.eColorFormat) {
                        case OMX_COLOR_FormatYUV420Planar:
                            /* YUV420Planar case it needed changed interleave UV plane (MFC spec.)*/
                            csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420Planar);
                            csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
                            break;
                        case OMX_COLOR_FormatYUV420SemiPlanar:
                        case OMX_SEC_COLOR_FormatNV12Tiled:
                        case OMX_SEC_COLOR_FormatNV21Linear:
                            csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
                            csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
                            break;
                        default:
                            break;
                        }
                    }
#ifdef USE_METADATABUFFERTYPE
                    else {
                        if (pSECPort->portDefinition.format.video.eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                                OMX_PTR pOutBuffer;
                                csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_Format32bitARGB8888);
                                csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);

                                SEC_OSAL_GetInfoFromMetaData(inputData, ppBuf);
                                SEC_OSAL_LockANBHandle((OMX_U32)ppBuf[0], width, height, OMX_COLOR_FormatAndroidOpaque, &pOutBuffer);
                                pSrcBuf[0]  = (unsigned char *)pOutBuffer;
                                pSrcBuf[1]  = NULL;
                                pSrcBuf[2]  = NULL;
                        }
                    }
#endif

                    csc_set_src_format(
                        pVideoEnc->csc_handle,  /* handle */
                        width,                  /* width */
                        height,                 /* height */
                        0,                      /* crop_left */
                        0,                      /* crop_right */
                        width,                  /* crop_width */
                        height,                 /* crop_height */
                        csc_src_color_format,   /* color_format */
                        cacheable);             /* cacheable */
                    csc_set_dst_format(
                        pVideoEnc->csc_handle,  /* handle */
                        width,                  /* width */
                        height,                 /* height */
                        0,                      /* crop_left */
                        0,                      /* crop_right */
                        width,                  /* crop_width */
                        height,                 /* crop_height */
                        csc_dst_color_format,   /* color_format */
                        cacheable);             /* cacheable */
                    csc_set_src_buffer(
                        pVideoEnc->csc_handle,  /* handle */
                        pSrcBuf[0],             /* y addr */
                        pSrcBuf[1],             /* u addr or uv addr */
                        pSrcBuf[2],             /* v addr or none */
                        0);                     /* ion fd */
                    csc_set_dst_buffer(
                        pVideoEnc->csc_handle,  /* handle */
                        pDstBuf[0],             /* y addr */
                        pDstBuf[1],             /* u addr or uv addr */
                        pDstBuf[2],             /* v addr or none */
                        0);                     /* ion fd */
                    csc_convert(pVideoEnc->csc_handle);

#ifdef USE_METADATABUFFERTYPE
                    if (pSECPort->bStoreMetaData == OMX_TRUE) {
                        SEC_OSAL_UnlockANBHandle((OMX_U32)ppBuf[0]);
                    }
#endif
                }
            }

            inputUseBuffer->dataLen -= copySize; /* ???????????? do not need ?????????????? */
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
                }
            } else {
                if ((checkedSize == checkInputStreamLen) && (pSECComponent->bSaveFlagEOS == OMX_TRUE)) {
                    inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    flagEOF = OMX_TRUE;
                    pSECComponent->bSaveFlagEOS = OMX_FALSE;
                } else {
                    inputData->nFlags = (inputUseBuffer->nFlags & (~OMX_BUFFERFLAG_EOS));
                }
            }
        } else {
            /*????????????????????????????????? Error ?????????????????????????????????*/
            SEC_DataReset(pOMXComponent, INPUT_PORT_INDEX);
            flagEOF = OMX_FALSE;
        }

        if (inputUseBuffer->remainDataLen == 0) {
            if(flagEOF == OMX_FALSE)
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
        }

        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }
    return ret;
}

OMX_BOOL SEC_Postprocess_OutputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL               ret = OMX_FALSE;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_DATABUFFER    *outputUseBuffer = &pSECComponent->secDataBuffer[OUTPUT_PORT_INDEX];
    SEC_OMX_DATA          *outputData = &pSECComponent->processData[OUTPUT_PORT_INDEX];
    OMX_U32                copySize = 0;

    if (outputUseBuffer->dataValid == OMX_TRUE) {
        if (pSECComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE) {
            if (pSECComponent->checkTimeStamp.startTimeStamp == outputData->timeStamp){
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
            if (copySize > 0)
                SEC_OSAL_Memcpy((outputUseBuffer->bufferHeader->pBuffer + outputUseBuffer->dataLen),
                        (outputData->dataBuffer + outputData->usedDataLen),
                         copySize);

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
            SEC_OSAL_Log(SEC_LOG_ERROR, "output buffer is smaller than encoded data size Out Length");

            ret = OMX_FALSE;

            /* reset outputData */
            SEC_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);
        }
    } else {
        ret = OMX_FALSE;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE SEC_OMX_BufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
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

        while (SEC_Check_BufferProcess_State(pSECComponent) && !pSECComponent->bExitBufferProcessThread) {
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

                if (inputUseBuffer->remainDataLen == 0)
                    SEC_InputBufferReturn(pOMXComponent);
                else
                    inputUseBuffer->dataValid = OMX_TRUE;

                SEC_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                SEC_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);

                if (ret == OMX_ErrorInputDataEncodeYet)
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

OMX_ERRORTYPE SEC_OMX_VideoEncodeGetParameter(
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
        OMX_U32                         supportFormatNum = 0;

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

            switch (index) {
            case supportFormat_0:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatYUV420Planar;
                portFormat->xFramerate           = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_1:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12TPhysicalAddress;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_2:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12LPhysicalAddress;
                portFormat->xFramerate           = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_3:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatYUV420SemiPlanar;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_4:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12Tiled;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_5:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12LVirtualAddress;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_6:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV21LPhysicalAddress;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_7:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV21Linear;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_8:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatAndroidOpaque;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            }
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            supportFormatNum = OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
            portDefinition = &pSECPort->portDefinition;

            portFormat->eCompressionFormat = portDefinition->format.video.eCompressionFormat;
            portFormat->eColorFormat       = portDefinition->format.video.eColorFormat;
            portFormat->xFramerate           = portDefinition->format.video.xFramerate;
        }
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE  *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = videoRateControl->nPortIndex;
        SEC_OMX_BASEPORT         *pSECPort = NULL;
        SEC_OMX_VIDEOENC_COMPONENT   *pVideoEnc = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
            pSECPort = &pSECComponent->pSECPort[portIndex];
            portDefinition = &pSECPort->portDefinition;

            videoRateControl->eControlRate = pVideoEnc->eControlRate[portIndex];
            videoRateControl->nTargetBitrate = portDefinition->format.video.nBitrate;
        }
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoQuantization:
    {
        OMX_VIDEO_PARAM_QUANTIZATIONTYPE  *videoQuantizationControl = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = videoQuantizationControl->nPortIndex;
        SEC_OMX_BASEPORT         *pSECPort = NULL;
        SEC_OMX_VIDEOENC_COMPONENT   *pVideoEnc = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
            pSECPort = &pSECComponent->pSECPort[portIndex];
            portDefinition = &pSECPort->portDefinition;

            videoQuantizationControl->nQpI = pVideoEnc->quantization.nQpI;
            videoQuantizationControl->nQpP = pVideoEnc->quantization.nQpP;
            videoQuantizationControl->nQpB = pVideoEnc->quantization.nQpB;
        }
        ret = OMX_ErrorNone;

    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = portDefinition->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort;

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = SEC_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];
        SEC_OSAL_Memcpy(portDefinition, &pSECPort->portDefinition, portDefinition->nSize);

#ifdef USE_STOREMETADATA
        if ((portIndex == 0) &&
            (pSECPort->bStoreMetaData == OMX_TRUE) &&
            (pSECPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV12LVirtualAddress)) {
            portDefinition->nBufferSize = MAX_INPUT_METADATA_BUFFER_SIZE;
        }
#endif
    }
        break;
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
OMX_ERRORTYPE SEC_OMX_VideoEncodeSetParameter(
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
    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE  *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = videoRateControl->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort = NULL;
        SEC_OMX_VIDEOENC_COMPONENT   *pVideoEnc = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
            pSECPort = &pSECComponent->pSECPort[portIndex];
            portDefinition = &pSECPort->portDefinition;

            pVideoEnc->eControlRate[portIndex] = videoRateControl->eControlRate;
            portDefinition->format.video.nBitrate = videoRateControl->nTargetBitrate;
        }
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoQuantization:
    {
        OMX_VIDEO_PARAM_QUANTIZATIONTYPE  *videoQuantizationControl = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = videoQuantizationControl->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort = NULL;
        SEC_OMX_VIDEOENC_COMPONENT   *pVideoEnc = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
            pSECPort = &pSECComponent->pSECPort[portIndex];
            portDefinition = &pSECPort->portDefinition;

            pVideoEnc->quantization.nQpI = videoQuantizationControl->nQpI;
            pVideoEnc->quantization.nQpP = videoQuantizationControl->nQpP;
            pVideoEnc->quantization.nQpB = videoQuantizationControl->nQpB;
        }
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = pPortDefinition->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort;
        OMX_U32 width, height, size;

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
        if(pPortDefinition->nBufferCountActual < pSECPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        SEC_OSAL_Memcpy(&pSECPort->portDefinition, pPortDefinition, pPortDefinition->nSize);
        if (portIndex == INPUT_PORT_INDEX) {
            SEC_OMX_BASEPORT *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
            SEC_UpdateFrameSize(pOMXComponent);
            SEC_OSAL_Log(SEC_LOG_TRACE, "pSECOutputPort->portDefinition.nBufferSize: %d",
                            pSECOutputPort->portDefinition.nBufferSize);
        }
        ret = OMX_ErrorNone;
    }
        break;
#ifdef USE_STOREMETADATA
    case OMX_IndexParamStoreMetaDataBuffer:
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

OMX_ERRORTYPE SEC_OMX_VideoEncodeGetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
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
    case OMX_IndexConfigVideoBitrate:
    {
        OMX_VIDEO_CONFIG_BITRATETYPE *pEncodeBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)pComponentConfigStructure;
        OMX_U32                       portIndex = pEncodeBitrate->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pSECPort = &pSECComponent->pSECPort[portIndex];
            pEncodeBitrate->nEncodeBitrate = pSECPort->portDefinition.format.video.nBitrate;
        }
    }
        break;
    case OMX_IndexConfigVideoFramerate:
    {
        OMX_CONFIG_FRAMERATETYPE *pFramerate = (OMX_CONFIG_FRAMERATETYPE *)pComponentConfigStructure;
        OMX_U32                   portIndex = pFramerate->nPortIndex;
        SEC_OMX_BASEPORT         *pSECPort = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pSECPort = &pSECComponent->pSECPort[portIndex];
            pFramerate->xEncodeFramerate = pSECPort->portDefinition.format.video.xFramerate;
        }
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

OMX_ERRORTYPE SEC_OMX_VideoEncodeSetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
    {
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
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
    case OMX_IndexConfigVideoBitrate:
    {
        OMX_VIDEO_CONFIG_BITRATETYPE *pEncodeBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)pComponentConfigStructure;
        OMX_U32                       portIndex = pEncodeBitrate->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pSECPort = &pSECComponent->pSECPort[portIndex];
            pSECPort->portDefinition.format.video.nBitrate = pEncodeBitrate->nEncodeBitrate;
        }
    }
        break;
    case OMX_IndexConfigVideoFramerate:
    {
        OMX_CONFIG_FRAMERATETYPE *pFramerate = (OMX_CONFIG_FRAMERATETYPE *)pComponentConfigStructure;
        OMX_U32                   portIndex = pFramerate->nPortIndex;
        SEC_OMX_BASEPORT         *pSECPort = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pSECPort = &pSECComponent->pSECPort[portIndex];
            pSECPort->portDefinition.format.video.xFramerate = pFramerate->xEncodeFramerate;
        }
    }
        break;
    case OMX_IndexConfigVideoIntraVOPRefresh:
    {
        OMX_CONFIG_INTRAREFRESHVOPTYPE *pIntraRefreshVOP = (OMX_CONFIG_INTRAREFRESHVOPTYPE *)pComponentConfigStructure;
        SEC_OMX_VIDEOENC_COMPONENT *pVEncBase = ((SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle);
        OMX_U32 portIndex = pIntraRefreshVOP->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pVEncBase->IntraRefreshVOP = pIntraRefreshVOP->IntraRefreshVOP;
        }
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

OMX_ERRORTYPE SEC_OMX_VideoEncodeGetExtensionIndex(
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

#ifdef USE_STOREMETADATA
    if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_STORE_METADATA_BUFFER) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamStoreMetaDataBuffer;
    } else {
        ret = SEC_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
    }
#else
    ret = SEC_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_VideoEncodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;

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

    pVideoEnc = SEC_OSAL_Malloc(sizeof(SEC_OMX_VIDEOENC_COMPONENT));
    if (pVideoEnc == NULL) {
        SEC_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    SEC_OSAL_Memset(pVideoEnc, 0, sizeof(SEC_OMX_VIDEOENC_COMPONENT));
    pSECComponent->hComponentHandle = (OMX_HANDLETYPE)pVideoEnc;

    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    pVideoEnc->configChange = OMX_FALSE;
    pVideoEnc->quantization.nQpI = 20;
    pVideoEnc->quantization.nQpP = 20;
    pVideoEnc->quantization.nQpB = 20;

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
    pVideoEnc->eControlRate[INPUT_PORT_INDEX] = OMX_Video_ControlRateDisable;

    pSECPort->bStoreMetaData = OMX_FALSE;

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
    pVideoEnc->eControlRate[OUTPUT_PORT_INDEX] = OMX_Video_ControlRateDisable;

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

OMX_ERRORTYPE SEC_OMX_VideoEncodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    SEC_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
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

    pVideoEnc = (SEC_OMX_VIDEOENC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OSAL_Free(pVideoEnc);
    pSECComponent->hComponentHandle = pVideoEnc = NULL;

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
