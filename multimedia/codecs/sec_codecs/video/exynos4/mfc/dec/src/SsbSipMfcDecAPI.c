/*
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
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

#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

#include <utils/Log.h>
/*#define LOG_NDEBUG 0*/
#undef  LOG_TAG
#define LOG_TAG "MFC_DEC_APP"

#ifdef CONFIG_MFC_FPS
#include <sys/time.h>
#endif

#define _MFCLIB_MAGIC_NUMBER    0x92241000

#define USR_DATA_START_CODE     (0x000001B2)
#define VOP_START_CODE          (0x000001B6)
#define MP4_START_CODE          (0x000001)

#ifdef CONFIG_MFC_FPS
unsigned int framecount, over30ms;
struct timeval mDec1, mDec2, mAvg;
#endif

static char *mfc_dev_name = SAMSUNG_MFC_DEV_NAME;

static void getAByte(char *buff, int *code)
{
    int byte;

    *code = (*code << 8);
    byte = (int)*buff;
    byte &= 0xFF;
    *code |= byte;
}

static int isPBPacked(_MFCLIB *pCtx, int Frameleng)
{
    char *strmBuffer = NULL;
    int startCode = 0xFFFFFFFF;
    int leng_idx = 1;

    strmBuffer = (char*)pCtx->virStrmBuf;

    while (1) {
        while (startCode != USR_DATA_START_CODE) {
            if ((startCode == VOP_START_CODE) || (leng_idx == Frameleng)) {
                LOGI("isPBPacked] VOP START Found !!.....return");
                LOGW("isPBPacked] Non Packed PB");
                return 0;
            }
            getAByte(strmBuffer, &startCode);
            LOGV(">> StartCode = 0x%08x <<\n", startCode);
            strmBuffer++;
            leng_idx++;
        }
        LOGI("isPBPacked] User Data Found !!");

        do {
            if (*strmBuffer == 'p') {
                /*LOGI(">> peter strmBuffer = 0x%08x <<\n", *strmBuffer);*/
                LOGW("isPBPacked] Packed PB\n");
                return 1;
            }
            getAByte(strmBuffer, &startCode);
            strmBuffer++; leng_idx++;
        } while ((leng_idx <= Frameleng) && ((startCode >> 8) != MP4_START_CODE));

        if (leng_idx > Frameleng)
            break;
    }

    LOGW("isPBPacked] Non Packed PB");

    return 0;
}

void SsbSipMfcDecSetMFCName(char *devicename)
{
    mfc_dev_name = devicename;
}

