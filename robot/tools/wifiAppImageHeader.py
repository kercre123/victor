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
        assert (len(image) % BLOCK_SIZE) == 0
        self.sha1 = hashlib.sha1(image).digest()

    def serialize(self):
        return self.pack(self.size + self.imageSize, -1, -1, *self.sha1)

class EspressifNewStyleHeader(struct.Struct):
    """Python reflection of Espressif's new style ROM bootloader header."""
    
    def __init__(self, image):
        struct.Struct.__init__(self, "BBBBIII")
        self.fields = list(self.unpack(image[:self.size]))
        self.image  = image[self.size:]
    
    @property
    def header(self):
        return self.pack(*self.fields)

def fix_image(inFile, outFile):
    eh = EspressifNewStyleHeader(open(inFile, 'rb').read())
    eh.image += b"\xff" * (BLOCK_SIZE - (len(eh.image) % BLOCK_SIZE))
    aih = AppImageHeader(eh.image)
    eh.fields[-1] += aih.size # Length is last field, add size of our additional header
    of = open(outFile, 'wb')
    of.write(eh.header)
    of.write(aih.serialize())
    of.write(eh.image)
    
    
    
if __name__ == '__main__':
    fix_image(sys.argv[1], sys.argv[2])
