"""
Utility classes for interfacing with the Cozmo engine through a localhost UDP socket, by exchanging CLAD messages.
"""
__author__ = "Mark Wesley"

import sys, os, time
import threading # for threaded engine
from collections import deque # queue for threaded engine
from debugConsole import DebugConsoleManager
from moodManager import MoodManager

CLAD_SRC  = os.path.join("clad")
CLAD_DIR  = os.path.join("generated", "cladPython")

# Following ensures CLAD files are up-to-date for python
# We probably want to move this to the general configure script
# definitely no reason to ship this bit
if os.path.isfile(os.path.join(CLAD_SRC, "Makefile")):
    import subprocess
    make = subprocess.Popen(["make", "python", "-C", "clad"])
    if make.wait() != 0:
        sys.exit("Could't build/update python clad, exit status {:d}".format(make.wait(), linesep=os.linesep))

sys.path.insert(0, CLAD_DIR)

try:
    sys.path.append('robot/tools/') # [MARKW:TODO] Share ReliableTransport more cleanly between sdk and robot-tools
    from ReliableTransport import * # [MARKW:TODO] Just need UDPTransport? only using it for localhost socket?
except:
    sys.exit("Can't import ReliableTransport!{linesep}\t* Are you running from the base cozmo-engine directory?{linesep}".format(linesep=os.linesep))

try:
    from clad.externalInterface.messageEngineToGame import Anki
    from clad.externalInterface.messageGameToEngine import Anki as _Anki
except Exception as e:
    sys.stdout.write("Exception = " + str(e)  + os.linesep)
    sys.exit("Can't import CLAD libraries!{linesep}\t* Are you running from the base cozmo-engine directory?{linesep}".format(linesep=os.linesep))

# [MARKW:TODO] i don't know why robot interface does this?
#Anki.update(_Anki.deep_clone())

# namespace shortcuts
EToG  =  Anki.Cozmo
GToE  = _Anki.Cozmo
EToGI = EToG.ExternalInterface
GToEI = GToE.ExternalInterface
EToGM = EToGI.MessageEngineToGame
GToEM = GToEI.MessageGameToEngine


gEngineInterfaceInstance = None
gAsyncEngineInterfaceInstance = None


class EngineCommand:
    ""
    consoleVar  = 0
    consoleFunc = 1
    sendMsg     = 2
    setEmotion  = 3
    
    
    
class ConnectionState:
    "An enum for connection states"
    notConnected      = 0
    tryingToConnect   = 1
    connected         = 2
    startedEngine     = 3
    addedRobot        = 4
    requestedVars     = 5
    connectingToRobot = 6    
    connectedToRobot  = 7


def ArgListToString(*args):
    "Collapse an array of arguments into a space-separated string"
    argsToString = ""
    for i in range(0, len(args)):
        if i > 0:
            argsToString += " "
        argsToString += args[i]
    return argsToString
        
        
# ================================================================================    
# Internal Private _EngineInterfaceImpl for talking to/from cozmo-engine
# ================================================================================


