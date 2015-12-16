#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "hal/imu.h"
#include "MK02F12810.h"
#include "anki/cozmo/robot/drop.h"

static const int IMU_UPDATE_FREQUENCY = 200; // 200hz (5ms)

IMUData Anki::Cozmo::HAL::IMU::IMUState;

void Anki::Cozmo::HAL::IMU::Init(void) {
  // TODO: CONFIGURE IMU
  Manage();
}

static void copy_state(const void *data, int count) {
  using namespace Anki::Cozmo::HAL;
  memcpy(&IMU::IMUState, data, count);
}

void Anki::Cozmo::HAL::IMU::Manage(void) {
  return ;
  
  static int imu_update = 0;
  imu_update += IMU_UPDATE_FREQUENCY;
  if (imu_update < DROPS_PER_SECOND) {
    return ;
  }
  IMU::Manage();
  imu_update -= DROPS_PER_SECOND;

  static IMUData imu_state;
  static const uint8_t DATA_8 = 0x0C;
  
  I2C::Write(SLAVE_WRITE(ADDR_IMU), &DATA_8, sizeof(DATA_8), NULL, I2C_FORCE_START);
  I2C::Read(SLAVE_READ(ADDR_IMU), (uint8_t*) &imu_state, sizeof(IMUData), &copy_state);
}

uint8_t Anki::Cozmo::HAL::IMU::ReadID(void) {
  return I2C::ReadReg(ADDR_IMU, 0);
}
