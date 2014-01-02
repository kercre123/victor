// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW model common definitions
//
// Mainly simulation related
// -----------------------------------------------------------------------------

#ifndef __SIPP_COMMON_H__
#define __SIPP_COMMON_H__

#include "sippHwCommon.h"

#ifdef WIN32
#include <cstdint>
#include <dos.h>
#define sleep(n) Sleep(n)
#else
#include <stdint.h>
#include <unistd.h>
#endif

#define SLICE_SIZE       128*1024

// Bayer space macros
#define SQR(x)           ((x)*(x))
#define ABS(x)           (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a,b)         ((a) < (b) ? (a) : (b))
#define MAX(a,b)         ((a) > (b) ? (a) : (b))
#define LIM(x,min,max)   MAX(min,MIN(x,max))
#define ULIM(x,y,z)      ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x,max_val)  LIM(x,0,max_val)
#define CLIPF(x,max_val) LIM(x,fp16(0.f),max_val)
#define DOWNCAR(x,diff)  (((x) >> (diff)) + (((x) >> (diff - 1)) & 0x1))
#define UPCONV(x,diff)   ((x) << (diff))
// - Blue pixel
#define BP(filt_pattern, i, j) ( ((line_idx % 2) &&                      \
                   ( ((filt_pattern == GRBG) && !((i)%2) && !((j)%2)) || \
                     ((filt_pattern == RGGB) && !((i)%2) &&  ((j)%2)) || \
                     ((filt_pattern == GBRG) &&  ((i)%2) &&  ((j)%2)) || \
                     ((filt_pattern == BGGR) &&  ((i)%2) && !((j)%2)) ) )\
                            ||  (!(line_idx % 2) &&                      \
                   ( ((filt_pattern == GRBG) &&  ((i)%2) && !((j)%2)) || \
                     ((filt_pattern == RGGB) &&  ((i)%2) &&  ((j)%2)) || \
                     ((filt_pattern == GBRG) && !((i)%2) &&  ((j)%2)) || \
                     ((filt_pattern == BGGR) && !((i)%2) && !((j)%2)) ) ) )
// - Red pixel
#define RP(filt_pattern, i, j) ( ((line_idx % 2) &&                      \
                   ( ((filt_pattern == GRBG) &&  ((i)%2) &&  ((j)%2)) || \
	                 ((filt_pattern == RGGB) &&  ((i)%2) && !((j)%2)) || \
                     ((filt_pattern == GBRG) && !((i)%2) && !((j)%2)) || \
                     ((filt_pattern == BGGR) && !((i)%2) &&  ((j)%2)) ) )\
                            ||  (!(line_idx % 2) &&                      \
                   ( ((filt_pattern == GRBG) && !((i)%2) &&  ((j)%2)) || \
	                 ((filt_pattern == RGGB) && !((i)%2) && !((j)%2)) || \
                     ((filt_pattern == GBRG) &&  ((i)%2) && !((j)%2)) || \
                     ((filt_pattern == BGGR) &&  ((i)%2) &&  ((j)%2)) ) ) )

// - Blue line
#define BL(filt_pattern, i) ( ( (line_idx % 2) &&         \
                ( ((filt_pattern == GRBG) && !((i)%2)) || \
                  ((filt_pattern == RGGB) && !((i)%2)) || \
                  ((filt_pattern == GBRG) &&  ((i)%2)) || \
                  ((filt_pattern == BGGR) &&  ((i)%2)) ) )\
                         ||   (!(line_idx % 2) &&         \
                ( ((filt_pattern == GRBG) &&  ((i)%2)) || \
                  ((filt_pattern == RGGB) &&  ((i)%2)) || \
                  ((filt_pattern == GBRG) && !((i)%2)) || \
                  ((filt_pattern == BGGR) && !((i)%2)) ) ) )
// - Red line
#define RL(filt_pattern, i) ( ( (line_idx % 2) &&         \
	            ( ((filt_pattern == GRBG) &&  ((i)%2)) || \
	              ((filt_pattern == RGGB) &&  ((i)%2)) || \
	              ((filt_pattern == GBRG) && !((i)%2)) || \
	              ((filt_pattern == BGGR) && !((i)%2)) ) )\
                         ||   (!(line_idx % 2) &&         \
	            ( ((filt_pattern == GRBG) && !((i)%2)) || \
	              ((filt_pattern == RGGB) && !((i)%2)) || \
	              ((filt_pattern == GBRG) &&  ((i)%2)) || \
	              ((filt_pattern == BGGR) &&  ((i)%2)) ) ) )

