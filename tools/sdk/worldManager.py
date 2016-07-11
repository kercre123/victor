"""
World Manager to store the current world state
"""
__author__ = "Alec Solder"

import sys
import os
from imageManager import ImageManager

# ================================================================================    
# World Manager
# ================================================================================
class WorldManager:
    "WorldManager"

    def __init__(self, EToG, GToE):
        self.EToG = EToG
        self.GToE = GToE
        self.lastImageReceivedTime = 0
        # There are three distinct light cubes, this stores them in their
        # Objectype order
        self.lightCubes = [None, None, None]
        # These stores the objectID's of the lightcubes in order of time seen
        self.cubeOrder = []
        self.numCubes = len(self.cubeOrder)
        self.charger = {}
        self.faces = {}
        self.faceOrder = []
        self.numFaces = 0
        self.robotState = None
        self.movement = []
        self.imageManager = ImageManager(EToG, GToE)
        self.robotImageSendMode = False

    def _ImageChunk(self, msg):
        if msg.frameTimeStamp:
            self.lastImageReceivedTime = msg.frameTimeStamp
        self.imageManager.ImageChunk(msg)

    def GetImage(self, index = 1, drawViz = False):
        image = self.imageManager.GetImage(index)
        return self._DrawViz(image) if drawViz else image

    def IsObjectVisible(self, obj):
        # image times in milliseconds
        visionTimeout = 300
        return ((self.lastImageReceivedTime-obj.timestamp) < visionTimeout)

    def _ObservedObject(self, msg):
        if msg.objectFamily == self.EToG.ObjectFamily.LightCube:
            self.cubeOrder.append(msg.objectID)
            self.cubeOrder = sorted(set(self.cubeOrder))
            self.numCubes = len(self.cubeOrder)
            # Subtract 1 because there are 3 distinct lightcubes of type 1, 2 and 3
            self.lightCubes[msg.objectType - 1] = msg
            return True

        elif msg.objectFamily == self.EToG.ObjectFamily.Charger:
            self.charger = msg
            return True
    
    def DeleteAllObjects(self):
        self.cubeOrder = []
        self.numCubes = 0

    def ChargerExists(self):
        return bool(self.charger)

    def _RobotState(self, msg):
        self.robotState = msg

    def _ObservedFace(self, msg):
        # print("Face observed: " , msg.timestamp)
        if str(msg.faceID) in self.faces:
            self.faces[str(msg.faceID)] = msg
            return True
        self.faces[str(msg.faceID)] = msg
        # We store the objectID of the face, and also if it is known or not
        # (objectID, isKnown)
        self.faceOrder.append((abs(msg.faceID), msg.faceID > 0))
        self.faceOrder = sorted(set(self.faceOrder))
        self.numFaces = len(self.faceOrder)

    def GetFace(self, index = 0):
        maxIndex = len(self.faceOrder) - 1
        if (index < 0) or (index > maxIndex):
            sys.stderr.write("GetFace: index " + str(index)  + " out of 0.." + str(maxIndex) + " range" + os.linesep)
            return None

        faceOrder = self.faceOrder[index]
        faceId  = self.FaceID(faceOrder)
        return self.faces[str(faceId)]

    def FaceID(self, face):
        return -face[0] if not face[1] else face[0]

    def _ObservedMotion(self, msg):
        self.movement.append(msg)

    def _DrawCubes(self, image):
        for cube in self.lightCubes:
            if cube and self.IsObjectVisible(cube):
                if self.IsObjectVisible(cube):
                    x1 = int(cube.img_topLeft_x)
                    y1 = int(cube.img_topLeft_y)
                    x2 = int(x1 + cube.img_width)
                    y2 = int(y1 + cube.img_height)
                    self.imageManager.DrawRectangle(image, x1, y1, x2, y2, outline = (255, 255, 0))
        return image

    def _DrawFaces(self, image):
        visionTimeout = 300
        for key in self.faces:
            face = self.faces[key]
            if not self.IsObjectVisible(face):
                continue
            x1 = int(face.img_topLeft_x)
            y1 = int(face.img_topLeft_y)
            x2 = int(x1 + face.img_width)
            y2 = int(y1 + face.img_height)
            self.imageManager.DrawRectangle(image, x1, y1, x2, y2, outline = (255,0,255))
            if face.name:
                self.imageManager.DrawText(image, x1,y1, face.name, fill = (0,255,255))
        return image

    def _DrawViz(self, image, drawCubes = True, drawFaces = True):
        image = self._DrawCubes(image) if drawCubes else image
        image = self._DrawFaces(image) if drawFaces else image
        return image
            

    # def RobotDeletedObject(self, msg):
    #     pass
    # def RobotDeletedFace(self, msg):
    #     pass

    # def RobotMarkedObjectPostUnknown(self, msg):
    #     pass

    # def RobotChangedObservedFaceID(self, msg):
    #     pass
    #     for face in self.faces:
    #         if face["faceID"] == msg.oldID:
    #             face["faceID"] = msg.newID


    # def RobotPickedUp(self, msg):
    #     self.robotState["pickedUp"] = True

    # def RobotPutDown(self, msg):
    #     self.robotState["pickedUp"] = False

    # def RobotPoked(self, msg):
    #     self.robotState["poked"] = True
    
    # def RobotStopped(self, msg):
    #     self.robotState["stopped"] = True
    
    # def RobotOnBack(self, msg):
    #     self.robotState["onBack"] = True

    # def RobotOnBackFinished(self, msg):
    #     self.robotState["onBack"] = False

    # def RobotCliffEventFinished(self, msg):
    #     self.robotState["cliffEvent"] = False
          
