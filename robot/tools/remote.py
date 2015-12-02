#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, socket, threading, time, select
import pygame

TOOLS_DIR = os.path.join("tools")
CLAD_DIR  = os.path.join("generated", "cladPython", "robot")

#if not os.path.isdir(TOOLS_DIR):
#    sys.exit("Cannot find tools directory \"{}\". Are you running from the base robot directory?".format(TOOLS_DIR))
#elif not os.path.isdir(CLAD_DIR):
#    sys.exit("Cannot find CLAD directory \"{}\". Are you running from the base robot directory?".format(CLAD_DIR))

sys.path.insert(0, TOOLS_DIR)
sys.path.insert(0, CLAD_DIR)

from ReliableTransport import *

from clad.robotInterface.messageEngineToRobot import Anki
from clad.robotInterface.messageRobotToEngine import Anki as _Anki
Anki.update(_Anki.deep_clone())
Cozmo = Anki.Cozmo
RobotInterface = Anki.Cozmo.RobotInterface

class CozmoRemote(IDataReceiver):
    "A class for managing the CLI REPL"

    def __init__(self, unreliableTransport, robotAddress):
        sys.stdout.write("Connecting to robot at: %s\n" % repr(robotAddress))
        unreliableTransport.OpenSocket()
        self.transport = ReliableTransport(unreliableTransport, self)
        self.robot = robotAddress
        self.transport.Connect(robotAddress)
        self.transport.start()
        self.connected = False

    def __del__(self):
        self.transport.KillThread()

    def OnConnectionRequest(self, sourceAddress):
        raise Exception("CozmoCLI wasn't expecing a connection request")

    def OnConnected(self, sourceAddress):
        sys.stdout.write("Connected to robot at %s\r\n" % repr(sourceAddress))
        self.connected = True

    def OnDisconnected(self, sourceAddress):
        sys.stdout.write("Lost connection to robot at %s\r\n" % repr(sourceAddress))
        self.connected = False

    def ReceiveData(self, buffer, sourceAddress):
        msg = RobotInterface.RobotToEngine.unpack(buffer)
        if msg.tag == msg.Tag.printText:
            sys.stdout.write("ROBOT: " + msg.printText.text)
        

    def send(self, msg):
        return self.transport.SendData(True, False, self.robot, msg.pack())

    def pwm(self, params):
        "Send a DriveWheels message to the robot. Args: <left wheel speed> <right wheel speed>"
        if self.connected:
            self.send(RobotInterface.EngineToRobot(setRawPWM=Cozmo.RawPWM(params)))

def RemoteGame():
    """Container for remote "game" under pygame."""
    PWM_VAL = 20000
    
    pygame.init()
    screen_size = (320, 320)
    background = (207, 176, 123)
    screen = pygame.display.set_mode(screen_size)
    
    transport = UDPTransport()
    remote = CozmoRemote(transport, ("172.31.1.1", ROBOT_PORT))
    
    cmd = (0, 0, 0, 0)
    lastCmdTime = 0
    running = True
    while running:
        event = pygame.event.poll()
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.scancode == 16: # q
                running = False
            elif event.scancode == 72: # up arrow
                cmd = (+PWM_VAL, +PWM_VAL, 0, 0)
            elif event.scancode == 80: # down arrow
                cmd =(-PWM_VAL, -PWM_VAL, 0, 0)
            elif event.scancode == 75: # left arrow
                cmd =(-PWM_VAL, +PWM_VAL, 0, 0)
            elif event.scancode == 77: # right arrow
                cmd =(+PWM_VAL, -PWM_VAL, 0, 0)
            elif event.scancode == 17: # W
                cmd =(0, 0, PWM_VAL, 0)
            elif event.scancode == 31: # S
                cmd =(0, 0, -PWM_VAL, 0)
            elif event.scancode == 18: # E
                cmd =(0, 0, 0, PWM_VAL)
            elif event.scancode == 32: # D
                cmd =(0, 0, 0, -PWM_VAL)
            else:
                sys.stdout.write("Key = {}\r\n".format(event.scancode))
            lastCmdTime = 0
        elif event.type == pygame.KEYUP:
            cmd = (0, 0, 0, 0)
            lastCmdTime = 0
        
        now = time.time()
        if now - lastCmdTime > 0.100:
            remote.pwm(cmd)
            lastCmdTime = now
        
        screen.fill((0, 255, 0) if remote.connected else (255, 0, 0))
        pygame.display.flip()
    
    remote.pwm((0, 0, 0, 0))
    remote.transport.KillThread()

if __name__ == '__main__':
    RemoteGame()
