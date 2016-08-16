#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import numpy as np
import math
import time
import cv2

'''This is a simple script to have Cozmo follow a flashlight pointed at him.
A image will show up with his camera and the brightest spot in his view
He will drive in the direction of that brightest spot until you press escape'''

cozmo = CozmoInterface()

width = 320
height = 240

maxTreadSpeed = 25
maxHeadRads = .1
radius = 41
# This is adapted from http://www.pyimagesearch.com/2014/09/29/finding-brightest-spot-image-using-python-opencv/
while True:

    state = cozmo.GetState()
    image = state.GetImage()
    # Numpy's non-empty check
    if image.any():
        grey = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        blurred = cv2.GaussianBlur(grey, (radius,radius), 0)
        # print(str(blurred.type()))
        (minval, maxVal, minLoc, maxLoc) = cv2.minMaxLoc(blurred)
        final = blurred.copy()
        cv2.circle(final, maxLoc, radius, (255,0,0), 2)
        cv2.imshow("Brightest spot circle!", final)
        (x,y) = maxLoc
        # print(x,y)
        # Should be MTS when straight in middle, 2*MTS when on right, 0 when on left
        lSpeed = maxTreadSpeed * ((x / (width/2)))
        # Should be MTS when straight in middle, 2*MTS when on left, 0 when on right
        rSpeed = maxTreadSpeed * (((width - x) / (width/2)))
        # Should be 0 when middle, -MHR when top, MHR when bottom
        headRads = maxHeadRads * (((height/2) - y)/(height/2))
        # print(lSpeed, rSpeed, headRads)
        cozmo.DriveWheels(lSpeed,rSpeed,100,100)
        cozmo.MoveHead(headRads)
        k = cv2.waitKey(100)

        if k == 27:
            break

cozmo.Shutdown()

