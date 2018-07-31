"""
SDK Interface to Victor-Engine, supports communication over either a localhost UDP socket, or a TCP socket
All communication is via CLAD messages.
"""
__author__ = "Mark Wesley"

import sys, os, time
import threading # for threaded engine
from collections import deque # queue for threaded engine
from debugConsole import DebugConsoleManager
from moodManager import MoodManager
from animationManager import AnimationManager
from tcpConnection import TcpConnection
from udpConnection import UdpConnection
from verboseLevel import VerboseLevel

try:
    from clad.externalInterface.messageEngineToGame import Anki
    from clad.externalInterface.messageGameToEngine import Anki as _Anki
except Exception as e:
    sys.stdout.write("Exception = " + str(e)  + os.linesep)
    sys.exit("Can't import CLAD libraries!{linesep}\t* Are you running from the base SDK directory?{linesep}".format(linesep=os.linesep))

# [MARKW:TODO] i don't know why robot interface does this?
#Anki.update(_Anki.deep_clone())

# namespace shortcuts
EToG  =  Anki.Victor
GToE  = _Anki.Victor
EToGI = EToG.ExternalInterface
GToEI = GToE.ExternalInterface
EToGM = EToGI.MessageEngineToGame
GToEM = GToEI.MessageGameToEngine


gEngineInterfaceInstance = None
gAsyncEngineInterfaceInstance = None


class EngineCommand:
    "An enum of Command types that EngineInterface can perform"
    consoleVar         = 0
    consoleFunc        = 1
    sendMsg            = 2
    setEmotion         = 3
    playAnimation      = 4
    playAnimationGroup = 5
    setVerboseLevel    = 6
    resetConnection    = 7


def ArgListToString(*args):
    "Collapse an array of arguments into a space-separated string"
    argsToString = ""
    for i in range(0, len(args)):
        if i > 0:
            argsToString += " "
        argsToString += args[i]
    return argsToString


# ================================================================================    
# Internal Private _EngineInterfaceImpl for talking to/from victor-engine
# ================================================================================


