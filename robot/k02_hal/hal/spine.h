#ifndef __SPINE_H
#define __SPINE_H

#include "anki/cozmo/robot/spineData.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Spine {
        void Manage(SpineProtocol& msg);
        void Dequeue(SpineProtocol& msg);
      }
    }
  }
}

#endif
