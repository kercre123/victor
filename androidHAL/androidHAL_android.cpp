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

#include "androidHAL/android/proto_camera/victor_camera.h"

#include <vector>
#include <chrono>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using androidHAL.cpp
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      // Pointer to the current (latest) frame the camera has given us
      uint8_t* _currentFrame = nullptr;
      bool     _frameReady = false;
      TimeStamp_t _currentFrameSystemTimestamp_ms = 0;
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
    , _imageFrameID(1)
    {
      InitCamera();
    }
    
    AndroidHAL::~AndroidHAL()
    {
      DeleteCamera();
    }

    int CameraCallback(uint8_t* image, int width, int height)
    {
      DEV_ASSERT(image != nullptr, "AndroidHAL.CameraCallback.NullImage");
      _currentFrame = image;

      _currentFrameSystemTimestamp_ms = AndroidHAL::getInstance()->GetTimeStamp();

      _frameReady = true;
      return 0;
    }

    void AndroidHAL::InitCamera()
    {
      PRINT_NAMED_INFO("AndroidHAL.InitCamera.StartingInit", "");

      int res = camera_init();
      DEV_ASSERT(res == 0, "AndroidHAL.InitCamera.CameraInitFailed");
      
      res = camera_start(&CameraCallback);
      DEV_ASSERT(res == 0, "AndroidHAL.InitCamera.CameraStartFailed");
    }

    void AndroidHAL::DeleteCamera() {
      int res = camera_stop();
      DEV_ASSERT(res == 0, "AndroidHAL.Delete.CameraStopFailed");

      res = camera_cleanup();
      DEV_ASSERT(res == 0, "AndroidHAL.Delete.CameraCleanupFailed");
    }
    
    Result AndroidHAL::Update()
    {
      return RESULT_OK;
    }
    
    TimeStamp_t AndroidHAL::GetTimeStamp(void)
    {
      auto currTime = std::chrono::steady_clock::now();
      return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::milliseconds>(currTime.time_since_epoch()).count());
    }

    void AndroidHAL::CameraSetParameters(u16 exposure_ms, f32 gain)
    {
      // STUB
      return;
    }

    bool AndroidHAL::CameraGetFrame(u8*& frame, u32& imageID, TimeStamp_t& imageCaptureSystemTimestamp_ms)
    {
      if(_currentFrame != nullptr && _frameReady)
      {
        _frameReady = false;
        
        frame = _currentFrame;

        imageCaptureSystemTimestamp_ms = _currentFrameSystemTimestamp_ms;
        
        imageID = ++_imageFrameID;
        
        return true;
      }

      return false;
    } // CameraGetFrame()
    
    void AndroidHAL::CameraSwapLocks()
    {
      camera_swap_locks();
    }
  } // namespace Cozmo
} // namespace Anki
