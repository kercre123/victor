/**
* File: behaviorFeedingEat.cpp
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to interact with an "energy" filled cube
* and drain the energy out of it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/animations/animationContainers/cannedAnimationContainer.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubeAccelComponent.h"
#include "engine/components/cubeAccelComponentListeners.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {
  
namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

#define CONSOLE_GROUP "Behavior.FeedingEat"
  
CONSOLE_VAR(f32, kDistanceFromMarker_mm, CONSOLE_GROUP,  45.0f);
  
// Constants for the CubeAccelComponent MovementListener:
CONSOLE_VAR(f32, kHighPassFiltCoef,          CONSOLE_GROUP,  0.4f);
CONSOLE_VAR(f32, kMaxMovementScoreToAdd,     CONSOLE_GROUP,  3.f);
CONSOLE_VAR(f32, kMovementScoreDecay,        CONSOLE_GROUP,  2.f);
CONSOLE_VAR(f32, kFeedingMovementScoreMax,   CONSOLE_GROUP,  100.f);
CONSOLE_VAR(f32, kCubeMovedTooFastInterrupt, CONSOLE_GROUP,  8.f);

CONSOLE_VAR(f32, kFeedingPreActionAngleTol_deg, CONSOLE_GROUP, 15.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Disable reactions that interrupt Cozmo's eating animation when he lifts himself
// up on top of cube
constexpr ReactionTriggerHelpers::FullReactionArray kDisableCliffWhileEating = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           true},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kDisableCliffWhileEating),
              "Reaction triggers duplicate or non-sequential");
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingEat::BehaviorFeedingEat(const Json::Value& config)
: ICozmoBehavior(config)
, _timeCubeIsSuccessfullyDrained_sec(FLT_MAX)
, _hasRegisteredActionComplete(false)
, _currentState(State::DrivingToFood)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(_targetID.IsSet()){
    if( IsCubeBad(behaviorExternalInterface, _targetID ) ) {
      _targetID.SetToUnknown();
      return false;
    }
    
    const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);

    // require a known object so we don't drive to and try to eat a moved cube
    const bool canRun = (obj != nullptr) && obj->IsPoseStateKnown();
    if(!canRun){
      _targetID.SetToUnknown();
    }
    return canRun;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorFeedingEat::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID) == nullptr){
    return Result::RESULT_FAIL;
  }
  
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  _hasRegisteredActionComplete = false;

  // generic lambda closure for cube accel listeners
  auto movementDetectedCallback = [this, &behaviorExternalInterface] (const float movementScore) {
    CubeMovementHandler(behaviorExternalInterface, movementScore);
  };
  
  auto listener = std::make_shared<CubeAccelListeners::MovementListener>(kHighPassFiltCoef,
                                                                         kMaxMovementScoreToAdd,
                                                                         kMovementScoreDecay,
                                                                         kFeedingMovementScoreMax, // max allowed movement score
                                                                         movementDetectedCallback);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  robot.GetCubeAccelComponent().AddListener(_targetID, listener);
  DEV_ASSERT(_cubeMovementListener == nullptr,
             "BehaviorFeedingEat.InitInternal.PreviousListenerAlreadySetup");
  // keep a pointer to this listener around so that we can remove it later:
  _cubeMovementListener = listener;
  
  TransitionToDrivingToFood(behaviorExternalInterface);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorFeedingEat::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Feeding should be considered "complete" so long as the animation has reached
  // the point where all light has been drained from the cube.  If the behavior
  // is interrupted after that point in the animation or the animation completes
  // successfully, register the action as complete.  If it's interrupted before
  // reaching that time (indicated by _timeCubeIsSuccessfullyDrained_sec) then
  // Cozmo didn't successfully finish "eating" and doesn't get the energy for it
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(!_hasRegisteredActionComplete &&
     (currentTime_s > _timeCubeIsSuccessfullyDrained_sec)){
    _hasRegisteredActionComplete = true;
    auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
    if(needsManager != nullptr){
      needsManager->RegisterNeedsActionCompleted(NeedsActionId::Feed);
    }
    for(auto& listener: _feedingListeners){
      listener->EatingComplete(behaviorExternalInterface);
    }
  }

  
  if((_currentState != State::ReactingToInterruption) &&
     (behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::OnTreads) &&
     !_hasRegisteredActionComplete){
    TransitionToReactingToInterruption(behaviorExternalInterface);
  }
  
  return base::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::CubeMovementHandler(BehaviorExternalInterface& behaviorExternalInterface, const float movementScore)
{
  // Logic for determining whether the player has "stolen" cozmo's cube while he's
  // eating.  We only want to respond if the player pulls the cube away while
  // Cozmo is actively in the "eating" stage and has not drained the cube yet
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if(robot.IsPhysical()){
    if(movementScore > kCubeMovedTooFastInterrupt){
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const bool currentlyEating = (_currentState == State::Eating) &&
                       (_timeCubeIsSuccessfullyDrained_sec > currentTime_s);
      
      if(currentlyEating ||
         (_currentState == State::PlacingLiftOnCube)){
        StopActing(false);
        TransitionToReactingToInterruption(behaviorExternalInterface);
      }else if(_currentState == State::DrivingToFood){
        StopActing(false);
      }
      
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // If the behavior is being stopped while feeding is still ongoing notify
  // listeners that feeding is being interrupted
  if(!_hasRegisteredActionComplete &&
     (_currentState >= State::PlacingLiftOnCube)){
    for(auto& listener: _feedingListeners){
      listener->EatingInterrupted(behaviorExternalInterface);
    }
  }

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(true)));
  
  const bool removeSuccessfull = robot.GetCubeAccelComponent().RemoveListener(_targetID, _cubeMovementListener);
  ANKI_VERIFY(removeSuccessfull,
             "BehaviorFeedingEat.StopInternal.FailedToRemoveAccellComponent",
              "");
  _cubeMovementListener.reset();
  _targetID.UnSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(DrivingToFood);
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj == nullptr){
    return;
  }

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(robot,
                                                                          _targetID,
                                                                          kDistanceFromMarker_mm);
  action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(kFeedingPreActionAngleTol_deg));
  
  DelegateIfInControl(action, [this, &behaviorExternalInterface](ActionResult result){
    if( result == ActionResult::SUCCESS ){
      TransitionToPlacingLiftOnCube(behaviorExternalInterface);
    }
    else if( result == ActionResult::VISUAL_OBSERVATION_FAILED ) {
      // can't see the cube, maybe it's obstructed? give up on the cube until we see it again. Let the
      // behavior end (it may get re-selected with a different cube)
      MarkCubeAsBad(behaviorExternalInterface);
    } else if( result == ActionResult::NO_PREACTION_POSES){
      behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
    } else {
      const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);

      if( resCat == ActionResultCategory::RETRY ) {
        TransitionToDrivingToFood(behaviorExternalInterface);
      }
      else {
        // something else is wrong. Make this cube invalid, let the behavior end
        MarkCubeAsBad(behaviorExternalInterface);
      }
    }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToPlacingLiftOnCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(PlacingLiftOnCube);
  
  bool isNeedSevere = false;
  auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
  if(needsManager != nullptr){
    NeedsState& currNeedState = needsManager->GetCurNeedsStateMutable();
    isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy,
                                                 NeedBracketId::Critical);
  }
  
  AnimationTrigger bestAnim = isNeedSevere ?
                               AnimationTrigger::FeedingPlaceLiftOnCube_Severe :
                               AnimationTrigger::FeedingPlaceLiftOnCube_Normal;
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  DelegateIfInControl(new TriggerAnimationAction(robot, bestAnim),
              &BehaviorFeedingEat::TransitionToEating);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(Eating);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Disable reactions and StopOnCliff so that when cozmo lifts himself up on
  // the cube during an animation the animation isn't interrupted
  SmartDisableReactionsWithLock(GetIDStr(), kDisableCliffWhileEating);
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  
  AnimationTrigger eatingAnim = CheckNeedsStateAndCalculateAnimation(behaviorExternalInterface);
  
  uint32_t timeDrainCube_s = 0;
  RobotManager* robot_mgr = robot.GetContext()->GetRobotManager();
  if( robot_mgr->HasAnimationForTrigger(eatingAnim) )
  {
    // Extract the length of time that the animation will be playing for so that
    // it can be passed through to listeners
    const auto& animStreamer = robot.GetAnimationStreamer();
    const auto& cannedAnims = robot.GetContext()->GetRobotManager()->GetCannedAnimations();
    
    const auto& animGroupName = robot_mgr->GetAnimationForTrigger(eatingAnim);
    const auto& animName = animStreamer.GetAnimationNameFromGroup(animGroupName, robot);
    const Animation* eatingAnimRawPointer = cannedAnims.GetAnimation(animName);
    const auto& track = eatingAnimRawPointer->GetTrack<EventKeyFrame>();
    if(!track.IsEmpty()){
      // assumes only one keyframe per eating anim
      timeDrainCube_s = track.GetLastKeyFrame()->GetTriggerTime()/1000;
    }
  }
  
  
  for(auto & listener: _feedingListeners){
    if(ANKI_VERIFY(listener != nullptr,
                   "BehaviorFeedingEat.TransitionToEating.ListenerIsNull",
                   "")) {
      listener->StartedEating(behaviorExternalInterface, timeDrainCube_s);
    }
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeCubeIsSuccessfullyDrained_sec = currentTime_s + timeDrainCube_s;
  
  DelegateIfInControl(new TriggerAnimationAction(robot, eatingAnim));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToReactingToInterruption(BehaviorExternalInterface& behaviorExternalInterface)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool currentlyEating = (_currentState == State::Eating) &&
                 (_timeCubeIsSuccessfullyDrained_sec > currentTime_s);
  
  if(currentlyEating ||
     (_currentState == State::PlacingLiftOnCube)){
    for(auto& listener: _feedingListeners){
      listener->EatingInterrupted(behaviorExternalInterface);
    }
  }
  
  SET_STATE(ReactingToInterruption);
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  
  StopActing(false);
  AnimationTrigger trigger = AnimationTrigger::FeedingInterrupted;
  
  if(NeedId::Energy == behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression()){
    trigger = AnimationTrigger::FeedingInterrupted_Severe;
  }
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(robot, trigger));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorFeedingEat::CheckNeedsStateAndCalculateAnimation(BehaviorExternalInterface& behaviorExternalInterface)
{
  bool isSeverePreFeeding  = false;
  bool isWarningPreFeeding = false;
  bool isSeverePostFeeding = false;
  bool isWarningPostFeeding = false;
  bool isFullPostFeeding = false;
  // Eating animation is dependent on both the current and post feeding energy level
  // Use PredictNeedsActionResult to estimate the ending needs bracket
  auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
  if(needsManager != nullptr){
    NeedsState& currNeedState = needsManager->GetCurNeedsStateMutable();
    
    isSeverePreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
    isWarningPreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning);
    
    NeedsState predPostFeedNeed;
    needsManager->PredictNeedsActionResult(NeedsActionId::Feed, predPostFeedNeed);
    
    isSeverePostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                           NeedBracketId::Critical);
    isWarningPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                            NeedBracketId::Warning);
    isFullPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                         NeedBracketId::Full);
  }
  AnimationTrigger bestAnimation;
  if(isSeverePreFeeding && isSeverePostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Severe;
  }else if(isSeverePreFeeding && isWarningPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Severe;
  }else if(isWarningPreFeeding && !isFullPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Normal;
  }else{
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Normal;
  }
  
  PRINT_CH_INFO("Feeding",
                "BehaviorFeedingEat.UpdateNeedsStateCalcAnim.AnimationSelected",
                "AnimationTrigger: %s SeverePreFeeding: %d severePostFeeding: %d warningPreFeeding: %d fullyFullPost: %d ",
                AnimationTriggerToString(bestAnimation),
                isSeverePreFeeding, isSeverePostFeeding,
                isWarningPreFeeding, isFullPostFeeding);
  
  return bestAnimation;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::MarkCubeAsBad(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( ! ANKI_VERIFY(_targetID.IsSet(), "BehaviorFeedingEat.MarkCubeAsBad.NoTargetID",
                    "Behavior %s trying to mark target cube as bad, but target is unset",
                    GetIDStr().c_str()) ) {
    return;
  }
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  const TimeStamp_t lastPoseUpdateTime_ms = robot.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(_targetID);
  _badCubesMap[_targetID] = lastPoseUpdateTime_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsCubeBad(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& objectID) const
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  const TimeStamp_t lastPoseUpdateTime_ms = robot.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(objectID);

  auto iter = _badCubesMap.find( objectID );
  if( iter != _badCubesMap.end() ) {
    if( lastPoseUpdateTime_ms <= iter->second ) {
      // cube hasn't been re-observed, so is bad (shouldn't be used by the behavior
      return true;
    }
    // otherwise, the cube was invalid, but has a new pose, so consider it OK
  }

  // cube isn't bad
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}


  
} // namespace Cozmo
} // namespace Anki

