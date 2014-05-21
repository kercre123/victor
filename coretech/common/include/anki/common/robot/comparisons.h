/**
File: comparisons.h
Author: Peter Barnum
Created: 2013

Definitions of comparisons_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_

#include "anki/common/robot/comparisons_declarations.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark

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

    template<typename Type1, typename Type2> bool AreEqualSize(const Type1 &object1, const Type2 &object2)
    {
      if(!object1.IsValid() || !object2.IsValid())
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid())
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid())
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid())
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1) ||
        object1.get_size(0) != object5.get_size(0) || object1.get_size(1) != object5.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2> bool NotAliased(const Type1 &object1, const Type2 &object2)
    {
      if(!object1.IsValid() || !object2.IsValid())
        return false;

      if(object1.get_buffer() == object2.get_buffer())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid())
        return false;

      if(object1.get_buffer() == object2.get_buffer() ||
        object1.get_buffer() == object3.get_buffer())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid())
        return false;

      if(object1.get_buffer() == object2.get_buffer() ||
        object1.get_buffer() == object3.get_buffer() ||
        object1.get_buffer() == object4.get_buffer())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid())
        return false;

      if(object1.get_buffer() == object2.get_buffer() ||
        object1.get_buffer() == object3.get_buffer() ||
        object1.get_buffer() == object4.get_buffer() ||
        object1.get_buffer() == object5.get_buffer())
        return false;

      return true;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_
