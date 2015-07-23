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
#include "anki/cozmo/shared/cozmoTypes.h"

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
                     const f32 head_angle,
                     const f32 lift_angle,
                     const Pose3d* pose_origin);
      
      RobotPoseStamp(const PoseFrameID_t frameID,
                     const Pose3d& pose,
                     const f32 head_angle,
                     const f32 lift_angle);
      
      void SetPose(const PoseFrameID_t frameID,
                   const f32 pose_x, const f32 pose_y, const f32 pose_z,
                   const f32 pose_angle,
                   const f32 head_angle,
                   const f32 lift_angle,
                   const Pose3d* pose_origin);

      void SetPose(const PoseFrameID_t frameID,
                   const Pose3d& pose,
                   const f32 head_angle,
                   const f32 lift_angle);

      
      const Pose3d& GetPose() const {return pose_;}
      const f32 GetHeadAngle() const {return headAngle_;}
      const f32 GetLiftAngle() const {return liftAngle_;}
      const PoseFrameID_t GetFrameId() const {return frame_;}

      void Print() const;
      
    private:
      PoseFrameID_t frame_;
      Pose3d pose_;  // robot pose
      f32 headAngle_;
      f32 liftAngle_;
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
      size_t GetNumRawPoses() const {return poses_.size();}
      
      // Returns the number of poses that were added via AddVisionOnlyPose() that still remain in history
      size_t GetNumVisionPoses() const {return visPoses_.size();}
      
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
                            const f32 head_angle,
                            const f32 lift_angle);

      Result AddRawOdomPose(const TimeStamp_t t,
                            const RobotPoseStamp& p);

      // Adds a timestamped pose based off of a vision marker to the history.
      // These are used in conjunction with raw odometry poses to compute
      // better estimates of the pose at any point t in the history.
      Result AddVisionOnlyPose(const TimeStamp_t t,
                               const PoseFrameID_t frameID,
                               const f32 pose_x, const f32 pose_y, const f32 pose_z,
                               const f32 pose_angle,
                               const f32 head_angle,
                               const f32 lift_angle);
      
      Result AddVisionOnlyPose(const TimeStamp_t t,
                               const RobotPoseStamp& p);
      
      // Sets p to the raw odometry pose nearest the given timestamp t in the history.
      // Interpolates pose if withInterpolation == true.
      // Returns OK if t is between the oldest and most recent timestamps stored.
      Result GetRawPoseAt(const TimeStamp_t t_request,
                          TimeStamp_t& t, RobotPoseStamp& p,
                          bool withInterpolation = false) const;
      
      // Get raw odometry poses (and their times) immediately before and after
      // the pose nearest to the requested time. Returns failure if either cannot
      // be found (e.g. when the requested time corresponds to the first or last
      // pose in  history).
      Result GetRawPoseBeforeAndAfter(const TimeStamp_t t,
                                      TimeStamp_t&    t_before,
                                      RobotPoseStamp& p_before,
                                      TimeStamp_t&    t_after,
                                      RobotPoseStamp& p_after);
      
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
                                    HistPoseKey* key = nullptr,
                                    bool withInterpolation = false);

      // Points *p to a computed pose in the history that was insert via ComputeAndInsertPoseAt
      Result GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key = nullptr);

      // If at least one vision only pose exists, the most recent one is returned in p
      // and the time it occured at in t.
      Result GetLatestVisionOnlyPose(TimeStamp_t& t, RobotPoseStamp& p) const;
      
      // Get the last pose in history with the given frame ID.
      Result GetLastPoseWithFrameID(const PoseFrameID_t frameID, RobotPoseStamp& p) const;
      
      // Checks whether or not the given key is associated with a valid computed pose
      bool IsValidPoseKey(const HistPoseKey key) const;
      
      
      TimeStamp_t GetOldestTimeStamp() const;
      TimeStamp_t GetNewestTimeStamp() const;
      
      // Prints the entire history
      void Print() const;
      
    private:
      
      void CullToWindowSize();
      
      // All poses should be flattened and will use the same "world" origin
      Pose3d poseOrigin_;
      
      // Pose history as reported by robot
      typedef std::map<TimeStamp_t, RobotPoseStamp> PoseMap_t;
      typedef PoseMap_t::iterator PoseMapIter_t;
      typedef PoseMap_t::const_iterator const_PoseMapIter_t;
      PoseMap_t poses_;

      // Map of timestamps of vision-based poses as computed from mat markers
      PoseMap_t visPoses_;

      // Map of poses that were computed with ComputeAt
      PoseMap_t computedPoses_;

      // The last pose key assigned to a computed pose
      static HistPoseKey currHistPoseKey_;
      
      // Map of HistPoseKeys to timestamps and vice versa
      typedef std::map<HistPoseKey, TimeStamp_t> TimestampByKeyMap_t;
      typedef TimestampByKeyMap_t::iterator TimestampByKeyMapIter_t;
      typedef std::map<TimeStamp_t, HistPoseKey> KeyByTimestampMap_t;
      typedef KeyByTimestampMap_t::iterator KeyByTimestampMapIter_t;
      TimestampByKeyMap_t tsByKeyMap_;
      KeyByTimestampMap_t keyByTsMap_;
      
      // Size of history time window (ms)
      u32 windowSize_;
      
    }; // class RobotPoseHistory

    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot_pose_history__
