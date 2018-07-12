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
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "util/math/math.h"

#define SET_STATE(s) SetState_internal(s, #s)

namespace Anki{
namespace Cozmo{

namespace{
const char* kDebugName = "BehaviorKeepaway";
const char* kLogChannelName = "Behaviors";
// Distance past nominalPounceDist that we allow victor to creep. Helps ensure he eventually pounces.
const float kCreepOverlapDist_mm = 3.0f;
const char* kPointsToWinKey = "pointsToWin";
const char* kUseProxForDistance = "useProxForDistance";
const bool kAllowCallback = false;
}

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
  
  // Set up a filter for finding Keepaway-able items
  keepawayTargetFilter = std::make_unique<BlockWorldFilter>();
  keepawayTargetFilter->AddAllowedFamily(ObjectFamily::Block);
  keepawayTargetFilter->AddAllowedFamily(ObjectFamily::LightCube);
  keepawayTargetFilter->AddAllowedFamily(ObjectFamily::CustomObject);
  SET_FLOAT_HELPER(naturalGameEndTimeout_s);
  SET_FLOAT_HELPER(targetUnmovedGameEndTimeout_s);
  SET_FLOAT_HELPER(noVisibleTargetGameEndTimeout_s);
  SET_FLOAT_HELPER(noPointsEarnedTimeout_s);
  SET_FLOAT_HELPER(targetVisibleTimeout_s);
  SET_FLOAT_HELPER(animDistanceOffset_mm);
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
  SET_FLOAT_HELPER(instaPounceDistance_mm);
  SET_FLOAT_HELPER(pounceSuccessPitchDiff_deg);
  useProxForDistance = JsonTools::ParseBool(config, kUseProxForDistance, kDebugName);
  pointsToWin = JsonTools::ParseUint8(config, kPointsToWinKey, kDebugName);
}

BehaviorKeepaway::DynamicVariables::DynamicVariables(const InstanceConfig& iConfig)
: state(KeepawayState::GetIn)
, targetID(ObjectID())
, target(TargetStatus())
, pounceReadyState(PounceReadyState::Unready)
, gameStartTime_s(0.0f)
, targetLastValidTime_s(0.0f)
, targetLastObservedTime_s(0.0f)
, targetLastMovedTime_s(0.0f)
, targetPrevPose(Pose3d())
, targetDistance(0.0f)
, creepTime(0.0f)
, pounceChance(iConfig.basePounceChance)
, pounceTime(0.0f)
, pounceSuccessPitch_deg(0.0f)
, victorPoints(0)
, userPoints(0)
, isIdling(0)
, victorGotLastPoint(false)
, gameOver(false)
, victorIsBored(false)
{
}

BehaviorKeepaway::BehaviorKeepaway(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config)
, _dVars(_iConfig)
{
  // Add configurable params as ConsoleVars
  MakeMemberTunable(_iConfig.naturalGameEndTimeout_s, "naturalGameEndTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.targetUnmovedGameEndTimeout_s, "targetUnmovedGameEndTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.noVisibleTargetGameEndTimeout_s, "noVisibleTargetGameEndTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.noPointsEarnedTimeout_s, "noPointsEarnedTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.targetVisibleTimeout_s, "targetVisibleTimeout_s", kDebugName);
  MakeMemberTunable(_iConfig.animDistanceOffset_mm, "animDistanceOffset_mm", kDebugName);
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
  MakeMemberTunable(_iConfig.instaPounceDistance_mm, "instaPounceDistance_mm", kDebugName);
  MakeMemberTunable(_iConfig.pounceSuccessPitchDiff_deg, "pounceSuccessPitchDiff_deg", kDebugName);
  MakeMemberTunable(_iConfig.useProxForDistance, "useProxForDistance", kDebugName);
  MakeMemberTunable(_iConfig.pointsToWin, "pointsToWin", kDebugName);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKeepaway::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kPointsToWinKey );
  expectedKeys.insert( kUseProxForDistance );
  for( const auto& str : _iConfig.floatNames ) {
    expectedKeys.insert( str.c_str() );
  }
}

bool BehaviorKeepaway::WantsToBeActivatedBehavior() const 
{
  const auto* featureGate = GetBEI().GetRobotInfo().GetContext()->GetFeatureGate();
  const bool featureEnabled = featureGate->IsFeatureEnabled(Anki::Cozmo::FeatureType::CubeBehaviors);
  if(!featureEnabled)
  {
    return false;
  }

  return true;
}

