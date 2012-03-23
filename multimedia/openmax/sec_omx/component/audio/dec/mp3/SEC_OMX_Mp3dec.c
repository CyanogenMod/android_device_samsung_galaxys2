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
 * @file      SEC_OMX_Mp3dec.c
 * @brief
 * @author    Yunji Kim (yunji.kim@samsung.com)
 * @version   1.1.0
 * @history
 *   2011.10.18 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEC_OMX_Macros.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Adec.h"
#include "SEC_OSAL_ETC.h"
#include "SEC_OSAL_Semaphore.h"
#include "SEC_OSAL_Thread.h"
#include "library_register.h"
#include "SEC_OMX_Mp3dec.h"
#include "srp_api.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_MP3_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

//#define SRP_DUMP_TO_FILE
#ifdef SRP_DUMP_TO_FILE
#include "stdio.h"

FILE *inFile;
FILE *outFile;
#endif

OMX_ERRORTYPE SEC_SRP_Mp3Dec_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamAudioMp3:
    {
        OMX_AUDIO_PARAM_MP3TYPE *pDstMp3Param = (OMX_AUDIO_PARAM_MP3TYPE *)pComponentParameterStructure;
        OMX_AUDIO_PARAM_MP3TYPE *pSrcMp3Param = NULL;
        SEC_MP3_HANDLE          *pMp3Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstMp3Param, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMp3Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMp3Dec = (SEC_MP3_HANDLE *)((SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcMp3Param = &pMp3Dec->mp3Param;

        SEC_OSAL_Memcpy(pDstMp3Param, pSrcMp3Param, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
    }
        break;
    case OMX_IndexParamAudioPcm:
    {
        OMX_AUDIO_PARAM_PCMMODETYPE *pDstPcmParam = (OMX_AUDIO_PARAM_PCMMODETYPE *)pComponentParameterStructure;
        OMX_AUDIO_PARAM_PCMMODETYPE *pSrcPcmParam = NULL;
        SEC_MP3_HANDLE              *pMp3Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstPcmParam, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstPcmParam->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMp3Dec = (SEC_MP3_HANDLE *)((SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pSrcPcmParam = &pMp3Dec->pcmParam;

        SEC_OSAL_Memcpy(pDstPcmParam, pSrcPcmParam, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_S32 codecType;
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_MP3_DEC_ROLE);
    }
        break;
    default:
        ret = SEC_OMX_AudioDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamAudioMp3:
    {
        OMX_AUDIO_PARAM_MP3TYPE *pDstMp3Param = NULL;
        OMX_AUDIO_PARAM_MP3TYPE *pSrcMp3Param = (OMX_AUDIO_PARAM_MP3TYPE *)pComponentParameterStructure;
        SEC_MP3_HANDLE          *pMp3Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcMp3Param, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMp3Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMp3Dec = (SEC_MP3_HANDLE *)((SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstMp3Param = &pMp3Dec->mp3Param;

        SEC_OSAL_Memcpy(pDstMp3Param, pSrcMp3Param, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
    }
        break;
    case OMX_IndexParamAudioPcm:
    {
        OMX_AUDIO_PARAM_PCMMODETYPE *pDstPcmParam = NULL;
        OMX_AUDIO_PARAM_PCMMODETYPE *pSrcPcmParam = (OMX_AUDIO_PARAM_PCMMODETYPE *)pComponentParameterStructure;
        SEC_MP3_HANDLE              *pMp3Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcPcmParam, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcPcmParam->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMp3Dec = (SEC_MP3_HANDLE *)((SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
        pDstPcmParam = &pMp3Dec->pcmParam;

        SEC_OSAL_Memcpy(pDstPcmParam, pSrcPcmParam, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_MP3_DEC_ROLE)) {
            pSECComponent->pSECPort[INPUT_PORT_INDEX].portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingMP3;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    default:
        ret = SEC_OMX_AudioDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = SEC_OMX_AudioDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = SEC_OMX_AudioDecodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    ret = SEC_OMX_AudioDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    OMX_S32                  codecType;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex != (MAX_COMPONENT_ROLE_NUM - 1)) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_MP3_DEC_ROLE);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_AUDIODEC_COMPONENT *pAudioDec = (SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_MP3_HANDLE             *pMp3Dec = (SEC_MP3_HANDLE *)pAudioDec->hCodecHandle;

    FunctionIn();

    SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pSECComponent->bUseFlagEOF = OMX_TRUE; /* Mp3 extractor should parse into frame unit. */
    pSECComponent->bSaveFlagEOS = OMX_FALSE;
    pMp3Dec->hSRPMp3Handle.bConfiguredSRP = OMX_FALSE;
    pMp3Dec->hSRPMp3Handle.bSRPSendEOS = OMX_FALSE;
    pSECComponent->getAllDelayBuffer = OMX_FALSE;

#ifdef SRP_DUMP_TO_FILE
    inFile = fopen("/data/InFile.mp3", "w+");
    outFile = fopen("/data/OutFile.pcm", "w+");
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    FunctionIn();

#ifdef SRP_DUMP_TO_FILE
    fclose(inFile);
    fclose(outFile);
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3_Decode_Block(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT      *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_AUDIODEC_COMPONENT *pAudioDec = (SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle;
    SEC_MP3_HANDLE             *pMp3Dec = (SEC_MP3_HANDLE *)pAudioDec->hCodecHandle;
    struct srp_dec_info         codecDecInfo;
    OMX_S32                     returnCodec = 0;
    unsigned long               isSRPStopped = 0;
    OMX_PTR                     dataBuffer = NULL;
    unsigned int                dataLen = 0;
    OMX_BOOL                    isSRPIbufOverflow = OMX_FALSE;

    FunctionIn();

#ifdef SRP_DUMP_TO_FILE
    if (pSECComponent->reInputData == OMX_FALSE) {
        fwrite(pInputData->dataBuffer, pInputData->dataLen, 1, inFile);
    }
#endif

    /* Save timestamp and flags of input data */
    pOutputData->timeStamp = pInputData->timeStamp;
    pOutputData->nFlags = pInputData->nFlags & (~OMX_BUFFERFLAG_EOS);

    /* Decoding mp3 frames by SRP */
    if (pSECComponent->getAllDelayBuffer == OMX_FALSE) {
        returnCodec = SRP_Decode(pInputData->dataBuffer, pInputData->dataLen);

        if (returnCodec >= 0) {
            if (pInputData->nFlags & OMX_BUFFERFLAG_EOS) {
                SRP_Send_EOS();
                pMp3Dec->hSRPMp3Handle.bSRPSendEOS = OMX_TRUE;
            }
        } else if (returnCodec == SRP_ERROR_IBUF_OVERFLOW) {
            isSRPIbufOverflow = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        }
    }

    if (pMp3Dec->hSRPMp3Handle.bConfiguredSRP == OMX_FALSE) {
        returnCodec = SRP_Get_Dec_Info(&codecDecInfo);
        if (returnCodec < 0) {
            SEC_OSAL_Log(SEC_LOG_ERROR, "SRP_Get_Dec_Info failed: %d", returnCodec);
            ret = OMX_ErrorHardware;
            goto EXIT;
        }

        if (!codecDecInfo.sample_rate || !codecDecInfo.channels) {
            if (pMp3Dec->hSRPMp3Handle.bSRPSendEOS == OMX_TRUE) {
                pOutputData->dataLen = 0;
                pSECComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else {
                pSECComponent->getAllDelayBuffer = OMX_FALSE;
                if (isSRPIbufOverflow)
                    ret = OMX_ErrorInputDataDecodeYet;
                else
                    ret = OMX_ErrorNone;
            }
            goto EXIT;
        }

        SEC_OSAL_Log(SEC_LOG_TRACE, "numChannels(%d), samplingRate(%d)",
            codecDecInfo.channels, codecDecInfo.sample_rate);

        if (pMp3Dec->pcmParam.nChannels != codecDecInfo.channels ||
            pMp3Dec->pcmParam.nSamplingRate != codecDecInfo.sample_rate) {
            /* Change channel count and sampling rate information */
            pMp3Dec->pcmParam.nChannels = codecDecInfo.channels;
            pMp3Dec->pcmParam.nSamplingRate = codecDecInfo.sample_rate;

            /* Send Port Settings changed call back */
            (*(pSECComponent->pCallbacks->EventHandler))
                  (pOMXComponent,
                   pSECComponent->callbackData,
                   OMX_EventPortSettingsChanged, /* The command was completed */
                   OMX_DirOutput, /* This is the port index */
                   0,
                   NULL);
        }

        pMp3Dec->hSRPMp3Handle.bConfiguredSRP = OMX_TRUE;

        if (pMp3Dec->hSRPMp3Handle.bSRPSendEOS == OMX_TRUE) {
            pOutputData->dataLen = 0;
            pSECComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else {
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            if (isSRPIbufOverflow)
                ret = OMX_ErrorInputDataDecodeYet;
            else
                ret = OMX_ErrorNone;
        }
        goto EXIT;
    }

    /* Get decoded data from SRP */
    returnCodec = SRP_Get_PCM(&dataBuffer, &dataLen);
    if (dataLen > 0) {
        pOutputData->dataLen = dataLen;
        SEC_OSAL_Memcpy(pOutputData->dataBuffer, dataBuffer, dataLen);
    } else {
        pOutputData->dataLen = 0;
    }

#ifdef SRP_DUMP_TO_FILE
    if (pOutputData->dataLen > 0)
        fwrite(pOutputData->dataBuffer, pOutputData->dataLen, 1, outFile);
#endif

    /* Delay EOS signal until all the PCM is returned from the SRP driver. */
    if (pMp3Dec->hSRPMp3Handle.bSRPSendEOS == OMX_TRUE) {
        if (pInputData->nFlags & OMX_BUFFERFLAG_EOS) {
            returnCodec = SRP_GetParams(SRP_STOP_EOS_STATE, &isSRPStopped);
            if (returnCodec != 0)
                SEC_OSAL_Log(SEC_LOG_ERROR, "Fail SRP_STOP_EOS_STATE");
            if (isSRPStopped == 1) {
                pOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pSECComponent->getAllDelayBuffer = OMX_FALSE;
                pMp3Dec->hSRPMp3Handle.bSRPSendEOS = OMX_FALSE; /* for repeating one song */
                ret = OMX_ErrorNone;
            } else {
                pSECComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            }
        } else { /* Flush after EOS */
            pMp3Dec->hSRPMp3Handle.bSRPSendEOS = OMX_FALSE;
        }
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_SRP_Mp3Dec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_OMX_BASEPORT      *pInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT      *pOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pInputPort)) || (!CHECK_PORT_ENABLED(pOutputPort)) ||
            (!CHECK_PORT_POPULATED(pInputPort)) || (!CHECK_PORT_POPULATED(pOutputPort))) {
        if (pInputData->nFlags & OMX_BUFFERFLAG_EOS)
            ret = OMX_ErrorInputDataDecodeYet;
        else
            ret = OMX_ErrorNone;

        goto EXIT;
    }
    if (OMX_FALSE == SEC_Check_BufferProcess_State(pSECComponent)) {
        if (pInputData->nFlags & OMX_BUFFERFLAG_EOS)
            ret = OMX_ErrorInputDataDecodeYet;
        else
            ret = OMX_ErrorNone;

        goto EXIT;
    }

    ret = SEC_SRP_Mp3_Decode_Block(pOMXComponent, pInputData, pOutputData);

    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataDecodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                    pSECComponent->callbackData,
                                                    OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->previousDataLen = pInputData->dataLen;
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        pOutputData->usedDataLen = 0;
        pOutputData->remainDataLen = pOutputData->dataLen;
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE SEC_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT      *pSECComponent = NULL;
    SEC_OMX_BASEPORT           *pSECPort = NULL;
    SEC_OMX_AUDIODEC_COMPONENT *pAudioDec = NULL;
    SEC_MP3_HANDLE             *pMp3Dec = NULL;
    OMX_PTR                     pInputBuffer = NULL;
    OMX_PTR                     pOutputBuffer = NULL;
    unsigned int                inputBufferSize = 0;
    unsigned int                inputBufferNum = 0;
    unsigned int                outputBufferSize = 0;
    unsigned int                outputBufferNum = 0;
    OMX_S32                     returnCodec;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_MP3_DEC, componentName) != 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, componentName, ret);
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_AudioDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_OMX_AudioDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->codecType = HW_AUDIO_DEC_CODEC;

    pSECComponent->componentName = (OMX_STRING)SEC_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pSECComponent->componentName == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT_ERROR_1;
    }
    SEC_OSAL_Memset(pSECComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);
    SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_MP3_DEC);

    pMp3Dec = SEC_OSAL_Malloc(sizeof(SEC_MP3_HANDLE));
    if (pMp3Dec == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "%s: SEC_MP3_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT_ERROR_2;
    }
    SEC_OSAL_Memset(pMp3Dec, 0, sizeof(SEC_MP3_HANDLE));
    pAudioDec = (SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle;
    pAudioDec->hCodecHandle = (OMX_HANDLETYPE)pMp3Dec;

    /* Create and Init SRP */
    pMp3Dec->hSRPMp3Handle.bSRPLoaded = OMX_FALSE;
    returnCodec = SRP_Create(SRP_INIT_BLOCK_MODE);
    if (returnCodec < 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SRP_Create failed: %d", returnCodec);
        ret = OMX_ErrorHardware;
        goto EXIT_ERROR_3;
    }
    pMp3Dec->hSRPMp3Handle.hSRPHandle = (OMX_HANDLETYPE)returnCodec; /* SRP's fd */
    returnCodec = SRP_Init();
    if (returnCodec < 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SRP_Init failed: %d", returnCodec);
        ret = OMX_ErrorHardware;
        goto EXIT_ERROR_4;
    }
    pMp3Dec->hSRPMp3Handle.bSRPLoaded = OMX_TRUE;

    /* Get input buffer info from SRP */
    returnCodec = SRP_Get_Ibuf_Info(&pInputBuffer, &inputBufferSize, &inputBufferNum);
    if (returnCodec < 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SRP_Get_Ibuf_Info failed: %d", returnCodec);
        ret = OMX_ErrorHardware;
        goto EXIT_ERROR_5;
    }

    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = inputBufferSize;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = SEC_OSAL_Malloc(inputBufferSize);
    if (pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "Input data buffer alloc failed");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT_ERROR_5;
    }

    /* Get output buffer info from SRP */
    returnCodec = SRP_Get_Obuf_Info(&pOutputBuffer, &outputBufferSize, &outputBufferNum);
    if (returnCodec < 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SRP_Get_Obuf_Info failed: %d", returnCodec);
        ret = OMX_ErrorHardware;
        goto EXIT_ERROR_6;
    }

    /* Set componentVersion */
    pSECComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pSECComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pSECComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pSECComponent->componentVersion.s.nStep         = STEP_NUMBER;

    /* Set specVersion */
    pSECComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pSECComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pSECComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pSECComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Input port */
    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECPort->portDefinition.nBufferCountActual = inputBufferNum;
    pSECPort->portDefinition.nBufferCountMin = inputBufferNum;
    pSECPort->portDefinition.nBufferSize = inputBufferSize;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.audio.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.audio.cMIMEType, "audio/mpeg");
    pSECPort->portDefinition.format.audio.pNativeRender = 0;
    pSECPort->portDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.nBufferCountActual = outputBufferNum;
    pSECPort->portDefinition.nBufferCountMin = outputBufferNum;
    pSECPort->portDefinition.nBufferSize = outputBufferSize;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.audio.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.audio.cMIMEType, "audio/raw");
    pSECPort->portDefinition.format.audio.pNativeRender = 0;
    pSECPort->portDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    /* Default values for Mp3 audio param */
    INIT_SET_SIZE_VERSION(&pMp3Dec->mp3Param, OMX_AUDIO_PARAM_MP3TYPE);
    pMp3Dec->mp3Param.nPortIndex      = INPUT_PORT_INDEX;
    pMp3Dec->mp3Param.nChannels       = DEFAULT_AUDIO_CHANNELS_NUM;
    pMp3Dec->mp3Param.nBitRate        = 0;
    pMp3Dec->mp3Param.nSampleRate     = DEFAULT_AUDIO_SAMPLING_FREQ;
    pMp3Dec->mp3Param.nAudioBandWidth = 0;
    pMp3Dec->mp3Param.eChannelMode    = OMX_AUDIO_ChannelModeStereo;
    pMp3Dec->mp3Param.eFormat         = OMX_AUDIO_MP3StreamFormatMP1Layer3;

    /* Default values for PCM audio param */
    INIT_SET_SIZE_VERSION(&pMp3Dec->pcmParam, OMX_AUDIO_PARAM_PCMMODETYPE);
    pMp3Dec->pcmParam.nPortIndex         = OUTPUT_PORT_INDEX;
    pMp3Dec->pcmParam.nChannels          = DEFAULT_AUDIO_CHANNELS_NUM;
    pMp3Dec->pcmParam.eNumData           = OMX_NumericalDataSigned;
    pMp3Dec->pcmParam.eEndian            = OMX_EndianLittle;
    pMp3Dec->pcmParam.bInterleaved       = OMX_TRUE;
    pMp3Dec->pcmParam.nBitPerSample      = DEFAULT_AUDIO_BIT_PER_SAMPLE;
    pMp3Dec->pcmParam.nSamplingRate      = DEFAULT_AUDIO_SAMPLING_FREQ;
    pMp3Dec->pcmParam.ePCMMode           = OMX_AUDIO_PCMModeLinear;
    pMp3Dec->pcmParam.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    pMp3Dec->pcmParam.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    pOMXComponent->GetParameter      = &SEC_SRP_Mp3Dec_GetParameter;
    pOMXComponent->SetParameter      = &SEC_SRP_Mp3Dec_SetParameter;
    pOMXComponent->GetConfig         = &SEC_SRP_Mp3Dec_GetConfig;
    pOMXComponent->SetConfig         = &SEC_SRP_Mp3Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &SEC_SRP_Mp3Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &SEC_SRP_Mp3Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &SEC_OMX_ComponentDeinit;

    /* ToDo: Change the function name associated with a specific codec */
    pSECComponent->sec_mfc_componentInit      = &SEC_SRP_Mp3Dec_Init;
    pSECComponent->sec_mfc_componentTerminate = &SEC_SRP_Mp3Dec_Terminate;
    pSECComponent->sec_mfc_bufferProcess      = &SEC_SRP_Mp3Dec_bufferProcess;
    pSECComponent->sec_checkInputFrame = NULL;

    pSECComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;
    goto EXIT; /* This function is performed successfully. */

EXIT_ERROR_6:
    SEC_OSAL_Free(pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer);
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = 0;
EXIT_ERROR_5:
    SRP_Deinit();
EXIT_ERROR_4:
    SRP_Terminate();
EXIT_ERROR_3:
    SEC_OSAL_Free(pMp3Dec);
    pAudioDec->hCodecHandle = NULL;
EXIT_ERROR_2:
    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;
EXIT_ERROR_1:
    SEC_OMX_AudioDecodeComponentDeinit(pOMXComponent);
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_MP3_HANDLE        *pMp3Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;
    if (pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer) {
        SEC_OSAL_Free(pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer);
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = 0;
    }

    pMp3Dec = (SEC_MP3_HANDLE *)((SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle;
    if (pMp3Dec != NULL) {
        if (pMp3Dec->hSRPMp3Handle.bSRPLoaded == OMX_TRUE) {
            SRP_Deinit();
            SRP_Terminate();
        }
        SEC_OSAL_Free(pMp3Dec);
        ((SEC_OMX_AUDIODEC_COMPONENT *)pSECComponent->hComponentHandle)->hCodecHandle = NULL;
    }

    ret = SEC_OMX_AudioDecodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
