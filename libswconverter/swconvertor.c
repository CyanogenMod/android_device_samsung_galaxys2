/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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
 * @file    swconvertor.c
 *
 * @brief   SEC_OMX specific define
 *
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 *
 * @version 1.0
 *
 * @history
 *   2012.02.01 : Create
 */

#include "stdio.h"
#include "stdlib.h"
#include "swconverter.h"

/*
 * Get tiled address of position(x,y)
 *
 * @param x_size
 *   width of tiled[in]
 *
 * @param y_size
 *   height of tiled[in]
 *
 * @param x_pos
 *   x position of tield[in]
 *
 * @param src_size
 *   y position of tield[in]
 *
 * @return
 *   address of tiled data
 */
static int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
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

/*
 * De-interleaves src to dest1, dest2
 *
 * @param dest1
 *   Address of de-interleaved data[out]
 *
 * @param dest2
 *   Address of de-interleaved data[out]
 *
 * @param src
 *   Address of interleaved data[in]
 *
 * @param src_size
 *   Size of interleaved data[in]
 */
void csc_deinterleave_memcpy(
    unsigned char *dest1,
    unsigned char *dest2,
    unsigned char *src,
    unsigned int src_size)
{
    unsigned int i = 0;
    for(i=0; i<src_size/2; i++) {
        dest1[i] = src[i*2];
        dest2[i] = src[i*2+1];
    }
}

/*
 * Interleaves src1, src2 to dest
 *
 * @param dest
 *   Address of interleaved data[out]
 *
 * @param src1
 *   Address of de-interleaved data[in]
 *
 * @param src2
 *   Address of de-interleaved data[in]
 *
 * @param src_size
 *   Size of de-interleaved data[in]
 */
void csc_interleave_memcpy(
    unsigned char *dest,
    unsigned char *src1,
    unsigned char *src2,
    unsigned int src_size)
{
    unsigned int i = 0;
    for(i=0; i<src_size; i++) {
        dest[i * 2] = src1[i];
        dest[i * 2 + 1] = src2[i];
    }
}

