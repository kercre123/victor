# Use python 3 types by default (uses "future" package)
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
from future.builtins import *

import cv2
import numpy as np
from numpy import *
import pdb
import os
import anki
import platform
from getStereoParameters import getStereoParameters
from saveStereo import saveStereo

def captureImages(videoCaptures, undistortMaps, rightImageWarpHomography):
    imagesRaw =  []
    imagesRectified = []

    for i,cap in enumerate(videoCaptures):
        ret,imageRaw = cap.read()
        imageRaw = cv2.cvtColor(imageRaw, cv2.COLOR_BGR2GRAY)

        if undistortMaps is not None:
            imageRectified = cv2.remap(imageRaw, undistortMaps[i]['mapX'], undistortMaps[i]['mapY'], cv2.INTER_LINEAR)

            if (i == 1) and (rightImageWarpHomography is not None):
                imageRectified = cv2.warpPerspective(imageRectified, rightImageWarpHomography, imageRectified.shape[::-1])

        imagesRaw.append(imageRaw)
        imagesRectified.append(imageRectified)

    return imagesRaw, imagesRectified

def drawKeypointMatches(image, points0, points1, distanceColormap=None):
    """
    draw the tracks
    """

    keypointImage = cv2.merge((image,image,image))

    for i,(new,old) in enumerate(zip(points0,points1)):
        a,b = new.ravel()
        c,d = old.ravel()

        if distanceColormap is None:
            curColor = color[i].tolist()
        else:
            dist = int(sqrt((a-c)**2 + (b-d)**2))
            if dist > (len(distanceColormap)-1):
                dist = len(distanceColormap)-1
            curColor = distanceColormap[dist]

        cv2.line(keypointImage, (a,b),(c,d), curColor, 1)
        #cv2.circle(keypointImage, (a,b), 3, curColor, -1)

    return keypointImage

def drawStereoFlowKeypointMatches(image, points0, points1Flow, points1Stereo, distanceColormap):
    """
    draw the tracks
    """

    keypointImage = cv2.merge((image,image,image))

    for i, (point0,pointF,pointS) in enumerate(zip(points0,points1Flow,points1Stereo)):
        p0x,p0y = point0.ravel()
        pFx,pFy = pointF.ravel()
        pSx,pSy = pointS.ravel()

        if distanceColormap is None:
            curColor = color[i].tolist()
        else:
            dist = int(sqrt((p0x-pSx)**2 + (p0y-pSy)**2))
            if dist > (len(distanceColormap)-1):
                dist = len(distanceColormap)-1
            curColor = distanceColormap[dist]

        cv2.line(keypointImage, (p0x,p0y), (pFx,pFy), curColor, 1)

    return keypointImage

def localMinimaness(image, halfWindowSize, x, y):
    x = int(x)
    y = int(y)
    if ((halfWindowSize[0]+1) < x < (image.shape[1]-halfWindowSize[0]-2)) and ((halfWindowSize[1]+1) < y < (image.shape[0]-halfWindowSize[1]-2)):

        window_00 = image[(y-halfWindowSize[1]):(y+halfWindowSize[1]+1), (x-halfWindowSize[0]):(x+halfWindowSize[0]+1)]

        # x +1 or -1
        window_n0 = image[(y-halfWindowSize[1]):(y+halfWindowSize[1]+1), (x-halfWindowSize[0]-1):(x+halfWindowSize[0])]
        window_p0 = image[(y-halfWindowSize[1]):(y+halfWindowSize[1]+1), (x-halfWindowSize[0]+1):(x+halfWindowSize[0]+2)]
        leftSlope = abs(window_00.astype('float64') - window_n0.astype('float64')).mean()
        rightSlope = abs(window_00.astype('float64') - window_p0.astype('float64')).mean()

        # y +1 or -1
        window_0n = image[(y-halfWindowSize[1]-1):(y+halfWindowSize[1]), (x-halfWindowSize[0]):(x+halfWindowSize[0]+1)]
        window_0p = image[(y-halfWindowSize[1]+1):(y+halfWindowSize[1]+2), (x-halfWindowSize[0]):(x+halfWindowSize[0]+1)]
        topSlope = abs(window_00.astype('float64') - window_0n.astype('float64')).mean()
        bottomSlope = abs(window_00.astype('float64') - window_0p.astype('float64')).mean()

        minSlope = min([leftSlope, rightSlope, topSlope, bottomSlope])
    else:
        minSlope = 0

    return minSlope

