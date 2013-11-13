#include "anki/common/robot/sequences.h"

namespace Anki
{
  namespace Embedded
  {
    template<> s32 LinearSequence<f32>::computeSize(const f32 startValue, const f32 increment, const f32 endValue)
    {
      const f32 smallestIncrement = 0.00001f;

      assert(ABS(increment) > smallestIncrement);

      // 10:-1:12
      if(increment < 0.0f && startValue < endValue) {
        return 0;
      }

      // 12:1:10
      if(increment > 0.0f && startValue > endValue) {
        return 0;
      }

      const f32 minLimit = MIN(startValue, endValue);
      const f32 maxLimit = MAX(startValue, endValue);
      const f32 incrementMagnitude = ABS(increment);

      const f32 validRange = maxLimit - minLimit;
      const s32 size = static_cast<s32>(floor((validRange+incrementMagnitude)/incrementMagnitude));

      return size;
    }

    template<> s32 LinearSequence<f64>::computeSize(const f64 startValue, const f64 increment, const f64 endValue)
    {
      const f64 smallestIncrement = 0.00001;

      assert(ABS(increment) > smallestIncrement);

      // 10:-1:12
      if(increment < 0.0 && startValue < endValue) {
        return 0;
      }

      // 12:1:10
      if(increment > 0.0 && startValue > endValue) {
        return 0;
      }

      const f64 minLimit = MIN(startValue, endValue);
      const f64 maxLimit = MAX(startValue, endValue);
      const f64 incrementMagnitude = ABS(increment);

      const f64 validRange = maxLimit - minLimit;
      const s32 size = static_cast<s32>(floor((validRange+incrementMagnitude)/incrementMagnitude));

      return size;
    }

    LinearSequence<s32> IndexSequence(s32 arraySize)
    {
      return IndexSequence<s32>(0, 1, arraySize-1, arraySize);
    }
  } // namespace Embedded
} //namespace Anki