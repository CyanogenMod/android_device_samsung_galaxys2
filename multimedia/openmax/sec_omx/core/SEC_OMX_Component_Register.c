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
 * @file       SEC_OMX_Component_Register.c
 * @brief      SEC OpenMAX IL Component Register
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>

#include "OMX_Component.h"
#include "SEC_OSAL_Memory.h"
#include "SEC_OSAL_ETC.h"
#include "SEC_OSAL_Library.h"
#include "SEC_OMX_Component_Register.h"
#include "SEC_OMX_Macros.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_COMP_REGS"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

OMX_ERRORTYPE SEC_OMX_Component_Register(SEC_OMX_COMPONENT_REGLIST **compList, OMX_U32 *compNum)
{
    OMX_ERRORTYPE  ret = OMX_ErrorNone;
    int            componentNum = 0, roleNum = 0, totalCompNum = 0;
    int            read;
    char          *libName;
    size_t         len;
    const char    *errorMsg;
    DIR           *dir;
    struct dirent *d;

    int (*SEC_OMX_COMPONENT_Library_Register)(SECRegisterComponentType **secComponents);
    SECRegisterComponentType **secComponentsTemp;
    SEC_OMX_COMPONENT_REGLIST *componentList;

    FunctionIn();

    dir = opendir(SEC_OMX_INSTALL_PATH);
    if (dir == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    componentList = (SEC_OMX_COMPONENT_REGLIST *)SEC_OSAL_Malloc(sizeof(SEC_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    SEC_OSAL_Memset(componentList, 0, sizeof(SEC_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    libName = SEC_OSAL_Malloc(MAX_OMX_COMPONENT_LIBNAME_SIZE);

    while ((d = readdir(dir)) != NULL) {
        OMX_HANDLETYPE soHandle;
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s", d->d_name);

        if (SEC_OSAL_Strncmp(d->d_name, "libOMX.SEC.", SEC_OSAL_Strlen("libOMX.SEC.")) == 0) {
            SEC_OSAL_Memset(libName, 0, MAX_OMX_COMPONENT_LIBNAME_SIZE);
            SEC_OSAL_Strcpy(libName, SEC_OMX_INSTALL_PATH);
            SEC_OSAL_Strcat(libName, d->d_name);
            SEC_OSAL_Log(SEC_LOG_ERROR, "Path & libName : %s", libName);
            if ((soHandle = SEC_OSAL_dlopen(libName, RTLD_NOW)) != NULL) {
                SEC_OSAL_dlerror();    /* clear error*/
                if ((SEC_OMX_COMPONENT_Library_Register = SEC_OSAL_dlsym(soHandle, "SEC_OMX_COMPONENT_Library_Register")) != NULL) {
                    int i = 0;
                    unsigned int j = 0;

                    componentNum = (*SEC_OMX_COMPONENT_Library_Register)(NULL);
                    secComponentsTemp = (SECRegisterComponentType **)SEC_OSAL_Malloc(sizeof(SECRegisterComponentType*) * componentNum);
                    for (i = 0; i < componentNum; i++) {
                        secComponentsTemp[i] = SEC_OSAL_Malloc(sizeof(SECRegisterComponentType));
                        SEC_OSAL_Memset(secComponentsTemp[i], 0, sizeof(SECRegisterComponentType));
                    }
                    (*SEC_OMX_COMPONENT_Library_Register)(secComponentsTemp);

                    for (i = 0; i < componentNum; i++) {
                        SEC_OSAL_Strcpy(componentList[totalCompNum].component.componentName, secComponentsTemp[i]->componentName);
                        for (j = 0; j < secComponentsTemp[i]->totalRoleNum; j++)
                            SEC_OSAL_Strcpy(componentList[totalCompNum].component.roles[j], secComponentsTemp[i]->roles[j]);
                        componentList[totalCompNum].component.totalRoleNum = secComponentsTemp[i]->totalRoleNum;

                        SEC_OSAL_Strcpy(componentList[totalCompNum].libName, libName);

                        totalCompNum++;
                    }
                    for (i = 0; i < componentNum; i++) {
                        SEC_OSAL_Free(secComponentsTemp[i]);
                    }

                    SEC_OSAL_Free(secComponentsTemp);
                } else {
                    if ((errorMsg = SEC_OSAL_dlerror()) != NULL)
                        SEC_OSAL_Log(SEC_LOG_WARNING, "dlsym failed: %s", errorMsg);
                }
                SEC_OSAL_dlclose(soHandle);
            } else {
                SEC_OSAL_Log(SEC_LOG_WARNING, "dlopen failed: %s", SEC_OSAL_dlerror());
            }
        } else {
            /* not a component name line. skip */
            continue;
        }
    }

    SEC_OSAL_Free(libName);

    closedir(dir);

    *compList = componentList;
    *compNum = totalCompNum;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_Component_Unregister(SEC_OMX_COMPONENT_REGLIST *componentList)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    SEC_OSAL_Memset(componentList, 0, sizeof(SEC_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    SEC_OSAL_Free(componentList);

EXIT:
    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentAPICheck(OMX_COMPONENTTYPE *component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if ((NULL == component->GetComponentVersion)    ||
        (NULL == component->SendCommand)            ||
        (NULL == component->GetParameter)           ||
        (NULL == component->SetParameter)           ||
        (NULL == component->GetConfig)              ||
        (NULL == component->SetConfig)              ||
        (NULL == component->GetExtensionIndex)      ||
        (NULL == component->GetState)               ||
        (NULL == component->ComponentTunnelRequest) ||
        (NULL == component->UseBuffer)              ||
        (NULL == component->AllocateBuffer)         ||
        (NULL == component->FreeBuffer)             ||
        (NULL == component->EmptyThisBuffer)        ||
        (NULL == component->FillThisBuffer)         ||
        (NULL == component->SetCallbacks)           ||
        (NULL == component->ComponentDeInit)        ||
        (NULL == component->UseEGLImage)            ||
        (NULL == component->ComponentRoleEnum))
        ret = OMX_ErrorInvalidComponent;
    else
        ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentLoad(SEC_OMX_COMPONENT *sec_component)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    OMX_HANDLETYPE     libHandle;
    OMX_COMPONENTTYPE *pOMXComponent;

    FunctionIn();

    OMX_ERRORTYPE (*SEC_OMX_ComponentInit)(OMX_HANDLETYPE hComponent, OMX_STRING componentName);

    libHandle = SEC_OSAL_dlopen((OMX_STRING)sec_component->libName, RTLD_NOW);
    if (!libHandle) {
        ret = OMX_ErrorInvalidComponentName;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInvalidComponentName, Line:%d", __LINE__);
        goto EXIT;
    }

    SEC_OMX_ComponentInit = SEC_OSAL_dlsym(libHandle, "SEC_OMX_ComponentInit");
    if (!SEC_OMX_ComponentInit) {
        SEC_OSAL_dlclose(libHandle);
        ret = OMX_ErrorInvalidComponent;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)SEC_OSAL_Malloc(sizeof(OMX_COMPONENTTYPE));
    INIT_SET_SIZE_VERSION(pOMXComponent, OMX_COMPONENTTYPE);
    ret = (*SEC_OMX_ComponentInit)((OMX_HANDLETYPE)pOMXComponent, (OMX_STRING)sec_component->componentName);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Free(pOMXComponent);
        SEC_OSAL_dlclose(libHandle);
        ret = OMX_ErrorInvalidComponent;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
        goto EXIT;
    } else {
        if (SEC_OMX_ComponentAPICheck(pOMXComponent) != OMX_ErrorNone) {
            if (NULL != pOMXComponent->ComponentDeInit)
                pOMXComponent->ComponentDeInit(pOMXComponent);
            SEC_OSAL_Free(pOMXComponent);
            SEC_OSAL_dlclose(libHandle);
            ret = OMX_ErrorInvalidComponent;
            SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
            goto EXIT;
        }
        sec_component->libHandle = libHandle;
        sec_component->pOMXComponent = pOMXComponent;
        ret = OMX_ErrorNone;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentUnload(SEC_OMX_COMPONENT *sec_component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = NULL;

    FunctionIn();

    if (!sec_component) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = sec_component->pOMXComponent;
    if (pOMXComponent != NULL) {
        pOMXComponent->ComponentDeInit(pOMXComponent);
        SEC_OSAL_Free(pOMXComponent);
        sec_component->pOMXComponent = NULL;
    }

    if (sec_component->libHandle != NULL) {
        SEC_OSAL_dlclose(sec_component->libHandle);
        sec_component->libHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

