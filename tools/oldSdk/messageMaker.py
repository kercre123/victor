"""
A file for creating the applicable message to correspond to interface functions
"""
__author__ = "Alec Solder"

import sys, os, time, math

class MessageMaker:
    def __init__(self, GToEI, GToEM,EToG,GToE):
        self.GToEI = GToEI 
        self.GToEM = GToEM
        self.EToG = EToG
        self.GToE = GToE

    def _BuildQueueSingleAction(self, idTag):
        queueMessage = self.GToEI.QueueSingleAction()
        queueMessage.robotID = 1
        global gIdTag
        queueMessage.idTag = idTag
        queueMessage.numRetries = 1
        queueMessage.position = 0
        queueMessage.action = self.GToEI.RobotActionUnion()
        return queueMessage

    def StartSim(self):
        startEngMsg = self.GToEI.StartEngine()
        connectMsg = self.GToEI.ConnectToRobot()
        connectMsg.ipAddress = [49,50,55,46,48,46,48,46,49,0,0,0,0,0,0,0]
        connectMsg.robotID = 1
        connectMsg.isSimulated = 1

        toEngMessageStart = self.GToEM(StartEngine = startEngMsg)
        toEngMessageConnect = self.GToEM(ConnectToRobot = connectMsg)
        return [toEngMessageStart, toEngMessageConnect]

    def SetRobotImageSendMode(self, stream):
        imageSendModeMsg = self.GToEI.SetRobotImageSendMode()
        imageSendModeMsg.robotID = 1
        if stream:
            imageSendModeMsg.mode = self.EToG.ImageSendMode.Stream
        else:
            imageSendModeMsg.mode = self.EToG.ImageSendMode.Off

        imageSendModeMsg.resolution = self.EToG.ImageResolution.QVGA

        toEngMessage = self.GToEM(SetRobotImageSendMode = imageSendModeMsg)

        return toEngMessage

    def UnlockAll(self):
        unlockMsg = self.GToEI.RequestSetUnlock()
        unlockMsg.unlocked = True
        messages = []
        for behavior in range(self.EToG.UnlockId.Count):
            unlockMsg.unlockID = behavior
            toEngMessage = self.GToEM(RequestSetUnlock = unlockMsg)
            messages = messages + [toEngMessage]
        return messages


    def CreateFixedCustomObject(self, x_y_z, angle_rad, xSize_mm, ySize_mm, zSize_mm):
        fixedObjectMsg = self.GToEI.CreateFixedCustomObject()
        x,y,z = x_y_z
        fixedObjectMsg.pose.x = x
        fixedObjectMsg.pose.y = y
        fixedObjectMsg.pose.z = z
        # Quaternion of rotation angle_rad in the z axis
        fixedObjectMsg.pose.q0 = angle_rad
        fixedObjectMsg.pose.q1 = 0
        fixedObjectMsg.pose.q2 = 0
        fixedObjectMsg.pose.q3 = 1
        fixedObjectMsg.xSize_mm = xSize_mm
        fixedObjectMsg.ySize_mm = ySize_mm
        fixedObjectMsg.zSize_mm = zSize_mm

        toEngMessage = self.GToEM(CreateFixedCustomObject = fixedObjectMsg)

        return toEngMessage

    def StackBlocks(self):
        stackMsg = self.GToEI.ExecuteBehavior()
        stackMsg.behaviorType = self.GToE.BehaviorType.StackBlocks
        toEngMessage = self.GToEM(ExecuteBehavior = stackMsg)

        return toEngMessage

    def RollBlock(self):
        rollMsg = self.GToEI.ExecuteBehavior()
        rollMsg.behaviorType = self.GToE.BehaviorType.RollBlock
        toEngMessage = self.GToEM(ExecuteBehavior = rollMsg)

        return toEngMessage

    def StopBehavior(self):
        stopMsg = self.GToEI.ExecuteBehavior()
        stopMsg.behaviorType = 0
        toEngMessage = self.GToEM(ExecuteBehavior = stopMsg)

        return toEngMessage

    def LookAround(self):
        lookMsg = self.GToEI.ExecuteBehavior()
        lookMsg.behaviorType = 1
        toEngMessage = self.GToEM(ExecuteBehavior = lookMsg)

        return toEngMessage

    def DriveWheels(self, lWheelSpeed, rWheelSpeed, lWheelAcc, rWheelAcc):
        driveMsg = self.GToEI.DriveWheels()
        driveMsg.lwheel_speed_mmps = lWheelSpeed
        driveMsg.rwheel_speed_mmps = rWheelSpeed
        driveMsg.lwheel_accel_mmps2 = lWheelAcc
        driveMsg.rwheel_accel_mmps2 = rWheelAcc

        toEngMessage = self.GToEM(DriveWheels = driveMsg)

        return toEngMessage

    def Stop(self):
        stopMsg = self.GToEI.StopAllMotors()
        toEngMessage = self.GToEM(StopAllMotors = stopMsg)

        return toEngMessage

    def PlayAnimation(self, idTag, animName, robotID, numLoops):
        playAnimMsg = self.GToEI.PlayAnimation()
        playAnimMsg.robotID = 1

        playAnimMsg.animationName = animName
        
        playAnimMsg.numLoops = numLoops

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.playAnimation = playAnimMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        return toEngMessage

    def PlayAnimationTrigger(self, idTag, animTrigger, robotID, numLoops):
        playAnimTriggerMsg = self.GToEI.PlayAnimationTrigger()
        playAnimTriggerMsg.robotID = robotID

        playAnimTriggerMsg.trigger = animTrigger

        playAnimTriggerMsg.numLoops = numLoops

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.playAnimationTrigger = playAnimTriggerMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage

    def MoveHead(self, velocity):
        moveHeadMsg = self.GToEI.MoveHead()
        moveHeadMsg.speed_rad_per_sec = velocity

        toEngMessage = self.GToEM(MoveHead = moveHeadMsg);
        
        return toEngMessage 

    def MoveLift(self, velocity):
        moveLiftMsg = self.GToEI.MoveLift()
        moveLiftMsg.speed_rad_per_sec = velocity

        toEngMessage = self.GToEM(MoveLift = moveLiftMsg)

        return toEngMessage 

    def RollObject(self, idTag, objectID):
        rollObjectMsg = self.GToEI.RollObject()
        rollObjectMsg.objectID = objectID

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.rollObject = rollObjectMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage 

    def PopAWheelie(self):
        wheelieMsg = self.GToEI.PopAWheelie()

        toEngMessage = self.GToEM(PopAWheelie = wheelieMsg);
        
        return toEngMessage 

    def AlignWithObject(self, idTag, objectID, usePreDockPose, useManualSpeed):
        alignMsg = self.GToEI.AlignWithObject()
        alignMsg.objectID = objectID
        alignMsg.usePreDockPose = usePreDockPose
        alignMsg.useManualSpeed = useManualSpeed

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.alignWithObject = alignMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage 

    def GoToPose(self, idTag, x_mm, y_mm, rad):
        
        poseMsg = self.GToEI.GotoPose()
        poseMsg.x_mm = x_mm
        poseMsg.y_mm = y_mm
        poseMsg.rad = rad

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.goToPose = poseMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage

    def PlaceOnObject(self, idTag, objectID, usePreDockPose, useManualSpeed):
        placeMsg = self.GToEI.PlaceOnObject()
        placeMsg.objectID = objectID
        placeMsg.usePreDockPose = usePreDockPose
        placeMsg.useManualSpeed = useManualSpeed
        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.placeOnObject = placeMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage

    def PickupObject(self, idTag, objectID, usePreDockPose, useManualSpeed):
        pickupMsg = self.GToEI.PickupObject()
        pickupMsg.objectID = objectID
        pickupMsg.usePreDockPose = usePreDockPose
        pickupMsg.useManualSpeed = useManualSpeed
        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.pickupObject = pickupMsg


        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage

    def SayTextQueue(self, idTag, text):
        textMsg = self.GToEI.SayText()
        textMsg.text = text
        textMsg.playEvent = self.GToE.AnimationTrigger.OnSawNewNamedFace
        textMsg.style = self.GToE.SayTextStyle.Name_Normal
        
        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.sayText = textMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);

        return toEngMessage

    def SayText(self, text):
        textMsg = self.GToEI.SayText()
        textMsg.text = text
        textMsg.playEvent = self.GToE.AnimationTrigger.OnSawNewNamedFace
        textMsg.style = self.GToE.SayTextStyle.Name_Normal
        
        toEngMessage = self.GToEM(SayText = textMsg);

        return toEngMessage

    def EnableReactionaryBehaviors(self, on):
        enableMsg = self.GToEI.EnableReactionaryBehaviors()
        enableMsg.enabled = on

        toEngMessage = self.GToEM(EnableReactionaryBehaviors = enableMsg);
        
        return toEngMessage

    def TurnInPlace(self, rad):
        turnMsg = self.GToEI.TurnInPlace()
        turnMsg.angle_rad = rad
        turnMsg.isAbsolute = 0
        turnMsg.robotID = 1

        toEngMessage = self.GToEM(TurnInPlace = turnMsg);
        
        return toEngMessage

    def MountCharger(self, objectID):
        mountMsg = self.GToEI.MountCharger()
        mountMsg.objectID = objectID

        toEngMessage = self.GToEM(MountCharger = mountMsg);
        
        return toEngMessage

    def ClearAllObjects(self):
        clearMsg = self.GToEI.ClearAllObjects()
        clearMsg.robotID = 1

        toEngMessage = self.GToEM(ClearAllObjects = clearMsg)

        return toEngMessage

    def DeleteAllObjects(self):
        deleteMsg = self.GToEI.DeleteAllObjects()
        deleteMsg.robotID = 1

        toEngMessage = self.GToEM(DeleteAllObjects = deleteMsg)

        return toEngMessage

    def DeleteAllCustomObjects(self):
        deleteMsg = self.GToEI.DeleteAllCustomObjects()
        deleteMsg.robotID = 1

        toEngMessage = self.GToEM(DeleteAllCustomObjects=deleteMsg)

        return toEngMessage

    def SetBackpackLEDs(self, onColor, offColor, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms):
        setLEDMsg = self.GToEI.SetBackpackLEDs()
        setLEDMsg.onColor = onColor
        setLEDMsg.offColor = offColor
        setLEDMsg.onPeriod_ms = onPeriod_ms
        setLEDMsg.offPeriod_ms =offPeriod_ms
        setLEDMsg.transitionOnPeriod_ms = transitionOnPeriod_ms
        setLEDMsg.transitionOffPeriod_ms = transitionOffPeriod_ms
        setLEDMsg.robotID = 1
        
        toEngMessage = self.GToEM(SetBackpackLEDs = setLEDMsg)

        return toEngMessage


    def EnrollNamedFace(self, idTag, name, faceID):
        enrollMsg = self.GToEI.EnrollNamedFace()
        enrollMsg.faceID = faceID
        enrollMsg.name = name

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.enrollNamedFace = enrollMsg

        toEngMessage = self.GToEM(QueueSingleAction=queueMessage)

        return toEngMessage

    def TurnTowardsFace(self, idTag, faceID, maxTurnAngle_rad):
        turnMsg = self.GToEI.TurnTowardsFace()
        turnMsg.robotID = 1
        turnMsg.faceID = faceID
        turnMsg.maxTurnAngle_rad = maxTurnAngle_rad

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.turnTowardsFace = turnMsg

        toEngMessage = self.GToEM(QueueSingleAction=queueMessage)

        return toEngMessage

    def TurnTowardsPose(self, idTag, x_mm, y_mm, z_mm, maxTurnAngle_rad):
        turnMsg = self.GToEI.TurnTowardsPose()
        turnMsg.robotID = 1
        turnMsg.world_x = x_mm
        turnMsg.world_y = y_mm
        turnMsg.world_z = z_mm
        turnMsg.maxTurnAngle_rad = maxTurnAngle_rad

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.turnTowardsPose = turnMsg

        toEngMessage = self.GToEM(QueueSingleAction=queueMessage)

        return toEngMessage

    def SetActiveObjectLEDs(self, objectID, onColor, offColor, onPeriod_ms, offPeriod_ms,
                            transitionOnPeriod_ms, transitionOffPeriod_ms, relativeToX, relativeToY,
                            whichLEDs, makeRelative, turnOffUnspecifiedLEDs, robotID):
        lightMsg = self.GToEI.SetActiveObjectLEDs()

        lightMsg.objectID = objectID
        lightMsg.onColor = onColor
        lightMsg.offColor = offColor
        lightMsg.onPeriod_ms = onPeriod_ms
        lightMsg.offPeriod_ms = offPeriod_ms
        lightMsg.transitionOnPeriod_ms = transitionOnPeriod_ms
        lightMsg.transitionOffPeriod_ms = transitionOffPeriod_ms
        lightMsg.relativeToX = relativeToX
        lightMsg.relativeToY = relativeToY
        lightMsg.whichLEDs = whichLEDs
        lightMsg.makeRelative = makeRelative
        lightMsg.turnOffUnspecifiedLEDs = turnOffUnspecifiedLEDs
        lightMsg.robotID = robotID

        toEngMessage = self.GToEM(SetActiveObjectLEDs=lightMsg)

        return toEngMessage

    def DefineCustomObject(self, objectType, xSize_mm, ySize_mm, zSize_mm, markerWidth_mm, markerHeight_mm):
        defineMsg = self.GToEI.DefineCustomObject()

        defineMsg.objectType = objectType
        defineMsg.xSize_mm = xSize_mm
        defineMsg.ySize_mm = ySize_mm
        defineMsg.zSize_mm = zSize_mm
        defineMsg.markerWidth_mm = markerWidth_mm
        defineMsg.markerHeight_mm = markerHeight_mm

        toEngMessage = self.GToEM(DefineCustomObject=defineMsg)

        return toEngMessage

    def DisplayFaceImage(self, data):
        faceMsg = self.GToEI.DisplayFaceImage()
        # print("Facedata:" , data)
        faceMsg.faceData = data

        toEngMessage = self.GToEM(DisplayFaceImage = faceMsg)

        return toEngMessage

    def DisplayProceduralFace(self, faceCenX, faceCenY, faceAngle,
                                    lEyeCenX, lEyeCenY,
                                    lEyeScaleX, lEyeScaleY,
                                    rEyeCenX, rEyeCenY,
                                    rEyeScaleX, rEyeScaleY):

        leftEye = [7.13,0.0,1.48,0.97,
                   0.0,0.185,0.185,0.173,
                   0.173,0.2537,0.253,
                   0.185,0.185,0.0,0.0,0.0,0.0,0.0,0.0]
        rightEye = [-9.138,0.0,1.505,0.970,
                    0.0,0.1853,0.1853,0.1733,
                    0.173,0.253,0.253,
                    0.185,0.185,0.0,0.0,0.0,0.0,0.0,0.0]
        faceMsg = self.GToEI.DisplayProceduralFace()

        faceMsg.faceAngle = faceAngle
        faceMsg.faceScaleX = 1
        faceMsg.faceScaleY = 1
        faceMsg.robotID = 1

        faceMsg.faceCenX = faceCenX
        faceMsg.faceCenY = faceCenY

        leftEye[0] = lEyeCenX;leftEye[1] = lEyeCenY
        leftEye[2] = lEyeScaleX;leftEye[3] = lEyeScaleY
        rightEye[0] = rEyeCenX;rightEye[1] = rEyeCenY
        rightEye[2] = rEyeScaleX;rightEye[3] = rEyeScaleY
        faceMsg.leftEye = leftEye
        faceMsg.rightEye = rightEye

        toEngMessage = self.GToEM(DisplayProceduralFace=faceMsg)

        return toEngMessage

