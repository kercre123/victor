/**
 * File: numericCast
 *
 * Author: Mark Wesley
 * Created: 30/06/15
 *
 * Description: numeric_cast<> for asserting that a value won't under/over-flow into the new type
 *              resolves to a constexpr static_cast<> in NDEBUG
 *              numeric_cast_clamped<> clamps the value to the new types lowest..max in all build configs
 *              both utlize a bunch of helper functions
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_NumericCast_H__
#define __Util_NumericCast_H__

#include <limits>
#include <type_traits>
#include <assert.h>
#include "util/global/globalDefinitions.h"

namespace Anki {
namespace Util {


template<typename FirstType, typename SecondType>
inline constexpr bool DoesFirstTypeHaveGTRange()  // GT = Greater Than
{
  typedef std::numeric_limits<FirstType> FirstTypeLimits;
  typedef std::numeric_limits<SecondType> SecondTypeLimits;

  // Compare exponent first, then digits if tied, to ensure a float with
  // large range is sorted above an integer with a larger mantissa
  return (FirstTypeLimits::max_exponent > SecondTypeLimits::max_exponent) ||
         ((FirstTypeLimits::max_exponent == SecondTypeLimits::max_exponent) &&
          (FirstTypeLimits::digits > SecondTypeLimits::digits));
}


template<typename FirstType, typename SecondType>
inline constexpr bool DoesFirstTypeHaveGTERange() // GTE = Greater Than or Equal
{
  typedef std::numeric_limits<FirstType> FirstTypeLimits;
  typedef std::numeric_limits<SecondType> SecondTypeLimits;

  // Compare exponent first, then digits if tied, to ensure a float with
  // large range is sorted above an integer with a larger mantissa
  return (FirstTypeLimits::max_exponent > SecondTypeLimits::max_exponent) ||
         ((FirstTypeLimits::max_exponent == SecondTypeLimits::max_exponent) &&
          (FirstTypeLimits::digits >= SecondTypeLimits::digits));
}


template<typename ToType, typename FromType>
inline constexpr bool IsValidNumericCastBothSigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   //   signed

  return DoesFirstTypeHaveGTERange<ToType, FromType>() ? true : // going to same/larger type - must fit
          ((fromVal >= (FromType)ToTypeLimits::lowest()) &&    // check within lowest..max (compare as larger FromType)
           (fromVal <= (FromType)ToTypeLimits::max()));
}


template<typename ToType, typename FromType>
inline constexpr bool IsValidNumericCastFromSigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   // unsigned

  // We wish to avoid risk of casting a -ve value to the unsigned ToType

  return (fromVal < FromType(0)) ? false : // -ve signed value can't fit in any unsigned type
            DoesFirstTypeHaveGTERange<ToType, FromType>()      ? true // going to same/larger type - must fit
                        : (fromVal <= (FromType)ToTypeLimits::max()); // check within max (compare as larger FromType)
}


template<typename ToType, typename FromType>
inline constexpr bool IsValidNumericCastToSigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   //   signed

  // no need to check lowest - signed always goes lower than unsigned
  return DoesFirstTypeHaveGTERange<ToType, FromType>() ? true :  // going to same/larger type - must fit
        (fromVal <= (FromType) ToTypeLimits::max());             // check within max (compare as larger FromType)
}


template<typename ToType, typename FromType>
inline constexpr bool IsValidNumericCastBothUnsigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   // unsigned

  // no need to check lowest - both types >= 0
  return DoesFirstTypeHaveGTERange<ToType, FromType>() ? true :  // going to same/larger type - must fit
    (fromVal <= (FromType) ToTypeLimits::max());             // check within max (compare as larger FromType)
}


template<typename ToType, typename FromType>
inline constexpr
typename std::enable_if<!std::is_enum<FromType>::value, bool>::type
IsValidNumericCast(FromType fromVal)
{
  typedef std::numeric_limits<FromType> FromTypeLimits;
  typedef std::numeric_limits<ToType>   ToTypeLimits;

  static_assert((FromTypeLimits::lowest() != FromTypeLimits::max()), "numeric_limits improperly defined for FromType - if it's an enum then cast to int first");
  static_assert((ToTypeLimits::lowest() != ToTypeLimits::max()), "numeric_limits improperly defined for ToType   - if it's an enum then cast to int first");

  return (FromTypeLimits::is_signed && ToTypeLimits::is_signed)
                                      ? IsValidNumericCastBothSigned<ToType, FromType>(fromVal) :
          (FromTypeLimits::is_signed) ? IsValidNumericCastFromSigned<ToType, FromType>(fromVal) :
            (ToTypeLimits::is_signed) ? IsValidNumericCastToSigned<ToType, FromType>(fromVal)
                                      : IsValidNumericCastBothUnsigned<ToType, FromType>(fromVal);
}

// auto-cast enum to underlying type and call IsValidNumericCast for underlying_type
template<typename ToType, typename FromTypeEnum>
inline constexpr
typename std::enable_if<std::is_enum<FromTypeEnum>::value, bool>::type
IsValidNumericCast(FromTypeEnum fromValEnum)
{
  return IsValidNumericCast<ToType>( static_cast<typename std::underlying_type<FromTypeEnum>::type>(fromValEnum) );
}

template<typename ToType, typename FromType>
inline ANKI_NON_DEVELOPER_CONSTEXPR ToType numeric_cast(FromType inVal)
{
  ANKI_DEVELOPER_CODE_ONLY( assert(IsValidNumericCast<ToType>(inVal)) );
  return static_cast<ToType>(inVal);
}


template<typename ToType, typename FromType>
inline constexpr ToType Helper_numeric_cast_clampedBothSigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   //   signed

  return DoesFirstTypeHaveGTERange<ToType, FromType>()
                      ? static_cast<ToType>(fromVal) : // going to same/larger type - must fit
          (fromVal > (FromType)ToTypeLimits::max())
                      ? ToTypeLimits::max() :          // clamp to max    (cast to larger FromType type for comparison)
          (fromVal < (FromType)ToTypeLimits::lowest())
                      ? ToTypeLimits::lowest()         // clamp to lowest (cast to larger FromType type for comparison)
                      : static_cast<ToType>(fromVal);  // value in range
}


template<typename ToType, typename FromType>
inline constexpr ToType Helper_numeric_cast_clampedFromSigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   // unsigned

  return (fromVal < FromType(0))
                        ? ToTypeLimits::lowest() :      // -ve signed value can't fit in any unsigned type
          (DoesFirstTypeHaveGTRange<FromType, ToType>() &&
           (fromVal > (FromType)ToTypeLimits::max()))
                        ? ToTypeLimits::max()           // clamp to max (cast to larger FromType type for comparison)
                        : static_cast<ToType>(fromVal); // value in range
}


template<typename ToType, typename FromType>
inline constexpr ToType Helper_numeric_cast_clampedToSigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   //   signed

  return (DoesFirstTypeHaveGTRange<FromType, ToType>() && (fromVal > (FromType)ToTypeLimits::max()))
            ? ToTypeLimits::max()            // clamp to max (cast to larger FromType type for comparison)
            : static_cast<ToType>(fromVal);  // value in range
}


template<typename ToType, typename FromType>
inline constexpr ToType Helper_numeric_cast_clampedBothUnsigned(FromType fromVal)
{
  typedef std::numeric_limits<ToType> ToTypeLimits;   // unsigned

  return (DoesFirstTypeHaveGTRange<FromType, ToType>() && (fromVal > (FromType)ToTypeLimits::max()))
            ? ToTypeLimits::max()            // clamp to max (cast to larger FromType type for comparison)
            : static_cast<ToType>(fromVal);  // value in range
}


template<typename ToType, typename FromType>
inline constexpr
typename std::enable_if<!std::is_enum<FromType>::value, ToType>::type
numeric_cast_clamped(FromType fromVal)
{
  typedef std::numeric_limits<FromType> FromTypeLimits;
  typedef std::numeric_limits<ToType>   ToTypeLimits;

  static_assert((FromTypeLimits::lowest() != FromTypeLimits::max()),
                "numeric_limits improperly defined for FromType - if it's an enum then cast to int first");
  static_assert((ToTypeLimits::lowest() != ToTypeLimits::max()),
                "numeric_limits improperly defined for ToType   - if it's an enum then cast to int first");

  return (FromTypeLimits::is_signed && ToTypeLimits::is_signed)
                                          ? Helper_numeric_cast_clampedBothSigned<  ToType, FromType>(fromVal)
              : FromTypeLimits::is_signed ? Helper_numeric_cast_clampedFromSigned<  ToType, FromType>(fromVal)
              :   ToTypeLimits::is_signed ? Helper_numeric_cast_clampedToSigned<    ToType, FromType>(fromVal)
                                          : Helper_numeric_cast_clampedBothUnsigned<ToType, FromType>(fromVal);
}

// auto-cast enum to underlying type and call IsValidNumericCast for underlying_type
template<typename ToType, typename FromTypeEnum>
inline constexpr
typename std::enable_if<std::is_enum<FromTypeEnum>::value, ToType>::type
numeric_cast_clamped(FromTypeEnum fromValEnum)
{
  return numeric_cast_clamped<ToType>( static_cast<typename std::underlying_type<FromTypeEnum>::type>(fromValEnum) );
}

} // end namespace Anki
} // namespace Util

#endif //__Util_NumericCast_H__
