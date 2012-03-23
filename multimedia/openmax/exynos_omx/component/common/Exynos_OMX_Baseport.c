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
 * @file       Exynos_OMX_Baseport.c
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

#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"

#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_BASE_PORT"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


OMX_ERRORTYPE Exynos_OMX_FlushPort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE  *bufferHeader = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;
    OMX_U32                flushNum = 0;
    OMX_S32                semValue = 0;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];
    while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
        Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[portIndex].bufferSemID, &semValue);
        if (semValue == 0)
            Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[portIndex].bufferSemID);
        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);

        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
        if (message != NULL) {
            bufferHeader = (OMX_BUFFERHEADERTYPE *)message->pCmdData;
            bufferHeader->nFilledLen = 0;

            if (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                if (portIndex) {
                    OMX_EmptyThisBuffer(pExynosPort->tunneledComponent, bufferHeader);
                } else {
                    OMX_FillThisBuffer(pExynosPort->tunneledComponent, bufferHeader);
                }
                Exynos_OSAL_Free(message);
                message = NULL;
            } else if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Tunneled mode is not working, Line:%d", __LINE__);
                ret = OMX_ErrorNotImplemented;
                Exynos_OSAL_Queue(&pExynosPort->bufferQ, pExynosPort);
                goto EXIT;
            } else {
                if (portIndex == OUTPUT_PORT_INDEX) {
                    pExynosComponent->pCallbacks->FillBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
                } else {
                    pExynosComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
                }

                Exynos_OSAL_Free(message);
                message = NULL;
            }
        }
    }

    if (pExynosComponent->exynosDataBuffer[portIndex].dataValid == OMX_TRUE) {
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            message = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));
            message->pCmdData = pExynosComponent->exynosDataBuffer[portIndex].bufferHeader;
            message->messageType = 0;
            message->messageParam = -1;
            Exynos_OSAL_Queue(&pExynosPort->bufferQ, message);
            pExynosComponent->exynos_BufferReset(pOMXComponent, portIndex);
        } else {
            if (portIndex == INPUT_PORT_INDEX)
                pExynosComponent->exynos_InputBufferReturn(pOMXComponent);
            else if (portIndex == OUTPUT_PORT_INDEX)
                pExynosComponent->exynos_OutputBufferReturn(pOMXComponent);
        }
    }

    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) < (int)pExynosPort->assignedBufferNum) {
            Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);
        }
        if (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) != (int)pExynosPort->assignedBufferNum)
            Exynos_OSAL_SetElemNum(&pExynosPort->bufferQ, pExynosPort->assignedBufferNum);
    } else {
        while(1) {
            OMX_S32 cnt = 0;
            Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[portIndex].bufferSemID, &cnt);
            if (cnt <= 0)
                break;
            Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);
        }
        Exynos_OSAL_SetElemNum(&pExynosPort->bufferQ, 0);
    }

    pExynosComponent->processData[portIndex].dataLen       = 0;
    pExynosComponent->processData[portIndex].nFlags        = 0;
    pExynosComponent->processData[portIndex].remainDataLen = 0;
    pExynosComponent->processData[portIndex].timeStamp     = 0;
    pExynosComponent->processData[portIndex].usedDataLen   = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferFlushProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    EXYNOS_OMX_DATABUFFER    *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);

        flushBuffer = &pExynosComponent->exynosDataBuffer[portIndex];

        Exynos_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = Exynos_OMX_FlushPort(pOMXComponent, portIndex);
        Exynos_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pExynosComponent->pExynosPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (ret == OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE,"OMX_CommandFlush EventCmdComplete");
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandFlush, portIndex, NULL);
        }

        if (portIndex == INPUT_PORT_INDEX) {
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            pExynosComponent->bSaveFlagEOS = OMX_FALSE;
            pExynosComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pExynosComponent->remainOutputData = OMX_FALSE;
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferFlushProcessNoEvent(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    EXYNOS_OMX_DATABUFFER    *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pExynosComponent->pExynosPort[portIndex].bIsPortFlushed = OMX_TRUE;

        Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);

        flushBuffer = &pExynosComponent->exynosDataBuffer[portIndex];

        Exynos_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = Exynos_OMX_FlushPort(pOMXComponent, portIndex);
        Exynos_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pExynosComponent->pExynosPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (portIndex == INPUT_PORT_INDEX) {
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            pExynosComponent->bSaveFlagEOS = OMX_FALSE;
            pExynosComponent->remainOutputData = OMX_FALSE;
            pExynosComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pExynosComponent->remainOutputData = OMX_FALSE;
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
        pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}


OMX_ERRORTYPE Exynos_OMX_EnablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        ret = pExynosComponent->exynos_AllocateTunnelBuffer(pExynosPort, portIndex);
        if (OMX_ErrorNone != ret) {
            goto EXIT;
        }
        pExynosPort->portDefinition.bPopulated = OMX_TRUE;
        if (pExynosComponent->currentState == OMX_StateExecuting) {
            for (i=0; i<pExynosPort->tunnelBufferNum; i++) {
                Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[portIndex].bufferSemID);
            }
        }
    } else if (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            Exynos_OSAL_SemaphoreWait(pExynosPort->loadedResource);
            pExynosPort->portDefinition.bPopulated = OMX_TRUE;
        }
    } else {
        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            Exynos_OSAL_SemaphoreWait(pExynosPort->loadedResource);
            pExynosPort->portDefinition.bPopulated = OMX_TRUE;
        }
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_PortEnableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = Exynos_OMX_EnablePort(pOMXComponent, portIndex);
        if (ret == OMX_ErrorNone) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortEnable, portIndex, NULL);
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
        }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_DisablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_U32                i = 0, elemNum = 0;
    EXYNOS_OMX_MESSAGE       *message;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];

    if (!CHECK_PORT_ENABLED(pExynosPort)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pExynosComponent->currentState!=OMX_StateLoaded) {
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) >0 ) {
                message = (EXYNOS_OMX_MESSAGE*)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
                Exynos_OSAL_Free(message);
            }
            ret = pExynosComponent->exynos_FreeTunnelBuffer(pExynosPort, portIndex);
            if (OMX_ErrorNone != ret) {
                goto EXIT;
            }
            pExynosPort->portDefinition.bPopulated = OMX_FALSE;
        } else if (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            pExynosPort->portDefinition.bPopulated = OMX_FALSE;
            Exynos_OSAL_SemaphoreWait(pExynosPort->unloadedResource);
        } else {
            if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) >0 ) {
                    message = (EXYNOS_OMX_MESSAGE*)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
                    Exynos_OSAL_Free(message);
                }
            }
            pExynosPort->portDefinition.bPopulated = OMX_FALSE;
            Exynos_OSAL_SemaphoreWait(pExynosPort->unloadedResource);
        }
    }
    pExynosPort->portDefinition.bEnabled = OMX_FALSE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_PortDisableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    EXYNOS_OMX_DATABUFFER      *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    /* port flush*/
    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pExynosComponent->pExynosPort[portIndex].bIsPortFlushed = OMX_TRUE;

        flushBuffer = &pExynosComponent->exynosDataBuffer[portIndex];

        Exynos_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = Exynos_OMX_FlushPort(pOMXComponent, portIndex);
        Exynos_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pExynosComponent->pExynosPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (portIndex == INPUT_PORT_INDEX) {
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            pExynosComponent->bSaveFlagEOS = OMX_FALSE;
            pExynosComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pExynosComponent->remainOutputData = OMX_FALSE;
        }
    }

    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = Exynos_OMX_DisablePort(pOMXComponent, portIndex);
        pExynosComponent->pExynosPort[portIndex].bIsPortDisabled = OMX_FALSE;
        if (ret == OMX_ErrorNone) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortDisable, portIndex, NULL);
        }
    }

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
        pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                        pExynosComponent->callbackData,
                        OMX_EventError,
                        ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_EmptyThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BOOL               findBuffer = OMX_FALSE;
    EXYNOS_OMX_MESSAGE       *message;
    OMX_U32                i = 0;

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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
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

    ret = Exynos_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pExynosComponent->currentState != OMX_StateIdle) &&
        (pExynosComponent->currentState != OMX_StateExecuting) &&
        (pExynosComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pExynosPort)) ||
        ((CHECK_PORT_BEING_FLUSHED(pExynosPort) || CHECK_PORT_BEING_DISABLED(pExynosPort)) &&
        (!CHECK_PORT_TUNNELED(pExynosPort) || !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort))) ||
        ((pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pExynosPort->bufferHeader[i]) {
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

    message = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    message->messageType = EXYNOS_OMX_CommandEmptyBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = Exynos_OSAL_Queue(&pExynosPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    ret = Exynos_OSAL_SemaphorePost(pExynosPort->bufferSemID);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FillThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BOOL               findBuffer = OMX_FALSE;
    EXYNOS_OMX_MESSAGE       *message;
    OMX_U32                i = 0;

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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
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

    ret = Exynos_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pExynosComponent->currentState != OMX_StateIdle) &&
        (pExynosComponent->currentState != OMX_StateExecuting) &&
        (pExynosComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pExynosPort)) ||
        ((CHECK_PORT_BEING_FLUSHED(pExynosPort) || CHECK_PORT_BEING_DISABLED(pExynosPort)) &&
        (!CHECK_PORT_TUNNELED(pExynosPort) || !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort))) ||
        ((pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pExynosPort) && !CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pExynosPort->bufferHeader[i]) {
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

    message = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    message->messageType = EXYNOS_OMX_CommandFillBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    ret = Exynos_OSAL_Queue(&pExynosPort->bufferQ, (void *)message);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    ret = Exynos_OSAL_SemaphorePost(pExynosPort->bufferSemID);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Port_Constructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = NULL;
    int i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    INIT_SET_SIZE_VERSION(&pExynosComponent->portParam, OMX_PORT_PARAM_TYPE);
    pExynosComponent->portParam.nPorts = ALL_PORT_NUM;
    pExynosComponent->portParam.nStartPortNumber = INPUT_PORT_INDEX;

    pExynosPort = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_BASEPORT) * ALL_PORT_NUM);
    if (pExynosPort == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosPort, 0, sizeof(EXYNOS_OMX_BASEPORT) * ALL_PORT_NUM);
    pExynosComponent->pExynosPort = pExynosPort;

    /* Input Port */
    pExynosInputPort = &pExynosPort[INPUT_PORT_INDEX];

    Exynos_OSAL_QueueCreate(&pExynosInputPort->bufferQ);

    pExynosInputPort->bufferHeader = Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);
    if (pExynosInputPort->bufferHeader == NULL) {
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosInputPort->bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);

    pExynosInputPort->bufferStateAllocate = Exynos_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pExynosInputPort->bufferStateAllocate == NULL) {
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosInputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pExynosInputPort->bufferSemID = NULL;
    pExynosInputPort->assignedBufferNum = 0;
    pExynosInputPort->portState = OMX_StateMax;
    pExynosInputPort->bIsPortFlushed = OMX_FALSE;
    pExynosInputPort->bIsPortDisabled = OMX_FALSE;
    pExynosInputPort->tunneledComponent = NULL;
    pExynosInputPort->tunneledPort = 0;
    pExynosInputPort->tunnelBufferNum = 0;
    pExynosInputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pExynosInputPort->tunnelFlags = 0;
    ret = Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Free(pExynosInputPort->bufferStateAllocate);
        pExynosInputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        goto EXIT;
    }
    ret = Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->loadedResource);
        pExynosInputPort->loadedResource = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferStateAllocate);
        pExynosInputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        goto EXIT;
    }

    INIT_SET_SIZE_VERSION(&pExynosInputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pExynosInputPort->portDefinition.nPortIndex = INPUT_PORT_INDEX;
    pExynosInputPort->portDefinition.eDir = OMX_DirInput;
    pExynosInputPort->portDefinition.nBufferCountActual = 0;
    pExynosInputPort->portDefinition.nBufferCountMin = 0;
    pExynosInputPort->portDefinition.nBufferSize = 0;
    pExynosInputPort->portDefinition.bEnabled = OMX_FALSE;
    pExynosInputPort->portDefinition.bPopulated = OMX_FALSE;
    pExynosInputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pExynosInputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pExynosInputPort->portDefinition.nBufferAlignment = 0;
    pExynosInputPort->markType.hMarkTargetComponent = NULL;
    pExynosInputPort->markType.pMarkData = NULL;

    /* Output Port */
    pExynosOutputPort = &pExynosPort[OUTPUT_PORT_INDEX];

    Exynos_OSAL_QueueCreate(&pExynosOutputPort->bufferQ);

    pExynosOutputPort->bufferHeader = Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);
    if (pExynosOutputPort->bufferHeader == NULL) {
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->unloadedResource);
        pExynosInputPort->unloadedResource = NULL;
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->loadedResource);
        pExynosInputPort->loadedResource = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferStateAllocate);
        pExynosInputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosOutputPort->bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);

    pExynosOutputPort->bufferStateAllocate = Exynos_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pExynosOutputPort->bufferStateAllocate == NULL) {
        Exynos_OSAL_Free(pExynosOutputPort->bufferHeader);
        pExynosOutputPort->bufferHeader = NULL;

        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->unloadedResource);
        pExynosInputPort->unloadedResource = NULL;
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->loadedResource);
        pExynosInputPort->loadedResource = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferStateAllocate);
        pExynosInputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosOutputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pExynosOutputPort->bufferSemID = NULL;
    pExynosOutputPort->assignedBufferNum = 0;
    pExynosOutputPort->portState = OMX_StateMax;
    pExynosOutputPort->bIsPortFlushed = OMX_FALSE;
    pExynosOutputPort->bIsPortDisabled = OMX_FALSE;
    pExynosOutputPort->tunneledComponent = NULL;
    pExynosOutputPort->tunneledPort = 0;
    pExynosOutputPort->tunnelBufferNum = 0;
    pExynosOutputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pExynosOutputPort->tunnelFlags = 0;
    ret = Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Free(pExynosOutputPort->bufferStateAllocate);
        pExynosOutputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosOutputPort->bufferHeader);
        pExynosOutputPort->bufferHeader = NULL;

        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->unloadedResource);
        pExynosInputPort->unloadedResource = NULL;
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->loadedResource);
        pExynosInputPort->loadedResource = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferStateAllocate);
        pExynosInputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        goto EXIT;
    }
    ret = Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_SemaphoreTerminate(pExynosOutputPort->loadedResource);
        pExynosOutputPort->loadedResource = NULL;
        Exynos_OSAL_Free(pExynosOutputPort->bufferStateAllocate);
        pExynosOutputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosOutputPort->bufferHeader);
        pExynosOutputPort->bufferHeader = NULL;

        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->unloadedResource);
        pExynosInputPort->unloadedResource = NULL;
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->loadedResource);
        pExynosInputPort->loadedResource = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferStateAllocate);
        pExynosInputPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosInputPort->bufferHeader);
        pExynosInputPort->bufferHeader = NULL;
        Exynos_OSAL_Free(pExynosPort);
        pExynosPort = NULL;
        goto EXIT;
    }

    INIT_SET_SIZE_VERSION(&pExynosOutputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pExynosOutputPort->portDefinition.nPortIndex = OUTPUT_PORT_INDEX;
    pExynosOutputPort->portDefinition.eDir = OMX_DirOutput;
    pExynosOutputPort->portDefinition.nBufferCountActual = 0;
    pExynosOutputPort->portDefinition.nBufferCountMin = 0;
    pExynosOutputPort->portDefinition.nBufferSize = 0;
    pExynosOutputPort->portDefinition.bEnabled = OMX_FALSE;
    pExynosOutputPort->portDefinition.bPopulated = OMX_FALSE;
    pExynosOutputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pExynosOutputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pExynosOutputPort->portDefinition.nBufferAlignment = 0;
    pExynosOutputPort->markType.hMarkTargetComponent = NULL;
    pExynosOutputPort->markType.pMarkData = NULL;

    pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
    pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
    pExynosComponent->checkTimeStamp.startTimeStamp = 0;
    pExynosComponent->checkTimeStamp.nStartFlags = 0x0;

    pOMXComponent->EmptyThisBuffer = &Exynos_OMX_EmptyThisBuffer;
    pOMXComponent->FillThisBuffer  = &Exynos_OMX_FillThisBuffer;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Port_Destructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;

    FunctionIn();

    int i = 0;

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
    for (i = 0; i < ALL_PORT_NUM; i++) {
        pExynosPort = &pExynosComponent->pExynosPort[i];

        Exynos_OSAL_SemaphoreTerminate(pExynosPort->loadedResource);
        pExynosPort->loadedResource = NULL;
        Exynos_OSAL_SemaphoreTerminate(pExynosPort->unloadedResource);
        pExynosPort->unloadedResource = NULL;
        Exynos_OSAL_Free(pExynosPort->bufferStateAllocate);
        pExynosPort->bufferStateAllocate = NULL;
        Exynos_OSAL_Free(pExynosPort->bufferHeader);
        pExynosPort->bufferHeader = NULL;

        Exynos_OSAL_QueueTerminate(&pExynosPort->bufferQ);
    }
    Exynos_OSAL_Free(pExynosComponent->pExynosPort);
    pExynosComponent->pExynosPort = NULL;
    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}
