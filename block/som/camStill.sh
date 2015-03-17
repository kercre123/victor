

media-ctl -v -r -l '"ov2686":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

media-ctl -v -f '"ov2686":0 [SGRBG12 800x600 (0,0)/800x600], "OMAP3 ISP CCDC":2 [SGRBG10 800x600], "OMAP3 ISP preview":1 [UYVY 800x600], "OMAP3 ISP resizer":1 [UYVY 640x480]'

gst-launch  v4l2src device=/dev/video6 num-buffers=1 ! videocrop left=160 top=120 right=160 bottom=120 ! TIImgenc1 engineName=codecServer codecName=jpegenc iColorSpace=UYVY oColorSpace=YUV420P qValue=95 ! filesink location=still.jpg
