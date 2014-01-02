// ***************************************************************************
// Copyright(C)2011 Movidius Ltd. All rights reserved
// ---------------------------------------------------------------------------
// File       : moviFloat32.h
// Description: Header file for float operations
// ---------------------------------------------------------------------------
// HISTORY
// Version        | Date       | Owner         | Purpose
// ---------------------------------------------------------------------------
// 0.1            |            | Alin Dobre    | Initial version
// ***************************************************************************
#ifndef __MVFLOAT32_H__
#define __MVFLOAT32_H__

#define MOVIDIUS_FP32
// rounding modes
#define F32_RND_NEAREST_EVEN     0
#define F32_RND_MINUS_INF        1
#define F32_RND_PLUS_INF         2
#define F32_RND_TO_ZERO          3

// detect tinyness mode
#define F32_DETECT_TINY_AFTER_RND  0
#define F32_DETECT_TINY_BEFORE_RND 1

// exceptions
#define F32_EX_INEXACT     0x00000001//0x00000020
#define F32_EX_DIV_BY_ZERO 0x00000002//0x00000004
#define F32_EX_INVALID     0x00000004//0x00000001
#define F32_EX_UNDERFLOW   0x00000008//0x00000010
#define F32_EX_OVERFLOW    0x00000010//0x00000008

#define F32_NAN_DEFAULT    0xFFC00000

#define F64_NAN_DEFAULT_H  0xFFF80000
#define F64_NAN_DEFAULT_L  0x00000000

//Macros
#define EXTRACT_F16_SIGN(x)   ((x >> 15) & 0x1) 
#define EXTRACT_F16_EXP(x)    ((x >> 10) & 0x1F)
#define EXTRACT_F16_FRAC(x)   (x & 0x000003FF)
#define EXTRACT_F32_SIGN(x)   ((x >> 31) & 0x1) 
#define EXTRACT_F32_EXP(x)    ((x >> 23) & 0xFF)
#define EXTRACT_F32_FRAC(x)   (x & 0x007FFFFF)
#define EXTRACT_F64_FRAC_H(x) (x & 0x000FFFFF)
#define EXTRACT_F64_EXP(x)    ((x >> 20) & 0x7FF)
#define EXTRACT_F64_SIGN(x)   ((x >> 31) & 0x1)
#define RESET_SNAN_BIT(x)     x = x | 0x00400000

#define PACK_F32(x, y, z)     ((x << 31) + (y << 23) + z)
#define PACK_F16(x, y, z)     ((x << 15) + (y << 10) + z)

typedef struct 
{
    unsigned int low;
    unsigned int high;
} T_F64;

#define F16_IS_NAN(x)       ((x & 0x7FFF)> 0x7C00)
#define F16_IS_SNAN(x)      (((x & 0x7E00) == 0x7C00)&&((x & 0x1FF)> 0))
#define F32_IS_NAN(x)       ((x & 0x7FFFFFFF)> 0x7F800000)
#define F32_IS_SNAN(x)      (((x & 0x7FC00000) == 0x7F800000)&&((x & 0x3FFFFF)> 0))
#define F64_IS_NAN(x)       ((0xFFE00000 <=(x.high << 1))&&((x.low != 0)||((x.high & 0x000FFFFF) != 0)))
#define F64_IS_SNAN(x)      ((((x.high >> 19)& 0xFFF) == 0xFFE)&&((x.low != 0)||((x.high & 0x0007FFFF) != 0)))

unsigned int u32_to_f32_conv(int x, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int i32_to_f32_conv(int x, unsigned int rnd_mode, unsigned int* exceptions);
int f32_to_i32_conv(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions);
T_F64 f32_to_f64_conv(unsigned int x, unsigned int* exceptions);
unsigned int f64_to_f32_conv(unsigned int high, unsigned int low, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int round_f32_to_i32(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int f16_to_f32_conv(unsigned int x, unsigned int* exceptions);
unsigned int f32_to_f16_conv(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions);

unsigned int f32_add(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int f32_sub(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int f32_mul(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int f32_div(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions);
unsigned int f32_sqrt(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions);

T_F64 f64_add(T_F64 op1, T_F64 op2, unsigned int rnd_mode, unsigned int* exceptions);
T_F64 f64_sub(T_F64 op1, T_F64 op2, unsigned int rnd_mode, unsigned int* exceptions);
#endif
