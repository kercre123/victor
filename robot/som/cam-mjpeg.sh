#!/bin/bash

media-ctl -v -f '"mt9p031":0 [SGRBG8 1298x970 (664,541)/1298x970], "OMAP3 ISP CCDC":2 [SGRBG10 1298x970], "OMAP3 ISP preview":1 [UYVY 1298x970], "OMAP3 ISP resizer":1 [UYVY 640x480]'

gst-launch v4l2src device=/dev/video6 ? ffmpegcolorspace ! jpegenc ! avimux ! udpsink host=$1 port=$2
