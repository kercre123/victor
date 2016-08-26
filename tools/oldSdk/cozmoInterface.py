"""
SDK Interface to Cozmo-Engine, supports communication over either a localhost UDP socket, or a TCP socket
All communication is via CLAD messages.
"""
__author__ = "Mark Wesley"

import sys, os, time, math
import threading # for threaded engine
from collections import deque # queue for threaded engine
from debugConsole import DebugConsoleManager
from moodManager import MoodManager
from animationManager import AnimationManager
from worldManager import WorldManager
from tcpConnection import TcpConnection
from udpConnection import UdpConnection
from verboseLevel import VerboseLevel
from messageMaker import MessageMaker

try:
    from clad.externalInterface.messageEngineToGame import Anki
    from clad.externalInterface.messageGameToEngine import Anki as _Anki
except Exception as e:
    sys.stdout.write("Exception = " + str(e)  + os.linesep)
    sys.exit("Can't import CLAD libraries!{linesep}\t* Are you running from the base SDK directory?{linesep}".format(linesep=os.linesep))

# [MARKW:TODO] i don't know why robot interface does this?
#Anki.update(_Anki.deep_clone())

# namespace shortcuts
EToG  =  Anki.Cozmo
GToE  = _Anki.Cozmo
EToGI = EToG.ExternalInterface
GToEI = GToE.ExternalInterface
EToGM = EToGI.MessageEngineToGame
GToEM = GToEI.MessageGameToEngine


# We are using gIdTag as a counter to keep track of messages we sent. We set gKey when we are in a locked
# state (to the value of gIdTag) and wait for a message response from the engine with the value key
gIdTag = 100
gKey = 0
gState = []

