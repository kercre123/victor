#ifdef __cplusplus
extern "C" {
#endif

/*
***************************************************************************
  (C) Copyright 2009 Signal Essence; All Rights Reserved

  Module Name  - frmath.h

  Author: Hugh McLaughlin

  Date: Mar24,09 hjm created
        Dec09,09 hjm made compat with se_types, added FrMultiplyRightShifts
                     and FrMultiplyLeftShifts
        Sep 20, 2010 ryu Add LimitInt40ToInt32

  Fixed point, fractional math

**************************************************************************
*/

#ifndef __frmath_h
#define __frmath_h

#include "se_types.h"
#include "se_platform.h"
#include "optimized_routines.h"

#define UNITY_Q10 1024
#define UNITY_Q10_FLT 1024.0f

#define UNITY_Q12 4096
#define UNITY_Q12_FLT 4096.0f
#define Q12_LEFT_SHIFTS 3

#define UNITY_Q13 8192
#define UNITY_Q13_FLT 8192.0f

#define UNITY_Q14 16384
#define UNITY_Q14_FLT 16384.0f

//
// UNITY_Q15{_FLT} is a little strange:
// for the integer version, we need a version which fits in 16 bits, 
// hence 32767 rather than 32768.
// The value for the float version, 32768, is more accurate.
#define UNITY_Q15 32767
#define UNITY_Q15_FLT 32768.0f

//
// platform-safe replacement for (uint32)(1<<lshift)
// because C55 compiler is mentally challenged
//
// hint:  you can find all instances of "1 << N" via grep:
// grep -e "1\ *<<" *.c
//
#define FR_UNITY_LSHIFT_I32(lshift) ((uint32)2147483648UL>>(31-(lshift)))

//
// absolute value
// for integers
// this is a "safe" implementation in that
// FR_ABS_INT(-32768) = +32767, even though -1 * -32768 = undefined
#define FR_ABS_INT16(x)  ((int16)(x) == (SE_MIN_INT16) ?  (SE_MAX_INT16) : ((int16)(x)) < 0 ? -(x) : (x))

#define FR_ABS_INT32(x)  ((int32)(x) == (SE_MIN_INT32) ?  (SE_MAX_INT32) : ((int32)(x)) < 0 ? -(x) : (x))


typedef struct
{
    int16 Fr;
    int16 Exp;
} FrExp_t;


int16 FrMultiply(
        int16,                /* 1st multiplicand */
        int16                 /* 2nd multiplicand */
        );

int16 FrMultiplyWithExp(
        int16,                /* 1st multiplicand */
        int16,                /* 2nd multiplicand */
        int16                 /* number of left shifts */
        );

int16 FrMultiplyRightShifts(
        int16 x,                /* 1st multiplicand */
        int16 y,                /* 2nd multiplicand */
        uint16 rs );            // number of right shifts

int16 FrMultiplyLeftShifts(
        int16 x,                 /* 1st multiplicand */
        int16 y,                 /* 2nd multiplicand */
        uint16 ls );             // number of left shifts

void FrMultiplyExp(
        FrExp_t *,                /* 1st multiplicand, fractional part plus exponent */
        FrExp_t *,                /* 2nd multiplicand, fractional part plus exponent */
        FrExp_t *                 /* result, normalized fractional part */
        );

int16 FracMulNormExp(
        int16,                /* 1st multiplicand */
        int16,                /* 2nd multiplicand */
        int16*                /* * to result */
        );


/*=========================================================================
   Function Name:    FrMulAdd()

   Returns:          short result = (x*y) + z

===========================================================================*/

int16 FrMulAdd(
        int16 x,                // 1st multiplicand
        int16 y,                // 2nd multiplicand
        int16 z );              // number to add

// same thing except if in C code checking for saturation is expensive,
// so if you know beforehand it won't saturate then call this.
// for optimized code they will be the same thing
int16 FrMulAddNoSat(
        int16 x,                // 1st multiplicand
        int16 y,                // 2nd multiplicand
        int16 z );              // number to add



static INLINE int32 LimitFloatToInt32_ansi( float x )
{
    if (x > (float)SE_MAX_INT32)
	x = (float)SE_MAX_INT32;
    else if (x < (float)SE_MIN_INT32)
	x = (float)SE_MIN_INT32;
    return (int32)x;
}
static INLINE int16 LimitFloatToInt16_ansi( float x )
{
    if (x > (float)SE_MAX_INT16)
	x = (float)SE_MAX_INT16;
    else if (x < (float)SE_MIN_INT16)
	x = (float)SE_MIN_INT16;
    return (int16)x;
}

static INLINE int16 LimitInt32ToInt16_ansi( int32 x32 )
{
    return (x32 > SE_MAX_INT16) ? SE_MAX_INT16 : (x32 < SE_MIN_INT16) ? SE_MIN_INT16 : (int16)x32;
}
static INLINE int32 LimitInt64ToInt32_ansi( int64 x )
{
  return (x > (int64)SE_MAX_INT32) ? (int32)SE_MAX_INT32 : ( x < (int64)SE_MIN_INT32) ? (int32)SE_MIN_INT32 : (int32)x;
}

#ifndef LimitInt32ToInt16
#define LimitInt32ToInt16 LimitInt32ToInt16_ansi
#endif
#ifndef LimitInt64ToInt32
#define LimitInt64ToInt32 LimitInt64ToInt32_ansi
#endif

#ifndef LimitFloatToInt32
#define LimitFloatToInt32  LimitFloatToInt32_ansi
#endif
#ifndef LimitFloatToInt16
#define LimitFloatToInt16  LimitFloatToInt16_ansi
#endif


int16 ExpConvert(
        int16 input,            /* to be converted */
        int16 expin,            /* input exponent */
        int16 expout            /* output exponent */
        );

/* multiply a fractional 16 bit Q15 number by a long word resulting in a long */


//
// scale 16 bit number by Q10
int16 FrMul_i16_Q10_i16(int16 x16,  int16 q10);

//
// scale 16 bit number by Q15
int16 FrMul_i16_Q15_i16( int16 x16, int16 q15val );

typedef int32 (*FrMul_Q15_i32_t)(int16 x16, int32 y32);
extern FrMul_Q15_i32_t FrMul_Q15_i32;

//
// these are for testing only
int32 FrMul_Q15_i32_ansi(int16 x16, int32 y32);

/* Returns the left shift count needed to normalize src to a 32-bit signed long value. */

typedef uint16 (*GetNormExp_i32_t)( int32 src2 );
extern  GetNormExp_i32_t GetNormExp_i32;

uint16 GetNormExp_i16( int16 val );

uint16 GetNormExp_u32( uint32 ulval );  // positive numbers only
uint16 GetNormExp_u16( uint16 uy );
uint16 GetNormExpByte( uint16 uy );
uint16 GetNormExpNibble( uint16 uy );

int16 QdSqrt_i32_i16( uint32 u32Power );

int16 RMS_to_dB( int16 rms );

uint16 Divide_u32_Q10( uint32 y, uint32 x );

uint32 div_u32_Unity_2_23( uint32 y, uint32 x );

int32 Max_i32( int32 x, int32 y );

int32 FrMul_i32_Q10_i32( int32 x32, int16 q10val);
int32 FrMul_i32_Q12_i32( int32 x32, int16 q12val);
float32 FrMul_Q15_f32(int16 q15, float32 f);

#define FR_MIN_INT16(_x,_y) ((int16)(_x) <= (int16)(_y)) ? (int16)(_x) : (int16)(_y)
#define FR_MAX_INT16(_x,_y) ((int16)(_x) >= (int16)(_y)) ? (int16)(_x) : (int16)(_y)

#define FR_MIN_INT(_x,_y) ((_x) <= (_y)) ? (_x) : (_y)
#define FR_MAX_INT(_x,_y) ((_x) >= (_y)) ? (_x) : (_y)

#endif
#ifdef __cplusplus
}
#endif

