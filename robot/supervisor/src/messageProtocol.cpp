#include "anki/cozmo/messageProtocol.h"

// 4. Fill in the message information lookup table:

// TODO: Would be nice to use NUM_MSG_IDS instead of hard-coded 256 here.
MessageTableEntry MessageTable[256] = {
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
  {0, 0, 0} // Final dummy entry without comma at end
};
