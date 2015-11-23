#ifndef __REC_PROTOCOL_H
#define __REC_PROTOCOL_H

#include <stdint.h>

typedef uint16_t commandWord;

#define SHA1_DIGEST_LENGTH 20
#define TRANSMIT_BLOCK_SIZE 0x1000
#define SECURE_SPACE 0x1000

// These are used for the Head communication protocol
static const commandWord COMMAND_HEADER = 0x5478;

#ifndef SHA1_BLOCK_SIZE
#define SHA1_BLOCK_SIZE 20
#endif

typedef struct {
  uint32_t   flashBlock[TRANSMIT_BLOCK_SIZE / sizeof(uint32_t)];
  uint32_t   blockAddress;
  uint8_t    checkSum[SHA1_BLOCK_SIZE];
} FirmwareBlock;

enum RECOVERY_COMMAND {
  COMMAND_DONE  = 0x00,
  COMMAND_FLASH = 0x01
};

enum RECOVERY_STATE {
  STATE_SYNC,
  STATE_NACK,
  STATE_BUSY,
  STATE_RUNNING,
  STATE_UNKNOWN,
  STATE_IDLE = 0x5479
};

enum DROP_COMMANDS {
  DROP_EnterBootloader = 0xfe,
  DROP_BodyUpgradeData = 0xfd,
  DROP_BodyState       = 0xfc,
};

enum EnterBootloaderWhich {
  BOOTLOAD_RTIP,
  BOOTLOAD_BODY,
};

// WIFI -> RTIP
typedef struct {
  uint8_t which;
} EnterBootloader;


// RTIP -> WIFI
typedef struct {
  uint16_t state;
  uint16_t  count;
} BodyState;

// WIFI -> RTIP
typedef struct
{
  uint32_t data;
} BodyUpgradeData;

#endif
