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
 * @file       SEC_OMX_Component_Register.h
 * @brief      SEC OpenMAX IL Component Register
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef SEC_OMX_COMPONENT_REG
#define SEC_OMX_COMPONENT_REG

#include "SEC_OMX_Def.h"
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Component.h"


typedef struct _SECRegisterComponentType
{
    OMX_U8  componentName[MAX_OMX_COMPONENT_NAME_SIZE];
    OMX_U8  roles[MAX_OMX_COMPONENT_ROLE_NUM][MAX_OMX_COMPONENT_ROLE_SIZE];
    OMX_U32 totalRoleNum;
} SECRegisterComponentType;

typedef struct _SEC_OMX_COMPONENT_REGLIST
{
    SECRegisterComponentType component;
    OMX_U8  libName[MAX_OMX_COMPONENT_LIBNAME_SIZE];
} SEC_OMX_COMPONENT_REGLIST;

struct SEC_OMX_COMPONENT;
typedef struct _SEC_OMX_COMPONENT
{
    OMX_U8                    componentName[MAX_OMX_COMPONENT_NAME_SIZE];
    OMX_U8                    libName[MAX_OMX_COMPONENT_LIBNAME_SIZE];
    OMX_HANDLETYPE            libHandle;
    OMX_COMPONENTTYPE        *pOMXComponent;
    struct _SEC_OMX_COMPONENT *nextOMXComp;
} SEC_OMX_COMPONENT;


#ifdef __cplusplus
extern "C" {
#endif


OMX_ERRORTYPE SEC_OMX_Component_Register(SEC_OMX_COMPONENT_REGLIST **compList, OMX_U32 *compNum);
OMX_ERRORTYPE SEC_OMX_Component_Unregister(SEC_OMX_COMPONENT_REGLIST *componentList);
OMX_ERRORTYPE SEC_OMX_ComponentLoad(SEC_OMX_COMPONENT *sec_component);
OMX_ERRORTYPE SEC_OMX_ComponentUnload(SEC_OMX_COMPONENT *sec_component);


#ifdef __cplusplus
}
#endif

#endif
