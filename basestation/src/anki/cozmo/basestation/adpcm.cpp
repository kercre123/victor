#include "adpcm.h"

const static int8_t ima_index_table[] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

const static uint16_t stepsize_table[] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static inline int clamp(int min, int v, int max) {
	if (v > max) { return max; }
	if (v < min) { return min; }
	return v;
}

static uint8_t next_nibble(int &index, int &valpred, int16_t delta) {
	int step = stepsize_table[index];
	int diff = (int)delta - valpred;
	bool sign = diff < 0;
	uint8_t code;

	if (sign) {
		diff = -diff;
		code = 8;
	} else {
		code = 0;
	}

	int diffq = step >> 3;

	for (int i = 0; i < 3; i++) {
		if (diff >= step) {
			code |= 4 >> i;
			diffq += step >> i;
			diff -= step >> i;
		}
	}

	valpred = clamp(-32768, valpred + (sign ? -diffq : diffq), 32767);
	index = clamp(0, index + ima_index_table[code], 88);
  
	return code;
}

// First block should starts with index + predictor = 0
// Resulting values are the state for the next block
// Length is in input samples (800)

void encodeADPCM(int &index, int &predictor, int16_t *in, uint8_t *out, int length) {
	for(;length > 0; length -= 2) {
		uint8_t lo = next_nibble(index, predictor, *(in++));
		uint8_t hi = next_nibble(index, predictor, *(in++));
		*(out++) = (hi << 4) | (lo);
	}
}
