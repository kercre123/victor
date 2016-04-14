/**
 * File: rollingShutterCorrector.h
 *
 * Author: Al Chaussee
 * Created: 4-1-2016
 *
 * Description:
 *    Class for handling rolling shutter correction and imu data history
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef ANKI_COZMO_ROLLING_SHUTTER_CORRECTOR_H
#define ANKI_COZMO_ROLLING_SHUTTER_CORRECTOR_H

#include "anki/vision/basestation/image.h"
#include "anki/common/basestation/math/rotation.h"
#include <deque>
#include <vector>

namespace Anki {
  
  namespace Vision {
    class Camera;
  }

  namespace Cozmo {
  
    class Robot;
    struct VisionPoseData;
    
    class ImuDataHistory
    {
      public:
        ImuDataHistory() {};
      
        void AddImuData(u32 imageId, float rateX, float rateY, float rateZ, u8 line2Number);
      
        // Calculate timestamps for ImageIMU messages since they only have frameIDs and lineNumbers
        void CalculateTimestampForImageIMU(u32 imageId, TimeStamp_t t, f32 period, int height);
      
        struct ImuData
        {
          u32 imageId;
          float rateX;
          float rateY;
          float rateZ;
          u8 line2Number;
          TimeStamp_t timestamp = 0;
        };
      
        std::deque<ImuData>::const_iterator begin() const { return _history.begin(); }
        std::deque<ImuData>::const_iterator end() const { return _history.end(); }
        ImuData front() const { return _history.front(); }
        ImuData back() const { return _history.back(); }
      
      private:
        std::deque<ImuData> _history;
        static const int maxSizeOfHistory = 20;
    };
    
    class RollingShutterCorrector
    {
    public:
      RollingShutterCorrector(Vision::Camera& camera);
    
      // Calculates the warp matricies to account for rolling shutter
      void ComputeWarps(const VisionPoseData& poseData,
                        const VisionPoseData& prevPoseData);
      
      Vision::Image WarpImage(const Vision::Image& img);
      
      // Saves the historical camera rotations for each of the subdivided rows in the image we will be correction
      std::vector<RotationMatrix3d> PrecomputeHistoricalCameraRotations(Robot& robot, TimeStamp_t timestamp) const;
      
      // Calculates our rotation at time t using gyro rates from ImageIMUData messages
      // Returns false if unable to calculate rotation
      bool ComputeCameraRotationWithImageIMU(TimeStamp_t t,
                                             RotationMatrix3d& r,
                                             const VisionPoseData& poseData,
                                             const VisionPoseData& prevPoseData);
      
      std::vector<Matrix_3x3f> GetRollingShutterWarps() const { return _rollingShutterWarps; }
      int GetNumDivisions() const { return _rsNumDivisions; }
      
      static constexpr f32 timeBetweenFrames_ms = 65.0;
      
      // We believe, due to camera exposure changes, the image timestamps need to be adjusted by this amount
      // Without the offset, imu data recorded during a frame did not match the movement seen in the frame
      static const int imageTimestampOffset_ms = 55;
      
    private:
      // The number of rows to divide the image into and calculate warps for
      static const int _rsNumDivisions = 10;
      
      // Vector of warping matricies to correct for rolling shutter
      // The first matrix corresponds to the warp for the top row of the image
      std::vector<Matrix_3x3f> _rollingShutterWarps;
      
      Vision::Camera& _camera;
      
      
    };
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROLLING_SHUTTER_CORRECTOR
