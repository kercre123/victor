#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, socket, threading, time, select

if sys.version_info.major < 3:
    sys.stdout.write("Python2.x is depricated\r\n")

TOOLS_DIR = os.path.join("tools")
CLAD_DIR  = os.path.join("generated", "cladPython", "robot")

if not os.path.isdir(TOOLS_DIR):
    sys.exit("Cannot find tools directory \"{}\". Are you running from the base robot directory?".format(TOOLS_DIR))
elif not os.path.isdir(CLAD_DIR):
    sys.exit("Cannot find CLAD directory \"{}\". Are you running from the base robot directory?".format(CLAD_DIR))

sys.path.insert(0, TOOLS_DIR)
sys.path.insert(0, CLAD_DIR)

from ReliableTransport import *

from clad.robotInterface.messageEngineToRobot import Anki
from clad.robotInterface.messageRobotToEngine import Anki as _Anki
Anki.update(_Anki.deep_clone())
RobotInterface = Anki.Cozmo.RobotInterface

class CozmoCLI(IDataReceiver):
    "A class for managing the CLI REPL"

    def __init__(self, unreliableTransport, robotAddress, statePrintInterval):
        sys.stdout.write("Connecting to robot at: %s\n" % repr(robotAddress))
        unreliableTransport.OpenSocket()
        self.transport = ReliableTransport(unreliableTransport, self)
        self.robot = robotAddress
        self.transport.Connect(robotAddress)
        self.transport.start()
        self.input = input
        self.lastStatePrintTime = 0.0 if statePrintInterval > 0.0 else float('Inf')
        self.statePrintInterval = statePrintInterval

    def __del__(self):
        self.transport.KillThread()

    def OnConnectionRequest(self, sourceAddress):
        raise Exception("CozmoCLI wasn't expecing a connection request")

    def OnConnected(self, sourceAddress):
        sys.stdout.write("Connected to robot at %s\r\n" % repr(sourceAddress))

    def OnDisconnected(self, sourceAddress):
        sys.stdout.write("Lost connection to robot at %s\r\n" % repr(sourceAddress))

    def ReceiveData(self, buffer, sourceAddress):
        msg = RobotInterface.RobotToEngine.unpack(buffer)
        if msg.tag == msg.Tag.printText:
            sys.stdout.write("ROBOT: " + msg.printText.text)
        if msg.tag == msg.Tag.state:
            now = time.time()
            if now - self.lastStatePrintTime > self.statePrintInterval:
                sys.stdout.write(repr(msg.state))
                self.lastStatePrintTime = now
        

    def send(self, msg):
        return self.transport.SendData(True, False, self.robot, msg.pack())

    def helpFtn(self, *args):
        "Prints out help text on CLI functions"
        sys.stdout.write("Cozmo CLI:\n")
        for cmd, ftn in self.functions.items():
            sys.stdout.write("\t%s:\t%s\n" % (cmd, ftn.__doc__))
        sys.stdout.write("^D to end REPL\n\n")
        return True

    def exitFtn(self, *args):
        "Exit the REPL"
        raise EOFError
        return False

    def REP(self):
        "Read, eval, print"
        args = self.input("COZMO>>> ").split()
        if not len(args):
            return None
        else:
            if args[0] == 'help':
                return self.helpFtn(*args[1:])
            elif args[0] == 'exit':
                return self.exitFtn(*args[1:])
            elif not hasattr(RobotInterface.EngineToRobot, args[0]):
                sys.stderr.write("Unrecognized command \"{}\", try \"help\"\r\n".format(args[0]))
                return False
            else:
                try:
                    params = [eval(a) for a in args[1:]]
                except Exception as e:
                    sys.stderr.write("Couldn't parse command arguments for '{}':\r\n\t{}\r\n".format(args[0], e))
                    return False
                else:
                    t = getattr(RobotInterface.EngineToRobot.Tag, args[0])
                    y = RobotInterface.EngineToRobot.typeByTag(t)
                    try:
                        p = y(*params)
                    except Exception as e:
                        sys.stderr.write("Couldn't create {0} message from params *{1}:\r\n\t{2}\r\n".format(args[0], repr(params), str(e)))
                        return False
                    else:
                        m = RobotInterface.EngineToRobot(**{args[0]: p})
                        return self.send(m)
                    
    def loop(self):
        "Loops read eval print"
        while True:
            try:
                ret = self.REP()
            except EOFError:
                return
            except KeyboardInterrupt:
                return
            else:
                if ret not in (True, False, None):
                    sys.stdout.write("\t%s\n" % str(ret))

    def DriveWheels(self, *args):
        "Send a DriveWheels message to the robot. Args: <left wheel speed> <right wheel speed>"
        try:
            lws = float(eval(args[0]))
        except:
            sys.stderr.write("Drive wheels requires at least one floating point number argument\r\n")
            return False
        try:
            rws = float(eval(args[1]))
        except:
            rws = lws
        self.send(RobotInterface.EngineToRobot(drive=RobotInterface.DriveWheels(lws, rws)))
        return True
        
if __name__ == '__main__':
    transport = UDPTransport()
    #transport = UartSimRadio("com4", 115200)
    if '-s' in sys.argv:
        spi = 5.0
    else:
        spi = 0.0
    cli = CozmoCLI(transport, ("172.31.1.1", ROBOT_PORT), spi)
    cli.loop()
    cli.transport.KillThread()
