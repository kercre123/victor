// Low-level driver for Bosch BMI160 combo gyro+accelerometer
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "hal/imu.h"
#include "hal/uart.h"
#include "MK02F12810.h"
#include <string.h>
#include "anki/cozmo/robot/drop.h"

//#define IMU_DEBUG   // Uncomment for low level debugging printed out the UART

static const float IMU_UPDATE_FREQUENCY_HZ = 1.0/0.005;
static const float IMU_TIMESTAMP_TICKS_PER_UPDATE = 128;
static const float IMU_LATENCY_S = 0.010f; // 10ms TODO Better estimate of this
#define IMU_TIMESTAMP_TO_SECONDS(timestamp) ((timestamp) / (IMU_TIMESTAMP_TICKS_PER_UPDATE * IMU_UPDATE_FREQUENCY_HZ))
  
u32 Anki::Cozmo::HAL::IMU::frameNumberStamp;
u16 Anki::Cozmo::HAL::IMU::scanLineStamp;


static const uint8_t RANGE_2G = 0x03;
static const uint8_t INT_OPEN_DRAIN = 0x44;
static const uint8_t RANGE_500DPS = 0x02;
static const uint8_t BW_200 = 0x19;           // Maybe?
static const uint8_t CONF_GYRO = 0x09;    // 4x oversample, 200Hz update

static const float ACC_RANGE_CONST  = (1.0f/16384.0f)*9810.0f;      //In 2g mode, 16384 LSB/g
static const float GYRO_RANGE_CONST = (1.0f/65.6f)*(M_PI/180.0f);   //In FS500 mode, 65.6 deg/s / LSB

static IMUData imu_state;
static IMUData imu_offsets = {0};
IMUData Anki::Cozmo::HAL::IMU::IMUState;
static bool imu_changed = false;
static bool imu_updated = false;

static const int IMU_UPDATE_FREQUENCY = 200;  // 200hz
static const int MANAGE_FREQUENCY = DROPS_PER_SECOND;
static const int ADJUST_OVERSHOOT = IMU_UPDATE_FREQUENCY * 4; // This is a guess on how long it takes for IMU data to transmit

void Anki::Cozmo::HAL::IMU::Init(void) {
  // XXX: The first command is ignored - so power up twice - clearly I don't know what I'm doing here
  I2C::WriteReg(ADDR_IMU, CMD, 0x10 + 1); // Power up accelerometer (normal mode)
  MicroWait(4000);   // Datasheet says wait 4ms
  I2C::WriteReg(ADDR_IMU, CMD, 0x10 + 1); // Power up accelerometer (normal mode)
  MicroWait(4000);   // Datasheet says wait 4ms

  I2C::WriteReg(ADDR_IMU, CMD, 0x14 + 1); // Power up gyroscope (normal mode)
  MicroWait(81000);   // Datasheet says wait 80ms

#ifdef IMU_DEBUG
  UART::DebugPrintf("IMU status after power up: %02x %02x %02x\n", I2C::ReadReg(ADDR_IMU, 0x0), I2C::ReadReg(ADDR_IMU, 0x2), I2C::ReadReg(ADDR_IMU, 0x3));
#endif

  I2C::WriteReg(ADDR_IMU, ACC_RANGE, RANGE_2G);
  I2C::WriteReg(ADDR_IMU, ACC_CONF, BW_200);
  I2C::WriteReg(ADDR_IMU, INT_OUT_CTRL, INT_OPEN_DRAIN);
  I2C::WriteReg(ADDR_IMU, GYR_RANGE, RANGE_500DPS);
  I2C::WriteReg(ADDR_IMU, GYR_CONF, CONF_GYRO);
  
  ReadID();

  Manage();
}

static void state_updated(const void*) {
  // We received our IMU data
  static uint8_t lastTimestamp = 0x80;
  imu_changed = ((imu_state.timestamp ^ lastTimestamp) & 0x80) != 0;
  lastTimestamp = imu_state.timestamp;
}

