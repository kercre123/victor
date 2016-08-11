#ifndef __SPINE_H
#define __SPINE_H

#include "portable.h"
#include "anki/cozmo/robot/spineData.h"

namespace Spine {
  void dequeue(uint8_t* dest);
  void processMessages(const uint8_t* msg);
  void init(void);
}

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID);
    }
  }
}

#endif
