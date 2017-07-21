#!/usr/bin/env python2
"""
Simulates traffic to / from the robot to test the serial radio
"""
__author__ = "Daniel Casner"

import sys, os, struct, time, threading
import serRadio

# This is a hack until we get CLAD based python messages in a real path
if os.path.split(os.path.abspath(os.path.curdir))[-1] != 'testbench':
    sys.stderr.write("robosim bust be run from\r\n")
    sys.exit(1)
try:
    sys.path.insert(0, os.path.join('..', '..', 'som'))
    import messages
except:
    sys.stderr.write("Couldn't import python message definitions\r\n")
    sys.exit(1)

CAMERA_FRAME_RATE = 1.0
RADIO_TICK_INTERVAL = 1.0/CAMERA_FRAME_RATE


class BaseStationSimulator(threading.Thread, serRadio.UDPClient):
    "A simulator thread for the base station"

    def __init__(self, host="172.31.1.1", port=5551):
        "Sets up the simulated base station, specify host:port of robot"
        threading.Thread.__init__(self)
        serRadio.UDPClient.__init__(self, host, port)
        self.receiveCB = None
        self.sendCB = None
        self._continue = True
        self.pingPkt = messages.PingMessage().serialize()

    def __del__(self):
        "close down the simulator"
        self.stop()
        self.close()

    def step(self):
        "One main loop step for the base station"
        self.sendto(self.pingPkt, self.addr) # Send ping
        if self.sendCB:
            self.sendCB(self.pingPkt)
        incoming = self.receive()
        if incoming:
            if self.receiveCB:
                self.receiveCB(incoming)
            else:
                try:
                    sys.stdout.write("Basestation:\t%02d[%d]\r\n" % (ord(incoming[0]), len(incoming)))
                except:
                    sys.stderr.write("Basestation:\tmalformed packet %s... [%d]\r\n" % (incoming[:8], len(incoming)))
        time.sleep(RADIO_TICK_INTERVAL)

    def run(self):
        "Run function for the thread"
        self.connect()
        while self._continue:
            self.step()
            time.sleep(0)

    def stop(self):
        "Request thread execution stop"
        self._continue = False


class RobotSimulator(threading.Thread, serRadio.UARTRadioConn):
    "A simulator thread for the robot"

    CHUNKS_PER_FRAME = 7
    ICM_INTERVAL = 1.0/(CHUNKS_PER_FRAME*CAMERA_FRAME_RATE/2.0)
    RSM_INTERVAL = 1.0/CAMERA_FRAME_RATE

    def __init__(self, serial_device):
        "Sets up the simulated robot, specify serial device"
        threading.Thread.__init__(self)
        serRadio.UARTRadioConn.__init__(self, serial_device)
        self.receiveCB = None
        self.sendCB = None
        self._continue = True
        self.icm = messages.ImageChunk()
        self.rsm = messages.RobotState()
        self.lastICMTime = 0.0
        self.lastRSMTime = 0.0
        self.icm.imageChunkCount = self.CHUNKS_PER_FRAME

    def __del__(self):
        "Close down the simulator"
        self.stop()
        self.close()

    def transmit(self, pkt):
        "Transmit a packet over serial"
        self.write("\xbe\xef%s%s" % (struct.pack("I", len(pkt)), pkt))

    def step(self):
        "One main loop iteration for the robot sim"
        t = time.time()
        ti = int(t*1000) & 0xFFFFffff
        # Transmit
        if t - self.lastICMTime > self.ICM_INTERVAL:
            if self.icm.chunkId == self.CHUNKS_PER_FRAME-1:
                self.icm.imageId += 1
                self.icm.chunkId = 0
            else:
                self.icm.chunkId += 1
            self.icm.imageTimestamp = ti
            self.transmit(self.icm.serialize())
            self.lastICMTime = t
            if self.sendCB:
                self.sendCB(self.icm)
        elif t - self.lastRSMTime > self.RSM_INTERVAL:
            self.rsm.timestamp = ti
            self.rsm.poseFrameId += 1
            self.transmit(self.rsm.serialize())
            self.lastRSMtime = t
            if self.sendCB:
                self.sendCB(self.rsm)
        # Receive
        incoming = self.read(1500)
        if incoming:
            if self.receiveCB:
                self.receiveCB(incoming)
            else:
                try:
                    sys.stdout.write("Robot:\t\t%02d[%d]\r\n" % (ord(incoming[6]), len(incoming)))
                except:
                    sys.stderr.write("Robot:\t\tmalformed packet %s... [%d]\r\n" % (incoming[:8], len(incoming)))

    def run(self):
        "Run function for the thread"
        self.lastICMTime = time.time() # clear timers
        self.lastRSMTime = time.time()
        while self._continue:
            self.step()
            time.sleep(0)

    def stop(self):
        "Request thread execution stop"
        self._continue = False


class LossTester(object):

    def __init__(self, host, port, com):
        self.b = BaseStationSimulator(host, port)
        self.r = RobotSimulator(com)
        self.b.receiveCB = self.brcb
        self.b.sendCB    = self.bscb
        self.r.receiveCB = self.rrcb
        self.r.sendCB    = self.rscb
        self.robotTxBalance = 0
        self.basestationTxBalance = 0

    def brcb(self, arg):
        self.robotTxBalance -= 1

    def bscb(self, arg):
        self.basestationTxBalance += 1

    def rrcb(self, arg):
        self.basestationTxBalance -= 1

    def rscb(self, arg):
        self.robotTxBalance += 1

    def report(self):
        sys.stdout.write("R->B %d\t B->R %d\n" % (self.robotTxBalance, self.basestationTxBalance))

    def run(self):
        self.b.start()
        self.r.start()
        intvl = 1.0/10.0
        while True:
            self.report()
            time.sleep(intvl)

    def stop(self):
        self.b.stop()
        self.r.stop()

def runLoss(host="172.31.1.1", port=5551, com="com11"):
    lt = LossTester(host, port, com)
    try:
        lt.run()
    except KeyboardInterrupt:
        lt.stop()
        return
    finally:
        lt.stop()

def runSimple(host="172.31.1.1", port=5551, com="com11"):
    "Run a simple simulator"
    b = BaseStationSimulator(host, port)
    r = RobotSimulator(com)
    try:
        b.start()
        r.start()
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        b.stop()
        r.stop()
        return


if __name__ == '__main__':
    run()
    sys.exit(0)
