#!/bin/bash

media-ctl -v -r -l '"mt9p031":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

media-ctl -v -f '"mt9p031":0 [SBGGR12 800x600], "OMAP3 ISP CCDC":2 [SBGGR10 800x600], "OMAP3 ISP preview":1 [UYVY 800x600], "OMAP3 ISP resizer":1 [UYVY 320x240]'

gst-launch v4l2src device=/dev/video6 ! TIImgenc1 engineName=codecServer iColorSpace=UYVY oColorSpace=YUV420P qValue=97 numOutputBufs=2 ! udpsink host=192.168.2.139 port=9000
