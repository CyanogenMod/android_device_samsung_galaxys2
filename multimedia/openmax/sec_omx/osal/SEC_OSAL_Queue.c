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
 * @file        SEC_OSAL_Queue.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEC_OSAL_Memory.h"
#include "SEC_OSAL_Mutex.h"
#include "SEC_OSAL_Queue.h"


OMX_ERRORTYPE SEC_OSAL_QueueCreate(SEC_QUEUE *queueHandle)
{
    int i = 0;
    SEC_QElem *newqelem = NULL;
    SEC_QElem *currentqelem = NULL;
    SEC_QUEUE *queue = (SEC_QUEUE *)queueHandle;

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    ret = SEC_OSAL_MutexCreate(&queue->qMutex);
    if (ret != OMX_ErrorNone)
        return ret;

    queue->first = (SEC_QElem *)SEC_OSAL_Malloc(sizeof(SEC_QElem));
    if (queue->first == NULL)
        return OMX_ErrorInsufficientResources;

    SEC_OSAL_Memset(queue->first, 0, sizeof(SEC_QElem));
    currentqelem = queue->last = queue->first;
    queue->numElem = 0;

    for (i = 0; i < (MAX_QUEUE_ELEMENTS - 2); i++) {
        newqelem = (SEC_QElem *)SEC_OSAL_Malloc(sizeof(SEC_QElem));
        if (newqelem == NULL) {
            while (queue->first != NULL) {
                currentqelem = queue->first->qNext;
                SEC_OSAL_Free((OMX_PTR)queue->first);
                queue->first = currentqelem;
            }
            return OMX_ErrorInsufficientResources;
        } else {
            SEC_OSAL_Memset(newqelem, 0, sizeof(SEC_QElem));
            currentqelem->qNext = newqelem;
            currentqelem = newqelem;
        }
    }

    currentqelem->qNext = queue->first;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SEC_OSAL_QueueTerminate(SEC_QUEUE *queueHandle)
{
    int i = 0;
    SEC_QElem *currentqelem = NULL;
    SEC_QUEUE *queue = (SEC_QUEUE *)queueHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    for ( i = 0; i < (MAX_QUEUE_ELEMENTS - 2); i++) {
        currentqelem = queue->first->qNext;
        SEC_OSAL_Free(queue->first);
        queue->first = currentqelem;
    }

    if(queue->first) {
        SEC_OSAL_Free(queue->first);
        queue->first = NULL;
    }

    ret = SEC_OSAL_MutexTerminate(queue->qMutex);

    return ret;
}

int SEC_OSAL_Queue(SEC_QUEUE *queueHandle, void *data)
{
    SEC_QUEUE *queue = (SEC_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    SEC_OSAL_MutexLock(queue->qMutex);

    if ((queue->last->data != NULL) || (queue->numElem >= MAX_QUEUE_ELEMENTS)) {
        SEC_OSAL_MutexUnlock(queue->qMutex);
        return -1;
    }
    queue->last->data = data;
    queue->last = queue->last->qNext;
    queue->numElem++;

    SEC_OSAL_MutexUnlock(queue->qMutex);
    return 0;
}

void *SEC_OSAL_Dequeue(SEC_QUEUE *queueHandle)
{
    void *data = NULL;
    SEC_QUEUE *queue = (SEC_QUEUE *)queueHandle;
    if (queue == NULL)
        return NULL;

    SEC_OSAL_MutexLock(queue->qMutex);

    if ((queue->first->data == NULL) || (queue->numElem <= 0)) {
        SEC_OSAL_MutexUnlock(queue->qMutex);
        return NULL;
    }
    data = queue->first->data;
    queue->first->data = NULL;
    queue->first = queue->first->qNext;
    queue->numElem--;

    SEC_OSAL_MutexUnlock(queue->qMutex);
    return data;
}

int SEC_OSAL_GetElemNum(SEC_QUEUE *queueHandle)
{
    int ElemNum = 0;
    SEC_QUEUE *queue = (SEC_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    SEC_OSAL_MutexLock(queue->qMutex);
    ElemNum = queue->numElem;
    SEC_OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

int SEC_OSAL_SetElemNum(SEC_QUEUE *queueHandle, int ElemNum)
{
    SEC_QUEUE *queue = (SEC_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    SEC_OSAL_MutexLock(queue->qMutex);
    queue->numElem = ElemNum; 
    SEC_OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

