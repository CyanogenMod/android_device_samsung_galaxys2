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
 * @file       Exynos_OSAL_Library.h
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef Exynos_OSAL_LIBRARY
#define Exynos_OSAL_LIBRARY

#include "OMX_Types.h"


#ifdef __cplusplus
extern "C" {
#endif

void *Exynos_OSAL_dlopen(const char *filename, int flag);
void *Exynos_OSAL_dlsym(void *handle, const char *symbol);
int   Exynos_OSAL_dlclose(void *handle);
const char *Exynos_OSAL_dlerror(void);

#ifdef __cplusplus
}
#endif

#endif
