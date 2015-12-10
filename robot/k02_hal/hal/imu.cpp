#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "hal/imu.h"
#include "MK02F12810.h"

IMUData Anki::Cozmo::HAL::IMU::IMUState;

void Anki::Cozmo::HAL::IMU::Init(void) {
  uint8_t id = ReadID();
  Manage();
}

void Anki::Cozmo::HAL::IMU::Manage(void) {
  static const uint8_t DATA_8 = 0x0C;
  
  I2C::Write(SLAVE_WRITE(ADDR_IMU), &DATA_8, sizeof(DATA_8), NULL);
  I2C::Read(SLAVE_READ(ADDR_IMU), (uint8_t*) &IMUState, sizeof(IMUData), NULL);
}

uint8_t Anki::Cozmo::HAL::IMU::ReadID(void) {
  return I2C::ReadReg(ADDR_IMU, 0);
}
