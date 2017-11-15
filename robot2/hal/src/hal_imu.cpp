/**
 * File:        hal_imu.cpp
 *
 * Author:      Kevin Yoon
 * Created:     05/26/2017
 *
 * Description: IMU interface via SPI
 *
 **/

#include "anki/cozmo/robot/spi_imu.h"

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"


namespace Anki {
namespace Cozmo {

namespace { // "Private members"

  const int IMU_DATA_ARRAY_SIZE = 5;
  HAL::IMU_DataStructure _imuDataArr[IMU_DATA_ARRAY_SIZE];
  u8 _imuLastReadIdx = 0;
  u8 _imuNewestIdx = 0;

} // "private" namespace


void PushIMU(const HAL::IMU_DataStructure& data)
{
  if (++_imuNewestIdx >= IMU_DATA_ARRAY_SIZE) {
    _imuNewestIdx = 0;
  }
  if (_imuNewestIdx == _imuLastReadIdx) {
    //AnkiWarn( "HAL.PushIMU.ArrayIsFull", "Dropping data");
  }

  _imuDataArr[_imuNewestIdx] = data;
}

bool PopIMU(HAL::IMU_DataStructure& data)
{
  if (_imuNewestIdx == _imuLastReadIdx) {
    return false;
  }

  if (++_imuLastReadIdx >= IMU_DATA_ARRAY_SIZE) {
    _imuLastReadIdx = 0;
  }

  data = _imuDataArr[_imuLastReadIdx];
  return true;
}

void ProcessIMUEvents()
{
  IMURawData rawData[IMU_MAX_SAMPLES_PER_READ];
  HAL::IMU_DataStructure imuData;
  const int imu_read_samples = imu_manage(rawData);
  for (int i=0; i < imu_read_samples; i++) {
    imuData.acc_x = rawData[i].acc[0] * IMU_ACCEL_SCALE_G * MMPS2_PER_GEE;
    imuData.acc_y = rawData[i].acc[1] * IMU_ACCEL_SCALE_G * MMPS2_PER_GEE;
    imuData.acc_z = rawData[i].acc[2] * IMU_ACCEL_SCALE_G * MMPS2_PER_GEE;
    imuData.rate_x = rawData[i].gyro[0] * IMU_GYRO_SCALE_DPS * RADIANS_PER_DEGREE;
    imuData.rate_y = rawData[i].gyro[1] * IMU_GYRO_SCALE_DPS * RADIANS_PER_DEGREE;
    imuData.rate_z = rawData[i].gyro[2] * IMU_GYRO_SCALE_DPS * RADIANS_PER_DEGREE;
    PushIMU(imuData);
    
    static ImageImuData imageImuData;
    imageImuData.systemTimestamp_ms = HAL::GetTimeStamp();
    imageImuData.rateX = imuData.rate_x;
    imageImuData.rateY = imuData.rate_y;
    imageImuData.rateZ = imuData.rate_z;
    //RobotInterface::SendMessage(imageImuData);
  }
}


void InitIMU()
{
  const char* err = imu_open();
  //TODO: conditional err and return
  if (err) {
    AnkiError("HAL.InitIMU.OpenFailed", "%s", err);
    return;
  }
  imu_init();
}


bool HAL::IMUReadData(HAL::IMU_DataStructure &imuData)
{
#if(0) // For faking IMU data
  while (PopIMU(imuData)) {}; // Just to pop queue
  static TimeStamp_t lastIMURead = 0;
  TimeStamp_t now = HAL::GetTimeStamp();
  if (now - lastIMURead > 4) {
    // TEMP HACK: Send 0s because on my Nexus 5x, the gyro values are kinda crazy.
    imuData.acc_x = 0.f;
    imuData.acc_y = 0.f;
    imuData.acc_z = 9800.f;
    imuData.rate_x = 0.f;
    imuData.rate_y = 0.f;
    imuData.rate_z = 0.f;

    lastIMURead = now;
    return true;
  }
  return false;
#else
  return PopIMU(imuData);
#endif
}

} // namespace Cozmo
} // namespace Anki
