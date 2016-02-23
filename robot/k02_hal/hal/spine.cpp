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
  
  static u8 spinebuffer[QUEUE_DEPTH][SPINE_MAX_CLAD_MSG_SIZE];
  static volatile int spine_enter = 0;
  static volatile int spine_exit = 0;

  void Spine::Dequeue(u8* dest) {
    if (spine_enter == spine_exit) {
      *dest = RobotInterface::GLOBAL_INVALID_TAG;
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
      AnkiError( 128, "Spine.Enqueue.MessageTooLong", 382, "Message %x[%d] is too long to enqueue to body. MAX_SIZE = %d", 3, data[0], length, SPINE_MAX_CLAD_MSG_SIZE);
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
      AnkiError( 129, "Spine.Manage", 383, "Received message %x that seems bound below", 1, tag);
    }
    else if (tag > RobotInterface::TO_RTIP_END)
    {
      RadioSendMessage(g_dataToHead.cladBuffer+1, SPINE_MAX_CLAD_MSG_SIZE-1, tag);
    }
    else
    {
      u8 cladBuffer[SPINE_MAX_CLAD_MSG_SIZE + 4];
      RobotInterface::EngineToRobot* msg = reinterpret_cast<RobotInterface::EngineToRobot*>(cladBuffer);
      memcpy(msg->GetBuffer(), g_dataToHead.cladBuffer, SPINE_MAX_CLAD_MSG_SIZE);
      Messages::ProcessMessage(*msg);
    }
    // Prevent same messagr from getting processed twice (if the spine desyncs)
    g_dataToHead.cladBuffer[0] = RobotInterface::GLOBAL_INVALID_TAG;
  }

}
}
}
