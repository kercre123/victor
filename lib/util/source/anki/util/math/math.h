/**
 * File: math
 *
 * Author: damjan
 * Created: 5/6/14
 *
 * Description: helper math functions
 *
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_MATH_H_
#define UTIL_MATH_H_

#include <limits>
#include <assert.h>
#include <math.h>

#include "util/global/globalDefinitions.h"
#include "util/math/numericCast.h" // it used to be part of this file, leaving included until client code is tidied

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////

#ifndef M_PI
#define M_PI       3.14159265358979323846264338327950288
#endif

#ifndef M_TWO_PI
#define M_TWO_PI   (M_PI*2.0)
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923132169163975144
#endif

#ifndef M_PI_4
#define M_PI_4     0.785398163397448309615660845819875721
#endif

#ifndef M_PI_F
#define M_PI_F     ((float)M_PI)
#endif

#ifndef M_TWO_PI_F
#define M_TWO_PI_F ((float)M_PI*2.0f)
#endif

#ifndef M_PI_2_F
#define M_PI_2_F   ((float)M_PI_2)
#endif

#ifndef M_PI_4_F
#define M_PI_4_F   ((float)M_PI_4)
#endif

namespace Anki {
namespace Util {

// convert angle to range [0, 2PI)
inline float ClampAngle2PI(float radians) {
  radians = fmodf(radians, 2*M_PI_F);
  radians = (radians < 0) ? (2*M_PI_F+radians) : radians;
  return radians;
}

// convert angle to range [0, 2PI)
inline double ClampAngle2PI(double radians) {
  radians = fmod(radians, 2*M_PI_F);
  radians = (radians < 0) ? (2*M_PI_F+radians) : radians;
  return radians;
}

//////////////////////////////////////////////////////////////////////////////
// COMPARISON MACROS
//////////////////////////////////////////////////////////////////////////////


// Tolerance for which two floating point numbers are considered equal (to deal
// with imprecision in floating point representation).

#undef FLOATING_POINT_COMPARISON_TOLERANCE
constexpr double FLOATING_POINT_COMPARISON_TOLERANCE = 1e-5;
constexpr float  FLOATING_POINT_COMPARISON_TOLERANCE_FLT = (float)FLOATING_POINT_COMPARISON_TOLERANCE;
  
// TODO (COZMO-7841): Why not use std::abs()?
template<typename T>
inline constexpr T Abs(const T& inVal)
{
  return (inVal < T(0)) ? -inVal : inVal;
}
  
// IsFltNear implementations
// TRUE if x is near y by FLOATING_POINT_COMPARISON_TOLERANCE, else FALSE

inline constexpr bool IsFltNear(float x, float y)
{
  return (Anki::Util::Abs(x-y) < Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
}

inline constexpr bool IsFltNear(double x, double y)
{
  return (Anki::Util::Abs(x-y) < Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE);
}

inline constexpr bool IsFltNear(float x, double y)
{
  return IsFltNear((double)x, y);
}

inline constexpr bool IsFltNear(double x, float y)
{
  return IsFltNear(x, (double)y);
}

// IsNear implementations
// TRUE if x is within epsilon tolerance of y
  
inline constexpr bool IsNear(float x, float y, float epsilon=Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT)
{
  return (Anki::Util::Abs(x-y) < epsilon);
}

inline constexpr bool IsNear(double x, double y, double epsilon)
{
  return (Anki::Util::Abs(x-y) < epsilon);
}
  
// IsNearZero implementations
// TRUE if x is within FLOATING_POINT_COMPARISON_TOLERANCE of 0.0
  
inline constexpr bool IsNearZero(float x)
{
  return IsNear(x, 0.0f, Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
}

inline constexpr bool IsNearZero(double x)
{
  return IsNear(x, 0.0, Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE);
}

// IsFltGTZero implementations
// TRUE if greater than the tolerance
  
inline constexpr bool IsFltGTZero(float x)
{
  return (x > Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
}
 
inline constexpr bool IsFltGTZero(double x)
{
  return (x > Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE);
}
  
// IsFltGEZero implementations
// TRUE if greater or equal than the negative of the tolerance
  
inline constexpr bool IsFltGEZero(float x)
{
  return (x >= -Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
}
 
inline constexpr bool IsFltGEZero(double x)
{
  return (x >= -Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE);
}

// IsFltGE implementations
  
inline constexpr bool IsFltGE(float x, float y)
{
  return (x >= (y - Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT));
}

inline constexpr bool IsFltGE(double x, double y)
{
  return (x >= (y - Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE));
}

inline constexpr bool IsFltGE(float x, double y)
{
  return IsFltGE((double)x, y);
}
  
inline constexpr bool IsFltGE(double x, float y)
{
  return IsFltGE(x, (double)y);
}
  
// IsFltGT implementations

inline constexpr bool IsFltGT(float x, float y)
{
  return (x > (y + Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT));
}

inline constexpr bool IsFltGT(double x, double y)
{
  return (x > (y + FLOATING_POINT_COMPARISON_TOLERANCE));
}

// IsFltLE implementations
  
inline constexpr bool IsFltLE(float x, float y)
{
  return (x <= (y + Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT));
}

inline constexpr bool IsFltLE(double x, double y)
{
  return (x <= (y + Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE));
}
  
// IsFltLT implementations
  
inline constexpr bool IsFltLT(float x, float y)
{
  return (x < (y - Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT));
}

inline constexpr bool IsFltLT(double x, double y)
{
  return (x < (y - Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE));
}
  
inline constexpr bool IsFltLT(float x, double y)
{
  return IsFltLT((double)x, y);
}

inline constexpr bool IsFltLT(double x, float y)
{
  return IsFltLT(x, (double)y);
}

// IsFltLTZero implementations
// TRUE if less than the negative tolerance

inline constexpr bool IsFltLTZero(float x)
{
  return (x < -Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
}

inline constexpr bool IsFltLTZero(double x)
{
  return (x < -Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE);
}

// IsFltLEZero implementations
// TRUE if less than or equal to the tolerance

inline constexpr bool IsFltLEZero(float x)
{
  return (x <= Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
}

inline constexpr bool IsFltLEZero(double x)
{
  return (x <= Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE);
}
  
// TRUE if val is within the range [minVal, maxVal], else FALSE
template<typename T>
inline constexpr bool InRange(const T& val, const T& minVal, const T& maxVal) {
  return (val >= minVal) && (val <= maxVal);
}
  
template<typename T>
inline constexpr T Max(const T& a, const T& b) {
  return (a > b) ? a : b;
}

template<typename T>
inline constexpr T Min(const T& a, const T& b) {
  return (a < b) ? a : b;
}
  

// Square of a number
template<typename T>
inline constexpr T Square(const T& inVal) {
  return inVal * inVal;
}


// Convert between millimeters and meters

inline constexpr float ConvertMToMM(float inVal) {
  return inVal * 1000.0f;
}

inline constexpr double ConvertMToMM(double inVal) {
  return inVal * 1000.0;
}

inline constexpr float ConvertMMToM(float inVal) {
  return inVal * 0.001f;
}

inline constexpr double ConvertMMToM(double inVal) {
  return inVal * 0.001;
}

// Convert between radians and degrees
inline constexpr float DegToRad(float deg) {
  return deg * 0.017453292519943295474f;
}

inline constexpr float DegToRad(int deg) {
  return deg * 0.017453292519943295474f;
}

inline constexpr double DegToRad(double deg) {
  return deg * 0.017453292519943295474;
}

inline constexpr float RadToDeg(float rad) {
  return rad * 57.295779513082322865f;
}

inline constexpr double RadToDeg(double rad) {
  return rad * 57.295779513082322865;
}
  
// Time conversions
inline constexpr float NanoSecToSec(float nanoSec) {
  return nanoSec / 1000000000.0f;
}
  
inline constexpr double NanoSecToSec(double nanoSec) {
  return nanoSec / 1000000000.0;
}

inline constexpr double NanoSecToSec(unsigned long long int nanoSec) {
  return (double)nanoSec / 1000000000.0;
}
  
inline constexpr float SecToNanoSec(float sec) {
  return sec * 1000000000.0f;
}

inline constexpr double SecToNanoSec(double sec) {
  return sec * 1000000000.0;
}
  
inline constexpr double SecToNanoSec(unsigned long long int sec) {
  return sec * 1000000000.0;
}
  
inline constexpr float MilliSecToSec(float milliSec) {
  return milliSec / 1000.0f;
}

inline constexpr double MilliSecToSec(double milliSec) {
  return milliSec / 1000.0;
}
  
inline constexpr double MilliSecToSec(unsigned long long int milliSec) {
  return (double)milliSec / 1000.0;
}

inline constexpr double MilliSecToNanoSec(double milliSec) {
  return milliSec * 1000000.0;
}

inline constexpr float SecToMilliSec(float sec) {
  return sec * 1000.0f;
}

inline constexpr double SecToMilliSec(double sec) {
  return sec * 1000.0;
}
  
inline constexpr double SecToMilliSec(unsigned long long int sec) {
  return sec * 1000.0;
}
  
  
// Basic clamp impl. For faster one use fmax/fmin approach
template<class T>
inline ANKI_NON_DEVELOPER_CONSTEXPR T Clamp(const T& val, const T& min, const T& max)
{
  ANKI_DEVELOPER_CODE_ONLY( assert(min <= max) );
  return (val <= min) ? min : ((val >= max) ? max : val);
}

// Wrap a value into the 0..maxValue range (like mod but handles -ve case)
inline float WrapToMax(float inVal, float maxValue)
{
  const float modValue = fmodf(inVal, maxValue);
  // fmodf of a -ve number gives a result <0 (and >-maxValue), add one maxValue to get it in the +ve range
  const float zeroToMaxValue = (modValue < 0.0f) ? (modValue + maxValue) : modValue;
  return zeroToMaxValue;
}

// ClampedUnsignedAddition - (unsigned types only) clamp at max value instead of overflowing
template <typename T>
inline constexpr T ClampedUnsignedAddition(T a, T b)
{
  typedef std::numeric_limits<T> TypeLimits;
  static_assert((TypeLimits::lowest() != TypeLimits::max()), "Invalid numeric_limits for this type!");
  static_assert(!TypeLimits::is_signed, "Use ClampedSignedAddition!");
  
  return (a < (TypeLimits::max()-b)) ? (a + b) : TypeLimits::max();
}

// ClampedSignedAddition - (signed types only) clamp at min or max value instead of under/overflowing
template <typename T>
inline constexpr T ClampedSignedAddition(T a, T b)
{
  // Note: if a and b are of opposite sign then the addition cannot overlow by definition
  
  typedef std::numeric_limits<T> TypeLimits;
  static_assert((TypeLimits::lowest() != TypeLimits::max()), "Invalid numeric_limits for this type!");
  static_assert(TypeLimits::is_signed, "Use ClampedUnsignedAddition!");
  
  return ((a > 0) && (b > 0)) ? ((a < (TypeLimits::max()   -b)) ? (a + b) : TypeLimits::max()) :
         ((a < 0) && (b < 0)) ? ((a > (TypeLimits::lowest()-b)) ? (a + b) : TypeLimits::lowest()) : (a + b);
}
 
// ClampedAddition - uses helpers to determine signed or unsigned version at compile time
// need indirection via std::true/false_type to prevent compiler trying to build in both branches and static asserting
template<typename T>
inline constexpr T ClampedAdditionHelper(T a, T b, /*is_signed =*/ std::false_type)
{
  return ClampedUnsignedAddition<T>(a, b);
}
  
