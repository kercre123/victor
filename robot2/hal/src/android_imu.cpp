/**
 * File:        android_imu.cpp
 *
 * Author:      Kevin Yoon
 * Created:     05/26/2017
 *
 * Description: Access to android IMU
 *
 **/

// Android includes

#define RAW_IMU 1
#define ANDROID_IMU 2
#define IMU_INTERFACE RAW_IMU

#if IMU_INTERFACE == ANDROID_IMU      
#include <android/sensor.h>
#else
#include "anki/cozmo/robot/spi_imu.h"
#endif

// Our Includes
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"


namespace Anki {
  namespace Cozmo {

    namespace { // "Private members"

#if IMU_INTERFACE == ANDROID_IMU      
      // Android sensor (i.e. IMU)
      ASensorManager*    _sensorManager;
      const ASensor*     _accelerometer;
      const ASensor*     _gyroscope;
      ASensorEventQueue* _sensorEventQueue;
      ALooper*           _looper;

      const int SENSOR_REFRESH_RATE_HZ = 200;
      constexpr int32_t SENSOR_REFRESH_PERIOD_US = int32_t(1000000 / SENSOR_REFRESH_RATE_HZ);
#endif      

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
        AnkiWarn( "HAL.PushIMU.ArrayIsFull", "Dropping data");
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
      static int64_t lastAccTime, lastGyroTime;
#if IMU_INTERFACE == ANDROID_IMU      
      
      ASensorEvent event;


      // NOTE: For information on the meaning of event.timestamp: https://stackoverflow.com/a/41050188
      HAL::IMU_DataStructure imuData;
      while (ASensorEventQueue_getEvents(_sensorEventQueue, &event, 1) > 0) {
        if (event.type == ASENSOR_TYPE_ACCELEROMETER) {
//          AnkiDebug( "HAL.ProcessIMUEvents.Accel", "%d [%f]: %f, %f, %f", HAL::GetTimeStamp(), ((double)(event.timestamp-lastAccTime))/1000000000.0, event.acceleration.x, event.acceleration.y, event.acceleration.z);
          lastAccTime = event.timestamp;
          imuData.acc_x = event.acceleration.x * 1000;
          imuData.acc_y = event.acceleration.y * 1000;
          imuData.acc_z = event.acceleration.z * 1000;
        }
        else if (event.type == ASENSOR_TYPE_GYROSCOPE) {
//          AnkiDebug( "HAL.ProcessIMUEvents.Gyro", "%d [%f]: %f, %f, %f", HAL::GetTimeStamp(), ((double)(event.timestamp-lastGyroTime))/1000000000.0, event.vector.x, event.vector.y, event.vector.z);
          lastGyroTime = event.timestamp;
          imuData.rate_x = event.vector.x;
          imuData.rate_y = event.vector.y;
          imuData.rate_z = event.vector.z;
          PushIMU(imuData);
        }
      }
#else

      IMURawData rawData;
      HAL::IMU_DataStructure imuData;
      while ( imu_manage(&rawData) > 0 ) {
        imuData.acc_x = rawData.acc[0] * IMU_ACCEL_SCALE_G * MMPS2_PER_GEE;
        imuData.acc_y = rawData.acc[1] * IMU_ACCEL_SCALE_G * MMPS2_PER_GEE;
        imuData.acc_z = rawData.acc[2] * IMU_ACCEL_SCALE_G * MMPS2_PER_GEE;
        imuData.rate_x = rawData.gyro[0] * IMU_GYRO_SCALE_DPS * RADIANS_PER_DEGREE;
        imuData.rate_y = rawData.gyro[1] * IMU_GYRO_SCALE_DPS * RADIANS_PER_DEGREE;
        imuData.rate_z = rawData.gyro[2] * IMU_GYRO_SCALE_DPS * RADIANS_PER_DEGREE;
        imuData.temperature_degC = IMU_TEMP_RAW_TO_C(rawData.temperature);
        lastAccTime = lastGyroTime = rawData.timestamp * NS_PER_IMU_TICK;
        PushIMU(imuData);
        
        static ImageImuData imageImuData;
        imageImuData.systemTimestamp_ms = HAL::GetTimeStamp();
        imageImuData.rateX = imuData.rate_x;
        imageImuData.rateY = imuData.rate_y;
        imageImuData.rateZ = imuData.rate_z;
        RobotInterface::SendMessage(imageImuData);
      }

#endif
    }


    void InitIMU()
    {
#if IMU_INTERFACE == ANDROID_IMU      
      _sensorManager = ASensorManager_getInstance();
      AnkiConditionalErrorAndReturn(_sensorManager != nullptr, "HAL.InitIMU.NullSensorManager", "");

      _accelerometer = ASensorManager_getDefaultSensor(_sensorManager, ASENSOR_TYPE_ACCELEROMETER);
      AnkiConditionalErrorAndReturn(_accelerometer != nullptr, "HAL.InitIMU.NullAccelerometer", "");

      _gyroscope = ASensorManager_getDefaultSensor(_sensorManager, ASENSOR_TYPE_GYROSCOPE);
      AnkiConditionalErrorAndReturn(_gyroscope != nullptr, "HAL.InitIMU.NullGyroscope", "");

      _looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
      AnkiConditionalErrorAndReturn(_looper != nullptr, "HAL.InitIMU.NullLooper", "");

      AnkiDebug( "HAL.InitIMU.AccMinDelay", "%d us", ASensor_getMinDelay(_accelerometer));
      AnkiDebug( "HAL.InitIMU.GyroMinDelay", "%d us", ASensor_getMinDelay(_gyroscope));

      _sensorEventQueue = ASensorManager_createEventQueue(_sensorManager, _looper, 0, nullptr, nullptr);

      AnkiConditionalErrorAndReturn(_sensorEventQueue != nullptr, "HAL.InitIMU.CreateEventQueueFailed", "");

      auto status = ASensorEventQueue_enableSensor(_sensorEventQueue, _accelerometer);
      AnkiConditionalErrorAndReturn(status >= 0, "HAL.InitIMU.AccelEnableFailed", "");

      status = ASensorEventQueue_enableSensor(_sensorEventQueue, _gyroscope);
      AnkiConditionalErrorAndReturn(status >= 0, "HAL.InitIMU.GyroEnableFailed", "");

      // Set event rate hint
      status = ASensorEventQueue_setEventRate(_sensorEventQueue, _accelerometer, SENSOR_REFRESH_PERIOD_US);
      AnkiConditionalErrorAndReturn(status >= 0, "HAL.InitIMU.AccelSetRateFailed", "");

      status = ASensorEventQueue_setEventRate(_sensorEventQueue, _gyroscope, SENSOR_REFRESH_PERIOD_US);
      AnkiConditionalErrorAndReturn(status >= 0, "HAL.InitIMU.GyroSetRateFailed", "");
      (void)status;   //to silence unused compiler warning
#else
      const char* err = imu_open();
      //TODO: conditional err and return
      if (err) {printf("%s",err);return;}
      imu_init();
#endif      
    }


    bool HAL::IMUReadData(HAL::IMU_DataStructure &imuData)
    {
#if IMU_INTERFACE == ANDROID_IMU
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
