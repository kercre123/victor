"""
As simple as possible implementation of Anki UDPTransport class in python for testing and scripting.
This impementation uses an ANK3 header for Cozmo and does not calulate the CRC, the bytes are left 0.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, socket, threading
from ReliableTransport.reliableTransport import IUnreliableTransport

class UDPTransport(socket.socket, IUnreliableTransport):
    "A thin wrapper around socket.socket setting Anki params and header"

    MTU = 1500
    MAX_MESSAGE_SIZE = 1472  # Assuming a an MTU of 1500, minus (20+8) for the underlying IP and UDP headers, this should be optimal for most networks/users

    DEFAULT_PORT = 47817

    def __init__(self):
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        IUnreliableTransport.__init__(self)
        self.settimeout(0) # Make the socket non-blocking

    def OpenSocket(self, port=DEFAULT_PORT, interface=''):
        "Bind the socket so we can receive data"
        return self.bind((interface, port))

    def CloseSocket(self):
        "Close the underlying socket"
        return self.close()

    def SendData(self, destAddress, message):
        "Wraps a packet with header and sends it to the destination"
        if len(message) > self.MAX_MESSAGE_SIZE:
            raise ValueError("Max message size for UDPTransport is %d, given %d" % (self.MAX_MESSAGE_SIZE, len(message)))
        else:
            #print("%s--> %s" % (threading.currentThread().name, self.PACKET_HEADER + message))
            self.sendto(self.PACKET_HEADER + message, destAddress)

    def ReceiveData(self):
        """Receive a packet if any
If a packet is available returns data, sourceAddres. Otherwise returns None, None"""
        try:
            data, addr = self.recvfrom(self.MTU)
        except socket.timeout:
            return None, None
        except BlockingIOError:
            return None, None
        else:
            #print("%s<-- %s" % (threading.currentThread().name, data))
            if data.startswith(self.PACKET_HEADER):
                return data[len(self.PACKET_HEADER):], addr
            else:
                sys.stderr.write("UDPTransport: received packed with invalid header")
                return None, None


if __name__ == '__main__':
    "Self test for module"
    testData1 = b"foobarbaz"
    testData2 = b"fizbuzz57"
    alice = UDPTransport()
    alice.OpenSocket()
    bob = UDPTransport()
    bob.SendData(('localhost', alice.DEFAULT_PORT), testData1)
    d, a = alice.ReceiveData()
    assert(d == testData1)
    alice.SendData(a, testData2)
    d, a = bob.ReceiveData()
    assert(d == testData2)
    sys.stdout.write("PASS\n")
