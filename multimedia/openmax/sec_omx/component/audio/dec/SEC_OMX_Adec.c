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
 * @file        SEC_OMX_Adec.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 *
 * @version     1.1.0
 * @history
 *   2011.10.18 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SEC_OMX_Macros.h"
#include "SEC_OSAL_Event.h"
#include "SEC_OMX_Adec.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OSAL_Thread.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Mutex.h"
#include "SEC_OSAL_ETC.h"
#include "srp_api.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_AUDIO_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

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
    SEC_OMX_BASEPORT      *secInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
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

        /* Audio extractor should parse into frame units. */
        flagEOF = OMX_TRUE;
        checkedSize = checkInputStreamLen;
        copySize = checkedSize;

        if (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            pSECComponent->bSaveFlagEOS = OMX_TRUE;

        if (((inputData->allocSize) - (inputData->dataLen)) >= copySize) {
            if (copySize > 0)
                SEC_OSAL_Memcpy(inputData->dataBuffer + inputData->dataLen, checkInputStream, copySize);

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
        } else {
            /*????????????????????????????????? Error ?????????????????????????????????*/
            SEC_DataReset(pOMXComponent, INPUT_PORT_INDEX);
            flagEOF = OMX_FALSE;
        }

        if ((inputUseBuffer->remainDataLen == 0) ||
            (CHECK_PORT_BEING_FLUSHED(secInputPort)))
            SEC_InputBufferReturn(pOMXComponent);
        else
            inputUseBuffer->dataValid = OMX_TRUE;
    }

    if (flagEOF == OMX_TRUE) {
        if (pSECComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            /* Flush SRP buffers */
            SRP_Flush();

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
            if (pSECComponent->checkTimeStamp.startTimeStamp == outputData->timeStamp) {
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
                (outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) ||
                (CHECK_PORT_BEING_FLUSHED(secOutputPort)))
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
    SEC_OMX_AUDIODEC_COMPONENT *pVideoDec = (SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle;
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

OMX_ERRORTYPE SEC_OMX_AudioDecodeGetParameter(
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
    case OMX_IndexParamAudioInit:
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
    case OMX_IndexParamAudioPortFormat:
    {
        OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        SEC_OMX_BASEPORT               *pSECPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0; /* supportFormatNum = N-1 */

        ret = SEC_OMX_Check_SizeVersion(portFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
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

            portFormat->eEncoding = portDefinition->format.audio.eEncoding;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            supportFormatNum = OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
            portDefinition = &pSECPort->portDefinition;

            portFormat->eEncoding = portDefinition->format.audio.eEncoding;
        }
        ret = OMX_ErrorNone;
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
OMX_ERRORTYPE SEC_OMX_AudioDecodeSetParameter(
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
    case OMX_IndexParamAudioPortFormat:
    {
        OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        SEC_OMX_BASEPORT               *pSECPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0; /* supportFormatNum = N-1 */

        ret = SEC_OMX_Check_SizeVersion(portFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pSECComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];
        portDefinition = &pSECPort->portDefinition;

        portDefinition->format.audio.eEncoding = portFormat->eEncoding;
        ret = OMX_ErrorNone;
    }
        break;
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

OMX_ERRORTYPE SEC_OMX_AudioDecodeGetConfig(
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

OMX_ERRORTYPE SEC_OMX_AudioDecodeSetConfig(
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
    case OMX_IndexConfigAudioMute:
    {
        SEC_OSAL_Log(SEC_LOG_TRACE, "OMX_IndexConfigAudioMute");
        ret = OMX_ErrorUnsupportedIndex;
    }
        break;
    case OMX_IndexConfigAudioVolume:
    {
        SEC_OSAL_Log(SEC_LOG_TRACE, "OMX_IndexConfigAudioVolume");
        ret = OMX_ErrorUnsupportedIndex;
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

OMX_ERRORTYPE SEC_OMX_AudioDecodeGetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
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

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    ret = SEC_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_AudioDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT      *pSECComponent = NULL;
    SEC_OMX_BASEPORT           *pSECPort = NULL;
    SEC_OMX_AUDIODEC_COMPONENT *pAudioDec = NULL;

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

    pAudioDec = SEC_OSAL_Malloc(sizeof(SEC_OMX_AUDIODEC_COMPONENT));
    if (pAudioDec == NULL) {
        SEC_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    SEC_OSAL_Memset(pAudioDec, 0, sizeof(SEC_OMX_AUDIODEC_COMPONENT));
    pSECComponent->hComponentHandle = (OMX_HANDLETYPE)pAudioDec;
    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    /* Input port */
    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECPort->portDefinition.nBufferCountActual = MAX_AUDIO_INPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferCountMin = MAX_AUDIO_INPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferSize = DEFAULT_AUDIO_INPUT_BUFFER_SIZE;
    pSECPort->portDefinition.eDomain = OMX_PortDomainAudio;

    pSECPort->portDefinition.format.audio.cMIMEType = SEC_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.audio.cMIMEType, "audio/raw");
    pSECPort->portDefinition.format.audio.pNativeRender = 0;
    pSECPort->portDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingUnused;

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.nBufferCountActual = MAX_AUDIO_OUTPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferCountMin = MAX_AUDIO_OUTPUTBUFFER_NUM;
    pSECPort->portDefinition.nBufferSize = DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE;
    pSECPort->portDefinition.eDomain = OMX_PortDomainAudio;

    pSECPort->portDefinition.format.audio.cMIMEType = SEC_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.audio.cMIMEType, "audio/raw");
    pSECPort->portDefinition.format.audio.pNativeRender = 0;
    pSECPort->portDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingUnused;


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

OMX_ERRORTYPE SEC_OMX_AudioDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT      *pSECComponent = NULL;
    SEC_OMX_BASEPORT           *pSECPort = NULL;
    SEC_OMX_AUDIODEC_COMPONENT *pAudioDec = NULL;
    int                         i = 0;

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

    pAudioDec = (SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_OSAL_Free(pAudioDec);
    pSECComponent->hComponentHandle = pAudioDec = NULL;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        pSECPort = &pSECComponent->pSECPort[i];
        SEC_OSAL_Free(pSECPort->portDefinition.format.audio.cMIMEType);
        pSECPort->portDefinition.format.audio.cMIMEType = NULL;
    }

    ret = SEC_OMX_Port_Destructor(pOMXComponent);

    ret = SEC_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}
