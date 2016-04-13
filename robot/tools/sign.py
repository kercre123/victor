from __future__ import print_function

from zlib import crc32
from hashlib import sha1
from sys import argv
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH

BLOCK_LENGTH = 0x1000

def chunk(i, size):
	for x in range(0, len(i), size):
		yield i[x:x+size]

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
				fo.write(pack("<IIIII", crc32(data), 0, 0, 0, 0))
