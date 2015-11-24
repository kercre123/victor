#ifndef __REC_PROTOCOL_H
#define __REC_PROTOCOL_H

#include <stdint.h>

typedef uint16_t spiWord;
static const spiWord COMMAND_HEADER = 0x5478;
static const int TRANSMIT_BLOCK_SIZE = 0x1000;
static const int SECURE_SPACE = 0x1000;

enum RECOVERY_COMMAND {
  COMMAND_DONE  = 0x00,
  COMMAND_FLASH = 0x01
};

enum RECOVERY_STATE {
  STATE_SYNC = 0x5278,
  STATE_IDLE = 0x5230,
  STATE_NACK = 0x5231,
  STATE_BUSY = 0x523F
};

#endif