void *SsbSipMfcDecOpen(void)
{
    int hMFCOpen;
    unsigned int mapped_addr;
    _MFCLIB *pCTX = NULL;
    int mapped_size;
    struct mfc_common_args CommonArg;

    LOGI("[%s] MFC Library Ver %d.%02d\n",__func__, MFC_LIB_VER_MAJOR, MFC_LIB_VER_MINOR);
#ifdef CONFIG_MFC_FPS
    framecount = 0;
    over30ms = 0;
    mAvg.tv_sec = 0;
    mAvg.tv_usec = 0;
#endif
    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("SsbSipMfcDecOpen] malloc failed.\n");
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    if (access(mfc_dev_name, F_OK) != 0) {
        LOGE("SsbSipMfcDecOpen] MFC device node not exists");
        free(pCTX);
        return NULL;
    }

    hMFCOpen = open(mfc_dev_name, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        LOGE("SsbSipMfcDecOpen] MFC Open failure");
        free(pCTX);
        return NULL;
    }

    mapped_size = ioctl(hMFCOpen, IOCTL_MFC_GET_MMAP_SIZE, &CommonArg);
    if ((mapped_size < 0) || (CommonArg.ret_code != MFC_OK)) {
        LOGE("SsbSipMfcDecOpen] IOCTL_MFC_GET_MMAP_SIZE failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    mapped_addr = (unsigned int)mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        LOGE("SsbSipMfcDecOpen] FIMV5.x driver address mapping failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    pCTX->magic = _MFCLIB_MAGIC_NUMBER;
    pCTX->hMFC = hMFCOpen;
    pCTX->mapped_addr = mapped_addr;
    pCTX->mapped_size = mapped_size;
    pCTX->inter_buff_status = MFC_USE_NONE;

    return (void *)pCTX;
}

void *SsbSipMfcDecOpenExt(void *value)
{
    int hMFCOpen;
    unsigned int mapped_addr;
    _MFCLIB *pCTX = NULL;
    int mapped_size;
    int err;
    struct mfc_common_args CommonArg;

    LOGI("[%s] MFC Library Ver %d.%02d\n",__func__, MFC_LIB_VER_MAJOR, MFC_LIB_VER_MINOR);

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("SsbSipMfcDecOpenExt] malloc failed.\n");
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    if (access(mfc_dev_name, F_OK) != 0) {
        LOGE("SsbSipMfcDecOpen] MFC device node not exists");
        free(pCTX);
        return NULL;
    }

    hMFCOpen = open(mfc_dev_name, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        LOGE("SsbSipMfcDecOpenExt] MFC Open failure");
        free(pCTX);
        return NULL;
    }

    CommonArg.args.mem_alloc.buf_cache_type = *(SSBIP_MFC_BUFFER_TYPE *)value;

    err = ioctl(hMFCOpen, IOCTL_MFC_SET_BUF_CACHE, &CommonArg);
    if ((err < 0) || (CommonArg.ret_code != MFC_OK)) {
        LOGE("SsbSipMfcDecOpenExt] IOCTL_MFC_SET_BUF_CACHE failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    mapped_size = ioctl(hMFCOpen, IOCTL_MFC_GET_MMAP_SIZE, &CommonArg);
    if ((mapped_size < 0) || (CommonArg.ret_code != MFC_OK)) {
        LOGE("SsbSipMfcDecOpenExt] IOCTL_MFC_GET_MMAP_SIZE failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    mapped_addr = (unsigned int)mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        LOGE("SsbSipMfcDecOpenExt] FIMV5.x driver address mapping failed");
        free(pCTX);
        close(hMFCOpen);
        return NULL;
    }

    pCTX->magic = _MFCLIB_MAGIC_NUMBER;
    pCTX->hMFC = hMFCOpen;
    pCTX->mapped_addr = mapped_addr;
    pCTX->mapped_size = mapped_size;
    pCTX->inter_buff_status = MFC_USE_NONE;

    return (void *)pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecInit (void *openHandle, SSBSIP_MFC_CODEC_TYPE codec_type, int Frameleng)
{
    int r;
    int packedPB = 0;
    struct mfc_common_args DecArg;
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecInit] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));

    if ((codec_type != MPEG4_DEC)  &&
        (codec_type != H264_DEC)   &&
        (codec_type != H263_DEC)   &&
        (codec_type != MPEG1_DEC)  &&
        (codec_type != MPEG2_DEC)  &&
        (codec_type != FIMV1_DEC)  &&
        (codec_type != FIMV2_DEC)  &&
        (codec_type != FIMV3_DEC)  &&
        (codec_type != FIMV4_DEC)  &&
        (codec_type != XVID_DEC)   &&
        (codec_type != VC1RCV_DEC) &&
        (codec_type != VC1_DEC)) {
        LOGE("SsbSipMfcDecInit] Undefined codec type");
        return MFC_RET_INVALID_PARAM;
    }
    pCTX->codecType = codec_type;

    if ((pCTX->codecType == MPEG4_DEC) ||
        (pCTX->codecType == XVID_DEC)  ||
        (pCTX->codecType == FIMV1_DEC) ||
        (pCTX->codecType == FIMV2_DEC) ||
        (pCTX->codecType == FIMV3_DEC) ||
        (pCTX->codecType == FIMV4_DEC))
        packedPB = isPBPacked(pCTX, Frameleng);

    /* init args */
    DecArg.args.dec_init.in_codec_type = pCTX->codecType;
    DecArg.args.dec_init.in_strm_size = Frameleng;
    DecArg.args.dec_init.in_strm_buf = pCTX->phyStrmBuf;

    DecArg.args.dec_init.in_numextradpb = pCTX->dec_numextradpb;
    DecArg.args.dec_init.in_slice= pCTX->dec_slice;
    DecArg.args.dec_init.in_crc = pCTX->dec_crc;
    DecArg.args.dec_init.in_pixelcache = pCTX->dec_pixelcache;

    DecArg.args.dec_init.in_packed_PB = packedPB;

    /* mem alloc args */
    DecArg.args.dec_init.in_mapped_addr = pCTX->mapped_addr;

    /* get pyhs addr args */
    /* no needs */

    /* sequence start args */
    /* no needs */

    r = ioctl(pCTX->hMFC, IOCTL_MFC_DEC_INIT, &DecArg);
    if (DecArg.ret_code != MFC_OK) {
        LOGE("SsbSipMfcDecInit] IOCTL_MFC_DEC_INIT failed");
        return MFC_RET_DEC_INIT_FAIL;
    }

    pCTX->decOutInfo.img_width = DecArg.args.dec_init.out_frm_width;
    pCTX->decOutInfo.img_height = DecArg.args.dec_init.out_frm_height;
    pCTX->decOutInfo.buf_width = DecArg.args.dec_init.out_buf_width;
    pCTX->decOutInfo.buf_height = DecArg.args.dec_init.out_buf_height;

    pCTX->decOutInfo.crop_top_offset = DecArg.args.dec_init.out_crop_top_offset;
    pCTX->decOutInfo.crop_bottom_offset = DecArg.args.dec_init.out_crop_bottom_offset;
    pCTX->decOutInfo.crop_left_offset = DecArg.args.dec_init.out_crop_left_offset;
    pCTX->decOutInfo.crop_right_offset = DecArg.args.dec_init.out_crop_right_offset;

    /*
    pCTX->virFrmBuf.luma = DecArg.args.dec_init.out_u_addr.luma;
    pCTX->virFrmBuf.chroma = DecArg.args.dec_init.out_u_addr.chroma;

    pCTX->phyFrmBuf.luma = DecArg.args.dec_init.out_p_addr.luma;
    pCTX->phyFrmBuf.chroma = DecArg.args.dec_init.out_p_addr.chroma;
    pCTX->sizeFrmBuf.luma = DecArg.args.dec_init.out_frame_buf_size.luma;
    pCTX->sizeFrmBuf.chroma = DecArg.args.dec_init.out_frame_buf_size.chroma;
    */

    pCTX->inter_buff_status |= MFC_USE_YUV_BUFF;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExe(void *openHandle, int lengthBufFill)
{
    int ret;
    int Yoffset;
    int Coffset;
    _MFCLIB *pCTX;
    struct mfc_common_args DecArg;

#ifdef CONFIG_MFC_FPS
    long int diffTime, avgTime;
#endif
    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecExe] openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    if ((lengthBufFill < 0) || (lengthBufFill > MAX_DECODER_INPUT_BUFFER_SIZE)) {
        LOGE("SsbSipMfcDecExe] lengthBufFill is invalid. (lengthBufFill=%d)", lengthBufFill);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));

    DecArg.args.dec_exe.in_codec_type = pCTX->codecType;
    DecArg.args.dec_exe.in_strm_buf = pCTX->phyStrmBuf;
    DecArg.args.dec_exe.in_strm_size = lengthBufFill;
    DecArg.args.dec_exe.in_frm_buf.luma = pCTX->phyFrmBuf.luma;
    DecArg.args.dec_exe.in_frm_buf.chroma = pCTX->phyFrmBuf.chroma;
    DecArg.args.dec_exe.in_frm_size.luma = pCTX->sizeFrmBuf.luma;
    DecArg.args.dec_exe.in_frm_size.chroma = pCTX->sizeFrmBuf.chroma;
    DecArg.args.dec_exe.in_frametag = pCTX->inframetag;
    DecArg.args.dec_exe.in_immediately_disp = pCTX->immediatelydisp;

#ifdef CONFIG_MFC_FPS
    gettimeofday(&mDec1, NULL);

#ifdef CONFIG_MFC_PERF_LOG
    if (framecount != 0) {
        if (mDec2.tv_sec == mDec1.tv_sec)
            LOGI("SsbSipMfcDecExe] Interval between IOCTL_MFC_DEC_EXE's (end to start) = %8d", (mDec1.tv_usec - mDec2.tv_usec));
        else
            LOGI("SsbSipMfcDecExe] Interval between IOCTL_MFC_DEC_EXE's (end to start) = %8d", (1000000 + (mDec1.tv_usec - mDec2.tv_usec)));
    }
#endif
#endif

    ret = ioctl(pCTX->hMFC, IOCTL_MFC_DEC_EXE, &DecArg);

    if (DecArg.ret_code != MFC_OK) {
        LOGE("SsbSipMfcDecExe] IOCTL_MFC_DEC_EXE failed(ret : %d)", DecArg.ret_code);
        return MFC_RET_DEC_EXE_ERR;
    }

#ifdef CONFIG_MFC_FPS
    gettimeofday(&mDec2, NULL);
    framecount++;

    if (mDec1.tv_sec == mDec2.tv_sec) {
        if (mDec2.tv_usec - mDec1.tv_usec > 30000)
            over30ms++;
#ifdef CONFIG_MFC_PERF_LOG
        LOGI("SsbSipMfcDecExe] Time consumed for IOCTL_MFC_DEC_EXE = %8d", ((mDec2.tv_usec - mDec1.tv_usec)));
#endif
    } else {
        if (1000000 + mDec2.tv_usec - mDec1.tv_usec > 30000)
            over30ms++;
#ifdef CONFIG_MFC_PERF_LOG
        LOGI("SsbSipMfcDecExe] Time consumed for IOCTL_MFC_DEC_EXE = %8d", (1000000 + (mDec2.tv_usec - mDec1.tv_usec)));
#endif
    }

    diffTime = ((mDec2.tv_sec * 1000000) + mDec2.tv_usec) - ((mDec1.tv_sec * 1000000) + mDec1.tv_usec);
    avgTime = (mAvg.tv_sec * 1000000) + mAvg.tv_usec;
    avgTime = ((framecount - 1) * avgTime + diffTime) / framecount;

    mAvg.tv_sec = avgTime / 1000000;
    mAvg.tv_usec = avgTime % 1000000;
#endif

    /* FIXME: dynamic resolution change */
    if (DecArg.args.dec_exe.out_display_status == 4) {
        LOGI("SsbSipMfcDecExe] Resolution is chagned");
        /*
        pCTX->virFrmBuf.chroma = DecArg.args.dec_exe.out_u_addr.chroma;
        pCTX->virFrmBuf.luma = DecArg.args.dec_exe.out_u_addr.luma;
        pCTX->phyFrmBuf.chroma = DecArg.args.dec_exe.out_p_addr.chroma;
        pCTX->phyFrmBuf.luma = DecArg.args.dec_exe.out_p_addr.luma;
        pCTX->sizeFrmBuf.chroma = DecArg.args.dec_exe.out_frame_buf_size.chroma;
        pCTX->sizeFrmBuf.luma = DecArg.args.dec_exe.out_frame_buf_size.luma;
        */
        pCTX->decOutInfo.img_width =  DecArg.args.dec_exe.out_img_width;
        pCTX->decOutInfo.img_height = DecArg.args.dec_exe.out_img_height;
        pCTX->decOutInfo.buf_width = DecArg.args.dec_exe.out_buf_width;
        pCTX->decOutInfo.buf_height = DecArg.args.dec_exe.out_buf_height;
    }

    Yoffset = DecArg.args.dec_exe.out_display_Y_addr - DecArg.args.dec_exe.in_frm_buf.luma;
    Coffset = DecArg.args.dec_exe.out_display_C_addr - DecArg.args.dec_exe.in_frm_buf.chroma;

    pCTX->decOutInfo.YPhyAddr = (void*)(DecArg.args.dec_exe.out_display_Y_addr);
    pCTX->decOutInfo.CPhyAddr = (void*)(DecArg.args.dec_exe.out_display_C_addr);

    pCTX->decOutInfo.YVirAddr = (void*)(pCTX->virFrmBuf.luma + Yoffset);
    pCTX->decOutInfo.CVirAddr = (void*)(pCTX->virFrmBuf.chroma + Coffset);

    /* for new driver */
    pCTX->decOutInfo.YVirAddr = (void*)(pCTX->mapped_addr + DecArg.args.dec_exe.out_y_offset);
    pCTX->decOutInfo.CVirAddr = (void*)(pCTX->mapped_addr + DecArg.args.dec_exe.out_c_offset);

    pCTX->displayStatus = DecArg.args.dec_exe.out_display_status;

    pCTX->decOutInfo.disp_pic_frame_type = DecArg.args.dec_exe.out_disp_pic_frame_type;

    /* clear immediately display flag */
    pCTX->immediatelydisp = 0;
    pCTX->outframetagtop = DecArg.args.dec_exe.out_frametag_top;
    pCTX->outframetagbottom = DecArg.args.dec_exe.out_frametag_bottom;
    pCTX->decOutInfo.timestamp_top = DecArg.args.dec_exe.out_pic_time_top;
    pCTX->decOutInfo.timestamp_bottom = DecArg.args.dec_exe.out_pic_time_bottom;
    pCTX->decOutInfo.consumedByte =  DecArg.args.dec_exe.out_consumed_byte;

    pCTX->decOutInfo.crop_right_offset =  DecArg.args.dec_exe.out_crop_right_offset;
    pCTX->decOutInfo.crop_left_offset =  DecArg.args.dec_exe.out_crop_left_offset;
    pCTX->decOutInfo.crop_bottom_offset =  DecArg.args.dec_exe.out_crop_bottom_offset;
    pCTX->decOutInfo.crop_top_offset =  DecArg.args.dec_exe.out_crop_top_offset;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecClose(void *openHandle)
{
    int ret;
    _MFCLIB  *pCTX;
    struct mfc_common_args free_arg;

#ifdef CONFIG_MFC_FPS
    LOGI(">>> Statistics in MFC API:");
    LOGI(">>> Total number of IOCTL_MFC_DEC_EXE = %d", framecount);
    LOGI(">>> Number of IOCTL_MFC_DEC_EXE taking more than 30ms = %d", over30ms);
    LOGI(">>> Avg IOCTL_MFC_DEC_EXE time = %dsec %.2fmsec", (int)mAvg.tv_sec, (float)(mAvg.tv_usec / 1000.0));
#endif

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecClose] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    /* FIXME: free buffer? */
#if 0
    if (pCTX->inter_buff_status & MFC_USE_YUV_BUFF) {
        free_arg.args.mem_free.key = pCTX->virFrmBuf.luma;
        ret = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
        free_arg.args.mem_free.key = pCTX->virFrmBuf.chroma;
        ret = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }
#endif

    if (pCTX->inter_buff_status & MFC_USE_STRM_BUFF) {
        free_arg.args.mem_free.key = pCTX->virStrmBuf;
        ret = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }

    pCTX->inter_buff_status = MFC_USE_NONE;

    munmap((void *)pCTX->mapped_addr, pCTX->mapped_size);
    close(pCTX->hMFC);
    free(pCTX);

    return MFC_RET_OK;
}


