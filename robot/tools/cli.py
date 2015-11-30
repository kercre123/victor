#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, socket, threading, time, select

if sys.version_info.major < 3:
    sys.exit("Cozmo CLI only works with python3+")

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

    def __init__(self, unreliableTransport, robotAddress):
        sys.stdout.write("Connecting to robot at: %s\n" % repr(robotAddress))
        unreliableTransport.OpenSocket()
        self.transport = ReliableTransport(unreliableTransport, self)
        self.robot = robotAddress
        self.transport.Connect(robotAddress)
        self.transport.start()
        self.input = input

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
        

    def send(self, msg):
        return self.transport.SendData(True, False, self.robot, msg.pack())

    def helpFtn(self, *args):
        "Prints out help text on CLI functions"
        sys.stdout.write("Cozmo CLI:\n")
        for cmd, ftn in self.functions.items():
            sys.stdout.write("\t%s:\t%s\n" % (cmd, ftn.__doc__))
        sys.stdout.write("^D to end REPL\n\n")
        return True

    def REP(self):
        "Read, eval, print"
        args = self.input("COZMO>>> ").split()
        if not len(args):
            return None
        else:
            return self.functions.get(args[0], self.helpFtn)(self, *args[1:])

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
                sys.stdout.write("\t%s\n" % str(ret))

    def exitFtn(self, *args):
        "Exit the REPL"
        raise EOFError
        return False

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
        self.send(RobotInterface.EngineToRobot(drive=RobotInterface.DriveWheels(lws, rws))
        return True

    functions = {
        "help": helpFtn,
        "exit": exitFtn,
        "drive": DriveWheels,
    }


if __name__ == '__main__':
    transport = UDPTransport()
    #transport = UartSimRadio("com4", 115200)
    cli = CozmoCLI(transport, ("172.31.1.1", ROBOT_PORT))
    cli.loop()
    cli.transport.KillThread()

    #if len(sys.argv) == 2:
    #    robot = sys.argv[1]
    #else:
    #    robot = "172.31.1.1"
    #cli = CozmoCLI(robot)
    #cli.loop()
    #cli.pinger.stop()
