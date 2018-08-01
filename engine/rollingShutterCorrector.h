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

#include "coretech/vision/engine/image.h"
#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/engine/robotTimeStamp.h"
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
      
        void AddImuData(RobotTimeStamp_t systemTimestamp_ms, float rateX, float rateY, float rateZ);
      
        struct ImuData
        {
          float rateX = 0;
          float rateY = 0;
          float rateZ = 0;
          RobotTimeStamp_t timestamp = 0;
        };
      
        // Gets the imu data before and after the timestamp
        bool GetImuDataBeforeAndAfter(RobotTimeStamp_t t, ImuData& before, ImuData& after) const;
      
        // Returns true if the any of the numToLookBack imu data before timestamp t have rates that are greater than
        // the given rates
        bool IsImuDataBeforeTimeGreaterThan(const RobotTimeStamp_t t,
                                            const int numToLookBack,
                                            const f32 rateX, const f32 rateY, const f32 rateZ) const;
      
        std::deque<ImuData>::const_iterator begin() const { return _history.begin(); }
        std::deque<ImuData>::const_iterator end() const { return _history.end(); }
        ImuData front() const { return _history.front(); }
        ImuData back() const { return _history.back(); }
        size_t size() const { return _history.size(); }
        bool empty() const { return _history.empty(); }
      
      private:
        std::deque<ImuData> _history;
        static const int maxSizeOfHistory = 80;
    };
    
    class RollingShutterCorrector
    {
    public:
      RollingShutterCorrector() { };
    
      // Calculates the amount of pixel shift to account for rolling shutter
      void ComputePixelShifts(const VisionPoseData& poseData,
                              const VisionPoseData& prevPoseData,
                              const u32 numRows);
      
      // Shifts the image by the calculated pixel shifts
      Vision::Image WarpImage(const Vision::Image& img);
      
      const std::vector<Vec2f>& GetPixelShifts() const { return _pixelShifts; }
      int GetNumDivisions() const { return _rsNumDivisions; }
      
      static constexpr f32 timeBetweenFrames_ms = 65.0;
      
    private:
      // Calculates pixel shifts based on gyro rates from ImageIMUData messages
      // Returns false if unable to calculate shifts due to not having relevant gyro data
      bool ComputePixelShiftsWithImageIMU(RobotTimeStamp_t t,
                                          Vec2f& shift,
                                          const VisionPoseData& poseData,
                                          const VisionPoseData& prevPoseData,
                                          const f32 frac);
    
      // Vector of vectors of varying pixel shift amounts based on gyro rates and vertical position in the image
      std::vector<Vec2f> _pixelShifts;
      
      // Whether or not to do vertical rolling shutter correction
      // TODO: Do we want to be doing vertical correction?
      bool doVerticalCorrection = false;
      
      // The number of rows to divide the image into and calculate warps for
      static constexpr f32 _rsNumDivisions = 180.f;
      
      // Proportionality constant that relates gyro rates to pixel shift
      static constexpr f32 rateToPixelProportionalityConst = 22.0;
      
    };
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROLLING_SHUTTER_CORRECTOR
