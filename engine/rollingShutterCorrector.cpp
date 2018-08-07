/**
 * File: rollingShutterCorrector.cpp
 *
 * Author: Al Chaussee
 * Created: 4-1-2016
 *
 * Description:
 *    Class for handling rolling shutter correction
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/rollingShutterCorrector.h"

#include "engine/robot.h"
#include "engine/vision/visionSystem.h"


namespace Anki {
  namespace Vector {
  
    void ImuDataHistory::AddImuData(RobotTimeStamp_t systemTimestamp_ms,
                                    float rateX,
                                    float rateY,
                                    float rateZ)
    {
      ImuData data;
      data.timestamp = systemTimestamp_ms;
      data.rateX = rateX;
      data.rateY = rateY;
      data.rateZ = rateZ;
      
      if(_history.size() == maxSizeOfHistory)
      {
        _history.pop_front();
      }
      _history.push_back(data);
    }
    
    bool ImuDataHistory::GetImuDataBeforeAndAfter(RobotTimeStamp_t t,
                                                  ImuDataHistory::ImuData& before,
                                                  ImuDataHistory::ImuData& after) const
    {
      if(_history.size() < 2)
      {
        return false;
      }
    
      // Start at beginning + 1 because there is no data before the first element
      for(auto iter = _history.begin() + 1; iter != _history.end(); ++iter)
      {
        // If we get to the imu data after the timestamp
        // or We have gotten to the imu data that has yet to be given a timestamp and
        // our last known timestamped imu data is fairly close the time we are looking for
        // so use it and the data after it that doesn't have a timestamp
        const int64_t tDiff = ((int64_t)(TimeStamp_t)(iter - 1)->timestamp - (int64_t)(TimeStamp_t)t);
        if(iter->timestamp > t ||
           (iter->timestamp == 0 &&
            ABS(tDiff) < RollingShutterCorrector::timeBetweenFrames_ms))
        {
          after = *iter;
          before = *(iter - 1);
          return true;
        }
      }
      return false;
    }
    
    bool ImuDataHistory::IsImuDataBeforeTimeGreaterThan(const RobotTimeStamp_t t,
                                                        const int numToLookBack,
                                                        const f32 rateX, const f32 rateY, const f32 rateZ) const
    {
      if(_history.empty())
      {
        return false;
      }
      
      auto iter = _history.begin();
      // Separate check for the first element due to there not being anything before it
      if(iter->timestamp > t)
      {
        if(ABS(iter->rateX) > rateX && ABS(iter->rateY) > rateY && ABS(iter->rateZ) > rateZ)
        {
          return true;
        }
      }
      // If the first element has a timestamp of zero then nothing else will have valid timestamps
      // due to how ImuData comes in during an image so the data that we get for an image that we are in the process of receiving will not
      // have timestamps
      else if(iter->timestamp == 0)
      {
        return false;
      }
      
      for(iter = _history.begin() + 1; iter != _history.end(); ++iter)
      {
        // If we get to the imu data after the timestamp
        // or We have gotten to the imu data that has yet to be given a timestamp and
        // our last known timestamped imu data is fairly close the time we are looking for
        const int64_t tDiff = ((int64_t)(TimeStamp_t)(iter - 1)->timestamp - (int64_t)(TimeStamp_t)t);
        if(iter->timestamp > t ||
           (iter->timestamp == 0 &&
            ABS(tDiff) < RollingShutterCorrector::timeBetweenFrames_ms))
        {          
          // Once we get to the imu data after the timestamp look at the numToLookBack imu data before it
          for(int i = 0; i < numToLookBack; i++)
          {
            if(ABS(iter->rateX) > rateX && ABS(iter->rateY) > rateY && ABS(iter->rateZ) > rateZ)
            {
              return true;
            }
            if(iter-- == _history.begin())
            {
              return false;
            }
          }
          return false;
        }
      }
      return false;
    }
    
    void RollingShutterCorrector::ComputePixelShifts(const VisionPoseData& poseData,
                                                     const VisionPoseData& prevPoseData,
                                                     const u32 numRows)
    {
      _pixelShifts.clear();
      _pixelShifts.reserve(_rsNumDivisions);

      // Time difference between subdivided rows in the image
      const f32 timeDif = timeBetweenFrames_ms/_rsNumDivisions;
      
      // Whether or not a call to computePixelShiftsWithImageIMU returned false meaning it
      // was unable to compute the pixelShifts from imageIMU data
      bool didComputePixelShiftsFail = false;
      
      // Compounded pixelShifts across the image
      Vec2f shiftOffset = 0;
      
      // The fraction each subdivided row in the image will contribute to the total shifts for this image
      const f32 frac = 1.f / _rsNumDivisions;
      
      for(int i=1;i<=_rsNumDivisions;i++)
      {
        Vec2f pixelShifts;
        const RobotTimeStamp_t time = poseData.timeStamp - Anki::Util::numeric_cast<TimeStamp_t>(std::round(i*timeDif));
        didComputePixelShiftsFail |= !ComputePixelShiftsWithImageIMU(time,
                                                                     pixelShifts,
                                                                     poseData,
                                                                     prevPoseData,
                                                                     frac);
        
        _pixelShifts.insert(_pixelShifts.end(),
                            Vec2f(pixelShifts.x() + shiftOffset.x(), pixelShifts.y() + shiftOffset.y()));
        
        shiftOffset.x() += pixelShifts.x();
        shiftOffset.y() += pixelShifts.y();
      }
      
      if(didComputePixelShiftsFail)
      {
        if(!poseData.imuDataHistory.empty())
        {
          PRINT_NAMED_WARNING("RollingShutterCorrector.ComputePixelShifts.NoImageIMUData",
                              "No ImageIMU data from timestamp %i have data from time %i:%i",
                              (TimeStamp_t)poseData.timeStamp,
                              (TimeStamp_t)poseData.imuDataHistory.front().timestamp,
                              (TimeStamp_t)poseData.imuDataHistory.back().timestamp);
        }
        else
        {
          PRINT_NAMED_WARNING("RollingShutterCorrector.ComputePixelShifts.EmptyHistory",
                              "No ImageIMU data from timestamp %i, imuDataHistory is empty",
                              (TimeStamp_t)poseData.timeStamp);
        }
      }
    }
    
    Vision::Image RollingShutterCorrector::WarpImage(const Vision::Image& imgOrig)
    {
      Vision::Image img(imgOrig.GetNumRows(), imgOrig.GetNumCols());
      img.SetTimestamp(imgOrig.GetTimestamp());
      
      const int numRows = img.GetNumRows() - 1;
      
      const f32 rowsPerDivision = ((f32)numRows)/_rsNumDivisions;
      
      for(int i=1;i<=_rsNumDivisions;i++)
      {
        const Vec2f& pixelShifts = _pixelShifts[i-1];
        
        const int firstRow = numRows - (i * rowsPerDivision);
        const int lastRow  = numRows - ((i-1) * rowsPerDivision);
        
        for(int y = firstRow; y < lastRow; y++)
        {
          const unsigned char* row = imgOrig.GetRow(y);
          unsigned char* shiftRow = img.GetRow(y);
          for(int x=0;x<img.GetNumCols();x++)
          {
            int xIdx = x - pixelShifts.x();
            if(xIdx < 0 || xIdx >= img.GetNumCols())
            {
              shiftRow[x] = 0;
            }
            else
            {
              shiftRow[x] = row[xIdx];
            }
          }
        }
      }
      return img;
    }
    
    bool RollingShutterCorrector::ComputePixelShiftsWithImageIMU(RobotTimeStamp_t t,
                                                                 Vec2f& shift,
                                                                 const VisionPoseData& poseData,
                                                                 const VisionPoseData& prevPoseData,
                                                                 const f32 frac)
    {
      if(poseData.imuDataHistory.empty())
      {
        shift = Vec2f(0,0);
        return false;
      }
      
      std::deque<ImuDataHistory::ImuData>::const_iterator ImuBeforeT;
      std::deque<ImuDataHistory::ImuData>::const_iterator ImuAfterT;

      float rateY = 0;
      float rateZ = 0;

      bool beforeAfterSet = false;
      // Find the ImuData before and after the timestamp if it exists
      for(auto iter = poseData.imuDataHistory.begin(); iter != poseData.imuDataHistory.end(); ++iter)
      {
        if(iter->timestamp >= t)
        {
          // No imageIMU data before time t
          if(iter == poseData.imuDataHistory.begin())
          {
            shift = Vec2f(0, 0);
            return false;
          }
          else
          {
            ImuBeforeT = iter - 1;
            ImuAfterT = iter;
            beforeAfterSet = true;
          }
          
          const TimeStamp_t tMinusBeforeTime     = (TimeStamp_t)(t - ImuBeforeT->timestamp);
          const TimeStamp_t afterMinusBeforeTime = (TimeStamp_t)(ImuAfterT->timestamp - ImuBeforeT->timestamp);
          
          // Linearly interpolate the imu data using the timestamps before and after imu data was captured
          rateY = (((tMinusBeforeTime)*(ImuAfterT->rateY - ImuBeforeT->rateY)) / (afterMinusBeforeTime)) + ImuBeforeT->rateY;
          rateZ = (((tMinusBeforeTime)*(ImuAfterT->rateZ - ImuBeforeT->rateZ)) / (afterMinusBeforeTime)) + ImuBeforeT->rateZ;
          
          break;
        }
      }
      
      // If we don't have imu data for the timestamps before and after the timestamp we are looking for just
      // use the latest data
      if(!beforeAfterSet && !poseData.imuDataHistory.empty())
      {
        rateY = poseData.imuDataHistory.back().rateY;
        rateZ = poseData.imuDataHistory.back().rateZ;
      }
      
      // If we aren't doing vertical correction then setting rateY to zero will ensure no Y shift
      if(!doVerticalCorrection)
      {
        rateY = 0;
      }
      
      // The rates are in world coordinate frame but we want them in camera frame which is why Z and X are switched
      shift = Vec2f(rateZ * rateToPixelProportionalityConst * frac,
                    rateY * rateToPixelProportionalityConst * frac);
      
      return true;
    }
  }
}
