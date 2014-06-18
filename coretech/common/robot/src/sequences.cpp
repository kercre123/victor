/**
File: sequences.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/sequences.h"
#include "anki/common/robot/errorHandling.h"

namespace Anki
{
  namespace Embedded
  {
    template<> s32 LinearSequence<u8>::computeSize(const u8 start, const u8 increment, const u8 end)
    {
      if(start == end) {
        // WARNING: size is ignored
        //AnkiAssert(increment == 0);
        return 1;
      } else {
        AnkiAssert(increment != 0);
      }

      // 12:1:10
      if(increment > 0 && start > end) {
        return 0;
      }

      const u8 minLimit = MIN(start, end);
      const u8 maxLimit = MAX(start, end);
      const u8 incrementMagnitude = increment;

      const u8 validRange = maxLimit - minLimit + 1;
      const s32 size = (validRange+incrementMagnitude-1)/incrementMagnitude;

      return size;
    }

    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end)
    {
      const f32 smallestIncrement = static_cast<f32>(1e-8);

      if(ABS(end-start) < smallestIncrement) {
        // WARNING: size is ignored
        //AnkiAssert(ABS(increment) <= smallestIncrement);
        return 1;
      } else {
        AnkiConditionalErrorAndReturnValue(ABS(increment) > smallestIncrement,
          0, "LinearSequence<f32>::computeSize", "increment is too small");
      }

      // 10:-1:12
      if(increment < 0.0f && start < end) {
        return 0;
      }

      // 12:1:10
      if(increment > 0.0f && start > end) {
        return 0;
      }

      const f32 minLimit = MIN(start, end);
      const f32 maxLimit = MAX(start, end);
      const f32 incrementMagnitude = ABS(increment);

      const f32 validRange = maxLimit - minLimit;

      const f32 diff = (validRange+incrementMagnitude)/incrementMagnitude;
      const s32 size = static_cast<s32>(FLT_FLOOR(diff));

      //CoreTechPrint("(%f-%f)=%f %f %f %d %f %f 0x%x\n", maxLimit, minLimit, validRange, incrementMagnitude, incrementMagnitude, size, (validRange+incrementMagnitude)/incrementMagnitude, diff, *((int*)(&diff)));

      AnkiConditionalErrorAndReturnValue(size >= 0,
        0, "LinearSequence<f32>::computeSize", "size estimation failed");

      return size;
    }

    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end)
    {
      const f64 smallestIncrement = static_cast<f64>(1e-8);

      if(ABS(end-start) < smallestIncrement) {
        // WARNING: size is ignored
        //AnkiAssert(ABS(increment) <= smallestIncrement);
        return 1;
      } else {
        AnkiConditionalErrorAndReturnValue(ABS(increment) > smallestIncrement,
          0, "LinearSequence<f64>::computeSize", "increment is too small");
      }

      // 10:-1:12
      if(increment < 0.0 && start < end) {
        return 0;
      }

      // 12:1:10
      if(increment > 0.0 && start > end) {
        return 0;
      }

      const f64 minLimit = MIN(start, end);
      const f64 maxLimit = MAX(start, end);
      const f64 incrementMagnitude = ABS(increment);

      const f64 validRange = maxLimit - minLimit;
      const s32 size = static_cast<s32>(FLT_FLOOR(static_cast<f32>((validRange+incrementMagnitude)/incrementMagnitude)));

      AnkiConditionalErrorAndReturnValue(size >= 0,
        0, "LinearSequence<f32>::computeSize", "size estimation failed");

      return size;
    }

    LinearSequence<s32> IndexSequence(s32 arraySize)
    {
      return IndexSequence<s32>(0, 1, arraySize-1, arraySize);
    }
  } // namespace Embedded
} //namespace Anki
