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
 * @file       SEC_OMX_Baseport.h
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef SEC_OMX_BASE_PORT
#define SEC_OMX_BASE_PORT


#include "OMX_Component.h"
#include "SEC_OMX_Def.h"
#include "SEC_OSAL_Queue.h"


#define BUFFER_STATE_ALLOCATED  (1 << 0)
#define BUFFER_STATE_ASSIGNED   (1 << 1)
#define HEADER_STATE_ALLOCATED  (1 << 2)
#define BUFFER_STATE_FREE        0

#define MAX_BUFFER_NUM          20

#define INPUT_PORT_INDEX    0
#define OUTPUT_PORT_INDEX   1
#define ALL_PORT_INDEX     -1
#define ALL_PORT_NUM        2

typedef struct _SEC_OMX_BASEPORT
{
    OMX_BUFFERHEADERTYPE         **bufferHeader;
    OMX_U32                       *bufferStateAllocate;
    OMX_PARAM_PORTDEFINITIONTYPE   portDefinition;
    OMX_HANDLETYPE                 bufferSemID;
    SEC_QUEUE                      bufferQ;
    OMX_U32                        assignedBufferNum;
    OMX_STATETYPE                  portState;
    OMX_HANDLETYPE                 loadedResource;
    OMX_HANDLETYPE                 unloadedResource;

    OMX_BOOL                       bIsPortFlushed;
    OMX_BOOL                       bIsPortDisabled;
    OMX_MARKTYPE                   markType;

    OMX_CONFIG_RECTTYPE            cropRectangle;

    /* Tunnel Info */
    OMX_HANDLETYPE                 tunneledComponent;
    OMX_U32                        tunneledPort;
    OMX_U32                        tunnelBufferNum;
    OMX_BUFFERSUPPLIERTYPE         bufferSupplier;
    OMX_U32                        tunnelFlags;

    OMX_BOOL                       bIsANBEnabled;
    OMX_BOOL                       bStoreMetaData;
} SEC_OMX_BASEPORT;


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OMX_PortEnableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex);
OMX_ERRORTYPE SEC_OMX_PortDisableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex);
OMX_ERRORTYPE SEC_OMX_BufferFlushProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex);
OMX_ERRORTYPE SEC_OMX_BufferFlushProcessNoEvent(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex);
OMX_ERRORTYPE SEC_OMX_Port_Constructor(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE SEC_OMX_Port_Destructor(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif


#endif
