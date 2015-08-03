#include "adpcm.h"

static uint8_t MuLawCompressTable[] =
{
     0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
     5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
     6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
     6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

// Resulting values are the state for the next block
// Length is in input samples (800)

void encodeMuLaw(int16_t *in, uint8_t *out, int length) {
	for(;length > 0; length --) {
		int16_t sample = *(in++);
          bool sign = sample < 0;

          if (sign)	{
               sample = ~sample;
          }

          uint8_t exponent = MuLawCompressTable[sample >> 8];
          uint8_t mantessa;

          if (exponent) {
               mantessa = (sample >> (exponent + 3)) & 0xF;
          } else {
               mantessa = sample >> 4;
          }

		*(out++) = (sign ? 0x80 : 0) | (exponent << 4) | mantessa;
	}
}
