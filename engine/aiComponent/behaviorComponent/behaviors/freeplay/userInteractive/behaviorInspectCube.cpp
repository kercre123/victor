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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackObjectAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/cozmoObservableObject.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/math/math.h"

#define SET_STATE(s) SetState_internal(s, #s)

#define DEBUG_INSPECT_CUBE_BEHAVIOR 0

namespace Anki {
namespace Vector {

namespace{

// Internal Tunable params 
static const float kSearchGetOutTimeout_s         = 5.0f;
static const float kTrackingUnseenSearchTimeout_s = 2.5f;
static const float kTrackingMinGetOutTime_s       = 10.0f;
static const float kTrackingUnmovedGetOutTime_s   = 5.0f;
static const float kBoredGetOutTimeout_s          = 30.0f;

static const float kTriggerDistance_mm            = 80.0f;

static const float kMinTrackingTiltAngle_deg = 4.0f;
static const float kMinTrackingPanAngle_deg = 4.0f;
static const float kMinTrackingClampPeriod_s = 0.2f;
static const float kMaxTrackingClampPeriod_s = 0.7f;

// Constants
const char* kLogChannelName = "Behaviors";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::InstanceConfig::InstanceConfig(const Json::Value& config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::DynamicVariables::DynamicVariables(float startTime_s)
: state(InspectCubeState::GetIn)
, behaviorStartTime_s(startTime_s)
, searchEndTime_s(0.0f)
, trackingMinEndTime_s(0.0f)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::BehaviorInspectCube(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
, _dVars(0.0f)
, _currentTimeThisTick_s(0.0f)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::~BehaviorInspectCube()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.keepawayBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(Keepaway));

  if(!_iConfig.cubeSetDownBehaviorIDString.empty()){
    BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.cubeSetDownBehaviorIDString);
    _iConfig.cubeSetDownBehavior = BC.FindBehaviorByID(behaviorID);
  }
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

  // Interaction tracking is MUCH better when connected. Request a background connection once we've decided the 
  // user is trying to engage with a cube
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.keepawayBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  // const char* list[] = {
  // };
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds());

#if DEBUG_INSPECT_CUBE_BEHAVIOR
  //DNM
  float lastSeenTime_s = GetBEI().GetCubeInteractionTracker().GetTargetStatus().lastObservedTime_s;
  float timeSinceSeen_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - lastSeenTime_s;
  if(timeSinceSeen_s > 4.0f){
    PRINT_NAMED_WARNING("BehaviorInspectCube", "Too long since cube seen");
  }
#endif

  TransitionToSearching();
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

  _currentTimeThisTick_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  if( InspectCubeState::Searching == _dVars.state ||
      InspectCubeState::Tracking == _dVars.state){
    UpdateBoredomTimeouts(targetStatus);
  }

