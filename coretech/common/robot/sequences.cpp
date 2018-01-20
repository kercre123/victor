/**
File: sequences.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/sequences.h"
#include "coretech/common/robot/errorHandling.h"

static s32 computeSize_u32(const u32 start, const u32 increment, const u32 end)
{
  if(start == end) {
    return 1;
  } else if(increment == 0) {
    return 0;
  }

  // 12:1:10
  if(start > end) {
    return 0;
  }

  const u32 validRange = end - start;
  const s32 size = (validRange+increment) / increment;

  return size;
}

template<typename Type> s32 computeSize_float(const Type start, const Type increment, const Type end)
{
  const Type smallestIncrement = static_cast<Type>(1e-20);

  if(ABS(end-start) <= smallestIncrement) {
    return 1;
  } else {
    if(ABS(increment) <= smallestIncrement) {
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

  const Type diff = (validRange+incrementMagnitude) / incrementMagnitude;
  const s32 size = static_cast<s32>(floorf(static_cast<f32>(diff)));

  AnkiConditionalErrorAndReturnValue(size >= 0,
    0, "LinearSequence<Type>::computeSize", "size estimation failed");

  return size;
}

namespace Anki
{
  namespace Embedded
  {
    template<> s32 LinearSequence<u8>::computeSize(const u8 start, const u8 increment, const u8 end)
    {
      return computeSize_u32(start, increment,end);
    }

    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end)
    {
      return computeSize_float<f32>(start, increment, end);
    }

    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end)
    {
      return computeSize_float<f64>(start, increment, end);
    }

    LinearSequence<s32> IndexSequence(s32 arraySize)
    {
      return IndexSequence<s32>(0, 1, arraySize-1, arraySize);
    }
  } // namespace Embedded
} //namespace Anki
