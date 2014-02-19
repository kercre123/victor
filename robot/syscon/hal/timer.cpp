#include "timer.h"
#include "anki/cozmo/robot/hal.h"
#include "nrf.h"

namespace
{
  u32 m_low = 0;
  u8 m_high = 0;
}

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      u32 GetMicroCounter()
      {
        if (NRF_RTC1->COUNTER < m_low)
        {
          m_high++;
        }
        
        m_low = NRF_RTC1->COUNTER;
        u32 ticks = ((u32)m_high << 24) | m_low;
        
        // TODO: This is really 30.517
        return ticks * 30;
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

void TimerInit()
{
  // The synthesized LFCLK requires the 16MHz HFCLK to be running, since there's no
  // external crystal/oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  
  // Wait for the external oscillator to start
  while (!NRF_CLOCK->EVENTS_HFCLKSTARTED)
    ;
  
  // Enabling constant latency as indicated by PAN 11 "HFCLK: Base current with HFCLK 
  // running is too high" found at Product Anomaly document found at
  // https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822/#Downloads
  
  // This setting will ensure correct behaviour when routing TIMER events through 
  // PPI and low power mode simultaneously.
  NRF_POWER->TASKS_CONSTLAT = 1;
  
  // Change the source for LFCLK
  NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_Synth;
  
  // Start the LFCLK
  NRF_CLOCK->TASKS_LFCLKSTART = 1;
  
  // Power on the peripheral
  NRF_RTC1->POWER = 1;
  
  // Stop the RTC so it can be reset
  NRF_RTC1->TASKS_STOP = 1;
  NRF_RTC1->TASKS_CLEAR = 1;
  
  m_low = m_high = 0;
  
  // NOTE: When using the LFCLK with prescaler = 0, we only get 30.517 us
  // resolution. This should still provide enough for this chip/board.  
  NRF_RTC1->PRESCALER = 0;
  
  // Start the RTC
  NRF_RTC1->TASKS_START = 1;
}
