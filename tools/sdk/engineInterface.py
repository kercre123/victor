"""
Utility classes for interfacing with the Cozmo engine through a localhost UDP socket, by exchanging CLAD messages.
"""
__author__ = "Mark Wesley"

import sys, os, time, re, struct

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


class ConnectionState:
    "An enum for connection states"
    notConnected     = 0
    connected        = 1
    startedEngine    = 2
    addedRobot       = 3
    requestedVars    = 4
    connectingToRobot = 5    
    connectedToRobot = 6
    

class DebugConsoleVarWrapper:
    "Wraps an Anki.Cozmo.ExternalInterface.DebugConsoleVar and supports sorting"

    def __init__(self, debugConsoleVar):
        self.var = debugConsoleVar
        
    @property
    def varName(self):
        return self.var.varName
        
    @property
    def category(self):
        return self.var.category
        
    @property
    def minValue(self):
        return self.var.minValue
        
    @property
    def maxValue(self):
        return self.var.maxValue
        
    @property
    def varValue(self):
        return self.var.varValue
        
    @varValue.setter
    def varValue(self, value):
        self.var.varValue = value
        
    def formatValueToString(self, inVal):
        varValue = self.varValue
        if varValue.tag == varValue.Tag.varDouble:
            return str(inVal)
        elif varValue.tag == varValue.Tag.varUint:
            return "{:.0f}".format(inVal)
        elif varValue.tag == varValue.Tag.varInt:
            return "{:.0f}".format(inVal)
        elif varValue.tag == varValue.Tag.varBool:
            return "{:.0f}".format(inVal)
        elif varValue.tag == varValue.Tag.varFunction:
            return str(inVal)
        else:
            return "ERROR"
        
    def minValueToString(self):
        return self.formatValueToString(self.minValue)
        
    def maxValueToString(self):
        return self.formatValueToString(self.maxValue)

    def varValueTypeName(self):
        varValue = self.varValue
        if varValue.tag == varValue.Tag.varDouble:
            return "double"
        elif varValue.tag == varValue.Tag.varUint:
            return "uint32_t"
        elif varValue.tag == varValue.Tag.varInt:
            return "int"
        elif varValue.tag == varValue.Tag.varBool:
            return "bool"
        elif varValue.tag == varValue.Tag.varFunction:
            return "function"
        else:
            return "ERROR"
            
    def varValueContents(self):
        varValue = self.varValue
        if varValue.tag == varValue.Tag.varDouble:
            return varValue.varDouble
        elif varValue.tag == varValue.Tag.varUint:
            return varValue.varUint
        elif varValue.tag == varValue.Tag.varInt:
            return varValue.varInt
        elif varValue.tag == varValue.Tag.varBool:
            return varValue.varBool
        elif varValue.tag == varValue.Tag.varFunction:
            return varValue.varFunction
        else:
            return "ERROR"
                    
    def varValueToString(self):
        return self.formatValueToString(self.varValueContents())        
    
    def __lt__(self, rhs):
        if (self.category == rhs.category):
            return (self.varName < rhs.varName)
        else:
            return (self.category < rhs.category)


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
        
        self.consoleVars = {}
        self.consoleFuncs = {}
    
        
    def HandleInitDebugConsoleVarMessage(self, msg):
    
        for cVar in msg.varData:
            if cVar.varValue.tag == cVar.varValue.Tag.varFunction:
                self.consoleFuncs[cVar.varName] = DebugConsoleVarWrapper(cVar) 
            else:
                self.consoleVars[cVar.varName] = DebugConsoleVarWrapper(cVar)


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
                            sys.stdout.write("SDK Connected!" + os.linesep)
                            self.state = ConnectionState.connected
                        else:
                            sys.stdout.write("Not SDK?" + os.linesep)
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
                        self.HandleInitDebugConsoleVarMessage(msg)
                    elif fromEngMsg.tag == fromEngMsg.Tag.VerifyDebugConsoleVarMessage:
                        msg = fromEngMsg.VerifyDebugConsoleVarMessage
                        consoleVar = self.consoleVars.get(msg.varName)
                        #varValue
                        if msg.success:
                            sys.stdout.write("Recv: Var Success: '" + msg.varName + "' " + msg.statusMessage + os.linesep)
                            if consoleVar != None:
                                consoleVar.varValue = msg.varValue
                        else:
                            sys.stdout.write("Recv: Var Failure: '" + msg.varName + "' " + msg.statusMessage + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.VerifyDebugConsoleFuncMessage:
                        msg = fromEngMsg.VerifyDebugConsoleFuncMessage
                        # consoleFunc = self.consoleFuncs.get(msg.funcName)                         
                        if msg.success:
                            sys.stdout.write("Recv: Func Success: '" + msg.funcName + "' " + msg.statusMessage + os.linesep)                            
                        else:
                            sys.stdout.write("Recv: Func Failure: '" + msg.funcName + "' " + msg.statusMessage + os.linesep)
                    elif fromEngMsg.tag == fromEngMsg.Tag.MoodState:
                        msg = fromEngMsg.MoodState
                        #UpdateMoodState(msg)                    
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
    
    
    def HandleCVarCommand(self, *args):
    
        if (len(args) == 0):
            sys.stdout.write("Error - expected 'list' or variable name" + os.linesep)
            return
        
        varName = args[0]
        
        sys.stdout.write("HandleCVarCommand(): Var=" + str(varName) + ", args = " + str(args) + os.linesep)
        
        if varName.lower() == 'list':
            
            sys.stdout.write("List of Console Variables:" + os.linesep)
            
            # 1st sort the vars so we can display them in category and name order
            sortedVars = sorted(self.consoleVars.values())
            
            currentCat = ""
            
            for varValue in sortedVars:
                varName = varValue.varName
                if varValue.category != currentCat:
                    currentCat = varValue.category
                    sys.stdout.write("  " + currentCat + ":" + os.linesep)
                sys.stdout.write("    " + str(varName) + " = " + varValue.varValueToString() + " (" + varValue.varValueTypeName() + ")(" + varValue.minValueToString() + ".." + varValue.maxValueToString() + ")" + os.linesep)
        else:
        
            if varName in self.consoleVars:
                varValue = self.consoleVars[varName]
                sys.stdout.write("Pre  " + str(varName) + " = " + varValue.varValueToString() + " (" + varValue.varValueTypeName() + ")(" + varValue.minValueToString() + ".." + varValue.maxValueToString() + ")" + os.linesep)
            else:
                sys.stdout.write("Error: No var named" + str(varName) + "'" + os.linesep)
                return
                                        
            setVarMsg = GToEI.SetDebugConsoleVarMessage()
            setVarMsg.varName = varName
            
            argsToString = ""
            for i in range(1, len(args)):
                argsToString += args[i]
            
            setVarMsg.tryValue = argsToString
    
            toEngMessage = GToEM(SetDebugConsoleVarMessage = setVarMsg);
                
            sys.stdout.write("Send SetDebugConsoleVarMessage = '" + str(toEngMessage) + "'" + os.linesep)
            self.sendToEngine(toEngMessage)
            
            
    def PlayAnim(self, *args):
    
        playAnimMsg = GToEI.PlayAnimation()
        playAnimMsg.robotID = 1
        playAnimMsg.numLoops = 1
        playAnimMsg.animationName = "pounceForward"
        
        toEngMessage = GToEM(PlayAnimation = playAnimMsg);
        
        sys.stdout.write("Send PlayAnimation = '" + str(toEngMessage) + "'" + os.linesep)
        self.sendToEngine(toEngMessage)
        
        
    def Update(self):

        self.ReceiveFromEngine()
                
        if (self.state == ConnectionState.notConnected): 
              
            adMsg = GToE.AdvertisementRegistrationMsg()
            adMsg.toEnginePort=self.sdkToEngPort
            adMsg.fromEnginePort=self.engToSdkPort
            adMsg.ip = self.ipAddress
            adMsg.id = 1
            adMsg.enableAdvertisement = True
            adMsg.oneShot = True
            
            toEngMsg = GToEM(AdvertisementRegistrationMsg = adMsg);
            
            sys.stdout.write("Send AdvertisementRegistrationMsg" + os.linesep)
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
            sys.stdout.write("Unexpected state: " + str(self.state) + os.linesep)
    
    def IsConnected(self):
        return self.state >= ConnectionState.connectedToRobot
        
    def Disconnect(self):
        self.udpTransport.CloseSocket()

    def __del__(self):
        self.Disconnect()

    def sendToEngine(self, msg):
        return self.udpTransport.SendData(self.engDest, msg.pack())
        
    def sendToAd(self, msg):
        return self.udpTransport.SendData(self.adDest, msg.pack())


# ================================================================================    
# Public engineInterface:
# ================================================================================

        
def Init():
    "Initalize the engine interface. Must be called before any other methods"
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is None:
        gEngineInterfaceInstance = _EngineInterfaceImpl()
    return gEngineInterfaceInstance
    
def Update():
    global gEngineInterfaceInstance
    if gEngineInterfaceInstance is not None:
        gEngineInterfaceInstance.Update()

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