template<typename T>
inline constexpr T ClampedAdditionHelper(T a, T b, /*is_signed =*/ std::true_type)
{
  return ClampedSignedAddition<T>(a, b);
}

template <typename T>
inline constexpr T ClampedAddition(T a, T b)
{
  return ClampedAdditionHelper<T>(a, b, std::is_signed<T>{} );
}

  
} // end namespace Anki
} // namespace Util

// We want to have one math lib so the C++ version includes the C version but we want to make sure we use the inline templated version if only
// this file was include. The __cplusplus macro check doesn't work since the compiler is still C++ but this file might be externed "C"
#undef FLT_NEAR
#define FLT_NEAR(x, y)        Anki::Util::IsFltNear(x, y)

#undef NEAR
#define NEAR(x, y, epsilon)   Anki::Util::IsNear(x, y, epsilon)

#undef NEAR_ZERO
#define NEAR_ZERO(x)          Anki::Util::IsNearZero(x)


// float comparisons (with epsilon tolerance)
#undef FLT_GE_ZERO
#define FLT_GE_ZERO(x)        Anki::Util::IsFltGEZero(x)

#undef FLT_GE
#define FLT_GE(x, y)          Anki::Util::IsFltGE(x, y)

#undef FLT_GT
#define FLT_GT(x, y)          Anki::Util::IsFltGT(x, y)

