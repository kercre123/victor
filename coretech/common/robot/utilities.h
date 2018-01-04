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

#include <math.h>
#include <float.h>
#include "coretech/common/shared/types.h"
#include "coretech/common/robot/utilities_declarations.h"
#include "coretech/common/robot/utilities_c.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
    //template<typename Type> inline Type RoundUp(const Type number, const Type multiple)

    // void* and size_t is a special case, good for aligning pointers
    inline const void* RoundUp(const void* number, const size_t multiple)
    {
      const size_t numberT = reinterpret_cast<size_t>(number);
      return reinterpret_cast<void*>( (numberT + (multiple-1)) & ~(multiple-1) );
    }

    inline void* RoundUp(void* number, const size_t multiple)
    {
      const size_t numberT = reinterpret_cast<size_t>(number);
      return reinterpret_cast<void*>( (numberT + (multiple-1)) & ~(multiple-1) );
    }

    template<> inline u32 RoundUp(const u32 number, const u32 multiple)
    {
      return (number + (multiple-1)) & ~(multiple-1);
    }

    template<> inline s32 RoundUp(const s32 number, const s32 multiple)
    {
      if(number <= 0) {
        return multiple*( number/multiple );
      } else {
        return multiple*( (number-1)/multiple + 1 );
      }
    }

#if defined(__APPLE_CC__) || defined(__GNUC__)
    template<> inline unsigned long RoundUp(const unsigned long number, const unsigned long multiple)
    {
      return (number + (multiple-1)) & ~(multiple-1);
    }
#endif

    template<> inline u32 RoundDown(const u32 number, const u32 multiple)
    {
      return multiple * (number/multiple);
    }

    template<> inline s32 RoundDown(const s32 number, const s32 multiple)
    {
      if(number < 0) {
        return multiple * ((number-multiple+1) / multiple);
      } else {
        return multiple * (number/multiple);
      }
    }

#if defined(__APPLE_CC__) || defined(__GNUC__)
    template<> inline unsigned long RoundDown(const unsigned long number, const unsigned long multiple)
    {
      return multiple * (number/multiple);
    }
#endif

    template<typename Type> Type ApproximateExp(const Type exponent, const s32 numTerms)
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
      if (x==0 && y==0) {
        theta = 0;
        rho = 0;
      } else {
        theta = atan2f(y, x);
        rho = sqrtf(x*x + y*y);
      }
    }

    template<typename Type> void Pol2Cart(const Type rho, const Type theta, Type &x, Type &y)
    {
      x = rho * cosf(theta);
      y = rho * sinf(theta);
    }

    inline s32 FloorS32(f32 x)
    {
      return static_cast<s32>(floorf(x));
    }

    inline s32 CeilS32(f32 x)
    {
      return static_cast<s32>(ceilf(x));
    }

#if !defined(__EDG__)
    // Some platforms may not round to zero correctly, so do the function calls
    template<> inline u32 Round<u32> (const f32 v) { return (v > 0) ? static_cast<u32>(floorf(v + 0.5f)) : 0; }
    template<> inline u64 Round<u64> (const f32 v) { return (v > 0) ? static_cast<u64>(floorf(v + 0.5f)) : 0; }
    template<> inline s32 Round<s32> (const f32 v) { return (v > 0) ? static_cast<s32>(floorf(v + 0.5f)) : static_cast<s32>(ceilf(v - 0.5f)); }
    template<> inline s64 Round<s64> (const f32 v) { return (v > 0) ? static_cast<s64>(floorf(v + 0.5f)) : static_cast<s64>(ceilf(v - 0.5f)); }
    template<> inline f32 Round<f32> (const f32 v) { return (v > 0) ? floorf(v + 0.5f) : ceilf(v - 0.5f); }
    template<> inline f64 Round<f64> (const f32 v) { return (v > 0) ? floorf(v + 0.5f) : ceilf(v - 0.5f); }

    template<> inline u32 Round<u32> (const f64 v) { return (v > 0) ? static_cast<u32>(floor(v + 0.5)) : 0; }
    template<> inline u64 Round<u64> (const f64 v) { return (v > 0) ? static_cast<u64>(floor(v + 0.5)) : 0; }
    template<> inline s32 Round<s32> (const f64 v) { return (v > 0) ? static_cast<s32>(floor(v + 0.5)) : static_cast<s32>(ceil(v - 0.5)); }
    template<> inline s64 Round<s64> (const f64 v) { return (v > 0) ? static_cast<s64>(floor(v + 0.5)) : static_cast<s64>(ceil(v - 0.5)); }
    template<> inline f32 Round<f32> (const f64 v) { return (v > 0) ? static_cast<f32>(floor(v + 0.5)) : static_cast<f32>(ceil(v - 0.5)); }
    template<> inline f64 Round<f64> (const f64 v) { return (v > 0) ? floor(v + 0.5) : ceil(v - 0.5); }