#cameraIds = [1]
cameraIds = [1,0]

videoCaptures = []
for cameraId in cameraIds:
    videoCaptures.append(cv2.VideoCapture(cameraId))

useStereoCalibration = True
undistortMaps = None
computeStereoBm = True
computeFlow = False
computeStereoFlow = False

captureBaseFilename = '/Users/pbarnum/tmp/stereo_'

# Computed in Matlab with "toArray(computeStereoColormap(128), true)"

stereoDisparityColors128 = [(255, 0, 0), (255, 7, 0), (255, 16, 0), (255, 24, 0), (255, 32, 0), (255, 41, 0), (255, 49, 0), (255, 57, 0), (255, 65, 0), (255, 73, 0), (255, 82, 0), (255, 90, 0), (255, 99, 0), (255, 107, 0), (255, 115, 0), (255, 124, 0), (255, 132, 0), (255, 140, 0), (255, 148, 0), (255, 156, 0), (255, 165, 0), (255, 173, 0), (255, 182, 0), (255, 190, 0), (255, 198, 0), (255, 207, 0), (255, 215, 0), (255, 223, 0), (255, 231, 0), (255, 239, 0), (255, 248, 0), (255, 255, 0), (255, 255, 0), (248, 255, 0), (239, 255, 0), (231, 255, 0), (223, 255, 0), (215, 255, 0), (207, 255, 0), (198, 255, 0), (190, 255, 0), (182, 255, 0), (173, 255, 0), (165, 255, 0), (156, 255, 0), (148, 255, 0), (140, 255, 0), (132, 255, 0), (124, 255, 0), (115, 255, 0), (107, 255, 0), (99, 255, 0), (90, 255, 0), (82, 255, 0), (73, 255, 0), (65, 255, 0), (57, 255, 0), (49, 255, 0), (41, 255, 0), (32, 255, 0), (24, 255, 0), (16, 255, 0), (7, 255, 0), (0, 255, 0), (0, 255, 0), (0, 255, 7), (0, 255, 16), (0, 255, 24), (0, 255, 32), (0, 255, 41), (0, 255, 49), (0, 255, 57), (0, 255, 65), (0, 255, 73), (0, 255, 82), (0, 255, 90), (0, 255, 99), (0, 255, 107), (0, 255, 115), (0, 255, 124), (0, 255, 132), (0, 255, 140), (0, 255, 148), (0, 255, 156), (0, 255, 165), (0, 255, 173), (0, 255, 182), (0, 255, 190), (0, 255, 198), (0, 255, 207), (0, 255, 215), (0, 255, 223), (0, 255, 231), (0, 255, 239), (0, 255, 248), (0, 255, 255), (0, 255, 255), (0, 248, 255), (0, 239, 255), (0, 231, 255), (0, 223, 255), (0, 215, 255), (0, 207, 255), (0, 198, 255), (0, 190, 255), (0, 182, 255), (0, 173, 255), (0, 165, 255), (0, 156, 255), (0, 148, 255), (0, 140, 255), (0, 132, 255), (0, 124, 255), (0, 115, 255), (0, 107, 255), (0, 99, 255), (0, 90, 255), (0, 82, 255), (0, 73, 255), (0, 65, 255), (0, 57, 255), (0, 49, 255), (0, 41, 255), (0, 32, 255), (0, 24, 255), (0, 16, 255), (0, 7, 255), (0, 0, 255)]
stereoDisparityColors64 = [(255, 4, 0), (255, 20, 0), (255, 36, 0), (255, 53, 0), (255, 70, 0), (255, 86, 0), (255, 103, 0), (255, 119, 0), (255, 136, 0), (255, 152, 0), (255, 169, 0), (255, 185, 0), (255, 202, 0), (255, 220, 0), (255, 235, 0), (255, 251, 0), (251, 255, 0), (235, 255, 0), (220, 255, 0), (202, 255, 0), (185, 255, 0), (169, 255, 0), (152, 255, 0), (136, 255, 0), (119, 255, 0), (103, 255, 0), (86, 255, 0), (70, 255, 0), (53, 255, 0), (36, 255, 0), (20, 255, 0), (4, 255, 0), (0, 255, 4), (0, 255, 20), (0, 255, 36), (0, 255, 53), (0, 255, 70), (0, 255, 86), (0, 255, 103), (0, 255, 119), (0, 255, 136), (0, 255, 152), (0, 255, 169), (0, 255, 185), (0, 255, 202), (0, 255, 220), (0, 255, 235), (0, 255, 251), (0, 251, 255), (0, 235, 255), (0, 220, 255), (0, 202, 255), (0, 185, 255), (0, 169, 255), (0, 152, 255), (0, 136, 255), (0, 119, 255), (0, 103, 255), (0, 86, 255), (0, 70, 255), (0, 53, 255), (0, 36, 255), (0, 20, 255), (0, 4, 255)]

