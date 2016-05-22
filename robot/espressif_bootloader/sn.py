#!/usr/bin/env python3
import sys, struct
open("firmware/sn.bin", "wb").write(struct.pack("I", int(eval(sys.argv[1] if len(sys.argv) > 1 else input("Serial number?> ")))))
