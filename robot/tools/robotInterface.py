"""
Utility classes for interfacing with the Cozmo robot through reliable transport by exchanging CLAD messages.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time

CLAD_SRC  = os.path.join("clad")
CLAD_DIR  = os.path.join("generated", "cladPython", "robot")
ANKI_LOG_STRING_TABLE = os.path.join('..', 'resources', 'config', 'basestation', 'AnkiLogStringTables.json')

if os.path.isfile(os.path.join(CLAD_SRC, "Makefile")):
    import subprocess
    make = subprocess.Popen(["make", "python", "-C", "clad"])
    if make.wait() != 0:
        sys.exit("Could't build/update python clad, exit status {:d}".format(make.wait(), linesep=os.linesep))

if not os.path.isfile(ANKI_LOG_STRING_TABLE):
    import subprocess
    make = subprocess.Popen(['make', ANKI_LOG_STRING_TABLE])
    if make.wait() != 0:
        sys.exit("Anki log string table ({}) wasn't available and generating it failed with exit status {:d}".format(ANKI_LOG_STRING_TABLE, make.wait()))

sys.path.insert(0, CLAD_DIR)

try:
    from ReliableTransport import *
    from clad.robotInterface.messageEngineToRobot import Anki
    from clad.robotInterface.messageRobotToEngine import Anki as _Anki
except:
    sys.exit("Can't import ReliableTransport / CLAD libraries!{linesep}\t* Are you running from the base robot directory?{linesep}".format(linesepos.linesep))
from ankiLogPP import importTables

Anki.update(_Anki.deep_clone())
RI = Anki.Cozmo.RobotInterface # namespace shortcut

dispatcher = None

class ConnectionState:
    "An enum for connection states"
    notConnected     = 0
    waitingToConnect = 1
    connected        = 2
    failedToConnect  = 3
    disconnected     = 4

class _Dispatcher(IDataReceiver):
    "An IDataReciver interface implementation which dispatches messages to / from the robot"
    def __init__(self, transport):
        transport.OpenSocket()
        self.state = ConnectionState.notConnected
        self.dest = None
        self.OnConnectionRequestSubscribers = []
        self.OnConnectedSubscribers = []
        self.OnDisconnectedSubscribers = []
        self.ReceiveDataSubscribers = {} # Dict for message tags
        self.transport = ReliableTransport(transport, self)
        self.transport.start()
        self.nameTable, self.formatTable = importTables(ANKI_LOG_STRING_TABLE)

    def Connect(self, dest=("172.31.1.1", 5551)):
        "Initiate reliable Connection"
        self.dest = dest
        self.state = ConnectionState.waitingToConnect
        return self.transport.Connect(self.dest)

    def Disconnect(self, dest=None):
        if dest is None:
            dest = self.dest
        elif dest != self.dest:
            raise ValueError("Cannot disconnect from {} because not connected to it\r\n".format(repr(dest)))
        self.state = ConnectionState.notConnected
        return self.transport.Disconnect(dest)

    def __del__(self):
        self.Disconnect()
        time.sleep(0.1)
        self.transport.KillThread()

    def OnConnectionRequest(self, sourceAddress):
        if not self.OnConnectionRequestSubscribers:
            sys.stderr.write("Received connection request but no connection request subscribers to handle it!\r\n")
        else:
            for sub in self.OnConnectionRequestSubscribers:
                sub(sourceAddress)

    def OnConnected(self, sourceAddress):
        sys.stdout.write("Completed connection to %s\r\n" % repr(sourceAddress))
        self.state = ConnectionState.connected
        for sub in self.OnConnectedSubscribers:
            sub(sourceAddress)

    def OnDisconnected(self, sourceAddress):
        sys.stdout.write("Lost connection to %s\r\n" % repr(sourceAddress))
        if self.state is ConnectionState.connected:
            self.state = ConnectionState.disconnected
        elif self.state is ConnectionState.waitingToConnect:
            self.state = ConnectionState.failedToConnect
        else:
            raise Exception("Recieved disconnect in unexpected state self: {}\r\n".format(self.state))
        for sub in self.OnDisconnectedSubscribers:
            sub(sourceAddress)

    def ReceiveData(self, buffer, sourceAddress):
        assert sourceAddress == self.dest
        try:
            msg = RI.RobotToEngine.unpack(buffer)
        except Exception as e:
            tag = ord(buffer[0]) if sys.version_info.major < 3 else buffer[0]
            sys.stderr.write("Error decoding incoming message {0:02x}[{1:d}]\r\n\t{2:s}\r\n".format(tag, len(buffer), str(e)))
        else:
            if msg.tag == msg.Tag.printText:
                sys.stdout.write("ROBOT: " + msg.printText.text)
            for tag, subs in self.ReceiveDataSubscribers.items():
                if msg.tag == tag:
                    for sub in subs:
                        sub(getattr(msg, msg.tag_name))
        
    def send(self, msg):
        return self.transport.SendData(True, False, self.dest, msg.pack())
        
def Init(transport=UDPTransport()):
    "Initalize the robot interface. Must be called before any other methods"
    global dispatcher
    if dispatcher is None:
        dispatcher = _Dispatcher(transport)
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
