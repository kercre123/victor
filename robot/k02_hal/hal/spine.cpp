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
  
  enum QueueSlotState {
    QUEUE_INACTIVE,
    QUEUE_READY
  };

  struct QueueSlot {
    union {
      uint8_t raw[SPINE_MAX_CLAD_MSG_SIZE_DOWN];
      struct {
        uint8_t size;
        uint8_t tag;
        uint8_t payload[1];
      };
    };

    volatile QueueSlotState state;
  };

  static QueueSlot queue[QUEUE_DEPTH];

  void Spine::Dequeue(uint8_t* dest) {
    static int spine_read = 0;

    int remaining = SPINE_MAX_CLAD_MSG_SIZE_DOWN;
    
    while (queue[spine_read].state == QUEUE_READY) {
      QueueSlot* slot = &queue[spine_read];
      int total_size = slot->size + 1;

      if (total_size > remaining) {
        break ;
      }

      memcpy(dest, slot->raw, total_size);
      dest += total_size;
      remaining -= total_size;
      
      slot->state = QUEUE_INACTIVE;
      spine_read = (spine_read+1) % QUEUE_DEPTH;
    }

    // We need to clear out trailing garbage
    memset(dest, 0, remaining);
  }

  bool Spine::Enqueue(const void* data, const u8 length, u8 tag) {
    static int spine_write = 0;

    if (queue[spine_write].state == QUEUE_READY) {
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
      QueueSlot* slot = &queue[spine_write];
      
      slot->size = length + 1;
      slot->tag = tag;
      memcpy(slot->payload, data, length);
      
      slot->state = QUEUE_READY;
      spine_write = (spine_write+1) % QUEUE_DEPTH;
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
