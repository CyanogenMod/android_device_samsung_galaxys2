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
 * @file        SEC_OSAL_Semaphore.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OSAL_SEMAPHORE
#define SEC_OSAL_SEMAPHORE

#include "OMX_Types.h"
#include "OMX_Core.h"


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OSAL_SemaphoreCreate(OMX_HANDLETYPE *semaphoreHandle);
OMX_ERRORTYPE SEC_OSAL_SemaphoreTerminate(OMX_HANDLETYPE semaphoreHandle);
OMX_ERRORTYPE SEC_OSAL_SemaphoreWait(OMX_HANDLETYPE semaphoreHandle);
OMX_ERRORTYPE SEC_OSAL_SemaphorePost(OMX_HANDLETYPE semaphoreHandle);
OMX_ERRORTYPE SEC_OSAL_Set_SemaphoreCount(OMX_HANDLETYPE semaphoreHandle, OMX_S32 val);
OMX_ERRORTYPE SEC_OSAL_Get_SemaphoreCount(OMX_HANDLETYPE semaphoreHandle, OMX_S32 *val);

#ifdef __cplusplus
}
#endif

#endif
