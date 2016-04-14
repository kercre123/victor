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
      for(auto iter = _history.begin(); iter != _history.end(); iter++)
      {
        if(iter->imageId == imageId)
        {
          // It takes 65ms to capture an image which has 240 lines
          iter->timestamp = t - (period/height * (height - iter->line2Number));
        }
      }
    }
  
  
    RollingShutterCorrector::RollingShutterCorrector(Vision::Camera& camera)
    : _camera(camera)
    {
    
    }
    
    void RollingShutterCorrector::ComputeWarps(const VisionPoseData& poseData,
                                               const VisionPoseData& prevPoseData)
    {
      _rollingShutterWarps.clear();

      Pose3d pose = poseData.cameraPose.GetWithRespectToOrigin();
      
      RotationMatrix3d rPrime = poseData.cameraPose.GetWithRespectToOrigin().GetRotationMatrix();
      Radians x;
      Radians y;
      Radians z;
      // Only care about Z rotation
      rPrime.GetEulerAngles(x, y, z);
      rPrime = RotationMatrix3d(0, 0, z);
      
      // Time difference between subdivided rows in the image
      // We get a new frame every 65ms
      int timeDif = floor(timeBetweenFrames_ms/_rsNumDivisions);
      
      for(int i=1;i<=_rsNumDivisions;i++)
      {
        // For each subdivided row get the camera rotation at the timestamp that row in the image would have been captured
        RotationMatrix3d r;
        bool haveRotation = true;
        // If we can't get the rotation using integrated ImageIMUData then use historical camera rotation
        if(!ComputeCameraRotationWithImageIMU((TimeStamp_t)(poseData.timeStamp - (i*timeDif)),
                                              r,
                                              poseData,
                                              prevPoseData))
        {
          r = poseData.historicCameraRots[i-1];
          // If the historical camera rotation is the identity (ie we couldn't get a historical camera rotation
          // then use the identity matrix as our warp
          if(r == RotationMatrix3d())
          {
            haveRotation = false;
          }
        }
        
        Matrix_3x3f W;
        if(haveRotation)
        {
          // Only care about Z rotation
          r.GetEulerAngles(x, y, z);
          r = RotationMatrix3d(0, 0, z);
          
          Matrix_3x3f K = _camera.GetCalibration()->GetCalibrationMatrix();
          Matrix_3x3f Kinv = _camera.GetCalibration()->GetInvCalibrationMatrix();
          
          // Matrix to warp the image row by
          W = K * rPrime * r.Transpose() * Kinv;
          
          // Remove everything but shear in x direction
          W(1,0) = 0;
          W(0,0) = 1;
          W(1,1) = 1;
          W(1,2) = 0;
        }
        else
        {
          W = Matrix_3x3f({1,0,0, 0,1,0, 0,0,1});
        }
        
        // Put at front because warps W are calculated from the bottom of the image up and the order of
        // the vector is top of image to bottom
        _rollingShutterWarps.insert(_rollingShutterWarps.begin(), W);
      }
    }
    
    Vision::Image RollingShutterCorrector::WarpImage(const Vision::Image& imgOrig)
    {
      Vision::Image img;
      imgOrig.CopyTo(img);
      int numRows = img.GetNumRows() - 1;
      
      cv::Mat map_x;
      cv::Mat map_y;
      map_x.create(img.get_CvMat_().size(), CV_32FC1);
      map_y.create(img.get_CvMat_().size(), CV_32FC1);
      

      for(int i=1;i<=_rsNumDivisions;i++)
      {
        Matrix_3x3f W = _rollingShutterWarps[_rollingShutterWarps.size() - i];
        
        // Calculate and apply the inverse warp mapping of each pixel in the destination image to the original image
        W.Invert();
        cv::Mat temp(3, img.GetNumCols(), CV_32F);
        cv::Mat warped(3, img.GetNumCols(), CV_32F);
        for(int y = numRows - (i*1.0/_rsNumDivisions * numRows); y < (int)(numRows - ((i-1)*1.0/_rsNumDivisions * numRows)); y++)
        {
          float* temp_0 = temp.ptr<float>(0);
          for(int x=0;x<img.GetNumCols();x++)
          {
            temp_0[x] = x;
          }
          temp.row(1).setTo(y);
          temp.row(2).setTo(1);
          
          warped = cv::Mat(W.get_CvMatx_()) * temp;
          warped.row(0).copyTo(map_x.row(y));
          warped.row(1).copyTo(map_y.row(y));
        }
      }

      cv::remap(img.get_CvMat_(), img.get_CvMat_(), map_x, map_y, cv::INTER_NEAREST);
      return img;
    }
    
    
    bool RollingShutterCorrector::ComputeCameraRotationWithImageIMU(TimeStamp_t t,
                                                                    RotationMatrix3d& r,
                                                                    const VisionPoseData& poseData,
                                                                    const VisionPoseData& prevPoseData)
    {
      std::deque<ImuDataHistory::ImuData>::const_iterator ImuBeforeT;
      std::deque<ImuDataHistory::ImuData>::const_iterator ImuAfterT;
      
      bool beforeAfterSet = false;
      // Find the ImuData before and after the timestamp if it exists
      for(auto iter = poseData.imuDataHistory.begin(); iter != poseData.imuDataHistory.end(); ++iter)
      {
        if(iter->timestamp >= t)
        {
          if(iter == poseData.imuDataHistory.begin())
          {
            PRINT_NAMED_WARNING("VisionSystem.GetCameraRotationWithImageIMU",
                                "No ImageIMU data before timestamp %i have data from time %i:%i", t,
                                poseData.imuDataHistory.front().timestamp,
                                poseData.imuDataHistory.back().timestamp);
            return false;
          }
          else
          {
            ImuBeforeT = iter - 1;
            ImuAfterT = iter;
            beforeAfterSet = true;
          }
          
          // Move ImuBefore and ImuAfter to before and after our previous pose data
          while(ImuBeforeT->timestamp > prevPoseData.timeStamp)
          {
            if(ImuBeforeT == poseData.imuDataHistory.begin())
            {
              PRINT_NAMED_WARNING("VisionSystem.GetCameraRotationWithImageIMU",
                                  "No ImageIMU data before timestamp %i have data from time %i:%i", t,
                                  poseData.imuDataHistory.front().timestamp,
                                  poseData.imuDataHistory.back().timestamp);
              return false;
            }
            ImuBeforeT--;
            ImuAfterT--;
          }
          break;
        }
      }
      
      if(!beforeAfterSet)
      {
        return false;
      }
      
      Radians x = prevPoseData.poseStamp.GetPose().GetRotation().GetAngleAroundXaxis();
      Radians y = prevPoseData.poseStamp.GetPose().GetRotation().GetAngleAroundYaxis();
      Radians z = prevPoseData.poseStamp.GetPose().GetRotation().GetAngleAroundZaxis();
      
      TimeStamp_t deltaT = ImuAfterT->timestamp - ImuBeforeT->timestamp;
      x += (ImuAfterT->rateX - ImuBeforeT->rateX) * deltaT/1000.0;
      y += (ImuAfterT->rateY - ImuBeforeT->rateY) * deltaT/1000.0;
      z += (ImuAfterT->rateZ - ImuBeforeT->rateZ) * deltaT/1000.0;

      r = RotationMatrix3d(x, y, z-(DEG_TO_RAD(90)));
      
      // Verify the rotation is valid
      for(int i=0;i<2;i++)
      {
        for(int j=0;j<2;j++)
        {
          if(r(i,j) != r(i,j))
          {
            PRINT_NAMED_WARNING("VisionSystem.GetCameraRotationWithImageIMU", "Element in ImageIMU based rotation is nan");
            return false;
          }
        }
      }
      return true;
    }
    
    std::vector<RotationMatrix3d> RollingShutterCorrector::PrecomputeHistoricalCameraRotations(Robot& robot, TimeStamp_t timestamp) const
    {
      std::vector<RotationMatrix3d> histCameraRots;
      // Precompute historical camera rotations for this image for use with rolling shutter correction
      // should we be unable to calculate rotation using ImageIMU
      int timeDif = timeBetweenFrames_ms/_rsNumDivisions;
      for(int i=1;i<=_rsNumDivisions;i++)
      {
        RobotPoseStamp p;
        TimeStamp_t t;
        if(robot.GetPoseHistory()->ComputePoseAt((TimeStamp_t)(timestamp - (i*timeDif)), t, p, true) == RESULT_OK)
        {
          RotationMatrix3d r = robot.GetHistoricalCameraPose(p, (TimeStamp_t)(timestamp - (i*timeDif))).GetWithRespectToOrigin().GetRotationMatrix();
          histCameraRots.push_back(r);
        }
        else
        {
          // If we can't get a historical camera pose for this timestamp push the identity matrix
          histCameraRots.push_back({1,0,0, 0,1,0, 0,0,1});
        }
      }
      return histCameraRots;
    }

  }
}