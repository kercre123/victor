import numpy as np
#from numpy.linalg import *
import cv2
import pdb
import os
#from matplotlib import pyplot as plt

leftCameraId = 1
rightCameraId = 0
captureBaseFilename = '/Users/pbarnum/tmp/stereo_'

computeDisparity = True
disparityType = 'bm'
useStereoCalibration = True

if useStereoCalibration:
    # Calibration for the Spynet stereo pair
    distCoeffs1 = np.array([[0.182998, -0.417100, 0.005281, -0.004976, 0.000000]])

    distCoeffs2 = np.array([[0.165733, -0.391832, 0.002453, -0.011111, 0.000000]])

    cameraMatrix1 = np.array([[726.606787, 0.000000, 321.470791],
              [0.000000, 724.172303, 293.913773],
              [0.000000, 0.000000, 1.000000]])

    cameraMatrix2 = np.array([[743.337415, 0.000000, 308.270936],
              [0.000000, 741.060548, 254.206903],
              [0.000000, 0.000000, 1.000000]])

    R = np.array([[0.993527, -0.023641, 0.111109],
              [0.021796, 0.999604, 0.017785],
              [-0.111486, -0.015248, 0.993649]])

    T = np.array([-13.201223, -0.708310, 0.961289])

    imageSize = (640, 480)

    [R1, R2, P1, P2, Q, validPixROI1, validPixROI2] = cv2.stereoRectify(
        cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, R, T)

    leftUndistortMapX, leftUndistortMapY = cv2.initUndistortRectifyMap(
        cameraMatrix1, distCoeffs1, R1, P1, imageSize, cv2.CV_32FC1)

    rightUndistortMapX, rightUndistortMapY = cv2.initUndistortRectifyMap(
        cameraMatrix2, distCoeffs2, R2, P2, imageSize, cv2.CV_32FC1)

if computeDisparity:
    numDisparities = 128

    if disparityType == 'bm':
        disparityWindowSize = 51
        stereo = cv2.StereoBM(cv2.STEREO_BM_BASIC_PRESET, numDisparities, disparityWindowSize)
    elif disparityType == 'sgbm':
        stereo = cv2.StereoSGBM(
            minDisparity = 0, numDisparities=128,
            SADWindowSize=5,
            speckleWindowSize=150, speckleRange = 2,
            preFilterCap=4,
            uniquenessRatio=1,
            disp12MaxDiff = 10,
            P1 = 600, P2 = 2400)

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

saveIndex = 0
frameNumber = 0
while(True):
    frameNumber += 1

    ret, leftImage = cap0.read()
    ret, rightImage = cap1.read()

    leftImage = cv2.cvtColor(leftImage, cv2.COLOR_BGR2GRAY)
    rightImage = cv2.cvtColor(rightImage, cv2.COLOR_BGR2GRAY)

    if useStereoCalibration:
        leftImage = cv2.remap(leftImage, leftUndistortMapX, leftUndistortMapY, cv2.INTER_LINEAR)
        rightImage = cv2.remap(rightImage, rightUndistortMapX, rightUndistortMapY, cv2.INTER_LINEAR)

    # DoG filtering (doesn't work well with the JPEG)
    #leftImage_g1 = cv2.GaussianBlur(leftImage, (0,0), 1)
    #leftImage_g2 = cv2.GaussianBlur(leftImage, (0,0), 2)
    #leftImage = leftImage_g1 - leftImage_g2
    #rightImage_g1 = cv2.GaussianBlur(rightImage, (0,0), 1)
    #rightImage_g2 = cv2.GaussianBlur(rightImage, (0,0), 2)
    #rightImage = rightImage_g1 - rightImage_g2

    if computeDisparity:
        disparity = stereo.compute(leftImage, rightImage)
        disparity *= 100;
        cv2.imshow('disparity', disparity)
        cv2.moveWindow('disparity', 0, leftImage.shape[0]+50)

    cv2.imshow('leftImage', leftImage)
    cv2.imshow('rightImage', rightImage)

    cv2.moveWindow('leftImage', 0, 0)
    cv2.moveWindow('rightImage', leftImage.shape[1]+20, 0)

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
