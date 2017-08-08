
import cv2

# These don't have the OPENCV prefix CV_CAP_PROP_, but are otherwise identical
captureProperties = {
 'POS_MSEC': 0,
 'POS_FRAMES': 1,
 'POS_AVI_RATIO': 2,
 'FRAME_WIDTH': 3,
 'FRAME_HEIGHT': 4,
 'FPS': 5,
 'FOURCC': 6,
 'FRAME_COUNT': 7,
 'FORMAT': 8,
 'MODE': 9,
 'BRIGHTNESS': 10,
 'CONTRAST': 11,
 'SATURATION': 12,
 'HUE': 13,
 'GAIN': 14,
 'EXPOSURE': 15,
 'CONVERT_RGB': 16,
 'WHITE_BALANCE_BLUE_U': 17,
 'RECTIFICATION': 18,
 'MONOCROME': 19,
 'SHARPNESS': 20,
 'AUTO_EXPOSURE': 21,
 'GAMMA': 22,
 'TEMPERATURE': 23,
 'TRIGGER': 24,
 'TRIGGER_DELAY': 25,
 'WHITE_BALANCE_RED_V': 26,
 'ZOOM': 27,
 'FOCUS': 28,
 'GUID': 29,
 'ISO_SPEED': 30,
 'MAX_DC1394': 31,
 'BACKLIGHT': 32,
 'PAN': 33,
 'TILT': 34,
 'ROLL': 35,
 'IRIS': 36,
 'SETTINGS': 37}
 
def getProperty(cap, propertyName):
  return cap.get(captureProperties[propertyName.upper()])
 
def setProperty(cap, propertyName, value):
  return cap.set(captureProperties[propertyName.upper()], value)
 
def getValidProperties(cap):
  """
  import cv2
  from cameraControl import *
  cap = cv2.VideoCapture(0)
  ret, frame = cap.read()
  cv2.imshow('frame', frame)
  validProperties = getValidProperties(cap)
  print(validProperties)
  """
  
  validProperties = []
  
  for key in captureProperties.keys():
    if cap.get(captureProperties[key]) != 0:
      validProperties.append(key)
      
  return validProperties    
      
