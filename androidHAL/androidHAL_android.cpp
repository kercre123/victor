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

#include "androidHAL/androidHAL.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

// Android IMU
#include <android/sensor.h>

// Android camera
#include "androidHAL/android/camera/camera_manager.h"
#include "androidHAL/android/camera/image_reader.h"
#include "androidHAL/android/camera/utils/native_debug.h"
#include "androidHAL/android/fugly_camera/victor_camera.h"
#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/image.h"

#include <vector>
#include <chrono>

#include <fstream>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using androidHAL.cpp
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      CameraHandle _cameraHandle = nullptr;
      uint8_t* _latestFrame = nullptr;
      int _width = 0;
      int _height = 0;
      bool _cameraRunning = false;
      bool _waitingForFrame = false;
      bool _copyingNewFrame = false;
      bool _cameraInited = false;
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
      Util::SafeDelete(_instance);
    };
    
    AndroidHAL::AndroidHAL()
    : _timeOffset(std::chrono::steady_clock::now())
    , _sensorManager(nullptr)
    , _accelerometer(nullptr)
    , _gyroscope(nullptr)
    , _sensorEventQueue(nullptr)
    , _looper(nullptr)
    , _androidCamera(nullptr)
    , _reader(nullptr)
    , _imageCaptureResolution(ImageResolution::QVGA)
    , _imageFrameID(1)
    {
      //InitIMU();
     InitCamera();
    }
    
    AndroidHAL::~AndroidHAL()
    {
      DeleteCamera();
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

    int CameraCallback(const uint8_t* image, int width, int height)
    {
        // PRINT_NAMED_WARNING("Camera callback", "Width: %d Height: %d",width, height);
      if(_latestFrame == nullptr)
      {
        _latestFrame = (uint8_t*)malloc(width*height*sizeof(image));
        _width = width;
        _height = height;
      }
      _copyingNewFrame = true;
      std::memcpy(_latestFrame, image, width*height);
      
//      static int count = 0;
//      if(count < 20)
//      {
//        PRINT_NAMED_WARNING("Camera callback", "Width: %d Height: %d",width, height);

//        const char* buf = reinterpret_cast<const char*>(image);
//        std::ofstream f("/data/data/com.anki.cozmoengine/images/" + std::to_string(count) + ".jpg", std::ofstream::binary);
//        f.write(buf, width*height);
//        f.close();
//        ++count;
//      }
      
      _copyingNewFrame = false;
      
      _waitingForFrame = false;
      return 0;
    }

    void AndroidHAL::StartCamera()
    {
//      if(!_cameraInited)
//      {
//        return;
//      }
//      
//      if(!_cameraRunning && !_waitingForFrame)
//      {
//        _cameraRunning = true;
//        _waitingForFrame = true;
//        int res = camera_start(_cameraHandle, &CameraCallback);
//        PRINT_NAMED_WARNING("Camera start", "%d", res);
//      }
    }
    
    void AndroidHAL::StopCamera()
    {
//      if(!_cameraInited)
//      {
//        return;
//      }
//    
//      if(_cameraRunning && !_waitingForFrame)
//      {
//        int res = camera_stop(_cameraHandle);
//        PRINT_NAMED_WARNING("Camera stop", "%d", res);
//        _cameraRunning = false;
//        _waitingForFrame = false;
//      }
    }

    void AndroidHAL::InitCamera()
    {
      PRINT_NAMED_INFO("AndroidHAL.InitCamera.StartingInit", "");
      
      _cameraHandle = camera_alloc();

      int res = camera_init(_cameraHandle);
      PRINT_NAMED_WARNING("Camera init", "%d", res);
      
      res = camera_start(_cameraHandle, &CameraCallback);
      PRINT_NAMED_WARNING("Camera start", "%d", res);


      _cameraInited = true;
      // DeleteCamera();
      
      // _androidCamera = new NativeCamera(nullptr);
      // ASSERT(_androidCamera, "Failed to Create CameraObject");
      
      // // Get image resolution info
      // const int cameraRes = static_cast<const int>(_imageCaptureResolution);
      // const int width = Vision::CameraResInfo[cameraRes].width;
      // const int height = Vision::CameraResInfo[cameraRes].height;

      // PRINT_NAMED_INFO("AndroidHAL.InitCamera.StartingCaptureSession", "%d x %d", width, height);
      // ImageResolution_Android res {width, height, 0};
      // _reader= new ImageReader(&res);
      
      // _androidCamera->CreateSession(_reader->GetNativeWindow());
      // _androidCamera->Animate();
    }

    void AndroidHAL::DeleteCamera() {
      Util::SafeDelete(_androidCamera);
      Util::SafeDelete(_reader);

      int res = camera_stop(_cameraHandle);
      PRINT_NAMED_WARNING("Camera stop", "%d", res);

      res = camera_cleanup(_cameraHandle);
      PRINT_NAMED_WARNING("Camera cleanup", "%d", res);

      free(_cameraHandle);
    }
    
    Result AndroidHAL::Update()
    {
      //ProcessIMUEvents();

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

    bool AndroidHAL::CameraGetFrame(u8* frame, u32& imageID, std::vector<ImageImuData>& imuData )
    {
      DEV_ASSERT(frame != NULL, "androidHAL.CameraGetFrame.NullFramePointer");

      if(_latestFrame != nullptr && !_copyingNewFrame)
      {
        std::memcpy(frame, _latestFrame, _width*_height);
        imageID = ++_imageFrameID;

        ImageImuData imu_meas(imageID,
          0.f, 0.f, 0.f,
          125);          // IMU data point for middle of this image

        imuData.push_back(imu_meas);

        // Include IMU data for beginning of the next image (for rolling shutter correction purposes)
        imu_meas.imageId = imageID + 1;
        imu_meas.line2Number = 1;
        imuData.push_back(imu_meas);
        
        return true;
      }

      return false;

      // if (_reader && _reader->IsReady()) {
      //   u32 dataLength;
      //   _reader->GetLatestRGBImage(frame, dataLength);
      //   imageID = ++_imageFrameID;
        
      //   // --------------------------------------------------------------------
      //   // TEMP: Image-imu sync isn't implemented yet so, just fake the imu data for now.
      //   // See sim_hal::IMUGetCameraTime() for explanation of line2Number
      //   ImageImuData imu_meas(imageID,
      //                         0.f, 0.f, 0.f,
      //                         125);          // IMU data point for middle of this image

      //   imuData.push_back(imu_meas);
        
      //   // Include IMU data for beginning of the next image (for rolling shutter correction purposes)
      //   imu_meas.imageId = imageID + 1;
      //   imu_meas.line2Number = 1;
      //   imuData.push_back(imu_meas);
      //   // --------------------------------------------------------------------

      //   return true;
      // }
      
      // return false;

    } // CameraGetFrame()
    
    
    void AndroidHAL::FaceClear() {
      // Stub
    }
    
    void AndroidHAL::FaceDraw(u16* frame) {
      // Stub
    }
    
    void AndroidHAL::FacePrintf(const char *format, ...) {
      // Stub
    }
    

  } // namespace Cozmo
} // namespace Anki
