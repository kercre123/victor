import numpy as np
import cv2
import pdb
import os
import sys
from cameraControl import *

def captureImages(cameraId, captureBaseFilename, saveColor, drawCrosshair):
  #cameraId = 0
  #captureBaseFilename = '/Users/pbarnum/tmp/image_'
  #saveColor = True

  print('Starting camera...')

  cap0 = cv2.VideoCapture(cameraId)
  if cap0.isOpened():
      print('Capture0 initialized')
  else:
      print('Capture0 initialization failure')
      exit()
      pdb.set_trace()

  #validProperties = getProperties(cap0)
  #print(validProperties)

  for i in range(0,10):
      ret, frame = cap0.read()
      if frame is None:
          print('Error 0')
          pdb.set_trace()

  print('Camera ready')

  saveIndex = 0
  frameNumber = 0
  saveVideo = False
  while(True):
      frameNumber += 1

      ret, leftImage = cap0.read()

      if saveColor:
        leftImageToShow = leftImage.copy()
      else:
        leftImage = cv2.cvtColor(leftImage, cv2.COLOR_BGR2GRAY)
        leftImageToShow = cv2.merge([leftImage,leftImage,leftImage])

      if drawCrosshair:
        centerX = leftImageToShow.shape[1]/2
        centerY = leftImageToShow.shape[0]/2
        
        leftImageToShow[(centerY-1):(centerY+2), :, :] = 0;
        leftImageToShow[:, (centerX-1):(centerX+2), :] = 0;
        
        leftImageToShow[centerY, :, :] = 255;
        leftImageToShow[:, centerX, :] = 255;
        
      cv2.imshow('leftImage', leftImageToShow)

      c = cv2.waitKey(50)

      if (c & 0xFF) == ord('q'):
          print('Breaking')
          break
      elif (c & 0xFF) == ord('s'):
        saveVideo = not saveVideo
      elif saveVideo or ((c & 0xFF) == ord('c')):
          while True:
              leftImageFilename = captureBaseFilename + str(saveIndex) + '.png'
              saveIndex += 1

              if (not os.path.isfile(leftImageFilename)):
                  break

          print('Saving image to ' + leftImageFilename)

          cv2.imwrite(leftImageFilename, leftImage)

  # When everything done, release the capture
  cap0.release()
  cv2.destroyAllWindows()

if __name__ == '__main__':
  drawCrosshair = True
  cameraId = 0
  captureBaseFilename = '/Users/pbarnum/tmp/image_'
  saveColor = True
  
  if len(sys.argv) > 1:
    cameraId = int(sys.argv[1])

  if len(sys.argv) > 2:
    captureBaseFilename = sys.argv[2]
  
  if len(sys.argv) > 3:
    saveColor = bool(int(sys.argv[3]))
    
  print('cameraId: ' + str(cameraId) + '   captureBaseFilename: ' + captureBaseFilename + '   saveColor: ' + str(saveColor))
    
  captureImages(cameraId, captureBaseFilename, saveColor, drawCrosshair)
  