void BehaviorKeepaway::OnBehaviorActivated()
{
  // reset state for new game
  _dVars = DynamicVariables(_iConfig);

  // Initialize time stamps for various timeouts
  _dVars.gameStartTime_s = GetCurrentTimeInSeconds();
  _dVars.targetLastValidTime_s = GetCurrentTimeInSeconds();
  _dVars.targetLastObservedTime_s = GetCurrentTimeInSeconds();
  _dVars.targetLastMovedTime_s = GetCurrentTimeInSeconds();

  GetBEI().GetMoodManager().TriggerEmotionEvent("KeepawayStarted");
}

void BehaviorKeepaway::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(!_dVars.gameOver && !_dVars.victorIsBored){
    CheckTargetStatus();

    if((!_dVars.victorPoints && !_dVars.userPoints) && 
      (_iConfig.noPointsEarnedTimeout_s > 0.0f) &&
      (GetCurrentTimeInSeconds() - _dVars.gameStartTime_s > _iConfig.noPointsEarnedTimeout_s)){
      _dVars.victorIsBored = true;
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorKeepaway.GameEndTimeout", 
                    "Victor got bored and quit because no-one was scoring...");
    }
  }

  if(_dVars.victorIsBored){
    StopIdleAnimation();
    if(!IsControlDelegated()){
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetOutBored),
                          [this](){
                            BehaviorObjectiveAchieved(BehaviorObjective::KeepawayQuitBored);
                            CancelSelf();
                          });
    } 
    return;
  }

  switch(_dVars.state){
    case KeepawayState::GetIn:
    {
      if(!IsControlDelegated()){
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetIn),
                            &BehaviorKeepaway::TransitionToSearching);
      }
      break;
    }
    case KeepawayState::Searching:
    {
      UpdateSearching();
      break;
    }
    case KeepawayState::Stalking:
    {
      UpdateStalking();
      break;
    }
    case KeepawayState::Pouncing:
    {
      // Check whether we've hit anything throughout the pounce
      if(!_dVars.victorGotLastPoint && PitchIndicatesPounceSuccess()){
        _dVars.victorGotLastPoint = true;
      }
      break;
    }
    case KeepawayState::Creeping:
    case KeepawayState::FakeOut:
    case KeepawayState::Reacting:
    case KeepawayState::GetOut:
    {
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorKeepaway.InvalidKeepawayState", "Received an undefined KeepawayState, aborting.");
      CancelSelf();
    }
  }
}

void BehaviorKeepaway::OnBehaviorDeactivated()
{

}

void BehaviorKeepaway::SetState_internal(KeepawayState state, const std::string& stateName)
{
  _dVars.state = state;
  SetDebugStateName(stateName);
}

void BehaviorKeepaway::TransitionToSearching()
{
  SET_STATE(KeepawayState::Searching);
}

void BehaviorKeepaway::UpdateSearching(){
  if(_dVars.target.isVisible){
    CancelDelegates();
    TransitionToStalking();
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

void BehaviorKeepaway::TransitionToStalking()
{
  // Recompute the next creep and pounce times here as the SOONEST we will execute these actions
  // creepTime will get overwritten from UpdateTargetMotion() if the target moves
  float creepDelay = GetRNG().RandDblInRange(_iConfig.creepDelayTimeMin_s, _iConfig.creepDelayTimeMax_s);
  _dVars.creepTime = GetCurrentTimeInSeconds() + creepDelay;
  // pounceTime will get overwritten from UpdateTargetDistance() if the target enters pounce range
  float pounceDelay = GetRNG().RandDblInRange(_iConfig.pounceDelayTimeMin_s, _iConfig.pounceDelayTimeMax_s);
  _dVars.pounceTime = GetCurrentTimeInSeconds() + pounceDelay;
  
  SET_STATE(KeepawayState::Stalking);  
}

void BehaviorKeepaway::UpdateStalking()
{
  if(!IsControlDelegated() || _dVars.isIdling){

    if(!_dVars.target.isValid || !_dVars.target.isVisible){
      // If the target goes out of sight, don't change the ready state
      StopIdleAnimation();
      TransitionToSearching();
      return;
    }

    if(_dVars.target.isOffCenter){
      StopIdleAnimation();
      DelegateIfInControl(new TurnTowardsObjectAction(_dVars.targetID));
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
                              });
        } else if(_dVars.target.isInInstaPounceRange){
          StopIdleAnimation();
          TransitionToPouncing();
        } else if(_dVars.target.isInPounceRange && (GetCurrentTimeInSeconds() > _dVars.pounceTime)){
          StopIdleAnimation();
          if(GetRNG().RandDbl() < _dVars.pounceChance){
            TransitionToPouncing();
          } else {
            TransitionToFakeOut();
          }
        } else if (_dVars.target.isNotMoving && (GetCurrentTimeInSeconds() > _dVars.creepTime)){
          StopIdleAnimation();
          TransitionToCreeping();
        } else if(!_dVars.isIdling){
          StartIdleAnimation(AnimationTrigger::CubePounceIdleLiftUp);
        }
        break;
      }
      case PounceReadyState::Unready:
      {
        if(_dVars.target.isInPlay){
          // When the target comes back into play, TransitionToReady
          StopIdleAnimation();
          DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetReady),
                              [this](){
                                _dVars.pounceReadyState = PounceReadyState::Ready;
                              });
        } else if (!_dVars.isIdling){
          StartIdleAnimation(AnimationTrigger::CubePounceIdleLiftDown);
        }
        break;
      }
    }
  }
}

