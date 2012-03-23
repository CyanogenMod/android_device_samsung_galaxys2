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
 * @file    csc_tiled_to_linear_crop_neon.s
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
    .global csc_tiled_to_linear_crop_neon
    .type   csc_tiled_to_linear_crop_neon, %function
csc_tiled_to_linear_crop_neon:
    .fnstart

    @r0     yuv420_dest
    @r1     nv12t_src
    @r2     yuv420_width
    @r3     yuv420_height
	@r4
    @r5     i
    @r6     j
    @r7     tiled_offset
    @r8     tiled_offset1
    @r9     linear_offset
    @r10    temp1
    @r11    temp2
    @r12    temp3
    @r14    temp4

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    ldr         r12, [sp, #48]          @ r12 = right
    ldr         r10, [sp, #40]          @ r10 = left
    sub         r12, r2, r12            @ temp3 = yuv420_width-right@
    sub         r10, r12, r10           @ temp1 = temp3-left@
    cmp         r10, #256               @ if (temp1 >= 256)
    blt         LOOP_HEIGHT_64_START

    ldr         r5, [sp, #44]           @ i = top
LOOP_HEIGHT_256:
    ldr         r6, [sp, #40]           @ j = left
    mov         r14, r5, asr #5         @ temp4 = i>>5
    bic         r12, r6, #0xFF          @ temp3 = (j>>8)<<8
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    and         r11, r14, #0x1          @ if (temp4 & 0x1)
    cmp         r11, #0x1
    bne         LOOP_HEIGHT_256_GET_TILED_EVEN
LOOP_HEIGHT_256_GET_TILED_ODD:
    sub         r7, r14, #1             @ tiled_offset = temp4-1
    add         r10, r2, #127           @ temp1 = ((yuv420_width+127)>>7)<<7
    bic         r10, r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)
    mul         r7, r7, r10
    add         r7, r7, r12             @ tiled_offset = tiled_offset+temp3
    add         r7, r7, #2              @ tiled_offset = tiled_offset+2
    bic         r10, r12, #0x3          @ temp1 = (temp3>>2)<<2
    add         r7, r7, r10             @ tiled_offset = tiled_offset+temp1
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11
    add         r8, r7, #4096           @ tiled_offset1 = tiled_offset+2048*2
    mov         r14, #8
    b           LOOP_HEIGHT_256_GET_TILED_END

LOOP_HEIGHT_256_GET_TILED_EVEN:
    add         r11, r3, #31            @ temp2 = ((yuv420_height+31)>>5)<<5
    bic         r11, r11, #0x1F
    add         r10, r5, #32            @ if ((i+32)<temp2)
    cmp         r10, r11
    bge         LOOP_HEIGHT_256_GET_TILED_EVEN1
    add         r10, r12, #2            @ temp1 = temp3+2
    bic         r10, r10, #0x3          @ temp1 = (temp1>>2)<<2
    add         r7, r12, r10            @ tiled_offset = temp3+temp1@
    add         r10, r2, #127           @ temp1 = ((yuv420_width+127)>>7)<<7
    bic         r10, r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset+temp4*(temp1>>6)
    mla         r7, r14, r10, r7
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11
    add         r8, r7, #12288          @ tiled_offset1 = tiled_offset+2048*6
    mov         r14, #8
    b           LOOP_HEIGHT_256_GET_TILED_END

LOOP_HEIGHT_256_GET_TILED_EVEN1:
    add         r10, r2, #127           @ temp1 = ((yuv420_width+127)>>7)<<7
    bic         r10, r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = temp4*(temp1>>6)
    mul         r7, r14, r10
    add         r7, r7, r12             @ tiled_offset = tiled_offset+temp3
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11
    add         r8, r7, #4096           @ tiled_offset1 = tiled_offset+2048*2
    mov         r14, #4

LOOP_HEIGHT_256_GET_TILED_END:

    ldr         r12, [sp, #48]          @ right
    ldr         r9, [sp, #44]           @ top
    and         r10, r5, #0x1F          @ temp1 = i&0x1F
    add         r7, r7, r10, lsl #6     @ tiled_offset = tiled_offset+64*(temp1)
    add         r8, r8, r10, lsl #6     @ tiled_offset1 = tiled_offset1+64*(temp1)
    sub         r11, r2, r6             @ temp2 = yuv420_width-left(==j)-right
    sub         r11, r11, r12
    sub         r9, r5, r9              @ linear_offset = temp2*(i-top)@
    mul         r9, r11, r9
    add         r12, r6, #256           @ temp3 = ((j+256)>>8)<<8@
    bic         r12, r12, #0xFF
    sub         r12, r12, r6            @ temp3 = temp3-j@
    and         r10, r6, #0x3F          @ temp1 = left(==j)&0x3F

    cmp         r12, #192               @ if (temp3 > 192)
    ble         LOOP_HEIGHT_256_LEFT_192
    add         r11, r1, r7             @ r11 = nv12t_src+tiled_offset+temp1
    add         r11, r11, r10
    pld         [r11]
    add         r12, r1, r7             @ r12 = nv12t_src+tiled_offset+2048
    pld         [r11, #32]
    add         r12, r12, #2048
    pld         [r12]
    cmp         r10, #0
    pld         [r12, #32]
    stmnefd     sp!, {r9-r12, r14}      @ backup registers
    rsbne       r10, r10, #64
    blne        MEMCOPY_UNDER_64
    ldmnefd     sp!, {r9-r12, r14}      @ restore registers
    bne         LOOP_HEIGHT_256_LEFT_256_64
    vld1.8      {q0, q1}, [r11]!        @ load {nv12t_src+tiled_offset+temp1, 64}
    vld1.8      {q2, q3}, [r11]
    add         r11, r0, r9             @ r11 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r11]!        @ store {yuv420_dest+linear_offset, 64}
    vst1.8      {q2, q3}, [r11]!
LOOP_HEIGHT_256_LEFT_256_64:
    add         r11, r1, r8             @ r11 = nv12t_src+tiled_offset1
    pld         [r11]
    vld1.8      {q4, q5}, [r12]!        @ load {nv12t_src+tiled_offset+2048, 64}
    pld         [r11, #32]
    vld1.8      {q6, q7}, [r12]
    add         r12, r11, #2048         @ r12 = nv12t_src+tiled_offset1+2048
    pld         [r12]
    vld1.8      {q8, q9}, [r11]!        @ load {nv12t_src+tiled_offset1, 64}
    pld         [r12, #32]
    vld1.8      {q10, q11}, [r11]
    vld1.8      {q12, q13}, [r12]!      @ load {nv12t_src+tiled_offset1+2048, 64}
    vld1.8      {q14, q15}, [r12]

    sub         r11, r0, r10            @ r11 = yuv420_dest+linear_offset+64-temp1
    add         r12, r9, #64
    add         r11, r11, r12

    vst1.8      {q4, q5}, [r11]!       @ store {yuv420_dest+linear_offset+64-temp1, 64}
    vst1.8      {q6, q7}, [r11]!
    vst1.8      {q8, q9}, [r11]!       @ store {yuv420_dest+linear_offset+128-temp1, 64}
    vst1.8      {q10, q11}, [r11]!
    vst1.8      {q12, q13}, [r11]!     @ store {yuv420_dest+linear_offset+192-temp1, 64}
    vst1.8      {q14, q15}, [r11]!

    add         r9, r9, #256
    sub         r9, r9, r10
    b           LOOP_HEIGHT_256_LEFT_END

LOOP_HEIGHT_256_LEFT_192:
    cmp         r12, #128               @ if (temp3 > 128)
    ble         LOOP_HEIGHT_256_LEFT_128
    add         r11, r1, r7             @ r11 = nv12t_src+tiled_offset+2048+temp1
    add         r11, r11, r10
    add         r11, r11, #2048
    pld         [r11]
    add         r12, r1, r8             @ r12 = nv12t_src+tiled_offset1
    pld         [r11, #32]
    cmp         r10, #0
    pld         [r12]
    stmnefd     sp!, {r9-r12, r14}      @ backup registers
    pld         [r12, #32]
    rsbne       r10, r10, #64
    blne        MEMCOPY_UNDER_64
    ldmnefd     sp!, {r9-r12, r14}      @ restore registers
    bne         LOOP_HEIGHT_256_LEFT_192_64
    vld1.8      {q0, q1}, [r11]!        @ load {nv12t_src+tiled_offset+2048+temp1, 64}
    vld1.8      {q2, q3}, [r11]
    add         r11, r0, r9             @ r11 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r11]!        @ store {yuv420_dest+linear_offset, 64}
    vst1.8      {q2, q3}, [r11]!
LOOP_HEIGHT_256_LEFT_192_64:
    add         r11, r1, r8             @ r11 = nv12t_src+tiled_offset1+2048
    add         r11, r11, #2048
    pld         [r11]
    vld1.8      {q4, q5}, [r12]!        @ load {nv12t_src+tiled_offset1, 64}
    pld         [r11, #32]
    vld1.8      {q6, q7}, [r12]
    vld1.8      {q8, q9}, [r11]!        @ load {nv12t_src+tiled_offset1+2048, 64}
    vld1.8      {q10, q11}, [r11]

    sub         r11, r0, r10            @ r11 = yuv420_dest+linear_offset+64-temp1
    add         r12, r9, #64
    add         r11, r11, r12

    vst1.8      {q4, q5}, [r11]!        @ store {yuv420_dest+linear_offset+64-temp1, 64}
    vst1.8      {q6, q7}, [r11]!
    vst1.8      {q8, q9}, [r11]!        @ store {yuv420_dest+linear_offset+128-temp1, 64}
    vst1.8      {q10, q11}, [r11]!

    add         r9, r9, #192
    sub         r9, r9, r10
    b           LOOP_HEIGHT_256_LEFT_END

LOOP_HEIGHT_256_LEFT_128:
    cmp         r12, #64                @ if (temp3 > 64)
    ble         LOOP_HEIGHT_256_LEFT_64
    add         r11, r1, r8             @ r11 = nv12t_src+tiled_offset1+temp1
    add         r11, r11, r10
    pld         [r11]
    add         r12, r1, r8             @ r12 = nv12t_src+tiled_offset1
    add         r12, r12, #2048
    pld         [r11, #32]
    cmp         r10, #0
    pld         [r12]
    stmnefd     sp!, {r9-r12, r14}      @ backup registers
    pld         [r12, #32]
    rsbne       r10, r10, #64
    blne        MEMCOPY_UNDER_64
    ldmnefd     sp!, {r9-r12, r14}      @ restore registers
    bne         LOOP_HEIGHT_256_LEFT_128_64
    vld1.8      {q0, q1}, [r11]!        @ load {nv12t_src+tiled_offset1+temp1, 64}
    vld1.8      {q2, q3}, [r11]
    add         r11, r0, r9             @ r11 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r11]!        @ store {yuv420_dest+linear_offset, 64}
    vst1.8      {q2, q3}, [r11]!
LOOP_HEIGHT_256_LEFT_128_64:
    vld1.8      {q4, q5}, [r12]!        @ load {nv12t_src+tiled_offset1, 64}
    vld1.8      {q6, q7}, [r12]

    sub         r11, r0, r10            @ r11 = yuv420_dest+linear_offset+64-temp1
    add         r12, r9, #64
    add         r11, r11, r12

    vst1.8      {q4, q5}, [r11]!        @ store {yuv420_dest+linear_offset+64-temp1, 64}
    vst1.8      {q6, q7}, [r11]!

    add         r9, r9, #128
    sub         r9, r9, r10
    b           LOOP_HEIGHT_256_LEFT_END

LOOP_HEIGHT_256_LEFT_64:
    add         r11, r1, r8             @ r11 = nv12t_src+tiled_offset1+2048+temp1
    add         r11, r11, #2048
    add         r11, r11, r10
    cmp         r10, #0
    pld         [r11]
    stmnefd     sp!, {r9-r12, r14}      @ backup registers
    pld         [r11, #32]
    rsbne       r10, r10, #64
    blne        MEMCOPY_UNDER_64
    ldmnefd     sp!, {r9-r12, r14}      @ restore registers
    bne         LOOP_HEIGHT_256_LEFT_64_64
    vld1.8      {q0, q1}, [r11]!        @ load {nv12t_src+tiled_offset1+temp1, 64}
    vld1.8      {q2, q3}, [r11]
    add         r11, r0, r9             @ r11 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r11]!        @ store {yuv420_dest+linear_offset, 64}
    vst1.8      {q2, q3}, [r11]!
LOOP_HEIGHT_256_LEFT_64_64:
    add         r9, r9, #64
    sub         r9, r9, r10

LOOP_HEIGHT_256_LEFT_END:

    ldr         r12, [sp, #48]          @ right
    add         r7, r7, r14, lsl #11    @ tiled_offset = tiled_offset+temp4*2048
    add         r10, r1, r7             @ r10 = nv12t_src+tiled_offset
    pld         [r10]
    bic         r6, r6, #0xFF           @ j = (left>>8)<<8
    pld         [r10, #32]
    add         r6, r6, #256            @ j = j + 256
    sub         r11, r2, r12            @ temp2 = yuv420_width-right-256
    sub         r11, r11, #256
    cmp         r6, r11
    bgt         LOOP_HEIGHT_256_WIDTH_END

LOOP_HEIGHT_256_WIDTH:
    add         r12, r10, #2048         @ r12 = nv12t_src+tiled_offset+2048
    pld         [r12]
    vld1.8      {q0, q1}, [r10]!        @ load {nv12t_src+tiled_offset, 64}
    pld         [r12, #32]
    vld1.8      {q2, q3}, [r10]

    add         r8, r8, r14, lsl #11    @ tiled_offset1 = tiled_offset1+temp4*2048
    add         r10, r1, r8             @ r10 = nv12t_src+tiled_offset1
    pld         [r10]
    vld1.8      {q4, q5}, [r12]!        @ load {nv12t_src+tiled_offset+2048, 64}
    pld         [r10, #32]
    vld1.8      {q6, q7}, [r12]

    add         r12, r10, #2048         @ r12 = nv12t_src+tiled_offset+2048
    pld         [r12]
    vld1.8      {q8, q9}, [r10]!        @ load {nv12t_src+tiled_offset+2048, 64}
    pld         [r12, #32]
    vld1.8      {q10, q11}, [r10]

    add         r7, r7, r14, lsl #11    @ tiled_offset = tiled_offset+temp4*2048
    add         r10, r1, r7
    pld         [r10]
    vld1.8      {q12, q13}, [r12]!      @ load {nv12t_src+tiled_offset+2048, 64}
    pld         [r10, #32]
    vld1.8      {q14, q15}, [r12]

    add         r12, r0, r9             @ r12 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r12]!
    vst1.8      {q2, q3}, [r12]!
    vst1.8      {q4, q5}, [r12]!
    vst1.8      {q6, q7}, [r12]!
    vst1.8      {q8, q9}, [r12]!
    vst1.8      {q10, q11}, [r12]!
    vst1.8      {q12, q13}, [r12]!
    vst1.8      {q14, q15}, [r12]!
    add         r9, r9, #256            @ linear_offset = linear_offset+256

    add         r12, r10, #2048         @ r12 = nv12t_src+tiled_offset+2048

    add         r6, r6, #256            @ j=j+256
    cmp         r6, r11                 @ j<=temp2
    ble         LOOP_HEIGHT_256_WIDTH

LOOP_HEIGHT_256_WIDTH_END:

    add         r8, r8, r14, lsl #11    @ tiled_offset1 = tiled_offset1+temp4*2048
    ldr         r14, [sp, #48]          @ right
    sub         r11, r2, r6             @ temp2 = yuv420_width-right-j
    sub         r11, r11, r14
    cmp         r11, #0
    beq         LOOP_HEIGHT_256_RIGHT_END
    cmp         r11, #192
    ble         LOOP_HEIGHT_256_RIGHT_192
    add         r12, r10, #2048
    pld         [r12]
    vld1.8      {q0, q1}, [r10]!        @ load {nv12t_src+tiled_offset}
    pld         [r12, #32]
    vld1.8      {q2, q3}, [r10]

    add         r10, r1, r8             @ r10 = nv12t_src+tiled_offset1
    pld         [r10]
    vld1.8      {q4, q5}, [r12]!        @ load {nv12t_src+tiled_offset+2048}
    pld         [r10, #32]
    vld1.8      {q6, q7}, [r12]

    add         r14, r10, #2048         @ r10 = nv12t_src+tiled_offset1+2048
    pld         [r14]
    vld1.8      {q8, q9}, [r10]!        @ load {nv12t_src+tiled_offset1}
    pld         [r14, #32]
    vld1.8      {q10, q11}, [r10]

    add         r12, r0, r9             @ r12 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r12]!
    vst1.8      {q2, q3}, [r12]!
    vst1.8      {q4, q5}, [r12]!
    vst1.8      {q6, q7}, [r12]!
    vst1.8      {q8, q9}, [r12]!
    vst1.8      {q10, q11}, [r12]!
    add         r9, r9, #192            @ linear_offset = linear_offset+192

    stmfd       sp!, {r9-r12, r14}      @ backup registers
    sub         r10, r11, #192
    mov         r11, r14
    bl          MEMCOPY_UNDER_64
    ldmfd       sp!, {r9-r12, r14}      @ restore registers
    b           LOOP_HEIGHT_256_RIGHT_END

LOOP_HEIGHT_256_RIGHT_192:
    cmp         r11, #128
    ble         LOOP_HEIGHT_256_RIGHT_128
    add         r12, r10, #2048
    pld         [r12]
    vld1.8      {q0, q1}, [r10]!        @ load {nv12t_src+tiled_offset}
    pld         [r12, #32]
    vld1.8      {q2, q3}, [r10]

    add         r14, r1, r8             @ r10 = nv12t_src+tiled_offset1
    pld         [r14]
    vld1.8      {q4, q5}, [r12]!        @ load {nv12t_src+tiled_offset+2048}
    pld         [r14, #32]
    vld1.8      {q6, q7}, [r12]

    add         r12, r0, r9             @ r12 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r12]!
    vst1.8      {q2, q3}, [r12]!
    vst1.8      {q4, q5}, [r12]!
    vst1.8      {q6, q7}, [r12]!
    add         r9, r9, #128            @ linear_offset = linear_offset+128

    stmfd       sp!, {r9-r12, r14}      @ backup registers
    sub         r10, r11, #128
    mov         r11, r14
    bl          MEMCOPY_UNDER_64
    ldmfd       sp!, {r9-r12, r14}      @ restore registers
    b           LOOP_HEIGHT_256_RIGHT_END

LOOP_HEIGHT_256_RIGHT_128:
    cmp         r11, #64
    ble         LOOP_HEIGHT_256_RIGHT_64
    add         r14, r10, #2048
    pld         [r14]
    vld1.8      {q0, q1}, [r10]!        @ load {nv12t_src+tiled_offset}
    pld         [r14, #32]
    vld1.8      {q2, q3}, [r10]

    add         r12, r0, r9             @ r12 = yuv420_dest+linear_offset
    vst1.8      {q0, q1}, [r12]!
    vst1.8      {q2, q3}, [r12]!
    add         r9, r9, #64            @ linear_offset = linear_offset+64

    stmfd       sp!, {r9-r12, r14}      @ backup registers
    sub         r10, r11, #64
    mov         r11, r14
    bl          MEMCOPY_UNDER_64
    ldmfd       sp!, {r9-r12, r14}      @ restore registers
    b           LOOP_HEIGHT_256_RIGHT_END

LOOP_HEIGHT_256_RIGHT_64:
    stmfd       sp!, {r9-r12, r14}      @ backup registers
    mov         r14, r11
    mov         r11, r10
    mov         r10, r14
    bl          MEMCOPY_UNDER_64
    ldmfd       sp!, {r9-r12, r14}      @ restore registers

LOOP_HEIGHT_256_RIGHT_END:

    ldr         r14, [sp, #52]          @ buttom
    add         r5, r5, #1              @ i=i+1
    sub         r14, r3, r14            @ i<yuv420_height-buttom
    cmp         r5, r14
    blt         LOOP_HEIGHT_256
    b           RESTORE_REG

LOOP_HEIGHT_64_START:
    cmp         r10, #64               @ if (temp1 >= 64)
    blt         LOOP_HEIGHT_2_START

    ldr         r5, [sp, #44]           @ i = top
LOOP_HEIGHT_64:
    ldr         r6, [sp, #40]           @ j = left
    stmfd       sp!, {r0-r3, r12}       @ backup parameters
    mov         r0, r2
    mov         r1, r3
    mov         r2, r6
    mov         r3, r5
    bl          tile_4x2_read_asm
    mov         r7, r0
    ldmfd       sp!, {r0-r3, r12}       @ restore parameters
    ldr         r9, [sp, #44]           @ linear_offset = top
    add         r11, r6, #64            @ temp2 = ((j+64)>>6)<<6
    bic         r11, r11, #0x3F
    sub         r11, r11, r6            @ temp2 = temp2-j
    sub         r9, r5, r9              @ linear_offset = temp1*(i-top)
    mul         r9, r9, r10
    and         r14, r6, #0x3           @ temp4 = j&0x3
    add         r7, r7, r14             @ tiled_offset = tiled_offset+temp4
    stmfd       sp!, {r9-r12}           @ backup parameters
    mov         r10, r11
    add         r11, r1, r7
    bl          MEMCOPY_UNDER_64
    ldmfd       sp!, {r9-r12}           @ restore parameters
    add         r9, r9, r11             @ linear_offset = linear_offset+temp2
    add         r6, r6, r11             @ j = j+temp2@

    add         r14, r6, #64
    cmp         r14, r12
    bgt         LOOP_HEIGHT_64_1
    stmfd       sp!, {r0-r3, r12}       @ backup parameters
    mov         r0, r2
    mov         r1, r3
    mov         r2, r6
    mov         r3, r5
    bl          tile_4x2_read_asm
    mov         r7, r0
    ldmfd       sp!, {r0-r3, r12}       @ restore parameters
    add         r7, r1, r7
    vld1.8      {q0, q1}, [r7]!
    vld1.8      {q2, q3}, [r7]
    add         r7, r0, r9
    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]
    add         r9, r9, #64
    add         r6, r6, #64

LOOP_HEIGHT_64_1:
    add         r14, r6, #64
    cmp         r14, r12
    bgt         LOOP_HEIGHT_64_2
    stmfd       sp!, {r0-r3, r12}       @ backup parameters
    mov         r0, r2
    mov         r1, r3
    mov         r2, r6
    mov         r3, r5
    bl          tile_4x2_read_asm
    mov         r7, r0
    ldmfd       sp!, {r0-r3, r12}       @ restore parameters
    add         r7, r1, r7
    vld1.8      {q0, q1}, [r7]!
    vld1.8      {q2, q3}, [r7]
    add         r7, r0, r9
    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]
    add         r9, r9, #64
    add         r6, r6, #64

LOOP_HEIGHT_64_2:
    cmp         r6, r12
    bge         LOOP_HEIGHT_64_3
    stmfd       sp!, {r0-r3, r12}       @ backup parameters
    mov         r0, r2
    mov         r1, r3
    mov         r2, r6
    mov         r3, r5
    bl          tile_4x2_read_asm
    mov         r7, r0
    ldmfd       sp!, {r0-r3, r12}       @ restore parameters
    sub         r11, r12, r6
    stmfd       sp!, {r9-r12}           @ backup parameters
    mov         r10, r11
    add         r11, r1, r7
    bl          MEMCOPY_UNDER_64
    ldmfd       sp!, {r9-r12}           @ restore parameters

LOOP_HEIGHT_64_3:

    ldr         r14, [sp, #52]          @ buttom
    add         r5, r5, #1              @ i=i+1
    sub         r14, r3, r14            @ i<yuv420_height-buttom
    cmp         r5, r14
    blt         LOOP_HEIGHT_64
    b           RESTORE_REG

LOOP_HEIGHT_2_START:

    ldr         r5, [sp, #44]           @ i = top
LOOP_HEIGHT_2:

    ldr         r6, [sp, #40]           @ j = left
    ldr         r9, [sp, #44]           @ linear_offset = top
    add         r11, r6, #64            @ temp2 = ((j+64)>>6)<<6
    bic         r11, r11, #0x3F
    sub         r11, r11, r6            @ temp2 = temp2-j
    sub         r9, r5, r9              @ linear_offset = temp1*(i-top)
    mul         r9, r10, r9
    add         r9, r0, r9              @ linear_offset = linear_dst+linear_offset
LOOP_HEIGHT_2_WIDTH:
    stmfd       sp!, {r0-r3, r12}       @ backup parameters
    mov         r0, r2
    mov         r1, r3
    mov         r2, r6
    mov         r3, r5
    bl          tile_4x2_read_asm
    mov         r7, r0
    ldmfd       sp!, {r0-r3, r12}       @ restore parameters

    and         r14, r6, #0x3           @ temp4 = j&0x3@
    add         r7, r7, r14             @ tiled_offset = tiled_offset+temp4@
    add         r7, r1, r7

    ldrh        r14, [r7]
    strh        r14, [r9], #2

    ldr         r14, [sp, #48]          @ right
    add         r6, r6, #2              @ j=j+2
    sub         r14, r2, r14            @ j<yuv420_width-right
    cmp         r6, r14
    blt         LOOP_HEIGHT_2_WIDTH

    ldr         r14, [sp, #52]          @ buttom
    add         r5, r5, #1              @ i=i+1
    sub         r14, r3, r14            @ i<yuv420_height-buttom
    cmp         r5, r14
    blt         LOOP_HEIGHT_2

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

MEMCOPY_UNDER_64:                       @ count=r10, src=r11
    cmp         r10, #32
    add         r9, r0, r9              @ r9 = yuv420_dest+linear_offset
    blt         MEMCOPY_UNDER_32
    vld1.8      {q0, q1}, [r11]!        @ load {nv12t_src+tiled_offset+temp1, 64}
    sub         r10, r10, #32
    cmp         r10, #0
    vst1.8      {q0, q1}, [r9]!         @ load {nv12t_src+tiled_offset+temp1, 64}
    beq         MEMCOPY_UNDER_END
MEMCOPY_UNDER_32:
    cmp         r10, #16
    blt         MEMCOPY_UNDER_16
    vld1.8      {q0}, [r11]!            @ load {nv12t_src+tiled_offset+temp1, 64}
    sub         r10, r10, #16
    cmp         r10, #0
    vst1.8      {q0}, [r9]!             @ load {nv12t_src+tiled_offset+temp1, 64}
    beq         MEMCOPY_UNDER_END
MEMCOPY_UNDER_16:
    ldrb        r12, [r11], #1
    strb        r12, [r9], #1
    subs        r10, r10, #1
    bne         MEMCOPY_UNDER_16

MEMCOPY_UNDER_END:
    and         r10, r6, #0x3F          @ temp1 = left(==j)&0x3F
    cmp         r10, #0
    mov         pc, lr

tile_4x2_read_asm:
LFB0:
    add     ip, r3, #32
    sub     r0, r0, #1
    cmp     r1, ip
    cmple   r3, r1
    mov     ip, r2, asr #2
    mov     r0, r0, asr #7
    stmfd   sp!, {r4, r5, lr}
LCFI0:
    add     r0, r0, #1
    bge     L2
    sub     r1, r1, #1
    tst     r1, #32
    bne     L2
    tst     r3, #32
    bne     L2
    mov     r4, r2, asr #7
    and     r1, r3, #31
    eor     r4, r4, r3, asr #5
    ubfx    r3, r3, #6, #8
    tst     r4, #1
    ubfx    r4, r2, #8, #6
    and     ip, ip, #15
    mov     r2, r2, asr #6
    mla     r3, r0, r3, r4
    orr     r1, ip, r1, asl #4
    b       L9
L2:
    mov     r2, ip, asr #5
    and     r4, r3, #31
    eor     r1, r2, r3, asr #5
    and     r5, r2, #127
    ubfx    r3, r3, #6, #8
    tst     r1, #1
    and     r1, ip, #15
    mov     r2, ip, asr #4
    mla     r3, r0, r3, r5
    orr     r1, r1, r4, asl #4
L9:
    andne   r2, r2, #1
    andeq   r2, r2, #1
    orrne   r2, r2, #2
    mov     r1, r1, asl #2
    orr     r3, r1, r3, asl #13
    orr     r0, r3, r2, asl #11
    ldmfd   sp!, {r4, r5, pc}
LFE0:
    .fnend

