#!/usr/bin/env python3
"Python mu-law compression algorithm"
__author__ = "Daniel Casner <daniel@anki.com>"

mutable = [
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
]

def encodeSample(sample, mask=0xFFFF):
    sign = sample < 0
    
    if sign:
        sample = (~sample) & mask
    else:
        sample = sample & mask
        
    exponent = mutable[sample >> 8]
    if exponent:
        mantessa = (sample >> (exponent + 3)) & 0xf
    else:
        mantessa = sample >> 4
    
    ret = (0x80 if sign else 0) | (exponent << 4) | mantessa
    assert (ret & 0xff) == ret
    return ret
    
def decodeSample(byte):
    exp  = (byte >> 4) & 0x7
    bits = byte & 0xf
    if exp:
        bits = (0x10 | bits) << (exp - 1)
    return (-bits if (byte & 0x80) else bits)
    
def unitTest():
    original = range(-2**11, 2**11, (2**12//100))
    encoded  = [encodeSample(s) for s in original]
    assert max(encoded) < 256
    assert min(encoded) >= 0
    decoded  = [decodeSample(s)<<4 for s in encoded]
    diff = [d - o for d, o in zip(decoded, original)]
    print([hex(d) for d in diff])
