import numpy as np
from numpy.linalg import *
import cv2
import pdb

print('Starting cameras...')

cap0 = cv2.VideoCapture(0)
if cap0.isOpened():
  print('Capture0 initialized')
else:
  print('Capture0 initialization failure')
  exit(1)
  pdb.set_trace()
    
cap1 = cv2.VideoCapture(2)
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

frameNumber = 0
while(True):
    frameNumber += 1

    ret, frame0 = cap0.read()
    ret, frame1 = cap1.read()

    cv2.imshow('frame0', frame0)
    cv2.imshow('frame1', frame1)

    c = cv2.waitKey(50)
    
    if (cv2.waitKey(50) & 0xFF) == ord('q'):
        print('Breaking')
        break

# When everything done, release the capture
cap0.release()
cap1.release()
cv2.destroyAllWindows()