/*
 * Converts tiled data to linear
 * Crops left, top, right, buttom
 * 1. Y of NV12T to Y of YUV420P
 * 2. Y of NV12T to Y of YUV420S
 * 3. UV of NV12T to UV of YUV420S
 *
 * @param yuv420_dest
 *   Y or UV plane address of YUV420[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: Height of YUV420, UV: Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
static void csc_tiled_to_linear_crop(
    unsigned char *yuv420_dest,
    unsigned char *nv12t_src,
    unsigned int yuv420_width,
    unsigned int yuv420_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom)
{
    unsigned int i, j;
    unsigned int tiled_offset = 0, tiled_offset1 = 0;
    unsigned int linear_offset = 0;
    unsigned int temp1 = 0, temp2 = 0, temp3 = 0, temp4 = 0;

    temp3 = yuv420_width-right;
    temp1 = temp3-left;
    /* real width is greater than or equal 256 */
    if (temp1 >= 256) {
        for (i=top; i<yuv420_height-buttom; i=i+1) {
            j = left;
            temp3 = (j>>8)<<8;
            temp3 = temp3>>6;
            temp4 = i>>5;
            if (temp4 & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = temp4-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+temp3;
                tiled_offset = tiled_offset+2;
                temp1 = (temp3>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*2;
                temp4 = 8;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = temp3+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = temp3+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+temp4*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                    tiled_offset1 = tiled_offset+2048*6;
                    temp4 = 8;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = temp4*(temp1>>6);
                    tiled_offset = tiled_offset+temp3;
                    tiled_offset = tiled_offset<<11;
                    tiled_offset1 = tiled_offset+2048*2;
                    temp4 = 4;
                }
            }

            temp1 = i&0x1F;
            tiled_offset = tiled_offset+64*(temp1);
            tiled_offset1 = tiled_offset1+64*(temp1);
            temp2 = yuv420_width-left-right;
            linear_offset = temp2*(i-top);
            temp3 = ((j+256)>>8)<<8;
            temp3 = temp3-j;
            temp1 = left&0x3F;
            if (temp3 > 192) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset+temp1, 64-temp1);
                temp2 = ((left+63)>>6)<<6;
                temp3 = ((yuv420_width-right)>>6)<<6;
                if (temp2 == temp3) {
                    temp2 = yuv420_width-right-(64-temp1);
                }
                memcpy(yuv420_dest+linear_offset+64-temp1, nv12t_src+tiled_offset+2048, 64);
                memcpy(yuv420_dest+linear_offset+128-temp1, nv12t_src+tiled_offset1, 64);
                memcpy(yuv420_dest+linear_offset+192-temp1, nv12t_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+256-temp1;
            } else if (temp3 > 128) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset+2048+temp1, 64-temp1);
                memcpy(yuv420_dest+linear_offset+64-temp1, nv12t_src+tiled_offset1, 64);
                memcpy(yuv420_dest+linear_offset+128-temp1, nv12t_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+192-temp1;
            } else if (temp3 > 64) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset1+temp1, 64-temp1);
                memcpy(yuv420_dest+linear_offset+64-temp1, nv12t_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+128-temp1;
            } else if (temp3 > 0) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset1+2048+temp1, 64-temp1);
                linear_offset = linear_offset+64-temp1;
            }

            tiled_offset = tiled_offset+temp4*2048;
            j = (left>>8)<<8;
            j = j + 256;
            temp2 = yuv420_width-right-256;
            for (; j<=temp2; j=j+256) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 64);
                tiled_offset1 = tiled_offset1+temp4*2048;
                memcpy(yuv420_dest+linear_offset+64, nv12t_src+tiled_offset+2048, 64);
                memcpy(yuv420_dest+linear_offset+128, nv12t_src+tiled_offset1, 64);
                tiled_offset = tiled_offset+temp4*2048;
                memcpy(yuv420_dest+linear_offset+192, nv12t_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+256;
            }

            tiled_offset1 = tiled_offset1+temp4*2048;
            temp2 = yuv420_width-right-j;
            if (temp2 > 192) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 64);
                memcpy(yuv420_dest+linear_offset+64, nv12t_src+tiled_offset+2048, 64);
                memcpy(yuv420_dest+linear_offset+128, nv12t_src+tiled_offset1, 64);
                memcpy(yuv420_dest+linear_offset+192, nv12t_src+tiled_offset1+2048, temp2-192);
            } else if (temp2 > 128) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 64);
                memcpy(yuv420_dest+linear_offset+64, nv12t_src+tiled_offset+2048, 64);
                memcpy(yuv420_dest+linear_offset+128, nv12t_src+tiled_offset1, temp2-128);
            } else if (temp2 > 64) {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 64);
                memcpy(yuv420_dest+linear_offset+64, nv12t_src+tiled_offset+2048, temp2-64);
            } else {
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, temp2);
            }
        }
    } else if (temp1 >= 64) {
        for (i=top; i<(yuv420_height-buttom); i=i+1) {
            j = left;
            tiled_offset = tile_4x2_read(yuv420_width, yuv420_height, j, i);
            temp2 = ((j+64)>>6)<<6;
            temp2 = temp2-j;
            linear_offset = temp1*(i-top);
            temp4 = j&0x3;
            tiled_offset = tiled_offset+temp4;
            memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, temp2);
            linear_offset = linear_offset+temp2;
            j = j+temp2;
            if ((j+64) <= temp3) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_height, j, i);
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 64);
                linear_offset = linear_offset+64;
                j = j+64;
            }
            if ((j+64) <= temp3) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_height, j, i);
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 64);
                linear_offset = linear_offset+64;
                j = j+64;
            }
            if (j < temp3) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_height, j, i);
                temp2 = temp3-j;
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, temp2);
            }
        }
    } else {
        for (i=top; i<(yuv420_height-buttom); i=i+1) {
            linear_offset = temp1*(i-top);
            for (j=left; j<(yuv420_width-right); j=j+2) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_height, j, i);
                temp4 = j&0x3;
                tiled_offset = tiled_offset+temp4;
                memcpy(yuv420_dest+linear_offset, nv12t_src+tiled_offset, 2);
                linear_offset = linear_offset+2;
            }
        }
    }
}

