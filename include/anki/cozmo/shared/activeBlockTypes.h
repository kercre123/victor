#ifndef ACTIVE_BLOCK_TYPES_H
#define ACTIVE_BLOCK_TYPES_H

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {

    // Specify LEDs, relative to block's top face (as determined by accelerometer)
    // Looking down at top face (and through to bottom face), LEDs are
    // numbered as follows, with each value corresponding to a bit in a u8:
    // Note that the face labels are done such that they match the way Blocks'
    // faces are defined (imagine the robot is looking along the x axis, facing
    // the "Front" of the block, then it makes sense...)
    //
    //         (Left)                             Y
    //        0 ----- 4         2 ----- 6        ^
    //        |       |         |       |        |
    // (Front)|  TOP  |(Back)   |  BTM  |        |
    //        |       |         |       |        +----> X
    //        1 ----- 5         3 ----- 7
    //         (Right)
    //
    const s32 NUM_BLOCK_LEDS = 8;
    enum class WhichLEDs : u8 {
      NONE        = 0x00,
      ALL         = 0xFF,
      
      // Individual LEDs (indicated by a single bit set):
      TOP_UPPER_LEFT  = 0x01,
      TOP_UPPER_RIGHT = 0x10,
      TOP_LOWER_LEFT  = 0x02,
      TOP_LOWER_RIGHT = 0x20,
      BTM_UPPER_LEFT  = 0x04,
      BTM_UPPER_RIGHT = 0x40,
      BTM_LOWER_LEFT  = 0x08,
      BTM_LOWER_RIGHT = 0x80,
      
      // Top/Bottom Pairs:
      TOP_BTM_UPPER_LEFT  = TOP_UPPER_LEFT  | BTM_UPPER_LEFT,
      TOP_BTM_UPPER_RIGHT = TOP_UPPER_RIGHT | BTM_UPPER_RIGHT,
      TOP_BTM_LOWER_LEFT  = TOP_LOWER_LEFT  | BTM_LOWER_LEFT,
      TOP_BTM_LOWER_RIGHT = TOP_LOWER_RIGHT | BTM_LOWER_RIGHT,
      
      // Faces of 4 LEDs:
      TOP_FACE   = TOP_UPPER_LEFT  | TOP_UPPER_RIGHT | TOP_LOWER_LEFT  | TOP_LOWER_RIGHT,
      BTM_FACE   = BTM_UPPER_LEFT  | BTM_UPPER_RIGHT | BTM_LOWER_LEFT  | BTM_LOWER_RIGHT,
      LEFT_FACE  = TOP_UPPER_LEFT  | TOP_LOWER_LEFT  | BTM_UPPER_LEFT  | BTM_LOWER_LEFT,
      RIGHT_FACE = TOP_UPPER_RIGHT | TOP_LOWER_RIGHT | BTM_UPPER_RIGHT | BTM_LOWER_RIGHT,
      FRONT_FACE = TOP_LOWER_LEFT  | TOP_LOWER_RIGHT | BTM_LOWER_LEFT  | BTM_LOWER_RIGHT,
      BACK_FACE  = TOP_UPPER_LEFT  | TOP_UPPER_RIGHT | BTM_UPPER_LEFT  | BTM_UPPER_RIGHT
    };
    
    // TODO: This will expand as we want the lights to do fancier things
    typedef struct {
      u32 color; // Stored as RGBA, where A(lpha) is ignored
      u32 onPeriod_ms;
      u32 offPeriod_ms;
      TimeStamp_t nextSwitchTime; // for changing state when flashing
      bool        isOn;
    } LEDParams;

    
    // The amount of time between sending the flash ID message
    // to one block and the next block.
    // This must be larger than the flash pattern duration on the block!
    const u32 FLASH_BLOCK_TIME_INTERVAL_MS = 200;
    
  } // namespace Cozmo
} // namespace Anki


#endif // ACTIVE_CONFIG_H