void BehaviorKeepaway::TransitionToCreeping()
{
  float maxCreepDistance = _dVars.targetDistance - _iConfig.nominalPounceDistance_mm + kCreepOverlapDist_mm;
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
  SET_STATE(KeepawayState::Creeping);
}

void BehaviorKeepaway::TransitionToPouncing()
{
  GetBEI().GetMoodManager().TriggerEmotionEvent("KeepawayPounce");

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::AttemptedPounceOnCube);

  _dVars.victorGotLastPoint = false;
  float startingPitch = GetBEI().GetRobotInfo().GetPitchAngle().getDegrees();
  _dVars.pounceSuccessPitch_deg = startingPitch + _iConfig.pounceSuccessPitchDiff_deg;
  _dVars.pounceChance = _iConfig.basePounceChance;
  SET_STATE(KeepawayState::Pouncing);
  AnimationTrigger pounceTrigger = _dVars.target.isInInstaPounceRange ? 
                                   AnimationTrigger::CubePouncePounceClose :
                                   AnimationTrigger::CubePouncePounceNormal;
  DelegateIfInControl(new TriggerAnimationAction(pounceTrigger), &BehaviorKeepaway::TransitionToReacting);
}

void BehaviorKeepaway::TransitionToFakeOut()
{
  _dVars.pounceChance = Util::Clamp((_dVars.pounceChance + _iConfig.pounceChanceIncrement), 0.0f, 1.0f);
  SET_STATE(KeepawayState::FakeOut);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceFake), 
                      &BehaviorKeepaway::TransitionToStalking);
}

void BehaviorKeepaway::TransitionToReacting()
{
  // Assess points based on outcome of pounce 
  if(_dVars.victorGotLastPoint){
    ++_dVars.victorPoints;
  } else {
    ++_dVars.userPoints;
  }

  // Check whether the game should end due to a natural(between points) timeout or a point limit
  float gameRunTime = GetCurrentTimeInSeconds() - _dVars.gameStartTime_s;
  bool gameEndByTime = gameRunTime >= _iConfig.naturalGameEndTimeout_s;
  bool gameEndByPoints = (_dVars.userPoints >= _iConfig.pointsToWin) || (_dVars.victorPoints >= _iConfig.pointsToWin);
  if((gameEndByTime && (_iConfig.naturalGameEndTimeout_s > 0.0f)) ||
     (gameEndByPoints && (_iConfig.pointsToWin > 0))){
    _dVars.gameOver = true;
  } 

  SET_STATE(KeepawayState::Reacting);
  if(_dVars.gameOver){
    GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::PounceOnCubeSuccess);

    AnimationTrigger winResponseAnim = _dVars.victorGotLastPoint ? 
                                        AnimationTrigger::CubePounceWinSession :
                                        AnimationTrigger::CubePounceLoseSession;
    DelegateIfInControl(new TriggerAnimationAction(winResponseAnim), &BehaviorKeepaway::TransitionToGetOut);
  } else {
    AnimationTrigger pointResponseAnim = _dVars.victorGotLastPoint ? 
                                          AnimationTrigger::CubePounceWinHand :
                                          AnimationTrigger::CubePounceLoseHand;
    DelegateIfInControl(new TriggerAnimationAction(pointResponseAnim), &BehaviorKeepaway::TransitionToStalking);
  }
}

