#!/usr/bin/env python3
"""
A simple script to make a bootloader config bin
"""
import sys, struct

class BootloaderConfig(struct.Struct):
    "Python struct replica of C data structure"
    
    HEADER = 0xe1df0c05
    CHECKSUM_INIT = 0xef
    
    def __init__(self, start=0, size=0, version=0):
        struct.Struct.__init__(self, "<IIIIB")
        self.newImageStart = 0
        self.newImageSize  = 0
        self.version = 0
    
    @property
    def checksum(self):
        "Generate the checksum of the structure"
        bytes = self.pack(self.HEADER, self.newImageStart, self.newImageSize, self.version, 0)
        rslt = self.CHECKSUM_INIT
        for b in bytes: rslt ^= b
        return rslt

    def serialize(self):
        "Return the serialized version of the struct"
        return self.pack(self.HEADER, self.newImageStart, self.newImageSize, self.version, self.checksum)

if __name__ == "__main__":
    try:
        start = int(eval(sys.argv[1]))
    except:
        start = 0
    try:
        size = int(eval(sys.argv[2]))
    except:
        size = 0
    try:
        version = int(eval(sys.argv[3]))
    except:
        version = 0
    open("firmware/bootloaderConfig.bin", "wb").write(BootloaderConfig(start, size, version).serialize())
