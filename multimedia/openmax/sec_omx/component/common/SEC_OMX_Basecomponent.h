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
 * @file       SEC_OMX_Basecomponent.h
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    1.1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef SEC_OMX_BASECOMP
#define SEC_OMX_BASECOMP

#include "SEC_OMX_Def.h"
#include "OMX_Component.h"
#include "SEC_OSAL_Queue.h"
#include "SEC_OMX_Baseport.h"


typedef struct _SEC_OMX_MESSAGE
{
    OMX_U32 messageType;
    OMX_U32 messageParam;
    OMX_PTR pCmdData;
} SEC_OMX_MESSAGE;

typedef struct _SEC_OMX_DATABUFFER
{
    OMX_HANDLETYPE        bufferMutex;
    OMX_BUFFERHEADERTYPE* bufferHeader;
    OMX_BOOL              dataValid;
    OMX_U32               allocSize;
    OMX_U32               dataLen;
    OMX_U32               usedDataLen;
    OMX_U32               remainDataLen;
    OMX_U32               nFlags;
    OMX_TICKS             timeStamp;
} SEC_OMX_DATABUFFER;

typedef struct _SEC_OMX_DATA
{
    OMX_BYTE  dataBuffer;
    OMX_U32   allocSize;
    OMX_U32   dataLen;
    OMX_U32   usedDataLen;
    OMX_U32   remainDataLen;
    OMX_U32   previousDataLen;
    OMX_U32   nFlags;
    OMX_TICKS timeStamp;
} SEC_OMX_DATA;

/* for Check TimeStamp after Seek */
typedef struct _SEC_OMX_TIMESTAPM
{
    OMX_BOOL  needSetStartTimeStamp;
    OMX_BOOL  needCheckStartTimeStamp;
    OMX_TICKS startTimeStamp;
    OMX_U32   nStartFlags;
} SEC_OMX_TIMESTAMP;

typedef struct _SEC_OMX_BASECOMPONENT
{
    OMX_STRING               componentName;
    OMX_VERSIONTYPE          componentVersion;
    OMX_VERSIONTYPE          specVersion;

    OMX_STATETYPE            currentState;
    SEC_OMX_TRANS_STATETYPE  transientState;

    SEC_CODEC_TYPE           codecType;
    SEC_OMX_PRIORITYMGMTTYPE compPriority;
    OMX_MARKTYPE             propagateMarkType;
    OMX_HANDLETYPE           compMutex;

    OMX_HANDLETYPE           hComponentHandle;

    /* Message Handler */
    OMX_BOOL                 bExitMessageHandlerThread;
    OMX_HANDLETYPE           hMessageHandler;
    OMX_HANDLETYPE           msgSemaphoreHandle;
    SEC_QUEUE                messageQ;

    /* Buffer Process */
    OMX_BOOL                 bExitBufferProcessThread;
    OMX_HANDLETYPE           hBufferProcess;

    /* Buffer */
    SEC_OMX_DATABUFFER       secDataBuffer[2];

    /* Data */
    SEC_OMX_DATA             processData[2];

    /* Port */
    OMX_PORT_PARAM_TYPE      portParam;
    SEC_OMX_BASEPORT        *pSECPort;

    OMX_HANDLETYPE           pauseEvent;

    /* Callback function */
    OMX_CALLBACKTYPE        *pCallbacks;
    OMX_PTR                  callbackData;

    /* Save Timestamp */
    OMX_TICKS                timeStamp[MAX_TIMESTAMP];
    SEC_OMX_TIMESTAMP        checkTimeStamp;

    /* Save Flags */
    OMX_U32                  nFlags[MAX_FLAGS];

    OMX_BOOL                 getAllDelayBuffer;
    OMX_BOOL                 remainOutputData;
    OMX_BOOL                 reInputData;

    /* Android CapabilityFlags */
    OMXComponentCapabilityFlagsType capabilityFlags;

    OMX_BOOL bUseFlagEOF;
    OMX_BOOL bSaveFlagEOS;

    OMX_ERRORTYPE (*sec_mfc_componentInit)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*sec_mfc_componentTerminate)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*sec_mfc_bufferProcess) (OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData);

    OMX_ERRORTYPE (*sec_AllocateTunnelBuffer)(SEC_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*sec_FreeTunnelBuffer)(SEC_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*sec_BufferProcess)(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE (*sec_BufferReset)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*sec_InputBufferReturn)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*sec_OutputBufferReturn)(OMX_COMPONENTTYPE *pOMXComponent);

    OMX_ERRORTYPE (*sec_allocSecureInputBuffer)(OMX_IN OMX_HANDLETYPE hComponent,
                                                OMX_IN OMX_U32 nBufferSize,
                                                OMX_INOUT OMX_PTR *pInputBuffer_physicalAddress);
    OMX_ERRORTYPE (*sec_freeSecureInputBuffer)(OMX_IN OMX_HANDLETYPE hComponent,
                                               OMX_INOUT OMX_PTR pInputBuffer_physicalAddress);

    int (*sec_checkInputFrame)(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame);
} SEC_OMX_BASECOMPONENT;

OMX_ERRORTYPE SEC_OMX_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure);

OMX_ERRORTYPE SEC_OMX_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure);

OMX_ERRORTYPE SEC_OMX_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     pComponentConfigStructure);

OMX_ERRORTYPE SEC_OMX_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure);

OMX_ERRORTYPE SEC_OMX_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType);

OMX_ERRORTYPE SEC_OMX_BaseComponent_Constructor(OMX_IN OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE SEC_OMX_BaseComponent_Destructor(OMX_IN OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
extern "C" {
#endif

    OMX_ERRORTYPE SEC_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size);


#ifdef __cplusplus
};
#endif

#endif
