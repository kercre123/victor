#ifndef __VECTORS_H
#define __VECTORS_H

#include <stdint.h>

typedef void (*VectorPtr)(void);

struct HWInfo {
  uint32_t hw_revision;
  uint32_t hw_model;
  uint8_t ein[16];
};

static const HWInfo*  COZMO_HWINFO                  = (const HWInfo*) 0x8000010;

static const uint32_t COZMO_APPLICATION_FINGERPRINT = 0x4F4D3243;
static const uint32_t COZMO_APPLICATION_ADDRESS     = 0x8003000;
static const uint32_t COZMO_APPLICATION_HEADER      = 0x110;
static const uint32_t COZMO_APPLICATION_SIZE        = 0xD000;

static const uint32_t* COZMO_HARDWARE_VERSION       = (uint32_t*)0x10;
static const uint32_t* COZMO_MODEL_NUMBER           = (uint32_t*)0x14;

static const int FLASH_PAGE_SIZE = 0x400;

enum FaultType {
  FAULT_WATCHDOG = 0x0001,
  FAULT_USER_WIPE = 0x0002,
  FAULT_NONE = 0xFFFF
};

static const int MAX_FAULT_COUNT = 6;

extern bool validate(void); // This is used to validate the application image

struct SystemHeader {
  uint32_t          fingerPrint;      // This doubles as the "evil byte"
  FaultType         faultCounter[MAX_FAULT_COUNT];
  uint8_t           certificate[256];
  union {
    const uint32_t* stackStart;
    uint8_t   signedStart;
  };
  VectorPtr         resetVector;
  uint8_t           applicationVersion[16];
};

static const SystemHeader* const APP = (SystemHeader*)COZMO_APPLICATION_ADDRESS;

#endif
