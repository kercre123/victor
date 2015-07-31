#include "adpcm.h"

const int cBias = 0x84;
const int cClip = 32635;

static uint8_t MuLawCompressTable[256] =
{
     0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
     4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
     5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
     5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
     6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
     6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
     6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
     6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

// Resulting values are the state for the next block
// Length is in input samples (800)

void encodeMuLaw(int16_t *in, uint8_t *out, int length) {
	for(;length > 0; length --) {
		int sign = (sample >> 8) & 0x80;
		if (sign) {
			sample = (short)-sample;
		}
		if (sample > cClip) {
			sample = cClip;
		}

		sample = (short)(sample + cBias);
		int exponent = (int)MuLawCompressTable[(sample>>7) & 0xFF];
		int mantissa = (sample >> (exponent+3)) & 0x0F;
		int compressedByte = ~(sign | (exponent << 4) | mantissa);

		*(out++) = compressedByte;
	}
}
