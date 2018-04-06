#!/usr/bin/env python3
"""Python utility for working with images in 565 color space"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys
import os
import struct
import argparse
import numpy as np
from PIL import Image

FACE_XDIM = 184
FACE_YDIM = 96
MASK_RED = 0xF800
MASK_GRN = 0x07E0
MASK_BLU = 0x001F


def loadImage(file_path_name):
    "Load file with PIL"
    return Image.open(file_path_name)


def load565(file_path_name, xdim=FACE_XDIM, ydim=FACE_YDIM):
    "Load a binary 565 image"
    arr = np.fromfile(file_path_name, dtype=np.uint16).astype(np.uint32)
    arr = ((arr & MASK_RED) << (16-5-6+3)) | ((arr & MASK_GRN) << (8-5+2)) | ((arr & MASK_BLU) << (0-0+3)) | (0xFF << 24)
    return Image.frombuffer('RGBA', (xdim, ydim), arr, 'raw', 'RGBA', 0, 1)

if __name__ == '__main__':
    png = loadImage(sys.argv[1])
    img = load565(sys.argv[2])
    png.show()
    img.show()
