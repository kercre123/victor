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
  // NOTE: In the orientation displayed below, the Top marker is rightside up on screen
  //
  //
  //                 (Left)             Y
  //                 ---1---            ^
  //                |   ^   |           |
  //        (Front) 2  TOP  0 (Back)    |
  //                |       |           +----> X
  //                 ---3---
  //                 (Right)
  //
  //  When the block is on its side, FRONT is synonymous with the upper LED.
  const s32 NUM_BLOCK_LEDS = 4;
  
# if __cplusplus >= 201103L
  enum class WhichBlockLEDs : u8 {
# else
  typedef enum {
# endif
    NONE        = 0x00,
    ALL         = 0xFF,
    
    // Individual LEDs (indicated by a single bit set):
    BACK   = 0x01,
    LEFT   = 0x02,
    FRONT  = 0x04,
    RIGHT  = 0x08,

    FRONT_LEFT  = FRONT | LEFT,
    FRONT_RIGHT = FRONT | RIGHT,
    BACK_LEFT   = BACK | LEFT,
    BACK_RIGHT  = BACK | RIGHT
  }
# if __cplusplus < 201103L
  WhichBlockLEDs
#endif
  ;
  
  // Use OFF to specify a pattern of LEDs in an absolute sense.
  // Use BY_CORNER so that the specified pattern is rotated such that whatever is
  //   specified for LED_0 ends up closest to the given relative position.
  // Use BY_SIDE so that the specified pattern is rotated such that whatever is
  //   specified for the LED_0 - LED_4 side ends up closest to the given relative
  //   position.
  enum MakeRelativeMode {
    RELATIVE_LED_MODE_OFF = 0,
    RELATIVE_LED_MODE_BY_CORNER,
    RELATIVE_LED_MODE_BY_SIDE
  };
  
  // The amount of time between sending the flash ID message
  // to one block and the next block.
  // This must be larger than the flash pattern duration on the block!
  const u32 FLASH_BLOCK_TIME_INTERVAL_MS = 200;
  
    
  typedef enum {
    UP_AXIS_Xneg = 0,
    UP_AXIS_Xpos,
    UP_AXIS_Yneg,
    UP_AXIS_Ypos,
    UP_AXIS_Zneg,
    UP_AXIS_Zpos,
    NUM_UP_AXES,
    UP_AXIS_UNKNOWN = NUM_UP_AXES + 1,
  } UpAxis;
    
} // namespace Cozmo
} // namespace Anki


#endif // ACTIVE_CONFIG_H
