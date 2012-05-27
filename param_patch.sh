#!/sbin/sh

# Param.lfs patcher
# Copyright (C) 2012 The CyanogenMod Project

# blacken the samsung charging image at param.lfs
dd if=/dev/zero of=/dev/block/mmcblk0p4 bs=1 seek=977460 count=10911

exit 0
