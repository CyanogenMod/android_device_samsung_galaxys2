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
 * @file        SEC_OSAL_Mutex.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
*/

#ifndef SEC_OSAL_MUTEX
#define SEC_OSAL_MUTEX

#include "OMX_Types.h"
#include "OMX_Core.h"


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OSAL_MutexCreate(OMX_HANDLETYPE *mutexHandle);
OMX_ERRORTYPE SEC_OSAL_MutexTerminate(OMX_HANDLETYPE mutexHandle);
OMX_ERRORTYPE SEC_OSAL_MutexLock(OMX_HANDLETYPE mutexHandle);
OMX_ERRORTYPE SEC_OSAL_MutexUnlock(OMX_HANDLETYPE mutexHandle);

#ifdef __cplusplus
}
#endif

#endif
