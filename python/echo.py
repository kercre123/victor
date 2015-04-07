#!/usr/bin/env python
import socket

s = socket.socket(type=socket.SOCK_DGRAM)
s.bind(('', 6661))

while True:
    d,a=s.recvfrom(1500)
    s.sendto(d,a)
