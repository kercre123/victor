"""
Python UDP server to proxy messages to / from the MCU and the WiFi radio
"""
__author__  = "Daniel Canser"
__version__ = "0.0.1"

import sys, os, socket, time, serial
import messages

MTU = 1500

class MCUInterface(serial.Serial):
    "Class wrapper around MCU interface"

    SERIAL_DEVICE = 0
    BAUD_RATE = 3000000


    def __init__(self, serialDevice=self.SERIAL_DEVICE):
        "Initalize the interface instance"
        serial.Serial.__init__(self,
                               port     = serialDevice,
                               baudrate = self.BAUD_RATE,
                               parity   = serial.PARITY_NONE,
                               stopbits = serial.STOPBITS_ONE,
                               bytesize = serial.EIGHTBITS,
                               timeout  = 0.001 + (float(MTU)/self.BAUD_RATE))
        # TODO configure GPIO pin

    def xfer(self, toMcu):
        "Transfer data to and from the MCU"
        self.read() # Throw away anything lingering in the receive buffer to maintain sync
        self.slaveSelect(True) # Set the sync line
        if toMcu:
            self.write(toMcu) # Write outgoing data
            self.flush()      # Make sure it's all writen
        else:
            time.sleep(0.001)
        self.slaveSelect(False) # Deassert the sync
        fromMcu = self.read() # Read and return the resposne
        if len(fromMcu):
            return fromMcu
        else:
            return None


class MCUProxyServer(object):
    "Proxy server class"

    PORT = 9001
    CLIENT_TIMEOUT = 60.0 # One minute for right now

    def __init__(self, poller, serialDevice=MCUInterface.SERIAL_DEVICE):
        "Sets up the server instance"
        self.mcu = MCUInterface(serialDevice)
        self.radioConnected = False

    def __del__(self):
        "Send closeout messages"
        self.standby()

    def standby(self):
        "Put the sub server into standby mode"
        self.radioConnected = False
        self.mcu.xfer(messages.SoMRadioSate(0,0).serialize())

    def poll(self, message=None):
        "Poll for this subserver, passing incoming message if any and returning outgoing message if any."
        # Handle incoming messages if any
        if message and self.radioConnected is False: # If first message of connection
            self.mcu.xfer(messages.SoMRadioState(1, 0).serialize()) # Tell the MCU we have a connection
        return self.mcu.xfer(message)

    def recv(self):
        try:
            recvData, addr = self.recvfrom(MTU)
        except socket.timeout:
            if time.time() - self.lastRecvTime > self.CLIENT_TIMEOUT:
                if self.client:
                    self.client = None
                    self.mcu.xfer(messages.SoMRadioState(0, 0).serialize())
            return ""
        else:
            self.lastRecvTime = time.time()
            if addr != self.client:
                self.client = addr
                fromMcu = self.mcu.xfer(messages.SoMRadioState(1, 0).serialize())
                if fromMcu:
                    self.send(fromMcu)
            return recvData

    def send(self, data):
        if self.client:
            self.sendto(data, self.client)
            return len(data)
        else:
            return 0

    def step(self):
        "One main loop iteration"
        recvData = self.recv()
        fromMcu = self.mcu.xfer(recvData)
        if fromMcu:
            self.sock.send(fromMcu)
