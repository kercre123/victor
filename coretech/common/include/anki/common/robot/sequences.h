/**
File: sequences.h
Author: Peter Barnum
Created: 2013

A Sequence is a mathematically-defined, ordered list. The sequence classes allow for operations on sequences, without requiring them to be explicitly evaluated.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_
#define _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_

#include "anki/common/robot/sequences_declarations.h"
#include "anki/common/robot/arraySlices.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---

    template<typename Type> LinearSequence<Type>::LinearSequence()
      : start(-1), increment(static_cast<Type>(-1)), end(-1)
    {
      this->size = -1;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type end)
      : start(start), increment(static_cast<Type>(1))
    {
      this->size = computeSize(this->start, this->increment, end);
      this->end = this->start + (this->size-1) * this->increment;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type increment, const Type end)
      : start(start), increment(increment)
    {
      this->size = computeSize(this->start, this->increment, end);
      this->end = this->start + (this->size-1) * this->increment;
    }

    template<typename Type> Array<Type> LinearSequence<Type>::Evaluate(MemoryStack &memory, const Flags::Buffer flags) const
    {
      const s32 numRows = 1;
      const s32 numCols = this->get_size();

      Array<Type> array(numRows, numCols, memory, flags);

      this->Evaluate(array);

      return array;
    }

    template<typename Type> Result LinearSequence<Type>::Evaluate(ArraySlice<Type> array) const
    {
      const s32 size = this->get_size();
      Array<Type> &outArray = array.get_array();

      AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
        RESULT_FAIL, "LinearSequence<Type>::Evaluate", "Invalid array");

      AnkiConditionalErrorAndReturnValue(array.get_ySlice().get_size()==1 && array.get_xSlice().get_size()==size,
        RESULT_FAIL, "LinearSequence<Type>::Evaluate", "Invalid array");

      const Type sequenceStartValue = this->get_start();
      const Type sequenceIncrement = this->get_increment();

      const s32 yStart = array.get_ySlice().get_start();

      const s32 xStart = array.get_xSlice().get_start();
      const s32 xIncrement = array.get_xSlice().get_increment();
      const s32 xEnd = array.get_xSlice().get_end();

      Type * pArray = outArray.Pointer(yStart,0);
      Type curValue = sequenceStartValue;
      for(s32 x=xStart; x<=xEnd; x+=xIncrement) {
        pArray[x] = curValue;
        curValue += sequenceIncrement;
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

    template<typename Type> Type LinearSequence<Type>::get_end() const
    {
      return end;
    }

    template<typename Type> s32 LinearSequence<Type>::get_size() const
    {
      return size;
    }

    template<typename Type> s32 LinearSequence<Type>::computeSize(const Type start, const Type increment, const Type end)
    {
      assert(increment != static_cast<Type>(0));

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

      const Type validRange = maxLimit - minLimit + 1;
      const s32 size = (validRange+incrementMagnitude-1)/incrementMagnitude;

      return size;
    }

    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type end, s32 arraySize)
    {
      return IndexSequence(start, static_cast<Type>(1), end, arraySize);
    }

    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type increment, Type end, s32 arraySize)
    {
      if(start < 0)
        start += arraySize;

      assert(start >=0 && start < arraySize);

      if(end < 0)
        end += arraySize;

      assert(end >=0 && end < arraySize);

      LinearSequence<Type> sequence(start, increment, end);

      return sequence;
    }

    template<typename Type> LinearSequence<Type> Linspace(const Type start, const Type end, const s32 size)
    {
      const Type increment = (end-start) / (size-1);
      LinearSequence<Type> sequence(start, increment, end);

      // If Type is not a float, and the sequence is the wrong size, just give up
      if((sequence.get_size() != size) && static_cast<Type>(1e-5)==0) {
        AnkiError("Linspace", "Size is incorrect, probably because the Type is an integer type");
        return sequence;
      }

      // The size of the generated sequence may be incorrect, due to numerical precision. If so, try
      // to tweak it.
      // TODO: check if this actually can happen
      if(sequence.get_size() < size) {
        for(s32 i=0; i<128; i++) {
          const Type offset = static_cast<Type>(1e-15) * static_cast<Type>(1 << (i+1));
          sequence = LinearSequence<Type>(start, increment + offset, end);

          if(sequence.get_size() >= size)
            break;
        }
      } else if(sequence.get_size() > size) {
        for(s32 i=0; i<128; i++) {
          const Type offset = static_cast<Type>(1e-15) * static_cast<Type>(1 << (i+1));
          sequence = LinearSequence<Type>(start, increment - offset, end);

          if(sequence.get_size() <= size)
            break;
        }
      }

      AnkiConditionalErrorAndReturnValue(sequence.get_size() == size,
        sequence, "Linspace", "Could not set sequence to have the correct size.");

      return sequence;
    }

#pragma mark --- Specializations ---

    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end);
    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_