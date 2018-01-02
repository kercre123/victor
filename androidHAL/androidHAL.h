/**
 * File: androidHAL.h
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

#ifndef ANKI_COZMO_ANDROID_HARDWAREINTERFACE_H
#define ANKI_COZMO_ANDROID_HARDWAREINTERFACE_H
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/imageTypes.h"

// Forward declaration
namespace webots {
  class Supervisor;
}

namespace Anki
{
  namespace Cozmo
  {
    class AndroidHAL
    {
    public:

      // Method to fetch singleton instance.
      static AndroidHAL* getInstance();
      
      // Removes instance
      static void removeInstance();
      
      // Dtor
      ~AndroidHAL();
      
      // TODO: Is this necessary?
      TimeStamp_t GetTimeStamp();
      
      Result Update();
  
#ifdef SIMULATOR
      // NOTE: Only NVStorageComponent::LoadSimData() should call this function.
      //       Everyone else should be getting CameraCalibration data from NVStorageComponent!
      const CameraCalibration* GetHeadCamInfo();
      
      // Assign Webots supervisor
      // Webots processes must do this before creating AndroidHAL for the first time.
      // Unit test processes must call SetSupervisor(nullptr) to run without a supervisor.
      static void SetSupervisor(webots::Supervisor *sup);
      
#endif

// #pragma mark --- Cameras ---
      /////////////////////////////////////////////////////////////////////
      // CAMERAS
      // TODO: Add functions for adjusting ROI of cameras?
      //
      
      void InitCamera();
      
      // Sets the camera parameters (non-blocking call)
      void CameraSetParameters(u16 exposure_ms, f32 gain);

      // Points provided frame to a buffer of image data if available
      // Returns true if image available
      // TODO: How fast will this be in hardware? Is image ready and waiting?
      bool CameraGetFrame(u8*& frame, u32& imageID, TimeStamp_t& imageCaptureSystemTimestamp_ms);
      
      u16 CameraGetHeight() const {return _imageCaptureHeight;}
      u16 CameraGetWidth()  const {return _imageCaptureWidth; }
      
    private:

      AndroidHAL();
      static AndroidHAL* _instance;

      void DeleteCamera();
      
#ifdef SIMULATOR
      CameraCalibration headCamInfo_;
#else
      
      // Time
      std::chrono::steady_clock::time_point _timeOffset;
#endif
      
      // Camera
      u16             _imageCaptureHeight = DEFAULT_CAMERA_RESOLUTION_HEIGHT;
      u16             _imageCaptureWidth  = DEFAULT_CAMERA_RESOLUTION_WIDTH;
      u32             _imageFrameID;
      
      
    }; // class AndroidHAL
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANDROID_HARDWAREINTERFACE_H
