import numpy as np
from numpy.linalg import *
import cv2
import pdb
import anki

displayMatches = True
firstImageFilename = "/Users/pbarnum/Documents/Anki/products-cozmo-large-files/people.png" # Or None to use the first image captured from the camera
cameraId = 0

bruteForceMatchThreshold = 40

cap = cv2.VideoCapture(cameraId)

for i in range(0,30):
  ret, image = cap.read()

# Camera instrinsics are for the webcam
mainCamera = anki.Camera(
    np.array([[615.036453258611,	0,	284.497629312365],
              [0,	613.503595446291,	276.891781173461],
              [0,	0,	1]]),
    np.array([-0.248653804750381, -0.0186244159234586, -0.00169824021274891, 0.00422932546733130, 0]),
    (640,480))

if firstImageFilename is None:
    ret, image = cap.read()
    image = mainCamera.undistory(image)
else:
    image = cv2.imread(firstImageFilename)

    if len(image.shape) == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

planePattern = anki.PlanePattern(image)

frameNumber = 0
while(True):
    frameNumber += 1

    # Capture frame-by-frame
    ret, image = cap.read()
    image = mainCamera.undistort(image)

    planePattern.match(image, bruteForceMatchThreshold, displayMatches)

    if cv2.waitKey(50) & 0xFF == ord('q'):
        print('Breaking')
        break
    elif cv2.waitKey(50) & 0xFF == ord('c'):
      saveFilename = "/Users/pbarnum/Documents/tmp/savedImage.png"
      print('Saving to ' + saveFilename)
      cv2.imwrite(saveFilename, newFrameGray)

# When everything done, release the capture
cap.release()
cv2.destroyAllWindows()