#else
    // The M4 rounds to zero correctly, without the function calls
    template<> inline u32 Round<u32> (const f32 v) { return (v > 0) ? static_cast<u32>(v + 0.5f) : 0; }
    template<> inline u64 Round<u64> (const f32 v) { return (v > 0) ? static_cast<u64>(v + 0.5f) : 0; }
    template<> inline s32 Round<s32> (const f32 v) { return (v > 0) ? static_cast<s32>(v + 0.5f) : static_cast<s32>(v - 0.5f); }
    template<> inline s64 Round<s64> (const f32 v) { return (v > 0) ? static_cast<s64>(v + 0.5f) : static_cast<s64>(v - 0.5f); }
    template<> inline f32 Round<f32> (const f32 v) { return (v > 0) ? floorf(v + 0.5f) : ceilf(v - 0.5f); }
    template<> inline f64 Round<f64> (const f32 v) { return (v > 0) ? floorf(v + 0.5f) : ceilf(v - 0.5f); }

    template<> inline u32 Round<u32> (const f64 v) { return (v > 0) ? static_cast<u32>(v + 0.5) : 0; }
    template<> inline u64 Round<u64> (const f64 v) { return (v > 0) ? static_cast<u64>(v + 0.5) : 0; }
    template<> inline s32 Round<s32> (const f64 v) { return (v > 0) ? static_cast<s32>(v + 0.5) : static_cast<s32>(v - 0.5); }
    template<> inline s64 Round<s64> (const f64 v) { return (v > 0) ? static_cast<s64>(v + 0.5) : static_cast<s64>(v - 0.5); }
    template<> inline f32 Round<f32> (const f64 v) { return (v > 0) ? static_cast<f32>(floor(v + 0.5)) : static_cast<f32>(ceil(v - 0.5)); }
    template<> inline f64 Round<f64> (const f64 v) { return (v > 0) ? floor(v + 0.5) : ceil(v - 0.5); }
