/**
 * File: pathComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-03-28
 *
 * Description: Component that handles path planning and following
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/pathComponent.h"

#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/actionInterface.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceAndApproachPlanner.h"
#include "engine/latticePlanner.h"
#include "engine/xyPlanner.h"
#include "engine/minimalAnglePlanner.h"
#include "engine/pathDolerOuter.h"
#include "engine/pathPlanner.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/speedChooser.h"
#include "engine/viz/vizManager.h"
#include "util/logging/logging.h"
#include <limits>

namespace Anki {
namespace Vector {

namespace {
static constexpr const float kMaxDistanceForShortPlanner_mm = 40.0f;
static constexpr const float kMinDistanceForMinAnglePlanner_mm = 1.0f;
static constexpr const float kSendMsgFailedTimeout_s = 1.0f;
}

// info for the current plan
struct PlanParameters
{
  PlanParameters() = default;
  PlanParameters(Vector::Robot* robot, const std::vector<Pose3d>& poses);
  void Reset();
  bool IsEqual(const PlanParameters& otherPlan) const;

  // these target poses are in terms of the "drive center" of the robot, which may be different from the
  // "pose" the robot is at when it gets there (e.g. pose may be geometric center, instead of between front
  // wheels)
  std::vector<Pose3d> targetPoses;

  // starting point for the plan
  Pose3d driveCenter;

  // the driver center pose and target poses all must share this origin for the plan to be valid
  PoseOriginID_t commonOriginID = PoseOriginList::UnknownOriginID;
};

PlanParameters::PlanParameters(Vector::Robot* robot, const std::vector<Pose3d>& poses)
{
  this->commonOriginID = robot->GetPoseOriginList().GetCurrentOriginID();

  // Compute drive center pose for start pose and goal poses
  this->driveCenter = robot->GetDriveCenterPose();
  this->targetPoses.resize(poses.size());
  for (int i=0; i< poses.size(); ++i) {
    robot->ComputeDriveCenterPose(poses[i], this->targetPoses[i]);
  }
}

void PlanParameters::Reset() {
  targetPoses.clear();
  commonOriginID = PoseOriginList::UnknownOriginID;
}

bool PlanParameters::IsEqual(const PlanParameters& otherPlan) const
{
  if ( otherPlan.commonOriginID != this->commonOriginID )               { return false; }
  if ( otherPlan.targetPoses.size() != this->targetPoses.size())        { return false; }
  if (!otherPlan.driveCenter.IsSameAs( this->driveCenter, .001, .001 )) { return false; }

  for (int i = 0; i < this->targetPoses.size(); ++i)
  {
    if (!otherPlan.targetPoses[i].IsSameAs(this->targetPoses[i], .001, .001)) { return false; }
  }

  return true;
}

PathComponent::PathComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::PathPlanning)
, _shortPathPlanner(new FaceAndApproachPlanner)
, _shortMinAnglePathPlanner(new MinimalAnglePlanner)
, _pathMotionProfile(new PathMotionProfile(DEFAULT_PATH_MOTION_PROFILE))
, _currPlanParams(new PlanParameters)
{
}

PathComponent::~PathComponent()
{
  Abort();
}

void PathComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  const CozmoContext* context =  _robot->GetContext();
  _speedChooser = std::make_unique<SpeedChooser>(*_robot);
  if( context ) {
    // might not exist (e.g. unit tests)
    _pdo.reset(new PathDolerOuter(context->GetRobotManager()->GetMsgHandler()));

    if (nullptr != context->GetDataPlatform()) {      
      _longPathPlanner.reset(new XYPlanner(_robot));
    }
    else {
      // For unit tests, or cases where we don't have data, use the short planner in it's place
      PRINT_NAMED_WARNING("Robot.PathComponent.NoDataPlatform.WrongPlanner",
                          "Using short planner as the long planner, since we dont have a data platform");
      _longPathPlanner = _shortPathPlanner;
    }
  }
  else {
    PRINT_NAMED_WARNING("Robot.PathComponent.NoContext", "No cozmo context, won't be fully functional");
  }


  auto eventLambda = [this](const AnkiEvent<RobotInterface::RobotToEngine>& event)
  {
    RobotInterface::PathFollowingEvent payload = event.GetData().Get_pathFollowingEvent();

    PRINT_CH_DEBUG("Planner", "PathComponent.ReceivedPathEvent", "ID:%d Event:%s",
                   payload.pathID, EnumToString(payload.eventType));

    // handle complete and interrupted paths in cases where we wait to cancel. Returns true if this lambda
    // handled the message, false otherwise
    auto handleCancelLambda = [this](const RobotInterface::PathFollowingEvent& payload) {
      if( IsWaitingToCancelPath() ) {
        if( payload.pathID == _lastCanceledPathID ) {
          // path has canceled, update state appropriately
          if( _driveToPoseStatus == ERobotDriveToPoseStatus::WaitingToCancelPath ) {
            SetDriveToPoseStatus(ERobotDriveToPoseStatus::Ready);
          }
          else {
            ANKI_VERIFY(_driveToPoseStatus == ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure,
                        "PathComponent.PathEvent.Completion.StatusMismatch",
                        "We are waiting to cancel, but status is '%s'",
                        ERobotDriveToPoseStatusToString(_driveToPoseStatus));
            SetDriveToPoseStatus(ERobotDriveToPoseStatus::Failed);
          }
        }
        else {
          // This is possible in cases of high latency. For example, engine could send path 1, send a cancel
          // for path 1 and a new path, path 2, then abort path 2. After this, we might receive a message
          // about path 1 while waiting to cancel path 2
          PRINT_NAMED_WARNING("PathComponent.PathEvent.CanceledPathDifferentPathComplete",
                              "We are in status '%s' waiting for path %d to cancel, but got message that path %d is %s",
                              ERobotDriveToPoseStatusToString(_driveToPoseStatus),
                              _lastCanceledPathID,
                              payload.pathID,
                              PathEventTypeToString(payload.eventType));
        }
        // we handled the message here, so tell the caller not to continue to process it
        return true;
      }

      if( _lastCanceledPathID > 0 && payload.pathID == _lastCanceledPathID ) {
        // we got a complete or interrupted message, but we've already canceled that path (and moved on to a
        // new one, which is why our status isn't Waiting and this case isn't handled above). In this case,
        // ignore this message (mark it as handled)
        return true;
      }

      // in this case, let normal handling of the message continue
      return false;
    };

    switch(payload.eventType)
    {
      case PathEventType::PATH_STARTED:
      {

        if( IsWaitingToCancelPath() || payload.pathID == _lastCanceledPathID ) {

          // If we are waiting to cancel _any_ path, ignore paths starting. Also, separately check
          // _lastCanceledPathID because we may have moved on to a status for a new path while still expecting
          // to receive a cancel from an old path

          PRINT_CH_INFO("Planner", "PathComponent.PathEvent.OldPathStartedWhileWaitingForCancel",
                        "The robot started path %d which is about to be canceled. Status is %s",
                        payload.pathID,
                        ERobotDriveToPoseStatusToString(_driveToPoseStatus));
          // don't set any state, ignore this message
          break;
        }

        // In general, we should be coming from "Waiting to Begin" when we receive Path Started. We could be
        // "Following" already if path/segment ID in RobotState already transitioned us, or waiting to cancel
        // (which is handled above, so not listed here)
        if( !ANKI_VERIFY(ERobotDriveToPoseStatus::WaitingToBeginPath == _driveToPoseStatus ||
                         ERobotDriveToPoseStatus::FollowingPath == _driveToPoseStatus,
                         "PathComponent.PathEvent.UnexpectedStatus",
                         "Expecting to be WaitingToBeginPath or FollowingPath, not %s",
                         ERobotDriveToPoseStatusToString(_driveToPoseStatus)) ) {
          Abort();
          break;
        }

        ANKI_VERIFY(payload.pathID == _lastSentPathID,
                    "PathComponent.PathEvent.StartingUnexpectedPathID",
                    "Last sent ID:%d, starting ID:%d",
                    _lastSentPathID, payload.pathID);

        SetLastRecvdPathID(payload.pathID);

        // Note that this does nothing if we're already FollowingPath
        SetDriveToPoseStatus(ERobotDriveToPoseStatus::FollowingPath);
        break;
      }

      case PathEventType::PATH_COMPLETED:
      {
        if( handleCancelLambda(payload) ) {
          return;
        }

        // Verify we are completing the last path we sent.
        ANKI_VERIFY(payload.pathID == _lastSentPathID,
                    "PathComponent.PathEvent.CompletingUnexpectedPathID",
                    "Last sent ID:%d, completing ID:%d",
                    _lastSentPathID, payload.pathID);

        // Note that OnPathComplete is safe to call if the path is already complete (e.g. thanks to
        // RobotState message updates)
        OnPathComplete();

        break;
      }

      case PathEventType::PATH_INTERRUPTED:
      {
        if( handleCancelLambda(payload) ) {
          return;
        }

        // Verify we are interrupting the last path we received.
        ANKI_VERIFY(payload.pathID == _lastRecvdPathID,
                    "PathComponent.PathEvent.InterruptingUnexpectedPathID",
                    "Last received ID:%d, completing ID:%d",
                    _lastRecvdPathID, payload.pathID);

        // Only mark complete if this interruption is for the last path we actually sent and not an earlier one, which
        // we don't care about anymore
        if(payload.pathID == _lastSentPathID)
        {
          // Note that OnPathComplete is safe to call if the path is already complete (e.g. thanks to
          // RobotState message updates)
          OnPathComplete();
        }

        break;
      }
    }
  };

  _pathEventHandle = _robot->GetRobotMessageHandler()->Subscribe(RobotInterface::RobotToEngineTag::pathFollowingEvent,
                                                                 eventLambda);

}


Result PathComponent::Abort()
{
  PRINT_CH_INFO("Planner",
                "PathComponent.Abort",
                "Aborting from status '%s'",
                ERobotDriveToPoseStatusToString(_driveToPoseStatus));

  if( _selectedPathPlanner ) {
    _selectedPathPlanner->StopPlanning();
    _plannerActive = false;
  }
  _plannerSelectedPoseIndex.reset();

  Result ret = ClearPath();

  // update status to either ready or waiting to cancel
  switch(_driveToPoseStatus) {
    case ERobotDriveToPoseStatus::Failed:
    case ERobotDriveToPoseStatus::ComputingPath:
    case ERobotDriveToPoseStatus::Ready:
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::Ready);
      break;

    case ERobotDriveToPoseStatus::WaitingToBeginPath:
    case ERobotDriveToPoseStatus::FollowingPath:
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::WaitingToCancelPath);
      break;

    case ERobotDriveToPoseStatus::WaitingToCancelPath:
    case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure:
      // no change
      break;
  }

  _currPlanParams->Reset();

  return ret;
}

void PathComponent::AbortAndSetFailure()
{
  Result res = Abort();
  DEV_ASSERT(res == RESULT_OK, "PathComponent.Abort.FailedToAbort");
  SetDriveToPoseStatus(ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure);
}

void PathComponent::OnPathComplete()
{
  _hasStoppedBeforeExecuting = true;
  _robot->GetContext()->GetVizManager()->ErasePath(_robot->GetID());
  if( !_plannerActive ) {
    // If the planner isn't running (or replanning) then we should be done
    SetDriveToPoseStatus(ERobotDriveToPoseStatus::Ready);
  }
  if( _waitingToMatchReplanOrigin ) {
    // see if having reached the end of the path means it is complete
    TryCompletingPath();
  }
  if( (_selectedPathPlanner != nullptr) && (_selectedPathPlanner->CheckPlanningStatus() != EPlannerStatus::Running) ) {
    // we reached the end of the path and the planner isnt running, so there's no chance the robot
    // will make it to the replanning origin if it hasn't already. Set this as false to allow failure
    // if the robot isnt in place
    _waitingToMatchReplanOrigin = false;
  }
}

void PathComponent::UpdateCurrentPathSegment(s8 currPathSegment)
{
  const bool robotReceivedLastPath = (_lastSentPathID > 0) && (_lastRecvdPathID == _lastSentPathID);

  if( robotReceivedLastPath ) {

    // only update the current segment if we are on the right path
    SetCurrPathSegment(currPathSegment);

    if( _driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath &&
       currPathSegment < 0 &&
       _plannerActive) {

      PRINT_CH_INFO("Planner", "PathComponent.UpdateRobotData.ComputingNewPlan",
                    "Actively replanning and finished following path ID %d",
                    _lastRecvdPathID);

      // we are actively replanning, but ran out of our old path. Now we are waiting to compute a new plan
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::ComputingPath);
    }
  }

  if( _driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath ) {
    if( _pdo ) {
      // Dole out more path segments to the physical robot if needed:
      _pdo->Update(_currPathSegment);
    }
  }
}


void PathComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  if( false && _selectedPathPlanner != nullptr ) {
    PRINT_NAMED_WARNING("PathComponentStatus",
                        "plannerStatus=%s, driveStatus=%s, stopped=%d",
                        EPlannerStatusToString( _selectedPathPlanner->CheckPlanningStatus() ),
                        ERobotDriveToPoseStatusToString( GetDriveToPoseStatus() ),
                        _hasStoppedBeforeExecuting);
  }
  
  if( _plannerActive ) {
    UpdatePlanning();
  }

  // Try restarting the planner if the robot is following a path to deal with new obstacles or
  // origin shifts.
  // Also, to handle an edge case where the robot stopped short of its expected start pose during
  // replanning, try once to restart the planner, which will decide if it's actually
  // necessary or possible to do so.
  const bool tooFarFromReplanStart = _waitingToMatchReplanOrigin
                                     && _hasStoppedBeforeExecuting
                                     && (_selectedPathPlanner != nullptr)
                                     && (_selectedPathPlanner->GetErrorType() == EPlannerErrorType::TooFarFromPlan)
                                     && (_driveToPoseStatus == ERobotDriveToPoseStatus::ComputingPath);
  if( (_driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath) || tooFarFromReplanStart ) {
    if( tooFarFromReplanStart ) {
      _waitingToMatchReplanOrigin = false;
    }
    HandlePossibleOriginChanges();
  } else if( _waitingToMatchReplanOrigin ) {
    // Maybe having driven a bit since the last UpdateDependent() means the robot is closer to the start of
    // its replanned path. Let it attempt to start driving it again
    TryCompletingPath();
  }

  if( IsWaitingForRobotResponse() ) {
    // we are waiting on the robot to for something, make sure it hasn't timed out
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( currTime_s > _lastMsgSendTime_s + kSendMsgFailedTimeout_s ) {
      PRINT_NAMED_ERROR("PathComponent.SentUnreceivedPath",
                        "robot did not start executing path. Last send = %d, last recv = %d",
                        _lastSentPathID,
                        _lastRecvdPathID);
      AbortAndSetFailure();
    }
  }
}

void PathComponent::HandlePossibleOriginChanges()
{
  if( !_selectedPathPlanner ) {
    // can't handle any changes if there is no planner
    return;
  }
  
  const bool tooFar __attribute((unused)) = (_selectedPathPlanner->GetErrorType() == EPlannerErrorType::TooFarFromPlan);
  DEV_ASSERT( (_driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath)
             || ((_driveToPoseStatus == ERobotDriveToPoseStatus::ComputingPath) && tooFar),
             "PathComponent.Update.FollowingStateMismatch");

  // should always have valid origin since we're planning
  // TODO: COZMO-1637: Once we figure this out, switch these verifies back to dev_asserts for efficiency
  ANKI_VERIFY(_currPlanParams->commonOriginID != PoseOriginList::UnknownOriginID,
              "PathComponent.HandlePossibleOriginChanges.NullParamsOrigin", "");

  ANKI_VERIFY(_robot->GetPoseOriginList().ContainsOriginID(_currPlanParams->commonOriginID),
              "PathComponent.Update.CommonOriginNotInRobotPoseOriginList",
              "ID:%d", _currPlanParams->commonOriginID);

  // haveOriginsChanged: Check if the robot's origin has changed since planning. If no delocs, relocs, or
  // rejiggers have happened, our stored commonOrigin will be the robots world origin. Otherwise, we check
  // for origin rejiggering. If our stored origin was rejiggered, then it is no longer an origin itself. If
  // we can get it with respect to the new origin, we can rejigger targetPoses and continue. One example
  // where this happens would be that the robot localizes to object 1 (origin A), get picked up and put down
  // (creating origin B), starts driving somewhere, and while driving sees object 1, which causes a rejigger
  // to origin A
  const bool haveOriginsChanged = ( _currPlanParams->commonOriginID != _robot->GetPoseOriginList().GetCurrentOriginID() );

  const Pose3d& commonOrigin = _robot->GetPoseOriginList().GetOriginByID(_currPlanParams->commonOriginID);
  const bool canAdjustOrigin = _robot->IsPoseInWorldOrigin(commonOrigin);

  if ( haveOriginsChanged && !canAdjustOrigin ) {
    // the origins changed and we can't rejigger our goal to the new origin (we probably delocalized),
    // completely abort
    AbortAndSetFailure();
    PRINT_CH_INFO("Planner", "PathComponent.Update.Replan.NotPossible",
                  "Our goal is in another coordinate frame that we can't get wrt current, we can't replan");
  }
  else if (haveOriginsChanged && canAdjustOrigin) {
    RejiggerTargetsAndReplan();
  }
  else {
    // we didn't need to adjust origins, so delegate on Restart to decide if we should replan
    RestartPlannerIfNeeded();
  }

  // else we are following just fine and the world hasn't changed, so keep going
}

void PathComponent::RejiggerTargetsAndReplan()
{

  if( ! _selectedPathPlanner ) {
    // can't replan if we don't have a selected planner
    return;
  }

  _currPlanParams->commonOriginID = _robot->GetPoseOriginList().GetCurrentOriginID();

  if( _plannerActive ) {
    // no use carrying on with an old plan, if there is one going
    _selectedPathPlanner->StopPlanning();
    _plannerActive = false;
  }

  PRINT_CH_INFO("Planner", "PathComponent.Update.Replan.RejiggeringPlanner",
                "Our goal is in another coordinate frame, but we are updating to current frame");

  // our origin changed, but we can rejigger. Do so now, and re-plan to take any new information from
  // the rejigger into account
  bool rejiggerSuccess = true;
  for( auto& targetPose : _currPlanParams->targetPoses ) {
    rejiggerSuccess &= targetPose.GetWithRespectTo(_robot->GetWorldOrigin(), targetPose);
  }

  if( ANKI_VERIFY(rejiggerSuccess, "PathComponent.Update.Rejigger",
                  "Rejiggering %zu target poses to new origin '%s' failed",
                  _currPlanParams->targetPoses.size(),
                  _robot->GetWorldOrigin().GetName().c_str())) {

    const bool started = StartPlanner();
    if( started ) {
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::ComputingPath);
    }
    else if( ReplanWithFallbackPlanner() ) {
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::ComputingPath);
      PRINT_CH_INFO("Planner", "PathComponent.Update.Rejigger.Fallback",
                    "Planning to newly rejiggered poses failed, but fallback succeeded.");
    }
    else {
      PRINT_NAMED_WARNING("PathComponent.Update.Replan.RejiggeringPlanner",
                          "We could not start driving to rejiggered pose.");
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::Failed);
    }
  }
  else {
    SetDriveToPoseStatus(ERobotDriveToPoseStatus::Failed);
  }
}

void PathComponent::UpdatePlanning()
{
  DEV_ASSERT( _plannerActive, "PathComponent.UpdatePlanner.NotActive" );

  // we are waiting on a plan to currently compute
  switch( _selectedPathPlanner->CheckPlanningStatus() ) {
    case EPlannerStatus::Error: {
      if( ReplanWithFallbackPlanner() ) {
        SetDriveToPoseStatus(ERobotDriveToPoseStatus::ComputingPath);
        PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Error",
                      "Running planner returned error status, using fallback planner instead");
      }
      else {
        // abort in case we are currently following an invalid path
        AbortAndSetFailure();
        PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Error", "Running planner returned error status");
      }
      break;
    }

    case EPlannerStatus::Running: {
      DEV_ASSERT(_driveToPoseStatus != ERobotDriveToPoseStatus::Failed &&
                 _driveToPoseStatus != ERobotDriveToPoseStatus::Ready &&
                 _driveToPoseStatus != ERobotDriveToPoseStatus::WaitingToCancelPath &&
                 _driveToPoseStatus != ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure,
                 "PathComponent.UpdatePlanning.StatusMismatch");
      // still waiting on a response from the planner
      break;
    }

    case EPlannerStatus::CompleteWithPlan: {
      _plannerActive = false;
      if (_startFollowingPath) {      // wait here until we are clear to drive
        TryCompletingPath();
      }
      break;
    }

    case EPlannerStatus::CompleteNoPlan: {
      PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteNoPlan",
                    "Running planner complete with no plan");

      _plannerActive = false;
      
      if( _driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath ||
          _driveToPoseStatus == ERobotDriveToPoseStatus::WaitingToBeginPath ||
          _driveToPoseStatus == ERobotDriveToPoseStatus::WaitingToCancelPath ||
          _driveToPoseStatus == ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure ) {

        // we must have replanned and discovered that we are at the goal, so stop following the path
        PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.NoPlanWhileTraversing",
                      "Planner completed with empty plan while we were already following a plan. clearing plan");
        Abort();
        // don't set failure here, since we have an empty plan, we have technically succeeded
      }
      OnPathComplete();
      break;
    }
  }
}

void PathComponent::TryCompletingPath()
{
  if( _selectedPathPlanner->CheckPlanningStatus() == EPlannerStatus::Running ) {
    return;
  }
  
  bool wasReplanning = _isReplanning;
  _isReplanning = false;
  
  // get the path
  Planning::GoalID selectedPoseIdx;
  Planning::Path newPath;

  const Pose3d& driveCenterPose = _robot->GetDriveCenterPose();

  // NOTE: If object-carrying state changes between the time this function is called
  //       and the time the path is executed, the robot will either drive too fast and
  //       potentially fall off cliffs or too slow and potentially look strange.
  PathMotionProfile cliffSafeMotionProfile = ClampToCliffSafeSpeed(*_pathMotionProfile);

  _selectedPathPlanner->GetCompletePath(driveCenterPose,
                                        newPath,
                                        selectedPoseIdx,
                                        &cliffSafeMotionProfile);
  
  // the planner finished but returned no path... either the robot is at the goal, some internal error
  // occurred, or, if the robot was replanning, it probably isn't yet close enough to that position to
  // start following the path. In the latter case, set a flag so that we can retry picking up this
  // path up until reaching the end of it
  if( (newPath.GetNumSegments() == 0)
      && (_selectedPathPlanner->GetErrorType() == EPlannerErrorType::TooFarFromPlan) )
  {
    if( wasReplanning ) {
      _waitingToMatchReplanOrigin = true;
    }
    if( _waitingToMatchReplanOrigin ) {
      return;
    }
  }
  _waitingToMatchReplanOrigin = false;

  // collisions are always OK for empty paths, or if the selected planner actually checked for collisions
  const bool collisionsAcceptable = _selectedPathPlanner->ChecksForCollisions() || newPath.GetNumSegments()==0;

  // Some children of IPathPlanner may return a path that hasn't been checked for obstacles. Here, check if
  // the planner used to compute that path considers obstacles. If it doesn't, check for an obstacle
  // penalty. If there is one, re-run with the lattice planner, which always considers obstacles in its
  // search.
  if( (!collisionsAcceptable) && (nullptr != _longPathPlanner) ) {

    const float startPoseAngle_rad = driveCenterPose.GetRotationAngle<'Z'>().ToFloat();
    DEV_ASSERT(_longPathPlanner->PreloadObstacles(), "Lattice planner didn't preload obstacles.");
    if( !_longPathPlanner->CheckIsPathSafe(newPath, startPoseAngle_rad) ) {
      // bad path. try with the fallback planner if possible

      if( ReplanWithFallbackPlanner() ) {
        PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Collisions",
                      "Planner returned a path with obstacles, using fallback planner instead");
        return;
      }
      else {
        AbortAndSetFailure();
        PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.Collisions.ReplanFail",
                      "Planner returned a path with obstacles, can't get valid fallback plan, failing");
        return;
      }
    }
  }

  // if we get here, then the plan is safe

  Util::sInfo("robot.plan_complete",
              {{DDATA, std::to_string(newPath.GetNumSegments()).c_str()}},
              _selectedPathPlanner->GetName().c_str());

  if( newPath.GetNumSegments()==0 ) {
    if( _driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath ||
        _driveToPoseStatus == ERobotDriveToPoseStatus::WaitingToBeginPath ) {
      // we must have replanned and discovered that we are at the goal, so stop following the path
      PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.PlanComplete.EmptyPlan",
                    "Planner completed with empty plan while we were already following a plan. clearing plan");
      ClearPath();
    }
    else {
      PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteWithPlan.EmptyPlan",
                    "Planner completed but with an empty plan");
    }

    // we are already at the goal, nothing else to do
    OnPathComplete();
  } else {
    PRINT_CH_INFO("Planner", "PathComponent.Update.Planner.CompleteWithPlan",
                  "Running planner complete with a plan");

    Result res = ExecutePath(newPath);

    if( res != RESULT_OK ) {
      PRINT_NAMED_WARNING("Robot.PathComponent.UnableToExecuteCompletedPath",
                          "Planner completed a path, but couldn't execute it for some reason");
      SetDriveToPoseStatus(ERobotDriveToPoseStatus::Failed);
    }
    else {
      DEV_ASSERT(_driveToPoseStatus != ERobotDriveToPoseStatus::ComputingPath,
                 "Robot.PathComponent.HandleCompletePath.StillComputingBug");
    }

    if( _plannerSelectedPoseIndex ) {
      // When someone called StartDrivingToPose with multiple possible poses, they had an option to pass
      // in a pointer to be set when we know which pose was selected by the planner. If they did, set it now
      *_plannerSelectedPoseIndex = selectedPoseIdx;
    }
  }
}

void PathComponent::SelectPlannerHelper(const Pose3d& targetPose)
{
  Pose2d target2d(targetPose);
  Pose2d start2d(_robot->GetPose().GetWithRespectToRoot());

  float distSquared = pow(target2d.GetX() - start2d.GetX(), 2) + pow(target2d.GetY() - start2d.GetY(), 2);

  if(distSquared < std::pow(kMaxDistanceForShortPlanner_mm, 2)) {

    Radians finalAngleDelta = targetPose.GetRotationAngle<'Z'>() - _robot->GetDriveCenterPose().GetRotationAngle<'Z'>();
    const bool withinFinalAngleTolerance = finalAngleDelta.getAbsoluteVal().ToFloat() <=
      2 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

    Radians initialTurnAngle = atan2( target2d.GetY() - _robot->GetDriveCenterPose().GetTranslation().y(),
                                      target2d.GetX() - _robot->GetDriveCenterPose().GetTranslation().x()) -
      _robot->GetDriveCenterPose().GetRotationAngle<'Z'>();

    const bool initialTurnAngleLarge = initialTurnAngle.getAbsoluteVal().ToFloat() >
      0.5 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

    const bool farEnoughAwayForMinAngle = distSquared > std::pow( kMinDistanceForMinAnglePlanner_mm, 2);

    // if we would need to turn fairly far, but our current angle is fairly close to the goal, use the
    // planner which backs up first to minimize the turn
    if( withinFinalAngleTolerance && initialTurnAngleLarge && farEnoughAwayForMinAngle ) {
      PRINT_CH_INFO("Planner", "PathComponent.SelectPlanner.ShortMinAngle",
                    "distance^2 is %f, angleDelta is %f, initialTurnAngle is %f, selecting short min_angle planner '%s'",
                    distSquared,
                    finalAngleDelta.getAbsoluteVal().ToFloat(),
                    initialTurnAngle.getAbsoluteVal().ToFloat(),
                    _shortMinAnglePathPlanner->GetName().c_str());
      _selectedPathPlanner = _shortMinAnglePathPlanner;
    }
    else {
      PRINT_CH_INFO("Planner", "PathComponent.SelectPlanner.Short",
                    "distance^2 is %f, angleDelta is %f, initialTurnAngle is %f, selecting short planner '%s'",
                    distSquared,
                    finalAngleDelta.getAbsoluteVal().ToFloat(),
                    initialTurnAngle.getAbsoluteVal().ToFloat(),
                    _shortPathPlanner->GetName().c_str());
      _selectedPathPlanner = _shortPathPlanner;
    }
  }
  else {
    PRINT_CH_INFO("Planner", "PathComponent.SelectPlanner.Long",
                  "distance^2 is %f, selecting long planner '%s'", distSquared, _longPathPlanner->GetName().c_str());
    _selectedPathPlanner = _longPathPlanner;
  }

  Util::sInfo("robot.planner_selected",
              {{ DDATA, std::to_string(Util::numeric_cast<int>(std::round(distSquared))).c_str() }},
              _selectedPathPlanner->GetName().c_str());


  if( _selectedPathPlanner != _longPathPlanner ) {
    _fallbackPathPlanner = _longPathPlanner;
  } else {
    _fallbackPathPlanner.reset();
  }
}

void PathComponent::SelectPlanner()
{
  if( ! _currPlanParams->targetPoses.empty() ) {
    // for now, just grab the closest pose and use that to select the planner. Note that this pose might be
    // different from the target pose the selected planner actually plans to (because planning distance is not
    // the same as euclidean distance)
    Planning::GoalID closest = IPathPlanner::ComputeClosestGoalPose(_robot->GetDriveCenterPose(),
                                                                    _currPlanParams->targetPoses);
    SelectPlannerHelper(_currPlanParams->targetPoses[closest]);
  }
  else {
    PRINT_NAMED_WARNING("PathComponent.SelectPlanner.NoTargets", "can't select a planner with no target poses");
    _selectedPathPlanner.reset();
  }
}

void PathComponent::SetCustomMotionProfile(const PathMotionProfile& motionProfile)
{
  if( HasCustomMotionProfile() ) {
    PRINT_NAMED_WARNING("PathComponent.SetMotionProfile.Conflict",
                        "Trying to set custom motion profile, but one is already set! Overriding");
  }

  // warning for is custom
  if( !motionProfile.isCustom ) {
    PRINT_NAMED_WARNING("PathComponent.SetCustomMotionProfile.NotCustom",
                        "Motion profile passed in didn't have it's isCustom flag set. This may cause inconsistencies");
  }

  *_pathMotionProfile = motionProfile;
  _hasCustomMotionProfile = true;
}

void PathComponent::SetCustomMotionProfileForAction(const PathMotionProfile& motionProfile, IActionRunner* action)
{
  SetCustomMotionProfile(motionProfile);

  DEV_ASSERT(action != nullptr, "PathComponent.SetCustomMotionProfileForAction.NullAction");
  action->ClearMotionProfileOnCompletion();
}

void PathComponent::ClearCustomMotionProfile()
{
  _hasCustomMotionProfile = false;
  // the actual motion profile will get updated in StartDrivingToPose
}

bool PathComponent::HasCustomMotionProfile() const
{
  return _hasCustomMotionProfile;
}

PathMotionProfile PathComponent::GetCustomMotionProfile() const
{
  ANKI_VERIFY(_hasCustomMotionProfile, "PathComponent.GetCustomMotionProfile.NoCustomProfile",
              "Trying to get the custom profile, but none is set. This is a bug");
  return ClampToCliffSafeSpeed(*_pathMotionProfile);
}

Result PathComponent::ConfigureAndStartPlanner(const std::vector<Pose3d>& poses,
                                               std::shared_ptr<Planning::GoalID> selectedPoseIndexPtr)
{
  if( poses.empty() ) {
    PRINT_NAMED_WARNING("PathComponent.ConfigureAndStartPlanner.NoTargetPoses",
                        "Can't start driving with no provided targets");
    return RESULT_FAIL;
  }

  PlanParameters newPlanParams(_robot, poses);

  if( _plannerActive ) {
    if ( !_currPlanParams->IsEqual(newPlanParams) ) {
      _selectedPathPlanner->StopPlanning();
      _plannerActive = false;
    } else {
      // already started the planner, and will now start following the path on update cycle after plan completes
      return RESULT_OK;
    }
  }

  if( IsActive() ) {
    // we may be following the same path, so only check the goals and originID for equality
    newPlanParams.driveCenter = _currPlanParams->driveCenter;
    if ( !_currPlanParams->IsEqual(newPlanParams) ) {
      // stop doing what we are doing, so we can make a new plan
      PRINT_CH_INFO("Planner", "PathComponent.ConfigureAndStartPlanner.AlreadyBusy",
                    "Path component status was '%s'. Aborting current plan",
                    ERobotDriveToPoseStatusToString(_driveToPoseStatus));
      Abort();
    } else {
      // it's already executing this plan.
      if( !_plannerActive && IsPlanReady() && _startFollowingPath ) {
        TryCompletingPath();
      }
      return RESULT_OK;
    }
  }
  
  _waitingToMatchReplanOrigin = false;
  _hasStoppedBeforeExecuting = false;

  _plannerSelectedPoseIndex = selectedPoseIndexPtr;
  _currPlanParams = std::make_unique<PlanParameters>(newPlanParams);

  SelectPlanner();

  const bool somePlannerSucceeded = StartPlanner(_currPlanParams->driveCenter);
  if( !somePlannerSucceeded ) {
    SetDriveToPoseStatus(ERobotDriveToPoseStatus::Failed);
    return RESULT_FAIL;
  }
  else {
    // otherwise we are computing. Note that some planners finish immediately, in which case the status will
    // be updated on the next Update tick
    SetDriveToPoseStatus(ERobotDriveToPoseStatus::ComputingPath);
  }

  if( !_hasCustomMotionProfile ) {
    *_pathMotionProfile = _speedChooser->GetPathMotionProfile(poses);
  }

  return RESULT_OK;
}

Result PathComponent::PrecomputePath(const std::vector<Pose3d>& poses,
                                     std::shared_ptr<Planning::GoalID> selectedPoseIndexPtr)
{
  _startFollowingPath = false;
  return ConfigureAndStartPlanner(poses, selectedPoseIndexPtr);
}


Result PathComponent::StartDrivingToPose(const std::vector<Pose3d>& poses,
                                         std::shared_ptr<Planning::GoalID> selectedPoseIndexPtr)
{
  _startFollowingPath = true;
  return ConfigureAndStartPlanner(poses, selectedPoseIndexPtr);
}

bool PathComponent::StartPlanner(const Pose3d& driveCenterPose)
{
  if( !_selectedPathPlanner ) {
    PRINT_NAMED_ERROR("PathComponent.StartPlanner.NoSelectedPlanner", "Must select planner before starting");
    return false;
  }

  EComputePathStatus status = _selectedPathPlanner->ComputePath(driveCenterPose, _currPlanParams->targetPoses);
  if( status == EComputePathStatus::Error ) {
    return ReplanWithFallbackPlanner();
  }
  else {
    _plannerActive = true;
    return true;
  }
}

bool PathComponent::StartPlanner()
{
  const Pose3d& driveCenterPose(_robot->GetDriveCenterPose());
  return StartPlanner(driveCenterPose);
}

bool PathComponent::ReplanWithFallbackPlanner()
{
  if( ! _fallbackPathPlanner ) {
    return false;
  }

  const std::string& oldPlannerName = _selectedPathPlanner ? _selectedPathPlanner->GetName() : "NULL";

  // switch to fallback, and clear fallback
  _selectedPathPlanner = _fallbackPathPlanner;
  _fallbackPathPlanner.reset();

  if( StartPlanner() ) {
    Util::sInfo("robot.fallback_planner_used",
                {{DDATA, _selectedPathPlanner->GetName().c_str()}},
                oldPlannerName.c_str());
    return true;
  }
  else {
    return false;
  }
}

void PathComponent::RestartPlannerIfNeeded()
{
  DEV_ASSERT( _selectedPathPlanner, "PathComponent.NoSelectedPlanner" );
  if( ! _selectedPathPlanner ) {
    // can't replan if we don't have a selected planner
    return;
  }
  
  const bool tooFar __attribute((unused)) = (_selectedPathPlanner->GetErrorType() == EPlannerErrorType::TooFarFromPlan);
  DEV_ASSERT((_driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath)
             || ((_driveToPoseStatus == ERobotDriveToPoseStatus::ComputingPath) && tooFar),
             "PathComponent.RestartPlannerIfNeeded.WrongState");

  // if we are already planning, don't cut that off
  if( _plannerActive ) {
    return;
  }

  const bool forceReplan = false;
  switch( _selectedPathPlanner->ComputeNewPathIfNeeded( _robot->GetDriveCenterPose(), forceReplan, _replanningCanChangeGoal ) ) {

    case EComputePathStatus::Error:
    {
      if( ReplanWithFallbackPlanner() ) {
        PRINT_CH_INFO("Planner", "PathComponent.RestartIfNeeded.Error.Fallback",
                      "computing a new path resulted in an error, switching to fallback");
      }
      else {
        PRINT_CH_INFO("Planner", "PathComponent.RestartIfNeeded.Error.NoFallback",
                      "computing a new path resulted in an error but couldn't use fallback. Failing");
        // abort in case we are currently following an invalid path
        AbortAndSetFailure();
      }
      _isReplanning = false;
    }
      break;

    case EComputePathStatus::Running:
    {
      PRINT_CH_DEBUG("Planner", "PathComponent.Replan.Running", "ComputeNewPathIfNeeded running");
      const Planning::Path currPath = _pdo->GetPath();
      if (currPath.GetNumSegments() > 0)
      {
        const float startAngle = currPath.GetSegmentConstRef(0).GetStartAngle();
        Planning::Path validSubPath;
        if (!_selectedPathPlanner->CheckIsPathSafe(currPath, startAngle, validSubPath))
        {
          _isReplanning = true;
          // entire path is not safe, set the current path to just the valid portion
          if (_currPathSegment >= validSubPath.GetNumSegments())
          {
            // the robot already drove the extent of the valid path, so force stop
            PRINT_NAMED_INFO("PathComponent.RestartPlannerIfNeeded", "Replanning and current Path invalid. ESTOP Robot");
            ClearPath();
            _hasStoppedBeforeExecuting = true;
          } else {
            if( _pdo->GetLastDoledIdx() >= validSubPath.GetNumSegments() ) {
              // we already sent the extent of the valid path, so trim it before it's executed
              TrimRobotPathToLength( validSubPath.GetNumSegments() );
            }
            _pdo->ReplacePath( validSubPath );
            // redraw the path
            _robot->GetContext()->GetVizManager()->DrawPath(_robot->GetID(), _pdo->GetPath(), NamedColors::EXECUTED_PATH);
          }
        }
      }
      _plannerActive = true;
      break;
    }
    case EComputePathStatus::NoPlanNeeded:
    {
      DEV_ASSERT( _driveToPoseStatus == ERobotDriveToPoseStatus::FollowingPath,
                  "Robot.PathComponent.RestarTPlannerIfNeeded.NoPlan.InconsistentState" );
      // leave state as following
    }
      break;
  }
}

// Clears the path that the robot is executing which also stops the robot
Result PathComponent::ClearPath()
{
  if( _lastSentPathID != 0 ) {
    _lastCanceledPathID = _lastSentPathID;
  }

  _robot->GetContext()->GetVizManager()->ErasePath(_robot->GetID());
  if(_pdo) {
    _pdo->ClearPath();
  }

  _currPathSegment = -1;

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastMsgSendTime_s = currTime_s;

  return _robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::ClearPath(0)));
}
  
Result PathComponent::TrimRobotPathToLength( uint8_t length )
{
  Result result = RESULT_OK;
  if( _pdo != nullptr ) {
    const uint8_t numSegments = _pdo->GetPath().GetNumSegments();
    if( numSegments > length ) {
      const uint8_t numPopBack = numSegments - length;
      _lastMsgSendTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      result = _robot->SendMessage( RobotInterface::EngineToRobot( RobotInterface::TrimPath( 0, numPopBack ) ) );
    }
  }
  
  return result;
}

bool PathComponent::IsPlanReady() const
{
  bool ready = false;
  if( _selectedPathPlanner != nullptr ) {
    ready = (_selectedPathPlanner->CheckPlanningStatus() == EPlannerStatus::CompleteWithPlan);
  }
  return ready;
}

bool PathComponent::IsActive() const
{
  switch(_driveToPoseStatus) {
    case ERobotDriveToPoseStatus::Failed:                           return false;
    case ERobotDriveToPoseStatus::ComputingPath:                    return true;
    case ERobotDriveToPoseStatus::WaitingToBeginPath:               return true;
    case ERobotDriveToPoseStatus::FollowingPath:                    return true;
    case ERobotDriveToPoseStatus::Ready:                            return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPath:              return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure: return false;
  }
}

bool PathComponent::HasPathToFollow() const
{
  switch(_driveToPoseStatus) {
    case ERobotDriveToPoseStatus::Failed:                           return false;
    case ERobotDriveToPoseStatus::ComputingPath:                    return false;
    case ERobotDriveToPoseStatus::WaitingToBeginPath:               return true;
    case ERobotDriveToPoseStatus::FollowingPath:                    return true;
    case ERobotDriveToPoseStatus::Ready:                            return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPath:              return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure: return false;
  }
}

bool PathComponent::LastPathFailed() const
{
  switch(_driveToPoseStatus) {
    case ERobotDriveToPoseStatus::Failed:                           return true;
    case ERobotDriveToPoseStatus::ComputingPath:                    return false;
    case ERobotDriveToPoseStatus::WaitingToBeginPath:               return false;
    case ERobotDriveToPoseStatus::FollowingPath:                    return false;
    case ERobotDriveToPoseStatus::Ready:                            return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPath:              return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure: return true;
  }
}

bool PathComponent::IsWaitingToCancelPath() const
{
  switch(_driveToPoseStatus) {
    case ERobotDriveToPoseStatus::Failed:                           return false;
    case ERobotDriveToPoseStatus::ComputingPath:                    return false;
    case ERobotDriveToPoseStatus::WaitingToBeginPath:               return false;
    case ERobotDriveToPoseStatus::FollowingPath:                    return false;
    case ERobotDriveToPoseStatus::Ready:                            return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPath:              return true;
    case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure: return true;
  }
}

bool PathComponent::IsWaitingForRobotResponse() const
{
  switch(_driveToPoseStatus) {
    case ERobotDriveToPoseStatus::Failed:                           return false;
    case ERobotDriveToPoseStatus::ComputingPath:                    return false;
    case ERobotDriveToPoseStatus::WaitingToBeginPath:               return true;
    case ERobotDriveToPoseStatus::FollowingPath:                    return false;
    case ERobotDriveToPoseStatus::Ready:                            return false;
    case ERobotDriveToPoseStatus::WaitingToCancelPath:              return true;
    case ERobotDriveToPoseStatus::WaitingToCancelPathAndSetFailure: return true;
  }
}

Result PathComponent::ExecuteCustomPath(const Planning::Path& path)
{

  // clear the selected planner, so we don't replan along this manual path
  if(_plannerActive) {
    _selectedPathPlanner->StopPlanning();
  }
  // reset state
  _plannerActive = false;
  _isReplanning = false;
  _waitingToMatchReplanOrigin = false;
  _hasStoppedBeforeExecuting = false;
  _selectedPathPlanner.reset();

  return ExecutePath(path);
}

bool PathComponent::IsPathSafe(const Planning::Path& path, const Pose3d* driveCenter) const
{
  if (driveCenter == nullptr) {
    driveCenter = &_robot->GetDriveCenterPose();
  }

  // currently just check the long path planner, since it is the only one that supports collision checking at the moment
  if (_longPathPlanner) {
    return _longPathPlanner->CheckIsPathSafe(path, driveCenter->GetRotationAngle<'Z'>().ToFloat());
  } else {
    PRINT_NAMED_WARNING("PathComponent.IsPathSafe", "No planner exists that checks collisions, so all paths are safe");
    return true;
  }
}

Result PathComponent::ExecutePath(const Planning::Path& path)
{
  Result lastResult = RESULT_FAIL;

  _hasStoppedBeforeExecuting = false;
  
  if (path.GetNumSegments() == 0) {
    PRINT_NAMED_WARNING("PathComponent.ExecutePath.EmptyPath", "");
    lastResult = RESULT_OK;
    OnPathComplete();
  } else {

    // TODO: Clear currently executing path or write to buffered path?
    lastResult = ClearPath();
    if(lastResult == RESULT_OK) {
      ++_lastSentPathID;
      if( _pdo ) {
        _pdo->SetPath(path);
      }

      PRINT_CH_INFO("Planner", "PathComponent.SendExecutePath",
                    "sending start execution message (pathID = %d)",
                    _lastSentPathID);
      lastResult = _robot->SendMessage(RobotInterface::EngineToRobot(
                                        RobotInterface::ExecutePath(_lastSentPathID)));

      if( lastResult == RESULT_OK) {
        const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _lastMsgSendTime_s = currTime_s;
        // new path was successfully sent, wait for it to start
        SetDriveToPoseStatus(ERobotDriveToPoseStatus::WaitingToBeginPath);
      }
    }

    // Visualize path if robot has just started traversing it.
    _robot->GetContext()->GetVizManager()->DrawPath(_robot->GetID(), path, NamedColors::EXECUTED_PATH);
  }

  return lastResult;
}

void PathComponent::ExecuteTestPath(const PathMotionProfile& motionProfile)
{
  // NOTE: no need to use the custom motion profile here, we just manually pass it in to the test path
  Planning::Path p;
  _longPathPlanner->GetTestPath(_robot->GetPose(), p, &motionProfile);
  ExecutePath(p);
}

void PathComponent::SetDriveToPoseStatus(ERobotDriveToPoseStatus newValue)
{
  if( newValue != _driveToPoseStatus ) {
    PRINT_CH_INFO("Planner", "PathComponent.TransitionStatus",
                  "%s -> %s",
                  ERobotDriveToPoseStatusToString(_driveToPoseStatus),
                  ERobotDriveToPoseStatusToString(newValue));

    _driveToPoseStatus = newValue;
  }
}
  
void PathComponent::SetStartPath(bool autoStart)
{
  const bool needsRestarting = (autoStart && !_startFollowingPath && IsPlanReady() && HasPathToFollow());
  _startFollowingPath = autoStart;
  if( needsRestarting ) {
    TryCompletingPath();
  }
}

PathMotionProfile PathComponent::ClampToCliffSafeSpeed(const PathMotionProfile& motionProfile) const
{
  PathMotionProfile cliffSafeMotionProfile = motionProfile;
  const bool isCarrying = _robot->GetCarryingComponent().IsCarryingObject(); 
  const float maxSpeed = isCarrying ? 
                         MAX_SAFE_WHILE_CARRYING_WHEEL_SPEED_MMPS : 
                         MAX_SAFE_WHEEL_SPEED_MMPS;

  auto ClampSpeedFunc = [maxSpeed, isCarrying](float orig_speed) {
    if (Util::InRange(orig_speed, -maxSpeed, maxSpeed)) {
      return orig_speed;
    } else if (!isCarrying) {
      // Only print when not carrying since carrying is expected
      // to further clamp valid speed limits
      PRINT_NAMED_WARNING("PathComponent.ClampToCliffSafeSpeed.ClampingUnsafeSpeed", 
                          "%f mm/s will be clamped to %f mm/s", 
                          orig_speed, maxSpeed);
    }
    return CLIP(orig_speed, -maxSpeed, maxSpeed);
  };

  cliffSafeMotionProfile.speed_mmps        = ClampSpeedFunc(motionProfile.speed_mmps);
  cliffSafeMotionProfile.reverseSpeed_mmps = ClampSpeedFunc(motionProfile.reverseSpeed_mmps);
  cliffSafeMotionProfile.dockSpeed_mmps    = ClampSpeedFunc(motionProfile.dockSpeed_mmps);

  return cliffSafeMotionProfile;
}

}
}