void  *SsbSipMfcDecGetInBuf(void *openHandle, void **phyInBuf, int inputBufferSize)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args user_addr_arg, phys_addr_arg;

    if (inputBufferSize < 0) {
        LOGE("SsbSipMfcDecGetInBuf] inputBufferSize = %d is invalid", inputBufferSize);
        return NULL;
    }

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecGetInBuf] openHandle is NULL\n");
        return NULL;
    }

    pCTX  = (_MFCLIB *) openHandle;

    /*user_addr_arg.args.mem_alloc.codec_type = pCTX->codec_type; */
    user_addr_arg.args.mem_alloc.type = DECODER;
    user_addr_arg.args.mem_alloc.buff_size = inputBufferSize;
    user_addr_arg.args.mem_alloc.mapped_addr = pCTX->mapped_addr;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_IN_BUF, &user_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcDecGetInBuf] IOCTL_MFC_GET_IN_BUF failed");
        return NULL;
    }

    phys_addr_arg.args.real_addr.key = user_addr_arg.args.mem_alloc.offset;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_REAL_ADDR, &phys_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcDecGetInBuf] IOCTL_MFC_GET_PHYS_ADDR failed");
        return NULL;
    }

    /*
    pCTX->virStrmBuf = user_addr_arg.args.mem_alloc.offset;
    */
    pCTX->virStrmBuf = pCTX->mapped_addr + user_addr_arg.args.mem_alloc.offset;
    pCTX->phyStrmBuf = phys_addr_arg.args.real_addr.addr;

    pCTX->sizeStrmBuf = inputBufferSize;
    pCTX->inter_buff_status |= MFC_USE_STRM_BUFF;

    *phyInBuf = (void *)pCTX->phyStrmBuf;

    return (void *)pCTX->virStrmBuf;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetInBuf(void *openHandle, void *phyInBuf, void *virInBuf, int size)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecSetInBuf] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    pCTX->phyStrmBuf = (int)phyInBuf;
    pCTX->virStrmBuf = (int)virInBuf;
    pCTX->sizeStrmBuf = size;
    return MFC_RET_OK;
}

