#include "mic_tables.h"

#ifdef __x86_64__
typedef uint64_t intptr;
#else
typedef uint32_t intptr;
#endif

#define DEINTERLACE(input) \
	word  = DEINTERLACE_TABLE[0][*(input++)]; \
	word |= DEINTERLACE_TABLE[1][*(input++)];

#define SETUP_ACC(offset, channel) \
	coff_set = DECIMATION_TABLE[0][byte]; \
	*(output++) = accumulator[channel][(0+offset)&3] >> 16; \
	accumulator[channel][(0+offset)&3] = coff_set[0]; \
	accumulator[channel][(1+offset)&3] += coff_set[1]; \
	coff_set = DECIMATION_TABLE[7][REVERSE_TABLE[byte]]; \
	accumulator[channel][(2+offset)&3] += coff_set[1]; \
	accumulator[channel][(3+offset)&3] += coff_set[0]

#define ACCUMULATE(offset, channel, block) \
	coff_set = DECIMATION_TABLE[block][byte]; \
	accumulator[channel][(0+offset)&3] += coff_set[0]; \
	accumulator[channel][(1+offset)&3] += coff_set[1]; \
	coff_set = DECIMATION_TABLE[7-block][REVERSE_TABLE[byte]]; \
	accumulator[channel][(2+offset)&3] += coff_set[1]; \
	accumulator[channel][(3+offset)&3] += coff_set[0]

#define STAGE_0(offset) \
	DEINTERLACE(input_a); \
	byte = word & 0xFF; SETUP_ACC(offset, 0); \
	byte = word   >> 8; SETUP_ACC(offset, 1); \
	DEINTERLACE(input_b); \
	byte = word & 0xFF; SETUP_ACC(offset, 2); \
	byte = word   >> 8; SETUP_ACC(offset, 3)

#define STAGE_N(offset, block) \
	DEINTERLACE(input_a); \
	byte = word & 0xFF; ACCUMULATE(offset, 0, block); \
	byte = word   >> 8; ACCUMULATE(offset, 1, block); \
	DEINTERLACE(input_b); \
	byte = word & 0xFF; ACCUMULATE(offset, 2, block); \
	byte = word   >> 8; ACCUMULATE(offset, 3, block)

#define PASS(offset) \
	STAGE_0(offset); \
	STAGE_N(offset, 1); \
	STAGE_N(offset, 2); \
	STAGE_N(offset, 3); \
	STAGE_N(offset, 4); \
	STAGE_N(offset, 5); \
	STAGE_N(offset, 6); \
	STAGE_N(offset, 7)
