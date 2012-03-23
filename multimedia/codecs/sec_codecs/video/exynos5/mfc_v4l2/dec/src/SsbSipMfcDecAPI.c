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
#ifdef USE_ION
#include "ion.h"
#endif

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "MFC_DEC_APP"
#include <utils/Log.h>

/*#define CRC_ENABLE
#define SLICE_MODE_ENABLE */
#define POLL_DEC_WAIT_TIMEOUT 25

#define USR_DATA_START_CODE (0x000001B2)
#define VOP_START_CODE      (0x000001B6)
#define MP4_START_CODE      (0x000001)

#define DEFAULT_NUMBER_OF_EXTRA_DPB 5
#define CLEAR(x)    memset (&(x), 0, sizeof(x))
#ifdef S3D_SUPPORT
#define OPERATE_BIT(x, mask, shift)    ((x & (mask << shift)) >> shift)
#define FRAME_PACK_SEI_INFO_NUM  4
#endif

enum {
    NV12MT_FMT = 0,
    NV12M_FMT,
    NV21M_FMT,
};

static char *mfc_dev_name = SAMSUNG_MFC_DEV_NAME;
static int mfc_dev_node = 6;

int read_header_data(void *openHandle);
int init_mfc_output_stream(void *openHandle);
int isBreak_loop(void *openHandle);

int v4l2_mfc_querycap(int fd)
{
    struct v4l2_capability cap;
    int ret;

    CLEAR(cap);

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QUERYCAP failed", __func__);
        return ret;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("[%s] Device does not support capture", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        LOGE("[%s] Device does not support output", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("[%s] Device does not support streaming", __func__);
        return -1;
    }

    return 0;
}

int v4l2_mfc_s_fmt(int fd, enum v4l2_buf_type type,
                    int pixelformat, unsigned int sizeimage, int width, int height)
{
    int ret;
    struct v4l2_format fmt;

    CLEAR(fmt);

    fmt.type = type;

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        switch (pixelformat) {
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
            fmt.fmt.pix_mp.width = width;
            fmt.fmt.pix_mp.height = height;
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
#if defined (MFC6x_VERSION)
        case VP8_DEC:
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_VP8;
            break;
#endif
        default:
            LOGE("[%s] Does NOT support the codec type (%d)", __func__, pixelformat);
            return -1;
        }
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
    } else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        switch (pixelformat) {
        case NV12MT_FMT:
#if defined (MFC6x_VERSION)
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12MT_16X16;
#else
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12MT;
#endif
            break;
        case NV12M_FMT:
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
            break;
        case NV21M_FMT:
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21M;
            break;
        default:
            LOGE("[%s] Does NOT support the pixel format (%d)", __func__, pixelformat);
            return -1;
        }
    } else {
        LOGE("[%s] Wrong buffer type", __func__);
        return -1;
    }

    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);

    return ret;
}

int v4l2_mfc_reqbufs(int fd, enum v4l2_buf_type type, enum v4l2_memory memory, int *buf_cnt)
{
    struct v4l2_requestbuffers reqbuf;
    int ret;

    CLEAR(reqbuf);

    reqbuf.type = type;
    reqbuf.memory = memory;
    reqbuf.count = *buf_cnt;

    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
    *buf_cnt = reqbuf.count;

    return ret;
}

int v4l2_mfc_querybuf(int fd, struct v4l2_buffer *buf, enum v4l2_buf_type type,
                        enum v4l2_memory memory, int index, struct v4l2_plane *planes)
{
    int length = -1, ret;

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
        length = 1;
    else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        length = 2;

    CLEAR(*buf);
    buf->type = type;
    buf->memory = memory;
    buf->index = index;
    buf->m.planes = planes;
    buf->length = length;

    ret = ioctl(fd, VIDIOC_QUERYBUF, buf);

    return ret;
}

int v4l2_mfc_streamon(int fd, enum v4l2_buf_type type)
{
    int ret;

    ret = ioctl(fd, VIDIOC_STREAMON, &type);

    return ret;
}

int v4l2_mfc_streamoff(int fd, enum v4l2_buf_type type)
{
    int ret;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);

    return ret;
}

int v4l2_mfc_s_ctrl(int fd, int id, int value)
{
    struct v4l2_control ctrl;
    int ret;

    CLEAR(ctrl);
    ctrl.id = id;
    ctrl.value = value;

    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);

    return ret;
}

int v4l2_mfc_g_ctrl(int fd, int id, int *value)
{
    struct v4l2_control ctrl;
    int ret;

    CLEAR(ctrl);
    ctrl.id = id;

    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    *value = ctrl.value;

    return ret;
}

