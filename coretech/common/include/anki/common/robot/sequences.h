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

    // TODO: instantiate for float
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

#pragma mark --- Specializations ---

    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end);
    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_