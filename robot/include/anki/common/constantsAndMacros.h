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

#ifndef SHARED_CONSTANTS_AND_MACROS_H_
#define SHARED_CONSTANTS_AND_MACROS_H_

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

#ifndef M_PI
#define M_PI       3.14159265358979323846264338327950288
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923132169163975144
#endif

#ifndef M_PI_F
#define M_PI_F     ((float)M_PI)
#endif

#ifndef M_PI_2_F
#define M_PI_2_F   ((float)M_PI_2)
#endif

//////////////////////////////////////////////////////////////////////////////
// UNIT CONVERSION MACROS
//////////////////////////////////////////////////////////////////////////////

#ifndef DEG_TO_RAD
#define DEG_TO_RAD(deg) (((double)(deg))*0.017453292519943295474)
#endif
#ifndef DEG_TO_RAD_F32
#define DEG_TO_RAD_F32(deg) (((float)(deg))*0.017453292519943295474f)
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG(rad) (((double)(rad))*57.295779513082322865)
#endif
#define RAD_TO_DEG_F32(rad) (((float)(rad))*57.295779513082322865f)

#define NANOS_TO_SEC(nanos) ((nanos) / 1000000000.0f)
#define SEC_TO_NANOS(sec) ((sec) * 1000000000.0f)

// Convert between millimeters and meters
#ifndef M_TO_MM
#define M_TO_MM(x) ( ((double)(x)) * 1000.0 )
#endif
#ifndef MM_TO_M
#define MM_TO_M(x) ( ((double)(x)) / 1000.0 )
#endif

//////////////////////////////////////////////////////////////////////////////
// COMPARISON MACROS
//////////////////////////////////////////////////////////////////////////////

// Tolerance for which two floating point numbers are considered equal (to deal
// with imprecision in floating point representation).
#define FLOATING_POINT_COMPARISON_TOLERANCE 1e-5f

// TRUE if x is near y by the amount epsilon, else FALSE
#ifndef FLT_NEAR
#define FLT_NEAR(x,y) ( ((x)==(y)) || (((x) > (y)-(FLOATING_POINT_COMPARISON_TOLERANCE))      && ((x) < (y)+(FLOATING_POINT_COMPARISON_TOLERANCE))) )
#endif
#ifndef DBL_NEAR
#define DBL_NEAR(x,y) ( ((x)==(y)) || (((x) > (y)-((double)FLOATING_POINT_COMPARISON_TOLERANCE)) && ((x) < (y)+((double)FLOATING_POINT_COMPARISON_TOLERANCE))) )
#endif

#ifndef NEAR
#define NEAR(x,y,epsilon) ((x) == (y) || (((x) > (y)-(epsilon)) && ((x) < (y)+(epsilon))))
#endif

// TRUE if x is within FLOATING_POINT_COMPARISON_TOLERANCE of 0.0
#ifndef NEAR_ZERO
#define NEAR_ZERO(x) (NEAR(x, 0.0f, FLOATING_POINT_COMPARISON_TOLERANCE))
#endif

// TRUE if greater than the negative of the tolerance
#ifndef FLT_GTR_ZERO
#define FLT_GTR_ZERO(x) ((x) >= -FLOATING_POINT_COMPARISON_TOLERANCE)
#endif

// TRUE if x >= y - tolerance
#ifndef FLT_GE
#define FLT_GE(x,y) ((x) >= (y) || (((x) >= (y)-(FLOATING_POINT_COMPARISON_TOLERANCE))))
#endif

// TRUE if x - tolerance <= y
#ifndef FLT_LE
#define FLT_LE(x,y) ((x) >= (y) || (((x)-(FLOATING_POINT_COMPARISON_TOLERANCE) <= (y))))
#endif

// TRUE if val is within the range [minVal, maxVal], else FALSE
#ifndef IN_RANGE
#define IN_RANGE(val,minVal,maxVal) ((val) >= (minVal) && (val) <= (maxVal))
#endif

///////////////////////////////////////////////////////////////////
// MATH MACROS
//////////////////////////////////////////////////////////////////

// Returns the closest value within [lo, hi] to val
#ifndef CLIP
#define CLIP(val,lo,hi) (MIN(MAX((val),(lo)),(hi)))
#endif

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
#ifndef SQUARE
#define SQUARE(x) ((x) * (x))
#endif

///////////////////////////////////////////////////////////////////
// OTHER MACROS (e.g. for more complex code generation)
//////////////////////////////////////////////////////////////////

#define QUOTE_HELPER(__ARG__) #__ARG__
#define QUOTE(__ARG__) QUOTE_HELPER(__ARG__)

#endif // SHARED_CONSTANTS_AND_MACROS_H_
