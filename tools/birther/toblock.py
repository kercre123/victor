#!/usr/bin/python
"Copy old file system birth certificate and camera calibration to new block device location"

import os

OLD_EMR = "/factory/birthcertificate"
OLD_CAL = "/factory/nvStorage/80000001.nvdata"
NEW_EMR = "/dev/block/bootdevice/by-name/emr"

kCameraCalibMagic = "\x0D\xDF\xAC\xE5"
kCalibOffset = 4194304

if not os.path.isfile(OLD_EMR):
    exit("No old EMR file found")

if not os.path.islink(NEW_EMR):
    exit("No EMR partition, need partition table upgrade first")

with open(NEW_EMR, "wb") as new_emr_fh:

    new_emr_fh.write(open(OLD_EMR, "rb").read())

    if not os.path.isfile(OLD_CAL):
        print("No old camera calibration to preserve, skipping")
    else:
        new_emr_fh.seek(kCalibOffset, os.SEEK_SET)
        new_emr_fh.write(kCameraCalibMagic)
        new_emr_fh.write(open(OLD_CAL, "rb").read())

print("Done")
