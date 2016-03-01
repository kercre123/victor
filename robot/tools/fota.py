#!/usr/bin/env python3
"""
Firmware over the air upgrade controller for loading new firmware images onto robot
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, hashlib, struct

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

sys.path.insert(0, os.path.join("tools"))
import robotInterface
RI = robotInterface.RI

class Upgrader:
    "robotInterface client to control the firmwre upgrade transmission"

    def recieveAck(self, msg):
        sys.stdout.write(".")
        sys.stdout.flush()
        self.acked = True

    def send(self, msg):
        "Send a message to the robot"
        return robotInterface.Send(msg)

    def sendAndWait(self, msg):
        "Sends a flash command message and waits for a flash ack response"
        self.acked = False
        self.send(msg)
        while self.acked == False:
            time.sleep(0.0005)

    def ota(self, filePathName, command, version, flashAddress):
        """Load a file and send it to the robot for flash
        Note that flashAddress must be a multiple of 0x1000
        """
        assert (flashAddress % 0x1000) == 0, "Flash address must be a multiple of sector size"
        FW_CHUNK_SIZE = 1024
        try:
            fw = open(filePathName, "rb").read()
        except Exception as inst:
            sys.stderr.write("Couldn't load requested firmware file \"{}\"\r\n".format(filePathName))
            raise inst
        else:
            # Calculate signature
            sig = hashlib.sha1(fw).digest()
            # Prepend length of firmware
            fwSize = len(fw)
            sys.stdout.write("Loading \"{}\", {:d} bytes.\r\n".format(filePathName, fwSize))
            fw = struct.pack('i', fwSize) + fw
            fwSize = len(fw)
            # Wait for connection
            while not robotInterface.GetConnected():
                time.sleep(0.1)
            # Erase flash
            self.sendAndWait(RI.EngineToRobot(eraseFlash=RI.EraseFlash(flashAddress, fwSize)))
            # Write Firmware
            written = 0
            while written < fwSize:
                if sys.version_info.major < 3:
                    chunk = [ord(b) for b in fw[written:written+FW_CHUNK_SIZE]]
                else:
                    chunk = fw[written:written+FW_CHUNK_SIZE]
                self.sendAndWait(RI.EngineToRobot(writeFlash=RI.WriteFlash(flashAddress+written, chunk)))
                written += len(chunk)
            # Finish OTA
            self.send(RI.EngineToRobot(
                triggerOTAUpgrade=RI.OTAUpgrade(
                    start   = flashAddress,
                    version = version,
                    sig     = [ord(b) for b in sig] if sys.version_info.major < 3 else sig,
                    command = command
                )
            ))

    def __init__(self):
        self.acked = None
        robotInterface.Init(False)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.flashWriteAck, self.recieveAck)

def UpgradeWiFi(up, fwPathName, version=0, flashAddress=RI.OTAFlashRegions.OTA_WiFi_flash_address):
    "Sends a WiFi firmware upgrade"
    up.ota(fwPathName, RI.OTACommand.OTA_WiFi, version, flashAddress)

def UpgradeRTIP(up, fwPathName, version=0, flashAddress=RI.OTAFlashRegions.OTA_RTIP_flash_address):
    "Sends an RTIP firmware upgrade"
    up.ota(fwPathName, RI.OTACommand.OTA_RTIP, version, flashAddress)

def UpgradeBody(up, fwPathName, version=0, flashAddress=RI.OTAFlashRegions.OTA_body_flash_address):
    up.send(RI.EngineToRobot(bootloadBody=RI.BootloadBody()))
    up.ota(fwPathName, RI.OTACommand.OTA_body, version, flashAddress)

def UpgradeAssets(up, flashAddresss, assetPathNames, version=0):
    "Sends an asset to flash"
    for f, a in zip(flashAddresss, assetPathName):
        up.ota(a, RI.OTACommand.OTA_RTIP, version, f)
        time.sleep(1.0) # Wait for finish

DEFAULT_WIFI_IMAGE = os.path.join("releases", "esp.user.bin")
DEFAULT_RTIP_IMAGE = os.path.join("releases", "robot.safe")
DEFAULT_BODY_IMAGE = os.path.join("releases", "syscon.safe")

def UpgradeAll(up, version=0, wifiImage=DEFAULT_WIFI_IMAGE, rtipImage=DEFAULT_RTIP_IMAGE, bodyImage=DEFAULT_BODY_IMAGE):
    "Stages all firmware upgrades and triggers upgrade"
    assert os.path.isfile(wifiImage)
    assert os.path.isfile(rtipImage)
    assert os.path.isfile(bodyImage)
    up.ota(wifiImage, RI.OTACommand.OTA_none,  version, RI.OTAFlashRegions.OTA_WiFi_flash_address)
    up.ota(rtipImage, RI.OTACommand.OTA_none,  version, RI.OTAFlashRegions.OTA_RTIP_flash_address)
    up.ota(bodyImage, RI.OTACommand.OTA_stage, version, RI.OTAFlashRegions.OTA_body_flash_address)

# Script entry point
if __name__ == '__main__':
    up = Upgrader()
    robotInterface.Connect()
    if len(sys.argv) < 2:
        UpgradeAll(up)
    elif sys.argv[1] == 'all':
        UpgradeAll(up)
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
    time.sleep(5) # Wait for upgrade to finish
    del up
