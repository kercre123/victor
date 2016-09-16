"""
Utility classes for interfacing with the Cozmo robot through reliable transport by exchanging CLAD messages.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, re, struct, json

CLAD_SRC  = os.path.join("clad")
CLAD_DIR  = os.path.join("generated", "cladPython", "robot")
ANKI_LOG_STRING_TABLE_LOCAL  = 'AnkiLogStringTables.json'
ANKI_LOG_STRING_TABLE_GLOBAL = os.path.join('..', 'resources', 'config', 'basestation', 'AnkiLogStringTables.json')

if os.path.isfile(os.path.join(CLAD_SRC, "Makefile")):
    import subprocess
    make = subprocess.Popen(["make", "python", "-C", "clad", "-j4"])
    if make.wait() != 0:
        sys.exit("Could't build/update python clad, exit status {:d}".format(make.wait(), linesep=os.linesep))

if not os.path.isfile(ANKI_LOG_STRING_TABLE_GLOBAL) and not os.path.isfile(ANKI_LOG_STRING_TABLE_LOCAL):
    import subprocess
    make = subprocess.Popen(['make', ANKI_LOG_STRING_TABLE_GLOBAL])
    if make.wait() != 0:
        sys.exit("Anki log string table ({} or {}) wasn't available and generating it failed with exit status {:d}".format(ANKI_LOG_STRING_TABLE_GLOBAL, ANKI_LOG_STRING_TABLE_LOCAL, make.wait()))

sys.path.insert(0, CLAD_DIR)

try:
    from ReliableTransport import *
    from clad.robotInterface.messageEngineToRobot import Anki
    from clad.robotInterface.messageRobotToEngine import Anki as _Anki
    from clad.robotInterface.messageEngineToRobot_hash import messageEngineToRobotHash
    from clad.robotInterface.messageRobotToEngine_hash import messageRobotToEngineHash
except:
    sys.exit("Can't import ReliableTransport / CLAD libraries!{linesep}  * Are you running from the base robot directory?{linesep}".format(linesep=os.linesep))
from ankiLogPP import importTables

Anki.update(_Anki.deep_clone())
RI = Anki.Cozmo.RobotInterface # namespace shortcut

dispatcher = None

CameraResolutions = (
  (16, 16),     # VerificationSnapshot
  (40, 30),     # QQQQVGA
  (80, 60),     # QQQVGA
  (160, 120),   # QQVGA
  (320, 240),   # QVGA
  (400, 296),   # CVGA
  (640, 480),   # VGA
  (800, 600),   # SVGA
  (1024, 768),  # XGA
  (1280, 960),  # SXGA
  (1600, 1200), # UXGA
  (2048, 1536), # QXGA
  (3200, 2400), # QUXGA
)

reinterpret_cast = {
    "d": lambda x: x,
    "i": lambda x: x,
    "x": lambda x: x,
    "f": lambda x: struct.unpack("f", struct.pack("i", x))[0],
}

FORMATTER_KEY = re.compile(r'(?<!%)%[0-9.-]*([{}])'.format("".join(reinterpret_cast.keys()))) # Find singal % marks

def formatTrace(fmt, args):
    "Returns the formatted string from a trace, doing the nesisary type reinterpretation"
    convertedArgs = tuple([reinterpret_cast[t](a) for t, a in zip(FORMATTER_KEY.findall(fmt), args)])
    try:
        ret = fmt % convertedArgs
    except:
        ret = "Error formatting string: {} with args {}".format(fmt, repr(convertedArgs))
    return ret

class ConnectionState:
    "An enum for connection states"
    notConnected     = 0
    waitingToConnect = 1
    connected        = 2
    failedToConnect  = 3
    disconnected     = 4

class _Dispatcher(IDataReceiver):
    "An IDataReciver interface implementation which dispatches messages to / from the robot"
    def __init__(self, transport, warnMsgErrors, forkTransportThread=True):
        transport.OpenSocket()
        self.warnMsgErrors = warnMsgErrors
        self.state = ConnectionState.notConnected
        self.dest = None
        self.OnConnectionRequestSubscribers = []
        self.OnConnectedSubscribers = []
        self.OnDisconnectedSubscribers = []
        self.ReceiveDataSubscribers = {} # Dict for message tags
        self.transport = ReliableTransport(transport, self)
        if forkTransportThread: self.transport.start()
        self.nameTable, self.formatTable = importTables(ANKI_LOG_STRING_TABLE_LOCAL if os.path.isfile(ANKI_LOG_STRING_TABLE_LOCAL) else ANKI_LOG_STRING_TABLE_GLOBAL)

    def Connect(self, dest=("172.31.1.1", 5551), syncTime=0, imageRequest=False):
        "Initiate reliable Connection"
        self.dest = dest
        self.state = ConnectionState.waitingToConnect
        self.syncTime = syncTime
        self.imageRequest = imageRequest
        return self.transport.Connect(self.dest)

    def Disconnect(self, dest=None):
        if dest is None:
            dest = self.dest
        elif dest != self.dest:
            raise ValueError("Cannot disconnect from {} because not connected to it{linesep}".format(repr(dest), linesep=os.linesep))
        self.state = ConnectionState.notConnected
        return self.transport.Disconnect(dest)

    def __del__(self):
        self.Disconnect()
        time.sleep(0.1)
        self.transport.KillThread()

    def OnConnectionRequest(self, sourceAddress):
        if not self.OnConnectionRequestSubscribers:
            sys.stderr.write("Received connection request but no connection request subscribers to handle it!")
            sys.stderr.write(os.linesep)
        else:
            for sub in self.OnConnectionRequestSubscribers:
                sub(sourceAddress)

    def OnConnected(self, sourceAddress):
        sys.stdout.write("Completed connection to {}{linesep}".format(repr(sourceAddress), linesep=os.linesep))
        self.state = ConnectionState.connected
        if self.syncTime is not None:
            self.send(RI.EngineToRobot(syncTime=RI.SyncTime(1, self.syncTime)))
            self.send(RI.EngineToRobot(initAnimController=Anki.Cozmo.AnimKeyFrame.InitController()))
        if self.imageRequest:
            self.send(RI.EngineToRobot(imageRequest=RI.ImageRequest(Anki.Cozmo.ImageSendMode.Stream, Anki.Cozmo.ImageResolution.QVGA)))
        for sub in self.OnConnectedSubscribers:
            sub(sourceAddress)

    def OnDisconnected(self, sourceAddress):
        sys.stdout.write("Lost connection to {}{linesep}".format(repr(sourceAddress), linesep=os.linesep))
        if self.state is ConnectionState.connected:
            self.state = ConnectionState.disconnected
        elif self.state is ConnectionState.waitingToConnect:
            self.state = ConnectionState.failedToConnect
        else:
            raise Exception("Recieved disconnect in unexpected state self: {}{linesep}".format(self.state, linesep=os.linesep))
        for sub in self.OnDisconnectedSubscribers:
            sub(sourceAddress)

    def ReceiveData(self, buffer, sourceAddress):
        assert sourceAddress == self.dest
        try:
            msg = RI.RobotToEngine.unpack(buffer)
        except Exception as e:
            if self.warnMsgErrors:
                if len(buffer):
                    tag = ord(buffer[0]) if sys.version_info.major < 3 else buffer[0]
                    sys.stderr.write("Error decoding incoming message {0:02x}[{1:d}]{linesep}\t{2:s}{linesep}".format(tag, len(buffer), str(e), linesep=os.linesep))
                else:
                    sys.stderr.write("Got 0 length message!")
                    sys.stderr.write(os.linesep)
                sys.stderr.flush()
        else:
            if msg.tag == msg.Tag.printText:
                sys.stdout.write("ROBOT: " + msg.printText.text)
            elif msg.tag == msg.Tag.trace:
                base = "ROBOT TRACE"
                if not msg.trace.name in self.nameTable:
                    sys.stderr.write("{} unknown trace name ID {:d}{}".format(base, msg.trace.name, os.linesep))
                elif not msg.trace.stringId in self.formatTable:
                    kwds = {'linesep':  os.linesep,
                            'base':     base,
                            'level':    msg.trace.level,
                            'name':     self.nameTable[msg.trace.name],
                            'stringId': msg.trace.stringId,
                            'vals':     repr(msg.trace.value)
                    }
                    sys.stderr.write("{base} {level:d} {name}: Unknown format string id {stringId:d}.{linesep}\tValues = {vals}{linesep}".format(**kwds))
                elif len(msg.trace.value) != self.formatTable[msg.trace.stringId][1]:
                    kwds = {'linesep':  os.linesep,
                            'base':     base,
                            'level':    msg.trace.level,
                            'name':     self.nameTable[msg.trace.name],
                            'nameId':   msg.trace.name,
                            'fmt':      self.formatTable[msg.trace.stringId][0],
                            'fmtId':    msg.trace.stringId,
                            'nargs':    self.formatTable[msg.trace.stringId][1],
                            'vals':     repr(msg.trace.value),
                            'nvals':    len(msg.trace.value)
                    }
                    sys.stderr.write("{base} {level:d} {name} ({nameId:d}): Number of args ({nvals:d}) doesn't match format string ({nargs:d}){linesep}\tFormat ({fmtId:d}): \"{fmt}\"{linesep}\tVals: {vals}{linesep}".format(**kwds))
                else:
                    kwds = {'linesep':   os.linesep,
                            'base':      base,
                            'level':     msg.trace.level,
                            'name':      self.nameTable[msg.trace.name],
                            'formatted': formatTrace(self.formatTable[msg.trace.stringId][0], msg.trace.value)
                    }
                    sys.stdout.write("{base} ({level:d}) {name}: {formatted}{linesep}".format(**kwds))
            elif msg.tag == msg.Tag.firmwareVersion:
                jsonBytes = bytes(msg.firmwareVersion.json)
                try:
                    jsonString = jsonBytes.decode()
                except:
                    sys.stderr.write("FAILED TO DECODE FIRMWARE VERSION INFO FROM ROBOT:{ls}{0}{ls}{ls}".format(jsonBytes, ls=os.linesep))
                else:
                    try:
                        fwInfo = json.loads(jsonString)
                    except:
                        sys.stderr.write("FAILED TO PARSE FIRMWARE VERSION INFO FROM ROBOT:{ls}{0}{ls}{ls}".format(jsonString, ls=os.linesep))
                    else:
                        sys.stdout.write("Firmware version:")
                        sys.stdout.write(os.linesep)
                        for i in fwInfo.items():
                            sys.stdout.write("{0:>25}: {1}{ls}".format(*i, ls=os.linesep))
            elif msg.tag == msg.Tag.factoryFirmwareVersion:
                sys.stdout.write("FACTORY Firmware version:")
                sys.stdout.write(os.linesep)
                sys.stdout.write("{0:>25}: {vi.wifiVersion}{ls}{1:>25}: {vi.rtipVersion}{ls}{2:>25}: {vi.bodyVersion}{ls}{3:>25}: {vi.toRobotCLADHash}{ls}{4:>25}: {vi.toEngineCLADHash}{ls}".format("WiFi Version", "RTIP Version", "Body Version", "To robot CLAD hash", "To engine CLAD hash", vi=msg.factoryFirmwareVersion, ls=os.linesep))
            elif msg.tag == msg.Tag.mainCycleTimeError:
                sys.stdout.write(repr(msg.mainCycleTimeError))
                sys.stdout.write(os.linesep)
            for tag, subs in self.ReceiveDataSubscribers.items():
                if msg.tag == tag:
                    for sub in subs:
                        sub(getattr(msg, msg.tag_name))


    def send(self, msg):
        return self.transport.SendData(True, False, self.dest, msg.pack())

def Init(warnMsgErrors=True, transport=UDPTransport(), forkTransportThread=True):
    "Initalize the robot interface. Must be called before any other methods"
    global dispatcher
    if dispatcher is None:
        dispatcher = _Dispatcher(transport, warnMsgErrors, forkTransportThread)
    elif warnMsgErrors:
        dispatcher.warnMsgErrors = True
    return dispatcher

def Connect(*args, **kwargs):
    global dispatcher
    return dispatcher.Connect(*args, **kwargs)

def Disconnect(*args, **kwargs):
    global dispatcher
    return dispatcher.Disconnect(*args, **kwargs)

def Die():
    global dispatcher
    dispatcher.Disconnect()
    time.sleep(0.1)
    dispatcher.transport.KillThread()
    del dispatcher
    dispatcher = None

def Send(*args, **kwargs):
    global dispatcher
    return dispatcher.send(*args, **kwargs)

def GetState():
    global dispatcher
    return dispatcher.state

def GetConnected():
    return GetState() == ConnectionState.connected

def SubscribeToConnectionRequests(callback):
    global dispatcher
    assert callable(callback)
    dispatcher.OnConnectedSubscribers.append(callback)

def SubscribeToConnect(callback):
    global dispatcher
    assert callable(callback)
    dispatcher.OnConnectedSubscribers.append(callback)

def SubscribeToDisconnect(callback):
    global dispatcher
    assert callable(callback)
    dispatcher.OnDisconnectedSubscribers.append(callback)

def SubscribeToTag(tag, callback):
    global dispatcher
    assert callable(callback)
    if tag in dispatcher.ReceiveDataSubscribers:
        dispatcher.ReceiveDataSubscribers[tag].append(callback)
    else:
        dispatcher.ReceiveDataSubscribers[tag] = [callback]

def Step():
    "Step transport thread if not forked"
    global dispatcher
    dispatcher.transport.step()
