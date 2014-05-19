//
//  robotPoseHistory.cpp
//  Products_Cozmo
//
//  Created by Kevin Yoon 2014-05-13
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/general.h"


namespace Anki {
  namespace Cozmo {

    //////////////////////// RobotPoseStamp /////////////////////////
    
    RobotPoseStamp::RobotPoseStamp()
    { }
    
    RobotPoseStamp::RobotPoseStamp(const PoseFrameId frameID,
                                   const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                   const f32 pose_angle,
                                   const f32 head_angle)
    {
      SetPose(frameID, pose_x, pose_y, pose_z, pose_angle, head_angle);
    }


    void RobotPoseStamp::SetPose(const PoseFrameId frameID,
                                 const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                 const f32 pose_angle,
                                 const f32 head_angle)
    {
      frame_ = frameID;
      
      pose_.set_rotation(pose_angle, Vec3f(0,0,1));
      pose_.set_translation(Vec3f(pose_x, pose_y, pose_z));
      headAngle_ = head_angle;
    }
    
    void RobotPoseStamp::SetPose(const PoseFrameId frameID,
                                 const Pose3d& pose,
                                 const f32 head_angle)
    {
      frame_ = frameID;
      pose_ = pose;
      headAngle_ = head_angle;
    }
    
    /////////////////////// RobotPoseHistory /////////////////////////////
    
    RobotPoseHistory::RobotPoseHistory()
    : windowSize_(1000)
    {}

    void RobotPoseHistory::Clear()
    {
      poses_.clear();
      visPoses_.clear();
    }
    
    void RobotPoseHistory::SetTimeWindow(const u32 windowSize_ms)
    {
      windowSize_ = windowSize_ms;
      CullToWindowSize();
    }
    
    
    Result RobotPoseHistory::AddRawOdomPose(const TimeStamp_t t,
                                     const RobotPoseStamp& p)
    {
      return AddRawOdomPose(t,
                            p.GetFrameId(),
                            p.GetPose().get_translation().x(),
                            p.GetPose().get_translation().y(),
                            p.GetPose().get_translation().z(),
                            p.GetPose().get_rotationAngle().ToFloat(),
                            p.GetHeadAngle());
    }


    // Adds a pose to the history
    Result RobotPoseHistory::AddRawOdomPose(const TimeStamp_t t,
                                            const PoseFrameId frameID,
                                            const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                            const f32 pose_angle,
                                            const f32 head_angle)
    {
      // Should the pose be added?
      TimeStamp_t newestTime = poses_.rbegin()->first;
      if (newestTime > windowSize_ && t < newestTime - windowSize_) {
        return RESULT_FAIL;
      }
      
      std::pair<PoseMapIter_t, bool> res;
      res = poses_.emplace(std::piecewise_construct,
                           std::make_tuple(t),
                           std::make_tuple(frameID, pose_x, pose_y, pose_z, pose_angle, head_angle));
      
      if (!res.second) {
        return RESULT_FAIL;
      }

      CullToWindowSize();
      
      return RESULT_OK;
    }

    Result RobotPoseHistory::AddVisionOnlyPose(const TimeStamp_t t,
                                                const RobotPoseStamp& p)
    {
      return AddVisionOnlyPose(t,
                                p.GetFrameId(),
                                p.GetPose().get_translation().x(),
                                p.GetPose().get_translation().y(),
                                p.GetPose().get_translation().z(),
                                p.GetPose().get_rotationAngle().ToFloat(),
                                p.GetHeadAngle());
    }
    
    Result RobotPoseHistory::AddVisionOnlyPose(const TimeStamp_t t,
                                                const PoseFrameId frameID,
                                                const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                                const f32 pose_angle,
                                                const f32 head_angle)
    {
      // Should the pose be added?
      TimeStamp_t newestTime = visPoses_.rbegin()->first;
      if (newestTime > windowSize_ && t < newestTime - windowSize_) {
        return RESULT_FAIL;
      }
      
      std::pair<PoseMapIter_t, bool> res;
      res = visPoses_.emplace(std::piecewise_construct,
                             std::make_tuple(t),
                             std::make_tuple(frameID, pose_x, pose_y, pose_z, pose_angle, head_angle));
      
      if (!res.second) {
        return RESULT_FAIL;
      }
      
      CullToWindowSize();
      
      return RESULT_OK;
    }

    
    
    
    // Sets p to the pose nearest the given timestamp t.
    // Interpolates pose if withInterpolation == true.
    // Returns OK if t is between the oldest and most recent timestamps stored.
    Result RobotPoseHistory::GetRawPoseAt(const TimeStamp_t t_request,
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
          p.SetPose(prev_it->second.GetFrameId(), interpTrans.x(), interpTrans.y(), interpTrans.z(), interpRotation.ToFloat(), interpHeadAngle);
          
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

    
    Result RobotPoseHistory::ComputePoseAt(const TimeStamp_t t_request,
                                           TimeStamp_t& t, RobotPoseStamp& p,
                                           bool withInterpolation) const
    {
      // If the vision-based version of the pose exists, return it.
      const_PoseMapIter_t it = visPoses_.find(t_request);
      if (it != visPoses_.end()) {
        t = t_request;
        p = it->second;
        return RESULT_OK;
      }
      
      // Get the pose at the requested timestamp
      RobotPoseStamp p1;
      if (GetRawPoseAt(t_request, t, p1, withInterpolation) == RESULT_FAIL) {
        return RESULT_FAIL;
      }
      
      // Now get the previous vision-based pose
      const_PoseMapIter_t git = visPoses_.lower_bound(t);
      
      // If there are no vision-based poses then return the raw pose that we just got
      if (git == visPoses_.end()) {
        if (visPoses_.empty()) {
          p = p1;
          return RESULT_OK;
        } else {
          --git;
        }
      } else if (git->first != t) {
        // As long as the vision-based pose is not from time t,
        // decrement the pointer to get the previous vision-based
        --git;
      }
      
      // Check frame ID
      // If the vision pose frame id does not match the requested frame id
      // then just return the raw pose of the requested frame id since it
      // is already based on the previous vision-based pose.
      if (git->second.GetFrameId() < p1.GetFrameId()) {
        p = p1;
        return RESULT_OK;
      }
      
      //printf("gt: %d\n", git->first);
      //git->second.GetPose().Print();
      
      // git now points to the latest vision-based pose that exists before time t.
      // Now get the pose in poses_ that immediately follows the vision-based pose's time.
      const_PoseMapIter_t p0_it = poses_.lower_bound(git->first);

      //printf("p0_it: %d\n", p0_it->first);
      //p0_it->second.GetPose().Print();
      
      //printf("p1: %d\n", t);
      //p1.GetPose().Print();
      
      // Compute relative pose between p0_it and p1 and append to the vision-based pose.
      Pose3d pTransform = p1.GetPose().getWithRespectTo(&(p0_it->second.GetPose()));

      //printf("pTrans: %d\n", t);
      //pTransform.Print();
      
      pTransform.preComposeWith(git->second.GetPose());
      p.SetPose(p1.GetFrameId(), pTransform, p1.GetHeadAngle());
      
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
        const_PoseMapIter_t git = visPoses_.upper_bound(oldestAllowedTime);
        
        // Delete everything before the oldest allowed timestamp
        if (oldestAllowedTime > poses_.begin()->first) {
          poses_.erase(poses_.begin(), it);
        }
        if (oldestAllowedTime > visPoses_.begin()->first) {
          visPoses_.erase(visPoses_.begin(), git);
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
