#!/bin/bash

gst-launch v4l2src device=/dev/video6 ! ffmpegcolorspace ! jpegenc ! udpsink host=$1 port=$2
