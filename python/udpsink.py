"""
Test udp sink receiver for gstreamer video.
"""
__author__ = "Daniel Casner"


import sys, os, socket

MTU = 65535

class UDPSink:

    def __init__(self, outFileName, interface, port, debugLevel=0, numPackets=None):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind((interface, port))
        self.debugLevel = debugLevel
        self.numPackets = numPackets
        if numPackets is None:
            self.fh = file(outFileName, 'wb')
        else:
            self.basename = os.path.splitext(outFileName)[0]
            self.packetsCaptured = 0

    def stepStream(self):
        recvData, addr = self.socket.recvfrom(MTU)
        if self.debugLevel > 0:
            sys.stdout.write('%s %d bytes\n' % (str(addr), len(recvData)))
        self.fh.write(recvData)

    def stepPacket(self):
        recvData, addr = self.socket.recvfrom(MTU)
        fn = '%s_%04d.pkt' % (self.basename, self.packetsCaptured)
        if self.debugLevel > 0:
            sys.stdout.write('%s << %d bytes\n' % (fn, len(recvData)))
        fh = open(fn, 'wb')
        fh.write(recvData)
        fh.close()
        self.packetsCaptured += 1

    def run(self):
        if self.numPackets is None:
            while True: self.stepStream()
        else:
            while self.packetsCaptured < self.numPackets:
                self.stepPacket()



if __name__ == '__main__':
    if len(sys.argv) > 1:
        outFileName = sys.argv[1]
    else:
        outFileName = 'test.avi'
    if len(sys.argv) > 2:
        interface = sys.argv[2]
    else:
        interface = ''
    if len(sys.argv) > 3:
        port = int(sys.argv[3])
    else:
        port = 9100
    if len(sys.argv) > 4:
        numPackets = int(sys.argv[4])
    else:
        numPackets = None
    sys.stdout.write("Starting UDPsink\n")
    sink = UDPSink(outFileName, interface, port, 10, numPackets)
    try:
        sink.run()
    except KeyboardInterrupt:
        sys.exit(0)
