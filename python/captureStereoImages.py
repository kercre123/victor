import numpy as np
#from numpy.linalg import *
import cv2
import pdb
import os
#from matplotlib import pyplot as plt

leftCameraId = 2
rightCameraId = 0
captureBaseFilename = '/Users/pbarnum/tmp/stereo_'

print('Starting cameras...')

cap0 = cv2.VideoCapture(leftCameraId)
if cap0.isOpened():
  print('Capture0 initialized')
else:
  print('Capture0 initialization failure')
  exit(1)
  pdb.set_trace()

cap1 = cv2.VideoCapture(rightCameraId)
if cap1.isOpened():
  print('Capture1 initialized')
else:
  print('Capture1 initialization failure')
  exit(1)
  pdb.set_trace()

for i in range(0,10):
  ret, frame = cap0.read()
  if frame is None:
    print('Error 0')
    pdb.set_trace()

  ret, frame = cap1.read()
  if frame is None:
    print('Error 1')
    pdb.set_trace()

print('Cameras ready')

stereo = cv2.StereoBM(cv2.STEREO_BM_BASIC_PRESET, 0, 21)
#cv2.createStereoBM(numDisparities=32, blockSize=15)

saveIndex = 0
frameNumber = 0
while(True):
    frameNumber += 1

    ret, leftImage = cap0.read()
    ret, rightImage = cap1.read()

    leftImage = cv2.cvtColor(leftImage, cv2.COLOR_BGR2GRAY)
    rightImage = cv2.cvtColor(rightImage, cv2.COLOR_BGR2GRAY)

    # TODO: rectify

    disparity = stereo.compute(leftImage, rightImage)

    disparity *= 100;

    #pdb.set_trace()

    #plt.imshow(disparity,'gray')
#    plt.show()

    cv2.imshow('disparity', disparity)
    cv2.imshow('leftImage', leftImage)
    cv2.imshow('rightImage', rightImage)

    c = cv2.waitKey(50)

    if (c & 0xFF) == ord('q'):
      print('Breaking')
      break
    elif (c & 0xFF) == ord('c'):
      while True:
        leftImageFilename = captureBaseFilename + 'left_' + str(saveIndex) + '.png'
        rightImageFilename = captureBaseFilename + 'right_' + str(saveIndex) + '.png'
        saveIndex += 1

        if (not os.path.isfile(leftImageFilename)) and (not os.path.isfile(rightImageFilename)):
          break

      print('Saving images to ' + leftImageFilename + ' and ' + rightImageFilename)

      cv2.imwrite(leftImageFilename, leftImage)
      cv2.imwrite(rightImageFilename, rightImage)

# When everything done, release the capture
cap0.release()
cap1.release()
cv2.destroyAllWindows()
