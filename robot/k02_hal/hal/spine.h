#ifndef __SPINE_H
#define __SPINE_H

#include "anki/cozmo/robot/spineData.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Spine {
        void Manage();
        /// Dequeue data for body.
        // @warning Only call from hal exec thread
        void Dequeue(CladBufferDown* dest);
        /// Enqueue CLAD data to send to body.
        // @warning Only call from main executation thread
        bool Enqueue(const void* data, const u8 length, const u8 tag);
      }
    }
  }
}

#endif
