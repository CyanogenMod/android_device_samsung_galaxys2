/*
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <sys/poll.h>
#include "videodev2.h"

#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "MFC_ENC_APP"
#include <utils/Log.h>

#define POLL_ENC_WAIT_TIMEOUT 25

#ifndef true
#define true  (1)
#endif

#ifndef false
#define false (0)
#endif

#define MAX_STREAM_SIZE (2*1024*1024)

static char *mfc_dev_name = SAMSUNG_MFC_DEV_NAME;
static int mfc_dev_node = 7;

static void getMFCName(char *devicename, int size)
{
    snprintf(devicename, size, "%s%d", SAMSUNG_MFC_DEV_NAME, mfc_dev_node);
}

void SsbSipMfcEncSetMFCName(char *devicename)
{
    mfc_dev_name = devicename;
}

void *SsbSipMfcEncOpen(void)
{
    int hMFCOpen;
    _MFCLIB *pCTX;

    char mfc_dev_name[64];

    int ret;
    struct v4l2_capability cap;

    getMFCName(mfc_dev_name, 64);
    LOGI("[%s] dev name is %s\n",__func__,mfc_dev_name);

    if (access(mfc_dev_name, F_OK) != 0) {
        LOGE("[%s] MFC device node not exists",__func__);
        return NULL;
    }

    hMFCOpen = open(mfc_dev_name, O_RDWR | O_NONBLOCK, 0);
    if (hMFCOpen < 0) {
        LOGE("[%s] Failed to open MFC device",__func__);
        return NULL;
    }

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("[%s] malloc failed.",__func__);
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    pCTX->hMFC = hMFCOpen;

    memset(&cap, 0, sizeof(cap));
    ret = ioctl(pCTX->hMFC, VIDIOC_QUERYCAP, &cap);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QUERYCAP failed",__func__);
        close(pCTX->hMFC);
        free(pCTX);
        return NULL;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("[%s] Device does not support capture",__func__);
        close(pCTX->hMFC);
        free(pCTX);
        return NULL;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        LOGE("[%s] Device does not support output",__func__);
        close(pCTX->hMFC);
        free(pCTX);
        return NULL;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("[%s] Device does not support streaming",__func__);
        close(pCTX->hMFC);
        free(pCTX);
        return NULL;
    }

    pCTX->v4l2_enc.bRunning = 0;
    /* physical address is used for Input source */
    pCTX->v4l2_enc.bInputPhyVir = 1;

    pCTX->cacheablebuffer = NO_CACHE;

    return (void *)pCTX;
}

