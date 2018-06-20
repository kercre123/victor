from struct import pack
from xxtea import encrypt, decrypt
import sys, datetime,re 

BLOCK_SIZE = 20

with open(sys.argv[3], "r") as fo:
	key = bytes([int(c, 16) for c in re.split(r'\s*,\s*', fo.read())])

ts = datetime.datetime.today().isoformat(timespec='minutes').encode() + (b'\x00' * 16)

with open(sys.argv[1], "rb") as fi:
	original = ts[:16] + fi.read()[16:]
	original = original + b'\x00' * (4 - (len(original) & 4))

	data = ts[:16] + encrypt(original, key)

	with open(sys.argv[2], "wb") as fo:
		fo.write(data + b'\xff')
