#include <stdint.h>

#ifndef __CUBE_PROTOCOL_H
#define __CUBE_PROTOCOL_H

static const uint32_t CUBE_FIRMWARE_MAGIC = 0x65627543;

__packed struct RobotHandshake {
  uint8_t msg_id;
  uint8_t ledStatus[12]; // 4-LEDs, three colors
  uint8_t tapTime;
  uint8_t _reserved[3];
};

__packed struct AccessoryHandshake {
  uint8_t msg_id;
  int8_t  x,y,z;
  uint8_t tapCount;
  uint8_t tapTime;
  int8_t  tapNeg, tapPos;
  uint8_t batteryLevel;
  uint8_t _reserved[8];
};

__packed struct AdvertisePacket {
  uint32_t id;
  uint16_t model;
  uint16_t patchLevel;
  uint8_t  hwVersion;
  uint8_t  _reserved;
};

__packed struct CapturePacket {
  uint16_t ticksUntilStart;
  uint8_t  hopIndex;
  uint8_t  hopBlackout;
  uint8_t  ticksPerBeat;
  uint8_t  beatsPerHandshake;
  uint8_t  ticksToListen;
  uint8_t  beatsPerRead;
  uint8_t  beatsUntilRead;
  uint8_t  patchStart;
};

typedef uint8_t CubeFirmwareBlock[16];

__packed struct OTAFirmwareBlock {
  uint8_t             messageId;
  CubeFirmwareBlock   block;
};

#endif
