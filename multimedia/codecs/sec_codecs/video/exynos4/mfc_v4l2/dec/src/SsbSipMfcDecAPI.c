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

#include <sys/poll.h>
#include "videodev2.h"

#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "MFC_DEC_APP"
#include <utils/Log.h>

#ifdef CONFIG_MFC_FPS
#include <sys/time.h>
#endif

/*#define CRC_ENABLE
#define SLICE_MODE_ENABLE */
#define POLL_DEC_WAIT_TIMEOUT 25

#define USR_DATA_START_CODE (0x000001B2)
#define VOP_START_CODE      (0x000001B6)
#define MP4_START_CODE      (0x000001)

#ifdef CONFIG_MFC_FPS
unsigned int framecount, over30ms;
struct timeval mTS1, mTS2, mDec1, mDec2;
#endif

#define DEFAULT_NUMBER_OF_EXTRA_DPB 5

static char *mfc_dev_name = SAMSUNG_MFC_DEV_NAME;
static int mfc_dev_node = 6;

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
                LOGI("[%s] VOP START Found !!.....return",__func__);
                LOGW("[%s] Non Packed PB",__func__);
                return 0;
            }
            getAByte(strmBuffer, &startCode);
            LOGV(">> StartCode = 0x%08x <<\n", startCode);
            strmBuffer++;
            leng_idx++;
        }
        LOGI("[%s] User Data Found !!",__func__);

        do {
            if (*strmBuffer == 'p') {
                LOGW("[%s] Packed PB",__func__);
                return 1;
            }
            getAByte(strmBuffer, &startCode);
            strmBuffer++; leng_idx++;
        } while ((leng_idx <= Frameleng) && ((startCode >> 8) != MP4_START_CODE));

        if (leng_idx > Frameleng)
            break;
    }

    LOGW("[%s] Non Packed PB",__func__);

    return 0;
}

static void getMFCName(char *devicename, int size)
{
    snprintf(devicename, size, "%s%d", SAMSUNG_MFC_DEV_NAME, mfc_dev_node);
}

void SsbSipMfcDecSetMFCNode(int devicenode)
{
    mfc_dev_node = devicenode;
}

void SsbSipMfcDecSetMFCName(char *devicename)
{
    mfc_dev_name = devicename;
}

void *SsbSipMfcDecOpen(void)
{
    int hMFCOpen;
    _MFCLIB *pCTX;

    char mfc_dev_name[64];

    int ret;
    unsigned int i, j;
    struct v4l2_capability cap;
    struct v4l2_format fmt;

    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    LOGI("[%s] MFC Library Ver %d.%02d",__func__, MFC_LIB_VER_MAJOR, MFC_LIB_VER_MINOR);
#ifdef CONFIG_MFC_FPS
    framecount = 0;
    over30ms = 0;
    gettimeofday(&mTS1, NULL);
#endif
    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("[%s] malloc failed.",__func__);
        return NULL;
    }

    memset(pCTX, 0, sizeof(_MFCLIB));

    getMFCName(mfc_dev_name, 64);
    LOGI("[%s] dev name is %s",__func__,mfc_dev_name);

    if (access(mfc_dev_name, F_OK) != 0) {
        LOGE("[%s] MFC device node not exists",__func__);
        goto error_case1;
    }

    hMFCOpen = open(mfc_dev_name, O_RDWR|O_NONBLOCK, 0);
    if (hMFCOpen < 0) {
        LOGE("[%s] Failed to open MFC device",__func__);
        goto error_case1;
    }

    pCTX->hMFC = hMFCOpen;

    memset(&cap, 0, sizeof(cap));
    ret = ioctl(pCTX->hMFC, VIDIOC_QUERYCAP, &cap);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QUERYCAP failed",__func__);
        goto error_case2;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("[%s] Device does not support capture",__func__);
        goto error_case2;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        LOGE("[%s] Device does not support output",__func__);
        goto error_case2;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("[%s] Device does not support streaming",__func__);
        goto error_case2;
    }

    pCTX->inter_buff_status = MFC_USE_NONE;
    memset(&fmt, 0, sizeof(fmt));
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264; /* Default is set to H264 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = MAX_DECODER_INPUT_BUFFER_SIZE;

    ret = ioctl(pCTX->hMFC, VIDIOC_S_FMT, &fmt);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed",__func__);
        goto error_case2;
    }

    pCTX->v4l2_dec.mfc_src_bufs_len = MAX_DECODER_INPUT_BUFFER_SIZE;

    memset(&(reqbuf), 0, sizeof (reqbuf));
    reqbuf.count = MFC_DEC_NUM_SRC_BUFS;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(pCTX->hMFC, VIDIOC_REQBUFS, &reqbuf);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_REQBUFS failed",__func__);
        goto error_case2;
    }

    pCTX->v4l2_dec.mfc_num_src_bufs   = reqbuf.count;

    for (i = 0; i < pCTX->v4l2_dec.mfc_num_src_bufs; ++i) {
        memset(&(buf), 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.m.planes = planes;
        buf.length = 1;

        ret = ioctl(pCTX->hMFC, VIDIOC_QUERYBUF, &buf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QUERYBUF failed",__func__);
            goto error_case3;
        }

        pCTX->v4l2_dec.mfc_src_bufs[i] = mmap(NULL, buf.m.planes[0].length,
        PROT_READ | PROT_WRITE, MAP_SHARED, pCTX->hMFC, buf.m.planes[0].m.mem_offset);
        if (pCTX->v4l2_dec.mfc_src_bufs[i] == MAP_FAILED) {
            LOGE("[%s] mmap failed (%d)",__func__,i);
            goto error_case3;
        }
    }
    pCTX->inter_buff_status |= MFC_USE_STRM_BUFF;

    /* set extra DPB size to 5 as default for optimal performce (heuristic method) */
    pCTX->dec_numextradpb = DEFAULT_NUMBER_OF_EXTRA_DPB;

    pCTX->v4l2_dec.bBeingFinalized = 0;
    pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_NOT_RECEIVED;

    pCTX->cacheablebuffer = NO_CACHE;

    for (i = 0; i<MFC_DEC_NUM_SRC_BUFS; i++)
        pCTX->v4l2_dec.mfc_src_buf_flags[i] = BUF_DEQUEUED;

    pCTX->v4l2_dec.beingUsedIndex = 0;

    return (void *) pCTX;

