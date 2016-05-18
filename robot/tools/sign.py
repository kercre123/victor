from __future__ import print_function

from zlib import crc32
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH

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
args = parser.parse_args()

BLOCK_LENGTH = 0x800


#  TODO: ADD CERT BUILDING

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

# This generates a single file for RTIP that is actually both
with open(args.output, "wb") as fo:
	for kind, fn in [('body', args.body), ('wifi', args.wifi), ('rtip', args.rtip)]:
		if fn == None:
			continue

		if kind == 'wifi':
			base_addr = 0x40000000
			rom_data = open(fn, "rb").read()
		else:	
			rom_data, base_addr = get_image(fn)
			if kind == 'body':
				base_addr |= 0x80000000

		for block, data in enumerate(chunk(rom_data, BLOCK_LENGTH)):
			fo.write(data)
			fo.write(pack("<II", block*BLOCK_LENGTH+base_addr, crc32(data)))
