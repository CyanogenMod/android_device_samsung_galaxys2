/*
 * Video for Linux Two header file for samsung
 *
 * Copyright (C) 2009, Dongsoo Nathaniel Kim<dongsoo45.kim@samsung.com>
 *
 * This header file contains several v4l2 APIs to be proposed to v4l2
 * community and until bein accepted, will be used restrictly in Samsung's
 * camera interface driver FIMC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef __LINUX_VIDEODEV2_SAMSUNG_H
#define __LINUX_VIDEODEV2_SAMSUNG_H

/* Values for 'capabilities' field */
/* Object detection device */
#define V4L2_CAP_OBJ_RECOGNITION    0x10000000
/* strobe control */
#define V4L2_CAP_STROBE         0x20000000

#define V4L2_CID_FOCUS_MODE     (V4L2_CID_CAMERA_CLASS_BASE+17)
/* Focus Methods */
enum v4l2_focus_mode {
    V4L2_FOCUS_MODE_AUTO        = 0,
    V4L2_FOCUS_MODE_MACRO       = 1,
    V4L2_FOCUS_MODE_MANUAL      = 2,
    V4L2_FOCUS_MODE_LASTP       = 2,
};

#define V4L2_CID_ZOOM_MODE      (V4L2_CID_CAMERA_CLASS_BASE+18)
/* Zoom Methods */
enum v4l2_zoom_mode {
    V4L2_ZOOM_MODE_CONTINUOUS   = 0,
    V4L2_ZOOM_MODE_OPTICAL      = 1,
    V4L2_ZOOM_MODE_DIGITAL      = 2,
    V4L2_ZOOM_MODE_LASTP        = 2,
};

/* Exposure Methods */
#define V4L2_CID_PHOTOMETRY     (V4L2_CID_CAMERA_CLASS_BASE+19)
enum v4l2_photometry_mode {
    V4L2_PHOTOMETRY_MULTISEG    = 0, /*Multi Segment*/
    V4L2_PHOTOMETRY_CWA     = 1, /*Centre Weighted Average*/
    V4L2_PHOTOMETRY_SPOT        = 2,
    V4L2_PHOTOMETRY_AFSPOT      = 3, /*Spot metering on focused point*/
    V4L2_PHOTOMETRY_LASTP       = V4L2_PHOTOMETRY_AFSPOT,
};

/* Manual exposure control items menu type: iris, shutter, iso */
#define V4L2_CID_CAM_APERTURE   (V4L2_CID_CAMERA_CLASS_BASE+20)
#define V4L2_CID_CAM_SHUTTER    (V4L2_CID_CAMERA_CLASS_BASE+21)
#define V4L2_CID_CAM_ISO    (V4L2_CID_CAMERA_CLASS_BASE+22)

/* Following CIDs are menu type */
#define V4L2_CID_SCENEMODE  (V4L2_CID_CAMERA_CLASS_BASE+23)
#define V4L2_CID_CAM_STABILIZE  (V4L2_CID_CAMERA_CLASS_BASE+24)
#define V4L2_CID_CAM_MULTISHOT  (V4L2_CID_CAMERA_CLASS_BASE+25)

/* Control dynamic range */
#define V4L2_CID_CAM_DR     (V4L2_CID_CAMERA_CLASS_BASE+26)

/* White balance preset control */
#define V4L2_CID_WHITE_BALANCE_PRESET   (V4L2_CID_CAMERA_CLASS_BASE+27)

/* CID extensions */
#define V4L2_CID_ROTATION       (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PADDR_Y        (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_PADDR_CB       (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_PADDR_CR       (V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_PADDR_CBCR     (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_OVERLAY_AUTO       (V4L2_CID_PRIVATE_BASE + 5)
#define V4L2_CID_OVERLAY_VADDR0     (V4L2_CID_PRIVATE_BASE + 6)
#define V4L2_CID_OVERLAY_VADDR1     (V4L2_CID_PRIVATE_BASE + 7)
#define V4L2_CID_OVERLAY_VADDR2     (V4L2_CID_PRIVATE_BASE + 8)
#define V4L2_CID_OVLY_MODE      (V4L2_CID_PRIVATE_BASE + 9)
#define V4L2_CID_DST_INFO       (V4L2_CID_PRIVATE_BASE + 10)
/* UMP secure id control */
#define V4L2_CID_GET_UMP_SECURE_ID  (V4L2_CID_PRIVATE_BASE + 11)
#define V4L2_CID_IMAGE_EFFECT_FN    (V4L2_CID_PRIVATE_BASE + 16)
#define V4L2_CID_IMAGE_EFFECT_APPLY (V4L2_CID_PRIVATE_BASE + 17)
#define V4L2_CID_IMAGE_EFFECT_CB    (V4L2_CID_PRIVATE_BASE + 18)
#define V4L2_CID_IMAGE_EFFECT_CR    (V4L2_CID_PRIVATE_BASE + 19)
#define V4L2_CID_RESERVED_MEM_BASE_ADDR (V4L2_CID_PRIVATE_BASE + 20)
#define V4L2_CID_FIMC_VERSION       (V4L2_CID_PRIVATE_BASE + 21)

#define V4L2_CID_STREAM_PAUSE           (V4L2_CID_PRIVATE_BASE + 53)

/* CID Extensions for camera sensor operations */
#define V4L2_CID_CAM_PREVIEW_ONOFF      (V4L2_CID_PRIVATE_BASE + 64)
#define V4L2_CID_CAM_CAPTURE            (V4L2_CID_PRIVATE_BASE + 65)
#define V4L2_CID_CAM_JPEG_MEMSIZE       (V4L2_CID_PRIVATE_BASE + 66)

