/**
 * File: sim_androidHAL.cpp
 *
 * Author: Kevin Yoon
 * Created: 02/17/2017
 *
 * Description:
 *               Defines interface to all simulated hardware accessible from Android
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/androidHAL/androidHAL.h"
#include "util/logging/logging.h"

#include <vector>

#include <webots/Supervisor.hpp>
#include <webots/Camera.hpp>
#include <webots/Display.hpp>
#include <webots/Gyro.hpp>
#include <webots/Accelerometer.hpp>

#define BLUR_CAPTURED_IMAGES 1

#if BLUR_CAPTURED_IMAGES
#include "opencv2/imgproc/imgproc.hpp"
#endif

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using sim_androidHAL.cpp
#endif

namespace Anki {
  namespace Cozmo {
    
    webots::Supervisor* _engineSupervisor = nullptr;
    
    namespace { // "Private members"

      // Const paramters / settings
      const u32 VISION_TIME_STEP = 65; // This should be a multiple of the world's basic time step!

      // Cameras / Vision Processing
      webots::Camera* headCam_;
      
      // IMU
      webots::Gyro* gyro_;
      webots::Accelerometer* accel_;

    } // "private" namespace


#pragma mark --- Simulated Hardware Method Implementations ---
    
    // Declarations
    void FillCameraInfo(const webots::Camera *camera, CameraCalibration &info);
    
    
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
    
    void AndroidHAL::SetSupervisor(webots::Supervisor *sup)
    {
      _engineSupervisor = sup;
    }
    
    AndroidHAL::AndroidHAL()
    {
      // Did you remember to call SetSupervisor()?
      DEV_ASSERT(_engineSupervisor != nullptr, "sim_androidHAL.NullWebotsSupervisor");
      
      // Is the step time defined in the world file >= than the robot time? It should be!
      DEV_ASSERT(TIME_STEP >= _engineSupervisor->getBasicTimeStep(), "sim_androidHAL.UnexpectedTimeStep");

      headCam_ = _engineSupervisor->getCamera("HeadCamera");

      if(VISION_TIME_STEP % static_cast<u32>(_engineSupervisor->getBasicTimeStep()) != 0) {
        PRINT_NAMED_WARNING("sim_androidHAL.InvalidVisionTimeStep",
                            "VISION_TIME_STEP (%d) must be a multiple of the world's basic timestep (%.0f).",
                            VISION_TIME_STEP, _engineSupervisor->getBasicTimeStep());
        return;
      }
      headCam_->enable(VISION_TIME_STEP);
      FillCameraInfo(headCam_, headCamInfo_);

      // Gyro
      gyro_ = _engineSupervisor->getGyro("gyro");
      gyro_->enable(TIME_STEP);

      // Accelerometer
      accel_ = _engineSupervisor->getAccelerometer("accel");
      accel_->enable(TIME_STEP);

    }


    TimeStamp_t AndroidHAL::GetTimeStamp(void)
    {
      return static_cast<TimeStamp_t>(_engineSupervisor->getTime() * 1000.0);
    }

    // TODO: If we want higher resolution IMU data than this we need to change this
    //       function and tic the supervisor at a faster rate than engine.
    bool AndroidHAL::IMUReadData(IMU_DataStructure &IMUData)
    {
      const double* vals = gyro_->getValues();  // rad/s
      IMUData.rate_x = (f32)(vals[0]);
      IMUData.rate_y = (f32)(vals[1]);
      IMUData.rate_z = (f32)(vals[2]);

      vals = accel_->getValues();   // m/s^2
      IMUData.acc_x = (f32)(vals[0] * 1000);  // convert to mm/s^2
      IMUData.acc_y = (f32)(vals[1] * 1000);
      IMUData.acc_z = (f32)(vals[2] * 1000);
      
      // Return true if IMU was already read this timestamp
      static TimeStamp_t lastReadTimestamp = 0;
      bool newReading = lastReadTimestamp != GetTimeStamp();
      lastReadTimestamp = GetTimeStamp();
      return newReading;
    }
    

    Result AndroidHAL::Update()
    {

      if(_engineSupervisor->step(Cozmo::TIME_STEP) == -1) {
        return RESULT_FAIL;
      } else {
        //AudioUpdate();
        return RESULT_OK;
      }

    } // step()


    // Helper function to create a CameraInfo struct from Webots camera properties:
    void FillCameraInfo(const webots::Camera *camera,
                        CameraCalibration &info)
    {

      const u16 nrows  = static_cast<u16>(camera->getHeight());
      const u16 ncols  = static_cast<u16>(camera->getWidth());
      const f32 width  = static_cast<f32>(ncols);
      const f32 height = static_cast<f32>(nrows);
      //f32 aspect = width/height;

      const f32 fov_hor = camera->getFov();

      // Compute focal length from simulated camera's reported FOV:
      const f32 f = width / (2.f * std::tan(0.5f*fov_hor));

      // There should only be ONE focal length, because simulated pixels are
      // square, so no need to compute/define a separate fy
      //f32 fy = height / (2.f * std::tan(0.5f*fov_ver));

      info.focalLength_x = f;
      info.focalLength_y = f;
      info.center_x      = 0.5f*(width-1);
      info.center_y      = 0.5f*(height-1);
      info.skew          = 0.f;
      info.nrows         = nrows;
      info.ncols         = ncols;

      for(u8 i=0; i<NUM_RADIAL_DISTORTION_COEFFS; ++i) {
        info.distCoeffs[i] = 0.f;
      }

    } // FillCameraInfo

    const CameraCalibration* AndroidHAL::GetHeadCamInfo(void)
    {
      return &headCamInfo_;
    }

    
    void AndroidHAL::CameraGetParameters(DefaultCameraParams& params)
    {
      params.minExposure_ms = 0;
      params.maxExposure_ms = 67;
      params.gain = 2.f;
      params.maxGain = 4.f;
      
      u8 count = 0;
      for(u8 i = 0; i < static_cast<u8>(CameraConstants::GAMMA_CURVE_SIZE); ++i)
      {
        params.gammaCurve[i] = count;
        count += 255/static_cast<u8>(CameraConstants::GAMMA_CURVE_SIZE);
      }
      
      return;
    }

    void AndroidHAL::CameraSetParameters(u16 exposure_ms, f32 gain)
    {
      // Can't control simulated camera's exposure.

      // TODO: Simulate this somehow?

      return;

    } // HAL::CameraSetParameters()


    // Starts camera frame synchronization
    u32 AndroidHAL::CameraGetFrame(u8* frame, ImageResolution res, std::vector<ImageImuData>& imuData )
    {
      DEV_ASSERT(frame != NULL, "sim_androidHAL.CameraGetFrame.NullFramePointer");

      const u8* image = headCam_->getImage();
      DEV_ASSERT(image != NULL, "sim_androidHAL.CameraGetFrame.NullImagePointer");

      s32 pixel = 0;
      s32 imgWidth = headCam_->getWidth();
      for (s32 y=0; y < headCamInfo_.nrows; y++) {
        for (s32 x=0; x < headCamInfo_.ncols; x++) {
          frame[pixel++] = webots::Camera::imageGetRed(image, imgWidth, x, y);
          frame[pixel++] = webots::Camera::imageGetGreen(image, imgWidth, x, y);
          frame[pixel++] = webots::Camera::imageGetBlue(image,  imgWidth, x, y);
        }
      }

#     if BLUR_CAPTURED_IMAGES
      // Add some blur to simulated images
      cv::Mat cvImg(headCamInfo_.nrows, headCamInfo_.ncols, CV_8UC3, frame);
      cv::GaussianBlur(cvImg, cvImg, cv::Size(0,0), 0.75f);
#     endif

      
      // Return a few pieces of ImageImuData.
      // Webots camera has no global shutter so sending the current IMU values
      // for all ImageImuData messages should be sufficient.
      IMU_DataStructure imu;
      IMUReadData(imu);
      
      ImageImuData data;
      data.imageId = imageFrameID_;
      data.rateX = imu.rate_x;
      data.rateY = imu.rate_y;
      data.rateZ = imu.rate_z;
      
      // IMU data point for middle of this image
      // See sim_hal::IMUGetCameraTime() for explanation of line2Number
      data.line2Number = 125;
      imuData.push_back(data);
      
      // Include IMU data for beginning of the next image (for rolling shutter correction purposes)
      data.imageId = imageFrameID_ + 1;
      data.line2Number = 1;
      imuData.push_back(data);
      
      return imageFrameID_++;

    } // CameraGetFrame()
        
  } // namespace Cozmo
} // namespace Anki
