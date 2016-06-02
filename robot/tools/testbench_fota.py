#!/usr/bin/env python3
"""
Test bench to stimulate robot nvstorage module
"""
import sys, os, time, struct, pickle, argparse, random
from zlib import crc32

sys.path.insert(0, os.path.join("tools"))

from fota import *

class InfiniteUpgradeFile:
    
    def __init__(self):
        self.data = b""
        
    def read(self, bytes):
        if bytes > len(self.data):
            block = b"\xff" * 2048
            crc = crc32(block)
            self.data += block + struct.Struct("II").pack(0xFFFFfffc, crc)
        ret = self.data[:bytes]
        self.data = self.data[bytes:]
        return ret
        
if __name__ == '__main__':
    robotInterface.Init(True)
    up = OTAStreamer(InfiniteUpgradeFile())
    robotInterface.Connect()
    up.main()
    del up
