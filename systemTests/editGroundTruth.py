import sys
import json
import os.path
import time
import pdb
import random

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
    #   jsonData['Poses'][iPose]['VisionMarkers'][end+1] = []

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

def poseChanged(resetZoom):
  global g_imageWindow
  global g_data
  global g_curPoseIndex
  global g_rawImage
  global g_toShowImage

  fixBounds()

  curImageFilename = g_data['testPath'] + g_data['jsonData']['Poses'][g_curPoseIndex]['ImageFile']

  g_rawImage = QtGui.QPixmap(curImageFilename)
  g_toShowImage = QtGui.QPixmap(g_rawImage)

  painter = QtGui.QPainter(g_toShowImage)
  painter.setPen(QtGui.QColor(128, 128, 0))
  painter.drawLine(0, 0, 200, 200);
  painter.end()

  g_imageWindow.setPixmap(g_toShowImage)

  #g_imageWindow.resize(image.size() * 2)

  g_imageWindow.setScaledContents( True )

  g_imageWindow.setSizePolicy( QtGui.QSizePolicy.Ignored, QtGui.QSizePolicy.Ignored )

  #g_imageWindow.setStyleSheet("background-image:url(" + curImageFilename + ")")

#  pdb.set_trace()

def getFiducialCorners(poseIndex, markerIndex):
  global g_data

  numVisionMarkers = len(g_data['jsonData']['Poses'][poseIndex]['VisionMarkers'])

  #pdb.set_trace()

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
    #event.accept();
    
  def paintEvent(self, event):
    super(Ui_ImageWindow, self).paintEvent(event)
    painter = QtGui.QPainter(self)
    painter.setPen(QtGui.QColor(128, 0, 128))
    painter.drawLine(int(random.random()*400), int(random.random()*400), int(random.random()*400), int(random.random()*400))
    #painter.drawLine(0, 0, 400, 400);
    painter.end()
    
    #g_imageWindow.setPixmap(g_toShowImage)
    #print('Toasted')

  def mousePressEvent(self, event):
    #print('clicked ' + str(event))

    buttonModifiers = event.modifiers()

    [corners, whichCorners] = getFiducialCorners(g_curPoseIndex, g_curMarkerIndex)

    realImageSize = [float(g_imageWindow.pixmap().height()), float(g_imageWindow.pixmap().width())]
    displayImageSize = [float(self.height()), float(self.height())]

    #pdb.set_trace()

    xScale = displayImageSize[1] / realImageSize[1]
    yScale = displayImageSize[0] / realImageSize[0]

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

          #   meanCornersX = mean(cornersX(:,iMarker))
          #   meanCornersY = mean(cornersY(:,iMarker))
          #
          #   dist = sqrt((meanCornersX - newPoint.x)^2 + (meanCornersY - newPoint.y)^2)
          #   if dist < minDist:
          #     minDist = dist
          #     minInd = iMarker
          #
          # if minInd ~= -1:
          #   curMarkerIndex = minInd
          #   fixBounds()
      else: # left click       
        if (g_pointsType=='template') or (g_pointsType=='fiducialMarker'):
          if sum(whichCorners) < 4:
            for i in range(0,4):
              if whichCorners[i] == 0:
                if len(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'] <= g_curMarkerIndex):
                  g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'].append({})

                g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex][cornerNamesX[i]] = newPoint['x']
                g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex][cornerNamesY[i]] = newPoint['y']

                break

            changed = True
        else:
          print('Cannot add point, because only 4 fiduciual marker corners are allowed')

    elif event.button() == QtCore.Qt.MouseButton.RightButton: # right click
      minDist = Inf
      minInd = -1

      if (g_pointsType == 'template') or (g_pointsType == 'fiducialMarker'):
        ci = 0
        for i in range(0,len(whichCorners)):
          if whichCorners[i] == 0:
            continue

          ci += 1

          dist = sqrt(pow(corners[ci][0] - newPoint['x'], 2) + pow(corners[ci][1] - newPoint['y'], 2))
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
      print(g_data['jsonData']['Poses'][g_curPoseIndex]['VisionMarkers'][g_curMarkerIndex])
      #  Save()

    poseChanged(False)

class ControlMainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
      super(ControlMainWindow, self).__init__(parent)
      self.ui = ConnectedMainWindow()
      self.ui.setupUi(self)
      
    def closeEvent(self, event):
      sys.exit(0)
      #event.accept();

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



