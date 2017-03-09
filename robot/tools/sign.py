#!/usr/bin/env python3
from __future__ import print_function

from zlib import crc32
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH
import argparse
import random
import math
import json
import time
import sys
import os
import subprocess
import shutil
import re
import hashlib

from Crypto.Hash import SHA512
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES
from Crypto import Random
from hashlib import sha1

global messageEngineToRobotHash
global messageRobotToEngineHash

parser = argparse.ArgumentParser()
parser.add_argument("output", type=str,
                    help="target location for file")

parser.add_argument("-m", "--model", type=int, default=5,
                    help="Compatible model number")
parser.add_argument("-r", "--rtip", type=str,
                    help="K02 ELF Image")
parser.add_argument("-rb", "--rtipbin", type=str,
                    help="K02 Raw binary image")
parser.add_argument("-b", "--body", type=str,
                    help="NRF ELF Image")
parser.add_argument("-bb", "--bodybin", type=str,
                    help="NRF Raw binary image")
parser.add_argument("-w", "--wifi", type=str,
                    help="ESP Raw binary image")
parser.add_argument("-s", "--sign", nargs=2, type=str,
                    help="Create signature block")
parser.add_argument("-c", "--comment", type=int, nargs="?", const=int(time.time()),
                    help="Add version information comment block, argument is the desired version string")
parser.add_argument("-t", "--build_type_header", type=argparse.FileType('rt'),
                    help="Headerfile to read")
parser.add_argument("--prepend_size_word", action="store_true",
                    help="Put a single u32 size word at the front of the file for recovery images.")
parser.add_argument("--factory_upgrade", nargs=2, type=str,
                    help="Generate a factory firmware upgrade image")

FILE_TYPE_VERSION   = 0x00000002

AES_BLOCK_LENGTH    = 16
BLOCK_LENGTH        = 0x800
BODY_BLOCK          = 0x80000000
WIFI_BLOCK          = 0x40000000
FACTORY_UPGRD_BLOCK = 0xFFfacfac
COMMENT_BLOCK       = 0xFFFFFFFC    # this is the JSON used for the app
HEADER_INFORMATION  = 0xFFFFFFFE    # this is used for the robot
CERTIFICATE_BLOCK   = 0xFFFFFFFF
# Space available in flash map minus one block for version info.
# Factory firmware is locked down to 0x7c000 so we can't use anything larger
WIFI_MAX_FW_SIZE    = 0x07c000 - BLOCK_LENGTH

class DigestFile:
    def __init__(self, fn, mode, digestType = SHA512):
        self.digestType = digestType
        self.fo = open(fn, mode)
        self.hash = self.digestType.new()
        self.iv = Random.get_random_bytes(AES_BLOCK_LENGTH)

    def cfb(self, data, key):
        cipher = AES.AESCipher(key, AES.MODE_ECB)
        result = b''

        for _, block in chunk(data, AES_BLOCK_LENGTH):
            self.iv = bytes([block[i] ^ b for i, b in enumerate(cipher.encrypt(self.iv))])
            result += self.iv

        return result

    def write(self, data, block, aes_key = None):
        assert len(data) <= BLOCK_LENGTH

        data = data + b"\xFF" * (BLOCK_LENGTH - len(data))

        if aes_key:
            data = self.cfb(data, aes_key)

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
    for x in range(0, len(i), size):
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

def make_header(key=None, iv=None, digestType=None, model=0):
    timestamp = int(time.time())
    ctime = bytearray(time.ctime(), 'utf-8')

    header = pack("<4sI%isI32s20sB" % AES_BLOCK_LENGTH,
        b'CZM0',
        FILE_TYPE_VERSION,
        iv if iv else (b"\x00" * 16),
        timestamp,
        ctime,
        git_sha(),
        model)

    # Include OID for the hash algorithm used
    oid = digestType.new().oid if digestType else b''
    header += pack("<H", len(oid)) + oid

    # include the modulus used for encoding
    data = key.n.to_bytes(math.ceil(key.size()/8), byteorder='little') if key else b''
    header += pack("<H", len(data)) + data

    return header

def get_version_comment_block(args):
    MAX_COMMENT_LEN = 1024
    comment = {
        'version': args.comment,
        'git-rev': execute('git', 'rev-parse', 'HEAD').strip(),
        'date': time.ctime(),
        'time': int(time.time()),
        'messageEngineToRobotHash': hex(messageEngineToRobotHash)[2:],
        'messageRobotToEngineHash': hex(messageRobotToEngineHash)[2:],
    }
    if args.build_type_header:
        comment['build'] = re.match('#define\s+(\S+)', args.build_type_header.read()).groups()[0]
        sys.stdout.write("{0}{1}# OTA File build type is *{2}*{1}{0}{1}".format("#"*60, os.linesep, comment['build']))
    for fw in ('wifi', 'rtip', 'body'):
        fn = getattr(args, fw)
        if fn is not None:
            comment[fw + 'Sig'] = sha1(open(fn, 'rb').read()).hexdigest()
    encodded = json.dumps(comment).encode("UTF-8") + b"\x00"
    assert len(encodded) < MAX_COMMENT_LEN, "Comment block contents must be less than {:d}".format(MAX_COMMENT_LEN)
    return encodded + (b"\x00" * (MAX_COMMENT_LEN - len(encodded)))