#undef FLT_LE
#define FLT_LE(x, y)          Anki::Util::IsFltLE(x, y)

#undef FLT_LT
#define FLT_LT(x, y)          Anki::Util::IsFltLT(x, y)


#undef IN_RANGE
#define IN_RANGE(val, minVal, maxVal)   Anki::Util::InRange(val, minVal, maxVal)

#undef ABS
#define ABS(a)    Anki::Util::Abs(a)

#undef SQUARE
#define SQUARE(x) Anki::Util::Square(x)


// TODO( COZMO-7843): Get rid of this and use the templated version above eventually.
// Will need to cast args appropriately throughout the code though.
#undef MAX
//#define MAX(a, b)   Anki::Util::Max(a, b)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#undef MIN
//#define MIN(a, b)   Anki::Util::Min(a, b)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#undef CLIP
//#define CLIP(val, lo, hi)   Anki::Util::Clamp(val, lo, hi)
#define CLIP(val,lo,hi) ( (((((val) > (lo)) ? (val) : (lo)) < (hi)) ? (((val) > (lo)) ? (val) : (lo)) : (hi))   )



//////////////////////////////////////////////////////////////////////////////
// UNIT CONVERSION MACROS
//////////////////////////////////////////////////////////////////////////////

#undef M_TO_MM
#define M_TO_MM(x)   Anki::Util::ConvertMToMM(x)

#undef MM_TO_M
#define MM_TO_M(x)   Anki::Util::ConvertMMToM(x)

#undef DEG_TO_RAD
#define DEG_TO_RAD(deg)      Anki::Util::DegToRad(deg)

#undef RAD_TO_DEG
#define RAD_TO_DEG(rad)      Anki::Util::RadToDeg(rad)


#endif //UTIL_MATH_H_
