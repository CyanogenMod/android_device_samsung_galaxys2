#!/bin/sh

# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

DEVICE=galaxys2
COMMON=c1-common
MANUFACTURER=samsung

mkdir -p ../../../vendor/$MANUFACTURER/$DEVICE/proprietary
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/cameradata
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/egl
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/keychars
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/wifi
mkdir -p ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging

# galaxys2


# c1-common
adb pull /system/lib/libActionShot.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libActionShot.so
adb pull /system/lib/libakm.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libakm.so
adb pull /system/lib/libarccamera.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libarccamera.so
adb pull /system/lib/libcamera_client.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libcamera_client.so
adb pull /system/lib/libcameraservice.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libcameraservice.so
adb pull /system/lib/libcamera.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libcamera.so
adb pull /system/lib/libcaps.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libcaps.so
adb pull /system/lib/libEGL.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libEGL.so
adb pull /system/lib/libexif.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libexif.so
adb pull /system/lib/libfimc.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libfimc.so
adb pull /system/lib/libfimg.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libfimg.so
adb pull /system/lib/libGLESv1_CM.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libGLESv1_CM.so
adb pull /system/lib/libGLESv2.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libGLESv2.so
adb pull /system/lib/libMali.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libMali.so
adb pull /system/lib/libOpenSLES.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libOpenSLES.so
adb pull /system/lib/libPanoraMax3.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libPanoraMax3.so
adb pull /system/lib/libril.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libril.so
adb pull /system/lib/libs5pjpeg.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libs5pjpeg.so
adb pull /system/lib/libseccameraadaptor.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libseccameraadaptor.so
adb pull /system/lib/libseccamera.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libseccamera.so
adb pull /system/lib/libsecjpegarcsoft.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libsecjpegarcsoft.so
adb pull /system/lib/libsecjpegboard.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libsecjpegboard.so
adb pull /system/lib/libsecjpeginterface.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libsecjpeginterface.so
adb pull /system/lib/libsecril-client.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libsecril-client.so
adb pull /system/lib/libsec-ril.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libsec-ril.so
adb pull /system/lib/libtvoutcec.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvoutcec.so
adb pull /system/lib/libtvoutddc.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvoutddc.so
adb pull /system/lib/libtvoutedid.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvoutedid.so
adb pull /system/lib/lib_tvoutengine.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/lib_tvoutengine.so
adb pull /system/lib/libtvoutfimc.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvoutfimc.so
adb pull /system/lib/libtvoutfimg.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvoutfimg.so
adb pull /system/lib/libtvouthdmi.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvouthdmi.so
adb pull /system/lib/libtvout_jni.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvout_jni.so
adb pull /system/lib/libtvoutservice.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvoutservice.so
adb pull /system/lib/libtvout.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/libtvout.so
adb pull /system/bin/BCM4330B1_002.001.003.0221.0265.hcd ../../../vendor/$MANUFACTURER/$COMMON/proprietary/BCM4330B1_002.001.003.0221.0265.hcd
adb pull /system/bin/rild ../../../vendor/$MANUFACTURER/$COMMON/proprietary/rild
adb pull /system/bin/tvoutserver ../../../vendor/$MANUFACTURER/$COMMON/proprietary/tvoutserver
adb pull /system/cameradata/datapattern_420sp.yuv ../../../vendor/$MANUFACTURER/$COMMON/proprietary/cameradata/datapattern_420sp.yuv
adb pull /system/cameradata/datapattern_front_420sp.yuv ../../../vendor/$MANUFACTURER/$COMMON/proprietary/cameradata/datapattern_front_420sp.yuv
adb pull /system/lib/egl/libEGL_mali.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/egl/libEGL_mali.so
adb pull /system/lib/egl/libGLES_android.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/egl/libGLES_android.so
adb pull /system/lib/egl/libGLESv1_CM_mali.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/egl/libGLESv1_CM_mali.so
adb pull /system/lib/egl/libGLESv2_mali.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/egl/libGLESv2_mali.so
adb pull /system/etc/firmware/qt602240.fw ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware/qt602240.fw
adb pull /system/etc/firmware/RS_M5LS_OB.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware/RS_M5LS_OB.bin
adb pull /system/etc/firmware/RS_M5LS_OC.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware/RS_M5LS_OC.bin
adb pull /system/etc/firmware/RS_M5LS_OE.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware/RS_M5LS_OE.bin
adb pull /system/etc/firmware/RS_M5LS_TB.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware/RS_M5LS_TB.bin
adb pull /system/vendor/firmware/mfc_fw.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/firmware/mfc_fw.bin
adb pull /system/lib/hw/acoustics.default.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw/acoustics.default.so
adb pull /system/lib/hw/alsa.default.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw/alsa.default.so
adb pull /system/lib/hw/copybit.smdkv310.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw/copybit.GT-I9100.so
adb pull /system/lib/hw/vendor-gps.smdkv310.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw/gps.GT-I9100.so
adb pull /system/lib/hw/gralloc.default.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw/gralloc.default.so
adb pull /system/lib/hw/gralloc.smdkv310.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/hw/gralloc.GT-I9100.so
adb pull /system/usr/keychars/Broadcom_Bluetooth_HID.kcm.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/keychars/Broadcom_Bluetooth_HID.kcm.bin
adb pull /system/usr/keychars/qwerty2.kcm.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/keychars/qwerty2.kcm.bin
adb pull /system/usr/keychars/qwerty.kcm.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/keychars/qwerty.kcm.bin
adb pull /system/usr/keychars/sec_key.kcm.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/keychars/sec_key.kcm.bin
adb pull /system/usr/keychars/sec_touchkey.kcm.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/keychars/sec_touchkey.kcm.bin
adb pull /system/vendor/firmware/bcm4330_aps.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/wifi/bcm4330_aps.bin
adb pull /system/vendor/firmware/bcm4330_mfg.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/wifi/bcm4330_mfg.bin
adb pull /system/vendor/firmware/bcm4330_sta.bin ../../../vendor/$MANUFACTURER/$COMMON/proprietary/wifi/bcm4330_sta.bin
adb pull /system/bin/alsa_amixer ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/alsa_amixer
adb pull /system/bin/alsa_aplay ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/alsa_aplay
adb pull /system/bin/alsa_ctl ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/alsa_ctl
adb pull /system/bin/alsa_ucm ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/alsa_ucm
adb pull /system/lib/libasound.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libasound.so
adb pull /system/lib/libaudio.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libaudio.so
adb pull /system/lib/libaudioeffect_jni.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libaudioeffect_jni.so
adb pull /system/lib/libaudiohw_op.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libaudiohw_op.so
adb pull /system/lib/libaudiohw_sf.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libaudiohw_sf.so
adb pull /system/lib/libaudiopolicy.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libaudiopolicy.so
adb pull /system/lib/liblvvefs.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/liblvvefs.so
adb pull /system/lib/libmediayamaha.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libmediayamaha.so
adb pull /system/lib/libmediayamaha_jni.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libmediayamaha_jni.so
adb pull /system/lib/libmediayamahaservice.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libmediayamahaservice.so
adb pull /system/lib/libmediayamaha_tuning_jni.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libmediayamaha_tuning_jni.so
adb pull /system/lib/libsamsungAcousticeq.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libsamsungAcousticeq.so
adb pull /system/lib/lib_Samsung_Acoustic_Module_Llite.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/lib_Samsung_Acoustic_Module_Llite.so
adb pull /system/lib/lib_Samsung_Resampler.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/lib_Samsung_Resampler.so
adb pull /system/lib/libsamsungSoundbooster.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libsamsungSoundbooster.so
adb pull /system/lib/lib_Samsung_Sound_Booster.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/lib_Samsung_Sound_Booster.so
adb pull /system/lib/libsoundalive.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libsoundalive.so
adb pull /system/lib/libsoundpool.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libsoundpool.so
adb pull /system/lib/libSR_AudioIn.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libSR_AudioIn.so
adb pull /system/lib/libyamahasrc.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/audio/libyamahasrc.so
adb pull /system/bin/charging_mode ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/charging_mode
adb pull /system/bin/playlpm ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/playlpm
adb pull /system/lib/libQmageDecoder.so ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/libQmageDecoder.so
adb pull /system/media/battery_batteryerror.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_batteryerror.qmg
adb pull /system/media/battery_charging_5.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_5.qmg
adb pull /system/media/battery_charging_10.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_10.qmg
adb pull /system/media/battery_charging_15.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_15.qmg
adb pull /system/media/battery_charging_20.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_20.qmg
adb pull /system/media/battery_charging_25.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_25.qmg
adb pull /system/media/battery_charging_30.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_30.qmg
adb pull /system/media/battery_charging_35.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_35.qmg
adb pull /system/media/battery_charging_40.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_40.qmg
adb pull /system/media/battery_charging_45.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_45.qmg
adb pull /system/media/battery_charging_50.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_50.qmg
adb pull /system/media/battery_charging_55.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_55.qmg
adb pull /system/media/battery_charging_60.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_60.qmg
adb pull /system/media/battery_charging_65.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_65.qmg
adb pull /system/media/battery_charging_70.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_70.qmg
adb pull /system/media/battery_charging_75.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_75.qmg
adb pull /system/media/battery_charging_80.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_80.qmg
adb pull /system/media/battery_charging_85.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_85.qmg
adb pull /system/media/battery_charging_90.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_90.qmg
adb pull /system/media/battery_charging_95.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_95.qmg
adb pull /system/media/battery_charging_100.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_charging_100.qmg
adb pull /system/media/battery_error.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/battery_error.qmg
adb pull /system/media/chargingwarning.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/chargingwarning.qmg
adb pull /system/media/Disconnected.qmg ../../../vendor/$MANUFACTURER/$COMMON/proprietary/offmode_charging/Disconnected.qmg


