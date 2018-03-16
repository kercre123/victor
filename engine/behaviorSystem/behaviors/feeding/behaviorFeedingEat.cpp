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

#include "engine/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/animations/animationContainers/cannedAnimationContainer.h"
#include "engine/behaviorSystem/behaviorListenerInterfaces/iFeedingListener.h"
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
  {ReactionTrigger::RobotFalling,                 true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           true},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kDisableCliffWhileEating),
              "Reaction triggers duplicate or non-sequential");
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingEat::BehaviorFeedingEat(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _timeCubeIsSuccessfullyDrained_sec(FLT_MAX)
, _hasNotifiedNeedsSystemOfDrain(false)
, _currentState(State::DrivingToFood)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsRunnableInternal(const Robot& robot ) const
{
  if(_targetID.IsSet()){
    if( IsCubeBad(robot, _targetID ) ) {
      _targetID.SetToUnknown();
      return false;
    }
    
    const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);

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
Result BehaviorFeedingEat::InitInternal(Robot& robot)
{
  if(robot.GetBlockWorld().GetLocatedObjectByID(_targetID) == nullptr){
    return Result::RESULT_FAIL;
  }
  
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  _hasNotifiedNeedsSystemOfDrain = false;

  // generic lambda closure for cube accel listeners
  auto movementDetectedCallback = [this, &robot] (const float movementScore) {
    CubeMovementHandler(robot, movementScore);
  };
  
  auto listener = std::make_shared<CubeAccelListeners::MovementListener>(kHighPassFiltCoef,
                                                                         kMaxMovementScoreToAdd,
                                                                         kMovementScoreDecay,
                                                                         kFeedingMovementScoreMax, // max allowed movement score
                                                                         movementDetectedCallback);
  robot.GetCubeAccelComponent().AddListener(_targetID, listener);
  DEV_ASSERT(_cubeMovementListener == nullptr,
             "BehaviorFeedingEat.InitInternal.PreviousListenerAlreadySetup");
  // keep a pointer to this listener around so that we can remove it later:
  _cubeMovementListener = listener;
  
  TransitionToDrivingToFood(robot);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorFeedingEat::UpdateInternal(Robot& robot)
{
  if(!_hasNotifiedNeedsSystemOfDrain &&
     DidSuccessfullyDrainCube()){
    _hasNotifiedNeedsSystemOfDrain = true;
    robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(NeedsActionId::Feed);
  }
     
  
  if((_currentState != State::ReactingToInterruption) &&
     (robot.GetOffTreadsState() != OffTreadsState::OnTreads) &&
     !DidSuccessfullyDrainCube()){
    TransitionToReactingToInterruption(robot);
  }
  
  return base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::CubeMovementHandler(Robot& robot, const float movementScore)
{
  // Logic for determining whether the player has "stolen" cozmo's cube while he's
  // eating.  We only want to respond if the player pulls the cube away while
  // Cozmo is actively in the "eating" stage and has not drained the cube yet
  if(robot.IsPhysical()){
    if(movementScore > kCubeMovedTooFastInterrupt){
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const bool currentlyEating = (_currentState == State::Eating) &&
                       (_timeCubeIsSuccessfullyDrained_sec > currentTime_s);
      
      if(currentlyEating ||
         (_currentState == State::PlacingLiftOnCube)){
        StopActing(false);
        TransitionToReactingToInterruption(robot);
      }else if(_currentState == State::DrivingToFood){
        StopActing(false);
      }
      
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::StopInternal(Robot& robot)
{
  // If the behavior is being stopped while feeding is still ongoing notify
  // listeners that feeding is being interrupted
  if(!DidSuccessfullyDrainCube() &&
     (_currentState >= State::PlacingLiftOnCube)){
    for(auto& listener: _feedingListeners){
      listener->EatingInterrupted(robot);
    }
  }
  
  if(DidSuccessfullyDrainCube()){
    for(auto& listener: _feedingListeners){
      listener->EatingComplete(robot);
    }
  }
  
  
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
bool BehaviorFeedingEat::DidSuccessfullyDrainCube()
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return currentTime_s > _timeCubeIsSuccessfullyDrained_sec;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(Robot& robot)
{
  SET_STATE(DrivingToFood);
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj == nullptr){
    return;
  }

  DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(robot,
                                                                          _targetID,
                                                                          kDistanceFromMarker_mm);
  action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(kFeedingPreActionAngleTol_deg));
  
  StartActing(action, [this, &robot](ActionResult result){
    if( result == ActionResult::SUCCESS ){
      TransitionToPlacingLiftOnCube(robot);
    }
    else if( result == ActionResult::VISUAL_OBSERVATION_FAILED ) {
      // can't see the cube, maybe it's obstructed? give up on the cube until we see it again. Let the
      // behavior end (it may get re-selected with a different cube)
      MarkCubeAsBad(robot);
    } else if( result == ActionResult::NO_PREACTION_POSES){
      robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
    } else {
      const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);

      if( resCat == ActionResultCategory::RETRY ) {
        TransitionToDrivingToFood(robot);
      }
      else {
        // something else is wrong. Make this cube invalid, let the behavior end
        MarkCubeAsBad(robot);
      }
    }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToPlacingLiftOnCube(Robot& robot)
{
  SET_STATE(PlacingLiftOnCube);
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy,
                                                          NeedBracketId::Critical);
  
  AnimationTrigger bestAnim = isNeedSevere ?
                               AnimationTrigger::FeedingPlaceLiftOnCube_Severe :
                               AnimationTrigger::FeedingPlaceLiftOnCube_Normal;
  
  StartActing(new TriggerAnimationAction(robot, bestAnim),
              &BehaviorFeedingEat::TransitionToEating);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToEating(Robot& robot)
{
  SET_STATE(Eating);
  // Disable reactions and StopOnCliff so that when cozmo lifts himself up on
  // the cube during an animation the animation isn't interrupted
  SmartDisableReactionsWithLock(GetIDStr(), kDisableCliffWhileEating);
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));
  
  AnimationTrigger eatingAnimTrigger = CheckNeedsStateAndCalculateAnimation(robot);
  std::string eatingAnimName;
  
  float timeDrainCube_s = 0;
  RobotManager* robot_mgr = robot.GetContext()->GetRobotManager();
  if( robot_mgr->HasAnimationForTrigger(eatingAnimTrigger) )
  {
    // Extract the length of time that the animation will be playing for so that
    // it can be passed through to listeners
    const auto& animStreamer = robot.GetAnimationStreamer();
    const auto& cannedAnims = robot.GetContext()->GetRobotManager()->GetCannedAnimations();
    
    const auto& animGroupName = robot_mgr->GetAnimationForTrigger(eatingAnimTrigger);
    eatingAnimName = animStreamer.GetAnimationNameFromGroup(animGroupName, robot);
    const Animation* eatingAnimRawPointer = cannedAnims.GetAnimation(eatingAnimName);
    const auto& track = eatingAnimRawPointer->GetTrack<EventKeyFrame>();
    if(!track.IsEmpty()){
      // assumes only one keyframe per eating anim
      timeDrainCube_s = track.GetLastKeyFrame()->GetTriggerTime()/1000.0f;
      PRINT_CH_INFO("Behaviors",
                    "BehaviorFeedingEat.TransitionToEating.TimeDrainCube",
                    "For animation named %s time to drain cube is %f seconds",
                    eatingAnimName.c_str(),
                    timeDrainCube_s);
    }else{
      PRINT_NAMED_ERROR("BehaviorFeedingEat.TransitionToEating.NoEventKeyframeTrak",
                        "Track not found for event keyframes on anim %s",
                        eatingAnimName.c_str());
    }
  }else{
    DEV_ASSERT(false,
               "BehaviorFeedingEat.TransitionToEating.NoAnimForTrigger");
    return;
  }
  
  
  for(auto & listener: _feedingListeners){
    if(ANKI_VERIFY(listener != nullptr,
                   "BehaviorFeedingEat.TransitionToEating.ListenerIsNull",
                   "")) {
      listener->StartedEating(robot, timeDrainCube_s);
    }
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeCubeIsSuccessfullyDrained_sec = currentTime_s + timeDrainCube_s;
  
  StartActing(new PlayAnimationAction(robot, eatingAnimName));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToReactingToInterruption(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool currentlyEating = (_currentState == State::Eating) &&
                 (_timeCubeIsSuccessfullyDrained_sec > currentTime_s);
  
  if(currentlyEating ||
     (_currentState == State::PlacingLiftOnCube)){
    for(auto& listener: _feedingListeners){
      listener->EatingInterrupted(robot);
    }
  }
  
  SET_STATE(ReactingToInterruption);
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  
  StopActing(false);
  AnimationTrigger trigger = AnimationTrigger::FeedingInterrupted;
  
  if(NeedId::Energy == robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression()){
    trigger = AnimationTrigger::FeedingInterrupted_Severe;
  }
  StartActing(new TriggerLiftSafeAnimationAction(robot, trigger));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorFeedingEat::CheckNeedsStateAndCalculateAnimation(Robot& robot)
{
  // Eating animation is dependent on both the current and post feeding energy level
  // Use PredictNeedsActionResult to estimate the ending needs bracket
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  
  const bool isSeverePreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  const bool isWarningPreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning);
  
  NeedsState predPostFeedNeed;
  robot.GetContext()->GetNeedsManager()->PredictNeedsActionResult(NeedsActionId::Feed,
                                                                  predPostFeedNeed);
  
  const bool isSeverePostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                                    NeedBracketId::Critical);
  const bool isWarningPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                                     NeedBracketId::Warning);
  const bool isFullPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                                  NeedBracketId::Full);
  
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
void BehaviorFeedingEat::MarkCubeAsBad(const Robot& robot)
{
  if( ! ANKI_VERIFY(_targetID.IsSet(), "BehaviorFeedingEat.MarkCubeAsBad.NoTargetID",
                    "Behavior %s trying to mark target cube as bad, but target is unset",
                    GetIDStr().c_str()) ) {
    return;
  }

  const TimeStamp_t lastPoseUpdateTime_ms = robot.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(_targetID);
  _badCubesMap[_targetID] = lastPoseUpdateTime_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsCubeBad(const Robot& robot, const ObjectID& objectID) const
{
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

