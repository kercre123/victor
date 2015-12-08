#ifndef __IMU_H
#define __IMU_H

static const uint8_t ADDR_IMU = 0x68; // 7-bit slave address of gyro

struct IMUData {
  int16_t gyro[3];
  int16_t acc[3];
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
        void Manage(void);
        uint8_t ReadID(void);
      }
    }
  }
}

#endif
