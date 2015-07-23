#ifndef ANKI_COZMO_SHARED_LED_TYPES_H
#define ANKI_COZMO_SHARED_LED_TYPES_H

#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {
  
  // LED identifiers and colors
  /*
  // Updated for "neutral" (non-hardware specific) order in 2.1
  typedef enum {
    // Eye segments (Old)
    LED_RIGHT_EYE_TOP = 0,
    LED_RIGHT_EYE_RIGHT,
    LED_RIGHT_EYE_BOTTOM,
    LED_RIGHT_EYE_LEFT,
    LED_LEFT_EYE_TOP,
    LED_LEFT_EYE_RIGHT,
    LED_LEFT_EYE_BOTTOM,
    LED_LEFT_EYE_LEFT,
    
    NUM_EYE_LEDS
  }  EyeLEDId;
   */
  
  // Health / direction bar on the top of the backpack
  typedef enum {
    // Directional arrangement
    LED_BACKPACK_BACK = 0,
    LED_BACKPACK_MIDDLE,
    LED_BACKPACK_FRONT,
    LED_BACKPACK_LEFT,
    LED_BACKPACK_RIGHT,
    
    /*
    // Horizontal strip
    LED_BACKPACK_LEFT = 0,
    LED_BACKPACK_INNER_LEFT,
    LED_BACKPACK_MIDDLE, // Dummy light that is tied to INNER_RIGHT just so that the NUM_BACKPACK_LEDS stays the same
                         // and we don't have to change message sizes all over the place.
    LED_BACKPACK_INNER_RIGHT,
    LED_BACKPACK_RIGHT,
    */
    
    NUM_BACKPACK_LEDS
  } LEDId;
  
  // The color format is identical to HTML Hex Triplets (RGB)
  typedef enum {
    LED_CURRENT_COLOR = 0xffffffff, // Don't change color: leave as is
    
    LED_OFF =   0x000000,
    LED_RED =   0xff0000,
    LED_GREEN = 0x00ff00,
    LED_YELLOW= 0xffff00,
    LED_BLUE =  0x0000ff,
    LED_PURPLE= 0xff00ff,
    LED_CYAN =  0x00ffff,
    LED_WHITE = 0xffffff
  } LEDColor;
  
  typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
    LED_STATE_TURNING_ON,
    LED_STATE_TURNING_OFF
  } LEDState_t;
  
  // TODO: Add non-black "off" color
  // TODO: This will expand as we want the lights to do fancier things
  typedef struct {
    u32 onColor;                // "on" color, stored as RGBA, where A(lpha) is ignored
    u32 offColor;               // "off" color,  "
    u32 onPeriod_ms;            // time spent in "on" state
    u32 offPeriod_ms;           // time spent in "off" state
    u32 transitionOnPeriod_ms;  // time spent linearly transitioning from "off" to "on"
    u32 transitionOffPeriod_ms; // time spent linearly transitioning from "on" to "off"
    TimeStamp_t nextSwitchTime; // when next state transition will occur
    LEDState_t state;           // current state of this LED
  } LEDParams_t;
  
} // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_SHARED_LED_TYPES_H