(cat << EOF) | sed s/__DEVICE__/$DEVICE/g | sed s/__MANUFACTURER__/$MANUFACTURER/g > ../../../vendor/$MANUFACTURER/$DEVICE/$DEVICE-vendor-blobs.mk
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Prebuilt libraries that are needed to build open-source libraries
PRODUCT_COPY_FILES := \\

# All the blobs necessary for galaxys2 devices
PRODUCT_COPY_FILES += \\

EOF


(cat << EOF) | sed s/__COMMON__/$COMMON/g | sed s/__MANUFACTURER__/$MANUFACTURER/g > ../../../vendor/$MANUFACTURER/$COMMON/c1-vendor-blobs.mk
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Prebuilt libraries that are needed to build open-source libraries
PRODUCT_COPY_FILES := \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libcamera.so:obj/lib/libcamera.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libril.so:obj/lib/libril.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libsecril-client.so:obj/lib/libsecril-client.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudio.so:obj/lib/libaudio.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libmediayamahaservice.so:obj/lib/libmediayamahaservice.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudiopolicy.so:obj/lib/libaudiopolicy.so

# All the blobs necessary for galaxys2 devices
PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libActionShot.so:system/lib/libActionShot.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libakm.so:system/lib/libakm.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libarccamera.so:system/lib/libarccamera.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libcamera_client.so:system/lib/libcamera_client.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libcameraservice.so:system/lib/libcameraservice.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libcamera.so:system/lib/libcamera.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libcaps.so:system/lib/libcaps.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libEGL.so:system/lib/libEGL.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libexif.so:system/lib/libexif.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libfimc.so:system/lib/libfimc.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libfimg.so:system/lib/libfimg.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libGLESv1_CM.so:system/lib/libGLESv1_CM.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libGLESv2.so:system/lib/libGLESv2.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libMali.so:system/lib/libMali.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libOpenSLES.so:system/lib/libOpenSLES.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libPanoraMax3.so:system/lib/libPanoraMax3.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libril.so:system/lib/libril.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libs5pjpeg.so:system/lib/libs5pjpeg.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libseccameraadaptor.so:system/lib/libseccameraadaptor.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libseccamera.so:system/lib/libseccamera.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libsecjpegarcsoft.so:system/lib/libsecjpegarcsoft.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libsecjpegboard.so:system/lib/libsecjpegboard.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libsecjpeginterface.so:system/lib/libsecjpeginterface.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libsecril-client.so:system/lib/libsecril-client.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libsec-ril.so:system/lib/libsec-ril.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvoutcec.so:system/lib/libtvoutcec.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvoutddc.so:system/lib/libtvoutddc.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvoutedid.so:system/lib/libtvoutedid.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/lib_tvoutengine.so:system/lib/lib_tvoutengine.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvoutfimc.so:system/lib/libtvoutfimc.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvoutfimg.so:system/lib/libtvoutfimg.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvouthdmi.so:system/lib/libtvouthdmi.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvout_jni.so:system/lib/libtvout_jni.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvoutservice.so:system/lib/libtvoutservice.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/libtvout.so:system/lib/libtvout.so

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/BCM4330B1_002.001.003.0221.0265.hcd:system/bin/BCM4330B1_002.001.003.0221.0265.hcd \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/rild:system/bin/rild \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/tvoutserver:system/bin/tvoutserver

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/cameradata/datapattern_420sp.yuv:system/cameradata/datapattern_420sp.yuv \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/cameradata/datapattern_front_420sp.yuv:system/cameradata/datapattern_front_420sp.yuv

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/egl/libEGL_mali.so:system/lib/egl/libEGL_mali.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/egl/libGLES_android.so:system/lib/egl/libGLES_android.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/egl/libGLESv1_CM_mali.so:system/lib/egl/libGLESv1_CM_mali.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/egl/libGLESv2_mali.so:system/lib/egl/libGLESv2_mali.so

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/firmware/qt602240.fw:system/etc/firmware/qt602240.fw \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/firmware/RS_M5LS_OB.bin:system/etc/firmware/RS_M5LS_OB.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/firmware/RS_M5LS_OC.bin:system/etc/firmware/RS_M5LS_OC.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/firmware/RS_M5LS_OE.bin:system/etc/firmware/RS_M5LS_OE.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/firmware/RS_M5LS_TB.bin:system/etc/firmware/RS_M5LS_TB.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/firmware/mfc_fw.bin:system/vendor/firmware/mfc_fw.bin

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/hw/acoustics.default.so:system/lib/hw/acoustics.default.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/hw/alsa.default.so:system/lib/hw/alsa.default.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/hw/copybit.GT-I9100.so:system/lib/hw/copybit.smdkv310.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/hw/gps.GT-I9100.so:system/lib/hw/vendor-gps.smdkv310.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/hw/gralloc.default.so:system/lib/hw/gralloc.default.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/hw/gralloc.GT-I9100.so:system/lib/hw/gralloc.smdkv310.so

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/keychars/Broadcom_Bluetooth_HID.kcm.bin:system/usr/keychars/Broadcom_Bluetooth_HID.kcm.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/keychars/qwerty2.kcm.bin:system/usr/keychars/qwerty2.kcm.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/keychars/qwerty.kcm.bin:system/usr/keychars/qwerty.kcm.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/keychars/sec_key.kcm.bin:system/usr/keychars/sec_key.kcm.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/keychars/sec_touchkey.kcm.bin:system/usr/keychars/sec_touchkey.kcm.bin

PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/wifi/bcm4330_aps.bin:system/vendor/firmware/bcm4330_aps.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/wifi/bcm4330_mfg.bin:system/vendor/firmware/bcm4330_mfg.bin \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/wifi/bcm4330_sta.bin:system/vendor/firmware/bcm4330_sta.bin

# blobs necessary for audio
PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/alsa_amixer:system/bin/alsa_amixer \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/alsa_aplay:system/bin/alsa_aplay \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/alsa_ctl:system/bin/alsa_ctl \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/alsa_ucm:system/bin/alsa_ucm \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libasound.so:system/lib/libasound.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudio.so:system/lib/libaudio.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudioeffect_jni.so:system/lib/libaudioeffect_jni.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudiohw_op.so:system/lib/libaudiohw_op.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudiohw_sf.so:system/lib/libaudiohw_sf.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libaudiopolicy.so:system/lib/libaudiopolicy.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/liblvvefs.so:system/lib/liblvvefs.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libmediayamaha.so:system/lib/libmediayamaha.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libmediayamaha_jni.so:system/lib/libmediayamaha_jni.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libmediayamahaservice.so:system/lib/libmediayamahaservice.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libmediayamaha_tuning_jni.so:system/lib/libmediayamaha_tuning_jni.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libsamsungAcousticeq.so:system/lib/libsamsungAcousticeq.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/lib_Samsung_Acoustic_Module_Llite.so:system/lib/lib_Samsung_Acoustic_Module_Llite.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/lib_Samsung_Resampler.so:system/lib/lib_Samsung_Resampler.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libsamsungSoundbooster.so:system/lib/libsamsungSoundbooster.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/lib_Samsung_Sound_Booster.so:system/lib/lib_Samsung_Sound_Booster.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libsoundalive.so:system/lib/libsoundalive.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libsoundpool.so:system/lib/libsoundpool.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libSR_AudioIn.so:system/lib/libSR_AudioIn.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/audio/libyamahasrc.so:system/lib/libyamahasrc.so

