/**
File: utilities.h
Author: Peter Barnum
Created: 2013

Definitions of utilities_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_

#include "anki/common/robot/utilities_declarations.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/robot/trig_fast.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark

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
      AnkiAssert(numTerms > 2);

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

    template<typename Type> Type Determinant2x2(const Type a, const Type b, const Type c, const Type d)
    {
      return a*d - b*c;
    }

    template<typename Type> Type Determinant3x3(const Type a, const Type b, const Type c, const Type d, const Type e, const Type f, const Type g, const Type h, const Type i)
    {
      return a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
    }

    template<typename Type> void Invert3x3(Type &a, Type &b, Type &c, Type &d, Type &e, Type &f, Type &g, Type &h, Type &i)
    {
      const Type determinant = Determinant3x3(a,b,c,d,e,f,g,h,i);
      const Type determinantInverse = static_cast<Type>(1) / determinant;

      const Type A =  (e*i - f*h);
      const Type B = -(d*i - f*g);
      const Type C =  (d*h - e*g);
      const Type D = -(b*i - c*h);
      const Type E =  (a*i - c*g);
      const Type F = -(a*h - b*g);
      const Type G =  (b*f - c*e);
      const Type H = -(a*f - c*d);
      const Type I =  (a*e - b*d);

      a = A * determinantInverse;
      b = D * determinantInverse;
      c = G * determinantInverse;
      d = B * determinantInverse;
      e = E * determinantInverse;
      f = H * determinantInverse;
      g = C * determinantInverse;
      h = F * determinantInverse;
      i = I * determinantInverse;
    }

    template<typename Type> void Cart2Pol(const Type x, const Type y, Type &rho, Type &theta)
    {
      theta = atan2_fast(y, x);
      rho = sqrtf(x*x + y*y);
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
