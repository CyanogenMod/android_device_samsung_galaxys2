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
 * @file    Exynos_OSAL_Queue.h
 * @brief
 * @author    SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef Exynos_OSAL_QUEUE
#define Exynos_OSAL_QUEUE

#include "OMX_Types.h"
#include "OMX_Core.h"


#define MAX_QUEUE_ELEMENTS    10

typedef struct _EXYNOS_QElem
{
    void             *data;
    struct _EXYNOS_QElem *qNext;
} EXYNOS_QElem;

typedef struct _EXYNOS_QUEUE
{
    EXYNOS_QElem     *first;
    EXYNOS_QElem     *last;
    int            numElem;
    OMX_HANDLETYPE qMutex;
} EXYNOS_QUEUE;


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE Exynos_OSAL_QueueCreate(EXYNOS_QUEUE *queueHandle);
OMX_ERRORTYPE Exynos_OSAL_QueueTerminate(EXYNOS_QUEUE *queueHandle);
int           Exynos_OSAL_Queue(EXYNOS_QUEUE *queueHandle, void *data);
void         *Exynos_OSAL_Dequeue(EXYNOS_QUEUE *queueHandle);
int           Exynos_OSAL_GetElemNum(EXYNOS_QUEUE *queueHandle);
int           Exynos_OSAL_SetElemNum(EXYNOS_QUEUE *queueHandle, int ElemNum);

#ifdef __cplusplus
}
#endif

#endif