/*
 * Converts and Deinterleaves tiled data to linear
 * Crops left, top, right, buttom
 * 1. UV of NV12T to UV of YUV420P
 *
 * @param yuv420_u_dest
 *   U plane address of YUV420P[out]
 *
 * @param yuv420_v_dest
 *   V plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_uv_height
 *   Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
static void csc_tiled_to_linear_deinterleave_crop(
    unsigned char *yuv420_u_dest,
    unsigned char *yuv420_v_dest,
    unsigned char *nv12t_uv_src,
    unsigned int yuv420_width,
    unsigned int yuv420_uv_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom)
{
    unsigned int i, j;
    unsigned int tiled_offset = 0, tiled_offset1 = 0;
    unsigned int linear_offset = 0;
    unsigned int temp1 = 0, temp2 = 0, temp3 = 0, temp4 = 0;

    temp3 = yuv420_width-right;
    temp1 = temp3-left;
    /* real width is greater than or equal 256 */
    if (temp1 >= 256) {
        for (i=top; i<yuv420_uv_height-buttom; i=i+1) {
            j = left;
            temp3 = (j>>8)<<8;
            temp3 = temp3>>6;
            temp4 = i>>5;
            if (temp4 & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = temp4-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+temp3;
                tiled_offset = tiled_offset+2;
                temp1 = (temp3>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*2;
                temp4 = 8;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = temp3+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = temp3+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+temp4*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                    tiled_offset1 = tiled_offset+2048*6;
                    temp4 = 8;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = temp4*(temp1>>6);
                    tiled_offset = tiled_offset+temp3;
                    tiled_offset = tiled_offset<<11;
                    tiled_offset1 = tiled_offset+2048*2;
                    temp4 = 4;
                }
            }

            temp1 = i&0x1F;
            tiled_offset = tiled_offset+64*(temp1);
            tiled_offset1 = tiled_offset1+64*(temp1);
            temp2 = yuv420_width-left-right;
            linear_offset = temp2*(i-top)/2;
            temp3 = ((j+256)>>8)<<8;
            temp3 = temp3-j;
            temp1 = left&0x3F;
            if (temp3 > 192) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset, yuv420_v_dest+linear_offset, nv12t_uv_src+tiled_offset+temp1, 64-temp1);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+(32-temp1/2),
                                        yuv420_v_dest+linear_offset+(32-temp1/2),
                                        nv12t_uv_src+tiled_offset+2048, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+(64-temp1/2),
                                        yuv420_v_dest+linear_offset+(64-temp1/2),
                                        nv12t_uv_src+tiled_offset1, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+(96-temp1/2),
                                        yuv420_v_dest+linear_offset+(96-temp1/2),
                                        nv12t_uv_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+128-temp1/2;
            } else if (temp3 > 128) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset+2048+temp1, 64-temp1);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+(32-temp1/2),
                                        yuv420_v_dest+linear_offset+(32-temp1/2),
                                        nv12t_uv_src+tiled_offset1, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+(64-temp1/2),
                                        yuv420_v_dest+linear_offset+(64-temp1/2),
                                        nv12t_uv_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+96-temp1/2;
            } else if (temp3 > 64) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset1+temp1, 64-temp1);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+(32-temp1/2),
                                        yuv420_v_dest+linear_offset+(32-temp1/2),
                                        nv12t_uv_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+64-temp1/2;
            } else if (temp3 > 0) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset1+2048+temp1, 64-temp1);
                linear_offset = linear_offset+32-temp1/2;
            }

            tiled_offset = tiled_offset+temp4*2048;
            j = (left>>8)<<8;
            j = j + 256;
            temp2 = yuv420_width-right-256;
            for (; j<=temp2; j=j+256) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 64);
                tiled_offset1 = tiled_offset1+temp4*2048;
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+32,
                                        yuv420_v_dest+linear_offset+32,
                                        nv12t_uv_src+tiled_offset+2048, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+64,
                                        yuv420_v_dest+linear_offset+64,
                                        nv12t_uv_src+tiled_offset1, 64);
                tiled_offset = tiled_offset+temp4*2048;
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+96,
                                        yuv420_v_dest+linear_offset+96,
                                        nv12t_uv_src+tiled_offset1+2048, 64);
                linear_offset = linear_offset+128;
            }

            tiled_offset1 = tiled_offset1+temp4*2048;
            temp2 = yuv420_width-right-j;
            if (temp2 > 192) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+32,
                                        yuv420_v_dest+linear_offset+32,
                                        nv12t_uv_src+tiled_offset+2048, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+64,
                                        yuv420_v_dest+linear_offset+64,
                                        nv12t_uv_src+tiled_offset1, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+96,
                                        yuv420_v_dest+linear_offset+96,
                                        nv12t_uv_src+tiled_offset1+2048, temp2-192);
            } else if (temp2 > 128) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+32,
                                        yuv420_v_dest+linear_offset+32,
                                        nv12t_uv_src+tiled_offset+2048, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+64,
                                        yuv420_v_dest+linear_offset+64,
                                        nv12t_uv_src+tiled_offset1, temp2-128);
            } else if (temp2 > 64) {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 64);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset+32,
                                        yuv420_v_dest+linear_offset+32,
                                        nv12t_uv_src+tiled_offset+2048, temp2-64);
            } else {
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, temp2);
            }
        }
    } else if (temp1 >= 64) {
        for (i=top; i<(yuv420_uv_height-buttom); i=i+1) {
            j = left;
            tiled_offset = tile_4x2_read(yuv420_width, yuv420_uv_height, j, i);
            temp2 = ((j+64)>>6)<<6;
            temp2 = temp2-j;
            temp3 = yuv420_width-right;
            temp4 = temp3-left;
            linear_offset = temp4*(i-top)/2;
            temp4 = j&0x3;
            tiled_offset = tiled_offset+temp4;
            csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                    yuv420_v_dest+linear_offset,
                                    nv12t_uv_src+tiled_offset, temp2);
            linear_offset = linear_offset+temp2/2;
            j = j+temp2;
            if ((j+64) <= temp3) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_uv_height, j, i);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 64);
                linear_offset = linear_offset+32;
                j = j+64;
            }
            if ((j+64) <= temp3) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_uv_height, j, i);
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 64);
                linear_offset = linear_offset+32;
                j = j+64;
            }
            if (j < temp3) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_uv_height, j, i);
                temp1 = temp3-j;
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, temp1);
            }
        }
    } else {
        for (i=top; i<(yuv420_uv_height-buttom); i=i+1) {
            temp3 = yuv420_width-right;
            temp4 = temp3-left;
            linear_offset = temp4*(i-top)/2;
            for (j=left; j<(yuv420_width-right); j=j+2) {
                tiled_offset = tile_4x2_read(yuv420_width, yuv420_uv_height, j, i);
                temp3 = j&0x3;
                tiled_offset = tiled_offset+temp3;
                csc_deinterleave_memcpy(yuv420_u_dest+linear_offset,
                                        yuv420_v_dest+linear_offset,
                                        nv12t_uv_src+tiled_offset, 2);
                linear_offset = linear_offset+1;
            }
        }
    }
}