class CozmoInterface:
    def __init__(self, verboseLevel = 0, enableReactionaryBehaviors = False):
        "Initalize the engine interface. Must be called before any other methods"

        self.verboseLevel = verboseLevel
        self.asyncEngineInterfaceInstance = None

        self.asyncEngineInterfaceInstance = AsyncEngineInterface(verboseLevel = verboseLevel)
        self.asyncEngineInterfaceInstance.start()

        self.messageMaker = MessageMaker(GToEI,GToEM,EToG,GToE)

        # Initial speeds of head/treads. These are used to prevent message spam in DriveWheels
        # and MoveHead
        self.sentSpeeds = (0,0)
        self.sentHeadSpeed = 0

        while not self.IsConnected():
            time.sleep(.05)
            continue
        # Time to load on-startup engine information
        # TODO wait for an "engine loaded" message instead of sleeping
        # time.sleep(2)
        self.StartSim()
        self.EnableReactionaryBehaviors(enableReactionaryBehaviors)
        self.SetRobotImageSendMode(True)
        self.UnlockAll()
        self.DeleteAllCustomObjects()

    def Shutdown(self):
        self.Stop()
        time.sleep(1)
        if self.asyncEngineInterfaceInstance is not None:
            del self.asyncEngineInterfaceInstance
            self.asyncEngineInterfaceInstance = None

    def IsConnected(self):
        return self.asyncEngineInterfaceInstance.IsConnected()

    def GetState(self):
        global gState
        # We want to never load an entry that may be being written to in the backgorund
        # So we just read from the second most recent
        return gState[1] if len(gState) >= 2 else None

    def SetVerboseLevel(self, newLevel):
        if  0 <= newLevel <= VerboseLevel.Max:
            self.verboseLevel = newLevel
            self.asyncEngineInterfaceInstance.SetVerboseLevel(newLevel)
            return True 
        else:
            sys.stderr.write("[SetVerboseLevel] newLevel '" + str(newLevel) + "' out of range" + os.linesep)

    def QueueMessage(self, inMessage):
        self.asyncEngineInterfaceInstance.QueueMessage(inMessage)

    def SyncMessage(self, inMessage):
        self.asyncEngineInterfaceInstance.SyncMessage(inMessage)
        
    def WaitUntilSeeBlocks(self, numCubes, timeout = 0):
        startTime = time.time()
        # If timeout is the default value 0, just wait until the inside condition is satisfied 
        currState = self.GetState()
        if not currState:
            print("State is currently None")
            return 0
        else:
            num = currState.numCubes
            while ((not timeout) or ((time.time() - startTime) < timeout)):
                if (num >= numCubes):
                    return num
                currState = self.GetState()
                num = currState.numCubes
                time.sleep(.05)

            sys.stdout.write("WaitUntilSeeBlocks timed out: NumBlocksSeen = " + str(num) + os.linesep)
            return num
        return 0

    def WaitUntilSeeFaces(self, numFaces, timeout = 0):
        startTime = time.time()
        # If timeout is the default value 0, just wait until the inside condition is satisfied 
        currState = self.GetState()
        if not currState:
            print("State is currently None")
            return 0
        else:
            num = currState.numFaces
            while ((not timeout) or ((time.time() - startTime) < timeout)):
                if (num >= numFaces):
                    return num
                currState = self.GetState()
                num = currState.numFaces
                time.sleep(.05)

            sys.stdout.write("WaitUntilSeeFace timed out: NumFaceSeen = " + str(num) + os.linesep)
            return num
        return 0

    def CreateFixedCustomObjectRelativeToRobotPose(self, x_y_z, angle_rad, xSize_mm, ySize_mm, zSize_mm):
        preX,preY,preZ = x_y_z
        (x,y,z,angle) = self.GetPoseRelRobot(preX, preY, preZ, angle_rad)
        self.CreateFixedCustomObject((x,y,z), angle, xSize_mm, ySize_mm, zSize_mm)

    def CreateFixedCustomObject(self, x_y_z, angle_rad, xSize_mm, ySize_mm, zSize_mm):
        self.QueueMessage(self.messageMaker.CreateFixedCustomObject(x_y_z, angle_rad, xSize_mm, ySize_mm, zSize_mm))
        Lock(LockTypes.createdFixedCustomObject)

    def StartSim(self):
        self.QueueMessage(self.messageMaker.StartSim())
        # TODO: Need to wait for a success message from engine
        time.sleep(1)

    def SetRobotImageSendMode(self, stream):
        "Enables cozmo sending image chunks to the SDK, necessary before attempting to get messages"
        self.QueueMessage(self.messageMaker.SetRobotImageSendMode(stream))

    def UnlockAll(self):
        self.QueueMessage(self.messageMaker.UnlockAll())

    def StackBlocks(self, duration = 0):
        self.QueueMessage(self.messageMaker.StackBlocks())
        if duration:
            time.sleep(duration)
            self.StopBehavior()

    def RollBlock(self, duration = 0):
        self.QueueMessage(self.messageMaker.RollBlock())
        if duration:
            time.sleep(duration)
            self.StopBehavior()

    def StopBehavior(self):
        self.QueueMessage(self.messageMaker.StopBehavior())

    def LookAround(self, duration = 0):
        self.QueueMessage(self.messageMaker.LookAround())
        if duration:
            time.sleep(duration)
            self.StopBehavior()

    def DriveWheels(self, lWheelSpeed = 50, rWheelSpeed = 50, lWheelAcc = 50, rWheelAcc = 50, duration = 0.0):
        # Should help to prevent spam
        if (lWheelSpeed, rWheelSpeed) == self.sentSpeeds:
            return False
        self.QueueMessage(self.messageMaker.DriveWheels(lWheelSpeed, rWheelSpeed, 
                                                        lWheelAcc, rWheelAcc))
        self.sentSpeeds = (lWheelSpeed, rWheelSpeed)
        if duration:
            time.sleep(duration)
            self.Stop()
            time.sleep(.5)

    def Stop(self):
        if (0, 0) == self.sentSpeeds:
            return False
        self.QueueMessage(self.messageMaker.Stop())
        self.sentSpeeds = (0, 0)


    def PlayAnimation(self, animName, robotID = 1, numLoops = 1):
        if not self.asyncEngineInterfaceInstance.InAnimationNames(animName):
            sys.stderr.write("[PlayAnimation] Error: Animation requested not in list of possible animations" + os.linesep)
            return False
        self.QueueMessage(self.messageMaker.PlayAnimation(gIdTag, animName, robotID, numLoops))
        Lock()
        return True

    def PlayAnimationTrigger(self, trigger, robotID = 1, numLoops = 1):
        try:
            animTrigger = eval("GToE.AnimationTrigger."+trigger)
        except:
            print("Not a valid AnimationTrigger, AnimationTrigger lookup unsuccessful")
            return False
        self.QueueMessage(self.messageMaker.PlayAnimationTrigger(gIdTag, animTrigger, robotID, numLoops))
        Lock()
        return True

    def MoveHead(self, velocity):
        "Moves cozmos head, velocity in rad/s"
        if velocity == self.sentHeadSpeed:
            return False
        self.QueueMessage(self.messageMaker.MoveHead(velocity))
        self.sentHeadSpeed = velocity

    def MoveLift(self, velocity):
        self.QueueMessage(self.messageMaker.MoveLift(velocity))

    def RollObject(self, objectID = -1):
        self.QueueMessage(self.messageMaker.RollObject(gIdTag, objectID))
        Lock()

    def PopAWheelie(self):
        "Flips Cozmo into a wheelie position, needs a block to flip himself"
        self.QueueMessage(self.messageMaker.PopAWheelie())
        Lock(LockTypes.onBack)

    def AlignWithObject(self, objectID = -1, usePreDockPose = True, useManualSpeed = False):
        self.QueueMessage(self.messageMaker.AlignWithObject(gIdTag, objectID, usePreDockPose, useManualSpeed))
        Lock()

    def GoToPose(self, x_mm, y_mm, rad):
        self.QueueMessage(self.messageMaker.GoToPose(gIdTag, x_mm, y_mm, rad))

    def GetPoseRelRobot(self, x_mm, y_mm, z_mm, angle_rad):
        currState = self.GetState()
        if not currState:
            print("Cozmo's state is none!")
            return (0,0,0)
        currX = currState.robotState.pose.x
        currY = currState.robotState.pose.y
        currZ = currState.robotState.pose.z
        currRad = currState.robotState.poseAngle_rad
        finalX = currX + math.cos(currRad)*x_mm - math.sin(currRad)*y_mm
        finalY = currY + math.sin(currRad)*x_mm + math.cos(currRad)*y_mm
        # TODO adjust finalZ to be relative to cozmo's pitch
        finalZ = currZ + z_mm
        finalRad = currRad + angle_rad
        return (finalX, finalY, finalZ, finalRad)

    def DriveDistance(self, x_mm, y_mm = 0, angle_rad = 0, duration = 0):
        (x,y,z,angle) = self.GetPoseRelRobot(x_mm,y_mm,0,angle_rad)
        self.GoToPose(x, y, angle)
        if duration:
            time.sleep(duration)
            self.Stop()

    def PlaceOnObject(self, blockIndex = -1, usePreDockPose = True, useManualSpeed = False):
        state = self.GetState()
        if len(state.cubeOrder) > blockIndex:
            self.QueueMessage(self.messageMaker.PlaceOnObject(gIdTag, state.cubeOrder[blockIndex], usePreDockPose, useManualSpeed))
            Lock()
        else:
            sys.stdout.write('Numblocks < blockIndex, there is no block at that index' + os.linesep)
        
    def PickupObject(self, blockIndex = -1, usePreDockPose = True, useManualSpeed = False):
        state = self.GetState()
        if len(state.cubeOrder) > blockIndex:
            self.QueueMessage(self.messageMaker.PickupObject(gIdTag, state.cubeOrder[blockIndex], usePreDockPose, useManualSpeed))
            Lock()
        else:
            sys.stdout.write('Numblocks < blockIndex, there is no block at that index' + os.linesep)

    # Sounds are linked with specific behaviors and animations, so a sound is an animation basically
    # (Which is also linked to playing some sound)
    def SayTextQueue(self, text):
        self.QueueMessage(self.messageMaker.SayTextQueue(gIdTag, text))

    def SayText(self, text):
        self.QueueMessage(self.messageMaker.SayText(text))
        # TODO get completion message
        time.sleep(2)

    def EnableReactionaryBehaviors(self, on = True):
        self.QueueMessage(self.messageMaker.EnableReactionaryBehaviors(on))

    def TurnInPlace(self, radians = 0.0, degrees = 0.0, duration = 0):
        "Positive rad turns left, negative turns right"
        angle = radians if radians else degrees * math.pi / 180
        self.QueueMessage(self.messageMaker.TurnInPlace(angle))
        dur = duration if duration else angle * 2.0
        time.sleep(dur)
        self.Stop()
        # Give himself time to stop
        time.sleep(.5)

    def MountCharger(self, objectID):
        currState = self.GetState()
        if currState.ChargerExists():
            self.QueueMessage(self.messageMaker.MountCharger(currState.charger[objectID]))

    def GetObjectImageCenter(self, object):
        state = self.GetState()
        centerX = (object.img_topLeft_x + (object.img_width/2)) 
        centerY = (object.img_topLeft_y + (object.img_height/2))
        return (centerX, centerY)

    def GetImage(self, drawViz = False):
        state = self.GetState()
        if state != None:
            return state.GetImage(drawViz)
        return None

    def ClearAllObjects(self):
        self.QueueMessage(self.messageMaker.ClearAllObjects())

    def DeleteAllObjects(self):
        self.QueueMessage(self.messageMaker.DeleteAllObjects())
        Lock(LockTypes.robotDeletedAllObjects)
        self.asyncEngineInterfaceInstance.DeleteAllObjects()

    def DeleteAllCustomObjects(self):
        self.QueueMessage(self.messageMaker.DeleteAllCustomObjects())
        Lock(LockTypes.robotDeletedAllCustomObjects)

    def _ColorLookup(self, color):
        try:
            colorHex = eval("GToE.LEDColor." + color) << 8
        except:
            colorHex = 0x00000000
            sys.stdout.write("Color lookup failed, " + color + " is not a valid LEDColor" + os.linesep)
            sys.stdout.write("""Possible colors LED_RED,LED_GREEN,LED_YELLOW,
                                LED_BLUE,LED_PURPLE,LED_CYAN,LED_WHITE,LED_IR""")

        return colorHex

    def SetBackpackLEDs(self, onColor = 0xffffffff,
                              offColor = 0, 
                              onPeriod_ms = [250,250,250,250,250], 
                              offPeriod_ms = [0,0,0,0,0], 
                              transitionOnPeriod_ms = [0,0,0,0,0], 
                              transitionOffPeriod_ms = [0,0,0,0,0]):
        """
        Sets the color values of the backpack LEDs, each field is a list of length 5, for the 5 
        different leds on the backpack. LEDs 0 and 4 are red only.
        The color format is 0xRRGGBB??
        """
        finalColors = []
        if isinstance(onColor, str):
            finalColors = [self._ColorLookup(onColor)] * 5
        elif isinstance(onColor, int):
            finalColors = [onColor] * 5
        else:
            for color in onColor:
                finalColors.append(self._ColorLookup(color) if isinstance(color,str) else color)

        if not offColor:
            offColor = finalColors
        self.QueueMessage(self.messageMaker.SetBackpackLEDs(finalColors, offColor, 
                                                            onPeriod_ms, offPeriod_ms, 
                                                            transitionOnPeriod_ms, transitionOffPeriod_ms))

    def SetActiveObjectLEDs(self, objectID, onColor = 0xffffffff, offColor = 0,
                            onPeriod_ms = 255, offPeriod_ms = 0,
                            transitionOnPeriod_ms = 0, transitionOffPeriod_ms = 0,
                            relativeToX = 0, relativeToY = 0,
                            whichLEDs = 255, makeRelative = 2, turnOffUnspecifiedLEDs = 1, robotID = 1):
        if not offColor:
            offColor = onColor
        self.QueueMessage(self.messageMaker.SetActiveObjectLEDs(objectID, onColor, offColor, onPeriod_ms, offPeriod_ms,
                                                transitionOnPeriod_ms, transitionOffPeriod_ms, relativeToX, relativeToY,
                                                whichLEDs, makeRelative, turnOffUnspecifiedLEDs, robotID))

    def EnrollNamedFace(self, name, faceID):
        self.QueueMessage(self.messageMaker.EnrollNamedFace(gIdTag, name, faceID))

    # Hard coded angle because 2*pi rounds to zero
    def TurnTowardsFace(self, faceID, maxTurnAngle_rad = math.pi):
        self.QueueMessage(self.messageMaker.TurnTowardsFace(gIdTag, faceID, maxTurnAngle_rad))

    def TurnTowardsPose(self, x_mm, y_mm, z_mm = 0, maxTurnAngle_rad= math.pi):
        self.QueueMessage(self.messageMaker.TurnTowardsPose(gIdTag, x_mm, y_mm, z_mm, maxTurnAngle_rad))

    def ObjectTypeLookup(self, name):
        try:
            type = eval("GToE.ObjectType.Custom_" + name)
        except:
            type = None
            sys.stdout.write("Type lookup failed, " + name + " is not a valid objectType" + os.linesep)
            sys.stdout.write("Possible types are STAR5, ARROW")
        return type

    def DefineCustomObject(self, objectType, xSize_mm, ySize_mm, zSize_mm,
                           markerWidth_mm = 25.0, markerHeight_mm = 25.0):
        type = self.ObjectTypeLookup(objectType)
        self.QueueMessage(self.messageMaker.DefineCustomObject(type, xSize_mm, ySize_mm, zSize_mm,
                                                               markerWidth_mm, markerHeight_mm))
        Lock(LockTypes.definedCustomObject)

