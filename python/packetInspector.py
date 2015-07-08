#!/usr/bin/env python3
"""
A little script with some useful functions for inspecting dumps of packets between the robot and base station
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, struct

UnreliablePacketPrefix = b"COZ\x03"
ReliablePacketPrefix = b"RE\x01"
ReliablePacketHeader = struct.Struct("3sBHHH")


def loadPackets(file_path_name, has_unreliable_header=True):
    "Loads a binary dump of packets and returns the individual packets stripped of unreliable transport headers."
    dump = open(file_path_name, "rb").read()
    if has_unreliable_header:
        return dump.split(UnreliablePacketPrefix)
    else:
        return [ReliablePacketPrefix + p for p in dump.split(ReliablePacketPrefix)]

def parseReliableHeader(packet):
    "Parses the reliable transport header from a packet."
    try:
        return ReliablePacketHeader.unpack(packet[:ReliablePacketHeader.size])
    except:
        return packet[:10] + b"..."
    
