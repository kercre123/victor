#!/usr/bin/python
"""
Implements UDP server for robot comms and includes some useful library classes
"""
__author__ = "Daniel Casner"


import sys, os, time, socket, select
import camServer, mcuProxyServer

VERBOSE = False
PRINT_INTERVAL = False
PRINT_FRAMERATE = False

MTU = 1500

class CozmoServer(socket.socket):
    "Cozmo UDP robot comms server"

    CLIENT_IDLE_TIMEOUT = 1.0

    def __init__(self, address):
        "Initalize the server and start listening on UDP"
        if VERBOSE: sys.stdout.write("Server will be verbose\n")
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.bind(address)
        self.settimeout(0)
        self.poller = select.poll()
        self.poller.register(self, select.POLLIN)
        camServer = camServer.CameraSubServer(self.poller, VERBOSE, PRINT_FRAMERATE)
        mcu = mcuProxyServer.MCUProxyServer(self.poller, VERBOSE)
        self.subServers = [camServer, mcu]
        mcu.timestampCB = camServer.updateTimestamp # Setup crosslink for timestamps, hateful spaghetti
        self.client = None
        self.lastClientRecvTime = 0.0


    def fprintf(self, fh, fmt, *params):
        "Writes to file handle"
        fh.write(fmt % params)

    def printOut(self, fmt, *params):
        "Writes to stdout"
        self.fprintf(sys.stdout, fmt, *params)

    def printErr(self, fmt, *params):
        "Writes to stderr"
        self.fprintf(sys.stderr, fmt, *params)

    def clientRecv(self, maxLen):
        try:
            data, self.client = self.recvfrom(maxLen)
            if VERBOSE: sys.stdout.write("UDP pkt: %d[%d]\n" % (ord(data[0]), len(data)))
        except:
            return None
        else:
            self.lastClientRecvTime = time.time()
            if data[0] == '0' and len(data) == 1: # This is a special "non message" send by UdpClient.cpp
                return None # Establish that we have a connection but don't pass the packet along
            else:
                return data

    def clientSend(self, data):
        if self.client:
            self.sendto(data, self.client)

    def step(self, timeout=None):
        "One main loop iteration"
        self.poller.poll(timeout)
        recvData = self.clientRecv(MTU)
        for ss in self.subServers:
            outMsg = ss.poll(recvData)
            if outMsg:
                self.clientSend(outMsg)
        if self.client and (time.time() - self.lastClientRecvTime > self.CLIENT_IDLE_TIMEOUT):
            sys.stdout.write("Going to standby\n")
            sys.stdout.flush()
            for ss in self.subServers:
                ss.standby()
            self.client = None

    def run(self, loopHz):
        "Run main loop with target frequency"
        targetPeriod = 1.0/loopHz
        while True:
            st = time.time()
            self.step(targetPeriod)
            if PRINT_INTERVAL: sys.stdout.write('%d ms\n' % int((time.time()-st)*1000))


if __name__ == '__main__':
    if '-v'  in sys.argv: VERBOSE = True
    if '-vv' in sys.argv: VERBOSE = 10
    if '-i'  in sys.argv: PRINT_INTERVAL = True
    if '-f'  in sys.argv: PRINT_FRAMERATE = True
    address = ('', 5551)
    server = CozmoServer(address)
    sys.stdout.write("Starting server listening at ('%s', %d)\n" % address)
    try:
        server.run(200)
    except KeyboardInterrupt:
        sys.stdout.write("Shutting down server\n")
