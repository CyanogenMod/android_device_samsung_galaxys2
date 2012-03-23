/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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
 * @file    csc_linear_to_tiled_interleave_crop_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2012.02.01 : Create
 */

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
 *   Crop size of left. It should be even.
 *
 * @param top
 *   Crop size of top. It should be even.
 *
 * @param right
 *   Crop size of right. It should be even.
 *
 * @param buttom
 *   Crop size of buttom. It should be even.
 */

    .arch armv7-a
    .text
    .global csc_linear_to_tiled_interleave_crop_neon
    .type   csc_linear_to_tiled_interleave_crop_neon, %function
csc_linear_to_tiled_interleave_crop_neon:
    .fnstart

    @r0     tiled_dest
    @r1     linear_src_u
    @r2     linear_src_v
    @r3     yuv420_width
    @r4     yuv420_height
    @r5     j
    @r6     i
    @r7     tiled_addr
    @r8     linear_addr
    @r9     aligned_x_size
    @r10    aligned_y_size
    @r11    temp1
    @r12    temp2
    @r14    temp3

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    ldr         r4, [sp, #40]           @ load linear_y_size to r4

    ldr         r10, [sp, #48]          @ r10 = top
    ldr         r14, [sp, #56]          @ r14 = buttom
    ldr         r11, [sp, #44]          @ r11 = left
    ldr         r12, [sp, #52]          @ r12 = right

    sub         r10, r4, r10            @ aligned_y_size = ((yuv420_height-top-buttom)>>5)<<5
    sub         r10, r10, r14
    bic         r10, r10, #0x1F
    sub         r11, r3, r11            @ aligned_x_size = ((yuv420_width-left-right)>>6)<<6
    sub         r11, r11, r12
    bic         r9, r11, #0x3F

    mov         r6, #0                  @ i = 0
LOOP_ALIGNED_Y_SIZE:

    mov         r5, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE:

    bl          GET_TILED_OFFSET

    ldr         r12, [sp, #48]          @ r12 = top
    ldr         r8, [sp, #44]           @ r8 = left

    mov         r11, r3, asr #1         @ temp1 = (yuv420_width/2)*(i+top)
    add         r12, r6, r12
    mul         r11, r11, r12
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    add         r11, r11, r8, asr #1    @ temp1 = temp1+left/2

    mov         r12, r3, asr #1         @ temp2 = yuv420_width/2
    sub         r12, r12, #16           @ temp2 = yuv420_width-16

    add         r8, r1, r11             @ linear_addr = linear_src_u+temp1
    add         r11, r2, r11            @ temp1 = linear_src_v+temp1
    add         r7, r0, r7              @ tiled_addr = tiled_dest+tiled_addr

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*1, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*2, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*3, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*1, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*2, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*3, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*1}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*2}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*3}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*4, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*5, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*6, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*7, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*4, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*5, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*6, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*7, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*4}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*5}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*6}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*7}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*8, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*9, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*10, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*11, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*8, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*9, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*10, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*11, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*8}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*9}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*10}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*11}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*12, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*13, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*14, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*15, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*12, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*13, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*14, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*15, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*12}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*13}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*14}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*15}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*16, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*17, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*18, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*19, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*16, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*17, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*18, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*19, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*16}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*17}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*18}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*19}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*20, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*21, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*22, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*23, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*20, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*21, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*22, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*23, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*20}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*21}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*22}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*23}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*24, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*25, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*26, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*27, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*24, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*25, 32}
    vld1.8      {q7}, [r11], r12
    pld         [r8]
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*26, 32}
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3, asr #1]
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*27, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*24}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*25}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*26}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*27}
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*28, 32}
    vld1.8      {q2}, [r8], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*29, 32}
    vld1.8      {q6}, [r8], r12
    pld         [r11]
    vld1.8      {q8}, [r8]!             @ load {linear_src_u+(linear_x_size/2)*30, 32}
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3, asr #1]
    vld1.8      {q12}, [r8]!            @ load {linear_src_u+(linear_x_size/2)*31, 32}
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*28, 32}
    vld1.8      {q3}, [r11], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*29, 32}
    vld1.8      {q7}, [r11], r12
    vld1.8      {q9}, [r11]!            @ load {linear_src_v+(linear_x_size/2)*30, 32}
    vld1.8      {q11}, [r11], r12
    vld1.8      {q13}, [r11]!           @ load {linear_src_v+(linear_x_size/2)*31, 32}
    vld1.8      {q15}, [r11], r12
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr+64*28}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*29}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*30}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*31}
    vst2.8      {q14, q15}, [r7]!

    add         r5, r5, #64             @ j = j+64
    cmp         r5, r9                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE

    add         r6, r6, #32             @ i = i+32
    cmp         r6, r10                 @ i<aligned_y_size
    blt         LOOP_ALIGNED_Y_SIZE

    cmp         r6, r4
    beq         LOOP_LINEAR_Y_SIZE_2_START