SSBSIP_MFC_DEC_OUTBUF_STATUS SsbSipMfcDecGetOutBuf(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecGetOutBuf] openHandle is NULL");
        return MFC_GETOUTBUF_DISPLAY_END;
    }

    pCTX  = (_MFCLIB *) openHandle;

    output_info->YPhyAddr = pCTX->decOutInfo.YPhyAddr;
    output_info->CPhyAddr = pCTX->decOutInfo.CPhyAddr;

    output_info->YVirAddr = pCTX->decOutInfo.YVirAddr;
    output_info->CVirAddr = pCTX->decOutInfo.CVirAddr;

    output_info->img_width = pCTX->decOutInfo.img_width;
    output_info->img_height= pCTX->decOutInfo.img_height;

    output_info->buf_width = pCTX->decOutInfo.buf_width;
    output_info->buf_height= pCTX->decOutInfo.buf_height;

    output_info->timestamp_top =  pCTX->decOutInfo.timestamp_top;
    output_info->timestamp_bottom =  pCTX->decOutInfo.timestamp_bottom;
    output_info->consumedByte =  pCTX->decOutInfo.consumedByte;

    output_info->crop_right_offset =  pCTX->decOutInfo.crop_right_offset;
    output_info->crop_left_offset =  pCTX->decOutInfo.crop_left_offset;
    output_info->crop_bottom_offset =  pCTX->decOutInfo.crop_bottom_offset;
    output_info->crop_top_offset =  pCTX->decOutInfo.crop_top_offset;

    output_info->disp_pic_frame_type = pCTX->decOutInfo.disp_pic_frame_type;

    if (pCTX->displayStatus == 3)
        return MFC_GETOUTBUF_DISPLAY_END;
    else if (pCTX->displayStatus == 1)
        return MFC_GETOUTBUF_DISPLAY_DECODING;
    else if (pCTX->displayStatus == 2)
        return MFC_GETOUTBUF_DISPLAY_ONLY;
    else if (pCTX->displayStatus == 0)
        return MFC_GETOUTBUF_DECODING_ONLY;
    else if (pCTX->displayStatus == 4)
        return MFC_GETOUTBUF_CHANGE_RESOL;
    else
        return MFC_GETOUTBUF_DISPLAY_END;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args DecArg;
    struct mfc_dec_fimv1_info *fimv1_res;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecSetConfig] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("SsbSipMfcDecSetConfig] value is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));
#ifdef S3D_SUPPORT
    DecArg.args.config.type = conf_type;
#else
    DecArg.args.set_config.in_config_param = conf_type;
#endif
    switch (conf_type) {
    case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
        pCTX->dec_numextradpb = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_SLICE_ENABLE:
        pCTX->dec_slice = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_CRC_ENABLE:
        pCTX->dec_crc = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_PIXEL_CACHE:
        pCTX->dec_pixelcache = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_FRAME_TAG: /* be set before calling SsbSipMfcDecExe */
        pCTX->inframetag = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_IMMEDIATELY_DISPLAY: /* be set before calling SsbSipMfcDecExe */
        pCTX->immediatelydisp  = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT:
        fimv1_res = (struct mfc_dec_fimv1_info *)value;
        LOGI("fimv1->width  = %d\n", fimv1_res->width);
        LOGI("fimv1->height = %d\n", fimv1_res->height);
#ifdef S3D_SUPPORT
        DecArg.args.config.args.basic.values[0]  = (int)(fimv1_res->width);
        DecArg.args.config.args.basic.values[1]  = (int)(fimv1_res->height);
#else
        DecArg.args.set_config.in_config_value[0]  = (int)(fimv1_res->width);
        DecArg.args.set_config.in_config_value[1]  = (int)(fimv1_res->height);
#endif
        break;
    case MFC_DEC_SETCONF_IS_LAST_FRAME:
    case MFC_DEC_SETCONF_DPB_FLUSH:
#ifdef S3D_SUPPORT
    case MFC_DEC_SETCONF_SEI_PARSE:
    default:
        DecArg.args.config.args.basic.values[0] = *((int *) value);
        DecArg.args.config.args.basic.values[1]  = 0;
#else
    default:
        DecArg.args.set_config.in_config_value[0]  = *((unsigned int *) value);
        DecArg.args.set_config.in_config_value[1]  = 0;
#endif
        break;
    }

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_SET_CONFIG, &DecArg);
    if (DecArg.ret_code != MFC_OK) {
        LOGE("SsbSipMfcDecSetConfig] IOCTL_MFC_SET_CONFIG failed(ret : %d, conf_type: 0x%08x)", DecArg.ret_code, conf_type);
        return MFC_RET_DEC_SET_CONF_FAIL;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecGetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args DecArg;

    /*
    s3c_mfc_common_args phys_addr_arg;
    SSBSIP_MFC_BUFFER_ADDR *buf_addr;
    */

    SSBSIP_MFC_IMG_RESOLUTION *img_resolution;
    SSBSIP_MFC_CRC_DATA *crc_data;
    SSBSIP_MFC_CROP_INFORMATION *crop_information;
#ifdef S3D_SUPPORT
    SSBSIP_MFC_FRAME_PACKING *frame_packing;
#endif

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecGetConfig] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("SsbSipMfcDecGetConfig] value is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *) openHandle;

    switch (conf_type) {
#if 0
    case MFC_DEC_GETCONF_PHYS_ADDR:
        buf_addr = (SSBSIP_MFC_BUFFER_ADDR *)value;
        phys_addr_arg.args.get_phys_addr.u_addr = buf_addr->u_addr;
        r = ioctl(pCTX->hMFC, IOCTL_MFC_GET_PHYS_ADDR, &phys_addr_arg);
        if (r < 0) {
            LOGE("SsbSipMfcDecGetConfig] IOCTL_MFC_GET_PHYS_ADDR failed");
            return MFC_API_FAIL;
        }
        buf_addr->p_addr = phys_addr_arg.args.get_phys_addr.p_addr;
        break;
#endif
    case MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT:
        img_resolution = (SSBSIP_MFC_IMG_RESOLUTION *)value;
        img_resolution->width = pCTX->decOutInfo.img_width;
        img_resolution->height = pCTX->decOutInfo.img_height;
        img_resolution->buf_width = pCTX->decOutInfo.buf_width;
        img_resolution->buf_height = pCTX->decOutInfo.buf_height;
        break;
    case MFC_DEC_GETCONF_FRAME_TAG:
        *((unsigned int *)value) = pCTX->outframetagtop;
        break;
    case MFC_DEC_GETCONF_CROP_INFO:
        crop_information = (SSBSIP_MFC_CROP_INFORMATION *)value;
        crop_information->crop_top_offset = pCTX->decOutInfo.crop_top_offset;
        crop_information->crop_bottom_offset = pCTX->decOutInfo.crop_bottom_offset;
        crop_information->crop_left_offset = pCTX->decOutInfo.crop_left_offset;
        crop_information->crop_right_offset = pCTX->decOutInfo.crop_right_offset;
        break;
    case MFC_DEC_GETCONF_CRC_DATA:
#ifdef S3D_SUPPORT
    case MFC_DEC_GETCONF_FRAME_PACKING:
        memset(&DecArg, 0x00, sizeof(DecArg));
        DecArg.args.config.type = conf_type;

        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_CONFIG, &DecArg);
        if (DecArg.ret_code != MFC_OK) {
            LOGE("SsbSipMfcDecGetConfig] IOCTL_MFC_GET_CONFIG failed(ret : %d, conf_type: 0x%08x)", DecArg.ret_code, conf_type);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }

        if (conf_type == MFC_DEC_GETCONF_CRC_DATA) {
            crc_data = (SSBSIP_MFC_CRC_DATA *)value;

            crc_data->luma0 = DecArg.args.config.args.basic.values[0];
            crc_data->chroma0 = DecArg.args.config.args.basic.values[1];
        } else {
            frame_packing = (SSBSIP_MFC_FRAME_PACKING *)value;
            memcpy(frame_packing, &DecArg.args.config.args.frame_packing,
                sizeof(SSBSIP_MFC_FRAME_PACKING));
        }
#else
        crc_data = (SSBSIP_MFC_CRC_DATA *)value;

        memset(&DecArg, 0x00, sizeof(DecArg));
        DecArg.args.get_config.in_config_param = conf_type;

        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_CONFIG, &DecArg);
        if (DecArg.ret_code != MFC_OK) {
            LOGE("SsbSipMfcDecGetConfig] IOCTL_MFC_GET_CONFIG failed(ret : %d, conf_type: 0x%08x)", DecArg.ret_code, conf_type);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }
        crc_data->luma0 = DecArg.args.get_config.out_config_value[0];
        crc_data->chroma0 = DecArg.args.get_config.out_config_value[1];
