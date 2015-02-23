#!/bin/bash

media-ctl -v -r -l '"mt9p031":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

media-ctl -v -f '"mt9p031":0 [SGRBG12 800x600], "OMAP3 ISP CCDC":2 [SGRBG10 800x600], "OMAP3 ISP preview":1 [UYVY 800x600], "OMAP3 ISP resizer":1 [UYVY 320x240]'

gst-launch v4l2src device=/dev/video6 num-buffers=1000 always-copy=false queue-size=4 ! 'video/x-raw-yuv,format=(fourcc)UYVY,width=320,height=240,framerate=30/1' ! TIPrepEncBuf numOutputBufs=4 contiguousInputFrame=false ! tee name=tee tee. ! queue ! tidisplaysink2 mmap-buffer=true tee. ! queue ! TIVidenc1 engineName=codecServer codecName=h264enc contiguousInputFrame=true ! queue ! avimux ! udpsink host=192.168.2.139 port=9000
