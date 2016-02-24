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
  
  static CladBuffer spinebuffer[QUEUE_DEPTH];
  static volatile int spine_enter = 0;
  static volatile int spine_exit = 0;

  void Spine::Dequeue(CladBuffer* dest) {
    if (spine_enter == spine_exit) {
      dest->length = 0;
    }
    else
    {
      memcpy(dest, &spinebuffer[spine_enter], sizeof(CladBuffer));
      spine_enter = (spine_enter+1) % QUEUE_DEPTH;
    }
  }
  
  bool Spine::Enqueue(const u8* data, const u8 length) {
    const int exit = (spine_exit+1) % QUEUE_DEPTH;
    if (spine_enter == exit) {
      return false;
    }
    else if (length > SPINE_MAX_CLAD_MSG_SIZE)
    {
      AnkiError( 128, "Spine.Enqueue.MessageTooLong", 382, "Message %x[%d] is too long to enqueue to body. MAX_SIZE = %d", 3, data[0], length, SPINE_MAX_CLAD_MSG_SIZE);
      return false;
    }
    else
    {
      memcpy(spinebuffer[spine_exit].data, data, length);
      spinebuffer[spine_exit].length = length;
      spine_exit = exit;
      return true;
    }
  }

  void Spine::Manage() {
    RobotInterface::EngineToRobot* msg = reinterpret_cast<RobotInterface::EngineToRobot*>(&g_dataToHead.cladBuffer);
    const u8 tag = msg->tag;
    if (g_dataToHead.cladBuffer.length == 0 || tag == RobotInterface::GLOBAL_INVALID_TAG)
    {
      // pass
    }
    else if (tag < RobotInterface::TO_RTIP_START)
    {
      AnkiError( 129, "Spine.Manage", 383, "Received message %x[%d] that seems bound below", 2, tag, g_dataToHead.cladBuffer.length);
    }
    else if (tag > RobotInterface::TO_RTIP_END)
    {
      RadioSendMessage(g_dataToHead.cladBuffer.data + 1, g_dataToHead.cladBuffer.length-1, g_dataToHead.cladBuffer.data[0]);
    }
		else if (msg->Size() != g_dataToHead.cladBuffer.length)
		{
			AnkiError( 129, "Spine.Manage", 390, "Received message %x has %d bytes but should have %d", 3, tag, g_dataToHead.cladBuffer.length, msg->Size());
		}
    else
    {
      Messages::ProcessMessage(*msg);
    }
    // Prevent same message from getting processed twice (if the spine desyncs)
    g_dataToHead.cladBuffer.length = 0;
  }

}
}
}
