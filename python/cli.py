#!/usr/bin/env python
"""
Python command line interface for Robot over the network
"""

import sys, os, socket, threading, time, select

if sys.version_info.major < 3:
    sys.stderr.write("Cozmo CLI only works with python3+")
    sys.exit(1)

from ReliableTransport import *

# XXX Replace this with importing the CLAD python messages
assert os.path.split(os.path.abspath(os.path.curdir))[1] == 'products-cozmo', "Script must be run from root cozmo directory"
sys.path.insert(0, 'python')
sys.path.append(os.path.join(os.path.curdir, 'robot', 'som'))
from messages import *

ROBOT_PORT = 5551


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
        #sys.stdout.write("RX %d: %s\r\n" % (buffer[0], buffer[1:].decode("ASCII", "ignore")))
        pass

    def send(self, buffer):
        return self.transport.SendData(True, False, self.robot, buffer)

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

    def FlashBlockIDs(self, *args):
        "Sends a FlashBlockIDs message to the robot"
        self.r(FlashBlockIDs().serialize(), self.robot)
        return True

    def SetBlockLights(self, *args):
        "Sends a set block lights command. Args: <block ID> [LIGHT VALUE [LIGHT VALUE [...]]]"
        try:
            blockId = int(eval(args[0]))
            lights = eval(' '.join(args[1:]))
        except:
            sys.stderr.write("Couldn't evaluate arguments\n")
            return False
        else:
            try:
                msg = SetBlockLights(blockID=blockId, lights=lights)
            except Exception as e:
                sys.stderr.write("Incorrect arguments:\r\n%s\n\n" % str(e))
                return False
            else:
                self.send(msg.serialize())
                return lights

    def ImageRequest(self, *args):
        "Send an image request to the robot. Args: <command> <resolution>"
        if len(args) != 2:
            sys.stderr.write("ImageRequest needs command and resolution specified\n")
            return False
        else:
            try:
                cmd = int(eval(args[0]))
            except:
                sys.stderr.write("Couldn't interprate \"%s\" as an image request command\n" % args[0])
                return False
            try:
                res = int(eval(args[1]))
            except:
                sys.stderr.write("Couldn't interprate \"%s\" as an image request resolution\n" % args[1])
                return False
            self.send(ImageRequest(imageSendMode=cmd, resolution=res).serialize())
            return True

    def DriveWheels(self, *args):
        "Send a DriveWheels message to the robot. Args: <left wheel speed> <right wheel speed>"
        if len(args) != 2:
            sys.stderr.write("drive command requires two motor speeds as floats\n")
            return False
        else:
            try:
                lws = float(args[0])
            except:
                sys.stderr.write("Couldn't interprate \"%s\" as a floating point wheel speed\n" % args[0])
                return False
            try:
                rws = float(args[1])
            except:
                sys.stderr.write("Couldn't interprate \"%s\" as a floating point wheel speed\n" % args[1])
                return False
            self.send(DriveWheelsMessage(lws, rws).serialize())
            return True

    def StopAllMotors(self, *args):
        "Full stop"
        self.send(StopAllMotorsMessage().serialize())

    def SetHeadAngle(self, *args):
        "Commands the head angle. Args: <angle (radians)> [<max speed> [<acceleration>]]"
        if len(args) == 0:
            sys.stderr.write("Need at least an angle for SetHeadAngle")
            return False
        else:
            try:
                params = [float(a) for a in args]
            except:
                sys.stderr.write("Couldn't interpret arguments as floats: %s\n" % repr(args))
                return False
            else:
                self.send(SetHeadAngleMessage(*params).serialize())
                return True

    def SetLiftHeight(self, *args):
        "Commands the lift height. Args: <height (mm)> [<max speed> [<acceleration>]]"
        if len(args) == 0:
            sys.stderr.write("Need at least a height for SetLiftHeight")
            return False
        else:
            try:
                params = [float(a) for a in args]
            except:
                sys.stderr.write("Couldn't interpret arguments as floats: %s \n" % repr(args))
                return False
            else:
                self.send(SetLiftHeightMessage(*params).serialize())
                return True


    functions = {
        "help": helpFtn,
        "exit": exitFtn,
        "FlashBlockIDs": FlashBlockIDs,
        "SetBlockLights": SetBlockLights,
        "ImageRequest": ImageRequest,
        "drive": DriveWheels,
        "stop":  StopAllMotors,
        "HeadAngle": SetHeadAngle,
        "LiftHeight": SetLiftHeight,
    }


if __name__ == '__main__':
    from ReliableTransport.testbench import UartSimRadio
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
