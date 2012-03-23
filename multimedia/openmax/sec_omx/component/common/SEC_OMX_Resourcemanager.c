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
 * @file       SEC_OMX_Resourcemanager.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEC_OMX_Resourcemanager.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OSAL_Memory.h"
#include "SEC_OSAL_Mutex.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_RM"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"


#define MAX_RESOURCE_VIDEO_DEC 3 /* for Android */
#define MAX_RESOURCE_VIDEO_ENC 1 /* for Android */

/* Max allowable video scheduler component instance */
static SEC_OMX_RM_COMPONENT_LIST *gpVideoDecRMComponentList = NULL;
static SEC_OMX_RM_COMPONENT_LIST *gpVideoDecRMWaitingList = NULL;
static SEC_OMX_RM_COMPONENT_LIST *gpVideoEncRMComponentList = NULL;
static SEC_OMX_RM_COMPONENT_LIST *gpVideoEncRMWaitingList = NULL;
static OMX_HANDLETYPE ghVideoRMComponentListMutex = NULL;


OMX_ERRORTYPE addElementList(SEC_OMX_RM_COMPONENT_LIST **ppList, OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_RM_COMPONENT_LIST *pTempComp = NULL;
    SEC_OMX_BASECOMPONENT     *pSECComponent = NULL;

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (*ppList != NULL) {
        pTempComp = *ppList;
        while (pTempComp->pNext != NULL) {
            pTempComp = pTempComp->pNext;
        }
        pTempComp->pNext = (SEC_OMX_RM_COMPONENT_LIST *)SEC_OSAL_Malloc(sizeof(SEC_OMX_RM_COMPONENT_LIST));
        if (pTempComp->pNext == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        ((SEC_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pNext = NULL;
        ((SEC_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pOMXStandComp = pOMXComponent;
        ((SEC_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->groupPriority = pSECComponent->compPriority.nGroupPriority;
        goto EXIT;
    } else {
        *ppList = (SEC_OMX_RM_COMPONENT_LIST *)SEC_OSAL_Malloc(sizeof(SEC_OMX_RM_COMPONENT_LIST));
        if (*ppList == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        pTempComp = *ppList;
        pTempComp->pNext = NULL;
        pTempComp->pOMXStandComp = pOMXComponent;
        pTempComp->groupPriority = pSECComponent->compPriority.nGroupPriority;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE removeElementList(SEC_OMX_RM_COMPONENT_LIST **ppList, OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_RM_COMPONENT_LIST *pCurrComp = NULL;
    SEC_OMX_RM_COMPONENT_LIST *pPrevComp = NULL;
    OMX_BOOL                   bDetectComp = OMX_FALSE;

    if (*ppList == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pCurrComp = *ppList;
    while (pCurrComp != NULL) {
        if (pCurrComp->pOMXStandComp == pOMXComponent) {
            if (*ppList == pCurrComp) {
                *ppList = pCurrComp->pNext;
                SEC_OSAL_Free(pCurrComp);
            } else {
                if (pPrevComp != NULL)
                    pPrevComp->pNext = pCurrComp->pNext;

                SEC_OSAL_Free(pCurrComp);
            }
            bDetectComp = OMX_TRUE;
            break;
        } else {
            pPrevComp = pCurrComp;
            pCurrComp = pCurrComp->pNext;
        }
    }

    if (bDetectComp == OMX_FALSE)
        ret = OMX_ErrorComponentNotFound;
    else
        ret = OMX_ErrorNone;

EXIT:
    return ret;
}

int searchLowPriority(SEC_OMX_RM_COMPONENT_LIST *RMComp_list, OMX_U32 inComp_priority, SEC_OMX_RM_COMPONENT_LIST **outLowComp)
{
    int ret = 0;
    SEC_OMX_RM_COMPONENT_LIST *pTempComp = NULL;
    SEC_OMX_RM_COMPONENT_LIST *pCandidateComp = NULL;

    if (RMComp_list == NULL)
        ret = -1;

    pTempComp = RMComp_list;
    *outLowComp = 0;

    while (pTempComp != NULL) {
        if (pTempComp->groupPriority > inComp_priority) {
            if (pCandidateComp != NULL) {
                if (pCandidateComp->groupPriority < pTempComp->groupPriority)
                    pCandidateComp = pTempComp;
            } else {
                pCandidateComp = pTempComp;
            }
        }

        pTempComp = pTempComp->pNext;
    }

    *outLowComp = pCandidateComp;
    if (pCandidateComp == NULL)
        ret = 0;
    else
        ret = 1;

EXIT:
    return ret;
}

OMX_ERRORTYPE removeComponent(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateIdle) {
        (*(pSECComponent->pCallbacks->EventHandler))
            (pOMXComponent, pSECComponent->callbackData,
            OMX_EventError, OMX_ErrorResourcesLost, 0, NULL);
        ret = OMX_SendCommand(pOMXComponent, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
    } else if ((pSECComponent->currentState == OMX_StateExecuting) || (pSECComponent->currentState == OMX_StatePause)) {
        /* Todo */
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}


OMX_ERRORTYPE SEC_OMX_ResourceManager_Init()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();
    ret = SEC_OSAL_MutexCreate(&ghVideoRMComponentListMutex);
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ResourceManager_Deinit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    SEC_OMX_RM_COMPONENT_LIST *pCurrComponent;
    SEC_OMX_RM_COMPONENT_LIST *pNextComponent;

    FunctionIn();

    SEC_OSAL_MutexLock(ghVideoRMComponentListMutex);

    if (gpVideoDecRMComponentList) {
        pCurrComponent = gpVideoDecRMComponentList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            SEC_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoDecRMComponentList = NULL;
    }
    if (gpVideoDecRMWaitingList) {
        pCurrComponent = gpVideoDecRMWaitingList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            SEC_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoDecRMWaitingList = NULL;
    }

    if (gpVideoEncRMComponentList) {
        pCurrComponent = gpVideoEncRMComponentList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            SEC_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoEncRMComponentList = NULL;
    }
    if (gpVideoEncRMWaitingList) {
        pCurrComponent = gpVideoEncRMWaitingList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            SEC_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoEncRMWaitingList = NULL;
    }

    SEC_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    SEC_OSAL_MutexTerminate(ghVideoRMComponentListMutex);
    ghVideoRMComponentListMutex = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_Get_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT     *pSECComponent = NULL;
    SEC_OMX_RM_COMPONENT_LIST *pComponentTemp = NULL;
    SEC_OMX_RM_COMPONENT_LIST *pComponentCandidate = NULL;
    int numElem = 0;
    int lowCompDetect = 0;

    FunctionIn();

    SEC_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pSECComponent->codecType == HW_VIDEO_DEC_CODEC) {
        pComponentTemp = gpVideoDecRMComponentList;
        if (pComponentTemp != NULL) {
            while (pComponentTemp) {
                numElem++;
                pComponentTemp = pComponentTemp->pNext;
            }
        } else {
            numElem = 0;
        }
        if (numElem >= MAX_RESOURCE_VIDEO_DEC) {
            lowCompDetect = searchLowPriority(gpVideoDecRMComponentList, pSECComponent->compPriority.nGroupPriority, &pComponentCandidate);
            if (lowCompDetect <= 0) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            } else {
                ret = removeComponent(pComponentCandidate->pOMXStandComp);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    goto EXIT;
                } else {
                    ret = removeElementList(&gpVideoDecRMComponentList, pComponentCandidate->pOMXStandComp);
                    ret = addElementList(&gpVideoDecRMComponentList, pOMXComponent);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                }
            }
        } else {
            ret = addElementList(&gpVideoDecRMComponentList, pOMXComponent);
            if (ret != OMX_ErrorNone) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    } else if (pSECComponent->codecType == HW_VIDEO_ENC_CODEC) {
        pComponentTemp = gpVideoEncRMComponentList;
        if (pComponentTemp != NULL) {
            while (pComponentTemp) {
                numElem++;
                pComponentTemp = pComponentTemp->pNext;
            }
        } else {
            numElem = 0;
        }
        if (numElem >= MAX_RESOURCE_VIDEO_ENC) {
            lowCompDetect = searchLowPriority(gpVideoEncRMComponentList, pSECComponent->compPriority.nGroupPriority, &pComponentCandidate);
            if (lowCompDetect <= 0) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            } else {
                ret = removeComponent(pComponentCandidate->pOMXStandComp);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    goto EXIT;
                } else {
                    ret = removeElementList(&gpVideoEncRMComponentList, pComponentCandidate->pOMXStandComp);
                    ret = addElementList(&gpVideoEncRMComponentList, pOMXComponent);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                }
            }
        } else {
            ret = addElementList(&gpVideoEncRMComponentList, pOMXComponent);
            if (ret != OMX_ErrorNone) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }
    ret = OMX_ErrorNone;

EXIT:

    SEC_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_Release_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT     *pSECComponent = NULL;
    SEC_OMX_RM_COMPONENT_LIST *pComponentTemp = NULL;
    OMX_COMPONENTTYPE         *pOMXWaitComponent = NULL;
    int numElem = 0;

    FunctionIn();

    SEC_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    
    if (pSECComponent->codecType == HW_VIDEO_DEC_CODEC) {
        pComponentTemp = gpVideoDecRMWaitingList;
        if (gpVideoDecRMComponentList == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        ret = removeElementList(&gpVideoDecRMComponentList, pOMXComponent);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
        while (pComponentTemp) {
            numElem++;
            pComponentTemp = pComponentTemp->pNext;
        }
        if (numElem > 0) {
            pOMXWaitComponent = gpVideoDecRMWaitingList->pOMXStandComp;
            removeElementList(&gpVideoDecRMWaitingList, pOMXWaitComponent);
            ret = OMX_SendCommand(pOMXWaitComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
        }
    } else if (pSECComponent->codecType == HW_VIDEO_ENC_CODEC) {
        pComponentTemp = gpVideoEncRMWaitingList;
        if (gpVideoEncRMComponentList == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        ret = removeElementList(&gpVideoEncRMComponentList, pOMXComponent);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
        while (pComponentTemp) {
            numElem++;
            pComponentTemp = pComponentTemp->pNext;
        }
        if (numElem > 0) {
            pOMXWaitComponent = gpVideoEncRMWaitingList->pOMXStandComp;
            removeElementList(&gpVideoEncRMWaitingList, pOMXWaitComponent);
            ret = OMX_SendCommand(pOMXWaitComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
        }
    }

EXIT:

    SEC_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_In_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    SEC_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->codecType == HW_VIDEO_DEC_CODEC)
        ret = addElementList(&gpVideoDecRMWaitingList, pOMXComponent);
    else if (pSECComponent->codecType == HW_VIDEO_ENC_CODEC)
        ret = addElementList(&gpVideoEncRMWaitingList, pOMXComponent);

    SEC_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_Out_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    SEC_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->codecType == HW_VIDEO_DEC_CODEC)
        ret = removeElementList(&gpVideoDecRMWaitingList, pOMXComponent);
    else if (pSECComponent->codecType == HW_VIDEO_ENC_CODEC)
        ret = removeElementList(&gpVideoEncRMWaitingList, pOMXComponent);

    SEC_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

