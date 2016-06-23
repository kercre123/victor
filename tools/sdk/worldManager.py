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
        # There are three distinct light cubes, this stores them in their
        # Objectype order
        self.lightCubes = [{},{},{}]
        # These stores the objectID's of the lightcubes in order of time seen
        self.cubeOrder = []
        self.numCubes = len(self.cubeOrder)
        self.charger = {}
        self.faces = []
        self.robotState = {}
        self.movement = []
        self.imageManager = ImageManager(EToG, GToE)
        self.robotImageSendMode = False

    def _ImageChunk(self, msg):
        self.imageManager.ImageChunk(msg)

    def GetImage(self, index = 1):
        return self.imageManager.GetImage(index)

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

    def ChargerExists(self):
        return bool(self.charger)

    def _RobotState(self, msg):
        self.robotState = msg

    def _ObservedFace(self, msg):
        self.faces.append(msg)

    def _ObservedMotion(self, msg):
        self.movement.append(msg)

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
          