if len(cameraIds) == 1:
   useStereoCalibration = False

rightImageWarpHomography = matrix(eye(3))

if useStereoCalibration:
    numDisparities = 192
    disparityType = 'sgbm'
    if disparityType == 'bm':
        disparityWindowSize = 51
        stereo = cv2.StereoBM(cv2.STEREO_BM_BASIC_PRESET, numDisparities, disparityWindowSize)
    elif disparityType == 'sgbm':
         stereo = cv2.StereoSGBM(
             minDisparity = 0, numDisparities=numDisparities,
             SADWindowSize=5,
             speckleWindowSize=150, speckleRange = 2,
             preFilterCap=4,
             uniquenessRatio=1,
             disp12MaxDiff = 10,
             P1 = 600, P2 = 2400)

    stereoParameters = getStereoParameters(True)

    [R1, R2, P1, P2, Q, validPixROI1, validPixROI2] = cv2.stereoRectify(stereoParameters['cameraMatrix1'], stereoParameters['distCoeffs1'], stereoParameters['cameraMatrix2'], stereoParameters['distCoeffs2'], stereoParameters['imageSize'], stereoParameters['R'], stereoParameters['T'])

    leftUndistortMapX, leftUndistortMapY = cv2.initUndistortRectifyMap(
        stereoParameters['cameraMatrix1'], stereoParameters['distCoeffs1'], R1, P1, stereoParameters['imageSize'], cv2.CV_32FC1)

    rightUndistortMapX, rightUndistortMapY = cv2.initUndistortRectifyMap(
        stereoParameters['cameraMatrix2'], stereoParameters['distCoeffs2'], R2, P2, stereoParameters['imageSize'], cv2.CV_32FC1)

    undistortMaps = []
    undistortMaps.append({'mapX':leftUndistortMapX, 'mapY':leftUndistortMapY, 'roi':validPixROI1})
    undistortMaps.append({'mapX':rightUndistortMapX, 'mapY':rightUndistortMapY, 'roi':validPixROI2})

#windowSizes = [(15,15), (31,31), (51,51)]
#windowSizes = [(31,31)]
windowSizes = [(51,51)]

# Parameters for lucas kanade optical flow
flow_lk_params = dict(winSize  = (31,31),
                      maxLevel = 4,
                      criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03),
                      #minEigThreshold = 1e-4)
                      minEigThreshold = 5e-3)

stereo_lk_params = dict(winSize  = (31,31),
                        maxLevel = 5,
                        criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03),
                        #minEigThreshold = 1e-4)
                        minEigThreshold = 1e-4)

stereo_lk_maxAngle = pi/16

# Create some random colors
color = np.random.randint(0,255,(10000,3))

# Take first frame and find corners in it
imagesRaw, images = captureImages(videoCaptures, undistortMaps, rightImageWarpHomography)

lastImages = images[:]

waitKeyTime = 25
numPointsPerDimension = 50

allPoints0 = []
for image in images:
    points0 = []
    for y in linspace(0, image.shape[0], numPointsPerDimension):
        for x in linspace(0, image.shape[1], numPointsPerDimension):
            points0.append((x,y))
    points0 = array(points0).reshape(-1,1,2).astype('float32')
    allPoints0.append(points0)

