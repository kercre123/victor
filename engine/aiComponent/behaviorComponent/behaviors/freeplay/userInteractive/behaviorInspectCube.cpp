/**
 * File: BehaviorInspectCube.cpp
 *
 * Author: Sam Russell
 * Created: 2018-04-03
 *
 * Description: This behavior serves as a stage for implementing user interactive, cube driven behavior transitions.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorInspectCube.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackObjectAction.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorKeepaway.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/moodSystem/moodManager.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/math/math.h"

#include <set>

#define SET_STATE(s) SetState_internal(s, #s)

namespace Anki {
namespace Cozmo {

namespace{
// JSON parameter keys
static const char* kTargetUnmovedGetOutKey     = "targetUnmovedGetOutTimeout_s";
static const char* kNoValidTargetGetOutKey     = "noValidTargetGetOutTimeout_s";
static const char* kTargetHiddenGetOutKey      = "targetHiddenGetOutTimeout_s";
static const char* kUseProxSensorKey           = "useProxForDistance";
static const char* kTriggerDistKey             = "triggerDistance_mm";
static const char* kMinWithdrawalSpeedKey      = "minWithdrawalSpeed_mmpers";
static const char* kMinWithdrawalDistKey       = "minWithdrawalDist_mm";

// Internal Tunable params 
static const float kTargetVisibleTimeout_s = 1.0f;
static const float kTargetUnmovedDistance_mm = 4.0f;
static const float kTargetUnmovedAngle_deg = 5.0f;
static const float kMinApproachMovementThreshold_mm  = 10.0f;

// Constants
const char* kDebugName = "BehaviorInspectCube";
static const char* kLogChannelName = "Behaviors";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::TargetStatus::TargetStatus()
: prevPose(Pose3d())
, lastValidTime_s(0.0f)
, lastObservedTime_s(0.0f)
, lastMovedTime_s(0.0f)
, distance(0.0f)
, isValid(false)
, isVisible(false)
, isNotMoving(false)
{
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  targetFilter = std::make_unique<BlockWorldFilter>();
  targetFilter->AddAllowedFamily(ObjectFamily::Block);
  targetFilter->AddAllowedFamily(ObjectFamily::LightCube);
  targetFilter->AddAllowedFamily(ObjectFamily::CustomObject);

  targetUnmovedGetOutTimeout_s  = 10.0f;
  noValidTargetGetOutTimeout_s  = 20.0f;
  targetHiddenGetOutTimeout_s   = 10.0f;
  useProxForDistance            = true;
  triggerDistance_mm            = 125.0f;
  minWithdrawalSpeed_mmpers     = 50.0f;
  minWithdrawalDist_mm          = 40.0f;

  JsonTools::GetValueOptional(config, kTargetUnmovedGetOutKey,    targetUnmovedGetOutTimeout_s);
  JsonTools::GetValueOptional(config, kNoValidTargetGetOutKey,    noValidTargetGetOutTimeout_s);
  JsonTools::GetValueOptional(config, kTargetHiddenGetOutKey,     targetHiddenGetOutTimeout_s);
  JsonTools::GetValueOptional(config, kUseProxSensorKey,          useProxForDistance);
  JsonTools::GetValueOptional(config, kTriggerDistKey,            triggerDistance_mm);
  JsonTools::GetValueOptional(config, kMinWithdrawalSpeedKey,     minWithdrawalSpeed_mmpers);
  JsonTools::GetValueOptional(config, kMinWithdrawalDistKey,      minWithdrawalDist_mm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::DynamicVariables::DynamicVariables()
: state(InspectCubeState::GetIn)
, targetID(ObjectID())
, target(TargetStatus())
, approachState(TargetApproachState::Static)
, prevApproachState(TargetApproachState::Static)
, behaviorStartTime_s(0.0f)
, retreatStartDistance_mm(0.0f)
, retreatStartTime_s(0.0f)
, nearApproachTriggered(false)
, victorIsBored(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::BehaviorInspectCube(const Json::Value& config)
 : ICozmoBehavior(config)
 , _iConfig(config)
{
  MakeMemberTunable(_iConfig.targetUnmovedGetOutTimeout_s, kTargetUnmovedGetOutKey, kDebugName);
  MakeMemberTunable(_iConfig.noValidTargetGetOutTimeout_s, kNoValidTargetGetOutKey, kDebugName);
  MakeMemberTunable(_iConfig.targetHiddenGetOutTimeout_s, kTargetHiddenGetOutKey, kDebugName);
  MakeMemberTunable(_iConfig.useProxForDistance, kUseProxSensorKey, kDebugName);
  MakeMemberTunable(_iConfig.triggerDistance_mm, kTriggerDistKey, kDebugName);
  MakeMemberTunable(_iConfig.minWithdrawalSpeed_mmpers, kMinWithdrawalSpeedKey, kDebugName);
  MakeMemberTunable(_iConfig.minWithdrawalDist_mm, kMinWithdrawalDistKey, kDebugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::~BehaviorInspectCube()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(Keepaway),
                                 BEHAVIOR_CLASS(Keepaway),
                                 _iConfig.keepawayBehavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInspectCube::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.keepawayBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kTargetUnmovedGetOutKey,
    kNoValidTargetGetOutKey,
    kTargetHiddenGetOutKey,
    kUseProxSensorKey,
    kTriggerDistKey,
    kMinWithdrawalSpeedKey,
    kMinWithdrawalDistKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // Initialize time stamps for various timeouts
  _dVars.target.lastValidTime_s = GetCurrentTimeInSeconds();
  _dVars.target.lastObservedTime_s = GetCurrentTimeInSeconds();
  _dVars.target.lastMovedTime_s = GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::SetState_internal(InspectCubeState state, const std::string& stateName)
{
  _dVars.state = state;
  SetDebugStateName(stateName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  CheckTargetStatus();
  UpdateBoredomTimeouts();

  switch(_dVars.state){
    case InspectCubeState::GetIn:
    {
      if(!IsControlDelegated()){
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetIn),
                            &BehaviorInspectCube::TransitionToSearching);
      }
      break;
    }
    case InspectCubeState::Searching:
    {
      UpdateSearching();
      break;
    }
    case InspectCubeState::Tracking:
    {
      UpdateApproachState();
      UpdateTracking();
      break;
    }
    case InspectCubeState::Keepaway:
    case InspectCubeState::GetOutBored:
    {
      //Do Nothing
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateApproachState()
{
  _dVars.prevApproachState = _dVars.approachState;
  const float distanceDelta = _dVars.target.prevDistance - _dVars.target.distance;
  if(distanceDelta > kMinApproachMovementThreshold_mm){
    _dVars.approachState = TargetApproachState::Approaching;
  } else if (distanceDelta < -kMinApproachMovementThreshold_mm){
    _dVars.approachState = TargetApproachState::Retreating;
  } else {
    _dVars.approachState = TargetApproachState::Static;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateBoredomTimeouts()
{
  if(_dVars.victorIsBored){
    return;
  }

  // If we haven't had a valid target for too long, end the game bored
  float timeSinceTargetValid = GetCurrentTimeInSeconds() - _dVars.target.lastValidTime_s;
  if((_iConfig.noValidTargetGetOutTimeout_s > 0.0f) &&
     (timeSinceTargetValid > _iConfig.noValidTargetGetOutTimeout_s)){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.GameEndTimeout", 
                  "Victor got bored and quit because he coudn't find a cube...");
    _dVars.victorIsBored = true;
  }

  // Check if we want to end the behavior due to player inactivity
  if((_dVars.target.isValid) &&
     (_dVars.target.isNotMoving) && 
     (_iConfig.targetUnmovedGetOutTimeout_s > 0.0f) &&
     (GetCurrentTimeInSeconds() - _dVars.target.lastMovedTime_s > _iConfig.targetUnmovedGetOutTimeout_s)){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.Timeout", 
                  "Victor got bored and quit because no one was playing with him...");
    _dVars.victorIsBored = true;
  }

  if((_dVars.target.isValid) &&
     (!_dVars.target.isVisible) &&
     (_iConfig.targetHiddenGetOutTimeout_s > 0.0f) &&
     (GetCurrentTimeInSeconds() - _dVars.target.lastObservedTime_s) > _iConfig.targetHiddenGetOutTimeout_s){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.GameEndTimeout", 
                  "Victor got bored and quit because his cube was hidden too long...");
    _dVars.victorIsBored = true;
  }

  if(_dVars.victorIsBored){
    TransitionToGetOutBored();
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToSearching()
{
  SET_STATE(InspectCubeState::Searching);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateSearching(){
  if(_dVars.target.isVisible){
    CancelDelegates();
    TransitionToTracking();
    return;
  }

  if(!IsControlDelegated()) {
    if(_dVars.target.isValid){
      DelegateIfInControl(new SearchForNearbyObjectAction(_dVars.targetID));
    } else {
      DelegateIfInControl(new SearchForNearbyObjectAction());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToTracking()
{
  SET_STATE(InspectCubeState::Tracking);
  _dVars.nearApproachTriggered = false;
  PRINT_CH_INFO(kLogChannelName,
                "BehaviorInspectCube.DistanceTriggerReset", 
                "Cube must get within %f mm of victor before withdrawal to trigger keepaway",
                _iConfig.triggerDistance_mm);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceReactToCube));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTracking()
{
  if(!_dVars.target.isValid || !_dVars.target.isVisible){
    CancelDelegates(false);
    TransitionToSearching();
    return;
  }

  if(!IsControlDelegated()){
    TrackObjectAction* trackAction = new TrackObjectAction(_dVars.targetID, true);
    trackAction->SetPanDuration(0.2f);
    trackAction->SetTiltDuration(0.1f);
    DelegateIfInControl(trackAction, &BehaviorInspectCube::TransitionToSearching);
  } 

  // Monitor for the user "teasing" victor with a cube
  const float currentTime_s = GetCurrentTimeInSeconds();

  if(!_dVars.nearApproachTriggered && (_dVars.target.distance <= _iConfig.triggerDistance_mm)){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.DistanceTriggerSet", 
                  "Cube came within %f mm of victor, withdraw faster than %f mm/s over at least %f mm to trigger keepaway",
                  _iConfig.triggerDistance_mm,
                  _iConfig.minWithdrawalSpeed_mmpers,
                  _iConfig.minWithdrawalDist_mm);
    CancelDelegates(false);
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceReactToCube));

    _dVars.nearApproachTriggered = true;
  }
  if(!_dVars.nearApproachTriggered){
   return;
  }

 // Only Consider frames where the approach state has changed
  if(_dVars.approachState != _dVars.prevApproachState){
    if(_dVars.approachState == TargetApproachState::Retreating){
      // If we just started retreating note when and where it started
      _dVars.retreatStartDistance_mm = _dVars.target.distance;
      _dVars.retreatStartTime_s = currentTime_s;
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorInspectCube.StartRetreat", 
                    "Detected cube being pulled away from Victor");

    }else if(_dVars.prevApproachState == TargetApproachState::Retreating){
      // If we just stopped retreating, see if we should transition to keepaway
      float distanceDelta = _dVars.target.distance - _dVars.retreatStartDistance_mm;
      float timeDelta = currentTime_s - _dVars.retreatStartTime_s;
      float avgRetreatSpeed = distanceDelta / timeDelta;
      if(avgRetreatSpeed < _iConfig.minWithdrawalSpeed_mmpers){
        PRINT_CH_INFO(kLogChannelName,
                      "BehaviorInspectCube.FailedTease", 
                      "Cube was withdrawn too slow.");
      }
      if(distanceDelta < _iConfig.minWithdrawalDist_mm){
        PRINT_CH_INFO(kLogChannelName,
                      "BehaviorInspectCube.FailedTease", 
                      "Cube was not withdrawn far enough.");
      }

      if((avgRetreatSpeed > _iConfig.minWithdrawalSpeed_mmpers) &&
        (distanceDelta > _iConfig.minWithdrawalDist_mm)){
        TransitionToKeepaway();
      }
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToKeepaway()
{
  CancelDelegates(false);      
  DelegateIfInControl(_iConfig.keepawayBehavior.get(),
                      [this](){
                        CancelSelf();
                      });
  SET_STATE(InspectCubeState::Keepaway);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToGetOutBored()
{
  CancelDelegates(false);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetOutBored), [this](){CancelSelf();});
  SET_STATE(InspectCubeState::GetOutBored);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorInspectCube::GetCurrentTimeInSeconds() const
{
  return BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Target Tracking Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:(str) Consider pulling the code from TargetTrackingMethods to a new behavior component and refactoring Keepaway
// to get its data from the same component. Perhaps a subscription system akin to the VSM?
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInspectCube::CheckTargetStatus()
{
  if(CheckTargetObject()){
    UpdateTargetVisibility();
    UpdateTargetAngle();
    UpdateTargetDistance();
    UpdateTargetMotion();
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInspectCube::CheckTargetObject()
{
  const ObservableObject* targetObject;

  if(!_dVars.targetID.IsSet()){
    // If we don't yet have a target, see whether we have a recently observed cube
    targetObject = GetBEI().GetBlockWorld().FindMostRecentlyObservedObject(*_iConfig.targetFilter);
  } else {
    // We do already have a target selected, see if blockworld knows where it is
    targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  }

  if(nullptr == targetObject){
    // Reset the target status if we don't have a target
    _dVars.target.isValid = false;
    _dVars.target.isVisible = false;
    _dVars.target.isNotMoving = false;

    return false;
  } else if(!_dVars.targetID.IsSet()){
    _dVars.targetID = targetObject->GetID();
  }

  _dVars.target.lastValidTime_s = GetCurrentTimeInSeconds();

  if(!_dVars.target.isValid){
    // It is possible that visible or movement timeouts are shorter than validTarget timeout. In that case,
    // we will instantly timeout on them upon recovering a valid target unless we reset their timers.
    _dVars.target.lastMovedTime_s = GetCurrentTimeInSeconds();
    _dVars.target.lastObservedTime_s = GetCurrentTimeInSeconds();    
    _dVars.target.isValid = true;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTargetVisibility()
{
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

  if(targetObject->GetLastObservedTime() >= GetBEI().GetRobotInfo().GetLastImageTimeStamp()){
    _dVars.target.lastObservedTime_s = GetCurrentTimeInSeconds();
    _dVars.target.isVisible = true;

  } else{
    float timeSinceTargetObserved_s = GetCurrentTimeInSeconds() - _dVars.target.lastObservedTime_s;
    _dVars.target.isVisible = timeSinceTargetObserved_s < kTargetVisibleTimeout_s;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTargetAngle()
{
  // Use the center of the cube for AngleToTarget computation 
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  Pose3d targetWRTRobot;
  targetObject->GetPose().GetWithRespectTo(GetBEI().GetRobotInfo().GetPose(), targetWRTRobot);
  Pose2d robotToTargetFlat(targetWRTRobot);
  Radians angleToTarget(atan2f(robotToTargetFlat.GetY(), robotToTargetFlat.GetX()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTargetDistance()
{
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

  _dVars.target.prevDistance = _dVars.target.distance;
  bool usingProx = false;
  if(_iConfig.useProxForDistance){
    auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetValue<ProxSensorComponent>();
    bool targetIsInProxFOV = false;
    proxSensor.IsInFOV(targetObject->GetPose(), targetIsInProxFOV);
    u16 proxDist_mm = 0;

    // If we can see the cube (or have seen it recently enough) to verify its in prox range, use prox
    if(_dVars.target.isVisible && targetIsInProxFOV && proxSensor.GetLatestDistance_mm(proxDist_mm)){
      _dVars.target.distance = proxDist_mm;
      usingProx = true;
    }
  } 
  
  if(!usingProx){
    // Otherwise, use ClosestMarker for VisionBased distance checks
    Pose3d closestMarkerPose;
    targetObject->GetClosestMarkerPose(GetBEI().GetRobotInfo().GetPose(), true, closestMarkerPose);
    _dVars.target.distance = Point2f(closestMarkerPose.GetTranslation()).Length();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTargetMotion()
{
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

  if(TargetHasMoved(targetObject)){
    _dVars.target.lastMovedTime_s = GetCurrentTimeInSeconds();
    _dVars.target.isNotMoving = false;
  } else {
    float timeSinceTargetMoved = GetCurrentTimeInSeconds() - _dVars.target.lastMovedTime_s;
    _dVars.target.isNotMoving = timeSinceTargetMoved > _iConfig.targetUnmovedGetOutTimeout_s;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorInspectCube::TargetHasMoved(const ObservableObject* targetObject)
{
  // See if we can check this as an active object first
  ActiveObject* targetActiveObject = GetBEI().GetBlockWorld().GetConnectedActiveObjectByID(targetObject->GetID());
  if(nullptr != targetActiveObject){
    // See if its reporting movement
    return targetActiveObject->IsMoving();
  }

  // Target is not connected, lets see if we can check for movement by pose
  // If we can't validate that it has moved due to pose parenting changes, assume it hasn't moved
  bool hasMoved = false;
  Radians angleThreshold;
  angleThreshold.setDegrees(kTargetUnmovedAngle_deg);
  hasMoved = !targetObject->GetPose().IsSameAs(_dVars.target.prevPose, 
                                        kTargetUnmovedDistance_mm,
                                        angleThreshold);
  _dVars.target.prevPose = targetObject->GetPose();

  return hasMoved;
}


}
}
