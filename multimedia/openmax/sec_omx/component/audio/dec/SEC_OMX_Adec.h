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
 * @file        SEC_OMX_Adec.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 *
 * @version     1.1.0
 * @history
 *   2011.10.18 : Create
 */

#ifndef SEC_OMX_AUDIO_DECODE
#define SEC_OMX_AUDIO_DECODE

#include "OMX_Component.h"
#include "SEC_OMX_Def.h"
#include "SEC_OSAL_Queue.h"
#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Basecomponent.h"

#define MAX_AUDIO_INPUTBUFFER_NUM    2
#define MAX_AUDIO_OUTPUTBUFFER_NUM   2

#define DEFAULT_AUDIO_INPUT_BUFFER_SIZE     (16 * 1024)
#define DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE    (32 * 1024)

#define DEFAULT_AUDIO_SAMPLING_FREQ  44100
#define DEFAULT_AUDIO_CHANNELS_NUM   2
#define DEFAULT_AUDIO_BIT_PER_SAMPLE 16

#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    1
#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX   1

typedef struct _SRP_DEC_INPUT_BUFFER
{
    void *PhyAddr;      // physical address
    void *VirAddr;      // virtual address
    int   bufferSize;   // input buffer alloc size
    int   dataSize;     // Data length
} SRP_DEC_INPUT_BUFFER;

typedef struct _SEC_OMX_AUDIODEC_COMPONENT
{
    OMX_HANDLETYPE hCodecHandle;

    OMX_BOOL bFirstFrame;
    OMX_PTR pInputBuffer;
    SRP_DEC_INPUT_BUFFER SRPDecInputBuffer[MAX_AUDIO_INPUTBUFFER_NUM];
    OMX_U32  indexInputBuffer;
} SEC_OMX_AUDIODEC_COMPONENT;


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer);
OMX_ERRORTYPE SEC_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes);
OMX_ERRORTYPE SEC_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr);
OMX_ERRORTYPE SEC_OMX_AllocateTunnelBuffer(
    SEC_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE SEC_OMX_FreeTunnelBuffer(
    SEC_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE SEC_OMX_ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32         nPort,
    OMX_IN OMX_HANDLETYPE  hTunneledComp,
    OMX_IN OMX_U32         nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup);
OMX_ERRORTYPE SEC_OMX_BufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE SEC_OMX_AudioDecodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure);
OMX_ERRORTYPE SEC_OMX_AudioDecodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure);
OMX_ERRORTYPE SEC_OMX_AudioDecodeGetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE SEC_OMX_AudioDecodeSetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE SEC_OMX_AudioDecodeGetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType);
OMX_ERRORTYPE SEC_OMX_AudioDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE SEC_OMX_AudioDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_BOOL SEC_Check_BufferProcess_State(SEC_OMX_BASECOMPONENT *pSECComponent);
inline void SEC_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent);

#ifdef __cplusplus
}
#endif

#endif /* SEC_OMX_AUDIO_DECODE */
