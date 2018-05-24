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
#include "anki/cozmo/shared/factory/faultCodes.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#include <thread>
#include <mutex>

namespace Anki {
namespace Cozmo {

namespace { // "Private members"

  const int IMU_DATA_ARRAY_SIZE = 5;
  HAL::IMU_DataStructure _imuDataArr[IMU_DATA_ARRAY_SIZE];
  u8 _imuLastReadIdx = 0;
  u8 _imuNewestIdx = 0;

  // How often, in ticks, to update the imu tempurature
  const u32 IMU_TEMP_UPDATE_FREQ_TICKS = 200; // ~1 second

  // IMU interactions are handled on a thread due to blocking system calls
#if PROCESS_IMU_ON_THREAD
  std::thread _processor;
  std::mutex _mutex;
  bool _stopProcessing = false;
#endif

} // "private" namespace


void PushIMU(const HAL::IMU_DataStructure& data)
{
#if PROCESS_IMU_ON_THREAD
  std::lock_guard<std::mutex> lock(_mutex);
#endif
  if (++_imuNewestIdx >= IMU_DATA_ARRAY_SIZE) {
    _imuNewestIdx = 0;
  }
  if (_imuNewestIdx == _imuLastReadIdx) {
    AnkiWarn( "HAL.PushIMU.ArrayIsFull", "Dropping data");
  }

  _imuDataArr[_imuNewestIdx] = data;
}

bool PopIMU(HAL::IMU_DataStructure& data)
{
#if PROCESS_IMU_ON_THREAD
  std::lock_guard<std::mutex> lock(_mutex);
#endif

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
  static u8 tempCount = 0;
  if(tempCount++ >= IMU_TEMP_UPDATE_FREQ_TICKS)
  {
    tempCount = 0;
    imu_update_temperature();
  }

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
    imuData.temperature_degC = IMU_TEMP_RAW_TO_C(rawData[i].temperature);
    PushIMU(imuData);
  }
}

bool OpenIMU()
{
  const char* err = imu_open();
  if (err) {
    AnkiError("HAL.InitIMU.OpenFailed", "%s", err);
    FaultCode::DisplayFaultCode(FaultCode::IMU_FAILURE);
    return false;
  }
  imu_init();
  return true;
}

#if PROCESS_IMU_ON_THREAD
// Processing loop for reading imu on a thread
void ProcessLoop()
{
  if(!OpenIMU())
  {
    return;
  }

  while(!_stopProcessing)
  {
    const auto start = std::chrono::steady_clock::now();
    ProcessIMUEvents();
    const auto end = std::chrono::steady_clock::now();

    // Sleep such that there are 5ms between ProcessIMUEvent calls
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::chrono::duration<double, std::micro> sleepTime = std::chrono::milliseconds(5) - elapsed;
    std::this_thread::sleep_for(sleepTime);
  }

  imu_close();
}
#endif

void InitIMU()
{

#if PROCESS_IMU_ON_THREAD
  // Spin up the processing thread and detach it
  // This will open, init, and read the imu
  _processor = std::thread(ProcessLoop);
  _processor.detach();
#else
  OpenIMU();
#endif
}

void StopIMU()
{
#if PROCESS_IMU_ON_THREAD
  _stopProcessing = true;
#else
  imu_close();
#endif
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
