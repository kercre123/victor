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

#include "anki/cozmo/basestation/rollingShutterCorrector.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
  namespace Cozmo {
  
    void ImuDataHistory::AddImuData(u32 imageId, float rateX, float rateY, float rateZ, u8 line2Number)
    {
      ImuData data;
      data.imageId = imageId;
      data.line2Number = line2Number;
      data.rateX = rateX;
      data.rateY = rateY;
      data.rateZ = rateZ;
      
      if(_history.size() == maxSizeOfHistory)
      {
        _history.pop_front();
      }
      _history.push_back(data);
    }
    
    // Calculate timestamps for ImageIMU messages since they only have frameIDs and lineNumbers
    void ImuDataHistory::CalculateTimestampForImageIMU(u32 imageId, TimeStamp_t t, f32 period, int height)
    {
      const f32 timePerLine = period/height;
      for(auto iter = _history.begin(); iter != _history.end(); ++iter)
      {
        if(iter->imageId == imageId)
        {
          // The timestamp when this data was captured is
          // timestamp of the image - the time it took from the line it was captured to the end of the image
          iter->timestamp = t - (period/height * (height - iter->line2Number));
        }
        // Otherwise if the imageIMU is from the previous image and it still has a timestamp of 0
        // better calculate it now. This means we got the imageIMU data after the corresponding image was processed
        else if(iter->imageId == imageId - 1 &&
                iter->timestamp == 0)
        {
          if(iter != _history.begin())
          {
            const auto prevIter = iter - 1;
            // Both the last imageIMU data for the previous image and the one before both cant have timestamps of zero
            // the only time that they both might have zero timestamps is on start up before we have processed the first image
            if(prevIter->timestamp != 0)
            {
              iter->timestamp = prevIter->timestamp + ((iter->line2Number - prevIter->line2Number) * timePerLine);
            }
          }
        }
      }
    }
    
    bool ImuDataHistory::GetImuDataBeforeAndAfter(TimeStamp_t t,
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
        if(iter->timestamp > t ||
           (iter->timestamp == 0 &&
            ABS((int)((iter - 1)->timestamp - t)) < RollingShutterCorrector::timeBetweenFrames_ms))
        {
          after = *iter;
          before = *(iter - 1);
          return true;
        }
      }
      return false;
    }
    
    bool ImuDataHistory::IsImuDataBeforeTimeGreaterThan(const TimeStamp_t t,
                                                        const int numToLookBack,
                                                        const f32 rateX, const f32 rateY, const f32 rateZ) const
    {
      if(_history.size() == 0)
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
        if(iter->timestamp > t ||
           (iter->timestamp == 0 &&
            ABS((int)((iter - 1)->timestamp - t)) < RollingShutterCorrector::timeBetweenFrames_ms))
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

      Pose3d pose = poseData.cameraPose.GetWithRespectToOrigin();
      
      // Time difference between subdivided rows in the image
      const f32 timeDif = timeBetweenFrames_ms/_rsNumDivisions;
      
      // Whether or not a call to computePixelShiftsWithImageIMU returned false meaning it
      // was unable to compute the pixelShifts from imageIMU data
      bool didComputePixelShiftsFail = false;
      
      // Compounded pixelShifts across the image
      Vec2f shiftOffset = 0;
      
      // The fraction each subdivided row in the image will contribute to the total shifts for this image
      const f32 frac = 1.f / _rsNumDivisions;
      const f32 numRowsPerDivision = numRows/_rsNumDivisions;
      
      for(int i=1;i<=_rsNumDivisions;i++)
      {
        Vec2f pixelShifts;
        const int curLine = (_rsNumDivisions - i) * numRowsPerDivision;
        didComputePixelShiftsFail |= !ComputePixelShiftsWithImageIMU((TimeStamp_t)(poseData.timeStamp - std::round(i*timeDif)),
                                                                     pixelShifts,
                                                                     poseData,
                                                                     prevPoseData,
                                                                     frac,
                                                                     curLine,
                                                                     numRows);
        
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
                              poseData.timeStamp,
                              poseData.imuDataHistory.front().timestamp,
                              poseData.imuDataHistory.back().timestamp);
        }
        else
        {
          PRINT_NAMED_WARNING("RollingShutterCorrector.ComputePixelShifts.EmptyHistory",
                              "No ImageIMU data from timestamp %i, imuDataHistory is empty",
                              poseData.timeStamp);
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
    
    bool RollingShutterCorrector::ComputePixelShiftsWithImageIMU(TimeStamp_t t,
                                                                 Vec2f& shift,
                                                                 const VisionPoseData& poseData,
                                                                 const VisionPoseData& prevPoseData,
                                                                 const f32 frac,
                                                                 const int line,
                                                                 const u32 numRows)
    {
      std::deque<ImuDataHistory::ImuData>::const_iterator ImuBeforeT;
      std::deque<ImuDataHistory::ImuData>::const_iterator ImuAfterT;
      
      float rateY = 0;
      float rateZ = 0;
      
      if(poseData.imuDataHistory.size() == 0)
      {
        shift = Vec2f(0,0);
        return false;
      }
      
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
          
          int beforeLine = ImuBeforeT->line2Number;
          int afterLine = ImuAfterT->line2Number;
          
          // If the timestamp of the after imu data is zero then it is from the next image that has yet to be processed
          // which means we need to increase the line number to work with the current image
          // The current line we are calculating is towards the end of the image so the after imu data falls in the next image
          if(ImuAfterT->timestamp == 0)
          {
            afterLine += numRows;
          }
          // Otherwise if the after imu data has a valid timestamp but a different frame id than the current line must cause
          // the before imu data to fall in the previous image so we need to decrease the before line number
          else if(ImuBeforeT->imageId < ImuAfterT->imageId)
          {
            beforeLine -= numRows;
          }
          
          // Linearly interpolate the imu data using the line numbers before and after imu data was captured
          rateY = (((line - beforeLine)*(ImuAfterT->rateY - ImuBeforeT->rateY)) / (afterLine - beforeLine)) + ImuBeforeT->rateY;
          rateZ = (((line - beforeLine)*(ImuAfterT->rateZ - ImuBeforeT->rateZ)) / (afterLine - beforeLine)) + ImuBeforeT->rateZ;
          
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
