#include <string.h>
#include <stdint.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
}

#include "hardware.h"
#include "radio.h"
#include "lights.h"
#include "timer.h"
#include "messages.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Backpack {
  void update() {}
}

static const int MAX_PROPS = 7
  ;
static unsigned int rssi[MAX_PROPS];

static inline unsigned int abs(int x) { return x < 0 ? -x : x; }

static bool minRSSI(unsigned int measured, int& slot) {
  int maximum = 0;
  
  for (int i = 0; i < MAX_PROPS; i++) {
    if (rssi[i] >= maximum) {
      slot = i;
      maximum = rssi[i];
    }
  }
  
  if (measured < maximum) {
    rssi[slot] = measured;
    return true;
  }
  
  return false;
}

uint32_t isqrt(uint32_t a_nInput)
{
  uint32_t op  = a_nInput;
  uint32_t res = 0;
  uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type


  // "one" starts at the highest power of four <= than the argument.
  while (one > op)
  {
      one >>= 2;
  }

  while (one != 0)
  {
    if (op >= res + one)
    {
      op = op - (res + one);
      res = res +  2 * one;
    }
    res >>= 1;
    one >>= 2;
  }
  return res;
}

bool Anki::Cozmo::HAL::RadioSendMessage(const void *buffer, const u16 size, const u8 msgID) {
  using namespace Anki::Cozmo::RobotInterface;

  switch (msgID) {
  case RobotToEngine::Tag_activeObjectDiscovered:
    {
      const ObjectDiscovered* discovered = (ObjectDiscovered*) buffer;
      int slot;

      if (abs(discovered->rssi) > 50) break ;

      if (minRSSI(abs(discovered->rssi), slot)) {
        Radio::assignProp(slot, discovered->factory_id);
      }

      break ;
    }
  case EngineToRobot::Tag_getPropState:
    {
      const PropState* state = (PropState*) buffer;
      LightState colors[4];
      LightState allState;

      #ifdef PRETTY_SHADES
      int r = state->x, g = state->y, b = state->z;
      int len = isqrt(r*r+g*g+b*b);

      r = r * 0x70 / len + 0x80;
      g = g * 0x70 / len + 0x80;
      b = b * 0x70 / len + 0x80;
      
      #else
      int r = state->x, g = state->y, b = state->z;
      r = abs(r); g = abs(g); b = abs(b); 

      if (!r && !g && !b) {
        r = 0xFF; g = 0xFF; b = 0xFF;
      }
      else if (abs(state->x) < 6 && abs(state->z) < 6 && state->y > 58) {
        r = 0; g = 0xFF; b = 0;
      }
      else {
        r = 0xFF; g = 0; b = 0;
      }

      #endif

      uint16_t color = PACK_COLORS(0, r, g, b);
      allState.onColor = color;
      allState.offColor = color;
      
      for (int i = 0; i < 4; i++) {
        memcpy(&colors[i], &allState, sizeof(LightState));
      }
            
      Radio::setPropLights(state->slot, colors);
      break ;
    }
  }
  
  return true;
}

int main(void)
{
  memset(rssi, 0xFF, sizeof(rssi));
  
  // The synthesized LFCLK requires the 16MHz HFCLK to be running, since there's no
  // external crystal/oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  
  // Wait for the external oscillator to start
  while (!NRF_CLOCK->EVENTS_HFCLKSTARTED)
    ;
  
  // Change the source for LFCLK and start it
  NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_Synth;
  NRF_CLOCK->TASKS_LFCLKSTART = 1;

  // Initialize our scheduler
  RTOS::init();
  Timer::init();

  // Setup all tasks
  Radio::init();
  Radio::advertise();
  Radio::setLightGamma(0x80);

  Timer::start();

  __enable_irq();

  // Run forever, because we are awesome.
  for (;;) {
    static uint8_t last = 0;
    uint8_t cur = NRF_RTC1->COUNTER;
    
    if (cur == last) continue ;
    last = cur;

    for (int i = 0; i < WDOG_TOTAL_CHANNELS; i++) {
      RTOS::kick((watchdog_channels)i);
    }
    
    // This means that if the crypto engine is running, the lights will stop pulsing. 
    Lights::manage();
  }
}
