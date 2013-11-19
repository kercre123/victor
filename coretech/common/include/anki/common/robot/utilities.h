/**
File: utilities.h
Author: Peter Barnum
Created: 2013

Various simple macros and utilities.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_

#include "anki/common/robot/utilities_declarations.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---

    template<typename Type> inline Type RoundUp(Type number, Type multiple)
    {
      return multiple*( (number-1)/multiple + 1 );

      // For unsigned only?
      // return (number + (multiple-1)) & ~(multiple-1);
    }

    template<typename Type> inline Type RoundDown(Type number, Type multiple)
    {
      return multiple * (number/multiple);
    }

    template<typename Type> Type approximateExp(const Type exponent, const s32 numTerms)
    {
      assert(numTerms > 2);

      const Type exponentAbs = ABS(exponent);

      Type sum = static_cast<Type>(1) + exponentAbs;

      Type numerator = static_cast<Type>(exponentAbs);
      Type denominator = static_cast<Type>(1);
      for(s32 i=2; i<=numTerms; i++) {
        numerator *= exponentAbs;
        denominator *= i;

        sum += numerator / denominator;
      }

      if(exponent < 0) {
        sum = static_cast<Type>(1) / sum;
      }

      return sum;
    }

    template<typename Type> void Swap(Type &a, Type &b)
    {
      const Type tmp = a;
      a = b;
      b = tmp;
    } // template<typename Type> Swap(Type a, Type b)

    template<typename Type> u32 BinaryStringToUnsignedNumber(const FixedLengthList<Type> &bits, bool firstBitIsLow)
    {
      u32 number = 0;

      const s32 numBits = bits.get_size();

      for(s32 bit=0; bit<numBits; bit++) {
        if(firstBitIsLow) {
          if(bit == 0) {
            number += bits[bit];
          } else {
            number += bits[bit] << bit;
          }
        } else {
          if(bit == (numBits-1)) {
            number += bits[bit];
          } else {
            number += bits[bit] << (numBits - bit - 1);
          }
        }
      }

      return number;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
