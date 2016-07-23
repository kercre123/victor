#!/usr/bin/env python3
"""
Firmware over the air upgrade controller for loading new firmware images onto robot
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, hashlib, struct, argparse

DEFAULT_FIRMWARE_IMAGE = os.path.join("releases", "cozmo.safe")

parser = argparse.ArgumentParser(description="Upgrade firmware")
parser.add_argument("-w", "--wait", type=float,
                    nargs='?', const="2.0",
                    help="increase delay between blocks")
parser.add_argument("image", type=str,
                    nargs='?',
                    default=DEFAULT_FIRMWARE_IMAGE,
                    help="image to load on robot")
argv = parser.parse_args()

sys.path.insert(0, os.path.join("tools"))
import robotInterface
RI = robotInterface.RI

class OTAStreamer:
    "Streams OTA data from file to the robot"
    
    ROBOT_BUFFER_SIZE    = 16384 # Size of OTA RX buffer on the robot. Presently this is the animaiton key frame buffer
    MESSAGE_PAYLOAD_SIZE = len(RI.OTA.Write().data)
    
    def Write(self):
        "Sends a write to the robot"
        while ((self.bytesSent - self.bytesProcessed) < (self.ROBOT_BUFFER_SIZE - self.MESSAGE_PAYLOAD_SIZE)) and (self.ackCount > 1 or self.bytesSent < 5000):
            robotInterface.Step()
            payload = self.fw.read(self.MESSAGE_PAYLOAD_SIZE)
            eof = len(payload) < self.MESSAGE_PAYLOAD_SIZE
            if eof:
                payload += b"\xFF" * (self.MESSAGE_PAYLOAD_SIZE - len(payload))
            if str == bytes:
                payload = [ord(b) for b in payload]
            robotInterface.Send(RI.EngineToRobot(otaWrite=RI.OTA.Write(self.packetNumber, payload)))
            self.bytesSent += self.MESSAGE_PAYLOAD_SIZE
            #sys.stdout.write("Sending packet {0:d} {1:x}{linesep}".format(self.packetNumber, payload[0], linesep=os.linesep))
            sys.stdout.write('.')
            sys.stdout.flush()
            self.packetNumber += 1
            if eof:
                # Send EOF marker to robot
                robotInterface.Send(RI.EngineToRobot(otaWrite=RI.OTA.Write(-1, [0xff] * self.MESSAGE_PAYLOAD_SIZE)))
                self.doneSending = True
                sys.stdout.write("Finished sending firmware image to robot")
                sys.stdout.write(os.linesep)
                break
            if argv.wait: time.sleep(argv.wait)
        self.writing = False

    def OnConnect(self, connectionInfo):
        "Handles robot connected event, starts upgrade"
        self.writing = True
        
    def OnAck(self, ack):
        "Handles OTA.Ack messages and continues streaming"
        self.ackCount += 1
        self.bytesProcessed = ack.bytesProcessed
        if ack.result == RI.OTA.Result.OKAY:
            #sys.stdout.write("ACK p#={:d}\tbp={:d}{linesep}".format(ack.packetNumber, ack.bytesProcessed, linesep=os.linesep))
            if not self.doneSending:
                self.writing = True
        elif ack.result == RI.OTA.Result.SUCCESS:
            sys.stdout.write("Success :-)")
            sys.stdout.write(os.linesep)
            self.receivedSuccess = True
        else:
            sys.exit("OTA Error:{linesep}\t{ack}{linesep}".format(ack=repr(ack), linesep=os.linesep))
    
    def __init__(self, firmwareFile):
        if type(firmwareFile) in (str, bytes):
            self.fw = open(firmwareFile, "rb")
        else:
            self.fw = firmwareFile
        self.bytesSent    = 0
        self.bytesProcessed = 0;
        self.packetNumber = 0
        self.ackCount = 0
        self.writing = False
        self.doneSending = False
        self.receivedSuccess = False
        robotInterface.SubscribeToConnect(self.OnConnect)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.otaAck, self.OnAck)

    def main(self):
        while not self.receivedSuccess:
            if self.writing:
                self.Write()
            else:
                robotInterface.Step()

# Script entry point
if __name__ == '__main__':
    fwi = DEFAULT_FIRMWARE_IMAGE
    
    if os.path.isfile(argv.image):
        fwi = argv.image
    elif argv.image == 'factory':
        fwi = os.path.join("releases", "cozmo_factory_install.safe")
    else:
        sys.exit("No such file as {0}".format(argv.image))
    
    robotInterface.Init(True, forkTransportThread = False)
    up = OTAStreamer(fwi)
    robotInterface.Connect(syncTime = None)
    up.main()
    del up