#define V4L2_CID_CAM_DATE_INFO_YEAR             (V4L2_CID_PRIVATE_BASE + 14)
#define V4L2_CID_CAM_DATE_INFO_MONTH            (V4L2_CID_PRIVATE_BASE + 15)
#define V4L2_CID_CAM_DATE_INFO_DATE             (V4L2_CID_PRIVATE_BASE + 22)
#define V4L2_CID_CAM_SENSOR_VER                 (V4L2_CID_PRIVATE_BASE + 23)
#define V4L2_CID_CAM_FW_MINOR_VER               (V4L2_CID_PRIVATE_BASE + 24)
#define V4L2_CID_CAM_FW_MAJOR_VER               (V4L2_CID_PRIVATE_BASE + 25)
#define V4L2_CID_CAM_PRM_MINOR_VER              (V4L2_CID_PRIVATE_BASE + 26)
#define V4L2_CID_CAM_PRM_MAJOR_VER              (V4L2_CID_PRIVATE_BASE + 27)
#define V4L2_CID_CAM_FW_VER                     (V4L2_CID_PRIVATE_BASE + 28)
#define V4L2_CID_CAM_SET_FW_ADDR                (V4L2_CID_PRIVATE_BASE + 29)
#define V4L2_CID_CAM_SET_FW_SIZE                (V4L2_CID_PRIVATE_BASE + 30)
#define V4L2_CID_CAM_UPDATE_FW  (V4L2_CID_PRIVATE_BASE + 31)
#define V4L2_CID_CAM_JPEG_MAIN_SIZE     (V4L2_CID_PRIVATE_BASE + 32)
#define V4L2_CID_CAM_JPEG_MAIN_OFFSET       (V4L2_CID_PRIVATE_BASE + 33)
#define V4L2_CID_CAM_JPEG_THUMB_SIZE        (V4L2_CID_PRIVATE_BASE + 34)
#define V4L2_CID_CAM_JPEG_THUMB_OFFSET      (V4L2_CID_PRIVATE_BASE + 35)
#define V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET   (V4L2_CID_PRIVATE_BASE + 36)
#define V4L2_CID_CAM_JPEG_QUALITY   (V4L2_CID_PRIVATE_BASE + 37)
#define V4L2_CID_CAM_SENSOR_MAKER   (V4L2_CID_PRIVATE_BASE + 38)
#define V4L2_CID_CAM_SENSOR_OPTICAL (V4L2_CID_PRIVATE_BASE + 39)
#define V4L2_CID_CAM_AF_VER_LOW     (V4L2_CID_PRIVATE_BASE + 40)
#define V4L2_CID_CAM_AF_VER_HIGH    (V4L2_CID_PRIVATE_BASE + 41)
#define V4L2_CID_CAM_GAMMA_RG_LOW   (V4L2_CID_PRIVATE_BASE + 42)
#define V4L2_CID_CAM_GAMMA_RG_HIGH  (V4L2_CID_PRIVATE_BASE + 43)
#define V4L2_CID_CAM_GAMMA_BG_LOW   (V4L2_CID_PRIVATE_BASE + 44)
#define V4L2_CID_CAM_GAMMA_BG_HIGH  (V4L2_CID_PRIVATE_BASE + 45)
#define V4L2_CID_CAM_DUMP_FW        (V4L2_CID_PRIVATE_BASE + 46)
#define V4L2_CID_CAM_GET_DUMP_SIZE  (V4L2_CID_PRIVATE_BASE + 47)
#define V4L2_CID_CAMERA_VT_MODE         (V4L2_CID_PRIVATE_BASE + 48)
#define V4L2_CID_CAMERA_VGA_BLUR    (V4L2_CID_PRIVATE_BASE + 49)
#define V4L2_CID_CAMERA_CAPTURE     (V4L2_CID_PRIVATE_BASE + 50)

#define V4L2_CID_MAIN_SW_DATE_INFO_YEAR     (V4L2_CID_PRIVATE_BASE + 54)
#define V4L2_CID_MAIN_SW_DATE_INFO_MONTH    (V4L2_CID_PRIVATE_BASE + 55)
#define V4L2_CID_MAIN_SW_DATE_INFO_DATE     (V4L2_CID_PRIVATE_BASE + 56)
#define V4L2_CID_MAIN_SW_FW_MINOR_VER       (V4L2_CID_PRIVATE_BASE + 57)
#define V4L2_CID_MAIN_SW_FW_MAJOR_VER       (V4L2_CID_PRIVATE_BASE + 58)
#define V4L2_CID_MAIN_SW_PRM_MINOR_VER      (V4L2_CID_PRIVATE_BASE + 59)
#define V4L2_CID_MAIN_SW_PRM_MAJOR_VER      (V4L2_CID_PRIVATE_BASE + 60)

/* SLIM IS control */
#define V4L2_CID_FIMC_IS_BASE               (V4L2_CTRL_CLASS_CAMERA | 0x1000)

#define V4L2_CID_IS_LOAD_FW                 (V4L2_CID_FIMC_IS_BASE + 10)
#define V4L2_CID_IS_INIT_PARAM              (V4L2_CID_FIMC_IS_BASE + 11)
#define V4L2_CID_IS_RESETi                  (V4L2_CID_FIMC_IS_BASE + 12)
#define V4L2_CID_IS_S_POWER                 (V4L2_CID_FIMC_IS_BASE + 13)
enum is_set_power {
    IS_POWER_OFF,
    IS_POWER_ON
};

#define V4L2_CID_IS_S_STREAM                (V4L2_CID_FIMC_IS_BASE + 14)
enum is_set_stream {
    IS_DISABLE_STREAM,
    IS_ENABLE_STREAM
};

#define V4L2_CID_IS_S_SCENARIO_MODE         (V4L2_CID_FIMC_IS_BASE + 15)
#define V4L2_CID_IS_S_FORMAT_SCENARIO       (V4L2_CID_FIMC_IS_BASE + 16)
enum scenario_mode {
    IS_MODE_PREVIEW_STILL,
    IS_MODE_PREVIEW_VIDEO,
    IS_MODE_CAPTURE_STILL,
    IS_MODE_CAPTURE_VIDEO,
    IS_MODE_MAX
};

/* global */
#define V4L2_CID_IS_CAMERA_SHOT_MODE_NORMAL      (V4L2_CID_FIMC_IS_BASE + 101)
/* value : 1 : single shot , >=2 : continuous shot */

#define V4L2_CID_IS_CAMERA_SENSOR_NUM       (V4L2_CID_FIMC_IS_BASE + 201)

#define V4L2_CID_IS_CAMERA_FOCUS_MODE       (V4L2_CID_FIMC_IS_BASE + 401)
enum is_focus_mode {
    IS_FOCUS_MODE_AUTO,
    IS_FOCUS_MODE_MACRO,
    IS_FOCUS_MODE_INFINITY,
    IS_FOCUS_MODE_CONTINUOUS,
    IS_FOCUS_MODE_TOUCH,
    IS_FOCUS_MODE_FACEDETECT,
    IS_FOCUS_MODE_MAX,
};

#define V4L2_CID_IS_CAMERA_FLASH_MODE        (V4L2_CID_FIMC_IS_BASE + 402)
enum is_flash_mode {
    IS_FLASH_MODE_OFF,
    IS_FLASH_MODE_AUTO,
    IS_FLASH_MODE_AUTO_REDEYE,
    IS_FLASH_MODE_ON,
    IS_FLASH_MODE_TORCH,
    IS_FLASH_MODE_MAX
};

#define V4L2_CID_IS_CAMERA_AWB_MODE        (V4L2_CID_FIMC_IS_BASE + 403)
enum is_awb_mode {
    IS_AWB_AUTO,
    IS_AWB_DAYLIGHT,
    IS_AWB_CLOUDY,
    IS_AWB_TUNGSTEN,
    IS_AWB_FLUORESCENT,
    IS_AWB_MAX
};

#define V4L2_CID_IS_CAMERA_IMAGE_EFFECT        (V4L2_CID_FIMC_IS_BASE + 404)
enum is_image_effect {
    IS_IMAGE_EFFECT_DISABLE,
    IS_IMAGE_EFFECT_MONOCHROME,
    IS_IMAGE_EFFECT_NEGATIVE_MONO,
    IS_IMAGE_EFFECT_NEGATIVE_COLOR,
    IS_IMAGE_EFFECT_SEPIA,
    IS_IMAGE_EFFECT_SEPIA_CB,
    IS_IMAGE_EFFECT_SEPIA_CR,
    IS_IMAGE_EFFECT_NEGATIVE,
    IS_IMAGE_EFFECT_ARTFREEZE,
    IS_IMAGE_EFFECT_EMBOSSING,
    IS_IMAGE_EFFECT_SILHOUETTE,
    IS_IMAGE_EFFECT_MAX
};

#define V4L2_CID_IS_CAMERA_ISO            (V4L2_CID_FIMC_IS_BASE + 405)
enum is_iso {
    IS_ISO_AUTO,
    IS_ISO_50,
    IS_ISO_100,
    IS_ISO_200,
    IS_ISO_400,
    IS_ISO_800,
    IS_ISO_1600,
    IS_ISO_MAX
};

