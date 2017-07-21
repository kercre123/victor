#!/usr/bin/env python2
"""
A test script to go with the ESP8266 UDP throughput test firmware.
This script is specifically for sending formatted packets over UART for relay to UDP.
"""
__author__ = "Daniel Casner"

import sys, time, serial, socket, struct, random

def PacketGenerator(payloadLength, startSequenceNumber=0):
    "A generator for packets. payloadLength specifies how long the packet payload should be, minimum 8 bytes"
    text = open('test_data.txt', 'r')
    payloadLength = max(8, payloadLength)
    strlen = payloadLength - 8
    packer = struct.Struct("<Q%ds" % strlen)
    seqno = startSequenceNumber
    while True:
        try:
            pld = text.read(strlen)
        except:
            break
        yield seqno, packer.pack(seqno, pld)
        seqno += 1

def unpackSeqno(pkt):
    assert len(pkt) >= 8
    return struct.unpack("Q", pkt[:8])[0]

class UARTRadioConn(serial.Serial):
    "A subclass of Serial with the right parameters"
    def __init__(self, port="/dev/ttyUSB0"):
        serial.Serial.__init__(self, port, 3000000, timeout=0)
        self.data = ""

    def receive(self):
        "Search for a serial packet and return it if one has been found, else return"
        self.data += self.read(1500)
        pktStart = self.data.find(b"\xbe\xef")
        if pktStart < 0:
            return None
        else:
            self.data = self.data[pktStart:]
            if len(self.data) < 6:
                return None
            else:
                pktLen = struct.unpack('I', self.data[2:6])[0]
                if pktLen > 1500:
                    sys.stderr.write("Invalid serial packet length: %d\r\n" % pktLen)
                    self.data = self.data[2:]
                    return None
                else:
                    if len(self.data) < pktLen + 6:
                        return None
                    else:
                        ret = self.data[6:(pktLen + 6)]
                        self.data = self.data[(pktLen + 6):]
                        return ret

class UDPClient(socket.socket):
    "A wrapper around socket with the right parameters and some extra functionality"
    def __init__(self, host, port):
        socket.socket.__init__(self, socket.AF_INET, socket.SOCK_DGRAM)
        self.addr = host, port
        self.setblocking(0)

    def connect(self):
        "Send a wakeup message to the radio to establish a UDP \"connection\""
        self.sendto(b"0", self.addr)

    def receive(self):
        "Receive a packet or None"
        try:
            return self.recv(1500)
        except:
            return None

class TesterBase:
    "Base class for testing the serial radio"
    def __init__(self, serial_device="com11", udp_host="172.31.1.1", udp_port=5551, payload_length=1450):
        "Sets up the test"
        self.uart   = UARTRadioConn(serial_device)
        self.client = UDPClient(udp_host, udp_port)
        self.pktGen = PacketGenerator(payload_length)
        self.pktLog = {}

    def __del__(self):
        "Close up shop"
        self.client.close()
        self.uart.close()
        del self.uart

    def connect(self):
        "Start UDP \"connection\""
        self.client.connect()

class u2rt(TesterBase):
    "A class for testing UART to radio communication"

    def sendOne(self):
        "Send one packet over serial"
        s, p = self.pktGen.next()
        self.uart.write(struct.pack("<BBI%ds" % len(p), 0xbe, 0xef, len(p), p))
        self.pktLog[s] = time.time()

    def console(self, max_length=10000):
        "Read from UART and write to console"
        sys.stdout.write(self.uart.read(max_length))

    def run(self, interval=1.0, count=65535):
        "Run a test with a specified interval betandrween packets"
        self.connect()
        while count > 0:
            count -= 1
            st = time.time()
            self.sendOne()
            while (time.time()-st) < interval:
                self.console(120)
                pkt = self.client.receive()
                rt = time.time()
                if pkt:
                    seqno = unpackSeqno(pkt)
                    if seqno in self.pktLog:
                        sys.stdout.write("RTT: %f ms\r\n" % ((rt-self.pktLog[seqno])*1000.0))
                        del self.pktLog[seqno]
                    else:
                        sys.stdout.write("Unexpected packet: \"%s\"\r\n" % pkt)
                time.sleep(0.0003)
        sys.stdout.write("%d packets not returned\r\n" % len(self.pktLog))
        self.pktLog = {}

class r2ut(TesterBase):
    "A class for testing radio to UART communication"

    def sendOne(self):
        "Send one packet over radio"
        s, p = self.pktGen.next()
        self.client.sendto(p, self.client.addr)
        self.pktLog[s] = time.time()

    def run(self, interval=1.0, count=65535):
        "Run a test with a specified interval between packets"
        while count > 0:
            count -= 1
            st = time.time()
            self.sendOne()
            while (time.time() - st) < interval:
                pkt = self.uart.receive()
                rt = time.time()
                if pkt:
                    try:
                        seqno = unpackSeqno(pkt)
                    except:
                        sys.stderr.write("Couldn't unpack seqno from \"%s\"\r\n" % pkt)
                        seqno = -1
                    if seqno in self.pktLog:
                        sys.stdout.write("RTT: %fms\r\n" % ((rt-self.pktLog[seqno])*1000.0))
                        del self.pktLog[seqno]
                    else:
                        sys.stdout.write("Unexpected packet: %s...\r\n" % ''.join(["%02x" % ord(b) for b in pkt[:10]]))
                time.sleep(0.0003)
        sys,stdout.write("%d packets not returned\r\n" % len(self.pktLog))
        self.pktLog = {}

class bit(TesterBase):
    "A class for testing both directions"

    def sendR(self):
        "Send one packet over radio"
        s, p = self.pktGen.next()
        self.client.sendto(p, self.client.addr)
        self.pktLog[s] = time.time()

    def sendU(self):
        "Send one packet over serial"
        s, p = self.pktGen.next()
        self.uart.write(struct.pack("<BBI%ds" % len(p), 0xbe, 0xef, len(p), p))
        self.pktLog[s] = time.time()

    def run(self, interval=1.0, count=65535):
        "Run a test with a specified interval between packets"
        while count > 0:
            count -= 1
            st = time.time()
            self.sendR()
            self.sendU()
            while (time.time() - st) < interval:
                # UART receive
                pkt = self.uart.receive()
                rt = time.time()
                if pkt:
                    try:
                        seqno = unpackSeqno(pkt)
                    except:
                        sys.stderr.write("Couldn't unpack seqno from \"%s\"\r\n" % pkt)
                        seqno = -1
                    if seqno in self.pktLog:
                        sys.stdout.write("B->R: %fms\r\n" % ((rt-self.pktLog[seqno])*1000.0))
                        del self.pktLog[seqno]
                    else:
                        sys.stdout.write("Unexpected packet: %s...\r\n" % ''.join(["%02x" % ord(b) for b in pkt[:10]]))
                # Radio receive
                pkt = self.client.receive()
                rt = time.time()
                if pkt:
                    seqno = unpackSeqno(pkt)
                    if seqno in self.pktLog:
                        sys.stdout.write("B<-R: %f ms\r\n" % ((rt-self.pktLog[seqno])*1000.0))
                        del self.pktLog[seqno]
                    else:
                        sys.stdout.write("Unexpected packet: \"%s\"\r\n" % pkt)
                time.sleep(0.0003)
