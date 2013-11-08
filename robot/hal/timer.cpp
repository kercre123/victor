#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // TODO: Revisit this when we do other power modes
      static const u32 FREQUENCY = 180;

      static u32 m_microSeconds = 0;
      static u32 m_lastTick = 0;

      u32 GetMicroCounter()
      {
        u32 tick = *(volatile u32*)TIM_FREE_CNT_ADR;
        m_microSeconds += (tick - m_lastTick)/ FREQUENCY;
        m_lastTick = tick;

        return m_microSeconds;
      }
    }
  }
}

