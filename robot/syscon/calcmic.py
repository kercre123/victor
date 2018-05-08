import scipy.signal as signal
from json import dumps

DECIMATION          = 96
PDM_FTL_SAMPLE_F    = 1500
BYTE_TAPS           = 32
TAPS                = BYTE_TAPS * 8
PDM_FTL_CUT_OFF     = 5.859375 #Ideal PDM_FTL_SAMPLE_F / TAPS

taps = (signal.firwin(TAPS, PDM_FTL_CUT_OFF, nyq=PDM_FTL_SAMPLE_F/2) * 0x7FFFFFFF).astype(int)

DEINTERLACE_TABLE = []
for r in [0, 4]:
	l = []
	for x in range(0x100):
		o = ((x & 0x01) >> 0 <<  0) | ((x & 0x02) >> 1 <<  8) | ((x & 0x04) >> 2 <<  1) | ((x & 0x08) >> 3 <<  9) | ((x & 0x10) >> 4 <<  2) | ((x & 0x20) >> 5 << 10) | ((x & 0x40) >> 6 <<  3) | ((x & 0x80) >> 7 << 11)
		l += [o << r]
	DEINTERLACE_TABLE += [l]

DECIMATION_TABLE = []
for index in range(0, TAPS, 8):
	values = []
	for input in range(0x100):
		total = sum(coff if input & (1 << bit) else -coff for bit, coff in enumerate(taps[index:index+8]))
		values += [total]
	DECIMATION_TABLE += [values]

print "static const uint16_t DEINTERLACE_TABLE[2][0x100] ="
print "%s;" % dumps(DEINTERLACE_TABLE).replace("[", "{").replace("]", "}")
print "static const int32_t DECIMATION_TABLE[%i][0x100] =" % BYTE_TAPS
print "%s;" % dumps(DECIMATION_TABLE).replace("[", "{").replace("]", "}")
 