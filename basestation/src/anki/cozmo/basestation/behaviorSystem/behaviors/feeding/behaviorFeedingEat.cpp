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
#include "anki/cozmo/basestation/animationContainers/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iFeedingListener.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {
  
namespace{
CONSOLE_VAR(f32, kDistanceFromMarker_mm, "Behavior.FeedingEat",  30.0f);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kDisableCliffWhileEating = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
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
, _shouldSendUpdateUI(false)
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
  
  _shouldSendUpdateUI = false;
  TransitionToDrivingToFood(robot);
  return Result::RESULT_OK;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::StopInternal(Robot& robot)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(Robot& robot)
{
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
  SmartDisableReactionsWithLock(GetIDStr(), kDisableCliffWhileEating);
  
  AnimationTrigger eatingAnim = UpdateNeedsStateAndCalculateAnimation(robot);
  
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
  
  StartActing(new TriggerAnimationAction(robot, eatingAnim),[this, &robot](ActionResult res){
    if(res == ActionResult::SUCCESS){
      // Update
      robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(NeedsActionId::FeedBlue);
    }
  });
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorFeedingEat::UpdateNeedsStateAndCalculateAnimation(Robot& robot)
{

  
  // Eating animation is dependent on both the current and post feeding energy level
  // Use PredictNeedsActionResult to estimate the ending needs bracket
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  
  const bool isSeverePreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  const bool isWarningPreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning);
  
  NeedsState predPostFeedNeed;
  robot.GetContext()->GetNeedsManager()->PredictNeedsActionResult(NeedsActionId::FeedBlue,
                                                                  predPostFeedNeed);
  
  const bool isSeverePostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                                    NeedBracketId::Critical);
  const bool isWarningPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                                     NeedBracketId::Warning);
  const bool isNotFullPostFeeding = predPostFeedNeed.IsNeedAtBracket(NeedId::Energy,
                                                                     NeedBracketId::Full);
  
  AnimationTrigger bestAnimation;
  if(isSeverePreFeeding && isSeverePostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Severe;
  }else if(isSeverePreFeeding && isWarningPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Severe;
  }else if(isWarningPreFeeding && isNotFullPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Normal;
  }else{
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Normal;
  }
  
  _shouldSendUpdateUI = true;
  return bestAnimation;
}

  
} // namespace Cozmo
} // namespace Anki

