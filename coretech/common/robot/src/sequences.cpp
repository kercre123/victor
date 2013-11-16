#include "anki/common/robot/sequences.h"

namespace Anki
{
  namespace Embedded
  {
    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end)
    {
      const f32 smallestIncrement = 0.00001f;

      assert(ABS(increment) > smallestIncrement);

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
      const s32 size = static_cast<s32>(floorf((validRange+incrementMagnitude)/incrementMagnitude));

      return size;
    }

    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end)
    {
      const f64 smallestIncrement = 0.00001;

      assert(ABS(increment) > smallestIncrement);

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
      const s32 size = static_cast<s32>(floorf(static_cast<f32>((validRange+incrementMagnitude)/incrementMagnitude)));

      return size;
    }

    LinearSequence<s32> IndexSequence(s32 arraySize)
    {
      return IndexSequence<s32>(0, 1, arraySize-1, arraySize);
    }
  } // namespace Embedded
} //namespace Anki