/*
 * Converts linear data to tiled
 * Crops left, top, right, buttom
 * 1. Y of YUV420P to Y of NV12T
 * 2. Y of YUV420S to Y of NV12T
 * 3. UV of YUV420S to UV of NV12T
 *
 * @param nv12t_dest
 *   Y or UV plane address of NV12T[out]
 *
 * @param yuv420_src
 *   Y or UV plane address of YUV420P(S)[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: Height of YUV420, UV: Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
static void csc_linear_to_tiled_crop(
    unsigned char *nv12t_dest,
    unsigned char *yuv420_src,
    unsigned int yuv420_width,
    unsigned int yuv420_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom)
{
    unsigned int i, j;
    unsigned int tiled_x_index = 0, tiled_y_index = 0;
    unsigned int aligned_x_size = 0, aligned_y_size = 0;
    unsigned int tiled_offset = 0;
    unsigned int temp1 = 0, temp2 = 0;

    aligned_y_size = ((yuv420_height-top-buttom)>>5)<<5;
    aligned_x_size = ((yuv420_width-left-right)>>6)<<6;

    for (i=0; i<aligned_y_size; i=i+32) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = (((yuv420_height-top-buttom)+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            memcpy(nv12t_dest+tiled_offset, yuv420_src+left+j+yuv420_width*(i+top), 64);
            memcpy(nv12t_dest+tiled_offset+64*1, yuv420_src+left+j+yuv420_width*(i+top+1), 64);
            memcpy(nv12t_dest+tiled_offset+64*2, yuv420_src+left+j+yuv420_width*(i+top+2), 64);
            memcpy(nv12t_dest+tiled_offset+64*3, yuv420_src+left+j+yuv420_width*(i+top+3), 64);
            memcpy(nv12t_dest+tiled_offset+64*4, yuv420_src+left+j+yuv420_width*(i+top+4), 64);
            memcpy(nv12t_dest+tiled_offset+64*5, yuv420_src+left+j+yuv420_width*(i+top+5), 64);
            memcpy(nv12t_dest+tiled_offset+64*6, yuv420_src+left+j+yuv420_width*(i+top+6), 64);
            memcpy(nv12t_dest+tiled_offset+64*7, yuv420_src+left+j+yuv420_width*(i+top+7), 64);
            memcpy(nv12t_dest+tiled_offset+64*8, yuv420_src+left+j+yuv420_width*(i+top+8), 64);
            memcpy(nv12t_dest+tiled_offset+64*9, yuv420_src+left+j+yuv420_width*(i+top+9), 64);
            memcpy(nv12t_dest+tiled_offset+64*10, yuv420_src+left+j+yuv420_width*(i+top+10), 64);
            memcpy(nv12t_dest+tiled_offset+64*11, yuv420_src+left+j+yuv420_width*(i+top+11), 64);
            memcpy(nv12t_dest+tiled_offset+64*12, yuv420_src+left+j+yuv420_width*(i+top+12), 64);
            memcpy(nv12t_dest+tiled_offset+64*13, yuv420_src+left+j+yuv420_width*(i+top+13), 64);
            memcpy(nv12t_dest+tiled_offset+64*14, yuv420_src+left+j+yuv420_width*(i+top+14), 64);
            memcpy(nv12t_dest+tiled_offset+64*15, yuv420_src+left+j+yuv420_width*(i+top+15), 64);
            memcpy(nv12t_dest+tiled_offset+64*16, yuv420_src+left+j+yuv420_width*(i+top+16), 64);
            memcpy(nv12t_dest+tiled_offset+64*17, yuv420_src+left+j+yuv420_width*(i+top+17), 64);
            memcpy(nv12t_dest+tiled_offset+64*18, yuv420_src+left+j+yuv420_width*(i+top+18), 64);
            memcpy(nv12t_dest+tiled_offset+64*19, yuv420_src+left+j+yuv420_width*(i+top+19), 64);
            memcpy(nv12t_dest+tiled_offset+64*20, yuv420_src+left+j+yuv420_width*(i+top+20), 64);
            memcpy(nv12t_dest+tiled_offset+64*21, yuv420_src+left+j+yuv420_width*(i+top+21), 64);
            memcpy(nv12t_dest+tiled_offset+64*22, yuv420_src+left+j+yuv420_width*(i+top+22), 64);
            memcpy(nv12t_dest+tiled_offset+64*23, yuv420_src+left+j+yuv420_width*(i+top+23), 64);
            memcpy(nv12t_dest+tiled_offset+64*24, yuv420_src+left+j+yuv420_width*(i+top+24), 64);
            memcpy(nv12t_dest+tiled_offset+64*25, yuv420_src+left+j+yuv420_width*(i+top+25), 64);
            memcpy(nv12t_dest+tiled_offset+64*26, yuv420_src+left+j+yuv420_width*(i+top+26), 64);
            memcpy(nv12t_dest+tiled_offset+64*27, yuv420_src+left+j+yuv420_width*(i+top+27), 64);
            memcpy(nv12t_dest+tiled_offset+64*28, yuv420_src+left+j+yuv420_width*(i+top+28), 64);
            memcpy(nv12t_dest+tiled_offset+64*29, yuv420_src+left+j+yuv420_width*(i+top+29), 64);
            memcpy(nv12t_dest+tiled_offset+64*30, yuv420_src+left+j+yuv420_width*(i+top+30), 64);
            memcpy(nv12t_dest+tiled_offset+64*31, yuv420_src+left+j+yuv420_width*(i+top+31), 64);
        }
    }

    for (i=aligned_y_size; i<(yuv420_height-top-buttom); i=i+2) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = (((yuv420_height-top-buttom)+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            temp1 = i&0x1F;
            memcpy(nv12t_dest+tiled_offset+64*(temp1), yuv420_src+left+j+yuv420_width*(i+top), 64);
            memcpy(nv12t_dest+tiled_offset+64*(temp1+1), yuv420_src+left+j+yuv420_width*(i+top+1), 64);
        }
    }

    for (i=0; i<(yuv420_height-top-buttom); i=i+2) {
        for (j=aligned_x_size; j<(yuv420_width-left-right); j=j+2) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = (((yuv420_height-top-buttom)+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            temp1 = i&0x1F;
            temp2 = j&0x3F;
            memcpy(nv12t_dest+tiled_offset+temp2+64*(temp1), yuv420_src+left+j+yuv420_width*(i+top), 2);
            memcpy(nv12t_dest+tiled_offset+temp2+64*(temp1+1), yuv420_src+left+j+yuv420_width*(i+top+1), 2);
        }
    }

}

/*
 * Converts and Interleaves linear to tiled
 * Crops left, top, right, buttom
 * 1. UV of YUV420P to UV of NV12T
 *
 * @param nv12t_uv_dest
 *   UV plane address of NV12T[out]
 *
 * @param yuv420p_u_src
 *   U plane address of YUV420P[in]
 *
 * @param yuv420p_v_src
 *   V plane address of YUV420P[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_uv_height
 *   Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
static void csc_linear_to_tiled_interleave_crop(
    unsigned char *nv12t_uv_dest,
    unsigned char *yuv420_u_src,
    unsigned char *yuv420_v_src,
    unsigned int yuv420_width,
    unsigned int yuv420_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom)
{
    unsigned int i, j;
    unsigned int tiled_x_index = 0, tiled_y_index = 0;
    unsigned int aligned_x_size = 0, aligned_y_size = 0;
    unsigned int tiled_offset = 0;
    unsigned int temp1 = 0, temp2 = 0;

    aligned_y_size = ((yuv420_height-top-buttom)>>5)<<5;
    aligned_x_size = ((yuv420_width-left-right)>>6)<<6;

    for (i=0; i<aligned_y_size; i=i+32) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = (((yuv420_height-top-buttom)+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*1,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+1),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+1), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*2,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+2),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+2), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*3,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+3),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+3), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*4,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+4),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+4), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*5,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+5),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+5), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*6,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+6),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+6), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*7,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+7),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+7), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*8,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+8),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+8), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*9,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+9),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+9), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*10,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+10),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+10), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*11,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+11),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+11), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*12,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+12),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+12), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*13,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+13),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+13), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*14,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+14),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+14), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*15,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+15),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+15), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*16,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+16),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+16), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*17,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+17),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+17), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*18,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+18),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+18), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*19,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+19),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+19), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*20,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+20),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+20), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*21,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+21),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+21), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*22,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+22),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+22), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*23,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+23),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+23), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*24,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+24),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+24), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*25,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+25),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+25), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*26,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+26),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+26), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*27,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+27),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+27), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*28,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+28),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+28), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*29,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+29),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+29), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*30,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+30),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+30), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*31,
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top+31),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top+31), 32);

        }
    }

    for (i=aligned_y_size; i<(yuv420_height-top-buttom); i=i+1) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = (((yuv420_height-top-buttom)+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }
            temp1 = i&0x1F;
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*(temp1),
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top), 32);
       }
    }

    for (i=0; i<(yuv420_height-top-buttom); i=i+1) {
        for (j=aligned_x_size; j<(yuv420_width-left-right); j=j+2) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = (((yuv420_height-top-buttom)+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = (((yuv420_width-left-right)+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }
            temp1 = i&0x1F;
            temp2 = j&0x3F;
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+temp2+64*(temp1),
                                    yuv420_u_src+left/2+j/2+yuv420_width/2*(i+top),
                                    yuv420_v_src+left/2+j/2+yuv420_width/2*(i+top), 1);
       }
    }

}


/*
 * Converts tiled data to linear
 * Crops left, top, right, buttom
 * 1. Y of NV12T to Y of YUV420P
 * 2. Y of NV12T to Y of YUV420S
 * 3. UV of NV12T to UV of YUV420S
 *
 * @param yuv420_dest
 *   Y or UV plane address of YUV420[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: Height of YUV420, UV: Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
void csc_tiled_to_linear_crop_neon(
    unsigned char *yuv420_dest,
    unsigned char *nv12t_src,
    unsigned int yuv420_width,
    unsigned int yuv420_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom);

/*
 * Converts and Deinterleaves tiled data to linear
 * Crops left, top, right, buttom
 * 1. UV of NV12T to UV of YUV420P
 *
 * @param yuv420_u_dest
 *   U plane address of YUV420P[out]
 *
 * @param yuv420_v_dest
 *   V plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_uv_height
 *   Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
void csc_tiled_to_linear_deinterleave_crop_neon(
    unsigned char *yuv420_u_dest,
    unsigned char *yuv420_v_dest,
    unsigned char *nv12t_uv_src,
    unsigned int yuv420_width,
    unsigned int yuv420_uv_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom);

/*
 * Converts linear data to tiled
 * Crops left, top, right, buttom
 * 1. Y of YUV420P to Y of NV12T
 * 2. Y of YUV420S to Y of NV12T
 * 3. UV of YUV420S to UV of NV12T
 *
 * @param nv12t_dest
 *   Y or UV plane address of NV12T[out]
 *
 * @param yuv420_src
 *   Y or UV plane address of YUV420P(S)[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: Height of YUV420, UV: Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
void csc_linear_to_tiled_crop_neon(
    unsigned char *nv12t_dest,
    unsigned char *yuv420_src,
    unsigned int yuv420_width,
    unsigned int yuv420_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom);

/*
 * Converts and Interleaves linear to tiled
 * Crops left, top, right, buttom
 * 1. UV of YUV420P to UV of NV12T
 *
 * @param nv12t_uv_dest
 *   UV plane address of NV12T[out]
 *
 * @param yuv420p_u_src
 *   U plane address of YUV420P[in]
 *
 * @param yuv420p_v_src
 *   V plane address of YUV420P[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_uv_height
 *   Height/2 of YUV420[in]
 *
 * @param left
 *   Crop size of left
 *
 * @param top
 *   Crop size of top
 *
 * @param right
 *   Crop size of right
 *
 * @param buttom
 *   Crop size of buttom
 */