#ifdef S3D_SUPPORT
int v4l2_mfc_ext_g_ctrl(int fd, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    struct v4l2_ext_control ext_ctrl[FRAME_PACK_SEI_INFO_NUM];
    struct v4l2_ext_controls ext_ctrls;
    struct mfc_frame_pack_sei_info *sei_info;
    int ret, i;

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_CODEC;

    switch (conf_type) {
    case MFC_DEC_GETCONF_FRAME_PACKING:
        sei_info = (struct mfc_frame_pack_sei_info *)value;
        for (i=0; i<FRAME_PACK_SEI_INFO_NUM; i++)
            CLEAR(ext_ctrl[i]);

        ext_ctrls.count = FRAME_PACK_SEI_INFO_NUM;
        ext_ctrls.controls = ext_ctrl;
        ext_ctrl[0].id =  V4L2_CID_CODEC_FRAME_PACK_SEI_AVAIL;
        ext_ctrl[1].id =  V4L2_CID_CODEC_FRAME_PACK_ARRGMENT_ID;
        ext_ctrl[2].id =  V4L2_CID_CODEC_FRAME_PACK_SEI_INFO;
        ext_ctrl[3].id =  V4L2_CID_CODEC_FRAME_PACK_GRID_POS;

        ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls);

        sei_info->sei_avail = ext_ctrl[0].value;
        sei_info->arrgment_id = ext_ctrl[1].value;
        sei_info->sei_info = ext_ctrl[2].value;
        sei_info->grid_pos = ext_ctrl[3].value;
        break;
    }

    return ret;
}
#endif

int v4l2_mfc_qbuf(int fd, struct v4l2_buffer *qbuf, enum v4l2_buf_type type,
        enum v4l2_memory memory, int index,
        struct v4l2_plane *planes, int frame_length)
{
    int ret, length = 0;

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        CLEAR(*qbuf);
        length = 1;
    } else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        length = 2;
    }

    qbuf->type = type;
    qbuf->memory = memory;
    qbuf->index = index;
    qbuf->m.planes = planes;
    qbuf->length = length;
    qbuf->m.planes[0].bytesused = frame_length;

    ret = ioctl(fd, VIDIOC_QBUF, qbuf);

    return ret;
}

int v4l2_mfc_dqbuf(int fd, struct v4l2_buffer *dqbuf, enum v4l2_buf_type type,
                    enum v4l2_memory memory)
{
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];
    int ret, length = 0;

    CLEAR(*dqbuf);
    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
        length = 1;
    else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        length = 2;

    dqbuf->type = type;
    dqbuf->memory = memory;
    dqbuf->m.planes = planes;
    dqbuf->length = length;

    ret = ioctl(fd, VIDIOC_DQBUF, dqbuf);

    return ret;
}

int v4l2_mfc_g_fmt(int fd, struct v4l2_format *fmt, enum v4l2_buf_type type)
{
    int ret;

    CLEAR(*fmt);
    fmt->type = type;
    ret = ioctl(fd, VIDIOC_G_FMT, fmt);

    return ret;
}

int v4l2_mfc_g_crop(int fd, struct v4l2_crop *crop, enum v4l2_buf_type type)
{
    int ret;

    CLEAR(*crop);
    crop->type = type;
    ret = ioctl(fd, VIDIOC_G_CROP, crop);

    return ret;
}