void BehaviorKeepaway::TransitionToGetOut()
{
  SET_STATE(KeepawayState::GetOut);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePounceGetOut),
                      [this](){
                        BehaviorObjective objectiveOutcome = _dVars.victorGotLastPoint ? 
                                                              BehaviorObjective::KeepawayWon :
                                                              BehaviorObjective::KeepawayLost; 
                        BehaviorObjectiveAchieved(objectiveOutcome);
                        CancelSelf();
                      });
}

bool BehaviorKeepaway::CheckTargetStatus()
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

bool BehaviorKeepaway::CheckTargetObject()
{
  const ObservableObject* targetObject;

  if(!_dVars.targetID.IsSet()){
    // If we don't yet have a target, see whether we have a recently observed cube
    targetObject = GetBEI().GetBlockWorld().FindMostRecentlyObservedObject(*_iConfig.keepawayTargetFilter);
  } else {
    // We do already have a target selected, see if blockworld knows where it is
    targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  }

  if(nullptr == targetObject){
    _dVars.target = TargetStatus();

    // If we haven't had a valid target for too long, end the game bored
    float timeSinceTargetValid = GetCurrentTimeInSeconds() - _dVars.targetLastValidTime_s;
    if(timeSinceTargetValid > _iConfig.noVisibleTargetGameEndTimeout_s &&
       _iConfig.noVisibleTargetGameEndTimeout_s > 0.0f){
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorKeepaway.GameEndTimeout", 
                    "Victor got bored and quit because he coudn't find a cube...");
      _dVars.victorIsBored = true;
    }
    return false;
  } else if(!_dVars.targetID.IsSet()){
    _dVars.targetID = targetObject->GetID();
  }

  _dVars.targetLastValidTime_s = GetCurrentTimeInSeconds();

  if(!_dVars.target.isValid){
    // It is possible that visible or movement timeouts are shorter than validTarget timeout. In that case,
    // we will instantly timeout on them upon recovering a valid target unless we reset their timers.
    _dVars.targetLastMovedTime_s = GetCurrentTimeInSeconds();
    _dVars.targetLastObservedTime_s = GetCurrentTimeInSeconds();    
    _dVars.target.isValid = true;
  }
  return true;
}

void BehaviorKeepaway::UpdateTargetVisibility()
{
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

  if(targetObject->GetLastObservedTime() >= GetBEI().GetRobotInfo().GetLastImageTimeStamp()){
    _dVars.targetLastObservedTime_s = GetCurrentTimeInSeconds();
    _dVars.target.isVisible = true;

  } else{
    float timeSinceTargetObserved_s = GetCurrentTimeInSeconds() - _dVars.targetLastObservedTime_s;
    _dVars.target.isVisible = timeSinceTargetObserved_s < _iConfig.targetVisibleTimeout_s;

    if(timeSinceTargetObserved_s > _iConfig.noVisibleTargetGameEndTimeout_s){
      _dVars.victorIsBored = _iConfig.noVisibleTargetGameEndTimeout_s > 0.0f;
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorKeepaway.GameEndTimeout", 
                    "Victor got bored and quit because his cube was hidden too long...");
    }
  }
}

void BehaviorKeepaway::UpdateTargetAngle()
{
  // Use the center of the cube for AngleToTarget computation 
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  Pose3d targetWRTRobot;
  targetObject->GetPose().GetWithRespectTo(GetBEI().GetRobotInfo().GetPose(), targetWRTRobot);
  Pose2d robotToTargetFlat(targetWRTRobot);
  Radians angleToTarget(atan2f(robotToTargetFlat.GetY(), robotToTargetFlat.GetX()));
  _dVars.target.isOffCenter = ABS(angleToTarget.getDegrees()) > _iConfig.allowablePointingError_deg;
}

