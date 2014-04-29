#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

// Shared types between basestation and robot

namespace Anki {
  namespace Cozmo {

    typedef enum {
      DA_PICKUP_LOW = 0,  // Docking to block at level 0
      DA_PICKUP_HIGH,     // Docking to block at level 1
      DA_PLACE_HIGH       // Placing block atop another block at level 0
    } DockAction_t;

  }
}

#endif // COZMO_TYPES_H
