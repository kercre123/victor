/**
 * File: behaviorKeepaway.cpp
 *
 * Author: Sam Russell
 * Date:   02/26/2018
 *
 * Description: Victor attempts to pounce on an object while the user tries to dodge the pounce
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorKeepaway.h"

#include "clad/types/behaviorComponent/behaviorStats.h"
#include "clad/types/featureGateTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "util/math/math.h"

#define SET_STATE(s) do{ \
                          _dVars.state = KeepawayState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki{
namespace Vector{

namespace{
static const DrivingAnimationHandler::DrivingAnimations kKeepawayDrivingAnims { AnimationTrigger::CubePounceDriveGetIn,
                                                                                AnimationTrigger::CubePounceDriveLoop,
                                                                                AnimationTrigger::CubePounceDriveGetOut };
const char* kMinPouncesForSoloPlayKey = "minPouncesForSoloPlay";
const char* kMaxPouncesForSoloPlayKey = "maxPouncesForSoloPlay";
const char* kDebugName = "BehaviorKeepaway";
const char* kLogChannelName = "Behaviors";
// Distance past nominalPounceDist that we allow victor to creep. Helps ensure he eventually pounces.
const float kCreepOverlapDist_mm = 8.0f;
const char* kUseProxForDistance = "useProxForDistance";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKeepaway::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  // Helper macro to grab the key with the same name as the param struct member from the Json and
  // set it as a float
  # define SET_FLOAT_HELPER(__name__) do { \
    __name__ = JsonTools::ParseFloat(config, QUOTE(__name__), "BehaviorKeepaway.InstanceConfig");\
    if( ANKI_DEVELOPER_CODE ) {\
      floatNames.emplace_back( QUOTE(__name__) );\
    }\
  } while(0)

  minPouncesForSoloPlay = JsonTools::ParseInt8(config, kMinPouncesForSoloPlayKey, kDebugName);
  maxPouncesForSoloPlay = JsonTools::ParseInt8(config, kMaxPouncesForSoloPlayKey, kDebugName); 

  SET_FLOAT_HELPER(targetUnmovedGameEndTimeout_s);
  SET_FLOAT_HELPER(noVisibleTargetGameEndTimeout_s);
  SET_FLOAT_HELPER(outOfPlayGameEndTimeout_s);
  SET_FLOAT_HELPER(targetVisibleTimeout_s);
  SET_FLOAT_HELPER(globalOffsetDist_mm);
  SET_FLOAT_HELPER(inPlayDistance_mm);
  SET_FLOAT_HELPER(outOfPlayDistance_mm);
  SET_FLOAT_HELPER(allowablePointingError_deg);
  SET_FLOAT_HELPER(targetUnmovedDistance_mm);
  SET_FLOAT_HELPER(targetUnmovedAngle_deg);
  SET_FLOAT_HELPER(targetUnmovedCreepTimeout_s);
  SET_FLOAT_HELPER(creepDistanceMin_mm);
  SET_FLOAT_HELPER(creepDistanceMax_mm);
  SET_FLOAT_HELPER(creepDelayTimeMin_s);
  SET_FLOAT_HELPER(creepDelayTimeMax_s);
  SET_FLOAT_HELPER(pounceDelayTimeMin_s);
  SET_FLOAT_HELPER(pounceDelayTimeMax_s);
  SET_FLOAT_HELPER(basePounceChance);
  SET_FLOAT_HELPER(pounceChanceIncrement);
  SET_FLOAT_HELPER(nominalPounceDistance_mm);
  SET_FLOAT_HELPER(mousetrapPounceDistance_mm);
  SET_FLOAT_HELPER(probBackupInsteadOfMousetrap);
  SET_FLOAT_HELPER(pounceSuccessPitchDiff_deg);

  SET_FLOAT_HELPER(excitementIncPerHit);
  SET_FLOAT_HELPER(maxProbExitExcited);
  SET_FLOAT_HELPER(frustrationIncPerMiss);
  SET_FLOAT_HELPER(maxProbExitFrustrated);
  SET_FLOAT_HELPER(minProbToExit);

  SET_FLOAT_HELPER(baseProbReact);
  SET_FLOAT_HELPER(minProbToReact);
  SET_FLOAT_HELPER(probReactIncrement);
  SET_FLOAT_HELPER(probReactMax);

  useProxForDistance = JsonTools::ParseBool(config, kUseProxForDistance, kDebugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKeepaway::DynamicVariables::DynamicVariables(const InstanceConfig& iConfig)
: state(KeepawayState::GetIn)
, targetID(ObjectID())
, target(KeepawayTarget())
, pounceReadyState(PounceReadyState::Unready)
, gameStartTime_s(0.0f)
, outOfPlayExitTime_s(0.0f)
, creepTime(0.0f)
, pounceChance(iConfig.basePounceChance)
, pounceTime(0.0f)
, pounceSuccessPitch_deg(0.0f)
, frustrationExcitementScale(1.0f)
, probReactToHit(0.0f)
, probReactToMiss(0.0f)
, pounceCount(0)
, pounceCountToExit(1)
, isIdling(0)
, victorGotLastPoint(false)
, gameOver(false)
, soloExitAfterNextPounce(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKeepaway::BehaviorKeepaway(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
, _dVars(_iConfig)
{
  // Add configurable params as ConsoleVars
  MakeMemberTunable(_iConfig.minPouncesForSoloPlay, "minPouncesForSoloPlay", kDebugName);
  MakeMemberTunable(_iConfig.maxPouncesForSoloPlay, "maxPouncesForSoloPlay", kDebugName);
  MakeMemberTunable(_iConfig.targetUnmovedGameEndTimeout_s, "targetUnmovedGameEndTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.noVisibleTargetGameEndTimeout_s, "noVisibleTargetGameEndTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.outOfPlayGameEndTimeout_s, "outOfPlayGameEndTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.targetVisibleTimeout_s, "targetVisibleTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.globalOffsetDist_mm, "globalOffsetDist_mm", kDebugName);
  MakeMemberTunable(_iConfig.inPlayDistance_mm, "inPlayDistance_mm", kDebugName);
  MakeMemberTunable(_iConfig.outOfPlayDistance_mm, "outOfPlayDistance_mm", kDebugName);
  MakeMemberTunable(_iConfig.allowablePointingError_deg, "allowablePointingError_deg", kDebugName);
  MakeMemberTunable(_iConfig.targetUnmovedCreepTimeout_s, "targetUnmovedCreepTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.creepDistanceMin_mm, "creepDistanceMin_mm", kDebugName);
  MakeMemberTunable(_iConfig.creepDistanceMax_mm, "creepDistanceMax_mm", kDebugName);
  MakeMemberTunable(_iConfig.creepDelayTimeMin_s, "creepDelayTimeMin_s", kDebugName);
  MakeMemberTunable(_iConfig.creepDelayTimeMax_s, "creepDelayTimeMax_s", kDebugName);
  MakeMemberTunable(_iConfig.pounceDelayTimeMin_s, "pounceDelayTimeMin_s", kDebugName);
  MakeMemberTunable(_iConfig.pounceDelayTimeMax_s, "pounceDelayTimeMax_s", kDebugName);
  MakeMemberTunable(_iConfig.basePounceChance, "basePounceChance", kDebugName);
  MakeMemberTunable(_iConfig.pounceChanceIncrement, "pounceChanceIncrement", kDebugName);
  MakeMemberTunable(_iConfig.nominalPounceDistance_mm, "nominalPounceDistance_mm", kDebugName);
  MakeMemberTunable(_iConfig.mousetrapPounceDistance_mm, "mousetrapPounceDistance_mm", kDebugName);
  MakeMemberTunable(_iConfig.probBackupInsteadOfMousetrap, "probBackupInsteadOfMousetrap", kDebugName);
  MakeMemberTunable(_iConfig.pounceSuccessPitchDiff_deg, "pounceSuccessPitchDiff_deg", kDebugName);
  MakeMemberTunable(_iConfig.excitementIncPerHit, "excitementIncPerHit", kDebugName);
  MakeMemberTunable(_iConfig.maxProbExitExcited, "maxProbExitExcited", kDebugName);
  MakeMemberTunable(_iConfig.frustrationIncPerMiss, "frustrationIncPerMiss", kDebugName);
  MakeMemberTunable(_iConfig.maxProbExitFrustrated, "maxProbExitFrustrated", kDebugName);
  MakeMemberTunable(_iConfig.minProbToExit, "minProbToExit", kDebugName);
  MakeMemberTunable(_iConfig.baseProbReact, "baseProbReact", kDebugName);
  MakeMemberTunable(_iConfig.minProbToReact, "minProbToReact", kDebugName);
  MakeMemberTunable(_iConfig.probReactIncrement, "probReactIncrement", kDebugName);
  MakeMemberTunable(_iConfig.probReactMax, "probReactMax", kDebugName);
  MakeMemberTunable(_iConfig.useProxForDistance, "useProxForDistance", kDebugName);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kMinPouncesForSoloPlayKey );
  expectedKeys.insert( kMaxPouncesForSoloPlayKey );
  expectedKeys.insert( kUseProxForDistance );
  for( const auto& str : _iConfig.floatNames ) {
    expectedKeys.insert( str.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKeepaway::WantsToBeActivatedBehavior() const 
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::OnBehaviorActivated()
{
  // reset state for new game
  _dVars = DynamicVariables(_iConfig);
  _dVars.probReactToHit = _iConfig.baseProbReact;
  _dVars.probReactToMiss = _iConfig.baseProbReact;
  _dVars.pounceCountToExit = GetRNG().RandIntInRange(_iConfig.minPouncesForSoloPlay, _iConfig.maxPouncesForSoloPlay);

  // Initialize time stamps for various timeouts
  _dVars.gameStartTime_s = GetCurrentTimeInSeconds();

  GetBEI().GetMoodManager().TriggerEmotionEvent("KeepawayStarted");

  // push driving animations for creeping 
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetDrivingAnimationHandler().PushDrivingAnimations(kKeepawayDrivingAnims, GetDebugLabel());

  TransitionToGetIn();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::OnBehaviorDeactivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetDrivingAnimationHandler().RemoveDrivingAnimations(GetDebugLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(KeepawayState::GetOut != _dVars.state ||
     KeepawayState::GetOutSolo != _dVars.state){
    UpdateTargetStatus();
  }

  if( (KeepawayState::Searching == _dVars.state) ||
      (KeepawayState::Stalking == _dVars.state) ) {
      CheckGetOutTimeOuts();
  }

  if(KeepawayState::Searching == _dVars.state){
    if(_dVars.target.isVisible){
      bool allowCallback = false;
      CancelDelegates(allowCallback);
      TransitionToStalking();
      return;
    }
  }
  else if(KeepawayState::Stalking == _dVars.state){
    UpdateStalking();
  }
  else if(KeepawayState::Pouncing == _dVars.state ){
    // Check whether we've hit anything throughout the pounce
    if(!_dVars.victorGotLastPoint && PitchIndicatesPounceSuccess()){
      _dVars.victorGotLastPoint = true;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::UpdateTargetStatus()
{
  TargetStatus status = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  // Parse out target info into Keepaway context
  _dVars.target.isValid = (nullptr != status.observableObject);
  if(_dVars.target.isValid){
    _dVars.targetID = status.observableObject->GetID();
  }
  _dVars.target.isVisible = (GetCurrentTimeInSeconds() - status.lastObservedTime_s) < _iConfig.targetVisibleTimeout_s;
  _dVars.target.isInPlay = status.distance_mm < (_iConfig.globalOffsetDist_mm + _iConfig.outOfPlayDistance_mm) ;
  _dVars.target.isOffCenter = ABS(status.angleFromRobotFwd_deg) > _iConfig.allowablePointingError_deg;
  _dVars.target.isInMousetrapRange = status.distance_mm < (_iConfig.globalOffsetDist_mm + _iConfig.mousetrapPounceDistance_mm);

  // Handle pertinent state changes 
  bool isNotMoving = (GetCurrentTimeInSeconds() - status.lastMovedTime_s) > _iConfig.targetUnmovedCreepTimeout_s;
  bool isInPounceRange = status.distance_mm < (_iConfig.globalOffsetDist_mm + _iConfig.nominalPounceDistance_mm);

  // If the target just stopped moving
  if(!_dVars.target.isNotMoving && isNotMoving){
    float creepDelay = GetRNG().RandDblInRange(_iConfig.creepDelayTimeMin_s, _iConfig.creepDelayTimeMax_s);
    _dVars.creepTime = GetCurrentTimeInSeconds() + creepDelay; 
  }
  _dVars.target.isNotMoving = isNotMoving;

  // If the target just entered pounce range
  if(!_dVars.target.isInPounceRange && isInPounceRange){
    float pounceDelay = GetRNG().RandDblInRange(_iConfig.pounceDelayTimeMin_s, _iConfig.pounceDelayTimeMax_s);
    _dVars.pounceTime = GetCurrentTimeInSeconds() + pounceDelay;
  }
  _dVars.target.isInPounceRange = isInPounceRange;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::CheckGetOutTimeOuts()
{
  TargetStatus status = GetBEI().GetCubeInteractionTracker().GetTargetStatus();
  bool victorIsBored = false;

  if(status.visibleRecently){
    // Don't end on a move timeout if the target is not visible
    float timeSinceMoved = GetCurrentTimeInSeconds() - status.lastMovedTime_s;
    float timeSinceGameStart = GetCurrentTimeInSeconds() - _dVars.gameStartTime_s;

    // Don't do a movement timeout if the cube hasn't moved since starting. This typically indicates that he is
    // pouncing on a cube without an opponent
    if(timeSinceMoved > timeSinceGameStart){
      // just keep playing until we exit from too much success, or someone engages 
      if(_dVars.pounceCount == _dVars.pounceCountToExit - 1){
        _dVars.soloExitAfterNextPounce = true;
        return;
      }
    }
    else if( ( _iConfig.targetUnmovedGameEndTimeout_s > 0.0f ) &&
           ( timeSinceMoved > _iConfig.targetUnmovedGameEndTimeout_s) ){
      victorIsBored = true;
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorKeepaway.GameEndTimeout", 
                    "Victor got bored and quit because the cube didn't move for %f seconds",
                    _iConfig.targetUnmovedGameEndTimeout_s);
    }
  }
  else{
    if( (_iConfig.noVisibleTargetGameEndTimeout_s > 0.0f) &&
        (GetCurrentTimeInSeconds() - status.lastObservedTime_s) > _iConfig.noVisibleTargetGameEndTimeout_s){
      victorIsBored = true;
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorKeepaway.GameEndTimeout", 
                    "Victor got bored and quit because his cube was hidden for %f seconds",
                    _iConfig.noVisibleTargetGameEndTimeout_s);
    }
  }

  if( KeepawayState::Stalking == _dVars.state &&
      _dVars.outOfPlayExitTime_s != 0.0f && 
      GetCurrentTimeInSeconds() > _dVars.outOfPlayExitTime_s ){
    PRINT_CH_INFO(kLogChannelName,
                  "BehaviorKeepaway.OutOfPlayTimeout",
                  "Victor got bored and quit because his cube was out of play for %f seconds",
                  _iConfig.outOfPlayGameEndTimeout_s);
    victorIsBored = true;
  }

  if(victorIsBored){
    TransitionToGetOutBored();
  } 
  return;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToGetIn()
{
  SET_STATE(GetIn);

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetIn),
                      &BehaviorKeepaway::TransitionToSearching);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToSearching()
{
  if(KeepawayState::Searching != _dVars.state){
    SET_STATE(Searching);
    StopIdleAnimation();
    CancelDelegates(false);
  }

  if(_dVars.target.isVisible){
    TransitionToStalking();
  }
  else{
    if(_dVars.target.isValid){
      DelegateIfInControl(new SearchForNearbyObjectAction(_dVars.targetID), 
                          &BehaviorKeepaway::TransitionToSearching);
    } else {
      DelegateIfInControl(new SearchForNearbyObjectAction(), 
                          &BehaviorKeepaway::TransitionToSearching);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToStalking()
{
  SET_STATE(Stalking);  
  StopIdleAnimation();
  CancelDelegates(false);

  // Recompute the next creep and pounce times here as the SOONEST we will execute these actions
  // creepTime will get overwritten from UpdateTargetStatus() if the target moves
  float creepDelay = GetRNG().RandDblInRange(_iConfig.creepDelayTimeMin_s, _iConfig.creepDelayTimeMax_s);
  _dVars.creepTime = GetCurrentTimeInSeconds() + creepDelay;
  // pounceTime will get overwritten from UpdateTargetStatus() if the target enters pounce range
  float pounceDelay = GetRNG().RandDblInRange(_iConfig.pounceDelayTimeMin_s, _iConfig.pounceDelayTimeMax_s);
  _dVars.pounceTime = GetCurrentTimeInSeconds() + pounceDelay;

  _dVars.outOfPlayExitTime_s = GetCurrentTimeInSeconds() + _iConfig.outOfPlayGameEndTimeout_s;

  StartIdleAnimation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::UpdateStalking()
{
  if(!IsControlDelegated() || _dVars.isIdling){

    if(!_dVars.target.isValid || !_dVars.target.isVisible){
      // If the target goes out of sight, don't change the ready state
      TransitionToSearching();
      return;
    }

    if(_dVars.target.isOffCenter){
      StopIdleAnimation();
      SmartDisableKeepFaceAlive();
      DelegateIfInControl(new TurnTowardsObjectAction(_dVars.targetID),
        [this](){
          SmartReEnableKeepFaceAlive();
          StartIdleAnimation();
        });
      return;
    }

    switch(_dVars.pounceReadyState){
      case PounceReadyState::Ready:
      {
        if(!_dVars.target.isInPlay){
          // If the target goes outOfPlay but not outOfSight, TransitionToUnready
          StopIdleAnimation();
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetUnready),
            [this](){
              _dVars.pounceReadyState = PounceReadyState::Unready;
              _dVars.outOfPlayExitTime_s = GetCurrentTimeInSeconds() + _iConfig.outOfPlayGameEndTimeout_s; 
              StartIdleAnimation();
            });
        } else if(_dVars.target.isInMousetrapRange){
          if(GetRNG().RandDbl() < _iConfig.probBackupInsteadOfMousetrap){
            PRINT_NAMED_INFO("BehaviorKeepaway.BackupInsteadOfPounce","");
            StopIdleAnimation();
            DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::CubePounceBackup),
                                &BehaviorKeepaway::StartIdleAnimation);
          } else {
            if(GetRNG().RandDbl() < _dVars.pounceChance){
              TransitionToPouncing();
            } else {
              TransitionToFakeOut();
            }
          }
        } else if(_dVars.target.isInPounceRange && (GetCurrentTimeInSeconds() > _dVars.pounceTime)){
          if(GetRNG().RandDbl() < _dVars.pounceChance){
            TransitionToPouncing();
          } else {
            TransitionToFakeOut();
          }
        } else if (!_dVars.target.isInPounceRange && _dVars.target.isNotMoving && (GetCurrentTimeInSeconds() > _dVars.creepTime)){
          TransitionToCreeping();
        }
        break;
      }
      case PounceReadyState::Unready:
      {
        if(_dVars.target.isInPlay){
          // When the target comes back into play, TransitionToReady
          StopIdleAnimation();
          _dVars.outOfPlayExitTime_s = 0.0f;
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetReady),
                              [this](){
                                _dVars.pounceReadyState = PounceReadyState::Ready;
                                StartIdleAnimation();
                              });
        }
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToCreeping()
{
  SET_STATE(Creeping);
  StopIdleAnimation();
  CancelDelegates(false);

  TargetStatus status = GetBEI().GetCubeInteractionTracker().GetTargetStatus();

  float maxCreepDistance = status.distance_mm - _iConfig.nominalPounceDistance_mm + kCreepOverlapDist_mm;
  float creepDistance = 0.0f;
  if(maxCreepDistance <= 0.0f){
    TransitionToStalking();
    return; 
  } else if(maxCreepDistance < _iConfig.creepDistanceMin_mm){
    creepDistance = maxCreepDistance;
  } else {
    maxCreepDistance = std::min(maxCreepDistance, _iConfig.creepDistanceMax_mm);
    creepDistance = GetRNG().RandDblInRange(_iConfig.creepDistanceMin_mm,
                                                  maxCreepDistance);
  }

  DelegateIfInControl(new DriveStraightAction(creepDistance), &BehaviorKeepaway::TransitionToStalking);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToPouncing()
{
  SET_STATE(Pouncing);
  StopIdleAnimation();
  CancelDelegates(false);

  GetBEI().GetMoodManager().TriggerEmotionEvent("KeepawayPounce");

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::AttemptedPounceOnCube);

  _dVars.victorGotLastPoint = false;
  float startingPitch = GetBEI().GetRobotInfo().GetPitchAngle().getDegrees();
  _dVars.pounceSuccessPitch_deg = startingPitch + _iConfig.pounceSuccessPitchDiff_deg;
  _dVars.pounceChance = _iConfig.basePounceChance;
  AnimationTrigger pounceTrigger = _dVars.target.isInMousetrapRange ? 
                                   AnimationTrigger::CubePouncePounceClose :
                                   AnimationTrigger::CubePouncePounceNormal;
  ++_dVars.pounceCount;
  DelegateIfInControl(new TriggerAnimationAction(pounceTrigger), &BehaviorKeepaway::TransitionToReacting);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToFakeOut()
{
  SET_STATE(FakeOut);
  StopIdleAnimation();
  CancelDelegates(false);

  _dVars.pounceChance = Util::Clamp((_dVars.pounceChance + _iConfig.pounceChanceIncrement), 0.0f, 1.0f);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceFake), 
                      &BehaviorKeepaway::TransitionToStalking);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToReacting()
{
  SET_STATE(Reacting);
  StopIdleAnimation();
  CancelDelegates(false);

  // Watch for a solo play exit signal
  if(_dVars.soloExitAfterNextPounce){
    TransitionToSoloGetOut();
    // Bypass the usual reactions
    return;
  }

  // Based on the pounce outcome, accrue frustration or excitement
  float probExit = 0.0f;
  if(_dVars.victorGotLastPoint){
    GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::PounceOnCubeSuccess);

    float max = 1.0f + _iConfig.maxProbExitExcited;
    float newVal = _dVars.frustrationExcitementScale + _iConfig.excitementIncPerHit;
    _dVars.frustrationExcitementScale = Anki::Util::Min(newVal, max);
    probExit = Anki::Util::Max(0.0f, _dVars.frustrationExcitementScale - 1.0f);
    PRINT_NAMED_INFO("BehaviorKeepaway.Hit",
                     "Successful pounce! Scale value: %f. Exit probability: %f",
                     _dVars.frustrationExcitementScale,
                     probExit);
  } 
  else {
    float min = 1.0f - _iConfig.maxProbExitFrustrated;
    float newVal = _dVars.frustrationExcitementScale - _iConfig.frustrationIncPerMiss;
    _dVars.frustrationExcitementScale = Anki::Util::Max( newVal, min);
    probExit = Anki::Util::Max(0.0f, 1.0f - _dVars.frustrationExcitementScale);
    PRINT_NAMED_INFO("BehaviorKeepaway.Miss",
                     "Missed pounce. Scale value: %f. Exit probability: %f",
                     _dVars.frustrationExcitementScale,
                     probExit);
  }

  float exitRoll = GetRNG().RandDblInRange(0.0f, 1.0f);
  if(exitRoll < _iConfig.minProbToExit){
    exitRoll = _iConfig.minProbToExit;
    PRINT_NAMED_INFO("BehaviorKeepaway.ClampedExitProbability",
                     "Exit roll of: %f clamped to: %f",
                     exitRoll, _iConfig.minProbToExit);
  }

  if(exitRoll < probExit){
    PRINT_NAMED_INFO("BehaviorKeepaway.EmotionalExit", 
                     "Exiting keepaway due to excessive excitement/frustration. prob: %f. rand: %f.",
                     probExit, exitRoll);
                      
    AnimationTrigger winResponseAnim = _dVars.victorGotLastPoint ? 
                                        AnimationTrigger::CubePounceWinSession :
                                        AnimationTrigger::CubePounceLoseSession;
    // No Delegation. Behavior will end after winResponse.
    DelegateIfInControl(new TriggerAnimationAction(winResponseAnim));
  } else {
    if(_dVars.victorGotLastPoint){
      if(GetRNG().RandDblInRange(_iConfig.minProbToReact, 1.0f) < _dVars.probReactToHit){
        _dVars.probReactToHit = _iConfig.baseProbReact;
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceWinHand),
                            &BehaviorKeepaway::TransitionToStalking);
      }
      else{
        _dVars.probReactToHit = Anki::Util::Min(_dVars.probReactToHit + _iConfig.probReactIncrement,
                                                _iConfig.probReactMax);
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetReady),
                            &BehaviorKeepaway::TransitionToStalking);
      }
    }
    else{
      if(GetRNG().RandDblInRange(_iConfig.minProbToReact, 1.0f) < _dVars.probReactToMiss){
        _dVars.probReactToMiss = _iConfig.baseProbReact;
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceLoseHand),
                            &BehaviorKeepaway::TransitionToStalking);
      }
      else{
        _dVars.probReactToMiss = Anki::Util::Min(_dVars.probReactToMiss + _iConfig.probReactIncrement,
                                                 _iConfig.probReactMax);
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetReady),
                            &BehaviorKeepaway::TransitionToStalking);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToGetOutBored()
{
  SET_STATE(GetOut);
  StopIdleAnimation();
  CancelDelegates(false);

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetOutBored),
                      [this](){
                        BehaviorObjectiveAchieved(BehaviorObjective::KeepawayQuitBored);
                        // Don't delegate, cancel behavior.
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::TransitionToSoloGetOut()
{
  SET_STATE(GetOutSolo);
  StopIdleAnimation();
  CancelDelegates(false);

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceWinSession),
                      [this](){
                        BehaviorObjectiveAchieved(BehaviorObjective::KeepawayQuitBored);
                        // Don't delegate, cancel behavior.
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKeepaway::PitchIndicatesPounceSuccess() const
{
  if (GetBEI().GetRobotInfo().GetPitchAngle().getDegrees() > _dVars.pounceSuccessPitch_deg){
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::StartIdleAnimation()
{
  AnimationTrigger idleAnimationTrigger = (PounceReadyState::Ready == _dVars.pounceReadyState) ?
                                          AnimationTrigger::CubePounceIdleLiftUp :
                                          AnimationTrigger::CubePounceIdleLiftDown;

  if(IsControlDelegated()){
    PRINT_NAMED_ERROR("BehaviorKeepaway.InvalidStartIdleAnimationCall",
                      "Attempted to loop an idle animation while the behavior had already delegated control");
    _dVars.isIdling = false;
    return;
  }
  DelegateIfInControl(new TriggerAnimationAction(idleAnimationTrigger),
                      &BehaviorKeepaway::StartIdleAnimation);
  _dVars.isIdling = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::StopIdleAnimation()
{
  if(_dVars.isIdling){
    bool allowCallback = false;
    CancelDelegates(allowCallback);
    _dVars.isIdling = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorKeepaway::GetCurrentTimeInSeconds() const
{
  return BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

} // namespace Vector
} // namespace Anki
