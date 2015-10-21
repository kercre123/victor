#ifndef __IMU_H
#define __IMU_H

#define ADDR_IMU 0x68 // 7-bit slave address of gyro

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void IMUInit(void);
      void IMUManage(void);
    }
  }
}

#endif
