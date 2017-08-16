#!/usr/bin/env python3
from __future__ import print_function

from elftools.elf.elffile import ELFFile
from Crypto.Hash import SHA512
from Crypto.PublicKey import RSA
from Crypto import Random

import argparse
import random
import time
import math
import os


VICTOR_EPOCH = 1498237200 #2017-06-23 10:00a PST
BUILD_TIME = int(time.time()-VICTOR_EPOCH)
BUILD_TIMESTAMP = "{:07x}".format(BUILD_TIME)

parser = argparse.ArgumentParser()
parser.add_argument("-k", "--key", type=str,
                    help="Key used to sign image")
parser.add_argument("-b", "--binary", type=str,
                    help="Output file to a binary")
parser.add_argument("image", type=str,
                    help="AXF/ELF of image")
parser.add_argument("-v", "--version", type=str, default="DevBuild"+BUILD_TIMESTAMP,
                    help="Version of build")


FLASH_ADDRESS   = 0x08000000

HEADER_LENGTH   = 16
CERT_LENGTH     = 256
FLASH_SIZE      = 0x10000
IMAGE_BASE      = 0x4000

PROGRAM_SIZE    = FLASH_SIZE - IMAGE_BASE - CERT_LENGTH - HEADER_LENGTH

def rom_info(file):
    with open(file, "rb") as fo:
        elffile = ELFFile(fo)

        segments = {}

        # Build rom segments
        for segment in elffile.iter_segments():
            fo.seek(segment['p_offset'])
            segments[segment['p_vaddr']] = fo.read(segment['p_filesz'])

        magic_location = None

        # Locate vector table + magic header
        for section in elffile.iter_sections():
            if type(section.name) is bytes and str != bytes: # Deal with API changes between versions of elftools
                section.name = section.name.decode()
            if section.name == "ER_IROM1":
                temp = fo.tell()
                fo.seek(section.header.sh_offset)
                if fo.read(4) != b"C2MO":
                    raise Exception("Could not locate cozmo header")
                fo.seek(temp)

                magic_location = section.header.sh_offset

        if magic_location is None:
            raise Exception("Could not locate cozmo header. Sections = {}".format(repr([s.name for s in elffile.iter_sections()])))

        # Flatten our image
        rom_data, base_addr = None, None
        for index, data in segments.items():
            if rom_data == None:
                rom_data = data
                base_addr = index
            elif index < base_addr:
                extra =  base_addr - (index  + len(data))
                rom_data = data + (b"\xFF" * extra) + rom_data
            else:
                extra = index - (base_addr + len(rom_data))
                rom_data = rom_data + (b"\xFF" * extra) + data

    return rom_data, base_addr, magic_location

def MGF1(a, b, digestType=SHA512):
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

def sign(data, key, digestType=SHA512):
    # We digest the entire flash space (has to be uninitalized)
    hash = digestType.new()
    hash.update(data)
    hash.update(b"\xFF" * (PROGRAM_SIZE - len(data)))
    digest = hash.digest()

    # Fixed padding lets us know when that hash method we used was
    fixed_padding = (b"\x00\x01\xFF\xFF" + hash.oid[::-1] + b"\xFF\xFF\x00")[::-1]

    # determine lengths of the our fields
    pad_length = key.size() % 8
    mod_length = key.size() // 8
    db_length = mod_length - digestType.digest_size
    salt_length = db_length - len(fixed_padding)

    # digest used for signing
    salt = Random.get_random_bytes(salt_length)

    # Generate our PSS hash
    m_digest = digestType.new()
    m_digest.update(salt)
    m_digest.update(digest)
    m_digest.update(fixed_padding)
    m_digest = m_digest.digest()

    # Generate our database (and encode it with RSA)
    cert  = m_digest + MGF1(salt + fixed_padding, m_digest)
    cert  = int.from_bytes(cert, byteorder='little', signed=False)
    cert, = key.sign(cert << pad_length, 0)

    # Return our long as a byte array
    return cert.to_bytes(math.ceil(key.size() / 8), byteorder='little')

# Sign the syscon image
if __name__ == '__main__':
    args = parser.parse_args()

    commentBlock = None

    # Load our RSA key
    cert_fn = args.key if args.key else os.environ['COZMO2_CERT'] if 'COZMO2_CERT' in os.environ else os.path.join(os.path.dirname(__file__), "development.pem")
    print (cert_fn)
    with open(cert_fn, "r") as cert_fo:
        cert = RSA.importKey(cert_fo.read())
        print ("Signing data with a %i-bit certificate" % cert.size())

    axf_data, axf_address, magic_location = rom_info(args.image)

    version = str.encode(args.version[:15]) + b'\x00'
    axf_data = axf_data[:0x118] + version + axf_data[0x118+len(version):]

    if axf_address < (FLASH_ADDRESS + IMAGE_BASE) or (axf_address + len(axf_data)) > (FLASH_ADDRESS + FLASH_SIZE):
        raise Exception("Image too large for flash")

    certificate = sign(axf_data[HEADER_LENGTH+CERT_LENGTH:], cert)

    with open(args.binary, "wb") as fo:
        fo.write(axf_data)
        fo.seek(0)
        fo.write(version)
        fo.seek(HEADER_LENGTH)
        fo.write(certificate)
        fo.seek(0x118)
        fo.write(version)

    with open(args.image, "rb+") as fo:
        fo.seek(magic_location + HEADER_LENGTH)
        fo.write(certificate)
        fo.seek(magic_location + 0x118)
        fo.write(version)
