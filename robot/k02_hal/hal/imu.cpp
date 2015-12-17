#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "hal/imu.h"
#include "MK02F12810.h"
#include <string.h>
#include "anki/cozmo/robot/drop.h"

static const int IMU_UPDATE_FREQUENCY = 200; // 200hz (5ms)

IMUData Anki::Cozmo::HAL::IMU::IMUState;

static const uint8_t CMD = 0x7e;
static const uint8_t STEP_CONF_1 = 0x7b;
static const uint8_t STEP_CONF_0 = 0x7a;
static const uint8_t STEP_CNT_1 = 0x79;
static const uint8_t STEP_CNT_0 = 0x78;
static const uint8_t OFFSET_6 = 0x77;
static const uint8_t OFFSET_5 = 0x76;
static const uint8_t OFFSET_4 = 0x75;
static const uint8_t OFFSET_3 = 0x74;
static const uint8_t OFFSET_2 = 0x73;
static const uint8_t OFFSET_1 = 0x72;
static const uint8_t OFFSET_0 = 0x71;
static const uint8_t NV_CONF = 0x70;
static const uint8_t SELF_TEST = 0x6d;
static const uint8_t PMU_TRIGGER = 0x6c;
static const uint8_t IF_CONF = 0x6b;
static const uint8_t CONF = 0x6a;
static const uint8_t FOC_CONF = 0x69;
static const uint8_t INT_FLAT_1 = 0x68;
static const uint8_t INT_FLAT_0 = 0x67;
static const uint8_t INT_ORIENT_1 = 0x66;
static const uint8_t INT_ORIENT_0 = 0x65;
static const uint8_t INT_TAP_1 = 0x64;
static const uint8_t INT_TAP_0 = 0x63;
static const uint8_t INT_MOTION_3 = 0x62;
static const uint8_t INT_MOTION_2 = 0x61;
static const uint8_t INT_MOTION_1 = 0x60;
static const uint8_t INT_MOTION_0 = 0x5f;
static const uint8_t INT_LOWHIGH_4 = 0x5e;
static const uint8_t INT_LOWHIGH_3 = 0x5d;
static const uint8_t INT_LOWHIGH_2 = 0x5c;
static const uint8_t INT_LOWHIGH_1 = 0x5b;
static const uint8_t INT_LOWHIGH_0 = 0x5a;
static const uint8_t INT_DATA_1 = 0x59;
static const uint8_t INT_DATA_0 = 0x58;
static const uint8_t INT_MAP_2 = 0x57;
static const uint8_t INT_MAP_1 = 0x56;
static const uint8_t INT_MAP_0 = 0x55;
static const uint8_t INT_LATCH = 0x54;
static const uint8_t INT_OUT_CTRL = 0x53;
static const uint8_t INT_EN_2 = 0x52;
static const uint8_t INT_EN_1 = 0x51;
static const uint8_t INT_EN_0 = 0x50;
static const uint8_t MAG_IF_4 = 0x4f;
static const uint8_t MAG_IF_3 = 0x4e;
static const uint8_t MAG_IF_2 = 0x4d;
static const uint8_t MAG_IF_1 = 0x4c;
static const uint8_t MAG_IF_0 = 0x4b;
static const uint8_t FIFO_CONFIG_1 = 0x47;
static const uint8_t FIFO_CONFIG_0 = 0x46;
static const uint8_t FIFO_DOWNS = 0x45;
static const uint8_t MAG_CONF = 0x44;
static const uint8_t GYR_RANGE = 0x43;
static const uint8_t GYR_CONF = 0x42;
static const uint8_t ACC_RANGE = 0x41;
static const uint8_t ACC_CONF = 0x40;
static const uint8_t FIFO_DATA = 0x24;
static const uint8_t FIFO_LENGTH_1 = 0x23;
static const uint8_t FIFO_LENGTH_0 = 0x22;
static const uint8_t TEMPERATURE_1 = 0x21;
static const uint8_t TEMPERATURE_0 = 0x20;
static const uint8_t INT_STATUS_3 = 0x1f;
static const uint8_t INT_STATUS_2 = 0x1e;
static const uint8_t INT_STATUS_1 = 0x1d;
static const uint8_t INT_STATUS_0 = 0x1c;
static const uint8_t STATUS = 0x1b;
static const uint8_t SENSORTIME_2 = 0x1a;
static const uint8_t SENSORTIME_1 = 0x19;
static const uint8_t SENSORTIME_0 = 0x18;
static const uint8_t DATA_19 = 0x17;
static const uint8_t DATA_18 = 0x16;
static const uint8_t DATA_17 = 0x15;
static const uint8_t DATA_16 = 0x14;
static const uint8_t DATA_15 = 0x13;
static const uint8_t DATA_14 = 0x12;
static const uint8_t DATA_13 = 0x11;
static const uint8_t DATA_12 = 0x10;
static const uint8_t DATA_11 = 0x0f;
static const uint8_t DATA_10 = 0x0e;
static const uint8_t DATA_9 = 0x0d;
static const uint8_t DATA_8 = 0x0c;
static const uint8_t DATA_7 = 0x0b;
static const uint8_t DATA_6 = 0x0a;
static const uint8_t DATA_5 = 0x09;
static const uint8_t DATA_4 = 0x08;
static const uint8_t DATA_3 = 0x07;
static const uint8_t DATA_2 = 0x06;
static const uint8_t DATA_1 = 0x05;
static const uint8_t DATA_0 = 0x04;
static const uint8_t PMU_STATUS = 0x03;
static const uint8_t ERR_REG = 0x02;
static const uint8_t CHIP_ID = 0x00;

static const uint8_t RANGE_2G = 0x03;   
static const uint8_t INT_OPEN_DRAIN = 0x44;
static const uint8_t RANGE_500DPS = 0x02;
static const uint8_t BW_200 = 0x19;           // Maybe?

void Anki::Cozmo::HAL::IMU::Init(void) {
  I2C::WriteAndVerify(ADDR_IMU, ACC_RANGE, RANGE_2G);
  I2C::WriteAndVerify(ADDR_IMU, ACC_CONF, BW_200);
  I2C::WriteAndVerify(ADDR_IMU, INT_OUT_CTRL, INT_OPEN_DRAIN);
  I2C::WriteAndVerify(ADDR_IMU, GYR_RANGE, RANGE_500DPS);

  Manage();
}

static void copy_state(const void *data, int count) {
  using namespace Anki::Cozmo::HAL;
  memcpy(&IMU::IMUState, data, count);
}

void Anki::Cozmo::HAL::IMU::Manage(void) {
  // Eventually, this should probably be synced to the body
  static const uint8_t DATA_8 = 0x0C;
  static int imu_update = 0;
  static IMUData imu_state;

  imu_update += IMU_UPDATE_FREQUENCY;
  if (imu_update < DROPS_PER_SECOND) {
    return ;
  }
  imu_update -= DROPS_PER_SECOND;

  I2C::Write(SLAVE_WRITE(ADDR_IMU), &DATA_8, sizeof(DATA_8), NULL, I2C_FORCE_START);
  I2C::Read(SLAVE_READ(ADDR_IMU), (uint8_t*) &imu_state, sizeof(IMUData), &copy_state);
}

uint8_t Anki::Cozmo::HAL::IMU::ReadID(void) {
  return I2C::ReadReg(ADDR_IMU, 0);
}
