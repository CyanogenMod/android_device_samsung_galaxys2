/*
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Global header for Samsung MFC (Multi Function Codec - FIMV) driver
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

#ifndef __MFC_INTERFACE_H
#define __MFC_INTERFACE_H

#include "mfc_errno.h"
#include "SsbSipMfcApi.h"

#define IOCTL_MFC_DEC_INIT          (0x00800001)
#define IOCTL_MFC_ENC_INIT          (0x00800002)
#define IOCTL_MFC_DEC_EXE           (0x00800003)
#define IOCTL_MFC_ENC_EXE           (0x00800004)

#define IOCTL_MFC_GET_IN_BUF        (0x00800010)
#define IOCTL_MFC_FREE_BUF          (0x00800011)
#define IOCTL_MFC_GET_REAL_ADDR     (0x00800012)
#define IOCTL_MFC_GET_MMAP_SIZE     (0x00800014)
#define IOCTL_MFC_SET_IN_BUF        (0x00800018)

#define IOCTL_MFC_SET_CONFIG        (0x00800101)
#define IOCTL_MFC_GET_CONFIG        (0x00800102)

#define IOCTL_MFC_SET_BUF_CACHE     (0x00800201)

/* MFC H/W support maximum 32 extra DPB. */
#define MFC_MAX_EXTRA_DPB                      5
#define MFC_MAX_DISP_DELAY              0xF

#define MFC_LIB_VER_MAJOR               1
#define MFC_LIB_VER_MINOR               00

#define BUF_L_UNIT          (1024)
#define Align(x, alignbyte)     (((x)+(alignbyte)-1)/(alignbyte)*(alignbyte))

#define MFC_ENC_NUM_SRC_BUFS    2 /* Number of source buffers to request */
#define MFC_ENC_MAX_DST_BUFS    2 /* The maximum number of buffers */
#define MFC_ENC_NUM_PLANES  2 /* Number of planes used by MFC Input */

#define MFC_DEC_NUM_SRC_BUFS    2  /* Number of source buffers to request */
#define MFC_DEC_MAX_DST_BUFS    32 /* The maximum number of buffers */
#define MFC_DEC_NUM_PLANES  2  /* Number of planes used by MFC output */

enum inst_type {
    DECODER = 0x1,
    ENCODER = 0x2,
};

typedef enum {
    MFC_UNPACKED_PB = 0,
    MFC_PACKED_PB = 1
} mfc_packed_mode;

typedef enum {
    SSBSIP_MFC_LAST_FRAME_NOT_RECEIVED = 0,
    SSBSIP_MFC_LAST_FRAME_RECEIVED = 1,
    SSBSIP_MFC_LAST_FRAME_PROCESSED = 2
} SSBSIP_MFC_LAST_FRAME_STATUS;

typedef enum {
    MFC_USE_NONE         =  0x0000,
    MFC_USE_YUV_BUFF     =  0x0001,
    MFC_USE_STRM_BUFF    =  0x0010,
    MFC_USE_SRC_STREAMON =  0x0100,
    MFC_USE_DST_STREAMON =  0x1000,
} s3c_mfc_interbuff_status;

typedef struct {
    int luma0;  /* per frame (or top field) */
    int chroma0;    /* per frame (or top field) */
    int luma1;  /* per frame (or bottom field) */
    int chroma1;    /* per frame (or bottom field) */
} SSBSIP_MFC_CRC_DATA;

struct mfc_strm_ref_buf_arg {
    unsigned int strm_ref_y;
    unsigned int mv_ref_yc;
};

struct mfc_frame_buf_arg {
    unsigned int luma;
    unsigned int chroma;
};

