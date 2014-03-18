/**
File: utilities_declarations.h
Author: Peter Barnum
Created: 2013

Various simple macros and utilities.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class Array;
    template<typename Type> class FixedLengthList;

    template<typename Type> inline Type RoundUp(Type number, Type multiple);

    template<typename Type> inline Type RoundDown(Type number, Type multiple);

    // Taylor-series approximation of an exponential function
    template<typename Type> Type approximateExp(const Type exponent, const s32 numTerms = 10);

    // Swap a with b
    template<typename Type> void Swap(Type &a, Type &b);

    template<typename Type> u32 BinaryStringToUnsignedNumber(const FixedLengthList<Type> &bits, bool firstBitIsLow = false);

    // Simple matrix operations
    // |a b|
    // |c d|
    // return a*d - b*c;
    template<typename Type> Type Determinant2x2(const Type a, const Type b, const Type c, const Type d);

    // |a b c|
    // |d e f|
    // |g h i|
    // return (aei + bfg + cfh) - (ceg + bdi + afh)
    template<typename Type> Type Determinant3x3(const Type a, const Type b, const Type c, const Type d, const Type e, const Type f, const Type g, const Type h, const Type i);

    // Invert:
    // [a b c]
    // [d e f]
    // [g h i]
    template<typename Type> void Invert3x3(Type &a, Type &b, Type &c, Type &d, Type &e, Type &f, Type &g, Type &h, Type &i);

    // Cartesian coordinates to Polar coordinates
    template<typename Type> void Cart2Pol(const Type x, const Type y, Type &rho, Type &theta);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    // Converts from typeid names to openCV types
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

    // Returns the index of the first instance of bytePattern in buffer. If the pattern is not found, returns -1
    // Warning: bytePattern must not contain repeated bytes
    s32 FindBytePattern(const void * restrict buffer, const s32 bufferLength, const u8 * restrict bytePattern, const s32 bytePatternLength);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_DECLARATIONS_H_
