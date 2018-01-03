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
      // Pointer to the current (latest) frame the camera has given us (points to a raw frame)
      uint64_t* _currentFrame = nullptr;
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
      _currentFrame = (uint64_t*)image;

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

    #define X 640
    #define Y 360
    #define X6 214
    #define MASK_10BIT 1023
    // A raw frame is 720x214 where each element is 64 bits and contains 6 color channels
    // (each color channel is 10bits)
    // The least significant bits contain the left-most color channel so the 4 most significant bits are unused
    // A raw frame is in a 2x2 bayer format, each 2x2 area will become one pixel in the output image
    //
    // R G R G ...
    // G B G B ...
    // R G R G ...
    // ...
    //
    // (0,0) is red
    static void downsample_frame(uint64_t in[Y*2][X6], uint8_t out[Y][X][3]) {
      int x = 0, y = 0;
      for (y = 0; y < Y; y++)
      {
        for (x = 0; x < X6; x++)
        {          
          unsigned long long row1 = in[y*2][x], row2 = in[y*2+1][x];
          int raw10bits;
          
          uint8_t* pixel0 = (uint8_t*)&(out[y][x*3+0]);
          uint8_t* pixel1 = pixel0 + 3; // Pointer math: Add three to move forwards a pixel (3 uint8_ts)
          uint8_t* pixel2 = pixel1 + 3;
          
          // Red
          raw10bits = (((row1>>00) & MASK_10BIT));
          pixel0[0] = (raw10bits > 255) ? 255 : raw10bits;
          
          raw10bits = (((row1>>20) & MASK_10BIT));
          pixel1[0] = (raw10bits > 255) ? 255 : raw10bits;
          
          raw10bits = (((row1>>40) & MASK_10BIT));
          pixel2[0] = (raw10bits > 255) ? 255 : raw10bits;
          
          // Green (two green per 2x2 square so add and divide by two)
          raw10bits = (((row1>>10) & MASK_10BIT) + ((row2>>00) & MASK_10BIT)) >> 1;
          pixel0[1] = (raw10bits > 255) ? 255 : raw10bits;
          
          raw10bits = (((row1>>30) & MASK_10BIT) + ((row2>>20) & MASK_10BIT)) >> 1;
          pixel1[1] = (raw10bits > 255) ? 255 : raw10bits;
          
          raw10bits = (((row1>>50) & MASK_10BIT) + ((row2>>40) & MASK_10BIT)) >> 1;
          pixel2[1] = (raw10bits > 255) ? 255 : raw10bits;
          
          // Blue
          raw10bits = (((row2>>10) & MASK_10BIT));
          pixel0[2] = (raw10bits > 255) ? 255 : raw10bits;
          
          raw10bits = (((row2>>30) & MASK_10BIT));
          pixel1[2] = (raw10bits > 255) ? 255 : raw10bits;
          
          raw10bits = (((row2>>50) & MASK_10BIT));
          pixel2[2] = (raw10bits > 255) ? 255 : raw10bits;
        }
      }
    }

    bool AndroidHAL::CameraGetFrame(u8*& frame, u32& imageID, TimeStamp_t& imageCaptureSystemTimestamp_ms)
    {
      camera_request_frame();

      if(_currentFrame != nullptr && _frameReady)
      {
        _frameReady = false;

        // Tell the camera we will be processing this frame
        camera_set_processing_frame();

        // Downsample the raw frame into tempFrame and give the engine a pointer to that static memory
        static uint8_t tempFrame[Y][X][3];
        downsample_frame((uint64_t (*)[X6])_currentFrame, (uint8_t (*)[X][3])tempFrame);
        
        frame = (uint8_t*)&tempFrame;

        imageCaptureSystemTimestamp_ms = _currentFrameSystemTimestamp_ms;
        
        imageID = ++_imageFrameID;
        
        return true;
      }

      return false;
    } // CameraGetFrame()
    
  } // namespace Cozmo
} // namespace Anki