error_case3:
    for (j = 0; j < i; j++)
        munmap(pCTX->v4l2_dec.mfc_src_bufs[j], pCTX->v4l2_dec.mfc_src_bufs_len);

error_case2:
    close(pCTX->hMFC);

error_case1:
    free(pCTX);

    return NULL;
}

void *SsbSipMfcDecOpenExt(void *value)
{
    _MFCLIB *pCTX;

    pCTX = SsbSipMfcDecOpen();

    if (pCTX == NULL)
        return NULL;

    if (NO_CACHE == (*(SSBIP_MFC_BUFFER_TYPE *)value)) {
        pCTX->cacheablebuffer = NO_CACHE;
        LOGI("[%s] non cacheable buffer",__func__);
    } else {
        pCTX->cacheablebuffer = CACHE;
        LOGI("[%s] cacheable buffer",__func__);
    }

    return (void *)pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecClose(void *openHandle)
{
    int ret, i;
    _MFCLIB  *pCTX;

    enum v4l2_buf_type type;
#ifdef CONFIG_MFC_FPS
    LOGI(">>> MFC");
    gettimeofday(&mTS2, NULL);
    LOGI(">>> time=%d", mTS2.tv_sec-mTS1.tv_sec);
    LOGI(">>> framecount=%d", framecount);
    LOGI(">>> 30ms over=%d", over30ms);
#endif
    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    if (pCTX->inter_buff_status & MFC_USE_DST_STREAMON) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ret = ioctl(pCTX->hMFC, VIDIOC_STREAMOFF, &type);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMOFF failed (destination buffers)",__func__);
            return MFC_RET_CLOSE_FAIL;
        }
        pCTX->inter_buff_status &= ~(MFC_USE_DST_STREAMON);
    }

    if (pCTX->inter_buff_status & MFC_USE_SRC_STREAMON) {
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        ret = ioctl(pCTX->hMFC, VIDIOC_STREAMOFF, &type);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMOFF failed (source buffers)",__func__);
            return MFC_RET_CLOSE_FAIL;
        }
        pCTX->inter_buff_status &= ~(MFC_USE_SRC_STREAMON);
    }

    if (pCTX->inter_buff_status & MFC_USE_STRM_BUFF) {
        for (i = 0; i < pCTX->v4l2_dec.mfc_num_src_bufs; i++)
            munmap(pCTX->v4l2_dec.mfc_src_bufs[i], pCTX->v4l2_dec.mfc_src_bufs_len);
        pCTX->inter_buff_status &= ~(MFC_USE_STRM_BUFF);
    }

    if (pCTX->inter_buff_status & MFC_USE_YUV_BUFF) {
        for (i = 0; i < pCTX->v4l2_dec.mfc_num_dst_bufs; i++) {
            munmap(pCTX->v4l2_dec.mfc_dst_bufs[i][0], pCTX->v4l2_dec.mfc_dst_bufs_len[0]);
            munmap(pCTX->v4l2_dec.mfc_dst_bufs[i][1], pCTX->v4l2_dec.mfc_dst_bufs_len[1]);
        }
        pCTX->inter_buff_status &= ~(MFC_USE_YUV_BUFF);
    }

    close(pCTX->hMFC);
    free(pCTX);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecInit(void *openHandle, SSBSIP_MFC_CODEC_TYPE codec_type, int Frameleng)
{
    int packedPB = 0;
    _MFCLIB *pCTX;
    int ret;
    unsigned int i, j;

    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    struct v4l2_format fmt;
    struct v4l2_pix_format_mplane pix_mp;
    struct v4l2_control ctrl;
    struct v4l2_crop crop;
    enum v4l2_buf_type type;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    pCTX->codecType = codec_type;

    if ((pCTX->codecType == MPEG4_DEC) || (pCTX->codecType == XVID_DEC) ||
    (pCTX->codecType == FIMV1_DEC) || (pCTX->codecType == FIMV2_DEC) ||
    (pCTX->codecType == FIMV3_DEC) || (pCTX->codecType == FIMV4_DEC))
        packedPB = isPBPacked(pCTX, Frameleng);

    memset(&fmt, 0, sizeof(fmt));

    switch (pCTX->codecType) {
    case H264_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
        break;
    case MPEG4_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG4;
        break;
    case H263_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H263;
        break;
    case XVID_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_XVID;
        break;
    case MPEG2_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG12;
        break;
    case FIMV1_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_FIMV1;
        fmt.fmt.pix_mp.width =  pCTX->fimv1_res.width;
        fmt.fmt.pix_mp.height = pCTX->fimv1_res.height;
        break;
    case FIMV2_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_FIMV2;
        break;
    case FIMV3_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_FIMV3;
        break;
    case FIMV4_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_FIMV4;
        break;
    case VC1_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_VC1;
        break;
    case VC1RCV_DEC:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_VC1_RCV;
        break;
    default:
        LOGE("[%s] Does NOT support the standard (%d)",__func__,pCTX->codecType);
        ret = MFC_RET_INVALID_PARAM;
        goto error_case1;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = MAX_DECODER_INPUT_BUFFER_SIZE;

    ret = ioctl(pCTX->hMFC, VIDIOC_S_FMT, &fmt);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    memset(&qbuf, 0, sizeof(qbuf));

    qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    qbuf.memory = V4L2_MEMORY_MMAP;
    qbuf.index = pCTX->v4l2_dec.beingUsedIndex;
    qbuf.m.planes = planes;
    qbuf.length = 1;
    qbuf.m.planes[0].bytesused = Frameleng;

    ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    // Processing the header requires running streamon
    // on OUTPUT queue
    ret = ioctl(pCTX->hMFC, VIDIOC_STREAMON, &type);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_STREAMON failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    pCTX->inter_buff_status |= MFC_USE_SRC_STREAMON;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    ret = ioctl(pCTX->hMFC, VIDIOC_G_FMT, &fmt);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_FMT failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    pix_mp = fmt.fmt.pix_mp;
    pCTX->decOutInfo.buf_width = pix_mp.plane_fmt[0].bytesperline;
    pCTX->decOutInfo.buf_height =
        pix_mp.plane_fmt[0].sizeimage / pix_mp.plane_fmt[0].bytesperline;

    pCTX->decOutInfo.img_width = pix_mp.width;
    pCTX->decOutInfo.img_height = pix_mp.height;

    memset(&crop, 0, sizeof(crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    ret = ioctl(pCTX->hMFC, VIDIOC_G_CROP, &crop);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_CROP failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    pCTX->decOutInfo.crop_left_offset = crop.c.left;
    pCTX->decOutInfo.crop_top_offset = crop.c.top;
    pCTX->decOutInfo.crop_right_offset =
        pix_mp.width - crop.c.width - crop.c.left;
    pCTX->decOutInfo.crop_bottom_offset =
        pix_mp.height - crop.c.height - crop.c.top;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_CODEC_REQ_NUM_BUFS;

    ret = ioctl(pCTX->hMFC, VIDIOC_G_CTRL, &ctrl);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_CTRL failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    pCTX->v4l2_dec.mfc_num_dst_bufs = ctrl.value + pCTX->dec_numextradpb;

    /* Cacheable buffer */
    ctrl.id = V4L2_CID_CACHEABLE;
    if(pCTX->cacheablebuffer == NO_CACHE)
        ctrl.value = 0;
    else
        ctrl.value = 1;

    ret = ioctl(pCTX->hMFC, VIDIOC_S_CTRL, &ctrl);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_S_CTRL failed, V4L2_CID_CACHEABLE",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count  = pCTX->v4l2_dec.mfc_num_dst_bufs;
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(pCTX->hMFC, VIDIOC_REQBUFS, &reqbuf);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_REQBUFS failed (destination buffers)",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    pCTX->v4l2_dec.mfc_num_dst_bufs  = reqbuf.count;

    for (i = 0;  i < pCTX->v4l2_dec.mfc_num_dst_bufs; ++i) {
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = i;
        qbuf.m.planes = planes;
        qbuf.length = 2;

        ret = ioctl(pCTX->hMFC, VIDIOC_QUERYBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QUERYBUF failed (destination buffers)",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case1;
        }

        pCTX->v4l2_dec.mfc_dst_bufs_len[0] = qbuf.m.planes[0].length;
        pCTX->v4l2_dec.mfc_dst_bufs_len[1] = qbuf.m.planes[1].length;

        pCTX->v4l2_dec.mfc_dst_phys[i][0] = qbuf.m.planes[0].cookie;
        pCTX->v4l2_dec.mfc_dst_phys[i][1] = qbuf.m.planes[1].cookie;

        pCTX->v4l2_dec.mfc_dst_bufs[i][0] = mmap(NULL, qbuf.m.planes[0].length,
             PROT_READ | PROT_WRITE, MAP_SHARED, pCTX->hMFC, qbuf.m.planes[0].m.mem_offset);

        if (pCTX->v4l2_dec.mfc_dst_bufs[i][0] == MAP_FAILED) {
            LOGE("[%s] mmap failed (destination buffers (Y))",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case2;
        }

        pCTX->v4l2_dec.mfc_dst_bufs[i][1] = mmap(NULL, qbuf.m.planes[1].length,
        PROT_READ | PROT_WRITE, MAP_SHARED, pCTX->hMFC, qbuf.m.planes[1].m.mem_offset);
        if (pCTX->v4l2_dec.mfc_dst_bufs[i][1] == MAP_FAILED) {
            LOGE("[%s] mmap failed (destination buffers (UV))",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case2;
        }

        ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case2;
        }
    }
    pCTX->inter_buff_status |= MFC_USE_YUV_BUFF;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = ioctl(pCTX->hMFC, VIDIOC_STREAMON, &type);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_STREAMON failed (destination buffers)",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }
    pCTX->inter_buff_status |= MFC_USE_DST_STREAMON;

    memset(&qbuf, 0, sizeof(qbuf));
    qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    qbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
    if(ret != 0) {
        LOGE("[%s] VIDIOC_DQBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
        }

    return MFC_RET_OK;

error_case2:
    for (j = 0; j < i; j++) {
        munmap(pCTX->v4l2_dec.mfc_dst_bufs[j][0], pCTX->v4l2_dec.mfc_dst_bufs_len[0]);
        munmap(pCTX->v4l2_dec.mfc_dst_bufs[j][1], pCTX->v4l2_dec.mfc_dst_bufs_len[1]);
    }
error_case1:
    SsbSipMfcDecClose(openHandle);
    return ret;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExe(void *openHandle, int lengthBufFill)
{
    _MFCLIB *pCTX;
    int ret;
    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    struct pollfd poll_events;
    int poll_state;

#ifdef CONFIG_MFC_FPS
    framecount++;
#endif
    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if ((lengthBufFill < 0) || (lengthBufFill > MAX_DECODER_INPUT_BUFFER_SIZE)) {
        LOGE("[%s] lengthBufFill is invalid. (lengthBufFill=%d)",__func__, lengthBufFill);
        return MFC_RET_INVALID_PARAM;
    }

#ifdef CONFIG_MFC_FPS
    gettimeofday(&mDec1, NULL);
#endif
    pCTX  = (_MFCLIB *) openHandle;

    /* note: #define POLLOUT 0x0004 */
    poll_events.fd = pCTX->hMFC;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    if ((lengthBufFill > 0) && (SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe)) {
        /* Queue the stream frame */
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = pCTX->v4l2_dec.beingUsedIndex;
        qbuf.m.planes = planes;
        qbuf.length = 1;
        qbuf.m.planes[0].bytesused = lengthBufFill;

        ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }

        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = 1;

        /* wait for decoding */
        do {
            poll_state = poll((struct pollfd*)&poll_events, 1, POLL_DEC_WAIT_TIMEOUT);
            if (0 < poll_state) {
                if (poll_events.revents & POLLOUT) { /* POLLOUT */
                    ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
                    if (ret == 0) {
                        if (qbuf.flags & V4L2_BUF_FLAG_ERROR)
                            return MFC_RET_DEC_EXE_ERR;
                        break;
                    }
                } else if (poll_events.revents & POLLERR) { /* POLLERR */
                    LOGE("[%s] POLLERR\n",__func__);
                    return MFC_RET_DEC_EXE_ERR;
                } else {
                    LOGE("[%s] poll() returns 0x%x\n",__func__, poll_events.revents);
                    return MFC_RET_DEC_EXE_ERR;
                }
            } else if (0 > poll_state) {
                return MFC_RET_DEC_EXE_ERR;
            }
        } while (0 == poll_state);

        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = MFC_DEC_NUM_PLANES;

        ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);

        if (ret != 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DECODING_ONLY;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return MFC_RET_OK;
        } else {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_DECODING;
        }

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][1];

        if (SSBSIP_MFC_LAST_FRAME_RECEIVED == pCTX->lastframe)
            pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;

    } else if(pCTX->v4l2_dec.bBeingFinalized == 0) {
        pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;

        /* Queue the stream frame */
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = pCTX->v4l2_dec.beingUsedIndex;
        qbuf.m.planes = planes;
        qbuf.length = 1;
        qbuf.m.planes[0].bytesused = 0;

        ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }

        pCTX->v4l2_dec.bBeingFinalized = 1; /* true */

        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = MFC_DEC_NUM_PLANES;
        /* FIXME
         wait for decoding */
        do {
            ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
        } while (ret != 0);

        pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][1];
    } else {
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = MFC_DEC_NUM_PLANES;

        ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);

        if (qbuf.m.planes[0].bytesused == 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_END;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return MFC_RET_OK;
        } else {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;
        }

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][1];
    }

    pCTX->decOutInfo.disp_pic_frame_type = (qbuf.flags & (0x7 << 3));

    switch (pCTX->decOutInfo.disp_pic_frame_type) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pCTX->decOutInfo.disp_pic_frame_type = 1;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pCTX->decOutInfo.disp_pic_frame_type = 2;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pCTX->decOutInfo.disp_pic_frame_type = 3;
        break;
    default:
        pCTX->decOutInfo.disp_pic_frame_type = 0;
        break;
    }

    ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);

