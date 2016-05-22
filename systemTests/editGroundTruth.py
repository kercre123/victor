import sys
import json
import os
import os.path
import time
import pdb
import random
from math import *

import ankic # If this fails, be sure to run products-cozmo/python/cython/make.sh

import cv2 # If this fails, compile opencv, and set your dylib directory to the compiled dynamic libraries

from PySide import QtCore, QtGui # If this fails, install QT Open Source 4.8.6 and easy_install PySide

import numpy as np

from editGroundTruthWindow import Ui_MainWindow

g_toShowSize = [480, 640]

global g_imageWindow
global g_mainWindow

if os.name == 'nt':
    g_startJsonDirectory = 'c:/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/'
else:
    g_startJsonDirectory = os.path.expanduser('~/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/')

g_startJsonFilename = 'tracking_00000_all.json'
g_data = None
g_imageWindow = None
g_mainWindow = None
g_curTestIndex = 0
g_maxTestIndex = 0
g_curPoseIndex = 0
g_maxPoseIndex = 0
g_curMarkerIndex = 0
g_maxMarkerIndex = 0
g_pointsType = 'template'
g_toShowImage = None
g_rawImage = None

cornerNamesX = ['x_imgUpperLeft', 'x_imgUpperRight', 'x_imgLowerRight', 'x_imgLowerLeft']
cornerNamesY = ['y_imgUpperLeft', 'y_imgUpperRight', 'y_imgLowerRight', 'y_imgLowerLeft']
cornerNames = cornerNamesX + cornerNamesY

def sanitizeJsonTest(jsonData):
    if 'Poses' not in jsonData:
        assert(false) # Can't correct for this

    if isinstance(jsonData['Poses'], dict):
        jsonData['Poses'] = [jsonData['Poses']]

    if 'Blocks' in jsonData:
        if isinstance(jsonData['Blocks'], dict):
            jsonData['Blocks'] = [jsonData['Blocks']]

        for i in range(0,len(jsonData['Blocks'])):
            if 'templateWidth_mm' not in jsonData['Blocks'][i]:
                jsonData['Blocks'][i]['templateWidth_mm'] = 0

    if 'NumMarkersWithContext' not in jsonData:
        jsonData['NumMarkersWithContext'] = 0

    if 'NumPartialMarkersWithContext' not in jsonData:
        jsonData['NumPartialMarkersWithContext'] = 0

    for iPose in range(0,len(jsonData['Poses'])):
        if 'ImageFile' not in jsonData['Poses'][iPose]:
            assert(false) # Can't correct for this

        if 'VisionMarkers' not in jsonData['Poses'][iPose]:
            jsonData['Poses'][iPose]['VisionMarkers'] = []

        if 'Scene' not in jsonData['Poses'][iPose]:
            jsonData['Poses'][iPose]['Scene'] = {}
            jsonData['Poses'][iPose]['Scene']['Distance'] = -1
            jsonData['Poses'][iPose]['Scene']['Angle'] = -1
            jsonData['Poses'][iPose]['Scene']['CameraExposure'] = -1
            jsonData['Poses'][iPose]['Scene']['Light'] = -1

        if 'NumMarkers' not in jsonData['Poses'][iPose]:
            jsonData['Poses'][iPose]['NumMarkers'] = 0

        if 'NumMarkersWithContext' in jsonData['Poses'][iPose]:
            jsonData['Poses'][iPose].pop('NumMarkersWithContext')

        if 'NumPartialMarkersWithContext' in jsonData['Poses'][iPose]:
            jsonData['Poses'][iPose].pop('NumPartialMarkersWithContext')

        if 'RobotPose' not in jsonData['Poses'][iPose]:
            jsonData['Poses'][iPose]['RobotPose'] = {}
            jsonData['Poses'][iPose]['RobotPose']['Angle'] = 0
            jsonData['Poses'][iPose]['RobotPose']['Axis'] = [1,0,0]
            jsonData['Poses'][iPose]['RobotPose']['HeadAngle'] = 0
            jsonData['Poses'][iPose]['RobotPose']['Translation'] = [0,0,0]

        if isinstance(jsonData['Poses'][iPose]['VisionMarkers'], dict):
            jsonData['Poses'][iPose]['VisionMarkers'] = [jsonData['Poses'][iPose]['VisionMarkers']]

        maxMarkerIndex = len(jsonData['Poses'][iPose]['VisionMarkers'])

        # if len(jsonData['Poses'][iPose]['VisionMarkers']) < maxMarkerIndex:
        #     jsonData['Poses'][iPose]['VisionMarkers'][end+1] = []

        for iMarker in range(0, maxMarkerIndex):
            if 'Name' not in jsonData['Poses'][iPose]['VisionMarkers'][iMarker]:
                jsonData['Poses'][iPose]['VisionMarkers'][iMarker]['Name'] = 'MessageVisionMarker'

            if 'markerType' not in jsonData['Poses'][iPose]['VisionMarkers'][iMarker]:
                jsonData['Poses'][iPose]['VisionMarkers'][iMarker]['markerType'] = 'MARKER_UNKNOWN'

            if 'timestamp' not in jsonData['Poses'][iPose]['VisionMarkers'][iMarker]:
                jsonData['Poses'][iPose]['VisionMarkers'][iMarker]['timestamp'] = 0

        if isinstance(jsonData['Poses'][iPose]['RobotPose'], dict):
            jsonData['Poses'][iPose]['RobotPose'] = [jsonData['Poses'][iPose]['RobotPose']]

    return jsonData

