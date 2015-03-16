#!/usr/bin/python
"""
Implements UDP server for robot comms and includes some useful library classes
"""
__author__ = "Daniel Casner"


import sys, os, time, socket, threading, subprocess
import camServer, mcuProxyServer, blockProxy

VERBOSE = False
PRINT_INTERVAL = False
PRINT_FRAMERATE = False
FORCE_ADDRESS = False

MTU = 1500

class TimestampExtrapolator(object):
    "A threadsafe class for extrapolating timestamps from the MCU"

    def __init__(self, ticks_per_second=1000):
        self.mcuTS = 0
        self.toc = time.time()
        self.tps = ticks_per_second
        self.lock = threading.Lock()

    def update(self, tick):
        "Update the local copy of the MCU time"
        toc = time.time()
        self.lock.acquire()
        self.mcuTS = tick
        self.toc = toc
        self.lock.release()

    def get(self):
        "Retrive the estimated MCU time"
        self.lock.acquire()
        ret = self.mcuTS + int((time.time()-self.toc)*self.tps)
        self.lock.release()
        return ret


class CozmoServer(socket.socket):
    "Cozmo UDP robot comms server"

    CLIENT_IDLE_TIMEOUT = 10.0

    def __init__(self, address):
        "Initalize the server and start listening on UDP"
        if VERBOSE: sys.stdout.write("Server will be verbose\n")
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.settimeout(1.0)
        self.bind(address)
        self.timestamp = TimestampExtrapolator()
        cam = camServer.CameraSubServer(self, VERBOSE, PRINT_FRAMERATE)
        mcu = mcuProxyServer.MCUProxyServer(self, VERBOSE)
        blk = blockProxy.BlockProxyServer(self, VERBOSE)
        self.subServers = [cam, mcu, blk]
        self.client = None
        self.lastClientRecvTime = 0.0
        subprocess.call(['renice', '-n', '10', '-p', str(os.getpid())]) # Renice ourselves so the encoder comes first

    def __del__(self):
        self.stop()

    def stop(self):
        self.client = None
        for ss in self.subServers:
            ss.stop()
        self.close()

    def step(self):
        "One main loop iteration"
        try:
            data, client = self.recvfrom(MTU)
        except socket.timeout:
            pass
        except socket.error, e:
            if e.errno == socket.EBADF and self.client is None:
                return
            else:
                raise e
        else:
            if FORCE_ADDRESS:
                self.client = FORCE_ADDRESS
            else:
                self.client = client
            if VERBOSE: sys.stdout.write("UDP pkt: %d[%d]\n" % (ord(data[0]), len(data)))
            self.lastClientRecvTime = time.time()
            if data[0] == '0' and len(data) == 1: # This is a special "non message" send by UdpClient.cpp
                return # Establish that we have a connection but don't pass the packet along
            for ss in self.subServers:
                ss.giveMessage(data)
        if self.client and (time.time() - self.lastClientRecvTime > self.CLIENT_IDLE_TIMEOUT):
            sys.stdout.write("Going to standby\n")
            sys.stdout.flush()
            for ss in self.subServers:
                ss.standby()
            self.client = None

    def run(self):
        "Run main loop with target frequency"
        for ss in self.subServers:
            ss.start()
        while True:
            self.step()


if __name__ == '__main__':
    if '-v'  in sys.argv: VERBOSE = True
    if '-vv' in sys.argv: VERBOSE = 10
    if '-i'  in sys.argv: PRINT_INTERVAL = True
    if '-f'  in sys.argv: PRINT_FRAMERATE = True
    if '-a'  in sys.argv:
        aid = sys.argv.index('-a')
        FORCE_ADDRESS = eval(sys.argv[aid+1])
        sys.stdout.write("Forcing packets to go to: %s:%d\n" % FORCE_ADDRESS)
    address = ('', 5551)
    server = CozmoServer(address)
    sys.stdout.write("Starting server listening at ('%s', %d)\n" % address)
    try:
        server.run()
    except KeyboardInterrupt:
        sys.stdout.write("Shutting down server\n")
        server.stop()
        sys.stdout.write("Exiting\n")
        sys.stdout.flush()
        exit(0)
