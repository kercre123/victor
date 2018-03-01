from struct import pack
from xxtea import encrypt, decrypt
import sys

BLOCK_SIZE = 20
key = b'\x88\xdb\x37\x96\x76\x8f\x86\x91\x42\x24\xf4\x35\xc1\xfd\x7f\xd3'

with open(sys.argv[1], "rb") as fi:
	original = fi.read()
	original = original + b'\x00' * (4 - (len(original) & 4))

	data = encrypt(original, key)

	with open(sys.argv[2], "wb") as fo:
		fo.write(data + b'\xff')
