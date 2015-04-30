#!/usr/bin/env python
"""
Python command line interface for Robot over the network
"""

import sys, os, socket, threading, time, select

# XXX Replace this with importing the CLAD python messages
assert os.path.split(os.path.abspath(os.path.curdir))[1] == 'products-cozmo', "Script must be run from root cozmo directory"
sys.path.append(os.path.join(os.path.curdir, 'robot', 'som'))
from messages import *

ROBOT_PORT = 5551


class Pinger(threading.Thread, socket.socket):
    "A class that runs it's own thread to handle sending pings to the robot"

    def __init__(self, robotAddress):
        threading.Thread.__init__(self)
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.settimeout(0.030)
        self.pingAddr = (robotAddress, ROBOT_PORT)
        self.pingData = PingMessage().serialize()
        self._continue = True

    def __del__(self):
        self.stop()

    def run(self):
        "Thread main method"
        while self._continue:
            self.sendto(self.pingData, self.pingAddr)
            try:
                msg = self.recv(1500)
            except socket.timeout:
                continue
            else:
                t = time.time()
                sys.stdout.write("%02d[%04d]\r\n" % (ord(msg[0]), len(msg)))
                if PrintText.isa(msg):
                    printMsg = PrintText(buffer=msg)
                    sys.stdout.write("\r\n%s\r\nCOZMO>>> " % printMsg.text)
                #if ImageChunk.isa(msg):
                #    ic = ImageChunk(buffer=msg)
                #    sys.stdout.write("\rimgChunk %d[%02d] @ %fms: COZMO>>>" % (ic.imageId, ic.chunkId, t*1000.0))


    def stop(self):
        "Stop the thread running"
        self._continue = False



class CozmoCLI(socket.socket):
    "A class for managing the CLI REPL"

    def __init__(self, robotAddress):
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        sys.stdout.write("Connecting to robot at: %s\n" % robotAddress)
        self.bind(('', 6551))
        self.robot = (robotAddress, ROBOT_PORT)
        self.pinger = Pinger(robotAddress)
        self.pinger.start()
        if sys.version_info.major < 3:
            self.input = raw_input
        else:
            self.input = input

    def __del__(self):
        self.pinger.stop()

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
        self.sendto(FlashBlockIDs().serialize(), self.robot)
        return True

    def SetBlockLights(self, *args):
        "Sends a set block lights command. Args: <block ID> [LIGHT VALUE [LIGHT VALUE [...]]]"
        if len(args) > 9:
            sys.stderr.write("Too many arguments for SetBlockLights\n")
            return False
        msg = SetBlockLights(blockID=eval(args[0]))
        for i, a in enumerate(args[1:]):
            try:
                val = int(eval(a))
            except:
                sys.stderr.write("Couldn't interprate \"%s\" as an int\n" % a)
                return False
            else:
                msg.lights[i] = val
        self.sendto(msg.serialize(), self.robot)
        return True

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
            self.sendto(ImageRequest(imageSendMode=cmd, resolution=res).serialize(), self.robot)
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
            self.sendto(DriveWheelsMessage(lws, rws).serialize(), self.robot)
            return True

    def StopAllMotors(self, *args):
        "Full stop"
        self.sendto(StopAllMotorsMessage().serialize(), self.robot)

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
                self.sendto(SetHeadAngleMessage(*params).serialize(), self.robot)
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
                self.sendto(SetLiftHeightMessage(*params).serialize(), self.robot)
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
    if len(sys.argv) == 2:
        robot = sys.argv[1]
    else:
        robot = "172.31.1.1"
    cli = CozmoCLI(robot)
    cli.loop()
    cli.pinger.stop()