void csc_linear_to_tiled_interleave_crop_neon(
    unsigned char *nv12t_uv_dest,
    unsigned char *yuv420_u_src,
    unsigned char *yuv420_v_src,
    unsigned int yuv420_width,
    unsigned int yuv420_height,
    unsigned int left,
    unsigned int top,
    unsigned int right,
    unsigned int buttom);

/*
 * Converts tiled data to linear.
 * 1. y of nv12t to y of yuv420p
 * 2. y of nv12t to y of yuv420s
 *
 * @param dst
 *   y address of yuv420[out]
 *
 * @param src
 *   y address of nv12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *   it should be even
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *   it should be even.
 *
 */
void csc_tiled_to_linear_y(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{
    csc_tiled_to_linear_crop(y_dst, y_src, width, height, 0, 0, 0, 0);
}

/*
 * Converts tiled data to linear
 * 1. uv of nv12t to y of yuv420s
 *
 * @param dst
 *   uv address of yuv420s[out]
 *
 * @param src
 *   uv address of nv12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420s[in]
 *
 * @param yuv420_height
 *   real height of yuv420s[in]
 *
 */
void csc_tiled_to_linear_uv(
    unsigned char *uv_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
    csc_tiled_to_linear_crop(uv_dst, uv_src, width, height, 0, 0, 0, 0);
}

/*
 * Converts tiled data to linear
 * 1. uv of nt12t to uv of yuv420p
 *
 * @param u_dst
 *   u address of yuv420p[out]
 *
 * @param v_dst
 *   v address of yuv420p[out]
 *
 * @param uv_src
 *   uv address of nt12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420p[in]
 *
 * @param yuv420_height
 *   real height of yuv420p[in]
 */
void csc_tiled_to_linear_uv_deinterleave(
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
    csc_tiled_to_linear_deinterleave_crop(u_dst, v_dst, uv_src, width, height,
                                          0, 0, 0, 0);
}

/*
 * Converts linear data to tiled
 * 1. y of yuv420 to y of nv12t
 *
 * @param dst
 *   y address of nv12t[out]
 *
 * @param src
 *   y address of yuv420[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *   it should be even
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *   it should be even.
 *
 */
void csc_linear_to_tiled_y(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{
    csc_linear_to_tiled_crop(y_dst, y_src, width, height, 0, 0, 0, 0);
}

/*
 * Converts and interleaves linear data to tiled
 * 1. uv of nv12t to uv of yuv420
 *
 * @param dst
 *   uv address of nv12t[out]
 *
 * @param src
 *   u address of yuv420[in]
 *
 * @param src
 *   v address of yuv420[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *
 */
void csc_linear_to_tiled_uv(
    unsigned char *uv_dst,
    unsigned char *u_src,
    unsigned char *v_src,
    unsigned int width,
    unsigned int height)
{
    csc_linear_to_tiled_interleave_crop(uv_dst, u_src, v_src, width, height,
                                        0, 0, 0, 0);
}

/*
 * Converts tiled data to linear for mfc 6.x
 * 1. Y of NV12T to Y of YUV420P
 * 2. Y of NV12T to Y of YUV420S
 *
 * @param dst
 *   Y address of YUV420[out]
 *
 * @param src
 *   Y address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: real height of YUV420[in]
 *
 */
void csc_tiled_to_linear_y_neon(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{
    csc_tiled_to_linear_crop_neon(y_dst, y_src, width, height, 0, 0, 0, 0);
}

/*
 * Converts tiled data to linear for mfc 6.x
 * 1. UV of NV12T to Y of YUV420S
 *
 * @param u_dst
 *   UV plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]
 *
 * @param yuv420_height
 *   (real height)/2 of YUV420[in]
 */
void csc_tiled_to_linear_uv_neon(
    unsigned char *uv_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
    csc_tiled_to_linear_crop_neon(uv_dst, uv_src, width, height, 0, 0, 0, 0);
}

/*
 * Converts tiled data to linear for mfc 6.x
 * Deinterleave src to u_dst, v_dst
 * 1. UV of NV12T to Y of YUV420P
 *
 * @param u_dst
 *   U plane address of YUV420P[out]
 *
 * @param v_dst
 *   V plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]
 *
 * @param yuv420_height
 *   (real height)/2 of YUV420[in]
 */
void csc_tiled_to_linear_uv_deinterleave_neon(
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
    csc_tiled_to_linear_deinterleave_crop_neon(u_dst, v_dst, uv_src, width, height,
                                          0, 0, 0, 0);
}

/*
 * Converts linear data to tiled
 * 1. y of yuv420 to y of nv12t
 *
 * @param dst
 *   y address of nv12t[out]
 *
 * @param src
 *   y address of yuv420[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *   it should be even
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *   it should be even.
 *
 */
void csc_linear_to_tiled_y_neon(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{
    csc_linear_to_tiled_crop_neon(y_dst, y_src, width, height, 0, 0, 0, 0);
}

/*
 * Converts and interleaves linear data to tiled
 * 1. uv of nv12t to uv of yuv420
 *
 * @param dst
 *   uv address of nv12t[out]
 *
 * @param src
 *   u address of yuv420[in]
 *
 * @param src
 *   v address of yuv420[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *
 */
void csc_linear_to_tiled_uv_neon(
    unsigned char *uv_dst,
    unsigned char *u_src,
    unsigned char *v_src,
    unsigned int width,
    unsigned int height)
{
    csc_linear_to_tiled_interleave_crop_neon(uv_dst, u_src, v_src,
                                             width, height, 0, 0, 0, 0);
}

/*
 * Converts RGB565 to YUV420P
 *
 * @param y_dst
 *   Y plane address of YUV420P[out]
 *
 * @param u_dst
 *   U plane address of YUV420P[out]
 *
 * @param v_dst
 *   V plane address of YUV420P[out]
 *
 * @param rgb_src
 *   Address of RGB565[in]
 *
 * @param width
 *   Width of RGB565[in]
 *
 * @param height
 *   Height of RGB565[in]
 */
void csc_RGB565_to_YUV420P(
    unsigned char *y_dst,
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset1 = width * height;
    unsigned int offset2 = width/2 * height/2;

    unsigned short int *pSrc = (unsigned short int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstU = (unsigned char *)u_dst;
    unsigned char *pDstV = (unsigned char *)v_dst;

    unsigned int yIndex = 0;
    unsigned int uIndex = 0;
    unsigned int vIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x0000F800) >> 8;
            G = (tmp & 0x000007E0) >> 3;
            B = (tmp & 0x0000001F);
            B = B << 3;

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstU[uIndex++] = (unsigned char)U;
                pDstV[vIndex++] = (unsigned char)V;
            }
        }
    }
}

/*
 * Converts RGB565 to YUV420SP
 *
 * @param y_dst
 *   Y plane address of YUV420SP[out]
 *
 * @param uv_dst
 *   UV plane address of YUV420SP[out]
 *
 * @param rgb_src
 *   Address of RGB565[in]
 *
 * @param width
 *   Width of RGB565[in]
 *
 * @param height
 *   Height of RGB565[in]
 */
void csc_RGB565_to_YUV420SP(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset = width * height;

    unsigned short int *pSrc = (unsigned short int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstUV = (unsigned char *)uv_dst;

    unsigned int yIndex = 0;
    unsigned int uvIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x0000F800) >> 11;
            R = R * 8;
            G = (tmp & 0x000007E0) >> 5;
            G = G * 4;
            B = (tmp & 0x0000001F);
            B = B * 8;

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstUV[uvIndex++] = (unsigned char)U;
                pDstUV[uvIndex++] = (unsigned char)V;
            }
        }
    }
}

