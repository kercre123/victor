#!/bin/bash

#media-ctl -v -r -l '"mt9p031":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

#media-ctl -v -f '"mt9p031":0 [SGRBG8 2610x1954 (7,49)/2610x1954], "OMAP3 ISP CCDC":2 [SGRBG10 2610x1954], "OMAP3 ISP preview":1 [UYVY 2610x1954], "OMAP3 ISP resizer":1 [UYVY 2592x1944]'

gst-launch v4l2src device=/dev/video0 num-buffers=1 ! 'video/x-raw-bayer,format=bayer,width=640,height=480' ! TIImgenc1 engineName=codecServer codecName=jpegenc resolution=640x480 iColorSpace=UYVY oColorSpace=YUV420P qValue=97 ! filesink location=still.jpg
