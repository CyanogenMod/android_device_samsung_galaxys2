/*
 * Copyright@ Samsung Electronics Co. LTD
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

#include "videodev2.h"

#define JPEG_DEC_NODE        "/dev/video11"
#define JPEG_ENC_NODE        "/dev/video12"

#define JPEG_MAX_PLANE_CNT          3
#define JPEG_DEC_OUT_BYTE_ALIGN     8

//#define JPEG_PERF_MEAS

#ifdef JPEG_PERF_MEAS
#define JPEG_PERF_DEFINE(n) \
    struct timeval time_start_##n, time_stop_##n; unsigned long log_time_##n = 0;

#define JPEG_PERF_START(n) \
    gettimeofday(&time_start_##n, NULL);

#define JPEG_PERF_END(n) \
    gettimeofday(&time_stop_##n, NULL); log_time_##n = measure_time(&time_start_##n, &time_stop_##n);

#define JPEG_PERF(n) \
    log_time_##n
#else
#define JPEG_PERF_DEFINE(n)
#define JPEG_PERF_START(n)
#define JPEG_PERF_END(n)
#define JPEG_PERF(n)
#endif

enum jpeg_ret_type {
    JPEG_FAIL,
    JPEG_OK,
    JPEG_ENCODE_FAIL,
    JPEG_ENCODE_OK,
    JPEG_DECODE_FAIL,
    JPEG_DECODE_OK,
    JPEG_OUT_OF_MEMORY,
    JPEG_UNKNOWN_ERROR
};

enum jpeg_quality_level {
    QUALITY_LEVEL_1 = 0,    /* high */
    QUALITY_LEVEL_2,
    QUALITY_LEVEL_3,
    QUALITY_LEVEL_4,        /* low */
};

enum jpeg_mode {
    JPEG_ENCODE,
    JPEG_DECODE
};

struct jpeg_buf {
    int     num_planes;
    void    *start[JPEG_MAX_PLANE_CNT];
    int     length[JPEG_MAX_PLANE_CNT];
    enum    v4l2_memory    memory;
    enum    v4l2_buf_type  buf_type;    // Caller need not set this
};

struct jpeg_buf_info {
    int                 num_planes;
    enum v4l2_memory    memory;
    enum v4l2_buf_type  buf_type;
    int                 reserved[4];
};

struct jpeg_pixfmt {
    int in_fmt;
    int out_fmt;
    int reserved[4];
};

struct jpeg_config {
    enum jpeg_mode              mode;
    enum jpeg_quality_level     enc_qual; // for encoding

    int                         width;
    int                         height;

    int                         num_planes;

    int                         scaled_width; // 1/2, 1/4 scaling for decoding
    int                         scaled_height; // 1/2, 1/4 scaling for decoding

    int                         sizeJpeg;

    union {
        struct jpeg_pixfmt enc_fmt;
        struct jpeg_pixfmt dec_fmt;
    } pix;

    int                         reserved[8];
};

#ifdef __cplusplus
extern "C" {
#endif
int jpeghal_dec_init();
int jpeghal_enc_init();

int jpeghal_dec_setconfig(int fd, struct jpeg_config *config);
int jpeghal_enc_setconfig(int fd, struct jpeg_config *config);
int jpeghal_dec_getconfig(int fd, struct jpeg_config *config);
int jpeghal_enc_getconfig(int fd, struct jpeg_config *config);

int jpeghal_set_inbuf(int fd, struct jpeg_buf *buf);
int jpeghal_set_outbuf(int fd, struct jpeg_buf *buf);

int jpeghal_dec_exe(int fd, struct jpeg_buf *in_buf, struct jpeg_buf *out_buf);
int jpeghal_enc_exe(int fd, struct jpeg_buf *in_buf, struct jpeg_buf *out_buf);

int jpeghal_deinit(int fd, struct jpeg_buf *in_buf, struct jpeg_buf *out_buf);

int jpeghal_s_ctrl(int fd, int cid, int value);
int jpeghal_g_ctrl(int fd, int id);

unsigned long measure_time(struct timeval *start, struct timeval *stop);
#ifdef __cplusplus
}
#endif
