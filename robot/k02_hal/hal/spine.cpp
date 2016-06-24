#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "spine.h"
#include "uart.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "messages.h"

namespace Anki {
namespace Cozmo {
namespace HAL {
  static const int QUEUE_DEPTH = 8;
  
  static CladBufferDown spinebuffer[QUEUE_DEPTH];
  static bool active[QUEUE_DEPTH];
  static volatile int spine_read = 0;
  static volatile int spine_write = 0;

  void Spine::Dequeue(CladBufferDown* dest) {
    if (active[spine_read]) {
      // Skip flags field
      memcpy(dest, &(spinebuffer[spine_read]), sizeof(CladBufferDown));
      active[spine_read] = false;
      spine_read = (spine_read+1) % QUEUE_DEPTH;
    }
    else
    {
      dest->length = 0;
    }
  }

  bool Spine::Enqueue(const void* data, const u8 length, u8 tag) {
    if (active[spine_write]) {
      return false;
    }
    else if (tag == RobotInterface::GLOBAL_INVALID_TAG)
    {
      return true;
    }
    else if (length > SPINE_MAX_CLAD_MSG_SIZE_DOWN)
    {
      AnkiError( 128, "Spine.Enqueue.MessageTooLong", 382, "Message %x[%d] is too long to enqueue to body. MAX_SIZE = %d", 3, tag, length, SPINE_MAX_CLAD_MSG_SIZE_DOWN);
      return false;
    }
    else
    {
      spinebuffer[spine_write].msgID = tag;
      memcpy(spinebuffer[spine_write].data, data, length);      
      spinebuffer[spine_write].length = length + 1;
      active[spine_write] = true;
      spine_write = (spine_write+1) % QUEUE_DEPTH;
      return true;
    }
  }

  void Spine::Manage() {
    RobotInterface::EngineToRobot& msg = *reinterpret_cast<RobotInterface::EngineToRobot*>(&g_dataToHead.cladBuffer);

    if (g_dataToHead.cladBuffer.length == 0)
    {
      // pass
    }
    else if (msg.tag < RobotInterface::TO_RTIP_START)
    {
      AnkiError( 138, "Spine.Manage", 383, "Received message %x[%d] that seems bound below", 2, msg.tag, g_dataToHead.cladBuffer.length);
    }
    else if (msg.tag > RobotInterface::TO_RTIP_END)
    {
      RadioSendMessage(g_dataToHead.cladBuffer.data, g_dataToHead.cladBuffer.length - 1, g_dataToHead.cladBuffer.msgID);
    }
    else if (msg.Size() != g_dataToHead.cladBuffer.length)
    {
      AnkiError( 138, "Spine.Manage", 390, "Received message %x has %d bytes but should have %d", 3, msg.tag, g_dataToHead.cladBuffer.length, msg.Size());
    }
    else
    {
      Messages::ProcessMessage(msg);
    }

    // Prevent same message from getting processed twice (if the spine desyncs)
    g_dataToHead.cladBuffer.length = 0;
  }

}
}
}
