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

templateImageFilename = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/people.png' # Or None to use the first image captured from the camera
captureBaseFilename = '/Users/pbarnum/Documents/tmp/webcam_'

queryImageFilename = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/webcam_0.png' # Or None to capture images from the webcam
#queryImageFilename = None
    
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

if templateImageFilename is None:
    ret, image = cap.read()
    image = mainCamera.undistory(image)
else:
    image = cv2.imread(templateImageFilename)

    if len(image.shape) == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

planePattern = anki.PlanePattern(image)

if queryImageFilename is not None:
    image = cv2.imread(queryImageFilename)

    if len(image.shape) == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

saveIndex = 0
frameNumber = 0
while(True):
    frameNumber += 1

    if queryImageFilename is None:
        # Capture frame-by-frame
        ret, image = cap.read()
        image = mainCamera.undistort(image)

    planePattern.match(image, bruteForceMatchThreshold, displayMatches)

    if cv2.waitKey(50) & 0xFF == ord('q'):
        print('Breaking')
        break
    elif cv2.waitKey(50) & 0xFF == ord('c'):
        while True:
            imageFilename = captureBaseFilename + str(saveIndex) + '.png'
            saveIndex += 1

            if not os.path.isfile(imageFilename):
                break

        print('Saving image to ' + imageFilename)

        cv2.imwrite(imageFilename, image)

# When everything done, release the capture

cv2.destroyAllWindows()
if templateImageFilename is None or queryImageFilename is None:
    cap.release()