#endif

    // Most cases of RoundIfInteger are from int-to-int or float-to-float, so just do a normal cast
    template<typename Type> inline Type RoundIfInteger(const u8  v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const s8  v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const u16 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const s16 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const u32 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const s32 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const u64 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const s64 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const f32 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type RoundIfInteger(const f64 v) { return static_cast<Type>(v); }

    // Specialize for cases with float-to-int
    template<> inline u8  RoundIfInteger(const f32 v) { return static_cast<u8> (Round<s32>(v)); }
    template<> inline s8  RoundIfInteger(const f32 v) { return static_cast<s8> (Round<s32>(v)); }
    template<> inline u16 RoundIfInteger(const f32 v) { return static_cast<u16>(Round<s32>(v)); }
    template<> inline s16 RoundIfInteger(const f32 v) { return static_cast<s16>(Round<s32>(v)); }
    template<> inline u32 RoundIfInteger(const f32 v) { return static_cast<u32>(Round<u32>(v)); }
    template<> inline s32 RoundIfInteger(const f32 v) { return static_cast<s32>(Round<s32>(v)); }
    template<> inline u64 RoundIfInteger(const f32 v) { return static_cast<u64>(Round<u64>(v)); }
    template<> inline s64 RoundIfInteger(const f32 v) { return static_cast<s64>(Round<s64>(v)); }

    template<> inline u8  RoundIfInteger(const f64 v) { return static_cast<u8> (Round<s32>(v)); }
    template<> inline s8  RoundIfInteger(const f64 v) { return static_cast<s8> (Round<s32>(v)); }
    template<> inline u16 RoundIfInteger(const f64 v) { return static_cast<u16>(Round<s32>(v)); }
    template<> inline s16 RoundIfInteger(const f64 v) { return static_cast<s16>(Round<s32>(v)); }
    template<> inline u32 RoundIfInteger(const f64 v) { return static_cast<u32>(Round<u32>(v)); }
    template<> inline s32 RoundIfInteger(const f64 v) { return static_cast<s32>(Round<s32>(v)); }
    template<> inline u64 RoundIfInteger(const f64 v) { return static_cast<u64>(Round<u64>(v)); }
    template<> inline s64 RoundIfInteger(const f64 v) { return static_cast<s64>(Round<s64>(v)); }

    // Floats and complex data types aren't specialized
    template<typename Type> inline Type saturate_cast(const u8  v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const s8  v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const u16 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const s16 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const u32 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const s32 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const u64 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const s64 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const f32 v) { return static_cast<Type>(v); }
    template<typename Type> inline Type saturate_cast(const f64 v) { return static_cast<Type>(v); }

    // Most saturate_cast calls are explicitly specialized
    template<> inline u8  saturate_cast<u8> (const u8  v) { return v; }
    template<> inline u8  saturate_cast<u8> (const u16 v) { return (u8)             MIN((u32)std::numeric_limits<u8>::max(), (u32)v); }
    template<> inline u8  saturate_cast<u8> (const u32 v) { return (u8)             MIN((u32)std::numeric_limits<u8>::max(), (u32)v); }
    template<> inline u8  saturate_cast<u8> (const u64 v) { return (u8)             MIN((u64)std::numeric_limits<u8>::max(), (u64)v); }
    template<> inline u8  saturate_cast<u8> (const s8  v) { return (u8)                              MAX((s32)0, (s32)v);  }
    template<> inline u8  saturate_cast<u8> (const s16 v) { return (u8)             MIN((s32)std::numeric_limits<u8>::max(), MAX((s32)0, (s32)v)); }
    template<> inline u8  saturate_cast<u8> (const s32 v) { return (u8)             MIN((s32)std::numeric_limits<u8>::max(), MAX((s32)0, (s32)v)); }
    template<> inline u8  saturate_cast<u8> (const s64 v) { return (u8)             MIN((s64)std::numeric_limits<u8>::max(), MAX((s64)0, (s64)v)); }
    template<> inline u8  saturate_cast<u8> (const f32 v) { return (u8) Round<s32>( MIN((f32)std::numeric_limits<u8>::max(), MAX((f32)0, (f32)v)) ); }
    template<> inline u8  saturate_cast<u8> (const f64 v) { return (u8) Round<s32>( MIN((f64)std::numeric_limits<u8>::max(), MAX((f64)0, (f64)v)) ); }

    template<> inline s8  saturate_cast<s8> (const u8  v) { return (s8)             MIN((u32)std::numeric_limits<s8>::max(), (u32)v); }
    template<> inline s8  saturate_cast<s8> (const u16 v) { return (s8)             MIN((u32)std::numeric_limits<s8>::max(), (u32)v); }
    template<> inline s8  saturate_cast<s8> (const u32 v) { return (s8)             MIN((u32)std::numeric_limits<s8>::max(), (u32)v); }
    template<> inline s8  saturate_cast<s8> (const u64 v) { return (s8)             MIN((u64)std::numeric_limits<s8>::max(), (u64)v); }
    template<> inline s8  saturate_cast<s8> (const s8  v) { return v; }
    template<> inline s8  saturate_cast<s8> (const s16 v) { return (s8)             MIN((s32)std::numeric_limits<s8>::max(), MAX((s32)std::numeric_limits<s8>::min(), (s32)v)); }
    template<> inline s8  saturate_cast<s8> (const s32 v) { return (s8)             MIN((s32)std::numeric_limits<s8>::max(), MAX((s32)std::numeric_limits<s8>::min(), (s32)v)); }
    template<> inline s8  saturate_cast<s8> (const s64 v) { return (s8)             MIN((s64)std::numeric_limits<s8>::max(), MAX((s64)std::numeric_limits<s8>::min(), (s64)v)); }
    template<> inline s8  saturate_cast<s8> (const f32 v) { return (s8) Round<s32>( MIN((f32)std::numeric_limits<s8>::max(), MAX((f32)std::numeric_limits<s8>::min(), (f32)v)) ); }
    template<> inline s8  saturate_cast<s8> (const f64 v) { return (s8) Round<s32>( MIN((f64)std::numeric_limits<s8>::max(), MAX((f64)std::numeric_limits<s8>::min(), (f64)v)) ); }

    template<> inline u16 saturate_cast<u16>(const u8  v) { return v; }
    template<> inline u16 saturate_cast<u16>(const u16 v) { return v; }
    template<> inline u16 saturate_cast<u16>(const u32 v) { return (u16)             MIN((u32)std::numeric_limits<u16>::max(), (u32)v); }
    template<> inline u16 saturate_cast<u16>(const u64 v) { return (u16)             MIN((u64)std::numeric_limits<u16>::max(), (u64)v); }
    template<> inline u16 saturate_cast<u16>(const s8  v) { return (u16)                               MAX((s32)0, (s32)v);  }
    template<> inline u16 saturate_cast<u16>(const s16 v) { return (u16)                               MAX((s32)0, (s32)v);  }
    template<> inline u16 saturate_cast<u16>(const s32 v) { return (u16)             MIN((s32)std::numeric_limits<u16>::max(), MAX((s32)0, (s32)v)); }
    template<> inline u16 saturate_cast<u16>(const s64 v) { return (u16)             MIN((s64)std::numeric_limits<u16>::max(), MAX((s64)0, (s64)v)); }
    template<> inline u16 saturate_cast<u16>(const f32 v) { return (u16) Round<s32>( MIN((f32)std::numeric_limits<u16>::max(), MAX((f32)0, (f32)v)) ); }
    template<> inline u16 saturate_cast<u16>(const f64 v) { return (u16) Round<s32>( MIN((f64)std::numeric_limits<u16>::max(), MAX((f64)0, (f64)v)) ); }

    template<> inline s16 saturate_cast<s16>(const u8  v) { return v; }
    template<> inline s16 saturate_cast<s16>(const u16 v) { return (s16)             MIN((u32)std::numeric_limits<s16>::max(), (u32)v); }
    template<> inline s16 saturate_cast<s16>(const u32 v) { return (s16)             MIN((u32)std::numeric_limits<s16>::max(), (u32)v); }
    template<> inline s16 saturate_cast<s16>(const u64 v) { return (s16)             MIN((u64)std::numeric_limits<s16>::max(), (u64)v); }
    template<> inline s16 saturate_cast<s16>(const s8  v) { return v; }
    template<> inline s16 saturate_cast<s16>(const s16 v) { return v; }
    template<> inline s16 saturate_cast<s16>(const s32 v) { return (s16)             MIN((s32)std::numeric_limits<s16>::max(), MAX((s32)std::numeric_limits<s16>::min(), (s32)v)); }
    template<> inline s16 saturate_cast<s16>(const s64 v) { return (s16)             MIN((s64)std::numeric_limits<s16>::max(), MAX((s64)std::numeric_limits<s16>::min(), (s64)v)); }
    template<> inline s16 saturate_cast<s16>(const f32 v) { return (s16) Round<s32>( MIN((f32)std::numeric_limits<s16>::max(), MAX((f32)std::numeric_limits<s16>::min(), (f32)v)) ); }
    template<> inline s16 saturate_cast<s16>(const f64 v) { return (s16) Round<s32>( MIN((f64)std::numeric_limits<s16>::max(), MAX((f64)std::numeric_limits<s16>::min(), (f64)v)) ); }

    template<> inline u32 saturate_cast<u32>(const u8  v) { return v; }
    template<> inline u32 saturate_cast<u32>(const u16 v) { return v; }
    template<> inline u32 saturate_cast<u32>(const u32 v) { return v; }
    template<> inline u32 saturate_cast<u32>(const u64 v) { return (u32)             MIN((u64)std::numeric_limits<u32>::max(), (u64)v); }
    template<> inline u32 saturate_cast<u32>(const s8  v) { return (u32)                               MAX((s32)0, (s32)v);  }
    template<> inline u32 saturate_cast<u32>(const s16 v) { return (u32)                               MAX((s32)0, (s32)v);  }
    template<> inline u32 saturate_cast<u32>(const s32 v) { return (u32)                               MAX((s32)0, (s32)v);  }
    template<> inline u32 saturate_cast<u32>(const s64 v) { return (u32)             MIN((s64)std::numeric_limits<u32>::max(), MAX((s64)0, (s64)v)); }
    template<> inline u32 saturate_cast<u32>(const f32 v) { return (u32) (v > (f32)0xFFFFFF7F) ? 0xFFFFFFFF : Round<u32>(MAX((f32)0, (f32)v)); } // Due to precision issues, this cast is a little wierd
    template<> inline u32 saturate_cast<u32>(const f64 v) { return (u32) Round<u32>( MIN((f64)std::numeric_limits<u32>::max(), MAX((f64)0, (f64)v)) ); }

    template<> inline s32 saturate_cast<s32>(const u8  v) { return v; }
    template<> inline s32 saturate_cast<s32>(const u16 v) { return v; }
    template<> inline s32 saturate_cast<s32>(const u32 v) { return (s32)             MIN((u32)std::numeric_limits<s32>::max(), (u32)v); }
    template<> inline s32 saturate_cast<s32>(const u64 v) { return (s32)             MIN((u64)std::numeric_limits<s32>::max(), (u64)v); }
    template<> inline s32 saturate_cast<s32>(const s8  v) { return v; }
    template<> inline s32 saturate_cast<s32>(const s16 v) { return v; }
    template<> inline s32 saturate_cast<s32>(const s32 v) { return v; }
    template<> inline s32 saturate_cast<s32>(const s64 v) { return (s32)             MIN((s64)std::numeric_limits<s32>::max(), MAX((s64)std::numeric_limits<s32>::min(), (s64)v)); }
    template<> inline s32 saturate_cast<s32>(const f32 v) { return (s32) (v > (f32)0x7FFFFFBF) ? 0x7FFFFFFF : Round<s32>(MAX((f32)std::numeric_limits<s32>::min(), (f32)v)); } // Due to precision issues, this cast is a little wierd
    template<> inline s32 saturate_cast<s32>(const f64 v) { return (s32) Round<s32>( MIN((f64)std::numeric_limits<s32>::max(), MAX((f64)std::numeric_limits<s32>::min(), (f64)v)) ); }

    template<> inline u64 saturate_cast<u64>(const u8  v) { return v; }
    template<> inline u64 saturate_cast<u64>(const u16 v) { return v; }
    template<> inline u64 saturate_cast<u64>(const u32 v) { return v; }
    template<> inline u64 saturate_cast<u64>(const u64 v) { return v; }
    template<> inline u64 saturate_cast<u64>(const s8  v) { return (u64)                               MAX((s32)0, (s32)v);  }
    template<> inline u64 saturate_cast<u64>(const s16 v) { return (u64)                               MAX((s32)0, (s32)v);  }
    template<> inline u64 saturate_cast<u64>(const s32 v) { return (u64)                               MAX((s32)0, (s32)v);  }
    template<> inline u64 saturate_cast<u64>(const s64 v) { return (u64)                               MAX((s64)0, (s64)v);  }
    template<> inline u64 saturate_cast<u64>(const f32 v) { return (u64) (v > (f32)0XFFFFFF7FFFFFFBFFULL) ? 0xFFFFFFFFFFFFFFFFULL : Round<u64>(MAX((f32)0, (f32)v)); } // Due to precision issues, this cast is a little wierd
    template<> inline u64 saturate_cast<u64>(const f64 v) { return (u64) (v > (f64)0xFFFFFFFFFFFFFBFFULL) ? 0xFFFFFFFFFFFFFFFFULL : Round<u64>(MAX((f64)0, (f64)v)); } // Due to precision issues, this cast is a little wierd

    template<> inline s64 saturate_cast<s64>(const u8  v) { return v; }
    template<> inline s64 saturate_cast<s64>(const u16 v) { return v; }
    template<> inline s64 saturate_cast<s64>(const u32 v) { return v; }
    template<> inline s64 saturate_cast<s64>(const u64 v) { return (s64)             MIN((u64)std::numeric_limits<s64>::max(), (u64)v); }
    template<> inline s64 saturate_cast<s64>(const s8  v) { return v; }
    template<> inline s64 saturate_cast<s64>(const s16 v) { return v; }
    template<> inline s64 saturate_cast<s64>(const s32 v) { return v; }
    template<> inline s64 saturate_cast<s64>(const s64 v) { return v; }
    template<> inline s64 saturate_cast<s64>(const f32 v) { return (s64) (v > (f32)0x7FFFFFBFFFFFFDFFLL) ? 0x7FFFFFFFFFFFFFFFLL : Round<s64>(MAX((f32)std::numeric_limits<s64>::min(), (f32)v)); } // Due to precision issues, this cast is a little wierd
    template<> inline s64 saturate_cast<s64>(const f64 v) { return (s64) (v > (f64)0x7FFFFFFFFFFFFDFFLL) ? 0x7FFFFFFFFFFFFFFFLL : Round<s64>(MAX((f64)std::numeric_limits<s64>::min(), (f64)v)); } // Due to precision issues, this cast is a little wierd

    template<> inline f32 saturate_cast(const f64 v) { return (f32) MIN((f64)std::numeric_limits<f32>::max(), MAX(-(f64)std::numeric_limits<f32>::max(), (f64)v)); }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
