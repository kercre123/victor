/**
File: comparisons_declarations.h
Author: Peter Barnum
Created: 2013

Various elementwise tests, to ensure that the data of two Arrays is equal.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- Declarations ---
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

    // If any of the input objects are not valid, then return false
    // NOTE: the objects must have an IsValid() method
    template<typename Type1> bool AreValid(const Type1 &object1);
    template<typename Type1, typename Type2> bool AreValid(const Type1 &object1, const Type2 &object2);
    template<typename Type1, typename Type2, typename Type3> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3);
    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool AreValid(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9);

    // If the objects have different sizes or are uninitialized, then return false.
    // NOTE: the objects must have IsValid() and get_buffer() methods
    template<typename Type1, typename Type2> bool AreEqualSize(const Type1 &object1, const Type2 &object2);
    template<typename Type1, typename Type2, typename Type3> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3);
    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool AreEqualSize(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9);

    // Check sizes against an input height and width
    template<typename Type1> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1);
    template<typename Type1, typename Type2> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2);
    template<typename Type1, typename Type2, typename Type3> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3);
    template<typename Type1, typename Type2, typename Type3, typename Type4> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool AreEqualSize(const s32 height, const s32 width, const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9);

    // If the Arrays are aliased (pointing to the same location in memory) or uninitialized, then return false
    // NOTE: the objects must have IsValid() and get_buffer() methods
    template<typename Type1, typename Type2> bool NotAliased(const Type1 &object1, const Type2 &object2);
    template<typename Type1, typename Type2, typename Type3> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3);
    template<typename Type1, typename Type2, typename Type3, typename Type4> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8);
    template<typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Type9> bool NotAliased(const Type1 &object1, const Type2 &object2, const Type3 &object3, const Type4 &object4, const Type5 &object5, const Type6 &object6, const Type7 &object7, const Type8 &object8, const Type9 &object9);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPARISONS_DECLARATIONS_H_
