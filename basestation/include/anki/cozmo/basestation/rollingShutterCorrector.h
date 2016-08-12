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
          u32 imageId = 0;
          float rateX = 0;
          float rateY = 0;
          float rateZ = 0;
          u8 line2Number = 0;
          TimeStamp_t timestamp = 0;
        };
      
        // Gets the imu data before and after the timestamp
        bool GetImuDataBeforeAndAfter(TimeStamp_t t, ImuData& before, ImuData& after);
      
        // Returns true if the any of the numToLookBack imu data before timestamp t have rates that are greater than
        // the given rates
        bool IsImuDataBeforeTimeGreaterThan(const TimeStamp_t t,
                                            const int numToLookBack,
                                            const f32 rateX, const f32 rateY, const f32 rateZ);
      
        std::deque<ImuData>::const_iterator begin() const { return _history.begin(); }
        std::deque<ImuData>::const_iterator end() const { return _history.end(); }
        ImuData front() const { return _history.front(); }
        ImuData back() const { return _history.back(); }
        size_t size() const { return _history.size(); }
        bool empty() const { return _history.empty(); }
      
      private:
        std::deque<ImuData> _history;
        static const int maxSizeOfHistory = 20;
    };
    
    class RollingShutterCorrector
    {
    public:
      RollingShutterCorrector() { };
    
      // Calculates the amount of pixel shift to account for rolling shutter
      void ComputePixelShifts(const VisionPoseData& poseData,
                              const VisionPoseData& prevPoseData);
      
      // Shifts the image by the calculated pixel shifts
      Vision::Image WarpImage(const Vision::Image& img);
      
      std::vector<Vec2f> GetPixelShifts() const { return _pixelShifts; }
      int GetNumDivisions() const { return _rsNumDivisions; }
      
      static constexpr f32 timeBetweenFrames_ms = 65.0;
      
    private:
      // Calculates pixel shifts based on gyro rates from ImageIMUData messages
      // Returns false if unable to calculate shifts due to not having relevant gyro data
      void ComputePixelShiftsWithImageIMU(TimeStamp_t t,
                                          Vec2f& shift,
                                          const VisionPoseData& poseData,
                                          const VisionPoseData& prevPoseData,
                                          f32 frac);
    
      // Vector of vectors of varying pixel shift amounts based on gyro rates and vertical position in the image
      std::vector<Vec2f> _pixelShifts;
      
      // Whether or not to do vertical rolling shutter correction
      // TODO: Do we want to be doing vertical correction?
      bool doVerticalCorrection = false;
      
      // The number of rows to divide the image into and calculate warps for
      static const int _rsNumDivisions = 120;
      
      // Proportionality constant that relates gyro rates to pixel shift
      static constexpr f32 rateToPixelProportionalityConst = 22.0;
      
    };
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROLLING_SHUTTER_CORRECTOR