int v4l2_mfc_poll(int fd, int *revents, int timeout)
{
    struct pollfd poll_events;
    int ret;

    poll_events.fd = fd;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    ret = poll((struct pollfd*)&poll_events, 1, timeout);
    *revents = poll_events.revents;

    return ret;
}

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
    int req_count;
    unsigned int i, j;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    LOGI("[%s] MFC Library Ver %d.%02d",__func__, MFC_LIB_VER_MAJOR, MFC_LIB_VER_MINOR);

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

    ret = v4l2_mfc_querycap(pCTX->hMFC);
    if (ret != 0) {
        LOGE("[%s] QUERYCAP failed", __func__);
        goto error_case2;
    }

    pCTX->inter_buff_status = MFC_USE_NONE;
    ret = v4l2_mfc_s_fmt(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                                H264_DEC, MAX_DECODER_INPUT_BUFFER_SIZE, 0, 0);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed",__func__);
        goto error_case2;
    }

    pCTX->v4l2_dec.mfc_src_bufs_len = MAX_DECODER_INPUT_BUFFER_SIZE;

    req_count = MFC_DEC_NUM_SRC_BUFS;
    ret = v4l2_mfc_reqbufs(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                            V4L2_MEMORY_MMAP, &req_count);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_REQBUFS failed",__func__);
        goto error_case2;
    }

    pCTX->v4l2_dec.mfc_num_src_bufs = req_count;

    for (i = 0; i < pCTX->v4l2_dec.mfc_num_src_bufs; ++i) {
        ret = v4l2_mfc_querybuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                                V4L2_MEMORY_MMAP, i, planes);
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

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    if (pCTX->inter_buff_status & MFC_USE_DST_STREAMON) {
        ret = v4l2_mfc_streamoff(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMOFF failed (destination buffers)",__func__);
            return MFC_RET_CLOSE_FAIL;
        }
        pCTX->inter_buff_status &= ~(MFC_USE_DST_STREAMON);
    }

    if (pCTX->inter_buff_status & MFC_USE_SRC_STREAMON) {
        ret = v4l2_mfc_streamoff(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
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

#ifdef USE_ION
    ion_client_destroy(pCTX->ion_fd);
#endif
    close(pCTX->hMFC);
    free(pCTX);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecInit(void *openHandle, SSBSIP_MFC_CODEC_TYPE codec_type, int Frameleng)
{
    int packedPB = 0;
    _MFCLIB *pCTX;
    int ret;

    int width, height;
    int ctrl_value;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    int poll_state, poll_revents;

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

    if (pCTX->codecType == FIMV1_DEC) {
        width = pCTX->fimv1_res.width;
        height = pCTX->fimv1_res.height;
    } else {
        width = 0;
        height = 0;
    }
    ret = v4l2_mfc_s_fmt(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, pCTX->codecType,
                            MAX_DECODER_INPUT_BUFFER_SIZE, width, height);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed", __func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    /* Set default destination format as NV12MT */
    ret = v4l2_mfc_s_fmt(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, NV12MT_FMT,
                            0, width, height);
    if (ret != 0) {
        LOGE("[%s] S_FMT failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    /* PackedPB should be set after VIDIOC_S_FMT */
    if (packedPB) {
        ret = v4l2_mfc_s_ctrl(pCTX->hMFC, V4L2_CID_CODEC_PACKED_PB, 1);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_S_CTRL failed of PACKED_PB\n", __func__);
            return MFC_RET_DEC_SET_CONF_FAIL;
        }
    }

    ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                    V4L2_MEMORY_MMAP, pCTX->v4l2_dec.beingUsedIndex, planes, Frameleng);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    /* Processing the header requires running streamon
     on OUTPUT queue */
    ret = v4l2_mfc_streamon(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_STREAMON failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }
    pCTX->inter_buff_status |= MFC_USE_SRC_STREAMON;

    ret = read_header_data(pCTX);
    if (ret != 0)
        goto error_case1;

    /* cacheable buffer */
    if (pCTX->cacheablebuffer == NO_CACHE)
        ctrl_value = 0;
    else
        ctrl_value = 1;

    ret = v4l2_mfc_s_ctrl(pCTX->hMFC, V4L2_CID_CACHEABLE, ctrl_value);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_S_CTRL failed, V4L2_CID_CACHEABLE", __func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

#ifdef USE_ION
    pCTX->ion_fd = ion_client_create();
    if (pCTX->ion_fd < 3) {
        LOGE("[%s] Failed to get ion_fd : %d", __func__, pCTX->ion_fd);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }
#endif

    ret = init_mfc_output_stream(pCTX);
    if (ret != 0)
        goto error_case1;

    ret = v4l2_mfc_streamon(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_STREAMON failed (destination buffers)", __func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }
    pCTX->inter_buff_status |= MFC_USE_DST_STREAMON;

    do {
        poll_state = v4l2_mfc_poll(pCTX->hMFC, &poll_revents, POLL_DEC_WAIT_TIMEOUT);
        if (poll_state > 0) {
            if (poll_revents & POLLOUT) {
                ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP);
                if (ret == 0)
                    break;
            } else if (poll_revents & POLLERR) {
                LOGE("[%s] POLLERR\n", __func__);
                return MFC_GETOUTBUF_STATUS_NULL;
            } else {
                LOGE("[%s] poll() returns 0x%x\n", __func__, poll_revents);
                return MFC_GETOUTBUF_STATUS_NULL;
            }
        } else if (poll_state < 0) {
            return MFC_GETOUTBUF_STATUS_NULL;
        }
    } while (poll_state == 0);

    return MFC_RET_OK;

error_case1:
    SsbSipMfcDecClose(openHandle);
    return ret;
}

int read_header_data(void *openHandle)
{
    struct v4l2_format fmt;
    struct v4l2_crop crop;
    struct v4l2_pix_format_mplane pix_mp;
    int ctrl_value;
    int ret;

    _MFCLIB *pCTX;
    pCTX  = (_MFCLIB *) openHandle;

    ret = v4l2_mfc_g_fmt(pCTX->hMFC, &fmt, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_FMT failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case;
    }

    pix_mp = fmt.fmt.pix_mp;
    pCTX->decOutInfo.buf_width = pix_mp.plane_fmt[0].bytesperline;
    pCTX->decOutInfo.buf_height =
        pix_mp.plane_fmt[0].sizeimage / pix_mp.plane_fmt[0].bytesperline;

    pCTX->decOutInfo.img_width = pix_mp.width;
    pCTX->decOutInfo.img_height = pix_mp.height;

    ret = v4l2_mfc_g_crop(pCTX->hMFC, &crop, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_CROP failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case;
    }

    pCTX->decOutInfo.crop_left_offset = crop.c.left;
    pCTX->decOutInfo.crop_top_offset = crop.c.top;
    pCTX->decOutInfo.crop_right_offset =
        pix_mp.width - crop.c.width - crop.c.left;
    pCTX->decOutInfo.crop_bottom_offset =
        pix_mp.height - crop.c.height - crop.c.top;

    ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_REQ_NUM_BUFS, &ctrl_value);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_CTRL failed",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case;
    }

    pCTX->v4l2_dec.mfc_num_dst_bufs = ctrl_value + pCTX->dec_numextradpb;

    LOGV("[%s] Num of allocated buffers: %d\n",__func__, pCTX->v4l2_dec.mfc_num_dst_bufs);

    return 0;

error_case:
    return ret;
    }