# offmode charging    
PRODUCT_COPY_FILES += \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/charging_mode:system/bin/charging_mode \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/playlpm:system/bin/playlpm \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/libQmageDecoder.so:system/lib/libQmageDecoder.so \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_batteryerror.qmg:system/media/battery_batteryerror.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_5.qmg:system/media/battery_charging_5.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_10.qmg:system/media/battery_charging_10.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_15.qmg:system/media/battery_charging_15.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_20.qmg:system/media/battery_charging_20.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_25.qmg:system/media/battery_charging_25.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_30.qmg:system/media/battery_charging_30.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_35.qmg:system/media/battery_charging_35.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_40.qmg:system/media/battery_charging_40.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_45.qmg:system/media/battery_charging_45.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_50.qmg:system/media/battery_charging_50.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_55.qmg:system/media/battery_charging_55.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_60.qmg:system/media/battery_charging_60.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_65.qmg:system/media/battery_charging_65.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_70.qmg:system/media/battery_charging_70.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_75.qmg:system/media/battery_charging_75.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_80.qmg:system/media/battery_charging_80.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_85.qmg:system/media/battery_charging_85.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_90.qmg:system/media/battery_charging_90.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_95.qmg:system/media/battery_charging_95.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_charging_100.qmg:system/media/battery_charging_100.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/battery_error.qmg:system/media/battery_error.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/chargingwarning.qmg:system/media/chargingwarning.qmg \\
    vendor/__MANUFACTURER__/__COMMON__/proprietary/offmode_charging/Disconnected.qmg:system/media/Disconnected.qmg

EOF

./setup-makefiles.sh
