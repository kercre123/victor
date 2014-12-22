#!/bin/bash

media-ctl -v -f '"mt9p031":0 [SGRBG8 1298x970 (664,541)/1298x970], "OMAP3 ISP CCDC":2 [SGRBG10 1298x970], "OMAP3 ISP preview":1 [UYVY 1298x970], "OMAP3 ISP resizer":1 [UYVY 640x480]'

gst-launch v4l2src device=/dev/video6 num-buffers=300 always-copy=false queue-size=4 ! 'video/x-raw-yuv,format=(fourcc)UYVY,width=640,height=480,framerate=24/1' ! TIPrepEncBuf numOutputBufs=4 contiguousInputFrame=false ! tee name=tee tee. ! queue ! tidisplaysink2 mmap-buffer=true tee. ! queue ! TIVidenc1 engineName=codecServer codecName=h264enc contiguousInputFrame=true ! queue ! avimux ! filesink blocksize=65536 location=video640x480.avi
