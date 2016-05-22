"""
UDP throughput tester, sends packets to a host and measures the time to and between replies.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys,os,socket,select,time,random,struct

class TPT:

    MIN_SIZE = 8
    MAX_SIZE = 1400

    def __init__(self, host, port, count=None, interval=None, size=None, bandwidth=None, summerize=None):
        """Sets up the through put tester
host:  The host name to connect to
port:  The port on the remote host to connect to
count: The number of packets to send before terminating or None for infinite

Specify 0-2 but not all three of interval size and bandwidth, the remaining
parameter(s) will be derrived or default.

interval:  The number of seconds between packets (default 1.0) and may be
           less than 1.0
size:      Number of bytes per packet. Range %d to %d, default %d.
bandwidth: Bandwidth to use in bytes per second, if used only interval or size
           may be specified, the other will be derrived.
summerize  Optional method of summerizing data. None, rxbw, pkts
""" % (self.MIN_SIZE, self.MAX_SIZE, self.MAX_SIZE)
        self.addr = (host, port)
        self.count = count
        self.size     = size     if size     is not None else self.MAX_SIZE
        self.interval = interval if interval is not None else 1.0
        if bandwidth is not None:
            if interval is None:
                self.interval = self.size / float(bandwidth)
            elif size is None:
                self.size = int(bandwidth / self.interval)
            sys.stdout.write("From bandwidth, will send %d every %f seconds\n" % (self.size, self.interval))
        if self.size > self.MAX_SIZE:
            sys.stdout.write("Clamping size to %d instead of %d\n" % (self.MAX_SIZE, self.size))
            self.size = self.MAX_SIZE
        elif size < self.MIN_SIZE:
            sys.stdout.write("Clamping size to %d instead of %d\n" % (self.MIN_SIZE, self.size))
            self.size = self.MIN_SIZE
        self.summerize = summerize
        # Internal member setup
        self.seqno = 0
        if self.size > self.MIN_SIZE:
            self.randEls = self.size-self.MIN_SIZE
            packFmt = "Q%dB" % self.randEls
        else:
            self.randEls = 0
            packFmt = "Q"
        self.packer = struct.Struct(packFmt)
        self.seqSendTimes = {}
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setblocking(False)

    def send(self):
        "Send one sequenced packet of the right size to the specified host:port"
        self.socket.sendto(self.packer.pack(self.seqno, *[random.randint(0,255) for r in range(self.randEls)]), self.addr)
        self.seqSendTimes[self.seqno] = time.time()
        self.seqno += 1

    def recv(self):
        "Accept any messages from the remote host"
        try:
            msg = self.socket.recv(self.MAX_SIZE)
        except socket.error:
            return None
        else:
            rt = time.time()
            try:
                msgData = self.packer.unpack(msg)
            except struct.error:
                sys.stdout.write("Received %d byte message that couldn't be unpacked\n" % len(msg))
                return False
            else:
                st = self.seqSendTimes.get(msgData[0])
                if st is None and self.summerize is None:
                    sys.stdout.write("Received unexpected packet of %d bytes\n" % len(msg))
                else:
                    if self.summerize is None:
                        sys.stdout.write("Msg %08d RTT %fms\n" % (msgData[0], (rt-st)*1000.0))
                    del self.seqSendTimes[msgData[0]]

    def step(self):
        t = time.time()
        self.send()
        while time.time() - t < self.interval:
            self.recv()

    def run(self):
        if self.count is None:
            while True:
                self.step()
        else:
            while self.seqno < self.count:
                self.step()
        sys.stdout.write("Dropped %d packets\n" % len(self.seqSendTimes))



def simpleTest(host, port):
    "A really simple test program"
    s = socket.socket(type=socket.SOCK_DGRAM)
    s.sendto(b"hello", (host, port))
    movingAvg = 6.0
    t1 = time.time()
    while True:
        d, a = s.recvfrom(1500)
        t2 = time.time()
        msDiff = (t2-t1)*1000
        sys.stdout.write("\r%f ms, %f ms avg" % (msDiff, movingAvg))
        movingAvg = msDiff*0.01 + 0.99*movingAvg
        t1=t2
