#ifndef __JPEG_API_H__
#define __JPEG_API_H__

#define JPEG_DRIVER_NAME        "/dev/s5p-jpeg"

#define MAX_JPEG_WIDTH          3264
#define MAX_JPEG_HEIGHT         2448

#define MAX_JPEG_RES            (MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT)

#define JPEG_STREAM_BUF_SIZE    MAX_JPEG_RES
#define JPEG_FRAME_BUF_SIZE     (MAX_JPEG_RES * 3)

#define JPEG_TOTAL_BUF_SIZE     (JPEG_STREAM_BUF_SIZE + JPEG_FRAME_BUF_SIZE)

#define JPEG_IOCTL_MAGIC    'J'

#define IOCTL_JPEG_DEC_EXE              _IO(JPEG_IOCTL_MAGIC, 1)
#define IOCTL_JPEG_ENC_EXE              _IO(JPEG_IOCTL_MAGIC, 2)
#define IOCTL_GET_DEC_IN_BUF            _IO(JPEG_IOCTL_MAGIC, 3)
#define IOCTL_GET_DEC_OUT_BUF           _IO(JPEG_IOCTL_MAGIC, 4)
#define IOCTL_GET_ENC_IN_BUF            _IO(JPEG_IOCTL_MAGIC, 5)
#define IOCTL_GET_ENC_OUT_BUF           _IO(JPEG_IOCTL_MAGIC, 6)
#define IOCTL_SET_DEC_PARAM             _IO(JPEG_IOCTL_MAGIC, 7)
#define IOCTL_SET_ENC_PARAM             _IO(JPEG_IOCTL_MAGIC, 8)

enum jpeg_ret_type{
    JPEG_FAIL,
    JPEG_OK,
    JPEG_ENCODE_FAIL,
    JPEG_ENCODE_OK,
    JPEG_DECODE_FAIL,
    JPEG_DECODE_OK,
    JPEG_OUT_OF_MEMORY,
    JPEG_UNKNOWN_ERROR
};

enum jpeg_img_quality_level {
    QUALITY_LEVEL_1 = 0,     /* high */
    QUALITY_LEVEL_2,
    QUALITY_LEVEL_3,
    QUALITY_LEVEL_4,         /* low */
};

/* raw data image format */
enum jpeg_frame_format {
    YUV_422,    /* decode output, encode input */
    YUV_420,    /* decode output, encode output */
    RGB_565,    /* encode input */
};

/* jpeg data format */
enum jpeg_stream_format {
    JPEG_422,    /* decode input, encode output */
    JPEG_420,    /* decode input, encode output */
    JPEG_444,    /* decode input*/
    JPEG_GRAY,    /* decode input*/
    JPEG_RESERVED,
};

enum jpeg_test_mode {
    encode_mode,
    decode_mode,
};

struct jpeg_dec_param {
    unsigned int width;
    unsigned int height;
    unsigned int size;
    enum jpeg_stream_format in_fmt;
    enum jpeg_frame_format out_fmt;
};

struct jpeg_enc_param {
    unsigned int width;
    unsigned int height;
    unsigned int size;
    enum jpeg_frame_format in_fmt;
    enum jpeg_stream_format out_fmt;
    enum jpeg_img_quality_level quality;
};

struct jpeg_args{
    char                     *in_buf;
    unsigned int             in_cookie;
    unsigned int             in_buf_size;
    char                     *out_buf;
    unsigned int             out_cookie;
    unsigned int             out_buf_size;
    char                     *mmapped_addr;
    struct jpeg_dec_param    *dec_param;
    struct jpeg_enc_param    *enc_param;
};

struct jpeg_lib {
    int  jpeg_fd;
    struct jpeg_args args;
};

#ifdef __cplusplus
extern "C" {
#endif
int api_jpeg_decode_init();
int api_jpeg_encode_init();
int api_jpeg_decode_deinit(int dev_fd);
int api_jpeg_encode_deinit(int dev_fd);
void *api_jpeg_get_decode_in_buf(int dev_fd, unsigned int size);
void *api_jpeg_get_encode_in_buf(int dev_fd, unsigned int size);
void *api_jpeg_get_decode_out_buf(int dev_fd);
void *api_jpeg_get_encode_out_buf(int dev_fd);
void api_jpeg_set_decode_param(struct jpeg_dec_param *param);
void api_jpeg_set_encode_param(struct jpeg_enc_param *param);
enum jpeg_ret_type api_jpeg_decode_exe(int dev_fd,
                    struct jpeg_dec_param *dec_param);
enum jpeg_ret_type api_jpeg_encode_exe(int dev_fd,
                    struct jpeg_enc_param *enc_param);
#ifdef __cplusplus
}
#endif

#endif//__JPEG_API_H__
