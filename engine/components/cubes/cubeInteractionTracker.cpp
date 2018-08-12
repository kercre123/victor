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
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageEngineToGameTag.h"

#include "webServerProcess/src/webVizSender.h"

#define SET_STATE(s) do { \
                          _trackerState = ECITState::s; \
                          _debugStateString = #s; \
                          SendDataToWebViz(); \
                          PRINT_NAMED_INFO("CubeInteractionTracker.SetState", \
                                           "Tracker state set to %s", \
                                           #s); \
                        } while(0);

namespace Anki{
namespace Vector{

namespace{
// Internal Tunable params 
static const bool  kUseProxForDistance = true;

static const float kRecentlyVisibleLimit_s     = 1.0f;
static const float kRecentlyMovedLimit_s       = 2.0f;
static const float kRecentlyMovedFarLimit_s    = 2.0f;
static const float kTapSubscribeTimeout_s      = 5.0f;

static const float kTargetUnmovedDist_mm       = 4.0f;
static const float kTargetUnmovedAngle_deg     = 5.0f;

static const float kTargetMovedFarDist_mm        = 100.0f; // If we see the target move this far, its held.
static const float kTargetMovedFarHeldConfidence = 0.8f;

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

// WebViz
#if ANKI_DEV_CHEATS
const std::string kWebVizModuleNameCubes = "cubes";
const float kWebVizUpdatePeriod_s = 0.5f;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TargetStatus::TargetStatus()
: activeObject(nullptr)
, observableObject(nullptr)
, prevPose(Pose3d())
, distance_mm(0.0f)
, angleFromRobotFwd_deg(0.0f)
, lastObservedTime_s(0.0f)
, lastMovedTime_s(0.0f)
, lastMovedFarTime_s(0.0f)
, lastHeldTime_s(0.0f)
, lastTappedTime_s(0.0f)
, probabilityIsHeld(0.0f)
, distMeasuredWithProx(false)
, visibleThisFrame(false)
, visibleRecently(false)
, observedSinceMovedByRobot(false)
, movedThisFrame(false)
, movedRecently(false)
, movedFarThisFrame(false)
, movedFarRecently(false)
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
, _trackingAtHighRate(false)
, _filterIncrement(kFilterIncrement_idle)
, _filterDecrement(kFilterDecrement_idle)
#if ANKI_DEV_CHEATS
, _timeToUpdateWebViz_s(0.0f)
#endif
, _debugStateString("Idle")
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
      if(_trackingAtHighRate && (_targetStatus.probabilityIsHeld < kConnectedLowRateConfidence)){
        _trackingAtHighRate = false;
        auto& vsm = dependentComps.GetComponent<VisionScheduleMediator>();
        vsm.SetVisionModeSubscriptions(this, {{VisionMode::DetectingMarkers, EVisionUpdateFrequency::High}});
      }
      else if(!_trackingAtHighRate && (_targetStatus.probabilityIsHeld > kConnectedHighRateConfidence)){
        _trackingAtHighRate = true;
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
          _targetStatus.lastTappedTime_s = _currentTimeThisTick_s;
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

  // WebViz
#if ANKI_DEV_CHEATS
  if(_currentTimeThisTick_s >= _timeToUpdateWebViz_s){
    SendDataToWebViz();
    _timeToUpdateWebViz_s = _currentTimeThisTick_s + kWebVizUpdatePeriod_s;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Public API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CubeInteractionTracker::IsUserHoldingCube() const
{
  return _targetStatus.isHeld;
}

ObjectID CubeInteractionTracker::GetTargetID()
{
  if(nullptr != _targetStatus.activeObject){
    return _targetStatus.activeObject->GetID();
  }
  else if(nullptr != _targetStatus.observableObject){
    return _targetStatus.observableObject->GetID();
  }
  return ObjectID();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Members
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateIsHeldEstimate(const RobotCompMap& dependentComps)
{
   // Decide whether or not we think the cube is held on this frame
  auto& ccc = dependentComps.GetComponent<CubeConnectionCoordinator>();
  bool suspectHeldThisFrame = false;

  UpdateTargetStatus(dependentComps);

  // If the cube moved far from where it was last seen, directly modify held probability
  if(_targetStatus.movedFarThisFrame){
    _targetStatus.probabilityIsHeld = Anki::Util::Max(kTargetMovedFarHeldConfidence, _targetStatus.probabilityIsHeld);
  }
  else{
    suspectHeldThisFrame = _targetStatus.movedThisFrame;

    // Update a bi-stable, filtered, multi-frame estimate of the probability that the cube is currently being held
    float probHeldDelta = suspectHeldThisFrame ? _filterIncrement : -_filterDecrement;
    _targetStatus.probabilityIsHeld = Anki::Util::Clamp(_targetStatus.probabilityIsHeld + probHeldDelta, 0.0f, 1.0f);
  }

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
    _targetStatus.lastHeldTime_s = _currentTimeThisTick_s;
    // If we've been holding a connection open, release it
    if(ccc.IsConnectedToCube()){
      ccc.UnsubscribeFromCubeConnection(this);
    }
  } 

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
  _trackingAtHighRate = false;
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
  _trackingAtHighRate = false;
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
  _trackingAtHighRate = true;
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

  // First check if we're connected, we should always use our connected cube
  if(ccc.IsConnectedToCube()){
    _targetStatus.activeObject = ccc.GetConnectedActiveObject();
    ANKI_VERIFY(nullptr != _targetStatus.activeObject,
                "CubeInteractionTracker.InvalidActiveObject",
                "CubeConnectionCoordinator reported connected to cube, but had no valid active object");

    // Check whether we know where the target is, or just that its connected
    _targetStatus.observableObject = blockWorld.GetLocatedObjectByID(_targetStatus.activeObject->GetID());
  }
  else {
    // We're not connected...
    _targetStatus.activeObject = nullptr;
    
    // ...see whether we have a recently observed cube
    // NOTE: This will not work well with more than one cube in view
    _targetStatus.observableObject = blockWorld.FindMostRecentlyObservedObject(*_targetFilter);
  }

  if( (nullptr == _targetStatus.activeObject) && (nullptr == _targetStatus.observableObject) ){
    // We didn't get a target from either a connection or BlockWorld. 
    // Clear the target data
    _targetStatus = TargetStatus();
    return false;
  } 

  // If we made it all the way here, we found a target
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetVisibility(const RobotCompMap& dependentcomps)
{
  if( nullptr != _targetStatus.observableObject && 
      _targetStatus.observableObject->GetLastObservedTime() >= _robot->GetLastImageTimeStamp() ){
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
  bool targetHasMoved = false;
  _targetStatus.movedThisFrame = false;
  _targetStatus.movedFarThisFrame = false;

  // If we're UpdatingTargetMotion, we know we have a target and...
  // if the robot is carrying the target, we know the user isn't
  auto& carryingComp = dependentComps.GetComponent<CarryingComponent>();
  if(carryingComp.IsCarryingObject(GetTargetID())){
    targetHasMoved = false;
    _targetStatus.isHeld = false;
    _targetStatus.probabilityIsHeld = 0.0f;
    _targetStatus.observedSinceMovedByRobot = false;
  }
  else{
    // See if we can check motion from accel data as an active object first
    if(nullptr != _targetStatus.activeObject){
      targetHasMoved = _targetStatus.activeObject->IsMoving();
    }

    // If the target was seen this frame, update the pose
    if(_targetStatus.visibleThisFrame) {
      Pose3d currentPose = _targetStatus.observableObject->GetPose();

      // Ignore the first position update after being carried by the robot as far as being held goes.
      // This allows the robot to update the pose of something it just set down without triggering as held.
      if(_targetStatus.observedSinceMovedByRobot){
        Pose3d relMovementPose;
        // if the old pose can't be compared, assume no movement. This should cover delocalization.
        if(currentPose.GetWithRespectTo(_targetStatus.prevPose, relMovementPose)){
          Radians angleThreshold;
          angleThreshold.setDegrees(kTargetUnmovedAngle_deg);
          if(!currentPose.IsSameAs(_targetStatus.prevPose, kTargetUnmovedDist_mm, angleThreshold)){
            targetHasMoved = true;

            // Check if the target has "moved far" since last seen
            // If the target has moved far from where we last saw it, ramp up the probability that 
            // its currently held by the user
            float moved_mm = relMovementPose.GetTranslation().Length();
            if(moved_mm >= kTargetMovedFarDist_mm){
              _targetStatus.movedFarThisFrame = true;
              _targetStatus.movedFarRecently = true;
              _targetStatus.lastMovedFarTime_s = _currentTimeThisTick_s;
              PRINT_NAMED_INFO("CubeInteractionTracker.TargetMovedFar",
                                "Target moved %f mm since last seen. Assuming currently held.",
                                moved_mm);
            } 
          }
        }
      }
      else{
        // We've discarded one pose update since the robot moved the cube. Resume normal tracking next frame.
        _targetStatus.observedSinceMovedByRobot = true;
      }

      // Update the previous pose
      _targetStatus.prevPose = currentPose;
    } // visibleThisFrame
  } // not carried by robot

  // This is mostly tracked for debug purposes, but could be used elsewhere if interested
  if(_targetStatus.movedFarRecently && !_targetStatus.movedFarThisFrame){
    float timeSinceMovedFar = _currentTimeThisTick_s - _targetStatus.lastMovedFarTime_s; 
    _targetStatus.movedFarRecently = timeSinceMovedFar < kRecentlyMovedFarLimit_s;
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
  _targetStatus.observableObject->GetPose().GetWithRespectTo(_robot->GetPose(), targetWRTRobot);
  Pose2d robotToTargetFlat(targetWRTRobot);
  Radians angleToTarget(atan2f(robotToTargetFlat.GetY(), robotToTargetFlat.GetX()));
  _targetStatus.angleFromRobotFwd_deg = angleToTarget.getDegrees();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::UpdateTargetDistance(const RobotCompMap& dependentComps)
{
  _targetStatus.distMeasuredWithProx = false;

  if(!_targetStatus.visibleRecently || (nullptr == _targetStatus.observableObject) ){
    // We could possibly update distance from Prox if it hasn't been very long since we
    // last saw the target. BUT... if we're connected and deloc, the ObservableObject* 
    // will be null, so we have to watch out for that
    return;
  }

  bool usingProx = false;
  bool proxReadingValid = false;
  bool targetIsInProxFOV = false;
  if(kUseProxForDistance){
    auto& proxSensor = dependentComps.GetComponent<ProxSensorComponent>();
    proxSensor.IsInFOV(_targetStatus.observableObject->GetPose(), targetIsInProxFOV);
    u16 proxDist_mm = 0;

    // If we can see the cube (or have seen it recently enough) to verify its in prox range, use prox
    proxReadingValid = proxSensor.GetLatestDistance_mm(proxDist_mm);
    if(targetIsInProxFOV && proxReadingValid){
      _targetStatus.distance_mm = proxDist_mm;
      _targetStatus.distMeasuredWithProx = true;
      usingProx = true;
    }
  } 
  
  // If not using prox, we must have seen the target this frame to have any useful information
  if(!usingProx && _targetStatus.visibleThisFrame){
    // Otherwise, use ClosestMarker for VisionBased distance checks
    Pose3d closestMarkerPose;
    _targetStatus.observableObject->GetClosestMarkerPose(_robot->GetPose(), true, closestMarkerPose);
    _targetStatus.distance_mm = Point2f(closestMarkerPose.GetTranslation()).Length();
  }

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeInteractionTracker::SendDataToWebViz()
{
#if ANKI_DEV_CHEATS

  if(_robot != nullptr){
    const CozmoContext* context = _robot->GetContext();
    if(nullptr != context){
      if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameCubes,
                                                                        context->GetWebService()) ) {
        Json::Value citInfo = Json::objectValue;

        // Display Status Info
        citInfo["trackingState"]     = _debugStateString;
        if(GetTargetID().IsUnknown()){
          citInfo["NoTarget"]        = true;
        }
        citInfo["visTrackingRate"]   = _trackingAtHighRate ? "High" : "Low";
        citInfo["userHoldingCube"]   = _targetStatus.isHeld;
        citInfo["heldProbability"]   = _targetStatus.probabilityIsHeld * 100.0f;
        citInfo["movedFarRecently"]  = _targetStatus.movedFarRecently;
        citInfo["movedRecently"]     = _targetStatus.movedRecently;
        citInfo["visibleRecently"]   = _targetStatus.visibleRecently;
        citInfo["timeSinceHeld"]     = _currentTimeThisTick_s - _targetStatus.lastHeldTime_s;
        citInfo["timeSinceMoved"]    = _currentTimeThisTick_s - _targetStatus.lastMovedTime_s;
        citInfo["timeSinceObserved"] = _currentTimeThisTick_s - _targetStatus.lastObservedTime_s;
        citInfo["timeSinceTapped"]   = _currentTimeThisTick_s - _targetStatus.lastTappedTime_s;

        if(_targetStatus.visibleRecently){
          Json::Value targetInfo           = Json::objectValue;
          targetInfo["distance"]           = _targetStatus.distance_mm;
          targetInfo["distMeasuredByProx"] = _targetStatus.distMeasuredWithProx;
          targetInfo["angle"]              = _targetStatus.angleFromRobotFwd_deg;
          citInfo["targetInfo"]            = targetInfo;
        }

        webSender->Data()["citInfo"] = citInfo;
      }
    }
  }

#endif // ANKI_DEV_CHEATS
}

} // namespace Vector
} // namespace Anki