/* Initialize output stream of MFC */
int init_mfc_output_stream(void *openHandle)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];
    int ret;
    int i, j;
    _MFCLIB *pCTX;
    pCTX  = (_MFCLIB *) openHandle;

    ret = v4l2_mfc_reqbufs(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
                    V4L2_MEMORY_MMAP, (int *)&pCTX->v4l2_dec.mfc_num_dst_bufs);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_REQBUFS failed (destination buffers)",__func__);
        ret = MFC_RET_DEC_INIT_FAIL;
        goto error_case1;
    }

    for (i = 0;  i < pCTX->v4l2_dec.mfc_num_dst_bufs; ++i) {
        ret = v4l2_mfc_querybuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
                        V4L2_MEMORY_MMAP, i, planes);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QUERYBUF failed (destination buffers)",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case1;
        }

        pCTX->v4l2_dec.mfc_dst_bufs_len[0] = buf.m.planes[0].length;
        pCTX->v4l2_dec.mfc_dst_bufs_len[1] = buf.m.planes[1].length;

        pCTX->v4l2_dec.mfc_dst_phys[i][0] = buf.m.planes[0].cookie;
        pCTX->v4l2_dec.mfc_dst_phys[i][1] = buf.m.planes[1].cookie;

#ifdef USE_ION
        pCTX->dst_ion_fd[i][0] = (int)buf.m.planes[0].share;
        pCTX->dst_ion_fd[i][1] = (int)buf.m.planes[1].share;

        pCTX->v4l2_dec.mfc_dst_bufs[i][0] =
            ion_map(pCTX->dst_ion_fd[i][0],pCTX->v4l2_dec.mfc_dst_bufs_len[0],0);
        pCTX->v4l2_dec.mfc_dst_bufs[i][1] =
            ion_map(pCTX->dst_ion_fd[i][1],pCTX->v4l2_dec.mfc_dst_bufs_len[1],0);
        if (pCTX->v4l2_dec.mfc_dst_bufs[i][0] == MAP_FAILED ||
            pCTX->v4l2_dec.mfc_dst_bufs[i][0] == MAP_FAILED)
            goto error_case2;
#else
        pCTX->v4l2_dec.mfc_dst_bufs[i][0] = mmap(NULL, buf.m.planes[0].length,
             PROT_READ | PROT_WRITE, MAP_SHARED, pCTX->hMFC, buf.m.planes[0].m.mem_offset);
        if (pCTX->v4l2_dec.mfc_dst_bufs[i][0] == MAP_FAILED) {
            LOGE("[%s] mmap failed (destination buffers (Y))",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case2;
        }

        pCTX->v4l2_dec.mfc_dst_bufs[i][1] = mmap(NULL, buf.m.planes[1].length,
        PROT_READ | PROT_WRITE, MAP_SHARED, pCTX->hMFC, buf.m.planes[1].m.mem_offset);
        if (pCTX->v4l2_dec.mfc_dst_bufs[i][1] == MAP_FAILED) {
            LOGE("[%s] mmap failed (destination buffers (UV))",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case2;
        }
#endif

        ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
                        V4L2_MEMORY_MMAP, i, planes, 0);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
            ret = MFC_RET_DEC_INIT_FAIL;
            goto error_case2;
        }
    }
    pCTX->inter_buff_status |= MFC_USE_YUV_BUFF;

    return 0;

error_case2:
    for (j = 0; j < i; j++) {
        munmap(pCTX->v4l2_dec.mfc_dst_bufs[j][0], pCTX->v4l2_dec.mfc_dst_bufs_len[0]);
        munmap(pCTX->v4l2_dec.mfc_dst_bufs[j][1], pCTX->v4l2_dec.mfc_dst_bufs_len[1]);
    }