class _EngineInterfaceImpl:
    "Internal interface for talking to victor-engine"
    
    def __init__(self, useTcpConnection, verboseLevel):

        self.useTcpConnection = useTcpConnection;
        self.verboseLevel = verboseLevel

        if self.useTcpConnection:
            sys.stdout.write("Creating TCP SDK Connection" + os.linesep)

            engineIpAddress = "127.0.0.1"
            sdkTcpPort = 5107

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
        
        self.debugConsoleManager = DebugConsoleManager()
        self.moodManager = MoodManager()
        self.animationManager = AnimationManager()

        
        
    def __del__(self):
        self.Disconnect()    
         

    def OnConnectionAcked(self):
        "Called when we receive a confirmation message that we're connected to the Engine"
        sys.stdout.write("SDK Connected to Engine" + os.linesep)
        self.engineConnection.OnConnectionAcked()
        # Request any one-off state info (e.g. lists of anything only known at load-time)
        self.HandleSendMessageCommand(*["GetAllDebugConsoleVarMessage"])
        self.HandleSendMessageCommand(*["RequestAvailableAnimations"])
        self.HandleSendMessageCommand(*["RequestAvailableAnimationGroups"])


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
                        if self.useTcpConnection and (msg.connectionType == Anki.Victor.UiConnectionType.SdkOverTcp):
                            self.OnConnectionAcked()
                        elif (not self.useTcpConnection) and (msg.connectionType == Anki.Victor.UiConnectionType.SdkOverUdp):
                            self.OnConnectionAcked()
                        else:
                            sys.stdout.write("Something else Connected to Engine" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.UpdateEngineState:
                        msg = fromEngMsg.UpdateEngineState
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: UpdateEngineState from " + str(msg.oldState) + " to " + str(msg.newState) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotConnected:
                        msg = fromEngMsg.RobotConnected
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: RobotConnected id=" + str(msg.robotID) + " successful=" + str(msg.successful) + os.linesep)                        
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedPossibleObject:
                        msg = fromEngMsg.RobotObservedPossibleObject
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedObject:
                        msg = fromEngMsg.RobotObservedObject
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedMotion:
                        msg = fromEngMsg.RobotObservedMotion
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedFace:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotProcessedImage:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotChangedObservedFaceID:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotDeletedFace:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotErasedAllEnrolledFaces:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.NVStorageData:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.NVStorageOpResult:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotState:
                        msg = fromEngMsg.RobotState
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.ChargerEvent:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectConnectionState:
                        msg = fromEngMsg.ObjectConnectionState
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectMoved:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectStoppedMoving:
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
                    elif fromEngMsg.tag == fromEngMsg.Tag.AnimationGroupAvailable:
                        msg = fromEngMsg.AnimationGroupAvailable
                        self.animationManager.UpdateAnimationGroups(msg)
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
                        msg = fromEngMsg.LatencyMessage
                        # sys.stdout.write("wifiLatency = " + str(msg.wifiLatency) + os.linesep);
                        # sys.stdout.write("extSendQueueTime = " + str(msg.extSendQueueTime) + os.linesep);
                        # sys.stdout.write("sendQueueTime = " + str(msg.sendQueueTime) + os.linesep);
                        # sys.stdout.write("recvQueueTime = " + str(msg.recvQueueTime) + os.linesep);
                        # sys.stdout.write("unityEngineLatency = " + str(msg.unityEngineLatency) + os.linesep);
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
                    elif fromEngMsg.tag == fromEngMsg.Tag.DebugAnimationString:
                        # Current animation playing (set to "" when done)
                        msg = fromEngMsg.DebugAnimationString                        
                        #sys.stdout.write("Recv: DebugAnimationString '" + msg.text + "'" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.EngineRobotCLADVersionMismatch:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.EngineRobotCLADVersionMismatch:
                        pass
                    else:
                        if self.verboseLevel >= VerboseLevel.High:
                            sys.stdout.write("Recv: Unhandled " + str(fromEngMsg.tag) + " = " + EToGM._tags_by_value[fromEngMsg.tag] + " from " + str(srcAddr) +  os.linesep);
                            sys.stdout.write("     " + str(messageData) + os.linesep);
                            sys.stdout.write("     " + repr(fromEngMsg) + os.linesep);
                            
                    sys.stdout.flush()
                
                
    def HandleConsoleVarCommand(self, *args):
    
        if (len(args) == 0):
            sys.stderr.write("[HandleConsoleVarCommand] Error: no args, expected 'list' or variable name" + os.linesep)
            return False
        
        varName = args[0]
        varNameLower = varName.lower()
        
        if varNameLower == 'list':        
            return self.debugConsoleManager.ListConsoleVars()
        elif varNameLower == 'get_matching_names':
            return self.debugConsoleManager.GetMatchingVarNames(args[1])
        else:
            consoleVar = self.debugConsoleManager.GetVar(varName)
            if consoleVar == None:
                sys.stderr.write("[HandleConsoleVarCommand] Error - unknown variable '" + varName + "'" + os.linesep)
                return False

            sys.stdout.write("Var " + str(varName) + " was: " + consoleVar.varValueToString() + " (" + consoleVar.varValueTypeName() + ")(" + consoleVar.minValueToString() + ".." + consoleVar.maxValueToString() + ")" + os.linesep)
                                                    
            setVarMsg = GToEI.SetDebugConsoleVarMessage()
            setVarMsg.varName  = varName
            setVarMsg.tryValue = ArgListToString(*args[1:])
    
            toEngMessage = GToEM(SetDebugConsoleVarMessage = setVarMsg);
                
            sys.stdout.write("Send: " + str(setVarMsg) + os.linesep)
            self.engineConnection.SendMessage(toEngMessage, False)
            
            return True
            
            
    def HandleConsoleFuncCommand(self, *args):
    
        if (len(args) == 0):
            sys.stderr.write("[HandleConsoleFuncCommand] Error: no args, expected 'list' or function name" + os.linesep)
            return False
        
        funcName = args[0]
        funcNameLower = funcName.lower()
        
        if funcNameLower == 'list':
            return self.debugConsoleManager.ListConsoleFuncs()
        elif funcNameLower == 'get_matching_names':
            return self.debugConsoleManager.GetMatchingFuncNames(args[1])
        else:
            consoleFunc = self.debugConsoleManager.GetFunc(funcName)
            if consoleFunc == None:
                sys.stderr.write("[HandleConsoleFuncCommand] Error - unknown function '" + funcName + "'" + os.linesep)
                return False
                                        
            runFuncMsg = GToEI.RunDebugConsoleFuncMessage()
            runFuncMsg.funcName = funcName
            runFuncMsg.funcArgs = ArgListToString(*args[1:])
    
            toEngMessage = GToEM(RunDebugConsoleFuncMessage = runFuncMsg);
                
            sys.stdout.write("Send: " + str(runFuncMsg) + os.linesep)
            self.engineConnection.SendMessage(toEngMessage, False)
            
            return True
        
            
    def HandleHelpSpecificMessage(self, cmd, oneLine = False): 
        if not hasattr(GToEM, cmd):
            sys.stdout.write("No message of type '{}'{}".format(cmd, os.linesep))
            return False

        msgTag = getattr(GToEM.Tag, cmd)
        msgType = GToEM.typeByTag(msgTag)

        # grab the arguments passed into the init function, excluding "self"
        try:
            args = msgType.__init__.__code__.co_varnames[1:]
        except AttributeError:
            # in case it's a primitive type like 'int'
            #TODO: once we have enum to string, use that here and list possible values
            args = [msgType.__name__]

        defaults = []
        try:
            defaults = msgType.__init__.__defaults__
        except AttributeError:
            pass

        if not oneLine:
            print("{}: {} command".format(cmd, msgType.__name__))
            print("usage: {} {}".format(cmd, ' '.join(args)))
            if len(args) > 0:
                print()

            for argnum in range(len(args)):
                arg = args[argnum]
                if len(defaults) > argnum:
                    defaultStr = "default = {}".format(defaults[argnum])
                else:
                    defaultStr = ""

                typeStr = ""
                try:
                    doc = getattr(msgType, arg).__doc__
                except AttributeError:
                    pass
                else:
                    if doc:
                        docSplit = doc.split()
                        if len(docSplit) > 0:
                            typeStr = docSplit[0]

                    print("    {}: {} {}".format(arg, typeStr, defaultStr))
        else:
            print ("  {} {}".format(cmd, ' '.join(args)))

        return True
    
    def HandleSendMessageCommand(self, *args):
    
        if (len(args) == 0):
            sys.stderr.write("[HandleSendMessageCommand] Error: no args, expected 'list' or message name" + os.linesep)
            return False
            
        msgName = args[0]
        
        if msgName.lower() == "list":
            for cmd in GToEM._tags_by_name:
                self.HandleHelpSpecificMessage(cmd, oneLine = True)
                
            return True
    
        if not hasattr(GToEM, msgName):
            sys.stderr.write("[HandleSendMessageCommand] Unrecognized message + '" + str(msgName) + "'" + os.linesep)            
            return False
            
        commandArgs = args[1:]
    
        try:
            params = [eval(a) for a in commandArgs]
        except Exception as e:
            sys.stderr.write("Couldn't parse command arguments for '{0}':{2}\t{1}{2}".format(msgName, e, os.linesep))
            self.HandleHelpSpecificMessage(msgName, oneLine = False)
            return False
        else:
            t = getattr(GToEM.Tag, msgName)
            y = GToEM.typeByTag(t)                    
            try:
                p = y(*params)
            except Exception as e:
                sys.stderr.write("Couldn't create {0} message from params *{1}:{3}\t{2}{3}".format(msgName, repr(params), str(e), os.linesep))
                self.HandleHelpSpecificMessage(msgName, oneLine = False)
                return False
            else:
                m = GToEM(**{msgName: p})
                #sys.stdout.write("[HandleSendMessageCommand] Sending: '" + str(m) + "'" + os.linesep)
                return self.engineConnection.SendMessage(m)
                
        return False
    
    def HandlePlayAnimationCommand(self, *args):
        
        lenArgs = len(args)
        playAnimMsg = GToEI.PlayAnimation()
        playAnimMsg.robotID = 1

        if (lenArgs < 1):
            sys.stderr.write("[HandlePlayAnimationCommand] Error: No args, expected 'list' or 'animationName optional_numLoops'" + os.linesep)
            return False

        animName = args[0]

        if animName.lower() == "list":
            self.animationManager.PrintAnimations()
            return True

        if animName.lower() == "get_matching_names":
            return self.animationManager.GetMatchingAnimNames(args[1])

        if not self.animationManager.InAnimationNames(animName):
            sys.stderr.write("[PlayAnimation] Error: Animation requested not in list of possible animations" + os.linesep)
            return False

        playAnimMsg.animationName = animName
        
        playAnimMsg.numLoops = int(args[1]) if (lenArgs > 1) else 1

        toEngMessage = GToEM(PlayAnimation = playAnimMsg);
        
        sys.stdout.write("Send PlayAnimation = '" + str(playAnimMsg) + "'" + os.linesep)
        self.engineConnection.SendMessage(toEngMessage, False)
        
    def HandlePlayAnimationGroupCommand(self, *args):
        lenArgs = len(args)
        playAnimGroupMsg = GToEI.PlayAnimationGroup()
        playAnimGroupMsg.robotID = 1

        if (lenArgs < 1):
            sys.stderr.write("[HandlePlayAnimationGroupCommand] Error: No args, expected 'list' or 'animationGroupName optional_numLoops'" + os.linesep)
            return False

        animGroupName = args[0]
        if animGroupName.lower() == "list":
            self.animationManager.PrintAnimationGroups()
            return True

        if animGroupName.lower() == "get_matching_names":
            return self.animationManager.GetMatchingAnimGroupNames(args[1])

        if not self.animationManager.InAnimationGroupNames(animGroupName):
            sys.stderr.write("[PlayAnimationGroup] Error: Animation Group requested not in list of possible animation groups" + os.linesep)
            return False

        playAnimGroupMsg.animationGroupName = animGroupName
        
        playAnimGroupMsg.numLoops = int(args[1]) if (lenArgs > 1) else 1

        toEngMessage = GToEM(PlayAnimationGroup = playAnimGroupMsg);
        
        sys.stdout.write("Send PlayAnimationGroup = '" + str(playAnimGroupMsg) + "'" + os.linesep)
        self.engineConnection.SendMessage(toEngMessage, False)


    def HandleSetEmotionCommand(self, *args):
    
        lenArgs = len(args)
        
        if (lenArgs == 0):
            sys.stderr.write("[HandleSetEmotionCommand] Error: no args, expected 'list' or 'emotionName newValue'" + os.linesep)
            return False

        emotionName = args[0]
        
        if emotionName.lower() == "list":
            self.moodManager.PrintEmotions()
            return True
            
        emotionType = self.moodManager.EmotionNameToType(emotionName)
        if emotionType == None:
            sys.stderr.write("[HandleSetEmotionCommand] Error: unknown emotion '" + emotionName + "'!" + os.linesep)
            return False
        
        oldEmotionValue = self.moodManager.GetEmotionValue(emotionType)

        if (lenArgs == 1):
            sys.stdout.write("emotion " + emotionName + " = " + str(oldEmotionValue) + " (no newValue provided)" + os.linesep)
            return True
            
        newValue = args[1]
        
        sys.stdout.write("emotion " + emotionName + " was = " + str(oldEmotionValue) + os.linesep)
        
        setEmotionMsg = GToEI.SetEmotion()
        setEmotionMsg.emotionType = emotionType
        setEmotionMsg.newVal = newValue
        
        moodMessage = GToEI.MoodMessage()        
        moodMessage.robotID = 1
        moodMessage.MoodMessageUnion = GToEI.MoodMessageUnion()
        moodMessage.MoodMessageUnion.SetEmotion = setEmotionMsg

        toEngMessage = GToEM(MoodMessage = moodMessage);
    
        self.engineConnection.SendMessage(toEngMessage)
                        
        return True

    def SetVerboseLevel(self, newLevel):
        if (newLevel >= 0) and (newLevel <= VerboseLevel.Max):
            self.verboseLevel = newLevel
            self.engineConnection.verboseLevel = newLevel
            return True 
        else:
            sys.stderr.write("[SetVerboseLevel] newLevel '" + str(newLevel) + "' out of range" + os.linesep)
            return False
        
    def HandleSetVerboseLevel(self, *args):
        success = False
        if (len(args) > 0):
            try:
                verboseLevel = int(args[0])
                success = self.SetVerboseLevel(verboseLevel)
            except:
                sys.stderr.write("[HandleSetVerboseLevel] Error parsing '" + str(args[0]) + "' to an int" + os.linesep)
        if not success:
            sys.stderr.write("[HandleSetVerboseLevel] Improper usage - requires an int in range 0.." + str(VerboseLevel.Max) + " range " + os.linesep)
        return success
        
    def DoCommand(self, inCommand):
        "Perform a single command"
        commandType = inCommand[0]
                
        if (commandType == EngineCommand.consoleVar):
            return self.HandleConsoleVarCommand(*inCommand[1])
        elif (commandType == EngineCommand.consoleFunc):
            return self.HandleConsoleFuncCommand(*inCommand[1])
        elif (commandType == EngineCommand.sendMsg):
            return self.HandleSendMessageCommand(*inCommand[1])
        elif (commandType == EngineCommand.setEmotion):
            return self.HandleSetEmotionCommand(*inCommand[1])
        elif (commandType == EngineCommand.playAnimation):
            return self.HandlePlayAnimationCommand(*inCommand[1])
        elif (commandType == EngineCommand.playAnimationGroup):
            return self.HandlePlayAnimationGroupCommand(*inCommand[1])
        elif (commandType == EngineCommand.setVerboseLevel):
            return self.HandleSetVerboseLevel(*inCommand[1])
        elif (commandType == EngineCommand.resetConnection):
            return self.ResetConnection()
        else:
            sys.stderr.write("[DoCommand] Unhandled commande type: " + str(commandType) + os.linesep)
            return False
        
    def Update(self):
        "Update internals (currently just the network connection)"
        self.engineConnection.Update()
        self.ReceiveFromEngine()
                
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


# ================================================================================    
# Threaded Async Engine Interface (runs async)
# ================================================================================


class AsyncEngineInterface(threading.Thread):
    "Async Wrapper of EngineInterface"
    
    def __init__(self, inEngineInterface):
        self.engineInterface = inEngineInterface
        if os.name == 'posix' and sys.version_info.major > 2:
            threading.Thread.__init__(self, daemon=True)
        else:
            threading.Thread.__init__(self)
        self.queue = deque([]) # deque of commands in the order they were input
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
                    newCommand = self.PopCommand()
                    if newCommand is not None:
                        sys.stdout.write(os.linesep) # clean new line after the CLI prompt
                        self.engineInterface.DoCommand(newCommand)
                        
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
        
    def QueueCommand(self, inCommand):
        "Thread safe queing of an Async command, will run later in update loop"
        # [MARKW:TODO] would be great to get a unique ID (monotonically increasing?) back that can be queried
        #              e.g. get an OnComplete() callback, or do IsComplete(taskID)
        #sys.stdout.write("[AsyncEngineInterface.QueueCommand] '" + str(inCommand) + "'" + os.linesep)
        self.queueLock.acquire()
        self.queue.append(inCommand)
        self.queueLock.release()
        
    def SyncCommand(self, inCommand):
        "Thread safe blocking to call a command synchronously"
        self.updateLock.acquire()
        try:
            retVal = self.engineInterface.DoCommand(inCommand)
        except Exception as e:
            retVal = None
            sys.stderr.write("[AsyncEngineInterface.SyncCommand] Exception: " + str(e) + os.linesep)
        self.updateLock.release()
        return retVal
        
    def PopCommand(self):
        "Thread safe pop of oldest command queued"
        self.queueLock.acquire()
        try:
            poppedCommand = self.queue.popleft()
        except IndexError:
            poppedCommand = None
        self.queueLock.release()
        return poppedCommand;


# ================================================================================    
# Public engineInterface:
# ================================================================================

        
def Init(runAsync, useTcpConnection, verboseLevel):
    "Initalize the engine interface. Must be called before any other methods"
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        sys.stderr.write("Already Initialized!")
        return None        
    if gEngineInterfaceInstance is None:
        gEngineInterfaceInstance = _EngineInterfaceImpl(useTcpConnection, verboseLevel)
    if runAsync:
        global gAsyncEngineInterfaceInstance
        if gAsyncEngineInterfaceInstance is None:            
            gAsyncEngineInterfaceInstance = AsyncEngineInterface(gEngineInterfaceInstance)
            gAsyncEngineInterfaceInstance.start()
        return gAsyncEngineInterfaceInstance
    else:   
        return gEngineInterfaceInstance
    
def Update():
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        global gAsyncEngineInterfaceInstance
        if gAsyncEngineInterfaceInstance is None:
            sys.stderr.write("engineInterface.Update cannot be manually called for async engine")
        else:
            gEngineInterfaceInstance.Update()
        
def QueueCommand(inCommand):
    global gAsyncEngineInterfaceInstance
    global gEngineInterfaceInstance
    if gAsyncEngineInterfaceInstance is not None:
        gAsyncEngineInterfaceInstance.QueueCommand(inCommand)
    elif gEngineInterfaceInstance is not None:
        gEngineInterfaceInstance.DoCommand(inCommand)
        
def SyncCommand(inCommand):
    global gAsyncEngineInterfaceInstance
    global gEngineInterfaceInstance
    if gAsyncEngineInterfaceInstance is not None:
        return gAsyncEngineInterfaceInstance.SyncCommand(inCommand)
    elif gEngineInterfaceInstance is not None:
        return gEngineInterfaceInstance.DoCommand(newCommand)

def Shutdown():
    global gAsyncEngineInterfaceInstance
    global gEngineInterfaceInstance
    if gAsyncEngineInterfaceInstance is not None:
        del gAsyncEngineInterfaceInstance
        gAsyncEngineInterfaceInstance = None
    if gEngineInterfaceInstance is not None:
        del gEngineInterfaceInstance
        gEngineInterfaceInstance = None
    
def IsConnected():
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        return gEngineInterfaceInstance.IsConnected()
    else:
        return False    
