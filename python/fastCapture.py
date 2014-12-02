import numpy as np
import cv2
import pdb
import os

maxImagesToSave = 10000

imagesToSave = []

leftCameraId = 0
rightCameraId = 1
captureBaseFilename = '/Users/pbarnum/Documents/tmp/stereo/stereo_'

useStereoCalibration = True

if useStereoCalibration:
    # Calibration for the Spynet stereo pair
    distCoeffs1 = np.array([[0.164272, -0.376660, 0.000841, -0.013974, 0.000000]])

    distCoeffs2 = np.array([[0.168726, -0.365303, 0.006350, -0.007782, 0.000000]])

    cameraMatrix1 = np.array([[740.272685, 0.000000, 303.320456],
              [0.000000, 737.724452, 250.412135],
              [0.000000, 0.000000, 1.000000]])

    cameraMatrix2 = np.array([[723.767887, 0.000000, 317.103693],
              [0.000000, 721.290002, 295.642274],
              [0.000000, 0.000000, 1.000000]])

    R = np.array([[0.999395, 0.030507, -0.016679],
              [-0.030129, 0.999293, 0.022470],
              [0.017353, -0.021954, 0.999608]])

    T = np.array([-13.399603, 0.297130, 0.749115])

    imageSize = (640, 480)

    [R1, R2, P1, P2, Q, validPixROI1, validPixROI2] = cv2.stereoRectify(
        cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, R, T)

    leftUndistortMapX, leftUndistortMapY = cv2.initUndistortRectifyMap(
        cameraMatrix1, distCoeffs1, R1, P1, imageSize, cv2.CV_32FC1)

    rightUndistortMapX, rightUndistortMapY = cv2.initUndistortRectifyMap(
        cameraMatrix2, distCoeffs2, R2, P2, imageSize, cv2.CV_32FC1)

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
        leftRectifiedImageFilename = captureBaseFilename + 'leftRectified_' + str(saveIndex) + '.png'
        rightRectifiedImageFilename = captureBaseFilename + 'rightRectified_' + str(saveIndex) + '.png'
        saveIndex += 1
        if (not os.path.isfile(leftImageFilename)) and (not os.path.isfile(rightImageFilename)) and\
            (not os.path.isfile(leftRectifiedImageFilename)) and (not os.path.isfile(rightRectifiedImageFilename)):
                break

    cv2.imwrite(leftImageFilename, images[0])
    cv2.imwrite(rightImageFilename, images[1])

    if useStereoCalibration:
        leftRectifiedImage = cv2.remap(images[0], leftUndistortMapX, leftUndistortMapY, cv2.INTER_LINEAR)
        rightRectifiedImage = cv2.remap(images[1], rightUndistortMapX, rightUndistortMapY, cv2.INTER_LINEAR)
        cv2.imwrite(leftRectifiedImageFilename, leftRectifiedImage)
        cv2.imwrite(rightRectifiedImageFilename, rightRectifiedImage)

print('Done saving images')

# When everything done, release the capture
cap0.release()
cap1.release()
cv2.destroyAllWindows()