void *SsbSipMfcEncOpenExt(void *value)
{
    _MFCLIB *pCTX;

    pCTX = SsbSipMfcEncOpen();
    if (pCTX == NULL)
        return NULL;

    if (NO_CACHE == (*(SSBIP_MFC_BUFFER_TYPE *)value)) {
        pCTX->cacheablebuffer = NO_CACHE;
        /* physical address is used for Input source */
        pCTX->v4l2_enc.bInputPhyVir = 1;
        LOGI("[%s] non cacheable buffer",__func__);
    }
    else {
        pCTX->cacheablebuffer = CACHE;
        /* vitual address is used for Input source */
        pCTX->v4l2_enc.bInputPhyVir = 0;
        LOGI("[%s] cacheable buffer",__func__);
    }

    return (void *)pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncClose(void *openHandle)
{
    _MFCLIB *pCTX;
    int i;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *) openHandle;

    if (!pCTX->v4l2_enc.bInputPhyVir) {
        for (i = 0; i < pCTX->v4l2_enc.mfc_num_src_bufs; i++) {
            munmap(pCTX->v4l2_enc.mfc_src_bufs[i][0], pCTX->v4l2_enc.mfc_src_bufs_len[0]);
            munmap(pCTX->v4l2_enc.mfc_src_bufs[i][1], pCTX->v4l2_enc.mfc_src_bufs_len[1]);
        }
    }

    for (i = 0; i < pCTX->v4l2_enc.mfc_num_dst_bufs; i++)
        munmap(pCTX->v4l2_enc.mfc_dst_bufs[i], pCTX->v4l2_enc.mfc_dst_bufs_len);

    pCTX->inter_buff_status = MFC_USE_NONE;

    close(pCTX->hMFC);

    free(pCTX);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncInit(void *openHandle, void *param)
{
    int ret, i, j,index;
    _MFCLIB *pCTX;

    enum v4l2_buf_type type;
    struct v4l2_format fmt;
    struct v4l2_plane planes[MFC_ENC_NUM_PLANES];

    struct v4l2_buffer buf;
    struct v4l2_requestbuffers reqbuf;

    struct v4l2_control ctrl;

    struct pollfd poll_events;
    int poll_state;

    struct v4l2_ext_control ext_ctrl_mpeg4[27];
    struct v4l2_ext_control ext_ctrl_h263[19];
    struct v4l2_ext_control ext_ctrl[44];
    struct v4l2_ext_controls ext_ctrls;

    SSBSIP_MFC_ENC_H264_PARAM *h264_arg;
    SSBSIP_MFC_ENC_MPEG4_PARAM *mpeg4_arg;
    SSBSIP_MFC_ENC_H263_PARAM *h263_arg;

    if (openHandle == NULL) {
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM*)param;
    if (mpeg4_arg->codecType == MPEG4_ENC) {
        pCTX->codecType= MPEG4_ENC;
        pCTX->width = mpeg4_arg->SourceWidth;
        pCTX->height = mpeg4_arg->SourceHeight;
        pCTX->framemap = mpeg4_arg->FrameMap;
    } else {
        h263_arg = (SSBSIP_MFC_ENC_H263_PARAM*)param;
        if (h263_arg->codecType == H263_ENC) {
            pCTX->codecType = H263_ENC;
            pCTX->width = h263_arg->SourceWidth;
            pCTX->height = h263_arg->SourceHeight;
            pCTX->framemap = h263_arg->FrameMap;
        } else {
            h264_arg = (SSBSIP_MFC_ENC_H264_PARAM*)param;
            if (h264_arg->codecType == H264_ENC) {
                pCTX->codecType = H264_ENC;
                pCTX->width = h264_arg->SourceWidth;
                pCTX->height = h264_arg->SourceHeight;
                pCTX->framemap = h264_arg->FrameMap;
            } else {
                LOGE("[%s] Undefined codec type \n",__func__);
                ret = MFC_RET_INVALID_PARAM;
                goto error_case1;
            }
        }
    }

    switch (pCTX->codecType) {
    case MPEG4_ENC:
        ext_ctrl_mpeg4[0].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_PROFILE;
        ext_ctrl_mpeg4[0].value = mpeg4_arg->ProfileIDC;
        ext_ctrl_mpeg4[1].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_LEVEL;
        ext_ctrl_mpeg4[1].value = mpeg4_arg->LevelIDC;
        ext_ctrl_mpeg4[2].id = V4L2_CID_CODEC_MFC5X_ENC_GOP_SIZE;
        ext_ctrl_mpeg4[2].value = mpeg4_arg->IDRPeriod;
        ext_ctrl_mpeg4[3].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_QUARTER_PIXEL;
        ext_ctrl_mpeg4[3].value = mpeg4_arg->DisableQpelME;

        ext_ctrl_mpeg4[4].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MODE;
        ext_ctrl_mpeg4[4].value = mpeg4_arg->SliceMode; /* 0: one, 1: fixed #mb, 3: fixed #bytes */
        if (mpeg4_arg->SliceMode == 0) {
            ext_ctrl_mpeg4[5].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MB;
            ext_ctrl_mpeg4[5].value = 1;  /* default */
            ext_ctrl_mpeg4[6].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_BIT;
            ext_ctrl_mpeg4[6].value = 1900; /* default */
        } else if (mpeg4_arg->SliceMode == 1) {
            ext_ctrl_mpeg4[5].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MB;
            ext_ctrl_mpeg4[5].value = mpeg4_arg->SliceArgument;
            ext_ctrl_mpeg4[6].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_BIT;
            ext_ctrl_mpeg4[6].value = 1900; /* default */
        } else if (mpeg4_arg->SliceMode == 3) {
            ext_ctrl_mpeg4[5].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MB;
            ext_ctrl_mpeg4[5].value = 1; /* default */
            ext_ctrl_mpeg4[6].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_BIT;
            ext_ctrl_mpeg4[6].value = mpeg4_arg->SliceArgument;
        }
        /*
        It should be set using mpeg4_arg->NumberBFrames after being handled by appl.
         */
        ext_ctrl_mpeg4[7].id =  V4L2_CID_CODEC_MFC5X_ENC_MPEG4_B_FRAMES;
        ext_ctrl_mpeg4[7].value = mpeg4_arg->NumberBFrames;
        ext_ctrl_mpeg4[8].id = V4L2_CID_CODEC_MFC5X_ENC_INTRA_REFRESH_MB;
        ext_ctrl_mpeg4[8].value = mpeg4_arg->RandomIntraMBRefresh;

        ext_ctrl_mpeg4[9].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CTRL_ENABLE;
        ext_ctrl_mpeg4[9].value = mpeg4_arg->PadControlOn;
        ext_ctrl_mpeg4[10].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_LUMA_VALUE;
        ext_ctrl_mpeg4[10].value = mpeg4_arg->LumaPadVal;
        ext_ctrl_mpeg4[11].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CB_VALUE;
        ext_ctrl_mpeg4[11].value = mpeg4_arg->CbPadVal;
        ext_ctrl_mpeg4[12].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CR_VALUE;
        ext_ctrl_mpeg4[12].value = mpeg4_arg->CrPadVal;

        ext_ctrl_mpeg4[13].id = V4L2_CID_CODEC_MFC5X_ENC_RC_FRAME_ENABLE;
        ext_ctrl_mpeg4[13].value = mpeg4_arg->EnableFRMRateControl;
        ext_ctrl_mpeg4[14].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_VOP_TIME_RES;
        ext_ctrl_mpeg4[14].value = mpeg4_arg->TimeIncreamentRes;
        ext_ctrl_mpeg4[15].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_VOP_FRM_DELTA;
        ext_ctrl_mpeg4[15].value = mpeg4_arg->VopTimeIncreament;
        ext_ctrl_mpeg4[16].id = V4L2_CID_CODEC_MFC5X_ENC_RC_BIT_RATE;
        ext_ctrl_mpeg4[16].value = mpeg4_arg->Bitrate;

        ext_ctrl_mpeg4[17].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_RC_FRAME_QP;
        ext_ctrl_mpeg4[17].value = mpeg4_arg->FrameQp;
        ext_ctrl_mpeg4[18].id =  V4L2_CID_CODEC_MFC5X_ENC_MPEG4_RC_P_FRAME_QP;
        ext_ctrl_mpeg4[18].value = mpeg4_arg->FrameQp_P;
        ext_ctrl_mpeg4[19].id =  V4L2_CID_CODEC_MFC5X_ENC_MPEG4_RC_B_FRAME_QP;
        ext_ctrl_mpeg4[19].value = mpeg4_arg->FrameQp_B;

        ext_ctrl_mpeg4[20].id =  V4L2_CID_CODEC_MFC5X_ENC_MPEG4_RC_MAX_QP;
        ext_ctrl_mpeg4[20].value = mpeg4_arg->QSCodeMax;
        ext_ctrl_mpeg4[21].id = V4L2_CID_CODEC_MFC5X_ENC_MPEG4_RC_MIN_QP;
        ext_ctrl_mpeg4[21].value = mpeg4_arg->QSCodeMin;
        ext_ctrl_mpeg4[22].id = V4L2_CID_CODEC_MFC5X_ENC_RC_REACTION_COEFF;
        ext_ctrl_mpeg4[22].value = mpeg4_arg->CBRPeriodRf;

        if (V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_LEVEL == pCTX->enc_frameskip) {
            ext_ctrl_mpeg4[23].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl_mpeg4[23].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_LEVEL;
        } else if(V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_VBV_BUF_SIZE == pCTX->enc_frameskip) {
            ext_ctrl_mpeg4[23].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl_mpeg4[23].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_VBV_BUF_SIZE;
        } else { /* ENC_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl_mpeg4[23].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl_mpeg4[23].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_DISABLE;
        }

        ext_ctrl_mpeg4[24].id = V4L2_CID_CODEC_MFC5X_ENC_VBV_BUF_SIZE;
        ext_ctrl_mpeg4[24].value = 0;

        ext_ctrl_mpeg4[25].id = V4L2_CID_CODEC_MFC5X_ENC_SEQ_HDR_MODE;
        ext_ctrl_mpeg4[25].value = 0;

        ext_ctrl_mpeg4[26].id = V4L2_CID_CODEC_MFC5X_ENC_RC_FIXED_TARGET_BIT;
        ext_ctrl_mpeg4[26].value = V4L2_CODEC_MFC5X_ENC_SW_ENABLE;
        break;

    case H263_ENC:
        ext_ctrl_h263[0].id = V4L2_CID_CODEC_MFC5X_ENC_GOP_SIZE;
        ext_ctrl_h263[0].value = h263_arg->IDRPeriod;

        ext_ctrl_h263[1].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MODE;
        ext_ctrl_h263[1].value = h263_arg->SliceMode; /* 0: one, Check is needed if h264 support multi-slice */

        ext_ctrl_h263[2].id = V4L2_CID_CODEC_MFC5X_ENC_INTRA_REFRESH_MB;
        ext_ctrl_h263[2].value = h263_arg->RandomIntraMBRefresh;

        ext_ctrl_h263[3].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CTRL_ENABLE;
        ext_ctrl_h263[3].value = h263_arg->PadControlOn;
        ext_ctrl_h263[4].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_LUMA_VALUE;
        ext_ctrl_h263[4].value = h263_arg->LumaPadVal;
        ext_ctrl_h263[5].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CB_VALUE;
        ext_ctrl_h263[5].value = h263_arg->CbPadVal;
        ext_ctrl_h263[6].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CR_VALUE;
        ext_ctrl_h263[6].value = h263_arg->CrPadVal;

        ext_ctrl_h263[7].id = V4L2_CID_CODEC_MFC5X_ENC_RC_FRAME_ENABLE;
        ext_ctrl_h263[7].value = h263_arg->EnableFRMRateControl;

        ext_ctrl_h263[8].id = V4L2_CID_CODEC_MFC5X_ENC_H263_RC_FRAME_RATE;
        ext_ctrl_h263[8].value = h263_arg->FrameRate;

        ext_ctrl_h263[9].id = V4L2_CID_CODEC_MFC5X_ENC_RC_BIT_RATE;
        ext_ctrl_h263[9].value = h263_arg->Bitrate;

        ext_ctrl_h263[10].id = V4L2_CID_CODEC_MFC5X_ENC_H263_RC_FRAME_QP;
        ext_ctrl_h263[10].value = h263_arg->FrameQp;
        ext_ctrl_h263[11].id =  V4L2_CID_CODEC_MFC5X_ENC_H263_RC_P_FRAME_QP;
        ext_ctrl_h263[11].value = h263_arg->FrameQp_P;

        ext_ctrl_h263[12].id =  V4L2_CID_CODEC_MFC5X_ENC_H263_RC_MAX_QP;
        ext_ctrl_h263[12].value = h263_arg->QSCodeMax;
        ext_ctrl_h263[13].id = V4L2_CID_CODEC_MFC5X_ENC_H263_RC_MIN_QP;
        ext_ctrl_h263[13].value = h263_arg->QSCodeMin;
        ext_ctrl_h263[14].id = V4L2_CID_CODEC_MFC5X_ENC_RC_REACTION_COEFF;
        ext_ctrl_h263[14].value = h263_arg->CBRPeriodRf;

        if (V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_LEVEL == pCTX->enc_frameskip) {
            ext_ctrl_h263[15].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl_h263[15].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_LEVEL;
        } else if(V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_VBV_BUF_SIZE== pCTX->enc_frameskip) {
            ext_ctrl_h263[15].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl_h263[15].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_VBV_BUF_SIZE;
        } else { /* ENC_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl_h263[15].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl_h263[15].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_DISABLE;
        }

        ext_ctrl_h263[16].id = V4L2_CID_CODEC_MFC5X_ENC_VBV_BUF_SIZE;
        ext_ctrl_h263[16].value = 0;

        ext_ctrl_h263[17].id = V4L2_CID_CODEC_MFC5X_ENC_SEQ_HDR_MODE;
        ext_ctrl_h263[17].value = 0;

        ext_ctrl_h263[18].id = V4L2_CID_CODEC_MFC5X_ENC_RC_FIXED_TARGET_BIT;
        ext_ctrl_h263[18].value = V4L2_CODEC_MFC5X_ENC_SW_ENABLE;
        break;

    case H264_ENC:
        ext_ctrl[0].id = V4L2_CID_CODEC_MFC5X_ENC_H264_PROFILE;
        ext_ctrl[0].value = h264_arg->ProfileIDC;
        ext_ctrl[1].id = V4L2_CID_CODEC_MFC5X_ENC_H264_LEVEL;
        ext_ctrl[1].value = h264_arg->LevelIDC;
        ext_ctrl[2].id = V4L2_CID_CODEC_MFC5X_ENC_GOP_SIZE;
        ext_ctrl[2].value = h264_arg->IDRPeriod;
        ext_ctrl[3].id = V4L2_CID_CODEC_MFC5X_ENC_H264_MAX_REF_PIC;
        ext_ctrl[3].value = h264_arg->NumberReferenceFrames;
        ext_ctrl[4].id = V4L2_CID_CODEC_MFC5X_ENC_H264_NUM_REF_PIC_4P;
        ext_ctrl[4].value = h264_arg->NumberRefForPframes;
        ext_ctrl[5].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MODE;
        ext_ctrl[5].value = h264_arg->SliceMode;  /* 0: one, 1: fixed #mb, 3: fixed #bytes */
        if (h264_arg->SliceMode == 0) {
            ext_ctrl[6].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MB;
            ext_ctrl[6].value = 1;  /* default */
            ext_ctrl[7].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_BIT;
            ext_ctrl[7].value = 1900; /* default */
        } else if (h264_arg->SliceMode == 1) {
            ext_ctrl[6].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MB;
            ext_ctrl[6].value = h264_arg->SliceArgument;
            ext_ctrl[7].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_BIT;
            ext_ctrl[7].value = 1900; /* default */
        } else if (h264_arg->SliceMode == 3) {
            ext_ctrl[6].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_MB;
            ext_ctrl[6].value = 1; /* default */
            ext_ctrl[7].id = V4L2_CID_CODEC_MFC5X_ENC_MULTI_SLICE_BIT;
            ext_ctrl[7].value = h264_arg->SliceArgument;
        }
        /*
        It should be set using h264_arg->NumberBFrames after being handled by appl.
         */
        ext_ctrl[8].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_B_FRAMES;
        ext_ctrl[8].value = h264_arg->NumberBFrames;
        ext_ctrl[9].id = V4L2_CID_CODEC_MFC5X_ENC_H264_LOOP_FILTER_MODE;
        ext_ctrl[9].value = h264_arg->LoopFilterDisable;
        ext_ctrl[10].id = V4L2_CID_CODEC_MFC5X_ENC_H264_LOOP_FILTER_ALPHA;
        ext_ctrl[10].value = h264_arg->LoopFilterAlphaC0Offset;
        ext_ctrl[11].id = V4L2_CID_CODEC_MFC5X_ENC_H264_LOOP_FILTER_BETA;
        ext_ctrl[11].value = h264_arg->LoopFilterBetaOffset;
        ext_ctrl[12].id = V4L2_CID_CODEC_MFC5X_ENC_H264_ENTROPY_MODE;
        ext_ctrl[12].value = h264_arg->SymbolMode;
        ext_ctrl[13].id = V4L2_CID_CODEC_MFC5X_ENC_H264_INTERLACE;
        ext_ctrl[13].value = h264_arg->PictureInterlace;
        ext_ctrl[14].id = V4L2_CID_CODEC_MFC5X_ENC_H264_8X8_TRANSFORM;
        ext_ctrl[14].value = h264_arg->Transform8x8Mode;
        ext_ctrl[15].id = V4L2_CID_CODEC_MFC5X_ENC_INTRA_REFRESH_MB;
        ext_ctrl[15].value = h264_arg->RandomIntraMBRefresh;
        ext_ctrl[16].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CTRL_ENABLE;
        ext_ctrl[16].value = h264_arg->PadControlOn;
        ext_ctrl[17].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_LUMA_VALUE;
        ext_ctrl[17].value = h264_arg->LumaPadVal;
        ext_ctrl[18].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CB_VALUE;
        ext_ctrl[18].value = h264_arg->CbPadVal;
        ext_ctrl[19].id = V4L2_CID_CODEC_MFC5X_ENC_PAD_CR_VALUE;
        ext_ctrl[19].value = h264_arg->CrPadVal;
        ext_ctrl[20].id = V4L2_CID_CODEC_MFC5X_ENC_RC_FRAME_ENABLE;
        ext_ctrl[20].value = h264_arg->EnableFRMRateControl;
        ext_ctrl[21].id = V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MB_ENABLE;
        ext_ctrl[21].value = h264_arg->EnableMBRateControl;
        ext_ctrl[22].id = V4L2_CID_CODEC_MFC5X_ENC_H264_RC_FRAME_RATE;
        ext_ctrl[22].value = h264_arg->FrameRate;
        ext_ctrl[23].id = V4L2_CID_CODEC_MFC5X_ENC_RC_BIT_RATE;
        /* FIXME temporary fix */
        if (h264_arg->Bitrate)
            ext_ctrl[23].value = h264_arg->Bitrate;
        else
            ext_ctrl[23].value = 1; /* just for testing Movi studio */
        ext_ctrl[24].id = V4L2_CID_CODEC_MFC5X_ENC_H264_RC_FRAME_QP;
        ext_ctrl[24].value = h264_arg->FrameQp;
        ext_ctrl[25].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_P_FRAME_QP;
        ext_ctrl[25].value = h264_arg->FrameQp_P;
        ext_ctrl[26].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_B_FRAME_QP;
        ext_ctrl[26].value = h264_arg->FrameQp_B;
        ext_ctrl[27].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MAX_QP;
        ext_ctrl[27].value = h264_arg->QSCodeMax;
        ext_ctrl[28].id = V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MIN_QP;
        ext_ctrl[28].value = h264_arg->QSCodeMin;
        ext_ctrl[29].id = V4L2_CID_CODEC_MFC5X_ENC_RC_REACTION_COEFF;
        ext_ctrl[29].value = h264_arg->CBRPeriodRf;
        ext_ctrl[30].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MB_DARK;
        ext_ctrl[30].value = h264_arg->DarkDisable;
        ext_ctrl[31].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MB_SMOOTH;
        ext_ctrl[31].value = h264_arg->SmoothDisable;
        ext_ctrl[32].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MB_STATIC;
        ext_ctrl[32].value = h264_arg->StaticDisable;
        ext_ctrl[33].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_RC_MB_ACTIVITY;
        ext_ctrl[33].value = h264_arg->ActivityDisable;

        /* doesn't have to be set */
        ext_ctrl[34].id = V4L2_CID_CODEC_MFC5X_ENC_H264_OPEN_GOP;
        ext_ctrl[34].value = V4L2_CODEC_MFC5X_ENC_SW_DISABLE;
        ext_ctrl[35].id = V4L2_CID_CODEC_MFC5X_ENC_H264_I_PERIOD;
        ext_ctrl[35].value = 10;

        if (V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_LEVEL == pCTX->enc_frameskip) {
            ext_ctrl[36].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl[36].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_LEVEL;
        } else if(V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_VBV_BUF_SIZE== pCTX->enc_frameskip) {
            ext_ctrl[36].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl[36].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_VBV_BUF_SIZE;
        } else { /* ENC_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[36].id = V4L2_CID_CODEC_MFC5X_ENC_FRAME_SKIP_MODE;
            ext_ctrl[36].value = V4L2_CODEC_MFC5X_ENC_FRAME_SKIP_MODE_DISABLE;
        }

        ext_ctrl[37].id = V4L2_CID_CODEC_MFC5X_ENC_VBV_BUF_SIZE;
        ext_ctrl[37].value = 0;

        ext_ctrl[38].id = V4L2_CID_CODEC_MFC5X_ENC_SEQ_HDR_MODE;
        ext_ctrl[38].value = 0; /* 0: seperated header
                                              1: header + first frame */

        ext_ctrl[39].id = V4L2_CID_CODEC_MFC5X_ENC_RC_FIXED_TARGET_BIT;
        ext_ctrl[39].value = V4L2_CODEC_MFC5X_ENC_SW_ENABLE;

        ext_ctrl[40].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_AR_VUI_ENABLE;
        ext_ctrl[40].value = V4L2_CODEC_MFC5X_ENC_SW_DISABLE;
        ext_ctrl[41].id = V4L2_CID_CODEC_MFC5X_ENC_H264_AR_VUI_IDC;
        ext_ctrl[41].value = 0;
        ext_ctrl[42].id = V4L2_CID_CODEC_MFC5X_ENC_H264_EXT_SAR_WIDTH;
        ext_ctrl[42].value = 0;
        ext_ctrl[43].id =  V4L2_CID_CODEC_MFC5X_ENC_H264_EXT_SAR_HEIGHT;
        ext_ctrl[43].value = 0;

        break;

    default:
        LOGE("[%s] Undefined codec type",__func__);
        ret = MFC_RET_INVALID_PARAM;
        goto error_case1;
    }

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_CODEC;
    if (pCTX->codecType == MPEG4_ENC) {
        ext_ctrls.count = 27;
        ext_ctrls.controls = ext_ctrl_mpeg4;
    } else if (pCTX->codecType == H264_ENC) {
        ext_ctrls.count = 44;
        ext_ctrls.controls = ext_ctrl;
    } else if (pCTX->codecType == H263_ENC) {
        ext_ctrls.count = 19;
        ext_ctrls.controls = ext_ctrl_h263;
    }

    ret = ioctl(pCTX->hMFC, VIDIOC_S_EXT_CTRLS, &ext_ctrls);
    if (ret != 0) {
        LOGE("[%s] Failed to set extended controls",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case1;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    fmt.fmt.pix_mp.width = pCTX->width;
    fmt.fmt.pix_mp.height = pCTX->height;
    fmt.fmt.pix_mp.num_planes = 2;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = Align(fmt.fmt.pix_mp.width, 128);
    fmt.fmt.pix_mp.plane_fmt[1].bytesperline = Align(fmt.fmt.pix_mp.width, 128);

    if (NV12_TILE == pCTX->framemap) {
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12MT; /* 4:2:0, 2 Planes, 64x32 Tiles */
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage =
            Align(Align(fmt.fmt.pix_mp.width, 128) * Align(fmt.fmt.pix_mp.height, 32), 8192); /* tiled mode */
        fmt.fmt.pix_mp.plane_fmt[1].sizeimage =
            Align(Align(fmt.fmt.pix_mp.width, 128) * Align(fmt.fmt.pix_mp.height >> 1, 32), 8192); /* tiled mode */
    } else { /* NV12_LINEAR (default) */
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M; /* 4:2:0, 2 Planes, linear */
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage =
            Align((fmt.fmt.pix_mp.width * fmt.fmt.pix_mp.height), 2048); /* linear mode, 2K align */
        fmt.fmt.pix_mp.plane_fmt[1].sizeimage =
            Align((fmt.fmt.pix_mp.width * (fmt.fmt.pix_mp.height >> 1)), 2048); /* linear mode, 2K align */
    }

    ret = ioctl(pCTX->hMFC, VIDIOC_S_FMT, &fmt);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed on MFC output stream",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case1;
    }

    /* capture (dst) */
    memset(&fmt, 0, sizeof(fmt));

    switch (pCTX->codecType) {
    case H264_ENC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
        break;
    case MPEG4_ENC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG4;
        break;
    case H263_ENC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H263;
        break;
    default:
        LOGE("[%s] Codec has not been recognised",__func__);
        return MFC_RET_ENC_INIT_FAIL;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = MAX_STREAM_SIZE;

    ret = ioctl(pCTX->hMFC, VIDIOC_S_FMT, &fmt);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed on MFC output stream",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case1;
    }

    /* cacheable buffer */
    ctrl.id = V4L2_CID_CACHEABLE;
    if (pCTX->cacheablebuffer == NO_CACHE)
        ctrl.value = 0;
    else
        ctrl.value = 1;

    ret = ioctl(pCTX->hMFC, VIDIOC_S_CTRL, &ctrl);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_S_CTRL failed, V4L2_CID_CACHEABLE",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case1;
    }

    /* Initialize streams for input */
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count  = MFC_ENC_NUM_SRC_BUFS;
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (pCTX->v4l2_enc.bInputPhyVir)
        reqbuf.memory = V4L2_MEMORY_USERPTR;
    else
        reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(pCTX->hMFC, VIDIOC_REQBUFS, &reqbuf);
    if (ret != 0) {
        LOGE("[%s] Reqbufs src ioctl failed",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case1;
    }
    pCTX->v4l2_enc.mfc_num_src_bufs  = reqbuf.count;

    if (!pCTX->v4l2_enc.bInputPhyVir) {
        /* Then the buffers have to be queried and mmaped */
        for (i = 0;  i < pCTX->v4l2_enc.mfc_num_src_bufs; ++i) {
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            buf.m.planes = planes;
            buf.length = 2;

            ret = ioctl(pCTX->hMFC, VIDIOC_QUERYBUF, &buf);
            if (ret != 0) {
                LOGE("[%s] Querybuf src ioctl failed",__func__);
                ret = MFC_RET_ENC_INIT_FAIL;
                goto error_case2;
            }

            pCTX->v4l2_enc.mfc_src_bufs_len[0] = buf.m.planes[0].length;
            pCTX->v4l2_enc.mfc_src_bufs_len[1] = buf.m.planes[1].length;

            pCTX->v4l2_enc.mfc_src_phys[i][0] = buf.m.planes[0].cookie;
            pCTX->v4l2_enc.mfc_src_phys[i][1] = buf.m.planes[1].cookie;

            pCTX->v4l2_enc.mfc_src_bufs[i][0] =
                mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                MAP_SHARED, pCTX->hMFC, buf.m.planes[0].m.mem_offset);
            if (pCTX->v4l2_enc.mfc_src_bufs[i][0] == MAP_FAILED) {
                LOGE("[%s] Mmap on src buffer (0) failed",__func__);
                ret = MFC_RET_ENC_INIT_FAIL;
                goto error_case2;
            }

            pCTX->v4l2_enc.mfc_src_bufs[i][1] =
                mmap(NULL, buf.m.planes[1].length, PROT_READ | PROT_WRITE,
                MAP_SHARED, pCTX->hMFC, buf.m.planes[1].m.mem_offset);
            if (pCTX->v4l2_enc.mfc_src_bufs[i][1] == MAP_FAILED) {
                munmap(pCTX->v4l2_enc.mfc_src_bufs[i][0], pCTX->v4l2_enc.mfc_src_bufs_len[0]);
                LOGE("[%s] Mmap on src buffer (1) failed",__func__);
                ret = MFC_RET_ENC_INIT_FAIL;
                goto error_case2;
            }
        }
    } else
        LOGV("[%s] Camera Phys src buf %d",__func__,reqbuf.count);

    for (i = 0; i<pCTX->v4l2_enc.mfc_num_src_bufs; i++)
        pCTX->v4l2_enc.mfc_src_buf_flags[i] = BUF_DEQUEUED;

    pCTX->v4l2_enc.beingUsedIndex = 0;

    pCTX->sizeFrmBuf.luma = (unsigned int)(pCTX->width * pCTX->height);
    pCTX->sizeFrmBuf.chroma = (unsigned int)((pCTX->width * pCTX->height) >> 1);
    pCTX->inter_buff_status |= MFC_USE_YUV_BUFF;

    /* Initialize stream for output */
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count  = MFC_ENC_MAX_DST_BUFS;
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(pCTX->hMFC, VIDIOC_REQBUFS, &reqbuf);
    if (ret != 0) {
        LOGE("[%s] Reqbufs dst ioctl failed",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case2;
    }

    pCTX->v4l2_enc.mfc_num_dst_bufs   = reqbuf.count;

    for (i = 0; i<MFC_ENC_MAX_DST_BUFS; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.m.planes = planes;
        buf.length = 1;

        ret = ioctl(pCTX->hMFC, VIDIOC_QUERYBUF, &buf);
        if (ret != 0) {
            LOGE("[%s] Querybuf dst ioctl failed",__func__);
            ret = MFC_RET_ENC_INIT_FAIL;
            goto error_case3;
        }

        pCTX->v4l2_enc.mfc_dst_bufs_len = buf.m.planes[0].length;
        pCTX->v4l2_enc.mfc_dst_bufs[i] =
                mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                MAP_SHARED, pCTX->hMFC, buf.m.planes[0].m.mem_offset);
        if (pCTX->v4l2_enc.mfc_dst_bufs[i] == MAP_FAILED) {
            LOGE("[%s] Mmap on dst buffer failed",__func__);
            ret = MFC_RET_ENC_INIT_FAIL;
            goto error_case3;
        }

        ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &buf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
            ret = MFC_RET_ENC_INIT_FAIL;
            goto error_case3;
        }
    }

    pCTX->sizeStrmBuf = MAX_ENCODER_OUTPUT_BUFFER_SIZE;
    pCTX->inter_buff_status |= MFC_USE_STRM_BUFF;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    ret = ioctl(pCTX->hMFC, VIDIOC_STREAMON, &type);
    if (ret != 0) {
        LOGE("[%s] V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, VIDIOC_STREAMON failed",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case3;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.planes = planes;
    buf.length = 1;

    /* note: #define POLLOUT 0x0004 */
    poll_events.fd = pCTX->hMFC;
    poll_events.events = POLLIN | POLLERR;
    poll_events.revents = 0;

    /* wait for header encoding */
    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, POLL_ENC_WAIT_TIMEOUT);
        if (0 < poll_state) {
            if (poll_events.revents & POLLIN) { /* POLLIN */
                ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &buf);
                if (ret == 0)
                    break;
            } else if(poll_events.revents & POLLERR) { /*POLLERR */
                LOGE("[%s] POLLERR\n",__func__);
                ret = MFC_RET_ENC_INIT_FAIL;
                goto error_case3;
            } else {
                LOGE("[%s] poll() returns 0x%x\n",__func__, poll_events.revents);
                ret = MFC_RET_ENC_INIT_FAIL;
                goto error_case3;
            }
        } else if(0 > poll_state) {
            ret = MFC_RET_ENC_INIT_FAIL;
            goto error_case3;
        }
    } while (0 == poll_state);

    pCTX->v4l2_enc.mfc_dst_bufs_bytes_used_len = buf.m.planes[0].bytesused;
    pCTX->virStrmBuf = pCTX->v4l2_enc.mfc_dst_bufs[buf.index];

    /* stream dequeued index */
    index = buf.index;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    buf.m.planes = planes;
    buf.length = 1;

    ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &buf);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
        ret = MFC_RET_ENC_INIT_FAIL;
        goto error_case3;
    }
    LOGV("[%s] Strm out idx %d",__func__,index);

    return MFC_RET_OK;
error_case3:
    for (j = 0; j < i; j++)
        munmap(pCTX->v4l2_enc.mfc_dst_bufs[j], pCTX->v4l2_enc.mfc_dst_bufs_len);

    i = pCTX->v4l2_enc.mfc_num_src_bufs;
error_case2:
    if (!pCTX->v4l2_enc.bInputPhyVir) {
        for (j = 0; j < i; j++) {
            munmap(pCTX->v4l2_enc.mfc_src_bufs[j][0], pCTX->v4l2_enc.mfc_src_bufs_len[0]);
            munmap(pCTX->v4l2_enc.mfc_src_bufs[j][1], pCTX->v4l2_enc.mfc_src_bufs_len[1]);
        }
    }
error_case1:
    return ret;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetInBuf(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info)
{
    _MFCLIB *pCTX;
    int i;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    if (pCTX->v4l2_enc.bInputPhyVir) {
        input_info->YPhyAddr = (void*)0;
        input_info->CPhyAddr = (void*)0;
        input_info->YVirAddr = (void*)0;
        input_info->CVirAddr = (void*)0;

        if (NV12_TILE == pCTX->framemap) {
            /* 4:2:0, 2 Planes, 64x32 Tiles */
            input_info->YSize = Align(Align(pCTX->width, 128) * Align(pCTX->height, 32), 8192); /* tiled mode */
            input_info->CSize = Align(Align(pCTX->width, 128) * Align(pCTX->height >> 1, 32), 8192); /* tiled mode */
        } else { /* NV12_LINEAR (default) */
            /* 4:2:0, 2 Planes, linear */
            input_info->YSize = Align(Align(pCTX->width, 16) * Align(pCTX->height, 16), 2048); /* width = 16B, height = 16B align */
            input_info->CSize = Align(Align(pCTX->width, 16) * Align(pCTX->height >> 1, 8), 2048); /* width = 16B, height = 8B align */
        }
    } else {
        for (i = 0; i < pCTX->v4l2_enc.mfc_num_src_bufs; i++)
            if (BUF_DEQUEUED == pCTX->v4l2_enc.mfc_src_buf_flags[i])
                break;

        if (i == pCTX->v4l2_enc.mfc_num_src_bufs) {
            LOGV("[%s] No buffer is available.",__func__);
            return MFC_RET_ENC_GET_INBUF_FAIL;
        } else {
            /* FIXME check this for correct physical address */
            input_info->YPhyAddr = (void*)pCTX->v4l2_enc.mfc_src_phys[i][0];
            input_info->CPhyAddr = (void*)pCTX->v4l2_enc.mfc_src_phys[i][1];
            input_info->YVirAddr = (void*)pCTX->v4l2_enc.mfc_src_bufs[i][0];
            input_info->CVirAddr = (void*)pCTX->v4l2_enc.mfc_src_bufs[i][1];
            input_info->YSize = (int)pCTX->v4l2_enc.mfc_src_bufs_len[0];
            input_info->CSize = (int)pCTX->v4l2_enc.mfc_src_bufs_len[1];

            pCTX->v4l2_enc.mfc_src_buf_flags[i] = BUF_ENQUEUED;
        }
    }
    LOGV("[%s] Input Buffer idx %d",__func__,i);
    return MFC_RET_OK;
}


SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetInBuf(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info)
{
    _MFCLIB *pCTX;
    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_ENC_NUM_PLANES];
    int ret,i;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    memset(&qbuf, 0, sizeof(qbuf));
    if (pCTX->v4l2_enc.bInputPhyVir) {
        qbuf.memory = V4L2_MEMORY_USERPTR;
        qbuf.index = pCTX->v4l2_enc.beingUsedIndex;
        planes[0].m.userptr = (unsigned long)input_info->YPhyAddr;
        planes[0].length = input_info->YSize;
        planes[0].bytesused = input_info->YSize;
        planes[1].m.userptr = (unsigned long)input_info->CPhyAddr;
        planes[1].length = input_info->CSize;
        planes[1].bytesused = input_info->CSize;

        /* FIXME, this is only for case of not using B frame,
        Camera side should know which buffer is queued() refering to index of
        MFC dqbuf() */
        pCTX->v4l2_enc.beingUsedIndex++;
        pCTX->v4l2_enc.beingUsedIndex %= MFC_ENC_NUM_SRC_BUFS;
        LOGV("[%s] Phy Input Buffer idx Queued %d",__func__,pCTX->v4l2_enc.beingUsedIndex);
    } else {
        for (i = 0; i < pCTX->v4l2_enc.mfc_num_src_bufs; i++)
            if (pCTX->v4l2_enc.mfc_src_bufs[i][0] == input_info->YVirAddr)
                break;

        if (i == pCTX->v4l2_enc.mfc_num_src_bufs) {
            LOGE("[%s] Can not use the buffer",__func__);
            return MFC_RET_INVALID_PARAM;
        } else {
            pCTX->v4l2_enc.beingUsedIndex = i;
            //pCTX->v4l2_enc.mfc_src_buf_flags[i] = BUF_ENQUEUED;
        }
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = pCTX->v4l2_enc.beingUsedIndex;
        planes[0].bytesused = pCTX->width * pCTX->height;
        planes[1].bytesused = (pCTX->width * pCTX->height) >> 1;
        LOGV("[%s] Input Buffer idx Queued %d",__func__,pCTX->v4l2_enc.beingUsedIndex);
    }

    qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    qbuf.m.planes = planes;
    qbuf.length = 2;

    ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
        return MFC_RET_ENC_SET_INBUF_FAIL;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetOutBuf(void *openHandle, SSBSIP_MFC_ENC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *) openHandle;

    if (pCTX->v4l2_enc.bRunning == 0) {
        pCTX->encodedHeaderSize = pCTX->v4l2_enc.mfc_dst_bufs_bytes_used_len;
        output_info->dataSize = 0;
    } else {
        output_info->dataSize = pCTX->v4l2_enc.mfc_dst_bufs_bytes_used_len;
    }

    output_info->headerSize = pCTX->encodedHeaderSize;
    output_info->frameType = pCTX->encodedframeType;
    output_info->StrmPhyAddr = (void *)0;
    output_info->StrmVirAddr = (void *)pCTX->virStrmBuf;
    output_info->encodedYPhyAddr = (void*)0;
    output_info->encodedCPhyAddr = (void*)0;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetOutBuf(void *openHandle, void *phyOutbuf, void *virOutbuf, int outputBufferSize)
{
    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_ENC_SET_OUTBUF_FAIL;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncExe(void *openHandle)
{
    int ret;
    int dequeued_index;
    int loopcnt = 0;
    _MFCLIB *pCTX;

    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_ENC_NUM_PLANES];
    enum v4l2_buf_type type;

    struct v4l2_control ctrl;

    struct pollfd poll_events;
    int poll_state;

    LOGV("[%s] Enter \n",__func__);
    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *) openHandle;

    ctrl.id = V4L2_CID_CODEC_FRAME_TAG;
    ctrl.value = pCTX->inframetag;

    ret = ioctl(pCTX->hMFC, VIDIOC_S_CTRL, &ctrl);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_S_CTRL failed, V4L2_CID_CODEC_FRAME_TAG",__func__);
        return MFC_RET_ENC_EXE_ERR;
    }

    if (pCTX->v4l2_enc.bRunning == 0) {
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        ret = ioctl(pCTX->hMFC, VIDIOC_STREAMON, &type);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMON failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_ENC_EXE_ERR;
        }

        pCTX->v4l2_enc.bRunning = 1;
    }

    memset(&qbuf, 0, sizeof(qbuf));
    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    qbuf.memory = V4L2_MEMORY_MMAP;
    qbuf.m.planes = planes;
    qbuf.length = 1;

    /* note: #define POLLOUT 0x0004 */
    poll_events.fd = pCTX->hMFC;
    poll_events.events = POLLIN | POLLERR;
    poll_events.revents = 0;

    /* wait for encoding */
    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, POLL_ENC_WAIT_TIMEOUT);
        if (0 < poll_state) {
            if (poll_events.revents & POLLIN) { /* POLLIN */
                ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
                if (ret == 0)
                    break;
            } else if (poll_events.revents & POLLERR) { /* POLLERR */
                LOGE("[%s] POLLERR\n",__func__);
                return MFC_RET_ENC_EXE_ERR;
            } else {
                LOGE("[%s] poll() returns 0x%x\n",__func__, poll_events.revents);
                return MFC_RET_ENC_EXE_ERR;
            }
        } else if (0 > poll_state) {
            LOGE("[%s] poll() Encoder POLL Timeout 0x%x\n",__func__, poll_events.revents);
            return MFC_RET_ENC_EXE_ERR;
        }
        loopcnt++;
    } while ((0 == poll_state) && (loopcnt < 5));

    if (pCTX->v4l2_enc.bRunning != 0) {
        pCTX->encodedframeType = (qbuf.flags & 0x18) >> 3; /* encoded frame type */

        LOGV("[%s] encoded frame type = %d\n",__func__, pCTX->encodedframeType);
        switch (pCTX->encodedframeType) {
        case 1:
            pCTX->encodedframeType = MFC_FRAME_TYPE_I_FRAME;
            break;
        case 2:
            pCTX->encodedframeType = MFC_FRAME_TYPE_P_FRAME;
            break;
        case 4:
            pCTX->encodedframeType = MFC_FRAME_TYPE_B_FRAME;
            break;
        default:
             LOGE("[%s] VIDIOC_DQBUF failed, encoded frame type is wrong",__func__);
        }
    }

    dequeued_index = qbuf.index;

    if (qbuf.m.planes[0].bytesused > 0) { /* FIXME later */
        pCTX->v4l2_enc.mfc_dst_bufs_bytes_used_len = qbuf.m.planes[0].bytesused;
    }

    ctrl.id = V4L2_CID_CODEC_FRAME_TAG;
    ctrl.value = 0;

    ret = ioctl(pCTX->hMFC, VIDIOC_G_CTRL, &ctrl);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_CTRL failed, V4L2_CID_CODEC_FRAME_TAG",__func__);
        return MFC_RET_ENC_EXE_ERR;
    }

    pCTX->outframetagtop = ctrl.value;

    memset(&qbuf, 0, sizeof(qbuf));
    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    qbuf.memory = V4L2_MEMORY_MMAP;
    qbuf.index = dequeued_index;
    qbuf.m.planes = planes;
    qbuf.length = 1;

    ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
        return MFC_RET_ENC_EXE_ERR;
    }

    if (pCTX->v4l2_enc.bRunning != 0) {
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

        if (pCTX->v4l2_enc.bInputPhyVir)
            qbuf.memory = V4L2_MEMORY_USERPTR;
        else
            qbuf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_DQBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_ENC_EXE_ERR;
        }
    }
    pCTX->v4l2_enc.mfc_src_buf_flags[qbuf.index] = BUF_DEQUEUED;

    /* Update context stream buffer address */
    pCTX->virStrmBuf = pCTX->v4l2_enc.mfc_dst_bufs[dequeued_index];
    LOGV("[%s] Strm out idx %d",__func__,dequeued_index);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetConfig(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("[%s] value is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    switch (conf_type) {
    case MFC_ENC_SETCONF_FRAME_TAG:
        pCTX->inframetag = *((unsigned int *)value);
        return MFC_RET_OK;

    case MFC_ENC_SETCONF_ALLOW_FRAME_SKIP:
        pCTX->enc_frameskip = *((int *)value);
        return MFC_RET_OK;
#if 0
    case MFC_ENC_SETCONF_VUI_INFO:
        vui_info = *((struct mfc_enc_vui_info *) value);
        EncArg.args.set_config.in_config_value[0]  = (int)(vui_info.aspect_ratio_idc);
        EncArg.args.set_config.in_config_value[1]  = 0;
        break;

    case MFC_ENC_SETCONF_HIER_P:
        hier_p_qp = *((struct mfc_enc_hier_p_qp *) value);
        EncArg.args.set_config.in_config_value[0]  = (int)(hier_p_qp.t0_frame_qp);
        EncArg.args.set_config.in_config_value[1]  = (int)(hier_p_qp.t2_frame_qp);
        EncArg.args.set_config.in_config_value[2]  = (int)(hier_p_qp.t3_frame_qp);
        break;

    case MFC_ENC_SETCONF_I_PERIOD:
#endif
    default:
        LOGE("[%s] conf_type(%d) is NOT supported\n",__func__, conf_type);
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetConfig(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value)
{
    _MFCLIB *pCTX;

    pCTX = (_MFCLIB *) openHandle;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("[%s] value is NULL\n",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    switch (conf_type) {
    case MFC_ENC_GETCONF_FRAME_TAG:
        *((unsigned int *)value) = pCTX->outframetagtop;
        break;

    default:
        LOGE("[%s] conf_type(%d) is NOT supported\n",__func__, conf_type);
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_OK;
}

