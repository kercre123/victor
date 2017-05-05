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

#include <vector>
#include <chrono>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using androidHAL.cpp
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"

      // Time
      std::chrono::steady_clock::time_point timeOffset_ = std::chrono::steady_clock::now();
      
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
    }


    TimeStamp_t AndroidHAL::GetTimeStamp(void)
    {
      auto currTime = std::chrono::steady_clock::now();
      return static_cast<TimeStamp_t>(std::chrono::duration_cast<std::chrono::milliseconds>(currTime - timeOffset_).count());
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
