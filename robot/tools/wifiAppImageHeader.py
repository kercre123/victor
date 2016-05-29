#!/usr/bin/env python3
"""Prepends image header to WiFi firmware image."""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, struct, hashlib

BLOCK_SIZE = 64

class AppImageHeader(struct.Struct):
    """Python reflection of AppImageHeader as defined in flash_map.h
    Keeps fields required to be blank (0xFFFFffff) for OTA blank."""
    
    def __init__(self, image):
        "Calculates fields from an image file"
        struct.Struct.__init__(self, "iii20B")
        self.imageSize = len(image)
        self.sha1 = hashlib.sha1(image).digest()

    def serialize(self):
        return self.pack(self.size + self.imageSize, -1, -1, *self.sha1)

def fix_image(inFile, outFile):
    fw = open(inFile, 'rb').read()
    fw += b"\xff" * (BLOCK_SIZE - (len(fw) % BLOCK_SIZE))
    assert (len(fw) % BLOCK_SIZE) == 0
    open(outFile, 'wb').write(AppImageHeader(fw).serialize() + fw)
    
if __name__ == '__main__':
    fix_image(sys.argv[1], sys.argv[2])
