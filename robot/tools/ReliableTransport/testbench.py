"""
Test bench code for python reliableTransport implementation.
WARN: This code is written and tested against Python 3
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, threading, struct
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


def robotSetup(host=("172.31.1.1", 5551), *args):
    sock = udpTransport.UDPTransport()
    sock.OpenSocket(("", 6551), *args)
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
