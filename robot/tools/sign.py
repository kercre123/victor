#!/usr/bin/env python3
from __future__ import print_function

from zlib import crc32
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH
from argparse import ArgumentParser
import random
import math
import json
import time
import subprocess

from Crypto.Hash import SHA512
from Crypto.PublicKey import RSA
from Crypto.Signature import PKCS1_PSS
from Crypto import Random

parser = ArgumentParser()
parser.add_argument("output", type=str,
                    help="target location for file")
parser.add_argument("-r", "--rtip", type=str,
                    help="K02 ELF Image")
parser.add_argument("-b", "--body", type=str,
                    help="NRF ELF Image")
parser.add_argument("-w", "--wifi", type=str,
                    help="ESP Raw binary image")
parser.add_argument("-s", "--sign", type=str,
                    help="Create signature block")
parser.add_argument("-c", "--comment", action="store_true",
                    help="Add version information comment block")
parser.add_argument("--prepend_size_word", action="store_true",
                    help="Put a single u32 size word at the front of the file for recovery images.")
args = parser.parse_args()

FILE_TYPE_VERSION   = 0x00000001

BLOCK_LENGTH        = 0x800
BODY_BLOCK          = 0x80000000
WIFI_BLOCK          = 0x40000000
COMMENT_BLOCK       = 0xFFFFFFFC    # this is the JSON used for the app
HEADER_INFORMATION  = 0xFFFFFFFE    # this is used for the robot
CERTIFICATE_BLOCK   = 0xFFFFFFFF

class DigestFile:
    def __init__(self, fn, mode, digestType = SHA512):
        self.digestType = digestType
        self.fo = open(fn, mode)
        self.hash = self.digestType.new()

    def write(self, data, block):
        assert len(data) <= BLOCK_LENGTH

        data = data + b"\xFF" * (BLOCK_LENGTH - len(data))
        data += pack("<II", block, crc32(data))
        
        self.hash.update(data)
        self.fo.write(data)

    def sign(self, key):
        # Fixed padding lets us know when that hash method we used was
        fixed_padding = (b"\x00\x01\xFF\xFF" + self.hash.oid[::-1] + b"\xFF\xFF\x00")[::-1]

        # determine lengths of the our fields
        pad_length = key.size() % 8
        mod_length = math.floor(key.size() / 8)
        db_length = mod_length - self.digestType.digest_size
        salt_length = db_length - len(fixed_padding)

        # digest used for signing
        digest =  self.hash.digest()

        salt = Random.get_random_bytes(salt_length)

        # Generate our PSS hash
        m_digest = self.digestType.new()
        m_digest.update(salt)
        m_digest.update(digest)
        m_digest.update(fixed_padding)
        m_digest = m_digest.digest()

        # Generate our database (and encode it with RSA)
        cert  = m_digest + MGF1(salt + fixed_padding, m_digest)
        cert  = int.from_bytes(cert, byteorder='little', signed=False)
        cert, = key.sign(cert << pad_length, 0)
 
        # Convert to a proper byte array
        bytes = cert.to_bytes(math.ceil(key.size() / 8), byteorder='little')
 
        return pack("<I", len(bytes)) + bytes

    def writeCert(self, key):
        if not key:
            return
        
        # write out our certificate block
        self.write(self.sign(key), CERTIFICATE_BLOCK)
        self.hash = self.digestType.new()
        
    def close(self):
        return self.fo.close()

def MGF1(a, b, digestType = SHA512):
    a, index, counter = bytearray(a), 0, 0
    
    while True:
        mask = digestType.new()
        mask.update(b)
        mask.update(counter.to_bytes(4, byteorder='little'))

        for byte in mask.digest():
            a[index] ^= byte
            index += 1

            if index >= len(a):
                return a

        counter += 1

def chunk(i, size):
    offsets = range(0, len(i), size)

    # This shuffles the order of flash blocks to make it harder to do sha attacks on digests
    if len(offsets) >= 3:
        extra = list(offsets[1:-1])
        random.shuffle(extra)
        offsets = [offsets[-1], offsets[0]] + extra

    for x in offsets:
        yield x, i[x:x+size]

def get_image(fn):
    rom_data, base_addr, _ = rom_info(fn)
    
    # Clear our our evil byte
    rom_data = rom_data[:24] + b'\xFF\xFF\xFF\xFF' + rom_data[28:]

    return rom_data, base_addr

def execute(*kargs):
    p = subprocess.Popen(kargs, stdout=subprocess.PIPE)
    assert p.wait() == 0
    return p.communicate()[0].decode()

def git_sha():
    out = execute('git', 'rev-parse', 'HEAD')
    return int(out, 16).to_bytes(20, byteorder='little')

def make_header(key=None, digestType=None):
    timestamp = int(time.time())
    ctime = bytearray(time.ctime(), 'utf-8')

    header = pack("<4sII32s20s",
        b'CZM0',
        FILE_TYPE_VERSION, 
        timestamp,
        ctime,
        git_sha())

    # Include OID for the hash algorithm used
    oid = digestType.new().oid if digestType else b''
    header += pack("<H", len(oid)) + oid

    # include the modulus used for encoding
    data = key.n.to_bytes(math.ceil(key.size()/8), byteorder='little') if key else b''
    header += pack("<H", len(data)) + data

    return header

def get_version_comment_block():
    version = execute('git', 'rev-parse', 'HEAD')
    dirt = execute('git', 'status')

    comment = {
        'version': version.strip(),
        'date': time.ctime(),
        'time': time.time(),
        'dirt': dirt[:1000]
    }

    encodded = json.dumps(comment).encode("UTF-8")
    return encodded + b"\x00"


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
if __name__ == '__main__':
    args = parser.parse_args()

    # Load our RSA key
    if not args.sign == None:
        with open (args.sign, "r") as fo:
            key = RSA.importKey(fo.read())
            print ("Signing data with a %i-bit certificate" % key.size())
    else:
        key = None

    # start building our firmware image
    fo = DigestFile(args.output, "wb", SHA512)
    
    if args.comment:
        fo.write(get_version_comment_block(), COMMENT_BLOCK)

    if key:
        print ("Writting header information")
        fo.write(make_header(key, SHA512), HEADER_INFORMATION)
        fo.writeCert(key)

    print ("Writting rom block")
    # this is our rom information
    # It is nessessary to be in this order or it hangs for reasons not presently known :'(
    for kind, fn in [('body', args.body), ('rtip', args.rtip), ('wifi', args.wifi)]:
        if fn == None:
            continue

        if kind == 'wifi':
            base_addr = WIFI_BLOCK
            rom_data = open(fn, "rb").read()
        else:    
            rom_data, base_addr = get_image(fn)
            if kind == 'body':
                base_addr |= BODY_BLOCK

        for block, data in chunk(rom_data, BLOCK_LENGTH):
            fo.write(data, block+base_addr)

    fo.writeCert(key)
    
    fo.close()

    if args.prepend_size_word:
        fw = open(args.output, 'rb').read()
        with open(args.output, "wb") as fo:
            fo.write(pack("I", len(fw)))
            fo.write(fw)
