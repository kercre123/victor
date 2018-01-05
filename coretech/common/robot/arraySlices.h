/**
File: arraySlices.h
Author: Peter Barnum
Created: 2013

Definitions of arraySlices_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_

#include "coretech/common/robot/arraySlices_declarations.h"
#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/errorHandling.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> ConstArraySlice<Type>::ConstArraySlice()
      : ySlice(LinearSequence<s32>()), xSlice(LinearSequence<s32>()), array(Array<Type>()), constArrayData(NULL)
    {
    }

    template<typename Type> ConstArraySlice<Type>::ConstArraySlice(const Array<Type> &array)
      : ySlice(LinearSequence<s32>(0,array.get_size(0)-1)), xSlice(LinearSequence<s32>(0,array.get_size(1)-1)), array(array)
    {
      if(array.get_numElements() == 0) {
        this->constArrayData = NULL;
      } else {
        this->constArrayData = array.Pointer(0,0);
      }
    }

    template<typename Type> ConstArraySlice<Type>::ConstArraySlice(const Array<Type> &array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice)
      : ySlice(ySlice), xSlice(xSlice), array(array)
    {
      if(array.get_numElements() == 0) {
        this->constArrayData = NULL;
      } else {
        this->constArrayData = array.Pointer(0,0);
      }
    }

    template<typename Type> ConstArraySliceExpression<Type> ConstArraySlice<Type>::Transpose() const
    {
      ConstArraySliceExpression<Type> expression(*this, true);

      return expression;
    }

    template<typename Type> bool ConstArraySlice<Type>::IsValid() const
    {
      return this->array.IsValid();
    }

    template<typename Type> const LinearSequence<s32>& ConstArraySlice<Type>::get_ySlice() const
    {
      return ySlice;
    }

    template<typename Type> const LinearSequence<s32>& ConstArraySlice<Type>::get_xSlice() const
    {
      return xSlice;
    }

    template<typename Type> const Array<Type>& ConstArraySlice<Type>::get_array() const
    {
      return this->array;
    }

    template<typename Type> ArraySlice<Type>::ArraySlice()
      : ConstArraySlice<Type>(), arrayData(NULL)
    {
    }

    template<typename Type> ArraySlice<Type>::ArraySlice(Array<Type> array)
      : ConstArraySlice<Type>(array)
    {
      if(array.get_numElements() == 0) {
        this->arrayData = NULL;
      } else {
        this->arrayData = array.Pointer(0,0);
      }
    }

    template<typename Type> ArraySlice<Type>::ArraySlice(Array<Type> array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice)
      : ConstArraySlice<Type>(array, ySlice, xSlice)
    {
      if(array.get_numElements() == 0) {
        this->arrayData = NULL;
      } else {
        this->arrayData = array.Pointer(0,0);
      }
    }

    template<typename Type> s32 ArraySlice<Type>::Set(const ConstArraySliceExpression<Type> &input, bool automaticTranspose)
    {
      return this->SetCast<Type>(input, automaticTranspose);
    }

    template<typename Type> s32 ArraySlice<Type>::Set(const LinearSequence<Type> &input)
    {
      const Result result = input.Evaluate(*this);
      return (result==RESULT_OK) ? input.get_size() : 0;
    }

    template<typename Type> s32 ArraySlice<Type>::Set(const Type value)
    {
      Array<Type> &array = this->get_array();

      AnkiConditionalErrorAndReturnValue(array.IsValid(),
        0, "ArraySlice<Type>::Set", "Array<Type> is not valid");

      const ArraySliceLimits_in1_out0<s32> limits(this->get_ySlice(), this->get_xSlice());

      AnkiConditionalErrorAndReturnValue(limits.isValid,
        0, "ArraySlice<Type>::Set", "Limits is not valid");

      for(s32 iy=0; iy<limits.rawIn1Limits.ySize; iy++) {
        const s32 y = limits.rawIn1Limits.yStart + iy * limits.rawIn1Limits.yIncrement;
        Type * restrict pMat = array.Pointer(y, 0);

        for(s32 ix=0; ix<limits.rawIn1Limits.xSize; ix++) {
          const s32 x = limits.rawIn1Limits.xStart + ix * limits.rawIn1Limits.xIncrement;
          pMat[x] = value;
        }
      }

      return limits.rawIn1Limits.xSize*limits.rawIn1Limits.ySize;
    }

    template<typename Type> s32 ArraySlice<Type>::Set(const Type * const values, const s32 numValues)
    {
      Array<Type> &array = this->get_array();

      AnkiConditionalErrorAndReturnValue(array.IsValid(),
        0, "ArraySlice<Type>::Set", "Array<Type> is not valid");

      const ArraySliceLimits_in1_out0<s32> limits(this->get_ySlice(), this->get_xSlice());

      AnkiConditionalErrorAndReturnValue(limits.isValid,
        0, "ArraySlice<Type>::Set", "Limits is not valid");

      AnkiConditionalErrorAndReturnValue(limits.rawIn1Limits.ySize * limits.rawIn1Limits.xSize == numValues,
        0, "ArraySlice<Type>::Set", "Limits is not valid");

      s32 ci = 0;
      for(s32 iy=0; iy<limits.rawIn1Limits.ySize; iy++) {
        const s32 y = limits.rawIn1Limits.yStart + iy * limits.rawIn1Limits.yIncrement;
        Type * restrict pMat = array.Pointer(y, 0);

        for(s32 ix=0; ix<limits.rawIn1Limits.xSize; ix++) {
          const s32 x = limits.rawIn1Limits.xStart + ix * limits.rawIn1Limits.xIncrement;
          pMat[x] = values[ci];
          ci++;
        }
      }

      AnkiAssert(ci == limits.rawIn1Limits.ySize * limits.rawIn1Limits.xSize);

      return limits.rawIn1Limits.xSize*limits.rawIn1Limits.ySize;
    }

    template<typename Type> template<typename InType> s32 ArraySlice<Type>::SetCast(const ConstArraySliceExpression<Type> &input, bool automaticTranspose)
    {
      AnkiConditionalErrorAndReturnValue(AreValid(*this, input),
        0, "ArraySlice<Type>::Set", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(this->get_array().get_buffer() != input.get_array().get_buffer(),
        0, "ArraySlice<Type>::Set", "Arrays must be in different memory locations");

      ArraySliceLimits_in1_out1<s32> limits(
        input.get_ySlice(), input.get_xSlice(), input.get_isTransposed(),
        this->get_ySlice(), this->get_xSlice());

      if(!limits.isValid) {
        if(automaticTranspose) {
          // If we're allowed to transpose, give it another shot
          limits = ArraySliceLimits_in1_out1<s32> (input.get_ySlice(), input.get_xSlice(), !input.get_isTransposed(), this->get_ySlice(), this->get_xSlice());

          if(!limits.isValid) {
            AnkiError("ArraySlice<Type>::Set", "Subscripted assignment dimension mismatch");
            return 0;
          }
        } else {
          AnkiError("ArraySlice<Type>::Set", "Subscripted assignment dimension mismatch");
          return 0;
        }
      }

      Array<Type> &out1Array = this->get_array();
      const Array<InType> &in1Array = input.get_array();

      if(limits.isSimpleIteration) {
        // If the input isn't transposed, we will do the maximally efficient loop iteration

        for(s32 y=0; y<limits.ySize; y++) {
          const InType * restrict pIn1 = in1Array.Pointer(limits.in1Y, 0);
          Type * restrict pOut1 = out1Array.Pointer(limits.out1Y, 0);

          limits.OuterIncrementTop();

          for(s32 x=0; x<limits.xSize; x++) {
            pOut1[limits.out1X] = static_cast<Type>( pIn1[limits.in1X] );

            limits.out1X += limits.out1_xInnerIncrement;
            limits.in1X += limits.in1_xInnerIncrement;
          }

          limits.OuterIncrementBottom();
        }
      } else {
        for(s32 y=0; y<limits.ySize; y++) {
          Type * restrict pOut1 = out1Array.Pointer(limits.out1Y, 0);

          limits.OuterIncrementTop();

          for(s32 x=0; x<limits.xSize; x++) {
            const InType pIn1 = *in1Array.Pointer(limits.in1Y, limits.in1X);

            pOut1[limits.out1X] = static_cast<Type>( pIn1 );

            limits.out1X += limits.out1_xInnerIncrement;
            limits.in1Y += limits.in1_yInnerIncrement;
          }

          limits.OuterIncrementBottom();
        }
      }

      return limits.ySize*limits.xSize;
    }

    template<typename Type> Array<Type>& ArraySlice<Type>::get_array()
    {
      return this->array;
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression()
      : ConstArraySlice<Type>(), isTransposed(false)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression(const Array<Type> input, bool isTransposed)
      : ConstArraySlice<Type>(input), isTransposed(isTransposed)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression(const ArraySlice<Type> &input, bool isTransposed)
      : ConstArraySlice<Type>(input), isTransposed(isTransposed)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression(const ConstArraySlice<Type> &input, bool isTransposed)
      : ConstArraySlice<Type>(input), isTransposed(isTransposed)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type> ConstArraySliceExpression<Type>::Transpose() const
    {
      ConstArraySliceExpression<Type> expression(*this, !this->get_isTransposed());

      return expression;
    }

    template<typename Type> bool ConstArraySliceExpression<Type>::get_isTransposed() const
    {
      return isTransposed;
    }

    template<typename Type> ArraySliceSimpleLimits<Type>::ArraySliceSimpleLimits(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice)
      : xStart(in1_xSlice.get_start()), xIncrement(in1_xSlice.get_increment()), xSize(in1_xSlice.get_size()),
      yStart(in1_ySlice.get_start()), yIncrement(in1_ySlice.get_increment()), ySize(in1_ySlice.get_size())
    {
    }

    template<typename Type> ArraySliceLimits_in1_out0<Type>::ArraySliceLimits_in1_out0(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice)
      : isValid(true), rawIn1Limits(in1_ySlice, in1_xSlice)
    {
    } // ArraySliceLimits_in1_out0

    template<typename Type> ArraySliceLimits_in1_out1<Type>::ArraySliceLimits_in1_out1(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed, const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice)
      : ySize(out1_ySlice.get_size()), xSize(out1_xSlice.get_size()),
      rawOut1Limits(out1_ySlice, out1_xSlice),
      rawIn1Limits(in1_ySlice, in1_xSlice), in1_isTransposed(in1_isTransposed)
    {
      isValid = false;

      this->out1_xInnerIncrement = this->rawOut1Limits.xIncrement;

      if(!in1_isTransposed) {
        if(rawOut1Limits.xSize == rawIn1Limits.xSize && rawOut1Limits.ySize == rawIn1Limits.ySize) {
          isValid = true;
          isSimpleIteration = true;

          this->in1Y = this->rawIn1Limits.yStart;
          this->out1Y = this->rawOut1Limits.yStart;

          this->in1_xInnerIncrement = this->rawIn1Limits.xIncrement;
          this->in1_yInnerIncrement = 0;
        }
      } else { // if(!in1_isTransposed)
        if(rawOut1Limits.xSize == rawIn1Limits.ySize && rawOut1Limits.ySize == rawIn1Limits.xSize) {
          isValid = true;
          isSimpleIteration = false;

          this->in1X = this->rawIn1Limits.xStart;
          this->out1Y = this->rawOut1Limits.yStart;

          this->in1_xInnerIncrement = 0;
          this->in1_yInnerIncrement = this->rawIn1Limits.yIncrement;
        }
      } // if(!in1_isTransposed) ... else

      if(!isValid) {
        AnkiError("ArraySliceLimits_in1_out1", "Subscripted assignment dimension mismatch");
        return;
      }
    } // ArraySliceLimits_in1_out1

    // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out1 and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in1_out1<Type>::OuterIncrementTop()
    {
      if(isSimpleIteration) {
        this->in1X = this->rawIn1Limits.xStart;
        this->out1X = this->rawOut1Limits.xStart;
      } else { // if(isSimpleIteration)
        this->in1Y = this->rawIn1Limits.yStart;
        this->out1X = this->rawOut1Limits.xStart;
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in1_out1<Type>::OuterIncrementTop()

    // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in1_out1<Type>::OuterIncrementBottom()
    {
      if(isSimpleIteration) {
        this->in1Y += this->rawIn1Limits.yIncrement;
        this->out1Y += this->rawOut1Limits.yIncrement;
      } else { // if(isSimpleIteration)
        this->in1X += this->rawIn1Limits.xIncrement;
        this->out1Y += this->rawOut1Limits.yIncrement;
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in1_out1<Type>::OuterIncrementBottom()

    template<typename Type> ArraySliceLimits_in2_out1<Type>::ArraySliceLimits_in2_out1(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed, const LinearSequence<Type> &in2_ySlice, const LinearSequence<Type> &in2_xSlice, bool in2_isTransposed, const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice)
      : ySize(out1_ySlice.get_size()), xSize(out1_xSlice.get_size()),
      rawOut1Limits(out1_ySlice, out1_xSlice),
      rawIn1Limits(in1_ySlice, in1_xSlice), in1_isTransposed(in1_isTransposed),
      rawIn2Limits(in2_ySlice, in2_xSlice), in2_isTransposed(in2_isTransposed)
    {
      isValid = false;

      this->out1_xInnerIncrement = this->rawOut1Limits.xIncrement;
      this->in1_yInnerIncrement = 0;
      this->in1_xInnerIncrement = 0;
      this->in2_yInnerIncrement = 0;
      this->in2_xInnerIncrement = 0;

      if(!in1_isTransposed && !in2_isTransposed) {
        const bool sizesMatch = (in1_xSlice.get_size() == in2_xSlice.get_size()) && (in1_xSlice.get_size() == out1_xSlice.get_size()) && (in1_ySlice.get_size() == in2_ySlice.get_size()) && (in1_ySlice.get_size() == out1_ySlice.get_size());

        if(sizesMatch) {
          isValid = true;
          isSimpleIteration = true;

          this->in1_xInnerIncrement = this->rawIn1Limits.xIncrement;
          this->in2_xInnerIncrement = this->rawIn2Limits.xIncrement;

          this->in1Y = this->rawIn1Limits.yStart;
          this->in2Y = this->rawIn2Limits.yStart;
          this->out1Y = this->rawOut1Limits.yStart;
        }
      } else { // if(!in1_isTransposed)
        isSimpleIteration = false;

        bool sizesMatch = false;

        if(in1_isTransposed && in2_isTransposed) {
          sizesMatch = (in1_xSlice.get_size() == in2_xSlice.get_size()) && (in1_xSlice.get_size() == out1_ySlice.get_size()) && (in1_ySlice.get_size() == in2_ySlice.get_size()) && (in1_ySlice.get_size() == out1_xSlice.get_size());
          this->in1_yInnerIncrement = this->rawIn1Limits.yIncrement;
          this->in2_yInnerIncrement = this->rawIn2Limits.yIncrement;
        } else if(in1_isTransposed) {
          sizesMatch = (in1_xSlice.get_size() == in2_ySlice.get_size()) && (in1_xSlice.get_size() == out1_ySlice.get_size()) && (in1_ySlice.get_size() == in2_xSlice.get_size()) && (in1_ySlice.get_size() == out1_xSlice.get_size());
          this->in1_yInnerIncrement = this->rawIn1Limits.yIncrement;
          this->in2_xInnerIncrement = this->rawIn2Limits.xIncrement;
        } else if(in2_isTransposed) {
          sizesMatch = (in1_xSlice.get_size() == in2_ySlice.get_size()) && (in1_xSlice.get_size() == out1_xSlice.get_size()) && (in1_ySlice.get_size() == in2_xSlice.get_size()) && (in1_ySlice.get_size() == out1_ySlice.get_size());
          this->in1_xInnerIncrement = this->rawIn1Limits.xIncrement;
          this->in2_yInnerIncrement = this->rawIn2Limits.yIncrement;
        } else {
          AnkiAssert(false); // should not be possible
        }

        if(!sizesMatch) {
          AnkiError("ArraySliceLimits_in2_out1", "Subscripted assignment dimension mismatch");
          return;
        }

        isValid = true;

        this->in1X = this->rawIn1Limits.xStart;
        this->in1Y = this->rawIn1Limits.yStart;
        this->in2X = this->rawIn2Limits.xStart;
        this->in2Y = this->rawIn2Limits.yStart;

        this->out1Y = this->rawOut1Limits.yStart;
      } // if(!in1_isTransposed) ... else
    } // ArraySliceLimits_in1_out1

    // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out1 and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in2_out1<Type>::OuterIncrementTop()
    {
      if(isSimpleIteration) {
        this->out1X = this->rawOut1Limits.xStart;
        this->in1X = this->rawIn1Limits.xStart;
        this->in2X = this->rawIn2Limits.xStart;
      } else { // if(isSimpleIteration)
        this->out1X = this->rawOut1Limits.xStart;

        if(in1_isTransposed) {
          this->in1Y = this->rawIn1Limits.yStart;
        } else {
          this->in1X = this->rawIn1Limits.xStart;
        }

        if(in2_isTransposed) {
          this->in2Y = this->rawIn2Limits.yStart;
        } else {
          this->in2X = this->rawIn2Limits.xStart;
        }
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in2_out1<Type>::OuterIncrementTop()

    // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in2_out1<Type>::OuterIncrementBottom()
    {
      if(isSimpleIteration) {
        this->in1Y += this->rawIn1Limits.yIncrement;
        this->in2Y += this->rawIn2Limits.yIncrement;
        this->out1Y += this->rawOut1Limits.yIncrement;
      } else { // if(isSimpleIteration)
        this->out1Y += this->rawOut1Limits.yIncrement;

        if(in1_isTransposed) {
          this->in1X += this->rawIn1Limits.xIncrement;
        } else {
          this->in1Y += this->rawIn1Limits.yIncrement;
        }

        if(in2_isTransposed) {
          this->in2X += this->rawIn2Limits.xIncrement;
        } else {
          this->in2Y += this->rawIn2Limits.yIncrement;
        }
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in2_out1<Type>::OuterIncrementBottom()
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_
