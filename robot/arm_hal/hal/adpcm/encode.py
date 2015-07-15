import struct

BLOCK_SIZE = 800

indexTable = [
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8,
]

stepsizeTable = [
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
]

def chunk(d, s=2):
	for i in range(0, len(d), s):
		yield d[i:i+s]

def encodeADPCM(data):
	index = 0
	valpred = 0

	for delta in data:
		step = stepsizeTable[index]

		diff = delta - valpred

		if diff >= 0:
			code = 0;
		else:
			code = 8
			diff = -diff

		tempstep = step

		if diff >= tempstep:
			code = (code | 4)
			diff = diff - tempstep

		tempstep = tempstep >> 1

		if diff >= tempstep:
			code = (code | 2)
			diff = diff - tempstep

		tempstep = tempstep >> 1

		if diff >= tempstep:
			code = (code | 1)

		diffq = step >> 3
		
		if ((code & 4) == 4):
			diffq += step
		if ((code & 2) == 2):
			diffq += step >> 1
		if ((code & 1) == 1):
			diffq += step >> 2

		sign = (code & 8) == 8

		if sign:
			valpred -= diffq
		else:
			valpred += diffq

		if valpred > 32767:
			valpred = 32767
		elif valpred < -32768:
			valpred = -32768
					
		index += indexTable[code]
		
		if index < 0:
			index = 0
		if index > 88:
			index = 88

		yield index, valpred, code
		
def pack(i):
	i = iter(i)
	while True:
		try:
			a = i.next() & 15
			b = i.next() & 15

			yield a | (b << 4)
		except:
			break

data = file("adventure.wav","rb").read()
tag = data.find("data")
_, bytes, = struct.unpack("<4sI", data[tag:tag+8])
data = data[tag+8:tag+8+bytes]

# load samples, pad out to 33.3ms sections
print "Loading..."
samples = [struct.unpack("<h", x)[0] for x in chunk(data)]
align = len(samples) % BLOCK_SIZE

if align:
	samples += [0] * (BLOCK_SIZE - align)

with file("adventure.adp", "wb") as fo:
	for i, block in enumerate(chunk(samples, BLOCK_SIZE)):
		print "Encoding block %i of %i" % (i+1, len(samples) / BLOCK_SIZE)

		adpcm = [s for s in encodeADPCM(block)]
		index, pred, _ = adpcm[0]
		adpcm = [s for s in pack([s for i, p, s in adpcm])]

		fo.write(struct.pack("<400B", *adpcm))
		fo.write(struct.pack("<hB", pred, index))
