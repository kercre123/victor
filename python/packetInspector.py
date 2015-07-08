#!/usr/bin/env python3
"""
A little script with some useful functions for inspecting dumps of packets between the robot and base station
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, struct

ReliablePacketHeader = struct.Struct("3sBHHH")


def loadPackets(file_path_name):
    "Loads a binary dump of packets and returns the individual packets stripped of unreliable transport headers."
    return open(file_path_name, "rb").read().split(b"COZ\x03")

def parseReliableHeader(packet):
    "Parses the reliable transport header from a packet."
    try:
        return ReliablePacketHeader.unpack(packet[:ReliablePacketHeader.size])
    except:
        return packet[:10] + b"..."
    
