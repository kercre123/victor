/**
 * File: pathComponent.h
 *
 * Author: Brad Neuman
 * Created: 2017-03-28
 *
 * Description: Component that handles path planning and following
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Components_PathComponent_H__
#define __Cozmo_Basestation_Components_PathComponent_H__

#include "anki/common/types.h"
#include "anki/planning/shared/goalDefs.h"
#include "anki/planning/shared/path.h"
#include "util/helpers/noncopyable.h"
#include <vector>
#include <memory>

namespace Anki {

class Pose3d;

namespace Cozmo {

class CozmoContext;
class IPathPlanner;
class PathDolerOuter;
class Robot;
class SpeedChooser;
struct FallbackPlannerPoses;
struct PathMotionProfile;

enum class ERobotDriveToPoseStatus {
  // There was an internal error while planning
  Error,

  // computing the initial path (the robot is not moving)
  ComputingPath,

  // replanning based on an environment change. The robot is likely following the old path while this is
  // happening
  Replanning,

  // Following a planned path
  FollowingPath,

  // Stopped and waiting (not planning or following)
  Waiting,
};

class PathComponent : private Util::noncopyable
{
public:
  PathComponent(Robot& robot, const RobotID_t robotID, const CozmoContext* context);
  ~PathComponent();

  const SpeedChooser& GetSpeedChooser() const { return *_speedChooser; }
  SpeedChooser& GetSpeedChooser() { return *_speedChooser; }

  void ExecuteTestPath(const PathMotionProfile& motionProfile);

  // Begin computation of a path to drive to the given pose (or poses). Once the path is computed, the robot
  // will immediately start following it, and will replan (e.g. to avoid new obstacles) automatically. If
  // useManualSpeed is set to true, the robot will plan a path to the goal, but won't actually execute any
  // speed changes, so the user (or some other system) will have control of the speed along the "rails" of
  // the path.
  Result StartDrivingToPose(const Pose3d& pose,
                            const PathMotionProfile& motionProfile,
                            bool useManualSpeed = false);

  // Just like above, but will plan to any of the given poses. It's up to the robot / planner to pick which
  // pose it wants to go to. The optional second argument is a pointer to a Planning::GoalID, which, if not null, will
  // be set to the pose which is selected once planning is complete
  Result StartDrivingToPose(const std::vector<Pose3d>& poses,
                            const PathMotionProfile& motionProfile,                              
                            Planning::GoalID* selectedPoseIndex = nullptr,
                            bool useManualSpeed = false);
  
  // This function checks the planning / path following status of the robot. See the enum definition for
  // details
  ERobotDriveToPoseStatus CheckDriveToPoseStatus() const { return _driveToPoseStatus; }

  // TODO:(bn) do the StartPlanner functions really need to be public??
  
  // Starts the selected planner with ComputePath, returns true if the selected planner or its fallback
  // does not result in an error. The path may still contain obstacles if the selected planner didnt
  // consider obstacles in its search.
  bool StartPlanner(const Pose3d& driveCenterPose, const std::vector<Pose3d>& targetDriveCenterPoses);
  // Replans with ComputeNewPathIfNeeded
  void RestartPlannerIfNeeded(bool forceReplan);
  // Start the planner based on the stored fallback poses
  bool StartPlannerWithFallbackPoses();
    
  bool IsTraversingPath()      const { return (_currPathSegment >= 0) || (_lastSentPathID > _lastRecvdPathID); }
 
  s8   GetCurrentPathSegment() const { return _currPathSegment; }
  u16  GetLastRecvdPathID()    const { return _lastRecvdPathID; }
  u16  GetLastSentPathID()     const { return _lastSentPathID; }

  bool IsUsingManualPathSpeed() const {return _usingManualPathSpeed;}
  
  // Execute a manually-assembled path
  Result ExecutePath(const Planning::Path& path, const bool useManualSpeed = false);

  // handle new data from the robot
  void UpdateRobotData(s8 currPathSegment, u16 lastRecvdPathID);

  void Update();

  // stops planning and path following
  Result Abort();

  // TODO:(bn) merge ClearPath into Abort
  Result ClearPath();

  // depricated
  void SetUsingManualSpeed(bool useManualSpeed) { _usingManualPathSpeed = useManualSpeed; }


private:

  // track data from the robot
  void SetCurrPathSegment(const s8 s)     {_currPathSegment = s;}
  void SetLastRecvdPathID(u16 path_id)    {_lastRecvdPathID = path_id;}

  // these functions set _selectedPathPlanner to the appropriate planner
  void SelectPlanner(const Pose3d& targetPose);
  void SelectPlanner(const std::vector<Pose3d>& targetPoses);

  // Clears the path that the robot is executing which also stops the robot
  Result SendClearPath() const;
  
  std::unique_ptr<SpeedChooser>            _speedChooser;
  std::unique_ptr<PathDolerOuter>          _pdo;

  // There are multiple planners, only one of which can be selected at a time. Some of these might point to
  // the same planner.

  // These are the available planners
  std::shared_ptr<IPathPlanner> _longPathPlanner;
  std::shared_ptr<IPathPlanner> _shortPathPlanner;
  std::shared_ptr<IPathPlanner> _shortMinAnglePathPlanner;

  // References to one of the above planners
  std::shared_ptr<IPathPlanner> _selectedPathPlanner;
  std::shared_ptr<IPathPlanner> _fallbackPathPlanner;

  std::unique_ptr<Pose3d>  _currentPlannerGoal;
  Planning::GoalID*        _plannerSelectedPoseIndexPtr  = nullptr;
  int                      _numPlansStarted              = 0;
  int                      _numPlansFinished             = 0;
  ERobotDriveToPoseStatus  _driveToPoseStatus            = ERobotDriveToPoseStatus::Waiting;
  s8                       _currPathSegment              = -1;
  u16                      _lastSentPathID               = 0;
  u16                      _lastRecvdPathID              = 0;
  bool                     _usingManualPathSpeed         = false;
  bool                     _fallbackShouldForceReplan    = false;

  std::unique_ptr<PathMotionProfile>    _pathMotionProfile;
  std::unique_ptr<FallbackPlannerPoses> _fallbackPoseInfo;
  
  Robot& _robot;
};

}
}



#endif
