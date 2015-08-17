#!/usr/bin/env python3
"""
Throw away script for testing I2S(PI) functionality on the Espressif.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, socket, struct

ROBOT = ("172.31.1.1", 5551)

s = socket.socket(type=socket.SOCK_DGRAM)

msg = struct.Struct("201H")

fh = open("i2spi.hex", 'w')

s.sendto(msg.pack(*range(201)), ROBOT)
print(">")
while True:
    d = s.recv(1500)
    print("<{:d}".format(len(d)))
    fh.write(" ".join(["{:04x}".format(w) for w in msg.unpack(d)]))
    fh.write(os.linesep)
    s.sendto(msg.pack(lw, rw), d)