#endif
        break;
    default:
        LOGE("SsbSipMfcDecGetConfig] No such conf_type is supported");
        return MFC_RET_DEC_GET_CONF_FAIL;
    }

    return MFC_RET_OK;
}

void *SsbSipMfcDecAllocInputBuffer(void *openHandle, void **phyInBuf, int inputBufferSize)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_common_args user_addr_arg, phys_addr_arg;

    if (inputBufferSize < 0) {
        LOGE("SsbSipMfcDecAllocInputBuffer] inputBufferSize = %d is invalid\n", inputBufferSize);
        return NULL;
    }

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecAllocInputBuffer] openHandle is NULL\n");
        return NULL;
    }

    pCTX = (_MFCLIB *)openHandle;

    user_addr_arg.args.mem_alloc.type = DECODER;
    user_addr_arg.args.mem_alloc.buff_size = inputBufferSize;
    user_addr_arg.args.mem_alloc.mapped_addr = pCTX->mapped_addr;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_IN_BUF, &user_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcDecAllocInputBuffer] IOCTL_MFC_GET_IN_BUF failed");
        return NULL;
    }

    phys_addr_arg.args.real_addr.key = user_addr_arg.args.mem_alloc.offset;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_REAL_ADDR, &phys_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcDecGetInBuf] IOCTL_MFC_GET_PHYS_ADDR failed");
        return NULL;
    }

    pCTX->virStrmBuf = pCTX->mapped_addr + user_addr_arg.args.mem_alloc.offset;
    pCTX->phyStrmBuf = phys_addr_arg.args.real_addr.addr;
    pCTX->sizeStrmBuf = inputBufferSize;
    pCTX->inter_buff_status |= MFC_USE_STRM_BUFF;

    *phyInBuf = (void *)pCTX->phyStrmBuf;

    return (void *)pCTX->virStrmBuf;
}

