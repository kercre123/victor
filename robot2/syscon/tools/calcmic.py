from itertools import izip_longest
import scipy.signal as signal

PDM_FTL_TAPS       = 16
PDM_FTL_SAMPLE_F   = 1024
PDM_FTL_CUT_OFF    = 8

taps = signal.firwin(PDM_FTL_TAPS*16, PDM_FTL_CUT_OFF, nyq=PDM_FTL_SAMPLE_F/2)

def grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"

    args = [iter(iterable)] * n
    return izip_longest(fillvalue=fillvalue, *args)

print "static const int32_t DECIMATION_TABLE[0x100][16] = {"
for x in grouper(taps[:PDM_FTL_TAPS*8], 8):
	values = []
	for i in range(0x100):
		total = 0
		for b, c in enumerate(x):
			bit = 1 if i & (1 << b) else -1
			total += int(c * bit * 0x80000000)
		values += ["%i" % total]

	print "{ %s }," % ", ".join(values)
print "};"