#ifdef CONFIG_MFC_FPS
    gettimeofday(&mDec2, NULL);
    if (mDec2.tv_usec-mDec1.tv_usec > 30000) over30ms++;
#endif
    return MFC_RET_OK;
}

#if 0
SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExeNb(void *openHandle, int lengthBufFill)
{
    _MFCLIB *pCTX;
    int ret;

    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

#ifdef CONFIG_MFC_FPS
    framecount++;
#endif
    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if ((lengthBufFill < 0) || (lengthBufFill > MAX_DECODER_INPUT_BUFFER_SIZE)) {
        LOGE("[%s] lengthBufFill is invalid. (lengthBufFill=%d)",__func__, lengthBufFill);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    if ((lengthBufFill > 0) && (SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe)) {
        /* Queue the stream frame */
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = pCTX->v4l2_dec.beingUsedIndex;
        qbuf.m.planes = planes;
        qbuf.length = 1;
        qbuf.m.planes[0].bytesused = lengthBufFill;

        ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }
    } else if(pCTX->v4l2_dec.bBeingFinalized == 0) {
        /* Queue the stream frame */
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = pCTX->v4l2_dec.beingUsedIndex;
        qbuf.m.planes = planes;
        qbuf.length = 1;
        qbuf.m.planes[0].bytesused = 0;

        ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }
    }

    if ((SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe) && (lengthBufFill == 0))
        pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_RECEIVED;

    return MFC_RET_OK;
}

