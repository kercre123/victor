# Use python 3 types by default (uses "future" package)
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
from future.builtins import *

import cv2
import numpy as np
from numpy import *
import pdb
import os
import anki
import platform

cap = 0
cap = cv2.VideoCapture(0)

#windowSizes = [(15,15), (31,31), (51,51)]
windowSizes = [(51,51)]
#windowSizes = [(101,101)]
# Parameters for lucas kanade optical flow
lk_params = dict( winSize  = (31,31),
                  maxLevel = 3,
                  criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03),
                  #minEigThreshold = 1e-4)
                  minEigThreshold = 5e-3)

# Create some random colors
color = np.random.randint(0,255,(10000,3))

# Take first frame and find corners in it
ret, image = cap.read()
image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
harrisImage = cv2.cornerHarris(image, blockSize=lk_params['winSize'][0], ksize=3, k=0.04)

lastImage = image
lastHarrisImage = harrisImage

waitKeyTime = 25
numPointsPerDimension = 50

points0 = []
for y in linspace(0, image.shape[0], numPointsPerDimension):
    for x in linspace(0, image.shape[1], numPointsPerDimension):
        points0.append((x,y))
points0 = array(points0).reshape(-1,1,2).astype('float32')

frameNum = 0
while(1):
    ret,image = cap.read()
    image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    #harrisImage = cv2.cornerHarris(image, blockSize=lk_params['winSize'][0], ksize=3, k=0.04)

    for windowSize in windowSizes:

        lk_params['winSize'] = windowSize

        # calculate optical flow
        points1, st, err = cv2.calcOpticalFlowPyrLK(lastImage, image, points0, None, **lk_params)

        frameNum += 1

        # Select good points
        goodPoints0 = points0[st==1]
        goodPoints1 = points1[st==1]

        # draw the tracks
        keypointImage = cv2.merge((image,image,image))
        #harrisImageToShow = (harrisImage + .000001) / .000002
        #keypointImage = cv2.merge((harrisImageToShow,harrisImageToShow,harrisImageToShow))

        for i,(new,old) in enumerate(zip(goodPoints0,goodPoints1)):
            a,b = new.ravel()
            c,d = old.ravel()
            cv2.line(keypointImage, (a,b),(c,d), color[i].tolist(), 1)
            #cv2.circle(keypointImage,(a,b),3,color[i].tolist(),-1)

        cv2.imshow('keypointImage' + str(windowSize), keypointImage)

    keypress = cv2.waitKey(waitKeyTime)
    if keypress & 0xFF == ord('q'):
        print('Breaking')
        break

    # Now update the previous frame
    lastImage = image
    lastHarrisImage = harrisImage

cap.release()
cv2.destroyAllWindows()
