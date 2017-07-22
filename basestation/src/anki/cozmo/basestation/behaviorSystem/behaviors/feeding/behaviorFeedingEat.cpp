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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/animations/animationContainers/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iFeedingListener.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeAccelComponent.h"
#include "anki/cozmo/basestation/components/cubeAccelComponentListeners.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {
  
namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)
  
CONSOLE_VAR(f32, kDistanceFromMarker_mm, "Behavior.FeedingEat",  45.0f);
  
// Constants for the CubeAccelComponent MovementListener:
const float kHighPassFiltCoef = 0.8f;
const float kMaxMovementScoreToAdd = 10.f;
const float kMovementScoreDecay = 3.f;
const float kFeedingMovementScoreMax = 50.f;
CONSOLE_VAR(f32, kCubeMovedTooFastInterrupt, "Behavior.FeedingEat",  45.0f);

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
BehaviorFeedingEat::BehaviorFeedingEat(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _timeCubeIsSuccessfullyDrained_sec(FLT_MAX)
, _hasRegisteredActionComplete(false)
, _currentState(State::DrivingToFood)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingEat::IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData ) const
{
  if(ANKI_VERIFY(preReqData.GetTargets().size() == 1,
                 "BehaviorFeedingEat.PassedInInvalidNumberOfTargets",
                 "Passed in %zu targets",
                 preReqData.GetTargets().size())){
    _targetID = *preReqData.GetTargets().begin();
    const Robot& robot = preReqData.GetRobot();
    return robot.GetBlockWorld().GetLocatedObjectByID(_targetID) != nullptr;
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
  _hasRegisteredActionComplete = false;

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
    robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(NeedsActionId::Feed);
  }

  
  if((_currentState != State::ReactingToInterruption) &&
     (robot.GetOffTreadsState() != OffTreadsState::OnTreads) &&
     !_hasRegisteredActionComplete){
    TransitionToReactingToInterruption(robot);
  }
  
  return base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::CubeMovementHandler(Robot& robot, const float movementScore)
{
  PRINT_NAMED_WARNING("SEARCH_FOR_ME","movement: %f", movementScore);
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Logic for determining whether the player has "stolen" cozmo's cube while he's
  // eating.  We only want to respond if the player pulls the cube away while
  // Cozmo is actively in the "eating" stage and has not drained the cube yet
  if(robot.IsPhysical()){
    if(movementScore > kCubeMovedTooFastInterrupt){
      if((_currentState == State::Eating) &&
         (_timeCubeIsSuccessfullyDrained_sec > currentTime_s)){
        StopActing(false);
        TransitionToReactingToInterruption(robot);
        for(auto& listener: _feedingListeners){
          listener->EatingInterrupted(robot);
        }
      }else if(_currentState == State::DrivingToFood){
        StopActing(false);
        //TransitionToWaitingForUserToMoveCube(robot);
      }
      
    }else if((movementScore < kCubeMovedTooFastInterrupt) &&
             (_currentState == State::WaitForUserToMoveCube)){
      StopActing(false);
      TransitionToDrivingToFood(robot);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::StopInternal(Robot& robot)
{
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(true)));
  
  robot.GetCubeAccelComponent().RemoveListener(_targetID, _cubeMovementListener);
  _cubeMovementListener.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(Robot& robot)
{
  SET_STATE(DrivingToFood);
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj == nullptr){
    return;
  }
  
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy,
                                                          NeedBracketId::Critical);
  
  AnimationTrigger bestAnim = isNeedSevere ?
                      AnimationTrigger::FeedingPlaceLiftOnCube_Severe :
                      AnimationTrigger::FeedingPlaceLiftOnCube_Normal;
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new DriveToAlignWithObjectAction(robot, _targetID, kDistanceFromMarker_mm));
  action->AddAction(new TriggerAnimationAction(robot, bestAnim));
  StartActing(action, [this, &robot](ActionResult result){
    if(result == ActionResult::SUCCESS){
      TransitionToEating(robot);
    }else{
      TransitionToDrivingToFood(robot);
    }
  });
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
  
  AnimationTrigger eatingAnim = CheckNeedsStateAndCalculateAnimation(robot);
  
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
      listener->StartedEating(robot, timeDrainCube_s);
    }
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeCubeIsSuccessfullyDrained_sec = currentTime_s + timeDrainCube_s;
  
  StartActing(new TriggerAnimationAction(robot, eatingAnim));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToReactingToInterruption(Robot& robot)
{
  SET_STATE(ReactingToInterruption);
  _timeCubeIsSuccessfullyDrained_sec = FLT_MAX;
  
  StopActing(false);
  AnimationTrigger trigger = AnimationTrigger::FeedingInterrupted;
  
  if(NeedId::Energy == robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression()){
    trigger = AnimationTrigger::FeedingInterrupted_Severe;
  }
  StartActing(new TriggerLiftSafeAnimationAction(robot, trigger));
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToWaitingForUserToMoveCube(Robot& robot)
{
  // The user is trying to help cozmo - wait for the movement score to
  // fall below the movement threshold before trying to drive again
  SET_STATE(WaitForUserToMoveCube);
  StartActing(new WaitAction(robot, 1.f),
              &BehaviorFeedingEat::TransitionToWaitingForUserToMoveCube);
  
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
void BehaviorFeedingEat::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}


  
} // namespace Cozmo
} // namespace Anki