// Fixed point arithmetic
typedef union FIXED8_8tag {
	uint16_t full;
	struct part8_8tag {
		uint8_t fraction: 8;
		uint8_t integer: 8;
	} part;
} FIXED8_8;

typedef union FIXED0_16tag {
	uint16_t full;
	struct part0_16tag {
		uint16_t fraction: 16;
	} part;
} FIXED0_16;

typedef union FIXED16_8tag {
	uint32_t full;
	struct part16_8tag {
		uint32_t fraction: 8;
		uint32_t integer: 16;
	} part;
} FIXED16_8;

typedef union FIXED24_8tag {
	uint32_t full;
	struct part24_8tag {
		uint32_t fraction: 8;
		uint32_t integer: 24;
	} part;
} FIXED24_8;

typedef union FIXED16_16tag {
	uint32_t full;
	struct part16_16tag {
		uint16_t fraction: 16;
		uint16_t integer: 16;
	} part;
} FIXED16_16;

typedef union FIXED6_10tag {
	uint16_t full;
	struct part6_10tag {
		uint16_t fraction: 10;
		uint16_t integer: 5;
		uint16_t sign: 1;
	} part;
} FIXED6_10;

typedef union FIXED4_8tag {
	uint16_t full;
	struct part4_8tag {
		uint8_t fraction: 8;
		uint8_t integer: 4;
	} part;
} FIXED4_8;

typedef union FIXED1_7tag {
	uint8_t full;
	struct part1_7tag {
		uint8_t fraction: 7;
		uint8_t integer: 1;
	} part;
} FIXED1_7;

#define FLOAT_TO_FIXED8_8(A, B)     (uint16_t)(((A) << 8) + ((B) * 256))
#define FLOAT_TO_FIXED16_8(A, B)    (uint32_t)(((A) << 8) + ((B) * 256))
#define MUL8_8(A, B)                (uint32_t)(((uint64_t)A.full * B.full + (1<<7))>>8)
#define SUB8_8(A, B)                (uint16_t)(A.full - B.full)
#define ADD8_8(A, B)                (uint16_t)(A.full + B.full)
#define ADD168_88(A, B)             (uint32_t)(A.full + B.full)
#define FIXED8_8_TO_FLOAT(A)        (float)(A.part.integer + (float)A.part.fraction/256)
#define FIXED16_8_TO_FLOAT(A)       (float)(A.part.integer + (float)A.part.fraction/256)
								    
#define FLOAT_TO_FIXED0_16(A, B)    (uint16_t)(((A) << 16) + ((B) * 65536))
#define MUL0_16(A, B)               (uint32_t)(A.full * B.full + (1<<15))>>16
#define FIXED0_16_TO_FLOAT(A)       (float)((float)A.part.fraction/65536)
								    
#define MUL88_016(A, B)             (uint32_t)((((uint64_t)A.full<<16) * (B.full<<8) + (1<<23))>>24)
#define MUL88_016_R016(A, B)        (uint16_t)((((uint64_t)A.full<<16) * (B.full<<8) + ((uint64_t)1<<31))>>32)
#define MUL88_016_R88(A, B)         (uint16_t)((((uint64_t)A.full<<16) * (B.full<<8) + ((uint64_t)1<<39))>>40)
#define MUL88_80_R80(A, B)          (((uint64_t)A.full * (B<<8) + (1<<15))>>16)
#define MUL88_80_R160(A, B)         (((uint64_t)A.full * (B<<8) + (1<<7))>>8)
#define MUL88_160_R80(A, B)         (((uint64_t)A.full * (B<<8) + (1<<23))>>24)
#define MUL88_160_R160(A, B)        (((uint64_t)A.full * (B<<8) + (1<<15))>>16)
#define MUL160_016(A, B)            (((uint64_t)A<<16) * B.full + (1<<15))>>16
#define ADD1616_016(A, B)           (uint32_t)(A.full + B.full)
#define ADD1616_160(A, B)           (uint32_t)(A.full + (B<<16))
#define FIXED16_16_TO_FLOAT(A)      (float)(A.part.integer + (float)A.part.fraction/65536)
#define FLOAT_TO_FIXED6_10(A)       (int16_t)((A) * 1024)
#define FLOAT_TO_FIXED4_8(A, B)     (uint16_t)(((A) << 8) + ((B) * 256))
#define FLOAT_TO_FIXED1_7(A, B)     (uint8_t) (((A) << 7) + ((B) * 128))

#endif // __SIPP_COMMON_H__
