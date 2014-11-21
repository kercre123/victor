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

def captureImages(videoCaptures, undistortMaps):
    images =  []
    for i,cap in enumerate(videoCaptures):
        ret,image = cap.read()
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

        if undistortMaps is not None:
            image = cv2.remap(image, undistortMaps[i]['mapX'], undistortMaps[i]['mapY'], cv2.INTER_LINEAR)

        images.append(image)

    return images

def computeLk(lastImage, curImage, points0, lk_params):
    # calculate optical flow
    points1, st, err = cv2.calcOpticalFlowPyrLK(lastImage, curImage, points0, None, **lk_params)

    # Select good points
    goodPoints0 = points0[st==1]
    goodPoints1 = points1[st==1]

    # draw the tracks
    keypointImage = cv2.merge((curImage,curImage,curImage))

    for i,(new,old) in enumerate(zip(goodPoints0,goodPoints1)):
        a,b = new.ravel()
        c,d = old.ravel()
        cv2.line(keypointImage, (a,b),(c,d), color[i].tolist(), 1)
        #cv2.circle(keypointImage,(a,b),3,color[i].tolist(),-1)

    return keypointImage, goodPoints0, goodPoints1

#cameraIds = [0]
cameraIds = [1,2]

videoCaptures = []
for cameraId in cameraIds:
    videoCaptures.append(cv2.VideoCapture(cameraId))

useStereoCalibration = True

undistortMaps = None

if useStereoCalibration:
  # Calibration for the Spynet stereo pair

  distCoeffs1 = np.array([0.190152, -0.472407, 0.004230, -0.006271, 0.000000]);
  distCoeffs2 = np.array([0.169615, -0.418723, 0.000641, -0.012438, 0.000000]);

  cameraMatrix1 = np.array(
    [[724.938597, 0.000000, 319.709242],
     [0.000000, 722.297095, 291.756454],
     [0.000000, 0.000000, 1.000000]]);

  cameraMatrix2 = np.array(
    [[742.089046, 0.000000, 305.986048],
     [0.000000, 739.837304, 248.962878],
     [0.000000, 0.000000, 1.000000]]);

  R = np.array(
    [[0.994009, -0.022323, 0.106996],
     [0.019794, 0.999500, 0.024641],
     [-0.107492, -0.022376, 0.993954]]);

  T = np.array([-13.001413, -0.695831, 0.988237]);

  imageSize = (640, 480)

  [R1, R2, P1, P2, Q, validPixROI1, validPixROI2] = cv2.stereoRectify(
    cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, R, T)

  leftUndistortMapX, leftUndistortMapY = cv2.initUndistortRectifyMap(
    cameraMatrix1, distCoeffs1, R1, P1, imageSize, cv2.CV_32FC1)

  rightUndistortMapX, rightUndistortMapY = cv2.initUndistortRectifyMap(
    cameraMatrix2, distCoeffs2, R2, P2, imageSize, cv2.CV_32FC1)

  undistortMaps = []
  undistortMaps.append({'mapX':leftUndistortMapX, 'mapY':leftUndistortMapY})
  undistortMaps.append({'mapX':rightUndistortMapX, 'mapY':rightUndistortMapY})

#windowSizes = [(15,15), (31,31), (51,51)]
windowSizes = [(31,31)]

# Parameters for lucas kanade optical flow
lk_params = dict( winSize  = (31,31),
                  maxLevel = 3,
                  criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03),
                  #minEigThreshold = 1e-4)
                  minEigThreshold = 5e-3)

# Create some random colors
color = np.random.randint(0,255,(10000,3))

# Take first frame and find corners in it
images = captureImages(videoCaptures, undistortMaps)

lastImages = images[:]

waitKeyTime = 25
numPointsPerDimension = 50

allPoints0 = []
for image in images:
    points0 = []
    for y in linspace(0, image.shape[0], numPointsPerDimension):
        for x in linspace(0, image.shape[1], numPointsPerDimension):
            points0.append((x,y))
    points0 = array(points0).reshape(-1,1,2).astype('float32')
    allPoints0.append(points0)

while(1):
    images = captureImages(videoCaptures, undistortMaps)

    for iImage in range(0,len(images)):
        points0 = allPoints0[iImage]

        curImage = images[iImage]
        lastImage = lastImages[iImage]

        for windowSize in windowSizes:

            lk_params['winSize'] = windowSize

            keypointImage, goodPoints0, goodPoints1 = computeLk(lastImage, curImage, points0, lk_params)

            cv2.imshow('flow_' + str(iImage) + '_' + str(windowSize), keypointImage)

    if len(images) == 2:
        for windowSize in windowSizes:

            lk_params['winSize'] = windowSize

            keypointImage, goodPoints0, goodPoints1 = computeLk(images[0], images[1], points0, lk_params)

            cv2.imshow('stereo_' + str(windowSize), keypointImage)

    keypress = cv2.waitKey(waitKeyTime)
    if keypress & 0xFF == ord('q'):
        print('Breaking')
        break

    # Now update the previous frame
    lastImages = images

cap.release()
cv2.destroyAllWindows()
