#ifndef _ANKICORETECHEMBEDDED_COMMON_FIND_H_
#define _ANKICORETECHEMBEDDED_COMMON_FIND_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/geometry.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySlice;
    template<typename Type> class ConstArraySliceExpression;

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

#pragma mark --- Definitions ---
    template<typename Type1, typename Operator, typename Type2> class Find
    {
    public:
      // Warning: This evaluation is lazy, so it depends on the value of array1 and array2 at the
      //          time of evaluation, NOT at the time the constructor is called. If they are changed
      //          between the constructor and the evaluation, the ouput won't be correct.
      Find(const Array<Type1> &array1, const Array<Type2> &array2);
      Find(const Array<Type1> &array, const Type2 &value);

      // Allocated the memory for yIndexes and xIndexes, and sets them to the index values that match this expression
      // The Indexes Arrays will be allocated by these methods
      Result Evaluate(Array<s32> &indexes, MemoryStack &memory) const; // For 1-dimensional arrays only
      Result Evaluate(Array<s32> &yIndexes, Array<s32> &xIndexes, MemoryStack &memory) const; // For 1-dimensional or 2-dimensional arrays

      // TODO: implement all these
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ArrayType value) const;
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const Array<ArrayType> &input, bool useFindForInput=false) const;
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ConstArraySlice<ArrayType> &in) const;

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

      s32 outputDimensions;

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