error_case1:
    return ret;
}

int resolution_change(void *openHandle)
{
    int i, ret;
    int req_count;
    _MFCLIB *pCTX;
    pCTX  = (_MFCLIB *) openHandle;

    ret = v4l2_mfc_streamoff(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret != 0)
        goto error_case;

    pCTX->inter_buff_status &= ~(MFC_USE_DST_STREAMON);

    for (i = 0; i < pCTX->v4l2_dec.mfc_num_dst_bufs; i++) {
        munmap(pCTX->v4l2_dec.mfc_dst_bufs[i][0], pCTX->v4l2_dec.mfc_dst_bufs_len[0]);
        munmap(pCTX->v4l2_dec.mfc_dst_bufs[i][1], pCTX->v4l2_dec.mfc_dst_bufs_len[1]);
    }
    pCTX->inter_buff_status &= ~(MFC_USE_YUV_BUFF);

    req_count = 0;
    ret = v4l2_mfc_reqbufs(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
                    V4L2_MEMORY_MMAP, &req_count);
    if (ret != 0)
        goto error_case;

    read_header_data(pCTX);
    init_mfc_output_stream(pCTX);

    ret = v4l2_mfc_streamon(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret != 0)
        goto error_case;
    pCTX->inter_buff_status |= MFC_USE_DST_STREAMON;

    return 0;

error_case:
    return ret;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExe(void *openHandle, int lengthBufFill)
{
    _MFCLIB *pCTX;
    int ret;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];
    int loop_count, ctrl_value;

    int poll_state;
    int poll_revents;

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
        if (pCTX->displayStatus != MFC_GETOUTBUF_DISPLAY_ONLY) {
            /* Queue the stream frame */
            ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                                V4L2_MEMORY_MMAP, pCTX->v4l2_dec.beingUsedIndex, planes, lengthBufFill);
            if (ret != 0) {
                LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
                return MFC_RET_DEC_EXE_ERR;
            }

            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.m.planes = planes;
            buf.length = 1;

            /* wait for decoding */
            do {
                poll_state = v4l2_mfc_poll(pCTX->hMFC, &poll_revents, POLL_DEC_WAIT_TIMEOUT);
                if (poll_state > 0) {
                    if (poll_revents & POLLOUT) {
                        ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP);
                        if (ret == 0) {
                            if (buf.flags & V4L2_BUF_FLAG_ERROR)
                                return MFC_RET_DEC_EXE_ERR;
                            pCTX->v4l2_dec.mfc_src_buf_flags[buf.index] = BUF_DEQUEUED;
                            break;
                        }
                    } else if (poll_revents & POLLERR) {
                        LOGE("[%s] POLLERR\n",__func__);
                        return MFC_RET_DEC_EXE_ERR;
                    } else {
                        LOGE("[%s] poll() returns 0x%x\n", __func__, poll_revents);
                        return MFC_RET_DEC_EXE_ERR;
                    }
                } else if (poll_state < 0) {
                    return MFC_RET_DEC_EXE_ERR;
                }

                if (isBreak_loop(pCTX))
                    break;

            } while(0 == poll_state);
        }

        ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP);
        if (ret != 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DECODING_ONLY;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return MFC_RET_OK;
        } else {
            ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_DISPLAY_STATUS, &ctrl_value);
            if (ret != 0) {
                LOGE("[%s] VIDIOC_G_CTRL failed", __func__);
                return MFC_RET_DEC_GET_CONF_FAIL;
            }

            switch (ctrl_value) {
            case 0:
                pCTX->displayStatus = MFC_GETOUTBUF_DECODING_ONLY;
                break;
            case 1:
                pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_DECODING;
                break;
            case 2:
                pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;
                break;
            case 3:
                pCTX->displayStatus = MFC_GETOUTBUF_CHANGE_RESOL;
                break;
            }
        }

        if (pCTX->displayStatus == MFC_GETOUTBUF_CHANGE_RESOL) {
            resolution_change(pCTX);
        } else {
            pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][0];
            pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][1];

            pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][0];
            pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][1];
        }

        if (SSBSIP_MFC_LAST_FRAME_RECEIVED == pCTX->lastframe)
            pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;

    } else if (pCTX->v4l2_dec.bBeingFinalized == 0) {
        pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;
        pCTX->v4l2_dec.bBeingFinalized = 1; /* true */

        /* Queue the stream frame */
        ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                            V4L2_MEMORY_MMAP, pCTX->v4l2_dec.beingUsedIndex, planes, 0);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }

        /* wait for decoding */
        loop_count = 0;
        do {
            ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP);
            if (ret != 0)
                usleep(1000);
            loop_count++;
            if (loop_count >= 1000) {
                LOGE("[%s] Error in do-while loop",__func__);
                break;
            }
        } while (ret != 0);

        ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_DISPLAY_STATUS, &ctrl_value);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_G_CTRL failed", __func__);
            return MFC_RET_DEC_EXE_ERR;
        }
        if (ctrl_value == 3) {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_END;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return MFC_RET_OK;
        }

        pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][1];
    } else {
        loop_count = 0;
        do {
            ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP);
            if (ret != 0)
                usleep(1000);
            loop_count++;
            if (loop_count >= 1000) {
                LOGE("[%s] Error in do-while loop",__func__);
                break;
            }
        } while (ret != 0);

        if (buf.m.planes[0].bytesused == 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_END;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return MFC_RET_OK;
        } else {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;
        }

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][1];
    }

    pCTX->decOutInfo.disp_pic_frame_type = (buf.flags & (0x7 << 3));

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

    ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP,
                        buf.index, planes, 0);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExeNb(void *openHandle, int lengthBufFill)
{
    _MFCLIB *pCTX;
    int ret;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    if (openHandle == NULL) {
        LOGE("[%s] openHandle is NULL",__func__);
        return MFC_RET_INVALID_PARAM;
    }

    if ((lengthBufFill < 0) || (lengthBufFill > MAX_DECODER_INPUT_BUFFER_SIZE)) {
        LOGE("[%s] lengthBufFill is invalid. (lengthBufFill=%d)",__func__, lengthBufFill);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *) openHandle;

    if ((lengthBufFill > 0) && (SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe)
                            && (pCTX->displayStatus != MFC_GETOUTBUF_DISPLAY_ONLY)) {
        /* Queue the stream frame */
        ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                            V4L2_MEMORY_MMAP, pCTX->v4l2_dec.beingUsedIndex, planes, lengthBufFill);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }
    } else if (pCTX->v4l2_dec.bBeingFinalized == 0) {
        /* Queue the stream frame */
        ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
                            V4L2_MEMORY_MMAP, pCTX->v4l2_dec.beingUsedIndex, planes, 0);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",__func__);
            return MFC_RET_DEC_EXE_ERR;
        }
    }

    if ((SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe) && (lengthBufFill == 0))
        pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_RECEIVED;

    return MFC_RET_OK;
}

