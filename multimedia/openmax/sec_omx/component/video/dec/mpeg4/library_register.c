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
 * @file        library_register.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "SEC_OSAL_Memory.h"
#include "SEC_OSAL_ETC.h"
#include "library_register.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_MPEG4_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"


OSCL_EXPORT_REF int SEC_OMX_COMPONENT_Library_Register(SECRegisterComponentType **ppSECComponent)
{
    FunctionIn();

    if (ppSECComponent == NULL)
        goto EXIT;

    /* component 1 - video decoder MPEG4 */
    SEC_OSAL_Strcpy(ppSECComponent[0]->componentName, SEC_OMX_COMPONENT_MPEG4_DEC);
    SEC_OSAL_Strcpy(ppSECComponent[0]->roles[0], SEC_OMX_COMPONENT_MPEG4_DEC_ROLE);
    ppSECComponent[0]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    /* component 2 - video decoder H.263 */
    SEC_OSAL_Strcpy(ppSECComponent[1]->componentName, SEC_OMX_COMPONENT_H263_DEC);
    SEC_OSAL_Strcpy(ppSECComponent[1]->roles[0], SEC_OMX_COMPONENT_H263_DEC_ROLE);
    ppSECComponent[1]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

EXIT:
    FunctionOut();
    return MAX_COMPONENT_NUM;
}