struct mfc_enc_init_common_arg {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN] codec type */

    int in_width;                        /* [IN] width of YUV420 frame to be encoded */
    int in_height;                       /* [IN] height of YUV420 frame to be encoded */

    int in_gop_num;                      /* [IN] GOP Number (interval of I-frame) */
    int in_vop_quant;   /* [IN] VOP quant */
    int in_vop_quant_p; /* [IN] VOP quant for P frame */

    /* [IN] RC enable */
    /* [IN] RC enable (0:disable, 1:frame level RC) */
    int in_rc_fr_en;
    int in_rc_bitrate;  /* [IN]  RC parameter (bitrate in kbps) */

    int in_rc_qbound_min;   /* [IN]  RC parameter (Q bound Min) */
    int in_rc_qbound_max;   /* [IN]  RC parameter (Q bound Max) */
    int in_rc_rpara;    /* [IN]  RC parameter (Reaction Coefficient) */

    /* [IN] Multi-slice mode (0:single, 1:multiple) */
    int in_ms_mode;
    /* [IN] Multi-slice size (in num. of mb or byte) */
    int in_ms_arg;

    int in_mb_refresh;                   /* [IN] Macroblock refresh */

    /* [IN] Enable (1) / Disable (0) padding with the specified values */
    int in_pad_ctrl_on;

    /* [IN] pad value if pad_ctrl_on is Enable */
    int in_y_pad_val;
    int in_cb_pad_val;
    int in_cr_pad_val;

    /* linear or tiled */
    int in_frame_map;

    unsigned int in_pixelcache;

    unsigned int in_mapped_addr;
    struct mfc_strm_ref_buf_arg out_u_addr;
    struct mfc_strm_ref_buf_arg out_p_addr;
    struct mfc_strm_ref_buf_arg out_buf_size;
    unsigned int out_header_size;
};

struct mfc_enc_init_h263_arg {
    int in_rc_framerate;    /* [IN]  RC parameter (framerate) */
};

struct mfc_enc_init_mpeg4_arg {
    int in_profile;     /* [IN] profile */
    int in_level;       /* [IN] level */

    int in_vop_quant_b; /* [IN] VOP quant for B frame */

    /* [IN] B frame number */
    int in_bframenum;

    /* [IN] Quarter-pel MC enable (1:enabled, 0:disabled) */
    int in_quart_pixel;

    int in_TimeIncreamentRes;   /* [IN] VOP time resolution */
    int in_VopTimeIncreament;   /* [IN] Frame delta */
};

struct mfc_enc_init_h264_arg {
    int in_profile;     /* [IN] profile */
    int in_level;       /* [IN] level */

    int in_vop_quant_b; /* [IN] VOP quant for B frame */

    /* [IN] B frame number */
    int in_bframenum;

    /* [IN] interlace mode(0:progressive, 1:interlace) */
    int in_interlace_mode;

    /* [IN]  reference number */
    int in_reference_num;
    /* [IN]  reference number of P frame */
    int in_ref_num_p;

    int in_rc_framerate;    /* [IN]  RC parameter (framerate) */
    int in_rc_mb_en;    /* [IN] RC enable (0:disable, 1:MB level RC) */
    /* [IN] MB level rate control dark region adaptive feature */
    int in_rc_mb_dark_dis;  /* (0:enable, 1:disable) */
    /* [IN] MB level rate control smooth region adaptive feature */
    int in_rc_mb_smooth_dis;    /* (0:enable, 1:disable) */
    /* [IN] MB level rate control static region adaptive feature */
    int in_rc_mb_static_dis;    /* (0:enable, 1:disable) */
    /* [IN] MB level rate control activity region adaptive feature */
    int in_rc_mb_activity_dis;  /* (0:enable, 1:disable) */

    /* [IN]  disable deblocking filter idc */
    int in_deblock_dis; /* (0: enable,1: disable, 2:Disable at slice boundary) */
    /* [IN]  slice alpha c0 offset of deblocking filter */
    int in_deblock_alpha_c0;
    /* [IN]  slice beta offset of deblocking filter */
    int in_deblock_beta;

    /* [IN]  ( 0 : CAVLC, 1 : CABAC ) */
    int in_symbolmode;
    /* [IN] (0: only 4x4 transform, 1: allow using 8x8 transform) */
    int in_transform8x8_mode;

    /* [IN] Inter weighted parameter for mode decision */
    int in_md_interweight_pps;
    /* [IN] Intra weighted parameter for mode decision */
    int in_md_intraweight_pps;
};

