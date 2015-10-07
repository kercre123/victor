/**
* File: constantsAndMacros.h
*
* Author:  Andrew Stein (andrew)
* Created: 10/7/2013
*
* Information on last revision to this file:
*    $LastChangedDate$
*    $LastChangedBy$
*    $LastChangedRevision$
*
* Description:   This should only contain very basic constants and macros
*                that are used *everywhere*.  It should not pull in lots of
*                extra #includes or pollute namespaces with "using" directives.
*                Note that it is also used by embedded code (not just
*                basestation), so it should only use / define things that are
*                safe for that build environment.  So no basestation-specific
*                code or types (and no throwing exceptions).
*
*                Please limit additions to this file and try to keep it
*                organized.  Add new definitions in an existing / appropriate
*                or add a new, descriptive section when needed (not "misc").
*                This should not become a dumping ground for every random
*                little utility macro.
*
* Copyright: Anki, Inc. 2013
*
**/

#ifndef CORETECH_COMMON_CONSTANTS_AND_MACROS_H_
#define CORETECH_COMMON_CONSTANTS_AND_MACROS_H_

#include <math.h>

//////////////////////////////////////////////////////////////////////////////
// MISCELLANEOUS CONSTANTS
//////////////////////////////////////////////////////////////////////////////
#ifndef TRUE
#define TRUE                  1
#endif

#ifndef FALSE
#define FALSE                 0
#endif

//////////////////////////////////////////////////////////////////////////////
// MATH CONSTANTS
//////////////////////////////////////////////////////////////////////////////

/* Why not use M_PI and M_PI_2 that are provided by the math.h include above? */

#ifndef M_PI
#define M_PI       3.14159265358979323846f
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923f
#endif

#ifndef PI
#define PI M_PI
#endif

#ifndef PI_F
#define PI_F ((float) PI)
#endif

#ifndef PIDIV2
#define PIDIV2 M_PI_2
#endif

#ifndef PIDIV2_F
#define PIDIV2_F ((float) M_PI_2)
#endif

//////////////////////////////////////////////////////////////////////////////
// MAX / MIN VALUES
//////////////////////////////////////////////////////////////////////////////
// why not numeric_limits?!?! (DS)

#ifndef u8_MAX
#define u8_MAX ( (u8)(0xFF))
#endif

#ifndef u16_MAX
#define u16_MAX ( (u16)(0xFFFF) )
#endif

#ifndef u32_MAX
#define u32_MAX ( (u32)(0xFFFFFFFF) )
#endif

#ifndef u64_MAX
#define u64_MAX ( (u64)(0xFFFFFFFFFFFFFFFFLL) )
#endif

#ifndef s8_MIN
#define s8_MIN ( (s8)(-1 - 0x7F) )
#endif

#ifndef s8_MAX
#define s8_MAX ( (s8)(0x7F) )
#endif

#ifndef s16_MIN
#define s16_MIN ( (s16)(-1 - 0x7FFF) )
#endif

#ifndef s16_MAX
#define s16_MAX ( (s16)(0x7FFF) )
#endif

#ifndef s32_MIN
#define s32_MIN ( (s32)(-1 - 0x7FFFFFFF) )
#endif

#ifndef s32_MAX
#define s32_MAX ( (s32)(0x7FFFFFFF) )
#endif

#ifndef s64_MIN
#define s64_MIN ( (s64)(-1 - 0X7FFFFFFFFFFFFFFFLL) )
#endif

#ifndef s64_MAX
#define s64_MAX ( (s64)(0x7FFFFFFFFFFFFFFFLL) )
#endif

//////////////////////////////////////////////////////////////////////////////
// UNIT CONVERSION MACROS
//////////////////////////////////////////////////////////////////////////////

#define DEG_TO_RAD(deg) (((f64)deg)*0.017453292519943295474)
#define DEG_TO_RAD_F32(deg) (((f32)deg)*0.017453292519943295474f)

#define RAD_TO_DEG(rad) (((f64)rad)*57.295779513082322865)
#define RAD_TO_DEG_F32(rad) (((f32)rad)*57.295779513082322865f)

#define NANOS_TO_SEC(nanos) ((nanos) / 1000000000.0f)
#define SEC_TO_NANOS(sec) ((sec) * 1000000000.0f)

// Convert between millimeters and meters
#define M_TO_MM(x) ( ((double)(x)) * 1000.0 )
#define MM_TO_M(x) ( ((double)(x)) / 1000.0 )

//////////////////////////////////////////////////////////////////////////////
// COMPARISON MACROS
//////////////////////////////////////////////////////////////////////////////

// Tolerance for which two floating point numbers are considered equal (to deal
// with imprecision in floating point representation).
#define FLOATING_POINT_COMPARISON_TOLERANCE 1e-5f

// TRUE if x is near y by the amount epsilon, else FALSE
#define FLT_NEAR(x,y) ( ((x)==(y)) || (((x) > (y)-(FLOATING_POINT_COMPARISON_TOLERANCE))      && ((x) < (y)+(FLOATING_POINT_COMPARISON_TOLERANCE))) )
#define DBL_NEAR(x,y) ( ((x)==(y)) || (((x) > (y)-((f64)FLOATING_POINT_COMPARISON_TOLERANCE)) && ((x) < (y)+((f64)FLOATING_POINT_COMPARISON_TOLERANCE))) )

#ifndef NEAR
#define NEAR(x,y,epsilon) ((x) == (y) || (((x) > (y)-(epsilon)) && ((x) < (y)+(epsilon))))
#endif

// TRUE if x is within FLOATING_POINT_COMPARISON_TOLERANCE of 0.0
#define NEAR_ZERO(x) (NEAR(x, 0.0f, FLOATING_POINT_COMPARISON_TOLERANCE))

// TRUE if greater than the negative of the tolerance
#define FLT_GTR_ZERO(x) ((x) >= -FLOATING_POINT_COMPARISON_TOLERANCE)

// TRUE if x >= y - tolerance
#define FLT_GE(x,y) ((x) >= (y) || (((x) >= (y)-(FLOATING_POINT_COMPARISON_TOLERANCE))))

// TRUE if x - tolerance <= y
#define FLT_LE(x,y) ((x) >= (y) || (((x)-(FLOATING_POINT_COMPARISON_TOLERANCE) <= (y))))

// TRUE if val is within the range [minVal, maxVal], else FALSE
#define IN_RANGE(val,minVal,maxVal) ((val) >= (minVal) && (val) <= (maxVal))

///////////////////////////////////////////////////////////////////
// MATH MACROS
//////////////////////////////////////////////////////////////////

// Returns the closest value within [lo, hi] to val
#define CLIP(val,lo,hi) (MIN(MAX((val),(lo)),(hi)))

// Max, min of two numbers and absolute value
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(a)    (((a) >= 0) ? (a) : -(a))
#endif

// Square of a number
#define SQUARE(x) ((x) * (x))

///////////////////////////////////////////////////////////////////
// OTHER MACROS (e.g. for more complex code generation)
//////////////////////////////////////////////////////////////////

#define QUOTE_HELPER(__ARG__) #__ARG__
#define QUOTE(__ARG__) QUOTE_HELPER(__ARG__)

#endif // CORETECH_COMMON_CONSTANTS_AND_MACROS_H_
