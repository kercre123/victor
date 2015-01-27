"""
Python UDP server to proxy messages to / from the MCU and the WiFi radio
"""
__author__  = "Daniel Canser"
__version__ = "0.0.1"

import sys, os, socket, time, serial, select
import messages



class MCUProxyServer(object):
    "Proxy server class"

    PORT = 9001
    CLIENT_TIMEOUT = 60.0 # One minute for right now
    SERIAL_DEVICE = "/dev/ttyO2"
    BAUD_RATE = 3000000
    MTU = 1500
    SERIAL_HEADER = "\xbe\xef"

    def __init__(self, poller, verbose=False):
        "Sets up the server instance"
        self.v=verbose
        if self.v: sys.stdout.write("MCUProxyServer will be verbose\n")
        self.poller = poller
        self.mcu = serial.Serial(port     = self.SERIAL_DEVICE,
                                 baudrate = self.BAUD_RATE,
                                 parity   = serial.PARITY_NONE,
                                 stopbits = serial.STOPBITS_ONE,
                                 bytesize = serial.EIGHTBITS,
                                 timeout  = 0.001 + (float(self.MTU)/self.BAUD_RATE))
        self.radioConnected = False
        self._toMcuQ   = []
        self._fromMcuQ = []
        self.poller.register(self.mcu, select.POLLIN)
        self.rawSerData = ""

    def __del__(self):
        "Send closeout messages"
        self.sendToMcu(messages.SoMRadioState(0,0).serialize())

    def __addToMcuQ(self, msg):
        self._toMcuQ.append(msg)
        self.poller.modify(self.mcu, select.POLLIN | select.POLLOUT) # Register for ready to tx
    def __getToMcuQ(self):
        if len(self._toMcuQ):
            ret = self._toMcuQ.pop(0)
            if len(self._toMcuQ) == 0:
                self.poller.modify(self.mcu, select.POLLIN) # Unregister since we have nothing to send
            return ret
        else:
            return None
    def __clearToMcuQ(self):
        self._toMcuQ = []
        self.poller.modify(self.mcu, select.POLLIN)
    toMcuQ = property(__getToMcuQ, __addToMcuQ, __clearToMcuQ, "Meta accessor for messages queued to the MCU, set to queue a message, get to pop one, del to clear the queue.")

    def __addFromMcuQ(self, msg):
        self._fromMcuQ.append(msg)
    def __getFromMcuQ(self):
        if len(self._fromMcuQ):
            return self._fromMcuQ.pop(0)
        else:
            return None
    def __clearFromMcuQ(self):
        self._fromMcuQ = []
    fromMcuQ = property(__getFromMcuQ, __addFromMcuQ, __clearFromMcuQ, "Meta accessor for messages queued from the MCU to the radio. Set to queue a message, get to pop, del to clear queue.")

    def standby(self):
        "Put the sub server into standby mode"
        self.radioConnected = False
        self.toMcuQ = messages.SoMRadioState(0,0).serialize()

    def poll(self, message=None):
        "Poll for this subserver, passing incoming message if any and returning outgoing message if any."
        # Handle incoming messages if any
        if message:
            if self.radioConnected is False: # If first message of connection
                self.toMcuQ = messages.SoMRadioState(1, 0).serialize() # Queue message to tell the MCU we have a connection
            self.toMcuQ = message # Queue the message
        # Handle serial port
        self.sendToMcu(self.toMcuQ)
        self.recvFromMcu()
        return self.fromMcuQ

    def sendToMcu(self, message):
        "Send one message (with header and footer) to MCU over serial link"
        if message is not None: # Skip if no message to send
            length = len(message)
            self.mcu.write(self.SERIAL_HEADER + ''.join([chr((length >> i) & 0xff) for i in (0, 8, 16, 24)]))
            self.mcu.write(message)
            self.mcu.flush()

    def recvFromMcu(self):
        "Read the serial connection to the MCU and append any messages to the receive queue"
        self.rawSerData += self.mcu.read(self.MTU)
        while len(self.rawSerData):
            if self.v > 10: sys.stdout.write("serRx: %s\n" % self.rawSerData)
            messageStart = self.rawSerData.find(self.SERIAL_HEADER)
            if messageStart == -1:
                return
            self.rawSerData = self.rawSerData[messageStart + len(self.SERIAL_HEADER):]
            if len(self.rawSerData) < 4: self.rawSerData += self.mcu.read(4 - len(data))
            if len(self.rawSerData) < 4: # Somethings wrong
                self.rawSerData = "" # Throw everything out and start over
                return
            length = sum([ord(d) << i for d, i in zip(self.rawSerData, (0, 8, 16, 24))]) # Calculate message length
            self.rawSerData = self.rawSerData[4:]
            if len(self.rawSerData) < length: self.rawSerData += self.mcu.read(length - len(data))
            if len(self.rawSerData) < length: # Something is wrong
                self.rawSerData = "" # Throw everything out and start over
                return
            if self.v: sys.stdout.write("New packet from mcu: %s\n" % self.rawSerData[:length])
            self.fromMcuQ = self.rawSerData[:length]
            self.rawSerData = self.rawSerData[length:]
