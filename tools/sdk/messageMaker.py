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

    def SetRobotImageSendMode(self, stream = True):
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


    def CreateObjectAtPose(self, x_mm, y_mm, angle_rad, depth_mm, width_mm, height_mm):
        createObjectMsg = self.GToEI.CreateObjectAtPose()
        createObjectMsg.x_mm = x_mm
        createObjectMsg.y_mm = y_mm
        createObjectMsg.angle_rad = angle_rad
        createObjectMsg.depth_mm = depth_mm
        createObjectMsg.width_mm = width_mm
        createObjectMsg.height_mm = height_mm

        toEngMessage = self.GToEM(CreateObjectAtPose = createObjectMsg)

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

    def PlayAnimationGroup(self, idTag, animGroupName, robotID, numLoops):
        playAnimGroupMsg = self.GToEI.PlayAnimationGroup()
        playAnimGroupMsg.robotID = 1

        playAnimGroupMsg.animationGroupName = animGroupName
        
        playAnimGroupMsg.numLoops = numLoops

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.playAnimationGroup = playAnimationGroupMsg

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

    def AlignWithObject(self, idTag, objectID):
        alignMsg = self.GToEI.AlignWithObject()
        alignMsg.objectID = objectID

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

    def PlaceOnObject(self, idTag, objectID):
        placeMsg = self.GToEI.PlaceOnObject()
        placeMsg.objectID = objectID

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.placeOnObject = placeMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage

    def PickupObject(self, idTag, objectID):
        pickupMsg = self.GToEI.PickupObject()
        pickupMsg.objectID = objectID

        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.pickupObject = pickupMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);
        
        return toEngMessage

    def SayTextQueue(self, idTag, text):
        textMsg = self.GToEI.SayText()
        textMsg.text = text
        textMsg.playEvent = self.EToG.GameEvent.OnSawOldNamedFace
        textMsg.style = self.GToE.SayTextStyle.Normal
        
        queueMessage = self._BuildQueueSingleAction(idTag)
        queueMessage.action.sayText = textMsg

        toEngMessage = self.GToEM(QueueSingleAction = queueMessage);

        return toEngMessage

    def SayText(self, text):
        textMsg = self.GToEI.SayText()
        textMsg.text = text
        textMsg.playEvent = self.EToG.GameEvent.OnSawOldNamedFace
        textMsg.style = self.GToE.SayTextStyle.Normal
        
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