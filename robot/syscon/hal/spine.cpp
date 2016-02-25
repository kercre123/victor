#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/logging.h"
#include "lights.h"
#include "radio.h"
#include "spine.h"

using namespace Anki::Cozmo;

extern void EnterRecovery(void);

static const int QUEUE_DEPTH = 4;
static CladBufferUp spinebuffer[QUEUE_DEPTH];
static volatile int spine_enter = 0;
static volatile int spine_exit  = 0;


bool HAL::RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
{
  const int exit = (spine_exit+1) % QUEUE_DEPTH;
  if (spine_enter == exit) {
    return false;
  }
  else if ((size + 1) > SPINE_MAX_CLAD_MSG_SIZE_UP)
  {
    AnkiError( 128, "Spine.Enqueue.MessageTooLong", 386, "Message %x[%d] too long to enqueue to head. MAX_SIZE = %d", 3, msgID, size, SPINE_MAX_CLAD_MSG_SIZE_UP);
    return false;
  }
  else
  {
    u8* dest = spinebuffer[spine_exit].data;
    if (msgID != 0) {
      *dest = msgID;
      ++dest;
    }
    memcpy(dest, buffer, size);
    spinebuffer[spine_exit].length = size + 1;
    spine_exit = exit;
    return true;
  }
}

void Spine::Dequeue(CladBufferUp* dest) {
  if (spine_enter == spine_exit)
  {
    dest->length = 0;
  }
  else
  {
    memcpy(dest, &spinebuffer[spine_enter], sizeof(CladBufferUp));
    spine_enter = (spine_enter + 1) % QUEUE_DEPTH;
  }
}
