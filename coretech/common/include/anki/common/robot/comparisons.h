#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---
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

#pragma mark --- Implementations ---

    template<typename Type> bool AreElementwiseEqual(const Array<Type> &array1, const Array<Type> &array2, const Type threshold)
    {
      if(!AreEqualSize(array1, array2))
        return false;

      const s32 height = array1.get_size(0);
      const s32 width = array1.get_size(1);

      for(s32 y=0; y<height; y++) {
        const Type * const pArray1 = array1.Pointer(y, 0);
        const Type * const pArray2 = array2.Pointer(y, 0);
        for(s32 x=0; x<width; x++) {
          if(pArray1[x] > pArray2[x]) {
            if((pArray1[x] - pArray2[x]) > threshold)
              return false;
          } else {
            if((pArray2[x] - pArray1[x]) > threshold)
              return false;
          }
        }
      }

      return true;
    }

    template<typename Type> bool AreElementwiseEqual_PercentThreshold(const Array<Type> &array1, const Array<Type> &array2, const double percentThreshold, const double absoluteThreshold)
    {
      if(!AreEqualSize(array1, array2))
        return false;

      const s32 height = array1.get_size(0);
      const s32 width = array1.get_size(1);

      for(s32 y=0; y<height; y++) {
        const Type * const pArray1 = array1.Pointer(y, 0);
        const Type * const pArray2 = array2.Pointer(y, 0);
        for(s32 x=0; x<width; x++) {
          const double value1 = static_cast<double>(pArray1[x]);
          const double value2 = static_cast<double>(pArray2[x]);
          const double percentThresholdValue = percentThreshold * MAX(value1,value2);

          if(fabs(value1 - value2) > percentThresholdValue && fabs(value1 - value2) > absoluteThreshold)
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2> bool AreEqualSize(const Array<Type1> &array1, const Array<Type2> &array2)
    {
      if(!array1.IsValid())
        return false;

      if(!array2.IsValid())
        return false;

      if(array1.get_size(0) != array2.get_size(0) || array1.get_size(1) != array2.get_size(1))
        return false;

      return true;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_