class _EngineInterfaceImpl:
    "Internal interface for talking to cozmo-engine"
    
    def __init__(self):
        self.udpTransport = UDPTransport(None, None, False)
        self.ipAddress = "127.0.0.1" # localhost, using raw "unreliable" udp
        self.engineIpAddress = "127.0.0.1"
        self.sdkToEngPort = 5114     # = 0x13FA, byte-swapped = 0xFA13 64019
        self.engToSdkPort = 5116     # = 0x13FC, byte-swapped = 0xFC13 64531        
        htons_sdkToEngPort = 64019   # [MARKW:TODO] don't hardcode this, htons in the correct places
        self.engDest = (self.engineIpAddress, htons_sdkToEngPort)        
        self.adPort = 5105
        self.adDest = (self.engineIpAddress, self.adPort)
        self.udpTransport.OpenSocket(self.engToSdkPort)
        self.state = ConnectionState.notConnected
        
        self.numAnims = 0
        
        self.verboseAd = False
        self.verboseEngine = True
        
        self.debugConsoleManager = DebugConsoleManager()
        self.moodManager = MoodManager()
        
        
    def __del__(self):
        self.Disconnect()    
         

    def ReceiveFromEngine(self):
    
        keepPumpingIncomingSocket = True
        while keepPumpingIncomingSocket:
            d, a = self.udpTransport.ReceiveData()
            if (d == None):
                # nothing left to read
                keepPumpingIncomingSocket = False
            elif ((len(d) == 1) and (d[0] == 0)):
                # special case from engine at start of connection - ignore
                #sys.stdout.write("Recv: 1 zero byte from " + str(a) + os.linesep + "     " + str(d) + os.linesep)
                pass
            else:
                buffer = d    
                try:
                    fromEngMsg = EToGM.unpack(buffer)
                except Exception as e:
                    sys.stderr.write("Exception unpacking message from " + str(a) + os.linesep)
                    sys.stderr.write("     " + str(d) + os.linesep);
                    if len(buffer):
                        tag = ord(buffer[0]) if sys.version_info.major < 3 else buffer[0]
                        sys.stderr.write("Error decoding incoming message {0:02x}[{1:d}]{linesep}\t{2:s}{linesep}".format(tag, len(buffer), str(e), linesep=os.linesep))
                    else:
                        sys.stderr.write("Got 0 length message!" + os.linesep)
                    sys.stderr.flush()
                else:
                    
                    #sys.stdout.write("Recv: ... " + str(fromEngMsg.tag) + " = " + EToGM._tags_by_value[fromEngMsg.tag] + " from " + str(a) +  os.linesep);
                    #sys.stdout.write("     " + str(d) + os.linesep);
                        
                    #if msg.tag == msg.Tag.AdvertisementMsg:
                    #    msgAdv = msg.AdvertisementMsg
                    #    sys.stdout.write("  GToEM: Adv, id = " + str(msgAdv.id) + os.linesep)
                    #    sys.stdout.write("  GToEM: Adv, port = " + str(msgAdv.port) + os.linesep)
                    #    sys.stdout.write("  GToEM: Adv, protocol = " + str(msgAdv.protocol) + os.linesep)
                    #    sys.stdout.write("  GToEM: Adv, ip = " + msgAdv.ip + os.linesep)
                    if fromEngMsg.tag == fromEngMsg.Tag.UiDeviceConnected:
                        msg = fromEngMsg.UiDeviceConnected 
                        sys.stdout.write("Recv: UiDeviceConnected Type=" + str(msg.connectionType) + ", id=" + str(msg.deviceID) + ", success=" + str(msg.successful) + os.linesep)
                        if msg.connectionType == Anki.Cozmo.UiConnectionType.SDK:
                            sys.stdout.write("SDK Connected to Engine!" + os.linesep)
                            self.state = ConnectionState.connected
                        else:
                            sys.stdout.write("Something else Connected to Engine" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.UpdateEngineState:
                        msg = fromEngMsg.UpdateEngineState
                        sys.stdout.write("Recv: UpdateEngineState from " + str(msg.oldState) + " to " + str(msg.newState) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotConnected:
                        msg = fromEngMsg.RobotConnected
                        sys.stdout.write("Recv: RobotConnected id=" + str(msg.robotID) + " successful=" + str(msg.successful) + os.linesep)
                        if self.state == ConnectionState.connectingToRobot:
                            self.state = ConnectionState.connectedToRobot;
                        
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedNothing:
                        msg = fromEngMsg.RobotObservedNothing
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedPossibleObject:
                        msg = fromEngMsg.RobotObservedPossibleObject
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotObservedObject:
                        msg = fromEngMsg.RobotObservedObject
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotState:
                        msg = fromEngMsg.RobotState
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectConnectionState:
                        msg = fromEngMsg.ObjectConnectionState
                        #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectMoved:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.ObjectStoppedMoving:
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.Ping:
                         msg = fromEngMsg.Ping
                         #UpdateBlah
                    elif fromEngMsg.tag == fromEngMsg.Tag.DebugString:
                         #From Robot::SendDebugString() summary of moving (lift/head/body), carrying, simpleMood, imageProc framerate and behavior
                         msg = fromEngMsg.DebugString
                         #sys.stdout.write("Recv: DebugString = '" + str(msg.text) + "'" + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotAvailable:
                        # these arrive frequently until we send ConnectToRobot and receive RobotConnected (see a couple more after that)
                        msg = fromEngMsg.RobotAvailable
                        #sys.stdout.write("Recv: RobotAvailable " + str(msg.robotID) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.AnimationAvailable:
                        msg = fromEngMsg.AnimationAvailable
                        #sys.stdout.write("Recv: AnimationAvailable '" + msg.animName + "'" + os.linesep)
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
                    elif fromEngMsg.tag == fromEngMsg.Tag.DebugLatencyMessage:
                        # [MARKW:TODO], update this locally for anything that wants to query engine<->robot latency
                        pass
                    elif fromEngMsg.tag == fromEngMsg.Tag.UnlockStatus:
                         msg = fromEngMsg.UnlockStatus
                         #sys.stdout.write("Recv: UnlockStatus length=" + str(len(msg.unlocks)) + " c= " + str(msg.unlocks) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.AnimationAborted:
                        msg = fromEngMsg.AnimationAborted                        
                        sys.stdout.write("Recv: AnimationAborted tag = " + str(msg.tag) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.RobotCompletedAction:
                        msg = fromEngMsg.RobotCompletedAction                        
                        sys.stdout.write("Recv: RobotCompletedAction robot="+str(msg.robotID)+", tag="+str(msg.idTag)+", actionType="+str(msg.actionType)+", res="+str(msg.result)+", completionInfo="+str(msg.completionInfo) + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.DebugAnimationString:
                        # Current animation playing (set to "" when done)
                        msg = fromEngMsg.DebugAnimationString                        
                        #sys.stdout.write("Recv: DebugAnimationString '" + msg.text + "'" + os.linesep)
                    else:
                        sys.stdout.write("Recv: Unhandled " + str(fromEngMsg.tag) + " = " + EToGM._tags_by_value[fromEngMsg.tag] + " from " + str(a) +  os.linesep);
                        sys.stdout.write("     " + str(d) + os.linesep);
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
            self.sendToEngine(toEngMessage)
            
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
            self.sendToEngine(toEngMessage)
            
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
                return self.sendToEngine(m)
                
        return False
            
            
    def PlayAnim(self, *args):
    
        playAnimMsg = GToEI.PlayAnimation()
        playAnimMsg.robotID = 1
        playAnimMsg.numLoops = 1
        playAnimMsg.animationName = "pounceForward"
        
        toEngMessage = GToEM(PlayAnimation = playAnimMsg);
        
        sys.stdout.write("Send PlayAnimation = '" + str(toEngMessage) + "'" + os.linesep)
        self.sendToEngine(toEngMessage)
        
        
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
    
        self.sendToEngine(toEngMessage)
                        
        return True
        
        
    def DoCommand(self, inCommand):
        
        commandType = inCommand[0]
                
        if (commandType == EngineCommand.consoleVar):
            return self.HandleConsoleVarCommand(*inCommand[1])
        elif (commandType == EngineCommand.consoleFunc):
            return self.HandleConsoleFuncCommand(*inCommand[1])
        elif (commandType == EngineCommand.sendMsg):
            return self.HandleSendMessageCommand(*inCommand[1])
        elif (commandType == EngineCommand.setEmotion):
            return self.HandleSetEmotionCommand(*inCommand[1])
        else:
            sys.stderr.write("[DoCommand] Unhandled commande type: " + str(commandType) + os.linesep)
            return False
            
        
    def Update(self):

        self.ReceiveFromEngine()
                
        if (self.state == ConnectionState.notConnected): 

            sys.stdout.write("Trying to connect to Engine..." + os.linesep);
            self.state = ConnectionState.tryingToConnect

        if (self.state == ConnectionState.tryingToConnect):
              
            adMsg = GToE.AdvertisementRegistrationMsg()
            adMsg.toEnginePort=self.sdkToEngPort
            adMsg.fromEnginePort=self.engToSdkPort
            adMsg.ip = self.ipAddress
            adMsg.id = 1
            adMsg.enableAdvertisement = True
            adMsg.oneShot = True
            
            toEngMsg = GToEM(AdvertisementRegistrationMsg = adMsg);
            
            self.sendToAd(toEngMsg)
            
        elif (self.state == ConnectionState.connected):
         
            startMsg = GToEI.StartEngine()
            toEngMsg = GToEM(StartEngine = startMsg);
            
            sys.stdout.write("Send StartEngine" + os.linesep)
            
            self.sendToEngine(toEngMsg)
            
            self.state = ConnectionState.startedEngine
            
        elif (self.state == ConnectionState.startedEngine):
        
            addRobotMsg = GToEI.ForceAddRobot()
            
            addRobotMsg.robotID = 1        
            addRobotMsg.ipAddress = "127.0.0.1\0\0\0\0\0\0\0".encode('ascii') # "127.0.0.1"
            addRobotMsg.isSimulated = 1 # [MARKW:TODO] Need to distinguish somewhere, maybe never do this outside engine?
            
            toEngMsg = GToEM(ForceAddRobot = addRobotMsg);
            
            sys.stdout.write("Send ForceAddRobot" + os.linesep)
            self.sendToEngine(toEngMsg)
            
            self.state = ConnectionState.addedRobot
        
        elif (self.state == ConnectionState.addedRobot):

            getVarsMsg = GToEI.GetAllDebugConsoleVarMessage() 
            toEngMsg = GToEM(GetAllDebugConsoleVarMessage = getVarsMsg);
            
            sys.stdout.write("Send GetAllDebugConsoleVarMessage" + os.linesep)            
            self.sendToEngine(toEngMsg)
            
            self.state = ConnectionState.requestedVars
            
        elif (self.state == ConnectionState.requestedVars):
        
            connMsg = GToEI.ConnectToRobot()
            connMsg.robotID = 1
           
            toEngMsg = GToEM(ConnectToRobot = connMsg);
            
            sys.stdout.write("Send ConnectToRobot" + os.linesep)
            self.sendToEngine(toEngMsg)

            self.state = ConnectionState.connectingToRobot;
        elif (self.state == ConnectionState.connectingToRobot):
            pass # wait for connected message
        elif (self.state == ConnectionState.connectedToRobot):
            pass # all is running
        else:
            sys.stderr.write("[Update] Unexpected state: " + str(self.state) + os.linesep)
    
    def IsConnected(self):
        return self.state >= ConnectionState.connectedToRobot
        
    def Disconnect(self):
        self.udpTransport.CloseSocket()

    def sendToEngine(self, msg):
        if self.verboseEngine:
            sys.stdout.write("[sendToEngine] '" + str(msg) + "'" + os.linesep)
        return self.udpTransport.SendData(self.engDest, msg.pack())
        
    def sendToAd(self, msg):
        if self.verboseAd:
            sys.stdout.write("[sendToAd] '" + str(msg) + "'" + os.linesep)
        return self.udpTransport.SendData(self.adDest, msg.pack())


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
                
                time.sleep(0.1)
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

        
def Init(runAsync=False):
    "Initalize the engine interface. Must be called before any other methods"
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        sys.stderr.write("Already Initialized!")
        return None        
    if gEngineInterfaceInstance is None:
        gEngineInterfaceInstance = _EngineInterfaceImpl()
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
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        del gEngineInterfaceInstance
        gEngineInterfaceInstance = None
    
def IsConnected():
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        return gEngineInterfaceInstance.IsConnected()
    else:
        return False    
