#ifndef __REC_PROTOCOL_H
#define __REC_PROTOCOL_H

#define TRANSMIT_BLOCK_SIZE (0x1000)
#define SECURE_SPACE (0x1000)
#define SHA1_DIGEST_LENGTH		20

enum RECOVERY_COMMAND {
  COMMAND_HEADER = 0x5478,
  COMMAND_DONE  = 0x00,
  COMMAND_FLASH = 0x01
};

enum RECOVERY_STATE {
  STATE_SYNC = 0x5278,
  STATE_IDLE = 0x5230,
  STATE_NACK = 0x5231,
  STATE_BUSY = 0x523F,
  STATE_UNKNOWN = 0xFFFF,
};

typedef struct {
  uint32_t  flashBlock[TRANSMIT_BLOCK_SIZE / sizeof(uint32_t)];
  uint32_t  blockOffset;
  uint8_t   checkSum[SHA1_DIGEST_LENGTH];
} RecoveryPacket;

#endif