#pragma mark --- Implementations ---

    template<typename Type1, typename Operator, typename Type2> Find<Type1,Operator,Type2>::Find(const Array<Type1> &array1, const Array<Type2> &array2)
      : array1(array1), array2(array2), compareWithValue(false), value(static_cast<Type2>(0)), outputDimensions(0)
    {
      if(!array1.IsValid() ||
        !array2.IsValid() ||
        array1.get_size(0) != array2.get_size(0) ||
        array1.get_size(1) != array2.get_size(1)) {
          this->isValid = false;
          AnkiError("Find", "Invalid inputs");
          return;
      }

      Initialize();
    }

    template<typename Type1, typename Operator, typename Type2> Find<Type1,Operator,Type2>::Find(const Array<Type1> &array, const Type2 &value)
      : array1(array), array2(Array<Type2>()), compareWithValue(true), value(value), outputDimensions(0)
    {
      if(!array1.IsValid()) {
        this->isValid = false;
        AnkiError("Find", "Invalid inputs");
        return;
      }

      Initialize();
    }

    template<typename Type1, typename Operator, typename Type2> Result Find<Type1,Operator,Type2>::Evaluate(Array<s32> &indexes, MemoryStack &memory) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Find.Evaluate", "This Find object is invalid");

      AnkiConditionalErrorAndReturnValue(outputDimensions == 1,
        RESULT_FAIL, "Find.Evaluate", "One-dimensional Evaluate only works with one-dimensional Array input");

      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      assert( (arrayHeight==1) || (arrayWidth==1) );

      indexes = Array<s32>(1, this->get_numMatches(), memory);

      s32 * const pIndexes = indexes.Pointer(0,0);

      s32 curIndex = 0;

      if(compareWithValue) {
        if(arrayHeight == 1) {
          const s32 y = 0;
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              pIndexes[curIndex] = x;
              curIndex++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } else { // if(arrayHeight == 1)
          assert(arrayWidth == 1);
          const s32 x = 0;

          for(s32 y=0; y<arrayHeight; y++) {
            const Type1 * const pArray1 = array1.Pointer(y, 0);

            if(Operator::Compare(pArray1[x], value)) {
              pIndexes[curIndex] = y;
              curIndex++;
            }
          } // for(s32 y=0; y<arrayHeight; y++)
        } // if(arrayHeight == 1) ... else
      } else { // if(compareWithValue)
        // These should be checked earlier
        assert(array1.get_size(0) == array2.get_size(0));
        assert(array1.get_size(1) == array2.get_size(1));

        if(arrayHeight == 1) {
          const s32 y = 0;
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], pArray2[x])) {
              pIndexes[curIndex] = x;
              curIndex++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } else { // if(arrayHeight == 1)
          assert(arrayWidth == 1);
          const s32 x = 0;

          for(s32 y=0; y<arrayHeight; y++) {
            const Type1 * const pArray1 = array1.Pointer(y, 0);
            const Type2 * const pArray2 = array2.Pointer(y, 0);

            if(Operator::Compare(pArray1[x], pArray2[x])) {
              pIndexes[curIndex] = y;
              curIndex++;
            }
          } // for(s32 y=0; y<arrayHeight; y++)
        } // if(arrayHeight == 1) ... else
      } // if(compareWithValue) ... else

      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> Result Find<Type1,Operator,Type2>::Evaluate(Array<s32> &yIndexes, Array<s32> &xIndexes, MemoryStack &memory) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Find.Evaluate", "This Find object is invalid");

      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      yIndexes = Array<s32>(1, this->get_numMatches(), memory);
      xIndexes = Array<s32>(1, this->get_numMatches(), memory);

      s32 * const pYIndexes = yIndexes.Pointer(0,0);
      s32 * const pXIndexes = xIndexes.Pointer(0,0);

      s32 curIndex = 0;

      if(compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              pYIndexes[curIndex] = y;
              pXIndexes[curIndex] = x;
              curIndex++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(compareWithValue)
        // These should be checked earlier
        assert(array1.get_size(0) == array2.get_size(0));
        assert(array1.get_size(1) == array2.get_size(1));

        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], pArray2[x])) {
              pYIndexes[curIndex] = y;
              pXIndexes[curIndex] = x;
              curIndex++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } // if(compareWithValue) ... else

      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> void Find<Type1,Operator,Type2>::Initialize()
    {
      this->isValid = true;

      if(MIN(array1.get_size(0), array1.get_size(1)) == 1) {
        this->outputDimensions = 1;
      } else {
        this->outputDimensions = 2;
      }

      this->numMatchesComputed = false;
      this->limitsComputed = false;

      this->numMatches = -1;
      this->limits = Rectangle<s32>(-1, -1, -1, -1);
    }

    template<typename Type1, typename Operator, typename Type2> Result Find<Type1,Operator,Type2>::ComputeNumMatches() const
    {
      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      s32 newNumMatches = 0;

      if(compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              newNumMatches++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(compareWithValue)
        // These should be checked earlier
        assert(array1.get_size(0) == array2.get_size(0));
        assert(array1.get_size(1) == array2.get_size(1));

        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], pArray2[x])) {
              newNumMatches++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } // if(compareWithValue) ... else

      if(this->numMatchesComputed) {
        assert(newNumMatches == this->numMatches); // This should only happen if the data is changed, which it shouldn't be
      }

      this->numMatches = newNumMatches;
      this->numMatchesComputed = true;

      return RESULT_OK;
    } // template<typename Type1, typename Operator, typename Type2> s32 Find<Type1,Operator,Type2>::ComputeNumMatches() const

    template<typename Type1, typename Operator, typename Type2> Result Find<Type1,Operator,Type2>::ComputeLimits() const
    {
      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      Rectangle<s32> newLimits(arrayWidth+1, -1, arrayHeight+1, -1);
      s32 newNumMatches = 0;

      if(compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              newLimits.top = MIN(newLimits.top, y);
              newLimits.bottom = MAX(newLimits.bottom, y);
              newLimits.left = MIN(newLimits.left, x);
              newLimits.right = MAX(newLimits.right, x);
              newNumMatches++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(compareWithValue)
        // These should be checked earlier
        assert(array1.get_size(0) == array2.get_size(0));
        assert(array1.get_size(1) == array2.get_size(1));

        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], pArray2[x])) {
              newLimits.top = MIN(newLimits.top, y);
              newLimits.bottom = MAX(newLimits.bottom, y);
              newLimits.left = MIN(newLimits.left, x);
              newLimits.right = MAX(newLimits.right, x);
              newNumMatches++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } // if(compareWithValue) ... else

      if(this->numMatchesComputed) {
        assert(newNumMatches == this->numMatches); // This should only happen if the data is changed, which it shouldn't be
      }
      this->numMatches = newNumMatches;
      this->numMatchesComputed = true;

      if(this->limitsComputed) {
        assert(newLimits == this->limits);
      }
      this->limits = newLimits;
      this->limitsComputed = true;

      return RESULT_OK;
    } // template<typename Type1, typename Operator, typename Type2> Rectangle<s32> Find<Type1,Operator,Type2>::ComputeLimits() const

    template<typename Type1, typename Operator, typename Type2> template<typename ArrayType> Result Find<Type1,Operator,Type2>::SetArray(Array<ArrayType> &out, const ArrayType value) const
    {
      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Find.SetArray", "This Find object is invalid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "Find.SetArray", "out is invalid");

      AnkiConditionalErrorAndReturnValue(out.get_size(0) == arrayHeight && out.get_size(1) == arrayWidth,
        RESULT_FAIL, "Find.SetArray", "out is not the same size as the input(s)");

      if(compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          Type1 * const pOut = out.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              pOut[x] = value;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(compareWithValue)
        // These should be checked earlier
        assert(array1.get_size(0) == array2.get_size(0));
        assert(array1.get_size(1) == array2.get_size(1));

        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          Type1 * const pOut = out.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], pArray2[x])) {
              pOut[x] = value;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } // if(compareWithValue) ... else

      return RESULT_OK;
    }

    //template<typename Type1, typename Operator, typename Type2, typename Artemplate<typename Type1, typename Operator, typename Type2> template<typename ArrayType>rayType> Result Find<Type1,Operator,Type2>::SetArray(Array<ArrayType> &out, const Array<ArrayType> &input, bool useFindForInput) const
    //{
    //  return RESULT_OK;
    //}

    //template<typename Type1, typename Operator, typename Type2> template<typename ArrayType> Result Find<Type1,Operator,Type2>::SetArray(Array<ArrayType> &out, const ConstArraySlice<ArrayType> &in) const
    //{
    //  return RESULT_OK;
    //}

    template<typename Type1, typename Operator, typename Type2> bool Find<Type1,Operator,Type2>::IsValid() const
    {
      if(!this->isValid)
        return false;

      // TODO: check other things with the arrays, like to guess if they've been changed

      return true;
    }

    template<typename Type1, typename Operator, typename Type2> s32 Find<Type1,Operator,Type2>::get_numMatches() const
    {
      if(!numMatchesComputed)
        ComputeNumMatches();

      return numMatches;
    }

    template<typename Type1, typename Operator, typename Type2> const Rectangle<s32>& Find<Type1,Operator,Type2>::get_limits() const
    {
      if(!limitsComputed)
        ComputeLimits();

      return limits;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FIND_H_