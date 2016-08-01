from __future__ import print_function

from sys import argv
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH
import zlib

def fix_header(fn):
	print("Fixing header for %s" % fn)

	rom_data, base_addr, magic_location = rom_info(fn)

	with open(fn, "rb+") as fo:
		checksum = zlib.crc32(rom_data[HEADER_LENGTH:])

		fo.seek(magic_location)
		fo.write(pack("<IIi", base_addr+HEADER_LENGTH, len(rom_data) - HEADER_LENGTH, checksum))

	return 

# Fix headers
for fn in argv[1:]:
	fix_header(fn)
