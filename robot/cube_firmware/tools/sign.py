from struct import pack
from xxtea import encrypt, decrypt
import sys

BLOCK_SIZE = 15
key = b'\x88\xdb\x37\x96\x76\x8f\x86\x91\x42\x24\xf4\x35\xc1\xfd\x7f\xd3'

def chunk(fi):
	offset = 0
	while True:
		block = fi.read(BLOCK_SIZE)

		if not block:
			break

		block += b'\x00' * (BLOCK_SIZE - len(block))

		yield offset, block

		offset += BLOCK_SIZE

with open(sys.argv[1], "rb") as fi:
	all_chunks = [s for s in chunk(fi)]
	all_chunks[-1] = all_chunks[-1][0] | 0x8000, all_chunks[-1][1]

	with open(sys.argv[2], "wb") as fo:
		for offset, data in all_chunks:
			encoded = encrypt(pack("<HH%is" % BLOCK_SIZE, offset, sum(data), data), key)
			fo.write(encoded)
