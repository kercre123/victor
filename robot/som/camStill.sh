

media-ctl -v -r -l '"ov2686":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

media-ctl -v -f '"ov2686":0 [SGRBG12 800x600], "OMAP3 ISP CCDC":2 [SGRBG10 800x600], "OMAP3 ISP preview":1 [UYVY 800x600], "OMAP3 ISP resizer":1 [UYVY 800x600]'

gst-launch  v4l2src device=/dev/video6 num-buffers=1 ! 'video/x-raw-yuv,format=(fourcc)UYVY,width=800,height=600' ! TIImgenc1 engineName=codecServer codecName=jpegenc resolution=800x600 iColorSpace=UYVY oColorSpace=YUV420P qValue=95 ! filesink location=still.jpg
