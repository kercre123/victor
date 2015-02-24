"""
Python UDP server to proxy messages to / from the MCU and the WiFi radio
"""
__author__  = "Daniel Canser"
__version__ = "0.0.3"

import sys, os, socket, time, serial, threading, struct
import messages



class MCUProxyServer(threading.Thread):
    "Proxy server class"

    CLIENT_TIMEOUT = 60.0 # One minute for right now
    SERIAL_DEVICE = "/dev/ttyO2"
    BAUD_RATE = 3000000
    SERIAL_HEADER = "\xbe\xef"

    def __init__(self, server, verbose=False):
        "Sets up the server instance"
        threading.Thread.__init__(self)
        self.v=verbose
        if self.v: sys.stdout.write("MCUProxyServer will be verbose\n")
        self.server = server
        self.mcu = serial.Serial(port     = self.SERIAL_DEVICE,
                                 baudrate = self.BAUD_RATE,
                                 parity   = serial.PARITY_NONE,
                                 stopbits = serial.STOPBITS_ONE,
                                 bytesize = serial.EIGHTBITS,
                                 timeout  = None)
        self.radioConnected = False

    def __del__(self):
        "Send closeout messages"
        self.stop()

    def stop(self):
        sys.stdout.write("Closing MCUProxyServer\n")
        self.sendToMcu(messages.ClientConnectionStatus(0,0).serialize())
        self.mcu.close()
        sys.stdout.flush()
        exit()

    def giveMessage(self, message):
        "Pass a message from the server along"
        if self.radioConnected is False: # First message of connection
            self.sendToMcu(messages.ClientConnectionStatus(1, 0).serialize())
            self.radioConnected = True
        self.sendToMcu(message)

    def standby(self):
        "Put the sub server into standby mode"
        self.radioConnected = False
        self.sentToMcu(messages.ClientConnectionStatus(0,0).serialize())

    def sendToMcu(self, message):
        "Send one message (with header and footer) to MCU over serial link"
        self.mcu.write(self.SERIAL_HEADER + struct.pack('I', len(message)))
        self.mcu.write(message)
        self.mcu.flush()

    def run(self):
        "A single execution step for this thread"
        rawSerData = ""
        while True:
            rawSerData += self.mcu.read(6) # 6 bytes is the shortest it ever makes sense to read
            while len(rawSerData):
                if self.v > 9: sys.stdout.write("serRx: %s\n" % rawSerData)
                messageStart = rawSerData.find(self.SERIAL_HEADER)
                if messageStart == -1:
                    continue
                rawSerData = rawSerData[messageStart + len(self.SERIAL_HEADER):]
                if len(rawSerData) < 4: rawSerData += self.mcu.read(4 - len(rawSerData))
                if len(rawSerData) < 4: # Something's wrong
                    rawSerData = "" # Throw everything out and start over
                    continue
                length = struct.unpack('I', rawSerData[:4])[0]
                if self.v > 9: sys.stdout.write("\t length = %d\n" % length)
                length = min(1500, max(length, 4))
                rawSerData = rawSerData[4:]
                if len(rawSerData) < length:
                    #if self.v: sys.stdout.write("sr read %d\n" % length)
                    rawSerData += self.mcu.read(length - len(rawSerData))
                if len(rawSerData) < length: # Something is wrong
                    if self.v: sys.stdout.write("Insufficient serial data %d < %d" % (len(rawSerData), length))
                    rawSerData = "" # Throw everything out and start over
                    continue
                msgID = ord(rawSerData[0])
                if msgID == messages.PrintText.ID:
                    sys.stdout.write("M4: %s" % (rawSerData[1:length],)) # Print statement
                elif msgID == messages.RobotState.ID:
                    rsmts = struct.unpack('I', rawSerData[1:5])[0]
                    server.timestamp.update(rsmts) # Unpack the timestamp member of the RobotState message
                    if self.v:
                        sys.stdout.write("M4 RSM ts: %d\n" % rsmts)
                elif self.v:
                    sys.stdout.write("M4 pkt: %d[%d]\n" % (ord(rawSerData[0]), length))
                    #sys.stdout.write(repr([ord(c) for c in rawSerData[:10]]) + '\n')
                    sys.stdout.flush()
                self.server.clientSend(rawSerData[:length])
                rawSerData = rawSerData[length:]
