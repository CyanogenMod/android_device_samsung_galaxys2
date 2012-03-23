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
 * @file       SEC_OMX_Basecomponent.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SEC_OSAL_Event.h"
#include "SEC_OSAL_Thread.h"
#include "SEC_OSAL_ETC.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Mutex.h"
#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OMX_Resourcemanager.h"
#include "SEC_OMX_Macros.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_BASE_COMP"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"


/* Change CHECK_SIZE_VERSION Macro */
OMX_ERRORTYPE SEC_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_VERSIONTYPE* version = NULL;
    if (header == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    version = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    if (*((OMX_U32*)header) != size) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (version->s.nVersionMajor != VERSIONMAJOR_NUMBER ||
        version->s.nVersionMinor != VERSIONMINOR_NUMBER) {
        ret = OMX_ErrorVersionMismatch;
        goto EXIT;
    }
    ret = OMX_ErrorNone;
EXIT:
    return ret;
}

OMX_ERRORTYPE SEC_OMX_GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE   hComponent,
    OMX_OUT OMX_STRING       pComponentName,
    OMX_OUT OMX_VERSIONTYPE *pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE *pSpecVersion,
    OMX_OUT OMX_UUIDTYPE    *pComponentUUID)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    OMX_U32                compUUID[3];

    FunctionIn();

    /* check parameters */
    if (hComponent     == NULL ||
        pComponentName == NULL || pComponentVersion == NULL ||
        pSpecVersion   == NULL || pComponentUUID    == NULL) {
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

    SEC_OSAL_Strcpy(pComponentName, pSECComponent->componentName);
    SEC_OSAL_Memcpy(pComponentVersion, &(pSECComponent->componentVersion), sizeof(OMX_VERSIONTYPE));
    SEC_OSAL_Memcpy(pSpecVersion, &(pSECComponent->specVersion), sizeof(OMX_VERSIONTYPE));

    /* Fill UUID with handle address, PID and UID.
     * This should guarantee uiniqness */
    compUUID[0] = (OMX_U32)pOMXComponent;
    compUUID[1] = getpid();
    compUUID[2] = getuid();
    SEC_OSAL_Memcpy(*pComponentUUID, compUUID, 3 * sizeof(*compUUID));

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_GetState (
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pState == NULL) {
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

    *pState = pSECComponent->currentState;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SEC_OMX_BufferProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->sec_BufferProcess(pOMXComponent);

    SEC_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentStateSet(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 messageParam)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_MESSAGE       *message;
    OMX_STATETYPE          destState = messageParam;
    OMX_STATETYPE          currentState = pSECComponent->currentState;
    SEC_OMX_BASEPORT      *pSECPort = NULL;
    OMX_S32                countValue = 0;
    unsigned int           i = 0, j = 0;
    int                    k = 0;

    FunctionIn();

    /* check parameters */
    if (currentState == destState) {
         ret = OMX_ErrorSameState;
            goto EXIT;
    }
    if (currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if ((currentState == OMX_StateLoaded) && (destState == OMX_StateIdle)) {
        ret = SEC_OMX_Get_Resource(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
    }
    if (((currentState == OMX_StateIdle) && (destState == OMX_StateLoaded))       ||
        ((currentState == OMX_StateIdle) && (destState == OMX_StateInvalid))      ||
        ((currentState == OMX_StateExecuting) && (destState == OMX_StateInvalid)) ||
        ((currentState == OMX_StatePause) && (destState == OMX_StateInvalid))) {
        SEC_OMX_Release_Resource(pOMXComponent);
    }

    SEC_OSAL_Log(SEC_LOG_TRACE, "destState: %d", destState);

    switch (destState) {
    case OMX_StateInvalid:
        switch (currentState) {
        case OMX_StateWaitForResources:
            SEC_OMX_Out_WaitForResource(pOMXComponent);
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
        case OMX_StateLoaded:
            pSECComponent->currentState = OMX_StateInvalid;
            if (pSECComponent->hBufferProcess) {
                pSECComponent->bExitBufferProcessThread = OMX_TRUE;

                for (i = 0; i < ALL_PORT_NUM; i++) {
                    SEC_OSAL_Get_SemaphoreCount(pSECComponent->pSECPort[i].bufferSemID, &countValue);
                    if (countValue == 0)
                        SEC_OSAL_SemaphorePost(pSECComponent->pSECPort[i].bufferSemID);
                }

                SEC_OSAL_SignalSet(pSECComponent->pauseEvent);
                SEC_OSAL_ThreadTerminate(pSECComponent->hBufferProcess);
                pSECComponent->hBufferProcess = NULL;

                for (i = 0; i < ALL_PORT_NUM; i++) {
                    SEC_OSAL_MutexTerminate(pSECComponent->secDataBuffer[i].bufferMutex);
                    pSECComponent->secDataBuffer[i].bufferMutex = NULL;
                }

                SEC_OSAL_SignalTerminate(pSECComponent->pauseEvent);
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    SEC_OSAL_SemaphoreTerminate(pSECComponent->pSECPort[i].bufferSemID);
                    pSECComponent->pSECPort[i].bufferSemID = NULL;
                }
            }
            if (pSECComponent->sec_mfc_componentTerminate != NULL)
                pSECComponent->sec_mfc_componentTerminate(pOMXComponent);

            ret = OMX_ErrorInvalidState;
            break;
        default:
            ret = OMX_ErrorInvalidState;
            break;
        }
        break;
    case OMX_StateLoaded:
        switch (currentState) {
        case OMX_StateIdle:
            pSECComponent->bExitBufferProcessThread = OMX_TRUE;

            for (i = 0; i < ALL_PORT_NUM; i++) {
                SEC_OSAL_Get_SemaphoreCount(pSECComponent->pSECPort[i].bufferSemID, &countValue);
                if (countValue == 0)
                    SEC_OSAL_SemaphorePost(pSECComponent->pSECPort[i].bufferSemID);
            }

            SEC_OSAL_SignalSet(pSECComponent->pauseEvent);
            SEC_OSAL_ThreadTerminate(pSECComponent->hBufferProcess);
            pSECComponent->hBufferProcess = NULL;

            for (i = 0; i < ALL_PORT_NUM; i++) {
                SEC_OSAL_MutexTerminate(pSECComponent->secDataBuffer[i].bufferMutex);
                pSECComponent->secDataBuffer[i].bufferMutex = NULL;
            }

            SEC_OSAL_SignalTerminate(pSECComponent->pauseEvent);
            for (i = 0; i < ALL_PORT_NUM; i++) {
                SEC_OSAL_SemaphoreTerminate(pSECComponent->pSECPort[i].bufferSemID);
                pSECComponent->pSECPort[i].bufferSemID = NULL;
            }

            pSECComponent->sec_mfc_componentTerminate(pOMXComponent);

            for (i = 0; i < (pSECComponent->portParam.nPorts); i++) {
                pSECPort = (pSECComponent->pSECPort + i);
                if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    while (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) > 0) {
                        message = (SEC_OMX_MESSAGE*)SEC_OSAL_Dequeue(&pSECPort->bufferQ);
                        if (message != NULL)
                            SEC_OSAL_Free(message);
                    }
                    ret = pSECComponent->sec_FreeTunnelBuffer(pSECPort, i);
                    if (OMX_ErrorNone != ret) {
                        goto EXIT;
                    }
                } else {
                    if (CHECK_PORT_ENABLED(pSECPort)) {
                        SEC_OSAL_SemaphoreWait(pSECPort->unloadedResource);
                        pSECPort->portDefinition.bPopulated = OMX_FALSE;
                    }
                }
            }
            pSECComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateWaitForResources:
            ret = SEC_OMX_Out_WaitForResource(pOMXComponent);
            pSECComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateIdle:
        switch (currentState) {
        case OMX_StateLoaded:
            for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
                pSECPort = (pSECComponent->pSECPort + i);
                if (pSECPort == NULL) {
                    ret = OMX_ErrorBadParameter;
                    goto EXIT;
                }
                if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    if (CHECK_PORT_ENABLED(pSECPort)) {
                        ret = pSECComponent->sec_AllocateTunnelBuffer(pSECPort, i);
                        if (ret!=OMX_ErrorNone)
                            goto EXIT;
                    }
                } else {
                    if (CHECK_PORT_ENABLED(pSECPort)) {
                        SEC_OSAL_SemaphoreWait(pSECComponent->pSECPort[i].loadedResource);
                        pSECPort->portDefinition.bPopulated = OMX_TRUE;
                    }
                }
            }
            ret = pSECComponent->sec_mfc_componentInit(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */
                goto EXIT;
            }
            pSECComponent->bExitBufferProcessThread = OMX_FALSE;
            SEC_OSAL_SignalCreate(&pSECComponent->pauseEvent);
            for (i = 0; i < ALL_PORT_NUM; i++) {
                ret = SEC_OSAL_SemaphoreCreate(&pSECComponent->pSECPort[i].bufferSemID);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
                    goto EXIT;
                }
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                ret = SEC_OSAL_MutexCreate(&pSECComponent->secDataBuffer[i].bufferMutex);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
                    goto EXIT;
                }
            }
            ret = SEC_OSAL_ThreadCreate(&pSECComponent->hBufferProcess,
                             SEC_OMX_BufferProcessThread,
                             pOMXComponent);
            if (ret != OMX_ErrorNone) {
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */

                SEC_OSAL_SignalTerminate(pSECComponent->pauseEvent);
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    SEC_OSAL_MutexTerminate(pSECComponent->secDataBuffer[i].bufferMutex);
                    pSECComponent->secDataBuffer[i].bufferMutex = NULL;
                }
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    SEC_OSAL_SemaphoreTerminate(pSECComponent->pSECPort[i].bufferSemID);
                    pSECComponent->pSECPort[i].bufferSemID = NULL;
                }

                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pSECComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
            SEC_OMX_BufferFlushProcessNoEvent(pOMXComponent, ALL_PORT_INDEX);
            pSECComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateWaitForResources:
            pSECComponent->currentState = OMX_StateIdle;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateExecuting:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
                pSECPort = &pSECComponent->pSECPort[i];
                if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort) && CHECK_PORT_ENABLED(pSECPort)) {
                    for (j = 0; j < pSECPort->tunnelBufferNum; j++) {
                        SEC_OSAL_SemaphorePost(pSECComponent->pSECPort[i].bufferSemID);
                    }
                }
            }

            pSECComponent->transientState = SEC_OMX_TransStateMax;
            pSECComponent->currentState = OMX_StateExecuting;
            SEC_OSAL_SignalSet(pSECComponent->pauseEvent);
            break;
        case OMX_StatePause:
            for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
                pSECPort = &pSECComponent->pSECPort[i];
                if (CHECK_PORT_TUNNELED(pSECPort) && CHECK_PORT_BUFFER_SUPPLIER(pSECPort) && CHECK_PORT_ENABLED(pSECPort)) {
                    OMX_S32 semaValue = 0, cnt = 0;
                    SEC_OSAL_Get_SemaphoreCount(pSECComponent->pSECPort[i].bufferSemID, &semaValue);
                    if (SEC_OSAL_GetElemNum(&pSECPort->bufferQ) > semaValue) {
                        cnt = SEC_OSAL_GetElemNum(&pSECPort->bufferQ) - semaValue;
                        for (k = 0; k < cnt; k++) {
                            SEC_OSAL_SemaphorePost(pSECComponent->pSECPort[i].bufferSemID);
                        }
                    }
                }
            }

            pSECComponent->currentState = OMX_StateExecuting;
            SEC_OSAL_SignalSet(pSECComponent->pauseEvent);
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StatePause:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            pSECComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateExecuting:
            pSECComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateWaitForResources:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = SEC_OMX_In_WaitForResource(pOMXComponent);
            pSECComponent->currentState = OMX_StateWaitForResources;
            break;
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    default:
        ret = OMX_ErrorIncorrectStateTransition;
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pSECComponent->pCallbacks != NULL) {
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pSECComponent->callbackData,
            OMX_EventCmdComplete, OMX_CommandStateSet,
            destState, NULL);
        }
    } else {
        if (pSECComponent->pCallbacks != NULL) {
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pSECComponent->callbackData,
            OMX_EventError, ret, 0, NULL);
        }
    }
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SEC_OMX_MessageHandlerThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_MESSAGE       *message = NULL;
    OMX_U32                messageType = 0, portIndex = 0;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    while (pSECComponent->bExitMessageHandlerThread == OMX_FALSE) {
        SEC_OSAL_SemaphoreWait(pSECComponent->msgSemaphoreHandle);
        message = (SEC_OMX_MESSAGE *)SEC_OSAL_Dequeue(&pSECComponent->messageQ);
        if (message != NULL) {
            messageType = message->messageType;
            switch (messageType) {
            case OMX_CommandStateSet:
                ret = SEC_OMX_ComponentStateSet(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandFlush:
                ret = SEC_OMX_BufferFlushProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandPortDisable:
                ret = SEC_OMX_PortDisableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandPortEnable:
                ret = SEC_OMX_PortEnableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandMarkBuffer:
                portIndex = message->messageParam;
                pSECComponent->pSECPort[portIndex].markType.hMarkTargetComponent = ((OMX_MARKTYPE *)message->pCmdData)->hMarkTargetComponent;
                pSECComponent->pSECPort[portIndex].markType.pMarkData            = ((OMX_MARKTYPE *)message->pCmdData)->pMarkData;
                break;
            case (OMX_COMMANDTYPE)SEC_OMX_CommandComponentDeInit:
                pSECComponent->bExitMessageHandlerThread = OMX_TRUE;
                break;
            default:
                break;
            }
            SEC_OSAL_Free(message);
            message = NULL;
        }
    }

    SEC_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SEC_StateSet(SEC_OMX_BASECOMPONENT *pSECComponent, OMX_U32 nParam)
{
    OMX_U32 destState = nParam;
    OMX_U32 i = 0;

    if ((destState == OMX_StateIdle) && (pSECComponent->currentState == OMX_StateLoaded)) {
        pSECComponent->transientState = SEC_OMX_TransStateLoadedToIdle;
        for(i = 0; i < pSECComponent->portParam.nPorts; i++) {
            pSECComponent->pSECPort[i].portState = OMX_StateIdle;
        }
        SEC_OSAL_Log(SEC_LOG_TRACE, "to OMX_StateIdle");
    } else if ((destState == OMX_StateLoaded) && (pSECComponent->currentState == OMX_StateIdle)) {
        pSECComponent->transientState = SEC_OMX_TransStateIdleToLoaded;
        for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
            pSECComponent->pSECPort[i].portState = OMX_StateLoaded;
        }
        SEC_OSAL_Log(SEC_LOG_TRACE, "to OMX_StateLoaded");
    } else if ((destState == OMX_StateIdle) && (pSECComponent->currentState == OMX_StateExecuting)) {
        pSECComponent->transientState = SEC_OMX_TransStateExecutingToIdle;
        SEC_OSAL_Log(SEC_LOG_TRACE, "to OMX_StateIdle");
    } else if ((destState == OMX_StateExecuting) && (pSECComponent->currentState == OMX_StateIdle)) {
        pSECComponent->transientState = SEC_OMX_TransStateIdleToExecuting;
        SEC_OSAL_Log(SEC_LOG_TRACE, "to OMX_StateExecuting");
    } else if (destState == OMX_StateInvalid) {
        for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
            pSECComponent->pSECPort[i].portState = OMX_StateInvalid;
        }
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE SEC_SetPortFlush(SEC_OMX_BASECOMPONENT *pSECComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT *pSECPort = NULL;
    OMX_S32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0, index = 0;


    if ((pSECComponent->currentState == OMX_StateExecuting) ||
        (pSECComponent->currentState == OMX_StatePause)) {
        if ((portIndex != ALL_PORT_INDEX) &&
           ((OMX_S32)portIndex >= (OMX_S32)pSECComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /*********************
        *    need flush event set ?????
        **********************/
        cnt = (portIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;
        for (i = 0; i < cnt; i++) {
            if (portIndex == ALL_PORT_INDEX)
                index = i;
            else
                index = portIndex;
            pSECComponent->pSECPort[index].bIsPortFlushed = OMX_TRUE;
        }
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    ret = OMX_ErrorNone;

EXIT:
    return ret;
}

static OMX_ERRORTYPE SEC_SetPortEnable(SEC_OMX_BASECOMPONENT *pSECComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT *pSECPort = NULL;
    OMX_S32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0;

    FunctionIn();

    if ((portIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)portIndex >= (OMX_S32)pSECComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (portIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
            pSECPort = &pSECComponent->pSECPort[i];
            if (CHECK_PORT_ENABLED(pSECPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            } else {
                pSECPort->portState = OMX_StateIdle;
            }
        }
    } else {
        pSECPort = &pSECComponent->pSECPort[portIndex];
        if (CHECK_PORT_ENABLED(pSECPort)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        } else {
            pSECPort->portState = OMX_StateIdle;
        }
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;

}

static OMX_ERRORTYPE SEC_SetPortDisable(SEC_OMX_BASECOMPONENT *pSECComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT *pSECPort = NULL;
    OMX_S32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0;

    FunctionIn();

    if ((portIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)portIndex >= (OMX_S32)pSECComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (portIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pSECComponent->portParam.nPorts; i++) {
            pSECPort = &pSECComponent->pSECPort[i];
            if (!CHECK_PORT_ENABLED(pSECPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
            pSECPort->portState = OMX_StateLoaded;
            pSECPort->bIsPortDisabled = OMX_TRUE;
        }
    } else {
        pSECPort = &pSECComponent->pSECPort[portIndex];
        pSECPort->portState = OMX_StateLoaded;
        pSECPort->bIsPortDisabled = OMX_TRUE;
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SEC_SetMarkBuffer(SEC_OMX_BASECOMPONENT *pSECComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    SEC_OMX_BASEPORT *pSECPort = NULL;
    OMX_U32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0;


    if (nParam >= pSECComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pSECComponent->currentState == OMX_StateExecuting) ||
        (pSECComponent->currentState == OMX_StatePause)) {
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
    }

EXIT:
    return ret;
}

static OMX_ERRORTYPE SEC_OMX_CommandQueue(
    SEC_OMX_BASECOMPONENT *pSECComponent,
    OMX_COMMANDTYPE        Cmd,
    OMX_U32                nParam,
    OMX_PTR                pCmdData)
{
    OMX_ERRORTYPE    ret = OMX_ErrorNone;
    SEC_OMX_MESSAGE *command = (SEC_OMX_MESSAGE *)SEC_OSAL_Malloc(sizeof(SEC_OMX_MESSAGE));

    if (command == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    command->messageType  = (OMX_U32)Cmd;
    command->messageParam = nParam;
    command->pCmdData     = pCmdData;

    ret = SEC_OSAL_Queue(&pSECComponent->messageQ, (void *)command);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    ret = SEC_OSAL_SemaphorePost(pSECComponent->msgSemaphoreHandle);

EXIT:
    return ret;
}

OMX_ERRORTYPE SEC_OMX_SendCommand(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32         nParam,
    OMX_IN OMX_PTR         pCmdData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_OMX_MESSAGE       *message = NULL;

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

    switch (Cmd) {
    case OMX_CommandStateSet :
        SEC_OSAL_Log(SEC_LOG_TRACE, "Command: OMX_CommandStateSet");
        SEC_StateSet(pSECComponent, nParam);
        break;
    case OMX_CommandFlush :
        SEC_OSAL_Log(SEC_LOG_TRACE, "Command: OMX_CommandFlush");
        ret = SEC_SetPortFlush(pSECComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortDisable :
        SEC_OSAL_Log(SEC_LOG_TRACE, "Command: OMX_CommandPortDisable");
        ret = SEC_SetPortDisable(pSECComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortEnable :
        SEC_OSAL_Log(SEC_LOG_TRACE, "Command: OMX_CommandPortEnable");
        ret = SEC_SetPortEnable(pSECComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandMarkBuffer :
        SEC_OSAL_Log(SEC_LOG_TRACE, "Command: OMX_CommandMarkBuffer");
        ret = SEC_SetMarkBuffer(pSECComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
/*
    case SEC_CommandFillBuffer :
    case SEC_CommandEmptyBuffer :
    case SEC_CommandDeInit :
*/
    default:
        break;
    }

    ret = SEC_OMX_CommandQueue(pSECComponent, Cmd, nParam, pCmdData);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
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

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case (OMX_INDEXTYPE)OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
    {
        /* For Android PV OpenCORE */
        OMXComponentCapabilityFlagsType *capabilityFlags = (OMXComponentCapabilityFlagsType *)ComponentParameterStructure;
        SEC_OSAL_Memcpy(capabilityFlags, &pSECComponent->capabilityFlags, sizeof(OMXComponentCapabilityFlagsType));
    }
        break;
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = SEC_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        portParam->nPorts         = 0;
        portParam->nStartPortNumber     = 0;
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
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        ret = SEC_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        compPriority->nGroupID       = pSECComponent->compPriority.nGroupID;
        compPriority->nGroupPriority = pSECComponent->compPriority.nGroupPriority;
    }
        break;

    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = bufferSupplier->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort;

        if ((pSECComponent->currentState == OMX_StateLoaded) ||
            (pSECComponent->currentState == OMX_StateWaitForResources)) {
            if (portIndex >= pSECComponent->portParam.nPorts) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            ret = SEC_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            pSECPort = &pSECComponent->pSECPort[portIndex];


            if (pSECPort->portDefinition.eDir == OMX_DirInput) {
                if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else if (CHECK_PORT_TUNNELED(pSECPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            } else {
                if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else if (CHECK_PORT_TUNNELED(pSECPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            }
        }
        else
        {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
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

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = SEC_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pSECComponent->currentState != OMX_StateLoaded) &&
            (pSECComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
        ret = OMX_ErrorUndefined;
        /* SEC_OSAL_Memcpy(&pSECComponent->portParam, portParam, sizeof(OMX_PORT_PARAM_TYPE)); */
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32               portIndex = portDefinition->nPortIndex;
        SEC_OMX_BASEPORT         *pSECPort;

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = SEC_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
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
        if (portDefinition->nBufferCountActual < pSECPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        SEC_OSAL_Memcpy(&pSECPort->portDefinition, portDefinition, portDefinition->nSize);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        if ((pSECComponent->currentState != OMX_StateLoaded) &&
            (pSECComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        ret = SEC_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pSECComponent->compPriority.nGroupID = compPriority->nGroupID;
        pSECComponent->compPriority.nGroupPriority = compPriority->nGroupPriority;
    }
        break;
    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32           portIndex = bufferSupplier->nPortIndex;
        SEC_OMX_BASEPORT *pSECPort = NULL;


        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = SEC_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
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

        if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyUnspecified) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
        if (CHECK_PORT_TUNNELED(pSECPort) == 0) {
            ret = OMX_ErrorNone; /*OMX_ErrorNone ?????*/
            goto EXIT;
        }

        if (pSECPort->portDefinition.eDir == OMX_DirInput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pSECPort->tunnelFlags |= SEC_TUNNEL_IS_SUPPLIER;
                bufferSupplier->nPortIndex = pSECPort->tunneledPort;
                ret = OMX_SetParameter(pSECPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    pSECPort->tunnelFlags &= ~SEC_TUNNEL_IS_SUPPLIER;
                    bufferSupplier->nPortIndex = pSECPort->tunneledPort;
                    ret = OMX_SetParameter(pSECPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                }
                goto EXIT;
            }
        } else if (pSECPort->portDefinition.eDir == OMX_DirOutput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    pSECPort->tunnelFlags &= ~SEC_TUNNEL_IS_SUPPLIER;
                    ret = OMX_ErrorNone;
                }
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pSECPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pSECPort->tunnelFlags |= SEC_TUNNEL_IS_SUPPLIER;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     pComponentConfigStructure)
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
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
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
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_GetExtensionIndex(
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

    ret = OMX_ErrorBadParameter;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_SetCallbacks (
    OMX_IN OMX_HANDLETYPE    hComponent,
    OMX_IN OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN OMX_PTR           pAppData)
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

    if (pCallbacks == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if (pSECComponent->currentState != OMX_StateLoaded) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pSECComponent->pCallbacks = pCallbacks;
    pSECComponent->callbackData = pAppData;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN void                     *eglImage)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE SEC_OMX_BaseComponent_Constructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = SEC_OSAL_Malloc(sizeof(SEC_OMX_BASECOMPONENT));
    if (pSECComponent == NULL) {
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECComponent, 0, sizeof(SEC_OMX_BASECOMPONENT));
    pOMXComponent->pComponentPrivate = (OMX_PTR)pSECComponent;

    ret = SEC_OSAL_SemaphoreCreate(&pSECComponent->msgSemaphoreHandle);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    ret = SEC_OSAL_MutexCreate(&pSECComponent->compMutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    pSECComponent->bExitMessageHandlerThread = OMX_FALSE;
    SEC_OSAL_QueueCreate(&pSECComponent->messageQ);
    ret = SEC_OSAL_ThreadCreate(&pSECComponent->hMessageHandler, SEC_OMX_MessageHandlerThread, pOMXComponent);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    pOMXComponent->GetComponentVersion = &SEC_OMX_GetComponentVersion;
    pOMXComponent->SendCommand         = &SEC_OMX_SendCommand;
    pOMXComponent->GetState            = &SEC_OMX_GetState;
    pOMXComponent->SetCallbacks        = &SEC_OMX_SetCallbacks;
    pOMXComponent->UseEGLImage         = &SEC_OMX_UseEGLImage;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_BaseComponent_Destructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    OMX_S32                semaValue = 0;

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

    SEC_OMX_CommandQueue(pSECComponent, SEC_OMX_CommandComponentDeInit, 0, NULL);
    SEC_OSAL_SleepMillisec(0);
    SEC_OSAL_Get_SemaphoreCount(pSECComponent->msgSemaphoreHandle, &semaValue);
    if (semaValue == 0)
        SEC_OSAL_SemaphorePost(pSECComponent->msgSemaphoreHandle);
    SEC_OSAL_SemaphorePost(pSECComponent->msgSemaphoreHandle);

    SEC_OSAL_ThreadTerminate(pSECComponent->hMessageHandler);
    pSECComponent->hMessageHandler = NULL;

    SEC_OSAL_MutexTerminate(pSECComponent->compMutex);
    pSECComponent->compMutex = NULL;
    SEC_OSAL_SemaphoreTerminate(pSECComponent->msgSemaphoreHandle);
    pSECComponent->msgSemaphoreHandle = NULL;
    SEC_OSAL_QueueTerminate(&pSECComponent->messageQ);

    SEC_OSAL_Free(pSECComponent);
    pSECComponent = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}