void SsbSipMfcDecFreeInputBuffer(void *openHandle, void *phyInBuf)
{
    int ret;
    _MFCLIB  *pCTX;
    struct mfc_common_args free_arg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecFreeInputBuffer] openHandle is NULL");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    if (pCTX->inter_buff_status & MFC_USE_STRM_BUFF) {
        free_arg.args.mem_free.key = pCTX->virStrmBuf;
        ret = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }
    pCTX->inter_buff_status = MFC_USE_NONE;
    return MFC_RET_OK;
}

/* CRESPO */
#if 1
int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
{
    int pixel_x_m1, pixel_y_m1;
    int roundup_x, roundup_y;
    int linear_addr0, linear_addr1, bank_addr ;
    int x_addr;
    int trans_addr;

    pixel_x_m1 = x_size -1;
    pixel_y_m1 = y_size -1;

    roundup_x = ((pixel_x_m1 >> 7) + 1);
    roundup_y = ((pixel_x_m1 >> 6) + 1);

    x_addr = x_pos >> 2;

    if ((y_size <= y_pos+32) && ( y_pos < y_size) &&
        (((pixel_y_m1 >> 5) & 0x1) == 0) && (((y_pos >> 5) & 0x1) == 0)) {
        linear_addr0 = (((y_pos & 0x1f) <<4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 6) & 0x3f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    } else {
        linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 5) & 0x7f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    }

    linear_addr0 = linear_addr0 << 2;
    trans_addr = (linear_addr1 <<13) | (bank_addr << 11) | linear_addr0;

    return trans_addr;
}