SSBSIP_MFC_DEC_OUTBUF_STATUS SsbSipMfcDecWaitForOutBuf(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;
    int ret;

    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    struct pollfd poll_events;
    int poll_state;

    pCTX  = (_MFCLIB *) openHandle;

    /* note: #define POLLOUT 0x0004 */
    poll_events.fd = pCTX->hMFC;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    if (SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe) {
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = 1;

        /* wait for decoding */
        do {
            poll_state = poll((struct pollfd*)&poll_events, 1, POLL_DEC_WAIT_TIMEOUT);
            if (0 < poll_state) {
                if (poll_events.revents & POLLOUT) { /* POLLOUT */
                    ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
                    if (ret == 0) {
                        if (qbuf.flags & V4L2_BUF_FLAG_ERROR)
                            return MFC_GETOUTBUF_STATUS_NULL;
                        break;
                    }
                } else if (poll_events.revents & POLLERR) { /* POLLERR */
                    LOGE("[%s] POLLERR\n",__func__);
                    return MFC_GETOUTBUF_STATUS_NULL;
                } else {
                    LOGE("[%s] poll() returns 0x%x\n",__func__, poll_events.revents);
                    return MFC_GETOUTBUF_STATUS_NULL;
                }
            } else if (0 > poll_state) {
                return MFC_GETOUTBUF_STATUS_NULL;
            }
        } while (0 == poll_state);

        pCTX->v4l2_dec.mfc_src_buf_flags[qbuf.index] = BUF_DEQUEUED;

        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = MFC_DEC_NUM_PLANES;

        ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);

        if (ret != 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DECODING_ONLY;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return SsbSipMfcDecGetOutBuf(pCTX, output_info);;
        } else {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_DECODING;
        }

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][1];

        if (SSBSIP_MFC_LAST_FRAME_RECEIVED == pCTX->lastframe)
            pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;
    } else if (pCTX->v4l2_dec.bBeingFinalized == 0) {
        pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;

        pCTX->v4l2_dec.bBeingFinalized = 1; /* true */

        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = MFC_DEC_NUM_PLANES;

        /* wait for decoding */
        do {
            ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);
        } while (ret != 0);

        pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][1];
    } else {
        memset(&qbuf, 0, sizeof(qbuf));
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = planes;
        qbuf.length = MFC_DEC_NUM_PLANES;

        ret = ioctl(pCTX->hMFC, VIDIOC_DQBUF, &qbuf);

        if (qbuf.m.planes[0].bytesused == 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_END;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return SsbSipMfcDecGetOutBuf(pCTX, output_info);;
        } else {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;
        }

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[qbuf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[qbuf.index][1];
    }

    pCTX->decOutInfo.disp_pic_frame_type = (qbuf.flags & (0x7 << 3));

    switch (pCTX->decOutInfo.disp_pic_frame_type) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pCTX->decOutInfo.disp_pic_frame_type = 1;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pCTX->decOutInfo.disp_pic_frame_type = 2;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pCTX->decOutInfo.disp_pic_frame_type = 3;
        break;
    default:
        pCTX->decOutInfo.disp_pic_frame_type = 0;
        break;
    }

    ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);

    return SsbSipMfcDecGetOutBuf(pCTX, output_info);
}
#endif

