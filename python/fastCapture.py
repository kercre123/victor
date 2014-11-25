import numpy as np
import cv2
import pdb
import os

maxImagesToSave = 10000

imagesToSave = []

leftCameraId = 1
rightCameraId = 2
captureBaseFilename = '/Users/pbarnum/Documents/tmp/stereo/stereo_'

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

saveIndex = 0
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
    while True:
        leftImageFilename = captureBaseFilename + 'left_' + str(saveIndex) + '.png'
        rightImageFilename = captureBaseFilename + 'right_' + str(saveIndex) + '.png'
        saveIndex += 1
        if (not os.path.isfile(leftImageFilename)) and (not os.path.isfile(rightImageFilename)):
            break

    cv2.imwrite(leftImageFilename, images[0])
    cv2.imwrite(rightImageFilename, images[1])

print('Done saving images')

# When everything done, release the capture
cap0.release()
cap1.release()
cv2.destroyAllWindows()
