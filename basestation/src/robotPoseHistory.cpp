//
//  robotPoseHistory.cpp
//  Products_Cozmo
//
//  Created by Kevin Yoon 2014-05-13
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/common/basestation/general.h"


namespace Anki {
  namespace Cozmo {

    //////////////////////// RobotPoseStamp /////////////////////////
    
    RobotPoseStamp::RobotPoseStamp(const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                   const f32 pose_angle,
                                   const f32 head_angle)
    {
      SetPose(pose_x, pose_y, pose_z, pose_angle, head_angle);
    }


    void RobotPoseStamp::SetPose(const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                 const f32 pose_angle,
                                 const f32 head_angle)
    {
      pose_.set_rotation(pose_angle, Vec3f(0,0,1));
      pose_.set_translation(Vec3f(pose_x, pose_y, pose_z));
      headAngle_ = head_angle;
    }
    
    
    /////////////////////// RobotPoseHistory /////////////////////////////
    
    RobotPoseHistory::RobotPoseHistory()
    : windowSize_(1000)
    {}

    void RobotPoseHistory::Clear()
    {
      poses_.clear();
      gtPoses_.clear();
    }
    
    void RobotPoseHistory::SetTimeWindow(const u32 windowSize_ms)
    {
      windowSize_ = windowSize_ms;
      CullToWindowSize();
    }
    
    
    Result RobotPoseHistory::AddPose(const TimeStamp_t t,
                                     const RobotPoseStamp& p,
                                     bool groundTruth)
    {
      return AddPose(t,
                     p.GetPose().get_translation().x(),
                     p.GetPose().get_translation().y(),
                     p.GetPose().get_translation().z(),
                     p.GetPose().get_rotationAngle().ToFloat(),
                     p.GetHeadAngle(),
                     groundTruth);
    }


    // Adds a pose to the history
    Result RobotPoseHistory::AddPose(const TimeStamp_t t,
                                     const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                     const f32 pose_angle,
                                     const f32 head_angle,
                                     bool groundTruth)
    {
      // Should the pose be added?
      TimeStamp_t newestTime = poses_.rbegin()->first;
      if (newestTime > windowSize_ && t < newestTime - windowSize_) {
        return RESULT_FAIL;
      }
      
      std::pair<PoseMapIter_t, bool> res;
      res = poses_.emplace(std::piecewise_construct,
                           std::make_tuple(t),
                           std::make_tuple(pose_x, pose_y, pose_z, pose_angle, head_angle));
      
      if (!res.second) {
        // An existing pose can only be overwritten if the groundTruth flag is set.
        if (groundTruth) {
          res.first->second.SetPose(pose_x, pose_y, pose_z, pose_angle, head_angle);
          gtPoses_[t] = res.first;
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }

      if (groundTruth) {
        gtPoses_[t] = res.first;
      }
      
      CullToWindowSize();
      
      return RESULT_OK;
    }
    
    // Sets p to the pose nearest the given timestamp t.
    // Interpolates pose if withInterpolation == true.
    // Returns OK if t is between the oldest and most recent timestamps stored.
    Result RobotPoseHistory::GetPoseAt(const TimeStamp_t t_request,
                                       TimeStamp_t& t, RobotPoseStamp& p,
                                       bool withInterpolation) const
    {
      // This pose occurs at or immediately after t_request
      const_PoseMapIter_t it = poses_.lower_bound(t_request);
      
      // Check if in range
      if (it == poses_.end() || t_request < poses_.begin()->first) {
        return RESULT_FAIL;
      }
      
      if (t_request == it->first) {
        // If the exact timestamp was found, return the corresponding pose.
        t = it->first;
        p = it->second;
      } else {

        // Get iterator to the pose just before t_request
        const_PoseMapIter_t prev_it = it;
        prev_it--;
        
        if (withInterpolation) {
          
          // Get the pose transform between the two poses.
          Pose3d pTransform = it->second.GetPose().getWithRespectTo(&(prev_it->second.GetPose()));
          
          // Compute scale factor between time to previous pose and time between previous pose and next pose.
          f32 timeScale = (f32)(t_request - prev_it->first) / (it->first - prev_it->first);
          
          // Compute scaled transform
          Vec3f interpTrans(prev_it->second.GetPose().get_translation());
          interpTrans += pTransform.get_translation() * timeScale;
          
          // NOTE: Assuming there is only z-axis rotation!
          // TODO: Make generic?
          Radians interpRotation = prev_it->second.GetPose().get_rotationAngle() + Radians(pTransform.get_rotationAngle() * timeScale);
          
          // Interp head angle
          f32 interpHeadAngle = prev_it->second.GetHeadAngle() + timeScale * (it->second.GetHeadAngle() - prev_it->second.GetHeadAngle());
        
          t = t_request;
          p.SetPose(interpTrans.x(), interpTrans.y(), interpTrans.z(), interpRotation.ToFloat(), interpHeadAngle);
          
        } else {
          
          // Return the pose closest to the requested time
          if (it->first - t_request < t_request - prev_it->first) {
            t = it->first;
            p = it->second;
          } else {
            t = prev_it->first;
            p = prev_it->second;
          }
        }
      }
      
      return RESULT_OK;
    }

    
    void RobotPoseHistory::CullToWindowSize()
    {
      if (poses_.size() > 1) {
        
        // Get the most recent timestamp
        TimeStamp_t mostRecentTime = poses_.rbegin()->first;
        
        // If most recent time is less than window size, we're done.
        if (mostRecentTime < windowSize_) {
          return;
        }
        
        // Get pointer to the oldest timestamp that may remain in the map
        TimeStamp_t oldestAllowedTime = mostRecentTime - windowSize_;
        const_PoseMapIter_t it = poses_.upper_bound(oldestAllowedTime);
        const_GTPoseMapIter_t git = gtPoses_.upper_bound(oldestAllowedTime);
        
        // Delete everything before the oldest allowed timestamp
        if (oldestAllowedTime > poses_.begin()->first) {
          poses_.erase(poses_.begin(), it);
        }
        if (oldestAllowedTime > gtPoses_.begin()->first) {
          gtPoses_.erase(gtPoses_.begin(), git);
        }
      }
    }
    
    TimeStamp_t RobotPoseHistory::GetOldestTimeStamp() const
    {
      return (poses_.empty() ? 0 : poses_.begin()->first);
    }
    
    TimeStamp_t RobotPoseHistory::GetNewestTimeStamp() const
    {
      return (poses_.empty() ? 0 : poses_.rbegin()->first);
    }
    
  } // namespace Cozmo
} // namespace Anki