struct mfc_enc_init_arg {
    struct mfc_enc_init_common_arg cmn;
    union {
        struct mfc_enc_init_h264_arg h264;
        struct mfc_enc_init_mpeg4_arg mpeg4;
        struct mfc_enc_init_h263_arg h263;
    } codec;
};

struct mfc_enc_exe_arg {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN]  codec type */
    unsigned int in_Y_addr;              /* [IN]  In-buffer addr of Y component */
    unsigned int in_CbCr_addr;           /* [IN]  In-buffer addr of CbCr component */
    unsigned int in_Y_addr_vir;          /* [IN]  In-buffer addr of Y component */
    unsigned int in_CbCr_addr_vir;       /* [IN]  In-buffer addr of CbCr component */
    unsigned int in_strm_st;             /* [IN]  Out-buffer start addr of encoded strm */
    unsigned int in_strm_end;            /* [IN]  Out-buffer end addr of encoded strm */
    unsigned int in_frametag;   /* [IN]  unique frame ID */

    unsigned int out_frame_type;         /* [OUT] frame type */
    int out_encoded_size;                /* [OUT] Length of Encoded video stream */
    unsigned int out_Y_addr;    /*[OUT]Out-buffer addr of encoded Y component */
    unsigned int out_CbCr_addr; /*[OUT]Out-buffer addr of encoded CbCr component */
    unsigned int out_frametag_top;  /* [OUT] unique frame ID of an output frame or top field */
    unsigned int out_frametag_bottom;/* [OUT] unique frame ID of bottom field */

#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
    unsigned int out_y_secure_id;
    unsigned int out_c_secure_id;
#elif defined(CONFIG_S5P_VMEM)
    unsigned int out_y_cookie;
    unsigned int out_c_cookie;
#endif
};

struct mfc_dec_init_arg {
    SSBSIP_MFC_CODEC_TYPE in_codec_type;    /* [IN] codec type */
    int in_strm_buf;        /* [IN] address of stream buffer */
    int in_strm_size;       /* [IN] filled size in stream buffer */
    int in_packed_PB;                    /* [IN]  Is packed PB frame or not, 1: packedPB  0: unpacked */

    unsigned int in_crc;        /* [IN] */
    unsigned int in_pixelcache; /* [IN] */
    unsigned int in_slice;      /* [IN] */
    unsigned int in_numextradpb;    /* [IN] */

    unsigned int in_mapped_addr;

    int out_frm_width;      /* [OUT] width  of YUV420 frame */
    int out_frm_height;     /* [OUT] height of YUV420 frame */
    int out_buf_width;  /* [OUT] width  of YUV420 frame */
    int out_buf_height; /* [OUT] height of YUV420 frame */

    int out_dpb_cnt;    /* [OUT] the number of buffers which is nessary during decoding. */

    int out_crop_right_offset; /* [OUT] crop information for h264 */
    int out_crop_left_offset;
    int out_crop_bottom_offset;
    int out_crop_top_offset;
};

struct mfc_dec_exe_arg {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN]  codec type */
    int in_strm_buf;  /* [IN]  the physical address of STRM_BUF */
    /* [IN]  Size of video stream filled in STRM_BUF */
    int in_strm_size;
    /* [IN] the address of dpb FRAME_BUF */
    struct mfc_frame_buf_arg in_frm_buf;
    /* [IN] size of dpb FRAME_BUF */
    struct mfc_frame_buf_arg in_frm_size;
    /* [IN] Unique frame ID eg. application specific timestamp */
    unsigned int in_frametag;
    /* [IN] immdiate Display for seek,thumbnail and one frame */
    int in_immediately_disp;
    /* [OUT]  the physical address of display buf */
    int out_display_Y_addr;
    /* [OUT]  the physical address of display buf */
    int out_display_C_addr;
    int out_display_status;
    /* [OUT] unique frame ID of an output frame or top field */
    unsigned int out_frametag_top;
     /* [OUT] unique frame ID of bottom field */
    unsigned int out_frametag_bottom;
    int out_pic_time_top;
    int out_pic_time_bottom;
    int out_consumed_byte;

    int out_crop_right_offset;
    int out_crop_left_offset;
    int out_crop_bottom_offset;
    int out_crop_top_offset;

    /* in new driver, each buffer offset must be return to the user */
    int out_y_offset;
    int out_c_offset;

