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
#include <math.h>

#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

#include <utils/Log.h>
/* #define LOG_NDEBUG 0 */
#undef  LOG_TAG
#define LOG_TAG "MFC_ENC_APP"

#define _MFCLIB_MAGIC_NUMBER	0x92241001

static char *mfc_dev_name = SAMSUNG_MFC_DEV_NAME;

void SsbSipMfcEncSetMFCName(char *devicename)
{
    mfc_dev_name = devicename;
}

void *SsbSipMfcEncOpen(void)
{
    int hMFCOpen;
    _MFCLIB *pCTX = NULL;
    unsigned int mapped_addr;
    int mapped_size;
    struct mfc_common_args CommonArg;

    LOGI("[%s] MFC Library Ver %d.%02d",__func__, MFC_LIB_VER_MAJOR, MFC_LIB_VER_MINOR);

#if 0
    if ((codecType != MPEG4_ENC) &&
        (codecType != H264_ENC) &&
        (codecType != H263_ENC)) {
        LOGE("SsbSipMfcEncOpen] Undefined codec type");
        return NULL;
    }
#endif

    if (access(mfc_dev_name, F_OK) != 0) {
        LOGE("SsbSipMfcEncOpen] MFC device node not exists");
        return NULL;
    }

    hMFCOpen = open(mfc_dev_name, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        LOGE("SsbSipMfcEncOpen] MFC Open failure");
        return NULL;
    }

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("SsbSipMfcEncOpen] malloc failed.");
        close(hMFCOpen);
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    mapped_size = ioctl(hMFCOpen, IOCTL_MFC_GET_MMAP_SIZE, &CommonArg);
    if ((mapped_size < 0) || (CommonArg.ret_code != MFC_OK)) {
        LOGE("SsbSipMfcEncOpen] IOCTL_MFC_GET_MMAP_SIZE failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    mapped_addr = (unsigned int)mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        LOGE("SsbSipMfcEncOpen] FIMV5.x driver address mapping failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    pCTX->magic = _MFCLIB_MAGIC_NUMBER;
    pCTX->hMFC = hMFCOpen;
    pCTX->mapped_addr = mapped_addr;
    pCTX->mapped_size = mapped_size;
    pCTX->inter_buff_status = MFC_USE_NONE;

    return (void *) pCTX;
}


void *SsbSipMfcEncOpenExt(void *value)
{
    int hMFCOpen;
    _MFCLIB *pCTX = NULL;
    unsigned int mapped_addr;
    int mapped_size;
    int err;
    struct mfc_common_args CommonArg;

    LOGI("[%s] MFC Library Ver %d.%02d",__func__, MFC_LIB_VER_MAJOR, MFC_LIB_VER_MINOR);

#if 0
    if ((codecType != MPEG4_ENC) &&
        (codecType != H264_ENC) &&
        (codecType != H263_ENC)) {
        LOGE("SsbSipMfcEncOpen] Undefined codec type");
        return NULL;
    }
#endif

    if (access(mfc_dev_name, F_OK) != 0) {
        LOGE("SsbSipMfcEncOpenExt] MFC device node not exists");
        return NULL;
    }

    hMFCOpen = open(mfc_dev_name, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        LOGE("SsbSipMfcEncOpenExt] MFC Open failure");
        return NULL;
    }

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("SsbSipMfcEncOpenExt] malloc failed.");
        close(hMFCOpen);
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    CommonArg.args.mem_alloc.buf_cache_type = *(SSBIP_MFC_BUFFER_TYPE *)value;

    err = ioctl(hMFCOpen, IOCTL_MFC_SET_BUF_CACHE, &CommonArg);
    if ((err < 0) || (CommonArg.ret_code != MFC_OK)) {
        LOGE("SsbSipMfcEncOpenExt] IOCTL_MFC_SET_BUF_CACHE failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    mapped_size = ioctl(hMFCOpen, IOCTL_MFC_GET_MMAP_SIZE, &CommonArg);
    if ((mapped_size < 0) || (CommonArg.ret_code != MFC_OK)) {
        LOGE("SsbSipMfcEncOpenExt] IOCTL_MFC_GET_MMAP_SIZE failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    mapped_addr = (unsigned int)mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        LOGE("SsbSipMfcEncOpenExt] FIMV5.x driver address mapping failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    pCTX->magic = _MFCLIB_MAGIC_NUMBER;
    pCTX->hMFC = hMFCOpen;
    pCTX->mapped_addr = mapped_addr;
    pCTX->mapped_size = mapped_size;
    pCTX->inter_buff_status = MFC_USE_NONE;

    return (void *) pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncInit(void *openHandle, void *param)
{
    int ret_code;

    _MFCLIB *pCTX;
    struct mfc_common_args EncArg;
    SSBSIP_MFC_ENC_H264_PARAM *h264_arg;
    SSBSIP_MFC_ENC_MPEG4_PARAM *mpeg4_arg;
    SSBSIP_MFC_ENC_H263_PARAM *h263_arg;

    pCTX  = (_MFCLIB *) openHandle;
    memset(&EncArg, 0, sizeof(struct mfc_common_args));

    pCTX->encode_cnt = 0;

    mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM*)param;
    if (mpeg4_arg->codecType == MPEG4_ENC) {
        pCTX->codecType= MPEG4_ENC;
    } else {
        h263_arg = (SSBSIP_MFC_ENC_H263_PARAM*)param;
        if (h263_arg->codecType == H263_ENC) {
            pCTX->codecType = H263_ENC;
        } else {
            h264_arg = (SSBSIP_MFC_ENC_H264_PARAM*)param;
            if (h264_arg->codecType == H264_ENC) {
                pCTX->codecType = H264_ENC;
            } else {
                LOGE("SsbSipMfcEncInit] Undefined codec type");
                return MFC_RET_INVALID_PARAM;
            }
        }
    }

    LOGI("SsbSipMfcEncInit] Encode Init start");

    switch (pCTX->codecType) {
    case MPEG4_ENC:
        LOGI("SsbSipMfcEncInit] MPEG4 Encode");
        mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM *)param;

        pCTX->width = mpeg4_arg->SourceWidth;
        pCTX->height = mpeg4_arg->SourceHeight;
        break;

    case H263_ENC:
        LOGI("SsbSipMfcEncInit] H263 Encode");
        h263_arg = (SSBSIP_MFC_ENC_H263_PARAM *)param;

        pCTX->width = h263_arg->SourceWidth;
        pCTX->height = h263_arg->SourceHeight;
        break;

    case H264_ENC:
        LOGI("SsbSipMfcEncInit] H264 Encode");
        h264_arg = (SSBSIP_MFC_ENC_H264_PARAM *)param;

        pCTX->width = h264_arg->SourceWidth;
        pCTX->height = h264_arg->SourceHeight;
        break;

    default:
        break;
    }

    switch (pCTX->codecType) {
    case MPEG4_ENC:
        mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM *)param;

        EncArg.args.enc_init.cmn.in_codec_type = pCTX->codecType;

        EncArg.args.enc_init.cmn.in_width = mpeg4_arg->SourceWidth;
        EncArg.args.enc_init.cmn.in_height = mpeg4_arg->SourceHeight;
        EncArg.args.enc_init.cmn.in_gop_num = mpeg4_arg->IDRPeriod;

        EncArg.args.enc_init.cmn.in_ms_mode = mpeg4_arg->SliceMode;
        EncArg.args.enc_init.cmn.in_ms_arg = mpeg4_arg->SliceArgument;
        EncArg.args.enc_init.cmn.in_output_mode = mpeg4_arg->OutputMode;

        EncArg.args.enc_init.cmn.in_mb_refresh = mpeg4_arg->RandomIntraMBRefresh;

        /* rate control*/
        EncArg.args.enc_init.cmn.in_rc_fr_en = mpeg4_arg->EnableFRMRateControl;
        if ((mpeg4_arg->QSCodeMin > 31) || (mpeg4_arg->QSCodeMax > 31)) {
            LOGE("SsbSipMfcEncInit] No such Min/Max QP is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_rc_qbound_min = mpeg4_arg->QSCodeMin;
        EncArg.args.enc_init.cmn.in_rc_qbound_max = mpeg4_arg->QSCodeMax;
        EncArg.args.enc_init.cmn.in_rc_rpara = mpeg4_arg->CBRPeriodRf;

        /* pad control */
        EncArg.args.enc_init.cmn.in_pad_ctrl_on = mpeg4_arg->PadControlOn;
        if ((mpeg4_arg->LumaPadVal > 255) || (mpeg4_arg->CbPadVal > 255) || (mpeg4_arg->CrPadVal > 255)) {
            LOGE("SsbSipMfcEncInit] No such Pad value is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_y_pad_val = mpeg4_arg->LumaPadVal;
        EncArg.args.enc_init.cmn.in_cb_pad_val = mpeg4_arg->CbPadVal;
        EncArg.args.enc_init.cmn.in_cr_pad_val = mpeg4_arg->CrPadVal;

        /* Input stream Mode  NV12_Linear or NV12_Tile*/
        EncArg.args.enc_init.cmn.in_frame_map = mpeg4_arg->FrameMap;

        EncArg.args.enc_init.cmn.in_rc_bitrate = mpeg4_arg->Bitrate;
        if ((mpeg4_arg->FrameQp > 31) || (mpeg4_arg->FrameQp_P > 31)) {
            LOGE("SsbSipMfcEncInit] No such FrameQp is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_vop_quant = mpeg4_arg->FrameQp;
        EncArg.args.enc_init.cmn.in_vop_quant_p = mpeg4_arg->FrameQp_P;

        /* MPEG4 only */
        EncArg.args.enc_init.codec.mpeg4.in_profile = mpeg4_arg->ProfileIDC;
        EncArg.args.enc_init.codec.mpeg4.in_level = mpeg4_arg->LevelIDC;

        if (mpeg4_arg->FrameQp_B > 31) {
            LOGE("SsbSipMfcEncInit] No such FrameQp is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.codec.mpeg4.in_vop_quant_b = mpeg4_arg->FrameQp_B;

        if (mpeg4_arg->NumberBFrames > 2) {
            LOGE("SsbSipMfcEncInit] No such BframeNum is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.codec.mpeg4.in_bframenum = mpeg4_arg->NumberBFrames;

        EncArg.args.enc_init.codec.mpeg4.in_quart_pixel = mpeg4_arg->DisableQpelME;

        EncArg.args.enc_init.codec.mpeg4.in_TimeIncreamentRes = mpeg4_arg->TimeIncreamentRes;
        EncArg.args.enc_init.codec.mpeg4.in_VopTimeIncreament = mpeg4_arg->VopTimeIncreament;

        break;

    case H263_ENC:
        h263_arg = (SSBSIP_MFC_ENC_H263_PARAM *)param;

        EncArg.args.enc_init.cmn.in_codec_type = pCTX->codecType;

        EncArg.args.enc_init.cmn.in_width = h263_arg->SourceWidth;
        EncArg.args.enc_init.cmn.in_height = h263_arg->SourceHeight;
        EncArg.args.enc_init.cmn.in_gop_num = h263_arg->IDRPeriod;

        EncArg.args.enc_init.cmn.in_ms_mode = h263_arg->SliceMode;
        EncArg.args.enc_init.cmn.in_ms_arg = 0;
        EncArg.args.enc_init.cmn.in_output_mode = FRAME;  /* not support to slice output mode */

        EncArg.args.enc_init.cmn.in_mb_refresh = h263_arg->RandomIntraMBRefresh;

        /* rate control*/
        EncArg.args.enc_init.cmn.in_rc_fr_en = h263_arg->EnableFRMRateControl;
        if ((h263_arg->QSCodeMin > 31) || (h263_arg->QSCodeMax > 31)) {
            LOGE("SsbSipMfcEncInit] No such Min/Max QP is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_rc_qbound_min = h263_arg->QSCodeMin;
        EncArg.args.enc_init.cmn.in_rc_qbound_max = h263_arg->QSCodeMax;
        EncArg.args.enc_init.cmn.in_rc_rpara = h263_arg->CBRPeriodRf;

        /* pad control */
        EncArg.args.enc_init.cmn.in_pad_ctrl_on = h263_arg->PadControlOn;
        if ((h263_arg->LumaPadVal > 255) || (h263_arg->CbPadVal > 255) || (h263_arg->CrPadVal > 255)) {
            LOGE("SsbSipMfcEncInit] No such Pad value is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_y_pad_val = h263_arg->LumaPadVal;
        EncArg.args.enc_init.cmn.in_cb_pad_val = h263_arg->CbPadVal;
        EncArg.args.enc_init.cmn.in_cr_pad_val = h263_arg->CrPadVal;

        /* Input stream Mode  NV12_Linear or NV12_Tile*/
        EncArg.args.enc_init.cmn.in_frame_map = h263_arg->FrameMap;

        EncArg.args.enc_init.cmn.in_rc_bitrate = h263_arg->Bitrate;
        if ((h263_arg->FrameQp > 31) || (h263_arg->FrameQp_P > 31)) {
            LOGE("SsbSipMfcEncInit] No such FrameQp is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_vop_quant = h263_arg->FrameQp;
        EncArg.args.enc_init.cmn.in_vop_quant_p = h263_arg->FrameQp_P;

        /* H.263 only */
        EncArg.args.enc_init.codec.h263.in_rc_framerate = h263_arg->FrameRate;

        break;

    case H264_ENC:
        h264_arg = (SSBSIP_MFC_ENC_H264_PARAM *)param;

        EncArg.args.enc_init.cmn.in_codec_type = H264_ENC;

        EncArg.args.enc_init.cmn.in_width = h264_arg->SourceWidth;
        EncArg.args.enc_init.cmn.in_height = h264_arg->SourceHeight;
        EncArg.args.enc_init.cmn.in_gop_num = h264_arg->IDRPeriod;

        if ((h264_arg->SliceMode == 0)||(h264_arg->SliceMode == 1)||
            (h264_arg->SliceMode == 2)||(h264_arg->SliceMode == 4)) {
            EncArg.args.enc_init.cmn.in_ms_mode = h264_arg->SliceMode;
        } else {
            LOGE("SsbSipMfcEncInit] No such slice mode is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_ms_arg = h264_arg->SliceArgument;
        EncArg.args.enc_init.cmn.in_output_mode = h264_arg->OutputMode;

        EncArg.args.enc_init.cmn.in_mb_refresh = h264_arg->RandomIntraMBRefresh;
        /* pad control */
        EncArg.args.enc_init.cmn.in_pad_ctrl_on = h264_arg->PadControlOn;
        if ((h264_arg->LumaPadVal > 255) || (h264_arg->CbPadVal > 255) || (h264_arg->CrPadVal > 255)) {
            LOGE("SsbSipMfcEncInit] No such Pad value is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_y_pad_val = h264_arg->LumaPadVal;
        EncArg.args.enc_init.cmn.in_cb_pad_val = h264_arg->CbPadVal;
        EncArg.args.enc_init.cmn.in_cr_pad_val = h264_arg->CrPadVal;

        /* Input stream Mode  NV12_Linear or NV12_Tile*/
        EncArg.args.enc_init.cmn.in_frame_map = h264_arg->FrameMap;

        /* rate control*/
        EncArg.args.enc_init.cmn.in_rc_fr_en = h264_arg->EnableFRMRateControl;
        EncArg.args.enc_init.cmn.in_rc_bitrate = h264_arg->Bitrate;
        if ((h264_arg->FrameQp > 51) || (h264_arg->FrameQp_P > 51)) {
            LOGE("SsbSipMfcEncInit] No such FrameQp is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_vop_quant = h264_arg->FrameQp;
        EncArg.args.enc_init.cmn.in_vop_quant_p = h264_arg->FrameQp_P;

        if ((h264_arg->QSCodeMin > 51) || (h264_arg->QSCodeMax > 51)) {
            LOGE("SsbSipMfcEncInit] No such Min/Max QP is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.cmn.in_rc_qbound_min = h264_arg->QSCodeMin;
        EncArg.args.enc_init.cmn.in_rc_qbound_max = h264_arg->QSCodeMax;
        EncArg.args.enc_init.cmn.in_rc_rpara = h264_arg->CBRPeriodRf;


        /* H.264 Only */
        EncArg.args.enc_init.codec.h264.in_profile = h264_arg->ProfileIDC;
        EncArg.args.enc_init.codec.h264.in_level = h264_arg->LevelIDC;

        if (h264_arg->FrameQp_B > 51) {
            LOGE("SsbSipMfcEncInit] No such FrameQp is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.codec.h264.in_vop_quant_b = h264_arg->FrameQp_B;

        if (h264_arg->NumberBFrames > 2) {
            LOGE("SsbSipMfcEncInit] No such BframeNum is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.codec.h264.in_bframenum = h264_arg->NumberBFrames;

        EncArg.args.enc_init.codec.h264.in_interlace_mode = h264_arg->PictureInterlace;

        if ((h264_arg->NumberRefForPframes > 2)||(h264_arg->NumberReferenceFrames >2)) {
            LOGE("SsbSipMfcEncInit] No such ref Num is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.codec.h264.in_reference_num = h264_arg->NumberReferenceFrames;
        EncArg.args.enc_init.codec.h264.in_ref_num_p = h264_arg->NumberRefForPframes;

        EncArg.args.enc_init.codec.h264.in_rc_framerate = h264_arg->FrameRate;

        EncArg.args.enc_init.codec.h264.in_rc_mb_en = h264_arg->EnableMBRateControl;
        EncArg.args.enc_init.codec.h264.in_rc_mb_dark_dis = h264_arg->DarkDisable;
        EncArg.args.enc_init.codec.h264.in_rc_mb_smooth_dis = h264_arg->SmoothDisable;
        EncArg.args.enc_init.codec.h264.in_rc_mb_static_dis = h264_arg->StaticDisable;
        EncArg.args.enc_init.codec.h264.in_rc_mb_activity_dis = h264_arg->ActivityDisable;

        EncArg.args.enc_init.codec.h264.in_deblock_dis = h264_arg->LoopFilterDisable;
        if ((abs(h264_arg->LoopFilterAlphaC0Offset) > 6) || (abs(h264_arg->LoopFilterBetaOffset) > 6)) {
            LOGE("SsbSipMfcEncInit] No such AlphaC0Offset or BetaOffset is supported");
            return MFC_RET_INVALID_PARAM;
        }
        EncArg.args.enc_init.codec.h264.in_deblock_alpha_c0 = h264_arg->LoopFilterAlphaC0Offset;
        EncArg.args.enc_init.codec.h264.in_deblock_beta = h264_arg->LoopFilterBetaOffset;

        EncArg.args.enc_init.codec.h264.in_symbolmode = h264_arg->SymbolMode;
        EncArg.args.enc_init.codec.h264.in_transform8x8_mode = h264_arg->Transform8x8Mode;

        /* FIXME: is it removed? */
        EncArg.args.enc_init.codec.h264.in_md_interweight_pps = 300;
        EncArg.args.enc_init.codec.h264.in_md_intraweight_pps = 170;

        break;

    default:
        LOGE("SsbSipMfcEncInit] No such codec type is supported");
        return MFC_RET_INVALID_PARAM;
    }

    EncArg.args.enc_init.cmn.in_mapped_addr = pCTX->mapped_addr;

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_ENC_INIT, &EncArg);
    if (EncArg.ret_code != MFC_OK) {
        LOGE("SsbSipMfcEncInit] IOCTL_MFC_ENC_INIT failed");
        return MFC_RET_ENC_INIT_FAIL;
    }

    pCTX->virStrmBuf = EncArg.args.enc_init.cmn.out_u_addr.strm_ref_y;
    pCTX->phyStrmBuf = EncArg.args.enc_init.cmn.out_p_addr.strm_ref_y;

    pCTX->sizeStrmBuf = MAX_ENCODER_OUTPUT_BUFFER_SIZE;
    pCTX->encodedHeaderSize = EncArg.args.enc_init.cmn.out_header_size;

    pCTX->virMvRefYC = EncArg.args.enc_init.cmn.out_u_addr.mv_ref_yc;

    pCTX->inter_buff_status |= MFC_USE_STRM_BUFF;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncExe(void *openHandle)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args EncArg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncExe] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    memset(&EncArg, 0x00, sizeof(struct mfc_common_args));

    EncArg.args.enc_exe.in_codec_type = pCTX->codecType;
    EncArg.args.enc_exe.in_Y_addr    = (unsigned int)pCTX->phyFrmBuf.luma;
    EncArg.args.enc_exe.in_CbCr_addr = (unsigned int)pCTX->phyFrmBuf.chroma;
#if 0 /* peter for debug */
    EncArg.args.enc_exe.in_Y_addr_vir    = (unsigned int)pCTX->virFrmBuf.luma;
    EncArg.args.enc_exe.in_CbCr_addr_vir = (unsigned int)pCTX->virFrmBuf.chroma;
#endif
    EncArg.args.enc_exe.in_frametag = pCTX->inframetag;
    if (pCTX->encode_cnt == 0) {
        EncArg.args.enc_exe.in_strm_st   = (unsigned int)pCTX->phyStrmBuf;
        EncArg.args.enc_exe.in_strm_end  = (unsigned int)pCTX->phyStrmBuf + pCTX->sizeStrmBuf;
    } else {
        EncArg.args.enc_exe.in_strm_st = (unsigned int)pCTX->phyStrmBuf + (MAX_ENCODER_OUTPUT_BUFFER_SIZE / 2);
        EncArg.args.enc_exe.in_strm_end = (unsigned int)pCTX->phyStrmBuf + pCTX->sizeStrmBuf + (MAX_ENCODER_OUTPUT_BUFFER_SIZE / 2);
    }

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_ENC_EXE, &EncArg);
    if (EncArg.ret_code != MFC_OK) {
        LOGE("SsbSipMfcEncExe] IOCTL_MFC_ENC_EXE failed(ret : %d)", EncArg.ret_code);
        return MFC_RET_ENC_EXE_ERR;
    }

    pCTX->encodedDataSize = EncArg.args.enc_exe.out_encoded_size;
    pCTX->encodedframeType = EncArg.args.enc_exe.out_frame_type;
    pCTX->encodedphyFrmBuf.luma = EncArg.args.enc_exe.out_Y_addr;
    pCTX->encodedphyFrmBuf.chroma = EncArg.args.enc_exe.out_CbCr_addr;
    pCTX->outframetagtop = EncArg.args.enc_exe.out_frametag_top;
    pCTX->outframetagbottom = EncArg.args.enc_exe.out_frametag_bottom;

    LOGV("SsbSipMfcEncExe] Encode success ==================");

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncClose(void *openHandle)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args free_arg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncClose] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    /* FIXME: free buffer? */
    if (pCTX->inter_buff_status & MFC_USE_YUV_BUFF) {
        free_arg.args.mem_free.key = pCTX->virFrmBuf.luma;
        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }

    if (pCTX->inter_buff_status & MFC_USE_STRM_BUFF) {
        free_arg.args.mem_free.key = pCTX->virStrmBuf;
        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
        free_arg.args.mem_free.key = pCTX->virMvRefYC;
        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }

    pCTX->inter_buff_status = MFC_USE_NONE;

    munmap((void *)pCTX->mapped_addr, pCTX->mapped_size);

    close(pCTX->hMFC);

    free(pCTX);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetInBuf(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args user_addr_arg, real_addr_arg;
    int y_size, c_size;
    int aligned_y_size, aligned_c_size;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncGetInBuf] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    /* FIXME: */
    y_size = pCTX->width * pCTX->height;
    c_size = (pCTX->width * pCTX->height) >> 1;

    /* lenear: 2KB, tile: 8KB */
    aligned_y_size = Align(y_size, 64 * BUF_L_UNIT);
    aligned_c_size = Align(c_size, 64 * BUF_L_UNIT);

    /* Allocate luma & chroma buf */
    user_addr_arg.args.mem_alloc.type = ENCODER;
    user_addr_arg.args.mem_alloc.buff_size = aligned_y_size + aligned_c_size;
    user_addr_arg.args.mem_alloc.mapped_addr = pCTX->mapped_addr;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_IN_BUF, &user_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcEncGetInBuf] IOCTL_MFC_GET_IN_BUF failed");
        return MFC_RET_ENC_GET_INBUF_FAIL;
    }

    pCTX->virFrmBuf.luma = pCTX->mapped_addr + user_addr_arg.args.mem_alloc.offset;
    pCTX->virFrmBuf.chroma = pCTX->mapped_addr + user_addr_arg.args.mem_alloc.offset + (unsigned int)aligned_y_size;

    real_addr_arg.args.real_addr.key = user_addr_arg.args.mem_alloc.offset;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_REAL_ADDR, &real_addr_arg);
    if (ret_code  < 0) {
        LOGE("SsbSipMfcEncGetInBuf] IOCTL_MFC_GET_REAL_ADDR failed");
        return MFC_RET_ENC_GET_INBUF_FAIL;
    }
    pCTX->phyFrmBuf.luma = real_addr_arg.args.real_addr.addr;
    pCTX->phyFrmBuf.chroma = real_addr_arg.args.real_addr.addr + (unsigned int)aligned_y_size;

    pCTX->sizeFrmBuf.luma = (unsigned int)y_size;
    pCTX->sizeFrmBuf.chroma = (unsigned int)c_size;
    pCTX->inter_buff_status |= MFC_USE_YUV_BUFF;

    input_info->YPhyAddr = (void*)pCTX->phyFrmBuf.luma;
    input_info->CPhyAddr = (void*)pCTX->phyFrmBuf.chroma;
    input_info->YVirAddr = (void*)pCTX->virFrmBuf.luma;
    input_info->CVirAddr = (void*)pCTX->virFrmBuf.chroma;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetInBuf(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info)
{
    _MFCLIB *pCTX;
    int ret_code;
    struct mfc_common_args user_addr_arg, real_addr_arg;
    int y_size, c_size;
    int aligned_y_size, aligned_c_size;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncSetInBuf] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    LOGV("SsbSipMfcEncSetInBuf] input_info->YPhyAddr & input_info->CPhyAddr should be 64KB aligned");

    pCTX  = (_MFCLIB *) openHandle;

    /* FIXME: */
    y_size = pCTX->width * pCTX->height;
    c_size = (pCTX->width * pCTX->height) >> 1;

    /* lenear: 2KB, tile: 8KB */
    aligned_y_size = Align(y_size, 64 * BUF_L_UNIT);
    aligned_c_size = Align(c_size, 64 * BUF_L_UNIT);

    pCTX->phyFrmBuf.luma = (unsigned int)input_info->YPhyAddr;
    pCTX->phyFrmBuf.chroma = (unsigned int)input_info->CPhyAddr;

    pCTX->sizeFrmBuf.luma = (unsigned int)input_info->YSize;
    pCTX->sizeFrmBuf.chroma = (unsigned int)input_info->CSize;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetOutBuf(void *openHandle, SSBSIP_MFC_ENC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncGetOutBuf] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    output_info->headerSize = pCTX->encodedHeaderSize;
    output_info->dataSize = pCTX->encodedDataSize;
    output_info->frameType = pCTX->encodedframeType;

    if (pCTX->encode_cnt == 0) {
        output_info->StrmPhyAddr = (void *)pCTX->phyStrmBuf;
        output_info->StrmVirAddr = (unsigned char *)pCTX->virStrmBuf + pCTX->mapped_addr;
    } else {
        output_info->StrmPhyAddr = (unsigned char *)pCTX->phyStrmBuf + (MAX_ENCODER_OUTPUT_BUFFER_SIZE / 2);
        output_info->StrmVirAddr = (unsigned char *)pCTX->virStrmBuf + pCTX->mapped_addr + (MAX_ENCODER_OUTPUT_BUFFER_SIZE / 2);
    }

    pCTX->encode_cnt ++;
    pCTX->encode_cnt %= 2;

    output_info->encodedYPhyAddr = (void*)pCTX->encodedphyFrmBuf.luma;
    output_info->encodedCPhyAddr = (void*)pCTX->encodedphyFrmBuf.chroma;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetOutBuf(void *openHandle, void *phyOutbuf, void *virOutbuf, int outputBufferSize)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncSetOutBuf] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    pCTX->phyStrmBuf = (int)phyOutbuf;
    pCTX->virStrmBuf = (int)virOutbuf;
    pCTX->sizeStrmBuf = outputBufferSize;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetConfig(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args EncArg;
    struct mfc_enc_vui_info vui_info;
    struct mfc_enc_hier_p_qp hier_p_qp;
#ifdef S3D_SUPPORT
    struct mfc_enc_set_config set_info;
    struct mfc_frame_packing *frame_packing;
#endif

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncSetConfig] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("SsbSipMfcEncSetConfig] value is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;
    memset(&EncArg, 0x00, sizeof(struct mfc_common_args));

#ifdef S3D_SUPPORT
    EncArg.args.config.type = conf_type;

    switch (conf_type) {
    case MFC_ENC_SETCONF_FRAME_TAG:
        pCTX->inframetag = *((unsigned int *)value);
        return MFC_RET_OK;
    case MFC_ENC_SETCONF_ALLOW_FRAME_SKIP:
        set_info = *((struct mfc_enc_set_config *) value);
        EncArg.args.config.args.basic.values[0] = set_info.enable;
        if (set_info.enable == 2)
            EncArg.args.config.args.basic.values[1] = set_info.number;
        else
            EncArg.args.config.args.basic.values[1] = 0;
        break;
    case MFC_ENC_SETCONF_VUI_INFO:
        set_info = *((struct mfc_enc_set_config *) value);
        EncArg.args.config.args.basic.values[0] = set_info.enable;
        if (set_info.enable == 255) //Re-check this part of code with Jeongtae Park
            EncArg.args.config.args.basic.values[1] = set_info.number;
        else
            EncArg.args.config.args.basic.values[1] = 0;

        EncArg.args.config.args.basic.values[1] = set_info.number;
        break;
    case MFC_ENC_SETCONF_FRAME_PACKING:
        frame_packing = (struct mfc_frame_packing *)value;
        /*
        memcpy(&EncArg.args.config.args.frame_packing, frame_packing,
            sizeof(struct mfc_frame_packing));
        */
        EncArg.args.config.args.basic.values[0] = frame_packing->arrangement_type;
        EncArg.args.config.args.basic.values[1] = frame_packing->current_frame_is_frame0_flag;
        break;
    case MFC_ENC_SETCONF_FRAME_TYPE:
    case MFC_ENC_SETCONF_CHANGE_FRAME_RATE:
    case MFC_ENC_SETCONF_CHANGE_BIT_RATE:
    case MFC_ENC_SETCONF_I_PERIOD:
    case MFC_ENC_SETCONF_HIER_P:
    case MFC_ENC_SETCONF_SEI_GEN:
        EncArg.args.config.args.basic.values[0] = *((int *) value);
        EncArg.args.config.args.basic.values[1] = 0;
        break;
    default:
        LOGE("SsbSipMfcEncSetConfig] not supported type");
        return MFC_RET_ENC_SET_CONF_FAIL;
    }
#else
    EncArg.args.set_config.in_config_param = conf_type;
    switch (conf_type) {
    case MFC_ENC_SETCONF_FRAME_TAG:
        pCTX->inframetag = *((unsigned int *)value);
        return MFC_RET_OK;
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
    default:
        EncArg.args.set_config.in_config_value[0]  = *((int *) value);
        EncArg.args.set_config.in_config_value[1]  = 0;
        break;
    }
#endif

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_SET_CONFIG, &EncArg);
    if (EncArg.ret_code != MFC_OK) {
        LOGE("SsbSipMfcEncSetConfig] IOCTL_MFC_SET_CONFIG failed(ret : %d)", EncArg.ret_code);
        return MFC_RET_ENC_SET_CONF_FAIL;
    }

    return MFC_RET_OK;
}


SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetConfig(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value)
{
    _MFCLIB *pCTX;
    /*
    unsigned int *encoded_header_size;
    */

    pCTX = (_MFCLIB *)openHandle;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncGetConfig] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }
    if (value == NULL) {
        LOGE("SsbSipMfcEncGetConfig] value is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    switch (conf_type) {
    case MFC_ENC_GETCONF_FRAME_TAG:
        *((unsigned int *)value) = pCTX->outframetagtop;
        break;
#if 0
    case MFC_ENC_GETCONF_HEADER_SIZE:
        encoded_header_size = (unsigned int *)value;
        *encoded_header_size = pCTX->encodedHeaderSize;
        break;
#endif
    default:
        LOGE("SsbSipMfcEncGetConfig] No such conf_type is supported.");
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_OK;
}

