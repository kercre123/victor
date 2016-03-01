#ifndef __SPINE_H
#define __SPINE_H

#include "portable.h"
#include "anki/cozmo/robot/spineData.h"

namespace Spine {
  void Dequeue(CladBufferUp* dest);
}

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID=0);
    }
  }
}

#endif