void  *SsbSipMfcDecGetInBuf(void *openHandle, void **phyInBuf, int inputBufferSize)
{
    _MFCLIB *pCTX;
    int i;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return NULL;
    }

    if ((inputBufferSize < 0) || (inputBufferSize > MAX_DECODER_INPUT_BUFFER_SIZE)) {
        LOGE("[%s] inputBufferSize = %d is invalid",__func__, inputBufferSize);
        return NULL;
    }

    pCTX  = (_MFCLIB *) openHandle;

    for (i = 0; i < MFC_DEC_NUM_SRC_BUFS; i++)
        if (BUF_DEQUEUED == pCTX->v4l2_dec.mfc_src_buf_flags[i])
            break;

    if (i == MFC_DEC_NUM_SRC_BUFS) {
        LOGV("[%s] No buffer is available.",__func__);
        return NULL;
    } else {
        pCTX->virStrmBuf = (unsigned int)pCTX->v4l2_dec.mfc_src_bufs[i];
        /* Set the buffer flag as Enqueued for NB_mode_process*/
        /* FIXME: Check this assignment in case of using New API ExeNb() */
        pCTX->v4l2_dec.mfc_src_buf_flags[i] = BUF_ENQUEUED;
    }

    return (void *)pCTX->virStrmBuf;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetInBuf(void *openHandle, void *phyInBuf, void *virInBuf, int size)
{
    _MFCLIB *pCTX;
    int i;

    LOGV("[%s] Enter",__func__);
    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    for (i = 0; i<MFC_DEC_NUM_SRC_BUFS; i++)
        if (pCTX->v4l2_dec.mfc_src_bufs[i] == virInBuf)
            break;

    if (i == MFC_DEC_NUM_SRC_BUFS) {
        LOGE("[%s] Can not use the buffer",__func__);
        return MFC_RET_INVALID_PARAM;
    } else {
        pCTX->virStrmBuf = (unsigned int)virInBuf;
        pCTX->v4l2_dec.beingUsedIndex = i;
        pCTX->v4l2_dec.mfc_src_buf_flags[i] = BUF_ENQUEUED;
    }
    LOGV("[%s] Exit idx %d",__func__,pCTX->v4l2_dec.beingUsedIndex);
    return MFC_RET_OK;
}

