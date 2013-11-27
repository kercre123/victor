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
        swcLeonDisableTraps();
        u32 tick = *(volatile u32*)TIM_FREE_CNT_ADR;
        u32 increment = (tick - m_lastTick) / FREQUENCY;
        m_microSeconds += increment;
        m_lastTick += increment * FREQUENCY;
        swcLeonEnableTraps();

        return m_microSeconds;
      }

      void MicroWait(u32 microseconds)
      {
        u32 now = GetMicroCounter();
        while ((GetMicroCounter() - now) < microseconds)
          ;
      }
    }
  }
}

