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
#include "engine/actions/compoundActions.h"
#include "engine/actions/trackObjectAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/cozmoObservableObject.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/math/math.h"

#define SET_STATE(s) do{ \
                          _dVars.state = InspectCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

#define DEBUG_INSPECT_CUBE_BEHAVIOR 0

namespace Anki {
namespace Vector {

namespace{
// JSON parameter keys
static const char* kPlayWithCubeDelegateKey = "PlayWithCubeBehaviorID";

// Internal Tunable params 
static const float kSearchGetOutTimeout_s            = 5.0f;
static const float kTrackingUnseenSearchTimeout_s    = 2.0f;
static const float kTrackingNotHeldPlayTime_s        = 0.3f;
static const float kBoredGetOutTimeout_s             = 30.0f;

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
  JsonTools::GetValueOptional(config, kPlayWithCubeDelegateKey, playWithCubeBehaviorIDString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorInspectCube::DynamicVariables::DynamicVariables(float startTime_s)
: state(InspectCubeState::Init)
, behaviorStartTime_s(startTime_s)
, searchEndTime_s(0.0f)
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
  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DriveOffChargerCube));

  if(!_iConfig.playWithCubeBehaviorIDString.empty()){
    BehaviorID behaviorID;
    if(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.playWithCubeBehaviorIDString, behaviorID)){
      _iConfig.playWithCubeBehavior = BC.FindBehaviorByID(behaviorID);
    }
    else {
      PRINT_NAMED_ERROR("BehaviorInspectCube.InvalidBehaviorID",
                        "%s is not a valid behaviorID, InspectCube will self-cancel when it would have delegated",
                        _iConfig.playWithCubeBehaviorIDString.c_str());
    }
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
  modifiers.behaviorAlwaysDelegates = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
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
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  if(nullptr != _iConfig.playWithCubeBehavior){
    delegates.insert(_iConfig.playWithCubeBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kPlayWithCubeDelegateKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds());

#if DEBUG_INSPECT_CUBE_BEHAVIOR
  float lastSeenTime_s = GetBEI().GetCubeInteractionTracker().GetTargetStatus().lastObservedTime_s;
  float timeSinceSeen_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - lastSeenTime_s;
  if(timeSinceSeen_s > 4.0f){
    PRINT_NAMED_WARNING("BehaviorInspectCube", "Too long since cube seen");
  }
#endif

  TransitionToGetIn();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  _currentTimeThisTick_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  if(InspectCubeState::Searching == _dVars.state){
    UpdateBoredomTimeouts(targetStatus);
    UpdateSearching(targetStatus);
  }

  if(InspectCubeState::Tracking == _dVars.state){
    UpdateBoredomTimeouts(targetStatus);
    UpdateTracking(targetStatus);
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
void BehaviorInspectCube::TransitionToGetIn()
{
  SET_STATE(GetIn);

  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::InvestigateHeldCubeGetIn),
    [this](){
      const bool currOnCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
      const bool currOnChargerPlatform = GetBEI().GetRobotInfo().IsOnChargerPlatform();
      if(currOnCharger || currOnChargerPlatform) {
        TransitionToDriveOffCharger();
      }
      else {
        TransitionToSearching();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToDriveOffCharger()
{
  SET_STATE(DriveOffCharger);

  if(_iConfig.driveOffChargerBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(),
                        &BehaviorInspectCube::TransitionToSearching);
  }
  else{
    PRINT_NAMED_ERROR("BehaviorInspectCube.DriveOffChargerBehaviorNotActivatable","");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToSearching()
{
  SET_STATE(Searching);
  CancelDelegates(false);
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();
  if(targetStatus.visibleRecently){
    TransitionToTracking();
  }
  else{
    // If we don't find the target soon, exit bored
    _dVars.searchEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kSearchGetOutTimeout_s;

    // Kick of a recursive search delegation loop
    SearchLoopHelper();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::SearchLoopHelper()
{
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  if(nullptr != targetStatus.observableObject){
    DelegateIfInControl(new SearchForNearbyObjectAction(targetStatus.observableObject->GetID()),
                        &BehaviorInspectCube::SearchLoopHelper);
  }
  else {
    DelegateIfInControl(new SearchForNearbyObjectAction(),
                        &BehaviorInspectCube::SearchLoopHelper);
  }  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateSearching(const TargetStatus& targetStatus){
  if(targetStatus.visibleThisFrame){
    TransitionToTracking();
  }
  else if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _dVars.searchEndTime_s){
    TransitionToGetOutTargetLost();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToTracking()
{
  SET_STATE(Tracking);
  CancelDelegates(false);
  TargetStatus targetStatus = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  if(nullptr == targetStatus.observableObject){
    TransitionToGetOutTargetLost();
    return;
  }

  bool trackByType = false;
  TrackObjectAction* trackAction = new TrackObjectAction(targetStatus.observableObject->GetID(), trackByType);
  trackAction->SetTiltTolerance(DEG_TO_RAD(kMinTrackingTiltAngle_deg));
  trackAction->SetPanTolerance(DEG_TO_RAD(kMinTrackingPanAngle_deg));
  trackAction->SetClampSmallAnglesToTolerances(true);
  trackAction->SetClampSmallAnglesPeriod(kMinTrackingClampPeriod_s, kMaxTrackingClampPeriod_s);

  auto* trackAnimAction = new TriggerLiftSafeAnimationAction(AnimationTrigger::InvestigateHeldCubeTrackingLoop);

  auto compoundAction = new CompoundActionParallel({
    trackAction,
    trackAnimAction
  });

  // If the TrackObjectAction returns, it has lost its target. Go to searching
  DelegateIfInControl(compoundAction, &BehaviorInspectCube::TransitionToSearching); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::UpdateTracking(const TargetStatus& targetStatus)
{
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

  // If the user set the cube down, go play with it
  float timeSinceHeld_s = currentTime_s - targetStatus.lastHeldTime_s;
  if(!targetStatus.isHeld && (timeSinceHeld_s > kTrackingNotHeldPlayTime_s)){
    TransitionToPlayingWithCube();
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.GoPlayWithcube", 
                  "CubeInteractionTracker Indicated that the cube had been set down. Going to interact.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToKeepaway()
{
  SET_STATE(Keepaway);
  CancelDelegates(false);      
  if(_iConfig.keepawayBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.keepawayBehavior.get());
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToPlayingWithCube()
{
  SET_STATE(PlayingWithCube);
  CancelDelegates(false);
  if(nullptr == _iConfig.playWithCubeBehavior){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.NoSpecifiedDelegate", 
                  "Conditions to play with cube were met, but there was no delegate specified for this state");
    // behavior should exit due to non-delegation
    return;
  }

  if(_iConfig.playWithCubeBehavior->WantsToBeActivated()){
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::InvestigateHeldCubeOnSetDown),
      [this](){
        DelegateIfInControl(_iConfig.playWithCubeBehavior.get());
      });
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorInspectCube.DelegatingToPlayWithCubeBehavior", "");
  }
  else{
    PRINT_NAMED_ERROR("BehaviorInspectCube.DelegateDidNotWantToBeActivated",
                      "PlayWithCube delegate behavior implementation error. InspectCube will self-cancel");
    // behavior should exit due to non-delegation
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToGetOutBored()
{
  SET_STATE(GetOutBored);
  CancelDelegates(false);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::InvestigateHeldCubeGetOutBored));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorInspectCube::TransitionToGetOutTargetLost()
{
  SET_STATE(GetOutTargetLost);
  CancelDelegates(false);

  PRINT_CH_INFO(kLogChannelName,
                "BehaviorInspectCube.TargetLost", 
                "CubeInteractionTracker Reported target as invalid, exiting");
  // TODO:(str) replace this with a "Where'd ma cube get off to?" anim
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::InvestigateHeldCubeGetOutCubeLost));
}

}
}
