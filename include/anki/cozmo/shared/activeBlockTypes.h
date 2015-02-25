#ifndef ACTIVE_BLOCK_TYPES_H
#define ACTIVE_BLOCK_TYPES_H

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {

    // Letters represents the labels for the faces. X corresponds to the face that accelerometer x-axis
    // points to and x corresponds to the opposite face.
    // The numbers represent the indices of the 8 LED channels.
    //
    //
    //            4 ---------- 5
    //              |        |
    //              |   Z    |
    //            7 |        | 6
    //     ----------------------------
    //     |        |        |        |
    //     |   y    |   X    |    Y   |
    //     |        |        |        |
    //     ----------------------------
    //            3 |        | 2
    //              |   z    |
    //              |        |
    //            0 ---------- 1
    //              |        |
    //              |   x    |
    //              |        |
    //              ----------
    //
    
    
    // Positions of LEDs relative to a given top-front face pair.
    // Described as if you are inside the block with the top face above you
    // and the front face in front of you.
    // If you know which face is the front face, you can specify the LED you
    // want according to these labels.
    typedef enum {
      TopFrontLeft = 0,
      TopFrontRight,
      TopBackLeft,
      TopBackRight,
      BottomFrontLeft,
      BottomFrontRight,
      BottomBackLeft,
      BottomBackRight,
      NUM_BLOCK_LEDS
    } BlockLEDPosition;
    
    // TODO: This will expand as we want the lights to do fancier things
    typedef struct {
      u32 color;
    } LEDParams;

    
    // The amount of time between sending the flash ID message
    // to one block and the next block.
    // This must be larger than the flash pattern duration on the block!
    const u32 FLASH_BLOCK_TIME_INTERVAL_MS = 200;
    
  } // namespace Cozmo
} // namespace Anki


#endif // ACTIVE_CONFIG_H
