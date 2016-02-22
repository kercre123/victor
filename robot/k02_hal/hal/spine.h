#ifndef __SPINE_H
#define __SPINE_H

#include "anki/cozmo/robot/spineData.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Spine {
        void Manage();
        /// Dequeu data for body.
        // @warning Only call from hal exec thread
        void Dequeue(u8* dest);
        /// Enqueue CLAD data to send to body.
        // @warning Only call from main executation thread
        bool Enqueue(const u8* data, const u8 length);
      }
    }
  }
}

#endif