void Y_tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, index;
    unsigned char data8[4];
    unsigned int max_index = x_size * y_size;

    for (i = 0; i < y_size; i = i + 16) {
        for (j = 0; j < x_size; j = j + 16) {
            trans_addr = tile_4x2_read(x_size, y_size, j, i);
            for (k = 0; k < 16; k++) {
                /* limit check - prohibit segmentation fault */
                index = (i * x_size) + (x_size * k) + j;
                /* remove equal condition to solve thumbnail bug */
                if (index + 16 > max_index) {
                    continue;
                }

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 0];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 1];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 2];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 3];

                p_linear_addr[index] = data8[0];
                p_linear_addr[index + 1] = data8[1];
                p_linear_addr[index + 2] = data8[2];
                p_linear_addr[index + 3] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 4];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 5];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 6];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 7];

                p_linear_addr[index + 4] = data8[0];
                p_linear_addr[index + 5] = data8[1];
                p_linear_addr[index + 6] = data8[2];
                p_linear_addr[index + 7] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 8];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 9];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 10];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 11];

                p_linear_addr[index + 8] = data8[0];
                p_linear_addr[index + 9] = data8[1];
                p_linear_addr[index + 10] = data8[2];
                p_linear_addr[index + 11] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 12];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 13];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 14];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 15];

                p_linear_addr[index + 12] = data8[0];
                p_linear_addr[index + 13] = data8[1];
                p_linear_addr[index + 14] = data8[2];
                p_linear_addr[index + 15] = data8[3];
            }
        }
    }
}

