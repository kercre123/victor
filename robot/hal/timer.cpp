#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static const u32 FREQUENCY = 180;

      static u32 m_count = 0;

      u32 GetMicroCounter()
      {
        u32 difference;
        u32 count = *(volatile u32*)TIM_FREE_CNT_ADR;
        if (count < m_count)
        {
          difference = (u32)((0x100000000LL) + (u64)count - (u64)m_count);
        } else {
          difference = count - m_count;
        }

        m_count = count;

        return difference / FREQUENCY;
      }
    }
  }
}

