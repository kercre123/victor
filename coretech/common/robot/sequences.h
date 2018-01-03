/**
File: sequences.h
Author: Peter Barnum
Created: 2013

Definitions of sequences_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_
#define _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_

#include "coretech/common/robot/sequences_declarations.h"
#include "coretech/common/robot/arraySlices.h"
#include "coretech/common/robot/flags.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark

    template<typename Type> LinearSequence<Type>::LinearSequence()
      : size(-1), start(-1), increment(static_cast<Type>(-1))
    {
      this->size = -1;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type end)
      : start(start), increment(1)
    {
      this->size = computeSize(this->start, this->increment, end);
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type increment, const Type end)
      : start(start), increment(increment)
    {
      this->size = computeSize(this->start, this->increment, end);
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type increment, const Type /*end*/, const s32 size)
      : size(size), start(start), increment(increment)
    {
    }

    template<typename Type> Array<Type> LinearSequence<Type>::Evaluate(MemoryStack &memory, const Flags::Buffer flags) const
    {
      const s32 numRows = 1;
      const s32 numCols = this->get_size();

      Array<Type> out(numRows, numCols, memory, flags);

      this->Evaluate(out);

      return out;
    }

    template<typename Type> Result LinearSequence<Type>::Evaluate(ArraySlice<Type> out) const
    {
      const s32 size = this->get_size();
      Array<Type> &outArray = out.get_array();

      AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "LinearSequence<Type>::Evaluate", "Invalid array");

      AnkiConditionalErrorAndReturnValue(out.get_ySlice().get_size()==1 && out.get_xSlice().get_size()==size,
        RESULT_FAIL_INVALID_OBJECT, "LinearSequence<Type>::Evaluate", "Invalid array");

      const Type sequenceStartValue = this->get_start();
      const Type sequenceIncrement = this->get_increment();

      const s32 yStart = out.get_ySlice().get_start();

      const s32 xStart = out.get_xSlice().get_start();
      const s32 xIncrement = out.get_xSlice().get_increment();
      const s32 xSize = out.get_xSlice().get_size();

      Type * pOut = outArray.Pointer(yStart,0);
      Type curSequenceValue = sequenceStartValue;
      for(s32 ix=0; ix<xSize; ix++) {
        const s32 x = xStart + ix * xIncrement;
        pOut[x] = curSequenceValue;
        curSequenceValue += sequenceIncrement;
      }

      return RESULT_OK;
    }

    template<typename Type> Type LinearSequence<Type>::get_start() const
    {
      return start;
    }

    template<typename Type> Type LinearSequence<Type>::get_increment() const
    {
      return increment;
    }

    template<typename Type> s32 LinearSequence<Type>::get_size() const
    {
      return size;
    }

    template<typename Type> s32 LinearSequence<Type>::computeSize(const Type start, const Type increment, const Type end)
    {
      if(start == end) {
        return 1;
      } else {
        if(ABS(increment) <= Flags::numeric_limits<Type>::epsilon()) {
          return 0;
        }
      }

      // 10:-1:12
      if(increment < 0 && start < end) {
        return 0;
      }

      // 12:1:10
      if(increment > 0 && start > end) {
        return 0;
      }

      const Type minLimit = MIN(start, end);
      const Type maxLimit = MAX(start, end);
      const Type incrementMagnitude = ABS(increment);

      const Type validRange = maxLimit - minLimit;
      const s32 size = (validRange+incrementMagnitude) / incrementMagnitude;

      AnkiConditionalErrorAndReturnValue(size >= 0,
        0, "LinearSequence<Type>::computeSize", "size estimation failed");

      return size;
    }

    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type end, s32 arraySize)
    {
      return IndexSequence(start, 1, end, arraySize);
    }

    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type increment, Type end, s32 arraySize)
    {
      // A negative value means (end-value)
      if(start < 0)
        start += arraySize;

      AnkiAssert(start >=0 && start < arraySize);

      // A negative value means (end-value)
      if(end < 0)
        end += arraySize;

      AnkiAssert(end >=0 && end < arraySize);

      LinearSequence<Type> sequence(start, increment, end);

      return sequence;
    }

    template<typename Type> LinearSequence<Type> Linspace(const Type start, const Type end, const s32 size)
    {
      Type increment;

      LinearSequence<Type> sequence;

      if(ABS(end-start) <= Flags::numeric_limits<Type>::epsilon()) {
        sequence = LinearSequence<Type>(start, 0, end, size);
      } else {
        if(size <= 0) {
          // Empty sequence
          sequence = LinearSequence<Type>(start, 1, end, 0);
        } else if(size == 1) {
          // If size == 1, match output with Matlab
          sequence = LinearSequence<Type>(end, 1, end, size);
        } else {
          increment = (end-start) / (size-1);
          sequence = LinearSequence<Type>(start, increment, end, size);
        }
      }

      return sequence;
    }

    template<typename Type> Meshgrid<Type>::Meshgrid()
      : xGridVector(), yGridVector()
    {
    }

    template<typename Type> Meshgrid<Type>::Meshgrid(const LinearSequence<Type> xGridVector, const LinearSequence<Type> yGridVector)
      : xGridVector(xGridVector), yGridVector(yGridVector)
    {
    }

    template<typename Type> Array<Type> Meshgrid<Type>::EvaluateX1(bool isOutColumnMajor, MemoryStack &memory, const Flags::Buffer flags) const
    {
      const s32 numRows = 1;
      const s32 numCols = this->xGridVector.get_size()*this->yGridVector.get_size();

      Array<Type> out(numRows, numCols, memory, flags);

      this->EvaluateX1(isOutColumnMajor, out);

      return out;
    }

    template<typename Type> Array<Type> Meshgrid<Type>::EvaluateX2(MemoryStack &memory, const Flags::Buffer flags) const
    {
      const s32 numRows = this->yGridVector.get_size();
      const s32 numCols = this->xGridVector.get_size();

      Array<Type> out(numRows, numCols, memory, flags);

      this->EvaluateX2(out);

      return out;
    }

    template<typename Type> Array<Type> Meshgrid<Type>::EvaluateY1(bool isOutColumnMajor, MemoryStack &memory, const Flags::Buffer flags) const
    {
      const s32 numRows = 1;
      const s32 numCols = this->xGridVector.get_size()*this->yGridVector.get_size();

      Array<Type> out(numRows, numCols, memory, flags);

      this->EvaluateY1(isOutColumnMajor, out);

      return out;
    }

    template<typename Type> Array<Type> Meshgrid<Type>::EvaluateY2(MemoryStack &memory, const Flags::Buffer flags) const
    {
      const s32 numRows = this->yGridVector.get_size();
      const s32 numCols = this->xGridVector.get_size();

      Array<Type> out(numRows, numCols, memory, flags);

      this->EvaluateY2(out);

      return out;
    }

    template<typename Type> Result Meshgrid<Type>::EvaluateX1(bool isOutColumnMajor, ArraySlice<Type> out) const
    {
      const s32 xGridSize = this->xGridVector.get_size();
      const s32 yGridSize = this->yGridVector.get_size();
      const s32 numElements = xGridSize * yGridSize;

      Array<Type> &outArray = out.get_array();

      AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Meshgrid<Type>::Evaluate", "Invalid array");

      AnkiConditionalErrorAndReturnValue(out.get_ySlice().get_size()==1 && out.get_xSlice().get_size()==numElements,
        RESULT_FAIL_INVALID_SIZE, "Meshgrid<Type>::Evaluate", "Array out is not the correct size");

      //const s32 outXStart = out.get_xSlice().get_start();
      const s32 outYStart = out.get_ySlice().get_start();

      const s32 outXIncrement = out.get_xSlice().get_increment();
      //const s32 outYIncrement = out.get_ySlice().get_increment();

      // Matlab equivalent: [x,y] = meshgrid(1:N,1:M)

      const Type sequenceStartValue = this->xGridVector.get_start();
      const Type sequenceIncrement = this->xGridVector.get_increment();

      if(isOutColumnMajor) {
        // Matlab equivalent: x(:)

        Type * pOut = outArray.Pointer(outYStart,0);
        s32 curOutX = out.get_xSlice().get_start();
        Type curSequenceValue = sequenceStartValue;

        for(s32 x=0; x<xGridSize; x++) {
          for(s32 y=0; y<yGridSize; y++) {
            pOut[curOutX] = curSequenceValue;

            curOutX += outXIncrement;
          }

          curSequenceValue += sequenceIncrement;
        }
      } else { // if(isOutColumnMajor)
        // Matlab equivalent: x=x'; x(:)

        Type * pOut = outArray.Pointer(outYStart,0);
        s32 curOutX = out.get_xSlice().get_start();

        for(s32 y=0; y<yGridSize; y++) {
          Type curSequenceValue = sequenceStartValue;

          for(s32 x=0; x<xGridSize; x++) {
            pOut[curOutX] = curSequenceValue;

            curOutX += outXIncrement;
            curSequenceValue += sequenceIncrement;
          }
        }
      } // if(isOutColumnMajor) ... else

      return RESULT_OK;
    }

    template<typename Type> Result Meshgrid<Type>::EvaluateX2(ArraySlice<Type> out) const
    {
      const s32 xGridSize = this->xGridVector.get_size();
      const s32 yGridSize = this->yGridVector.get_size();
      //const s32 numElements = xGridSize * yGridSize;

      Array<Type> &outArray = out.get_array();

      AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Meshgrid<Type>::Evaluate", "Invalid array");

      AnkiConditionalErrorAndReturnValue(out.get_ySlice().get_size()==yGridSize && out.get_xSlice().get_size()==xGridSize,
        RESULT_FAIL_INVALID_SIZE, "Meshgrid<Type>::Evaluate", "Array out is not the correct size");

      const s32 outXStart = out.get_xSlice().get_start();
      const s32 outYStart = out.get_ySlice().get_start();

      const s32 outXIncrement = out.get_xSlice().get_increment();
      const s32 outYIncrement = out.get_ySlice().get_increment();

      // Matlab equivalent: [x,y] = meshgrid(1:N,1:M)

      const Type sequenceStartValue = this->xGridVector.get_start();
      const Type sequenceIncrement = this->xGridVector.get_increment();

      // Repeat the numbers in the vertical direction

      s32 curOutY = outYStart;

      for(s32 y=0; y<yGridSize; y++) {
        Type * pOut = outArray.Pointer(curOutY,0);
        s32 curOutX = outXStart;
        Type curSequenceValue = sequenceStartValue;

        for(s32 x=0; x<xGridSize; x++) {
          pOut[curOutX] = curSequenceValue;

          curOutX += outXIncrement;
          curSequenceValue += sequenceIncrement;
        } // for(s32 x=0; x<xGridSize; x++)

        curOutY += outYIncrement;
      } // for(s32 y=0; y<yGridSize; y++)

      return RESULT_OK;
    }

    template<typename Type> Result Meshgrid<Type>::EvaluateY1(bool isOutColumnMajor, ArraySlice<Type> out) const
    {
      const s32 xGridSize = this->xGridVector.get_size();
      const s32 yGridSize = this->yGridVector.get_size();
      const s32 numElements = xGridSize * yGridSize;

      Array<Type> &outArray = out.get_array();

      AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Meshgrid<Type>::Evaluate", "Invalid array");

      AnkiConditionalErrorAndReturnValue(out.get_ySlice().get_size()==1 && out.get_xSlice().get_size()==numElements,
        RESULT_FAIL_INVALID_SIZE, "Meshgrid<Type>::Evaluate", "Array out is not the correct size");

      //const s32 outXStart = out.get_xSlice().get_start();
      const s32 outYStart = out.get_ySlice().get_start();

      const s32 outXIncrement = out.get_xSlice().get_increment();
      //const s32 outYIncrement = out.get_ySlice().get_increment();

      //Type * pOut = outArray.Pointer(outYStart,0);

      // Matlab equivalent: [x,y] = meshgrid(1:N,1:M)

      const Type sequenceStartValue = this->yGridVector.get_start();
      const Type sequenceIncrement = this->yGridVector.get_increment();

      if(isOutColumnMajor) {
        // Matlab equivalent: y(:)

        Type * pOut = outArray.Pointer(outYStart,0);
        s32 curOutX = out.get_xSlice().get_start();

        for(s32 x=0; x<xGridSize; x++) {
          Type curSequenceValue = sequenceStartValue;

          for(s32 y=0; y<yGridSize; y++) {
            pOut[curOutX] = curSequenceValue;

            curOutX += outXIncrement;
            curSequenceValue += sequenceIncrement;
          }
        }
      } else { // if(isOutColumnMajor)
        // Matlab equivalent: y=y'; y(:)

        Type * pOut = outArray.Pointer(outYStart,0);
        s32 curOutX = out.get_xSlice().get_start();
        Type curSequenceValue = sequenceStartValue;

        for(s32 y=0; y<yGridSize; y++) {
          for(s32 x=0; x<xGridSize; x++) {
            pOut[curOutX] = curSequenceValue;

            curOutX += outXIncrement;
          }

          curSequenceValue += sequenceIncrement;
        }
      } // if(isOutColumnMajor) ... else

      return RESULT_OK;
    }

    template<typename Type> Result Meshgrid<Type>::EvaluateY2(ArraySlice<Type> out) const
    {
      const s32 xGridSize = this->xGridVector.get_size();
      const s32 yGridSize = this->yGridVector.get_size();
      //const s32 numElements = xGridSize * yGridSize;

      Array<Type> &outArray = out.get_array();

      AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Meshgrid<Type>::Evaluate", "Invalid array");

      AnkiConditionalErrorAndReturnValue(out.get_ySlice().get_size()==yGridSize && out.get_xSlice().get_size()==xGridSize,
        RESULT_FAIL_INVALID_SIZE, "Meshgrid<Type>::Evaluate", "Array out is not the correct size");

      const s32 outXStart = out.get_xSlice().get_start();
      const s32 outYStart = out.get_ySlice().get_start();

      const s32 outXIncrement = out.get_xSlice().get_increment();
      const s32 outYIncrement = out.get_ySlice().get_increment();

      //Type * pOut = outArray.Pointer(outYStart,0);

      // Matlab equivalent: [x,y] = meshgrid(1:N,1:M)

      const Type sequenceStartValue = this->yGridVector.get_start();
      const Type sequenceIncrement = this->yGridVector.get_increment();

      // Repeat the numbers in the horizontal direction

      s32 curOutY = outYStart;
      Type curSequenceValue = sequenceStartValue;

      for(s32 y=0; y<yGridSize; y++) {
        Type * pOut = outArray.Pointer(curOutY,0);
        s32 curOutX = outXStart;

        for(s32 x=0; x<xGridSize; x++) {
          pOut[curOutX] = curSequenceValue;

          curOutX += outXIncrement;
        } // for(s32 x=0; x<xGridSize; x++)

        curOutY += outYIncrement;
        curSequenceValue += sequenceIncrement;
      } // for(s32 y=0; y<yGridSize; y++)

      return RESULT_OK;
    }

    template<typename Type> s32 Meshgrid<Type>::get_numElements() const
    {
      const s32 numElements = xGridVector.get_size() * yGridVector.get_size();
      return numElements;
    }

    template<typename Type> inline const LinearSequence<Type>& Meshgrid<Type>::get_xGridVector() const
    {
      return xGridVector;
    }

    template<typename Type> inline const LinearSequence<Type>& Meshgrid<Type>::get_yGridVector() const
    {
      return yGridVector;
    }

    // #pragma mark --- Specializations ---

    template<> s32 LinearSequence<u8>::computeSize(const u8 start, const u8 increment, const u8 end);
    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end);
    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_
