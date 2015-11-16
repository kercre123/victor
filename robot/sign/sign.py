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

print "Building rom"
with file(argv[1], "rb") as fo:
	elffile = ELFFile(fo)

	segments = {}

	# Build rom segments
	for segment in elffile.iter_segments():
		fo.seek(segment['p_offset'])
		segments[segment['p_vaddr']] = fo.read(segment['p_filesz'])

	# Locate vector table + magic header
	for section in elffile.iter_sections():
		if section.name == "HEADER_ROM":
			magic_location = section.header.sh_offset + 12

	# flatten rom image
	first = min(segments.keys())
	last = max([addr+len(data) for addr, data in segments.iteritems()])

	base_addr = first
	rom_data = "\xFF" * (last-first)

	for addr, data in segments.iteritems():
		addr = addr - first
		rom_data = rom_data[:addr] + data + rom_data[addr+len(data):]

# Save modified ELF
print "Fixing header rom"
with file(argv[1], "rb+") as fo:
	checksum = SHA1Hash(rom_data[HEADER_LENGTH:]).digest()

	fo.seek(magic_location)
	fo.write(pack("<II20s", base_addr+HEADER_LENGTH, len(rom_data) - HEADER_LENGTH, checksum))

print "Creating signed image"
with file(argv[2], "wb") as fo:
	# Pad out rom data to 16-byte blocks
	rom_data += "\x00" * (16 - len(rom_data) % 16)

	aes_key = number.long_to_bytes(random.getrandbits(256))
	cypher = AESCipher(aes_key)
	encoded = cypher.encrypt(rom_data)

	checksum = SHA1Hash(encoded).digest()

	cert = checksum+aes_key

	key_path = path.join(path.dirname(argv[0]), 'mykey.pem')

	key = RSA.importKey(open(key_path).read())
	cipher = PKCS1_OAEP.new(key)
	ciphertext = cipher.encrypt(cert)

	fo.write(pack("<4sII", "CzmH", len(cert), len(encoded)))
	fo.write(cert)
	fo.write(encoded)
