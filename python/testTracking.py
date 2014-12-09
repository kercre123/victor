# Use python 3 types by default (uses "future" package)
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
from future.builtins import *

import cv2
import numpy as np
import pdb
import os
import anki
import platform

print('Starting tracking')

displayMatches = True

captureBaseFilename = '/Users/pbarnum/Documents/tmp/webcam_'

# Or None to use the first image captured from the camera
#templateImageFilename = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/people.png'
templateImageFilename = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/peopleScanned640x480.png'

# Or None to capture images from the webcam
#queryImageFilename = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/webcam_0.png'
#queryImageFilename = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/people_webcam_0.png'
#queryImageFilename = '/Users/pbarnum/Documents/tmp/webcam_1.png'
queryImageFilename = None

if platform.system() == 'Windows':
    if templateImageFilename is not None:
        templateImageFilename = templateImageFilename.replace('/Users/pbarnum', 'z:')

    if captureBaseFilename is not None:
        captureBaseFilename = captureBaseFilename.replace('/Users/pbarnum', 'z:')

    if queryImageFilename is not None:
        queryImageFilename = queryImageFilename.replace('/Users/pbarnum', 'z:')

cameraId = 0

bruteForceMatchThreshold = 40

if templateImageFilename is None or queryImageFilename is None:
    cap = cv2.VideoCapture(cameraId)

    for i in range(0,10):
      ret, image = cap.read()

# Camera instrinsics are for the webcam
mainCamera = anki.Camera(
    np.array([[615.036453258611,	0,	284.497629312365],
              [0,	613.503595446291,	276.891781173461],
              [0,	0,	1]]),
    np.array([-0.248653804750381, -0.0186244159234586, -0.00169824021274891, 0.00422932546733130, 0]),
    (480,640))

preMatchHomographies = [
    np.array([[1,0,0],
              [0,1,0],
              [0,0,1]]),
    np.array([[2.630014, 1.242951, -524.176621],
              [-0.325372, 5.798278, -958.557600],
              [-0.000597, 0.005180, 1.000000]]),
    np.array([[4.762305, 4.681719, -1002.478256],
              [-0.814753, 20.148339, -2401.653955],
              [-0.001467, 0.019998, 1.000000]]),
    np.array([[-0.421779, -1.299690, 395.029482],
              [0.269990, -4.149752, 1198.168246],
              [0.000445, -0.004759, 1.000000]]),
    np.array([[-0.481675, -1.343197, 420.610810],
              [-0.272449, -4.218317, 1138.355504],
              [-0.000316, -0.004615, 1.000000]]),
    np.array([[-1.327539, -4.234972, 690.321761],
              [-0.620908, -8.153917, 1118.305704],
              [-0.001250, -0.013606, 1.000000]])]

if templateImageFilename is None:
    ret, image = cap.read()
    assert image is not None, 'Could not capture image'
    image = mainCamera.undistort(image)
else:
    image = cv2.imread(templateImageFilename)
    assert image is not None, 'Could not load image'

    if len(image.shape) == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

planePattern = anki.PlanePattern(image)

if queryImageFilename is not None:
    image = cv2.imread(queryImageFilename)
    assert image is not None, 'Could not load image'

    if len(image.shape) == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

toUseHomographies = preMatchHomographies[:]

waitKeyTime = 200

saveIndex = 0
frameNumber = 0
while(True):
    frameNumber += 1

    if queryImageFilename is None:
        # Capture frame-by-frame
        ret, image = cap.read()
        assert image is not None, 'Could not capture image'
        image = mainCamera.undistort(image)
        undistortedImage = image.copy()

    newHomography = planePattern.match(image, bruteForceMatchThreshold, preMatchHomographies, displayMatches)

    toUseHomographies = preMatchHomographies[:]
    toUseHomographies.append(newHomography)

    keypress = cv2.waitKey(waitKeyTime)
    if keypress & 0xFF == ord('q'):
        print('Breaking')
        break
    elif keypress & 0xFF == ord('c'):
        while True:
            imageFilename = captureBaseFilename + str(saveIndex) + '.png'
            saveIndex += 1

            if not os.path.isfile(imageFilename):
                break

        print('Saving image to ' + imageFilename)

        cv2.imwrite(imageFilename, undistortedImage)

# When everything done, release the capture

cv2.destroyAllWindows()
if templateImageFilename is None or queryImageFilename is None:
    cap.release()

