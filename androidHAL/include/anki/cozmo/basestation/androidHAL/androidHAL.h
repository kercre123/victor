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
#include "anki/common/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/imageTypes.h"
#include "clad/types/cameraParams.h"


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
      
      // TODO: Is this necessary?
      TimeStamp_t GetTimeStamp();
      
#ifdef SIMULATOR
      // TODO: Break this out into sim_androidHAL.h?
      //       Need to make AndroidHAL namespace though.
      
      // Take a step (needed for webots only)
      Result Update();
      
      // NOTE: Only NVStorageComponent::LoadSimData() should call this function.
      //       Everyone else should be getting CameraCalibration data from NVStorageComponent!
      const CameraCalibration* GetHeadCamInfo();
#endif

// #pragma mark --- IMU ---
      /////////////////////////////////////////////////////////////////////
      // Inertial measurement unit (IMU)
      //

      // IMU_DataStructure contains 3-axis acceleration and 3-axis gyro data
      struct IMU_DataStructure
      {
        void Reset() {
          acc_x = acc_y = acc_z = 0;
          rate_x = rate_y = rate_z = 0;
        }
        
        f32 acc_x;      // mm/s/s
        f32 acc_y;
        f32 acc_z;
        f32 rate_x;     // rad/s
        f32 rate_y;
        f32 rate_z;
      };

      // Read acceleration and rate
      // x-axis points out cozmo's face
      // y-axis points out of cozmo's left
      // z-axis points out the top of cozmo's head
      bool IMUReadData(IMU_DataStructure &IMUData);


// #pragma mark --- Cameras ---
      /////////////////////////////////////////////////////////////////////
      // CAMERAS
      // TODO: Add functions for adjusting ROI of cameras?
      //

      void CameraGetParameters(DefaultCameraParams& params);

      // Sets the camera parameters (non-blocking call)
      void CameraSetParameters(u16 exposure_ms, f32 gain);

      // Starts camera frame synchronization (blocking call)
      // Returns image ID
      // TODO: How fast will this be in hardware? Is image ready and waiting?
      u32 CameraGetFrame(u8* frame, ImageResolution res, std::vector<ImageImuData>& imuData);

      
// #pragma mark --- Face ---
      /////////////////////////////////////////////////////////////////////
      // Face


    private:

      AndroidHAL();
      static AndroidHAL* _instance;

#ifdef SIMULATOR
      CameraCalibration headCamInfo_;
#endif
      
      u32 imageFrameID_ = 1;
      
    }; // class AndroidHAL
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANDROID_HARDWAREINTERFACE_H
