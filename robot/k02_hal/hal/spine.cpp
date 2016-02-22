#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "spine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Cozmo {
namespace HAL {
  static const int QUEUE_DEPTH = 8;
  
  static u8 spinebuffer[QUEUE_DEPTH][SPINE_MAX_CLAD_MSG_SIZE];
  static volatile int spine_enter = 0;
  static volatile int spine_exit = 0;

  int Spine::Dequeue(u8* dest) {
    if (spine_enter == spine_exit) {
      *dest = 0; // Invalid tag
    }
    else
    {
      memcpy(dest, spinebuffer[spine_enter], SPINE_MAX_CLAD_MSG_SIZE);
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
      AnkiWarn("Spine.Enqueue", "Message %x[%d] is too long to enqueue to body. MAX_SIZE = %d", data[0], length, SPINE_MAX_CLAD_MSG_SIZE);
      return false;
    }
    else
    {
      memcpy(spinebuffer[spine_exit], data, length);
      spine_exit = exit;
      return true;
    }
  }

  void Spine::Manage() {
    const u8 tag = g_dataToHead.cladBuffer[0];
    if (tag == RobotInterface::GLOBAL_INVALID_TAG)
    {
      // pass
    }
    else if (tag < RobotInterface::TO_RTIP_START)
    {
      AnkiError("Spine.Manage", "Received message %x that seems bound below", tag);
    }
    else if (tag > RobotInterface::TO_RTIP_END)
    {
      RadioSendMessage(g_dataToHead.cladBuffer+1, SPINE_MAX_CLAD_MSG_SIZE-1, tag);
    }
    else
    {
      RobotInterface::EngineToRobot* msg = reinterpret_cast<RobotInterface::EngineToRobot*>(g_dataToHead.cladBuffer);
      Messages::ProcessMessage(*msg);
    }
    // Prevent same messagr from getting processed twice (if the spine desyncs)
    msg.cladBuffer[0] = RobotInterface::GLOBAL_INVALID_TAG;
  }

}
}
}
