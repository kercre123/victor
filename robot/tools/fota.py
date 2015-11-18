#!/usr/bin/env python3
"""
Firmware over the air upgrade controller for loading new firmware images onto robot
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, socket, threading, time, select, hashlib

USAGE = """
Upgrade firmware:
    {exe} wifi|rtip|body|all [<ALTERNATE IMAGE>]
Where the WiFi, RTIP or body (or all of them) firmware is upgraded with the default image
If not upgrading all, an alternate image file may be specifid.

Upload asset:
    {exe} <ADDRESS> <ASSET FILE PATH NAME> [...]
Uploads one or more assets to the robot flash.
<ADDRESS> is the flash address to write the asset to and <ASSET FILE PATH NAME> is the file to load there.
Any number of pairs of addresses and files may be specified.

""".format(exe=sys.argv[0])

if sys.version_info.major < 3:
    sys.exit("Cozmo CLI only works with python3+")

TOOLS_DIR = os.path.join("tools")
CLAD_DIR  = os.path.join("generated", "cladPython", "robot")

if not os.path.isdir(TOOLS_DIR):
    sys.exit("Cannot find tools directory \"{}\". Are you running from the base robot directory?".format(TOOLS_DIR))
elif not os.path.isdir(CLAD_DIR):
    sys.exit("Cannot find CLAD directory \"{}\". Are you running from the base robot directory? Have you generated python clad?".format(CLAD_DIR))

sys.path.insert(0, TOOLS_DIR)
sys.path.insert(0, CLAD_DIR)

from ReliableTransport import *

from clad.robotInterface.messageEngineToRobot import Anki
from clad.robotInterface.messageRobotToEngine import Anki as _Anki
Anki.update(_Anki.deep_clone())
RobotInterface = Anki.Cozmo.RobotInterface

DEFAULT_ROBOT_ADDRESS = ("172.31.1.1", 5551)

class Upgrader(IDataReceiver):
    "ReliableTransport client to control the firmwre upgrade transmission"
    
    def __init__(self, robotAddress):
        sys.stdout.write("Connecting to robot at {}:{}\r\n".format(*robotAddress))
        self.robot = robotAddress
        self.acked = False
        self.connected = False
        ut = UDPTransport(logInPackets="fotaInPackets.bin")
        ut.OpenSocket()
        self.transport = ReliableTransport(ut, self)
        self.transport.Connect(self.robot)
        self.transport.start()

    def __del__(self):
        self.transport.KillThread()

    def OnConnectionRequest(self, sourceAddress):
        "Callback when a conenction request is received"
        raise Exception("Upgrader wasn't expecing a connection request")

    def OnConnected(self, sourceAddress):
        "Callback when robot accepts connection"
        sys.stdout.write("Connected to robot at %s\r\n" % repr(sourceAddress))
        self.connected = True

    def OnDisconnected(self, sourceAddress):
        "Callback if robot disconnects"
        sys.stdout.write("Lost connection to robot at %s\r\n" % repr(sourceAddress))
        self.connected = False

    def ReceiveData(self, buffer, sourceAddress):
        "Callback for received messages from robot"
        msg = RobotInterface.RobotToEngine.unpack(buffer)
        if msg.tag == msg.Tag.printText:
            sys.stdout.write("ROBOT: " + msg.printText.text)
        elif msg.tag == msg.Tag.flashWriteAck:
            sys.stdout.write("Received ACK {}\r\n".format(str(msg.flashWriteAck)))
            self.acked = True
        else:
            sys.stderr.write("Upgrader received unexpected message from robot:\r\n\t{}\r\n".format(str(msg)))

    def send(self, msg):
        "Send a message to the robot"
        return self.transport.SendData(True, False, self.robot, msg.pack())

    def sendAndWait(self, msg):
        "Sends a flash command message and waits for a flash ack response"
        self.acked = False
        self.send(msg)
        while self.acked == False:
            time.sleep(0.005)

    def ota(self, filePathName, command, version=0, flashAddress=0x100000):
        "Load a file and send it to the robot for flash"
        FW_CHUNK_SIZE = 1024
        try:
            fw = open(filePathName, "rb").read()
        except Error as inst:
            sys.stderr.write("Couldn't load requested firmware file \"{}\"\r\n".format(filePathName))
            raise inst
        else:
            fwSize    = len(fw)
            sys.stdout.write("Loading \"{}\", {:d} bytes.\r\n".format(filePathName, fwSize))
            # Calculate signature
            sig = hashlib.sha1(fw).digest()
            # Wait for connection
            while not self.connected:
                time.sleep(0.1)
            # Erase flash
            self.sendAndWait(RobotInterface.EngineToRobot(eraseFlash=RobotInterface.EraseFlash(flashAddress, fwSize)))
            # Write Firmware
            written = 0
            while written < fwSize:
                chunk = fw[written:written+FW_CHUNK_SIZE]
                self.sendAndWait(RobotInterface.EngineToRobot(writeFlash=RobotInterface.WriteFlash(flashAddress+written, chunk)))
                written += len(chunk)
            # Finish OTA
            self.send(RobotInterface.EngineToRobot(
                triggerOTAUpgrade=RobotInterface.OTAUpgrade(
                    start   = flashAddress,
                    size    = fwSize,
                    version = version,
                    sig     = sig,
                    command = command
                )
            ))
    
def UpgradeWiFi(up, fwPathName, version=0):
    "Sends a WiFi firmware upgrade"
    up.ota(fwPathName, RobotInterface.OTACommand.OTA_WiFi, version)

def UpgradeRTIP(up, fwPathName, version=0):
    "Sends an RTIP firmware upgrade"
    up.ota(fwPathName, RobotInterface.OTACommand.OTA_RTIP, version)

def UpgradeBody(up, fwPathName, version=0):
    up.ota(fwPathName, RobotInterface.OTACommand.OTA_body, version)

def UpgradeAssets(up, flashAddresss, assetPathNames, version=0):
    "Sends an asset to flash"
    for f, a in zip(flashAddresss, assetPathName):
        up.ota(a, RobotInterface.OTACommand.OTA_RTIP, version, f)
        time.sleep(1.0) # Wait for finish

def WaitForUserEnd():
    try:
        time.sleep(3600)
    except KeyboardInterrupt:
        return

DEFAULT_BODY_IMAGE=os.path.join("syscon", "???")
DEFAULT_RTIP_IMAGE=os.path.join("k02_hal", "???")
DEFAULT_WIFI_IMAGE=os.path.join("espressif", "bin", "upgrade", "user1.2048.new.3.bin")

# Script entry point
if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(USAGE)
    up = Upgrader(DEFAULT_ROBOT_ADDRESS)
    if sys.argv[1] == 'all':
        UpgradeBody(up, DEFAULT_BODY_IMAGE)
        time.sleep(5)
        UpgradeRTIP(up, DEFAULT_RTIP_IMAGE)
        time.sleep(5)
        UpgradeWiFi(up, DEFAULT_WIFI_IMAGE)
    elif sys.argv[1] == 'wifi':
        UpgradeWiFi(up, sys.argv[2] if len(sys.argv) > 2 else DEFAULT_WIFI_IMAGE)
    elif sys.argv[1] == 'rtip':
        UpgradeRTIP(up, sys.argv[2] if len(sys.argv) > 2 else DEFAULT_RTIP_IMAGE)
    elif sys.argv[1] == 'body':
        UpgradeBody(up, sys.argv[2] if len(sys.argv) > 2 else DEFAULT_BODY_IMAGE)
    else: # Try asset
        ind = 1
        addresses = []
        assetFNs  = []
        while ind + 1 < len(sys.argv):
            try:
                fa = int(eval(sys.argv[ind]))
            except:
                sys.exit("Couldn't parse {} as a flash address".format(sys.argv[ind]))
            addresses.append(fa)
            assetFNs.append(sys.argv[ind+1])
            ind += 2
        UpgradeAssets(up, addresses, assetFNs)
    WaitForUserEnd()
