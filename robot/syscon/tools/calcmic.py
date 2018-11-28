import scipy.signal as signal
from json import dumps

DECIMATION          = 96
PDM_FTL_SAMPLE_F    = 1500
BYTE_TAPS           = 32
TAPS                = BYTE_TAPS * 8
PDM_FTL_CUT_OFF     = 5.859375 #Ideal PDM_FTL_SAMPLE_F / TAPS

taps = (signal.firwin(TAPS, PDM_FTL_CUT_OFF, nyq=PDM_FTL_SAMPLE_F/2) * 0x7FFFFFFF).astype(int)

def groups():
	for i in [8,9,10,11,0,1,2,3,4,5,6,7]:
		yield i + 24
		yield i + 12
		yield i

groups = [k for k in groups() if k < 32]

def val(input, bit):
	bit_order = [
		0b00000001,
		0b00000100,
		0b00010000,
		0b01000000,
		0b00000010,
		0b00001000,
		0b00100000,
		0b10000000,
	]

	if input & bit_order[bit]:
		return 1
	else:
		return -1


print "  AREA    ER_BINARIES, CODE, READONLY"
print "  EXPORT DECIMATION_TABLE"
print "DECIMATION_TABLE"
for input in range(0x100):
	values = []
	for group in groups:
		index = group * 8
		total = sum(coff * val(input, bit) for bit, coff in enumerate(taps[index:index+8]))
		values += [total]
	print "  DCD %s" % ', '.join(["%i" % v for v in values])

print "  END"
