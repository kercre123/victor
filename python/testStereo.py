import numpy as np
from numpy.linalg import *
import cv2
import pdb

print('Starting cameras...')

cap0 = cv2.VideoCapture(1)
print('Capture0 initialized')

cap1 = cv2.VideoCapture(2)
print('Capture1 initialized')

for i in range(0,30):
  ret, frame = cap0.read()
  if frame is None:
    print('Error')
    pdb.set_trace()

  ret, frame = cap1.read()
  if frame is None:
    print('Error')
    pdb.set_trace()

print('Cameras ready')

frameNumber = 0
while(True):
    frameNumber += 1

    ret, frame0 = cap0.read()
    ret, frame1 = cap1.read()

    cv2.imshow('frame0', frame0)
    cv2.imshow('frame1', frame1)

    cv2.waitKey(50)

#    if cv2.waitKey(50) & 0xFF == ord('q'):
#        print('Breaking')
#        break
#    elif cv2.waitKey(50) & 0xFF == ord('c'):
#      saveFilename = "/Users/pbarnum/Documents/tmp/savedImage.png"
#      print('Saving to ' + saveFilename)
#      cv2.imwrite(saveFilename, newFrameGray)

# When everything done, release the capture
cap.release()
cv2.destroyAllWindows()
