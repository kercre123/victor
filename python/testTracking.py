import numpy as np
import cv2

cap = cv2.VideoCapture(0)

minHessian = 400

#detector = cv2.SURF( minHessian )
detector = cv2.SIFT()

matcher = cv2.FlannBasedMatcher

#matches = matcher.match( descriptors_1, descriptors_2, matches );

ret, firstFrame = cap.read()

firstFrameGray = cv2.cvtColor(firstFrame, cv2.COLOR_BGR2GRAY)
firstFrameGray = cv2.resize(firstFrameGray, dsize=(640,480))

firstKeypoints, firstDescriptors = detector.detectAndCompute(firstFrameGray, None)


# FLANN parameters
FLANN_INDEX_KDTREE = 0
index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
search_params = dict(checks=50)   # or pass empty dictionary

flann = cv2.FlannBasedMatcher(index_params,search_params)

#cv2.waitKey(1000) 

frameNumber = 0
while(True):
    frameNumber += 1

    # Capture frame-by-frame
    ret, frame = cap.read()

    # Our operations on the frame come here
    newFrameGray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    newFrameGray = cv2.resize(newFrameGray, dsize=(640,480))

    newKeypoints, newDescriptors = detector.detectAndCompute(newFrameGray, None)
  
    matches = flann.knnMatch(newDescriptors, firstDescriptors, k=2)

    # Need to draw only good matches, so create a mask
    matchesMask = [[0,0] for i in xrange(len(matches))]

    # ratio test as per Lowe's paper
    matched_original = []
    matched_new = []
    for i,(m,n) in enumerate(matches):
        if m.distance < 0.7*n.distance:
            matchesMask[i]=[1,0]
            
            #if m.trainIdx < len(firstKeypoints):
            matched_original.append(firstKeypoints[m.trainIdx])
             
            #if m.queryIdx < len(newKeypoints):
            matched_new.append(newKeypoints[m.queryIdx])
            
    #draw_params = dict(matchColor = (0,255,0),
    #                   singlePointColor = (255,0,0),
    #                   matchesMask = matchesMask,
    #                   flags = 0)

    #img3 = cv2.drawMatchesKnn(img1,kp1,img2,kp2,matches,None,**draw_params)

    #plt.imshow(img3,),plt.show()
    
    # Display the resulting frame
    #cv2.imshow('frame',gray)
    
    imgKeypoints1 = cv2.drawKeypoints(firstFrameGray, matched_original)  
    imgKeypoints2 = cv2.drawKeypoints(newFrameGray, matched_new)

 #   cv2.imshow('orig',imgKeypoints)
#    cv2.imshow('new',imgKeypoints)
    
    imgKeypointsBoth = np.concatenate((imgKeypoints1,imgKeypoints2), axis=1)
    imgKeypointsBoth = imgKeypointsBoth.copy()
    
    for key1, key2 in zip(matched_original, matched_new):
      pt1 = (int(round(key1.pt[0])), int(round(key1.pt[1])))
      pt2 = (int(round(key2.pt[0]))+imgKeypoints1.shape[1], int(round(key2.pt[1])))
      
      cv2.line(imgKeypointsBoth, pt1, pt2, (255,0,0))
    
    cv2.imshow('new',imgKeypointsBoth)
    
    if cv2.waitKey(50) & 0xFF == ord('q'):
        print('Breaking')
        break

# When everything done, release the capture
cap.release()
cv2.destroyAllWindows()
