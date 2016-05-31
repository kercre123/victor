#ifndef __REC_PROTOCOL_H
#define __REC_PROTOCOL_H

#include <stdint.h>

typedef uint16_t commandWord;

#define SHA1_DIGEST_LENGTH 20
#define TRANSMIT_BLOCK_SIZE 0x800

#define BODY_SECURE_SPACE 0x18000
#define BODY_BOOTLOADER 0x1F000
#define ROBOT_BOOTLOADER 0x1000

// These are used for the Head communication protocol
static const commandWord COMMAND_HEADER = 0x5478;
static const uint32_t		 BODY_RECOVERY_NOTICE = 0x58525a43;
static const uint32_t		 HEAD_RECOVERY_NOTICE = 0x49485a43;

#ifndef SHA1_BLOCK_SIZE
#define SHA1_BLOCK_SIZE 20
#endif

#define SPECIAL_BLOCK     (0xFF000000)
#define CERTIFICATE_BLOCK (0xFFFFffff)
#define HEADER_BLOCK      (0xFFFFfffe)
#define COMMENT_BLOCK     (0xFFFFfffc)
#define FACTORY_FIRMWARE_INSTALL (0xFF400000)
#define ESPRESSIF_BLOCK   (0x40000000)
#define BODY_BLOCK        (0x80000000)

typedef struct {
  uint32_t   flashBlock[TRANSMIT_BLOCK_SIZE / sizeof(uint32_t)];
  uint32_t   blockAddress;
  uint32_t   checkSum;
} FirmwareBlock;

typedef struct {
  uint32_t   length;
  uint8_t    data[2044];
} CertificateData;

enum RECOVERY_COMMAND {
  COMMAND_DONE        = 0x00,
  COMMAND_FLASH       = 0x01,
  COMMAND_SET_LED     = 0x02,
  COMMAND_CHECK_SIG   = 0x03,
  COMMAND_BOOT_READY  = 0x04,
  COMMAND_PAUSE       = 0x05,
  COMMAND_RESUME      = 0x06,
  COMMAND_IDLE        = 0x07
};

enum RECOVERY_STATE {
  STATE_SYNC,
  STATE_ACK,
  STATE_NACK,
  STATE_BUSY,
  STATE_RUNNING,
  STATE_UNKNOWN,
  STATE_IDLE = 0x5479
};

#endif
