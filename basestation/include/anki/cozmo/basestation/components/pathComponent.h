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
struct PlanParameters;
struct PathMotionProfile;

enum class ERobotDriveToPoseStatus {
  // Something went wrong with the last plan (either planning or traversing). This also implies that the
  // pathComponent is ready for a new request (i.e. it is equivalent to Ready, but holds the additional state
  // that the _last_ StartDrivingToPose request failed)
  Failed = 0,
  
  // Computing a path while _not_ following. This means that, if the computation is successful, the robot will
  // begin following the path
  ComputingPath,

  // Engine has completed planning, sent it's plan to the robot, and is waiting for the robot to begin path
  // traversal. The robot is not currently following a path
  WaitingToBeginPath,

  // Following a planned path. While in this state, it may also be replanning, sending new paths, etc.
  FollowingPath,
    
  // Stopped and waiting (not planning or following)
  Ready,
};

class PathComponent : private Util::noncopyable
{
public:

  // Constructor takes a robotID because the passed in robot may still be under construction when this
  // constructor is called, and therefore we shouldn't trust it's contents (just store it and pass it around)
  PathComponent(Robot& robot, const RobotID_t robotID, const CozmoContext* context);
  ~PathComponent();

  const SpeedChooser& GetSpeedChooser() const { return *_speedChooser; }
  SpeedChooser& GetSpeedChooser() { return *_speedChooser; }

  void ExecuteTestPath(const PathMotionProfile& motionProfile);

  // Begin computation of a path to drive to the given pose (or poses). Once the path is computed, the robot
  // will immediately start following it, and will replan (e.g. to avoid new obstacles) automatically. If
  // useManualSpeed is set to true, the robot will plan a path to the goal, but won't actually execute any
  // speed changes, so the user (or some other system) will have control of the speed along the "rails" of the
  // path. It's up to the robot / planner to pick which pose it wants to go to. The optional selectedPoseIndex
  // shared_ptr, if specified, will be updated when planning is complete to specify which of the target poses
  // the planner selected. 
  Result StartDrivingToPose(const std::vector<Pose3d>& poses,
                            const PathMotionProfile& motionProfile,                              
                            std::shared_ptr<Planning::GoalID> selectedPoseIndex = {},
                            bool useManualSpeed = false);
  
  // This function checks the planning / path following status of the robot. See the enum definition for
  // details
  ERobotDriveToPoseStatus GetDriveToPoseStatus() const { return _driveToPoseStatus; }

  // Opposite of IsReady, this component is actively doing something (planning, following, etc)
  bool IsActive() const;

  // True if there is a path to follow (state is following path or waiting to begin)
  bool HasPathToFollow() const;
  
  // Execute a manually-assembled path
  Result ExecuteCustomPath(const Planning::Path& path, const bool useManualSpeed = false);

  // Handle new data from the robot
  void UpdateRobotData(s8 currPathSegment, u16 lastRecvdPathID);

  void Update();

  // Stops planning and path following. Returns RESULT_OK if successfully aborted (this may fail, e.g., if
  // message sending to the robot fails)
  Result Abort();

  // These should only be used for debugging / printing. Use more direct functions for checking the state of
  // this component
  s8   GetCurrentPathSegment() const { return _currPathSegment; }
  u16  GetLastRecvdPathID()    const { return _lastRecvdPathID; }
  u16  GetLastSentPathID()     const { return _lastSentPathID; }
  
  // Deprecated
  void SetUsingManualSpeed(bool useManualSpeed) { _usingManualPathSpeed = useManualSpeed; }

private:

  // Track data from the robot
  void SetCurrPathSegment(const s8 s)     {_currPathSegment = s;}
  void SetLastRecvdPathID(u16 path_id)    {_lastRecvdPathID = path_id;}

  // Set _selectedPathPlanner to the appropriate planner based on the parameters in _currPlanParams
  void SelectPlanner();
  void SelectPlannerHelper(const Pose3d& targetPose);

  void HandlePossibleOriginChanges();
  void UpdatePlanning();

  // Used when the origin changes during path traversal. If possible, it will rejigger the targets into the
  // new origin and continue planning. Sets status internally
  void RejiggerTargetsAndReplan();

  // Used when the planner finishes, to begin following the proposed plan (if desired and the plan is
  // safe). Gets plan from _selectedPathPlanner
  void HandlePlanComplete();  

  // Starts the selected planner with ComputePath, using the params in _currPlanParams, returns true if the
  // selected planner or its fallback starts successfully. The path may still contain obstacles if the
  // selected planner didnt consider obstacles in its search. If the argument is omitted, the drive center
  // will be computed from the _robot pose. These functions do not modify _driveToPoseStatus
  bool StartPlanner();
  bool StartPlanner(const Pose3d& driveCenterPose);

  // Abort the plan and any path following, and set the status to Failure
  void AbortAndSetFailure();

  // Called when path following successfully finishes, sets status to Ready and takes care of any other
  // cleanup that may be needed
  void OnPathComplete();

  // If possible, switch to the fallback planner and begin planning. If this succeeds, return true, otherwise
  // return false. Does not set DriveToPoseStatus
  bool ReplanWithFallbackPlanner();
  
  // Replans with ComputeNewPathIfNeeded
  void RestartPlannerIfNeeded();

  // Clears the path that the robot is executing which also stops the robot
  Result ClearPath();
  Result SendClearPath() const;

  // Drive the given path
  Result ExecutePath(const Planning::Path& path, const bool useManualSpeed = false);

  void SetDriveToPoseStatus(ERobotDriveToPoseStatus newValue);
  
  std::unique_ptr<SpeedChooser>   _speedChooser;
  std::unique_ptr<PathDolerOuter> _pdo;

  // There are multiple planners, only one of which can be selected at a time. Some of these might point to
  // the same planner.

  // These are the available planners
  std::shared_ptr<IPathPlanner> _longPathPlanner;
  std::shared_ptr<IPathPlanner> _shortPathPlanner;
  std::shared_ptr<IPathPlanner> _shortMinAnglePathPlanner;

  // References to one of the above planners. Selected is the one we should use, fallback is the one we will
  // use if selected fails (or null if none, in which case we fail without trying another planner)
  std::shared_ptr<IPathPlanner> _selectedPathPlanner;
  std::shared_ptr<IPathPlanner> _fallbackPathPlanner;

  std::shared_ptr<Planning::GoalID>  _plannerSelectedPoseIndex;

  // Do not directly set _driveToPoseStatus (use transition function instead)
  ERobotDriveToPoseStatus  _driveToPoseStatus            = ERobotDriveToPoseStatus::Ready;
  float                    _lastPathSendTime_s           = 0.0f;
  s8                       _currPathSegment              = -1;
  u16                      _lastSentPathID               = 0;
  u16                      _lastRecvdPathID              = 0;
  bool                     _usingManualPathSpeed         = false;
  bool                     _plannerActive                = false;

  std::unique_ptr<PathMotionProfile> _pathMotionProfile;
  std::unique_ptr<PlanParameters>    _currPlanParams;
  
  Robot& _robot;
};

}
}



#endif
