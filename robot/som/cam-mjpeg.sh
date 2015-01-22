#!/bin/bash

yavta -w '0x00980911 720' /dev/v4l-subdev8
yavta -w '0x00980911 720' /dev/v4l-subdev8
yavta -w '0x0098090e 125' /dev/v4l-subdev8
yavta -w '0x0098090f 175' /dev/v4l-subdev8

media-ctl -v -f '"ov2686":0 [SGRBG8 1600x1200 (0,0)/1600x1200], "OMAP3 ISP CCDC":2 [SGRBG10 1600x1200], "OMAP3 ISP preview":1 [UYVY 1600x1200], "OMAP3 ISP resizer":1 [UYVY 1600x1200]'

gst-launch v4l2src device=/dev/video6 ! ffmpegcolorspace ! jpegenc ! udpsink host=$1 port=$2
