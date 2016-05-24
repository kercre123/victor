#!/usr/bin/env python3
from __future__ import print_function

from zlib import crc32
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH
import json
import time
import subprocess

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("output", type=str,
                    help="target location for file")
parser.add_argument("-r", "--rtip", type=str,
                    help="K02 ELF Image")
parser.add_argument("-b", "--body", type=str,
                    help="NRF ELF Image")
parser.add_argument("-w", "--wifi", type=str,
                    help="ESP Raw binary image")
parser.add_argument("-s", "--sign", action="store_true",
                    help="Include signature block")
parser.add_argument("-c", "--comment", action="store_true",
                    help="Add version information comment block")
args = parser.parse_args()

BLOCK_LENGTH = 0x800


#  TODO: ADD CERT BUILDING

def chunk(i, size):
    for x in range(0, len(i), size):
        yield i[x:x+size]

def pad_image(rom_data, length=BLOCK_LENGTH):
    while len(rom_data) % length:
        rom_data += b"\xFF"
    return rom_data

def get_image(fn):
    rom_data, base_addr, _ = rom_info(fn)
    
    # Clear our our evil byte
    rom_data = rom_data[:24] + b'\xFF\xFF\xFF\xFF' + rom_data[28:]
    
    return pad_image(rom_data), base_addr

def get_version_comment_block():
    p = subprocess.Popen(['git', 'rev-parse', 'HEAD'], stdout=subprocess.PIPE)
    assert p.wait() == 0
    comment = {}
    comment['version'] = p.communicate()[0].decode().strip()
    comment['date'] = time.ctime()
    comment['time'] = time.time()
    p = subprocess.Popen(['git', 'status'], stdout=subprocess.PIPE)
    assert p.wait() == 0
    comment['dirt'] = p.communicate()[0].decode()[:1000]
    encodded = json.dumps(comment).encode("UTF-8")
    paddingLength = BLOCK_LENGTH - len(encodded)
    assert paddingLength > 0, "Must have null termination"
    return encodded + (b"\x00" * paddingLength)
    

# This generates a single file for RTIP that is actually both
with open(args.output, "wb") as fo:
    # Commend must be first for app to parse it
    for kind, fn in [('comment', args.comment), ('sign', args.sign), ('body', args.body), ('rtip', args.rtip), ('wifi', args.wifi)]: # It is nessisary to be in this order or it hangs for reasons not presently known :'(
        if not fn:
            continue

        if kind == 'wifi':
            base_addr = 0x40000000
            rom_data = pad_image(open(fn, "rb").read())
        elif kind == 'comment':
            base_addr = 0xFFFFfffc
            rom_data = get_version_comment_block()
        else:    
            rom_data, base_addr = get_image(fn)
            if kind == 'body':
                base_addr |= 0x80000000

        for block, data in enumerate(chunk(rom_data, BLOCK_LENGTH)):
            fo.write(data)
            fo.write(pack("<II", block*BLOCK_LENGTH+base_addr, crc32(data)))
