/**
* File: trig_fast.h
*
* Author: Kevin Yoon
* Created: 22-OCT-2012
*
* Some trig functions to supplement incomplete math libraries on embedded targets.
* Error of all functions is less than +/- 0.01.
* For bettery accuracy, lookup tables should be regenerated with u16.
*
**/
#ifndef _TRIG_FAST_H
#define _TRIG_FAST_H

// When USE_SMALL_LUT defined, a smaller lookup table is used to conserve space.
// USE_INTERPOLATION is also automatically defined, since without it answers are probably too wrong to be useful.
// If USE_SMALL_LUT is not defined, a large LUT is used.
//#define USE_SMALL_LUT

// When defined, interpolates between lookup values for higher accuracy.
#define USE_INTERPOLATION

// Arctangent function based on lookup table
// returns answer in radians
float atan_fast(float x);

// Arcsine function based on lookup table
// returns answer in radians
float asin_fast(float x);

// Arctangent function which uses atan_fast
// returns answer in radians
float atan2_fast(float y, float x);

// Arctangent function which uses asin from math.h
// Useful on embedded systems that don't include atan2 in math.h
// More accurate than atan2_fast.
// Nothing particularly fast about this implementation.
// returns answer in radians
float atan2_acc(float y, float x);

#endif
