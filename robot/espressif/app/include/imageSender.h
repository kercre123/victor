#ifndef __img_sender_h_
#define __img_sender_h_
/** @file A simple function to send image data to the engine in CLAD messages
 * @author Daniel Casner
 */
 
#include "os_type.h"
#include "anki/cozmo/robot/ctassert.h"

#define IMAGE_DATA_MAX_LENGTH  ((int)(1200))

#define IMAGE_SEND_TASK_QUEUE_DEPTH 4
#define IMAGE_SEND_TASK_PRIO USER_TASK_PRIO_2

enum {
  QIS_none       = 0x00,
  QIS_endOfFrame = 0x01,
  QIS_overflow   = 0x02,
  QIS_resume     = 0x04,
};

/** Pure C container that can be converted to a CladImageChunk
 */
typedef struct
{
  int32_t header[4]; // CLAD ImageChunk fields we don't care about
  volatile int16_t status; // Book keeping for status
  int16_t length; // Amount of data in bytes
  uint8_t data[IMAGE_DATA_MAX_LENGTH]; // Image data
} ImageDataBuffer;

/// Task function for sending image data
void sendImageTask(os_event_t *event);

#endif
