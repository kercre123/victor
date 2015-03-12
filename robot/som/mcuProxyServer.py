"""
Python UDP server to proxy messages to / from the MCU and the WiFi radio
"""
__author__  = "Daniel Canser"

import sys, os, time, serial, struct
from subserver import *
import messages



class MCUProxyServer(BaseSubServer):
    "Proxy server class"

    CLIENT_TIMEOUT = 60.0 # One minute for right now
    SERIAL_DEVICE = "/dev/ttyO2"
    BAUD_RATE = 3000000
    SERIAL_HEADER = "\xbe\xef"

    def __init__(self, server, verbose=False):
        "Sets up the server instance"
        BaseSubServer.__init__(self, server, verbose)
        self.mcu = serial.Serial(port     = self.SERIAL_DEVICE,
                                 baudrate = self.BAUD_RATE,
                                 parity   = serial.PARITY_NONE,
                                 stopbits = serial.STOPBITS_ONE,
                                 bytesize = serial.EIGHTBITS,
                                 timeout  = 1.0)
        self.radioConnected = False
        self.rawSerData = ""
        self.lastRSMTS = 0
        self.sendToMcu(messages.ClientConnectionStatus().serialize())

    def stop(self):
        sys.stdout.write("Closing MCUProxyServer\n")
        BaseSubServer.stop(self)
        #self.sendToMcu(messages.ClientConnectionStatus(0,0).serialize())
        self.mcu.close()
        sys.stdout.flush()

    def giveMessage(self, message):
        "Pass a message from the server along"
        if self.radioConnected is False: # First message of connection
            self.sendToMcu(messages.ClientConnectionStatus(1, 0).serialize())
            self.radioConnected = True
        self.sendToMcu(message)

    def standby(self):
        "Put the sub server into standby mode"
        self.radioConnected = False
        #self.sendToMcu(messages.ClientConnectionStatus(0,0).serialize())

    def sendToMcu(self, message):
        "Send one message (with header and footer) to MCU over serial link"
        self.mcu.write(self.SERIAL_HEADER + struct.pack('I', len(message)))
        self.mcu.write(message)
        self.mcu.flush()
        if self.v: sys.stdout.write("toMcu: %d[%d]\n" % (ord(message[0]), len(message)))

    def step(self):
        "A single execution step for this thread"
        try:
            self.rawSerData += self.mcu.read(6)
        except Exception, e:
            if type(e) is ValueError and self._continue is False:
                return # Shutting down just pass
            else:
                raise e
        while len(self.rawSerData):
            xmitM4Message = True
            if self.v > 9: sys.stdout.write("serRx: %s\n" % self.rawSerData)
            messageStart = self.rawSerData.find(self.SERIAL_HEADER)
            if messageStart == -1:
                self.rawSerData = self.rawSerData[len(self.rawSerData)-len(self.SERIAL_HEADER):] # Throw out stuff we know we won't find header for
                return
            self.rawSerData = self.rawSerData[messageStart + len(self.SERIAL_HEADER):]
            if len(self.rawSerData) < 4: self.rawSerData += self.mcu.read(4 - len(self.rawSerData))
            if len(self.rawSerData) < 4: # Something's wrong
                self.rawSerData = "" # Throw everything out and start over
                return
            length = struct.unpack('I', self.rawSerData[:4])[0]
            if self.v > 9: sys.stdout.write("\t length = %d\n" % length)
            if length > 1500: # Should never happen, means corrupt data
                sys.stdout.write("MCU bad length: %d\n" % length)
                self.rawSerData = "" # Throw out what we have since we can't process it
                return
            self.rawSerData = self.rawSerData[4:]
            if len(self.rawSerData) < length:
                #if self.v: sys.stdout.write("sr read %d\n" % length)
                self.rawSerData += self.mcu.read(length - len(self.rawSerData))
            if len(self.rawSerData) < length: # Something is wrong
                if self.v: sys.stdout.write("Insufficient serial data %d < %d" % (len(self.rawSerData), length))
                self.rawSerData = "" # Throw everything out and start over
                return
            msgID = ord(self.rawSerData[0])
            if msgID == messages.PrintText.ID:
                sys.stdout.write("M4: %s" % (self.rawSerData[1:length],)) # Print statement
            elif msgID == messages.RobotState.ID:
                rsmts = struct.unpack('I', self.rawSerData[1:5])[0]
                if abs(rsmts - self.lastRSMTS) > 300:
                    err = "TORP: ts jump %d -> %d\n" % (self.lastRSMTS, rsmts)
                    self.clientSend(messages.PrintText(text=err).serialize())
                    sys.stderr.write(err)
                    xmitM4Message = False
                else:
                    self.server.timestamp.update(rsmts) # Unpack the timestamp member of the RobotState message
                    if self.v:
                        sys.stdout.write("M4 RSM ts: %d\n" % rsmts)
                self.lastRSMTS = rsmts
            elif self.v:
                sys.stdout.write("M4 pkt: %d[%d]\n" % (ord(self.rawSerData[0]), length))
                sys.stdout.flush()
            if xmitM4Message:
                self.clientSend(self.rawSerData[:length])
            self.rawSerData = self.rawSerData[length:]
