/**
 * File: __Util_FullEnumToValueArrayChecker_H__
 *
 * Author: raul
 * Created: 10/13/2016
 *
 * Description: Classes and operations to define an array of entries that requires at compilation type to specify
 * one entry per entry in the enum, up to the enum special '_Count' field.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Util_FullEnumToValueArrayChecker_H__
#define __Util_FullEnumToValueArrayChecker_H__

#include "templateHelpers.h"

namespace Anki {
namespace Util {
namespace FullEnumToValueArrayChecker {

//                                             - HOW TO USE -
/*
  Simply provide a definition for your the enum and value type you want to require. For example, I have an enum called
  'EMySetOfThings', and I want to require to specify one 'bool' per entry, and only one, so that whenever someone
  adds one entry to the enum, the code requires at compilation time for them to add a bool entry to my array, forcing
  them to decide if that value should be true/false given the usage of my array. If your enum does not have _Count, but
  other field as last/count, you can specify so in the template.
  Example:
 
    enum class EMySetOfThings
    {
      a,
      b,
      c,
      MyCountControlField  // _Count is default, but you can specify your own
    };
 
    // 1) by using 'FullEnumToValueArray', you are required to add one entry per enum value, since the array has to
    // have _Count elements.
 
    FullEnumToValueArray<EMySetOfThings, bool, MyCountControlField> entriesThatHaveVowels =
    {
     {a, true },
     {b, false},
     {c, false}
    };
    
    // 2) by using static_assert on IsSequentialArray, you are required to add one and only one entry per enum value, 
    // and to specify them in the same order the enum is declared.
 
    static_assert( IsSequentialArray(entriesThatHaveVowels), "The array does not define each entry once and only once!");
*/

//                                             - IMPLEMENTATION DETAILS -
// Note that by default I do not create the static_assert that checks for IsSequentialArray. We could provide
// a wrapper that automatically does this, but for now I prefer that to be programmer responsibility. The wrapper
// would add an extra layer to syntax, and nothing would prevent the caller from creating an array without the wrapper.

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// EnumToValueEntry: definition of every entry in the fullContentArray so that we can provide checks for specification
// requirements, for example that all entries are defined once and only once.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, typename ValueType, EnumType CountField>
class EnumToValueEntry {
public:

  // this static is not really required by the entry, however it allows the functions that receive an array to
  // infer the template for CountField given the array, since the entries store it. Otherwise the caller would
  // have to specify the value for this template. It doesn't take much space since it's a static (shared by instances),
  // so I'm adding it in favor of cleaner calling code.
  constexpr static const EnumType sCountField = CountField;
  
  // constexpr constructor
  constexpr EnumToValueEntry(EnumType e, ValueType&& v) : enumValue(e), value(v) {}
  constexpr EnumToValueEntry(EnumType e, const ValueType& v) : enumValue(e), value(v) {}
  
  // constexpr accessors
  constexpr EnumType EnumValue() const { return enumValue; }
  constexpr ValueType Value() const { return value; }
  
private:
  const EnumType  enumValue; // enumVal: the value of the enum for this entry
  const ValueType value;     // value: the actual meta-value we want to assign to this entry inside the array
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// FullEnumToValueArray: array of entries that requires you to add them all
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename EnumType, typename ValueType, EnumType CountField = EnumType::_Count>
using FullEnumToValueArray = EnumToValueEntry<EnumType, ValueType, CountField>[Util::EnumToUnderlying(CountField)];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// IsSequentialArray and helper checks: to static assert
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// returns true if all indices are sequential in the given array
template<typename EnumType, typename ValueType, EnumType CountField>
inline static constexpr bool IsSequentialArray(const FullEnumToValueArray<EnumType, ValueType, CountField>& array);

// returns true if the given index is sequential in the given array (the entry's enum value matches the index)
template<typename EnumType, typename ValueType, EnumType CountField>
inline static constexpr bool IsSequentialIndex(const FullEnumToValueArray<EnumType, ValueType, CountField>& array, size_t index);

// returns true if the given index is sequential, and all indices after it
template<typename EnumType, typename ValueType, EnumType CountField>
inline static constexpr bool IsSequentialAllIndicesFrom(const FullEnumToValueArray<EnumType, ValueType, CountField>& array, size_t index);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, typename ValueType, EnumType CountField>
inline static constexpr bool IsSequentialArray(const FullEnumToValueArray<EnumType, ValueType, CountField>& array)
{
  return IsSequentialAllIndicesFrom<EnumType, ValueType, CountField>(array, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, typename ValueType, EnumType CountField>
inline static constexpr bool IsSequentialIndex(const FullEnumToValueArray<EnumType, ValueType, CountField>& array, size_t index)
{
  return Util::EnumToUnderlying(array[index].EnumValue()) == index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, typename ValueType, EnumType CountField>
inline static constexpr bool IsSequentialAllIndicesFrom(const FullEnumToValueArray<EnumType, ValueType, CountField>& array, size_t index)
{
  // if we reach the last one
  return (index == Util::EnumToUnderlying(EnumToValueEntry<EnumType, ValueType, CountField>::sCountField)) ?
          // true
          true :
          // else check this and +1
          ( IsSequentialIndex<EnumType, ValueType, CountField>(array, index) &&
            IsSequentialAllIndicesFrom<EnumType, ValueType, CountField>(array, index+1) );
}

}; // namespace

}; // namespace
}; // namespace

#endif
