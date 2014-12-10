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
                               timeout  = 0.001 + float(MTU)/self.BAUD_RATE)

    def xfer(self, toMcu):
        "Transfer data to and from the MCU"
        self.read() # Throw away anything lingering in the receive buffer to maintain sync
        self.slaveSelect(True) # Set the sync line
        self.write(toMcu) # Write outgoing data
        self.flush()      # Make sure it's all writen
        self.slaveSelect(False) # Deassert the sync
        return self.read() # Read and return the resposne


class MCUProxyServer(socket.socket):
    "Proxy server class"

    PORT = 9001
    CLIENT_TIMEOUT = 60.0 # One minute for right now

    def __init__(self, networkPort=self.PORT, serialDevice=MCUInterface.SERIAL_DEVICE):
        "Sets up the server instance"
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.bind(('', networkPort))
        self.settimeout(0.002) # 2 ms
        self.mcu = MCUInterface(serialDevice)
        self.client = None
        self.lastRecvTime = 0.0

    def recv(self):
        try:
            recvData, addr = self.recvFrom(MTU)
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