SSBSIP_MFC_DEC_OUTBUF_STATUS SsbSipMfcDecGetOutBuf(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info)
{
    int ret;
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
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

    output_info->crop_right_offset =  pCTX->decOutInfo.crop_right_offset;
    output_info->crop_left_offset =  pCTX->decOutInfo.crop_left_offset;
    output_info->crop_bottom_offset = pCTX->decOutInfo.crop_bottom_offset;
    output_info->crop_top_offset = pCTX->decOutInfo.crop_top_offset;

    output_info->disp_pic_frame_type = pCTX->decOutInfo.disp_pic_frame_type;

    switch (pCTX->displayStatus) {
    case MFC_GETOUTBUF_DISPLAY_ONLY:
    case MFC_GETOUTBUF_DISPLAY_DECODING:
    case MFC_GETOUTBUF_DISPLAY_END:
#ifdef SSB_UMP
        ret = ump_secure_id_get_from_vaddr(pCTX->decOutInfo.YVirAddr, &output_info->y_cookie);
        if (ret) {
            LOGV("[%s] fail to get secure id(%d) from vaddr(%x)\n",__func__, \
            output_info->y_cookie, pCTX->decOutInfo.YVirAddr);
        }

        ret = ump_secure_id_get_from_vaddr(pCTX->decOutInfo.CVirAddr, &output_info->c_cookie);
        if (ret) {
            LOGV("[%s] fail to get secure id(%d) from vaddr(%x)\n",__func__, \
            output_info->c_cookie, pCTX->decOutInfo.CVirAddr);
        }
        break;
#endif
    case MFC_GETOUTBUF_DECODING_ONLY:
    case MFC_GETOUTBUF_CHANGE_RESOL:
        break;
    default:
        return MFC_GETOUTBUF_DISPLAY_END;
    }

    return pCTX->displayStatus;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    int ret, i;

    _MFCLIB *pCTX;
    struct mfc_dec_fimv1_info *fimv1_res;

    struct v4l2_buffer qbuf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];
    struct v4l2_control ctrl;

    enum v4l2_buf_type type;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if ((value == NULL) && (MFC_DEC_SETCONF_IS_LAST_FRAME !=conf_type)) {
        LOGE("[%s] value is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *) openHandle;

    /* First, process non-ioctl calling settings */
    switch (conf_type) {
    case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
        pCTX->dec_numextradpb = *((unsigned int *) value);
        return MFC_RET_OK;

    case MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT: /* be set before calling SsbSipMfcDecInit */
         fimv1_res = (struct mfc_dec_fimv1_info *)value;
         LOGI("fimv1->width  = %d\n", fimv1_res->width);
         LOGI("fimv1->height = %d\n", fimv1_res->height);
         pCTX->fimv1_res.width  = (int)(fimv1_res->width);
         pCTX->fimv1_res.height = (int)(fimv1_res->height);
         return MFC_RET_OK;

    case MFC_DEC_SETCONF_IS_LAST_FRAME:
        if (SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe) {
            pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_RECEIVED;
            return MFC_RET_OK;
        } else {
            return MFC_RET_FAIL;
        }

    case MFC_DEC_SETCONF_DPB_FLUSH:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ret = ioctl(pCTX->hMFC, VIDIOC_STREAMOFF, &type);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMOFF failed (destination buffers)",__func__);
            return MFC_RET_DEC_SET_CONF_FAIL;
        }
        pCTX->inter_buff_status &= ~(MFC_USE_DST_STREAMON);

        for (i = 0;  i < pCTX->v4l2_dec.mfc_num_dst_bufs; ++i) {
            memset(&qbuf, 0, sizeof(qbuf));

            qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            qbuf.memory = V4L2_MEMORY_MMAP;
            qbuf.index = i;
            qbuf.m.planes = planes;
            qbuf.length = 2;

            ret = ioctl(pCTX->hMFC, VIDIOC_QBUF, &qbuf);
            if (ret != 0) {
                LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
                return MFC_RET_DEC_SET_CONF_FAIL;
            }
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ret = ioctl(pCTX->hMFC, VIDIOC_STREAMON, &type);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMON failed (destination buffers)",__func__);
            return MFC_RET_DEC_SET_CONF_FAIL;
        }
        pCTX->inter_buff_status |= MFC_USE_DST_STREAMON;
        return MFC_RET_OK;
    default:
        /* Others will be processed next */
        break;
    }

    /* Process ioctl calling settings */
    memset(&ctrl, 0, sizeof(ctrl));
    switch (conf_type) {
    case MFC_DEC_SETCONF_DISPLAY_DELAY: /* be set before calling SsbSipMfcDecInit */
        ctrl.id = V4L2_CID_CODEC_DISPLAY_DELAY;
        ctrl.value = *((unsigned int *) value);
        break;

    case MFC_DEC_SETCONF_CRC_ENABLE:
        ctrl.id = V4L2_CID_CODEC_CRC_ENABLE;
        ctrl.value = 1;
        break;

    case MFC_DEC_SETCONF_SLICE_ENABLE:
        ctrl.id = V4L2_CID_CODEC_SLICE_INTERFACE;
        ctrl.value = 1;
        break;

    case MFC_DEC_SETCONF_FRAME_TAG: /*be set before calling SsbSipMfcDecExe */
        ctrl.id = V4L2_CID_CODEC_FRAME_TAG;
        ctrl.value = *((unsigned int*)value);
        break;

    case MFC_DEC_SETCONF_POST_ENABLE:
        ctrl.id = V4L2_CID_CODEC_LOOP_FILTER_MPEG4_ENABLE;
        ctrl.value = *((unsigned int*)value);
        break;

    default:
        LOGE("[%s] conf_type(%d) is NOT supported",__func__, conf_type);
        return MFC_RET_INVALID_PARAM;
    }

    ret = ioctl(pCTX->hMFC, VIDIOC_S_CTRL, &ctrl);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_S_CTRL failed (conf_type = %d)",__func__, conf_type);
        return MFC_RET_DEC_SET_CONF_FAIL;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecGetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    _MFCLIB *pCTX;

    SSBSIP_MFC_IMG_RESOLUTION *img_resolution;
    int ret;
    SSBSIP_MFC_CRC_DATA *crc_data;
    SSBSIP_MFC_CROP_INFORMATION *crop_information;
    struct v4l2_control ctrl;

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("[%s] value is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *) openHandle;

    switch (conf_type) {
    case MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT:
        img_resolution = (SSBSIP_MFC_IMG_RESOLUTION *)value;
        img_resolution->width = pCTX->decOutInfo.img_width;
        img_resolution->height = pCTX->decOutInfo.img_height;
        img_resolution->buf_width = pCTX->decOutInfo.buf_width;
        img_resolution->buf_height = pCTX->decOutInfo.buf_height;
        break;

    case MFC_DEC_GETCONF_CRC_DATA:
        crc_data = (SSBSIP_MFC_CRC_DATA *) value;

        ctrl.id = V4L2_CID_CODEC_CRC_DATA_LUMA;
        ctrl.value = 0;

        ret = ioctl(pCTX->hMFC, VIDIOC_G_CTRL, &ctrl);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_G_CTRL failed, V4L2_CID_CODEC_CRC_DATA_LUMA",__func__);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }
        crc_data->luma0 = ctrl.value;

        ctrl.id = V4L2_CID_CODEC_CRC_DATA_CHROMA;
        ctrl.value = 0;

        ret = ioctl(pCTX->hMFC, VIDIOC_G_CTRL, &ctrl);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_G_CTRL failed, V4L2_CID_CODEC_CRC_DATA_CHROMA",__func__);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }
        crc_data->chroma0 = ctrl.value;

        LOGI("[%s] crc_data->luma0=%d",__func__,ctrl.value);
        LOGI("[%s] crc_data->chroma0=%d",__func__,ctrl.value);
        break;

    case MFC_DEC_GETCONF_FRAME_TAG:
        ctrl.id = V4L2_CID_CODEC_FRAME_TAG;
        ctrl.value = 0;

        ret = ioctl(pCTX->hMFC, VIDIOC_G_CTRL, &ctrl);
        if (ret != 0) {
            printf("Error to do g_ctrl.\n");
        }
        *((unsigned int *)value) = ctrl.value;
        break;

    case MFC_DEC_GETCONF_CROP_INFO:
        crop_information = (SSBSIP_MFC_CROP_INFORMATION *)value;
        crop_information->crop_top_offset = pCTX->decOutInfo.crop_top_offset;
        crop_information->crop_bottom_offset = pCTX->decOutInfo.crop_bottom_offset;
        crop_information->crop_left_offset = pCTX->decOutInfo.crop_left_offset;
        crop_information->crop_right_offset = pCTX->decOutInfo.crop_right_offset;
        break;

    default:
        LOGE("[%s] conf_type(%d) is NOT supported",__func__, conf_type);
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_OK;
}
