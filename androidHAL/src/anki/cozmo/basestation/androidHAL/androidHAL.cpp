/**
 * File: androidHAL.cpp
 *
 * Author: Kevin Yoon
 * Created: 02/17/2017
 *
 * Description:
 *               Defines interface to all hardware accessible from Android
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/androidHAL/androidHAL.h"
#include "util/logging/logging.h"

// Android includes
#include <android/sensor.h>


#include <vector>
#include <chrono>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using androidHAL.cpp
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"

      // Time
      std::chrono::steady_clock::time_point _timeOffset = std::chrono::steady_clock::now();
      
      
      // Android sensor (i.e. IMU)
      ASensorManager*    _sensorManager;
      const ASensor*     _accelerometer;
      const ASensor*     _gyroscope;
      ASensorEventQueue* _sensorEventQueue;
      ALooper*           _looper;
      
      const int SENSOR_REFRESH_RATE_HZ = 16;
      constexpr int32_t SENSOR_REFRESH_PERIOD_US = int32_t(1000000 / SENSOR_REFRESH_RATE_HZ);
      
    } // "private" namespace


#pragma mark --- Simulated Hardware Method Implementations ---
    
    
    // Definition of static field
    AndroidHAL* AndroidHAL::_instance = 0;
    
    /**
     * Returns the single instance of the object.
     */
    AndroidHAL* AndroidHAL::getInstance() {
      // check if the instance has been created yet
      if(0 == _instance) {
        // if not, then create it
        _instance = new AndroidHAL;
      }
      // return the single instance
      return _instance;
    }
    
    /**
     * Removes instance
     */
    void AndroidHAL::removeInstance() {
      // check if the instance has been created yet
      if(0 != _instance) {
        delete _instance;
        _instance = 0;
      }
    };
    
    AndroidHAL::AndroidHAL()
    {
      InitIMU();
    }

    
    // TODO: Move all IMU functions to separate file... if they're even needed?
    void AndroidHAL::ProcessIMUEvents() {
      ASensorEvent event;
      
      static int64_t lastAccTime, lastGyroTime;
      
      // TODO: Don't need accel in engine. Remove?
      
      while (ASensorEventQueue_getEvents(_sensorEventQueue, &event, 1) > 0) {
        if(event.type == ASENSOR_TYPE_ACCELEROMETER) {
//          PRINT_NAMED_INFO("ProcessAndroidSensorEvents.Accel", "%d [%f]: %f, %f, %f", GetTimeStamp(), ((double)(event.timestamp-lastAccTime))/1000000000.0, event.acceleration.x, event.acceleration.y, event.acceleration.z);
          lastAccTime = event.timestamp;
        }
        else if(event.type == ASENSOR_TYPE_GYROSCOPE) {
//          PRINT_NAMED_INFO("ProcessAndroidSensorEvents.Gyro", "%d [%f]: %f, %f, %f", GetTimeStamp(), ((double)(event.timestamp-lastGyroTime))/1000000000.0, event.vector.x, event.vector.y, event.vector.z);
          lastGyroTime = event.timestamp;
        }
      }
    }

    
    void AndroidHAL::InitIMU()
    {
      _sensorManager = ASensorManager_getInstance();
      DEV_ASSERT(_sensorManager != nullptr, "AndroidHAL.Init.NullSensorManager");
      
      _accelerometer = ASensorManager_getDefaultSensor(_sensorManager, ASENSOR_TYPE_ACCELEROMETER);
      DEV_ASSERT(_accelerometer != nullptr, "AndroidHAL.Init.NullAccelerometer");
      
      _gyroscope = ASensorManager_getDefaultSensor(_sensorManager, ASENSOR_TYPE_GYROSCOPE);
      DEV_ASSERT(_gyroscope != nullptr, "AndroidHAL.Init.NullGyroscope");
      
      _looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
      DEV_ASSERT(_looper != nullptr, "AndroidHAL.Init.NullLooper");
      
      _sensorEventQueue = ASensorManager_createEventQueue(_sensorManager, _looper, 0, nullptr, nullptr);
      //_sensorEventQueue = ASensorManager_createEventQueue(_sensorManager, _looper, ALOOPER_POLL_CALLBACK, GetSensorEvents, _sensorDataBuf);
      DEV_ASSERT(_sensorEventQueue != nullptr, "AndroidHAL.Init.NullEventQueue");
      
      auto status = ASensorEventQueue_enableSensor(_sensorEventQueue, _accelerometer);
      DEV_ASSERT(status >= 0, "AndroidHAL.Init.AccelEnableFailed");
      
      status = ASensorEventQueue_enableSensor(_sensorEventQueue, _gyroscope);
      DEV_ASSERT(status >= 0, "AndroidHAL.Init.GyroEnableFailed");

      // Set rate hint
      status = ASensorEventQueue_setEventRate(_sensorEventQueue, _accelerometer, SENSOR_REFRESH_PERIOD_US);
      DEV_ASSERT(status >= 0, "AndroidHAL.Init.AccelSetRateFailed");

      status = ASensorEventQueue_setEventRate(_sensorEventQueue, _gyroscope, SENSOR_REFRESH_PERIOD_US);
      DEV_ASSERT(status >= 0, "AndroidHAL.Init.GyroSetRateFailed");
      (void)status;   //to silence unused compiler warning
    }

    
    Result AndroidHAL::Update()
    {
      ProcessIMUEvents();
      
      return RESULT_OK;
    }
    
    
    TimeStamp_t AndroidHAL::GetTimeStamp(void)
    {
      auto currTime = std::chrono::steady_clock::now();
      return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::milliseconds>(currTime - _timeOffset).count());
    }

    bool AndroidHAL::IMUReadData(IMU_DataStructure &IMUData)
    {
      // STUB
      return false;
    }
    
    void AndroidHAL::CameraGetParameters(DefaultCameraParams& params)
    {
      // STUB
      return;
    }

    void AndroidHAL::CameraSetParameters(u16 exposure_ms, f32 gain)
    {
      // STUB
      return;
    }

    u32 AndroidHAL::CameraGetFrame(u8* frame, ImageResolution res, std::vector<ImageImuData>& imuData )
    {
      DEV_ASSERT(frame != NULL, "androidHAL.CameraGetFrame.NullFramePointer");
      return imageFrameID_++;

    } // CameraGetFrame()
    
    
  } // namespace Cozmo
} // namespace Anki
