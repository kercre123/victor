from __future__ import print_function
from Crypto.Hash.SHA import SHA1Hash
from Crypto.Cipher.AES import AESCipher
from Crypto.Random import random
from Crypto.Util import number
from Crypto.Cipher import PKCS1_OAEP
from Crypto.PublicKey import RSA

import os.path as path
from sys import argv
from struct import pack

from elftools.elf.elffile import ELFFile

HEADER_LENGTH = 128
BLOCK_LENGTH = 0x1000

def chunk(i, size):
	for x in range(0, len(i), size):
		yield i[x:x+size]

print("Building rom")
with open(argv[1], "rb") as fo:
	elffile = ELFFile(fo)

	segments = {}

	# Build rom segments
	for segment in elffile.iter_segments():
		fo.seek(segment['p_offset'])
		segments[segment['p_vaddr']] = fo.read(segment['p_filesz'])

	# Locate vector table + magic header
	for section in elffile.iter_sections():
		if section.name == b"HEADER_ROM":
			magic_location = section.header.sh_offset + 12

	# flatten rom image
	first = min(segments.keys())
	last = max([addr+len(data) for addr, data in segments.items()])

	base_addr = first
	rom_data = b"\xFF" * (last-first)

	for addr, data in segments.items():
		addr = addr - first
		rom_data = rom_data[:addr] + data + rom_data[addr+len(data):]

# Save modified ELF
print("Fixing header rom")
with open(argv[1], "rb+") as fo:
	checksum = SHA1Hash(rom_data[HEADER_LENGTH:]).digest()

	fo.seek(magic_location)
	fo.write(pack("<II20s", base_addr+HEADER_LENGTH, len(rom_data) - HEADER_LENGTH, checksum))

if len(argv) >= 4:
	print("Outputting raw image")
	with open(argv[3], "wb") as fo:
		fo.write(rom_data)

if len(argv) >= 3:
	print("Creating signed image")
	with open(argv[2], "wb") as fo:
		# Zero pad to a 4k block (flash area)
		while len(rom_data) % BLOCK_LENGTH:
			rom_data += b"\x00"

		for block, data in enumerate(chunk(rom_data, BLOCK_LENGTH)):
			fo.write(data)
			fo.write(pack("<I", block*BLOCK_LENGTH+base_addr))
			fo.write(SHA1Hash(data).digest())

		# == TODO: UNCOMMENT THIS WHEN NEEDED
		#aes_key = number.long_to_bytes(random.getrandbits(256))
		#cypher = AESCipher(aes_key)
		#encoded = cypher.encrypt(rom_data)

		#checksum = SHA1Hash(encoded).digest()

		#cert = checksum+aes_key

		#key_path = path.join(path.dirname(argv[0]), 'mykey.pem')

		#key = RSA.importKey(open(key_path).read())
		#cipher = PKCS1_OAEP.new(key)
		#ciphertext = cipher.encrypt(cert)

		#fo.write(pack("<4sII", "CzmH", len(cert), len(encoded)))
		#fo.write(cert)	
		#fo.write(encoded)
