/**
File: utilities_declarations.h
Author: Peter Barnum
Created: 2013

Declarations of utilities.h

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

    template<typename Type> Type approximateExp(const Type exponent, const s32 numTerms = 10);

    // Swap a with b
    template<typename Type> void Swap(Type &a, Type &b);

    template<typename Type> u32 BinaryStringToUnsignedNumber(const FixedLengthList<Type> &bits, bool firstBitIsLow = false);

    // Movidius doesn't have floating point printf (no %f option), so do it with %d
    void PrintfOneArray_f32(const Array<f32> &array, const char * variableName);
    void PrintfOneArray_f64(const Array<f64> &array, const char * variableName);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    // Converts from typeid names to openCV types
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_DECLARATIONS_H_
