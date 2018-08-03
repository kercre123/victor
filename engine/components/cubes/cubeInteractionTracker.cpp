/**
 * File: cubeInteractionTracker.cpp
 *
 * Author: Sam Russell
 * Created: 2018-7-17
 *
 * Description: Robot component which monitors cube data and attempts to discern when a user is physically
 *              interacting with a cube.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

// NOTES:
// 1. This is currently only capable of handling a single cube

#include "engine/components/cubes/cubeInteractionTracker.h"

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageEngineToGameTag.h"

#define DEBUG_INTERACTION_TRACKER 0

#define SET_STATE(s) do { \
                          _trackerState = ECITState::s; \
                          PRINT_NAMED_INFO("CubeInteractionTracker.SetState", \
                                           "Tracker state set to %s", \
                                           #s); \
                        } while(0);

namespace Anki{
namespace Cozmo{

namespace{
// Internal Tunable params 
static const bool  kUseProxForDistance = true;

static const float kRecentlyVisibleLimit_s     = 1.0f;
static const float kRecentlyMovedLimit_s       = 2.0f;
static const float kTapSubscribeTimeout_s      = 5.0f;
static const u32   kTargetValidSeenTimeout_ms  = 10000; // Don't consider targets seen more than 10s ago.

static const float kTargetUnmovedDist_mm       = 4.0f;
static const float kTargetUnmovedAngle_deg     = 5.0f;

// - - - - - Filter params - - - - - 
static constexpr float kHeldProbabilityThreshold    = 0.75f;
static constexpr float kNotHeldProbabilityThreshold = 0.25f;
// -- Idle --
static constexpr float kIdleToTrackingConfidence = 0.4f;
static constexpr float kFramesToHeld_idle      = 1.5; // Contiguous frames WITH detected movement to flip to HELD
static constexpr float kFramesToNotHeld_idle   = 30; // Contiguous frames WITHOUT detected movement to flip to NOT HELD
// -- Visual Tracking --
static constexpr float kTrackingToIdleConfidence   = 0.33f;
static constexpr float kFramesToHeld_tracking      = 1.5;
static constexpr float kFramesToNotHeld_tracking   = 30;
// -- Connected Tracking --
static constexpr float kConnectedHighRateConfidence = 0.5f;
static constexpr float kConnectedLowRateConfidence  = 0.25f;
static constexpr float kFramesToHeld_connected      = 3;
static constexpr float kFramesToNotHeld_connected   = 9;
// -- Computed Params --
static constexpr float kFilterIncrement_idle = kHeldProbabilityThreshold / kFramesToHeld_idle;
static constexpr float kFilterDecrement_idle = (1.0f - kNotHeldProbabilityThreshold) / kFramesToNotHeld_idle;
static constexpr float kFilterIncrement_tracking = kHeldProbabilityThreshold / kFramesToHeld_tracking;
static constexpr float kFilterDecrement_tracking = (1.0f - kNotHeldProbabilityThreshold) / kFramesToNotHeld_tracking;
static constexpr float kFilterIncrement_connected = kHeldProbabilityThreshold / kFramesToHeld_connected;
static constexpr float kFilterDecrement_connected = (1.0f - kNotHeldProbabilityThreshold) / kFramesToNotHeld_connected;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TargetStatus::TargetStatus()
: object(nullptr)
, pose(Pose3d())
, distance_mm(0.0f)
, angleFromRobotFwd_deg(0.0f)
, lastObservedTime_s(0.0f)
, lastMovedTime_s(0.0f)
, probabilityIsHeld(0.0f)
, isValid(false)
, visibleThisFrame(false)
, visibleRecently(false)
, movedThisFrame(false)
, movedRecently(false)
, isHeld(false)
, tappedDuringLastTick(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeInteractionTracker::CubeInteractionTracker()
: IDependencyManagedComponent(this, RobotComponentID::CubeInteractionTracker)
, _trackerState(ECITState::Idle)
, _targetStatus(TargetStatus())
, _currentTimeThisTick_s(0.0f)
, _tapDetectedDuringTick(false)
, _connectedTrackAtHighRate(false)
, _filterIncrement(kFilterIncrement_idle)
, _filterDecrement(kFilterDecrement_idle)
{
  _targetFilter = std::make_unique<BlockWorldFilter>();
  _targetFilter->AddAllowedFamily(ObjectFamily::LightCube);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::GetInitDependencies(RobotCompIDSet& dependencies) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  if(ANKI_VERIFY(_robot->HasExternalInterface(),
                 "CubeInteractionTracker.InitDependent.NoExternalInterface", "")){
    auto callback = std::bind(&CubeInteractionTracker::HandleEngineEvents, this, std::placeholders::_1);
    auto* extInterface = _robot->GetExternalInterface();
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageEngineToGameTag::ObjectTapped, callback));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::GetUpdateDependencies(RobotCompIDSet& dependencies) const 
{
  dependencies.insert(RobotComponentID::BlockWorld);
  dependencies.insert(RobotComponentID::Carrying);
  dependencies.insert(RobotComponentID::CubeConnectionCoordinator);
  dependencies.insert(RobotComponentID::CubeLights);
  dependencies.insert(RobotComponentID::FullRobotPose);
  dependencies.insert(RobotComponentID::ProxSensor);
  dependencies.insert(RobotComponentID::VisionScheduleMediator);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateDependent(const RobotCompMap& dependentComps)
{
  _currentTimeThisTick_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  auto& ccc = dependentComps.GetComponent<CubeConnectionCoordinator>();
  UpdateIsHeldEstimate(dependentComps);
  switch(_trackerState){
    case ECITState::Idle:
    {
      if(ccc.IsConnectedToCube()){
        TransitionToTrackingConnected(dependentComps);
      } 
      else if(_targetStatus.probabilityIsHeld > kIdleToTrackingConfidence){
        TransitionToTrackingUnconnected(dependentComps);
      }
      break;
    }
    case ECITState::TrackingConnected:
    {
      if(!ccc.IsConnectedToCube()){
        TransitionToIdle(dependentComps);
        break;
      } 

      // Within connected tracking, DetectMarkers at high rate as we approach thinking the user is holding the cube
      if(_connectedTrackAtHighRate && (_targetStatus.probabilityIsHeld < kConnectedLowRateConfidence)){
        _connectedTrackAtHighRate = false;
        auto& vsm = dependentComps.GetComponent<VisionScheduleMediator>();
        vsm.SetVisionModeSubscriptions(this, {{VisionMode::DetectingMarkers, EVisionUpdateFrequency::High}});
      }
      else if(!_connectedTrackAtHighRate && (_targetStatus.probabilityIsHeld > kConnectedHighRateConfidence)){
        _connectedTrackAtHighRate = true;
        auto& vsm = dependentComps.GetComponent<VisionScheduleMediator>();
        vsm.ReleaseAllVisionModeSubscriptions(this);
      }

      if(ccc.IsConnectedInteractable() && !_targetStatus.isHeld ){
        if(_tapDetectedDuringTick){
          // Subscribe to the connection first so the CCC can work out state properly in case we were 
          // in the process of disconnecting
          ccc.SubscribeToCubeConnection(this, false, kTapSubscribeTimeout_s);
          auto& clc = dependentComps.GetComponent<CubeLightComponent>();
          clc.PlayTapResponseLights(ccc.GetConnectedActiveObject()->GetID());
        }
      }
      break;
    }
    case ECITState::TrackingUnconnected:
    {
      if(ccc.IsConnectedToCube()){
        TransitionToTrackingConnected(dependentComps);
      } 
      else {
        if(_targetStatus.probabilityIsHeld < kTrackingToIdleConfidence){
          TransitionToIdle(dependentComps);
        }
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("CubeInteractionTracker.InvalidState", "");
      break;
    }
  }
  
  // Clear this var in case we don't want to be handling taps right now, so we don't "handle" them later...
  _targetStatus.tappedDuringLastTick = _tapDetectedDuringTick;
  _tapDetectedDuringTick = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Public API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CubeInteractionTracker::IsUserHoldingCube() const
{
#if DEBUG_INTERACTION_TRACKER
  PRINT_NAMED_INFO("CubeInteractionTracker.IsHeldRequested",
                   "Is held state requested, responed isHeld: %s",
                   _targetStatus.isHeld ? "true" : "false");
#endif

  return _targetStatus.isHeld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Members
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateIsHeldEstimate(const RobotCompMap& dependentComps)
{
   // Decide whether or not we think the cube is held on this frame
  auto& ccc = dependentComps.GetComponent<CubeConnectionCoordinator>();
  auto& carryingComp = dependentComps.GetComponent<CarryingComponent>();

  UpdateTargetStatus(dependentComps);
  bool suspectHeldThisFrame = false;

  // Try to ignore robot induced motion
  if(!carryingComp.IsCarryingObject()){
    suspectHeldThisFrame = _targetStatus.movedThisFrame;
  }

  // Construct a bi-stable, filtered, multi-frame estimate of the probability that the cube is currently being held
  float probHeldDelta = suspectHeldThisFrame ? _filterIncrement : -_filterDecrement;
  _targetStatus.probabilityIsHeld = Anki::Util::Clamp(_targetStatus.probabilityIsHeld + probHeldDelta, 0.0f, 1.0f);

  if( !_targetStatus.isHeld && (_targetStatus.probabilityIsHeld > kHeldProbabilityThreshold) ) {
    PRINT_NAMED_INFO("CubeInteractionTracker.UserPickedUpCube",
                      "The model indicates that the user is now holding a cube");
    _targetStatus.isHeld = true;
    // If the user is holding a connected cube, maintain the connection state until they put it down
    if(ccc.IsConnectedInteractable()){
      ccc.SubscribeToCubeConnection(this);
    } 
    else if(ccc.IsConnectedToCube()) {
      bool connectInBackground = true;
      ccc.SubscribeToCubeConnection(this, connectInBackground);
    }
  } 
  else if( _targetStatus.isHeld && (_targetStatus.probabilityIsHeld < kNotHeldProbabilityThreshold) ){
    PRINT_NAMED_INFO("CubeInteractionTracker.UserPutDownCube",
                      "The model indicates that the user is no longer holding a cube");
    _targetStatus.isHeld = false;
    // If we've been holding a connection open, release it
    if(ccc.IsConnectedToCube()){
      ccc.UnsubscribeFromCubeConnection(this);
    }
  } 

#if DEBUG_INTERACTION_TRACKER
  if(suspectHeldThisFrame){
    PRINT_NAMED_INFO("CubeInteractionTracker.CubeHeldFrame",
                    "Cube held this frame. Overall probability estimate: %f",
                    _targetStatus.probabilityIsHeld);
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::TransitionToIdle(const RobotCompMap& dependentComps)
{
  SET_STATE(Idle);

  // Configure filter
  _filterIncrement = kFilterIncrement_idle;
  _filterDecrement = kFilterDecrement_idle;

  auto& vsm = dependentComps.GetComponent<VisionScheduleMediator>();
  vsm.ReleaseAllVisionModeSubscriptions(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::TransitionToTrackingConnected(const RobotCompMap& dependentComps)
{
  SET_STATE(TrackingConnected);

  // Configure filter
  _filterIncrement = kFilterIncrement_connected;
  _filterDecrement = kFilterDecrement_connected;
  
  auto& vsm = dependentComps.GetComponent<VisionScheduleMediator>();
  vsm.ReleaseAllVisionModeSubscriptions(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::TransitionToTrackingUnconnected(const RobotCompMap& dependentComps)
{
  SET_STATE(TrackingUnconnected);

  // Configure filter
  _filterIncrement = kFilterIncrement_tracking;
  _filterDecrement = kFilterDecrement_tracking;

  auto& vsm = dependentComps.GetComponent<VisionScheduleMediator>();
  vsm.SetVisionModeSubscriptions(this, {{VisionMode::DetectingMarkers, EVisionUpdateFrequency::High}});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Target Tracking Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetStatus(const RobotCompMap& dependentComps)
{
  // Don't update anything if we don't have a valid target
  if(SelectTarget(dependentComps)){
    UpdateTargetVisibility(dependentComps);
    UpdateTargetMotion(dependentComps);
    UpdateTargetAngleFromRobotFwd(dependentComps);
    UpdateTargetDistance(dependentComps);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CubeInteractionTracker::SelectTarget(const RobotCompMap& dependentComps)
{
  auto& blockWorld = dependentComps.GetComponent<BlockWorld>();
  auto& ccc = dependentComps.GetComponent<CubeConnectionCoordinator>();

  const ObservableObject* targetObject = nullptr;

  // First check if we're connected, we should always use our connected cube
  const ActiveObject* activeObject = ccc.GetConnectedActiveObject();
  if(nullptr != activeObject){
    targetObject = blockWorld.GetLocatedObjectByID(activeObject->GetID());
  }

  // If we're not connected...
  if(nullptr == targetObject){
    // ...see whether we have a recently observed cube
    // NOTE: This will not work well with more than one cube in view
    targetObject = blockWorld.FindMostRecentlyObservedObject(*_targetFilter);
    if(nullptr != targetObject){
      // If the last seen time is too old, don't use it
      const RobotTimeStamp_t timeSinceSeen_ms = _robot->GetLastImageTimeStamp() - targetObject->GetLastObservedTime();
      if(timeSinceSeen_ms > kTargetValidSeenTimeout_ms){
        // We haven't seen a target in too long. Consider it invalid
        targetObject = nullptr;
      }
    }
  }

  if(nullptr == targetObject){
    // We didn't get a target from either connection or BlockWorld. 
    // Reset the target status if we haven't already
    if(_targetStatus.isValid){
      _targetStatus = TargetStatus();
    }
    return false;
  } 

  // If we made it all the way here and...
  if(nullptr == _targetStatus.object){
    // ...we just found a target for the first time (in a while)
    _targetStatus.object = targetObject;
    _targetStatus.isValid = true;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetVisibility(const RobotCompMap& dependentcomps)
{
  if(_targetStatus.object->GetLastObservedTime() >= _robot->GetLastImageTimeStamp()){
#if DEBUG_INTERACTION_TRACKER
    PRINT_NAMED_INFO("CubeInteractionTracker.SawTarget","");
#endif
    _targetStatus.lastObservedTime_s = _currentTimeThisTick_s;
    _targetStatus.visibleThisFrame = true;
    _targetStatus.visibleRecently = true;
  } 
  else{
    _targetStatus.visibleThisFrame = false;
    float timeSinceSeen_s = _currentTimeThisTick_s - _targetStatus.lastObservedTime_s;
    _targetStatus.visibleRecently = timeSinceSeen_s < kRecentlyVisibleLimit_s;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetMotion(const RobotCompMap& dependentComps)
{
  auto& ccc = dependentComps.GetComponent<CubeConnectionCoordinator>();
  bool targetHasMoved = false;

  // See if we can check this from accel data as an active object first
  const ActiveObject* targetActiveObject = ccc.GetConnectedActiveObject();
  if(nullptr != targetActiveObject){
    targetHasMoved = targetActiveObject->IsMoving();
  } 
  else if(_targetStatus.visibleThisFrame) {
    // Target is not connected, lets see if we can check for movement by pose
    // If we can't validate that it has moved due to pose parenting changes, assume it hasn't moved
    Radians angleThreshold;
    angleThreshold.setDegrees(kTargetUnmovedAngle_deg);
    Pose3d currentPose = _targetStatus.object->GetPose();
    if(!currentPose.IsSameAs(_targetStatus.pose, kTargetUnmovedDist_mm, angleThreshold)){
      _targetStatus.pose = currentPose;
      targetHasMoved = true;
    }
  }

  if(targetHasMoved){
    _targetStatus.lastMovedTime_s = _currentTimeThisTick_s;
    _targetStatus.movedThisFrame = true;
    _targetStatus.movedRecently = true;
  } 
  else {
    _targetStatus.movedThisFrame = false;
    float timeSinceMoved_s = _currentTimeThisTick_s - _targetStatus.lastMovedTime_s;
    _targetStatus.movedRecently = timeSinceMoved_s < kRecentlyMovedLimit_s;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetAngleFromRobotFwd(const RobotCompMap& dependentComps)
{
  // We can only update the angleToTarget if we saw it this frame
  if(!_targetStatus.visibleThisFrame){
    return;
  }

  // Use the center of the cube for AngleFromRobotFwd computation 
  Pose3d targetWRTRobot;
  _targetStatus.object->GetPose().GetWithRespectTo(_robot->GetPose(), targetWRTRobot);
  Pose2d robotToTargetFlat(targetWRTRobot);
  Radians angleToTarget(atan2f(robotToTargetFlat.GetY(), robotToTargetFlat.GetX()));
  _targetStatus.angleFromRobotFwd_deg = angleToTarget.getDegrees();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetDistance(const RobotCompMap& dependentComps)
{
  if(!_targetStatus.visibleRecently){
    // We can't update from Prox or vision if we haven't at least seen the cube recently
    return;
  }

  bool usingProx = false;
  bool proxReadingValid = false;
  bool targetIsInProxFOV = false;
  if(kUseProxForDistance){
    auto& proxSensor = dependentComps.GetComponent<ProxSensorComponent>();
    proxSensor.IsInFOV(_targetStatus.object->GetPose(), targetIsInProxFOV);
    u16 proxDist_mm = 0;

    // If we can see the cube (or have seen it recently enough) to verify its in prox range, use prox
    proxReadingValid = proxSensor.GetLatestDistance_mm(proxDist_mm);
    if(targetIsInProxFOV && proxReadingValid){
      _targetStatus.distance_mm = proxDist_mm;
      usingProx = true;
    }
  } 
  
  // If not using prox, we must have seen the target this frame to have any useful information
  if(!usingProx && _targetStatus.visibleThisFrame){
    // Otherwise, use ClosestMarker for VisionBased distance checks
    Pose3d closestMarkerPose;
    _targetStatus.object->GetClosestMarkerPose(_robot->GetPose(), true, closestMarkerPose);
    _targetStatus.distance_mm = Point2f(closestMarkerPose.GetTranslation()).Length();
  }

#if DEBUG_INTERACTION_TRACKER
  PRINT_NAMED_INFO("CubeInteractionTracker.UpdateDistance",
                    "Distance measured: %f targetInFOV: %s prox Valid: %s",
                    _targetStatus.distance_mm,
                    (targetIsInProxFOV ? "true" : "false"),
                    (proxReadingValid ? "true" : "false"));
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ^ ^ ^ Target Tracking Methods ^ ^ ^
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::HandleEngineEvents(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
{
  switch(event.GetData().GetTag()){
    case EngineToGameTag::ObjectTapped:
    {
      _tapDetectedDuringTick = true;
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("CubeInteractionTracker.HandleEngineEvents.UnhandledEvent",
                        "Received event message of type %s, which is unhandled in this class",
                        MessageEngineToGameTagToString(event.GetData().GetTag()));
    }
  }
}

} // namespace Cozmo
} // namespace Anki