def saveTestFile():
    global g_data

    allTestFilenames, curTestIndex, maxTestIndex = getTestNamesFromDirectory()

    testFilename = allTestFilenames[curTestIndex]

    testFilename.replace('\\', '/')
    testFilename.replace('//', '/')

    jsonData = sanitizeJsonTest(g_data['jsonData'])

    jsonDataString = json.dumps(jsonData, indent=2)

    # TODO: only change the leading spaces
    jsonDataString = jsonDataString.replace('  ', '\t')

    f = open(testFilename, 'w')
    #jsonData = json.dump(jsonData, f, indent=2)
    f.write(jsonDataString)
    f.close()

def loadTestFile():
    global g_mainWindow
    global g_data
    global g_curPoseIndex
    global g_curMarkerIndex

    # TODO: cache in more efficient format?

    allTestFilenames, curTestIndex, maxTestIndex = getTestNamesFromDirectory()

    if curTestIndex >= len(allTestFilenames):
        return

    testFilename = allTestFilenames[curTestIndex]

    testFilename.replace('\\', '/')
    testFilename.replace('//', '/')

    print('Loading json ' + testFilename)

    f = open(testFilename, 'r')
    jsonData = json.load(f)
    f.close()

    jsonData = sanitizeJsonTest(jsonData)

    modificationTime_json = os.path.getmtime(testFilename)

    testPath = testFilename[:testFilename.rfind('/')]
    testFilename = testFilename[testFilename.rfind('/'):]

    g_data = {
        'jsonData':jsonData,
        'allTestFilenames':allTestFilenames,
        'testFilename':testFilename,
        'testPath':testPath,
        'testFileModificationTime':modificationTime_json}

    g_curPoseIndex = 0
    g_curMarkerIndex = 0

    poseChanged(True)

def fixBounds():
    # TODO: implement
    pass

def getMaxMarkerIndex(poseIndex):
    global g_data

    numMarkers = len(g_data['jsonData']['Poses'][poseIndex]['VisionMarkers'])

    if numMarkers == 0:
        return 0

    [corners, whichCorners] = getFiducialCorners(poseIndex, numMarkers-1)

    if len(corners) == 4:
        maxIndex = numMarkers
    else:
        maxIndex = numMarkers - 1

    #pdb.set_trace()

    #print('maxIndex = ' + str(maxIndex))

    return maxIndex

def fixBounds():
    global g_mainWindow
    global g_data
    global g_curPoseIndex
    global g_curMarkerIndex
    global g_maxMarkerIndex

    boundsFixed = False

    g_data['jsonData'] = sanitizeJsonTest(g_data['jsonData'])

    curPoseIndexOriginal = g_curPoseIndex

    g_curPoseIndex   = max(0, min(len(g_data['jsonData']['Poses'])-1, g_curPoseIndex))
    g_curMarkerIndex = max(0, min(getMaxMarkerIndex(g_curPoseIndex),  g_curMarkerIndex))

    g_mainWindow.ui.setMaxMarkerIndex(getMaxMarkerIndex(g_curPoseIndex))

    if curPoseIndexOriginal != g_curPoseIndex:
        boundsFixed = True

    return boundsFixed

def getTestNamesFromDirectory():
    global g_curTestIndex

    if g_mainWindow is None:
        return None, 0, 0

    testDirectory = g_mainWindow.ui.testJsonFilename1.text()
    testDirectory = testDirectory.replace('\\', '/')
    testDirectory = testDirectory.replace('//', '/')

    testFilename  = g_mainWindow.ui.testJsonFilename2.text()
    testFilename = testFilename.replace('\\', '/')
    testFilename = testFilename.replace('//', '/')

    if (len(testFilename) > 5) and (testFilename[(-5):] != '.json'):
        testFilename = [testFilename, '.json'];

    files = []
    
    try:
        for f in os.listdir(testDirectory):
            if os.path.isfile(os.path.join(testDirectory,f)) and (len(f) > 5) and (f[(-5):] == '.json'):
                files.append(os.path.join(testDirectory,f))
    except:
        print('Could not load from directory ' + testDirectory)
        
    maxTestIndex = len(files) - 1

    return files, g_curTestIndex, maxTestIndex