  switch(_dVars.state){
    case InspectCubeState::Searching:
    {
      UpdateSearching(targetStatus);
      break;
    }
    case InspectCubeState::Tracking:
    {
      UpdateTracking(targetStatus);
      break;
    }
    case InspectCubeState::GetIn:
    case InspectCubeState::Keepaway:
    case InspectCubeState::CubeSetDown:
    // TODO(str) Watch for the user to set down the cube, then trigger behaviors like moveCube, rollCube, etc
    // as appropriate
    case InspectCubeState::GetOutBored:
    {
      //Do Nothing
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateBoredomTimeouts(const TargetStatus& targetStatus)
{
  if((_currentTimeThisTick_s - _dVars.behaviorStartTime_s) > kBoredGetOutTimeout_s){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.BoredTimeout", 
                  "We've been diddling about inspecing the cube too long with no result. Victor got bored.");
    TransitionToGetOutBored();
    // TODO:(str) hook this up to a cooldown so we don't jump right back in
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToSearching()
{
  SET_STATE(InspectCubeState::Searching);
  CancelDelegates(false);
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();
  if(targetStatus.visibleRecently){
    TransitionToTracking();
  }
  else{
    // If we don't find the target soon, exit bored
    _dVars.searchEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kSearchGetOutTimeout_s;
    if(nullptr != targetStatus.object){
      DelegateIfInControl(new SearchForNearbyObjectAction(targetStatus.object->GetID()),
                          &BehaviorInspectCube::TransitionToSearching);
    }
    else {
      DelegateIfInControl(new SearchForNearbyObjectAction(),
                          &BehaviorInspectCube::TransitionToSearching);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateSearching(const TargetStatus& targetStatus){
  if(targetStatus.visibleThisFrame){
    TransitionToTracking();
  }
  else if(!targetStatus.isValid){
    TransitionToGetOutTargetLost();
  }
  else if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _dVars.searchEndTime_s){
    TransitionToGetOutBored();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToTracking()
{
  SET_STATE(InspectCubeState::Tracking);
  CancelDelegates(false);
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  bool trackByType = false;
  TrackObjectAction* trackAction = new TrackObjectAction(targetStatus.object->GetID(), trackByType);
  trackAction->SetTiltTolerance(DEG_TO_RAD(kMinTrackingTiltAngle_deg));
  trackAction->SetPanTolerance(DEG_TO_RAD(kMinTrackingPanAngle_deg));
  trackAction->SetClampSmallAnglesToTolerances(true);
  trackAction->SetClampSmallAnglesPeriod(kMinTrackingClampPeriod_s, kMaxTrackingClampPeriod_s);

  // If the TrackObjectAction returns, it has lost its target. Go to searching
  DelegateIfInControl(trackAction, &BehaviorInspectCube::TransitionToSearching); 
  _dVars.trackingMinEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kTrackingMinGetOutTime_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTracking(const TargetStatus& targetStatus)
{
  if(!targetStatus.isValid){
    TransitionToGetOutTargetLost();
    return;
  }

  if(targetStatus.distance_mm <= kTriggerDistance_mm){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.DistanceTriggerTripped", 
                  "Cube came within %f mm of victor, while (we think) held by the user. Triggering keepaway",
                  kTriggerDistance_mm);
    TransitionToKeepaway();
    return;
  }

  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  float timeSinceSeen_s = currentTime_s - targetStatus.lastObservedTime_s;
  if(timeSinceSeen_s > kTrackingUnseenSearchTimeout_s){
    TransitionToSearching();
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.TrackingUnseenTimeout", 
                  "Target was unseen for %f sec while tracking, going to Searching",
                  kTrackingUnseenSearchTimeout_s);
    return;
  }

  float timeSinceMoved_s = currentTime_s - targetStatus.lastMovedTime_s;
  if( (currentTime_s > _dVars.trackingMinEndTime_s) && (timeSinceMoved_s > kTrackingUnmovedGetOutTime_s)){
    TransitionToGetOutBored();
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.TargetUnmovedTimeout", 
                  "Target was tracked for at least %f sec without moving for at least %f sec. Victor got bored.",
                  kTrackingMinGetOutTime_s, kTrackingUnmovedGetOutTime_s);
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToKeepaway()
{
  SET_STATE(InspectCubeState::Keepaway);
  CancelDelegates(false);      
  if(_iConfig.keepawayBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.keepawayBehavior.get(),
                        [this](){
                          CancelSelf();
                        });
  } else {
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToGetOutBored()
{
  SET_STATE(InspectCubeState::GetOutBored);
  CancelDelegates(false);
  // TODO:(str) replace this with a "Where'd ma cube get off to?" anim
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetOutBored), [this](){CancelSelf();});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToGetOutTargetLost()
{
  SET_STATE(InspectCubeState::GetOutBored);
  CancelDelegates(false);

  PRINT_CH_INFO(kLogChannelName,
                "BehaviorInspectCube.TargetLost", 
                "CubeInteractionTracker Reported target as invalid, exiting");

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetOutBored), [this](){CancelSelf();});
}

}
}
