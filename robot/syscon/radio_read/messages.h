#ifndef __SPINE_H
#define __SPINE_H

#include "portable.h"
#include "spineData.h"

namespace Spine {
  void Dequeue(CladBufferUp* dest);
  void ProcessHeadData();
  void ProcessMessage(void*msg);
}

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID=0);
    }
  }
}

#endif