def poseChanged(resetZoom):
    global g_mainWindow
    global g_imageWindow
    global g_data
    global g_curPoseIndex
    global g_curMarkerIndex
    global g_rawImage
    global g_toShowImage

    fixBounds()

    g_mainWindow.ui.setMaxTestIndex(len(g_data['allTestFilenames']) - 1)
    g_mainWindow.ui.testJsonFilename1.setText(g_data['testPath'])
    g_mainWindow.ui.testJsonFilename2.setText(g_data['testFilename'])
    g_mainWindow.ui.setCurPoseIndex(g_curPoseIndex, False)
    g_mainWindow.ui.setMaxPoseIndex(len(g_data['jsonData']['Poses'])-1)
    g_mainWindow.ui.setCurMarkerIndex(g_curMarkerIndex, False)
    g_mainWindow.ui.setMaxMarkerIndex(getMaxMarkerIndex(g_curPoseIndex))

    if len(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers']) > g_curMarkerIndex:
        g_mainWindow.ui.markerType.setText(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['markerType'][7:])
    else:
        g_mainWindow.ui.markerType.setText('UNKNOWN')

    # TODO: add in all the updates

    #allTestNames, curTestIndex, maxTestIndex = getTestNamesFromDirectory()

    #if g_mainWindow is not None:
    #    g_mainWindow.ui.updateNumbers()

    curImageFilename = g_data['testPath'] + '/' + g_data['jsonData']['Poses'][g_curPoseIndex]['ImageFile']

    g_mainWindow.ui.pose_filename.setText(curImageFilename)

    #print('Load image file ' + curImageFilename)
    g_rawImage = QtGui.QPixmap(curImageFilename)
    g_toShowImage = QtGui.QPixmap(g_rawImage)

    g_imageWindow.setPixmap(g_toShowImage)

    g_imageWindow.setScaledContents(True)

    g_imageWindow.setSizePolicy( QtGui.QSizePolicy.Ignored, QtGui.QSizePolicy.Ignored )

def getFiducialCorners(poseIndex, markerIndex):
    global g_data

    numVisionMarkers = len(g_data['jsonData']['Poses'][poseIndex]['VisionMarkers'])

    if markerIndex >= numVisionMarkers:
        return ([], [0]*4)

    corners = []

    if markerIndex is None:
        whichCorners = []

        # If markerIndex is empty, return all corners for all complete quads
        for iMarker in range(0, numVisionMarkers):
            curMarkerData = g_data['jsonData']['Poses'][poseIndex]['VisionMarkers'][iMarker]

            if all((x in curMarkerData for x in cornerNames)) == True:
                newCornersX = [0] * 4
                newCornersY = [0] * 4
                newWhichCorners = [1] * 4

                for i in range(0,4):
                    newCornersX[i] = curMarkerData[cornerNamesX[i]]
                    newCornersY[i] = curMarkerData[cornerNamesY[i]]

                corners.append(zip(newCornersX,newCornersY))
                whichCorners.append(whichCorners)
    else: # if markerIndex is None
        whichCorners = [0] * 4

        curMarkerData = g_data['jsonData']['Poses'][poseIndex]['VisionMarkers'][markerIndex]

        for i in range(0,4):
            if (cornerNamesX[i] in curMarkerData) and (cornerNamesY[i] in curMarkerData):
                corners.append((curMarkerData[cornerNamesX[i]], curMarkerData[cornerNamesY[i]]))
                whichCorners[i] = 1

    #pdb.set_trace()

    return (corners, whichCorners)

def extractMarkers(image, returnInvalidMarkers):
    if len(image.shape) == 3:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    imageSize = image.shape
    useIntegralImageFiltering = True
    scaleImage_thresholdMultiplier = 1.0
    scaleImage_numPyramidLevels = 3
    component1d_minComponentWidth = 0
    component1d_maxSkipDistance = 0
    minSideLength = round(0.01*max(imageSize[0],imageSize[1]))
    maxSideLength = round(0.97*min(imageSize[0],imageSize[1]))
    component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength))
    component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength))
    component_sparseMultiplyThreshold = 1000.0
    component_solidMultiplyThreshold = 2.0
    component_minHollowRatio = 1.0
    quads_minQuadArea = 100 / 4
    quads_quadSymmetryThreshold = 2.0
    quads_minDistanceFromImageEdge = 2
    decode_minContrastRatio = 1.25
    quadRefinementIterations = 5
    numRefinementSamples = 100
    quadRefinementMaxCornerChange = 5.0
    quadRefinementMinCornerChange = 0.005
    returnInvalidMarkers = int(returnInvalidMarkers)

    quads, markerTypes, markerNames, markerValidity, binaryImage = ankic.detectFiducialMarkers(image, useIntegralImageFiltering, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers)

    #pdb.set_trace()

    return quads, markerTypes, markerNames, markerValidity, binaryImage

def detectClosestFiducialMarker(image, quad):
    quads, markerTypes, markerNames, markerValidity, binaryImage = extractMarkers(image, 1)

    #print('quads = ' + str(quads))

    if binaryImage != []:
        cv2.imshow("binaryImage", binaryImage*255)

        binaryImage2 = cv2.cvtColor(binaryImage*255, cv2.COLOR_GRAY2RGB)

        for toDrawQuad in quads:
            cornerIndexes = [0,2,3,1,0]
            for i in range(0,4):
                iCur = cornerIndexes[i]
                iNext = cornerIndexes[i+1]
                p0 = tuple([int(x) for x in toDrawQuad[iCur]])
                p1 = tuple([int(x) for x in toDrawQuad[iNext]])
                cv2.line(binaryImage2, p0, p1, (0,255,0), 3)

        cv2.imshow("binaryImage2", binaryImage2)

        cv2.waitKey(1);

    if len(quads) == 0:
        return quad, float('Inf')

    # For each candidate quad corner, find the distance to the closest query quad corner

    closestQuadInd = -1
    closestQuadDist = float('Inf')

    for iQuad in range(0,len(quads)):
        curQuad = quads[iQuad]

        sumCornerDists = 0

        # For quad iQuad, what is the sum of the best distances between all four corners?
        for iCorner in range(0,4):
            closestCornerInd = -1
            closestCornerDist = float('Inf')

            for jCorner in range(0,4):
                curCornerDist = sqrt(pow(curQuad[jCorner][0] - quad[iCorner][0], 2.0) + pow(curQuad[jCorner][1] - quad[iCorner][1], 2.0))

                if curCornerDist < closestCornerDist:
                    closestCornerDist = curCornerDist
                    closestCornerInd = jCorner

            sumCornerDists += closestCornerDist

        if sumCornerDists < closestQuadDist:
            closestQuadDist = sumCornerDists
            closestQuadInd = iQuad

    reorderedQuad = [quads[closestQuadInd][0], quads[closestQuadInd][2], quads[closestQuadInd][3], quads[closestQuadInd][1]]

    # Rotate marker to match the original
    
    bestRotatedQuad = []
    bestRotatedDistance = float('Inf')
    
    reorderedQuadRotated = reorderedQuad[:]
    for iRotation in range(0,4):
        totalDist = 0
        for iCorner in range(0,4):
            totalDist += sqrt(pow(reorderedQuadRotated[iCorner][0] - quad[iCorner][0], 2.0) + pow(reorderedQuadRotated[iCorner][1] - quad[iCorner][1], 2.0))
            
        if totalDist < bestRotatedDistance:
            bestRotatedDistance = totalDist
            bestRotatedQuad = reorderedQuadRotated[:]
        
        reorderedQuadRotated = [reorderedQuadRotated[1], reorderedQuadRotated[2], reorderedQuadRotated[3], reorderedQuadRotated[0]]

    return bestRotatedQuad, bestRotatedDistance

def trackQuadrilateral(startImage, startQuad, endImage, drawDebug=False):
    numPointsPerDimension = 10

    xRange = (min((x for x,y in startQuad)), max((x for x,y in startQuad)))
    yRange = (min((y for x,y in startQuad)), max((y for x,y in startQuad)))

    #quadShortWidth = min([xRange[0], yRange[0]])
    quadLongWidth = max([abs(xRange[1]-xRange[0]), abs(yRange[1]-yRange[0])])

    windowSize = max(3, int(round(quadLongWidth * 2 / numPointsPerDimension)))

    #print('startQuad = ' + str(startQuad))
    #print('xRange = ' + str(xRange))
    #print('yRange = ' + str(yRange))
    #print('windowSize = ' + str(windowSize))

    flow_lk_params = dict(winSize  = (windowSize,windowSize), maxLevel = 4, criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03), minEigThreshold = 5e-3)

    points0 = []
    for y in np.linspace(yRange[0], yRange[1], numPointsPerDimension):
        for x in np.linspace(xRange[0], xRange[1], numPointsPerDimension):
            points0.append((x,y))

    points0 = np.array(points0).reshape(-1,1,2).astype('float32')

    points1, st, err = cv2.calcOpticalFlowPyrLK(startImage, endImage, points0, None, **flow_lk_params)

    goodPoints0 = points0[st==1]
    goodPoints1 = points1[st==1]

    if drawDebug:
        startImageToShow = cv2.cvtColor(startImage*255, cv2.COLOR_GRAY2RGB)

        for p0, p1 in zip(goodPoints0, goodPoints1):
            cv2.line(startImageToShow,
                     tuple(p0.round().astype('int32').tolist()),
                     tuple(p1.round().astype('int32').tolist()),
                     (0,255,0),
                     1)

        cv2.imshow("startImageToShow", startImageToShow)

    if goodPoints1.shape[0] < 4:
        return [[0,0], [0,0], [0,0], [0,0]]

    homography, inliers = cv2.findHomography( goodPoints0, goodPoints1, cv2.RANSAC, ransacReprojThreshold = 1)

    startQuadArray = np.zeros((3,4))

    startQuadArray[2,:] = 1

    for i in range(0,4):
        startQuadArray[0,:] = [x for x,y in startQuad]
        startQuadArray[1,:] = [y for x,y in startQuad]

    warpedQuadArray = np.matrix(homography) * np.matrix(startQuadArray)

    warpedQuad = []
    for i in range(0,4):
        warpedQuad.append((warpedQuadArray[0,i] / warpedQuadArray[2,i], warpedQuadArray[1,i] / warpedQuadArray[2,i]))

    return warpedQuad

def trackTemplate():
    global g_data
    global g_mainWindow
    global g_imageWindow
    global g_curPoseIndex
    global g_maxPoseIndex
    global g_curMarkerIndex

    if len(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers']) <= g_curMarkerIndex:
        print('No marker labeled')
        return

    if isinstance(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'], dict):
        g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'] = [ g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'] ]

    [corners, whichCorners] = getFiducialCorners(g_curPoseIndex, g_curMarkerIndex)

    if len(corners) != 4:
        print("The current marker doesn't have four corners")
        return

    imageFilename = g_data['testPath'] + '/' + g_data['jsonData']['Poses'][g_curPoseIndex]['ImageFile']
    lastImage = cv2.imread(imageFilename, 0)

    detectedQuad, detectedQuadDistance = detectClosestFiducialMarker(lastImage, corners)

    if detectedQuadDistance < (4*5):
        lastQuad = detectedQuad
    else:
        lastQuad = corners

    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['x_imgUpperLeft']  = lastQuad[0][0]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['y_imgUpperLeft']  = lastQuad[0][1]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['x_imgUpperRight'] = lastQuad[1][0]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['y_imgUpperRight'] = lastQuad[1][1]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['x_imgLowerRight'] = lastQuad[2][0]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['y_imgLowerRight'] = lastQuad[2][1]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['x_imgLowerLeft']  = lastQuad[3][0]
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['y_imgLowerLeft']  = lastQuad[3][1]

    if 'markerType' not in g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]:
        g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['markerType'] = 'MARKER_UNKNOWN'

    markerType = g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['markerType']

    startPoseIndex = g_curPoseIndex
    startMarkerIndex = g_curMarkerIndex

    for iPose in range(startPoseIndex+1, g_maxPoseIndex+1):
        imageFilename = g_data['testPath'] + '/' + g_data['jsonData']['Poses'][iPose]['ImageFile']

        curImage = cv2.imread(imageFilename, 0)

        # Track points between the previous image and the current image
        curQuad = trackQuadrilateral(lastImage, lastQuad, curImage, True)

        # Match this quad to the best detected quad
        detectedQuad, detectedQuadDistance = detectClosestFiducialMarker(curImage, curQuad)

        #print('detectedQuadDistance = ' + str(detectedQuadDistance))

        suffix = ''
        if detectedQuadDistance < (4*5):
            curQuad = detectedQuad
            #suffix = '_Good' # TODO: comment out
        else:
            curQuad = curQuad
            #suffix = '_Bad' # TODO: comment out

            #pdb.set_trace()
        
        cv2.waitKey(1)

        # TODO: will this work when the g_curMarkerIndex is not zero
        while len(g_data['jsonData']['Poses'][iPose]['VisionMarkers']) <= g_curMarkerIndex:
            g_data['jsonData']['Poses'][iPose]['VisionMarkers'].append({})

        if not isinstance(g_data['jsonData']['Poses'][iPose]['VisionMarkers'], dict):
            g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex] = {}

        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['x_imgUpperLeft']  = curQuad[0][0]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['y_imgUpperLeft']  = curQuad[0][1]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['x_imgUpperRight'] = curQuad[1][0]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['y_imgUpperRight'] = curQuad[1][1]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['x_imgLowerRight'] = curQuad[2][0]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['y_imgLowerRight'] = curQuad[2][1]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['x_imgLowerLeft']  = curQuad[3][0]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['y_imgLowerLeft']  = curQuad[3][1]
        g_data['jsonData']['Poses'][iPose]['VisionMarkers'][g_curMarkerIndex]['markerType'] = markerType + suffix

        lastImage = curImage
        lastQuad = curQuad

        #pdb.set_trace()

    saveTestFile()
    poseChanged(False)

def clearMarkers():
    global g_data
    global g_curPoseIndex
    global g_curMarkerIndex
    
    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex] = {}
    
    saveTestFile()
    poseChanged(False)

class ConnectedMainWindow(Ui_MainWindow):
    def __init__(self):
        super(ConnectedMainWindow, self).__init__()

        #loadTestFile(g_startJsonDirectory + '/' + g_startJsonFilename)

    def incrementCurTestIndex(self, increment, updatePose=False):
        global g_curTestIndex
        global g_maxTestIndex
        global g_startJsonDirectory
        global g_startJsonFilename

        g_curTestIndex = max(0, min(g_maxTestIndex, g_curTestIndex+increment))

        self.test_current.setText(str(g_curTestIndex))

        loadTestFile()

        if updatePose:
            poseChanged(True)

    def setCurTestIndex(self, value, updatePose=False):
        global g_curTestIndex
        global g_maxTestIndex
        global g_startJsonDirectory
        global g_startJsonFilename

        g_curTestIndex = max(0, min(g_maxTestIndex, value))

        self.test_current.setText(str(g_curTestIndex))

        loadTestFile()

        #files, curTestIndex, maxTestIndex = getTestNamesFromDirectory()

        #self.setMaxTestIndex(len(files))

        #pdb.set_trace()

        #loadTestFile(files[curTestIndex])
        #loadTestFile(testDirectory + '/' + testFilename)
        #loadTestFile('/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/tracking_00000_all.json')

        #pdb.set_trace()

        if updatePose:
            poseChanged(True)

    def setMaxTestIndex(self, value):
        global g_maxTestIndex;
        g_maxTestIndex = value
        self.test_max.setText(str(value))

    def setCurPoseIndex(self, value, updatePose=False):
        global g_curPoseIndex
        global g_maxPoseIndex

        g_curPoseIndex = max(0, min(g_maxPoseIndex, value))

        self.pose_current.setText(str(g_curPoseIndex))

        if updatePose:
            poseChanged(True)

    def incrementCurPoseIndex(self, increment, updatePose=False):
        global g_curPoseIndex
        global g_maxPoseIndex

        g_curPoseIndex = max(0, min(g_maxPoseIndex, g_curPoseIndex+increment))

        self.pose_current.setText(str(g_curPoseIndex))

        if updatePose:
            poseChanged(True)

    def setMaxPoseIndex(self, value):
        global g_maxPoseIndex;
        g_maxPoseIndex = value
        self.pose_max.setText(str(value))

    def setCurMarkerIndex(self, value, updatePose=False):
        global g_curMarkerIndex
        global g_curPoseIndex
        global g_maxMarkerIndex

        g_curMarkerIndex = max(0, min(g_maxMarkerIndex, value))

        self.marker_current.setText(str(g_curMarkerIndex))

        if updatePose:
            poseChanged(False)

    def incrementCurMarkerIndex(self, increment, updatePose=False):
        global g_curMarkerIndex
        global g_curPoseIndex
        global g_maxMarkerIndex

        g_curMarkerIndex = max(0, min(g_maxMarkerIndex, g_curMarkerIndex+increment))

        self.marker_current.setText(str(g_curMarkerIndex))

        if updatePose:
            poseChanged(False)

    def setMaxMarkerIndex(self, value):
        global g_maxMarkerIndex;
        g_maxMarkerIndex = value
        self.marker_max.setText(str(value))

    def updateMarkerName(self):
        global g_data
        global g_curPoseIndex
        global g_curMarkerIndex

        markerName = self.markerType.text().upper()

        if len(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers']) > g_curMarkerIndex:
            g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]['markerType'] = 'MARKER_' + markerName
            saveTestFile()

        poseChanged(False)

    def updateNumbers(self):
        allTestNames, curTestIndex, maxTestIndex = getTestNamesFromDirectory()
        
        if allTestNames is None or allTestNames == []:
            return

        self.setCurTestIndex(g_curTestIndex, False)
        self.setCurPoseIndex(g_curPoseIndex, False)
        self.setCurMarkerIndex(g_curMarkerIndex, False)

        self.setMaxTestIndex(maxTestIndex)
        self.setMaxPoseIndex(len(g_data['jsonData']['Poses']))
        self.setMaxMarkerIndex(getMaxMarkerIndex(g_curPoseIndex))

    def setupUi(self, mainWindow):
        super(ConnectedMainWindow, self).setupUi(mainWindow)

        self.testJsonFilename1.setText(g_startJsonDirectory)
        self.testJsonFilename2.setText(g_startJsonFilename)

        self.test_current.setText(str(g_curTestIndex))
        self.test_max.setText(str(g_maxTestIndex))

        self.pose_current.setText(str(g_curPoseIndex))
        self.pose_max.setText(str(g_maxPoseIndex))

        self.marker_current.setText(str(g_curMarkerIndex))
        self.marker_max.setText(str(g_maxMarkerIndex))

        self.testJsonFilename1.returnPressed.connect(loadTestFile)
        self.testJsonFilename2.returnPressed.connect(loadTestFile)
        self.markerType.returnPressed.connect(self.updateMarkerName)

        self.test_current.returnPressed.connect(lambda: self.setCurTestIndex(int(self.test_current.text())))

        self.trackMarkerAcrossPoses.clicked.connect(trackTemplate)
        self.clearAllMarkers.clicked.connect(clearMarkers)

        self.test_previous1.clicked.connect(lambda: self.incrementCurTestIndex(-1, True))
        self.test_previous2.clicked.connect(lambda: self.incrementCurTestIndex(-10, True))
        self.test_previous3.clicked.connect(lambda: self.incrementCurTestIndex(-100, True))
        self.test_next1.clicked.connect(lambda: self.incrementCurTestIndex(1, True))
        self.test_next2.clicked.connect(lambda: self.incrementCurTestIndex(10, True))
        self.test_next3.clicked.connect(lambda: self.incrementCurTestIndex(100, True))

        self.pose_previous1.clicked.connect(lambda: self.incrementCurPoseIndex(-1, True))
        self.pose_previous2.clicked.connect(lambda: self.incrementCurPoseIndex(-10, True))
        self.pose_previous3.clicked.connect(lambda: self.incrementCurPoseIndex(-100, True))
        self.pose_next1.clicked.connect(lambda: self.incrementCurPoseIndex(1, True))
        self.pose_next2.clicked.connect(lambda: self.incrementCurPoseIndex(10, True))
        self.pose_next3.clicked.connect(lambda: self.incrementCurPoseIndex(100, True))

        self.marker_previous1.clicked.connect(lambda: self.incrementCurMarkerIndex(-1, True))
        self.marker_previous2.clicked.connect(lambda: self.incrementCurMarkerIndex(-10, True))
        self.marker_previous3.clicked.connect(lambda: self.incrementCurMarkerIndex(-100, True))
        self.marker_next1.clicked.connect(lambda: self.incrementCurMarkerIndex(1, True))
        self.marker_next2.clicked.connect(lambda: self.incrementCurMarkerIndex(10, True))
        self.marker_next3.clicked.connect(lambda: self.incrementCurMarkerIndex(100, True))

class Ui_ImageWindow(QtGui.QLabel):
    def __init__(self, parent=None):
        super(Ui_ImageWindow, self).__init__(parent)

        self.resize(g_toShowSize[1], g_toShowSize[0])

    def closeEvent(self, event):
        sys.exit(0)
        #event.accept()

    def paintEvent(self, event):
        super(Ui_ImageWindow, self).paintEvent(event)

        if g_imageWindow.pixmap() is None:
            return

        # TODO: Draw the quads

        # painter = QtGui.QPainter(self)
        # painter.setPen(QtGui.QColor(128, 0, 128))
        # painter.drawLine(int(random.random()*400), int(random.random()*400), int(random.random()*400), int(random.random()*400))

        #g_imageWindow.setPixmap(g_toShowImage)
        #print('Toasted')

        #pdb.set_trace()

        realImageSize = [float(g_imageWindow.pixmap().height()), float(g_imageWindow.pixmap().width())]
        displayImageSize = [float(self.height()), float(self.width())]

        xScaleInv = displayImageSize[1] / realImageSize[1]
        yScaleInv = displayImageSize[0] / realImageSize[0]

        if g_pointsType == 'template':
            #padAmount = floor(size(imageResized)/2)
            #imageResized = padarray(imageResized, padAmount)
            # TODO: add padding
            padAmount = [0,0]
        else:
            padAmount = [0,0]

        if (g_pointsType == 'template') or (g_pointsType == 'fiducialMarker'):
            for iMarker in range(0,len(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'])):
                painter = QtGui.QPainter(self)

                if iMarker == g_curMarkerIndex:
                    lineColor = QtGui.QColor(0, 255, 0)
                else:
                    lineColor = QtGui.QColor(255, 255, 0)

                [corners, whichCorners] = getFiducialCorners(g_curPoseIndex, iMarker)

                #corners = [(x+padAmount[1]-0.5,y+padAmount[0]-0.5) for (x,y) in corners]

                # Plot the corners (and if 4 corners exist, the quad as well)
                if len(corners) == 4:
                    painter.setPen(QtGui.QPen(lineColor, 1, QtCore.Qt.SolidLine, QtCore.Qt.SquareCap, QtCore.Qt.BevelJoin))
                    for iCur in range(1,4):
                        iNext = (iCur+1) % 4
                        painter.drawLine(xScaleInv*corners[iCur][0], yScaleInv*corners[iCur][1], xScaleInv*corners[iNext][0], yScaleInv*corners[iNext][1])

                    painter.setPen(QtGui.QPen(lineColor, 3, QtCore.Qt.SolidLine, QtCore.Qt.SquareCap, QtCore.Qt.BevelJoin))
                    painter.drawLine(xScaleInv*corners[0][0], yScaleInv*corners[0][1], xScaleInv*corners[1][0], yScaleInv*corners[1][1])

                    markerName = g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][iMarker]['markerType'][7:]
                    painter.drawText(\
                        (xScaleInv*corners[0][0]+xScaleInv*corners[3][0])/2 + 5,\
                        (yScaleInv*corners[0][1]+yScaleInv*corners[3][1])/2,\
                        markerName)

                painter.setPen(QtGui.QPen(lineColor, 1, QtCore.Qt.SolidLine, QtCore.Qt.SquareCap, QtCore.Qt.BevelJoin))
                for i in range(0,len(corners)):
                    center = QtCore.QPoint(xScaleInv*corners[i][0], yScaleInv*corners[i][1])
                    #pdb.set_trace()
                    painter.drawEllipse(center, 5, 5)
                    #scatterHandle = scatter(xScaleInv*(cornersX(i)+0.5), yScaleInv*(cornersY(i)+0.5), [linePlotType,'o'])

                painter.end()

    def mousePressEvent(self, event):
        global g_curMarkerIndex

        #print('clicked ' + str(event))

        buttonModifiers = event.modifiers()

        [corners, whichCorners] = getFiducialCorners(g_curPoseIndex, g_curMarkerIndex)

        realImageSize = [float(g_imageWindow.pixmap().height()), float(g_imageWindow.pixmap().width())]
        displayImageSize = [float(self.height()), float(self.width())]

        xScale = realImageSize[1] / displayImageSize[1]
        yScale = realImageSize[0] / displayImageSize[0]

        # TODO: add padding
        if g_pointsType == 'template':
            padAmount = [0, 0] # floor(size(image) / 2)
        elif g_pointsType == 'fiducialMarker':
            padAmount = [0, 0]

        newPoint = {
            'x':(event.x() - padAmount[1]) * xScale,
            'y':(event.y() - padAmount[0]) * yScale}

        changed = False

        if event.button() == QtCore.Qt.MouseButton.LeftButton: # left click
            if buttonModifiers == QtCore.Qt.ShiftModifier: # shift + left click
                [corners, whichCorners] = getFiducialCorners(g_curPoseIndex, None)

                minDist = float('Inf')
                minInd = -1

                if (g_pointsType=='template') or (g_pointsType=='fiducialMarker'):
                    for iMarker in range(0,len(corners)):
                        meanCornersX = sum([x for x,y in corners[iMarker]]) / len(corners[iMarker])
                        meanCornersY = sum([y for x,y in corners[iMarker]]) / len(corners[iMarker])

                        dist = sqrt(pow(meanCornersX - newPoint['x'], 2.0) + pow(meanCornersY - newPoint['y'], 2.0))

                        if dist < minDist:
                            minDist = dist
                            minInd = iMarker

                    if minInd != -1:
                        g_curMarkerIndex = minInd
                        #fixBounds()
                        poseChanged(False)

            else: # left click
                if (g_pointsType=='template') or (g_pointsType=='fiducialMarker'):
                    #pdb.set_trace()
                    if sum(whichCorners) < 4:
                        for i in range(0,4):
                            if whichCorners[i] == 0:
                                if len(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers']) <= g_curMarkerIndex:
                                    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'].append({})

                                g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex][cornerNamesX[i]] = newPoint['x']
                                g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex][cornerNamesY[i]] = newPoint['y']

                                break

                        changed = True
                    else:
                        print('Cannot add point, because only 4 fiduciual marker corners are allowed')
        elif event.button() == QtCore.Qt.MouseButton.RightButton: # right click
            minDist = float('Inf')
            minInd = -1

            if (g_pointsType == 'template') or (g_pointsType == 'fiducialMarker'):
                ci = 0
                for i in range(0,len(whichCorners)):
                    if whichCorners[i] == 0:
                        continue

                    ci += 1

                    dist = sqrt(pow(corners[ci-1][0] - newPoint['x'], 2) + pow(corners[ci-1][1] - newPoint['y'], 2))
                    if dist < minDist:
                        minDist = dist
                        minInd = i

                if (minInd != -1) and (minDist < (min(realImageSize[0], realImageSize[1])/50.0)):
                    newMarkerData = g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex]

                    newMarkerData.pop(cornerNamesX[minInd])
                    newMarkerData.pop(cornerNamesY[minInd])

                    g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex] = newMarkerData
                    changed = True

        # TODO: save thread
        if changed:
            saveTestFile()

        poseChanged(False)

class ControlMainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(ControlMainWindow, self).__init__(parent)
        self.ui = ConnectedMainWindow()
        self.ui.setupUi(self)

    def closeEvent(self, event):
        sys.exit(0)
        #event.accept()

if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)

    g_imageWindow = Ui_ImageWindow()
    g_imageWindow.show()

    g_mainWindow = ControlMainWindow()
    g_mainWindow.show()
    g_mainWindow.ui.updateNumbers()

    #loadTestFile(g_startJsonDirectory + '/' + g_startJsonFilename)

    sys.exit(app.exec_())



