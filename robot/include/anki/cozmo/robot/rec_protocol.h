#ifndef __REC_PROTOCOL_H
#define __REC_PROTOCOL_H

#include <stdint.h>
#include <stdint.h>

typedef uint16_t commandWord;

static const int TRANSMIT_BLOCK_SIZE = 0x1000;
static const int SECURE_SPACE = 0x1000;

// These are used for the Head communication protocol
static const commandWord COMMAND_HEADER = 0x5478;

enum RECOVERY_COMMAND {
  COMMAND_DONE  = 0x00,
  COMMAND_FLASH = 0x01
};

enum RECOVERY_STATE {
  STATE_SYNC,
  STATE_IDLE,
  STATE_NACK,
  STATE_BUSY,
  STATE_RUNNING,
  STATE_UNKNOWN
};


#endif
