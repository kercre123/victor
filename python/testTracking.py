import numpy as np
from numpy.linalg import *
import cv2
import pdb

matchKeypoints = True
computeHomography = True
firstImageFilename = "/Users/pbarnum/Documents/Anki/products-cozmo-large-files/people.png" # Or None to use the first image captured from the camera
useKNNMatch = False # Can't use KNN match with ORB

minHessian = 400
bruteForceMatchThreshold = 40

#detector = cv2.SURF( minHessian )
#detector = cv2.SIFT()
detector = cv2.ORB()

cap = cv2.VideoCapture(0)

for i in range(0,30):
  ret, frame = cap.read()

#cameraMatrix = \
#   np.array([[521.6890083600868,                 0, 324.9274146511045],
#             [                0, 692.2733972989005, 239.6772278674742],
#             [                0,                 0, 1.000000000000000]])
#
#distortionCoefficients = np.array([0.103138670037567, -0.146159931330155, -0.004038458445021, 0.006540160653458, 0])

cameraMatrix = \
  np.array([[615.036453258611,	0,	284.497629312365],
            [0,	613.503595446291,	276.891781173461],
            [0,	0,	1]])

distortionCoefficients = np.array([-0.248653804750381, -0.0186244159234586, -0.00169824021274891, 0.00422932546733130, 0])

mapx, mapy = cv2.initUndistortRectifyMap(cameraMatrix, distortionCoefficients, np.eye(3), cameraMatrix, (640,480), cv2.CV_32FC1)

matcher = cv2.FlannBasedMatcher

if firstImageFilename is None:
  ret, firstFrame = cap.read()

  firstFrameGray = cv2.cvtColor(firstFrame, cv2.COLOR_BGR2GRAY)
  firstFrameGray = cv2.resize(firstFrameGray, dsize=(640,480))
  firstFrameGray = cv2.remap(firstFrameGray, mapx, mapy, cv2.INTER_LINEAR)
else:
  firstFrameGray = cv2.imread(firstImageFilename)

  if len(firstFrameGray.shape) == 3:
    firstFrameGray = cv2.cvtColor(firstFrameGray, cv2.COLOR_BGR2GRAY)

firstKeypoints, firstDescriptors = detector.detectAndCompute(firstFrameGray, None)

# FLANN parameters
FLANN_INDEX_KDTREE = 0
index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
search_params = dict(checks=50)   # or pass empty dictionary

flann = cv2.FlannBasedMatcher(index_params,search_params)
bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)

frameNumber = 0
while(True):
    frameNumber += 1

    # Capture frame-by-frame
    ret, frame = cap.read()

    # Our operations on the frame come here
    newFrameGray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    newFrameGray = cv2.resize(newFrameGray, dsize=(640,480))

    #newFrameGrayUndistorted = cv2.undistort(newFrameGray, cameraMatrix, distortionCoefficients);
    newFrameGray = cv2.remap(newFrameGray, mapx, mapy, cv2.INTER_LINEAR)

    #cv2.imshow('0', newFrameGrayUndistorted)

    if matchKeypoints:
      newKeypoints, newDescriptors = detector.detectAndCompute(newFrameGray, None)

      matched_original = []
      matched_new = []

      if useKNNMatch:
        matches = flann.knnMatch(newDescriptors, firstDescriptors, k=2)

        # Need to draw only good matches, so create a mask
        matchesMask = [[0,0] for i in xrange(len(matches))]

        # ratio test as per Lowe's paper
        for i,(m,n) in enumerate(matches):
            if m.distance < 0.7*n.distance:
                matchesMask[i]=[1,0]

                matched_original.append(firstKeypoints[m.trainIdx])
                matched_new.append(newKeypoints[m.queryIdx])
      else:
        matches = bf.match(newDescriptors, firstDescriptors)
        matches = sorted(matches, key = lambda x:x.distance)

        for i,m in enumerate(matches):
          if m.distance > bruteForceMatchThreshold:
            break;

          matched_original.append(firstKeypoints[m.trainIdx])
          matched_new.append(newKeypoints[m.queryIdx])

      imgKeypoints1 = cv2.drawKeypoints(firstFrameGray, matched_original)
      imgKeypoints2 = cv2.drawKeypoints(newFrameGray, matched_new)

      imgKeypointsBoth = np.concatenate((imgKeypoints1,imgKeypoints2), axis=1)
      imgKeypointsBoth = imgKeypointsBoth.copy()

      if len(matched_original) >= 4:
        matchedArray_original = np.zeros((len(matched_original),2))
        matchedArray_new = np.zeros((len(matched_original),2))

        for i in range(0,len(matched_original)):
          matchedArray_original[i,:] = matched_original[i].pt
          matchedArray_new[i,:] = matched_new[i].pt

        try:
          H = cv2.findHomography( matchedArray_new, matchedArray_original, cv2.RANSAC )
          H = H[0];

          originalPoints = np.matrix([(0,0), (newFrameGray.shape[1],0), (newFrameGray.shape[1],newFrameGray.shape[0]), (0,newFrameGray.shape[0])])
          originalPoints = np.concatenate((originalPoints, np.ones((4,1))), axis=1).transpose()

          warpedPoints = inv(H) * originalPoints
          warpedPoints = warpedPoints[0:2,:] / np.tile(warpedPoints[2,:], (2,1))
          warpedPoints[0,:] += imgKeypoints1.shape[1]
          warpedPoints = warpedPoints.astype('int32').transpose().tolist()
          warpedPoints = [tuple(x) for x in warpedPoints]

          cv2.line(imgKeypointsBoth, warpedPoints[0], warpedPoints[1], (0,255,0), 3)
          cv2.line(imgKeypointsBoth, warpedPoints[1], warpedPoints[2], (0,255,0), 3)
          cv2.line(imgKeypointsBoth, warpedPoints[2], warpedPoints[3], (0,255,0), 3)
          cv2.line(imgKeypointsBoth, warpedPoints[3], warpedPoints[0], (0,255,0), 3)
        except:
          pass

      for key1, key2 in zip(matched_original, matched_new):
        pt1 = (int(round(key1.pt[0])), int(round(key1.pt[1])))
        pt2 = (int(round(key2.pt[0]))+imgKeypoints1.shape[1], int(round(key2.pt[1])))

        cv2.line(imgKeypointsBoth, pt1, pt2, (255,0,0))

      cv2.imshow('new',imgKeypointsBoth)
    else:
      cv2.imshow('frame',newFrameGray)

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
