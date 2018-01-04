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

#include "coretech/common/robot/comparisons_declarations.h"
#include "coretech/common/robot/array2d.h"

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

    template<typename Type1> bool AreValid(const Type1 &object1)
    {
      if(!object1.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2> bool AreValid(const Type1 &object1, const Type2 &object2)
    {
      if(!object1.IsValid() || !object2.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid() || !object6.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid() || !object6.IsValid() || !object7.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid() || !object6.IsValid() || !object7.IsValid() || !object8.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9)
    {
      if(!object1.IsValid() || !object2.IsValid() || !object3.IsValid() || !object4.IsValid() || !object5.IsValid() || !object6.IsValid() || !object7.IsValid() || !object8.IsValid() || !object9.IsValid())
        return false;

      return true;
    }

    template<typename Type1, typename Type2> bool AreEqualSize(const Type1 &object1, const Type2 &object2)
    {
      if(!AreValid(object1, object2))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3)
    {
      if(!AreValid(object1, object2, object3))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4)
    {
      if(!AreValid(object1, object2, object3, object4))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5)
    {
      if(!AreValid(object1, object2, object3, object4, object5))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1) ||
        object1.get_size(0) != object5.get_size(0) || object1.get_size(1) != object5.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1) ||
        object1.get_size(0) != object5.get_size(0) || object1.get_size(1) != object5.get_size(1) ||
        object1.get_size(0) != object6.get_size(0) || object1.get_size(1) != object6.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1) ||
        object1.get_size(0) != object5.get_size(0) || object1.get_size(1) != object5.get_size(1) ||
        object1.get_size(0) != object6.get_size(0) || object1.get_size(1) != object6.get_size(1) ||
        object1.get_size(0) != object7.get_size(0) || object1.get_size(1) != object7.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7, object8))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1) ||
        object1.get_size(0) != object5.get_size(0) || object1.get_size(1) != object5.get_size(1) ||
        object1.get_size(0) != object6.get_size(0) || object1.get_size(1) != object6.get_size(1) ||
        object1.get_size(0) != object7.get_size(0) || object1.get_size(1) != object7.get_size(1) ||
        object1.get_size(0) != object8.get_size(0) || object1.get_size(1) != object8.get_size(1))
        return false;

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7, object8, object9))
        return false;

      if(object1.get_size(0) != object2.get_size(0) || object1.get_size(1) != object2.get_size(1) ||
        object1.get_size(0) != object3.get_size(0) || object1.get_size(1) != object3.get_size(1) ||
        object1.get_size(0) != object4.get_size(0) || object1.get_size(1) != object4.get_size(1) ||
        object1.get_size(0) != object5.get_size(0) || object1.get_size(1) != object5.get_size(1) ||
        object1.get_size(0) != object6.get_size(0) || object1.get_size(1) != object6.get_size(1) ||
        object1.get_size(0) != object7.get_size(0) || object1.get_size(1) != object7.get_size(1) ||
        object1.get_size(0) != object8.get_size(0) || object1.get_size(1) != object8.get_size(1) ||
        object1.get_size(0) != object9.get_size(0) || object1.get_size(1) != object9.get_size(1))
        return false;

      return true;
    }

    template<typename Type1> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1)
    {
      if(!AreValid(object1))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return true;
    }

    template<typename Type1, typename Type2> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2)
    {
      if(!AreValid(object1, object2))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2);
    }

    template<typename Type1, typename Type2, typename Type3> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3)
    {
      if(!AreValid(object1, object2, object3))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3);
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4)
    {
      if(!AreValid(object1, object2, object3, object4))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3, object4);
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5)
    {
      if(!AreValid(object1, object2, object3, object4, object5))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3, object4, object5);
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3, object4, object5, object6);
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3, object4, object5, object6, object7);
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7, object8))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3, object4, object5, object6, object7, object8);
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7, object8, object9))
        return false;

      if(object1.get_size(0) != height || object1.get_size(1) != width)
        return false;

      return AreEqualSize(object1, object2, object3, object4, object5, object6, object7, object8, object9);
    }

    template<typename Type1, typename Type2> bool NotAliased(const Type1 &object1, const Type2 &object2)
    {
      if(!AreValid(object1, object2))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer())};

      for(s32 i=0; i<2; i++) {
        for(s32 j=i+1; j<2; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3)
    {
      if(!AreValid(object1, object2, object3))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer())};

      for(s32 i=0; i<3; i++) {
        for(s32 j=i+1; j<3; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4)
    {
      if(!AreValid(object1, object2, object3, object4))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer()),
        reinterpret_cast<size_t>(object4.get_buffer())};

      for(s32 i=0; i<4; i++) {
        for(s32 j=i+1; j<4; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5)
    {
      if(!AreValid(object1, object2, object3, object4, object5))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer()),
        reinterpret_cast<size_t>(object4.get_buffer()),
        reinterpret_cast<size_t>(object5.get_buffer())};

      for(s32 i=0; i<5; i++) {
        for(s32 j=i+1; j<5; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer()),
        reinterpret_cast<size_t>(object4.get_buffer()),
        reinterpret_cast<size_t>(object5.get_buffer()),
        reinterpret_cast<size_t>(object6.get_buffer())};

      for(s32 i=0; i<6; i++) {
        for(s32 j=i+1; j<6; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer()),
        reinterpret_cast<size_t>(object4.get_buffer()),
        reinterpret_cast<size_t>(object5.get_buffer()),
        reinterpret_cast<size_t>(object6.get_buffer()),
        reinterpret_cast<size_t>(object7.get_buffer())};

      for(s32 i=0; i<7; i++) {
        for(s32 j=i+1; j<7; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7, object8))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer()),
        reinterpret_cast<size_t>(object4.get_buffer()),
        reinterpret_cast<size_t>(object5.get_buffer()),
        reinterpret_cast<size_t>(object6.get_buffer()),
        reinterpret_cast<size_t>(object7.get_buffer()),
        reinterpret_cast<size_t>(object8.get_buffer())};

      for(s32 i=0; i<8; i++) {
        for(s32 j=i+1; j<8; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }

    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9)
    {
      if(!AreValid(object1, object2, object3, object4, object5, object6, object7, object8, object9))
        return false;

      const size_t bufferPointers[] = {
        reinterpret_cast<size_t>(object1.get_buffer()),
        reinterpret_cast<size_t>(object2.get_buffer()),
        reinterpret_cast<size_t>(object3.get_buffer()),
        reinterpret_cast<size_t>(object4.get_buffer()),
        reinterpret_cast<size_t>(object5.get_buffer()),
        reinterpret_cast<size_t>(object6.get_buffer()),
        reinterpret_cast<size_t>(object7.get_buffer()),
        reinterpret_cast<size_t>(object8.get_buffer()),
        reinterpret_cast<size_t>(object9.get_buffer()) };

      for(s32 i=0; i<9; i++) {
        for(s32 j=i+1; j<9; j++) {
          if(bufferPointers[i] == bufferPointers[j])
            return false;
        }
      }

      return true;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_H_
