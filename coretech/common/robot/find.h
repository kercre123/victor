/**
File: find.h
Author: Peter Barnum
Created: 2013

Definitions of find_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_FIND_H_
#define _ANKICORETECHEMBEDDED_COMMON_FIND_H_

#include "coretech/common/robot/find_declarations.h"
#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/geometry.h"
#include "coretech/common/robot/comparisons.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark

    template<typename Type1, typename Operator, typename Type2> Find<Type1,Operator,Type2>::Find(const Array<Type1> &array1, const Array<Type2> &array2)
      : array1(array1), compareWithValue(false), array2(array2), value(static_cast<Type2>(0)), numOutputDimensions(0)
    {
      if(!AreValid(array1, array2) || !AreEqualSize(array1, array2)) {
        this->isValid = false;
        AnkiError("Find", "Invalid inputs");
        return;
      }

      Initialize();
    }

    template<typename Type1, typename Operator, typename Type2> Find<Type1,Operator,Type2>::Find(const Array<Type1> &array, const Type2 &value)
      : array1(array), compareWithValue(true), array2(array), value(value), numOutputDimensions(0)
      // array2 is initialized to array, but this is just because it has to point to something, though it should not be accessed
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
      AnkiConditionalErrorAndReturnValue(AreValid(*this, memory),
        RESULT_FAIL_INVALID_OBJECT, "Find.Evaluate", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(this->numOutputDimensions == 1,
        RESULT_FAIL_INVALID_PARAMETER, "Find.Evaluate", "One-dimensional Evaluate only works with one-dimensional Array input");

      const s32 arrayWidth = array1.get_size(1);

      AnkiAssert(array1.get_size(0) == 1); // Verify array height

      indexes = Array<s32>(1, this->get_numMatches(), memory);

      if(this->get_numMatches() == 0)
        return RESULT_OK;

      AnkiConditionalErrorAndReturnValue(AreValid(indexes),
        RESULT_FAIL_OUT_OF_MEMORY, "Find.Evaluate", "Invalid objects");

      s32 * const pIndexes = indexes.Pointer(0,0);

      s32 curIndex = 0;

      if(this->compareWithValue) {
        const s32 y = 0;
        const Type1 * const pArray1 = array1.Pointer(y, 0);

        for(s32 x=0; x<arrayWidth; x++) {
          if(Operator::Compare(pArray1[x], value)) {
            pIndexes[curIndex] = x;
            curIndex++;
          }
        } // for(s32 x=0; x<arrayWidth; x++)
      } else { // if(this->compareWithValue)
        // These should be checked earlier
        AnkiAssert(AreEqualSize(array1, array2));

        const s32 y = 0;
        const Type1 * const pArray1 = array1.Pointer(y, 0);
        const Type2 * const pArray2 = array2.Pointer(y, 0);

        for(s32 x=0; x<arrayWidth; x++) {
          if(Operator::Compare(pArray1[x], pArray2[x])) {
            pIndexes[curIndex] = x;
            curIndex++;
          }
        } // for(s32 x=0; x<arrayWidth; x++)
      } // if(this->compareWithValue) ... else

      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> Result Find<Type1,Operator,Type2>::Evaluate(Array<s32> &yIndexes, Array<s32> &xIndexes, MemoryStack &memory) const
    {
      AnkiConditionalErrorAndReturnValue(AreValid(*this, memory),
        RESULT_FAIL_INVALID_OBJECT, "Find.Evaluate", "Invalid objects");

      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      yIndexes = Array<s32>(1, this->get_numMatches(), memory);
      xIndexes = Array<s32>(1, this->get_numMatches(), memory);

      if(this->get_numMatches() == 0)
        return RESULT_OK;

      AnkiConditionalErrorAndReturnValue(AreValid(yIndexes, xIndexes),
        RESULT_FAIL_OUT_OF_MEMORY, "Find.Evaluate", "Invalid objects");

      s32 * const pYIndexes = yIndexes.Pointer(0,0);
      s32 * const pXIndexes = xIndexes.Pointer(0,0);

      s32 curIndex = 0;

      if(this->compareWithValue) {
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
      } else { // if(this->compareWithValue)
        // These should be checked earlier
        AnkiAssert(AreEqualSize(array1, array2));

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
      } // if(this->compareWithValue) ... else

      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> void Find<Type1,Operator,Type2>::Initialize()
    {
      this->isValid = true;

      // If the input is a row vector, then it can be used for 1-dimensional indexing
      if(array1.get_size(0) == 1) {
        this->numOutputDimensions = 1;
      } else {
        this->numOutputDimensions = 2;
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

      if(this->compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              newNumMatches++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(this->compareWithValue)
        // These should be checked earlier
        AnkiAssert(AreEqualSize(array1, array2));

        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);
          const Type2 * const pArray2 = array2.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], pArray2[x])) {
              newNumMatches++;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } // if(this->compareWithValue) ... else

      if(this->numMatchesComputed) {
        AnkiAssert(newNumMatches == this->numMatches); // This should only happen if the data is changed, which it shouldn't be
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

      if(this->compareWithValue) {
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
      } else { // if(this->compareWithValue)
        // These should be checked earlier
        AnkiAssert(AreEqualSize(array1, array2));

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
      } // if(this->compareWithValue) ... else

      if(this->numMatchesComputed) {
        AnkiAssert(newNumMatches == this->numMatches); // This should only happen if the data is changed, which it shouldn't be
      }
      this->numMatches = newNumMatches;
      this->numMatchesComputed = true;

      if(this->limitsComputed) {
        AnkiAssert(newLimits == this->limits);
      }
      this->limits = newLimits;
      this->limitsComputed = true;

      return RESULT_OK;
    } // template<typename Type1, typename Operator, typename Type2> Rectangle<s32> Find<Type1,Operator,Type2>::ComputeLimits() const

    template<typename Type1, typename Operator, typename Type2> template<typename ArrayType> Result Find<Type1,Operator,Type2>::SetArray(Array<ArrayType> &out, const ArrayType value) const
    {
      const s32 arrayHeight = array1.get_size(0);
      const s32 arrayWidth = array1.get_size(1);

      AnkiConditionalErrorAndReturnValue(AreValid(*this, out),
        RESULT_FAIL_INVALID_OBJECT, "Find.SetArray", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(AreEqualSize(array1, out),
        RESULT_FAIL_INVALID_SIZE, "Find.SetArray", "out is not the same size as the input(s)");

      if(this->compareWithValue) {
        for(s32 y=0; y<arrayHeight; y++) {
          const Type1 * const pArray1 = array1.Pointer(y, 0);

          Type1 * const pOut = out.Pointer(y, 0);

          for(s32 x=0; x<arrayWidth; x++) {
            if(Operator::Compare(pArray1[x], value)) {
              pOut[x] = value;
            }
          } // for(s32 x=0; x<arrayWidth; x++)
        } // for(s32 y=0; y<arrayHeight; y++)
      } else { // if(this->compareWithValue)
        // These should be checked earlier
        AnkiAssert(AreEqualSize(array1, array2));

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
      } // if(this->compareWithValue) ... else

      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> template<typename ArrayType> Result Find<Type1,Operator,Type2>::SetArray(Array<ArrayType> &out, const Array<ArrayType> &in, const s32 findWhichDimension) const
    {
      AnkiConditionalErrorAndReturnValue(AreValid(*this, in, out),
        RESULT_FAIL_INVALID_OBJECT, "Find.SetArray", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(this->numOutputDimensions == 1,
        RESULT_FAIL_INVALID_SIZE, "Find.SetArray", "One-dimensional SetArray only works with one-dimensional Array input");

      AnkiConditionalErrorAndReturnValue(findWhichDimension == 0 || findWhichDimension == 1,
        RESULT_FAIL_INVALID_PARAMETER, "Find.SetArray", "findWhichDimension must be zero or one");

      const s32 array1Height = array1.get_size(0);
      const s32 array1Width = array1.get_size(1);

      const s32 inHeight = in.get_size(0);
      const s32 inWidth = in.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      const s32 numMatches = this->get_numMatches();

      AnkiAssert(array1Height == 1);

      if(findWhichDimension == 0) {
        AnkiConditionalErrorAndReturnValue(outHeight == numMatches && outWidth == inWidth,
          RESULT_FAIL_INVALID_SIZE, "Find.SetArray", "out is not the correct size");

        AnkiConditionalErrorAndReturnValue(inHeight == MAX(array1Height, array1Width),
          RESULT_FAIL_INVALID_SIZE, "Find.SetArray", "in is not the correct size");
      } else {
        AnkiConditionalErrorAndReturnValue(outHeight == inHeight && outWidth == numMatches,
          RESULT_FAIL_INVALID_SIZE, "Find.SetArray", "out is not the correct size");

        AnkiConditionalErrorAndReturnValue(inWidth == MAX(array1Height, array1Width),
          RESULT_FAIL_INVALID_SIZE, "Find.SetArray", "in is not the correct size");
      }

      // This is a two-deep nested set of binary if-thens. Each of the four "leaves" will iterate
      // through the entire array, and set the appropriate values.
      //
      // Levels:
      // 1. Is this a array-to-value comparison? (versus array-to-array)
      // 2. Will we use the comparisons to set dimension 0? (versus dimension 1)
      if(this->compareWithValue) {
        if(findWhichDimension == 0) {
          const Type1 * const pArray1 = array1.Pointer(0, 0);

          s32 outY = 0;

          // i iterates on both the width of Array array1 and the height of Array in
          for(s32 i=0; i<array1Width; i++) {
            if(Operator::Compare(pArray1[i], value)) {
              const ArrayType * const pIn = in.Pointer(i, 0);

              ArrayType * const pOut = out.Pointer(outY, 0);

              for(s32 x=0; x<outWidth; x++) {
                pOut[x] = pIn[x];
              } // for(s32 x=0; x<array1Width; x++)

              outY++;
            } // if(Operator::Compare(pArray1[x], value))
          } // for(s32 i=0; i<array1Width; i++)
        } else { // if(findWhichDimension == 0)
          const Type1 * const pArray1 = array1.Pointer(0, 0);

          s32 outX = 0;

          // i iterates on both the width of Array array1 and the width of Array in
          for(s32 i=0; i<array1Width; i++) {
            if(Operator::Compare(pArray1[i], value)) {
              for(s32 y=0; y<outHeight; y++) {
                ArrayType * const pOut = out.Pointer(y, outX);
                const ArrayType * const pIn = in.Pointer(y, i);

                *pOut = *pIn;
              } // for(s32 y=0; y<outHeight; y++)

              outX++;
            } // if(Operator::Compare(pArray1[i], value))
          } // for(s32 i=0; i<array1Width; i++)
        } // if(findWhichDimension == 0) ... else
      } else { // if(this->compareWithValue)
        // These should be checked earlier
        AnkiAssert(AreEqualSize(array1, array2));

        if(findWhichDimension == 0) {
          const Type1 * const pArray1 = array1.Pointer(0, 0);
          const Type1 * const pArray2 = array2.Pointer(0, 0);

          s32 outY = 0;

          // i iterates on the widths of Array array1 and Array array2, and the height of Array in
          for(s32 i=0; i<array1Width; i++) {
            if(Operator::Compare(pArray1[i], pArray2[i])) {
              const ArrayType * const pIn = in.Pointer(i, 0);

              ArrayType * const pOut = out.Pointer(outY, 0);

              for(s32 x=0; x<outWidth; x++) {
                pOut[x] = pIn[x];
              } // for(s32 x=0; x<array1Width; x++)

              outY++;
            } // if(Operator::Compare(pArray1[x], value))
          } // for(s32 i=0; i<array1Width; i++)
        } else { // if(findWhichDimension == 0)
          const Type1 * const pArray1 = array1.Pointer(0, 0);
          const Type1 * const pArray2 = array2.Pointer(0, 0);

          s32 outX = 0;

          // i iterates on the widths of Array array1, Array array2, and Array in
          for(s32 i=0; i<array1Width; i++) {
            if(Operator::Compare(pArray1[i], pArray2[i])) {
              for(s32 y=0; y<outHeight; y++) {
                ArrayType * const pOut = out.Pointer(y, outX);
                const ArrayType * const pIn = in.Pointer(y, i);

                *pOut = *pIn;
              } // for(s32 y=0; y<outHeight; y++)

              outX++;
            } // if(Operator::Compare(pArray1[i], value))
          } // for(s32 i=0; i<array1Width; i++)
        } // if(findWhichDimension == 0) ... else
      } // if(this->compareWithValue) ... else

      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> template<typename ArrayType> Result Find<Type1,Operator,Type2>::SetArray(Array<ArrayType> &out, const ArrayType value, const s32 findWhichDimension) const
    {
      return RESULT_OK;
    }

    template<typename Type1, typename Operator, typename Type2> template<typename ArrayType> Array<ArrayType> Find<Type1,Operator,Type2>::SetArray(const Array<ArrayType> &in, const s32 findWhichDimension, MemoryStack &memory) const
    {
      const s32 numMatches = this->get_numMatches();

      Array<ArrayType> out;

      if(findWhichDimension == 0) {
        out = Array<ArrayType>(numMatches, in.get_size(1), memory);
      } else {
        out = Array<ArrayType>(in.get_size(0), numMatches, memory);
      }

      if(this->SetArray<ArrayType>(out, in, findWhichDimension) != RESULT_OK) {
        out.Resize(0,0,memory);
      }

      return out;
    }

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
