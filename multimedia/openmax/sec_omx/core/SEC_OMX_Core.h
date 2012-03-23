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
 * @file       SEC_OMX_Core.h
 * @brief      SEC OpenMAX IL Core
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef SEC_OMX_CORE
#define SEC_OMX_CORE

#include "SEC_OMX_Def.h"
#include "OMX_Types.h"
#include "OMX_Core.h"


#ifdef __cplusplus
extern "C" {
#endif


SEC_EXPORT_REF OMX_API OMX_ERRORTYPE OMX_APIENTRY SEC_OMX_Init(void);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE OMX_APIENTRY SEC_OMX_Deinit(void);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE OMX_APIENTRY SEC_OMX_ComponentNameEnum(
    OMX_OUT   OMX_STRING        cComponentName,
    OMX_IN    OMX_U32           nNameLength,
    OMX_IN    OMX_U32           nIndex);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE OMX_APIENTRY SEC_OMX_GetHandle(
    OMX_OUT   OMX_HANDLETYPE   *pHandle,
    OMX_IN    OMX_STRING        cComponentName,
    OMX_IN    OMX_PTR           pAppData,
    OMX_IN    OMX_CALLBACKTYPE *pCallBacks);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE OMX_APIENTRY SEC_OMX_FreeHandle(
    OMX_IN    OMX_HANDLETYPE    hComponent);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE OMX_APIENTRY SEC_OMX_SetupTunnel(
    OMX_IN    OMX_HANDLETYPE    hOutput,
    OMX_IN    OMX_U32           nPortOutput,
    OMX_IN    OMX_HANDLETYPE    hInput,
    OMX_IN    OMX_U32           nPortInput);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE              SEC_OMX_GetContentPipe(
    OMX_OUT   OMX_HANDLETYPE   *hPipe,
    OMX_IN    OMX_STRING        szURI);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE              SEC_OMX_GetComponentsOfRole(
    OMX_IN    OMX_STRING        role,
    OMX_INOUT OMX_U32          *pNumComps,
    OMX_INOUT OMX_U8          **compNames);
SEC_EXPORT_REF OMX_API OMX_ERRORTYPE              SEC_OMX_GetRolesOfComponent(
    OMX_IN    OMX_STRING        compName,
    OMX_INOUT OMX_U32          *pNumRoles,
    OMX_OUT   OMX_U8          **roles);


#ifdef __cplusplus
}
#endif

#endif

