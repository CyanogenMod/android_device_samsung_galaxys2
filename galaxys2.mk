#
# Copyright (C) 2012 The Android Open-Source Project
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
#

$(call inherit-product, device/samsung/galaxys2/galaxys2_base.mk)

# Keylayout
PRODUCT_COPY_FILES += \
    device/samsung/galaxys2/usr/keylayout/AVRCP.kl:system/usr/keylayout/AVRCP.kl \
    device/samsung/galaxys2/usr/keylayout/Generic.kl:system/usr/keylayout/Generic.kl \
    device/samsung/galaxys2/usr/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
    device/samsung/galaxys2/usr/keylayout/max8997-muic.kl:system/usr/keylayout/max8997-muic.kl \
    device/samsung/galaxys2/usr/keylayout/melfas-touchkey.kl:system/usr/keylayout/melfas-touchkey.kl \
    device/samsung/galaxys2/usr/keylayout/qwerty.kl:system/usr/keylayout/qwerty.kl \
    device/samsung/galaxys2/usr/keylayout/samsung-keypad.kl:system/usr/keylayout/samsung-keypad.kl \
    device/samsung/galaxys2/usr/keylayout/sec_key.kl:system/usr/keylayout/sec_key.kl \
    device/samsung/galaxys2/usr/keylayout/sec_touchkey.kl:system/usr/keylayout/sec_touchkey.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_05ac_Product_0239.kl:system/usr/keylayout/Vendor_05ac_Product_0239.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_22b8_Product_093d.kl:system/usr/keylayout/Vendor_22b8_Product_093d.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_045e_Product_028e.kl:system/usr/keylayout/Vendor_045e_Product_028e.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_046d_Product_c216.kl:system/usr/keylayout/Vendor_046d_Product_c216.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_046d_Product_c294.kl:system/usr/keylayout/Vendor_046d_Product_c294.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_046d_Product_c299.kl:system/usr/keylayout/Vendor_046d_Product_c299.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_046d_Product_c532.kl:system/usr/keylayout/Vendor_046d_Product_c532.kl \
    device/samsung/galaxys2/usr/keylayout/Vendor_054c_Product_0268.kl:system/usr/keylayout/Vendor_054c_Product_0268.kl

# Keychars
PRODUCT_COPY_FILES += \
    device/samsung/galaxys2/usr/keychars/Generic.kcm:system/usr/keychars/Generic.kcm \
    device/samsung/galaxys2/usr/keychars/qwerty.kcm:system/usr/keychars/qwerty.kcm \
    device/samsung/galaxys2/usr/keychars/qwerty2.kcm:system/usr/keychars/qwerty2.kcm \
    device/samsung/galaxys2/usr/keychars/Virtual.kcm:system/usr/keychars/Virtual.kcm

# Idc
PRODUCT_COPY_FILES += \
    device/samsung/galaxys2/usr/idc/melfas_ts.idc:system/usr/idc/melfas_ts.idc \
    device/samsung/galaxys2/usr/idc/mxt224_ts_input.idc:system/usr/idc/mxt224_ts_input.idc \
    device/samsung/galaxys2/usr/idc/qwerty.idc:system/usr/idc/qwerty.idc \
    device/samsung/galaxys2/usr/idc/qwerty2.idc:system/usr/idc/qwerty2.idc \
    device/samsung/galaxys2/usr/idc/sec_touchscreen.idc:system/usr/idc/sec_touchscreen.idc

$(call inherit-product, frameworks/base/build/phone-hdpi-512-dalvik-heap.mk)
$(call inherit-product-if-exists, vendor/samsung/galaxys2/galaxys2-vendor.mk)