void Anki::Cozmo::HAL::IMU::Manage(void) {
  static int update_counter = 0;

  // We have a new bundle of IMU data, stuff it into the buffer
  if (imu_changed) {
    // Attempt to find the adjustment for the update counter
    uint8_t offset = 0x80 + 0x40 - (imu_state.timestamp & ~0x80);
    update_counter = offset * (MANAGE_FREQUENCY / IMU_UPDATE_FREQUENCY) + ADJUST_OVERSHOOT;
    
    IMU::frameNumberStamp = CameraGetFrameNumber();
    IMU::scanLineStamp    = CameraGetScanLine();

    memcpy(&IMUState, &imu_state, sizeof(IMUData));

    imu_changed = false;
    imu_updated = true;

    #ifdef IMU_DEBUG
      for (int i = 0; i < sizeof(IMUData); i++)
        UART::DebugPrintf("%02x ", ((uint8_t*) data)[i]);
      UART::DebugPrintf("\n");
      UART::DebugPutc('.');
    #endif
  }

  // Adjust up the IMU update frequency
  update_counter -= IMU_UPDATE_FREQUENCY;
  
  if (update_counter > 0) {
    return ;
  }

  // This will be overridden, but make sure we don't read too quickly if the IMU data doesn't get read fast enough
  update_counter += MANAGE_FREQUENCY;

  // Configure I2C bus to read IMU data
  I2C::SetupRead(&imu_state, sizeof(imu_state), state_updated);
  
  I2C::Write(SLAVE_WRITE(ADDR_IMU), &DATA_8, sizeof(DATA_8), I2C_FORCE_START);
  I2C::Read(SLAVE_READ(ADDR_IMU));
}

void Anki::Cozmo::HAL::IMUSetCalibrationOffsets(const int16_t* accel, const int16_t* gyro) {
  // TODO: Accelerometer offsets were acquired with head in the down position.
  //       Need to do some math to make use of this, but ignoring for now.
  imu_offsets.acc[0] = 0; //accel[0];  
  imu_offsets.acc[1] = 0; //accel[1];  
  imu_offsets.acc[2] = 0; //accel[2];  
  imu_offsets.gyro[0] = gyro[0];
  imu_offsets.gyro[1] = gyro[1];
  imu_offsets.gyro[2] = gyro[2];  
}

uint8_t Anki::Cozmo::HAL::IMU::ReadID(void) {
  return I2C::ReadReg(ADDR_IMU, 0);
}

void Anki::Cozmo::HAL::IMUReadRawData(int16_t* accel, int16_t* gyro, uint8_t* timestamp)
{
  using namespace IMU;
  // This does not advance the fifo counter
  
  accel[0] = IMUState.acc[2];
  accel[1] = IMUState.acc[1];
  accel[2] = -IMUState.acc[0];
  gyro[0] = IMUState.gyro[2];
  gyro[1] = IMUState.gyro[1];
  gyro[2] = -IMUState.gyro[0];
  *timestamp = IMUState.timestamp;
}

bool Anki::Cozmo::HAL::IMUReadData(Anki::Cozmo::HAL::IMU_DataStructure &imuData)
{  
  if (!imu_updated) {
    return false;
  }
  
  imu_updated = false;

  // Accelerometer uses 12 most significant bits - gyro uses all 16
  #define ACC_CONVERT(raw)  (ACC_RANGE_CONST  * (raw))
  #define GYRO_CONVERT(raw) (GYRO_RANGE_CONST * (raw))

  imuData.acc_x  = ACC_CONVERT(IMU::IMUState.acc[2] - imu_offsets.acc[2]);
  imuData.rate_x = GYRO_CONVERT(IMU::IMUState.gyro[2] - imu_offsets.gyro[2]);
  imuData.acc_y  = ACC_CONVERT(IMU::IMUState.acc[1] - imu_offsets.acc[1]);
  imuData.rate_y = GYRO_CONVERT(IMU::IMUState.gyro[1] - imu_offsets.gyro[1]);
  imuData.acc_z  = ACC_CONVERT(-IMU::IMUState.acc[0] + imu_offsets.acc[0]);
  imuData.rate_z = GYRO_CONVERT(-IMU::IMUState.gyro[0] + imu_offsets.gyro[0]);
  
  return true;
}

void Anki::Cozmo::HAL::IMUGetCameraTime(uint32_t* const frameNumber, uint8_t* const line2Number)
{
  using namespace IMU;

  static const int SCAN_LINES_PER_FRAME = 500; ///< In future this may be 500 +/- 1 for multi-player synchronization
  static const int SCAN_LINE_RATE_HZ = 7440;
  const float LINES_PER_FRAME = static_cast<float>(SCAN_LINES_PER_FRAME);
  float scanLine = static_cast<float>(IMU::scanLineStamp)
                 - (IMU_TIMESTAMP_TO_SECONDS(IMUState.timestamp) * SCAN_LINE_RATE_HZ)
                 - (IMU_LATENCY_S * SCAN_LINE_RATE_HZ)
                 + CameraGetExposureDelay();
  uint32_t correctedFrameNumber = IMU::frameNumberStamp;
  while(scanLine < 0.0f)
  {
    correctedFrameNumber--;
    scanLine += LINES_PER_FRAME;
  }
  while(scanLine >= LINES_PER_FRAME)
  {
    correctedFrameNumber++;
    scanLine -= LINES_PER_FRAME;
  }
  
  *frameNumber = correctedFrameNumber;
  *line2Number = static_cast<uint8_t>(scanLine/2.0f);
}