#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
    unsigned int out_y_secure_id;
    unsigned int out_c_secure_id;
#elif defined(CONFIG_S5P_VMEM)
    unsigned int out_y_cookie;
    unsigned int out_c_cookie;
#endif
    int out_img_width;                   /* [OUT] width  of YUV420 frame */
    int out_img_height;                  /* [OUT] height of YUV420 frame */
    int out_buf_width;                   /* [OUT] width  of YUV420 frame */
    int out_buf_height;                  /* [OUT] height of YUV420 frame */

    int out_disp_pic_frame_type;        /* [OUT] display picture frame type information */
};

struct mfc_get_config_arg {
    /* [IN] Configurable parameter type */
    int in_config_param;

    /* [IN] Values to get for the configurable parameter. */
    /* Maximum four integer values can be obtained; */
    int out_config_value[4];
};

struct mfc_set_config_arg {
    /* [IN] Configurable parameter type */
    int in_config_param;

    /* [IN]  Values to be set for the configurable parameter. */
    /* Maximum four integer values can be set. */
    int in_config_value[4];
};

struct mfc_get_real_addr_arg {
    unsigned int key;
    unsigned int addr;
};

struct mfc_buf_alloc_arg {
    enum inst_type type;
    int size;
    /*
    unsigned int mapped;
    */
    unsigned int align;

    unsigned int addr;
    /*
    unsigned int phys;
    */
#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
    /* FIMXE: invalid secure id == -1 */
    unsigned int secure_id;
#elif defined(CONFIG_S5P_VMEM)
    unsigned int cookie;
#else
    unsigned int offset;
#endif
};

struct mfc_buf_free_arg {
    unsigned int addr;
};

/* RMVME */
struct mfc_mem_alloc_arg {
    enum inst_type type;
    int buff_size;
    SSBIP_MFC_BUFFER_TYPE buf_cache_type;
    unsigned int mapped_addr;
#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
    unsigned int secure_id;
#elif defined(CONFIG_S5P_VMEM)
    unsigned int cookie;
#else
    unsigned int offset;
#endif
};

struct mfc_mem_free_arg {
    unsigned int key;
};
/* RMVME */

union mfc_args {
    /*
    struct mfc_enc_init_arg enc_init;

    struct mfc_enc_init_mpeg4_arg enc_init_mpeg4;
    struct mfc_enc_init_mpeg4_arg enc_init_h263;
    struct mfc_enc_init_h264_arg enc_init_h264;
    */
    struct mfc_enc_init_arg enc_init;
    struct mfc_enc_exe_arg enc_exe;

    struct mfc_dec_init_arg dec_init;
    struct mfc_dec_exe_arg dec_exe;

    struct mfc_get_config_arg get_config;
    struct mfc_set_config_arg set_config;

    struct mfc_buf_alloc_arg buf_alloc;
    struct mfc_buf_free_arg buf_free;
    struct mfc_get_real_addr_arg real_addr;

    /* RMVME */
    struct mfc_mem_alloc_arg mem_alloc;
    struct mfc_mem_free_arg mem_free;
    /* RMVME */
};

struct mfc_common_args {
    enum mfc_ret_code ret_code; /* [OUT] error code */
    union mfc_args args;
};

struct mfc_enc_vui_info {
    int aspect_ratio_idc;
};

struct mfc_dec_fimv1_info {
    int width;
    int height;
};

struct mfc_enc_hier_p_qp {
    int t0_frame_qp;
    int t2_frame_qp;
    int t3_frame_qp;
};

enum BUF_STATUS {
    BUF_ENQUEUED,
    BUF_DEQUEUED
};

