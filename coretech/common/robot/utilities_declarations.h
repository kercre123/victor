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

#include <stdlib.h>
#include "coretech/common/shared/types.h"
#include "coretech/common/robot/config.h"
#include "coretech/common/robot/utilities_c.h"

//#if ANKICORETECH_EMBEDDED_USE_OPENCV
//namespace cv {
//  class Mat;
//  template<typename Type> class Mat_;
//
//  template<typename Type> class Point_;
//  template<typename Type> class Point3_;
//
//  typedef Point_<int> Point2i;
//  typedef Point2i Point;
//
//  template<typename Type> class Scalar_;
//  typedef Scalar_<double> Scalar;
//}
//#endif

#include <opencv2/core.hpp>

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class Array;
    template<typename Type> class FixedLengthList;

    template<typename Type> inline Type RoundUp(const Type number, const Type multiple);

    // void* and size_t is a special case, good for aligning pointers
    const void* RoundUp(const void* number, const size_t multiple);
    void* RoundUp(void* number, const size_t multiple);

    template<typename Type> inline Type RoundDown(const Type number, const Type multiple);

    // Only s32, u32, f32, and f64 outputs are supported. If you need something else, also do a saturate_cast
    // Rounding to unsigned also saturates negative numbers to zero
    template<typename Type> inline Type Round(const f32 v);
    template<typename Type> inline Type Round(const f64 v);

    // Some templated functions should only round a result if it is converting from float to integer.
    // Using saturate_cast<> would work, but RoundIfInteger is faster if the value is never too big or small.
    // The int-to-int cases are here for templates with arbitrary InType and OutType
    template<typename Type> inline Type RoundIfInteger(const u8  v);
    template<typename Type> inline Type RoundIfInteger(const s8  v);
    template<typename Type> inline Type RoundIfInteger(const u16 v);
    template<typename Type> inline Type RoundIfInteger(const s16 v);
    template<typename Type> inline Type RoundIfInteger(const u32 v);
    template<typename Type> inline Type RoundIfInteger(const s32 v);
    template<typename Type> inline Type RoundIfInteger(const u64 v);
    template<typename Type> inline Type RoundIfInteger(const s64 v);
    template<typename Type> inline Type RoundIfInteger(const f32 v);
    template<typename Type> inline Type RoundIfInteger(const f64 v);

    // Taylor-series approximation of an exponential function
    template<typename Type> Type ApproximateExp(const Type exponent, const s32 numTerms = 10);

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
    template<typename Type> void Pol2Cart(const Type rho, const Type theta, Type &x, Type &y);

    // This saturate cast is similar to OpenCV 2.4.8, except:
    // 1. It saturates signed to unsigned without wrapping (OpenCV does not clip negative s32 ints, which makes -1 become 0xffffffff etc. I don't know why?)
    // 2. It includes some cases that were missing
    // 3. It doesn't use saturate_casts that call other saturate_casts, which helps less-sophisticated compilers
    //
    // WARNING: When rounding from very large floats to ints, the exact value may differ on the M4
    //          and PC. For some examples, see the unit test "CoreTech_Common RoundAndSaturate".
    template<typename Type> inline Type saturate_cast(const u8  v);
    template<typename Type> inline Type saturate_cast(const s8  v);
    template<typename Type> inline Type saturate_cast(const u16 v);
    template<typename Type> inline Type saturate_cast(const s16 v);
    template<typename Type> inline Type saturate_cast(const u32 v);
    template<typename Type> inline Type saturate_cast(const s32 v);
    template<typename Type> inline Type saturate_cast(const u64 v);
    template<typename Type> inline Type saturate_cast(const s64 v);
    template<typename Type> inline Type saturate_cast(const f32 v);
    template<typename Type> inline Type saturate_cast(const f64 v);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    // Converts from typeid names to openCV types
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth);

    // Calls cv::putText, but uses a fixed width font
    // NOTE: It recomputes the width only when you change the font type or size
    void CvPutTextFixedWidth(
      cv::Mat& img, const char * text, cv::Point org,
      int fontFace, double fontScale, cv::Scalar color,
      int thickness=1, int lineType=8,
      bool bottomLeftOrigin=false);

    void CvPutTextWithBackground(
      cv::Mat& img, const char * text, cv::Point org,
      int fontFace, double fontScale, cv::Scalar textColor, cv::Scalar backgroundColor,
      int thickness=1, int lineType=8,
      bool orgIsCenter=false);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

    // Returns the index of the first instance of bytePattern in buffer. If the pattern is not found, returns -1
    // Warning: bytePattern must not contain repeated bytes
    s32 FindBytePattern(const void * restrict buffer, const s32 bufferLength, const u8 * restrict bytePattern, const s32 bytePatternLength);

    // Returns a number [minLimit, maxLimit]
    s32 RandS32(const s32 minLimit, const s32 maxLimit);

    // Get the current system time in seconds
    // WARNING: f32 is enough for only about two hours (2*60*60*1000 ~= 2^23)
    f32 GetTimeF32(); // In seconds
    f64 GetTimeF64(); // In seconds
    u32 GetTimeU32(); // In microseconds

    // Returns the percentage of cpu usage for this process, since the last call (100% is for the total maximum of all cores, including virtual Hyperthreaded cores).
    // NOTE: The first call will return zero.
    // NOTE: Only works on systems with an operating system.
    // WARNING: The more often this function is called, the less accurate it will be. Once a second should be plenty.
    // WARNING: Not multi-thread safe (though it should give the correct answer when called from one thread at a time)
    f32 GetCpuUsage();
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_DECLARATIONS_H_
