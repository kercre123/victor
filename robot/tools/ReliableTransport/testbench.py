"""
Test bench code for python reliableTransport implementation.
WARN: This code is written and tested against Python 3
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, threading, serial, struct
from ReliableTransport import reliableTransport, udpTransport

class TestDataReceiver(reliableTransport.IDataReceiver):
    "A simple data receiver object implementing required callbacks"
    def __init__(self, unreliableTransport):
        self.transport = reliableTransport.ReliableTransport(unreliableTransport, self)
        self.transport.start()

    def send(self, *args):
        "Pass send args along to transport"
        self.transport.SendData(*args)

    def OnConnectionRequest(self, sourceAddress):
        sys.stdout.write("%s: connection request from: %s\r\n" % (repr(self), str(sourceAddress)))
        self.transport.FinishConnection(sourceAddress)
    def OnConnected(self, sourceAddress):
        sys.stdout.write("%s: connection response from: %s\r\n" % (repr(self), str(sourceAddress)))
    def OnDisconnected(self, sourceAddress):
        sys.stdout.write("%s: disconnected from %s\r\n" % (repr(self), str(sourceAddress)))
    def ReceiveData(self, buffer, sourceAddress):
        if not len(buffer):
            sys.stdout.write("%s: got empty message from %s\r\n" % (repr(self), str(sourceAddress)))
        else:
            sys.stdout.write("%s: got \"%s\" from %s\r\n" % (repr(self), buffer, str(sourceAddress)))

    def __repr__(self):
        return threading.currentThread().name

class UartSimRadio(serial.Serial, reliableTransport.IUnreliableTransport):
    "A simulated radio / unreliable transport link via serial"

    MAX_MESSAGE_SIZE = 1472 # Match UDP

    SERIAL_PREFIX = b"\xbe\xef"

    PACKET_PREFIX = b"COZ\x03"

    def __init__(self, port, baud, printOther=True, printAll=False):
        serial.Serial.__init__(self, port     = port,
                                     baudrate = baud,
                                     parity   = serial.PARITY_NONE,
                                     stopbits = serial.STOPBITS_ONE,
                                     bytesize = serial.EIGHTBITS,
                                     timeout  = 0.001)
        reliableTransport.IUnreliableTransport.__init__(self)
        self.printOther = printOther
        self.printAll   = printAll
        self.buffer = b""
        self.pktLenPacker = struct.Struct("I")
        self.SERIAL_PACKET_HEADER_LEN = len(self.SERIAL_PREFIX) + self.pktLenPacker.size

    def __del__(self):
        self.close()

    def OpenSocket(self, *args):
        assert len(args) == 0 or args[0] == self.port

    def SendData(self, destAddress, message):
        assert destAddress == self.port
        serPkt = b"".join([self.SERIAL_PREFIX, self.pktLenPacker.pack(len(message)+len(self.PACKET_HEADER)), self.PACKET_HEADER, message])
        if self.printAll:
            sys.stdout.write("TX: %s\r\n" % serPkt.decode("ASCII", "ignore"))
        self.write(serPkt)

    def ReceiveData(self):
        self.buffer += self.read(1500)
        pktStart = self.buffer.find(self.SERIAL_PREFIX)
        if pktStart < 0: # Header not found
            if self.printOther:
                sys.stdout.write(self.buffer[:-len(self.SERIAL_PREFIX)].decode("ASCII", "ignore"))
            self.buffer = self.buffer[-len(self.SERIAL_PREFIX):]
            return None, None
        elif pktStart > 0: # bytes before header
            if self.printOther:
                sys.stdout.write(self.buffer[:pktStart].decode("ASCII", "ignore"))
            self.buffer = self.buffer[pktStart:]
        if len(self.buffer) < self.SERIAL_PACKET_HEADER_LEN: # Not enough bytes yet
            return None, None
        else:
            pktLen = self.pktLenPacker.unpack(self.buffer[len(self.SERIAL_PREFIX):self.SERIAL_PACKET_HEADER_LEN])[0]
            if pktLen > self.MAX_MESSAGE_SIZE:
                sys.stderr.write("Bad serial packet length %d\r\n" % pktLen)
                self.buffer = self.buffer[self.SERIAL_PACKET_HEADER_LEN:]
                return None, None
            elif pktLen + self.SERIAL_PACKET_HEADER_LEN > len(self.buffer):
                return None, None # Not enough bytes yet
            else:
                ret = self.buffer[self.SERIAL_PACKET_HEADER_LEN:self.SERIAL_PACKET_HEADER_LEN + pktLen]
                self.buffer = self.buffer[self.SERIAL_PACKET_HEADER_LEN + pktLen:]
                if self.printAll:
                    sys.stdout.write("RX: %s\r\n" % ret.decode("ASCII", "ignore"))
                if ret.startswith(self.PACKET_PREFIX):
                    return ret[len(self.PACKET_PREFIX):], self.port
                else:
                    sys.stderr.write("UartSimRadio got packet with incorrect header:\r\n\t%s\r\n" % ret.decode("ASCII", "ignore"))
                    return None, None


def robotSetup(host=("172.31.1.1", 5551), *args):
    if type(host) is tuple:
        sock = udpTransport.UDPTransport()
        sock.OpenSocket(("", 6551), *args)
    elif type(host) is str:
        sock = UartSimRadio(host, *args)
    rcvr = TestDataReceiver(sock)
    rcvr.transport.Connect(host)
    return rcvr


if __name__ == '__main__':
    if False:
        localhost = "127.0.0.1"
        urt1 = udpTransport.UDPTransport()
        urt1.OpenSocket(6551, localhost)
        urt2 = udpTransport.UDPTransport()
        urt2.OpenSocket(6552, localhost)

        tdr1 = TestDataReceiver(urt1)
        tdr2 = TestDataReceiver(urt2)

        tdr1.transport.Connect((localhost, 6552))

        time.sleep(0.5)

        tdr1.send(False, False, (localhost, 6552), b"This is an unreliable bessage from 1 to 2")
        tdr1.send(True,  False, (localhost, 6552), b"This is a reliable message from 1 to 2")
        tdr2.send(True,  False, (localhost, 6551), b"And this is a reliable message from 2 to 1")

        time.sleep(6)

        sys.stdout.write(tdr1.transport.Print())
        sys.stdout.write(tdr2.transport.Print())

        tdr1.transport.KillThread()
        tdr2.transport.KillThread()
        sys.exit()
    if True:
        r = robotSetup("com4", 115200, True, False)
