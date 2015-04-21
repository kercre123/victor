import sys
import json
import os.path
import time
import pdb

from PySide import QtCore, QtGui

import numpy as np

from editGroundTruthWindow import Ui_MainWindow

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

def sanitizeJsonTest(jsonData):
  pass
      
def loadJson(filename):
  print('Loading json ' + filename)

  f = open(filename, 'r')	
  jsonData = json.load(f)
  f.close()
  
  # TODO: sanitize
  
  #print(jsonData)
  
  return jsonData

def loadTestFile(testFilename):
  global g_data
  global g_image
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
    
  g_image = np.zeros([240,320], 'uint8')
  
  g_curPoseIndex = 5
  g_curMarkerIndex = 1
    
  poseChanged(True)

def fixBounds():
  # TODO: implement
  pass

def poseChanged(resetZoom):
  global g_imageWindow
  global g_data
  global g_curPoseIndex
  
  fixBounds()
    
  curImageFilename = g_data['testPath'] + g_data['jsonData']['Poses'][g_curPoseIndex]['ImageFile']
  
  image = QtGui.QPixmap(curImageFilename)
  
  g_imageWindow.setPixmap(image)
  
  #g_imageWindow.resize(image.size() * 2)
  
  g_imageWindow.setScaledContents( True )

  g_imageWindow.setSizePolicy( QtGui.QSizePolicy.Ignored, QtGui.QSizePolicy.Ignored )
  
  #g_imageWindow.setStyleSheet("background-image:url(" + curImageFilename + ")");
  
#  pdb.set_trace()
  
  
class ConnectedMainWindow(Ui_MainWindow):
  def __init__(self):
    super(ConnectedMainWindow, self).__init__()

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
    
    loadTestFile(self.testJsonFilename1.text() + '/' + self.testJsonFilename2.text())
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

    self.resize(640, 480)

class ControlMainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
      super(ControlMainWindow, self).__init__(parent)
      self.ui = ConnectedMainWindow()
      self.ui.setupUi(self)
 
if __name__ == "__main__":
  global g_imageWindow
  global g_curTestIndex
  global g_maxTestIndex

  app = QtGui.QApplication(sys.argv)
  
  g_imageWindow = Ui_ImageWindow()
  g_imageWindow.show()
  
  mySW = ControlMainWindow()
  mySW.show()
  
  loadTestFile(g_startJsonDirectory + '/' + g_startJsonFilename)
  
  sys.exit(app.exec_())
  
  
  
  