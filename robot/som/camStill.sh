#!/bin/bash

media-ctl -v -r -l '"ov2686":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

media-ctl -v -f '"ov2686":0 [SBGGR8 1600x1200], "OMAP3 ISP CCDC":2 [SBGGR8 1600x1200], "OMAP3 ISP preview":1 [UYVY 1600x1200], "OMAP3 ISP resizer":1 [UYVY 640x480]'

gst-launch v4l2src device=/dev/video6 num-buffers=1 ! 'video/x-raw-yuv,format=(fourcc)UYVY,width=640,height=480' ! TIImgenc1 engineName=codecServer codecName=jpegenc resolution=640x480 iColorSpace=UYVY oColorSpace=YUV420P qValue=97 ! filesink location=still.jpg
