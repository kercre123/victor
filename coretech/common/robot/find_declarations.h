/**
File: find_declarations.h
Author: Peter Barnum
Created: 2013

Find is used similarly to the Matlab function, and allows for easy prototyping, with low memory overhead.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_FIND_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_FIND_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"
#include "coretech/common/robot/geometry_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    // A Comparison is a simple functor that compares the equality of two operands.
    namespace Comparison
    {
      template<typename Type1, typename Type2> class Equal {
      public:
        static inline bool Compare(const Type1 value1, const Type2 value2) {return value1 == value2;}
      };

      template<typename Type1, typename Type2> class NotEqual {
      public:
        static inline bool Compare(const Type1 value1, const Type2 value2) {return value1 != value2;}
      };

      template<typename Type1, typename Type2> class LessThan {
      public:
        static inline bool Compare(const Type1 value1, const Type2 value2) {return value1 < value2;}
      };

      template<typename Type1, typename Type2> class LessThanOrEqual {
      public:
        static inline bool Compare(const Type1 value1, const Type2 value2) {return value1 <= value2;}
      };

      template<typename Type1, typename Type2> class GreaterThan {
      public:
        static inline bool Compare(const Type1 value1, const Type2 value2) {return value1 > value2;}
      };

      template<typename Type1, typename Type2> class GreaterThanOrEqual {
      public:
        static inline bool Compare(const Type1 value1, const Type2 value2) {return value1 >= value2;}
      };
    } // namepace Comparison

// #pragma mark --- Declarations ---

    // Find acts similar to the Matlab find(). It is useful for quick prototyping of Matlab-like c++ code.
    template<typename Type1, typename Operator, typename Type2> class Find
    {
    public:
      // WARNING: This evaluation is lazy, so it depends on the value of array1 and array2 at the
      //          time of evaluation, NOT at the time the constructor is called. If they are changed
      //          between the constructor and the evaluation, the ouput won't be correct.
      Find(const Array<Type1> &array1, const Array<Type2> &array2);
      Find(const Array<Type1> &array, const Type2 &value);

      // Allocated the memory for yIndexes and xIndexes, and sets them to the index values that match this expression
      // The Indexes Arrays will be allocated by these methods
      Result Evaluate(Array<s32> &indexes, MemoryStack &memory) const; // For 1-dimensional arrays only
      Result Evaluate(Array<s32> &yIndexes, Array<s32> &xIndexes, MemoryStack &memory) const; // For 1-dimensional or 2-dimensional arrays

      // For all matching (y,x) indexes, set the value of out(y,x) to value
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ArrayType value) const;

      // For the result of a vector comparison only, set the matching values of out to those of input.
      // Matlab equivalent for findWhichDimension==0: out = in(comparison, :);
      // Matlab equivalent for findWhichDimension==1: out = in(:, comparison);
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const Array<ArrayType> &in, const s32 findWhichDimension) const;
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ArrayType value, const s32 findWhichDimension) const;

      // Same as SetArray above, but also allocates the memory for out from MemoryStack memory
      template<typename ArrayType> Array<ArrayType> SetArray(const Array<ArrayType> &in, const s32 findWhichDimension, MemoryStack &memory) const;

      //template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const Array<ArrayType> &input, s32 findWhichDimension, bool useFindForInput=false) const;

      //template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ConstArraySlice<ArrayType> &in) const;

      bool IsValid() const;

      // Return the number of elements where the expression is true
      // The first time this is called, it computes the number. Subsequent times use the stored value.
      s32 get_numMatches() const;

      // Return the min and max values for the X and Y dimensions
      // The first time this is called, it computes the number. Subsequent times use the stored value.
      const Rectangle<s32>& get_limits() const;

    protected:
      const Array<Type1> &array1;

      const bool compareWithValue;
      const Array<Type2> &array2;
      const Type2 value;

      s32 numOutputDimensions;

      bool isValid;

      mutable bool numMatchesComputed;
      mutable s32 numMatches;

      mutable bool limitsComputed;
      mutable Rectangle<s32> limits;

      void Initialize();

      // Compute the number of matches
      Result ComputeNumMatches() const;

      // Compute the min and max values for the X and Y dimensions, plus the number of matches
      Result ComputeLimits() const;
    }; // class FindLazy
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FIND_DECLARATIONS_H_