#define V4L2_CID_IS_CAMERA_CONTRAST        (V4L2_CID_FIMC_IS_BASE + 406)
enum is_contrast {
    IS_CONTRAST_AUTO,
    IS_CONTRAST_MINUS_2,
    IS_CONTRAST_MINUS_1,
    IS_CONTRAST_DEFAULT,
    IS_CONTRAST_PLUS_1,
    IS_CONTRAST_PLUS_2,
    IS_CONTRAST_MAX
};

#define V4L2_CID_IS_CAMERA_SATURATION        (V4L2_CID_FIMC_IS_BASE + 407)
enum is_saturation {
    IS_SATURATION_MINUS_2,
    IS_SATURATION_MINUS_1,
    IS_SATURATION_DEFAULT,
    IS_SATURATION_PLUS_1,
    IS_SATURATION_PLUS_2,
    IS_SATURATION_MAX
};

#define V4L2_CID_IS_CAMERA_SHARPNESS        (V4L2_CID_FIMC_IS_BASE + 408)
enum is_sharpness {
    IS_SHARPNESS_MINUS_2,
    IS_SHARPNESS_MINUS_1,
    IS_SHARPNESS_DEFAULT,
    IS_SHARPNESS_PLUS_1,
    IS_SHARPNESS_PLUS_2,
    IS_SHARPNESS_MAX
};

#define V4L2_CID_IS_CAMERA_EXPOSURE        (V4L2_CID_FIMC_IS_BASE + 409)
enum is_exposure {
    IS_EXPOSURE_MINUS_4,
    IS_EXPOSURE_MINUS_3,
    IS_EXPOSURE_MINUS_2,
    IS_EXPOSURE_MINUS_1,
    IS_EXPOSURE_DEFAULT,
    IS_EXPOSURE_PLUS_1,
    IS_EXPOSURE_PLUS_2,
    IS_EXPOSURE_PLUS_3,
    IS_EXPOSURE_PLUS_4,
    IS_EXPOSURE_MAX
};

#define V4L2_CID_IS_CAMERA_BRIGHTNESS        (V4L2_CID_FIMC_IS_BASE + 410)
enum is_brightness {
    IS_BRIGHTNESS_MINUS_2,
    IS_BRIGHTNESS_MINUS_1,
    IS_BRIGHTNESS_DEFAULT,
    IS_BRIGHTNESS_PLUS_1,
    IS_BRIGHTNESS_PLUS_2,
    IS_BRIGHTNESS_MAX
};

#define V4L2_CID_IS_CAMERA_HUE            (V4L2_CID_FIMC_IS_BASE + 411)
enum is_hue {
    IS_HUE_MINUS_2,
    IS_HUE_MINUS_1,
    IS_HUE_DEFAULT,
    IS_HUE_PLUS_1,
    IS_HUE_PLUS_2,
    IS_HUE_MAX
};

#define V4L2_CID_IS_CAMERA_METERING        (V4L2_CID_FIMC_IS_BASE + 412)
enum is_metering {
    IS_METERING_CENTER,
    IS_METERING_SPOT,
    IS_METERING_MATRIX,
    IS_METERING_MAX
};
#define V4L2_CID_IS_CAMERA_METERING_POSITION_X  (V4L2_CID_FIMC_IS_BASE + 500)
#define V4L2_CID_IS_CAMERA_METERING_POSITION_Y  (V4L2_CID_FIMC_IS_BASE + 501)
#define V4L2_CID_IS_CAMERA_METERING_WINDOW_X    (V4L2_CID_FIMC_IS_BASE + 502)
#define V4L2_CID_IS_CAMERA_METERING_WINDOW_Y    (V4L2_CID_FIMC_IS_BASE + 503)

#define V4L2_CID_IS_CAMERA_AFC_MODE        (V4L2_CID_FIMC_IS_BASE + 413)
enum is_afc_mode {
    IS_AFC_DISABLE,
    IS_AFC_AUTO,
    IS_AFC_MANUAL_50HZ,
    IS_AFC_MANUAL_60HZ,
    IS_AFC_MAX
};

#define V4L2_CID_IS_FD_GET_FACE_COUNT               (V4L2_CID_FIMC_IS_BASE + 600)
#define V4L2_CID_IS_FD_GET_FACE_FRAME_NUMBER        (V4L2_CID_FIMC_IS_BASE + 601)
#define V4L2_CID_IS_FD_GET_FACE_CONFIDENCE          (V4L2_CID_FIMC_IS_BASE + 602)
#define V4L2_CID_IS_FD_GET_FACE_SMILE_LEVEL         (V4L2_CID_FIMC_IS_BASE + 603)
#define V4L2_CID_IS_FD_GET_FACE_BLINK_LEVEL         (V4L2_CID_FIMC_IS_BASE + 604)
#define V4L2_CID_IS_FD_GET_FACE_TOPLEFT_X           (V4L2_CID_FIMC_IS_BASE + 605)
#define V4L2_CID_IS_FD_GET_FACE_TOPLEFT_Y           (V4L2_CID_FIMC_IS_BASE + 606)
#define V4L2_CID_IS_FD_GET_FACE_BOTTOMRIGHT_X       (V4L2_CID_FIMC_IS_BASE + 607)
#define V4L2_CID_IS_FD_GET_FACE_BOTTOMRIGHT_Y       (V4L2_CID_FIMC_IS_BASE + 608)
#define V4L2_CID_IS_FD_GET_LEFT_EYE_TOPLEFT_X       (V4L2_CID_FIMC_IS_BASE + 609)
#define V4L2_CID_IS_FD_GET_LEFT_EYE_TOPLEFT_Y       (V4L2_CID_FIMC_IS_BASE + 610)
#define V4L2_CID_IS_FD_GET_LEFT_EYE_BOTTOMRIGHT_X   (V4L2_CID_FIMC_IS_BASE + 611)
#define V4L2_CID_IS_FD_GET_LEFT_EYE_BOTTOMRIGHT_Y   (V4L2_CID_FIMC_IS_BASE + 612)
#define V4L2_CID_IS_FD_GET_RIGHT_EYE_TOPLEFT_X      (V4L2_CID_FIMC_IS_BASE + 613)
#define V4L2_CID_IS_FD_GET_RIGHT_EYE_TOPLEFT_Y      (V4L2_CID_FIMC_IS_BASE + 614)
#define V4L2_CID_IS_FD_GET_RIGHT_EYE_BOTTOMRIGHT_X  (V4L2_CID_FIMC_IS_BASE + 615)
#define V4L2_CID_IS_FD_GET_RIGHT_EYE_BOTTOMRIGHT_Y  (V4L2_CID_FIMC_IS_BASE + 616)
#define V4L2_CID_IS_FD_GET_MOUTH_TOPLEFT_X          (V4L2_CID_FIMC_IS_BASE + 617)
#define V4L2_CID_IS_FD_GET_MOUTH_TOPLEFT_Y          (V4L2_CID_FIMC_IS_BASE + 618)
#define V4L2_CID_IS_FD_GET_MOUTH_BOTTOMRIGHT_X      (V4L2_CID_FIMC_IS_BASE + 619)
#define V4L2_CID_IS_FD_GET_MOUTH_BOTTOMRIGHT_Y      (V4L2_CID_FIMC_IS_BASE + 620)
#define V4L2_CID_IS_FD_GET_ANGLE                    (V4L2_CID_FIMC_IS_BASE + 621)
#define V4L2_CID_IS_FD_GET_NEXT                     (V4L2_CID_FIMC_IS_BASE + 622)
#define V4L2_CID_IS_FD_GET_DATA                     (V4L2_CID_FIMC_IS_BASE + 623)

