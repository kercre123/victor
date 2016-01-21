#ifndef __IMU_H
#define __IMU_H

#include <stdint.h>

static const uint8_t ADDR_IMU = 0x68; // 7-bit slave address of gyro

struct __attribute__((packed)) IMUData {
  int16_t gyro[3];
  int16_t acc[3];
  uint8_t timestamp;
};

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace IMU
      {
        extern IMUData IMUState;
        void Init(void);
        void Update(void);
        void Manage(void);
        uint8_t ReadID(void);
      }
    }
  }
}

#endif
