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
 * @file    SEC_OSAL_Queue.h
 * @brief
 * @author    SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OSAL_QUEUE
#define SEC_OSAL_QUEUE

#include "OMX_Types.h"
#include "OMX_Core.h"


#define MAX_QUEUE_ELEMENTS    10

typedef struct _SEC_QElem
{
    void             *data;
    struct _SEC_QElem *qNext;
} SEC_QElem;

typedef struct _SEC_QUEUE
{
    SEC_QElem     *first;
    SEC_QElem     *last;
    int            numElem;
    OMX_HANDLETYPE qMutex;
} SEC_QUEUE;


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OSAL_QueueCreate(SEC_QUEUE *queueHandle);
OMX_ERRORTYPE SEC_OSAL_QueueTerminate(SEC_QUEUE *queueHandle);
int           SEC_OSAL_Queue(SEC_QUEUE *queueHandle, void *data);
void         *SEC_OSAL_Dequeue(SEC_QUEUE *queueHandle);
int           SEC_OSAL_GetElemNum(SEC_QUEUE *queueHandle);
int           SEC_OSAL_SetElemNum(SEC_QUEUE *queueHandle, int ElemNum);

#ifdef __cplusplus
}
#endif

#endif
