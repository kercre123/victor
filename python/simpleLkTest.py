import numpy as np
import cv2
import pdb

cap = 0
cap = cv2.VideoCapture(0)

useFast = False

# params for ShiTomasi corner detection
feature_params = dict( maxCorners = 5000,
                       qualityLevel = 0.5,
                       minDistance = 2,
                       blockSize = 21 )

# Parameters for lucas kanade optical flow
lk_params = dict( winSize  = (15,15),
                  maxLevel = 5,
                  criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))

# Create some random colors
color = np.random.randint(0,255,(10000,3))

# Take first frame and find corners in it
ret, old_frame = cap.read()
old_gray = cv2.cvtColor(old_frame, cv2.COLOR_BGR2GRAY)

if useFast:
  fast = cv2.FastFeatureDetector()
  fastCorners = fast.detect(old_gray,None)
  p0 = np.zeros((len(fastCorners), 1, 2), 'float32')

  for i in range(0,len(fastCorners)):
    p0[i,0,0] = fastCorners[i].pt[0]
    p0[i,0,1] = fastCorners[i].pt[1]
else:
  p0 = cv2.goodFeaturesToTrack(old_gray, mask = None, **feature_params)

# Create a mask image for drawing purposes
mask = np.zeros_like(old_frame)

frameNum = 0
while(1):
    ret,frame = cap.read()
    frame_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # calculate optical flow
    p1, st, err = cv2.calcOpticalFlowPyrLK(old_gray, frame_gray, p0, None, **lk_params)

    frameNum += 1

    if p1 is None or st is None or (frameNum % 60 == 0):
      if useFast:
        fastCorners = fast.detect(frame_gray,None)
        p0 = np.zeros((len(fastCorners), 1, 2), 'float32')

        for i in range(0,len(fastCorners)):
          p0[i,0,0] = fastCorners[i].pt[0]
          p0[i,0,1] = fastCorners[i].pt[1]
      else:
        p0 = cv2.goodFeaturesToTrack(frame_gray, mask = None, **feature_params)

      mask = np.zeros_like(old_frame)
      continue

    # Select good points
    good_new = p1[st==1]
    good_old = p0[st==1]

    # color = np.random.randint(0,255,(len(good_new),3))

    # draw the tracks
    for i,(new,old) in enumerate(zip(good_new,good_old)):
        a,b = new.ravel()
        c,d = old.ravel()
        #cv2.line(mask, (a,b),(c,d), color[i].tolist(), 1)
        cv2.circle(frame,(a,b),3,color[i].tolist(),-1)
    img = cv2.add(frame,mask)

    cv2.imshow('frame',img)
    k = cv2.waitKey(30) & 0xff
    if k == 27:
        break

    # Now update the previous frame and previous points
    old_gray = frame_gray.copy()
    p0 = good_new.reshape(-1,1,2)

cv2.destroyAllWindows()
cap.release()
