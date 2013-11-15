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

#pragma mark --- Definitions ---
    template<typename Type1, typename Type2> class Find
    {
    public:
      enum Comparison {
        EQUAL,
        NOT_EQUAL,
        LESS_THAN,
        LESS_THAN_OR_EQUAL,
        GREATER_THAN,
        GREATER_THAN_OR_EQUAL
      };

      // Warning: This evaluation is lazy, so it depends on the value of array1 and array2 at the
      //          time of evaluation, NOT at the time the constructor is called. If they are changed
      //          between the constructor and the evaluation, the ouput won't be correct.
      Find(const Array<Type1> &array1, const Comparison comparison, const Array<Type2> &array2);
      Find(const Array<Type1> &array, const Comparison comparison, const Type2 &value);

      // Allocated the memory for yIndexes and xIndexes, and sets them to the index values that match this expression
      Result Evaluate(Array<s32> &yIndexes, Array<s32> &xIndexes, MemoryStack &memory) const;

      // Compute the min and max values for the X and Y dimensions
      Rectangle<s32> Limits() const;

      // TODO: implement all these
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ArrayType value);
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const Array<ArrayType> &in, bool useFindForInput=false);
      template<typename ArrayType> Result SetArray(Array<ArrayType> &out, const ConstArraySlice<ArrayType> &in);

      bool IsValid() const;

      // Return the number of elements where the expression is true
      s32 get_numMatches() const;

    protected:
      const Array<Type1> &array1;
      const Comparison comparison;

      const bool compareWithValue;
      const Array<Type2> &array2;
      const Type2 value;

      bool isValid;
      s32 numMatches;

      s32 computeNumMatches() const;
    }; // class FindLazy

#pragma mark --- Implementations ---

    template<typename Type1, typename Type2> Find<Type1,Type2>::Find(const Array<Type1> &array1, const Comparison comparison, const Array<Type2> &array2)
      : array1(array1), comparison(comparison), array2(array2), compareWithValue(false)
    {
      if(!array1.IsValid() ||
        !array2.IsValid() ||
        array1.get_size(0) != array2.get_size(0) ||
        array1.get_size(1) != array2.get_size(1)) {
          this->isValid = false;
          AnkiError("Find", "Invalid inputs");
          return;
      }

      this->isValid = true;

      this->numMatches = computeNumMatches();
    }

    template<typename Type1, typename Type2> Find<Type1,Type2>::Find(const Array<Type1> &array, const Comparison comparison, const Type2 &value)
      : array1(array), comparison(comparison), value(value), compareWithValue(true)
    {
      if(!array1.IsValid()) {
        this->isValid = false;
        AnkiError("Find", "Invalid inputs");
        return;
      }

      this->isValid = true;

      this->numMatches = computeNumMatches();
    }

    template<typename Type1, typename Type2> s32 Find<Type1,Type2>::computeNumMatches() const
    {
      s32 numMatches = 0;

      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      if(compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          switch(this->comparison) {
          case Find::EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] == value)
                numMatches++;
            }
            break;
          case Find::NOT_EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] != value)
                numMatches++;
            }
            break;
          case Find::LESS_THAN:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] < value)
                numMatches++;
            }
            break;
          case Find::LESS_THAN_OR_EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] <= value)
                numMatches++;
            }
            break;
          case Find::GREATER_THAN:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] > value)
                numMatches++;
            }
            break;
          case Find::GREATER_THAN_OR_EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] >= value)
                numMatches++;
            }
            break;
          default:
            assert(false);
            break;
          } // switch(this->comparison)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(compareWithValue)
        // These should be checked earlier
        assert(array1.get_size(0) == array2.get_size(0));
        assert(array1.get_size(1) == array2.get_size(1));

        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          switch(this->comparison) {
          case Find::EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] == pArray2[x])
                numMatches++;
            }
            break;
          case Find::NOT_EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] != pArray2[x])
                numMatches++;
            }
            break;
          case Find::LESS_THAN:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] < pArray2[x])
                numMatches++;
            }
            break;
          case Find::LESS_THAN_OR_EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] <= pArray2[x])
                numMatches++;
            }
            break;
          case Find::GREATER_THAN:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] > pArray2[x])
                numMatches++;
            }
            break;
          case Find::GREATER_THAN_OR_EQUAL:
            for(s32 x=0; x<arrayWidth; x++) {
              if(pArray1[x] >= pArray2[x])
                numMatches++;
            }
            break;
          default:
            assert(false);
            break;
          } // switch(this->comparison)
        } // for(s32 y=0; y<arrayHeight; y++)
      } // if(compareWithValue) ... else

      return numMatches;
    }

    template<typename Type1, typename Type2> bool Find<Type1,Type2>::IsValid() const
    {
      if(!this->isValid)
        return false;

      // TODO: check other things with the arrays, like to guess if they've been changed

      return true;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FIND_H_