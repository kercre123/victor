#ifndef __VECTORS_H
#define __VECTORS_H

#include <stdint.h>

typedef void (*IRQHandler)(void);

static const uint32_t COZMO_APPLICATION_FINGERPRINT = 0x4F4D3243;

struct SystemHeader {
  uint32_t          fingerPrint;      // This doubles as the "evil byte"
  uint32_t          faultCounter;     // If this is ever zero, the system will auto wipe
  uint8_t           certificate[256];
  const uint32_t*   stackStart;
  IRQHandler        resetVector;
};

enum FaultType {
  FAULT_WATCHDOG = 0x0001,
  FAULT_USER_WIPE = 0x0002,
  FAULT_I2C_FAILED = 0x0003,
  FAULT_NONE = 0xFFFF
};

#endif
