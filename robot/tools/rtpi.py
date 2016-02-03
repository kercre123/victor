#!/usr/bin/env python3
"""Relialbe Transport Packet Inspector"""
__author__ = "Daniel Casner"

import sys, os, struct, time, mmap
if sys.version_info.major < 3:
    sys.exit("Python below version 3 not supported")
sys.path.insert(0, "tools")

import ReliableTransport as rt

def openDump(fileName):
    fh = open(fileName, "rb")
    return mmap.mmap(fh.fileno(), 0, access=mmap.ACCESS_READ)
    
def findPackets(dump):
    found = []
    ind = dump.find(rt.AnkiReliablePacketHeader.PREFIX, 0)
    while ind >= 0:
        found.append(ind)
        ind = dump.find(rt.AnkiReliablePacketHeader.PREFIX, ind+1)
    return found

def getHeaders(dump, found):
    return [(i, rt.AnkiReliablePacketHeader(buffer=dump, offset=i)) for i in found]

def getPacket(dump, found, index):
    if index == len(found)-1 or index == -1:
        pkt = dump[found[index]:]
    else:
        pkt = dump[found[index]:found[index+1]]
    return pkt

def parsePacket(pkt):
    offset = 0
    header = rt.AnkiReliablePacketHeader(buffer=pkt, offset=offset)
    ret = [header]
    print(header)
    offset += header.size
    while offset < len(pkt):
        type = pkt[offset]
        size = pkt[offset+1] | (pkt[offset+2] << 8)
        msg = pkt[offset+3:offset+3+size]
        print("({:d})\t{:d}[{:d}]:\t{:s}".format(offset, type, size, repr(msg)))
        ret.append((type, size, msg))
        offset += 3 + size
    return ret

if len(sys.argv) > 1:
    dump  = openDump(sys.argv[1])
    found = findPackets(dump)
    pkts  = getHeaders(dump, found)
