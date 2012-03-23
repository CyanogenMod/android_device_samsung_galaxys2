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
 * @file    csc_linear_to_tiled_crop_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2012.02.01 : Create
 */

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
    .global csc_linear_to_tiled_crop_neon
    .type   csc_linear_to_tiled_crop_neon, %function
csc_linear_to_tiled_crop_neon:
    .fnstart

    @r0     tiled_dest
    @r1     linear_src
    @r2     yuv420_width
    @r3     yuv420_height
    @r4     j
    @r5     i
    @r6     nn(tiled_addr)
    @r7     mm(linear_addr)
    @r8     aligned_x_size
    @r9     aligned_y_size
    @r10    temp1
    @r11    temp2
    @r12    temp3
    @r14    temp4

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    ldr         r11, [sp, #44]           @ top
    ldr         r14, [sp, #52]           @ buttom
    ldr         r10, [sp, #40]           @ left
    ldr         r12, [sp, #48]           @ right

    sub         r9, r3, r11             @ aligned_y_size = ((yuv420_height-top-buttom)>>5)<<5
    sub         r9, r9, r14
    bic         r9, r9, #0x1F

    sub         r8, r2, r10             @ aligned_x_size = ((yuv420_width-left-right)>>6)<<6
    sub         r8, r8, r12
    bic         r8, r8, #0x3F

    mov         r5, #0                  @ i = 0
LOOP_ALIGNED_Y_SIZE:

    mov         r4, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE:

    bl          GET_TILED_OFFSET

    ldr         r10, [sp, #44]          @ r10 = top
    ldr         r14, [sp, #40]          @ r14 = left
    add         r10, r5, r10            @ temp1 = linear_x_size*(i+top)
    mul         r10, r2, r10
    add         r7, r1, r4              @ linear_addr = linear_src+j
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1
    add         r7, r7, r14             @ linear_addr = linear_addr+left
    sub         r10, r2, #32

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*1, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*2, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*3, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    add         r6, r0, r6              @ tiled_addr = tiled_dest+tiled_addr
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*1}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*2}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*3}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*4, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*5, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*6, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*7, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*4}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*5}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*6}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*7}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*8, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*9, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*10, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*11, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*8}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*9}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*10}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*11}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*12, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*13, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*14, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*15, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*12}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*13}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*14}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*15}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*16, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*17, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*18, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*19, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*16}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*17}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*18}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*19}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*20, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*21, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*22, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*23, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*20}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*21}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*22}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*23}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*24, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*25, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*26, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*27, 64}
    pld         [r7, r2]
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*24}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*25}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*26}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*27}
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src+linear_x_size*28, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*29, 64}
    pld         [r7, r2]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2]
    vld1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*30, 64}
    pld         [r7, r2]
    vld1.8      {q10, q11}, [r7], r10
    vld1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*31, 64}
    vld1.8      {q14, q15}, [r7], r10
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr+64*28}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*29}
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*30}
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*31}
    vst1.8      {q14, q15}, [r6]!

    add         r4, r4, #64             @ j = j+64
    cmp         r4, r8                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE

    add         r5, r5, #32             @ i = i+32
    cmp         r5, r9                  @ i<aligned_y_size
    blt         LOOP_ALIGNED_Y_SIZE

    ldr         r10, [sp, #44]          @ r10 = top
    ldr         r11, [sp, #52]          @ r11 = buttom
    sub         r10, r3, r10
    sub         r10, r10, r11
    cmp         r5, r10                 @ i == (yuv420_height-top-buttom)
    beq         LOOP_LINEAR_Y_SIZE_2_START

LOOP_LINEAR_Y_SIZE_1:

    mov         r4, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE_1:

    bl          GET_TILED_OFFSET

    ldr         r10, [sp, #44]          @ r10 = top
    ldr         r14, [sp, #40]          @ r14 = left
    add         r10, r5, r10            @ temp1 = yuv420_width*(i+top)
    mul         r10, r2, r10
    add         r7, r1, r4              @ linear_addr = linear_src+j
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1
    add         r7, r7, r14             @ linear_addr = linear_addr+left
    sub         r10, r2, #32            @ temp1 = yuv420_width-32

    pld         [r7, r2]
    vld1.8      {q0, q1}, [r7]!         @ load {linear_src, 64}
    pld         [r7, r2]
    vld1.8      {q2, q3}, [r7], r10
    vld1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*1, 64}
    vld1.8      {q6, q7}, [r7]
    add         r6, r0, r6              @ tiled_addr = tiled_dest+tiled_addr
    and         r10, r5, #0x1F          @ temp1 = i&0x1F
    mov         r10, r10, lsl #6        @ temp1 = 64*temp1
    add         r6, r6, r10             @ tiled_addr = tiled_addr+temp1
    vst1.8      {q0, q1}, [r6]!         @ store {tiled_addr}
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*1}
    vst1.8      {q6, q7}, [r6]!

    add         r4, r4, #64             @ j = j+64
    cmp         r4, r8                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE_1

    add         r5, r5, #2              @ i = i+2
    ldr         r10, [sp, #44]          @ r10 = top
    ldr         r14, [sp, #52]          @ r14 = buttom
    sub         r10, r3, r10
    sub         r10, r10, r14
    cmp         r5, r10                 @ i<yuv420_height-top-buttom
    blt         LOOP_LINEAR_Y_SIZE_1

LOOP_LINEAR_Y_SIZE_2_START:
    ldr         r10, [sp, #40]          @ r10 = left
    ldr         r11, [sp, #48]          @ r11 = right
    sub         r10, r2, r10
    sub         r10, r10, r11
    cmp         r8, r10                 @ aligned_x_size == (yuv420_width-left-right)
    beq         RESTORE_REG

    mov         r5, #0                  @ i = 0
LOOP_LINEAR_Y_SIZE_2:

    mov         r4, r8                  @ j = aligned_x_size
LOOP_LINEAR_X_SIZE_2:

    bl          GET_TILED_OFFSET

    ldr         r10, [sp, #44]          @ r14 = top
    ldr         r14, [sp, #40]          @ r10 = left
    add         r10, r5, r10
    mul         r10, r2, r10            @ temp1 = linear_x_size*(i+top)
    add         r7, r1, r4              @ linear_addr = linear_src+j
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1
    add         r7, r7, r14             @ linear_addr = linear_addr+left

    add         r6, r0, r6              @ tiled_addr = tiled_dest+tiled_addr
    and         r11, r5, #0x1F          @ temp2 = i&0x1F
    mov         r11, r11, lsl #6        @ temp2 = 64*temp2
    add         r6, r6, r11             @ tiled_addr = tiled_addr+temp2
    and         r11, r4, #0x3F          @ temp2 = j&0x3F
    add         r6, r6, r11             @ tiled_addr = tiled_addr+temp2

    ldrh        r10, [r7], r2
    ldrh        r11, [r7]
    strh        r10, [r6], #64
    strh        r11, [r6]

    ldr         r12, [sp, #40]          @ r12 = left
    ldr         r14, [sp, #48]          @ r14 = right
    add         r4, r4, #2              @ j = j+2
    sub         r12, r2, r12
    sub         r12, r12, r14
    cmp         r4, r12                 @ j<(yuv420_width-left-right)
    blt         LOOP_LINEAR_X_SIZE_2

    ldr         r12, [sp, #44]          @ r12 = top
    ldr         r14, [sp, #52]          @ r14 = buttom
    add         r5, r5, #2              @ i = i+2
    sub         r12, r3, r12
    sub         r12, r12, r14
    cmp         r5, r12                 @ i<(yuv420_height-top-buttom)
    blt         LOOP_LINEAR_Y_SIZE_2

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

GET_TILED_OFFSET:

    mov         r11, r5, asr #5         @ temp2 = i>>5
    mov         r10, r4, asr #6         @ temp1 = j>>6

    and         r12, r11, #0x1          @ if (temp2 & 0x1)
    cmp         r12, #0x1
    bne         GET_TILED_OFFSET_EVEN_FORMULA_1

GET_TILED_OFFSET_ODD_FORMULA:
    sub         r6, r11, #1             @ tiled_addr = temp2-1

    ldr         r7, [sp, #40]          @ left
    add         r12, r2, #127           @ temp3 = linear_x_size+127
    sub         r12, r12, r7
    ldr         r7, [sp, #48]          @ right
    sub         r12, r12, r7
    bic         r12, r12, #0x7F         @ temp3 = (temp3 >>7)<<7
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    mul         r6, r6, r12             @ tiled_addr = tiled_addr*temp3
    add         r6, r6, r10             @ tiled_addr = tiled_addr+temp1
    add         r6, r6, #2              @ tiled_addr = tiled_addr+2
    bic         r12, r10, #0x3          @ temp3 = (temp1>>2)<<2
    add         r6, r6, r12             @ tiled_addr = tiled_addr+temp3
    mov         r6, r6, lsl #11         @ tiled_addr = tiled_addr<<11
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_1:
    ldr         r7, [sp, #44]          @ top
    add         r12, r3, #31            @ temp3 = linear_y_size+31
    sub         r12, r12, r7
    ldr         r7, [sp, #52]          @ buttom
    sub         r12, r12, r7
    bic         r12, r12, #0x1F         @ temp3 = (temp3>>5)<<5
    sub         r12, r12, #32           @ temp3 = temp3 - 32
    cmp         r5, r12                 @ if (i<(temp3-32)) {
    bge         GET_TILED_OFFSET_EVEN_FORMULA_2
    add         r12, r10, #2            @ temp3 = temp1+2
    bic         r12, r12, #3            @ temp3 = (temp3>>2)<<2
    add         r6, r10, r12            @ tiled_addr = temp1+temp3
    ldr         r7, [sp, #40]          @ left
    add         r12, r2, #127           @ temp3 = linear_x_size+127
    sub         r12, r12, r7
    ldr         r7, [sp, #48]          @ right
    sub         r12, r12, r7
    bic         r12, r12, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    mul         r11, r11, r12           @ tiled_y_index = tiled_y_index*temp3
    add         r6, r6, r11             @ tiled_addr = tiled_addr+tiled_y_index
    mov         r6, r6, lsl #11         @
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_2:
    ldr         r7, [sp, #40]          @ left
    add         r12, r2, #127           @ temp3 = linear_x_size+127
    sub         r12, r12, r7
    ldr         r7, [sp, #48]          @ right
    sub         r12, r12, r7
    bic         r12, r12, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    mul         r6, r11, r12            @ tiled_addr = temp2*temp3
    add         r6, r6, r10             @ tiled_addr = tiled_addr+temp3
    mov         r6, r6, lsl #11         @ tiled_addr = tiled_addr<<11@

GET_TILED_OFFSET_RETURN:
    mov         pc, lr

    .fnend
