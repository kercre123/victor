from itertools import izip_longest
from json import dumps
import scipy.signal as signal

PDM_FTL_TAPS       = 16
PDM_FTL_SAMPLE_F   = 1024
PDM_FTL_CUT_OFF    = 8

taps = (signal.firwin(PDM_FTL_TAPS*16, PDM_FTL_CUT_OFF, nyq=PDM_FTL_SAMPLE_F/2) * 0x7FFFFFFF).astype(int)

REVERSE_TABLE = []
for b in range(0x100):
	b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1)
	b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2)
	b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4)

	REVERSE_TABLE += [b]

DEINTERLACE_TABLE = []
for r in [0, 4]:
	l = []
	for x in range(0x100):
		o = ((x & 0x01) >> 0 <<  0) | ((x & 0x02) >> 1 <<  8) | ((x & 0x04) >> 2 <<  1) | ((x & 0x08) >> 3 <<  9) | ((x & 0x10) >> 4 <<  2) | ((x & 0x20) >> 5 << 10) | ((x & 0x40) >> 6 <<  3) | ((x & 0x80) >> 7 << 11)
		l += [o << r]
	DEINTERLACE_TABLE += [l]

DECIMATION_TABLE = []
for block in range(0, 64, 8):
	entries = []
	for input in range(0x100):
		values = []
		for offset in range(0, 128, 64):
			index = block + offset
			values += [sum(coff if input & (1 << bit) else -coff for bit, coff in enumerate(taps[index:index+8]))]
		entries += [values]
	DECIMATION_TABLE += [entries]

print "static const uint16_t REVERSE_TABLE[0x100] ="
print "%s;" % dumps(REVERSE_TABLE).replace("[", "{").replace("]", "}")
print "static const uint16_t DEINTERLACE_TABLE[2][0x100] ="
print "%s;" % dumps(DEINTERLACE_TABLE).replace("[", "{").replace("]", "}")
print "static const int32_t DECIMATION_TABLE[8][0x100][2] ="
print "%s;" % dumps(DECIMATION_TABLE).replace("[", "{").replace("]", "}")