#define V4L2_CID_IS_FD_SET_MAX_FACE_NUMBER          (V4L2_CID_FIMC_IS_BASE + 650)
#define V4L2_CID_IS_FD_SET_ROLL_ANGLE               (V4L2_CID_FIMC_IS_BASE + 651)
enum is_fd_roll_angle {
    /* 0, 45, 0, -45 */
    IS_FD_ROLL_ANGLE_BASIC = 0,
    /* 0, 30, 0, -30, 0, 45, 0, -45 */
    IS_FD_ROLL_ANGLE_PRECISE_BASIC = 1,
    /* 0, 90, 0, -90 */
    IS_FD_ROLL_ANGLE_SIDES = 2,
    /* 0, 90, 0, -90 0, 45, 0, -45 */
    IS_FD_ROLL_ANGLE_PRECISE_SIDES = 3,
    /* 0, 90, 0, -90, 0, 180 */
    IS_FD_ROLL_ANGLE_FULL = 4,
    /* 0, 90, 0, -90, 0, 180, 0, 135, 0, -135 */
    IS_FD_ROLL_ANGLE_PRECISE_FULL = 5,
};

#define V4L2_CID_IS_FD_SET_YAW_ANGLE            (V4L2_CID_FIMC_IS_BASE + 652)
enum is_fd_yaw_angle {
    IS_FD_YAW_ANGLE_0    = 0,
    IS_FD_YAW_ANGLE_45    = 1,
    IS_FD_YAW_ANGLE_90    = 2,
    IS_FD_YAW_ANGLE_45_90    = 3,
};

#define V4L2_CID_IS_FD_SET_SMILE_MODE           (V4L2_CID_FIMC_IS_BASE + 653)
enum is_fd_smile_mode {
    IS_FD_SMILE_MODE_DISABLE    = 0,
    IS_FD_SMILE_MODE_ENABLE        = 1,
};

#define V4L2_CID_IS_FD_SET_BLINK_MODE           (V4L2_CID_FIMC_IS_BASE + 654)
enum is_fd_blink_mode {
    IS_FD_BLINK_MODE_DISABLE    = 0,
    IS_FD_BLINK_MODE_ENABLE        = 1,
};

#define V4L2_CID_IS_FD_SET_EYE_DETECT_MODE      (V4L2_CID_FIMC_IS_BASE + 655)
enum is_fd_eye_detect_mode {
    IS_FD_EYE_DETECT_DISABLE    = 0,
    IS_FD_EYE_DETECT_ENABLE        = 1,
};

#define V4L2_CID_IS_FD_SET_MOUTH_DETECT_MODE    (V4L2_CID_FIMC_IS_BASE + 656)
enum is_fd_mouth_detect_mode {
    IS_FD_MOUTH_DETECT_DISABLE    = 0,
    IS_FD_MOUTH_DETECT_ENABLE    = 1,
};

#define V4L2_CID_IS_FD_SET_ORIENTATION_MODE     (V4L2_CID_FIMC_IS_BASE + 657)
enum is_fd_orientation_mode {
    IS_FD_ORIENTATION_DISABLE    = 0,
    IS_FD_ORIENTATION_ENABLE    = 1,
};

#define V4L2_CID_IS_FD_SET_ORIENTATION          (V4L2_CID_FIMC_IS_BASE + 658)
#define V4L2_CID_IS_FD_SET_DATA_ADDRESS         (V4L2_CID_FIMC_IS_BASE + 659)

#define V4L2_CID_IS_SET_ISP            (V4L2_CID_FIMC_IS_BASE + 440)
enum is_isp_bypass_mode {
    IS_ISP_BYPASS_DISABLE,
    IS_ISP_BYPASS_ENABLE,
    IS_ISP_BYPASS_MAX
};

#define V4L2_CID_IS_SET_DRC            (V4L2_CID_FIMC_IS_BASE + 441)
enum is_drc_bypass_mode {
    IS_DRC_BYPASS_DISABLE,
    IS_DRC_BYPASS_ENABLE,
    IS_DRC_BYPASS_MAX
};

#define V4L2_CID_IS_SET_FD            (V4L2_CID_FIMC_IS_BASE + 442)
enum is_fd_bypass_mode {
    IS_FD_BYPASS_DISABLE,
    IS_FD_BYPASS_ENABLE,
    IS_FD_BYPASS_MAX
};

#define V4L2_CID_IS_SET_ODC            (V4L2_CID_FIMC_IS_BASE + 443)
enum is_odc_bypass_mode {
    IS_ODC_BYPASS_DISABLE,
    IS_ODC_BYPASS_ENABLE,
    IS_ODC_BYPASS_MAX
};

#define V4L2_CID_IS_SET_DIS            (V4L2_CID_FIMC_IS_BASE + 444)
enum is_dis_bypass_mode {
    IS_DIS_BYPASS_DISABLE,
    IS_DIS_BYPASS_ENABLE,
    IS_DIS_BYPASS_MAX
};

#define V4L2_CID_IS_SET_3DNR            (V4L2_CID_FIMC_IS_BASE + 445)
enum is_tdnr_bypass_mode {
    IS_TDNR_BYPASS_DISABLE,
    IS_TDNR_BYPASS_ENABLE,
    IS_TDNR_BYPASS_MAX
};

#define V4L2_CID_IS_SET_SCALERC            (V4L2_CID_FIMC_IS_BASE + 446)
enum is_scalerc_bypass_mode {
    IS_SCALERC_BYPASS_DISABLE,
    IS_SCALERC_BYPASS_ENABLE,
    IS_SCALERC_BYPASS_MAX
};

#define V4L2_CID_IS_SET_SCALERP            (V4L2_CID_FIMC_IS_BASE + 446)
enum is_scalerp_bypass_mode {
    IS_SCALERP_BYPASS_DISABLE,
    IS_SCALERP_BYPASS_ENABLE,
    IS_SCALERP_BYPASS_MAX
};

#define V4L2_CID_IS_ROTATION_MODE        (V4L2_CID_FIMC_IS_BASE + 450)
enum is_rotation_mode {
    IS_ROTATION_0,
    IS_ROTATION_90,
    IS_ROTATION_180,
    IS_ROTATION_270,
    IS_ROTATION_MAX
};

#define V4L2_CID_IS_3DNR_1ST_FRAME_MODE        (V4L2_CID_FIMC_IS_BASE + 451)
enum is_tdnr_1st_frame_mode {
    IS_TDNR_1ST_FRAME_NOPROCESSING,
    IS_TDNR_1ST_FRAME_2DNR,
    IS_TDNR_MAX
};

#define V4L2_CID_IS_CAMERA_OBJECT_POSITION_X    (V4L2_CID_FIMC_IS_BASE + 452)
#define V4L2_CID_IS_CAMERA_OBJECT_POSITION_Y    (V4L2_CID_FIMC_IS_BASE + 453)
#define V4L2_CID_IS_CAMERA_WINDOW_SIZE_X        (V4L2_CID_FIMC_IS_BASE + 454)
#define V4L2_CID_IS_CAMERA_WINDOW_SIZE_Y        (V4L2_CID_FIMC_IS_BASE + 455)

