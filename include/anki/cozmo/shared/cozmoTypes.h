#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

#include "anki/common/types.h"

// Shared types between basestation and robot

namespace Anki {
  namespace Cozmo {

    typedef enum {
      SAVE_OFF = 0,
      SAVE_ONE_SHOT,
      SAVE_CONTINUOUS
    } SaveMode_t;
    
  }
}

#endif // COZMO_TYPES_H