void BehaviorKeepaway::UpdateTargetDistance()
{
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

  bool usingProx = false;
  if(_iConfig.useProxForDistance){
    auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
    bool targetIsInProxFOV = false;
    proxSensor.IsInFOV(targetObject->GetPose(), targetIsInProxFOV);
    u16 proxDist_mm = 0;

    // If we can see the cube (or have seen it recently enough) to verify its in prox range, use prox
    if(_dVars.target.isVisible && targetIsInProxFOV && proxSensor.GetLatestDistance_mm(proxDist_mm)){
      _dVars.targetDistance = proxDist_mm;
      usingProx = true;
    }
  } 
  
  if(!usingProx){
    // Otherwise, use ClosestMarker for VisionBased distance checks
    Pose3d closestMarkerPose;
    targetObject->GetClosestMarkerPose(GetBEI().GetRobotInfo().GetPose(), true, closestMarkerPose);
    _dVars.targetDistance = Point2f(closestMarkerPose.GetTranslation()).Length();
  }

  if(_dVars.targetDistance > _iConfig.animDistanceOffset_mm + _iConfig.outOfPlayDistance_mm){
    _dVars.target.isInPlay = false;
  } else if(_dVars.targetDistance < _iConfig.animDistanceOffset_mm + _iConfig.inPlayDistance_mm){
    _dVars.target.isInPlay = true;
  }

  if((_dVars.targetDistance < _iConfig.animDistanceOffset_mm + _iConfig.nominalPounceDistance_mm) &&
     (!_dVars.target.isInPounceRange)){
    _dVars.target.isInPounceRange = true;
    float pounceDelay = GetRNG().RandDblInRange(_iConfig.pounceDelayTimeMin_s, _iConfig.pounceDelayTimeMax_s);
    _dVars.pounceTime = GetCurrentTimeInSeconds() + pounceDelay;
  } else {
    _dVars.target.isInPounceRange = false;
  }

  _dVars.target.isInInstaPounceRange = 
    _dVars.targetDistance < _iConfig.animDistanceOffset_mm + _iConfig.instaPounceDistance_mm;

}

void BehaviorKeepaway::UpdateTargetMotion()
{
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);

  if(TargetHasMoved(targetObject)){
    _dVars.targetLastMovedTime_s = GetCurrentTimeInSeconds();
    _dVars.target.isNotMoving = false;

  } else {
    float timeSinceTargetMoved = GetCurrentTimeInSeconds() - _dVars.targetLastMovedTime_s;
    _dVars.target.isNotMoving = timeSinceTargetMoved > _iConfig.targetUnmovedCreepTimeout_s;

    // If the target just stopped moving, computed a random creep delay to drive the transition from Stalking 
    if((timeSinceTargetMoved > _iConfig.targetUnmovedCreepTimeout_s) &&
       (false == _dVars.target.isNotMoving)){
      _dVars.target.isNotMoving = true;
      float creepDelay = GetRNG().RandDblInRange(_iConfig.creepDelayTimeMin_s, _iConfig.creepDelayTimeMax_s);
      _dVars.creepTime = GetCurrentTimeInSeconds() + creepDelay;
    }

    // Also check if we want to end the game due to player inactivity
    if((_dVars.target.isNotMoving) && 
       (_iConfig.targetUnmovedGameEndTimeout_s > 0.0f) &&
       (timeSinceTargetMoved > _iConfig.targetUnmovedGameEndTimeout_s)){
      PRINT_CH_INFO(kLogChannelName,
                    "BehaviorKeepaway.GameEndTimeout", 
                    "Victor got bored and quit because no one was playing with him...");
      _dVars.victorIsBored = true;
    }
  }
}

bool BehaviorKeepaway::TargetHasMoved(const ObservableObject* targetObject)
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
  angleThreshold.setDegrees(_iConfig.targetUnmovedAngle_deg);
  hasMoved = !targetObject->GetPose().IsSameAs(_dVars.targetPrevPose, 
                                        _iConfig.targetUnmovedDistance_mm,
                                        angleThreshold);
  _dVars.targetPrevPose = targetObject->GetPose();

  return hasMoved;
}

bool BehaviorKeepaway::PitchIndicatesPounceSuccess() const
{
  if (GetBEI().GetRobotInfo().GetPitchAngle().getDegrees() > _dVars.pounceSuccessPitch_deg){
    return true;
  }
  return false;
}

void BehaviorKeepaway::StartIdleAnimation(const AnimationTrigger& idleAnimationTrigger)
{
  if(IsControlDelegated()){
    PRINT_NAMED_ERROR("BehaviorKeepaway.InvalidStartIdleAnimationCall",
                      "Attempted to loop an idle animation while the behavior had already delegated control");
    _dVars.isIdling = false;
    return;
  }
  DelegateIfInControl(new TriggerAnimationAction(idleAnimationTrigger),
                      [this, idleAnimationTrigger](){
                        if(!IsControlDelegated()){
                          StartIdleAnimation(idleAnimationTrigger);
                        }
                      });
  _dVars.isIdling = true;
}

void BehaviorKeepaway::StopIdleAnimation()
{
  if(_dVars.isIdling){
    CancelDelegates(kAllowCallback);
    _dVars.isIdling = false;
  }
}

float BehaviorKeepaway::GetCurrentTimeInSeconds() const
{
  return BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

} // namespace Cozmo
} // namespace Anki
