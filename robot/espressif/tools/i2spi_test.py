#!/usr/bin/env python3
"""
Throw away script for testing I2S(PI) functionality on the Espressif.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, socket, struct

ROBOT = ("172.31.1.1", 5551)

s = socket.socket(type=socket.SOCK_DGRAM)

s.sendto(bytes(range(256)), ROBOT)
print(s.recvfrom(1500))
