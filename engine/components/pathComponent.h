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

#include "coretech/common/shared/types.h"
#include "coretech/planning/shared/goalDefs.h"
#include "coretech/planning/shared/path.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <memory>

namespace Anki {

class Pose3d;

namespace Cozmo {

class CozmoContext;
class IActionRunner;
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

  // Following a planned path. While in this state, it may also be replanning, sending new paths, scanning prox obstacles, etc.
  FollowingPath,
    
  // Stopped and waiting (not planning or following)
  Ready,

  // Similar to WaitingToBeginPath, this is the state the component is in when it has sent down a "cancel"
  // request (e.g. in response to Abort), but the robot hasn't confirmed it yet. Note that the path
  // component is able to start a new path from this state as well
  WaitingToCancelPath,

  // Same as above, but also counts as a failure
  WaitingToCancelPathAndSetFailure
};

constexpr const char* ERobotDriveToPoseStatusToString(ERobotDriveToPoseStatus status)
{

#define HANDLE_ETDTPS_CASE(v) case ERobotDriveToPoseStatus::v: return #v

  switch(status) {
    HANDLE_ETDTPS_CASE(Failed);
    HANDLE_ETDTPS_CASE(ComputingPath);
    HANDLE_ETDTPS_CASE(WaitingToBeginPath);
    HANDLE_ETDTPS_CASE(FollowingPath);
    HANDLE_ETDTPS_CASE(Ready);
    HANDLE_ETDTPS_CASE(WaitingToCancelPath);
    HANDLE_ETDTPS_CASE(WaitingToCancelPathAndSetFailure);
  }
  
#undef HANDLE_ETDTPS_CASE
}

class PathComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  PathComponent();
  ~PathComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override{
    dependencies.insert(RobotComponentID::AIComponent);
    dependencies.insert(RobotComponentID::ActionList);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  const SpeedChooser& GetSpeedChooser() const { return *_speedChooser; }
  SpeedChooser& GetSpeedChooser() { return *_speedChooser; }

  void ExecuteTestPath(const PathMotionProfile& motionProfile);

  // Begin computation of a path to drive to the given pose (or poses). Once the path is computed, the robot
  // will immediately start following it, and will replan (e.g. to avoid new obstacles) automatically. 
  // It's up to the robot / planner to pick which pose it wants to go to. The optional selectedPoseIndex
  // shared_ptr, if specified, will be updated when planning is complete to specify which of the target poses
  // the planner selected. The speed along the path will be determined by the CustomPathMotionProfile held in
  // this component, or will get set by the SpeedChooser if this component hasn't been set to use a custom
  // profile
  Result StartDrivingToPose(const std::vector<Pose3d>& poses,
                            std::shared_ptr<Planning::GoalID> selectedPoseIndexPtr = {});

  // Can be used to start the planner before calling StartDrivingToPose
  Result PrecomputePath(const std::vector<Pose3d>& poses,
                        std::shared_ptr<Planning::GoalID> selectedPoseIndexPtr = {});
  // Check if a precomputed path is ready
  bool IsPlanReady() const;

  // set or clear the custom motion profile that all motion should follow. If cleared, then defaults will be
  // used, or the speed chooser will be used if enabled
  void SetCustomMotionProfile(const PathMotionProfile& motionProfile);
  void ClearCustomMotionProfile();

  // Set a motion profile and have it cleared automatically when the action finishes. Note that this will
  // clear when the action is _destroyed_.
  void SetCustomMotionProfileForAction(const PathMotionProfile& motionProfile, IActionRunner* action);
  
  // Determines if replanning can select a different goal than was originally selected. no effect if there is only one goal
  void SetCanReplanningChangeGoal(bool canChangeGoal) { _replanningCanChangeGoal = canChangeGoal; }

  // check / get the custom motion profile. Note that there is always _some_ motion profile that paths follow,
  // but these functions refer to the _custom_ profile which, for example, might get set by a behavior
  bool HasCustomMotionProfile() const;

  // Returns a copy of the custom motion profile with speeds that
  // have been clamped for speed safety.
  // The returned profile also depends on whether or not the
  // robot is currently carrying an object.
  PathMotionProfile GetCustomMotionProfile() const;
  
  // This function checks the planning / path following status of the robot. See the enum definition for
  // details. In 99% of cases you should prefer the direct bool functions below, like IsActive() or
  // LastPathFailed()
  ERobotDriveToPoseStatus GetDriveToPoseStatus() const { return _driveToPoseStatus; }

  // Opposite of IsReady, this component is actively doing something (planning, following, etc)
  bool IsActive() const;

  // returns true if the automatic replanning is running, until the search is complete.
  // returns false during the initial planning stage, or once a search is complete
  bool IsReplanning() const { return _isReplanning; }
  
  // True if there is a path to follow (state is following path or waiting to begin)
  bool HasPathToFollow() const;
  
  // True if the robot is following a path but is momentarily stopped (the robot issued a
  // PATH_COMPLETED). This is probably because it's replanning and it exhaused its safe subpath
  bool HasStoppedBeforeExecuting() const { return _hasStoppedBeforeExecuting; }

  // True if the last path failed (based on status of failure)
  bool LastPathFailed() const;
  
  std::shared_ptr<Planning::GoalID> GetLastSelectedIndex() const { return _plannerSelectedPoseIndex; }
  
  // Execute a manually-assembled path
  Result ExecuteCustomPath(const Planning::Path& path);

  // Checks if the path provided is safe. If driveCenter is provided, then check using the angle of that pose 
  // (PathPlanner interface does not yet support full start state specification, just starting angle)
  bool IsPathSafe(const Planning::Path& path, const Pose3d* driveCenter = nullptr) const;

  // Handle new data from the robot
  void UpdateCurrentPathSegment(s8 currPathSegment);

  // Stops planning and path following. Returns RESULT_OK if successfully aborted (this may fail, e.g., if
  // message sending to the robot fails)
  Result Abort();
  
  // If you called PrecomputePath, the robot will not start following the path until StartDrivingToPose
  // is called. For replanning, the robot will automatically start following the path unless you call this
  // with autoStart == false. Call it again with autoStart == true to start driving when replanning is finished,
  // or now if replanning already finished
  void SetStartPath(bool autoStart);

  // These should only be used for debugging / printing. Use more direct functions for checking the state of
  // this component
  s8   GetCurrentPathSegment() const { return _currPathSegment; }
  u16  GetLastRecvdPathID()    const { return _lastRecvdPathID; }
  u16  GetLastSentPathID()     const { return _lastSentPathID; }

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
  // safe). Gets plan from _selectedPathPlanner. This method may set abort flags if the proposed plan
  // is invalid, or it may set retry flags if the proposed plan is NOT YET valid (e.g., while still
  // driving during replanning).
  void TryCompletingPath();

  // Starts the selected planner with ComputePath, using the params in _currPlanParams, returns true if the
  // selected planner or its fallback starts successfully. The path may still contain obstacles if the
  // selected planner didnt consider obstacles in its search. If the argument is omitted, the drive center
  // will be computed from the _robot pose. These functions do not modify _driveToPoseStatus
  bool StartPlanner();
  bool StartPlanner(const Pose3d& driveCenterPose);

  // configures the start state and goal states for path, and launches the planner if it is 
  // not already computing that path
  Result ConfigureAndStartPlanner(const std::vector<Pose3d>& poses,
                            std::shared_ptr<Planning::GoalID> selectedPoseIndexPtr = {});

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
  // Trims the end of the path that the robot is executing
  Result TrimRobotPathToLength( uint8_t length );

  bool IsWaitingToCancelPath() const;

  // true if we're waiting for something from the robot (e.g. start path or cancel path)
  bool IsWaitingForRobotResponse() const;
  
  // Drive the given path
  Result ExecutePath(const Planning::Path& path);

  void SetDriveToPoseStatus(ERobotDriveToPoseStatus newValue);
  
  PathMotionProfile ClampToCliffSafeSpeed(const PathMotionProfile& motionProfile) const;

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
  float                    _lastMsgSendTime_s            = 0.0f;
  s8                       _currPathSegment              = -1;
  u16                      _lastSentPathID               = 0;
  u16                      _lastRecvdPathID              = 0;
  bool                     _plannerActive                = false;
  bool                     _hasCustomMotionProfile       = false;
  bool                     _startFollowingPath           = true;
  bool                     _isReplanning                 = false;
  bool                     _replanningCanChangeGoal      = true;
  bool                     _waitingToMatchReplanOrigin   = false;
  bool                     _hasStoppedBeforeExecuting    = false;

  // keep track of the last path ID we canceled. Note that when we cancel a path we will transition to one of
  // the "waiting to cancel" statuses, but then we may start working on a new plan before we actually get a
  // message letting us know that the robot canceled the path. In that case, this variable will remain set but
  // the status may move on to, e.g. ComputingPath or WaitingToBeginPath. We always leave this set, rather
  // than resetting it to 0, just in case the robot send us multiple path event type messages with the same
  // path ID (e.g. a completed followed by an interrupted)
  u16 _lastCanceledPathID = 0;

  std::unique_ptr<PathMotionProfile> _pathMotionProfile;
  std::unique_ptr<PlanParameters>    _currPlanParams;
  
  Robot* _robot;
  
  Signal::SmartHandle _pathEventHandle;
  
};


}
}



#endif
