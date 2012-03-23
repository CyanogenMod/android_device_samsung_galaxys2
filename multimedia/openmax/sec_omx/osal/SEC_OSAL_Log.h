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
 * @file        SEC_OSAL_Log.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 *   2010.8.27 : Add trace function
 */

#ifndef SEC_OSAL_LOG
#define SEC_OSAL_LOG

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SEC_LOG_OFF
#define SEC_LOG
#endif

#ifndef SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_LOG"
#endif

#ifdef SEC_TRACE_ON
#define SEC_TRACE
#endif

typedef enum _LOG_LEVEL
{
    SEC_LOG_TRACE,
    SEC_LOG_INFO,
    SEC_LOG_WARNING,
    SEC_LOG_ERROR
} SEC_LOG_LEVEL;

#ifdef SEC_LOG
#define SEC_OSAL_Log(a, ...)    ((void)_SEC_OSAL_Log(a, SEC_LOG_TAG, __VA_ARGS__))
#else
#define SEC_OSAL_Log(a, ...)                                                \
    do {                                                                \
        if (a == SEC_LOG_ERROR)                                     \
            ((void)_SEC_OSAL_Log(a, SEC_LOG_TAG, __VA_ARGS__)); \
    } while (0)
#endif

#ifdef SEC_TRACE
#define FunctionIn() _SEC_OSAL_Log(SEC_LOG_TRACE, SEC_LOG_TAG, "%s In , Line: %d", __FUNCTION__, __LINE__)
#define FunctionOut() _SEC_OSAL_Log(SEC_LOG_TRACE, SEC_LOG_TAG, "%s Out , Line: %d", __FUNCTION__, __LINE__)
#else
#define FunctionIn() ((void *)0)
#define FunctionOut() ((void *)0)
#endif

extern void _SEC_OSAL_Log(SEC_LOG_LEVEL logLevel, const char *tag, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif
