import sys
import json
import os.path
import time
import pdb
import random
from math import *

from PySide import QtCore, QtGui

import numpy as np

from editGroundTruthWindow import Ui_MainWindow

g_toShowSize = [480, 640]

g_startJsonDirectory = '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/'
g_startJsonFilename = 'tracking_00000_all.json'
g_data = None
g_imageWindow = None
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

def loadJson(filename):
    print('Loading json ' + filename)

    f = open(filename, 'r')
    jsonData = json.load(f)
    f.close()

    jsonData = sanitizeJsonTest(jsonData)

    #print(jsonData)

    return jsonData

def loadTestFile(testFilename):
    global g_data
    global g_curPoseIndex
    global g_curMarkerIndex

    # TODO: cache in more efficient format?

    testFilename.replace('\\', '/')
    testFilename.replace('//', '/')

    jsonData = loadJson(testFilename)
    modificationTime_json = os.path.getmtime(testFilename)

    testPath = testFilename[:testFilename.rfind('/')]
    testFilename = testFilename[testFilename.rfind('/'):]

    g_data = {
        'jsonData':jsonData,
        'testFilename':testFilename,
        'testPath':testPath,
        'testFileModificationTime':modificationTime_json}

    g_curPoseIndex = 4
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
    
    [corners, whichCorners] = getFiducialCorners(poseIndex, numMarkers)
    
    if len(corners) == 4:
            maxIndex = numMarkers
    else:
            maxIndex = numMarkers - 1
    
    return maxIndex
        
def fixBounds():
    global g_data
    global g_curPoseIndex
    global g_curMarkerIndex
    
    boundsFixed = False
    
    g_data['jsonData'] = sanitizeJsonTest(g_data['jsonData'])
    
    curPoseIndexOriginal = g_curPoseIndex
    
    g_curPoseIndex     = max(0, min(len(g_data['jsonData']['Poses'])-1, g_curPoseIndex))
    g_curMarkerIndex = max(0, min(getMaxMarkerIndex(g_curPoseIndex),    g_curMarkerIndex))
        
    if curPoseIndexOriginal != g_curPoseIndex:
        boundsFixed = True

    return boundsFixed

def poseChanged(resetZoom):
    global g_imageWindow
    global g_data
    global g_curPoseIndex
    global g_rawImage
    global g_toShowImage

    fixBounds()
    
    # TODO: add in all the updates
    
    curImageFilename = g_data['testPath'] + g_data['jsonData']['Poses'][g_curPoseIndex]['ImageFile']

    g_rawImage = QtGui.QPixmap(curImageFilename)
    g_toShowImage = QtGui.QPixmap(g_rawImage)

    g_imageWindow.setPixmap(g_toShowImage)

    g_imageWindow.setScaledContents( True )

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

                corners.append((newCornersX,newCornersY))
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

class ConnectedMainWindow(Ui_MainWindow):
    def __init__(self):
        super(ConnectedMainWindow, self).__init__()

        loadTestFile(g_startJsonDirectory + '/' + g_startJsonFilename)

    def setTestIndex(self, value):
        global g_curTestIndex
        global g_maxTestIndex

        print('setTestIndex pressed', value)

        g_curTestIndex = max(0, min(g_maxTestIndex, value))

        self.test_current.setText(str(g_curTestIndex))

    def setPoseIndex(self, value):
        global g_curPoseIndex
        global g_maxPoseIndex

        print('setPoseIndex pressed', value)

        g_curPoseIndex = max(0, min(g_maxPoseIndex, value))

        self.pose_current.setText(str(g_curPoseIndex))

    def setMarkerIndex(self, value):
        global g_curMarkerIndex
        global g_maxMarkerIndex

        print('setMarkerIndex pressed', value)

        g_curMarkerIndex = max(0, min(g_maxMarkerIndex, value))

        self.marker_current.setText(str(g_curMarkerIndex))

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

        self.testJsonFilename1.returnPressed.connect(lambda: loadTestFile(self.testJsonFilename1.text() + '/' + self.testJsonFilename2.text()))
        self.testJsonFilename2.returnPressed.connect(lambda: loadTestFile(self.testJsonFilename1.text() + '/' + self.testJsonFilename2.text()))

        self.test_current.returnPressed.connect(lambda: self.setTestIndex(int(self.test_current.text())))

        self.test_previous1.clicked.connect(lambda: self.setTestIndex(g_curTestIndex-1))
        self.test_previous2.clicked.connect(lambda: self.setTestIndex(g_curTestIndex-10))
        self.test_previous3.clicked.connect(lambda: self.setTestIndex(g_curTestIndex-100))
        self.test_next1.clicked.connect(lambda: self.setTestIndex(g_curTestIndex+1))
        self.test_next2.clicked.connect(lambda: self.setTestIndex(g_curTestIndex+10))
        self.test_next3.clicked.connect(lambda: self.setTestIndex(g_curTestIndex+100))

        self.pose_previous1.clicked.connect(lambda: self.setPoseIndex(g_curPoseIndex-1))
        self.pose_previous2.clicked.connect(lambda: self.setPoseIndex(g_curPoseIndex-10))
        self.pose_previous3.clicked.connect(lambda: self.setPoseIndex(g_curPoseIndex-100))
        self.pose_next1.clicked.connect(lambda: self.setPoseIndex(g_curPoseIndex+1))
        self.pose_next2.clicked.connect(lambda: self.setPoseIndex(g_curPoseIndex+10))
        self.pose_next3.clicked.connect(lambda: self.setPoseIndex(g_curPoseIndex+100))

        self.marker_previous1.clicked.connect(lambda: self.setMarkerIndex(g_curMarkerIndex-1))
        self.marker_previous2.clicked.connect(lambda: self.setMarkerIndex(g_curMarkerIndex-10))
        self.marker_previous3.clicked.connect(lambda: self.setMarkerIndex(g_curMarkerIndex-100))
        self.marker_next1.clicked.connect(lambda: self.setMarkerIndex(g_curMarkerIndex+1))
        self.marker_next2.clicked.connect(lambda: self.setMarkerIndex(g_curMarkerIndex+10))
        self.marker_next3.clicked.connect(lambda: self.setMarkerIndex(g_curMarkerIndex+100))

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
                    #linePlotType = 'g'
                    #painter.setPen(QtGui.QColor(0, 255, 0))
                    lineColor = QtGui.QColor(0, 255, 0)
                else:
                    #linePlotType = 'y'
                    #painter.setPen(QtGui.QColor(255, 255, 0))
                    lineColor = QtGui.QColor(255, 255, 0)
                
                [corners, whichCorners] = getFiducialCorners(g_curPoseIndex, iMarker)
                
                #pdb.set_trace()
                
                corners = [(x+padAmount[1],y+padAmount[0]) for (x,y) in corners]
                
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

                minDist = Inf
                minInd = -1

                if (g_pointsType=='template') or (g_pointsType=='fiducialMarker'):
                    ci = 0

                    for iMarker in range(0,len(corners)):
                        ci += 1

                        pdb.set_trace()

                        # TODO: fix

                    #     meanCornersX = mean(cornersX(:,iMarker))
                    #     meanCornersY = mean(cornersY(:,iMarker))
                    #
                    #     dist = sqrt((meanCornersX - newPoint.x)^2 + (meanCornersY - newPoint.y)^2)
                    #     if dist < minDist:
                    #         minDist = dist
                    #         minInd = iMarker
                    #
                    # if minInd ~= -1:
                    #     curMarkerIndex = minInd
                    #     fixBounds()
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
            pass
            #print(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex])
            #    Save()

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
    #global g_imageWindow
    #global g_curTestIndex
    #global g_maxTestIndex

    app = QtGui.QApplication(sys.argv)

    g_imageWindow = Ui_ImageWindow()
    g_imageWindow.show()

    mySW = ControlMainWindow()
    mySW.show()

    #loadTestFile(g_startJsonDirectory + '/' + g_startJsonFilename)

    sys.exit(app.exec_())



