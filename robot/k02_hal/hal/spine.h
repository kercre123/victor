#ifndef __SPINE_H
#define __SPINE_H

#include "anki/cozmo/robot/spineData.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Spine {
        void Manage();
        /// Dequeu data for body.
        // @warning Only call from hal exec thread
        void Dequeue(CladBuffer* dest);
        /// Enqueue CLAD data to send to body.
        // @warning Only call from main executation thread
        bool Enqueue(const u8* data, const u8 length, const u8 tag=RobotInterface::GLOBAL_INVALID_TAG);
      }
    }
  }
}

#endif
