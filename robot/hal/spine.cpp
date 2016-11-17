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
  static const int QUEUE_SIZE = 256;
  
  static uint8_t queue[QUEUE_SIZE];
  static int queue_used = 0;
  
  static int inc(int& ptr) {
    int prev = ptr;
    ptr = (ptr+1) % sizeof(queue);
    return prev;
  }

  void Spine::Dequeue(uint8_t* dest) {
    static int queue_read = 0;

    int remaining = SPINE_MAX_CLAD_MSG_SIZE_DOWN;
    
    while (queue_used > 0) {
      int total_size = queue[queue_read] + 1;

      if (total_size > remaining) {
        break ;
      }

      remaining -= total_size;
      queue_used -= total_size;

      while (total_size-- > 0) {
        *(dest++) = queue[inc(queue_read)];
      }
    }

    // We need to clear out trailing garbage
    memset(dest, 0, remaining);
  }

  bool Spine::Enqueue(const void* data, const u8 length, u8 tag) {
    static int queue_write = 0;
    int total_size = length + 2;
    
    if (sizeof(queue) < total_size + queue_used) {
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
      const uint8_t* src = (const uint8_t*) data;

      queue_used += total_size;
      
      queue[inc(queue_write)] = length + 1;
      queue[inc(queue_write)] = tag;

      for (int i = 0; i < length; i++) {
        queue[inc(queue_write)] = *(src++);
      }

      return true;
    }
  }

  void Spine::Manage() {
    RobotInterface::EngineToRobot msg;

    int remaining = sizeof(g_dataToHead.cladData);
    uint8_t* data = g_dataToHead.cladData;

    while (remaining > 0) {
      int size = *(data++);
      // Dequeue message
      memcpy(msg.GetBuffer(), data, size);
      data += size;
      remaining -= size + 1;

      if (size == 0)
      {
        // Out of messages
        break ;
      }
      else if (msg.tag < RobotInterface::TO_RTIP_START)
      {
        AnkiError( 138, "Spine.Manage", 383, "Received message %x[%d] that seems bound below", 2, msg.tag, size);
      }
      else if (msg.tag > RobotInterface::TO_RTIP_END)
      {
        RadioSendMessage(msg.GetBuffer() + 1, size - 1, msg.tag);
      }
      else if (msg.Size() != size)
      {
        AnkiError( 138, "Spine.Manage", 390, "Received message %x has %d bytes but should have %d", 3, msg.tag, size, msg.Size());
      }
      else
      {
        Messages::ProcessMessage(msg);
      }
    }

    // Prevent same message from getting processed twice (if the spine desyncs)
    memset(g_dataToHead.cladData, 0, sizeof(g_dataToHead.cladData));
  }

}
}
}