# This generates a single file for RTIP that is actually both
if __name__ == '__main__':
    args = parser.parse_args()

    if not args.prepend_size_word: #cladPython doesn't exist in prepend use environment
        sys.path.insert(0, os.path.join("generated", "cladPython", "robot"))
        from clad.robotInterface.messageEngineToRobot_hash import messageEngineToRobotHash
        from clad.robotInterface.messageRobotToEngine_hash import messageRobotToEngineHash

    # start building our firmware image
    fo = DigestFile(args.output, "wb", SHA512)
    
    commentBlock = None
    
    if args.comment is not None:
        commentBlock = get_version_comment_block(args)
        fo.write(commentBlock, COMMENT_BLOCK)

    # Load our RSA key
    if not args.sign == None:
        aes_key, cert_file = args.sign

        with open (cert_file, "r") as cert_fo:
            cert = RSA.importKey(cert_fo.read())
            print ("Signing data with a %i-bit certificate" % cert.size())

        aes_key = int(aes_key,16).to_bytes(AES_BLOCK_LENGTH, byteorder='little')
    
        print ("Writting header information")
        fo.write(make_header(cert, fo.iv, SHA512, args.model), HEADER_INFORMATION)
        fo.writeCert(cert)
    else:
        cert, aes_key = None, None

    print ("Writting rom block")
    # this is our rom information
    # It is nessessary to be in this order or it hangs for reasons not presently known :'(
    for kind, fn in [('body', args.body), ('bodybin', args.bodybin), ('rtip', args.rtip), ('rtipbin', args.rtipbin), ('wifi', args.wifi)]:
        if fn == None:
            continue

        if kind == 'wifi':
            base_addr = WIFI_BLOCK
            rom_data = open(fn, "rb").read()
            assert len(rom_data) <= WIFI_MAX_FW_SIZE
        elif kind == 'bodybin':
            base_addr = 0x18000 #found experimentally from output of syscon.axf
            base_addr |= BODY_BLOCK
            rom_data = open(fn, "rb").read()
            rom_data = rom_data[:24] + b'\xFF\xFF\xFF\xFF' + rom_data[28:] #WTF? copied from get_image() method. "Clear our our evil byte"?
            assert len(rom_data) <= 0x6C00 #BODY_MAX_FW_SIZE, from syscon project config
        elif kind == 'rtipbin':
            base_addr = 0x1000 #found experimentally from output of robot.axf
            rom_data = open(fn, "rb").read()
            rom_data = rom_data[:24] + b'\xFF\xFF\xFF\xFF' + rom_data[28:] #WTF? copied from get_image() method. "Clear our our evil byte"?
            assert len(rom_data) <= 0xF000 #RTIP_MAX_FW_SIZE, from robot41 project config
        else:
            rom_data, base_addr = get_image(fn)
            if kind == 'body':
                base_addr |= BODY_BLOCK

        #print('%s'%kind, '%s'%fn, 'len=%i'%len(rom_data), 'addr=%s'%hex(base_addr), 'md5:%s'% hashlib.md5(rom_data).hexdigest())
        
        for block, data in chunk(rom_data, BLOCK_LENGTH):
            fo.write(data, block+base_addr, aes_key)
        if kind == 'wifi' and commentBlock is not None:
            fo.write(commentBlock, WIFI_BLOCK + WIFI_MAX_FW_SIZE, aes_key) # Add version info to end of wifi
    
    if args.factory_upgrade is not None:
        fo.write(b"", FACTORY_UPGRD_BLOCK, aes_key)
        wifi,rtip = args.factory_upgrade
        rom_data = open(wifi, 'rb').read()
        for block, data in chunk(rom_data, BLOCK_LENGTH):
            fo.write(data, block + WIFI_BLOCK)
        rom_data = open(rtip, 'rb').read()
        for block, data in chunk(rom_data, BLOCK_LENGTH):
            fo.write(data, block + WIFI_BLOCK + 0x45000)

    fo.writeCert(cert)
    fo.close()

    if args.prepend_size_word:
        fw = open(args.output, 'rb').read()
        with open(args.output, "wb") as fo:
            fo.write(pack("I", len(fw)))
            fo.write(fw)
            
    if args.wifi:
        Spath = os.path.split(args.wifi)[0]
        S = os.path.join(Spath, 'esp.S')
        if os.path.isfile(S):
            shutil.copyfile(S, os.path.join(Spath, "esp-{}.S".format(args.comment)))
        else:
            print("No S file to preserve")