LOOP_LINEAR_Y_SIZE_1:

    mov         r5, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE_1:

    bl          GET_TILED_OFFSET

    ldr         r12, [sp, #48]          @ r12 = top
    ldr         r8, [sp, #44]           @ r8 = left

    mov         r11, r3, asr #1         @ temp1 = (yuv420_width/2)*(i+top)
    add         r12, r6, r12
    mul         r11, r11, r12
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    add         r11, r11, r8, asr #1    @ temp1 = temp1+left/2

    add         r8, r1, r11             @ linear_addr = linear_src_u+temp1
    add         r11, r2, r11            @ temp1 = linear_src_v+temp1
    add         r7, r0, r7              @ tiled_addr = tiled_dest+tiled_addr
    and         r14, r6, #0x1F          @ temp3 = i&0x1F@
    mov         r14, r14, lsl #6        @ temp3 = temp3*64
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3

    vld1.8      {q0}, [r8]!             @ load {linear_src_u, 32}
    vld1.8      {q2}, [r8]
    vld1.8      {q1}, [r11]!            @ load {linear_src_v, 32}
    vld1.8      {q3}, [r11]
    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr}
    vst2.8      {q2, q3}, [r7]!

    add         r5, r5, #64             @ j = j+64
    cmp         r5, r9                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE_1

    ldr         r12, [sp, #48]          @ r12 = top
    ldr         r8, [sp, #56]           @ r8 = buttom
    add         r6, r6, #1              @ i = i+1
    sub         r12, r4, r12
    sub         r12, r12, r8
    cmp         r6, r12                 @ i<(yuv420_height-top-buttom)
    blt         LOOP_LINEAR_Y_SIZE_1

LOOP_LINEAR_Y_SIZE_2_START:
    cmp         r5, r3
    beq         RESTORE_REG

    mov         r6, #0                  @ i = 0
LOOP_LINEAR_Y_SIZE_2:

    mov         r5, r9                  @ j = aligned_x_size
LOOP_LINEAR_X_SIZE_2:

    bl          GET_TILED_OFFSET

    ldr         r12, [sp, #48]          @ r12 = top
    ldr         r8, [sp, #44]           @ r8 = left

    mov         r11, r3, asr #1         @ temp1 = (yuv420_width/2)*(i+top)
    add         r12, r6, r12
    mul         r11, r11, r12
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    add         r11, r11, r8, asr #1    @ temp1 = temp1+left/2

    mov         r12, r3, asr #1         @ temp2 = linear_x_size/2
    sub         r12, r12, #1            @ temp2 = linear_x_size-1

    add         r8, r1, r11             @ linear_addr = linear_src_u+temp1
    add         r11, r2, r11            @ temp1 = linear_src_v+temp1
    add         r7, r0, r7              @ tiled_addr = tiled_dest+tiled_addr
    and         r14, r6, #0x1F          @ temp3 = i&0x1F@
    mov         r14, r14, lsl #6        @ temp3 = temp3*64
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3
    and         r14, r5, #0x3F          @ temp3 = j&0x3F
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3

    ldrb        r10, [r8], #1
    ldrb        r14, [r11], #1
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #2

    ldr         r12, [sp, #44]          @ r12 = left
    ldr         r8, [sp, #52]           @ r8 = right
    add         r5, r5, #2              @ j = j+2
    sub         r12, r3, r12
    sub         r12, r12, r8
    cmp         r5, r12                 @ j<(yuv420_width-left-right)
    blt         LOOP_LINEAR_X_SIZE_2

    ldr         r12, [sp, #48]          @ r12 = top
    ldr         r8, [sp, #56]           @ r8 = buttom
    add         r6, r6, #1              @ i = i+1
    sub         r12, r4, r12
    sub         r12, r12, r8
    cmp         r6, r12                 @ i<(yuv420_height-top-buttom)
    blt         LOOP_LINEAR_Y_SIZE_2

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

GET_TILED_OFFSET:
    stmfd       sp!, {r14}

    mov         r12, r6, asr #5         @ temp2 = i>>5
    mov         r11, r5, asr #6         @ temp1 = j>>6

    and         r14, r12, #0x1          @ if (temp2 & 0x1)
    cmp         r14, #0x1
    bne         GET_TILED_OFFSET_EVEN_FORMULA_1

GET_TILED_OFFSET_ODD_FORMULA:

    ldr         r7, [sp, #48]           @ r7 = left , (r14 was pushed to stack)
    ldr         r8, [sp, #56]           @ r8 = right , (r14 was pushed to stack)
    sub         r14, r3, r7
    sub         r14, r14, r8
    add         r14, r14, #127          @ temp3 = (((yuv420_width-left-right)+127)>>7)<<7
    bic         r14, r14, #0x7F         @ temp3 = (temp3 >>7)<<7
    mov         r14, r14, asr #6        @ temp3 = temp3>>6
    sub         r7, r12, #1             @ tiled_addr = temp2-1
    mul         r7, r7, r14             @ tiled_addr = tiled_addr*temp3
    add         r7, r7, r11             @ tiled_addr = tiled_addr+temp1
    add         r7, r7, #2              @ tiled_addr = tiled_addr+2
    bic         r14, r11, #0x3          @ temp3 = (temp1>>2)<<2
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3
    mov         r7, r7, lsl #11         @ tiled_addr = tiled_addr<<11
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_1:
    ldr         r7, [sp, #52]           @ r7 = top, (r14 was pushed to stack)
    ldr         r8, [sp, #60]           @ r8 = buttom, (r14 was pushed to stack)
    sub         r14, r4, r7
    sub         r14, r14, r8
    add         r14, r14, #31           @ temp3 = (((yuv420_height-top-buttom)+31)>>5)<<5
    bic         r14, r14, #0x1F         @ temp3 = (temp3>>5)<<5
    sub         r14, r14, #32           @ temp3 = temp3 - 32
    cmp         r6, r14                 @ if (i<(temp3-32)) {
    bge         GET_TILED_OFFSET_EVEN_FORMULA_2
    add         r14, r11, #2            @ temp3 = temp1+2
    bic         r14, r14, #3            @ temp3 = (temp3>>2)<<2
    add         r7, r11, r14            @ tiled_addr = temp1+temp3
    ldr         r8, [sp, #48]           @ r8 = left, (r14 was pushed to stack)
    sub         r14, r3, r8
    ldr         r8, [sp, #56]           @ r8 = right, (r14 was pushed to stack)
    sub         r14, r14, r8
    add         r14, r14, #127          @ temp3 = (((yuv420_width-left-right)+127)>>7)<<7
    bic         r14, r14, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r14, r14, asr #6        @ temp3 = temp3>>6
    mul         r12, r12, r14           @ tiled_y_index = tiled_y_index*temp3
    add         r7, r7, r12             @ tiled_addr = tiled_addr+tiled_y_index
    mov         r7, r7, lsl #11         @
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_2:
    ldr         r8, [sp, #48]           @ r8 = left, (r14 was pushed to stack)
    sub         r14, r3, r8
    ldr         r8, [sp, #56]           @ r8 = right, (r14 was pushed to stack)
    sub         r14, r14, r8
    add         r14, r14, #127          @ temp3 = (((yuv420_width-left-right)+127)>>7)<<7
    bic         r14, r14, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r14, r14, asr #6        @ temp3 = temp3>>6
    mul         r7, r12, r14            @ tiled_addr = temp2*temp3
    add         r7, r7, r11             @ tiled_addr = tiled_addr+temp3
    mov         r7, r7, lsl #11         @ tiled_addr = tiled_addr<<11@

GET_TILED_OFFSET_RETURN:
    ldmfd       sp!, {r15}              @ restore registers

    .fnend

