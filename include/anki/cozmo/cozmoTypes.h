#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

// Shared types between basestation and robot

namespace Anki {
  namespace Cozmo {

    typedef enum {
      DA_PICKUP_LOW_BLOCK = 0,
      DA_PICKUP_HIGH_BLOCK,
      DA_PLACE_HIGH_BLOCK
    } DockAction_t;
    
  }
}

#endif // COZMO_TYPES_H
