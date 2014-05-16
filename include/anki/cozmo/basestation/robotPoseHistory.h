//
//  robotPoseHistory.h
//  Products_Cozmo
//
//  Created by Kevin Yoon on 2014-05-13.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__robot_pose_history__
#define __Products_Cozmo__robot_pose_history__

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/vision/basestation/camera.h"

namespace Anki {
  namespace Cozmo {
    
    // Snapshot of robot pose
    class RobotPoseStamp
    {
    public:
      RobotPoseStamp() {}
      RobotPoseStamp(const f32 pose_x, const f32 pose_y, const f32 pose_z,
                     const f32 pose_angle,
                     const f32 head_angle);
      
      void SetPose(const f32 pose_x, const f32 pose_y, const f32 pose_z,
                   const f32 pose_angle,
                   const f32 head_angle);
      
      const Pose3d& GetPose() const {return pose_;}
      const f32 GetHeadAngle() const {return headAngle_;}
      
      //const Vision::Camera& GetCam() const {return cam_;}
      //void SetCamCalib(const Vision::CameraCalibration& calib) {cam_.set_calibration(calib);}
      //const Pose3d& GetRobot2NeckPose() const {return robot2NeckPose_;}
      
    private:
      Pose3d pose_;  // robot pose
      //Pose3d robot2NeckPose_;
      f32 headAngle_;
      
      //Vision::Camera cam_;
    };
    
    
    class RobotPoseHistory
    {
    public:
      
      RobotPoseHistory();
      
      void Clear();
      u32 Size() const {return poses_.size();}
      
      // Specify the maximum time span of poses that can be held
      void SetTimeWindow(const u32 windowSize_ms);
      
      // Adds a timestamped pose to the history.
      // Returns RESULT_FAIL if an entry for that timestamp already exists.
      Result AddPose(const TimeStamp_t t,
                     const f32 pose_x, const f32 pose_y, const f32 pose_z,
                     const f32 pose_angle,
                     const f32 head_angle,
                     bool groundTruth = false);

      Result AddPose(const TimeStamp_t t,
                     const RobotPoseStamp& p,
                     bool groundTruth = false);
      
      
      // Sets p to the pose nearest the given timestamp t.
      // Interpolates pose if withInterpolation == true.
      // Returns OK if t is between the oldest and most recent timestamps stored.
      //
      // TODO: When an interpolated pose is requested, add it to the history as well so that
      // the reference persists?
      Result GetPoseAt(const TimeStamp_t t_request,
                       TimeStamp_t& t, RobotPoseStamp& p,
                       bool withInterpolation = false) const;
      
      TimeStamp_t GetOldestTimeStamp() const;
      TimeStamp_t GetNewestTimeStamp() const;
      
      
      
    private:
      
      void CullToWindowSize();
      
      // Pose history as reported by robot
      typedef std::map<TimeStamp_t, RobotPoseStamp> PoseMap_t;
      typedef std::map<TimeStamp_t, RobotPoseStamp>::iterator PoseMapIter_t;
      typedef std::map<TimeStamp_t, RobotPoseStamp>::const_iterator const_PoseMapIter_t;
      PoseMap_t poses_;

      // Map of timestamps of ground truth poses in poses_
      typedef std::map<TimeStamp_t, PoseMapIter_t> GTPoseMap_t;
      typedef std::map<TimeStamp_t, PoseMapIter_t>::iterator GTPoseMapIter_t;
      typedef std::map<TimeStamp_t, PoseMapIter_t>::const_iterator const_GTPoseMapIter_t;
      GTPoseMap_t gtPoses_;
      
      
      // Pose corrections as computed by mat marker updates
      PoseMap_t corrections_;
      
      u32 windowSize_;
      
    }; // class RobotPoseHistory

    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot_pose_history__
