#!/usr/bin/env python3
"""
Throw away script for testing I2S(PI) functionality on the Espressif.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, socket, struct

ROBOT = ("172.31.1.1", 5551)

s = socket.socket(type=socket.SOCK_DGRAM)

lw = 1
rw = 2

msg = struct.Struct("II")

s.sendto(msg.pack(lw,rw), ROBOT)
while True:
    r, d = s.recvfrom(1500)
    print(len(r), max(r), r)
    lw += 1
    rw += 1
    s.sendto(msg.pack(lw, rw), d)
