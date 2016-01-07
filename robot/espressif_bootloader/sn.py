#!/usr/bin/env python3
import struct
open("firmware/sn.bin", "wb").write(struct.pack("I", int(eval(input("Serial number?> ")))))
