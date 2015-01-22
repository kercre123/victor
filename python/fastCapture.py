import numpy as np
import cv2
import pdb
import os
from getStereoParameters import getStereoParameters
maxImagesToSave = 10000
from saveStereo import saveStereo

imagesToSave = []

leftCameraId = 0
rightCameraId = 1
captureBaseFilename = '/Users/pbarnum/Documents/tmp/stereo/stereo_'

useStereoCalibration = True

if useStereoCalibration:
    stereoParameters = getStereoParameters(True)

    [R1, R2, P1, P2, Q, validPixROI1, validPixROI2] = cv2.stereoRectify(stereoParameters['cameraMatrix1'], stereoParameters['distCoeffs1'], stereoParameters['cameraMatrix2'], stereoParameters['distCoeffs2'], stereoParameters['imageSize'], stereoParameters['R'], stereoParameters['T'])

    leftUndistortMapX, leftUndistortMapY = cv2.initUndistortRectifyMap(
        stereoParameters['cameraMatrix1'], stereoParameters['distCoeffs1'], R1, P1, stereoParameters['imageSize'], cv2.CV_32FC1)

    rightUndistortMapX, rightUndistortMapY = cv2.initUndistortRectifyMap(
        stereoParameters['cameraMatrix2'], stereoParameters['distCoeffs2'], R2, P2, stereoParameters['imageSize'], cv2.CV_32FC1)

print('Starting cameras...')

cap0 = cv2.VideoCapture(leftCameraId)
if cap0.isOpened():
  print('Capture0 initialized')
else:
  print('Capture0 initialization failure')
  exit()
  pdb.set_trace()

cap1 = cv2.VideoCapture(rightCameraId)
if cap1.isOpened():
  print('Capture1 initialized')
else:
  print('Capture1 initialization failure')
  exit()
  pdb.set_trace()

for i in range(0,1):
  ret, frame = cap0.read()
  if frame is None:
    print('Error 0')
    pdb.set_trace()

  ret, frame = cap1.read()
  if frame is None:
    print('Error 1')
    pdb.set_trace()

print('Cameras ready')

frameNumber = 0
while(True):
    frameNumber += 1
    if frameNumber > maxImagesToSave:
        break

    ret, leftImage = cap0.read()
    ret, rightImage = cap1.read()

    leftImage = cv2.cvtColor(leftImage, cv2.COLOR_BGR2GRAY)
    rightImage = cv2.cvtColor(rightImage, cv2.COLOR_BGR2GRAY)

    imagesToSave.append([leftImage, rightImage])

    cv2.imshow('leftImage', leftImage)
    cv2.imshow('rightImage', rightImage)

    c = cv2.waitKey(10)

    if (c & 0xFF) == ord('q'):
        print('Breaking')
        break

print('Saving images')
for images in imagesToSave:
    if useStereoCalibration:
        leftRectifiedImage = cv2.remap(images[0], leftUndistortMapX, leftUndistortMapY, cv2.INTER_LINEAR)
        rightRectifiedImage = cv2.remap(images[1], rightUndistortMapX, rightUndistortMapY, cv2.INTER_LINEAR)
    else:
        leftRectifiedImage = None
        rightRectifiedImage = None

    saveStereo(images[0], images[1], leftRectifiedImage, rightRectifiedImage, captureBaseFilename)

print('Done saving images')

# When everything done, release the capture
cap0.release()
cap1.release()
cv2.destroyAllWindows()
