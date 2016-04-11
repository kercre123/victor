#ifndef __REC_PROTOCOL_H
#define __REC_PROTOCOL_H

#include <stdint.h>

typedef uint16_t commandWord;

#define SHA1_DIGEST_LENGTH 20
#define TRANSMIT_BLOCK_SIZE 0x800
#define SECURE_SPACE 0x18000
#define BOOTLOADER 0x1F000

// These are used for the Head communication protocol
static const commandWord COMMAND_HEADER = 0x5478;

#ifndef SHA1_BLOCK_SIZE
#define SHA1_BLOCK_SIZE 20
#endif

typedef struct {
  uint32_t   flashBlock[TRANSMIT_BLOCK_SIZE / sizeof(uint32_t)];
  uint32_t   blockAddress;
  uint32_t   checkSum;
} FirmwareBlock;

enum RECOVERY_COMMAND {
  COMMAND_DONE  			= 0x00,
  COMMAND_FLASH 			= 0x01,
	COMMAND_FLASH_BODY 	= 0x02,
	COMMAND_SET_LED 		= 0x03
};

enum RECOVERY_STATE {
  STATE_SYNC,
  STATE_NACK,
  STATE_BUSY,
  STATE_RUNNING,
  STATE_UNKNOWN,
  STATE_IDLE = 0x5479
};

#endif
