#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   (C) Copyright 2010 SignalEssence; All Rights Reserved

    Module Name: floatmath

    Author: 

    Description:
    Signal Essence floating point routines

    History:
    2010-11-11 first created  ryu

    Machine/Compiler:
    (ANSI C)

**************************************************************************/
#ifndef FLOAT_MATH_H
#define FLOAT_MATH_H

#include <float.h>
#include "se_types.h"
#include <math.h>
//
// one definition of pi to rule them all
#define SE_PI     3.14159265358979323846f
#define SE_TWO_PI 6.283185307179586476925286766559f

//
// log10 with lower bound:
// compute log10(x+epsilon), where epsilon s.t.
// log10(0 + epsilon) ~= -379.29
//
// the motivation behind this function is that different compilers treat
// (int16)(-1.#INF) differently; under Win32,
// (int16)(-1.#INF) = -32768 (correct)
// but on the C55,
// (int16)(-1.#INF) = 0 (WRONG!!!)
#define FLT_LOG10_LWR_BOUND(_x)   ((float32) ( log10((_x) + FLT_MIN) ))
#define FLT_LN_LWR_BOUND(_x)      ((float32) ( log((_x) + FLT_MIN)))
#define FLT_LOG2_LWR_BOUND(_x)    ((float32) ( log10((_x) + FLT_MIN)/log10(2.0)))

float32 ConvertVoltsToDb(float32 x);
float32 ConvertPowerToDb(float32 x);
float32 ConvertDbToVoltageRatio(float32 x);
float32 ConvertDbToPowerRatio(float32 x);


int16 ConvertLinearPowerToDbX10_i16( float32 power );
int16 ConvertFloatToQ10( float32 fin );
int16 ConvertFloatToQ12( float32 fin );
int16 ConvertFloatToQ13( float32 fin );
int16 ConvertFloatToQ15( float32 fin );
int16 ConvertFloatToInt16( float32 fx);
int16 ConvertFloatToInt16RoundAwayFromZero( float32 fx);
int32 ConvertFloatToInt32( float32 fx);

#define MAX_FLOAT(_x,_y) ((float)(_x) >= (float)(_y)) ? (float)(_x) : (float)(_y)
#define MIN_FLOAT(_x,_y) ((float)(_x) <= (float)(_y)) ? (float)(_x) : (float)(_y)

float32 ConvertQ15ToFloat(int16 q15);
float32 ConvertQ12ToFloat(int16 q12);

//
// GCC implements C99 "round()" function which rounds to closest integer.
// MSVC does not.
#define SE_ROUND_FLT32(_x) ((float32)(floor(_x + 0.5f)))

// check if x==inf
// again, MSVC does not
#ifdef _MSC_VER
#define SE_IS_INF(x) (!_finite(x))
#define SE_IS_NAN(x) _isnan(x)
#else
#define SE_IS_INF(x) isinf(x)
#define SE_IS_NAN(x) isnan(x)
#endif

#endif // #ifndef FLOAT_MATH_H


#ifdef __cplusplus
}
#endif