int isBreak_loop(void *openHandle)
{
    _MFCLIB *pCTX;
    pCTX  = (_MFCLIB *) openHandle;
    int ctrl_value;
    int ret = 0;

    if (pCTX->displayStatus == MFC_GETOUTBUF_DISPLAY_ONLY)
        return 1;

    ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_CHECK_STATE, &ctrl_value);
    if (ret != 0) {
        LOGE("[%s] VIDIOC_G_CTRL failed", __func__);
        return 0;
    }

    if (ctrl_value == MFCSTATE_DEC_RES_DETECT) {
        LOGV("[%s] Resolution Change detect",__func__);
        return 1;
    } else if (ctrl_value == MFCSTATE_DEC_TERMINATING) {
        LOGV("[%s] Decoding Finish!!!",__func__);
        return 1;
    }

    return 0;
}

SSBSIP_MFC_DEC_OUTBUF_STATUS SsbSipMfcDecWaitForOutBuf(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;
    int ret;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];
    int loop_count, ctrl_value;

    int poll_state;
    int poll_revents;

    pCTX  = (_MFCLIB *) openHandle;

    if (SSBSIP_MFC_LAST_FRAME_PROCESSED != pCTX->lastframe) {
        if (pCTX->displayStatus != MFC_GETOUTBUF_DISPLAY_ONLY) {
            /* wait for decoding */
            do {
                poll_state = v4l2_mfc_poll(pCTX->hMFC, &poll_revents, POLL_DEC_WAIT_TIMEOUT);
                if (poll_state > 0) {
                    if (poll_revents & POLLOUT) {
                        buf.m.planes = planes;
                        buf.length = 1;
                        ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP);
                        if (ret == 0) {
                            if (buf.flags & V4L2_BUF_FLAG_ERROR)
                                return MFC_GETOUTBUF_STATUS_NULL;
                            pCTX->v4l2_dec.mfc_src_buf_flags[buf.index] = BUF_DEQUEUED;
                            break;
                        }
                    } else if (poll_revents & POLLERR) {
                        LOGE("[%s] POLLERR\n",__func__);
                        return MFC_GETOUTBUF_STATUS_NULL;
                    } else {
                        LOGE("[%s] poll() returns 0x%x\n", __func__, poll_revents);
                        return MFC_GETOUTBUF_STATUS_NULL;
                    }
                } else if (poll_state < 0) {
                    return MFC_GETOUTBUF_STATUS_NULL;
                }

                if (isBreak_loop(pCTX))
                    break;

            } while (0 == poll_state);
        }

        ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP);
        if (ret != 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DECODING_ONLY;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return SsbSipMfcDecGetOutBuf(pCTX, output_info);;
        } else {
            ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_DISPLAY_STATUS, &ctrl_value);
            if (ret != 0) {
                LOGE("[%s] VIDIOC_G_CTRL failed", __func__);
                return MFC_RET_DEC_GET_CONF_FAIL;
            }

            switch (ctrl_value) {
            case 0:
                pCTX->displayStatus = MFC_GETOUTBUF_DECODING_ONLY;
                break;
            case 1:
                pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_DECODING;
                break;
            case 2:
                pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;
                break;
            case 3:
                pCTX->displayStatus = MFC_GETOUTBUF_CHANGE_RESOL;
                break;
            }
        }

        if (pCTX->displayStatus == MFC_GETOUTBUF_CHANGE_RESOL) {
            resolution_change(pCTX);
        } else {
            pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][0];
            pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][1];

            pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][0];
            pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][1];
        }

        if (SSBSIP_MFC_LAST_FRAME_RECEIVED == pCTX->lastframe)
            pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;
    } else if (pCTX->v4l2_dec.bBeingFinalized == 0) {
        pCTX->lastframe = SSBSIP_MFC_LAST_FRAME_PROCESSED;
        pCTX->v4l2_dec.bBeingFinalized = 1; /* true */

        /* wait for decoding */
        loop_count = 0;
        do {
            ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP);
            if (ret != 0)
                usleep(1000);
            loop_count++;
            if (loop_count >= 1000) {
                LOGE("[%s] Error in do-while loop",__func__);
                break;
            }
        } while (ret != 0);

        ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_DISPLAY_STATUS, &ctrl_value);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_G_CTRL failed", __func__);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }
        if (ctrl_value == 3) {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_END;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return SsbSipMfcDecGetOutBuf(pCTX, output_info);;
        }

        pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][1];
    } else {
        loop_count = 0;
        do {
            ret = v4l2_mfc_dqbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP);
            if (ret != 0)
                usleep(1000);
            loop_count++;
            if (loop_count >= 1000) {
                LOGE("[%s] Error in do-while loop",__func__);
                break;
            }
        } while (ret != 0);

        if (buf.m.planes[0].bytesused == 0) {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_END;
            pCTX->decOutInfo.disp_pic_frame_type = -1;
            return SsbSipMfcDecGetOutBuf(pCTX, output_info);;
        } else {
            pCTX->displayStatus = MFC_GETOUTBUF_DISPLAY_ONLY;
        }

        pCTX->decOutInfo.YVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][0];
        pCTX->decOutInfo.CVirAddr = pCTX->v4l2_dec.mfc_dst_bufs[buf.index][1];

        pCTX->decOutInfo.YPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][0];
        pCTX->decOutInfo.CPhyAddr = (unsigned int)pCTX->v4l2_dec.mfc_dst_phys[buf.index][1];
    }

    pCTX->decOutInfo.disp_pic_frame_type = (buf.flags & (0x7 << 3));

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

    ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP,
                        buf.index, planes, 0);

    return SsbSipMfcDecGetOutBuf(pCTX, output_info);
}

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
        pCTX->v4l2_dec.beingUsedIndex = i;
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

    for (i = 0; i < MFC_DEC_NUM_SRC_BUFS; i++) {
        if (pCTX->v4l2_dec.mfc_src_bufs[i] == virInBuf)
            break;
    }

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

    struct v4l2_buffer buf;
    struct v4l2_plane planes[MFC_DEC_NUM_PLANES];

    int id, ctrl_value;

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
        ret = v4l2_mfc_streamoff(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_STREAMOFF failed (destination buffers)",__func__);
            return MFC_RET_DEC_SET_CONF_FAIL;
        }
        pCTX->inter_buff_status &= ~(MFC_USE_DST_STREAMON);

        for (i = 0;  i < pCTX->v4l2_dec.mfc_num_dst_bufs; ++i) {
            ret = v4l2_mfc_qbuf(pCTX->hMFC, &buf, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
                        V4L2_MEMORY_MMAP, i, planes, 0);
            if (ret != 0) {
                LOGE("[%s] VIDIOC_QBUF failed, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",__func__);
                return MFC_RET_DEC_SET_CONF_FAIL;
            }
        }

        ret = v4l2_mfc_streamon(pCTX->hMFC, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
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
    switch (conf_type) {
    case MFC_DEC_SETCONF_DISPLAY_DELAY: /* be set before calling SsbSipMfcDecInit */
        id = V4L2_CID_CODEC_DISPLAY_DELAY;
        ctrl_value = *((unsigned int *) value);
        break;

    case MFC_DEC_SETCONF_CRC_ENABLE:
        id = V4L2_CID_CODEC_CRC_ENABLE;
        ctrl_value = 1;
        break;

    case MFC_DEC_SETCONF_SLICE_ENABLE:
        id = V4L2_CID_CODEC_SLICE_INTERFACE;
        ctrl_value = 1;
        break;

    case MFC_DEC_SETCONF_FRAME_TAG: /*be set before calling SsbSipMfcDecExe */
        id = V4L2_CID_CODEC_FRAME_TAG;
        ctrl_value = *((unsigned int*)value);
        break;

    case MFC_DEC_SETCONF_POST_ENABLE:
        id = V4L2_CID_CODEC_LOOP_FILTER_MPEG4_ENABLE;
        ctrl_value = *((unsigned int*)value);
        break;
#ifdef S3D_SUPPORT
    case MFC_DEC_SETCONF_SEI_PARSE:
        id = V4L2_CID_CODEC_FRAME_PACK_SEI_PARSE;
        ctrl_value = 1;
        break;
#endif
    default:
        LOGE("[%s] conf_type(%d) is NOT supported",__func__, conf_type);
        return MFC_RET_INVALID_PARAM;
    }

    ret = v4l2_mfc_s_ctrl(pCTX->hMFC, id, ctrl_value);
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
#ifdef S3D_SUPPORT
    SSBSIP_MFC_FRAME_PACKING *frame_packing;
    struct mfc_frame_pack_sei_info sei_info;
#endif
    SSBSIP_MFC_CROP_INFORMATION *crop_information;

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

    case MFC_DEC_GETCONF_FRAME_TAG:
        ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_FRAME_TAG, (int*)value);
        if (ret != 0)
            LOGE("[%s] VIDIOC_G_CTRL failed, V4L2_CID_CODEC_FRAME_TAG", __func__);
        break;

    case MFC_DEC_GETCONF_CRC_DATA:
        crc_data = (SSBSIP_MFC_CRC_DATA *) value;

        ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_CRC_DATA_LUMA, &crc_data->luma0);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_G_CTRL failed, V4L2_CID_CODEC_CRC_DATA_LUMA",__func__);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }

        ret = v4l2_mfc_g_ctrl(pCTX->hMFC, V4L2_CID_CODEC_CRC_DATA_CHROMA, &crc_data->chroma0);
        if (ret != 0) {
            LOGE("[%s] VIDIOC_G_CTRL failed, V4L2_CID_CODEC_CRC_DATA_CHROMA",__func__);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }
        LOGI("[%s] crc_data->luma0=0x%x\n", __func__, crc_data->luma0);
        LOGI("[%s] crc_data->chroma0=0x%x\n", __func__, crc_data->chroma0);
        break;
