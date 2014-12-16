"""
Implements UDP server for robot comms and includes some useful library classes
"""
__author__ = "Daniel Casner"


import sys, os, time, socket, select
import camServer

MTU = 1500

CLIENT_IDLE_TIMEOUT = 100.0

class CozmoServer(socket.socket):
    "Cozmo UDP robot comms server"

    CLIENT_IDLE_TIMEOUT = 100.0

    def __init__(self, address=('', 9000)):
        "Initalize the server and start listening on UDP"
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.bind(address)
        self.poller = select.poll()
        self.poller.register(self, select.POLLIN)
        self.subServers = [camServer.CameraSubServer(self.poller)] # Add MCU proxy sub server to list
        self.client = None
        self.lastClientRecvTime = 0.0

    def recv(self, maxLen):
        data, self.client = self.recvfrom(maxLen)

    def send(self, data):
        if self.client:
            self.sendto(data, self.client)

    def step(self, timeout=None):
        "One main loop iteration"
        rdyList = self.poller.poll(timeout)
        if (self, select.POLLIN) in rdyList:
            recvData = self.recv(MTU)
            self.lastClientRecvTime = time.time()
        else:
            recvData = None
        for ss in self.subServers:
            outMsg = ss.poll(recvData)
            if outMsg:
                self.send(outMsg)
        if self.client and (time.time() - self.lastClientRecvTime > self.CLIENT_IDLE_TIMEOUT):
            for ss in self.subServers:
                ss.standby()
            self.client = None

    def run(self, loopHz):
        "Run main loop with target frequency"
        targetPeriod = 1.0/loopHz
        st = time.time()
        while True:
            self.step(max(0, targetPriod - (time.time()-st)))
            st = time.time()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    else:
        port = 9000
    if len(sys.argv) > 2:
        host = sys.argv[2]
    else:
        host = ''
    server = CozmoServer((host, port))
    try:
        server.run()
    except KeyboardInterrupt:
        pass
