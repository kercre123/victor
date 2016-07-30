#!/usr/bin/env python3
import sys, struct, random
fh = open("firmware/sn.bin", "wb")
modelNumber = 0xFFFF
fh.write(struct.pack("I", int(eval(sys.argv[1] if len(sys.argv) > 1 else input("Serial number?> ")))))
fh.write(struct.pack("HH", modelNumber, 0xFFFF))
fh.write(struct.pack("8I", *[random.randrange(0, 2**32) for i in range(8)]))
fh.close()