#ifdef S3D_SUPPORT
    case MFC_DEC_GETCONF_FRAME_PACKING:
        frame_packing = (SSBSIP_MFC_FRAME_PACKING *)value;

        ret = v4l2_mfc_ext_g_ctrl(pCTX->hMFC, conf_type, &sei_info);
        if (ret != 0) {
            printf("Error to do ext_g_ctrl.\n");
        }
        frame_packing->available = sei_info.sei_avail;
        frame_packing->arrangement_id = sei_info.arrgment_id;

        frame_packing->arrangement_cancel_flag = OPERATE_BIT(sei_info.sei_info, 0x1, 0);
        frame_packing->arrangement_type = OPERATE_BIT(sei_info.sei_info, 0x3f, 1);
        frame_packing->quincunx_sampling_flag = OPERATE_BIT(sei_info.sei_info, 0x1, 8);
        frame_packing->content_interpretation_type = OPERATE_BIT(sei_info.sei_info, 0x3f, 9);
        frame_packing->spatial_flipping_flag = OPERATE_BIT(sei_info.sei_info, 0x1, 15);
        frame_packing->frame0_flipped_flag = OPERATE_BIT(sei_info.sei_info, 0x1, 16);
        frame_packing->field_views_flag = OPERATE_BIT(sei_info.sei_info, 0x1, 17);
        frame_packing->current_frame_is_frame0_flag = OPERATE_BIT(sei_info.sei_info, 0x1, 18);

        frame_packing->frame0_grid_pos_x = OPERATE_BIT(sei_info.sei_info, 0xf, 0);
        frame_packing->frame0_grid_pos_y = OPERATE_BIT(sei_info.sei_info, 0xf, 4);
        frame_packing->frame1_grid_pos_x = OPERATE_BIT(sei_info.sei_info, 0xf, 8);
        frame_packing->frame1_grid_pos_y = OPERATE_BIT(sei_info.sei_info, 0xf, 12);
        break;
#endif
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