struct mfc_dec_v4l2 {
    char *mfc_src_bufs[MFC_DEC_NUM_SRC_BUFS];                   /* information of source buffers */
    char *mfc_dst_bufs[MFC_DEC_MAX_DST_BUFS][MFC_DEC_NUM_PLANES];   /* information of destination buffers */
    char *mfc_dst_phys[MFC_DEC_MAX_DST_BUFS][MFC_DEC_NUM_PLANES];   /* cma information of destination buffers */

    unsigned int mfc_src_bufs_len;                      /* needed for munmap */
    unsigned int mfc_dst_bufs_len[MFC_DEC_NUM_PLANES];  /* needed for munmap */

    unsigned int mfc_num_src_bufs;  /* the number of source buffers */
    unsigned int mfc_num_dst_bufs;  /* the number of destination buffers */

    char mfc_src_buf_flags[MFC_DEC_NUM_SRC_BUFS];
    int bBeingFinalized;
    int allocIndex;
    int beingUsedIndex;
};

struct mfc_enc_v4l2 {
    char *mfc_src_bufs[MFC_ENC_NUM_SRC_BUFS][MFC_ENC_NUM_PLANES];
    char *mfc_src_phys[MFC_ENC_NUM_SRC_BUFS][MFC_ENC_NUM_PLANES];
    char *mfc_dst_bufs[MFC_ENC_MAX_DST_BUFS];

    unsigned int mfc_src_bufs_len[MFC_ENC_NUM_PLANES];
    unsigned int mfc_dst_bufs_len;

    unsigned int mfc_num_src_bufs;
    unsigned int mfc_num_dst_bufs;

    unsigned int mfc_dst_bufs_bytes_used_len;
    char mfc_src_buf_flags[MFC_ENC_NUM_SRC_BUFS];
    int bRunning;
    int bInputPhyVir;               /* Flag to use MFC src as physical or virtual 0: virtual  1: physical */
    int beingUsedIndex;
};

typedef struct {
    int magic;
    int hMFC;
    int hVMEM;
    int width;
    int height;
    int sizeStrmBuf;
    struct mfc_frame_buf_arg sizeFrmBuf;
    int displayStatus;
    int inter_buff_status;
    unsigned int virFreeStrmAddr;
    unsigned int phyStrmBuf;
    unsigned int virStrmBuf;
    unsigned int virMvRefYC;
    struct mfc_frame_buf_arg phyFrmBuf;
    struct mfc_frame_buf_arg virFrmBuf;
    unsigned int mapped_addr;
    unsigned int mapped_size;
    struct mfc_common_args MfcArg;
    SSBSIP_MFC_CODEC_TYPE codecType;
    SSBSIP_MFC_DEC_OUTPUT_INFO decOutInfo;
    unsigned int inframetag;
    unsigned int outframetagtop;
    unsigned int outframetagbottom;
    unsigned int immediatelydisp;
    unsigned int encodedHeaderSize;
    int encodedDataSize;
    unsigned int encodedframeType;
    struct mfc_frame_buf_arg encodedphyFrmBuf;

    unsigned int dec_crc;
    unsigned int dec_pixelcache;
    unsigned int dec_slice;
    unsigned int dec_numextradpb;

    int input_cookie;
    int input_secure_id;
    int input_size;

    /* to support non-blocking mode */
    unsigned int encode_cnt;

    struct mfc_dec_v4l2 v4l2_dec;
    struct mfc_enc_v4l2 v4l2_enc;

    int enc_frameskip;
    int cacheablebuffer;
    struct mfc_dec_fimv1_info fimv1_res;
    SSBSIP_MFC_LAST_FRAME_STATUS lastframe;
    SSBSIP_MFC_INSTRM_MODE_TYPE framemap;
} _MFCLIB;

#define ENC_PROFILE_LEVEL(profile, level)      ((profile) | ((level) << 8))
#define ENC_RC_QBOUND(min_qp, max_qp)          ((min_qp) | ((max_qp) << 8))

#define SSBSIP_MFC_FAIL                  (0)

#endif /* __MFC_INTERFACE_H */
