#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Declarations ---
    // Check every element of this array against the input array. If the arrays are different
    // sizes, uninitialized, or if any element is more different than the threshold, then
    // return false.
    template<typename Type> bool AreElementwiseEqual(const Array<Type> &array1, const Array<Type> &array2, const Type threshold = static_cast<Type>(0.0001));

    // Check every element of this array against the input array. If the arrays are different
    // sizes or uninitialized, return false. The percentThreshold is between 0.0 and 1.0. To
    // return false, an element must fail both thresholds. The percent threshold fails if an
    // element is more than a percentage different than its matching element (calulated from the
    // maximum of the two).
    template<typename Type> bool AreElementwiseEqual_PercentThreshold(const Array<Type> &array1, const Array<Type> &array2, const double percentThreshold = 0.01, const double absoluteThreshold = 0.0001);

    // If this array or array2 are different sizes or uninitialized, then return false.
    template<typename Type1, typename Type2> bool AreEqualSize(const Array<Type1> &array1, const Array<Type2> &array2);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_DECLARATIONS_H_