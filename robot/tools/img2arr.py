#!/usr/bin/env python3

import sys, os
from PIL import Image
from math import *

def img2arr(img):
    assert img.size[0] <= 128 and img.size[1] <= 64
    yr = 8
    while yr < img.size[1]:
        yr *= 2
    fmt = "0x{:0%dx},{}" % (yr//4)
    out = "{" 
    for x in range(img.size[0]):
        num = 0
        for y in range(img.size[1]):
            num |= (0 if img.getpixel((x,y)) else 1) << y
        out += fmt.format(num, os.linesep if yr >= 32 else " ")
    out += "},"
    return out

if __name__ == '__main__':
    for fn in sys.argv[1:]:
        print(img2arr(Image.open(fn)))
