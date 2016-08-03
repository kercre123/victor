from __future__ import print_function

from sys import argv, version_info
from struct import pack
from elfInfo import rom_info, HEADER_LENGTH
import zlib

def fix_header(fn):
	print("Fixing header for %s" % fn)

	rom_data, base_addr, magic_location = rom_info(fn)

	with open(fn, "rb+") as fo:
		checksum = zlib.crc32(rom_data[HEADER_LENGTH:])

		fmt = "<IIi" if version_info.major < 3 else "<III" # Zlib crc32 output changed from signed to unsigned

		fo.seek(magic_location)
		fo.write(pack(fmt, base_addr+HEADER_LENGTH, len(rom_data) - HEADER_LENGTH, checksum))

	return 

# Fix headers
for fn in argv[1:]:
	fix_header(fn)