#define V4L2_CID_IS_CAMERA_EXIF_EXPTIME         (V4L2_CID_FIMC_IS_BASE + 456)
#define V4L2_CID_IS_CAMERA_EXIF_FLASH           (V4L2_CID_FIMC_IS_BASE + 457)
#define V4L2_CID_IS_CAMERA_EXIF_ISO             (V4L2_CID_FIMC_IS_BASE + 458)
#define V4L2_CID_IS_CAMERA_EXIF_SHUTTERSPEED    (V4L2_CID_FIMC_IS_BASE + 459)
#define V4L2_CID_IS_CAMERA_EXIF_BRIGHTNESS      (V4L2_CID_FIMC_IS_BASE + 460)

#define V4L2_CID_IS_CAMERA_ISP_SEL_INPUT        (V4L2_CID_FIMC_IS_BASE + 461)
enum is_isp_sel_input {
    IS_ISP_INPUT_OTF,
    IS_ISP_INPUT_DMA1,
    IS_ISP_INPUT_DMA2,
    IS_ISP_INPUT_DMA12,
    IS_ISP_INPUT_MAX
};

#define V4L2_CID_IS_CAMERA_ISP_SEL_OUTPUT    (V4L2_CID_FIMC_IS_BASE + 462)
enum is_isp_sel_output {
    IS_ISP_OUTPUT_OTF,
    IS_ISP_OUTPUT_DMA1,
    IS_ISP_OUTPUT_DMA2,
    IS_ISP_OUTPUT_DMA12,
    IS_ISP_OUTPUT_OTF_DMA1,
    IS_ISP_OUTPUT_OTF_DMA2,
    IS_ISP_OUTPUT_OTF_DMA12,
    IS_ISP_OUTPUT_MAX
};

#define V4L2_CID_IS_CAMERA_DRC_SEL_INPUT    (V4L2_CID_FIMC_IS_BASE + 463)
enum is_drc_sel_input {
    IS_DRC_INPUT_OTF,
    IS_DRC_INPUT_DMA,
    IS_DRC_INPUT_MAX
};

#define V4L2_CID_IS_CAMERA_FD_SEL_INPUT        (V4L2_CID_FIMC_IS_BASE + 464)
enum is_fd_sel_input {
    IS_FD_INPUT_OTF,
    IS_FD_INPUT_DMA,
    IS_FD_INPUT_MAX
};

#define V4L2_CID_IS_CAMERA_INIT_WIDTH        (V4L2_CID_FIMC_IS_BASE + 465)
#define V4L2_CID_IS_CAMERA_INIT_HEIGHT       (V4L2_CID_FIMC_IS_BASE + 466)