void CbCr_tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, index;
    unsigned char data8[4];
    unsigned int half_y_size = y_size / 2;
    unsigned int max_index = x_size * half_y_size;
    unsigned char *pUVAddr[2];

    pUVAddr[0] = p_linear_addr;
    pUVAddr[1] = p_linear_addr + ((x_size * half_y_size) / 2);

    for (i = 0; i < half_y_size; i = i + 16) {
        for (j = 0; j < x_size; j = j + 16) {
            trans_addr = tile_4x2_read(x_size, half_y_size, j, i);
            for (k = 0; k < 16; k++) {
                /* limit check - prohibit segmentation fault */
                index = (i * x_size) + (x_size * k) + j;
                /* remove equal condition to solve thumbnail bug */
                if (index + 16 > max_index) {
                    continue;
                }

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 0];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 1];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 2];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 3];

                pUVAddr[index%2][index/2] = data8[0];
                pUVAddr[(index+1)%2][(index+1)/2] = data8[1];
                pUVAddr[(index+2)%2][(index+2)/2] = data8[2];
                pUVAddr[(index+3)%2][(index+3)/2] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 4];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 5];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 6];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 7];

                pUVAddr[(index+4)%2][(index+4)/2] = data8[0];
                pUVAddr[(index+5)%2][(index+5)/2] = data8[1];
                pUVAddr[(index+6)%2][(index+6)/2] = data8[2];
                pUVAddr[(index+7)%2][(index+7)/2] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 8];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 9];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 10];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 11];

                pUVAddr[(index+8)%2][(index+8)/2] = data8[0];
                pUVAddr[(index+9)%2][(index+9)/2] = data8[1];
                pUVAddr[(index+10)%2][(index+10)/2] = data8[2];
                pUVAddr[(index+11)%2][(index+11)/2] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 12];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 13];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 14];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 15];

                pUVAddr[(index+12)%2][(index+12)/2] = data8[0];
                pUVAddr[(index+13)%2][(index+13)/2] = data8[1];
                pUVAddr[(index+14)%2][(index+14)/2] = data8[2];
                pUVAddr[(index+15)%2][(index+15)/2] = data8[3];
            }
        }
    }
}
#else
int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
{
    int pixel_x_m1, pixel_y_m1;
    int roundup_x, roundup_y;
    int linear_addr0, linear_addr1, bank_addr;
    int x_addr;
    int trans_addr;

    pixel_x_m1 = x_size -1;
    pixel_y_m1 = y_size -1;

    roundup_x = ((pixel_x_m1 >> 7) + 1);
    roundup_y = ((pixel_x_m1 >> 6) + 1);

    x_addr = x_pos >> 2;

    if ((y_size <= y_pos+32)             &&
        ( y_pos < y_size)                &&
        (((pixel_y_m1 >> 5) & 0x1) == 0) &&
        (((y_pos >> 5) & 0x1) == 0)) {
        linear_addr0 = (((y_pos & 0x1f) <<4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 6) & 0x3f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    } else {
        linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 5) & 0x7f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    }

    linear_addr0 = linear_addr0 << 2;
    trans_addr = (linear_addr1 <<13) | (bank_addr << 11) | linear_addr0;

    return trans_addr;
}


void tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, nn, mm;
    unsigned int ix,iy, nx, ny;

    nx = x_size % 16;
    ny = y_size % 16;

    if (nx != 0)
        ix = 16;
    else
        ix = 1;

    if (ny != 0)
        iy = 16;
    else
        iy = 1;

    for (i = 0; i < y_size - iy; i = i + 16) {
        for (j = 0; j < x_size -ix; j = j + 16)	{
            trans_addr = tile_4x2_read(x_size, y_size, j, i);

            k = 0;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 1;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 2;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 3;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 4;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 5;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 6;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 7;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 8;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 9;        nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 10;       nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 11;       nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 12;       nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 13;       nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 14;       nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);

            k = 15;       nn = trans_addr + (k << 6);        mm =x_size*(i+k) + j;
            memcpy(p_linear_addr+mm, p_tiled_addr+nn, 16);
        }
    }
}
#endif