while(1):
    imagesRaw, images = captureImages(videoCaptures, undistortMaps, rightImageWarpHomography)

    cv2.imshow('leftImage', images[0])
    cv2.imshow('rightImage', images[1])

    if computeFlow:
        # Compute the flow
        allFlowPoints = []
        for iImage in range(0,len(images)):
            points0 = allPoints0[iImage]

            curImage = images[iImage]
            lastImage = lastImages[iImage]

            curFlowPoints = []

            for windowSize in windowSizes:

                flow_lk_params['winSize'] = windowSize

                points1, st, err = cv2.calcOpticalFlowPyrLK(lastImage, curImage, points0, None, **flow_lk_params)

                # Select good points
                goodPoints0 = points0[st==1]
                goodPoints1 = points1[st==1]

                curFlowPoints.append({'points1':points1,'st':st,'err':err,'goodPoints0':goodPoints0,'goodPoints1':goodPoints1,'curImage':curImage, 'lastImage':lastImage, 'windowSize':windowSize, 'iImage':iImage})

            allFlowPoints.append(curFlowPoints)

        # Draw the flow
        for curFlowPoints in allFlowPoints:
            for flowPoints in curFlowPoints:
                keypointImage = drawKeypointMatches(flowPoints['curImage'], flowPoints['goodPoints0'], flowPoints['goodPoints1'], stereoDisparityColors128)
                cv2.imshow('flow_' + str(flowPoints['iImage']) + '_' + str(flowPoints['windowSize']), keypointImage)

    # Stereo
    if len(images) == 2 and computeStereoBm:
        for windowSize in windowSizes:
            disparity = stereo.compute(cv2.equalizeHist(images[0]), cv2.equalizeHist(images[1]))
            disparity >>= 5

            disparityToShow = disparity.copy()
            disparityToShow[disparityToShow<0] = 0
            disparityToShow = disparityToShow.astype('uint8')

            roiLeft = max(undistortMaps[0]['roi'][0], undistortMaps[1]['roi'][0])
            roiRight = min(undistortMaps[0]['roi'][0] + undistortMaps[0]['roi'][2], undistortMaps[1]['roi'][0] + undistortMaps[1]['roi'][2])
            roiTop = max(undistortMaps[0]['roi'][1], undistortMaps[1]['roi'][1])
            roiBottom = min(undistortMaps[0]['roi'][1] + undistortMaps[0]['roi'][3], undistortMaps[1]['roi'][1] + undistortMaps[1]['roi'][3])

            disparityToShow[:,:roiLeft] = 0
            disparityToShow[:,roiRight:] = 0
            disparityToShow[:roiTop,:] = 0
            disparityToShow[roiBottom:,:] = 0

            cv2.imshow('stereoBM_' + str(windowSize), disparityToShow)

    if len(images) == 2 and computeStereoFlow:
        # Compute the stereo
        allStereoPoints = []
        for windowSize in windowSizes:

            stereo_lk_params['winSize'] = windowSize

            points1, st, err = cv2.calcOpticalFlowPyrLK(images[0], images[1], points0, None, **stereo_lk_params)

            # Compute the minima-ness of each tracked point
            halfWindowSize = [windowSize[0] // 2, windowSize[0] // 2]
            points0Minimaness = []
            points1Minimaness = []
            for (point0, point1) in zip(points0, points1):
                minSlope0 = localMinimaness(images[0], halfWindowSize, point0[0][0], point0[0][1])
                minSlope1 = localMinimaness(images[1], halfWindowSize, point1[0][0], point1[0][1])

                points0Minimaness.append(minSlope0)
                points1Minimaness.append(minSlope1)

            # Select good points
            goodPoints0_tmp = points0[st==1]
            goodPoints1_tmp = points1[st==1]

            goodPoints0 = []
            goodPoints1 = []
            for point0, point1 in zip(goodPoints0_tmp, goodPoints1_tmp):
                absAngle = abs(math.atan2(point0[1]-point1[1], point0[0]-point1[0]))
                if absAngle <= stereo_lk_maxAngle:
                    goodPoints0.append(point0.tolist())
                    goodPoints1.append(point1.tolist())

            allStereoPoints.append({'points1':points1,'st':st,'err':err,'goodPoints0':goodPoints0,'goodPoints1':goodPoints1,'images':images, 'windowSize':windowSize, 'points0Minimaness':points0Minimaness, 'points1Minimaness':points1Minimaness})

        # Draw the stereo matches
        for stereoPoints in allStereoPoints:
            validPixROI1, validPixROI2

            showBadMatches = True
            if showBadMatches:
                toShowPoints0 = [x[0] for x in points0.tolist()]
                toShowPoints1 = [x[0] for x in stereoPoints['points1'].tolist()]
            else:
                toShowPoints0 = stereoPoints['goodPoints0']
                toShowPoints1 = stereoPoints['goodPoints1']

            # Remove points that are totally out of bounds
            extraLeftBorderPixels = 0

            left0 = validPixROI1[0] + extraLeftBorderPixels
            right0 = validPixROI1[0] + validPixROI1[2]
            top0 = validPixROI1[1]
            bottom0 = validPixROI1[1] + validPixROI1[3]

            left1 = validPixROI2[0] + extraLeftBorderPixels
            right1 = validPixROI2[0] + validPixROI2[2]
            top1 = validPixROI2[1]
            bottom1 = validPixROI2[1] + validPixROI2[3]

            validInds = []
            for index, (point0, point1) in enumerate(zip(toShowPoints0, toShowPoints1)):
                #pdb.set_trace()
                if bottom0 > point0[1] > top0 and right0 > point0[0] > left0 and \
                   bottom1 > point1[1] > top1 and right1 > point1[0] > left1:
                       validInds.append(index)

            toShowPoints0 = array(toShowPoints0)[validInds].astype('int32')
            toShowPoints1 = array(toShowPoints1)[validInds].astype('int32')

            keypointImage = drawKeypointMatches(stereoPoints['images'][0], toShowPoints0, toShowPoints1, stereoDisparityColors128)

            cv2.imshow('stereoLK_' + str(stereoPoints['windowSize']), keypointImage)

        # Compute the flow + stereo for every valid point
        if False:
            for flowPoints, stereoPoints in zip(allFlowPoints[0], allStereoPoints):

                validIndexes = nonzero((flowPoints['st']==1) & (stereoPoints['st']==1))[0].tolist()

                validIndexesStereo = []
                for index in validIndexes:
                    point0 = points0[index].tolist()[0]
                    point1 = stereoPoints['points1'][index].tolist()[0]
                    #pdb.set_trace()
                    absAngle = abs(math.atan2(point0[1]-point1[1], point0[0]-point1[0]))
                    if absAngle <= stereo_lk_maxAngle:
                        validIndexesStereo.append(index)

                validIndexes = validIndexesStereo

                goodPoints0 = points0[validIndexes]
                goodPointsFlow = flowPoints['points1'][validIndexes]
                goodPointsStereo = stereoPoints['points1'][validIndexes]

                flowStereoImage = drawStereoFlowKeypointMatches(flowPoints['curImage'], goodPoints0, goodPointsFlow, goodPointsStereo, stereoDisparityColors128)

                cv2.imshow('flowStereo' + str(stereoPoints['windowSize']), flowStereoImage)

    keypress = cv2.waitKey(waitKeyTime)
    if keypress & 0xFF == ord('q'):
        print('Breaking')
        break
    elif keypress & 0xFF == ord('c'):
        saveStereo(imagesRaw[0], imagesRaw[1], images[0], images[1], captureBaseFilename)
    elif keypress & 0xFF == ord('a'):
        print('Auto-calibrating stereo')
        goodPoints0 = allStereoPoints[0]['goodPoints0']
        goodPoints1 = allStereoPoints[0]['goodPoints1']

        p0Array = zeros((len(goodPoints0),2))
        p1Array = zeros((len(goodPoints0),2))

        # Only change based on Y difference
        for i in range(0,len(goodPoints0)):
            p0Array[i,:] = [goodPoints0[i][0], goodPoints0[i][1]]
            p1Array[i,:] = [goodPoints0[i][0], goodPoints1[i][1]]

        newRightImageWarpHomography, inliers = cv2.findHomography( p1Array, p0Array, cv2.RANSAC, ransacReprojThreshold = 5)
        rightImageWarpHomography = rightImageWarpHomography*matrix(newRightImageWarpHomography)

        stereo_lk_maxAngle = pi/64

        #pdb.set_trace()

    # Now update the previous frame
    lastImages = images

for cap in videoCaptures:
    cap.release()

cv2.destroyAllWindows()
