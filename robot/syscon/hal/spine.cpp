#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"
#include "radio.h"
#include "spine.h"
#include "lights.h"

extern void EnterRecovery(void);

static const int QUEUE_DEPTH = 4;
static u8 spinebuffer[QUEUE_DEPTH][SPINE_MAX_CLAD_MSG_SIZE];
static volatile int spine_enter = 0;
static volatile int spine_exit  = 0;


bool Anki::Cozmo::HAL::RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
{
  const int exit = (spine_exit+1) % QUEUE_DEPTH;
  if (spine_enter == exit) {
    return false;
  }
  else if ((size + 1) > SPINE_MAX_CLAD_MSG_SIZE)
  {
    AnkiError("Spine.Enqueue.MessageTooLong", "Message %x[%d] too long to enqueue to head. MAX_SIZE = %d", msgID == 0 ? data[0] : msgID, size, SPINE_MAX_CLAD_MSG_SIZE);
    return false;
  }
  else
  {
    u8* dest = spinebuffer[spine_exit];
    if (msgID != 0) {
      *dest = msgID;
      ++dest;
    }
    memcpy(dest, data, size);
    spine_exit = exit;
    return true;
  }
}

void Spine::Dequeue(u8* dest) {
  if (spine_enter == spine_exit)
  {
    *dest = 0; // Invalid tag
  }
  else
  {
    memcpy(dest, spinebuffer[spine_enter, SPINE_MAX_CLAD_MSG_SIZE]);
    spine_enter = (spine_enter + 1) % QUEUE_DEPTH;
  }
}
