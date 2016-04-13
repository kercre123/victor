from __future__ import print_function
import zlib
from hashlib import sha1
import os.path as path
from sys import argv
from struct import pack

from elftools.elf.elffile import ELFFile

HEADER_LENGTH = 128
BLOCK_LENGTH = 0x1000

def chunk(i, size):
	for x in range(0, len(i), size):
		yield i[x:x+size]

def rom_info(file):
	with open(file, "rb") as fo:
		elffile = ELFFile(fo)

		segments = {}

		# Build rom segments
		for segment in elffile.iter_segments():
			fo.seek(segment['p_offset'])
			segments[segment['p_vaddr']] = fo.read(segment['p_filesz'])

		# Locate vector table + magic header
		for section in elffile.iter_sections():
			if section.name == b"ER_IROM1":
				temp = fo.tell()
				fo.seek(section.header.sh_offset + 4)
				if fo.read(4) != b"CZM0":
					raise Exception("Could not locate cozmo header")
				fo.seek(temp)

				magic_location = section.header.sh_offset + 12

		# flatten rom image
		first = min(segments.keys())
		last = max([addr+len(data) for addr, data in segments.items() if addr < 0x40000])

		base_addr = first
		rom_data = b"\xFF" * (last-first)

		for addr, data in segments.items():
			addr = addr - first
			rom_data = rom_data[:addr] + data + rom_data[addr+len(data):]
	return rom_data, base_addr, magic_location			

def fix_header(fn):
	print("Fixing header for %s" % fn)

	rom_data, base_addr, magic_location = rom_info(fn)
	
	with open(fn, "rb+") as fo:
		checksum = zlib.crc32(rom_data[HEADER_LENGTH:])

		fo.seek(magic_location)
		fo.write(pack("<III", base_addr+HEADER_LENGTH, len(rom_data) - HEADER_LENGTH, checksum))

	return 

def get_image(fn):
	rom_data, base_addr, _ = rom_info(fn)

	# Clear our our evil byte
	rom_data = rom_data[:24] + b'\xFF\xFF\xFF\xFF' + rom_data[28:]

	# Zero pad to a 4k block (flash area)
	while len(rom_data) % BLOCK_LENGTH:
		rom_data += b"\xFF"
	
	return rom_data, base_addr

# Fix headers
with open(argv[-1], "wb") as fo:
	for fn in argv[1:-1]:		
		kind, fn = fn.split(":")
		fix_header(fn)

		# Write to our safe
		rom_data, base_addr = get_image(fn)

		for block, data in enumerate(chunk(rom_data, BLOCK_LENGTH)):
			if kind == 'body':
				base_addr |= 0x80000000

			fo.write(data)
			fo.write(pack("<I", block*BLOCK_LENGTH+base_addr))

			if kind == 'legacy':
				fo.write(sha1(data).digest())
			else:
				fo.write(pack("<IIIII", zlib.crc32(data), 0, 0, 0, 0))
