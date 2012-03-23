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
 * @file       SEC_OMX_Baseport.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEC_OMX_Macros.h"
#include "SEC_OSAL_Event.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Mutex.h"

#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Basecomponent.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_BASE_PORT"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"


OMX_ERRORTYPE SEC_OMX_FlushPort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_BUFFERHEADERTYPE  *bufferHeader = NULL;
    SEC_OMX_MESSAGE       *message = NULL;
    OMX_U32                flushNum = 0;
    OMX_S32                semValue = 0;

    FunctionIn();

    pSECPort = &pSECComponent->pSECPort[portIndex];
    while (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) > 0) {
        SEC_OSAL_Get_SemaphoreCount(pSECComponent->pSECPort[portIndex].bufferSemID, &semValue);
        if (semValue == 0)
            SEC_OSAL_SemaphorePost(pSECComponent->pSECPort[portIndex].bufferSemID);
        SEC_OSAL_SemaphoreWait(pSECComponent->pSECPort[portIndex].bufferSemID);

        message = (SEC_OMX_MESSAGE *)SEC_OSAL_Dequeue(&pSECPort->bufferQ);
        if (message != NULL) {
            bufferHeader = (OMX_BUFFERHEADERTYPE *)message->pCmdData;
            bufferHeader->nFilledLen = 0;

            if (CHECK_PORT_TUNNELED(pSECPort) && !CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                if (portIndex) {
                    OMX_EmptyThisBuffer(pSECPort->tunneledComponent, bufferHeader);
                } else {
                    OMX_FillThisBuffer(pSECPort->tunneledComponent, bufferHeader);
                }
                SEC_OSAL_Free(message);
                message = NULL;
            } else if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                SEC_OSAL_Log(SEC_LOG_ERROR, "Tunneled mode is not working, Line:%d", __LINE__);
                ret = OMX_ErrorNotImplemented;
                SEC_OSAL_Queue(&pSECPort->bufferQ, pSECPort);
                goto EXIT;
            } else {
                if (portIndex == OUTPUT_PORT_INDEX) {
                    pSECComponent->pCallbacks->FillBufferDone(pOMXComponent, pSECComponent->callbackData, bufferHeader);
                } else {
                    pSECComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pSECComponent->callbackData, bufferHeader);
                }

                SEC_OSAL_Free(message);
                message = NULL;
            }
        }
    }

    if (pSECComponent->secDataBuffer[portIndex].dataValid == OMX_TRUE) {
        if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
            message = SEC_OSAL_Malloc(sizeof(SEC_OMX_MESSAGE));
            message->pCmdData = pSECComponent->secDataBuffer[portIndex].bufferHeader;
            message->messageType = 0;
            message->messageParam = -1;
            SEC_OSAL_Queue(&pSECPort->bufferQ, message);
            pSECComponent->sec_BufferReset(pOMXComponent, portIndex);
        } else {
            if (portIndex == INPUT_PORT_INDEX)
                pSECComponent->sec_InputBufferReturn(pOMXComponent);
            else if (portIndex == OUTPUT_PORT_INDEX)
                pSECComponent->sec_OutputBufferReturn(pOMXComponent);
        }
    }

    if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
        while (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) < (int)pSECPort->assignedBufferNum) {
            SEC_OSAL_SemaphoreWait(pSECComponent->pSECPort[portIndex].bufferSemID);
        }
        if (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) != (int)pSECPort->assignedBufferNum)
            SEC_OSAL_SetElemNum(&pSECPort->bufferQ, pSECPort->assignedBufferNum);
    } else {
        while(1) {
            OMX_S32 cnt = 0;
            SEC_OSAL_Get_SemaphoreCount(pSECComponent->pSECPort[portIndex].bufferSemID, &cnt);
            if (cnt <= 0)
                break;
            SEC_OSAL_SemaphoreWait(pSECComponent->pSECPort[portIndex].bufferSemID);
        }
        SEC_OSAL_SetElemNum(&pSECPort->bufferQ, 0);
    }

    pSECComponent->processData[portIndex].dataLen       = 0;
    pSECComponent->processData[portIndex].nFlags        = 0;
    pSECComponent->processData[portIndex].remainDataLen = 0;
    pSECComponent->processData[portIndex].timeStamp     = 0;
    pSECComponent->processData[portIndex].usedDataLen   = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_BufferFlushProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    SEC_OMX_DATABUFFER    *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        SEC_OSAL_SignalSet(pSECComponent->pauseEvent);

        flushBuffer = &pSECComponent->secDataBuffer[portIndex];

        SEC_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = SEC_OMX_FlushPort(pOMXComponent, portIndex);
        SEC_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pSECComponent->pSECPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (ret == OMX_ErrorNone) {
            SEC_OSAL_Log(SEC_LOG_TRACE,"OMX_CommandFlush EventCmdComplete");
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandFlush, portIndex, NULL);
        }

        if (portIndex == INPUT_PORT_INDEX) {
            pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pSECComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            pSECComponent->bSaveFlagEOS = OMX_FALSE;
            pSECComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pSECComponent->remainOutputData = OMX_FALSE;
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pSECComponent != NULL)) {
            pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_BufferFlushProcessNoEvent(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    SEC_OMX_DATABUFFER    *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pSECComponent->pSECPort[portIndex].bIsPortFlushed = OMX_TRUE;

        SEC_OSAL_SignalSet(pSECComponent->pauseEvent);

        flushBuffer = &pSECComponent->secDataBuffer[portIndex];

        SEC_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = SEC_OMX_FlushPort(pOMXComponent, portIndex);
        SEC_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pSECComponent->pSECPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (portIndex == INPUT_PORT_INDEX) {
            pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pSECComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            pSECComponent->bSaveFlagEOS = OMX_FALSE;
            pSECComponent->remainOutputData = OMX_FALSE;
            pSECComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pSECComponent->remainOutputData = OMX_FALSE;
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pSECComponent != NULL)) {
        pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}


OMX_ERRORTYPE SEC_OMX_EnablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    pSECPort = &pSECComponent->pSECPort[portIndex];
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
        ret = pSECComponent->sec_AllocateTunnelBuffer(pSECPort, portIndex);
        if (OMX_ErrorNone != ret) {
            goto EXIT;
        }
        pSECPort->portDefinition.bPopulated = OMX_TRUE;
        if (pSECComponent->currentState == OMX_StateExecuting) {
            for (i=0; i<pSECPort->tunnelBufferNum; i++) {
                SEC_OSAL_SemaphorePost(pSECComponent->pSECPort[portIndex].bufferSemID);
            }
        }
    } else if (CHECK_PORT_TUNNELED(pSECPort) && !CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            SEC_OSAL_SemaphoreWait(pSECPort->loadedResource);
            pSECPort->portDefinition.bPopulated = OMX_TRUE;
        }
    } else {
        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            SEC_OSAL_SemaphoreWait(pSECPort->loadedResource);
            pSECPort->portDefinition.bPopulated = OMX_TRUE;
        }
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_PortEnableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = SEC_OMX_EnablePort(pOMXComponent, portIndex);
        if (ret == OMX_ErrorNone) {
            pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortEnable, portIndex, NULL);
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pSECComponent != NULL)) {
            pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
        }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_DisablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_U32                i = 0, elemNum = 0;
    SEC_OMX_MESSAGE       *message;

    FunctionIn();

    pSECPort = &pSECComponent->pSECPort[portIndex];

    if (!CHECK_PORT_ENABLED(pSECPort)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pSECComponent->currentState!=OMX_StateLoaded) {
        if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
            while (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) >0 ) {
                message = (SEC_OMX_MESSAGE*)SEC_OSAL_Dequeue(&pSECPort->bufferQ);
                SEC_OSAL_Free(message);
            }
            ret = pSECComponent->sec_FreeTunnelBuffer(pSECPort, portIndex);
            if (OMX_ErrorNone != ret) {
                goto EXIT;
            }
            pSECPort->portDefinition.bPopulated = OMX_FALSE;
        } else if (CHECK_PORT_TUNNELED(pSECPort) && !CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
            pSECPort->portDefinition.bPopulated = OMX_FALSE;
            SEC_OSAL_SemaphoreWait(pSECPort->unloadedResource);
        } else {
            if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                while (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) >0 ) {
                    message = (SEC_OMX_MESSAGE*)SEC_OSAL_Dequeue(&pSECPort->bufferQ);
                    SEC_OSAL_Free(message);
                }
            }
            pSECPort->portDefinition.bPopulated = OMX_FALSE;
            SEC_OSAL_SemaphoreWait(pSECPort->unloadedResource);
        }
    }
    pSECPort->portDefinition.bEnabled = OMX_FALSE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_PortDisableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    SEC_OMX_DATABUFFER      *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    /* port flush*/
    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pSECComponent->pSECPort[portIndex].bIsPortFlushed = OMX_TRUE;

        flushBuffer = &pSECComponent->secDataBuffer[portIndex];

        SEC_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = SEC_OMX_FlushPort(pOMXComponent, portIndex);
        SEC_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pSECComponent->pSECPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (portIndex == INPUT_PORT_INDEX) {
            pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pSECComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            pSECComponent->bSaveFlagEOS = OMX_FALSE;
            pSECComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pSECComponent->remainOutputData = OMX_FALSE;
        }
    }

    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = SEC_OMX_DisablePort(pOMXComponent, portIndex);
        pSECComponent->pSECPort[portIndex].bIsPortDisabled = OMX_FALSE;
        if (ret == OMX_ErrorNone) {
            pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                            pSECComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortDisable, portIndex, NULL);
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pSECComponent != NULL)) {
        pSECComponent->pCallbacks->EventHandler(pOMXComponent,
                        pSECComponent->callbackData,
                        OMX_EventError,
                        ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_EmptyThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_BOOL               findBuffer = OMX_FALSE;
    SEC_OMX_MESSAGE       *message;
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
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nInputPortIndex != INPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = SEC_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pSECComponent->currentState != OMX_StateIdle) &&
        (pSECComponent->currentState != OMX_StateExecuting) &&
        (pSECComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pSECPort)) ||
        ((CHECK_PORT_BEING_FLUSHED(pSECPort) || CHECK_PORT_BEING_DISABLED(pSECPort)) &&
        (!CHECK_PORT_TUNNELED(pSECPort) || !CHECK_PORT_BUFFER_SUPPLIER(pSECPort))) ||
        ((pSECComponent->transientState == SEC_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pSECPort) && !CHECK_PORT_BUFFER_SUPPLIER(pSECPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    for (i = 0; i < pSECPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pSECPort->bufferHeader[i]) {
            findBuffer = OMX_TRUE;
            break;
        }
    }

    if (findBuffer == OMX_FALSE) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    } else {
        ret = OMX_ErrorNone;
    }

    message = SEC_OSAL_Malloc(sizeof(SEC_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    message->messageType = SEC_OMX_CommandEmptyBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = SEC_OSAL_Queue(&pSECPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    ret = SEC_OSAL_SemaphorePost(pSECPort->bufferSemID);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_FillThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_BOOL               findBuffer = OMX_FALSE;
    SEC_OMX_MESSAGE       *message;
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
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nOutputPortIndex != OUTPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = SEC_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pSECComponent->currentState != OMX_StateIdle) &&
        (pSECComponent->currentState != OMX_StateExecuting) &&
        (pSECComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pSECPort)) ||
        ((CHECK_PORT_BEING_FLUSHED(pSECPort) || CHECK_PORT_BEING_DISABLED(pSECPort)) &&
        (!CHECK_PORT_TUNNELED(pSECPort) || !CHECK_PORT_BUFFER_SUPPLIER(pSECPort))) ||
        ((pSECComponent->transientState == SEC_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pSECPort) && !CHECK_PORT_BUFFER_SUPPLIER(pSECPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    for (i = 0; i < pSECPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pSECPort->bufferHeader[i]) {
            findBuffer = OMX_TRUE;
            break;
        }
    }

    if (findBuffer == OMX_FALSE) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    } else {
        ret = OMX_ErrorNone;
    }

    message = SEC_OSAL_Malloc(sizeof(SEC_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    message->messageType = SEC_OMX_CommandFillBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = SEC_OSAL_Queue(&pSECPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    ret = SEC_OSAL_SemaphorePost(pSECPort->bufferSemID);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_Port_Constructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    SEC_OMX_BASEPORT      *pSECInputPort = NULL;
    SEC_OMX_BASEPORT      *pSECOutputPort = NULL;
    int i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    INIT_SET_SIZE_VERSION(&pSECComponent->portParam, OMX_PORT_PARAM_TYPE);
    pSECComponent->portParam.nPorts = ALL_PORT_NUM;
    pSECComponent->portParam.nStartPortNumber = INPUT_PORT_INDEX;

    pSECPort = SEC_OSAL_Malloc(sizeof(SEC_OMX_BASEPORT) * ALL_PORT_NUM);
    if (pSECPort == NULL) {
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECPort, 0, sizeof(SEC_OMX_BASEPORT) * ALL_PORT_NUM);
    pSECComponent->pSECPort = pSECPort;

    /* Input Port */
    pSECInputPort = &pSECPort[INPUT_PORT_INDEX];

    SEC_OSAL_QueueCreate(&pSECInputPort->bufferQ);

    pSECInputPort->bufferHeader = SEC_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);
    if (pSECInputPort->bufferHeader == NULL) {
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECInputPort->bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);

    pSECInputPort->bufferStateAllocate = SEC_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pSECInputPort->bufferStateAllocate == NULL) {
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECInputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pSECInputPort->bufferSemID = NULL;
    pSECInputPort->assignedBufferNum = 0;
    pSECInputPort->portState = OMX_StateMax;
    pSECInputPort->bIsPortFlushed = OMX_FALSE;
    pSECInputPort->bIsPortDisabled = OMX_FALSE;
    pSECInputPort->tunneledComponent = NULL;
    pSECInputPort->tunneledPort = 0;
    pSECInputPort->tunnelBufferNum = 0;
    pSECInputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pSECInputPort->tunnelFlags = 0;
    ret = SEC_OSAL_SemaphoreCreate(&pSECInputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Free(pSECInputPort->bufferStateAllocate);
        pSECInputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        goto EXIT;
    }
    ret = SEC_OSAL_SemaphoreCreate(&pSECInputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_SemaphoreTerminate(pSECInputPort->loadedResource);
        pSECInputPort->loadedResource = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferStateAllocate);
        pSECInputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        goto EXIT;
    }

    INIT_SET_SIZE_VERSION(&pSECInputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pSECInputPort->portDefinition.nPortIndex = INPUT_PORT_INDEX;
    pSECInputPort->portDefinition.eDir = OMX_DirInput;
    pSECInputPort->portDefinition.nBufferCountActual = 0;
    pSECInputPort->portDefinition.nBufferCountMin = 0;
    pSECInputPort->portDefinition.nBufferSize = 0;
    pSECInputPort->portDefinition.bEnabled = OMX_FALSE;
    pSECInputPort->portDefinition.bPopulated = OMX_FALSE;
    pSECInputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pSECInputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pSECInputPort->portDefinition.nBufferAlignment = 0;
    pSECInputPort->markType.hMarkTargetComponent = NULL;
    pSECInputPort->markType.pMarkData = NULL;

    /* Output Port */
    pSECOutputPort = &pSECPort[OUTPUT_PORT_INDEX];

    SEC_OSAL_QueueCreate(&pSECOutputPort->bufferQ);

    pSECOutputPort->bufferHeader = SEC_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);
    if (pSECOutputPort->bufferHeader == NULL) {
        SEC_OSAL_SemaphoreTerminate(pSECInputPort->unloadedResource);
        pSECInputPort->unloadedResource = NULL;
        SEC_OSAL_SemaphoreTerminate(pSECInputPort->loadedResource);
        pSECInputPort->loadedResource = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferStateAllocate);
        pSECInputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECOutputPort->bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);

    pSECOutputPort->bufferStateAllocate = SEC_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pSECOutputPort->bufferStateAllocate == NULL) {
        SEC_OSAL_Free(pSECOutputPort->bufferHeader);
        pSECOutputPort->bufferHeader = NULL;

        SEC_OSAL_SemaphoreTerminate(pSECInputPort->unloadedResource);
        pSECInputPort->unloadedResource = NULL;
        SEC_OSAL_SemaphoreTerminate(pSECInputPort->loadedResource);
        pSECInputPort->loadedResource = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferStateAllocate);
        pSECInputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECOutputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pSECOutputPort->bufferSemID = NULL;
    pSECOutputPort->assignedBufferNum = 0;
    pSECOutputPort->portState = OMX_StateMax;
    pSECOutputPort->bIsPortFlushed = OMX_FALSE;
    pSECOutputPort->bIsPortDisabled = OMX_FALSE;
    pSECOutputPort->tunneledComponent = NULL;
    pSECOutputPort->tunneledPort = 0;
    pSECOutputPort->tunnelBufferNum = 0;
    pSECOutputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pSECOutputPort->tunnelFlags = 0;
    ret = SEC_OSAL_SemaphoreCreate(&pSECOutputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Free(pSECOutputPort->bufferStateAllocate);
        pSECOutputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECOutputPort->bufferHeader);
        pSECOutputPort->bufferHeader = NULL;

        SEC_OSAL_SemaphoreTerminate(pSECInputPort->unloadedResource);
        pSECInputPort->unloadedResource = NULL;
        SEC_OSAL_SemaphoreTerminate(pSECInputPort->loadedResource);
        pSECInputPort->loadedResource = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferStateAllocate);
        pSECInputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        goto EXIT;
    }
    ret = SEC_OSAL_SemaphoreCreate(&pSECOutputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_SemaphoreTerminate(pSECOutputPort->loadedResource);
        pSECOutputPort->loadedResource = NULL;
        SEC_OSAL_Free(pSECOutputPort->bufferStateAllocate);
        pSECOutputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECOutputPort->bufferHeader);
        pSECOutputPort->bufferHeader = NULL;

        SEC_OSAL_SemaphoreTerminate(pSECInputPort->unloadedResource);
        pSECInputPort->unloadedResource = NULL;
        SEC_OSAL_SemaphoreTerminate(pSECInputPort->loadedResource);
        pSECInputPort->loadedResource = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferStateAllocate);
        pSECInputPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECInputPort->bufferHeader);
        pSECInputPort->bufferHeader = NULL;
        SEC_OSAL_Free(pSECPort);
        pSECPort = NULL;
        goto EXIT;
    }

    INIT_SET_SIZE_VERSION(&pSECOutputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pSECOutputPort->portDefinition.nPortIndex = OUTPUT_PORT_INDEX;
    pSECOutputPort->portDefinition.eDir = OMX_DirOutput;
    pSECOutputPort->portDefinition.nBufferCountActual = 0;
    pSECOutputPort->portDefinition.nBufferCountMin = 0;
    pSECOutputPort->portDefinition.nBufferSize = 0;
    pSECOutputPort->portDefinition.bEnabled = OMX_FALSE;
    pSECOutputPort->portDefinition.bPopulated = OMX_FALSE;
    pSECOutputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pSECOutputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pSECOutputPort->portDefinition.nBufferAlignment = 0;
    pSECOutputPort->markType.hMarkTargetComponent = NULL;
    pSECOutputPort->markType.pMarkData = NULL;

    pSECComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
    pSECComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
    pSECComponent->checkTimeStamp.startTimeStamp = 0;
    pSECComponent->checkTimeStamp.nStartFlags = 0x0;

    pOMXComponent->EmptyThisBuffer = &SEC_OMX_EmptyThisBuffer;
    pOMXComponent->FillThisBuffer  = &SEC_OMX_FillThisBuffer;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_Port_Destructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_BASEPORT      *pSECPort = NULL;

    FunctionIn();

    int i = 0;

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
    for (i = 0; i < ALL_PORT_NUM; i++) {
        pSECPort = &pSECComponent->pSECPort[i];

        SEC_OSAL_SemaphoreTerminate(pSECPort->loadedResource);
        pSECPort->loadedResource = NULL;
        SEC_OSAL_SemaphoreTerminate(pSECPort->unloadedResource);
        pSECPort->unloadedResource = NULL;
        SEC_OSAL_Free(pSECPort->bufferStateAllocate);
        pSECPort->bufferStateAllocate = NULL;
        SEC_OSAL_Free(pSECPort->bufferHeader);
        pSECPort->bufferHeader = NULL;

        SEC_OSAL_QueueTerminate(&pSECPort->bufferQ);
    }
    SEC_OSAL_Free(pSECComponent->pSECPort);
    pSECComponent->pSECPort = NULL;
    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}
