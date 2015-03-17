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

class Logger(threading.Thread):
    "A simple (but thread safe) log size manager and file rotator for Cozmo server."

    ACTIVE_FILE = 'cozmo_server_log.txt'
    ROTATE_FILE = 'cozmo_server_log1.txt'

    def __init__(self, file_size_limit=1000000):
        threading.Thread.__init__(self)
        self.fh = open(self.ACTIVE_FILE, 'w')
        self.fh.write(time.ctime() + '\n')
        self.fh.flush()
        self.bytes = 0
        self.WQ = []
        self.fileSizeLimit = file_size_limit
        self.qLock = threading.Lock()
        self.fLock = threading.Lock()
        self._continue = True

    def __del__(self):
        self.stop()
        self.fh.close()

    def stop(self):
        sys.stdout.write("Stopping logging thread\n")
        self._continue = False
        self.flush()

    def write(self, data):
        "Add data to the log"
        sys.stdout.write(data) # Echo to console
        self.qLock.acquire()
        self.WQ.append(data)
        self.qLock.release()

    def flush(self):
        "Write queued data to disk"
        self.fLock.acquire()
        # Exchange to queue quickly so as not to block other tasks for long
        self.qLock.acquire()
        q = self.WQ
        self.WQ = []
        self.qLock.release()
        # Now that we've got our own queue
        if q: # If there is anything to write out
            data = ''.join(q)
            self.fh.write(data)
            self.fh.flush()
            self.bytes += len(data)
            if self.bytes > self.fileSizeLimit: # Size exceeded, rotate log
                # Rotate the log files
                self.fh.close()
                if os.path.isfile(self.ROTATE_FILE):
                    os.remove(self.ROTATE_FILE)
                os.rename(self.ACTIVE_FILE, self.ROTATE_FILE)
                self.fh = open(self.ACTIVE_FILE, 'w')
                self.fh.write(time.ctime() + '\n')
                self.bytes = 0
        self.fLock.release()

    def run(self):
        "Thread to flush queued data to disk"
        while self._continue:
            self.flush()
            time.sleep(2.0) # Don't need to write to disk too often


class CozmoServer(socket.socket):
    "Cozmo UDP robot comms server"

    CLIENT_IDLE_TIMEOUT = 10.0

    def __init__(self, address):
        "Initalize the server and start listening on UDP"
        self.logger = Logger()
        if VERBOSE: self.logger.write("Server will be verbose\n")
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
        self.logger.stop()

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
            if VERBOSE: self.logger.write("UDP pkt: %d[%d]\n" % (ord(data[0]), len(data)))
            self.lastClientRecvTime = time.time()
            if data[0] == '0' and len(data) == 1: # This is a special "non message" send by UdpClient.cpp
                return # Establish that we have a connection but don't pass the packet along
            for ss in self.subServers:
                ss.giveMessage(data)
        if self.client and (time.time() - self.lastClientRecvTime > self.CLIENT_IDLE_TIMEOUT):
            self.logger.write("Going to standby\n")
            sys.stdout.flush()
            for ss in self.subServers:
                ss.standby()
            self.client = None

    def run(self):
        "Run main loop with target frequency"
        self.logger.start()
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
    server.logger.write("Starting server listening at ('%s', %d)\n" % address)
    try:
        server.run()
    except KeyboardInterrupt:
        server.logger.write("Shutting down server\n")
        server.stop()
        server.logger.write("Exiting\n")
        server.logger.flush()
        exit(0)
