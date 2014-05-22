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
    
    /*
     * RobotPoseStamp
     *
     * Snapshot of robot pose along with head angle and the pose frame ID
     */
    class RobotPoseStamp
    {
    public:
      RobotPoseStamp();
      RobotPoseStamp(const PoseFrameID_t frameID,
                     const f32 pose_x, const f32 pose_y, const f32 pose_z,
                     const f32 pose_angle,
                     const f32 head_angle);
      
      RobotPoseStamp(const PoseFrameID_t frameID,
                     const Pose3d& pose,
                     const f32 head_angle);
      
      void SetPose(const PoseFrameID_t frameID,
                   const f32 pose_x, const f32 pose_y, const f32 pose_z,
                   const f32 pose_angle,
                   const f32 head_angle);

      void SetPose(const PoseFrameID_t frameID,
                   const Pose3d& pose,
                   const f32 head_angle);

      
      const Pose3d& GetPose() const {return pose_;}
      const f32 GetHeadAngle() const {return headAngle_;}
      const PoseFrameID_t GetFrameId() const {return frame_;}
      
    private:
      PoseFrameID_t frame_;
      Pose3d pose_;  // robot pose
      f32 headAngle_;
    };
    
    
    /*
     * RobotPoseHistory
     *
     * A collection of timestamped RobotPoseStamps for a specified time range.
     * Can be used to compute better pose estimated based on a combination of 
     * raw odometry based poses from the robot and vision-based poses computed by Blockworld.
     *
     */
    class RobotPoseHistory
    {
    public:
      
      RobotPoseHistory();
    
      // Clear all history
      void Clear();
      
      // Returns the number of poses that were added via AddRawOdomPose() that still remain in history
      u32 GetNumRawPoses() const {return poses_.size();}
      
      // Returns the number of poses that were added via AddVisionOnlyPose() that still remain in history
      u32 GetNumVisionPoses() const {return visPoses_.size();}
      
      // Specify the maximum time span of poses that can be held.
      // Poses that are older than the newest/largest timestamp stored
      // minus windowSize_ms are automatically removed.
      void SetTimeWindow(const u32 windowSize_ms);
      
      // Adds a timestamped pose received from the robot to the history.
      // Returns RESULT_FAIL if an entry for that timestamp already exists
      // or the pose is too old to be added.
      Result AddRawOdomPose(const TimeStamp_t t,
                            const PoseFrameID_t frameID,
                            const f32 pose_x, const f32 pose_y, const f32 pose_z,
                            const f32 pose_angle,
                            const f32 head_angle);

      Result AddRawOdomPose(const TimeStamp_t t,
                            const RobotPoseStamp& p);

      // Adds a timestamped pose based off of a vision marker to the history.
      // These are used in conjunction with raw odometry poses to compute
      // better estimates of the pose at any point t in the history.
      Result AddVisionOnlyPose(const TimeStamp_t t,
                                const PoseFrameID_t frameID,
                                const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                const f32 pose_angle,
                                const f32 head_angle);

      Result AddVisionOnlyPose(const TimeStamp_t t,
                               const RobotPoseStamp& p);
      
      // Sets p to the raw odometry pose nearest the given timestamp t in the history.
      // Interpolates pose if withInterpolation == true.
      // Returns OK if t is between the oldest and most recent timestamps stored.
      Result GetRawPoseAt(const TimeStamp_t t_request,
                          TimeStamp_t& t, RobotPoseStamp& p,
                          bool withInterpolation = false) const;
      
      // Returns OK and points p to a vision-based pose at the specified time if such a pose exists.
      // Note: The pose that p points to may be invalidated by subsequent calls to
      //       the history like Clear() or Add...(). Use carefully!
      Result GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p);
      
      // Same as above except that it uses the last vision-based
      // pose that exists at or before t_request to compute a
      // better estimate of the pose at time t.
      Result ComputePoseAt(const TimeStamp_t t_request,
                       TimeStamp_t& t, RobotPoseStamp& p,
                       bool withInterpolation = false) const;

      // Same as above except that it also inserts the resulting pose
      // as a vision-based pose back into history.
      Result ComputeAndInsertPoseAt(const TimeStamp_t t_request,
                                    TimeStamp_t& t, RobotPoseStamp** p,
                                    bool withInterpolation = false);

      
      TimeStamp_t GetOldestTimeStamp() const;
      TimeStamp_t GetNewestTimeStamp() const;
      
      
    private:
      
      void CullToWindowSize();
      
      // Pose history as reported by robot
      typedef std::map<TimeStamp_t, RobotPoseStamp> PoseMap_t;
      typedef std::map<TimeStamp_t, RobotPoseStamp>::iterator PoseMapIter_t;
      typedef std::map<TimeStamp_t, RobotPoseStamp>::const_iterator const_PoseMapIter_t;
      PoseMap_t poses_;

      // Map of timestamps of vision-based poses as computed from mat markers
      PoseMap_t visPoses_;
      
      // Size of history time window (ms)
      u32 windowSize_;
      
    }; // class RobotPoseHistory

    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot_pose_history__
