/*
 * Copyright 2011 Samsung Electronics S.LSI Co. LTD
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
 * @file        SEC_OSAL_Android.h
 * @brief
 * @author      Seungbeom Kim (sbcrux.kim@samsung.com)
 * @author      Hyeyeon Chung (hyeon.chung@samsung.com)
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.1.0
 * @history
 *   2011.7.15 : Create
 */

#ifndef SEC_OSAL_ANDROID
#define SEC_OSAL_ANDROID

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Index.h"

#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OSAL_GetANBParameter(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_IN OMX_INDEXTYPE nIndex,
                                       OMX_INOUT OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE SEC_OSAL_SetANBParameter(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_IN OMX_INDEXTYPE nIndex,
                                       OMX_IN OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE SEC_OSAL_LockANB(OMX_IN OMX_PTR pBuffer,
                               OMX_IN OMX_U32 width,
                               OMX_IN OMX_U32 height,
                               OMX_IN OMX_COLOR_FORMATTYPE format,
                               OMX_OUT OMX_U32 *pStride,
                               OMX_OUT OMX_PTR *vaddr);

OMX_ERRORTYPE SEC_OSAL_GetPhysANB(OMX_IN OMX_PTR pBuffer,
                                  OMX_OUT OMX_PTR *paddr);

OMX_ERRORTYPE SEC_OSAL_UnlockANB(OMX_IN OMX_PTR pBuffer);

OMX_ERRORTYPE SEC_OSAL_LockANBHandle(OMX_IN OMX_U32 pBuffer,
                                     OMX_IN OMX_U32 width,
                                     OMX_IN OMX_U32 height,
                                     OMX_IN OMX_COLOR_FORMATTYPE format,
                                     OMX_OUT OMX_PTR *vaddr);

OMX_ERRORTYPE SEC_OSAL_UnlockANBHandle(OMX_IN OMX_U32 pBuffer);

OMX_ERRORTYPE SEC_OSAL_GetPhysANBHandle(OMX_IN OMX_U32 pBuffer,
                                        OMX_OUT OMX_PTR *paddr);

OMX_ERRORTYPE SEC_OSAL_GetInfoFromMetaData(OMX_IN SEC_OMX_DATA *pBuffer,
                                           OMX_OUT OMX_PTR *pOutBuffer);

OMX_ERRORTYPE SEC_OSAL_CheckANB(OMX_IN SEC_OMX_DATA *pBuffer,
                                OMX_OUT OMX_BOOL *bIsANBEnabled);

#ifdef __cplusplus
}
#endif

#endif