class _EngineInterfaceImpl:
    "Internal interface for talking to cozmo-engine"
    
    def __init__(self, verboseLevel):
        self.useTcpConnection = True
        for arg in sys.argv:
            if arg == "-u": self.useTcpConnection = False
        self.verboseLevel = verboseLevel

        self.debugConsoleManager = DebugConsoleManager()
        self.moodManager = MoodManager()
        self.animationManager = AnimationManager()
        self.worldManager = WorldManager(EToG, GToE)

        if self.useTcpConnection:
            sys.stdout.write("Creating TCP SDK Connection" + os.linesep)

            engineIpAddress = "127.0.0.1"
            sdkTcpPort = 5106

            self.engineConnection = TcpConnection(verboseLevel, engineIpAddress, sdkTcpPort, GToEI, GToEM)
        else:
            sys.stdout.write("Creating UDP SDK Connection" + os.linesep)
            
            # [MARKW:TODO] Tidy up the mess of things that we have to manually pass into UdpConnection
            ourIpAddress = "127.0.0.1" # localhost, using raw "unreliable" udp
            engineIpAddress = "127.0.0.1"
            sdkToEngPort = 5114     # = 0x13FA, byte-swapped = 0xFA13 64019
            engToSdkPort = 5116     # = 0x13FC, byte-swapped = 0xFC13 64531        
            htons_sdkToEngPort = 64019   # [MARKW:TODO] don't hardcode this, htons in the correct places

            adPort = 5105
            
            self.engineConnection = UdpConnection(verboseLevel, engToSdkPort, GToEI, GToEM)

            adMsg = GToE.AdvertisementRegistrationMsg()
            adMsg.toEnginePort=sdkToEngPort
            adMsg.fromEnginePort=engToSdkPort
            adMsg.ip = ourIpAddress
            adMsg.id = 1
            adMsg.enableAdvertisement = True
            adMsg.oneShot = True            
            toEngAdMsg = GToEM(AdvertisementRegistrationMsg = adMsg);
            
            self.engineConnection.adDest = (engineIpAddress, adPort)
            self.engineConnection.engDest = (engineIpAddress, htons_sdkToEngPort)
            self.engineConnection.adMsg = toEngAdMsg
        
    def __del__(self):
        self.Disconnect()    
         
    def SendMessage(self, message, logIfVerbose=True):
        self.engineConnection.SendMessage(message, logIfVerbose)

    def OnConnectionAcked(self):
        "Called when we receive a confirmation message that we're connected to the Engine"
        sys.stdout.write("SDK Connected to Engine" + os.linesep)
        sys.stdout.write("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" + os.linesep)
        self.engineConnection.OnConnectionAcked()
        # Request any one-off state info (e.g. lists of anything only known at load-time)
        self.SendMessage(GToEM(GetAllDebugConsoleVarMessage    =  GToEI.GetAllDebugConsoleVarMessage()),    False)
        self.SendMessage(GToEM(RequestAvailableAnimations      =  GToEI.RequestAvailableAnimations()),      False)

    def SetVerboseLevel(self, newLevel):
        self.verboseLevel = newLevel

    def InAnimationNames(self, animName):
        return self.animationManager.InAnimationNames(animName)

    def ReceiveFromEngine(self):
    
        keepPumpingIncomingSocket = True
        while keepPumpingIncomingSocket:

            messageData, srcAddr = self.engineConnection.ReceiveMessage()

            if (messageData == None) or (len(messageData) == 0):
                # nothing left to read
                keepPumpingIncomingSocket = False
            elif ((len(messageData) == 1) and (messageData[0] == 0)):
                # special case from engine at start of connection - ignore
                if self.verboseLevel >= VerboseLevel.High:
                    sys.stdout.write("Recv: Ignoring message of 1 zero byte from " + str(srcAddr) + " = '" + str(messageData) + "'" + os.linesep)
                pass
            else:
                try:
                    fromEngMsg = EToGM.unpack(messageData)
                except Exception as e:
                    lenMessageData = len(messageData)
                    if lenMessageData > 0:
                        tag = ord(messageData[0]) if sys.version_info.major < 3 else messageData[0]
                    else:
                        tag = -1

                    if self.verboseLevel >= VerboseLevel.High:
                        sys.stderr.write("Exception unpacking message from " + str(srcAddr) + ":" + os.linesep + "  " + str(e) + os.linesep)
                        sys.stderr.write("  " + str(messageData) + " (" + str(lenMessageData) + " bytes, tag = " + str(tag) + ")" + os.linesep);
                        sys.stderr.flush()
                else:
                    
                    #sys.stdout.write("Recv: ... " + str(fromEngMsg.tag) + " = " + EToGM._tags_by_value[fromEngMsg.tag] + " from " + str(srcAddr) +  os.linesep);
                    #sys.stdout.write("     " + str(messageData) + os.linesep);

                    self.lastMsgRecvTime = time.time()
                        
                    if fromEngMsg.tag == fromEngMsg.Tag.UiDeviceConnected:
                        msg = fromEngMsg.UiDeviceConnected
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: UiDeviceConnected Type=" + str(msg.connectionType) + ", id=" + str(msg.deviceID) + ", success=" + str(msg.successful) + os.linesep)
                        if self.useTcpConnection and (msg.connectionType == Anki.Cozmo.UiConnectionType.SdkOverTcp):
                            self.OnConnectionAcked()
                        elif (not self.useTcpConnection) and (msg.connectionType == Anki.Cozmo.UiConnectionType.SdkOverUdp):
                            self.OnConnectionAcked()
                        else:
                            sys.stdout.write("Something else Connected to Engine" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.UpdateEngineState:
                        msg = fromEngMsg.UpdateEngineState
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: UpdateEngineState from " + str(msg.oldState) + " to " + str(msg.newState) + os.linesep)
                    # elif fromEngMsg.tag == fromEngMsg.Tag.RobotConnected:
                    #     msg = fromEngMsg.RobotConnected
                    #     Unlock(msg.robotID)
                    #     if self.verboseLevel >= VerboseLevel.High:
                    #         sys.stdout.write("Recv: RobotConnected id=" + str(msg.robotID) + " successful=" + str(msg.successful) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedPossibleObject:
                        msg = fromEngMsg.RobotObservedPossibleObject
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedObject:
                        msg = fromEngMsg.RobotObservedObject
                        self.worldManager._ObservedObject(msg)
                        if self.verboseLevel >= VerboseLevel.Max:
                            sys.stdout.write("Observed object: Obj type: " + str(msg.objectType) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedMotion:
                        msg = fromEngMsg.RobotObservedMotion
                        self.worldManager._ObservedMotion(msg)
                        if self.verboseLevel >= VerboseLevel.Max:
                            sys.stdout.write("Observed motion: " + str(msg.img_area) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedFace:
                        msg = fromEngMsg.RobotObservedFace
                        self.worldManager._ObservedFace(msg)
                        # if self.verboseLevel >= VerboseLevel.High:
                        #     sys.stdout.write("Observed Face: Name: " + str(msg.name) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotProcessedImage:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotChangedObservedFaceID:
                        msg = fromEngMsg.RobotChangedObservedFaceID
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotDeletedFace:
                        msg = fromEngMsg.RobotDeletedFace
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotErasedAllEnrolledFaces:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.NVStorageData:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.NVStorageOpResult:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotState:
                        msg = fromEngMsg.RobotState
                        self.worldManager._RobotState(msg)
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.ChargerEvent:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectConnectionState:
                        msg = fromEngMsg.ObjectConnectionState
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectMoved:
                        # TODO Add to world manager
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectStoppedMoving:
                        # TODO Add to world manager
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.Ping:
                        self.engineConnection.HandlePingMessage(fromEngMsg.Ping)
                    elif fromEngMsg.tag == fromEngMsg.Tag.DebugString:
                        #From Robot::SendDebugString() summary of moving (lift/head/body), carrying, simpleMood, imageProc framerate and behavior
                        msg = fromEngMsg.DebugString
                        #sys.stdout.write("Recv: DebugString = '" + str(msg.text) + "'" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.AnimationAvailable:
                        msg = fromEngMsg.AnimationAvailable
                        self.animationManager.UpdateAnimations(msg)
                    elif fromEngMsg.tag == fromEngMsg.Tag.InitDebugConsoleVarMessage:
                        # multiple of these arrive after we send GetAllDebugConsoleVarMessage (they don't all fit in 1 message)
                        msg = fromEngMsg.InitDebugConsoleVarMessage
                        self.debugConsoleManager.HandleInitDebugConsoleVarMessage(msg)
                    elif fromEngMsg.tag == fromEngMsg.Tag.VerifyDebugConsoleVarMessage:
                        msg = fromEngMsg.VerifyDebugConsoleVarMessage
                        if msg.success:
                            sys.stdout.write("Recv: ConsoleVar Success: '" + msg.varName + "' = '" + msg.statusMessage + "'" + os.linesep)
                            consoleVar = self.debugConsoleManager.GetVar(msg.varName)
                            if consoleVar != None:
                                consoleVar.varValue = msg.varValue
                        else:
                            sys.stdout.write("Recv: ConsoleVar Failure: '" + msg.varName + "' " + msg.statusMessage + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.VerifyDebugConsoleFuncMessage:
                        msg = fromEngMsg.VerifyDebugConsoleFuncMessage                         
                        if msg.success:
                            sys.stdout.write("Recv: ConsoleFunc Success: '" + msg.funcName + "':" + os.linesep + msg.statusMessage)
                        else:
                            sys.stdout.write("Recv: ConsoleFunc Failure: '" + msg.funcName + "': " + msg.statusMessage)
                    elif fromEngMsg.tag == fromEngMsg.Tag.MoodState:
                        msg = fromEngMsg.MoodState
                        self.moodManager.UpdateMoodState(msg)         
                    elif fromEngMsg.tag == fromEngMsg.Tag.LatencyMessage:
                        # [MARKW:TODO], update this locally for anything that wants to query engine<->robot latency
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.UnlockStatus:
                        msg = fromEngMsg.UnlockStatus
                        #sys.stdout.write("Recv: UnlockStatus length=" + str(len(msg.unlocks)) + " c= " + str(msg.unlocks) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.AnimationAborted:
                        msg = fromEngMsg.AnimationAborted                        
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: AnimationAborted tag = " + str(msg.tag) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotCompletedAction:
                        msg = fromEngMsg.RobotCompletedAction                        
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: RobotCompletedAction robot="+str(msg.robotID)+", tag="+str(msg.idTag)+", actionType="+str(msg.actionType)+", res="+str(msg.result)+", completionInfo="+str(msg.completionInfo) + os.linesep)
                        Unlock(msg.idTag)
                    elif fromEngMsg.tag == fromEngMsg.Tag.DebugAnimationString:
                        # Current animation playing (set to "" when done)
                        msg = fromEngMsg.DebugAnimationString                        
                        #sys.stdout.write("Recv: DebugAnimationString '" + msg.text + "'" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.EngineRobotCLADVersionMismatch:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.EngineRobotCLADVersionMismatch:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.ImageChunk:
                        msg = fromEngMsg.ImageChunk
                        self.worldManager._ImageChunk(msg)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotOnBack:
                        Unlock(LockTypes.onBack)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotDeletedAllObjects:
                        Unlock(LockTypes.robotDeletedAllObjects)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotDeletedAllCustomObjects:
                        Unlock(LockTypes.robotDeletedAllCustomObjects)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotPoked:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.DefinedCustomObject:
                        Unlock(LockTypes.definedCustomObject)
                    elif fromEngMsg.tag == fromEngMsg.Tag.CreatedFixedCustomObject:
                        Unlock(LockTypes.createdFixedCustomObject)
                    else:
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: Unhandled " + str(fromEngMsg.tag) + " = " + EToGM._tags_by_value[fromEngMsg.tag] + " from " + str(srcAddr) +  os.linesep);
                            sys.stdout.write("     " + str(messageData) + os.linesep);
                            sys.stdout.write("     " + repr(fromEngMsg) + os.linesep);
                            
                    sys.stdout.flush()
                    
    def Update(self):
        "Update internals (currently just the network connection)"
        self.engineConnection.Update()
        self.ReceiveFromEngine()
        global gState
        if len(gState) == 0:
            gState.append(self.worldManager)
        else:
            gState = [self.worldManager] + gState
            gState = gState[:2]
                
    def IsConnected(self):
        "Are we sucessfully connected via a network connection"
        return self.engineConnection.IsConnected()
        
    def Disconnect(self):
        "Disconnect any network connections"
        self.engineConnection.Disconnect()

    def ResetConnection(self):
        "Try to connect to engine again on the current connection"
        sys.stdout.write("Forcing ResetConnection..." + os.linesep)
        self.engineConnection.ResetConnection()

    def DeleteAllObjects(self):
        self.worldManager.DeleteAllObjects()


class AsyncEngineInterface(threading.Thread):
    "Async Wrapper of EngineInterface"
    
    def __init__(self, verboseLevel):
        self.engineInterface =  _EngineInterfaceImpl(verboseLevel)
        if os.name == 'posix' and sys.version_info.major > 2:
            threading.Thread.__init__(self, daemon=True)
        else:
            threading.Thread.__init__(self)
        self.queue = deque([]) # deque of Messages in the order they were input
        self.queueLock = threading.Lock()
        self.updateLock = threading.Lock()
        self._continue = True
        
    def __del__(self):
        self.KillThread()
        if self.engineInterface is not None:
            del self.engineInterface
            self.engineInterface = None        
            
    def run(self):
        while self._continue:
            try:
                self.updateLock.acquire()
                try:
                    message = self.PopMessage()
                    if message is not None:
                        if type(message) is list:
                            for msg in message:
                                self.engineInterface.SendMessage(msg)
                        else:
                            self.engineInterface.SendMessage(message)

                    self.engineInterface.Update()
                except Exception as e:
                    sys.stderr.write("[AsyncEngineInterface.Update] Exception: " + str(e) + os.linesep)

                self.updateLock.release()
                time.sleep(0.01)
            except Exception as e:
                sys.stderr.write("[AsyncEngineInterface.Run] Exception: " + str(e) + os.linesep)

    def KillThread(self):
        "Clean up the thread"
        self.queueLock.acquire()
        self._continue = False
        self.queue = []
        self.queueLock.release()
        
    def QueueMessage(self, inMessage):
        "Thread safe queing of an Async Message, will run later in update loop"
        # [MARKW:TODO] would be great to get a unique ID (monotonically increasing?) back that can be queried
        #              e.g. get an OnComplete() callback, or do IsComplete(taskID)
        #sys.stdout.write("[AsyncEngineInterface.QueueMessage] '" + str(inMessage) + "'" + os.linesep)
        self.queueLock.acquire()
        self.queue.append(inMessage)
        self.queueLock.release()
        
    def SyncMessage(self, inMessage):
        "Thread safe blocking to call a Message synchronously"
        self.updateLock.acquire()
        try:
            retVal = inMessage()
        except Exception as e:
            retVal = None
            sys.stderr.write("[AsyncEngineInterface.SyncMessage] Exception: " + str(e) + os.linesep)
        self.updateLock.release()
        return retVal
        
    def PopMessage(self):
        "Thread safe pop of oldest Message queued"
        self.queueLock.acquire()
        try:
            poppedMessage = self.queue.popleft()
        except IndexError:
            poppedMessage = None
        self.queueLock.release()
        return poppedMessage

    def IsConnected(self):
        return self.engineInterface.IsConnected()

    def SetVerboseLevel(self, newLevel):
        self.engineInterface.SetVerboseLevel(newLevel)

    def InAnimationNames(self, animName):
        return self.engineInterface.InAnimationNames(animName)

    def DeleteAllObjects(self):
        self.engineInterface.DeleteAllObjects()


def Lock(lockType = 0):
    # TODO make locks optional so it could possibly be run in the background
    # Maybe make it so a callback could be passed in instead, rename WaitUntilComplete
    global gKey
    global gIdTag
    if lockType:
        gKey = lockType
    else:
        gKey = gIdTag
    while gKey:
        time.sleep(.05)
        continue
    gIdTag += 1

def Unlock(idTag):
    global gKey
    if gKey == idTag:
        gKey = 0

class LockTypes():
    "An enum for storing a few types of locks (so its not random numbers)"
    onBack = 1
    robotDeletedAllObjects = 2
    robotDeletedAllCustomObjects = 3
    createdFixedCustomObject = 4
    definedCustomObject = 5