/*
 * Converts ARGB8888 to YUV420P
 *
 * @param y_dst
 *   Y plane address of YUV420P[out]
 *
 * @param u_dst
 *   U plane address of YUV420P[out]
 *
 * @param v_dst
 *   V plane address of YUV420P[out]
 *
 * @param rgb_src
 *   Address of ARGB8888[in]
 *
 * @param width
 *   Width of ARGB8888[in]
 *
 * @param height
 *   Height of ARGB8888[in]
 */
void csc_ARGB8888_to_YUV420P(
    unsigned char *y_dst,
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset1 = width * height;
    unsigned int offset2 = width/2 * height/2;

    unsigned int *pSrc = (unsigned int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstU = (unsigned char *)u_dst;
    unsigned char *pDstV = (unsigned char *)v_dst;

    unsigned int yIndex = 0;
    unsigned int uIndex = 0;
    unsigned int vIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x00FF0000) >> 16;
            G = (tmp & 0x0000FF00) >> 8;
            B = (tmp & 0x000000FF);

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstU[uIndex++] = (unsigned char)U;
                pDstV[vIndex++] = (unsigned char)V;
            }
        }
    }
}


/*
 * Converts ARGB8888 to YUV420SP
 *
 * @param y_dst
 *   Y plane address of YUV420SP[out]
 *
 * @param uv_dst
 *   UV plane address of YUV420SP[out]
 *
 * @param rgb_src
 *   Address of ARGB8888[in]
 *
 * @param width
 *   Width of ARGB8888[in]
 *
 * @param height
 *   Height of ARGB8888[in]
 */
void csc_ARGB8888_to_YUV420SP(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset = width * height;

    unsigned int *pSrc = (unsigned int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstUV = (unsigned char *)uv_dst;

    unsigned int yIndex = 0;
    unsigned int uvIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x00FF0000) >> 16;
            G = (tmp & 0x0000FF00) >> 8;
            B = (tmp & 0x000000FF);

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstUV[uvIndex++] = (unsigned char)U;
                pDstUV[uvIndex++] = (unsigned char)V;
            }
        }
    }
}