#define V4L2_CID_IS_CMD_ISP            (V4L2_CID_FIMC_IS_BASE + 467)
enum is_isp_cmd_mode {
    IS_ISP_COMMAND_STOP,
    IS_ISP_COMMAND_START,
    IS_ISP_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_DRC            (V4L2_CID_FIMC_IS_BASE + 468)
enum is_drc_cmd_mode {
    IS_DRC_COMMAND_STOP,
    IS_DRC_COMMAND_START,
    IS_DRC_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_FD            (V4L2_CID_FIMC_IS_BASE + 469)
enum is_fd_cmd_mode {
    IS_FD_COMMAND_STOP,
    IS_FD_COMMAND_START,
    IS_FD_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_ODC            (V4L2_CID_FIMC_IS_BASE + 470)
enum is_odc_cmd_mode {
    IS_ODC_COMMAND_STOP,
    IS_ODC_COMMAND_START,
    IS_ODC_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_DIS            (V4L2_CID_FIMC_IS_BASE + 471)
enum is_dis_cmd_mode {
    IS_DIS_COMMAND_STOP,
    IS_DIS_COMMAND_START,
    IS_DIS_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_TDNR           (V4L2_CID_FIMC_IS_BASE + 472)
enum is_tdnr_cmd_mode {
    IS_TDNR_COMMAND_STOP,
    IS_TDNR_COMMAND_START,
    IS_TDNR_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_SCALERC        (V4L2_CID_FIMC_IS_BASE + 473)
enum is_scalerc_cmd_mode {
    IS_SCALERC_COMMAND_STOP,
    IS_SCALERC_COMMAND_START,
    IS_SCALERC_COMMAND_MAX
};

#define V4L2_CID_IS_CMD_SCALERP        (V4L2_CID_FIMC_IS_BASE + 474)
enum is_scalerp_cmd_mode {
    IS_SCALERP_COMMAND_STOP,
    IS_SCALERP_COMMAND_START,
    IS_SCALERP_COMMAND_MAX
};

#define V4L2_CID_IS_GET_SENSOR_OFFSET_X        (V4L2_CID_FIMC_IS_BASE + 480)
#define V4L2_CID_IS_GET_SENSOR_OFFSET_Y        (V4L2_CID_FIMC_IS_BASE + 481)
#define V4L2_CID_IS_GET_SENSOR_WIDTH           (V4L2_CID_FIMC_IS_BASE + 482)
#define V4L2_CID_IS_GET_SENSOR_HEIGHT          (V4L2_CID_FIMC_IS_BASE + 483)

#define V4L2_CID_IS_GET_FRAME_VALID            (V4L2_CID_FIMC_IS_BASE + 484)
#define V4L2_CID_IS_SET_FRAME_VALID            (V4L2_CID_FIMC_IS_BASE + 485)
#define V4L2_CID_IS_GET_FRAME_BADMARK          (V4L2_CID_FIMC_IS_BASE + 486)
#define V4L2_CID_IS_SET_FRAME_BADMARK          (V4L2_CID_FIMC_IS_BASE + 487)
#define V4L2_CID_IS_GET_FRAME_CAPTURED         (V4L2_CID_FIMC_IS_BASE + 488)
#define V4L2_CID_IS_SET_FRAME_CAPTURED         (V4L2_CID_FIMC_IS_BASE + 489)
#define V4L2_CID_IS_SET_FRAME_NUMBER           (V4L2_CID_FIMC_IS_BASE + 490)
#define V4L2_CID_IS_GET_FRAME_NUMBER           (V4L2_CID_FIMC_IS_BASE + 491)
#define V4L2_CID_IS_CLEAR_FRAME_NUMBER         (V4L2_CID_FIMC_IS_BASE + 492)
#define V4L2_CID_IS_GET_LOSTED_FRAME_NUMBER    (V4L2_CID_FIMC_IS_BASE + 493)
#define V4L2_CID_IS_ISP_DMA_BUFFER_NUM         (V4L2_CID_FIMC_IS_BASE + 494)
#define V4L2_CID_IS_ISP_DMA_BUFFER_ADDRESS     (V4L2_CID_FIMC_IS_BASE + 495)

#define V4L2_CID_IS_ZOOM_STATE                 (V4L2_CID_FIMC_IS_BASE + 660)
#define V4L2_CID_IS_ZOOM_MAX_LEVEL             (V4L2_CID_FIMC_IS_BASE + 661)
#define V4L2_CID_IS_ZOOM                       (V4L2_CID_FIMC_IS_BASE + 662)
#define V4L2_CID_IS_FW_DEBUG_REGION_ADDR       (V4L2_CID_FIMC_IS_BASE + 663)

enum v4l2_blur {
    BLUR_LEVEL_0 = 0,
    BLUR_LEVEL_1,
    BLUR_LEVEL_2,
    BLUR_LEVEL_3,
    BLUR_LEVEL_MAX,
};

#if 1
#define V4L2_CID_CAMERA_SCENE_MODE      (V4L2_CID_PRIVATE_BASE+70)
enum v4l2_scene_mode {
    SCENE_MODE_BASE,
    SCENE_MODE_NONE,
    SCENE_MODE_PORTRAIT,
    SCENE_MODE_NIGHTSHOT,
    SCENE_MODE_BACK_LIGHT,
    SCENE_MODE_LANDSCAPE,
    SCENE_MODE_SPORTS,
    SCENE_MODE_PARTY_INDOOR,
    SCENE_MODE_BEACH_SNOW,
    SCENE_MODE_SUNSET,
    SCENE_MODE_DUST_DAWN,
    SCENE_MODE_FALL_COLOR,
    SCENE_MODE_FIREWORKS,
    SCENE_MODE_TEXT,
    SCENE_MODE_CANDLE_LIGHT,
    SCENE_MODE_MAX,
};

#define V4L2_CID_CAMERA_FLASH_MODE      (V4L2_CID_PRIVATE_BASE+71)
enum v4l2_flash_mode {
    FLASH_MODE_BASE,
    FLASH_MODE_OFF,
    FLASH_MODE_AUTO,
    FLASH_MODE_ON,
    FLASH_MODE_TORCH,
    FLASH_MODE_MAX,
};

#define V4L2_CID_CAMERA_BRIGHTNESS      (V4L2_CID_PRIVATE_BASE+72)
enum v4l2_ev_mode {
    EV_MINUS_4  = -4,
    EV_MINUS_3  = -3,
    EV_MINUS_2  = -2,
    EV_MINUS_1  = -1,
    EV_DEFAULT  = 0,
    EV_PLUS_1   = 1,
    EV_PLUS_2   = 2,
    EV_PLUS_3   = 3,
    EV_PLUS_4   = 4,
    EV_MAX,
};

#define V4L2_CID_CAMERA_WHITE_BALANCE   (V4L2_CID_PRIVATE_BASE+73)
enum v4l2_wb_mode {
    WHITE_BALANCE_BASE = 0,
    WHITE_BALANCE_AUTO,
    WHITE_BALANCE_SUNNY,
    WHITE_BALANCE_CLOUDY,
    WHITE_BALANCE_TUNGSTEN,
    WHITE_BALANCE_FLUORESCENT,
    WHITE_BALANCE_MAX,
};

#define V4L2_CID_CAMERA_EFFECT          (V4L2_CID_PRIVATE_BASE+74)
enum v4l2_effect_mode {
    IMAGE_EFFECT_BASE = 0,
    IMAGE_EFFECT_NONE,
    IMAGE_EFFECT_BNW,
    IMAGE_EFFECT_SEPIA,
    IMAGE_EFFECT_AQUA,
    IMAGE_EFFECT_ANTIQUE,
    IMAGE_EFFECT_NEGATIVE,
    IMAGE_EFFECT_SHARPEN,
    IMAGE_EFFECT_MAX,
};

#define V4L2_CID_CAMERA_ISO             (V4L2_CID_PRIVATE_BASE+75)
enum v4l2_iso_mode {
    ISO_AUTO = 0,
    ISO_50,
    ISO_100,
    ISO_200,
    ISO_400,
    ISO_800,
    ISO_1600,
    ISO_SPORTS,
    ISO_NIGHT,
    ISO_MOVIE,
    ISO_MAX,
};

#define V4L2_CID_CAMERA_METERING            (V4L2_CID_PRIVATE_BASE+76)
enum v4l2_metering_mode {
    METERING_BASE = 0,
    METERING_MATRIX,
    METERING_CENTER,
    METERING_SPOT,
    METERING_MAX,
};

#define V4L2_CID_CAMERA_CONTRAST            (V4L2_CID_PRIVATE_BASE+77)
enum v4l2_contrast_mode {
    CONTRAST_MINUS_2 = 0,
    CONTRAST_MINUS_1,
    CONTRAST_DEFAULT,
    CONTRAST_PLUS_1,
    CONTRAST_PLUS_2,
    CONTRAST_MAX,
};

#define V4L2_CID_CAMERA_SATURATION      (V4L2_CID_PRIVATE_BASE+78)
enum v4l2_saturation_mode {
    SATURATION_MINUS_2 = 0,
    SATURATION_MINUS_1,
    SATURATION_DEFAULT,
    SATURATION_PLUS_1,
    SATURATION_PLUS_2,
    SATURATION_MAX,
};

#define V4L2_CID_CAMERA_SHARPNESS       (V4L2_CID_PRIVATE_BASE+79)
enum v4l2_sharpness_mode {
    SHARPNESS_MINUS_2 = 0,
    SHARPNESS_MINUS_1,
    SHARPNESS_DEFAULT,
    SHARPNESS_PLUS_1,
    SHARPNESS_PLUS_2,
    SHARPNESS_MAX,
};

#define V4L2_CID_CAMERA_WDR             (V4L2_CID_PRIVATE_BASE+80)
enum v4l2_wdr_mode {
    WDR_OFF,
    WDR_ON,
    WDR_MAX,
};

#define V4L2_CID_CAMERA_ANTI_SHAKE      (V4L2_CID_PRIVATE_BASE+81)
enum v4l2_anti_shake_mode {
    ANTI_SHAKE_OFF,
    ANTI_SHAKE_STILL_ON,
    ANTI_SHAKE_MOVIE_ON,
    ANTI_SHAKE_MAX,
};

#define V4L2_CID_CAMERA_TOUCH_AF_START_STOP     (V4L2_CID_PRIVATE_BASE+82)
enum v4l2_touch_af {
    TOUCH_AF_STOP = 0,
    TOUCH_AF_START,
    TOUCH_AF_MAX,
};

#define V4L2_CID_CAMERA_SMART_AUTO      (V4L2_CID_PRIVATE_BASE+83)
enum v4l2_smart_auto {
    SMART_AUTO_OFF = 0,
    SMART_AUTO_ON,
    SMART_AUTO_MAX,
};

#define V4L2_CID_CAMERA_VINTAGE_MODE        (V4L2_CID_PRIVATE_BASE+84)
enum v4l2_vintage_mode {
    VINTAGE_MODE_BASE,
    VINTAGE_MODE_OFF,
    VINTAGE_MODE_NORMAL,
    VINTAGE_MODE_WARM,
    VINTAGE_MODE_COOL,
    VINTAGE_MODE_BNW,
    VINTAGE_MODE_MAX,
};

#define V4L2_CID_CAMERA_JPEG_QUALITY        (V4L2_CID_PRIVATE_BASE+85)
#define V4L2_CID_CAMERA_GPS_LATITUDE        (V4L2_CID_CAMERA_CLASS_BASE + 30)//(V4L2_CID_PRIVATE_BASE+86)
#define V4L2_CID_CAMERA_GPS_LONGITUDE   (V4L2_CID_CAMERA_CLASS_BASE + 31)//(V4L2_CID_PRIVATE_BASE+87)
#define V4L2_CID_CAMERA_GPS_TIMESTAMP   (V4L2_CID_CAMERA_CLASS_BASE + 32)//(V4L2_CID_PRIVATE_BASE+88)
#define V4L2_CID_CAMERA_GPS_ALTITUDE        (V4L2_CID_CAMERA_CLASS_BASE + 33)//(V4L2_CID_PRIVATE_BASE+89)
#define V4L2_CID_CAMERA_EXIF_TIME_INFO  (V4L2_CID_CAMERA_CLASS_BASE + 34)
#define V4L2_CID_CAMERA_GPS_PROCESSINGMETHOD                     (V4L2_CID_CAMERA_CLASS_BASE+35)
#define V4L2_CID_CAMERA_ZOOM                (V4L2_CID_PRIVATE_BASE+90)
enum v4l2_zoom_level {
    ZOOM_LEVEL_0 = 0,
    ZOOM_LEVEL_1,
    ZOOM_LEVEL_2,
    ZOOM_LEVEL_3,
    ZOOM_LEVEL_4,
    ZOOM_LEVEL_5,
    ZOOM_LEVEL_6,
    ZOOM_LEVEL_7,
    ZOOM_LEVEL_8,
    ZOOM_LEVEL_9,
    ZOOM_LEVEL_10,
    ZOOM_LEVEL_11,
    ZOOM_LEVEL_12,
    ZOOM_LEVEL_MAX = 31,
};

#define V4L2_CID_CAMERA_FACE_DETECTION      (V4L2_CID_PRIVATE_BASE+91)
enum v4l2_face_detection {
    FACE_DETECTION_OFF = 0,
    FACE_DETECTION_ON,
    FACE_DETECTION_NOLINE,
    FACE_DETECTION_ON_BEAUTY,
    FACE_DETECTION_MAX,
};

#define V4L2_CID_CAMERA_SMART_AUTO_STATUS       (V4L2_CID_PRIVATE_BASE+92)
enum v4l2_smart_auto_status {
    SMART_AUTO_STATUS_AUTO = 0,
    SMART_AUTO_STATUS_LANDSCAPE,
    SMART_AUTO_STATUS_PORTRAIT,
    SMART_AUTO_STATUS_MACRO,
    SMART_AUTO_STATUS_NIGHT,
    SMART_AUTO_STATUS_PORTRAIT_NIGHT,
    SMART_AUTO_STATUS_BACKLIT,
    SMART_AUTO_STATUS_PORTRAIT_BACKLIT,
    SMART_AUTO_STATUS_ANTISHAKE,
    SMART_AUTO_STATUS_PORTRAIT_ANTISHAKE,
    SMART_AUTO_STATUS_MAX,
};

#define V4L2_CID_CAMERA_SET_AUTO_FOCUS      (V4L2_CID_PRIVATE_BASE+93)
enum v4l2_auto_focus {
    AUTO_FOCUS_OFF = 0,
    AUTO_FOCUS_ON,
    AUTO_FOCUS_MAX,
};

#define V4L2_CID_CAMERA_BEAUTY_SHOT     (V4L2_CID_PRIVATE_BASE+94)
enum v4l2_beauty_shot {
    BEAUTY_SHOT_OFF = 0,
    BEAUTY_SHOT_ON,
    BEAUTY_SHOT_MAX,
};

#define V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK       (V4L2_CID_PRIVATE_BASE+95)
enum v4l2_ae_awb_lockunlock {
    AE_UNLOCK_AWB_UNLOCK = 0,
    AE_LOCK_AWB_UNLOCK,
    AE_UNLOCK_AWB_LOCK,
    AE_LOCK_AWB_LOCK,
    AE_AWB_MAX
};

#define V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK   (V4L2_CID_PRIVATE_BASE+96)
enum v4l2_face_lock {
    FACE_LOCK_OFF = 0,
    FACE_LOCK_ON,
    FIRST_FACE_TRACKING,
    FACE_LOCK_MAX,
};

#define V4L2_CID_CAMERA_OBJECT_POSITION_X   (V4L2_CID_PRIVATE_BASE+97)
#define V4L2_CID_CAMERA_OBJECT_POSITION_Y   (V4L2_CID_PRIVATE_BASE+98)
#define V4L2_CID_CAMERA_FOCUS_MODE      (V4L2_CID_PRIVATE_BASE+99)
enum v4l2_focusmode {
    FOCUS_MODE_AUTO = 0,
    FOCUS_MODE_MACRO,
    FOCUS_MODE_FACEDETECT,
    FOCUS_MODE_INFINITY,
    FOCUS_MODE_CONTINOUS,
    FOCUS_MODE_TOUCH,
    FOCUS_MODE_MAX,
    FOCUS_MODE_DEFAULT = (1 << 8),
};

#define V4L2_CID_CAMERA_OBJ_TRACKING_STATUS (V4L2_CID_PRIVATE_BASE+100)
enum v4l2_obj_tracking_status {
    OBJECT_TRACKING_STATUS_BASE,
    OBJECT_TRACKING_STATUS_PROGRESSING,
    OBJECT_TRACKING_STATUS_SUCCESS,
    OBJECT_TRACKING_STATUS_FAIL,
    OBJECT_TRACKING_STATUS_MISSING,
    OBJECT_TRACKING_STATUS_MAX,
};

#define V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP (V4L2_CID_PRIVATE_BASE+101)
enum v4l2_ot_start_stop {
    OT_STOP = 0,
    OT_START,
    OT_MAX,
};

#define V4L2_CID_CAMERA_CAF_START_STOP  (V4L2_CID_PRIVATE_BASE+102)
enum v4l2_caf_start_stop {
    CAF_STOP = 0,
    CAF_START,
    CAF_MAX,
};

#define V4L2_CID_CAMERA_AUTO_FOCUS_RESULT           (V4L2_CID_PRIVATE_BASE+103)
#define V4L2_CID_CAMERA_FRAME_RATE          (V4L2_CID_PRIVATE_BASE+104)
enum v4l2_frame_rate {
    FRAME_RATE_AUTO = 0,
    FRAME_RATE_7 = 7,
    FRAME_RATE_15 = 15,
    FRAME_RATE_30 = 30,
    FRAME_RATE_60 = 60,
    FRAME_RATE_120 = 120,
    FRAME_RATE_MAX
};

#define V4L2_CID_CAMERA_ANTI_BANDING                    (V4L2_CID_PRIVATE_BASE+105)
enum v4l2_anti_banding {
    ANTI_BANDING_AUTO = 0,
    ANTI_BANDING_50HZ = 1,
    ANTI_BANDING_60HZ = 2,
    ANTI_BANDING_OFF = 3,
};

#define V4L2_CID_CAMERA_SET_GAMMA   (V4L2_CID_PRIVATE_BASE+106)
enum v4l2_gamma_mode {
    GAMMA_OFF = 0,
    GAMMA_ON = 1,
    GAMMA_MAX,
};

#define V4L2_CID_CAMERA_SET_SLOW_AE (V4L2_CID_PRIVATE_BASE+107)
enum v4l2_slow_ae_mode {
    SLOW_AE_OFF,
    SLOW_AE_ON,
    SLOW_AE_MAX,
};

#define V4L2_CID_CAMERA_BATCH_REFLECTION                    (V4L2_CID_PRIVATE_BASE+108)
#define V4L2_CID_CAMERA_EXIF_ORIENTATION                    (V4L2_CID_PRIVATE_BASE+109)

#define V4L2_CID_CAMERA_RESET                               (V4L2_CID_PRIVATE_BASE+111) //s1_camera [ Defense process by ESD input ]
#define V4L2_CID_CAMERA_CHECK_DATALINE                      (V4L2_CID_PRIVATE_BASE+112)
#define V4L2_CID_CAMERA_CHECK_DATALINE_STOP                 (V4L2_CID_PRIVATE_BASE+113)

#endif

#if defined(CONFIG_ARIES_NTT) // Modify NTTS1
#define V4L2_CID_CAMERA_AE_AWB_DISABLE_LOCK                 (V4L2_CID_PRIVATE_BASE+114)
#endif
#define V4L2_CID_CAMERA_THUMBNAIL_NULL                                  (V4L2_CID_PRIVATE_BASE+115)
#define V4L2_CID_CAMERA_SENSOR_MODE                 (V4L2_CID_PRIVATE_BASE+116)

#define V4L2_CID_CAMERA_EXIF_EXPTIME                (V4L2_CID_PRIVATE_BASE+117)
#define V4L2_CID_CAMERA_EXIF_FLASH                  (V4L2_CID_PRIVATE_BASE+118)
#define V4L2_CID_CAMERA_EXIF_ISO                    (V4L2_CID_PRIVATE_BASE+119)
#define V4L2_CID_CAMERA_EXIF_TV                     (V4L2_CID_PRIVATE_BASE+120)
#define V4L2_CID_CAMERA_EXIF_BV                     (V4L2_CID_PRIVATE_BASE+121)
#define V4L2_CID_CAMERA_EXIF_EBV                    (V4L2_CID_PRIVATE_BASE+122)

#define V4L2_CID_CAMERA_BUSFREQ_LOCK                (V4L2_CID_PRIVATE_BASE+125)
#define V4L2_CID_CAMERA_BUSFREQ_UNLOCK              (V4L2_CID_PRIVATE_BASE+126)

/*      Pixel format FOURCC depth  Description  */
/* 12  Y/CbCr 4:2:0 64x32 macroblocks */
#define V4L2_PIX_FMT_NV12T    v4l2_fourcc('T', 'V', '1', '2')

/*
 *  * V4L2 extention for digital camera
 *   */
/* Strobe flash light */
enum v4l2_strobe_control {
    /* turn off the flash light */
    V4L2_STROBE_CONTROL_OFF     = 0,
    /* turn on the flash light */
    V4L2_STROBE_CONTROL_ON      = 1,
    /* act guide light before splash */
    V4L2_STROBE_CONTROL_AFGUIDE = 2,
    /* charge the flash light */
    V4L2_STROBE_CONTROL_CHARGE  = 3,
};

enum v4l2_strobe_conf {
    V4L2_STROBE_OFF         = 0,    /* Always off */
    V4L2_STROBE_ON          = 1,    /* Always splashes */
    /* Auto control presets */
    V4L2_STROBE_AUTO        = 2,
    V4L2_STROBE_REDEYE_REDUCTION    = 3,
    V4L2_STROBE_SLOW_SYNC       = 4,
    V4L2_STROBE_FRONT_CURTAIN   = 5,
    V4L2_STROBE_REAR_CURTAIN    = 6,
    /* Extra manual control presets */
    /* keep turned on until turning off */
    V4L2_STROBE_PERMANENT       = 7,
    V4L2_STROBE_EXTERNAL        = 8,
};

enum v4l2_strobe_status {
    V4L2_STROBE_STATUS_OFF      = 0,
    /* while processing configurations */
    V4L2_STROBE_STATUS_BUSY     = 1,
    V4L2_STROBE_STATUS_ERR      = 2,
    V4L2_STROBE_STATUS_CHARGING = 3,
    V4L2_STROBE_STATUS_CHARGED  = 4,
};

/* capabilities field */
/* No strobe supported */
#define V4L2_STROBE_CAP_NONE        0x0000
/* Always flash off mode */
#define V4L2_STROBE_CAP_OFF     0x0001
/* Always use flash light mode */
#define V4L2_STROBE_CAP_ON      0x0002
/* Flashlight works automatic */
#define V4L2_STROBE_CAP_AUTO        0x0004
/* Red-eye reduction */
#define V4L2_STROBE_CAP_REDEYE      0x0008
/* Slow sync */
#define V4L2_STROBE_CAP_SLOWSYNC    0x0010
/* Front curtain */
#define V4L2_STROBE_CAP_FRONT_CURTAIN   0x0020
/* Rear curtain */
#define V4L2_STROBE_CAP_REAR_CURTAIN    0x0040
/* keep turned on until turning off */
#define V4L2_STROBE_CAP_PERMANENT   0x0080
/* use external strobe */
#define V4L2_STROBE_CAP_EXTERNAL    0x0100

/* Set mode and Get status */
struct v4l2_strobe {
    /* off/on/charge:0/1/2 */
    enum    v4l2_strobe_control control;
    /* supported strobe capabilities */
    __u32   capabilities;
    enum    v4l2_strobe_conf mode;
    enum    v4l2_strobe_status status;  /* read only */
    /* default is 0 and range of value varies from each models */
    __u32   flash_ev;
    __u32   reserved[4];
};

#define VIDIOC_S_STROBE     _IOWR('V', 83, struct v4l2_strobe)
#define VIDIOC_G_STROBE     _IOR('V', 84, struct v4l2_strobe)

/* Object recognition and collateral actions */
enum v4l2_recog_mode {
    V4L2_RECOGNITION_MODE_OFF   = 0,
    V4L2_RECOGNITION_MODE_ON    = 1,
    V4L2_RECOGNITION_MODE_LOCK  = 2,
};

enum v4l2_recog_action {
    V4L2_RECOGNITION_ACTION_NONE    = 0,    /* only recognition */
    V4L2_RECOGNITION_ACTION_BLINK   = 1,    /* Capture on blinking */
    V4L2_RECOGNITION_ACTION_SMILE   = 2,    /* Capture on smiling */
};

enum v4l2_recog_pattern {
    V4L2_RECOG_PATTERN_FACE     = 0, /* Face */
    V4L2_RECOG_PATTERN_HUMAN    = 1, /* Human */
    V4L2_RECOG_PATTERN_CHAR     = 2, /* Character */
};

struct v4l2_recog_rect {
    enum    v4l2_recog_pattern  p;  /* detected pattern */
    struct  v4l2_rect  o;   /* detected area */
    __u32   reserved[4];
};

struct v4l2_recog_data {
    __u8    detect_cnt;     /* detected object counter */
    struct  v4l2_rect   o;  /* detected area */
    __u32   reserved[4];
};

struct v4l2_recognition {
    enum v4l2_recog_mode    mode;

    /* Which pattern to detect */
    enum v4l2_recog_pattern  pattern;

    /* How many object to detect */
    __u8    obj_num;

    /* select detected object */
    __u32   detect_idx;

    /* read only :Get object coordination */
    struct v4l2_recog_data  data;

    enum v4l2_recog_action  action;
    __u32   reserved[4];
};

#define VIDIOC_S_RECOGNITION    _IOWR('V', 85, struct v4l2_recognition)
#define VIDIOC_G_RECOGNITION    _IOR('V', 86, struct v4l2_recognition)

/* We use this struct as the v4l2_streamparm raw_data for
 * VIDIOC_G_PARM and VIDIOC_S_PARM
 */
struct sec_cam_parm {
    struct v4l2_captureparm capture;
    int contrast;
    int effects;
    int brightness;
    int exposure;
    int flash_mode;
    int focus_mode;
    int aeawb_mode;
    int iso;
    int metering;
    int saturation;
    int scene_mode;
    int sharpness;
    int hue;
    int white_balance;
    int anti_banding;
};

#endif /* __LINUX_VIDEODEV2_SAMSUNG_H */
