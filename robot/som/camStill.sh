

media-ctl -v -r -l '"ov2686":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

media-ctl -v -f '"ov2686":0 [SBGGR12 1600x1200], "OMAP3 ISP CCDC":2 [SBGGR10 1600x1200], "OMAP3 ISP preview":1 [UYVY 1600x1200], "OMAP3 ISP resizer":1 [UYVY 1600x1200]'

gst-launch v4l2src device=/dev/video6 num-buffers=1 ! 'video/x-raw-yuv,format=(fourcc)UYVY,width=1600,height=1200' ! TIImgenc1 engineName=codecServer codecName=jpegenc resolution=1600x1200 iColorSpace=UYVY oColorSpace=YUV420P qValue=95 ! filesink location=still.jpg

#yavta -f UYVY -s 1600x1200 -c=1 /dev/video6 --file still.raw

########################################################################################################################

#media-ctl -v -r -l '"ov2686":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]'

#media-ctl -v -f '"ov2686":0 [SBGGR8 1600x1200], "OMAP3 ISP CCDC":2 [SBGGR10 1600x1200], "OMAP3 ISP preview":1 [UYVY 1600x1200], "OMAP3 ISP resizer":1 [UYVY 1600x1200]'

#gst-launch v4l2src device=/dev/video6 num-buffers=1 ! 'video/x-raw-yuv,format=(fourcc)UYVY,width=1600,height=1200' ! TIImgenc1 engineName=codecServer codecName=jpegenc resolution=1600x1200 iColorSpace=UYVY oColorSpace=YUV420P qValue=97 ! filesink location=still.jpg
