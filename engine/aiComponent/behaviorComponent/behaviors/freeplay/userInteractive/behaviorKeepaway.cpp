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

#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/math/math.h"

#define SET_STATE(s) SetState_internal(s, #s)

namespace Anki{
namespace Cozmo{

namespace{
const char* kDebugName = "BehaviorKeepaway";
const char* kLogChannelName = "Behaviors";
// Distance past nominalPounceDist that we allow victor to creep. Helps ensure he eventually pounces.
const float kCreepOverlapDist_mm = 3.0f;
const bool kAllowCallback = false;
}

BehaviorKeepaway::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  // Set up a filter for finding Keepaway-able items
  keepawayTargetFilter = std::make_unique<BlockWorldFilter>();
  keepawayTargetFilter->AddAllowedFamily(ObjectFamily::Block);
  keepawayTargetFilter->AddAllowedFamily(ObjectFamily::LightCube);
  keepawayTargetFilter->AddAllowedFamily(ObjectFamily::CustomObject);
  naturalGameEndTimeout_s = JsonTools::ParseFloat(config, "naturalGameEndTimeout_s", kDebugName);
  targetUnmovedGameEndTimeout_s = JsonTools::ParseFloat(config, "targetUnmovedGameEndTimeout_s", kDebugName);
  noVisibleTargetGameEndTimeout_s = JsonTools::ParseFloat(config, "noVisibleTargetGameEndTimeout_s", kDebugName);
  noPointsEarnedTimeout_s = JsonTools::ParseFloat(config, "noPointsEarnedTimeout_s", kDebugName);
  targetVisibleTimeout_s = JsonTools::ParseFloat(config, "targetVisibleTimeout_s", kDebugName);
  animDistanceOffset_mm = JsonTools::ParseFloat(config, "animDistanceOffset_mm", kDebugName);
  inPlayDistance_mm = JsonTools::ParseFloat(config, "inPlayDistance_mm", kDebugName);
  outOfPlayDistance_mm = JsonTools::ParseFloat(config, "outOfPlayDistance_mm", kDebugName);
  allowablePointingError_deg = JsonTools::ParseFloat(config, "allowablePointingError_deg", kDebugName);
  targetUnmovedDistance_mm = JsonTools::ParseFloat(config, "targetUnmovedDistance_mm", kDebugName);
  targetUnmovedAngle_deg = JsonTools::ParseFloat(config, "targetUnmovedAngle_deg", kDebugName);
  targetUnmovedCreepTimeout_s = JsonTools::ParseFloat(config, "targetUnmovedCreepTimeout_s", kDebugName);
  creepDistanceMin_mm = JsonTools::ParseFloat(config, "creepDistanceMin_mm", kDebugName);
  creepDistanceMax_mm = JsonTools::ParseFloat(config, "creepDistanceMax_mm", kDebugName);
  creepDelayTimeMin_s = JsonTools::ParseFloat(config, "creepDelayTimeMin_s", kDebugName);
  creepDelayTimeMax_s = JsonTools::ParseFloat(config, "creepDelayTimeMax_s", kDebugName);
  pounceDelayTimeMin_s = JsonTools::ParseFloat(config, "pounceDelayTimeMin_s", kDebugName);
  pounceDelayTimeMax_s = JsonTools::ParseFloat(config, "pounceDelayTimeMax_s", kDebugName);
  basePounceChance = JsonTools::ParseFloat(config, "basePounceChance", kDebugName);
  pounceChanceIncrement = JsonTools::ParseFloat(config, "pounceChanceIncrement", kDebugName);
  nominalPounceDistance_mm = JsonTools::ParseFloat(config, "nominalPounceDistance_mm", kDebugName);
  instaPounceDistance_mm = JsonTools::ParseFloat(config, "instaPounceDistance_mm", kDebugName);
  pounceSuccessPitchDiff_deg = JsonTools::ParseFloat(config, "pounceSuccessPitchDiff_deg", kDebugName);
  pointsToWin = JsonTools::ParseUint8(config, "pointsToWin", kDebugName);
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
  MakeMemberTunable(_iConfig.naturalGameEndTimeout_s, "naturalGameEndTimeout_s");
  MakeMemberTunable(_iConfig.targetUnmovedGameEndTimeout_s, "targetUnmovedGameEndTimeout_s");
  MakeMemberTunable(_iConfig.noVisibleTargetGameEndTimeout_s, "noVisibleTargetGameEndTimeout_s");
  MakeMemberTunable(_iConfig.noPointsEarnedTimeout_s, "noPointsEarnedTimeout_s");
  MakeMemberTunable(_iConfig.targetVisibleTimeout_s, "targetVisibleTimeout_s");
  MakeMemberTunable(_iConfig.animDistanceOffset_mm, "animDistanceOffset_mm");
  MakeMemberTunable(_iConfig.inPlayDistance_mm, "inPlayDistance_mm");
  MakeMemberTunable(_iConfig.outOfPlayDistance_mm, "outOfPlayDistance_mm");
  MakeMemberTunable(_iConfig.allowablePointingError_deg, "allowablePointingError_deg");
  MakeMemberTunable(_iConfig.targetUnmovedCreepTimeout_s, "targetUnmovedCreepTimeout_s");
  MakeMemberTunable(_iConfig.creepDistanceMin_mm, "creepDistanceMin_mm");
  MakeMemberTunable(_iConfig.creepDistanceMax_mm, "creepDistanceMax_mm");
  MakeMemberTunable(_iConfig.creepDelayTimeMin_s, "creepDelayTimeMin_s");
  MakeMemberTunable(_iConfig.creepDelayTimeMax_s, "creepDelayTimeMax_s");
  MakeMemberTunable(_iConfig.pounceDelayTimeMin_s, "pounceDelayTimeMin_s");
  MakeMemberTunable(_iConfig.pounceDelayTimeMax_s, "pounceDelayTimeMax_s");
  MakeMemberTunable(_iConfig.basePounceChance, "basePounceChance");
  MakeMemberTunable(_iConfig.pounceChanceIncrement, "pounceChanceIncrement");
  MakeMemberTunable(_iConfig.nominalPounceDistance_mm, "nominalPounceDistance_mm");
  MakeMemberTunable(_iConfig.instaPounceDistance_mm, "instaPounceDistance_mm");
  MakeMemberTunable(_iConfig.pounceSuccessPitchDiff_deg, "pounceSuccessPitchDiff_deg");
  MakeMemberTunable(_iConfig.pointsToWin, "pointsToWin");
}

bool BehaviorKeepaway::WantsToBeActivatedBehavior() const 
{
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
  _dVars.victorGotLastPoint = false;
  float startingPitch = GetBEI().GetRobotInfo().GetPitchAngle().getDegrees();
  _dVars.pounceSuccessPitch_deg = startingPitch + _iConfig.pounceSuccessPitchDiff_deg;
  _dVars.pounceChance = _iConfig.basePounceChance;
  SET_STATE(KeepawayState::Pouncing);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::CubePouncePounceNormal), &BehaviorKeepaway::TransitionToReacting);
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
  // Use ClosestMarker for distance checks
  const ObservableObject* targetObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  Pose3d closestMarkerPose;
  targetObject->GetClosestMarkerPose(GetBEI().GetRobotInfo().GetPose(), true, closestMarkerPose);
  _dVars.targetDistance = Point2f(closestMarkerPose.GetTranslation()).Length();

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
  Pose3d throwaway;
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